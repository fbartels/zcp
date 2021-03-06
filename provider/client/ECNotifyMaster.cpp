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

#include <algorithm>

#include <mapidefs.h>

#include "ECNotifyClient.h"
#include "ECNotifyMaster.h"
#include "ECSessionGroupManager.h"
#include <zarafa/stringutil.h>
#include "SOAPUtils.h"
#include "WSTransport.h"

#ifdef LINUX
#include <sys/signal.h>
#include <sys/types.h>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember)) 

inline ECNotifySink::ECNotifySink(ECNotifyClient *lpClient, NOTIFYCALLBACK fnCallback)
	: m_lpClient(lpClient)
	, m_fnCallback(fnCallback)
{ }

inline HRESULT ECNotifySink::Notify(ULONG ulConnection,
    const NOTIFYLIST &lNotifications) const
{
	return CALL_MEMBER_FN(*m_lpClient, m_fnCallback)(ulConnection, lNotifications);
}

inline bool ECNotifySink::IsClient(const ECNotifyClient *lpClient) const
{
	return lpClient == m_lpClient;
}

ECNotifyMaster::ECNotifyMaster(SessionGroupData *lpData)
{
	TRACE_NOTIFY(TRACE_ENTRY, "ECNotifyMaster::ECNotifyMaster", "");

	/* Initialize threading */
	pthread_mutexattr_init(&m_hMutexAttrib);
	pthread_mutexattr_settype(&m_hMutexAttrib, PTHREAD_MUTEX_RECURSIVE);

	pthread_mutex_init(&m_hMutex, &m_hMutexAttrib);

	pthread_attr_init(&m_hAttrib);
	m_bThreadRunning = false;
	m_bThreadExit = false;

	/* Initialize connection */
	m_lpSessionGroupData = lpData; /* DON'T AddRef() */
	m_lpTransport = NULL;
	m_ulConnection = 1;

	TRACE_NOTIFY(TRACE_RETURN, "ECNotifyMaster::ECNotifyMaster", "");
}

ECNotifyMaster::~ECNotifyMaster(void)
{
	TRACE_NOTIFY(TRACE_ENTRY, "ECNotifyMaster::~ECNotifyMaster", "");

	ASSERT(m_listNotifyClients.empty());

	/* Disable Notifications */
	StopNotifyWatch();

	if (m_lpSessionGroupData)
		m_lpSessionGroupData = NULL; /* DON'T Release() */
	if (m_lpTransport)
		m_lpTransport->Release();

	pthread_mutex_destroy(&m_hMutex);
	pthread_mutexattr_destroy(&m_hMutexAttrib);

	pthread_attr_destroy(&m_hAttrib);

	TRACE_NOTIFY(TRACE_RETURN, "ECNotifyMaster::~ECNotifyMaster", "");
}

HRESULT ECNotifyMaster::Create(SessionGroupData *lpData, ECNotifyMaster **lppMaster)
{
	TRACE_NOTIFY(TRACE_ENTRY, "ECNotifyMaster::Create", "");

	HRESULT hr = hrSuccess;
	ECNotifyMaster *lpMaster = NULL;

	lpMaster = new ECNotifyMaster(lpData);
	lpMaster->AddRef();

	*lppMaster = lpMaster;

	TRACE_NOTIFY(TRACE_RETURN, "ECNotifyMaster::Create", "hr=0x%08X", hr);
	return hr;
}

HRESULT ECNotifyMaster::ConnectToSession()
{
	TRACE_NOTIFY(TRACE_ENTRY, "ECNotifyMaster::ConnectToSession", "");

	HRESULT			hr = hrSuccess;

	pthread_mutex_lock(&m_hMutex);

	/* This function can be called from NotifyWatch, and could race against StopNotifyWatch */
	if (m_bThreadExit) {
		hr = MAPI_E_END_OF_SESSION;
		goto exit;
	}

	/*
	 * Cancel connection IO operations before switching Transport.
	 */
	if (m_lpTransport) {
		hr = m_lpTransport->HrCancelIO();
		if (hr != hrSuccess)
			goto exit;
		m_lpTransport->Release();
		m_lpTransport = NULL;
	}

	/* Open notification transport */
	hr = m_lpSessionGroupData->GetTransport(&m_lpTransport);
	if (hr != hrSuccess)
		goto exit;

exit:
	pthread_mutex_unlock(&m_hMutex);

	TRACE_NOTIFY(TRACE_RETURN, "ECNotifyMaster::ConnectToSession", "hr=0x%08X", hr);
	return hr;
}

HRESULT ECNotifyMaster::AddSession(ECNotifyClient* lpClient)
{
	TRACE_NOTIFY(TRACE_ENTRY, "ECNotifyMaster::AddSession", "");

	pthread_mutex_lock(&m_hMutex);

	m_listNotifyClients.push_back(lpClient);

	/* Enable Notifications */
	if (StartNotifyWatch() != hrSuccess)
		ASSERT(FALSE);

	pthread_mutex_unlock(&m_hMutex);

	TRACE_NOTIFY(TRACE_RETURN, "ECNotifyMaster::AddSession", "");
	return hrSuccess;
}

struct findConnectionClient
{
	ECNotifyClient* lpClient;

	findConnectionClient(ECNotifyClient* lpClient) : lpClient(lpClient) {}

	bool operator()(const NOTIFYCONNECTIONCLIENTMAP::value_type &entry) const
	{
		return entry.second.IsClient(lpClient);
	}
};

HRESULT ECNotifyMaster::ReleaseSession(ECNotifyClient* lpClient)
{
	TRACE_NOTIFY(TRACE_ENTRY, "ECNotifyMaster::ReleaseSession", "");

	HRESULT hr = hrSuccess;
	NOTIFYCONNECTIONCLIENTMAP::iterator iter;
	
	pthread_mutex_lock(&m_hMutex);

	/* Remove all connections attached to client */
	iter = m_mapConnections.begin();
	while (true) {
		iter = find_if(iter, m_mapConnections.end(), findConnectionClient(lpClient));
		if (iter == m_mapConnections.end())
			break;
		m_mapConnections.erase(iter++);
	}

	/* Remove client from list */
	m_listNotifyClients.remove(lpClient);

	pthread_mutex_unlock(&m_hMutex);

	TRACE_NOTIFY(TRACE_RETURN, "ECNotifyMaster::ReleaseSession", "");
	return hr;
}

HRESULT ECNotifyMaster::ReserveConnection(ULONG *lpulConnection)
{
	pthread_mutex_lock(&m_hMutex);

	*lpulConnection = m_ulConnection;
	++m_ulConnection;
	pthread_mutex_unlock(&m_hMutex);

	return hrSuccess;
}

HRESULT ECNotifyMaster::ClaimConnection(ECNotifyClient* lpClient, NOTIFYCALLBACK fnCallback, ULONG ulConnection)
{
	pthread_mutex_lock(&m_hMutex);

	m_mapConnections.insert(NOTIFYCONNECTIONCLIENTMAP::value_type(ulConnection, ECNotifySink(lpClient, fnCallback)));

	pthread_mutex_unlock(&m_hMutex);

	return hrSuccess;
}

HRESULT ECNotifyMaster::DropConnection(ULONG ulConnection)
{
	pthread_mutex_lock(&m_hMutex);

	m_mapConnections.erase(ulConnection);

	pthread_mutex_unlock(&m_hMutex);

	return hrSuccess;
}

HRESULT ECNotifyMaster::StartNotifyWatch()
{
	TRACE_NOTIFY(TRACE_ENTRY, "ECNotifyMaster::StartNotifyWatch", "");

	HRESULT hr = hrSuccess;

	/* Thread is already running */
	if (m_bThreadRunning)
		goto exit;

	hr = ConnectToSession();
	if (hr != hrSuccess)
		goto exit;

	/* Make thread joinable which we need during shutdown */
	pthread_attr_setdetachstate(&m_hAttrib, PTHREAD_CREATE_JOINABLE);

#ifdef LINUX
	/* 1Mb of stack space per thread */
	if (pthread_attr_setstacksize(&m_hAttrib, 1024 * 1024)) {
		hr = MAPI_E_CALL_FAILED;
		goto exit;
	}
#endif

	if (pthread_create(&m_hThread, &m_hAttrib, NotifyWatch, (void *)this)) {
		hr = MAPI_E_CALL_FAILED;
		goto exit;
	}

	set_thread_name(m_hThread, "NotifyThread");

	m_bThreadRunning = TRUE;

exit:
	TRACE_NOTIFY(TRACE_RETURN, "ECNotifyMaster::StartNotifyWatch", "hr=0x%08X", hr);
	return hr;
}

HRESULT ECNotifyMaster::StopNotifyWatch()
{
	TRACE_NOTIFY(TRACE_ENTRY, "ECNotifyMaster::StopNotifyWatch", "");

	HRESULT hr = hrSuccess;
	WSTransport *lpTransport = NULL;

	/* Thread was already halted, or connection is broken */
	if (!m_bThreadRunning)
		goto exit;

	pthread_mutex_lock(&m_hMutex);

	/* Let the thread exit during its busy looping */
	m_bThreadExit = TRUE;

	if (m_lpTransport) {
		/* Get another transport so we can tell the server to end the session. We 
		 * can't use our own m_lpTransport since it is probably in a blocking getNextNotify()
		 * call. Seems like a bit of a shame to open an new connection, but there's no
		 * other option */
		hr = m_lpTransport->HrClone(&lpTransport);
		if (hr != hrSuccess) {
			pthread_mutex_unlock(&m_hMutex);
			goto exit;
		}
    
		lpTransport->HrLogOff();

		/* Cancel any pending IO if the network transport is down, causing the logoff to fail */
		m_lpTransport->HrCancelIO();
	}

	pthread_mutex_unlock(&m_hMutex);

	if (pthread_join(m_hThread, NULL) != 0)
		TRACE_NOTIFY(TRACE_WARNING, "ECNotifyMaster::StopNotifyWatch", "Invalid thread join");

	m_bThreadRunning = FALSE;

exit:
    if(lpTransport)
        lpTransport->Release();
        
	TRACE_NOTIFY(TRACE_RETURN, "ECNotifyMaster::StopNotifyWatch", "hr=0x%08X", hr);
	return hr;
}

void* ECNotifyMaster::NotifyWatch(void *pTmpNotifyMaster)
{
	TRACE_NOTIFY(TRACE_ENTRY, "NotifyWatch", "");

	ECNotifyMaster*		pNotifyMaster = (ECNotifyMaster *)pTmpNotifyMaster;
	ASSERT(pNotifyMaster != NULL);

	HRESULT							hr = hrSuccess;
	NOTIFYCONNECTIONMAP				mapNotifications;
	NOTIFYCONNECTIONMAP::iterator	iterNotifications;
	notifyResponse					notifications;
	bool							bReconnect = false;

#ifdef LINUX
	/* Ignore SIGPIPE which may be caused by HrGetNotify writing to the closed socket */
	signal(SIGPIPE, SIG_IGN);
#endif

	while (!pNotifyMaster->m_bThreadExit) {
		memset(&notifications, 0, sizeof(notifications));

		if (pNotifyMaster->m_bThreadExit)
			goto exit;

		/* 'exitable' sleep before reconnect */
		if (bReconnect) {
			for (ULONG i = 10; i > 0; --i) {
				Sleep(100);
				if (pNotifyMaster->m_bThreadExit)
					goto exit;
			}
		}

		/*
		 * Request notification (Blocking Call)
		 */
		notificationArray *pNotifyArray = NULL;

		hr = pNotifyMaster->m_lpTransport->HrGetNotify(&pNotifyArray);
		if (hr == ZARAFA_W_CALL_KEEPALIVE) {
			if (bReconnect) {
				TRACE_NOTIFY(TRACE_WARNING, "NotifyWatch::Reconnection", "OK connection: %d", pNotifyMaster->m_ulConnection);
				bReconnect = false;
			}
			continue;
		} else if (hr == MAPI_E_NETWORK_ERROR) {
			bReconnect = true;
			TRACE_NOTIFY(TRACE_WARNING, "NotifyWatch::Reconnection", "for connection: %d", pNotifyMaster->m_ulConnection);
			continue;
		} else if (hr != hrSuccess) {
			/*
			 * Session was killed by server, try to start a new login.
			 * This is not a foolproof recovery because 3 things might have happened:
			 *  1) WSTransport has been logged off during StopNotifyWatch().
			 *	2) Notification Session on server has died
			 *  3) SessionGroup on server has died
			 * If (1) m_bThreadExit will be set to TRUE, which means this thread is no longer desired. No
			 * need to make a big deal out of it.
			 * If (2) it is not a disaster (but it is a bad situation), the simple logon should do the trick
			 * of restoring the notification retreival for all sessions for this group. Some notifications
			 * might have arrived later then we might want, but that shouldn't be a total loss (the notificataions
			 * themselves will not have disappeared since they have been queued on the server).
			 * If (3) the problem is that _all_ sessions attached to the server has died and we have lost some
			 * notifications. The main issue however is that a new login for the notification session will not
			 * reanimate the other sessions belonging to this group and neither can we inform all ECNotifyClients
			 * that they now belong to a dead session. A new login is important however, when new sessions are
			 * attached to this group we must ensure that they will get notifications as expected.
			 */
			if (!pNotifyMaster->m_bThreadExit) {
				TRACE_NOTIFY(TRACE_WARNING, "NotifyWatch::End of session", "reconnect");
				while(pNotifyMaster->ConnectToSession() != hrSuccess && !pNotifyMaster->m_bThreadExit) {
					// On Windows, the ConnectToSession() takes a while .. the windows kernel does 3 connect tries
					// But on linux, this immediately returns a connection error when the server socket is closed
					// so we wait before we try again
					Sleep(1000);
				}
			}

			if (pNotifyMaster->m_bThreadExit)
				goto exit;
			else {
				NOTIFYCLIENTLIST::const_iterator iterNotifyClients;

				// We have a new session ID, notify reload

				pthread_mutex_lock(&pNotifyMaster->m_hMutex);

				for (iterNotifyClients = pNotifyMaster->m_listNotifyClients.begin();
				     iterNotifyClients != pNotifyMaster->m_listNotifyClients.end();
				     ++iterNotifyClients)
					(*iterNotifyClients)->NotifyReload();

				pthread_mutex_unlock(&pNotifyMaster->m_hMutex);
				continue;
            }
		}

		if (bReconnect) {
			TRACE_NOTIFY(TRACE_WARNING, "NotifyWatch::Reconnection", "OK connection: %d", pNotifyMaster->m_ulConnection);
			bReconnect = false;
		}

		/* This is when the connection is interupted */
		if (pNotifyArray == NULL)
			continue;

		TRACE_NOTIFY(TRACE_ENTRY, "NotifyWatch::GetNotify", "%d", pNotifyArray->__size);

		/*
		 * Loop through all notifications and sort them by connection number
		 * with these mappings we can later send all notifications per connection to the appropriate client.
		 */
		for (ULONG item = 0; item < pNotifyArray->__size; ++item) {
			ULONG ulConnection = pNotifyArray->__ptr[item].ulConnection;

			// No need to do a find before an insert with a default object.
			iterNotifications =
				mapNotifications.insert(NOTIFYCONNECTIONMAP::value_type(ulConnection, NOTIFYLIST())).first;

			iterNotifications->second.push_back(&pNotifyArray->__ptr[item]);
		}

		for (iterNotifications = mapNotifications.begin();
		     iterNotifications != mapNotifications.end();
		     ++iterNotifications)
		{
			NOTIFYCONNECTIONCLIENTMAP::const_iterator iterClient;

			/*
			 * Check if we have a client registered for this connection
			 * Be careful when locking this, Client->m_hMutex has priority over Master->m_hMutex
			 * which means we should NEVER call a Client function while holding the Master->m_hMutex!
			 */
			pthread_mutex_lock(&pNotifyMaster->m_hMutex);

			iterClient = pNotifyMaster->m_mapConnections.find(iterNotifications->first);
			if (iterClient == pNotifyMaster->m_mapConnections.end()) {
				pthread_mutex_unlock(&pNotifyMaster->m_hMutex);
				continue;
			}

			iterClient->second.Notify(iterNotifications->first, iterNotifications->second);

			/* All access to map completd, unlock mutex and send notification to client. */
			pthread_mutex_unlock(&pNotifyMaster->m_hMutex);
		}

		/* We're done, clean the map for next round */
		mapNotifications.clear();

		/* Cleanup */
		if (pNotifyArray != NULL) {
			FreeNotificationArrayStruct(pNotifyArray, true);
			pNotifyArray = NULL;
		}
	}

exit:
	TRACE_NOTIFY(TRACE_RETURN, "NotifyWatch", "");

	return NULL;
}

