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

// ECSessionGroup.h: interface for the ECSessionGroup class.
//
//////////////////////////////////////////////////////////////////////

#ifndef ECSESSIONGROUP
#define ECSESSIONGROUP

#include <zarafa/zcdefs.h>
#include <list>
#include <map>
#include <set>

#include <zarafa/ECKeyTable.h>
#include "ECNotification.h"
#include <zarafa/ZarafaCode.h>
#include <zarafa/CommonUtil.h>

class ECSession;
class ECSessionGroup;
class ECSessionManager;

struct sessionInfo {
	sessionInfo(ECSession *lpSession) : lpSession(lpSession) {}
	ECSession	 *lpSession;
};

typedef std::map<ECSESSIONID, sessionInfo> SESSIONINFOMAP;

struct subscribeItem {
	ECSESSIONID	 ulSession;		// Unique session identifier
	unsigned int ulConnection;	// Unique client identifier for notification
	unsigned int ulKey;			// database object id (also storeid) or a tableid
	unsigned int ulEventMask;
};

typedef std::list<ECNotification> NOTIFICATIONLIST;
typedef std::map<unsigned int, subscribeItem> SUBSCRIBEMAP;
typedef std::multimap<unsigned int, unsigned int> SUBSCRIBESTOREMULTIMAP;

struct changeSubscribeItem {
	ECSESSIONID		ulSession;
	unsigned int	ulConnection;
	notifySyncState	sSyncState;
};
typedef std::multimap<unsigned int, changeSubscribeItem> CHANGESUBSCRIBEMAP;	// SyncId -> changeSubscribeItem

class ECSessionGroup _zcp_final {
public:
	ECSessionGroup(ECSESSIONGROUPID sessionGroupId, ECSessionManager *lpSessionManager);
	virtual ~ECSessionGroup();

	/*
	 * Thread safety handlers
	 */
	virtual void Lock();
	virtual void Unlock();
	virtual bool IsLocked();

	/*
	 * Returns the SessionGroupId
	 */
	virtual ECSESSIONGROUPID GetSessionGroupId();

	/*
	 * Add/Remove Session from group
	 */
	virtual void AddSession(ECSession *lpSession);
	virtual void ShutdownSession(ECSession *lpSession);
	virtual void ReleaseSession(ECSession *lpSession);

	/*
	 * Update session time for all attached sessions
	 */
	virtual void UpdateSessionTime();

	/*
	 * Check is SessionGroup has lost all its children
	 */
	virtual bool isOrphan();

	/*
	 * Item subscription
	 */
	virtual ECRESULT AddAdvise(ECSESSIONID ulSessionId, unsigned int ulConnection, unsigned int ulKey, unsigned int ulEventMask);
	virtual ECRESULT AddChangeAdvise(ECSESSIONID ulSessionId, unsigned int ulConnection, notifySyncState *lpSyncState);
	virtual ECRESULT DelAdvise(ECSESSIONID ulSessionId, unsigned int ulConnection);

	/*
	 * Notifications
	 */
	virtual ECRESULT AddNotification(notification *notifyItem, unsigned int ulKey, unsigned int ulStore, ECSESSIONID ulSessionId = 0);
	virtual ECRESULT AddNotificationTable(ECSESSIONID ulSessionId, unsigned int ulType, unsigned int ulObjType, unsigned int ulTableId,
										  sObjectTableKey* lpsChildRow, sObjectTableKey* lpsPrevRow, struct propValArray *lpRow);
	virtual ECRESULT AddChangeNotification(const std::set<unsigned int> &syncIds, unsigned int ulChangeId, unsigned int ulChangeType);
	virtual ECRESULT AddChangeNotification(ECSESSIONID ulSessionId, unsigned int ulConnection, unsigned int ulSyncId, unsigned long ulChangeId);
	virtual ECRESULT GetNotifyItems(struct soap *soap, ECSESSIONID ulSessionId, struct notifyResponse *notifications);

	unsigned int GetObjectSize();

private:
	ECRESULT releaseListeners();

	/* Personal SessionGroupId */
	ECSESSIONGROUPID	m_sessionGroupId;

	/* All Sessions attached to this group */
	SESSIONINFOMAP		m_mapSessions;

	/* List of all items the group is subscribed to */
	SUBSCRIBEMAP        m_mapSubscribe;
	CHANGESUBSCRIBEMAP	m_mapChangeSubscribe;
	pthread_mutex_t		m_hSessionMapLock;

	/* Notifications */
	NOTIFICATIONLIST	m_listNotification;
	double				m_dblLastQueryTime;

	/* Notifications lock/event */
	pthread_mutex_t		m_hNotificationLock;
	pthread_cond_t      m_hNewNotificationEvent;
	ECSESSIONID			m_getNotifySession;

	/* Thread safety mutex/event */
	unsigned int		m_ulRefCount;
	pthread_mutex_t		m_hThreadReleasedMutex;
	pthread_cond_t		m_hThreadReleased;

	/* Set to TRUE if no more GetNextNotifyItems() should be done on this group since the main
	 * session has exited
	 */
	bool				m_bExit;
	
	/* Reference to the session manager needed to notify changes in our queue */
	ECSessionManager *	m_lpSessionManager;
	
	/* Multimap of subscriptions that we have (key -> store id) */
	SUBSCRIBESTOREMULTIMAP	m_mapSubscribedStores;
	pthread_mutex_t		m_mutexSubscribedStores;
	
private:
	// Make ECSessionGroup non-copyable
	ECSessionGroup(const ECSessionGroup &);
	ECSessionGroup& operator=(const ECSessionGroup &);
};

#endif // #ifndef ECSESSIONGROUP
