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

#ifndef TABLEMANAGER_H
#define TABLEMANAGER_H

#include <zarafa/zcdefs.h>
#include <map>

#include "ECDatabase.h"
#include "ECGenericObjectTable.h"
#include <zarafa/ZarafaCode.h>


class ECSession;
class ECSessionManager;

/* 
 * The table manager is responsible for opening tables, and providing
 * access to open tables by a handle ID for each table, and updating tables when
 * a change is made to the underlying data and sending notifications to clients
 * which require table update notifications.
 *
 * Each session has its own table manager. This could be optimised in the future
 * by having one large table manager, with each table having multiple cursors for
 * each user that has the table open. This would save memory on overlapping tables
 * (probably resulting in around 30% less memory usage for the server).
 */

typedef struct {
	typedef enum { TABLE_TYPE_GENERIC, TABLE_TYPE_OUTGOINGQUEUE, TABLE_TYPE_MULTISTORE, TABLE_TYPE_USERSTORES,
		   TABLE_TYPE_SYSTEMSTATS, TABLE_TYPE_THREADSTATS, TABLE_TYPE_USERSTATS, TABLE_TYPE_SESSIONSTATS, TABLE_TYPE_COMPANYSTATS, TABLE_TYPE_SERVERSTATS,
			TABLE_TYPE_MAILBOX} TABLE_TYPE;
		   
    TABLE_TYPE ulTableType;
    
	union __sTable {
		struct __sGeneric {
			unsigned int ulParentId;
			unsigned int ulObjectType;
			unsigned int ulObjectFlags;
		} sGeneric ;
		struct __sOutgoingQueue {
			unsigned int ulStoreId;
			unsigned int ulFlags;
		} sOutgoingQueue;
	} sTable;
	ECGenericObjectTable *lpTable; // Actual table object
	unsigned int ulSubscriptionId; // Subscription ID for table event subscription on session manager
} TABLE_ENTRY;

typedef std::map<unsigned int, TABLE_ENTRY *> TABLEENTRYMAP;

class ECTableManager _zcp_final {
public:
	ECTableManager(ECSession *lpSession);
	~ECTableManager();

	ECRESULT	OpenGenericTable(unsigned int ulParent, unsigned int ulObjType, unsigned int ulFlags, unsigned int *lpulTableId, bool fLoad = true);
	ECRESULT	OpenOutgoingQueueTable(unsigned int ulStoreId, unsigned int *lpulTableId);
	ECRESULT	OpenABTable(unsigned int ulParent, unsigned int ulParentType, unsigned int ulObjType, unsigned int ulFlags, unsigned int *lpulTableId);
	ECRESULT	OpenMultiStoreTable(unsigned int ulObjType, unsigned int ulFlags, unsigned int *lpulTableId);
	ECRESULT	OpenUserStoresTable(unsigned int ulFlags, unsigned int *lpulTableId);
	ECRESULT	OpenStatsTable(unsigned int ulTableType, unsigned int ulFlags, unsigned int *lpulTableId);
	ECRESULT	OpenMailBoxTable(unsigned int ulflags, unsigned int *lpulTableId);

	ECRESULT	GetTable(unsigned int lpulTableId, ECGenericObjectTable **lppTable);
	ECRESULT	CloseTable(unsigned int lpulTableId);

	ECRESULT	UpdateOutgoingTables(ECKeyTable::UpdateType ulType, unsigned int ulStoreId, std::list<unsigned int> &lstObjId, unsigned int ulFlags, unsigned int ulObjType);
	ECRESULT	UpdateTables(ECKeyTable::UpdateType ulType, unsigned int ulFlags, unsigned int ulObjId, std::list<unsigned int> &lstChildId, unsigned int ulObjType);

	ECRESULT	GetStats(unsigned int *lpulTables, unsigned int *lpulObjectSize);

private:
	static	void *	SearchThread(void *lpParam);

	void		AddTableEntry(TABLE_ENTRY *lpEntry, unsigned int *lpulTableId);

	ECSession								*lpSession;
	TABLEENTRYMAP							mapTable;
	unsigned int							ulNextTableId;
	pthread_mutex_t							hListMutex;
};

#endif // TABLEMANAGER_H

