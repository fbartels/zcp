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

#include <zarafa/platform.h>
#include "Zarafa.h"

#include <zarafa/ZarafaCode.h>
#include <mapi.h>
#include <mapispi.h>
#include "WSABTableView.h"

#include "Mem.h"
#include <zarafa/ECGuid.h>

// Utils
#include "SOAPUtils.h"
#include "WSUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

WSABTableView::WSABTableView(ULONG ulType, ULONG ulFlags, ZarafaCmd *lpCmd, pthread_mutex_t *lpDataLock, ECSESSIONID ecSessionId, ULONG cbEntryId, LPENTRYID lpEntryId, ECABLogon* lpABLogon, WSTransport *lpTransport) : WSTableView(ulType, ulFlags, lpCmd, lpDataLock, ecSessionId, cbEntryId, lpEntryId, lpTransport, "WSABTableView")
{
	m_lpProvider = lpABLogon;
	m_ulTableType = TABLETYPE_AB;
}

WSABTableView::~WSABTableView()
{

}


HRESULT WSABTableView::Create(ULONG ulType, ULONG ulFlags, ZarafaCmd *lpCmd, pthread_mutex_t *lpDataLock, ECSESSIONID ecSessionId, ULONG cbEntryId, LPENTRYID lpEntryId, ECABLogon* lpABLogon, WSTransport *lpTransport, WSTableView **lppTableView)
{
	HRESULT hr = hrSuccess;
	WSABTableView *lpTableView = NULL; 

	lpTableView = new WSABTableView(ulType, ulFlags, lpCmd, lpDataLock, ecSessionId, cbEntryId, lpEntryId, lpABLogon, lpTransport);

	hr = lpTableView->QueryInterface(IID_ECTableView, (void **) lppTableView);
	
	if(hr != hrSuccess)
		delete lpTableView;

	return hr;
}

HRESULT WSABTableView::QueryInterface(REFIID refiid, void **lppInterface)
{
	REGISTER_INTERFACE(IID_ECTableView, this);

	return MAPI_E_INTERFACE_NOT_SUPPORTED;
}
