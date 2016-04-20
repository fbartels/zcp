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

#ifndef ECSEARCHFOLDERS_H
#define ECSEARCHFOLDERS_H

#include <zarafa/zcdefs.h>
#include "ECDatabaseFactory.h"
#include <zarafa/ECKeyTable.h>
#include "ECStoreObjectTable.h"

#include "soapH.h"
#include "SOAPUtils.h"

#include <map>
#include <set>
#include <list>

class ECSessionManager;

typedef struct SEARCHFOLDER _zcp_final {
	SEARCHFOLDER(unsigned int ulStoreId, unsigned int ulFolderId) {
		this->lpSearchCriteria = NULL;
		/* sThreadId */
		pthread_mutex_init(&this->mMutexThreadFree, NULL);
		this->bThreadExit = false;
		this->bThreadFree = true;
		this->ulStoreId = ulStoreId;
		this->ulFolderId = ulFolderId;
	}
	~SEARCHFOLDER() {
		if (this->lpSearchCriteria)
			FreeSearchCriteria(this->lpSearchCriteria);
		pthread_mutex_destroy(&this->mMutexThreadFree);
	}

    struct searchCriteria 	*lpSearchCriteria;
    pthread_t 				sThreadId;
    pthread_mutex_t			mMutexThreadFree;
    bool 					bThreadFree;
    bool					bThreadExit;
    unsigned int			ulStoreId;
    unsigned int			ulFolderId;
} SEARCHFOLDER;

typedef struct EVENT {
    unsigned int			ulStoreId;
    unsigned int			ulFolderId;
    unsigned int			ulObjectId;
    ECKeyTable::UpdateType  ulType;
    
    bool operator<(const struct EVENT &b) const { return ulFolderId < b.ulFolderId ? true : (ulType < b.ulType ? true : ( ulObjectId < b.ulObjectId ? true : false ) ); }
    bool operator==(const struct EVENT &b) const { return ulFolderId == b.ulFolderId && ulType == b.ulType && ulObjectId ==  b.ulObjectId; }
} EVENT;

typedef std::map<unsigned int, SEARCHFOLDER *> FOLDERIDSEARCH;
typedef std::map<unsigned int, FOLDERIDSEARCH> STOREFOLDERIDSEARCH;
typedef std::map<unsigned int, pthread_t> SEARCHTHREADMAP;

typedef struct tagsSearchFolderStats
{
	ULONG ulStores;
	ULONG ulFolders;
	ULONG ulEvents;
	ULONGLONG ullSize;
}sSearchFolderStats;

/**
 * Searchfolder handler
 *
 * This represents a single manager of all searchfolders on the server; a single thread runs on behalf of this
 * manager to handle all object changes, and another thread can be running for each searchfolder that is rebuilding. Most of
 * the time only the single update thread is running though.
 *
 * The searchfolder manager does four things:
 * - Loading all searchfolder definitions (restriction and folderlist) at startup
 * - Adding and removing searchfolders when users create/remove searchfolders
 * - Rebuilding searchfolder contents (when users rebuild searchfolders)
 * - Getting searchfolder results (when users open searchfolders)
 *
 * Storage of searchresults is on-disk in the MySQL database; restarts of zarafa-server do not affect searchfolders
 * except rebuilding searchfolders; when the server starts and finds a searchfolder that was only half-built, a complete
 * rebuild is started since we don't know how far the rebuild got last time.
 */
class ECSearchFolders _zcp_final {
public:
    ECSearchFolders(ECSessionManager *lpSessionManager, ECDatabaseFactory *lpFactory);
    virtual ~ECSearchFolders();

    /**
     * Does the initial load of all searchfolders by looking in the hierarchy table for ALL searchfolders and retrieving the
     * information for each of them. Will also rebuild folders that need rebuilding (folders with the REBUILDING state)
     */
    virtual ECRESULT LoadSearchFolders();

    /**
     * Set search criteria for a new or existing search folder
     *
     * Will remove any previous search criteria on the folder, cleanup the search results and rebuild the search results.
     * This function is called almost directly from the SetSearchCriteria() MAPI function.
     *
     * @param[in] ulStoreId The store id (hierarchyid) of the searchfolder being modified
     * @param[in] ulFolderId The folder id (hierarchyid) of the searchfolder being modified
     * @param[in] lpSearchCriteria Search criteria to be set
     */
    virtual ECRESULT SetSearchCriteria(unsigned int ulStoreId, unsigned int ulFolderId, struct searchCriteria *lpSearchCriteria);

    /**
     * Retrieve search criteria for an existing search folder
     *
     * @param[in] ulStoreId The store id (hierarchyid) of the searchfolder being modified
     * @param[in] ulFolderId The folder id (hierarchyid) of the searchfolder being modified
     * @param[out] lpSearchCriteria Search criteria previously set via SetSearchCriteria
     */
    virtual ECRESULT GetSearchCriteria(unsigned int ulStoreId, unsigned int ulFolderId, struct searchCriteria **lppSearchCriteria, unsigned int *lpulSearchFlags);

    /**
     * Get current search results for a folder. Simply a database query, nothing more.
     *
     * This retrieves all the items that the search folder contains as a list of hierarchy IDs. Since the
     * search results are already available, the data is returned directly from the database.
     *
     * @param[in] ulStoreId The store id (hierarchyid) of the searchfolder being modified
     * @param[in] ulFolderId The folder id (hierarchyid) of the searchfolder being modified
     * @param[out] lstObjIds List of object IDs in the result set. List is cleared and populated.
     */
    virtual ECRESULT GetSearchResults(unsigned int ulStoreId, unsigned int ulFolderId, std::list<unsigned int> *lstObjIds);

    /**
     * Queue a messages change that should be processed to update the search folders
     *
     * This function should be called for any message object that has been modified so that the change can be processed
     * in all searchfolders. You must specify if the item was modified (added) or deleted since delete processing
     * is much simpler (just remove the item from all searchfolders)
     *
     * This function should be called AFTER the change has been written to the database and AFTER the change
     * has been comitted, otherwise the change will be invisible to the searchfolder update code.
     *
     * Folder changes never need to be processed since searchfolders cannot be used for other folders
     *
     * @param[in] ulStoreId The store id (hierarchyid) of the object that should be processed
     * @param[in] ulFolderId The folder id (hierarchyid) of the object that should be processed
     * @param[in] ulObjId The hierarchyid of the modified object
     * @param[in] ulType ECKeyTable::TABLE_ROW_ADD or TABLE_ROW_MODIFY or TABLE_ROW_DELETE
     */
    virtual ECRESULT UpdateSearchFolders(unsigned int ulStoreId, unsigned int ulFolderId, unsigned int ulObjId, ECKeyTable::UpdateType ulType);
    
    /** 
     * Returns erSuccess if the folder is a search folder
     *
     * @param[in] ulStoreId The store id (hierarchyid) of the folder being queried
     * @param[in] ulFolderId The folder id (hierarchyid) of the folder being queried
     */
    virtual ECRESULT IsSearchFolder(unsigned int ulStoreId, unsigned int ulFolderId);

    /** 
     * Remove a search folder because it has been deleted. Cancels the search before removing the information. It will
     * remove all results from the database. 
     *
     * This is differenct from Cancelling a search folder (see CancelSearchFolder()) because the results are actually
     * deleted after cancelling.
     *
     * @param[in] ulStoreId The store id (hierarchyid) of the folder to be removed
     * @param[in] ulFolderId The folder id (hierarchyid) of the folder to be removed
     */
    virtual ECRESULT RemoveSearchFolder(unsigned int ulStoreId, unsigned int ulFolderId);

	/**
	 * Remove a search folder of a specific store because it has been deleted. Cancels the search before removing the 
	 * information. It will remove all results from the database. 
	 *
	 * @param[in] ulStoreId The store id (hierarchyid) of the folder to be removed
	 */
	virtual ECRESULT RemoveSearchFolder(unsigned int ulStoreID);

	/**
	 * Wait till all threads are down and free the data of a searchfolder
	 *
	 * @param[in] lpFolder	Search folder data object
	 */
	void DestroySearchFolder(SEARCHFOLDER *lpFolder);

    /** 
     * Restart all searches. 
     * This is a rather heavy operation, and runs synchronously. You have to wait until it has finished.
     * This is only called with the --restart-searches option of zarafa-server and never used in a running
     * system
     */
    virtual ECRESULT RestartSearches();
    
	/**
	 * Save search criteria to the database
	 *
	 * Purely writes the given search criteria to the database without any further processing. This is really
	 * a private function but it is used hackishly from ECDatabaseUpdate() when upgrading from really old (4.1)
	 * versions of zarafa which have a slightly different search criteria format. Do not use this function for
	 * anything else!
	 *
	 * @param[in] lpDatabase Database handle
	 * @param[in] ulStoreId Store id (hierarchy id) of the searchfolder to write
	 * @param[in] ulFolderId Folder id (hierarchy id) of the searchfolder to write
	 * @param[in] lpSearchCriteria Search criteria to write
	 */
	static ECRESULT SaveSearchCriteria(ECDatabase *lpDatabase, unsigned int ulStoreId, unsigned int ulFolderId, struct searchCriteria *lpSearchCriteria);

	/**
	 * Get the searchfolder statistics
	 */
	virtual ECRESULT GetStats(sSearchFolderStats &sStats);

	/**
	 * Kick search thread to flush events, and wait for the results.
	 * Only used in the test protocol.
	 */
	virtual void FlushAndWait();

private:
    /**
     * Process all events in the queue and remove them from the queue.
     *
     * Events for changed objects are queued internally and only processed after being flushed here. This function
     * groups same-type events together to increase performance because changes in the same folder can be processed
     * more efficiently at on time
     */
    virtual ECRESULT FlushEvents();

    /**
     * Processes a list of message changes in a single folder that should be processed. This in turn
     * will update the search results views through the Table Manager to update the actual user views.
     *
     * @param[in] ulStoreId Store id of the message changes to be processed
     * @param[in] ulFolderId Folder id of the message changes to be processed
     * @param[in] lstObjectIDs List of hierarchyids of messages to be processed
     * @param[in] ulType Type of change: ECKeyTable::TABLE_ROW_ADD, TABLE_ROW_DELETE or TABLE_ROW_MODIFY
     */
    virtual ECRESULT ProcessMessageChange(unsigned int ulStoreId, unsigned int ulFolderId, ECObjectTableList *lstObjectIDs, ECKeyTable::UpdateType ulType);

    /**
     * Add a search folder to the list of active searches
     *
     * This function add a search folder that should be monitored. This means that changes on objects received via UpdateSearchFolders()
     * will be matched against the criteria passed to this function and processed accordingly.
     *
     * Optionally, a rebuild can be started with the fStartSearch flag. This should be done if the search should be rebuilt, or 
     * if this is a new search folder. On rebuild, existing searches for this search folder will be cancelled first.
     *
     * @param[in] ulStoreId Store id of the search folder
     * @param[in] ulFolderId Folder id of the search folder
     * @param[in] fStartSearch TRUE if a rebuild must take place, FALSE if not (eg this happens at server startup)
     * @param[in] lpSearchCriteria Search criteria for this search folder
     */
     
    virtual ECRESULT AddSearchFolder(unsigned int ulStoreId, unsigned int ulFolderId, bool fStartSearch, struct searchCriteria *lpSearchCriteria);
    
    /** 
     * Cancel a search. 
     *
     * This means that the search results are 'frozen'. If a search thread is running, it is cancelled.
     * After a search has been cancelled, we can ignore any updates for that folder, so it is removed from the list
     * of active searches. (but the results remain in the database). We also have to remember this fact in the database because
     * after a server restart, the search should still be 'stopped' and not rebuilt or active.
     *
     * @param[in] ulStoreId Store id of the search folder
     * @param[in] ulFolderId Folder id of the search folder
     */
    virtual ECRESULT CancelSearchFolder(unsigned int ulStoreID, unsigned int ulFolderId);
    
    /**
     * Does an actual search for all matching items for a searchfolder
     *
     * Adds information in the database, and sends updates through the table manager to
     * previously opened tables. This is called only from the search thread and from RestartSearches(). After the
     * search is done, changes in the searchfolder are only done incrementally through calls to UpdateSearchFolders().
     *
     * @param[in] ulStoreId Store id of the search folder
     * @param[in] ulFolderId Folder id of the search folder
     * @param[in] lpSearchCriteria Search criteria to match
     * @param[in] lpbCancel Pointer to cancel flag. This is polled frequently to be able to cancel the search action
     * @param[in] bNotify If TRUE, send notifications to table listeners, else do not (eg when doing RestartSearches())
     */
    virtual ECRESULT Search(unsigned int ulStoreId, unsigned int ulFolderId, struct searchCriteria *lpSearchCriteria, bool *lpbCancel, bool bNotify = true);

    /** 
     * Get the state of a search folder
     *
     * It may be rebuilding (thread running), running (no thread) or stopped (not active - 'frozen')
     *
     * @param[in] ulStoreId Store id of the search folder
     * @param[in] ulFolderId Folder id of the search folder
     * @param[out] lpulState Current state of folder (SEARCH_RUNNING | SEARCH_REBUILD, SEARCH_RUNNING, 0)
     */
    virtual ECRESULT GetState(unsigned int ulStoreId, unsigned int ulFolderId, unsigned int *lpulState);

    /** 
     * Search thread entrypoint. 
     *
     * Simply a wrapper for Search(), and has code to do thread deregistration.
     * @param[in] lpParam THREADINFO * thread information
     */
    static void* SearchThread(void *lpParam);

    // Functions to do things in the database
    
    /**
     * Reset all results for a searchfolder (removes all results)
     *
     * @param[in] ulStoreId Store id of the search folder
     * @param[in] ulFolderId Folder id of the search folder
     */
    virtual ECRESULT ResetResults(unsigned int ulStoreId, unsigned int ulFolderId);

    /**
     * Add a search result to a search folder (one message id with flags)
     *
     * @param[in] ulStoreId Store id of the search folder
     * @param[in] ulFolderId Folder id of the search folder
     * @param[in] ulObjId Object hierarchy id of the matching message
     * @param[in] ulFlags Flags of the object (this should be in-sync with hierarchy table!). May be 0 or MSGFLAG_READ
     * @param[out] lpfInserted true if a new record was inserted, false if flags were updated in an existing record
     */
    virtual ECRESULT AddResults(unsigned int ulStoreId, unsigned int ulFolderId, unsigned int ulObjId, unsigned int ulFlags, bool *lpfInserted);
    
    /**
     * Add multiple search results
     *
     * @param[in] ulStoreId Store id of the search folder
     * @param[in] ulFolderId Folder id of the search folder
     * @param[in] ulObjId Object hierarchy id of the matching message
     * @param[in] ulFlags Flags of the object (this should be in-sync with hierarchy table!). May be 0 or MSGFLAG_READ
     * @param[out] lpulCount Int to be modified with inserted count
     * @param[out] lpulUnread Int to be modified with inserted unread count
     */
    virtual ECRESULT AddResults(unsigned int ulStoreId, unsigned int ulFolderId, std::list<unsigned int> &lstObjId, std::list<unsigned int>& lstFlags, int *lpulCount, int *lpulUnread);
    /**
     * Delete matching results from a search folder
     *
     * @param[in] ulStoreId Store id of the search folder
     * @param[in] ulFolderId Folder id of the search folder
     * @param[in] ulObjId Object hierarchy id of the matching message
     * @param[out] lpulFlags Flags of the object that was just deleted
     */
    virtual ECRESULT DeleteResults(unsigned int ulStoreId, unsigned int ulFolderId, unsigned int ulObjId, unsigned int *lpulFlags);

    /**
     * Set the status of a searchfolder
     *
     * @param[in] ulFolderId Folder id of the search folder
     * @param[in] ulStatus SEARCH_RUNNING or 0
     */
    virtual ECRESULT SetStatus(unsigned int ulFolderId, unsigned int ulStatus);

    // Functions to load/save search criteria to the database

    /**
     * Load serialized search criteria from database
     *
     * @param[in] ulStoreId Store id of the search folder
     * @param[in] ulFolderId Folder id of the search folder
     * @param[in] lppSearchCriteria Loaded search criteria
     */
    virtual ECRESULT LoadSearchCriteria(unsigned int ulStoreId, unsigned int ulFolderId, struct searchCriteria **lppSearchCriteria);

    /**
     * Save serialized search criteria to database
     *
     * @param[in] ulStoreId Store id of the search folder
     * @param[in] ulFolderId Folder id of the search folder
     * @param[in] lpSearchCriteria Search criteria to save
     */
    virtual ECRESULT SaveSearchCriteria(unsigned int ulStoreId, unsigned int ulFolderId, struct searchCriteria *lpSearchCriteria);

    /**
     * Main processing thread entrypoint
     *
     * This thread is running throughout the lifetime of the server and polls the queue periodically to process
     * message changes many-at-a-time.
     *
     * @param[in] lpSearchFolders Pointer to 'this' of search folder manager instance
     */
    static void * ProcessThread(void *lpSearchFolders);

    /**
     * Process candidate rows and add them to search folder results
     *
     * This function processes the list of rows provides against the restriction provides, and
     * adds rows to the given folder's result set if the rows match. Each row is evaluated seperately.
     *
     * @param[in] lpDatabase Database handle
     * @param[in] lpSession Session handle
     * @param[in] lpRestrict Restriction to match the items with
     * @param[in] lpbCancel Pointer to cancellation boolean; processing is stopped when *lpbCancel == true
     * @param[in] ulStoreId Store in which the items in ecRows reside
     * @param[in] ulFolder The hierarchy of the searchfolder to update with the results
     * @param[in] ecODStore Store information
     * @param[in] ecRows Rows to evaluate
     * @param[in] lpPropTags List of precomputed property tags that are needed to resolve the restriction. The first property in this array MUST be PR_MESSAGE_FLAGS.
     * @param[in] locale Locale to use for string comparisons in the restriction
     * @param[in] bNotify TRUE on a live system, FALSE if only the database must be updated.
     * @return result
     */
    virtual ECRESULT ProcessCandidateRows(ECDatabase *lpDatabase, ECSession *lpSession, struct restrictTable *lpRestrict, bool *lpbCancel, unsigned int ulStoreId, unsigned int ulFolderId, ECODStore *ecODStore, ECObjectTableList ecRows, struct propTagArray *lpPropTags, const ECLocale &locale, bool bNotify);

    // Map StoreID -> SearchFolderId -> SearchCriteria
    // Because searchfolders only work within a store, this allows us to skip 99% of all
    // search folders during UpdateSearchFolders (depending on how many users you have)
    pthread_mutex_t m_mutexMapSearchFolders;
    STOREFOLDERIDSEARCH m_mapSearchFolders;

    // Pthread condition to signal a thread exit
    pthread_cond_t m_condThreadExited;

    ECDatabaseFactory *m_lpDatabaseFactory;
    ECSessionManager *m_lpSessionManager;

    // List of change events
    std::list<EVENT> m_lstEvents;
    pthread_mutex_t m_mutexEvents;
    pthread_cond_t m_condEvents;
    
    // Change processing thread
    pthread_t m_threadProcess;
    
    // Exit request for processing thread
    bool m_bExitThread;
	bool m_bRunning;
};

#endif
