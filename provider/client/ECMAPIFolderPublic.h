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

#ifndef ECMAPIFOLDERPUBLIC_H
#define ECMAPIFOLDERPUBLIC_H

#include "ECMAPIFolder.h"
#include <zarafa/ECMemTable.h>
#include "ClientUtil.h"

class ECMAPIFolderPublic : public ECMAPIFolder
{
protected:
	ECMAPIFolderPublic(ECMsgStore *lpMsgStore, BOOL fModify, WSMAPIFolderOps *lpFolderOps, enumPublicEntryID ePublicEntryID);
	virtual ~ECMAPIFolderPublic(void);

public:
	static HRESULT Create(ECMsgStore *lpMsgStore, BOOL fModify, WSMAPIFolderOps *lpFolderOps, enumPublicEntryID ePublicEntryID, ECMAPIFolder **lppECMAPIFolder);
	static HRESULT GetPropHandler(ULONG ulPropTag, void* lpProvider, ULONG ulFlags, LPSPropValue lpsPropValue, void *lpParam, void *lpBase);
	static HRESULT SetPropHandler(ULONG ulPropTag, void* lpProvider, LPSPropValue lpsPropValue, void *lpParam);

	virtual HRESULT QueryInterface(REFIID refiid, void **lppInterface);

	virtual HRESULT SetEntryId(ULONG cbEntryId, LPENTRYID lpEntryId);

public:
	// Override ECMAPIContainer
	virtual HRESULT GetContentsTable(ULONG ulFlags, LPMAPITABLE *lppTable);
	virtual HRESULT GetHierarchyTable(ULONG ulFlags, LPMAPITABLE *lppTable);
	virtual HRESULT OpenEntry(ULONG cbEntryID, LPENTRYID lpEntryID, LPCIID lpInterface, ULONG ulFlags, ULONG *lpulObjType, LPUNKNOWN *lppUnk);

	virtual HRESULT SetProps(ULONG cValues, LPSPropValue lpPropArray, LPSPropProblemArray FAR * lppProblems);
	virtual HRESULT DeleteProps(LPSPropTagArray lpPropTagArray, LPSPropProblemArray FAR * lppProblems);

	virtual HRESULT CreateMessage(LPCIID lpInterface, ULONG ulFlags, LPMESSAGE *lppMessage);
	virtual HRESULT CopyMessages(LPENTRYLIST lpMsgList, LPCIID lpInterface, LPVOID lpDestFolder, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags);
	virtual HRESULT CopyFolder(ULONG cbEntryID, LPENTRYID lpEntryID, LPCIID lpInterface, LPVOID lpDestFolder, LPTSTR lpszNewFolderName, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags);
	virtual HRESULT DeleteFolder(ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags);

	enumPublicEntryID	m_ePublicEntryID;

protected:
	virtual HRESULT SaveChanges(ULONG ulFlags);

};


#endif //#ifndef ECMAPIFOLDERPUBLIC_H
