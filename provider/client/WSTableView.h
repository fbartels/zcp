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

/*
	class WSTableView
*/

#ifndef WSTABLEVIEW_H
#define WSTABLEVIEW_H

#include <zarafa/ECUnknown.h>
#include "Zarafa.h"

#include <zarafa/ZarafaCode.h>
#include <mapi.h>
#include <mapispi.h>

#include <pthread.h>
#include "soapZarafaCmdProxy.h"
class WSTransport;

typedef HRESULT (*RELOADCALLBACK)(void *lpParam);

class WSTableView : public ECUnknown
{
protected:
	WSTableView(ULONG ulType, ULONG ulFlags, ZarafaCmd *lpCmd, pthread_mutex_t *lpDataLock, ECSESSIONID ecSessionId, ULONG cbEntryId, LPENTRYID lpEntryId, WSTransport *lpTransport, const char *szClassName = NULL);
	virtual ~WSTableView();

public:
	virtual	HRESULT	QueryInterface(REFIID refiid, void **lppInstanceID);

	virtual HRESULT HrOpenTable();
	virtual HRESULT HrCloseTable();

	// You must call HrOpenTable before calling the following methods
	virtual HRESULT HrSetColumns(LPSPropTagArray lpsPropTagArray);
	virtual HRESULT HrFindRow(LPSRestriction lpsRestriction, BOOKMARK bkOrigin, ULONG ulFlags);
	virtual HRESULT HrQueryColumns(ULONG ulFlags, LPSPropTagArray *lppsPropTags);
	virtual HRESULT HrSortTable(LPSSortOrderSet lpsSortOrderSet);
	virtual HRESULT HrRestrict(LPSRestriction lpsRestriction);
	virtual HRESULT HrQueryRows(ULONG ulRowCount, ULONG ulFlags, LPSRowSet *lppRowSet);
	virtual HRESULT HrGetRowCount(ULONG *lpulRowCount, ULONG *lpulCurrentRow);
	virtual HRESULT HrSeekRow(BOOKMARK bkOrigin, LONG ulRows, LONG *lplRowsSought);
	virtual HRESULT HrExpandRow(ULONG cbInstanceKey, LPBYTE pbInstanceKey, ULONG ulRowCount, ULONG ulFlags, LPSRowSet * lppRows, ULONG *lpulMoreRows);
	virtual HRESULT HrCollapseRow(ULONG cbInstanceKey, LPBYTE pbInstanceKey, ULONG ulFlags, ULONG *lpulRowCount);
	virtual HRESULT HrGetCollapseState(BYTE **lppCollapseState, ULONG *lpcbCollapseState, BYTE *lpbInstanceKey, ULONG cbInstanceKey);
	virtual HRESULT HrSetCollapseState(BYTE *lpCollapseState, ULONG cbCollapseState, BOOKMARK *lpbkPosition);

	virtual HRESULT HrMulti(ULONG ulDeferredFlags, LPSPropTagArray lpsPropTagArray, LPSRestriction lpsRestriction, LPSSortOrderSet lpsSortOrderSet, ULONG ulRowCount, ULONG ulFlags, LPSRowSet *lppRowSet);

	virtual HRESULT FreeBookmark(BOOKMARK bkPosition);
	virtual HRESULT CreateBookmark(BOOKMARK* lpbkPosition);

	static HRESULT Reload(void *lpParam, ECSESSIONID sessionID);
	virtual HRESULT SetReloadCallback(RELOADCALLBACK callback, void *lpParam);

public:
	ULONG		ulTableId;

protected:
	virtual HRESULT LockSoap();
	virtual HRESULT UnLockSoap();

protected:
	ZarafaCmd*		lpCmd;
	pthread_mutex_t *lpDataLock;
	ECSESSIONID		ecSessionId;
	entryId			m_sEntryId;
	void *			m_lpProvider;
	ULONG			m_ulTableType;
	ULONG			m_ulSessionReloadCallback;
	WSTransport*	m_lpTransport;

	LPSPropTagArray m_lpsPropTagArray;
	LPSSortOrderSet m_lpsSortOrderSet;
	LPSRestriction	m_lpsRestriction;

	ULONG		ulFlags;
	ULONG		ulType;

	void *			m_lpParam;
	RELOADCALLBACK  m_lpCallback;
};

#endif
