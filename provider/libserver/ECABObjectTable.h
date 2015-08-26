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

#ifndef ECAB_OBJECTTABLE_H
#define ECAB_OBJECTTABLE_H

#include "soapH.h"
#include "ECGenericObjectTable.h"
#include "ECUserManagement.h"

// Objectdata for abprovider
typedef struct{
	unsigned int	ulABId;
	unsigned int	ulABType; // MAPI_ABCONT, MAPI_DISTLIST, MAPI_MAILUSER
	unsigned int 	ulABParentId;
	unsigned int	ulABParentType; // MAPI_ABCONT, MAPI_DISTLIST, MAPI_MAILUSER
}ECODAB;

#define AB_FILTER_SYSTEM		0x00000001
#define AB_FILTER_ADDRESSLIST	0x00000002
#define AB_FILTER_CONTACTS		0x00000004

class ECABObjectTable : public ECGenericObjectTable
{
protected:
	ECABObjectTable(ECSession *lpSession, unsigned int ulABId, unsigned int ulABType, unsigned int ulABParentId, unsigned int ulABParentType, unsigned int ulFlags, const ECLocale &locale);
	virtual ~ECABObjectTable();
public:
	static ECRESULT Create(ECSession *lpSession, unsigned int ulABId, unsigned int ulABType, unsigned int ulABParentId, unsigned int ulABParentType, unsigned int ulFlags, const ECLocale &locale, ECABObjectTable **lppTable);

	//Overrides
	ECRESULT GetColumnsAll(ECListInt* lplstProps);
	ECRESULT Load();

	static ECRESULT QueryRowData(ECGenericObjectTable *lpThis, struct soap *soap, ECSession *lpSession, ECObjectTableList* lpRowList, struct propTagArray *lpsPropTagArray, void* lpObjectData, struct rowSet **lppRowSet, bool bTableData,bool bTableLimit);

protected:
	/* Load hierarchy objects */
	ECRESULT LoadHierarchyAddressList(unsigned int ulObjectId, unsigned int ulFlags,
									  list<localobjectdetails_t> **lppObjects);
	ECRESULT LoadHierarchyCompany(unsigned int ulObjectId, unsigned int ulFlags,
								  list<localobjectdetails_t> **lppObjects);
	ECRESULT LoadHierarchyContainer(unsigned int ulObjectId, unsigned int ulFlags,
									list<localobjectdetails_t> **lppObjects);

	/* Load contents objects */
	ECRESULT LoadContentsAddressList(unsigned int ulObjectId, unsigned int ulFlags,
									 list<localobjectdetails_t> **lppObjects);
	ECRESULT LoadContentsCompany(unsigned int ulObjectId, unsigned int ulFlags,
								 list<localobjectdetails_t> **lppObjects);
	ECRESULT LoadContentsDistlist(unsigned int ulObjectId, unsigned int ulFlags,
								  list<localobjectdetails_t> **lppObjects);

private:
	virtual ECRESULT GetMVRowCount(unsigned int ulObjId, unsigned int *lpulCount);
	ECRESULT ReloadTableMVData(ECObjectTableList* lplistRows, ECListInt* lplistMVPropTag);

protected:
	unsigned int m_ulUserManagementFlags;
};

#endif
