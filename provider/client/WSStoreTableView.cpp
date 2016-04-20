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


#include "ECMsgStore.h"

#include "WSStoreTableView.h"

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

WSStoreTableView::WSStoreTableView(ULONG ulType, ULONG ulFlags, ZarafaCmd *lpCmd, pthread_mutex_t *lpDataLock, ECSESSIONID ecSessionId, ULONG cbEntryId, LPENTRYID lpEntryId, ECMsgStore *lpMsgStore, WSTransport *lpTransport) : WSTableView(ulType, ulFlags, lpCmd, lpDataLock, ecSessionId, cbEntryId, lpEntryId, lpTransport, "WSStoreTableView")
{

	// OK, this is ugly, but the static row-wrapper routines need this object
	// to get the guid and other information that has to be inlined into the table row. Really, 
	// the whole transport layer should have no references to the ECMAPI* classes, because that's
	// upside-down in the layer model, but having the transport layer first deliver the properties,
	// and have a different routine then go through all the properties is more memory intensive AND
	// slower as we have 2 passes to fill the queried rows.

	this->m_lpProvider = (void *)lpMsgStore;
	this->m_ulTableType = TABLETYPE_MS;
}

WSStoreTableView::~WSStoreTableView()
{
	
}


HRESULT WSStoreTableView::Create(ULONG ulType, ULONG ulFlags, ZarafaCmd *lpCmd, pthread_mutex_t *lpDataLock, ECSESSIONID ecSessionId, ULONG cbEntryId, LPENTRYID lpEntryId, ECMsgStore *lpMsgStore, WSTransport *lpTransport, WSTableView **lppTableView)
{
	HRESULT hr = hrSuccess;
	WSStoreTableView *lpTableView = NULL; 

	lpTableView = new WSStoreTableView(ulType, ulFlags, lpCmd, lpDataLock, ecSessionId, cbEntryId, lpEntryId, lpMsgStore, lpTransport);

	hr = lpTableView->QueryInterface(IID_ECTableView, (void **) lppTableView);
	
	if(hr != hrSuccess)
		delete lpTableView;

	return hr;
}

HRESULT WSStoreTableView::QueryInterface(REFIID refiid, void **lppInterface)
{
	REGISTER_INTERFACE(IID_ECTableView, this);
	return MAPI_E_INTERFACE_NOT_SUPPORTED;
}

//////////////////////////////////////////////
// WSTableMultiStore view
//
WSTableMultiStore::WSTableMultiStore(ULONG ulFlags, ZarafaCmd *lpCmd, pthread_mutex_t *lpDataLock, ECSESSIONID ecSessionId, ULONG cbEntryId, LPENTRYID lpEntryId, ECMsgStore *lpMsgStore, WSTransport *lpTransport) : WSStoreTableView(MAPI_MESSAGE, ulFlags, lpCmd, lpDataLock, ecSessionId, cbEntryId, lpEntryId, lpMsgStore, lpTransport)
{
    memset(&m_sEntryList, 0, sizeof(m_sEntryList));

	m_ulTableType = TABLETYPE_MULTISTORE;
	ulTableId = 0;
}

WSTableMultiStore::~WSTableMultiStore()
{
	FreeEntryList(&m_sEntryList, false);
}

HRESULT WSTableMultiStore::Create(ULONG ulFlags, ZarafaCmd *lpCmd, pthread_mutex_t *lpDataLock, ECSESSIONID ecSessionId, ULONG cbEntryId, LPENTRYID lpEntryId, ECMsgStore *lpMsgStore, WSTransport *lpTransport, WSTableMultiStore **lppTableMultiStore)
{
	HRESULT hr = hrSuccess;
	WSTableMultiStore *lpTableMultiStore = NULL; 

	lpTableMultiStore = new WSTableMultiStore(ulFlags, lpCmd, lpDataLock, ecSessionId, cbEntryId, lpEntryId, lpMsgStore, lpTransport);

	// interface ?!
	hr = lpTableMultiStore->QueryInterface(IID_ECTableView, (void **) lppTableMultiStore);
	
	if(hr != hrSuccess)
		delete lpTableMultiStore;

	return hr;
}

HRESULT WSTableMultiStore::HrOpenTable()
{
	ECRESULT		er = erSuccess;
	HRESULT			hr = hrSuccess;

	struct tableOpenResponse sResponse;

	LockSoap();

	if(this->ulTableId != 0)
	    goto exit;

	//m_sEntryId is the id of a store
	if(SOAP_OK != lpCmd->ns__tableOpen(ecSessionId, m_sEntryId, m_ulTableType, MAPI_MESSAGE, this->ulFlags, &sResponse))
		er = ZARAFA_E_NETWORK_ERROR;
	else
		er = sResponse.er;

	hr = ZarafaErrorToMAPIError(er);
	if(hr != hrSuccess)
		goto exit;

	this->ulTableId = sResponse.ulTableId;

	if (SOAP_OK != lpCmd->ns__tableSetMultiStoreEntryIDs(ecSessionId, ulTableId, &m_sEntryList, &er))
		er = ZARAFA_E_NETWORK_ERROR;

	hr = ZarafaErrorToMAPIError(er);
	if(hr != hrSuccess)
		goto exit;

exit:
	UnLockSoap();

	return hr;
}


HRESULT WSTableMultiStore::HrSetEntryIDs(LPENTRYLIST lpMsgList)
{
	// Not really a transport function, but this is the best place for it for now

	return CopyMAPIEntryListToSOAPEntryList(lpMsgList, &m_sEntryList);
}


/*
  Miscellaneous tables are not really store tables, but the is the same, so it inherits from the store table
  Supported tables are the stats tables, and userstores table.
*/
WSTableMisc::WSTableMisc(ULONG ulTableType, ULONG ulFlags, ZarafaCmd *lpCmd, pthread_mutex_t *lpDataLock, ECSESSIONID ecSessionId, ULONG cbEntryId, LPENTRYID lpEntryId, ECMsgStore *lpMsgStore, WSTransport *lpTransport)
	// is MAPI_STATUS even valid here?
	: WSStoreTableView(MAPI_STATUS, ulFlags, lpCmd, lpDataLock, ecSessionId, cbEntryId, lpEntryId, lpMsgStore, lpTransport)
{
	m_ulTableType = ulTableType;
	ulTableId = 0;
}

WSTableMisc::~WSTableMisc()
{
}

HRESULT WSTableMisc::Create(ULONG ulTableType, ULONG ulFlags, ZarafaCmd *lpCmd, pthread_mutex_t *lpDataLock, ECSESSIONID ecSessionId, ULONG cbEntryId, LPENTRYID lpEntryId, ECMsgStore *lpMsgStore, WSTransport *lpTransport, WSTableMisc **lppTableMisc)
{
	HRESULT hr = hrSuccess;
	WSTableMisc *lpTableMisc = NULL;

	lpTableMisc = new WSTableMisc(ulTableType, ulFlags, lpCmd, lpDataLock, ecSessionId, cbEntryId, lpEntryId, lpMsgStore, lpTransport);

	hr = lpTableMisc->QueryInterface(IID_ECTableView, (void **) lppTableMisc);
	if (hr != hrSuccess)
		delete lpTableMisc;

	return hr;
}

HRESULT WSTableMisc::HrOpenTable()
{
	ECRESULT er = erSuccess;
	HRESULT hr = hrSuccess;
	struct tableOpenResponse sResponse;

	LockSoap();
	
	if(ulTableId != 0)
	    goto exit;

	// the class is actually only to call this function with the correct ulTableType .... hmm.
	if(SOAP_OK != lpCmd->ns__tableOpen(ecSessionId, m_sEntryId, m_ulTableType, ulType, this->ulFlags, &sResponse))
		er = ZARAFA_E_NETWORK_ERROR;
	else
		er = sResponse.er;

	hr = ZarafaErrorToMAPIError(er);
	if(hr != hrSuccess)
		goto exit;

	this->ulTableId = sResponse.ulTableId;

exit:
	UnLockSoap();

	return hr;
}


//////////////////////////////////////////////
// WSTableMailBox view
//
WSTableMailBox::WSTableMailBox(ULONG ulFlags, ZarafaCmd *lpCmd, pthread_mutex_t *lpDataLock, ECSESSIONID ecSessionId, ECMsgStore *lpMsgStore, WSTransport *lpTransport) : WSStoreTableView(MAPI_STORE, ulFlags, lpCmd, lpDataLock, ecSessionId, 0, NULL, lpMsgStore, lpTransport)
{
	m_ulTableType = TABLETYPE_MAILBOX;
}

WSTableMailBox::~WSTableMailBox()
{

}

HRESULT WSTableMailBox::Create(ULONG ulFlags, ZarafaCmd *lpCmd, pthread_mutex_t *lpDataLock, ECSESSIONID ecSessionId, ECMsgStore *lpMsgStore, WSTransport *lpTransport, WSTableMailBox **lppTable)
{
	HRESULT hr = hrSuccess;
	WSTableMailBox *lpTable = NULL; 

	lpTable = new WSTableMailBox(ulFlags, lpCmd, lpDataLock, ecSessionId, lpMsgStore, lpTransport);

	//@todo add a new interface
	hr = lpTable->QueryInterface(IID_ECTableView, (void **) lppTable);
	
	if(hr != hrSuccess)
		delete lpTable;

	return hr;
}
