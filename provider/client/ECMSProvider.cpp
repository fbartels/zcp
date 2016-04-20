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

#include <zarafa/platform.h>

#include <memory.h>
#include <mapi.h>
#include <mapiutil.h>
#include <mapispi.h>

#include <zarafa/ECGetText.h>

#include "Mem.h"

#include <zarafa/ECGuid.h>

#include "ECMSProvider.h"
#include "ECMsgStore.h"
#include "ECABProvider.h"

#include <zarafa/ECDebug.h>


#include "ClientUtil.h"
#include "EntryPoint.h"

#include "WSUtil.h"
#include "ZarafaUtil.h"
#include "ProviderUtil.h"
#include <zarafa/stringutil.h>

using namespace std;

#include <edkguid.h>

#include <cwchar>
#include <zarafa/charset/convert.h>
#include <zarafa/charset/utf8string.h>

#include <zarafa/mapi_ptr/mapi_memory_ptr.h>
typedef mapi_memory_ptr<ECUSER>	ECUserPtr;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

ECMSProvider::ECMSProvider(ULONG ulFlags, const char *szClassName) :
    ECUnknown(szClassName)
{
	TRACE_MAPI(TRACE_ENTRY, "ECMSProvider::ECMSProvider","");
	
	m_ulFlags = ulFlags;
}

ECMSProvider::~ECMSProvider()
{
	TRACE_MAPI(TRACE_ENTRY, "ECMSProvider::~ECMSProvider","");
}

HRESULT ECMSProvider::Create(ULONG ulFlags, ECMSProvider **lppECMSProvider) {
	ECMSProvider *lpECMSProvider = new ECMSProvider(ulFlags, "IMSProvider");

	return lpECMSProvider->QueryInterface(IID_ECMSProvider, (void **)lppECMSProvider);
}

HRESULT ECMSProvider::QueryInterface(REFIID refiid, void **lppInterface)
{
	REGISTER_INTERFACE(IID_ECMSProvider, this);

	REGISTER_INTERFACE(IID_IMSProvider, &this->m_xMSProvider);

	REGISTER_INTERFACE(IID_ISelectUnicode, &this->m_xUnknown);

	return MAPI_E_INTERFACE_NOT_SUPPORTED;
}

HRESULT ECMSProvider::Shutdown(ULONG * lpulFlags) 
{
	return hrSuccess;
}

HRESULT ECMSProvider::Logon(LPMAPISUP lpMAPISup, ULONG ulUIParam, LPTSTR lpszProfileName, ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulFlags, LPCIID lpInterface, ULONG *lpcbSpoolSecurity, LPBYTE *lppbSpoolSecurity, LPMAPIERROR *lppMAPIError, LPMSLOGON *lppMSLogon, LPMDB *lppMDB)
{
	HRESULT			hr = hrSuccess;
	WSTransport*	lpTransport = NULL;
	ECMsgStore*		lpECMsgStore = NULL;
	ECMSLogon*		lpECMSLogon = NULL;

	LPPROFSECT		lpProfSect = NULL;
	ULONG			cValues = 0;
	LPSPropTagArray	lpsPropTagArray = NULL;
	LPSPropValue	lpsPropArray = NULL;
	BOOL			fIsDefaultStore = FALSE;
	ULONG			ulStoreType = 0;
	MAPIUID			guidMDBProvider;
	BOOL			bOfflineStore = FALSE;
	
	sGlobalProfileProps	sProfileProps;

#ifdef HAVE_OFFLINE_SUPPORT
	ULONG		cbUnWrapStoreID = 0;
	LPENTRYID	lpUnWrapStoreID = NULL;
	ULONG		cbRootId = 0;
	LPENTRYID	lpRootId = NULL;
	GUID		guidStore;
	BOOL		bFirstLogin = FALSE;
	BOOL		bFirstUserLogin = FALSE;
	ULONG		cbTmp = 0; // unused vars
	LPENTRYID	lpTmpId = NULL; // unused vars
	string strUniqueId;
	string strDBDirectory;
	string strDBConfigFile;
	std::string strOfflineServerPath;
	LPSTREAM lpTmpStream = NULL;

	ECUserPtr		ptrUser;
#endif

#if defined(WIN32) && !defined(WINCE)
	if(ulUIParam == 0)
		ulUIParam = (ULONG)GetWindow(GetDesktopWindow(), GW_CHILD);
#endif

	// Always suppress UI when running in a service
	if(m_ulFlags & MAPI_NT_SERVICE)
		ulFlags |= MDB_NO_DIALOG;

	// If the EntryID is not configured, return MAPI_E_UNCONFIGURED, this will
	// cause MAPI to call our configuration entry point (MSGServiceEntry)
	if(lpEntryID == NULL) {
		hr = MAPI_E_UNCONFIGURED;
		goto exit;
	}

	if(lpcbSpoolSecurity)
		*lpcbSpoolSecurity = 0;
	if(lppbSpoolSecurity)
		*lppbSpoolSecurity = NULL;

	// Get the username and password from the profile settings
	hr = ClientUtil::GetGlobalProfileProperties(lpMAPISup, &sProfileProps);
	if(hr != hrSuccess)
		goto exit;

	// Open profile settings
	hr = lpMAPISup->OpenProfileSection(NULL, MAPI_MODIFY, &lpProfSect);
	if(hr != hrSuccess)
		goto exit;

	cValues = 2;
	hr = MAPIAllocateBuffer(CbNewSPropTagArray(cValues), (void **)&lpsPropTagArray);
	if(hr != hrSuccess)
		goto exit;

	lpsPropTagArray->cValues = 2;
	lpsPropTagArray->aulPropTag[0] = PR_MDB_PROVIDER;
	lpsPropTagArray->aulPropTag[1] = PR_RESOURCE_FLAGS;
	
	hr = lpProfSect->GetProps(lpsPropTagArray, 0, &cValues, &lpsPropArray);
	if (FAILED(hr))
		goto exit;

	TRACE_MAPI(TRACE_ENTRY, "ECMSProvider::Logon::MDB", "PR_MDB_PROVIDER = %s", lpsPropArray[0].ulPropTag == PR_MDB_PROVIDER ? DBGGUIDToString(*(IID*)lpsPropArray[0].Value.bin.lpb).c_str() : "<Unknown>");

	if (lpsPropArray[1].ulPropTag == PR_RESOURCE_FLAGS && (lpsPropArray[1].Value.ul & STATUS_DEFAULT_STORE) == STATUS_DEFAULT_STORE)
		fIsDefaultStore = TRUE;

#ifdef HAVE_OFFLINE_SUPPORT
	// The only possible offline provider is default store(ZARAFA_SERVICE_GUID / fIsDefaultStore == TRUE)
	if ((m_ulFlags & EC_PROVIDER_OFFLINE) == EC_PROVIDER_OFFLINE && fIsDefaultStore == FALSE) {
		hr = MAPI_E_CALL_FAILED;
		ASSERT(FALSE);
		goto exit;
	}
#endif

	// Create a transport for this message store
	hr = WSTransport::Create(ulFlags, &lpTransport);
	if(hr != hrSuccess)
		goto exit;

#ifdef HAVE_OFFLINE_SUPPORT
	if ((m_ulFlags & EC_PROVIDER_OFFLINE) == EC_PROVIDER_OFFLINE && fIsDefaultStore == TRUE) { 
		if (!sProfileProps.strOfflinePath.empty())
			g_strUserLocalAppDataZarafa = sProfileProps.strOfflinePath;

		hr = CheckStartServerAndGetServerURL(lpMAPISup, g_strUserLocalAppDataZarafa.c_str(), g_strZarafaDirectory.c_str(), &strOfflineServerPath);		
		if (hr != hrSuccess)
			goto exit;

		// It's an offline store
		bOfflineStore = TRUE;
		sProfileProps.strServerPath = strOfflineServerPath;

		// Log on the transport to the server
		hr = lpTransport->HrLogon(sProfileProps);
	}
	else
#endif
	{
		hr = LogonByEntryID(&lpTransport, &sProfileProps, cbEntryID, lpEntryID);
	}

	if (lpsPropArray[0].ulPropTag == PR_MDB_PROVIDER) {
		memcpy(&guidMDBProvider, lpsPropArray[0].Value.bin.lpb, sizeof(MAPIUID));
	} else if (fIsDefaultStore == FALSE){
		// also fallback to private store when logon failed (hr, do not change)
		if(hr != hrSuccess || lpTransport->HrGetStoreType(cbEntryID, lpEntryID, &ulStoreType) != hrSuccess) {
			// Maintain backward-compat: if connecting to a server that does not support the storetype
			// call, assume private store, which is what happened before this call was introduced
			ulStoreType = ECSTORE_TYPE_PRIVATE;
		}

		if (ulStoreType == ECSTORE_TYPE_PRIVATE)
			memcpy(&guidMDBProvider, &ZARAFA_STORE_DELEGATE_GUID, sizeof(MAPIUID));
		else if (ulStoreType == ECSTORE_TYPE_PUBLIC)
			memcpy(&guidMDBProvider, &ZARAFA_STORE_PUBLIC_GUID, sizeof(MAPIUID));
		else if (ulStoreType == ECSTORE_TYPE_ARCHIVE)
			memcpy(&guidMDBProvider, &ZARAFA_STORE_ARCHIVE_GUID, sizeof(MAPIUID));
		else {
			ASSERT(FALSE);
			hr = MAPI_E_NO_SUPPORT;
			goto exit;
		}
	} else {
		memcpy(&guidMDBProvider, &ZARAFA_SERVICE_GUID, sizeof(MAPIUID));
	}
	TRACE_MAPI(TRACE_ENTRY, "ECMSProvider::Logon::MDB", "PR_MDB_PROVIDER = %s", DBGGUIDToString(*(IID*)&guidMDBProvider).c_str());

#ifdef HAVE_OFFLINE_SUPPORT
	if ( hr == hrSuccess && (m_ulFlags&EC_PROVIDER_OFFLINE) == EC_PROVIDER_OFFLINE && CompareMDBProvider(&guidMDBProvider, &ZARAFA_SERVICE_GUID))
	{
		// check if there is already a store
		hr = lpTransport->HrGetStore(cbEntryID, lpEntryID, &cbTmp, &lpTmpId, 0 , NULL);
		
		if(lpTmpId) {
			ECFreeBuffer(lpTmpId);
			lpTmpId = NULL;
		}

	}else if(hr == MAPI_E_LOGON_FAILED){
		bFirstUserLogin = TRUE;
	}else if(hr == MAPI_E_SESSION_LIMIT) {
		if(!(ulFlags & MDB_NO_DIALOG)) {
			MessageBox((HWND)ulUIParam, _("Cannot use the profile because you are over the license limit. To continue, you must purchase additional client licenses."), g_strProductName.c_str(), MB_ICONEXCLAMATION | MB_OK);
		}
	}else if(hr == MAPI_E_NO_ACCESS) {
		if(!(ulFlags & MDB_NO_DIALOG)) {
			MessageBox((HWND)ulUIParam, _("Cannot use the profile because the server was unable to contact the license server. Please consult your system administrator."), g_strProductName.c_str(), MB_ICONEXCLAMATION | MB_OK);
		}
	}

	// As this the first time you logon, you must be an administrator
	if (hr != hrSuccess && (m_ulFlags&EC_PROVIDER_OFFLINE) == EC_PROVIDER_OFFLINE && CompareMDBProvider(&guidMDBProvider, &ZARAFA_SERVICE_GUID))
	{
		sGlobalProfileProps sLocalServerProfileProps;
		sGlobalProfileProps sOnlineProfileProps;

		hr = ClientUtil::GetGlobalProfileProperties(lpMAPISup, &sLocalServerProfileProps);
		if(hr != hrSuccess)
			goto exit;

		sOnlineProfileProps = sLocalServerProfileProps;

		sLocalServerProfileProps.strServerPath = strOfflineServerPath;
		sLocalServerProfileProps.strUserName = ZARAFA_SYSTEM_USER_W;
		sLocalServerProfileProps.strPassword = ZARAFA_SYSTEM_USER_W;

		// Log off the user
		lpTransport->HrLogOff();

		// Login as admin
		hr = lpTransport->HrLogon(sLocalServerProfileProps);
		if(hr != hrSuccess)
			goto exit; // Only when the offline server is killed on a bad moment

		// Check if it's a new offline profile
		hr = lpTransport->HrGetStore(cbEntryID, lpEntryID, &cbTmp, &lpTmpId, 0 , NULL);
		if(hr == MAPI_E_NOT_FOUND)
			bFirstLogin = TRUE;
		else if(hr != hrSuccess)
			goto exit;

		if(bFirstLogin == TRUE)
		{
			lpTransport->HrLogOff();

			hr = lpTransport->HrLogon(sOnlineProfileProps);
			if(hr != hrSuccess)
				goto exit;

			hr = lpTransport->HrGetUser(0, NULL, MAPI_UNICODE, &ptrUser);
			if(hr != hrSuccess)
				goto exit;

			lpTransport->HrLogOff();

			hr = lpTransport->HrLogon(sLocalServerProfileProps);
			if(hr != hrSuccess)
				goto exit; // Only when the offline server is killed on a bad moment

			if(bFirstUserLogin){
				// Add user to offline store
				ptrUser->lpszPassword = (LPTSTR)L"dummy";
				hr = lpTransport->HrSetUser(ptrUser, MAPI_UNICODE);
				if(hr != hrSuccess)
					goto exit;
			}

			// Create new offline store
			//

			hr = UnWrapServerClientStoreEntry(cbEntryID, lpEntryID, &cbUnWrapStoreID,&lpUnWrapStoreID);
			if(hr != hrSuccess)
				goto exit;
			
			hr = HrGetStoreGuidFromEntryId(cbUnWrapStoreID, (LPBYTE)lpUnWrapStoreID, &guidStore);
			if(hr != hrSuccess)
				goto exit;

			// create root entryid
			hr = HrCreateEntryId(guidStore, MAPI_FOLDER, &cbRootId, &lpRootId);
			if(hr != hrSuccess)
				goto exit;
			
			hr = lpTransport->HrCreateStore(ECSTORE_TYPE_PRIVATE, ptrUser->sUserId.cb, (LPENTRYID)ptrUser->sUserId.lpb, cbUnWrapStoreID, lpUnWrapStoreID, cbRootId, lpRootId, 0);
			if(hr != hrSuccess)
				goto exit;

			hr = lpTransport->HrSetReceiveFolder(cbUnWrapStoreID, lpUnWrapStoreID, utf8string::from_string("IPC"), cbRootId, lpRootId);
			if(hr != hrSuccess)
				goto exit;
		} // if(bFirstLogin == TRUE)

		// Log off the admin store
		lpTransport->HrLogOff();

		// Login with the user
		hr = lpTransport->HrLogon(sProfileProps);

	} // if (hr != hrSuccess .....)

#endif

	if(hr != hrSuccess)
		goto exit;

	// Get a message store object
	hr = CreateMsgStoreObject((LPSTR)sProfileProps.strProfileName.c_str(), lpMAPISup, cbEntryID, lpEntryID, ulFlags, sProfileProps.ulProfileFlags, lpTransport,
							&guidMDBProvider, false, fIsDefaultStore, bOfflineStore,
							&lpECMsgStore);
	if(hr != hrSuccess)
		goto exit;

#ifdef HAVE_OFFLINE_SUPPORT
	// We now have a store. We'll now check for EC_AB_SYNC_STATUS in order to set the correct
	// user. If it is missing, we are dealing with an upgrade that forced a resync in order to upgrade
	// the extern ids. We need to upgrade ourselve now to get the correct owner property on the store.
	if (!bFirstUserLogin && bOfflineStore) {
		sGlobalProfileProps sLocalServerProfileProps;
		sGlobalProfileProps sOnlineProfileProps;

		hr = ClientUtil::GetGlobalProfileProperties(lpMAPISup, &sLocalServerProfileProps);
		if(hr != hrSuccess)
			goto exit;

		sOnlineProfileProps = sLocalServerProfileProps;

		sLocalServerProfileProps.strServerPath = strOfflineServerPath;
		sLocalServerProfileProps.strUserName = ZARAFA_SYSTEM_USER_W;
		sLocalServerProfileProps.strPassword = ZARAFA_SYSTEM_USER_W;

		hr = lpECMsgStore->OpenProperty(PR_EC_AB_SYNC_STATUS, &IID_IStream, 0, 0, (IUnknown**)&lpTmpStream);
		if (lpTmpStream) { lpTmpStream->Release(); lpTmpStream = NULL; }

		if (hr == MAPI_E_NOT_FOUND) {
			// Destroy the just opened store.
			lpECMsgStore->Release();
			lpECMsgStore = NULL;

			// Get the online user information.
			hr = lpTransport->HrLogon(sOnlineProfileProps);
			if(hr != hrSuccess)
				goto exit;

			hr = lpTransport->HrGetUser(0, NULL, MAPI_UNICODE, &ptrUser);
			if(hr != hrSuccess)
				goto exit;

			lpTransport->HrLogOff();

			// Store the information offline.
			hr = lpTransport->HrLogon(sLocalServerProfileProps);
			if(hr != hrSuccess)
				goto exit; // Only when the offline server is killed on a bad moment

			// Add user to offline store
			ptrUser->lpszPassword = (LPTSTR)L"dummy";
			hr = lpTransport->HrSetUser(ptrUser, MAPI_UNICODE);
			if(hr != hrSuccess)
				goto exit;

			// Log off the admin store
			lpTransport->HrLogOff();

			// Recreate the store object with the user
			hr = lpTransport->HrLogon(sProfileProps);
			if (hr != hrSuccess)
				goto exit;

			hr = CreateMsgStoreObject((LPSTR)sProfileProps.strProfileName.c_str(), lpMAPISup, cbEntryID, lpEntryID, ulFlags, sProfileProps.ulProfileFlags, lpTransport,
									&guidMDBProvider, false, fIsDefaultStore, bOfflineStore,
									&lpECMsgStore);
			if(hr != hrSuccess)
				goto exit;
		}
	}
#endif
	
	// Register ourselves with mapisupport
	//hr = lpMAPISup->SetProviderUID((MAPIUID *)&lpMsgStore->GetStoreGuid(), 0); 
	//if(hr != hrSuccess)
	//	goto exit;

	
	// Return the variables
	if(lppMDB) { 
		hr = lpECMsgStore->QueryInterface(IID_IMsgStore, (void **)lppMDB);

		if(hr != hrSuccess)
			goto exit;

	}

	// We don't count lpMSLogon as a child, because its lifetime is coupled to lpMsgStore
	if(lppMSLogon) {
		hr = ECMSLogon::Create(lpECMsgStore, &lpECMSLogon);

		if(hr != hrSuccess)
			goto exit;
			
		hr = lpECMSLogon->QueryInterface(IID_IMSLogon, (void **)lppMSLogon);
		
		if(hr != hrSuccess)
			goto exit;
	}

exit:
	if(lpProfSect)
		lpProfSect->Release();

	if(lpECMsgStore)
		lpECMsgStore->Release();
		
	if(lpECMSLogon)
		lpECMSLogon->Release();

	if(lpTransport)
		lpTransport->Release();
	MAPIFreeBuffer(lpsPropTagArray);
	MAPIFreeBuffer(lpsPropArray);
#ifdef HAVE_OFFLINE_SUPPORT
	if(lpTmpId)
		ECFreeBuffer(lpTmpId);

	if (lpTmpStream)
		lpTmpStream->Release();
#endif

	return hr;
}

//FIXME: What todo with offline??
//TODO: online/offline state???
HRESULT ECMSProvider::SpoolerLogon(LPMAPISUP lpMAPISup, ULONG ulUIParam, LPTSTR lpszProfileName, ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulFlags, LPCIID lpInterface, ULONG cbSpoolSecurity, LPBYTE lpbSpoolSecurity, LPMAPIERROR *lppMAPIError, LPMSLOGON *lppMSLogon, LPMDB *lppMDB)
{
	HRESULT hr = hrSuccess;
	WSTransport *lpTransport = NULL;
	ECMsgStore *lpMsgStore = NULL;
	ECMSLogon *lpLogon = NULL;
	MAPIUID	guidMDBProvider;

	LPPROFSECT lpProfSect = NULL;
	ULONG cValues = 0;
	LPSPropTagArray lpsPropTagArray = NULL;
	LPSPropValue lpsPropArray = NULL;
	bool bOfflineStore = false;
	sGlobalProfileProps	sProfileProps;
	wchar_t *strSep = NULL;


	if(lpEntryID == NULL) {
		hr = MAPI_E_UNCONFIGURED;
		goto exit;
	}
	
	if(cbSpoolSecurity == 0 || lpbSpoolSecurity == NULL) {
		hr = MAPI_E_NO_ACCESS;
		goto exit;
	}

	// Get Global profile settings
	hr = ClientUtil::GetGlobalProfileProperties(lpMAPISup, &sProfileProps);
	if(hr != hrSuccess)
		goto exit;

	// Open profile settings
	hr = lpMAPISup->OpenProfileSection(NULL, MAPI_MODIFY, &lpProfSect);
	if(hr != hrSuccess)
		goto exit;

	ECAllocateBuffer(CbNewSPropTagArray(2), (void **)&lpsPropTagArray);

	lpsPropTagArray->cValues = 2;
	lpsPropTagArray->aulPropTag[0] = PR_MDB_PROVIDER;
	lpsPropTagArray->aulPropTag[1] = PR_RESOURCE_FLAGS;

	// Get the MDBProvider from the profile settings
	hr = lpProfSect->GetProps(lpsPropTagArray, 0, &cValues, &lpsPropArray);
	if(hr == hrSuccess || hr == MAPI_W_ERRORS_RETURNED)
	{
		if(lpsPropArray[0].ulPropTag == PR_MDB_PROVIDER){
				memcpy(&guidMDBProvider, lpsPropArray[0].Value.bin.lpb, sizeof(MAPIUID));
		}
		if(lpsPropArray[1].ulPropTag == PR_RESOURCE_FLAGS) {
			if(!(lpsPropArray[1].Value.ul & STATUS_DEFAULT_STORE)) {
				hr = MAPI_E_NOT_FOUND; // Deny spooler logon to any store that is not the default store
				goto exit;
			}
		}
	}


	if (cbSpoolSecurity % sizeof(wchar_t) != 0) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}
	
	strSep = (wchar_t*)wmemchr((wchar_t*)lpbSpoolSecurity, 0, cbSpoolSecurity / sizeof(wchar_t));
	if (strSep == NULL) {
		hr = MAPI_E_NO_ACCESS;
		goto exit;
	}
	++strSep;
	sProfileProps.strUserName = (wchar_t*)lpbSpoolSecurity;
	sProfileProps.strPassword = strSep;

	// Create a transport for this message store
	hr = WSTransport::Create(ulFlags, &lpTransport);

	if(hr != hrSuccess)
		goto exit;

#ifdef HAVE_OFFLINE_SUPPORT
	if ( (m_ulFlags&EC_PROVIDER_OFFLINE) == EC_PROVIDER_OFFLINE) { 

		hr = GetOfflineServerURL(lpMAPISup, &sProfileProps.strServerPath);
		if(hr != hrSuccess) {
			hr = MAPI_E_FAILONEPROVIDER;
			goto exit;
		}
		bOfflineStore = true;

		// Log on the transport to the server
		hr = lpTransport->HrLogon(sProfileProps);
	} else
#endif
	{
		hr = LogonByEntryID(&lpTransport, &sProfileProps, cbEntryID, lpEntryID);
	}
	
	if(hr != hrSuccess) {
		if(ulFlags & MDB_NO_DIALOG) {
			hr = MAPI_E_FAILONEPROVIDER;
			goto exit;
		} else {
			hr = MAPI_E_UNCONFIGURED;
			goto exit;
		}
	}

	// Get a message store object
	hr = CreateMsgStoreObject((LPSTR)sProfileProps.strProfileName.c_str(), lpMAPISup, cbEntryID, lpEntryID, ulFlags, sProfileProps.ulProfileFlags, lpTransport,
								&guidMDBProvider, true, true, bOfflineStore, 
								&lpMsgStore);
	if(hr != hrSuccess)
		goto exit;


	// Register ourselves with mapisupport
	//guidStore = lpMsgStore->GetStoreGuid();
	//hr = lpMAPISup->SetProviderUID((MAPIUID *)&guidStore, 0); 
	//if(hr != hrSuccess)
	//	goto exit;

	// Return the variables
	if(lppMDB) {
		hr = lpMsgStore->QueryInterface(IID_IMsgStore, (void **)lppMDB);

		if(hr != hrSuccess)
			goto exit;
	}

	if(lppMSLogon) {
		hr = ECMSLogon::Create(lpMsgStore, &lpLogon);
		if(hr != hrSuccess)
			goto exit;
			
		hr = lpLogon->QueryInterface(IID_IMSLogon, (void **)lppMSLogon);
		if(hr != hrSuccess)
			goto exit;
	}

exit:
	if(lpProfSect)
		lpProfSect->Release();

	if(lpMsgStore)
		lpMsgStore->Release();
		
	if(lpLogon)
		lpLogon->Release();

	if(lpTransport)
		lpTransport->Release();

	return hr;
}
	
HRESULT ECMSProvider::CompareStoreIDs(ULONG cbEntryID1, LPENTRYID lpEntryID1, ULONG cbEntryID2, LPENTRYID lpEntryID2, ULONG ulFlags, ULONG *lpulResult)
{
	return ::CompareStoreIDs(cbEntryID1, lpEntryID1, cbEntryID2, lpEntryID2, ulFlags, lpulResult);
}

/**
 * Log a WSTransport object on to a server based on the URL found in the provided store entryid.
 *
 * @param[in,out]	lppTransport		Double pointer to the WSTransport object that is to be logged on. This
 *										is a double pointer because a new WSTransport object will be returned if
 *										the entryid contains a pseudo URL that is resolved to another server than
 *										the server specified in the profile.
 * @param[in,out]	lpsProfileProps		The profile properties used to connect to logon to the server. If the
 *										entryid doesn't contain a pseudo URL, the serverpath in the profile properties
 *										will be updated to contain the path extracted from the entryid.
 * @param[in]		cbEntryID			The length of the passed entryid in bytes.
 * @param[in]		lpEntryID			Pointer to the store entryid from which the server path will be extracted.
 *
 * @retval	MAPI_E_FAILONEPROVIDER		Returned when the extraction of the URL failed.
 */
HRESULT ECMSProvider::LogonByEntryID(WSTransport **lppTransport, sGlobalProfileProps *lpsProfileProps, ULONG cbEntryID, LPENTRYID lpEntryID)
{
	HRESULT hr;
	string		extractedServerPath;		// The extracted server path
	bool		bIsPseudoUrl = false;
	WSTransport	*lpTransport = NULL;

	ASSERT(lppTransport && *lppTransport);
	lpTransport = *lppTransport;

	hr = HrGetServerURLFromStoreEntryId(cbEntryID, lpEntryID, extractedServerPath, &bIsPseudoUrl);
	if (hr != hrSuccess)
		return MAPI_E_FAILONEPROVIDER;

	// Log on the transport to the server
	if (!bIsPseudoUrl) {
		sGlobalProfileProps sOtherProps = *lpsProfileProps;
		
		sOtherProps.strServerPath = extractedServerPath;
		hr = lpTransport->HrLogon(sOtherProps);
		if(hr != hrSuccess) {
			// If we failed to open a non-pseudo-URL, fallback to using the server from the global
			// profile section. We need this because some older versions wrote a non-pseudo URL, which
			// we should still support - even when the hostname of the server changes for example.
			hr = lpTransport->HrLogon(*lpsProfileProps);
		}
	} else {
		string strServerPath;				// The resolved server path
		bool bIsPeer;
		WSTransport *lpAltTransport = NULL;

		hr = lpTransport->HrLogon(*lpsProfileProps);
		if (hr != hrSuccess)
			return hr;

		hr = HrResolvePseudoUrl(lpTransport, extractedServerPath.c_str(), strServerPath, &bIsPeer);
		if (hr != hrSuccess)
			return hr;

		if (!bIsPeer) {
			hr = lpTransport->CreateAndLogonAlternate(strServerPath.c_str(), &lpAltTransport);
			if (hr != hrSuccess)
				return hr;

			lpTransport->HrLogOff();
			lpTransport->Release();
			*lppTransport = lpAltTransport;
		}
	}
	return hrSuccess;
}

ULONG ECMSProvider::xMSProvider::AddRef() 
{
	TRACE_MAPI(TRACE_ENTRY, "IMSProvider::AddRef", "");
	METHOD_PROLOGUE_(ECMSProvider, MSProvider);
	return pThis->AddRef();
}

ULONG ECMSProvider::xMSProvider::Release()
{
	TRACE_MAPI(TRACE_ENTRY, "IMSProvider::Release", "");
	METHOD_PROLOGUE_(ECMSProvider, MSProvider);
	return pThis->Release();
}

HRESULT ECMSProvider::xMSProvider::QueryInterface(REFIID refiid, void **lppInterface)
{
	TRACE_MAPI(TRACE_ENTRY, "IMSProvider::QueryInterface", "%s", DBGGUIDToString(refiid).c_str());
	METHOD_PROLOGUE_(ECMSProvider, MSProvider);
	HRESULT hr = pThis->QueryInterface(refiid, lppInterface);
	TRACE_MAPI(TRACE_RETURN, "IMSProvider::QueryInterface", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ECMSProvider::xMSProvider::Shutdown(ULONG *lpulFlags)
{
	TRACE_MAPI(TRACE_ENTRY, "IMSProvider::Shutdown", "");
	METHOD_PROLOGUE_(ECMSProvider, MSProvider);
	return pThis->Shutdown(lpulFlags);
}

HRESULT ECMSProvider::xMSProvider::Logon(LPMAPISUP lpMAPISup, ULONG ulUIParam, LPTSTR lpszProfileName, ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulFlags, LPCIID lpInterface, ULONG *lpcbSpoolSecurity, LPBYTE *lppbSpoolSecurity, LPMAPIERROR *lppMAPIError, LPMSLOGON *lppMSLogon, LPMDB *lppMDB)
{
	TRACE_MAPI(TRACE_ENTRY, "IMSProvider::Logon", "flags=%x, cbEntryID=%d, lpEntryid=%s", ulFlags, cbEntryID, lpEntryID ? bin2hex(cbEntryID, (LPBYTE)lpEntryID).c_str() : "NULL");
	METHOD_PROLOGUE_(ECMSProvider, MSProvider);
	HRESULT hr = pThis->Logon(lpMAPISup, ulUIParam, lpszProfileName, cbEntryID, lpEntryID,ulFlags, lpInterface, lpcbSpoolSecurity, lppbSpoolSecurity, lppMAPIError, lppMSLogon, lppMDB);
	TRACE_MAPI(TRACE_RETURN, "IMSProvider::Logon", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ECMSProvider::xMSProvider::SpoolerLogon(LPMAPISUP lpMAPISup, ULONG ulUIParam, LPTSTR lpszProfileName, ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulFlags, LPCIID lpInterface, ULONG lpcbSpoolSecurity, LPBYTE lppbSpoolSecurity, LPMAPIERROR *lppMAPIError, LPMSLOGON *lppMSLogon, LPMDB *lppMDB)
{
	TRACE_MAPI(TRACE_ENTRY, "IMSProvider::SpoolerLogon", "flags=%x", ulFlags);
	METHOD_PROLOGUE_(ECMSProvider, MSProvider);
	HRESULT hr = pThis->SpoolerLogon(lpMAPISup, ulUIParam, lpszProfileName, cbEntryID, lpEntryID,ulFlags, lpInterface, lpcbSpoolSecurity, lppbSpoolSecurity, lppMAPIError, lppMSLogon, lppMDB);
	TRACE_MAPI(TRACE_RETURN, "IMSProvider::SpoolerLogon", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ECMSProvider::xMSProvider::CompareStoreIDs(ULONG cbEntryID1, LPENTRYID lpEntryID1, ULONG cbEntryID2, LPENTRYID lpEntryID2, ULONG ulFlags, ULONG *lpulResult)
{
	TRACE_MAPI(TRACE_ENTRY, "IMSProvider::CompareStoreIDs", "flags: %d\ncb=%d  entryid1: %s\n cb=%d entryid2: %s", ulFlags, cbEntryID1, bin2hex(cbEntryID1, (BYTE*)lpEntryID1).c_str(), cbEntryID2, bin2hex(cbEntryID2, (BYTE*)lpEntryID2).c_str());
	METHOD_PROLOGUE_(ECMSProvider, MSProvider);
	HRESULT hr = pThis->CompareStoreIDs(cbEntryID1, lpEntryID1, cbEntryID2, lpEntryID2, ulFlags, lpulResult);
	TRACE_MAPI(TRACE_RETURN, "IMSProvider::CompareStoreIDs", "%s %s", GetMAPIErrorDescription(hr).c_str(), (*lpulResult == TRUE)?"TRUE": "FALSE");
	return hr;
}

