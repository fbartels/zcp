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

#ifndef ECTPROPSPURGE_H
#define ECTPROPSPURGE_H

#include <zarafa/zcdefs.h>

class ECDatabase;
class ECConfig;
class ECDatabaseFactory;
class ECSession;

class ECTPropsPurge _zcp_final {
public:
    ECTPropsPurge(ECConfig *lpConfig, ECDatabaseFactory *lpDatabaseFactory);
    ~ECTPropsPurge();

    static ECRESULT PurgeDeferredTableUpdates(ECDatabase *lpDatabase, unsigned int ulFolderId);
    static ECRESULT GetDeferredCount(ECDatabase *lpDatabase, unsigned int *lpulCount);
    static ECRESULT GetLargestFolderId(ECDatabase *lpDatabase, unsigned int *lpulFolderId);
    static ECRESULT AddDeferredUpdate(ECSession *lpSession, ECDatabase *lpDatabase, unsigned int ulFolderId, unsigned int ulOldFolderId, unsigned int ulObjId);
    static ECRESULT AddDeferredUpdateNoPurge(ECDatabase *lpDatabase, unsigned int ulFolderId, unsigned int ulOldFolderId, unsigned int ulObjId);
    static ECRESULT NormalizeDeferredUpdates(ECSession *lpSession, ECDatabase *lpDatabase, unsigned int ulFolderId);

private:
    ECRESULT PurgeThread();
    ECRESULT PurgeOverflowDeferred(ECDatabase *lpDatabase);
    static ECRESULT GetDeferredCount(ECDatabase *lpDatabase, unsigned int ulFolderId, unsigned int *lpulCount);
    
    static void *Thread(void *param);

    pthread_mutex_t		m_hMutexExit;
    pthread_cond_t		m_hCondExit;
    pthread_t			m_hThread;
    bool				m_bExit;

    ECConfig *m_lpConfig;    
    ECDatabaseFactory *m_lpDatabaseFactory;
};

#endif
