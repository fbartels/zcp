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

#ifndef ECNOTIFYMASTER_H
#define ECNOTIFYMASTER_H

#include <list>
#include <map>
#include <pthread.h>

#include "ECSessionGroupManager.h"
#include <zarafa/ECUnknown.h>
#include <zarafa/ZarafaCode.h>

class ECNotifyClient;
class ECNotifyMaster;
class WSTransport;
class WSTransportNotify;
struct notification;

typedef std::list<notification*> NOTIFYLIST;
typedef HRESULT(ECNotifyClient::*NOTIFYCALLBACK)(ULONG,const NOTIFYLIST &);
class ECNotifySink {
public:
	ECNotifySink(ECNotifyClient *lpClient, NOTIFYCALLBACK fnCallback);
	HRESULT Notify(ULONG ulConnection, const NOTIFYLIST &lNotifications) const;
	bool IsClient(const ECNotifyClient *) const;

private:
	ECNotifyClient	*m_lpClient;
	NOTIFYCALLBACK	m_fnCallback;
};

typedef std::list<ECNotifyClient*> NOTIFYCLIENTLIST;
typedef std::map<ULONG, ECNotifySink> NOTIFYCONNECTIONCLIENTMAP;
typedef std::map<ULONG, NOTIFYLIST> NOTIFYCONNECTIONMAP;

class ECNotifyMaster : public ECUnknown
{
protected:
	ECNotifyMaster(SessionGroupData *lpData);
	virtual ~ECNotifyMaster(void);

public:
	static HRESULT Create(SessionGroupData *lpData, ECNotifyMaster **lppMaster);

	virtual HRESULT ConnectToSession();

	virtual HRESULT AddSession(ECNotifyClient* lpClient);
	virtual HRESULT ReleaseSession(ECNotifyClient* lpClient);

	virtual HRESULT ReserveConnection(ULONG *lpulConnection);
	virtual HRESULT ClaimConnection(ECNotifyClient* lpClient, NOTIFYCALLBACK fnCallback, ULONG ulConnection);
	virtual HRESULT DropConnection(ULONG ulConnection);

private:
	/* Threading functions */
	virtual HRESULT StartNotifyWatch();
	virtual HRESULT StopNotifyWatch();

	static void* NotifyWatch(void *pTmpNotifyClient);

	/* List of Clients attached to this master */
	NOTIFYCLIENTLIST			m_listNotifyClients;

	/* List of all connections, mapped to client */
	NOTIFYCONNECTIONCLIENTMAP	m_mapConnections;

	/* Connection settings */
	SessionGroupData*			m_lpSessionGroupData;
	WSTransport*				m_lpTransport;
	ULONG						m_ulConnection;

	/* Threading information */
	pthread_mutex_t				m_hMutex;
	pthread_mutexattr_t			m_hMutexAttrib;
	pthread_attr_t				m_hAttrib;
	pthread_t					m_hThread;
	BOOL						m_bThreadRunning;
	BOOL						m_bThreadExit;
};

#endif /* ECNOTIFYMASTER_H */

