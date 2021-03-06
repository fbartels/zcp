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
#include "m4l.mapispi.h"
#include "m4l.mapiutil.h"
#include "m4l.mapix.h"
#include "m4l.debug.h"

#include <mapi.h>
#include <zarafa/CommonUtil.h>
#include <zarafa/Util.h>
#include <zarafa/ECGuid.h>

#include <algorithm>
#include <zarafa/mapi_ptr.h>

M4LMAPIGetSession::M4LMAPIGetSession(LPMAPISESSION new_session) {
	ASSERT(new_session);
	session = new_session;
	session->AddRef();
}

M4LMAPIGetSession::~M4LMAPIGetSession() {
	session->Release();
}

HRESULT M4LMAPIGetSession::GetMAPISession(LPUNKNOWN *lppSession)
{
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::GetMAPISession", "");
	HRESULT hr;
	hr = session->QueryInterface(IID_IMAPISession, (void**)lppSession);
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::GetMAPISession", "0x%08x", hr);
	return hr;
}

// iunknown passthru
ULONG M4LMAPIGetSession::AddRef() {
    return M4LUnknown::AddRef();
}
ULONG M4LMAPIGetSession::Release() {
    return M4LUnknown::Release();
}
HRESULT M4LMAPIGetSession::QueryInterface(REFIID refiid, void **lpvoid) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::QueryInterface", "");
	HRESULT hr = hrSuccess;

	if (refiid == IID_IMAPIGetSession || refiid == IID_IUnknown) {
		AddRef();
		*lpvoid = (IMAPISupport *)this;
		hr = hrSuccess;
    } else
		hr = MAPI_E_INTERFACE_NOT_SUPPORTED;

	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::QueryInterface", "0x%08x", hr);
	return hr;
}

M4LMAPISupport::M4LMAPISupport(LPMAPISESSION new_session, LPMAPIUID lpUid, SVCService* lpService) {
	session = new_session;
	service = lpService;

	pthread_mutex_init(&m_advises_mutex, NULL);
	m_connections = 0;
	if(lpUid) {
    	this->lpsProviderUID = new MAPIUID;
        memcpy(this->lpsProviderUID, lpUid, sizeof(MAPIUID));
    } else {
        this->lpsProviderUID = NULL;
    }
}

M4LMAPISupport::~M4LMAPISupport() {
	M4LSUPPORTADVISES::const_iterator i;

	delete lpsProviderUID;
	for (i = m_advises.begin(); i != m_advises.end(); ++i)
		MAPIFreeBuffer(i->second.lpKey);

	pthread_mutex_destroy(&m_advises_mutex);
}

HRESULT M4LMAPISupport::GetLastError(HRESULT hResult, ULONG ulFlags, LPMAPIERROR * lppMAPIError) {
    return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPISupport::GetMemAllocRoutines(LPALLOCATEBUFFER * lpAllocateBuffer, LPALLOCATEMORE * lpAllocateMore,
											LPFREEBUFFER * lpFreeBuffer) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::GetMemAllocRoutines", "");
	HRESULT hr = hrSuccess;

	*lpAllocateBuffer = MAPIAllocateBuffer;
	*lpAllocateMore   = MAPIAllocateMore;
	*lpFreeBuffer     = MAPIFreeBuffer;

	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::GetMemAllocRoutines", "0x%08x", hr);
    return hr;
}

HRESULT M4LMAPISupport::Subscribe(LPNOTIFKEY lpKey, ULONG ulEventMask, ULONG ulFlags, LPMAPIADVISESINK lpAdviseSink,
								  ULONG * lpulConnection) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::Subscribe", "");
	HRESULT hr = hrSuccess;
	LPNOTIFKEY lpNewKey = NULL;

	/* Copy key (this should prevent deletion of the key while it is still in the list */
	hr = MAPIAllocateBuffer(CbNewNOTIFKEY(sizeof(GUID)), (void **)&lpNewKey);
	if (hr != hrSuccess)
		goto exit;

	memcpy(lpNewKey, lpKey, sizeof(*lpKey));

	pthread_mutex_lock(&m_advises_mutex);

	++m_connections;
	m_advises.insert(M4LSUPPORTADVISES::value_type(m_connections, M4LSUPPORTADVISE(lpNewKey, ulEventMask, ulFlags, lpAdviseSink)));
	*lpulConnection = m_connections;

	pthread_mutex_unlock(&m_advises_mutex);

exit:
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::Subscribe", "0x%08x", hr);
    return hr;
}

HRESULT M4LMAPISupport::Unsubscribe(ULONG ulConnection) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::Unsubscribe", "");
	HRESULT hr = hrSuccess;
	M4LSUPPORTADVISES::iterator i;

	pthread_mutex_lock(&m_advises_mutex);

	i = m_advises.find(ulConnection);
	if (i != m_advises.end()) {
		MAPIFreeBuffer(i->second.lpKey);
		m_advises.erase(i);
	} else
		hr = MAPI_E_NOT_FOUND;

	pthread_mutex_unlock(&m_advises_mutex);

	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::Unsubscribe", "0x%08x", hr);
    return hr;
}

HRESULT M4LMAPISupport::Notify(LPNOTIFKEY lpKey, ULONG cNotification, LPNOTIFICATION lpNotifications, ULONG * lpulFlags) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::Notify", "");
	HRESULT hr = hrSuccess;
	LPMAPIADVISESINK lpAdviseSink = NULL;
	M4LSUPPORTADVISES::const_iterator iter;

	pthread_mutex_lock(&m_advises_mutex);

	iter = find_if(m_advises.begin(), m_advises.end(), findKey(lpKey));
	if (iter == m_advises.end()) {
		pthread_mutex_unlock(&m_advises_mutex);
		/* Should this be reported as error? */
		goto exit;
	}
	lpAdviseSink = iter->second.lpAdviseSink;
	lpAdviseSink->AddRef();

	pthread_mutex_unlock(&m_advises_mutex);

	hr = lpAdviseSink->OnNotify(cNotification, lpNotifications);

exit:
	if (lpAdviseSink)
		lpAdviseSink->Release();

	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::Notify", "0x%08x", hr);
    return hr;
}

HRESULT M4LMAPISupport::ModifyStatusRow(ULONG cValues, LPSPropValue lpColumnVals, ULONG ulFlags) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::ModifyStatusRow", "");
	HRESULT hr = hrSuccess;
	M4LMAPISession *lpSession = (M4LMAPISession *)this->session;

	hr = lpSession->setStatusRow(cValues, lpColumnVals);

	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::ModifyStatusRow", "0x%08x", hr);
    return hr;
}

HRESULT M4LMAPISupport::OpenProfileSection(LPMAPIUID lpUid, ULONG ulFlags, LPPROFSECT * lppProfileObj) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::OpenProfileSection", "");
	if (lpUid == NULL)
		lpUid = lpsProviderUID;
        
	HRESULT hr = session->OpenProfileSection(lpUid, NULL, ulFlags, lppProfileObj);

	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::OpenProfileSection", "0x%08x", hr);
	return hr;
}

HRESULT M4LMAPISupport::RegisterPreprocessor(LPMAPIUID lpMuid, LPTSTR lpszAdrType, LPTSTR lpszDLLName, LPSTR lpszPreprocess,
											 LPSTR lpszRemovePreprocessInfo, ULONG ulFlags) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::RegisterPreprocessor", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::RegisterPreprocessor", "0x%08x", MAPI_E_NO_SUPPORT);
    return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPISupport::NewUID(LPMAPIUID lpMuid) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::NewUID", "");
    HRESULT hr = CoCreateGuid((GUID*)lpMuid);
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::NewUID", "0x%08x", hr);
	return hr;
}

HRESULT M4LMAPISupport::MakeInvalid(ULONG ulFlags, LPVOID lpObject, ULONG ulRefCount, ULONG cMethods) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::MakeInvalid", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::MakeInvalid", "0x%08x", MAPI_E_NO_SUPPORT);
    return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPISupport::SpoolerYield(ULONG ulFlags) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::SpoolerYield", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::SpoolerYield", "0x%08x", MAPI_E_NO_SUPPORT);
    return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPISupport::SpoolerNotify(ULONG ulFlags, LPVOID lpvData) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::SpoolerNotify", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::SpoolerNotify", "0x%08x", MAPI_E_NO_SUPPORT);
    return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPISupport::CreateOneOff(LPTSTR lpszName, LPTSTR lpszAdrType, LPTSTR lpszAddress, ULONG ulFlags,
									 ULONG * lpcbEntryID, LPENTRYID * lppEntryID) {
	// although it's called EC... the return value is HRESULT :)
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::CreateOneOff", "");
	HRESULT hr = ECCreateOneOff(lpszName, lpszAdrType, lpszAddress, ulFlags, lpcbEntryID, lppEntryID);
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::CreateOneOff", "0x%08x", hr);
	return hr;
}

HRESULT M4LMAPISupport::SetProviderUID(LPMAPIUID lpProviderID, ULONG ulFlags) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::SetProviderUID", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::SetProviderUID", "0x%08x", hrSuccess);
    return hrSuccess;
}

HRESULT M4LMAPISupport::CompareEntryIDs(ULONG cbEntry1, LPENTRYID lpEntry1, ULONG cbEntry2, LPENTRYID lpEntry2,
										ULONG ulCompareFlags, ULONG * lpulResult) {
	HRESULT hr = hrSuccess;
	TRACE_MAPILIB(TRACE_ENTRY, "CompareEntryIDs::CompareEntryIDs", "");

	if (cbEntry1 != cbEntry2)
		*lpulResult = FALSE;
	else if (!lpEntry1 || !lpEntry2)
		hr = MAPI_E_INVALID_ENTRYID;
	else if (memcmp(lpEntry1, lpEntry2, cbEntry1) != 0)
		*lpulResult = FALSE;
	else
		*lpulResult = TRUE;

	TRACE_MAPILIB1(TRACE_RETURN, "CompareEntryIDs::CompareEntryIDs", "0x%08x", hr);
	return hr;
}

HRESULT M4LMAPISupport::OpenTemplateID(ULONG cbTemplateID, LPENTRYID lpTemplateID, ULONG ulTemplateFlags, LPMAPIPROP lpMAPIPropData,
									   LPCIID lpInterface, LPMAPIPROP * lppMAPIPropNew, LPMAPIPROP lpMAPIPropSibling) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::OpenTemplateID", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::OpenTemplateID", "0x%08x", MAPI_E_NO_SUPPORT);
    return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPISupport::OpenEntry(ULONG cbEntryID, LPENTRYID lpEntryID, LPCIID lpInterface, ULONG ulOpenFlags, ULONG * lpulObjType,
								  LPUNKNOWN * lppUnk) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::OpenEntry", "");
	HRESULT hr = session->OpenEntry(cbEntryID, lpEntryID, lpInterface, ulOpenFlags, lpulObjType, lppUnk);
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::OpenEntry", "0x%08x", hr);
	return hr;
}

HRESULT M4LMAPISupport::GetOneOffTable(ULONG ulFlags, LPMAPITABLE * lppTable) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::GetOneOffTable", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::GetOneOffTable", "0x%08x", MAPI_E_NO_SUPPORT);
    return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPISupport::Address(ULONG * lpulUIParam, LPADRPARM lpAdrParms, LPADRLIST * lppAdrList) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::Address", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::Address", "0x%08x", MAPI_E_NO_SUPPORT);
    return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPISupport::Details(ULONG * lpulUIParam, LPFNDISMISS lpfnDismiss, LPVOID lpvDismissContext, ULONG cbEntryID,
								LPENTRYID lpEntryID, LPFNBUTTON lpfButtonCallback, LPVOID lpvButtonContext, LPTSTR lpszButtonText,
								ULONG ulFlags) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::Details", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::Details", "0x%08x", MAPI_E_NO_SUPPORT);
    return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPISupport::NewEntry(ULONG ulUIParam, ULONG ulFlags, ULONG cbEIDContainer, LPENTRYID lpEIDContainer, ULONG cbEIDNewEntryTpl,
								 LPENTRYID lpEIDNewEntryTpl, ULONG * lpcbEIDNewEntry, LPENTRYID * lppEIDNewEntry) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::NewEntry", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::NewEntry", "0x%08x", MAPI_E_NO_SUPPORT);
    return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPISupport::DoConfigPropsheet(ULONG ulUIParam, ULONG ulFlags, LPTSTR lpszTitle, LPMAPITABLE lpDisplayTable,
										  LPMAPIPROP lpCOnfigData, ULONG ulTopPage) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::DoConfigPropsheet", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::DoConfigPropsheet", "0x%08x", MAPI_E_NO_SUPPORT);
    return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPISupport::CopyMessages(LPCIID lpSrcInterface, LPVOID lpSrcFolder, LPENTRYLIST lpMsgList, LPCIID lpDestInterface,
									 LPVOID lpDestFolder, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::CopyMessages", "");
	HRESULT hr = hrSuccess;
	LPMAPIFOLDER lpSource = NULL;
	LPMAPIFOLDER lpDest = NULL;
	LPMESSAGE lpSrcMessage = NULL;
	LPMESSAGE lpDestMessage = NULL;
	ULONG ulObjType;
	LPENTRYLIST lpDeleteEntries = NULL;
	bool bPartial = false;
	ULONG i;


	if (!lpSrcInterface || !lpSrcFolder || !lpDestFolder || !lpMsgList) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (*lpSrcInterface != IID_IMAPIFolder || (lpDestInterface != NULL && *lpDestInterface != IID_IMAPIFolder)) {
		hr = MAPI_E_INTERFACE_NOT_SUPPORTED;
		goto exit;
	}

	lpSource = (LPMAPIFOLDER)lpSrcFolder;
	lpDest = (LPMAPIFOLDER)lpDestFolder;

	hr = MAPIAllocateBuffer(sizeof(ENTRYLIST), (void**)&lpDeleteEntries);
	if (hr != hrSuccess)
		goto exit;

	hr = MAPIAllocateMore(sizeof(SBinary)*lpMsgList->cValues, lpDeleteEntries, (void**)&lpDeleteEntries->lpbin);
	if (hr != hrSuccess)
		goto exit;

	lpDeleteEntries->cValues = 0;

	for (i = 0; i < lpMsgList->cValues; ++i) {
		hr = lpSource->OpenEntry(lpMsgList->lpbin[i].cb, (LPENTRYID)lpMsgList->lpbin[i].lpb, &IID_IMessage, 0, &ulObjType, (LPUNKNOWN*)&lpSrcMessage);
		if (hr != hrSuccess) {
			// partial, or error to calling client?
			bPartial = true;
			goto next_item;
		}

		hr = lpDest->CreateMessage(&IID_IMessage, MAPI_MODIFY, &lpDestMessage);
		if (hr != hrSuccess) {
			bPartial = true;
			goto next_item;
		}

		hr = this->DoCopyTo(&IID_IMessage, lpSrcMessage, 0, NULL, NULL, ulUIParam, lpProgress, &IID_IMessage, lpDestMessage, 0, NULL);
		if (FAILED(hr)) {
			goto exit;
		} else if (hr != hrSuccess) {
			bPartial = true;
			goto next_item;
		}

		hr = lpDestMessage->SaveChanges(0);
		if (hr != hrSuccess) {
			bPartial = true;
		} else if (ulFlags & MAPI_MOVE) {
			lpDeleteEntries->lpbin[lpDeleteEntries->cValues].cb = lpMsgList->lpbin[i].cb;
			lpDeleteEntries->lpbin[lpDeleteEntries->cValues].lpb = lpMsgList->lpbin[i].lpb;
			++lpDeleteEntries->cValues;
		}

next_item:
		if (lpDestMessage)
			lpDestMessage->Release();
		lpDestMessage = NULL;

		if (lpSrcMessage)
			lpSrcMessage->Release();
		lpSrcMessage = NULL;
	}

	if ((ulFlags & MAPI_MOVE) && lpDeleteEntries->cValues > 0) {
		if (lpSource->DeleteMessages(lpDeleteEntries, 0, NULL, 0) != hrSuccess)
			bPartial = true;
	}
	
	if (bPartial)
		hr = MAPI_W_PARTIAL_COMPLETION;

exit:
	MAPIFreeBuffer(lpDeleteEntries);
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::CopyMessages", "0x%08x", hr);
	return hr;
}

HRESULT M4LMAPISupport::CopyFolder(LPCIID lpSrcInterface, LPVOID lpSrcFolder, ULONG cbEntryID, LPENTRYID lpEntryID,
								   LPCIID lpDestInterface, LPVOID lpDestFolder, LPTSTR lpszNewFolderName, ULONG ulUIParam,
								   LPMAPIPROGRESS lpProgress, ULONG ulFlags) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::CopyFolder", "");
	HRESULT hr = hrSuccess;
	LPMAPIFOLDER lpSource = NULL;
	LPMAPIFOLDER lpDest = NULL;
	LPMAPIFOLDER lpFolder = NULL;
	LPMAPIFOLDER lpSubFolder = NULL;
	LPSPropValue lpSourceName = NULL;
	ULONG ulObjType  = 0;
	ULONG ulFolderFlags = 0;

	SizedSPropTagArray (1, sExcludeProps) = {
		1, {
			PR_DISPLAY_NAME_A,
		}
	};

	if (!lpSrcInterface || !lpSrcFolder || cbEntryID == 0 || !lpEntryID || !lpDestFolder) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (*lpSrcInterface != IID_IMAPIFolder) {
		hr = MAPI_E_INTERFACE_NOT_SUPPORTED;
		goto exit;
	}

	hr = ((LPUNKNOWN)lpSrcFolder)->QueryInterface(IID_IMAPIFolder, (void**)&lpSource);
	if (hr != hrSuccess)
		goto exit;

	// lpDestInterface == NULL or IID_IMAPIFolder compatible
	hr = ((LPUNKNOWN)lpDestFolder)->QueryInterface(IID_IMAPIFolder, (void**)&lpDest);
	if (hr != hrSuccess)
		goto exit;

	hr = lpSource->OpenEntry(cbEntryID, lpEntryID, &IID_IMAPIFolder, 0, &ulObjType, (LPUNKNOWN *)&lpFolder);
	if (hr != hrSuccess)
		goto exit;

	if (!lpszNewFolderName) {
		hr = HrGetOneProp(lpFolder, PR_DISPLAY_NAME_W, &lpSourceName);
		if (hr != hrSuccess)
			goto exit;

		ulFolderFlags |= MAPI_UNICODE;
		lpszNewFolderName = (LPTSTR)lpSourceName->Value.lpszW;
	} else
		ulFolderFlags |= (ulFlags & MAPI_UNICODE);

	hr = lpDest->CreateFolder(FOLDER_GENERIC, lpszNewFolderName, NULL, &IID_IMAPIFolder, ulFolderFlags, &lpSubFolder);
	if (hr != hrSuccess)
		goto exit;

	hr = this->DoCopyTo(&IID_IMAPIFolder, lpFolder, 0, NULL, (LPSPropTagArray)&sExcludeProps, ulUIParam, lpProgress, &IID_IMAPIFolder, lpSubFolder, ulFlags, NULL);
	if (hr != hrSuccess)
		goto exit;

	if (ulFlags & MAPI_MOVE)
		lpSource->DeleteFolder(cbEntryID, (LPENTRYID)lpEntryID, 0, NULL, DEL_FOLDERS | DEL_MESSAGES);

exit:
	if (lpDest)
		lpDest->Release();

	if (lpSource)
		lpSource->Release();

	if (lpFolder)
		lpFolder->Release();
	MAPIFreeBuffer(lpSourceName);
	if (lpSubFolder)
		lpSubFolder->Release();

	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::CopyFolder", "0x%08x", hr);
	return hr;
}

HRESULT M4LMAPISupport::DoCopyTo(LPCIID lpSrcInterface, LPVOID lpSrcObj, ULONG ciidExclude, LPCIID rgiidExclude,
								 LPSPropTagArray lpExcludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpDestInterface,
								 LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray * lppProblems)
{
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::DoCopyTo", "");
	HRESULT hr = hrSuccess;

	hr = Util::DoCopyTo(lpSrcInterface, lpSrcObj, ciidExclude, rgiidExclude, lpExcludeProps, ulUIParam, lpProgress, lpDestInterface, lpDestObj, ulFlags, lppProblems);

	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::DoCopyTo", "0x%08x", hr);
	return hr;
}

HRESULT M4LMAPISupport::DoCopyProps(LPCIID lpSrcInterface, LPVOID lpSrcObj, LPSPropTagArray lpIncludeProps, ULONG ulUIParam,
									LPMAPIPROGRESS lpProgress, LPCIID lpDestInterface, LPVOID lpDestObj, ULONG ulFlags,
									LPSPropProblemArray * lppProblems) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::DoCopyProps", "");
	HRESULT hr = hrSuccess;

	hr = Util::DoCopyProps(lpSrcInterface, lpSrcObj, lpIncludeProps, ulUIParam, lpProgress, lpDestInterface, lpDestObj, ulFlags, lppProblems);

	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::DoCopyProps", "0x%08x", hr);
    return hr;
}

HRESULT M4LMAPISupport::DoProgressDialog(ULONG ulUIParam, ULONG ulFlags, LPMAPIPROGRESS * lppProgress) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::DoProgressDialog", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::DoProgressDialog", "0x%08x", MAPI_E_NO_SUPPORT);
    return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPISupport::ReadReceipt(ULONG ulFlags, LPMESSAGE lpReadMessage, LPMESSAGE * lppEmptyMessage) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::ReadReceipt", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::ReadReceipt", "0x%08x", MAPI_E_NO_SUPPORT);
    return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPISupport::PrepareSubmit(LPMESSAGE lpMessage, ULONG * lpulFlags) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::PrepareSubmit", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::PrepareSubmit", "0x%08x", MAPI_E_NO_SUPPORT);
	// for every recipient,
	// if mapi_submitted flag, clear flag, responsibility == false
	// else set recipient type P1, responsibility == true
    return MAPI_E_NO_SUPPORT;
}

/** 
 * Should perform the following tasks:
 *
 * 1 Expand certain personal distribution lists to their component recipients.
 * 2 Replace all display names that have been changed with the original names.
 * 3 Mark any duplicate entries.
 * 4 Resolve all one-off addresses. 
 * 5 Check whether the message needs preprocessing and, if it does,
 *   set the flag pointed to by lpulFlags to NEEDS_PREPROCESSING.
 * 
 * Currently we only do step 1. The rest is done by the spooler and inetmapi.
 *
 * @param[in] lpMessage The message to process
 * @param[out] lpulFlags Return flags for actions required by the caller.
 * 
 * @return MAPI Error code
 */
HRESULT M4LMAPISupport::ExpandRecips(LPMESSAGE lpMessage, ULONG * lpulFlags) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::ExpandRecips", "");
	HRESULT hr = hrSuccess;
	MAPITablePtr ptrRecipientTable;
	SRowSetPtr ptrRow;
	AddrBookPtr ptrAddrBook;
	std::set<std::vector<unsigned char> > setFilter;
	SPropTagArrayPtr ptrColumns;

	hr = session->OpenAddressBook(0, NULL, AB_NO_DIALOG, &ptrAddrBook);
	if (hr != hrSuccess)
		goto exit;

	hr = lpMessage->GetRecipientTable(fMapiUnicode | MAPI_DEFERRED_ERRORS, &ptrRecipientTable);
	if (hr != hrSuccess)
		goto exit;

	hr = ptrRecipientTable->QueryColumns(TBL_ALL_COLUMNS, &ptrColumns);
	if (hr != hrSuccess)
		goto exit;

	hr = ptrRecipientTable->SetColumns(ptrColumns, 0);
	if (hr != hrSuccess)
		goto exit;

	while (true) {
		LPSPropValue lpAddrType = NULL;
		LPSPropValue lpDLEntryID = NULL;
		ULONG ulObjType;
		DistListPtr ptrDistList;
		MAPITablePtr ptrMemberTable;
		SRowSetPtr ptrMembers;

		hr = ptrRecipientTable->QueryRows(1, 0L, &ptrRow);
		if (hr != hrSuccess)
			goto exit;

		if (ptrRow.size() == 0)
			break;

		lpAddrType = PpropFindProp(ptrRow[0].lpProps, ptrRow[0].cValues, PR_ADDRTYPE);
		if (!lpAddrType)
			continue;

		if (_tcscmp(lpAddrType->Value.LPSZ, _T("MAPIPDL")))
			continue;


		lpDLEntryID = PpropFindProp(ptrRow[0].lpProps, ptrRow[0].cValues, PR_ENTRYID);
		if (!lpDLEntryID)
			continue;

		if (setFilter.find(std::vector<unsigned char>(lpDLEntryID->Value.bin.lpb, lpDLEntryID->Value.bin.lpb + lpDLEntryID->Value.bin.cb)) != setFilter.end()) {
			// already expanded this group so continue without opening
			hr = lpMessage->ModifyRecipients(MODRECIP_REMOVE, (LPADRLIST)ptrRow.get());
			if (hr != hrSuccess)
				goto exit;
			continue;
		}
		setFilter.insert(std::vector<unsigned char>(lpDLEntryID->Value.bin.lpb, lpDLEntryID->Value.bin.lpb + lpDLEntryID->Value.bin.cb));

		hr = ptrAddrBook->OpenEntry(lpDLEntryID->Value.bin.cb, (LPENTRYID)lpDLEntryID->Value.bin.lpb, NULL, 0, &ulObjType, &ptrDistList);
		if (hr != hrSuccess)
			continue;

		// remove MAPIPDL entry when distlist is opened
		hr = lpMessage->ModifyRecipients(MODRECIP_REMOVE, (LPADRLIST)ptrRow.get());
		if (hr != hrSuccess)
			goto exit;

		hr = ptrDistList->GetContentsTable(fMapiUnicode, &ptrMemberTable);
		if (hr != hrSuccess)
			continue;

		// Same columns as recipient table
		hr = ptrMemberTable->SetColumns(ptrColumns, 0);
		if (hr != hrSuccess)
			goto exit;

		// Get all recipients in distlist, and add to message.
		// If another distlist is here, it will expand in the next loop.
		hr = ptrMemberTable->QueryRows(-1, fMapiUnicode, &ptrMembers);
		if (hr != hrSuccess)
			continue;

		// find all unknown properties in the rows, reference-copy those from the original recipient
		// ModifyRecipients() will actually copy the data
		for (ULONG c = 0; c < ptrMembers.size(); ++c) {
			for (ULONG i = 0; i < ptrMembers[c].cValues; ++i) {
				LPSPropValue lpRecipProp = NULL;

				if (PROP_TYPE(ptrMembers[c].lpProps[i].ulPropTag) != PT_ERROR)
					continue;

				// prop is unknown, find prop in recip, and copy value
				lpRecipProp = PpropFindProp(ptrRow[0].lpProps, ptrRow[0].cValues, CHANGE_PROP_TYPE(ptrMembers[c].lpProps[i].ulPropTag, PT_UNSPECIFIED));
				if (lpRecipProp)
					ptrMembers[c].lpProps[i] = *lpRecipProp;
				// else: leave property unknown
			}
		}

		hr = lpMessage->ModifyRecipients(MODRECIP_ADD, (LPADRLIST)ptrMembers.get());
		if (hr != hrSuccess)
			goto exit;
	}
	hr = hrSuccess;

	// Return 0 (no spooler needed, no preprocessing needed)
	if(lpulFlags)
		*lpulFlags = 0;

exit:
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::ExpandRecips", "0x%08x", hrSuccess);
	return hr;
}

HRESULT M4LMAPISupport::UpdatePAB(ULONG ulFlags, LPMESSAGE lpMessage) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::UpdatePAB", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::UpdatePAB", "0x%08x", MAPI_E_NO_SUPPORT);
    return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPISupport::DoSentMail(ULONG ulFlags, LPMESSAGE lpMessage) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::DoSentMail", "");
	HRESULT hr = ::DoSentMail(session, NULL, ulFlags, lpMessage); // from CommonUtil
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::DoSentMail", "0x%08x", hr);
	return hr;
}

HRESULT M4LMAPISupport::OpenAddressBook(LPCIID lpInterface, ULONG ulFlags, LPADRBOOK * lppAdrBook) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::OpenAddressBook", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::OpenAddressBook", "0x%08x", MAPI_E_NO_SUPPORT);
    return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPISupport::Preprocess(ULONG ulFlags, ULONG cbEntryID, LPENTRYID lpEntryID) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::Preprocess", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::Preprocess", "0x%08x", MAPI_E_NO_SUPPORT);
    return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPISupport::CompleteMsg(ULONG ulFlags, ULONG cbEntryID, LPENTRYID lpEntryID) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::CompleteMsg", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::CompleteMsg", "0x%08x", MAPI_E_NO_SUPPORT);
    return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPISupport::StoreLogoffTransports(ULONG * lpulFlags) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::StoreLogoffTransports", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::StoreLogoffTransports", "0x%08x", MAPI_E_NO_SUPPORT);
    return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPISupport::StatusRecips(LPMESSAGE lpMessage, LPADRLIST lpRecipList) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::StatusRecips", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::StatusRecips", "0x%08x", MAPI_E_NO_SUPPORT);
    return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPISupport::WrapStoreEntryID(ULONG cbOrigEntry, LPENTRYID lpOrigEntry, ULONG * lpcbWrappedEntry,
										 LPENTRYID * lppWrappedEntry) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::WrapStoreEntryID", "");
	// get the dll name from SVCService
	HRESULT hr = hrSuccess;
	LPSPropValue lpDLLName = NULL;

	if (!service) {
		// addressbook provider doesn't have the SVCService object
		hr = MAPI_E_CALL_FAILED;
		goto exit;
	}

	lpDLLName = service->GetProp(PR_SERVICE_DLL_NAME_A);
	if (!lpDLLName || !lpDLLName->Value.lpszA) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	// call mapiutil version
	hr = ::WrapStoreEntryID(0, (TCHAR*)lpDLLName->Value.lpszA, cbOrigEntry, lpOrigEntry, lpcbWrappedEntry, lppWrappedEntry);

exit:	
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::WrapStoreEntryID", "0x%08x", hr);
	return hr;
}

HRESULT M4LMAPISupport::ModifyProfile(ULONG ulFlags) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::ModifyProfile", "");
	HRESULT hr = hrSuccess;

	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::ModifyProfile", "0x%08x", hr);
    return hr;
}

HRESULT M4LMAPISupport::IStorageFromStream(LPUNKNOWN lpUnkIn, LPCIID lpInterface, ULONG ulFlags, LPSTORAGE * lppStorageOut) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::IStorageFromStream", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::IStorageFromStream", "0x%08x", MAPI_E_NO_SUPPORT);
    return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPISupport::GetSvcConfigSupportObj(ULONG ulFlags, LPMAPISUP * lppSvcSupport) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::GetSvcConfigSupportObj", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::GetSvcConfigSupportObj", "0x%08x", MAPI_E_NO_SUPPORT);
    return MAPI_E_NO_SUPPORT;
}

// iunknown passthru
ULONG M4LMAPISupport::AddRef() {
    return M4LUnknown::AddRef();
}
ULONG M4LMAPISupport::Release() {
    return M4LUnknown::Release();
}
HRESULT M4LMAPISupport::QueryInterface(REFIID refiid, void **lpvoid) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPISupport::QueryInterface", "");
	HRESULT hr = hrSuccess;

	if (refiid == IID_IMAPISup || refiid == IID_IUnknown) {
		AddRef();
		*lpvoid = (IMAPISupport *)this;
		hr = hrSuccess;
	} else if (refiid == IID_IMAPIGetSession) {
		IMAPIGetSession *lpGetSession = new M4LMAPIGetSession(session);
		lpGetSession->AddRef();
		*lpvoid = lpGetSession;
    } else
		hr = MAPI_E_INTERFACE_NOT_SUPPORTED;

	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPISupport::QueryInterface", "0x%08x", hr);
	return hr;
}
