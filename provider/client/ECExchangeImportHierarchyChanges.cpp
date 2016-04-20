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

#include "ECExchangeImportHierarchyChanges.h"
#include "ECExchangeImportContentsChanges.h"

#include <zarafa/Util.h>
#include <zarafa/ECGuid.h>
#include <edkguid.h>
#include <mapiguid.h>
#include <zarafa/mapiext.h>
#include <zarafa/ECDebug.h>
#include <zarafa/stringutil.h>
#include "ZarafaUtil.h"
#include "ZarafaICS.h"
#include <mapiutil.h>
#include "Mem.h"
#include <zarafa/mapi_ptr.h>
#include "EntryPoint.h"

#include <zarafa/charset/convert.h>
#include <zarafa/charset/utf8string.h>
#include <zarafa/charset/convstring.h>

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

ECExchangeImportHierarchyChanges::ECExchangeImportHierarchyChanges(ECMAPIFolder *lpFolder){
	m_lpFolder = lpFolder;
	m_lpStream = NULL;
	m_lpFolder->AddRef();
}

ECExchangeImportHierarchyChanges::~ECExchangeImportHierarchyChanges(){
	m_lpFolder->Release();
}

HRESULT ECExchangeImportHierarchyChanges::Create(ECMAPIFolder *lpFolder, LPEXCHANGEIMPORTHIERARCHYCHANGES* lppExchangeImportHierarchyChanges){

	if(!lpFolder)
		return MAPI_E_INVALID_PARAMETER;

	ECExchangeImportHierarchyChanges *lpEIHC = new ECExchangeImportHierarchyChanges(lpFolder);
	return lpEIHC->QueryInterface(IID_IExchangeImportHierarchyChanges, reinterpret_cast<void **>(lppExchangeImportHierarchyChanges));
}

HRESULT	ECExchangeImportHierarchyChanges::QueryInterface(REFIID refiid, void **lppInterface)
{
	REGISTER_INTERFACE(IID_ECExchangeImportHierarchyChanges, this);
	REGISTER_INTERFACE(IID_ECUnknown, this);

	REGISTER_INTERFACE(IID_IExchangeImportHierarchyChanges, &this->m_xExchangeImportHierarchyChanges);
	REGISTER_INTERFACE(IID_IUnknown, &this->m_xExchangeImportHierarchyChanges);

	return MAPI_E_INTERFACE_NOT_SUPPORTED;
}

HRESULT ECExchangeImportHierarchyChanges::GetLastError(HRESULT hResult, ULONG ulFlags, LPMAPIERROR *lppMAPIError){
	HRESULT		hr = hrSuccess;
	LPMAPIERROR	lpMapiError = NULL;
	LPTSTR		lpszErrorMsg = NULL;
	
	//FIXME: give synchronization errors messages
	hr = Util::HrMAPIErrorToText((hResult == hrSuccess)?MAPI_E_NO_ACCESS : hResult, &lpszErrorMsg);
	if (hr != hrSuccess)
		goto exit;

	hr = ECAllocateBuffer(sizeof(MAPIERROR),(void **)&lpMapiError);
	if(hr != hrSuccess)
		goto exit;

	if ((ulFlags & MAPI_UNICODE) == MAPI_UNICODE) {
		std::wstring wstrErrorMsg = convert_to<std::wstring>(lpszErrorMsg);
		std::wstring wstrCompName = convert_to<std::wstring>(g_strProductName.c_str());

		if ((hr = MAPIAllocateMore(sizeof(std::wstring::value_type) * (wstrErrorMsg.size() + 1), lpMapiError, (void**)&lpMapiError->lpszError)) != hrSuccess)
			goto exit;
		wcscpy((wchar_t*)lpMapiError->lpszError, wstrErrorMsg.c_str());

		if ((hr = MAPIAllocateMore(sizeof(std::wstring::value_type) * (wstrCompName.size() + 1), lpMapiError, (void**)&lpMapiError->lpszComponent)) != hrSuccess)
			goto exit;
		wcscpy((wchar_t*)lpMapiError->lpszComponent, wstrCompName.c_str()); 

	} else {
		std::string strErrorMsg = convert_to<std::string>(lpszErrorMsg);
		std::string strCompName = convert_to<std::string>(g_strProductName.c_str());

		if ((hr = MAPIAllocateMore(strErrorMsg.size() + 1, lpMapiError, (void**)&lpMapiError->lpszError)) != hrSuccess)
			goto exit;
		strcpy((char*)lpMapiError->lpszError, strErrorMsg.c_str());

		if ((hr = MAPIAllocateMore(strCompName.size() + 1, lpMapiError, (void**)&lpMapiError->lpszComponent)) != hrSuccess)
			goto exit;
		strcpy((char*)lpMapiError->lpszComponent, strCompName.c_str());
	}

	lpMapiError->ulContext		= 0;
	lpMapiError->ulLowLevelError= 0;
	lpMapiError->ulVersion		= 0;

	*lppMAPIError = lpMapiError;

exit:
	MAPIFreeBuffer(lpszErrorMsg);
	if( hr != hrSuccess && lpMapiError)
		ECFreeBuffer(lpMapiError);

	return hr;
}

HRESULT ECExchangeImportHierarchyChanges::Config(LPSTREAM lpStream, ULONG ulFlags){
	HRESULT hr = hrSuccess;
	LARGE_INTEGER zero = {{0,0}};
	ULONG ulLen = 0;
	LPSPropValue lpPropSourceKey = NULL;
	
	m_lpStream = lpStream;

	if(lpStream == NULL) {
		m_ulSyncId = 0;
		m_ulChangeId = 0;
	} else {
		hr = lpStream->Seek(zero, STREAM_SEEK_SET, NULL);
		if(hr != hrSuccess)
			goto exit;
		
		hr = lpStream->Read(&m_ulSyncId, 4, &ulLen);
		if(hr != hrSuccess)
			goto exit;
			
		if(ulLen != 4) {
			hr = MAPI_E_INVALID_PARAMETER;
			goto exit;
		}
		
		hr = lpStream->Read(&m_ulChangeId, 4, &ulLen);
		if(hr != hrSuccess)
			goto exit;
			
		if(ulLen != 4) {
			hr = MAPI_E_INVALID_PARAMETER;
			goto exit;
		}
		
		hr = HrGetOneProp(&m_lpFolder->m_xMAPIFolder, PR_SOURCE_KEY, &lpPropSourceKey);
		if(hr != hrSuccess)
			goto exit;
		
		// The user specified the special sync key '0000000000000000', get a sync key from the server.
		if(m_ulSyncId == 0) {
			hr = m_lpFolder->GetMsgStore()->lpTransport->HrSetSyncStatus(std::string((char *)lpPropSourceKey->Value.bin.lpb, lpPropSourceKey->Value.bin.cb), m_ulSyncId, m_ulChangeId, ICS_SYNC_HIERARCHY, 0, &m_ulSyncId);
			if(hr != hrSuccess)
				goto exit;
		}
		
		// The sync key we got from the server can be used to retrieve all items in the database now when given to IEEC->Config(). At the same time, any
		// items written to this importer will send the sync ID to the server so that any items written here will not be returned by the exporter,
		// preventing local looping of items.
	}		
		
	m_ulFlags = ulFlags;
exit:	
	MAPIFreeBuffer(lpPropSourceKey);
	return hrSuccess;
}

//write into the stream 4 bytes syncid and 4 bytes changeid
HRESULT ECExchangeImportHierarchyChanges::UpdateState(LPSTREAM lpStream){
	HRESULT hr;
	LARGE_INTEGER zero = {{0,0}};
	ULONG ulLen = 0;
	
	if(lpStream == NULL) {
		if (m_lpStream == NULL)
			return hrSuccess;
		lpStream = m_lpStream;
	}

	if(m_ulSyncId == 0)
		return hrSuccess; // config() called with NULL stream, so we'll ignore the UpdateState()
	
	hr = lpStream->Seek(zero, STREAM_SEEK_SET, NULL);
	if(hr != hrSuccess)
		return hr;
		
	hr = lpStream->Write(&m_ulSyncId, 4, &ulLen);
	if(hr != hrSuccess)
		return hr;
		
	if(m_ulSyncId == 0){
		m_ulChangeId = 0;
	}

	return lpStream->Write(&m_ulChangeId, 4, &ulLen);
}


HRESULT ECExchangeImportHierarchyChanges::ImportFolderChange(ULONG cValue, LPSPropValue lpPropArray){
	HRESULT hr = hrSuccess;

	////The array must contain at least the PR_PARENT_SOURCE_KEY, PR_SOURCE_KEY, PR_CHANGE_KEY, PR_PREDECESSOR_CHANGE_LIST, and MAPI PR_DISPLAY_NAME properties.
	
	LPSPropValue lpPropParentSourceKey = PpropFindProp(lpPropArray, cValue, PR_PARENT_SOURCE_KEY);
	LPSPropValue lpPropSourceKey = PpropFindProp(lpPropArray, cValue, PR_SOURCE_KEY);
	LPSPropValue lpPropDisplayName = PpropFindProp(lpPropArray, cValue, PR_DISPLAY_NAME);
	LPSPropValue lpPropComment = PpropFindProp(lpPropArray, cValue, PR_COMMENT);
	LPSPropValue lpPropChangeKey = PpropFindProp(lpPropArray, cValue, PR_CHANGE_KEY);
	LPSPropValue lpPropFolderType = PpropFindProp(lpPropArray, cValue, PR_FOLDER_TYPE);
	LPSPropValue lpPropChangeList = PpropFindProp(lpPropArray, cValue, PR_PREDECESSOR_CHANGE_LIST);
	LPSPropValue lpPropEntryId = PpropFindProp(lpPropArray, cValue, PR_ENTRYID);
	LPSPropValue lpPropAdditionalREN = PpropFindProp(lpPropArray, cValue, PR_ADDITIONAL_REN_ENTRYIDS);

	LPSPropValue lpPropVal = NULL;
	LPENTRYID lpEntryId = NULL;
	ULONG cbEntryId;
	LPENTRYID lpDestEntryId = NULL;
	ULONG cbDestEntryId;
	ULONG ulObjType;
	LPMAPIFOLDER lpFolder = NULL;
	ECMAPIFolder *lpECFolder = NULL;
	LPMAPIFOLDER lpParentFolder = NULL;
	ECMAPIFolder *lpECParentFolder = NULL;

	ULONG ulFolderType = FOLDER_GENERIC;

	utf8string strFolderComment;
	ULONG cbOrigEntryId = 0;
	BYTE *lpOrigEntryId = NULL;
	LPSBinary lpOrigSourceKey = NULL;

	std::string strChangeList;
	ULONG ulPos = 0;
	ULONG ulSize = 0;
	bool bConflict = false;

	if(!lpPropParentSourceKey || !lpPropSourceKey || !lpPropDisplayName){
		hr = MAPI_E_CALL_FAILED;
		goto exit;
	}

	if (lpPropComment)
		strFolderComment = convert_to<utf8string>(lpPropComment->Value.lpszW);

	if (lpPropEntryId && IsZarafaEntryId(lpPropEntryId->Value.bin.cb, lpPropEntryId->Value.bin.lpb)) {
		cbOrigEntryId = lpPropEntryId->Value.bin.cb;
		lpOrigEntryId = lpPropEntryId->Value.bin.lpb;
	}

	if (lpPropSourceKey) {
		lpOrigSourceKey = &lpPropSourceKey->Value.bin;
	}

	if(lpPropFolderType){
		ulFolderType = lpPropFolderType->Value.ul;
	}

	if(ulFolderType == FOLDER_SEARCH){
		//ignore search folder
		goto exit;
	}

	hr = m_lpFolder->GetMsgStore()->lpTransport->HrEntryIDFromSourceKey(m_lpFolder->GetMsgStore()->m_cbEntryId, m_lpFolder->GetMsgStore()->m_lpEntryId, lpPropSourceKey->Value.bin.cb, lpPropSourceKey->Value.bin.lpb, 0, NULL, &cbEntryId, &lpEntryId);
	if(hr == MAPI_E_NOT_FOUND){
		// Folder is not yet available in our store
		if(lpPropParentSourceKey->Value.bin.cb > 0){
			MAPIFreeBuffer(lpEntryId);
			lpEntryId = NULL;
			
			// Find the parent folder in which the new folder is to be created
			hr = m_lpFolder->GetMsgStore()->lpTransport->HrEntryIDFromSourceKey(m_lpFolder->GetMsgStore()->m_cbEntryId, m_lpFolder->GetMsgStore()->m_lpEntryId , lpPropParentSourceKey->Value.bin.cb, lpPropParentSourceKey->Value.bin.lpb, 0, NULL, &cbEntryId, &lpEntryId);
			if(hr != hrSuccess)
				goto exit;

			if(cbEntryId == 0){
				hr = MAPI_E_CALL_FAILED;
				goto exit;
			}
				
			hr = m_lpFolder->OpenEntry(cbEntryId, lpEntryId, &IID_IMAPIFolder, MAPI_MODIFY, &ulObjType, (LPUNKNOWN*)&lpParentFolder);
			if(hr != hrSuccess)
				goto exit;
				
			MAPIFreeBuffer(lpEntryId);
			lpEntryId = NULL;
				
			hr = lpParentFolder->QueryInterface(IID_ECMAPIFolder, (void**)&lpECParentFolder);
			if(hr != hrSuccess)
				goto exit;
			
			// Create the folder, loop through some names if it collides
			hr = lpECParentFolder->lpFolderOps->HrCreateFolder(ulFolderType, convstring(lpPropDisplayName->Value.lpszW), strFolderComment, 0, m_ulSyncId, lpOrigSourceKey, cbOrigEntryId, (LPENTRYID)lpOrigEntryId, &cbEntryId, &lpEntryId);
			if(hr != hrSuccess)
				goto exit;
			
		}else{
			hr = m_lpFolder->lpFolderOps->HrCreateFolder(ulFolderType, convstring(lpPropDisplayName->Value.lpszW), strFolderComment, 0, m_ulSyncId, lpOrigSourceKey, cbOrigEntryId, (LPENTRYID)lpOrigEntryId, &cbEntryId, &lpEntryId);
			if (hr != hrSuccess)
				goto exit;
		}
		// Open the folder we just created
		hr = m_lpFolder->OpenEntry(cbEntryId, lpEntryId, &IID_IMAPIFolder, MAPI_MODIFY, &ulObjType, (LPUNKNOWN*)&lpFolder);
		if(hr != hrSuccess)
			goto exit;

	}else if(hr != hrSuccess){
		goto exit;
	}else if(cbEntryId == 0){
		hr = MAPI_E_CALL_FAILED;
		goto exit;
	}else if(cbEntryId == m_lpFolder->m_cbEntryId && memcmp(lpEntryId, m_lpFolder->m_lpEntryId, cbEntryId)==0){
		// We are the changed folder
		hr = m_lpFolder->QueryInterface(IID_IMAPIFolder, (LPVOID*)&lpFolder);
		if(hr != hrSuccess)
			goto exit;
	}else{
		bool bRestored = false;

		// Changed folder is an existing subfolder
		hr = m_lpFolder->OpenEntry(cbEntryId, lpEntryId, &IID_IMAPIFolder, MAPI_MODIFY, &ulObjType, (LPUNKNOWN*)&lpFolder);
		if(hr != hrSuccess){
			hr = m_lpFolder->OpenEntry(cbEntryId, lpEntryId, &IID_IMAPIFolder, MAPI_MODIFY | SHOW_SOFT_DELETES, &ulObjType, (LPUNKNOWN*)&lpFolder);
			if(hr != hrSuccess)
				goto exit;

			/**
			 * If the folder was deleted locally, it must have been resotored remote in order to get a change for it.
			 */
			bRestored = true;
		}

		hr = HrGetOneProp(lpFolder, PR_PARENT_SOURCE_KEY, &lpPropVal);
		if(hr != hrSuccess)
			goto exit;
		
		//check if we have to move the folder
		if(bRestored || lpPropVal->Value.bin.cb != lpPropParentSourceKey->Value.bin.cb || memcmp(lpPropVal->Value.bin.lpb, lpPropParentSourceKey->Value.bin.lpb, lpPropVal->Value.bin.cb) != 0){
			if(lpPropParentSourceKey->Value.bin.cb > 0){
				hr = m_lpFolder->GetMsgStore()->lpTransport->HrEntryIDFromSourceKey(m_lpFolder->GetMsgStore()->m_cbEntryId, m_lpFolder->GetMsgStore()->m_lpEntryId , lpPropParentSourceKey->Value.bin.cb, lpPropParentSourceKey->Value.bin.lpb, 0, NULL, &cbDestEntryId, &lpDestEntryId);
				if(hr == MAPI_E_NOT_FOUND){
					//move to a folder we don't have
					hr = m_lpFolder->lpFolderOps->HrDeleteFolder(cbEntryId, lpEntryId, DEL_FOLDERS | DEL_MESSAGES | DELETE_HARD_DELETE, m_ulSyncId);
					goto exit;
				}
				if(hr != hrSuccess)
					goto exit;
			}else{
				cbDestEntryId = m_lpFolder->m_cbEntryId;
				hr = MAPIAllocateBuffer(cbDestEntryId, (void **) &lpDestEntryId);
				if(hr != hrSuccess)
					goto exit;

				memcpy(lpDestEntryId, m_lpFolder->m_lpEntryId, cbDestEntryId);
			}
			
			// Do the move
			hr = m_lpFolder->lpFolderOps->HrCopyFolder(cbEntryId, lpEntryId, cbDestEntryId, lpDestEntryId, utf8string(), FOLDER_MOVE, m_ulSyncId);
			if(hr != hrSuccess)
				goto exit;
		}
		
		MAPIFreeBuffer(lpPropVal);
		lpPropVal = NULL;
	}
	
	MAPIFreeBuffer(lpEntryId);
	lpEntryId = NULL;
	
	//ignore change if remote changekey is in local changelist
	if(lpPropChangeKey && HrGetOneProp(lpFolder, PR_PREDECESSOR_CHANGE_LIST, &lpPropVal) == hrSuccess){
		strChangeList.assign((char *)lpPropVal->Value.bin.lpb, lpPropVal->Value.bin.cb);
		ulPos = 0;

		while(ulPos < strChangeList.size()){
			ulSize = strChangeList.at(ulPos);
			if(ulSize <= sizeof(GUID)){
				break;
			}else if(ulSize == lpPropChangeKey->Value.bin.cb && memcmp(strChangeList.substr(ulPos+1, ulSize).c_str(), lpPropChangeKey->Value.bin.lpb, ulSize) == 0){
				hr = SYNC_E_IGNORE;
				goto exit;
			}
			ulPos += ulSize + 1;
		}
		MAPIFreeBuffer(lpPropVal);
		lpPropVal = NULL;
	}

	//ignore change if local changekey in remote changelist
	if(lpPropChangeList && HrGetOneProp(lpFolder, PR_CHANGE_KEY, &lpPropVal) == hrSuccess){

		strChangeList.assign((char *)lpPropChangeList->Value.bin.lpb, lpPropChangeList->Value.bin.cb);
		ulPos = 0;

		while(ulPos < strChangeList.size()){
			ulSize = strChangeList.at(ulPos);
			if(ulSize <= sizeof(GUID)){
				break;
			}else if(lpPropVal->Value.bin.cb > sizeof(GUID) && memcmp(strChangeList.substr(ulPos+1, ulSize).c_str(), lpPropVal->Value.bin.lpb, sizeof(GUID)) == 0){
				bConflict = !(ulSize == lpPropVal->Value.bin.cb && memcmp(strChangeList.substr(ulPos+1, ulSize).c_str(), lpPropVal->Value.bin.lpb, ulSize) == 0);
				break;
			}
			ulPos += ulSize + 1;
		}
		MAPIFreeBuffer(lpPropVal);
		lpPropVal = NULL;
	}

	if(bConflict){
		//TODO: handle conflicts
	}

	hr = lpFolder->QueryInterface(IID_ECMAPIFolder, (LPVOID*)&lpECFolder);
	if(hr != hrSuccess)
		goto exit;

	hr = lpECFolder->HrSetSyncId(m_ulSyncId);
	if(hr != hrSuccess)
		goto exit;

	hr = lpECFolder->SetProps(cValue, lpPropArray, NULL);
	if(hr != hrSuccess)
		goto exit;

	hr = lpECFolder->SaveChanges(KEEP_OPEN_READWRITE);
	if(hr != hrSuccess)
		goto exit;

	/**
	 * If PR_ADDITIONAL_REN_ENTRYIDS exist this is assumed to be either the Inbox or the root-container. The
	 * root container is only synced during the initial folder sync, but we'll perform the check here anyway.
	 * If we have a PR_ADDITIONAL_REN_ENTRYIDS on the inbox, we'll set the same value on the root-container as
	 * they're supposed to be in sync.
	 * NOTE: This is a workaround for Zarafa not handling this property (and some others) as special properties.
	 */
	if (lpPropAdditionalREN != NULL && lpPropEntryId != NULL && lpPropEntryId->Value.bin.cb > 0) {
		HRESULT hrTmp = hrSuccess;
		MAPIFolderPtr ptrRoot;

		hrTmp = m_lpFolder->OpenEntry(0, NULL, &ptrRoot.iid, MAPI_BEST_ACCESS|MAPI_DEFERRED_ERRORS, &ulObjType, &ptrRoot);
		if (hrTmp != hrSuccess)
			goto exit;

		hrTmp = ptrRoot->SetProps(1, lpPropAdditionalREN, NULL);
		if (hrTmp != hrSuccess)
			goto exit;

		hrTmp = ptrRoot->SaveChanges(KEEP_OPEN_READWRITE);
		if (hrTmp != hrSuccess)
			goto exit;

		hrTmp = ECExchangeImportContentsChanges::HrUpdateSearchReminders(ptrRoot, lpPropAdditionalREN);
	}


exit:
	MAPIFreeBuffer(lpPropVal);
	MAPIFreeBuffer(lpEntryId);
	MAPIFreeBuffer(lpDestEntryId);
	if(lpFolder)
		lpFolder->Release();

	if(lpECFolder)
		lpECFolder->Release();

	if(lpECParentFolder)
		lpECParentFolder->Release();

	if(lpParentFolder)
		lpParentFolder->Release();

	return hr;
}

//ulFlags = SYNC_SOFT_DELETE, SYNC_EXPIRY
HRESULT ECExchangeImportHierarchyChanges::ImportFolderDeletion(ULONG ulFlags, LPENTRYLIST lpSourceEntryList){
	HRESULT hr = hrSuccess;
	ULONG ulSKNr;
	ULONG cbEntryId;
	LPENTRYID lpEntryId = NULL;

	for (ulSKNr = 0; ulSKNr < lpSourceEntryList->cValues; ++ulSKNr) {
		MAPIFreeBuffer(lpEntryId);
		lpEntryId = NULL;
		hr = m_lpFolder->GetMsgStore()->lpTransport->HrEntryIDFromSourceKey(m_lpFolder->GetMsgStore()->m_cbEntryId, m_lpFolder->GetMsgStore()->m_lpEntryId, lpSourceEntryList->lpbin[ulSKNr].cb, lpSourceEntryList->lpbin[ulSKNr].lpb, 0, NULL, &cbEntryId, &lpEntryId);
		if(hr == MAPI_E_NOT_FOUND){
			hr = hrSuccess;
			continue;
		}
		if(hr != hrSuccess)
			goto exit;

		hr = m_lpFolder->lpFolderOps->HrDeleteFolder(cbEntryId, lpEntryId, DEL_FOLDERS | DEL_MESSAGES, m_ulSyncId);
		if(hr !=  hrSuccess)
			goto exit;
	}
	
	if(hr != hrSuccess)
		goto exit;

exit:
	MAPIFreeBuffer(lpEntryId);
	return hr;
}

ULONG ECExchangeImportHierarchyChanges::xExchangeImportHierarchyChanges::AddRef(){
	TRACE_MAPI(TRACE_ENTRY, "IExchangeImportHierarchyChanges::AddRef", "");
	METHOD_PROLOGUE_(ECExchangeImportHierarchyChanges, ExchangeImportHierarchyChanges);
	return pThis->AddRef();
}

ULONG ECExchangeImportHierarchyChanges::xExchangeImportHierarchyChanges::Release()
{
	TRACE_MAPI(TRACE_ENTRY, "IExchangeImportHierarchyChanges::Release", "");
	METHOD_PROLOGUE_(ECExchangeImportHierarchyChanges, ExchangeImportHierarchyChanges);
	return pThis->Release();
}

HRESULT ECExchangeImportHierarchyChanges::xExchangeImportHierarchyChanges::QueryInterface(REFIID refiid, void **lppInterface)
{
	TRACE_MAPI(TRACE_ENTRY, "IExchangeImportHierarchyChanges::QueryInterface", "");
	METHOD_PROLOGUE_(ECExchangeImportHierarchyChanges, ExchangeImportHierarchyChanges);
	return pThis->QueryInterface(refiid, lppInterface);
}

HRESULT ECExchangeImportHierarchyChanges::xExchangeImportHierarchyChanges::GetLastError(HRESULT hError, ULONG ulFlags, LPMAPIERROR * lppMapiError)
{
	TRACE_MAPI(TRACE_ENTRY, "IExchangeImportHierarchyChanges::GetLastError", "");
	METHOD_PROLOGUE_(ECExchangeImportHierarchyChanges, ExchangeImportHierarchyChanges);
	return pThis->GetLastError(hError, ulFlags, lppMapiError);
}

HRESULT ECExchangeImportHierarchyChanges::xExchangeImportHierarchyChanges::Config(LPSTREAM lpStream, ULONG ulFlags){
	TRACE_MAPI(TRACE_ENTRY, "IExchangeImportHierarchyChanges::Config", "");
	METHOD_PROLOGUE_(ECExchangeImportHierarchyChanges, ExchangeImportHierarchyChanges);
	return pThis->Config(lpStream, ulFlags);
}

HRESULT ECExchangeImportHierarchyChanges::xExchangeImportHierarchyChanges::UpdateState(LPSTREAM lpStream){
	TRACE_MAPI(TRACE_ENTRY, "IExchangeImportHierarchyChanges::UpdateState", "");
	METHOD_PROLOGUE_(ECExchangeImportHierarchyChanges, ExchangeImportHierarchyChanges);
	return pThis->UpdateState(lpStream);
}

HRESULT ECExchangeImportHierarchyChanges::xExchangeImportHierarchyChanges::ImportFolderChange(ULONG cValue, LPSPropValue lpPropArray){
	TRACE_MAPI(TRACE_ENTRY, "IExchangeImportHierarchyChanges::ImportFolderChange", "");
	METHOD_PROLOGUE_(ECExchangeImportHierarchyChanges, ExchangeImportHierarchyChanges);
	return pThis->ImportFolderChange(cValue, lpPropArray);
}

HRESULT ECExchangeImportHierarchyChanges::xExchangeImportHierarchyChanges::ImportFolderDeletion(ULONG ulFlags, LPENTRYLIST lpSourceEntryList){
		TRACE_MAPI(TRACE_ENTRY, "IExchangeImportHierarchyChanges::ImportFolderDeletion", "");
	METHOD_PROLOGUE_(ECExchangeImportHierarchyChanges, ExchangeImportHierarchyChanges);
	return pThis->ImportFolderDeletion(ulFlags, lpSourceEntryList);
}
