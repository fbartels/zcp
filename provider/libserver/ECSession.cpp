/*
 * Copyright 2005 - 2015  Zarafa B.V. and its licensors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

//
//////////////////////////////////////////////////////////////////////

#include <zarafa/platform.h>
#include <new>

#ifdef DEBUG
#ifdef WIN32
#pragma warning(disable: 4786)
#endif
#endif

#ifdef LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h>
#endif

#include <mapidefs.h>
#include <mapitags.h>

#include "ECSession.h"
#include "ECSessionManager.h"
#include "ECUserManagement.h"
#include "ECUserManagementOffline.h"
#include "ECSecurity.h"
#include "ECSecurityOffline.h"
#include "ECPluginFactory.h"
#include <zarafa/base64.h>
#include "SSLUtil.h"
#include <zarafa/stringutil.h>

#include "ECDatabaseMySQL.h"
#include "ECDatabaseUtils.h" // used for PR_INSTANCE_KEY
#include "SOAPUtils.h"
#include "ZarafaICS.h"
#include "ECICS.h"
#include <zarafa/ECIConv.h>
#include "ZarafaVersions.h"

#include "pthreadutil.h"
#include <zarafa/threadutil.h>
#include <zarafa/boost_compat.h>

#include <boost/filesystem.hpp>
namespace bfs = boost::filesystem;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

#if defined LINUX || !defined UNICODE
#define WHITESPACE " \t\n\r"
#else
#define WHITESPACE L" \t\n\r"
#endif

// possible missing ssl function
#ifndef HAVE_EVP_PKEY_CMP
static int EVP_PKEY_cmp(EVP_PKEY *a, EVP_PKEY *b)
    {
    if (a->type != b->type)
        return -1;

    if (EVP_PKEY_cmp_parameters(a, b) == 0)
        return 0;

    switch (a->type)
        {
    case EVP_PKEY_RSA:
        if (BN_cmp(b->pkey.rsa->n,a->pkey.rsa->n) != 0
            || BN_cmp(b->pkey.rsa->e,a->pkey.rsa->e) != 0)
            return 0;
        break;
    case EVP_PKEY_DSA:
        if (BN_cmp(b->pkey.dsa->pub_key,a->pkey.dsa->pub_key) != 0)
            return 0;
        break;
    case EVP_PKEY_DH:
        return -2;
    default:
        return -2;
        }

    return 1;
    }
#endif

void CreateSessionID(unsigned int ulCapabilities, ECSESSIONID *lpSessionId)
{
	ssl_random(!!(ulCapabilities & ZARAFA_CAP_LARGE_SESSIONID), lpSessionId);
}

/*
  BaseType session
*/
BTSession::BTSession(const char *src_addr, ECSESSIONID sessionID,
    ECDatabaseFactory *lpDatabaseFactory, ECSessionManager *lpSessionManager,
    unsigned int ulCapabilities) :
	m_strSourceAddr(src_addr), m_sessionID(sessionID),
	m_lpDatabaseFactory(lpDatabaseFactory),
	m_lpSessionManager(lpSessionManager),
	m_ulClientCapabilities(ulCapabilities)
{
	m_ulRefCount = 0;
	m_sessionTime = GetProcessTime();

	m_ulSessionTimeout = 300;
	m_bCheckIP = true;

	m_lpUserManagement = NULL;
	m_ulRequests = 0;

	m_ulLastRequestPort = 0;

	// Protects the object from deleting while a thread is running on a method in this object
	pthread_cond_init(&m_hThreadReleased, NULL);
	pthread_mutex_init(&m_hThreadReleasedMutex, NULL);

	pthread_mutex_init(&m_hRequestStats, NULL);
}

BTSession::~BTSession() {
	// derived destructor still uses these vars
	pthread_cond_destroy(&m_hThreadReleased);
	pthread_mutex_destroy(&m_hThreadReleasedMutex);
	pthread_mutex_destroy(&m_hRequestStats);
}

void BTSession::SetClientMeta(const char *const lpstrClientVersion, const char *const lpstrClientMisc)
{
	m_strClientApplicationVersion = lpstrClientVersion ? lpstrClientVersion : "";
	m_strClientApplicationMisc = lpstrClientMisc ? lpstrClientMisc : "";
}

void BTSession::GetClientApplicationVersion(std::string *lpstrClientApplicationVersion)
{
        lpstrClientApplicationVersion->assign(m_strClientApplicationVersion);
}

void BTSession::GetClientApplicationMisc(std::string *lpstrClientApplicationMisc)
{
	scoped_lock lock(m_hRequestStats);
        lpstrClientApplicationMisc->assign(m_strClientApplicationMisc);
}

ECRESULT BTSession::Shutdown(unsigned int ulTimeout) {
	return erSuccess;
}

ECRESULT BTSession::ValidateOriginator(struct soap *soap)
{
	if (!m_bCheckIP)
		return erSuccess;
	const char *s = ::GetSourceAddr(soap);
	if (strcmp(m_strSourceAddr.c_str(), s) == 0)
		return erSuccess;
	ec_log_err("Denying access to session from source \"%s\" due to unmatched establishing source \"%s\"",
		s, m_strSourceAddr.c_str());
	return ZARAFA_E_END_OF_SESSION;
}

void BTSession::UpdateSessionTime()
{
	m_sessionTime = GetProcessTime();
}

ECRESULT BTSession::GetDatabase(ECDatabase **lppDatabase)
{
	return GetThreadLocalDatabase(this->m_lpDatabaseFactory, lppDatabase);
}

ECRESULT BTSession::GetAdditionalDatabase(ECDatabase **lppDatabase)
{
	std::string str;
	return this->m_lpDatabaseFactory->CreateDatabaseObject(lppDatabase, str);
}


ECRESULT BTSession::GetServerGUID(GUID* lpServerGuid){
	return 	m_lpSessionManager->GetServerGUID(lpServerGuid);
}

ECRESULT BTSession::GetNewSourceKey(SOURCEKEY* lpSourceKey){
	return m_lpSessionManager->GetNewSourceKey(lpSourceKey);
}

void BTSession::Lock()
{
	// Increase our refcount by one
	pthread_mutex_lock(&m_hThreadReleasedMutex);
	++this->m_ulRefCount;
	pthread_mutex_unlock(&m_hThreadReleasedMutex);
}

void BTSession::Unlock()
{
	// Decrease our refcount by one, signal ThreadReleased if RefCount == 0
	pthread_mutex_lock(&m_hThreadReleasedMutex);
	--this->m_ulRefCount;
	if(!IsLocked())
		pthread_cond_signal(&m_hThreadReleased);
	pthread_mutex_unlock(&m_hThreadReleasedMutex);
}

time_t BTSession::GetIdleTime()
{
	return GetProcessTime() - m_sessionTime;
}

void BTSession::RecordRequest(struct soap* soap)
{
	scoped_lock lock(m_hRequestStats);
	m_strLastRequestURL = soap->endpoint;
	m_ulLastRequestPort = soap->port;
	if (soap->proxy_from && ((SOAPINFO*)soap->user)->bProxy)
		m_strProxyHost = soap->host;
	++m_ulRequests;
}

unsigned int BTSession::GetRequests()
{
	scoped_lock lock(m_hRequestStats);
    return m_ulRequests;
}

void BTSession::GetRequestURL(std::string *lpstrClientURL)
{
	scoped_lock lock(m_hRequestStats);
	lpstrClientURL->assign(m_strLastRequestURL);
}

void BTSession::GetProxyHost(std::string *lpstrProxyHost)
{
	scoped_lock lock(m_hRequestStats);
	lpstrProxyHost->assign(m_strProxyHost);
}

void BTSession::GetClientPort(unsigned int *lpulPort)
{
	scoped_lock lock(m_hRequestStats);
	*lpulPort = m_ulLastRequestPort;
}

size_t BTSession::GetInternalObjectSize()
{
	scoped_lock lock(m_hRequestStats);
	return MEMORY_USAGE_STRING(m_strSourceAddr) +
			MEMORY_USAGE_STRING(m_strLastRequestURL) +
			MEMORY_USAGE_STRING(m_strProxyHost);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ECSession::ECSession(const char *src_addr, ECSESSIONID sessionID,
    ECSESSIONGROUPID ecSessionGroupId, ECDatabaseFactory *lpDatabaseFactory,
    ECSessionManager *lpSessionManager, unsigned int ulCapabilities,
    bool bIsOffline, AUTHMETHOD ulAuthMethod, int pid,
    const std::string &cl_ver, const std::string &cl_app,
    const std::string &cl_app_ver, const std::string &cl_app_misc) :
	BTSession(src_addr, sessionID, lpDatabaseFactory, lpSessionManager,
	    ulCapabilities)
{
	m_lpTableManager		= new ECTableManager(this);
	m_lpEcSecurity			= NULL;
	m_dblUser				= 0;
	m_dblSystem				= 0;
	m_dblReal				= 0;
	m_ulAuthMethod			= ulAuthMethod;
	m_ulConnectingPid		= pid;
	m_ecSessionGroupId		= ecSessionGroupId;
	m_strClientVersion		= cl_ver;
	m_ulClientVersion		= ZARAFA_VERSION_UNKNOWN;
	m_strClientApp			= cl_app;
	m_strClientApplicationVersion   = cl_app_ver;
	m_strClientApplicationMisc	= cl_app_misc;

	ParseZarafaVersion(cl_ver, &m_ulClientVersion);
	// Ignore result.

	m_ulSessionTimeout = atoi(lpSessionManager->GetConfig()->GetSetting("session_timeout"));
	if (m_ulSessionTimeout < 300)
		m_ulSessionTimeout = 300;

	m_bCheckIP = strcmp(lpSessionManager->GetConfig()->GetSetting("session_ip_check"), "no") != 0;

	// Offline implements its own versions of these objects
	if (bIsOffline == false) {
		m_lpUserManagement = new ECUserManagement(this, m_lpSessionManager->GetPluginFactory(), m_lpSessionManager->GetConfig());
		m_lpEcSecurity = new ECSecurity(this, m_lpSessionManager->GetConfig(), m_lpSessionManager->GetAudit());
	} else {
		m_lpUserManagement = new ECUserManagementOffline(this, m_lpSessionManager->GetPluginFactory(), m_lpSessionManager->GetConfig());

		m_lpEcSecurity = new ECSecurityOffline(this, m_lpSessionManager->GetConfig());
	}

	// Atomically get and AddSession() on the sessiongroup. Needs a ReleaseSession() on the session group to clean up.
	m_lpSessionManager->GetSessionGroup(ecSessionGroupId, this, &m_lpSessionGroup);

	pthread_mutex_init(&m_hStateLock, NULL);
	pthread_mutex_init(&m_hLocksLock, NULL);
}


ECSession::~ECSession()
{
	Shutdown(0);

	/*
	 * Release our reference to the session group; none of the threads of this session are
	 * using the object since there are now 0 threads on this session (except this thread)
	 * Afterwards tell the session manager that the sessiongroup may be an orphan now.
	 */
	if (m_lpSessionGroup) {
		m_lpSessionGroup->ReleaseSession(this);
    	m_lpSessionManager->DeleteIfOrphaned(m_lpSessionGroup);
	}

	pthread_mutex_destroy(&m_hLocksLock);
	pthread_mutex_destroy(&m_hStateLock);

	delete m_lpTableManager;
	delete m_lpUserManagement;
	delete m_lpEcSecurity;
}

/**
 * Shut down the session:
 *
 * - Signal sessiongroup that long-running requests should be cancelled
 * - Wait for all users of the session to exit
 *
 * If the wait takes longer than ulTimeout milliseconds, ZARAFA_E_TIMEOUT is
 * returned. If this is the case, it is *not* safe to delete the session
 *
 * @param ulTimeout Timeout in milliseconds
 * @result erSuccess or ZARAFA_E_TIMEOUT
 */
ECRESULT ECSession::Shutdown(unsigned int ulTimeout)
{
	ECRESULT er = erSuccess;

	/* Shutdown blocking calls for this session on our session group */
	if (m_lpSessionGroup) {
		m_lpSessionGroup->ShutdownSession(this);
	}

	/* Wait until there are no more running threads using this session */
	pthread_mutex_lock(&m_hThreadReleasedMutex);
	while(IsLocked())
		if(pthread_cond_timedwait(&m_hThreadReleased, &m_hThreadReleasedMutex, ulTimeout) == ETIMEDOUT)
			break;
	pthread_mutex_unlock(&m_hThreadReleasedMutex);

	if(IsLocked()) {
		er = ZARAFA_E_TIMEOUT;
	}

	return er;
}

ECSession::AUTHMETHOD ECSession::GetAuthMethod()
{
    return m_ulAuthMethod;
}

ECSESSIONGROUPID ECSession::GetSessionGroupId() {
    return m_ecSessionGroupId;
}

int ECSession::GetConnectingPid() {
    return m_ulConnectingPid;
}

ECRESULT ECSession::AddAdvise(unsigned int ulConnection, unsigned int ulKey, unsigned int ulEventMask)
{
	ECRESULT		hr = erSuccess;

	Lock();

	if (m_lpSessionGroup)
		hr = m_lpSessionGroup->AddAdvise(m_sessionID, ulConnection, ulKey, ulEventMask);
	else
		hr = ZARAFA_E_NOT_INITIALIZED;

	Unlock();

	return hr;
}

ECRESULT ECSession::AddChangeAdvise(unsigned int ulConnection, notifySyncState *lpSyncState)
{
	ECRESULT		er = erSuccess;
	string			strQuery;
	ECDatabase*		lpDatabase = NULL;
	DB_RESULT		lpDBResult	= NULL;
	DB_ROW			lpDBRow;
	ULONG			ulChangeId = 0;

	Lock();

	if (!m_lpSessionGroup) {
		er = ZARAFA_E_NOT_INITIALIZED;
		goto exit;
	}

	er = m_lpSessionGroup->AddChangeAdvise(m_sessionID, ulConnection, lpSyncState);
	if (er != hrSuccess)
		goto exit;

	er = GetDatabase(&lpDatabase);
	if (er != erSuccess)
		goto exit;

	strQuery =	"SELECT c.id FROM changes AS c JOIN syncs AS s "
					"ON s.sourcekey=c.parentsourcekey "
				"WHERE s.id=" + stringify(lpSyncState->ulSyncId) + " "
					"AND c.id>" + stringify(lpSyncState->ulChangeId) + " "
					"AND c.sourcesync!=" + stringify(lpSyncState->ulSyncId) + " "
					"AND c.change_type >=  " + stringify(ICS_MESSAGE) + " "
					"AND c.change_type & " + stringify(ICS_MESSAGE) + " !=  0 "
				"ORDER BY c.id DESC "
				"LIMIT 1";

	er = lpDatabase->DoSelect(strQuery, &lpDBResult);
	if (er != hrSuccess)
		goto exit;

	if (lpDatabase->GetNumRows(lpDBResult) == 0)
		goto exit;

    lpDBRow = lpDatabase->FetchRow(lpDBResult);
	if (lpDBRow == NULL || lpDBRow[0] == NULL) {
		er = ZARAFA_E_DATABASE_ERROR;
		ec_log_err("ECSession::AddChangeAdvise(): row or column null");
		goto exit;
	}

	ulChangeId = strtoul(lpDBRow[0], NULL, 0);
	er = m_lpSessionGroup->AddChangeNotification(m_sessionID, ulConnection, lpSyncState->ulSyncId, ulChangeId);

exit:
	 if (lpDBResult)
		 lpDatabase->FreeResult(lpDBResult);

	Unlock();

	return er;
}

ECRESULT ECSession::DelAdvise(unsigned int ulConnection)
{
	ECRESULT hr = erSuccess;

	Lock();

	if (m_lpSessionGroup)
		hr = m_lpSessionGroup->DelAdvise(m_sessionID, ulConnection);
	else
		hr = ZARAFA_E_NOT_INITIALIZED;

	Unlock();

	return hr;
}

ECRESULT ECSession::AddNotificationTable(unsigned int ulType, unsigned int ulObjType, unsigned int ulTableId, sObjectTableKey* lpsChildRow, sObjectTableKey* lpsPrevRow, struct propValArray *lpRow)
{
	ECRESULT		hr = hrSuccess;

	Lock();

	if (m_lpSessionGroup)
		hr = m_lpSessionGroup->AddNotificationTable(m_sessionID, ulType, ulObjType, ulTableId, lpsChildRow, lpsPrevRow, lpRow);
	else
		hr = ZARAFA_E_NOT_INITIALIZED;

	Unlock();

	return hr;
}

ECRESULT ECSession::GetNotifyItems(struct soap *soap, struct notifyResponse *notifications)
{
	ECRESULT		hr = erSuccess;

	Lock();

	if (m_lpSessionGroup)
		hr = m_lpSessionGroup->GetNotifyItems(soap, m_sessionID, notifications);
	else
		hr = ZARAFA_E_NOT_INITIALIZED;

	Unlock();

	return hr;
}

ECTableManager* ECSession::GetTableManager()
{
	return m_lpTableManager;
}

ECSecurity* ECSession::GetSecurity()
{
	return m_lpEcSecurity;
}

void ECSession::AddBusyState(pthread_t threadId, const char* lpszState, struct timespec threadstart, double start)
{
	if (!lpszState) {		
		ec_log_err("Invalid argument \"lpszState\" in call to ECSession::AddBusyState()");
	} else {
		pthread_mutex_lock(&m_hStateLock);
		m_mapBusyStates[threadId].fname = lpszState;
		m_mapBusyStates[threadId].threadstart = threadstart;
		m_mapBusyStates[threadId].start = start;
		m_mapBusyStates[threadId].threadid = threadId;
		m_mapBusyStates[threadId].state = SESSION_STATE_PROCESSING;
		pthread_mutex_unlock(&m_hStateLock);
	}
}

void ECSession::UpdateBusyState(pthread_t threadId, int state)
{
	std::map<pthread_t, BUSYSTATE>::iterator i;

	pthread_mutex_lock(&m_hStateLock);

	i = m_mapBusyStates.find(threadId);

	if(i != m_mapBusyStates.end()) {
		i->second.state = state;
	} else {
		ASSERT(FALSE);
	}

	pthread_mutex_unlock(&m_hStateLock);
}

void ECSession::RemoveBusyState(pthread_t threadId)
{
	std::map<pthread_t, BUSYSTATE>::const_iterator i;

	pthread_mutex_lock(&m_hStateLock);

	i = m_mapBusyStates.find(threadId);

	if(i != m_mapBusyStates.end()) {
#ifndef WIN32
		clockid_t clock;
		struct timespec end;

		// Since the specified thread is done now, record how much work it has done for us
		if(pthread_getcpuclockid(threadId, &clock) == 0) {
			clock_gettime(clock, &end);

			AddClocks(timespec2dbl(end) - timespec2dbl(i->second.threadstart), 0, GetTimeOfDay() - i->second.start);
		} else {
			ASSERT(FALSE);
		}
#endif
		m_mapBusyStates.erase(threadId);
	} else {
		ASSERT(FALSE);
	}

	pthread_mutex_unlock(&m_hStateLock);
}

void ECSession::GetBusyStates(std::list<BUSYSTATE> *lpStates)
{
	map<pthread_t, BUSYSTATE>::const_iterator iMap;

	// this map is very small, since a session only performs one or two functions at a time
	// so the lock time is short, which will block _all_ incoming functions
	lpStates->clear();
	pthread_mutex_lock(&m_hStateLock);
	for (iMap = m_mapBusyStates.begin(); iMap != m_mapBusyStates.end(); ++iMap)
		lpStates->push_back(iMap->second);
	pthread_mutex_unlock(&m_hStateLock);
}

void ECSession::AddClocks(double dblUser, double dblSystem, double dblReal)
{
	scoped_lock lock(m_hRequestStats);
	m_dblUser += dblUser;
	m_dblSystem += dblSystem;
	m_dblReal += dblReal;
}

void ECSession::GetClocks(double *lpdblUser, double *lpdblSystem, double *lpdblReal)
{
	scoped_lock lock(m_hRequestStats);
	*lpdblUser = m_dblUser;
	*lpdblSystem = m_dblSystem;
	*lpdblReal = m_dblReal;
}

void ECSession::GetClientVersion(std::string *lpstrVersion)
{
	scoped_lock lock(m_hRequestStats);
    lpstrVersion->assign(m_strClientVersion);
}

void ECSession::GetClientApp(std::string *lpstrClientApp)
{
	scoped_lock lock(m_hRequestStats);
    lpstrClientApp->assign(m_strClientApp);
}

/**
 * Get the object id of the object specified by the provided entryid.
 * This entryid can either be a short term or 'normal' entryid. If the entryid is a
 * short term entryid, the STE manager for this session will be queried for the object id.
 * If the entryid is a 'normal' entryid, the cache manager / database will be queried.
 *
 * @param[in]	lpEntryID		The entryid to get an object id for.
 * @param[out]	lpulObjId		Pointer to an unsigned int that will be set to the returned object id.
 * @param[out]	lpbIsShortTerm	Optional pointer to a boolean that will be set to true when the entryid
 * 								is a short term entryid.
 *
 * @retval	ZARAFA_E_INVALID_PARAMETER	lpEntryId or lpulObjId is NULL.
 * @retval	ZARAFA_E_INVALID_ENTRYID	The provided entryid is invalid.
 * @retval	ZARAFA_E_NOT_FOUND			No object was found for the provided entryid.
 */
ECRESULT ECSession::GetObjectFromEntryId(const entryId *lpEntryId, unsigned int *lpulObjId, unsigned int *lpulEidFlags)
{
	ECRESULT er = erSuccess;
	unsigned int ulObjId = 0;

	if (lpEntryId == NULL || lpulObjId == NULL) {
		er = ZARAFA_E_INVALID_PARAMETER;
		goto exit;
	}

	er = m_lpSessionManager->GetCacheManager()->GetObjectFromEntryId(lpEntryId, &ulObjId);
	if (er != erSuccess)
		goto exit;

	*lpulObjId = ulObjId;

	if(lpulEidFlags)
		*lpulEidFlags = ((EID *)lpEntryId->__ptr)->usFlags;

exit:
	return er;
}

ECRESULT ECSession::LockObject(unsigned int ulObjId)
{
	ECRESULT er = erSuccess;
	std::pair<LockMap::iterator, bool> res;
	scoped_lock lock(m_hLocksLock);

	res = m_mapLocks.insert(LockMap::value_type(ulObjId, ECObjectLock()));
	if (res.second == true)
		er = m_lpSessionManager->GetLockManager()->LockObject(ulObjId, m_sessionID, &res.first->second);

	return er;
}

ECRESULT ECSession::UnlockObject(unsigned int ulObjId)
{
	ECRESULT er = erSuccess;
	LockMap::iterator i;
	scoped_lock lock(m_hLocksLock);

	i = m_mapLocks.find(ulObjId);
	if (i == m_mapLocks.end())
		goto exit;

	er = i->second.Unlock();
	if (er == erSuccess)
		m_mapLocks.erase(i);

exit:
	return er;
}

size_t ECSession::GetObjectSize()
{
	size_t ulSize = sizeof(*this);

	ulSize += GetInternalObjectSize();
	ulSize += MEMORY_USAGE_STRING(m_strClientApp) +
			MEMORY_USAGE_STRING(m_strUsername) +
			MEMORY_USAGE_STRING(m_strClientVersion);

	ulSize += MEMORY_USAGE_MAP(m_mapBusyStates.size(), BusyStateMap);
	ulSize += MEMORY_USAGE_MAP(m_mapLocks.size(), LockMap);

	if (m_lpEcSecurity)
		ulSize += m_lpEcSecurity->GetObjectSize();


	// The Table manager size is not callculated here
//	ulSize += GetTableManager()->GetObjectSize();

	return ulSize;
}


/*
  ECAuthSession
*/
ECAuthSession::ECAuthSession(const char *src_addr, ECSESSIONID sessionID,
    ECDatabaseFactory *lpDatabaseFactory, ECSessionManager *lpSessionManager,
    unsigned int ulCapabilities) :
	BTSession(src_addr, sessionID, lpDatabaseFactory, lpSessionManager,
	    ulCapabilities)
{
	m_ulUserID = 0;
	m_bValidated = false;
	m_ulSessionTimeout = 30;	// authenticate within 30 seconds, or else!

	m_lpUserManagement = new ECUserManagement(this, m_lpSessionManager->GetPluginFactory(), m_lpSessionManager->GetConfig());

	m_ulConnectingPid = 0;

#ifdef LINUX
	m_NTLM_pid = -1;
#ifdef HAVE_GSSAPI
	m_gssServerCreds = GSS_C_NO_CREDENTIAL;
	m_gssContext = GSS_C_NO_CONTEXT;
#endif
#else
	SecInvalidateHandle(&m_hCredentials);
	SecInvalidateHandle(&m_hContext);
	m_cPackages = 0;
	m_ulPid = 0;
	m_lpPackageInfo = NULL;
#endif
}

ECAuthSession::~ECAuthSession()
{
#ifdef HAVE_GSSAPI
	OM_uint32 status;

	if (m_gssServerCreds)
		gss_release_cred(&status, &m_gssServerCreds);

	if (m_gssContext)
		gss_delete_sec_context(&status, &m_gssContext, GSS_C_NO_BUFFER);
#endif

	/* Wait until all locks have been closed */
	pthread_mutex_lock(&m_hThreadReleasedMutex);
	while (IsLocked())
		pthread_cond_wait(&m_hThreadReleased, &m_hThreadReleasedMutex);
	pthread_mutex_unlock(&m_hThreadReleasedMutex);

#ifdef LINUX
	if (m_NTLM_pid != -1) {
		int status;

		// close I/O to make ntlm_auth exit
		close(m_stdin);
		close(m_stdout);
		close(m_stderr);

		// wait for process status
		waitpid(m_NTLM_pid, &status, 0);
		ec_log_info("Removing ntlm_auth on pid %d. Exitstatus: %d", m_NTLM_pid, status);
		if (status == -1) {
			ec_log_err(string("System call waitpid failed: ") + strerror(errno));
		} else {
#ifdef WEXITSTATUS
				if(WIFEXITED(status)) { /* Child exited by itself */
					if(WEXITSTATUS(status))
						ec_log_notice("ntlm_auth exited with non-zero status %d", WEXITSTATUS(status));
				} else if(WIFSIGNALED(status)) {        /* Child was killed by a signal */
					ec_log_err("ntlm_auth was killed by signal %d", WTERMSIG(status));

				} else {                        /* Something strange happened */
					ec_log_err("ntlm_auth terminated abnormally");
				}
#else
				if (status)
					ec_log_notice("ntlm_auth exited with status %d", status);
#endif
		}
	}
#else // LINUX
	if (m_lpPackageInfo)
		FreeContextBuffer(m_lpPackageInfo);

	if (SecIsValidHandle(&m_hCredentials))
		FreeCredentialHandle(&m_hCredentials);

	if (SecIsValidHandle(&m_hContext))
		DeleteSecurityContext(&m_hContext);
#endif

	delete m_lpUserManagement;
}


ECRESULT ECAuthSession::CreateECSession(ECSESSIONGROUPID ecSessionGroupId,
    const std::string &cl_ver, const std::string &cl_app,
    const std::string &cl_app_ver, const std::string &cl_app_misc,
    ECSESSIONID *sessionID, ECSession **lppNewSession)
{
	ECRESULT er = erSuccess;
	ECSession *lpSession = NULL;
	ECSESSIONID newSID;

	if (!m_bValidated) {
		er = ZARAFA_E_LOGON_FAILED;
		goto exit;
	}

	CreateSessionID(m_ulClientCapabilities, &newSID);

	// ECAuthSessionOffline creates offline version .. no bOverrideClass construction
	lpSession = new(std::nothrow) ECSession(m_strSourceAddr.c_str(),
	            newSID, ecSessionGroupId, m_lpDatabaseFactory,
	            m_lpSessionManager, m_ulClientCapabilities, false,
	            m_ulValidationMethod, m_ulConnectingPid,
	            cl_ver, cl_app, cl_app_ver, cl_app_misc);
	if (!lpSession) {
		er = ZARAFA_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}

	er = lpSession->GetSecurity()->SetUserContext(m_ulUserID, m_ulImpersonatorID);
	if (er != erSuccess)
		goto exit;				// user not found anymore, or error in getting groups

	*sessionID = newSID;
	*lppNewSession = lpSession;

exit:
	if (er != erSuccess)
		delete lpSession;

	return er;
}

// This is a standard user/pass login.
// You always log in as the user you are authenticating with.
ECRESULT ECAuthSession::ValidateUserLogon(const char* lpszName, const char* lpszPassword, const char* lpszImpersonateUser)
{
	ECRESULT er = erSuccess;

	if (!lpszName)
	{
		ec_log_err("Invalid argument \"lpszName\" in call to ECAuthSession::ValidateUserLogon()");
		er = ZARAFA_E_INVALID_PARAMETER;
		goto exit;
    }
	if (!lpszPassword) {
		ec_log_err("Invalid argument \"lpszPassword\" in call to ECAuthSession::ValidateUserLogon()");
		er = ZARAFA_E_INVALID_PARAMETER;
		goto exit;
	}

	// SYSTEM can't login with user/pass
	if(stricmp(lpszName, ZARAFA_ACCOUNT_SYSTEM) == 0) {
		er = ZARAFA_E_NO_ACCESS;
		goto exit;
	}

	er = m_lpUserManagement->AuthUserAndSync(lpszName, lpszPassword, &m_ulUserID);
	if(er != erSuccess)
		goto exit;

	er = ProcessImpersonation(lpszImpersonateUser);
	if (er != erSuccess)
		goto exit;

	m_bValidated = true;
	m_ulValidationMethod = METHOD_USERPASSWORD;

exit:
	return er;
}

// Validate a user through the socket they are connecting through. This has the special feature
// that you can connect as a different user than you are specifying in the username. For example,
// you could be connecting as 'root' and being granted access because the zarafa-server process
// is also running as 'root', but you are actually loggin in as user 'user1'.
ECRESULT ECAuthSession::ValidateUserSocket(int socket, const char* lpszName, const char* lpszImpersonateUser)
{
	ECRESULT 		er = erSuccess;
	const char *p = NULL;
	bool			allowLocalUsers = false;
	int				pid = 0;
#ifdef WIN32
	TCHAR			*pt = NULL;
	TCHAR			*localAdminUsers = NULL;
	TCHAR			szUsernameServer[256+1] = {0}; // max size is UNLEN +1 (defined in Lmcons.h)
	TCHAR			szUsernameClient[256+1] = {0};
	DWORD			dwSize = 0;
#else
	char			*ptr = NULL;
	char			*localAdminUsers = NULL;
#endif

    if (!lpszName)
    {
		ec_log_err("Invalid argument \"lpszName\" in call to ECAuthSession::ValidateUserSocket()");
		er = ZARAFA_E_INVALID_PARAMETER;
		goto exit;
    }
	if (!lpszImpersonateUser) {
		ec_log_err("Invalid argument \"lpszImpersonateUser\" in call to ECAuthSession::ValidateUserSocket()");
		er = ZARAFA_E_INVALID_PARAMETER;
		goto exit;
	}
	p = m_lpSessionManager->GetConfig()->GetSetting("allow_local_users");
	if (p && !stricmp(p, "yes")) {
		allowLocalUsers = true;
	}

	// Authentication stage
#ifdef LINUX
	localAdminUsers = strdup(m_lpSessionManager->GetConfig()->GetSetting("local_admin_users"));

	struct passwd pwbuf;
	struct passwd *pw;
	uid_t uid;
	char strbuf[1024];
#ifdef SO_PEERCRED
	struct ucred cr;
	unsigned int cr_len;

	cr_len = sizeof(struct ucred);
	if(getsockopt(socket, SOL_SOCKET, SO_PEERCRED, &cr, &cr_len) != 0 || cr_len != sizeof(struct ucred)) {
		er = ZARAFA_E_LOGON_FAILED;
		goto exit;
	}

	uid = cr.uid; // uid is the uid of the user that is connecting
	pid = cr.pid;
#else // SO_PEERCRED
#ifdef HAVE_GETPEEREID
	gid_t gid;

	if (getpeereid(socket, &uid, &gid)) {
		er = ZARAFA_E_LOGON_FAILED;
		goto exit;
	}
#else // HAVE_GETPEEREID
#error I have no way to find out the remote user and I want to cry
#endif // HAVE_GETPEEREID
#endif // SO_PEERCRED

	if (geteuid() == uid) {
		// User connecting is connecting under same UID as the server is running under, allow this
		goto userok;
	}

	// Lookup user name
	pw = NULL;
#ifdef HAVE_GETPWNAM_R
	getpwnam_r(lpszName, &pwbuf, strbuf, sizeof(strbuf), &pw);
#else
	// OpenBSD does not have getpwnam_r() .. FIXME: threading issue!
	pw = getpwnam(lpszName);
#endif

	if (allowLocalUsers && pw && pw->pw_uid == uid)
		// User connected as himself
		goto userok;

	p = strtok_r(localAdminUsers, WHITESPACE, &ptr);

	while (p) {
	    pw = NULL;
#ifdef HAVE_GETPWNAM_R
		getpwnam_r(p, &pwbuf, strbuf, sizeof(strbuf), &pw);
#else
		pw = getpwnam(p);
#endif

		if (pw) {
			if (pw->pw_uid == uid) {
				// A local admin user connected - ok
				goto userok;
			}
		}
		p = strtok_r(NULL, WHITESPACE, &ptr);
	}

#else // LINUX

	localAdminUsers = _tcsdup(GetConfigSetting(m_lpSessionManager->GetConfig(), "local_admin_users"));

	dwSize = arraySize(szUsernameServer);

	if (!GetNamedPipeHandleState((HANDLE)socket, NULL, NULL, NULL, NULL, szUsernameClient, arraySize(szUsernameClient)) ||
		!GetUserName(szUsernameServer, &dwSize))
	{
		//GetLastError();
		er = ZARAFA_E_LOGON_FAILED;
		goto exit;
	}

	if (_tcscmp(szUsernameServer, szUsernameClient) == 0)
		goto userok;

	pt = _tcstok(localAdminUsers, WHITESPACE);

	while (pt) {
		if (_tcscmp(szUsernameClient, pt) == 0)
			goto userok;

		pt = _tcstok(NULL, WHITESPACE);
	}

#endif // LINUX

	er = ZARAFA_E_LOGON_FAILED;
	goto exit;

userok:
    // Check whether user exists in the user database
	er = m_lpUserManagement->ResolveObjectAndSync(OBJECTCLASS_USER, lpszName, &m_ulUserID);
	if (er != erSuccess)
	    goto exit;

	er = ProcessImpersonation(lpszImpersonateUser);
	if (er != erSuccess)
		goto exit;

	m_bValidated = true;
	m_ulValidationMethod = METHOD_SOCKET;
	m_ulConnectingPid = pid;

exit:
	free(localAdminUsers);
	return er;
}

ECRESULT ECAuthSession::ValidateUserCertificate(struct soap* soap, const char* lpszName, const char* lpszImpersonateUser)
{
	ECRESULT		er = ZARAFA_E_LOGON_FAILED;
	X509			*cert = NULL;			// client certificate
	EVP_PKEY		*pubkey = NULL;			// client public key
	EVP_PKEY		*storedkey = NULL;
	int				res = -1;

	const char *sslkeys_path = m_lpSessionManager->GetConfig()->GetSetting("sslkeys_path", "", NULL);
	BIO 			*biofile = NULL;

	bfs::path		keysdir;
	bfs::directory_iterator key_last;

	if (!soap) {
		ec_log_err("Invalid argument \"soap\" in call to ECAuthSession::ValidateUserCertificate()");
		er = ZARAFA_E_INVALID_PARAMETER;
		goto exit;
	}
	if (!lpszName) {
		ec_log_err("Invalid argument \"lpszName\" in call to ECAuthSession::ValidateUserCertificate()");
		er = ZARAFA_E_INVALID_PARAMETER;
		goto exit;
	}
	if (!lpszImpersonateUser) {
		ec_log_err("Invalid argument \"lpszImpersonateUser\" in call to ECAuthSession::ValidateUserCertificate()");
		er = ZARAFA_E_INVALID_PARAMETER;
		goto exit;
	}

	if (!sslkeys_path || sslkeys_path[0] == '\0') {
		ec_log_warn("No public keys directory defined in sslkeys_path.");
		goto exit;
	}

	cert = SSL_get_peer_certificate(soap->ssl);
	if (!cert) {
		// windows client without ssl certificate
		ec_log_info("No certificate in SSL connection.");
		goto exit;
	}
	pubkey = X509_get_pubkey(cert);	// need to free
	if (!pubkey) {
		// if you get here, please tell me how, 'cause I'd like to know :)
		ec_log_info("No public key in certificate.");
		goto exit;
	}

	try {
		keysdir = sslkeys_path;
		if (!bfs::exists(keysdir)) {
			ec_log_info("Certificate path \"%s\" is not present.", sslkeys_path);
			er = ZARAFA_E_LOGON_FAILED;
			goto exit;
		}

		for (bfs::directory_iterator key(keysdir); key != key_last; ++key) {
			if (is_directory(key->status()))
				continue;

			std::string filename = path_to_string(key->path());
			const char *lpFileName = filename.c_str();

			biofile = BIO_new_file(lpFileName, "r");
			if (!biofile) {
				ec_log_info("Unable to create BIO for \"%s\": %s", lpFileName, ERR_error_string(ERR_get_error(), NULL));
				continue;
			}

			storedkey = PEM_read_bio_PUBKEY(biofile, NULL, NULL, NULL);
			if (!storedkey) {
				ec_log_info("Unable to read PUBKEY from \"%s\": %s", lpFileName, ERR_error_string(ERR_get_error(), NULL));
				BIO_free(biofile);
				continue;
			}

			res = EVP_PKEY_cmp(pubkey, storedkey);

			BIO_free(biofile);
			EVP_PKEY_free(storedkey);

			if (res <= 0) {
				ec_log_info("Certificate \"%s\" does not match.", lpFileName);
			} else {
				er = erSuccess;
				ec_log_info("Accepted certificate \"%s\" from client.", lpFileName);
				break;
			}
		}
	} catch (const bfs::filesystem_error&) {
		// @todo: use get_error_info ?
		ec_log_info("Boost exception during certificate validation.");
	} catch (const std::exception& e) {
		ec_log_info("STD exception during certificate validation: %s", e.what());
	}
	if (er != erSuccess)
		goto exit;

    // Check whether user exists in the user database
	er = m_lpUserManagement->ResolveObjectAndSync(OBJECTCLASS_USER, lpszName, &m_ulUserID);
	if (er != erSuccess)
		goto exit;

	er = ProcessImpersonation(lpszImpersonateUser);
	if (er != erSuccess)
		goto exit;

	m_bValidated = true;
	m_ulValidationMethod = METHOD_SSL_CERT;

exit:
	if (cert)
		X509_free(cert);

	if (pubkey)
		EVP_PKEY_free(pubkey);

	return er;
}

#ifdef LINUX
#define NTLMBUFFER 8192
ECRESULT ECAuthSession::ValidateSSOData(struct soap* soap, const char* lpszName, const char* lpszImpersonateUser, const char* szClientVersion, const char *szClientApp, const char *szClientAppVersion, const char *szClientAppMisc, const struct xsd__base64Binary* lpInput, struct xsd__base64Binary **lppOutput)
{
	ECRESULT er = ZARAFA_E_INVALID_PARAMETER;
	if (!soap) {
		ec_log_err("Invalid argument \"soap\" in call to ECAuthSession::ValidateSSOData()");
		goto exit;
	}
	if (!lpszName) {
		ec_log_err("Invalid argument \"lpszName\" in call to ECAuthSession::ValidateSSOData()");
		goto exit;
	}
	if (!lpszImpersonateUser) {
		ec_log_err("Invalid argument \"lpszImpersonateUser\" in call to ECAuthSession::ValidateSSOData()");
		goto exit;
	}
	if (!szClientVersion) {
		ec_log_err("Invalid argument \"szClientVersion\" in call to ECAuthSession::ValidateSSOData()");
		goto exit;
	}
	if (!szClientApp) {
		ec_log_err("Invalid argument \"szClientApp\" in call to ECAuthSession::ValidateSSOData()");
		goto exit;
	}
	if (!lpInput) {
		ec_log_err("Invalid argument \"lpInput\" in call to ECAuthSession::ValidateSSOData()");
		goto exit;
	}
	if (!lppOutput) {
		ec_log_err("Invalid argument \"lppOutput\" in call to ECAuthSession::ValidateSSOData()");
		goto exit;
	}

	er = ZARAFA_E_LOGON_FAILED;

	// first NTLM package starts with that signature, continues are detected by the filedescriptor
	if (m_NTLM_pid != -1 || strncmp((const char*)lpInput->__ptr, "NTLM", 4) == 0)
		er = ValidateSSOData_NTLM(soap, lpszName, szClientVersion, szClientApp, szClientAppVersion, szClientAppMisc, lpInput, lppOutput);
	else
		er = ValidateSSOData_KRB5(soap, lpszName, szClientVersion, szClientApp, szClientAppVersion, szClientAppMisc, lpInput, lppOutput);
	if (er != erSuccess)
		goto exit;

	er = ProcessImpersonation(lpszImpersonateUser);
	if (er != erSuccess)
		goto exit;

exit:
	return er;
}

#ifdef HAVE_GSSAPI
const char gss_display_status_fail_message[] = "Call to gss_display_status failed. Reason: ";

static ECRESULT LogKRB5Error_2(const char *msg, OM_uint32 code, OM_uint32 type)
{
	gss_buffer_desc gssMessage = GSS_C_EMPTY_BUFFER;
	OM_uint32 status = 0;
	OM_uint32 context = 0;

	if (msg == NULL) {
		ec_log_err("Invalid argument \"msg\" in call to ECAuthSession::LogKRB5Error()");
		return ZARAFA_E_INVALID_PARAMETER;
	}
	ECRESULT retval = ZARAFA_E_CALL_FAILED;
	do {
		OM_uint32 result = gss_display_status(&status, code, type, GSS_C_NULL_OID, &context, &gssMessage);
		switch (result) {
		case GSS_S_COMPLETE:
			ec_log_warn("%s: %s", msg, (char*)gssMessage.value);
			retval = erSuccess;
			break;
		case GSS_S_BAD_MECH:
			ec_log_warn("%s: %s", gss_display_status_fail_message, "unsupported mechanism type was requested.");
			retval = ZARAFA_E_CALL_FAILED;
			break;
		case GSS_S_BAD_STATUS:
			ec_log_warn("%s: %s", gss_display_status_fail_message, "status value was not recognized, or the status type was neither GSS_C_GSS_CODE nor GSS_C_MECH_CODE.");
			retval = ZARAFA_E_CALL_FAILED;
			break;
		}
		gss_release_buffer(&status, &gssMessage);
	} while (context != 0);
	return retval;
}

ECRESULT ECAuthSession::LogKRB5Error(const char* msg, OM_uint32 major, OM_uint32 minor)
{
	if (!msg) {
		ec_log_err("Invalid argument \"msg\" in call to ECAuthSession::LogKRB5Error()");
		return ZARAFA_E_INVALID_PARAMETER;
	}
	LogKRB5Error_2(msg, major, GSS_C_GSS_CODE);
	return LogKRB5Error_2(msg, minor, GSS_C_MECH_CODE);
}
#endif

ECRESULT ECAuthSession::ValidateSSOData_KRB5(struct soap* soap, const char* lpszName, const char* szClientVersion, const char* szClientApp, const char *szClientAppVersion, const char *szClientAppMisc, const struct xsd__base64Binary* lpInput, struct xsd__base64Binary** lppOutput)
{
	ECRESULT er = ZARAFA_E_INVALID_PARAMETER;
#ifndef HAVE_GSSAPI
	ec_log_err("Incoming Kerberos request, but this server was build without GSSAPI support.");
#else
	OM_uint32 retval, status;

	gss_name_t gssServername = GSS_C_NO_NAME;
	gss_buffer_desc gssInputBuffer = GSS_C_EMPTY_BUFFER;
	const char *szHostname = NULL;
	std::string principal;

	gss_name_t gssUsername = GSS_C_NO_NAME;
	gss_buffer_desc gssUserBuffer = GSS_C_EMPTY_BUFFER;
	gss_buffer_desc gssOutputToken = GSS_C_EMPTY_BUFFER;
	std::string strUsername;
	string::size_type pos;

	struct xsd__base64Binary *lpOutput = NULL;

	if (!soap) {
		ec_log_err("Invalid argument \"soap\" in call to ECAuthSession::ValidateSSOData_KRB5()");
		goto exit;
	}
	if (!lpszName) {
		ec_log_err("Invalid argument \"lpszName\" in call to ECAuthSession::ValidateSSOData_KRB5()");
		goto exit;
	}
	if (!szClientVersion) {
		ec_log_err("Invalid argument \"zClientVersionin\" in call to ECAuthSession::ValidateSSOData_KRB5()");
		goto exit;
	}
	if (!szClientApp) {
		ec_log_err("Invalid argument \"szClientApp\" in call to ECAuthSession::ValidateSSOData_KRB5()");
		goto exit;
	}
	if (!lpInput) {
		ec_log_err("Invalid argument \"lpInput\" in call to ECAuthSession::ValidateSSOData_KRB5()");
		goto exit;
	}
	if (!lppOutput) {
		ec_log_err("Invalid argument \"lppOutput\" in call to ECAuthSession::ValidateSSOData_KRB5()");
		goto exit;
	}
	er = ZARAFA_E_LOGON_FAILED;
	if (m_gssServerCreds == GSS_C_NO_CREDENTIAL) {
		m_gssContext = GSS_C_NO_CONTEXT;

		// ECServer made sure this setting option always contains the best hostname
		// If it's not there, that's unacceptable.
		szHostname = m_lpSessionManager->GetConfig()->GetSetting("server_hostname");
		if (!szHostname || szHostname[0] == '\0') {
			ec_log_crit("Hostname not found, required for Kerberos");
			goto exit;
		}
		principal = "zarafa@";
		principal += szHostname;

		ec_log_debug("Kerberos principal: %s", principal.c_str());

		gssInputBuffer.value = (void*)principal.data();
		gssInputBuffer.length = principal.length() + 1;

		retval = gss_import_name(&status, &gssInputBuffer, GSS_C_NT_HOSTBASED_SERVICE, &gssServername);
		if (retval != GSS_S_COMPLETE) {
			LogKRB5Error("Unable to import server name", retval, status);
			goto exit;
		}

		retval = gss_acquire_cred(&status, gssServername, GSS_C_INDEFINITE, GSS_C_NO_OID_SET, GSS_C_ACCEPT, &m_gssServerCreds, NULL, NULL);
		if (retval != GSS_S_COMPLETE) {
			LogKRB5Error("Unable to acquire credentials handle", retval, status);
			goto exit;
		}
	}

	gssInputBuffer.length = lpInput->__size;
	gssInputBuffer.value = lpInput->__ptr;

	retval = gss_accept_sec_context(&status, &m_gssContext, m_gssServerCreds, &gssInputBuffer, GSS_C_NO_CHANNEL_BINDINGS, &gssUsername, NULL, &gssOutputToken, NULL, NULL, NULL);

	if (gssOutputToken.length) {
		// we need to send data back to the client, no need to consider retval
		lpOutput = s_alloc<struct xsd__base64Binary>(soap);
		lpOutput->__size = gssOutputToken.length;
		lpOutput->__ptr = s_alloc<unsigned char>(soap, gssOutputToken.length);
		memcpy(lpOutput->__ptr, gssOutputToken.value, gssOutputToken.length);

		gss_release_buffer(&status, &gssOutputToken);
	}

	if (retval == GSS_S_CONTINUE_NEEDED) {
		er = ZARAFA_E_SSO_CONTINUE;
		goto exit;
	} else if (retval != GSS_S_COMPLETE) {
		LogKRB5Error("Unable to accept security context", retval, status);
		ZLOG_AUDIT(m_lpSessionManager->GetAudit(), "authenticate failed user='%s' from='%s' method='kerberos sso' program='%s'",
			lpszName, soap->host, szClientApp);
		goto exit;
	}

	retval = gss_display_name(&status, gssUsername, &gssUserBuffer, NULL);
	if (retval) {
		LogKRB5Error("Unable to convert username", retval, status);
		goto exit;
	}

	ec_log_debug("Kerberos username: %s", static_cast<const char *>(gssUserBuffer.value));
	// kerberos returns: username@REALM, username is case-insensitive
	strUsername.assign((char*)gssUserBuffer.value, gssUserBuffer.length);
	pos = strUsername.find_first_of('@');
	if (pos != string::npos)
		strUsername.erase(pos);

	if (stricmp(strUsername.c_str(), lpszName) == 0) {
		er = m_lpUserManagement->ResolveObjectAndSync(ACTIVE_USER, lpszName, &m_ulUserID);
		// don't check NONACTIVE, since those shouldn't be able to login
		if(er != erSuccess)
			goto exit;

		m_bValidated = true;
		m_ulValidationMethod = METHOD_SSO;
		ec_log_info("Kerberos Single Sign-On: User \"%s\" authenticated", lpszName);
		ZLOG_AUDIT(m_lpSessionManager->GetAudit(), "authenticate ok user='%s' from='%s' method='kerberos sso' program='%s'",
			lpszName, soap->host, szClientApp);
	} else {
		ec_log_err("Kerberos username \"%s\" authenticated, but user \"%s\" requested.", (char*)gssUserBuffer.value, lpszName);
		ZLOG_AUDIT(m_lpSessionManager->GetAudit(), "authenticate spoofed user='%s' requested='%s' from='%s' method='kerberos sso' program='%s'",
			static_cast<char *>(gssUserBuffer.value), lpszName, soap->host, szClientApp);
	}

exit:
	if (gssUserBuffer.length)
		gss_release_buffer(&status, &gssUserBuffer);

	if (gssOutputToken.length)
		gss_release_buffer(&status, &gssOutputToken);

	if (gssUsername != GSS_C_NO_NAME)
		gss_release_name(&status, &gssUsername);

	if (gssServername != GSS_C_NO_NAME)
		gss_release_name(&status, &gssServername);

	*lppOutput = lpOutput;
#endif

	return er;
}

ECRESULT ECAuthSession::ValidateSSOData_NTLM(struct soap* soap, const char* lpszName, const char* szClientVersion, const char* szClientApp, const char *szClientAppVersion, const char *szClientAppMisc, const struct xsd__base64Binary* lpInput, struct xsd__base64Binary **lppOutput)
{
	ECRESULT er = ZARAFA_E_INVALID_PARAMETER;
	struct xsd__base64Binary *lpOutput = NULL;
	char buffer[NTLMBUFFER];
	std::string strEncoded, strDecoded, strAnswer;
	ssize_t bytes = 0;
	char separator = '\\';      // get config version
	fd_set fds;
	int max, ret;
	struct timeval tv;

	if (!soap) {
		ec_log_err("Invalid argument \"soap\" in call to ECAuthSession::ValidateSSOData_NTLM()");
		goto exit;
	}
	if (!lpszName) {
		ec_log_err("Invalid argument \"lpszName\" in call to ECAuthSession::ValidateSSOData_NTLM()");
		goto exit;
	}
	if (!szClientVersion) {
		ec_log_err("Invalid argument \"zClientVersionin\" in call to ECAuthSession::ValidateSSOData_NTLM()");
		goto exit;
	}
	if (!szClientApp) {
		ec_log_err("Invalid argument \"szClientApp\" in call to ECAuthSession::ValidateSSOData_NTLM()");
		goto exit;
	}
	if (!lpInput) {
		ec_log_err("Invalid argument \"lpInput\" in call to ECAuthSession::ValidateSSOData_NTLM()");
		goto exit;
	}
	if (!lppOutput) {
		ec_log_err("Invalid argument \"lppOutput\" in call to ECAuthSession::ValidateSSOData_NTLM()");
		goto exit;
	}
	er = ZARAFA_E_LOGON_FAILED;
	strEncoded = base64_encode(lpInput->__ptr, lpInput->__size);
	errno = 0;

	if (m_NTLM_pid == -1) {
		// start new ntlmauth pipe
		// TODO: configurable path?

		if (pipe(m_NTLM_stdin) == -1 || pipe(m_NTLM_stdout) == -1 || pipe(m_NTLM_stderr) == -1) {
			ec_log_crit(string("Unable to create communication pipes for ntlm_auth: ") + strerror(errno));
			goto exit;
		}

		/*
		 * Why are we using vfork() ?
		 *
		 * You might as well use fork() here but vfork() is much faster in our case; this is because vfork() doesn't actually duplicate
		 * any pages, expecting you to call execl(). Watch out though, since data changes done in the client process before execl() WILL
		 * affect the mother process. (however, the file descriptor table is correctly cloned)
		 *
		 * The reason fork() is slow is that even though it is doing a Copy-On-Write copy, it still needs to do some page-copying to set up your
		 * process. This copying time increases with memory usage of the mother process; in fact, running 200 forks() on a process occupying
		 * 512MB of memory takes 15 seconds, while the same vfork()/exec() loop takes under .5 of a second.
		 *
		 * If vfork() is not available, or is broken on another platform, it is safe to simply replace it with fork(), but it will be quite slow!
		 */

		m_NTLM_pid = vfork();
		if (m_NTLM_pid == -1) {
			// broken
			ec_log_crit(string("Unable to start new process for ntlm_auth: ") + strerror(errno));
			goto exit;
		} else if (m_NTLM_pid == 0) {
			// client
			int j, k;

			close(m_NTLM_stdin[1]);
			close(m_NTLM_stdout[0]);
			close(m_NTLM_stderr[0]);

			dup2(m_NTLM_stdin[0], 0);
			dup2(m_NTLM_stdout[1], 1);
			dup2(m_NTLM_stderr[1], 2);

			// close all other open file descriptors, so ntlm doesn't keep the zarafa-server sockets open
			j = getdtablesize();
			for (k = 3; k < j; ++k)
				close(k);

			execl("/bin/sh", "sh", "-c", "ntlm_auth -d0 --helper-protocol=squid-2.5-ntlmssp", NULL);

			ec_log_crit(string("Cannot start ntlm_auth: ") + strerror(errno));
			_exit(2);
		} else {
			// parent
			ec_log_info("New ntlm_auth started on pid %d", m_NTLM_pid);
			close(m_NTLM_stdin[0]);
			close(m_NTLM_stdout[1]);
			close(m_NTLM_stderr[1]);
			m_stdin = ec_relocate_fd(m_NTLM_stdin[1]);
			m_stdout = ec_relocate_fd(m_NTLM_stdout[0]);
			m_stderr = ec_relocate_fd(m_NTLM_stderr[0]);

			// Yo! Refresh!
			write(m_stdin, "YR ", 3);
			write(m_stdin, strEncoded.c_str(), strEncoded.length());
			write(m_stdin, "\n", 1);
		}
	} else {
		// Knock knock! who's there?
		write(m_stdin, "KK ", 3);
		write(m_stdin, strEncoded.c_str(), strEncoded.length());
		write(m_stdin, "\n", 1);
	}

	memset(buffer, 0, NTLMBUFFER);

	tv.tv_sec = 10;             // timeout of 10 seconds before ntlm_auth can respond too large?
	tv.tv_usec = 0;

	FD_ZERO(&fds);
	FD_SET(m_stdout, &fds);
	FD_SET(m_stderr, &fds);
	max = m_stderr > m_stdout ? m_stderr : m_stdout;

retry:
	ret = select(max+1, &fds, NULL, NULL, &tv);
	if (ret < 0) {
		if (errno == EINTR)
			goto retry;

		ec_log_err(string("Error while waiting for data from ntlm_auth: ") + strerror(errno));
		goto exit;
	}

	if (ret == 0) {
		// timeout
		ec_log_err("Timeout while reading from ntlm_auth");
		goto exit;
	}

	// stderr is optional, and always written first
	if (FD_ISSET(m_stderr, &fds)) {
		// log stderr of ntlm_auth to logfile (loop?)
		bytes = read(m_stderr, buffer, NTLMBUFFER-1);
		if (bytes >= 0)
			buffer[bytes] = '\0';
		// print in lower level. if ntlm_auth was not installed (kerberos only environment), you won't care that ntlm_auth doesn't work.
		// login error is returned to the client, which was expected anyway.
		ec_log_notice(string("Received error from ntlm_auth:\n") + buffer);
		goto exit;
	}

	// stdout is mandatory, so always read from this pipe
	memset(buffer, 0, NTLMBUFFER);
	bytes = read(m_stdout, buffer, NTLMBUFFER-1);
	if (bytes < 0) {
		ec_log_err(string("Unable to read data from ntlm_auth: ") + strerror(errno));
		goto exit;
	} else if (bytes == 0) {
		ec_log_err("Nothing read from ntlm_auth");
		goto exit;
	}
	if (buffer[bytes-1] == '\n')
		/*
		 * Strip newline right away, it is not useful for logging,
		 * nor for base64_decode.
		 */
		buffer[--bytes] = '\0';
	if (bytes < 2) {
		/* Ensure buffer[0]==.. && buffer[1]==.. is valid to do */
		ec_log_err("Short reply from ntlm_auth");
		goto exit;
	}

	if (bytes >= 3)
		/*
		 * Extract response text (if any) following the reply code
		 * (and space). Else left empty.
		 */
		strAnswer.assign(buffer + 3, bytes - 3);

	if (buffer[0] == 'B' && buffer[1] == 'H') {
		// Broken Helper
		ec_log_err("Incorrect data fed to ntlm_auth");
		goto exit;
	} else if (buffer[0] == 'T' && buffer[1] == 'T') {
		// Try This
		strDecoded = base64_decode(strAnswer);

		lpOutput = s_alloc<struct xsd__base64Binary>(soap);
		lpOutput->__size = strDecoded.length();
		lpOutput->__ptr = s_alloc<unsigned char>(soap, strDecoded.length());
		memcpy(lpOutput->__ptr, strDecoded.data(), strDecoded.length());

		er = ZARAFA_E_SSO_CONTINUE;

	} else if (buffer[0] == 'A' && buffer[1] == 'F') {
		// Authentication Fine
		// Samba default runs in UTF-8 and setting 'unix charset' to windows-1252 in the samba config will break ntlm_auth
		// convert the username before we use it in Zarafa
		ECIConv iconv("windows-1252", "utf-8");
		if (!iconv.canConvert()) {
			ec_log_crit("Problem setting up windows-1252 to utf-8 converter");
			goto exit;
		}

		strAnswer = iconv.convert(strAnswer);

		ec_log_info("Found username (%s)", strAnswer.c_str());

		// if the domain separator is not found, assume we only have the username (samba)
		string::size_type pos = strAnswer.find_first_of(separator);
		if (pos != string::npos) {
			++pos;
			strAnswer.assign(strAnswer, pos, strAnswer.length()-pos);
		}

		// Check whether user exists in the user database
		er = m_lpUserManagement->ResolveObjectAndSync(ACTIVE_USER, (char *)strAnswer.c_str(), &m_ulUserID);
		// don't check NONACTIVE, since those shouldn't be able to login
		if(er != erSuccess)
			goto exit;

		if (stricmp(lpszName, strAnswer.c_str()) != 0) {
			// cannot open another user without password
			// or should we check permissions ?
			ec_log_warn("Single Sign-On: User \"%s\" authenticated, but user \"%s\" requested.", strAnswer.c_str(), lpszName);
			ZLOG_AUDIT(m_lpSessionManager->GetAudit(), "authenticate spoofed user='%s' requested='%s' from='%s' method='ntlm sso' program='%s'",
				strAnswer.c_str(), lpszName, soap->host, szClientApp);
			er = ZARAFA_E_LOGON_FAILED;
		} else {
			m_bValidated = true;
			m_ulValidationMethod = METHOD_SSO;
			er = erSuccess;
			ec_log_info("Single Sign-On: User \"%s\" authenticated", strAnswer.c_str());
			ZLOG_AUDIT(m_lpSessionManager->GetAudit(), "authenticate ok user='%s' from='%s' method='ntlm sso' program='%s'",
				lpszName, soap->host, szClientApp);
		}

	} else if (buffer[0] == 'N' && buffer[1] == 'A') {
		// Not Authenticated
		ec_log_info("Requested user \"%s\" denied. Not authenticated: \"%s\"", lpszName, strAnswer.c_str());
		ZLOG_AUDIT(m_lpSessionManager->GetAudit(), "authenticate failed user='%s' from='%s' method='ntlm sso' program='%s'",
			lpszName, soap->host, szClientApp);
		er = ZARAFA_E_LOGON_FAILED;
	} else {
		// unknown response?
		ec_log_err("Unknown response from ntlm_auth: %.*s", static_cast<int>(bytes), buffer);
		er = ZARAFA_E_CALL_FAILED;
		goto exit;
	}

	*lppOutput = lpOutput;

exit:
	return er;
}
#undef NTLMBUFFER
#endif
#ifdef WIN32
ECRESULT ECAuthSession::ValidateSSOData(struct soap* soap, const char* lpszName, const char* lpszImpersonateUser, const char* szClientVersion, const char* szClientApp, const char *szClientAppVersion, const char *szClientAppMisc, const struct xsd__base64Binary* lpInput, struct xsd__base64Binary** lppOutput)
{
	ECRESULT er = ZARAFA_E_INVALID_PARAMETER;
	SECURITY_STATUS	SecStatus;
	bool			bFirst = false;
	SecBuffer		OutSecBuffer, InSecBuffer[2];
	SecBufferDesc	OutSecBufferDesc, InSecBufferDesc;
	ULONG			fContextAttr = 0;
	struct xsd__base64Binary *lpOutput = NULL;

	OutSecBuffer.pvBuffer = NULL;

	if (!soap) {
		ec_log_err("Invalid argument \"soap\" in call to ECAuthSession::ValidateSSOData()");
		goto exit;
	}
	if (!lpszName) {
		ec_log_err("Invalid argument \"lpszName\" in call to ECAuthSession::ValidateSSOData()");
		goto exit;
	}
	if (!szClientVersion) {
		ec_log_err("Invalid argument \"zClientVersionin\" in call to ECAuthSession::ValidateSSOData()");
		goto exit;
	}
	if (!szClientApp) {
		ec_log_err("Invalid argument \"szClientApp\" in call to ECAuthSession::ValidateSSOData()");
		goto exit;
	}
	if (!lpInput) {
		ec_log_err("Invalid argument \"lpInput\" in call to ECAuthSession::ValidateSSOData()");
		goto exit;
	}
	if (!lppOutput) {
		ec_log_err("Invalid argument \"lppOutput\" in call to ECAuthSession::ValidateSSOData()");
		goto exit;
	}
	er = ZARAFA_E_LOGON_FAILED;
	if (!SecIsValidHandle(&m_hCredentials)) {
		// new connection

		SecStatus = EnumerateSecurityPackages(&m_cPackages, &m_lpPackageInfo);
		if (SecStatus != SEC_E_OK) {
			ec_log_err("Unable to get security packages list");
			goto exit;
		}

		for (m_ulPid = 0; m_ulPid < m_cPackages; ++m_ulPid) {
			// find auto detect method, always (?) first item in the list
			// TODO: config option to force a method?
			if (_tcsicmp(_T("Negotiate"), m_lpPackageInfo[m_ulPid].Name) == 0)
				break;
		}
		if (m_ulPid == m_cPackages) {
			ec_log_err("Negotiate security package was not found");
			goto exit;
		}

		SecStatus = AcquireCredentialsHandle(/* principal */NULL, /* package */m_lpPackageInfo[m_ulPid].Name,
			/*fCredentialUse*/SECPKG_CRED_INBOUND, /*pvLogonID*/NULL, /*pAuthData*/NULL, NULL, NULL,
			&m_hCredentials, &m_tsExpiry);
		if (SecStatus != SEC_E_OK) {
			ec_log_err("Unable to acquire credentials handle: 0x%08X", SecStatus);
			goto exit;
		}

		bFirst = true;
	} else {
		// step 2
	}

	// prepare input buffer
	InSecBufferDesc.ulVersion = SECBUFFER_VERSION;
	InSecBufferDesc.cBuffers = 2;
	InSecBufferDesc.pBuffers = InSecBuffer;
	InSecBuffer[0].BufferType = SECBUFFER_TOKEN;
	InSecBuffer[0].cbBuffer = lpInput->__size;
	InSecBuffer[0].pvBuffer = (void*)lpInput->__ptr;
	InSecBuffer[1].BufferType = SECBUFFER_EMPTY;
	InSecBuffer[1].cbBuffer = 0;
	InSecBuffer[1].pvBuffer = NULL;

	OutSecBufferDesc.ulVersion = SECBUFFER_VERSION;
	OutSecBufferDesc.cBuffers = 1;
	OutSecBufferDesc.pBuffers = &OutSecBuffer;
	OutSecBuffer.BufferType = SECBUFFER_TOKEN;
	OutSecBuffer.cbBuffer = m_lpPackageInfo[m_ulPid].cbMaxToken;
	OutSecBuffer.pvBuffer = NULL;

	SecStatus = AcceptSecurityContext(&m_hCredentials, bFirst ? NULL : &m_hContext, &InSecBufferDesc,
		ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_CONFIDENTIALITY | ASC_REQ_CONNECTION, SECURITY_NATIVE_DREP,
		&m_hContext, &OutSecBufferDesc, &fContextAttr, &m_tsExpiry);

	if (FAILED(SecStatus)) {
		ec_log_err("Error accepting security context: 0x%08X", SecStatus);
		goto exit;
	}

	if (SecStatus) {
		// continue data
		lpOutput = s_alloc<struct xsd__base64Binary>(soap);
		lpOutput->__size = OutSecBuffer.cbBuffer;
		lpOutput->__ptr = s_alloc<unsigned char>(soap, OutSecBuffer.cbBuffer);
		memcpy(lpOutput->__ptr, OutSecBuffer.pvBuffer, OutSecBuffer.cbBuffer);

		er = ZARAFA_E_SSO_CONTINUE;
	} else {
		// logged on
		TCHAR username[256];
		ULONG size = 256;
		string strUsername;

		SecStatus = ImpersonateSecurityContext(&m_hContext);
		if (SecStatus != SEC_E_OK)
			goto exit;

		GetUserName(username, &size);
		strUsername = convert_to<std::string>("UTF-8", username, rawsize(username), CHARSET_TCHAR);

		SecStatus = RevertSecurityContext(&m_hContext);
		if (SecStatus != SEC_E_OK)
			goto exit;

		// Check whether user exists in the user database
		er = m_lpUserManagement->ResolveObjectAndSync(ACTIVE_USER, (char *)strUsername.c_str(), &m_ulUserID);
		// don't check NONACTIVE, since those shouldn't be able to login
		if (er != erSuccess)
			goto exit;

		// stricmp ??
		if (strUsername.compare(lpszName) != 0) {
			// cannot open another user without password
			// or should be check permissions ?
			ec_log_warn("Single Sign-On: User \"%s\" authenticated, but user \"%s\" requested.", username, lpszName);
			er = ZARAFA_E_LOGON_FAILED;
		} else {
			m_bValidated = true;
			er = erSuccess;
			ec_log_info("Single Sign-On: User \"%s\" authenticated", username);
		}
	}

	*lppOutput = lpOutput;

exit:
	if (OutSecBuffer.pvBuffer)
		FreeContextBuffer(OutSecBuffer.pvBuffer);

	return er;
}

ECRESULT ECAuthSession::ValidateSSOData_KRB5(struct soap* soap, const char* lpszName, const char* szClientVersion, const char* szClientApp, const char *szClientAppVersion, const char *szClientAppMisc, const struct xsd__base64Binary* lpInput, struct xsd__base64Binary** lppOutput)
{
	return ZARAFA_E_LOGON_FAILED;
}

ECRESULT ECAuthSession::ValidateSSOData_NTLM(struct soap* soap, const char* lpszName, const char* szClientVersion, const char* szClientApp, const char *szClientAppVersion, const char *szClientAppMisc, const struct xsd__base64Binary* lpInput, struct xsd__base64Binary **lppOutput)
{
	return ZARAFA_E_LOGON_FAILED;
}
#endif

ECRESULT ECAuthSession::ProcessImpersonation(const char* lpszImpersonateUser)
{
	ECRESULT er = erSuccess;

	if (lpszImpersonateUser == NULL || *lpszImpersonateUser == '\0') {
		m_ulImpersonatorID = EC_NO_IMPERSONATOR;
		goto exit;
	}

	m_ulImpersonatorID = m_ulUserID;
	er = m_lpUserManagement->ResolveObjectAndSync(OBJECTCLASS_USER, lpszImpersonateUser, &m_ulUserID);

exit:
	return er;
}

size_t ECAuthSession::GetObjectSize()
{
	size_t ulSize = sizeof(*this);

	return ulSize;
}


ECAuthSessionOffline::ECAuthSessionOffline(const char *src_addr,
    ECSESSIONID sessionID, ECDatabaseFactory *lpDatabaseFactory,
    ECSessionManager *lpSessionManager, unsigned int ulCapabilities) :
	ECAuthSession(src_addr, sessionID, lpDatabaseFactory, lpSessionManager,
	    ulCapabilities)
{
	// nothing todo
}

ECRESULT
ECAuthSessionOffline::CreateECSession(ECSESSIONGROUPID ecSessionGroupId,
    const std::string &cl_ver, const std::string &cl_app,
    const std::string &cl_app_ver, const std::string &cl_app_misc,
    ECSESSIONID *sessionID, ECSession **lppNewSession)
{
	ECRESULT er = erSuccess;
	ECSession *lpSession = NULL;
	ECSESSIONID newSID;

	if (!m_bValidated) {
		er = ZARAFA_E_LOGON_FAILED;
		goto exit;
	}

	CreateSessionID(m_ulClientCapabilities, &newSID);

	// Offline version
	lpSession = new(std::nothrow) ECSession(m_strSourceAddr.c_str(), newSID,
	            ecSessionGroupId, m_lpDatabaseFactory, m_lpSessionManager,
	            m_ulClientCapabilities, true, m_ulValidationMethod,
	            m_ulConnectingPid, cl_ver, cl_app, cl_app_ver, cl_app_misc);
	if (!lpSession) {
		er = ZARAFA_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}

	er = lpSession->GetSecurity()->SetUserContext(m_ulUserID, m_ulImpersonatorID);
	if (er != erSuccess)
		goto exit;				// user not found anymore, or error in getting groups

	*sessionID = newSID;
	*lppNewSession = lpSession;

exit:
	if (er != erSuccess)
		delete lpSession;

	return er;
}
