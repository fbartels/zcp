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

#ifndef ECNOTIFYCLIENT_H
#define ECNOTIFYCLIENT_H

#include <zarafa/ECUnknown.h>
#include <IECChangeAdviseSink.h>

#include "ECICS.h"
#include "ECNotifyMaster.h"

#include <pthread.h>
#include <map>
#include <list>
#include <mapispi.h>

typedef struct {
	ULONG				cbKey;
	LPBYTE				lpKey;
	ULONG				ulEventMask;
	LPMAPIADVISESINK	lpAdviseSink;
	ULONG				ulConnection;
	GUID				guid;
	ULONG				ulSupportConnection;
} ECADVISE;

typedef struct {
	ULONG					ulSyncId;
	ULONG					ulChangeId;
	ULONG					ulEventMask;
	IECChangeAdviseSink *lpAdviseSink;
	ULONG					ulConnection;
	GUID					guid;
} ECCHANGEADVISE;

typedef std::map<int, ECADVISE*> ECMAPADVISE;
typedef std::map<int, ECCHANGEADVISE*> ECMAPCHANGEADVISE;
typedef std::list<std::pair<syncid_t,connection_t> > ECLISTCONNECTION;

class SessionGroupData;

class ECNotifyClient : public ECUnknown
{
protected:
	ECNotifyClient(ULONG ulProviderType, void *lpProvider, ULONG ulFlags, LPMAPISUP lpSupport);
	virtual ~ECNotifyClient();
public:
	static HRESULT Create(ULONG ulProviderType, void *lpProvider, ULONG ulFlags, LPMAPISUP lpSupport, ECNotifyClient**lppNotifyClient);

	virtual HRESULT QueryInterface(REFIID refiid, void **lppInterface);

	virtual HRESULT Advise(ULONG cbKey, LPBYTE lpKey, ULONG ulEventMask, LPMAPIADVISESINK lpAdviseSink, ULONG *lpulConnection);
	virtual HRESULT Advise(const ECLISTSYNCSTATE &lstSyncStates, IECChangeAdviseSink *lpChangeAdviseSink, ECLISTCONNECTION *lplstConnections);
	virtual HRESULT Unadvise(ULONG ulConnection);
	virtual HRESULT Unadvise(const ECLISTCONNECTION &lstConnections);

	// Re-request the connection from the server. You may pass an updated key if required.
	virtual HRESULT Reregister(ULONG ulConnection, ULONG cbKey = 0, LPBYTE lpKey = NULL);

	virtual HRESULT ReleaseAll();

	virtual HRESULT Notify(ULONG ulConnection, const NOTIFYLIST &lNotifications);
	virtual HRESULT NotifyChange(ULONG ulConnection, const NOTIFYLIST &lNotifications);
	virtual HRESULT NotifyReload(); // Called when all tables should be notified of RELOAD

	// Only register an advise client side
	virtual HRESULT RegisterAdvise(ULONG cbKey, LPBYTE lpKey, ULONG ulEventMask, bool bSynchronous, LPMAPIADVISESINK lpAdviseSink, ULONG *lpulConnection);
	virtual HRESULT RegisterChangeAdvise(ULONG ulSyncId, ULONG ulChangeId, IECChangeAdviseSink *lpChangeAdviseSink, ULONG *lpulConnection);
	virtual HRESULT UnRegisterAdvise(ULONG ulConnection);

	virtual HRESULT UpdateSyncStates(const ECLISTSYNCID &lstSyncID, ECLISTSYNCSTATE *lplstSyncState);

private:
	ECMAPADVISE				m_mapAdvise;		// Map of all advise request from the client (outlook)
	ECMAPCHANGEADVISE		m_mapChangeAdvise;	// ExchangeChangeAdvise(s)

	SessionGroupData*		m_lpSessionGroup;
	ECNotifyMaster*			m_lpNotifyMaster;
	WSTransport*			m_lpTransport;
	LPMAPISUP				m_lpSupport;

	void*					m_lpProvider;
	ULONG					m_ulProviderType;

	pthread_mutex_t			m_hMutex;
	pthread_mutexattr_t		m_hMutexAttrib;
	ECSESSIONGROUPID		m_ecSessionGroupId;
};

#endif // #ifndef ECNOTIFYCLIENT_H
