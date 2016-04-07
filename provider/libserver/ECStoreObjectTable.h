/*
 * Copyright 2005 - 2015  Zarafa B.V. and its licensors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation with the following
 * additional terms according to sec. 7:
 *
 * "Zarafa" is a registered trademark of Zarafa B.V.
 * The licensing of the Program under the AGPL does not imply a trademark
 * license. Therefore any rights, title and interest in our trademarks
 * remain entirely with us.
 *
 * Our trademark policy (see TRADEMARKS.txt) allows you to use our trademarks
 * in connection with Propagation and certain other acts regarding the Program.
 * In any case, if you propagate an unmodified version of the Program you are
 * allowed to use the term "Zarafa" to indicate that you distribute the Program.
 * Furthermore you may use our trademarks where it is necessary to indicate the
 * intended purpose of a product or service provided you use it in accordance
 * with honest business practices. For questions please contact Zarafa at
 * trademark@zarafa.com.
 *
 * The interactive user interface of the software displays an attribution
 * notice containing the term "Zarafa" and/or the logo of Zarafa.
 * Interactive user interfaces of unmodified and modified versions must
 * display Appropriate Legal Notices according to sec. 5 of the GNU Affero
 * General Public License, version 3, when you propagate unmodified or
 * modified versions of the Program. In accordance with sec. 7 b) of the GNU
 * Affero General Public License, version 3, these Appropriate Legal Notices
 * must retain the logo of Zarafa or display the words "Initial Development
 * by Zarafa" if the display of the logo is not reasonably feasible for
 * technical reasons.
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

#ifndef ECSTORE_OBJECTTABLE_H
#define ECSTORE_OBJECTTABLE_H

#include "soapH.h"
#include "ECDatabase.h"

#include "ECGenericObjectTable.h"

/*
 * This object is an actual table, with a cursor in-memory. We also keep the complete
 * keyset of the table in memory, so seeks and queries can be really fast. Also, we
 * sort the table once on loading, and simply use that keyset when the rows are actually
 * required. The actual data is always loaded directly from the database.
 */
class ECSession;

// Objectdata for a store
typedef struct _ECODStore {
	unsigned int	ulStoreId;		// The Store ID this table is watching (0 == multi-store)
	unsigned int	ulFolderId;		// The Folder ID this table is watching (0 == multi-folder)
	unsigned int	ulObjType;
	unsigned int	ulFlags;
	unsigned int 	ulTableFlags;
	GUID*			lpGuid;			// The GUID of the store
}ECODStore;

// For ulTableFlags
#define TABLE_FLAG_OVERRIDE_HOME_MDB 0x00000001

class ECStoreObjectTable : public ECGenericObjectTable
{
protected:
	ECStoreObjectTable(ECSession *lpSession, unsigned int ulStoreId, GUID *lpGuid, unsigned int ulFolderId, unsigned int ulObjType, unsigned int ulFlags, unsigned int ulTableFlags, const ECLocale &locale);
	virtual ~ECStoreObjectTable();
public:
	static ECRESULT Create(ECSession *lpSession, unsigned int ulStoreId, GUID *lpGuid, unsigned int ulFolderId, unsigned int ulObjType, unsigned int ulFlags, unsigned int ulTableFlags, const ECLocale &locale, ECStoreObjectTable **lppTable);
	virtual ECRESULT Load();

	//Overrides
	virtual ECRESULT GetColumnsAll(ECListInt* lplstProps);
    
	// Static database row functions, can be used externally aswell .. Obviously these are *not* threadsafe, make sure that
	// you either lock the passed arguments or all arguments are from the local stack.

	static ECRESULT CopyEmptyCellToSOAPPropVal(struct soap *soap, unsigned int ulPropTag, struct propVal *lpPropVal);

	static ECRESULT QueryRowData(ECGenericObjectTable *lpThis, struct soap *soap, ECSession *lpSession, ECObjectTableList* lpRowList, struct propTagArray *lpsPropTagArray, void* lpObjectData, struct rowSet **lppRowSet, bool bCacheTableData, bool bTableLimit);
	static ECRESULT QueryRowData(ECGenericObjectTable *lpThis, struct soap *soap, ECSession *lpSession, ECObjectTableList* lpRowList, struct propTagArray *lpsPropTagArray, void* lpObjectData, struct rowSet **lppRowSet, bool bCacheTableData, bool bTableLimit, bool bSubObjects);

protected:
	virtual ECRESULT AddRowKey(ECObjectTableList* lpRows, unsigned int *lpulLoaded, unsigned int ulFlags, bool bInitialLoad, bool bOverride, struct restrictTable *lpOverride);
	
	static ECRESULT QueryRowDataByColumn(ECGenericObjectTable *lpThis, struct soap *soap, ECSession *lpSesion, const std::multimap<unsigned int, unsigned int> &mapColumns, unsigned int ulFolderId, const std::map<sObjectTableKey, unsigned int> &mapObjIds, struct rowSet *lpRowSet);
	static ECRESULT QueryRowDataByRow(ECGenericObjectTable *lpThis, struct soap *soap, ECSession *lpSession, const sObjectTableKey &sKey, unsigned int ulRowNum, std::multimap<unsigned int, unsigned int> &mapColumns, bool bTableLimit, struct rowSet *lpsRowSet);

private:
	virtual ECRESULT GetMVRowCount(unsigned int ulObjId, unsigned int *lpulCount);
	virtual ECRESULT ReloadTableMVData(ECObjectTableList* lplistRows, ECListInt* lplistMVPropTag);
	virtual ECRESULT CheckPermissions(unsigned int ulObjId);

	unsigned int ulPermission;
	bool		 fPermissionRead;

};

ECRESULT GetDeferredTableUpdates(ECDatabase *lpDatabase, unsigned int ulFolderId, std::list<unsigned int> *lpDeferred);
ECRESULT GetDeferredTableUpdates(ECDatabase *lpDatabase, ECObjectTableList* lpRowList, std::list<unsigned int> *lpDeferred);


#endif // OBJECTTABLE_H
