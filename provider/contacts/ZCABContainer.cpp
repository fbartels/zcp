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
#include "ZCABContainer.h"
#include "ZCMAPIProp.h"
#include <zarafa/Trace.h>

#include <mapiutil.h>

#include <zarafa/ECMemTable.h>
#include <zarafa/ECGuid.h>
#include <zarafa/ECDebug.h>
#include <zarafa/CommonUtil.h>
#include <zarafa/mapiext.h>
#include <zarafa/mapiguidext.h>
#include <zarafa/namedprops.h>
#include <zarafa/charset/convert.h>
#include <zarafa/mapi_ptr.h>
#include <zarafa/ECGetText.h>
#include <zarafa/EMSAbTag.h>
#include <zarafa/ECRestriction.h>

#include <iostream>
#include <zarafa/stringutil.h>
using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

ZCABContainer::ZCABContainer(std::vector<zcabFolderEntry> *lpFolders,
    IMAPIFolder *lpContacts, LPMAPISUP lpMAPISup, void *lpProvider,
    const char *szClassName) :
	ECUnknown(szClassName)
{
	ASSERT(!(lpFolders && lpContacts));

	m_lpFolders = lpFolders;
	m_lpContactFolder = lpContacts;
	m_lpMAPISup = lpMAPISup;
	m_lpProvider = lpProvider;
	m_lpDistList = NULL;

	if (m_lpMAPISup)
		m_lpMAPISup->AddRef();
	if (m_lpContactFolder)
		m_lpContactFolder->AddRef();
}

ZCABContainer::~ZCABContainer()
{
	if (m_lpMAPISup)
		m_lpMAPISup->Release();
	if (m_lpContactFolder)
		m_lpContactFolder->Release();
	if (m_lpDistList)
		m_lpDistList->Release();
}

HRESULT	ZCABContainer::QueryInterface(REFIID refiid, void **lppInterface)
{
	if (m_lpDistList == NULL) {
		REGISTER_INTERFACE(IID_ZCABContainer, this);
	} else {
		REGISTER_INTERFACE(IID_ZCDistList, this);
	}
	REGISTER_INTERFACE(IID_ECUnknown, this);

	if (m_lpDistList == NULL) {
		REGISTER_INTERFACE(IID_IABContainer, &this->m_xABContainer);
	} else {
		REGISTER_INTERFACE(IID_IDistList, &this->m_xABContainer);
	}
	REGISTER_INTERFACE(IID_IMAPIProp, &this->m_xABContainer);
	REGISTER_INTERFACE(IID_IUnknown, &this->m_xABContainer);

	return MAPI_E_INTERFACE_NOT_SUPPORTED;
}

/** 
 * Create a ZCABContainer as either the top level (lpFolders is set) or
 * as a subfolder (lpContacts is set).
 * 
 * @param[in] lpFolders Only the top container has the list to the wanted Zarafa Contacts Folders, NULL otherwise.
 * @param[in] lpContacts Create this object as wrapper for the lpContacts folder, NULL if 
 * @param[in] lpMAPISup 
 * @param[in] lpProvider 
 * @param[out] lppABContainer The newly created ZCABContainer class
 * 
 * @return 
 */
HRESULT	ZCABContainer::Create(std::vector<zcabFolderEntry> *lpFolders, IMAPIFolder *lpContacts, LPMAPISUP lpMAPISup, void* lpProvider, ZCABContainer **lppABContainer)
{
	HRESULT			hr = hrSuccess;
	ZCABContainer*	lpABContainer = NULL;

	try {
		lpABContainer = new ZCABContainer(lpFolders, lpContacts, lpMAPISup, lpProvider, "IABContainer");

		hr = lpABContainer->QueryInterface(IID_ZCABContainer, (void **)lppABContainer);
	} catch (...) {
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
	}

	return hr;
}

HRESULT	ZCABContainer::Create(IMessage *lpContact, ULONG cbEntryID, LPENTRYID lpEntryID, LPMAPISUP lpMAPISup, ZCABContainer **lppABContainer)
{
	HRESULT hr = hrSuccess;
	ZCABContainer*	lpABContainer = NULL;
	ZCMAPIProp* lpDistList = NULL;

	try {
		lpABContainer = new ZCABContainer(NULL, NULL, lpMAPISup, NULL, "IABContainer");
	} catch (...) {
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
	}
	if (hr != hrSuccess)
		goto exit;

	hr = ZCMAPIProp::Create(lpContact, cbEntryID, lpEntryID, &lpDistList);
	if (hr != hrSuccess)
		goto exit;

	hr = lpDistList->QueryInterface(IID_IMAPIProp, (void **)&lpABContainer->m_lpDistList);
	if (hr != hrSuccess)
		goto exit;

	hr = lpABContainer->QueryInterface(IID_ZCDistList, (void **)lppABContainer);

exit:
	if (hr != hrSuccess)
		delete lpABContainer;

	if (lpDistList)
		lpDistList->Release();

	return hr;
}

/////////////////////////////////////////////////
// IMAPIContainer
//

HRESULT ZCABContainer::MakeWrappedEntryID(ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulObjType, ULONG ulOffset, ULONG *lpcbEntryID, LPENTRYID *lppEntryID)
{
	HRESULT hr = hrSuccess;
	ULONG cbWrapped = 0;
	cabEntryID *lpWrapped = NULL;

	cbWrapped = CbNewCABENTRYID(cbEntryID);
	hr = MAPIAllocateBuffer(cbWrapped, (void**)&lpWrapped);
	if (hr != hrSuccess)
		goto exit;

	memset(lpWrapped, 0, cbWrapped);
	memcpy(&lpWrapped->muid, &MUIDZCSAB, sizeof(MAPIUID));
	lpWrapped->ulObjType = ulObjType;
	lpWrapped->ulOffset = ulOffset;
	memcpy(lpWrapped->origEntryID, lpEntryID, cbEntryID);
	

	*lpcbEntryID = cbWrapped;
	*lppEntryID = (LPENTRYID)lpWrapped;

exit:
	return hr;
}

HRESULT ZCABContainer::GetFolderContentsTable(ULONG ulFlags, LPMAPITABLE *lppTable)
{
	HRESULT hr = hrSuccess;
	MAPITablePtr ptrContents;
	SRowSetPtr	ptrRows;
	ECMemTable*		lpTable = NULL;
	ECMemTableView*	lpTableView = NULL;
	ULONG i, j = 0;
	ECOrRestriction resOr;
	ECAndRestriction resAnd;
	SPropValue sRestrictProp;
	SRestrictionPtr ptrRestriction;

#define I_NCOLS 11
	// data from the contact
	SizedSPropTagArray(I_NCOLS, inputCols) = {I_NCOLS, {PR_DISPLAY_NAME, PR_ADDRTYPE, PR_EMAIL_ADDRESS, PR_NORMALIZED_SUBJECT, PR_ENTRYID, PR_MESSAGE_CLASS, PR_ORIGINAL_DISPLAY_NAME,
														PR_PARENT_ENTRYID, PR_SOURCE_KEY, PR_PARENT_SOURCE_KEY, PR_CHANGE_KEY}};
	// I_MV_INDEX is dispidABPEmailList from mnNamedProps
	enum {I_DISPLAY_NAME = 0, I_ADDRTYPE, I_EMAIL_ADDRESS, I_NORMALIZED_SUBJECT, I_ENTRYID, I_MESSAGE_CLASS, I_ORIGINAL_DISPLAY_NAME, 
		  I_PARENT_ENTRYID, I_SOURCE_KEY, I_PARENT_SOURCE_KEY, I_CHANGE_KEY, I_MV_INDEX, I_NAMEDSTART};
	SPropTagArrayPtr ptrInputCols;

#define O_NCOLS 21
	// data for the table
	SizedSPropTagArray(O_NCOLS, outputCols) = {O_NCOLS, {PR_DISPLAY_NAME, PR_ADDRTYPE, PR_EMAIL_ADDRESS, PR_NORMALIZED_SUBJECT, PR_ENTRYID, PR_DISPLAY_TYPE, PR_OBJECT_TYPE, PR_ORIGINAL_DISPLAY_NAME, 
														 PR_ZC_ORIGINAL_ENTRYID, PR_ZC_ORIGINAL_PARENT_ENTRYID, PR_ZC_ORIGINAL_SOURCE_KEY, PR_ZC_ORIGINAL_PARENT_SOURCE_KEY, PR_ZC_ORIGINAL_CHANGE_KEY,
														 PR_SEARCH_KEY, PR_INSTANCE_KEY, PR_ROWID}};
	enum {O_DISPLAY_NAME = 0, O_ADDRTYPE, O_EMAIL_ADDRESS, O_NORMALIZED_SUBJECT, O_ENTRYID, O_DISPLAY_TYPE, O_OBJECT_TYPE, O_ORIGINAL_DISPLAY_NAME, 
		  O_ZC_ORIGINAL_ENTRYID, O_ZC_ORIGINAL_PARENT_ENTRYID, O_ZC_ORIGINAL_SOURCE_KEY, O_ZC_ORIGINAL_PARENT_SOURCE_KEY, O_ZC_ORIGINAL_CHANGE_KEY,
		  O_SEARCH_KEY, O_INSTANCE_KEY, O_ROWID};
	SPropTagArrayPtr ptrOutputCols;

	SPropTagArrayPtr ptrContactCols;
	SPropValue lpColData[O_NCOLS];

	// named properties
	SPropTagArrayPtr ptrNameTags;
	LPMAPINAMEID *lppNames = NULL;
	ULONG ulNames = (6 * 5) + 2;
	ULONG ulType = (ulFlags & MAPI_UNICODE) ? PT_UNICODE : PT_STRING8;
	MAPINAMEID mnNamedProps[(6 * 5) + 2] = {
		// index with MVI_FLAG
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidABPEmailList}},

		// MVI offset 0: email1 set
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidEmail1DisplayName}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidEmail1AddressType}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidEmail1Address}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidEmail1OriginalDisplayName}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidEmail1OriginalEntryID}},

		// MVI offset 1: email2 set
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidEmail2DisplayName}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidEmail2AddressType}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidEmail2Address}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidEmail2OriginalDisplayName}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidEmail2OriginalEntryID}},

		// MVI offset 2: email3 set
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidEmail3DisplayName}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidEmail3AddressType}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidEmail3Address}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidEmail3OriginalDisplayName}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidEmail3OriginalEntryID}},

		// MVI offset 3: business fax (fax2) set
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidFax2DisplayName}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidFax2AddressType}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidFax2Address}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidFax2OriginalDisplayName}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidFax2OriginalEntryID}},

		// MVI offset 4: home fax (fax3) set
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidFax3DisplayName}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidFax3AddressType}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidFax3Address}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidFax3OriginalDisplayName}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidFax3OriginalEntryID}},		

		// MVI offset 5: primary fax (fax1) set
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidFax1DisplayName}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidFax1AddressType}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidFax1Address}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidFax1OriginalDisplayName}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidFax1OriginalEntryID}},

		// restriction
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidABPArrayType}},
	};


	ulFlags = ulFlags & MAPI_UNICODE;

	hr = Util::HrCopyUnicodePropTagArray(ulFlags, (LPSPropTagArray)&inputCols, &ptrInputCols);
	if (hr != hrSuccess)
		goto exit;
	hr = Util::HrCopyUnicodePropTagArray(ulFlags, (LPSPropTagArray)&outputCols, &ptrOutputCols);
	if (hr != hrSuccess)
		goto exit;

	hr = ECMemTable::Create(ptrOutputCols, PR_ROWID, &lpTable);
	if(hr != hrSuccess)
		goto exit;

	// root container has no contents, on hierarchy entries
	if (m_lpContactFolder == NULL)
		goto done;

	hr = m_lpContactFolder->GetContentsTable(ulFlags | MAPI_DEFERRED_ERRORS, &ptrContents);
	if (hr != hrSuccess)
		goto exit;

	hr = MAPIAllocateBuffer(sizeof(LPMAPINAMEID) * (ulNames), (void**)&lppNames);
	if (hr != hrSuccess)
		goto exit;

	for (i = 0; i < ulNames; ++i)
		lppNames[i] = &mnNamedProps[i];

	hr = m_lpContactFolder->GetIDsFromNames(ulNames, lppNames, MAPI_CREATE, &ptrNameTags);
	if (FAILED(hr))
		goto exit;

	// fix types
	ptrNameTags->aulPropTag[0] = CHANGE_PROP_TYPE(ptrNameTags->aulPropTag[0], PT_MV_LONG | MV_INSTANCE);
	for (i = 0; i < (ulNames - 2) / 5; ++i) {
		ptrNameTags->aulPropTag[1+ (i*5) + 0] = CHANGE_PROP_TYPE(ptrNameTags->aulPropTag[1+ (i*5) + 0], ulType);
		ptrNameTags->aulPropTag[1+ (i*5) + 1] = CHANGE_PROP_TYPE(ptrNameTags->aulPropTag[1+ (i*5) + 1], ulType);
		ptrNameTags->aulPropTag[1+ (i*5) + 2] = CHANGE_PROP_TYPE(ptrNameTags->aulPropTag[1+ (i*5) + 2], ulType);
		ptrNameTags->aulPropTag[1+ (i*5) + 3] = CHANGE_PROP_TYPE(ptrNameTags->aulPropTag[1+ (i*5) + 3], ulType);
		ptrNameTags->aulPropTag[1+ (i*5) + 4] = CHANGE_PROP_TYPE(ptrNameTags->aulPropTag[1+ (i*5) + 4], PT_BINARY);
	}
	ptrNameTags->aulPropTag[ulNames-1] = CHANGE_PROP_TYPE(ptrNameTags->aulPropTag[ulNames-1], PT_LONG);

	// add func HrCombinePropTagArrays(part1, part2, dest);
	hr = MAPIAllocateBuffer(CbNewSPropTagArray(ptrInputCols->cValues + ptrNameTags->cValues), &ptrContactCols);
	if (hr != hrSuccess)
		goto exit;
	j = 0;
	for (i = 0; i < ptrInputCols->cValues; ++i)
		ptrContactCols->aulPropTag[j++] = ptrInputCols->aulPropTag[i];
	for (i = 0; i < ptrNameTags->cValues; ++i)
		ptrContactCols->aulPropTag[j++] = ptrNameTags->aulPropTag[i];
	ptrContactCols->cValues = j;

	// the exists is extra compared to the outlook restriction
	// restrict: ( distlist || ( contact && exist(abparraytype) && abparraytype != 0 ) )
	sRestrictProp.ulPropTag = PR_MESSAGE_CLASS_A;
	sRestrictProp.Value.lpszA = const_cast<char *>("IPM.DistList");
	resOr.append(ECContentRestriction(FL_PREFIX|FL_IGNORECASE, PR_MESSAGE_CLASS_A, &sRestrictProp));

	sRestrictProp.Value.lpszA = const_cast<char *>("IPM.Contact");
	resAnd.append(ECContentRestriction(FL_PREFIX|FL_IGNORECASE, PR_MESSAGE_CLASS_A, &sRestrictProp));
	sRestrictProp.ulPropTag = ptrNameTags->aulPropTag[ulNames-1];
	sRestrictProp.Value.ul = 0;
	resAnd.append(ECExistRestriction(sRestrictProp.ulPropTag));
	resAnd.append(ECPropertyRestriction(RELOP_NE, sRestrictProp.ulPropTag, &sRestrictProp));

	resOr.append(resAnd);

	hr = resOr.CreateMAPIRestriction(&ptrRestriction);
	if (hr != hrSuccess)
		goto exit;

	hr = ptrContents->Restrict(ptrRestriction, TBL_BATCH);
	if (hr != hrSuccess)
		goto exit;

	// set columns
	hr = ptrContents->SetColumns(ptrContactCols, TBL_BATCH);
	if (hr != hrSuccess)
		goto exit;

	j = 0;
	while (true) {
		hr = ptrContents->QueryRows(256, 0, &ptrRows);
		if (hr != hrSuccess)
			goto exit;

		if (ptrRows.empty())
			break;

		for (i = 0; i < ptrRows.size(); ++i) {
			ULONG ulOffset = 0;
			std::string strSearchKey;
			std::wstring wstrSearchKey;

			memset(lpColData, 0, sizeof(lpColData));

			if (ptrRows[i].lpProps[I_MV_INDEX].ulPropTag == (ptrNameTags->aulPropTag[0] & ~MVI_FLAG)) {
				// do not index outside named properties
				if (ptrRows[i].lpProps[I_MV_INDEX].Value.ul > 5)
					continue;
				ulOffset = ptrRows[i].lpProps[I_MV_INDEX].Value.ul * 5;
			}

			if (PROP_TYPE(ptrRows[i].lpProps[I_MESSAGE_CLASS].ulPropTag) == PT_ERROR)
				// no PR_MESSAGE_CLASS, unusable
				continue;

			if (
				((ulFlags & MAPI_UNICODE) && wcscasecmp(ptrRows[i].lpProps[I_MESSAGE_CLASS].Value.lpszW, L"IPM.Contact") == 0) ||
				((ulFlags & MAPI_UNICODE) == 0 && stricmp(ptrRows[i].lpProps[I_MESSAGE_CLASS].Value.lpszA, "IPM.Contact") == 0)
				)
			{
				lpColData[O_DISPLAY_TYPE].ulPropTag = PR_DISPLAY_TYPE;
				lpColData[O_DISPLAY_TYPE].Value.ul = DT_MAILUSER;

				lpColData[O_OBJECT_TYPE].ulPropTag = PR_OBJECT_TYPE;
				lpColData[O_OBJECT_TYPE].Value.ul = MAPI_MAILUSER;

				lpColData[O_ADDRTYPE].ulPropTag = CHANGE_PROP_TYPE(ptrOutputCols->aulPropTag[O_ADDRTYPE], PROP_TYPE(ptrRows[i].lpProps[I_NAMEDSTART + ulOffset + 1].ulPropTag));
				lpColData[O_ADDRTYPE].Value = ptrRows[i].lpProps[I_NAMEDSTART + ulOffset + 1].Value;
			} else if (
					   ((ulFlags & MAPI_UNICODE) && wcscasecmp(ptrRows[i].lpProps[I_MESSAGE_CLASS].Value.lpszW, L"IPM.DistList") == 0) ||
					   ((ulFlags & MAPI_UNICODE) == 0 && stricmp(ptrRows[i].lpProps[I_MESSAGE_CLASS].Value.lpszA, "IPM.DistList") == 0)
					   )
			{
				lpColData[O_DISPLAY_TYPE].ulPropTag = PR_DISPLAY_TYPE;
				lpColData[O_DISPLAY_TYPE].Value.ul = DT_PRIVATE_DISTLIST;

				lpColData[O_OBJECT_TYPE].ulPropTag = PR_OBJECT_TYPE;
				lpColData[O_OBJECT_TYPE].Value.ul = MAPI_DISTLIST;

				lpColData[O_ADDRTYPE].ulPropTag = PR_ADDRTYPE_W;
				lpColData[O_ADDRTYPE].Value.lpszW = const_cast<wchar_t *>(L"MAPIPDL");
			} else {
				continue;
			}

			// devide by 5 since a block of properties on a contact is a set of 5 (see mnNamedProps above)
			hr = MakeWrappedEntryID(ptrRows[i].lpProps[I_ENTRYID].Value.bin.cb, (LPENTRYID)ptrRows[i].lpProps[I_ENTRYID].Value.bin.lpb,
									lpColData[O_OBJECT_TYPE].Value.ul, ulOffset/5,
									&lpColData[O_ENTRYID].Value.bin.cb, (LPENTRYID*)&lpColData[O_ENTRYID].Value.bin.lpb);
			if (hr != hrSuccess)
				goto exit;

			lpColData[O_ENTRYID].ulPropTag = PR_ENTRYID;

			ulOffset += I_NAMEDSTART;

			lpColData[O_DISPLAY_NAME].ulPropTag = CHANGE_PROP_TYPE(ptrOutputCols->aulPropTag[O_DISPLAY_NAME], PROP_TYPE(ptrRows[i].lpProps[ulOffset + 0].ulPropTag));
			if (PROP_TYPE(lpColData[O_DISPLAY_NAME].ulPropTag) == PT_ERROR) {
				// Email#Display not available, fallback to normal PR_DISPLAY_NAME
				lpColData[O_DISPLAY_NAME] = ptrRows[i].lpProps[I_DISPLAY_NAME];
			} else {
				lpColData[O_DISPLAY_NAME].Value = ptrRows[i].lpProps[ulOffset + 0].Value;
			}

			lpColData[O_EMAIL_ADDRESS].ulPropTag = CHANGE_PROP_TYPE(ptrOutputCols->aulPropTag[O_EMAIL_ADDRESS], PROP_TYPE(ptrRows[i].lpProps[ulOffset + 2].ulPropTag));
			if (PROP_TYPE(lpColData[O_EMAIL_ADDRESS].ulPropTag) == PT_ERROR) {
				// Email#Address not available, fallback to normal PR_EMAIL_ADDRESS
				lpColData[O_EMAIL_ADDRESS] = ptrRows[i].lpProps[I_EMAIL_ADDRESS];
			} else {
				lpColData[O_EMAIL_ADDRESS].Value = ptrRows[i].lpProps[ulOffset + 2].Value;
			}

			lpColData[O_NORMALIZED_SUBJECT].ulPropTag = CHANGE_PROP_TYPE(ptrOutputCols->aulPropTag[O_NORMALIZED_SUBJECT], PROP_TYPE(ptrRows[i].lpProps[ulOffset + 3].ulPropTag));
			if (PROP_TYPE(lpColData[O_NORMALIZED_SUBJECT].ulPropTag) == PT_ERROR) {
				// Email#OriginalDisplayName not available, fallback to normal PR_NORMALIZED_SUBJECT
				lpColData[O_NORMALIZED_SUBJECT] = ptrRows[i].lpProps[I_NORMALIZED_SUBJECT];
			} else {
				lpColData[O_NORMALIZED_SUBJECT].Value = ptrRows[i].lpProps[ulOffset + 3].Value;
			}

			lpColData[O_ORIGINAL_DISPLAY_NAME].ulPropTag = CHANGE_PROP_TYPE(ptrOutputCols->aulPropTag[O_ORIGINAL_DISPLAY_NAME], PROP_TYPE(ptrRows[i].lpProps[I_DISPLAY_NAME].ulPropTag));
			lpColData[O_ORIGINAL_DISPLAY_NAME].Value = ptrRows[i].lpProps[I_DISPLAY_NAME].Value;

			lpColData[O_ZC_ORIGINAL_ENTRYID].ulPropTag = CHANGE_PROP_TYPE(ptrOutputCols->aulPropTag[O_ZC_ORIGINAL_ENTRYID], PROP_TYPE(ptrRows[i].lpProps[I_ENTRYID].ulPropTag));
			lpColData[O_ZC_ORIGINAL_ENTRYID].Value = ptrRows[i].lpProps[I_ENTRYID].Value;

			lpColData[O_ZC_ORIGINAL_PARENT_ENTRYID].ulPropTag = CHANGE_PROP_TYPE(ptrOutputCols->aulPropTag[O_ZC_ORIGINAL_PARENT_ENTRYID], PROP_TYPE(ptrRows[i].lpProps[I_PARENT_ENTRYID].ulPropTag));
			lpColData[O_ZC_ORIGINAL_PARENT_ENTRYID].Value = ptrRows[i].lpProps[I_PARENT_ENTRYID].Value;

			lpColData[O_ZC_ORIGINAL_SOURCE_KEY].ulPropTag = CHANGE_PROP_TYPE(ptrOutputCols->aulPropTag[O_ZC_ORIGINAL_SOURCE_KEY], PROP_TYPE(ptrRows[i].lpProps[I_SOURCE_KEY].ulPropTag));
			lpColData[O_ZC_ORIGINAL_SOURCE_KEY].Value = ptrRows[i].lpProps[I_SOURCE_KEY].Value;

			lpColData[O_ZC_ORIGINAL_PARENT_SOURCE_KEY].ulPropTag = CHANGE_PROP_TYPE(ptrOutputCols->aulPropTag[O_ZC_ORIGINAL_PARENT_SOURCE_KEY], PROP_TYPE(ptrRows[i].lpProps[I_PARENT_SOURCE_KEY].ulPropTag));
			lpColData[O_ZC_ORIGINAL_PARENT_SOURCE_KEY].Value = ptrRows[i].lpProps[I_PARENT_SOURCE_KEY].Value;

			lpColData[O_ZC_ORIGINAL_CHANGE_KEY].ulPropTag = CHANGE_PROP_TYPE(ptrOutputCols->aulPropTag[O_ZC_ORIGINAL_CHANGE_KEY], PROP_TYPE(ptrRows[i].lpProps[I_CHANGE_KEY].ulPropTag));
			lpColData[O_ZC_ORIGINAL_CHANGE_KEY].Value = ptrRows[i].lpProps[I_CHANGE_KEY].Value;

			// @note, outlook seems to set the gab original search key (if possible, otherwise SMTP). The IMessage contact in the folder contains some unusable binary blob.
			if (PROP_TYPE(lpColData[O_ADDRTYPE].ulPropTag) == PT_STRING8 && PROP_TYPE(lpColData[O_EMAIL_ADDRESS].ulPropTag) == PT_STRING8) {
				strSearchKey = string(lpColData[O_ADDRTYPE].Value.lpszA) + ":" + lpColData[O_EMAIL_ADDRESS].Value.lpszA;
			} else if (PROP_TYPE(lpColData[O_ADDRTYPE].ulPropTag) == PT_UNICODE && PROP_TYPE(lpColData[O_EMAIL_ADDRESS].ulPropTag) == PT_UNICODE) {
				wstrSearchKey = wstring(lpColData[O_ADDRTYPE].Value.lpszW) + L":" + lpColData[O_EMAIL_ADDRESS].Value.lpszW;
				strSearchKey = convert_to<string>(wstrSearchKey);
			} else {
				// eg. distlists
				hr = MAPI_E_NOT_FOUND;
			}
			if (hr == hrSuccess) {
				transform(strSearchKey.begin(), strSearchKey.end(), strSearchKey.begin(), ::toupper);
				lpColData[O_SEARCH_KEY].ulPropTag = PR_SEARCH_KEY;
				lpColData[O_SEARCH_KEY].Value.bin.cb = strSearchKey.length()+1;
				lpColData[O_SEARCH_KEY].Value.bin.lpb = (BYTE*)strSearchKey.data();
			} else {
				lpColData[O_SEARCH_KEY].ulPropTag = CHANGE_PROP_TYPE(PR_SEARCH_KEY, PT_ERROR);
				lpColData[O_SEARCH_KEY].Value.ul = MAPI_E_NOT_FOUND;
			}

			lpColData[O_INSTANCE_KEY].ulPropTag = PR_INSTANCE_KEY;
			lpColData[O_INSTANCE_KEY].Value.bin.cb = sizeof(ULONG);
			lpColData[O_INSTANCE_KEY].Value.bin.lpb = (LPBYTE)&j;

			lpColData[O_ROWID].ulPropTag = PR_ROWID;
			lpColData[O_ROWID].Value.ul = j++;

			hr = lpTable->HrModifyRow(ECKeyTable::TABLE_ROW_ADD, NULL, lpColData, O_NCOLS);
			if (hr != hrSuccess)
				goto exit;
		}
	}
		
done:
	AddChild(lpTable);

	hr = lpTable->HrGetView(createLocaleFromName(NULL), ulFlags, &lpTableView);
	if(hr != hrSuccess)
		goto exit;
	
	hr = lpTableView->QueryInterface(IID_IMAPITable, (void **)lppTable);

exit:
	MAPIFreeBuffer(lppNames);
	if(lpTable)
		lpTable->Release();

	if(lpTableView)
		lpTableView->Release();

	return hr;
#undef TCOLS
}

HRESULT ZCABContainer::GetDistListContentsTable(ULONG ulFlags, LPMAPITABLE *lppTable)
{
	HRESULT hr = hrSuccess;
	SizedSPropTagArray(13, sptaCols) = {13, {PR_NULL /* reserve for PR_ROWID */,
											 PR_ADDRTYPE, PR_DISPLAY_NAME, PR_DISPLAY_TYPE, PR_EMAIL_ADDRESS, PR_ENTRYID,
											 PR_INSTANCE_KEY, PR_OBJECT_TYPE, PR_RECORD_KEY, PR_SEARCH_KEY, PR_SEND_INTERNET_ENCODING,
											 PR_SEND_RICH_INFO, PR_TRANSMITABLE_DISPLAY_NAME }};
	SPropTagArrayPtr ptrCols;
	ECMemTable* lpTable = NULL;
	ECMemTableView*	lpTableView = NULL;
	SPropValuePtr ptrEntries;
	MAPIPropPtr ptrUser;
	ULONG ulObjType;
	ULONG cValues;
	SPropArrayPtr ptrProps;
	SPropValue sKey;
	mapi_object_ptr<ZCMAPIProp> ptrZCMAPIProp;

	hr = Util::HrCopyUnicodePropTagArray(ulFlags, (LPSPropTagArray)&sptaCols, &ptrCols);
	if (hr != hrSuccess)
		goto exit;

	hr = ECMemTable::Create(ptrCols, PR_ROWID, &lpTable);
	if(hr != hrSuccess)
		goto exit;

	// getprops, open real contacts, make table
	hr = HrGetOneProp(m_lpDistList, 0x81051102, &ptrEntries); // Members "entryids" named property, see data layout below
	if (hr != hrSuccess)
		goto exit;


	sKey.ulPropTag = PR_ROWID;
	sKey.Value.ul = 0;
	for (ULONG i = 0; i < ptrEntries->Value.MVbin.cValues; ++i) {
		ULONG ulOffset = 0;
		BYTE cType = 0;

		// Wrapped entryid's:
		// Flags: (ULONG) 0
		// Provider: (GUID) 0xC091ADD3519DCF11A4A900AA0047FAA4
		// Type: (BYTE) <value>, describes wrapped enrtyid
		//  lower 4 bits:
		//   0x00 = OneOff (use addressbook)
		//   0x03 = Contact (use folder / session?)
		//   0x04 = PDL  (use folder / session?)
		//   0x05 = GAB IMailUser (use addressbook)
		//   0x06 = GAB IDistList (use addressbook)
		//  next 3 bits: if lower is 0x03
		//   0x00 = business fax, or oneoff entryid
		//   0x10 = home fax
		//   0x20 = primary fax
		//   0x30 = no contact data
		//   0x40 = email 1
		//   0x50 = email 2
		//   0x60 = email 3
		//  top bit:
		//   0x80 default on, except for oneoff entryids

		// either WAB_GUID or ONE_OFF_MUID
		if (memcmp(ptrEntries->Value.MVbin.lpbin[i].lpb + sizeof(ULONG), (void*)&WAB_GUID, sizeof(GUID)) == 0) {
			// handle wrapped entryids
			ulOffset = sizeof(ULONG) + sizeof(GUID) + sizeof(BYTE);
			cType = ptrEntries->Value.MVbin.lpbin[i].lpb[sizeof(ULONG) + sizeof(GUID)];
		}

		hr = m_lpMAPISup->OpenEntry(ptrEntries->Value.MVbin.lpbin[i].cb - ulOffset, (LPENTRYID)(ptrEntries->Value.MVbin.lpbin[i].lpb + ulOffset), NULL, 0, &ulObjType, &ptrUser);
		if (hr != hrSuccess)
			continue;

		if ((cType & 0x80) && (cType & 0x0F) < 5 && (cType & 0x0F) > 0) {
			ULONG cbEntryID;
			EntryIdPtr ptrEntryID;
			SPropValuePtr ptrPropEntryID;
			ULONG ulObjOffset = 0;
			ULONG ulObjType = 0;

			hr = HrGetOneProp(ptrUser, PR_ENTRYID, &ptrPropEntryID);
			if (hr != hrSuccess)
				goto exit;

			if ((cType & 0x0F) == 3) {
				ulObjType = MAPI_MAILUSER;
				ulObjOffset = cType >> 4;
			} else 
				ulObjType = MAPI_DISTLIST;

			hr = MakeWrappedEntryID(ptrPropEntryID->Value.bin.cb, (LPENTRYID)ptrPropEntryID->Value.bin.lpb, ulObjType, ulObjOffset, &cbEntryID, &ptrEntryID);
			if (hr != hrSuccess)
				goto exit;

			hr = ZCMAPIProp::Create(ptrUser, cbEntryID, ptrEntryID, &ptrZCMAPIProp);
			if (hr != hrSuccess)
				goto exit;

			hr = ptrZCMAPIProp->GetProps(ptrCols, 0, &cValues, &ptrProps);
			if (FAILED(hr))
				continue;

		} else {
			hr = ptrUser->GetProps(ptrCols, 0, &cValues, &ptrProps);
			if (FAILED(hr))
				continue;
		}

		ptrProps[0] = sKey;

		hr = lpTable->HrModifyRow(ECKeyTable::TABLE_ROW_ADD, NULL, ptrProps.get(), cValues);
		if (hr != hrSuccess)
			goto exit;
		++sKey.Value.ul;
	}
	hr = hrSuccess;

	AddChild(lpTable);

	hr = lpTable->HrGetView(createLocaleFromName(NULL), ulFlags, &lpTableView);
	if(hr != hrSuccess)
		goto exit;
	
	hr = lpTableView->QueryInterface(IID_IMAPITable, (void **)lppTable);

exit:
	if(lpTable)
		lpTable->Release();

	if(lpTableView)
		lpTableView->Release();

	return hr;
}


/** 
 * Returns an addressbook contents table of the IPM.Contacts folder in m_lpContactFolder.
 * 
 * @param[in] ulFlags MAPI_UNICODE for default unicode columns
 * @param[out] lppTable contents table of all items found in folder
 * 
 * @return 
 */
HRESULT ZCABContainer::GetContentsTable(ULONG ulFlags, LPMAPITABLE *lppTable)
{
	HRESULT hr = hrSuccess;

	if (m_lpDistList)
		hr = GetDistListContentsTable(ulFlags, lppTable);
	else
		hr = GetFolderContentsTable(ulFlags, lppTable);

	return hr;
}

/** 
 * Can return 3 kinds of tables:
 * 1. Root Container, contains one entry: the provider container
 * 2. Provider Container, contains user folders
 * 3. CONVENIENT_DEPTH: 1 + 2
 * 
 * @param[in] ulFlags MAPI_UNICODE | CONVENIENT_DEPTH
 * @param[out] lppTable root container table
 * 
 * @return MAPI Error code
 */
HRESULT ZCABContainer::GetHierarchyTable(ULONG ulFlags, LPMAPITABLE *lppTable)
{
	HRESULT			hr = hrSuccess;
	ECMemTable*		lpTable = NULL;
	ECMemTableView*	lpTableView = NULL;
#define TCOLS 9
	SizedSPropTagArray(TCOLS, sptaCols) = {TCOLS, {PR_ENTRYID, PR_STORE_ENTRYID, PR_DISPLAY_NAME_W, PR_OBJECT_TYPE, PR_CONTAINER_FLAGS, PR_DISPLAY_TYPE, PR_AB_PROVIDER_ID, PR_DEPTH, PR_INSTANCE_KEY}};
	enum {ENTRYID = 0, STORE_ENTRYID, DISPLAY_NAME, OBJECT_TYPE, CONTAINER_FLAGS, DISPLAY_TYPE, AB_PROVIDER_ID, DEPTH, INSTANCE_KEY, ROWID};
	std::vector<zcabFolderEntry>::const_iterator iter;
	ULONG ulInstance = 0;
	SPropValue sProps[TCOLS + 1];
	convert_context converter;

	if ((ulFlags & MAPI_UNICODE) == 0)
		sptaCols.aulPropTag[DISPLAY_NAME] = PR_DISPLAY_NAME_A;

	hr = ECMemTable::Create((LPSPropTagArray)&sptaCols, PR_ROWID, &lpTable);
	if(hr != hrSuccess)
		goto exit;

	/*
	  1. root container		: m_lpFolders = NULL, m_lpContactFolder = NULL, m_lpDistList = NULL, m_lpProvider = ZCABLogon (one entry: provider)
	  2. provider container	: m_lpFolders = data, m_lpContactFolder = NULL, m_lpDistList = NULL, m_lpProvider = ZCABLogon (N entries: folders)
	  3. contact folder		: m_lpFolders = NULL, m_lpContactFolder = data, m_lpDistList = NULL, m_lpProvider = ZCABContainer from (2), (no hierarchy entries)
	  4. distlist			: m_lpFolders = NULL, m_lpContactFolder = NULL, m_lpDistList = data, m_lpProvider = ZCABContainer from (3), (no hierarchy entries)

	  when ulFlags contains CONVENIENT_DEPTH, (1) also contains (2)
	  so we open (2) through the provider, which gives the m_lpFolders
	*/

	if (m_lpFolders) {
		// create hierarchy with folders from user stores
		for (iter = m_lpFolders->begin(); iter != m_lpFolders->end(); ++iter, ++ulInstance) {
			std::string strName;
			cabEntryID *lpEntryID = NULL;
			ULONG cbEntryID = CbNewCABENTRYID(iter->cbFolder);

			hr = MAPIAllocateBuffer(cbEntryID, (void**)&lpEntryID);
			if (hr != hrSuccess)
				goto exit;

			memset(lpEntryID, 0, cbEntryID);
			memcpy(&lpEntryID->muid, &MUIDZCSAB, sizeof(MAPIUID));
			lpEntryID->ulObjType = MAPI_ABCONT;
			lpEntryID->ulOffset = 0;
			memcpy(lpEntryID->origEntryID, iter->lpFolder, iter->cbFolder);

			sProps[ENTRYID].ulPropTag = sptaCols.aulPropTag[ENTRYID];
			sProps[ENTRYID].Value.bin.cb = cbEntryID;
			sProps[ENTRYID].Value.bin.lpb = (BYTE*)lpEntryID;

			sProps[STORE_ENTRYID].ulPropTag = CHANGE_PROP_TYPE(sptaCols.aulPropTag[STORE_ENTRYID], PT_ERROR);
			sProps[STORE_ENTRYID].Value.err = MAPI_E_NOT_FOUND;

			sProps[DISPLAY_NAME].ulPropTag = sptaCols.aulPropTag[DISPLAY_NAME];
			if ((ulFlags & MAPI_UNICODE) == 0) {
				sProps[DISPLAY_NAME].ulPropTag = PR_DISPLAY_NAME_A;
				strName = converter.convert_to<std::string>(iter->strwDisplayName);
				sProps[DISPLAY_NAME].Value.lpszA = (char*)strName.c_str();
			} else {
				sProps[DISPLAY_NAME].Value.lpszW = (WCHAR*)iter->strwDisplayName.c_str();
			}

			sProps[OBJECT_TYPE].ulPropTag = sptaCols.aulPropTag[OBJECT_TYPE];
			sProps[OBJECT_TYPE].Value.ul = MAPI_ABCONT;

			sProps[CONTAINER_FLAGS].ulPropTag = sptaCols.aulPropTag[CONTAINER_FLAGS];
			sProps[CONTAINER_FLAGS].Value.ul = AB_RECIPIENTS | AB_UNMODIFIABLE | AB_UNICODE_OK;

			sProps[DISPLAY_TYPE].ulPropTag = sptaCols.aulPropTag[DISPLAY_TYPE];
			sProps[DISPLAY_TYPE].Value.ul = DT_NOT_SPECIFIC;

			sProps[AB_PROVIDER_ID].ulPropTag = sptaCols.aulPropTag[AB_PROVIDER_ID];
			sProps[AB_PROVIDER_ID].Value.bin.cb = sizeof(GUID);
			sProps[AB_PROVIDER_ID].Value.bin.lpb = (BYTE*)&MUIDZCSAB;

			sProps[DEPTH].ulPropTag = PR_DEPTH;
			sProps[DEPTH].Value.ul = (ulFlags & CONVENIENT_DEPTH) ? 1 : 0;

			sProps[INSTANCE_KEY].ulPropTag = PR_INSTANCE_KEY;
			sProps[INSTANCE_KEY].Value.bin.cb = sizeof(ULONG);
			sProps[INSTANCE_KEY].Value.bin.lpb = (LPBYTE)&ulInstance;

			sProps[ROWID].ulPropTag = PR_ROWID;
			sProps[ROWID].Value.ul = ulInstance;

			hr = lpTable->HrModifyRow(ECKeyTable::TABLE_ROW_ADD, NULL, sProps, TCOLS + 1);

			MAPIFreeBuffer(lpEntryID);

			if (hr != hrSuccess)
				goto exit;
		}
	} else if (!m_lpContactFolder) {
		// only if not using a contacts folder, which should make the contents table. so this would return an empty hierarchy table, which is true.
		// create toplevel hierarchy. one entry: "Zarafa Contacts Folders"
		BYTE sEntryID[4 + sizeof(GUID)]; // minimum sized entryid. no extra info needed

		memset(sEntryID, 0, sizeof(sEntryID));
		memcpy(sEntryID + 4, &MUIDZCSAB, sizeof(GUID));

		sProps[ENTRYID].ulPropTag = sptaCols.aulPropTag[ENTRYID];
		sProps[ENTRYID].Value.bin.cb = sizeof(sEntryID);
		sProps[ENTRYID].Value.bin.lpb = sEntryID;

		sProps[STORE_ENTRYID].ulPropTag = CHANGE_PROP_TYPE(sptaCols.aulPropTag[STORE_ENTRYID], PT_ERROR);
		sProps[STORE_ENTRYID].Value.err = MAPI_E_NOT_FOUND;

		sProps[DISPLAY_NAME].ulPropTag = sptaCols.aulPropTag[DISPLAY_NAME];
		if ((ulFlags & MAPI_UNICODE) == 0) {
			sProps[DISPLAY_NAME].ulPropTag = PR_DISPLAY_NAME_A;
			sProps[DISPLAY_NAME].Value.lpszA = _A("Zarafa Contacts Folders");
		} else {
			sProps[DISPLAY_NAME].Value.lpszW = _W("Zarafa Contacts Folders");
		}

		sProps[OBJECT_TYPE].ulPropTag = sptaCols.aulPropTag[OBJECT_TYPE];
		sProps[OBJECT_TYPE].Value.ul = MAPI_ABCONT;

		// AB_SUBCONTAINERS flag causes this folder to be skipped in the IAddrBook::GetSearchPath()
		sProps[CONTAINER_FLAGS].ulPropTag = sptaCols.aulPropTag[CONTAINER_FLAGS];
		sProps[CONTAINER_FLAGS].Value.ul = AB_SUBCONTAINERS | AB_UNMODIFIABLE | AB_UNICODE_OK;

		sProps[DISPLAY_TYPE].ulPropTag = sptaCols.aulPropTag[DISPLAY_TYPE];
		sProps[DISPLAY_TYPE].Value.ul = DT_NOT_SPECIFIC;

		sProps[AB_PROVIDER_ID].ulPropTag = sptaCols.aulPropTag[AB_PROVIDER_ID];
		sProps[AB_PROVIDER_ID].Value.bin.cb = sizeof(GUID);
		sProps[AB_PROVIDER_ID].Value.bin.lpb = (BYTE*)&MUIDZCSAB;

		sProps[DEPTH].ulPropTag = PR_DEPTH;
		sProps[DEPTH].Value.ul = 0;

		sProps[INSTANCE_KEY].ulPropTag = PR_INSTANCE_KEY;
		sProps[INSTANCE_KEY].Value.bin.cb = sizeof(ULONG);
		sProps[INSTANCE_KEY].Value.bin.lpb = (LPBYTE)&ulInstance;

		sProps[ROWID].ulPropTag = PR_ROWID;
		sProps[ROWID].Value.ul = ulInstance++;

		hr = lpTable->HrModifyRow(ECKeyTable::TABLE_ROW_ADD, NULL, sProps, TCOLS + 1);
		if (hr != hrSuccess)
			goto exit;

		if (ulFlags & CONVENIENT_DEPTH) {
			ABContainerPtr ptrContainer;			
			ULONG ulObjType;
			MAPITablePtr ptrTable;
			SRowSetPtr	ptrRows;

			hr = ((ZCABLogon*)m_lpProvider)->OpenEntry(sizeof(sEntryID), (LPENTRYID)sEntryID, NULL, 0, &ulObjType, &ptrContainer);
			if (hr != hrSuccess)
				goto exit;

			hr = ptrContainer->GetHierarchyTable(ulFlags, &ptrTable);
			if (hr != hrSuccess)
				goto exit;

			hr = ptrTable->QueryRows(-1, 0, &ptrRows);
			if (hr != hrSuccess)
				goto exit;

			for (SRowSetPtr::size_type i = 0; i < ptrRows.size(); ++i) {
				// use PR_STORE_ENTRYID field to set instance key, since that is always MAPI_E_NOT_FOUND (see above)
				LPSPropValue lpProp = PpropFindProp(ptrRows[i].lpProps, ptrRows[i].cValues, CHANGE_PROP_TYPE(PR_STORE_ENTRYID, PT_ERROR));
				if (lpProp == NULL)
					continue;
				lpProp->ulPropTag = PR_ROWID;
				lpProp->Value.ul = ulInstance++;

				hr = lpTable->HrModifyRow(ECKeyTable::TABLE_ROW_ADD, NULL, ptrRows[i].lpProps, ptrRows[i].cValues);
				if (hr != hrSuccess)
					goto exit;
			}
		}
	}

	AddChild(lpTable);

	hr = lpTable->HrGetView(createLocaleFromName(NULL), ulFlags, &lpTableView);
	if(hr != hrSuccess)
		goto exit;
		
	hr = lpTableView->QueryInterface(IID_IMAPITable, (void **)lppTable);

exit:
	if(lpTable)
		lpTable->Release();

	if(lpTableView)
		lpTableView->Release();

	return hr;
#undef TCOLS
}

/** 
 * Opens the contact from any given contact folder, and makes a ZCMAPIProp object for that contact.
 * 
 * @param[in] cbEntryID wrapped contact entryid bytes
 * @param[in] lpEntryID 
 * @param[in] lpInterface requested interface
 * @param[in] ulFlags unicode flags
 * @param[out] lpulObjType returned object type
 * @param[out] lppUnk returned object
 * 
 * @return MAPI Error code
 */
HRESULT ZCABContainer::OpenEntry(ULONG cbEntryID, LPENTRYID lpEntryID, LPCIID lpInterface, ULONG ulFlags, ULONG *lpulObjType, LPUNKNOWN *lppUnk)
{
	HRESULT hr = hrSuccess;
	cabEntryID *lpCABEntryID = (cabEntryID*)lpEntryID;
	ULONG cbNewCABEntryID = CbNewCABENTRYID(0);
	ULONG cbFolder = 0;
	LPENTRYID lpFolder = NULL;
	ULONG ulObjType = 0;
	MAPIFolderPtr ptrContactFolder;
	ZCABContainer *lpZCABContacts = NULL;
	MessagePtr ptrContact;
	ZCMAPIProp *lpZCMAPIProp = NULL;

	if (cbEntryID < cbNewCABEntryID || memcmp((LPBYTE)&lpCABEntryID->muid, &MUIDZCSAB, sizeof(MAPIUID)) != 0) {
		hr = MAPI_E_UNKNOWN_ENTRYID;
		goto exit;
	}

	if (m_lpDistList) {
		// there is nothing to open from the distlist point of view
		// @todo, passthrough to IMAPISupport object?
		hr = MAPI_E_NO_SUPPORT;
		goto exit;
	}

	cbFolder = cbEntryID - cbNewCABEntryID;
	lpFolder = (LPENTRYID)((LPBYTE)lpEntryID + cbNewCABEntryID);

	if (lpCABEntryID->ulObjType == MAPI_ABCONT) {
		hr = m_lpMAPISup->OpenEntry(cbFolder, lpFolder, NULL, 0, &ulObjType, &ptrContactFolder);
		if (hr == MAPI_E_NOT_FOUND) {
			// the folder is most likely in a store that is not yet available through this MAPI session
			// try opening the store through the support object, and see if we can get it anyway
			MsgStorePtr ptrStore;
			MAPIGetSessionPtr ptrGetSession;
			MAPISessionPtr ptrSession;

			hr = m_lpMAPISup->QueryInterface(IID_IMAPIGetSession, (void**)&ptrGetSession);
			if (hr != hrSuccess)
				goto exit;

			hr = ptrGetSession->GetMAPISession(&ptrSession);
			if (hr != hrSuccess)
				goto exit;

			std::vector<zcabFolderEntry>::const_iterator i;
			// find the store of this folder
			for (i = m_lpFolders->begin(); i != m_lpFolders->end(); ++i) {
				ULONG res;
				if ((m_lpMAPISup->CompareEntryIDs(i->cbFolder, (LPENTRYID)i->lpFolder, cbFolder, lpFolder, 0, &res) == hrSuccess) && res == TRUE)
					break;
			}
			if (i == m_lpFolders->end()) {
				hr = MAPI_E_NOT_FOUND;
				goto exit;
			}

			hr = ptrSession->OpenMsgStore(0, i->cbStore, (LPENTRYID)i->lpStore, NULL, 0, &ptrStore);
			if (hr != hrSuccess)
				goto exit;

			hr = ptrStore->OpenEntry(cbFolder, lpFolder, NULL, 0, &ulObjType, &ptrContactFolder);
		}
		if (hr != hrSuccess)
			goto exit;

		hr = ZCABContainer::Create(NULL, ptrContactFolder, m_lpMAPISup, m_lpProvider, &lpZCABContacts);
		if (hr != hrSuccess)
			goto exit;

		AddChild(lpZCABContacts);

		if (lpInterface)
			hr = lpZCABContacts->QueryInterface(*lpInterface, (void**)lppUnk);
		else
			hr = lpZCABContacts->QueryInterface(IID_IABContainer, (void**)lppUnk);
	} else if (lpCABEntryID->ulObjType == MAPI_DISTLIST) {
		// open the Original Message
		hr = m_lpMAPISup->OpenEntry(cbFolder, lpFolder, NULL, 0, &ulObjType, &ptrContact);
		if (hr != hrSuccess)
			goto exit;
		
		hr = ZCABContainer::Create(ptrContact, cbEntryID, lpEntryID, m_lpMAPISup, &lpZCABContacts);
		if (hr != hrSuccess)
			goto exit;

		AddChild(lpZCABContacts);

		if (lpInterface)
			hr = lpZCABContacts->QueryInterface(*lpInterface, (void**)lppUnk);
		else
			hr = lpZCABContacts->QueryInterface(IID_IDistList, (void**)lppUnk);
	} else if (lpCABEntryID->ulObjType == MAPI_MAILUSER) {
		// open the Original Message
		hr = m_lpMAPISup->OpenEntry(cbFolder, lpFolder, NULL, 0, &ulObjType, &ptrContact);
		if (hr != hrSuccess)
			goto exit;

		hr = ZCMAPIProp::Create(ptrContact, cbEntryID, lpEntryID, &lpZCMAPIProp);
		if (hr != hrSuccess)
			goto exit;

		AddChild(lpZCMAPIProp);

		if (lpInterface)
			hr = lpZCMAPIProp->QueryInterface(*lpInterface, (void**)lppUnk);
		else
			hr = lpZCMAPIProp->QueryInterface(IID_IMAPIProp, (void**)lppUnk);
	} else {
		hr = MAPI_E_UNKNOWN_ENTRYID;
		goto exit;
	}

	*lpulObjType = lpCABEntryID->ulObjType;

exit:
	if (lpZCMAPIProp)
		lpZCMAPIProp->Release();

	if (lpZCABContacts)
		lpZCABContacts->Release();

	return hr;
}

HRESULT ZCABContainer::SetSearchCriteria(LPSRestriction lpRestriction, LPENTRYLIST lpContainerList, ULONG ulSearchFlags)
{
	return MAPI_E_NO_SUPPORT;
}

HRESULT ZCABContainer::GetSearchCriteria(ULONG ulFlags, LPSRestriction *lppRestriction, LPENTRYLIST *lppContainerList, ULONG *lpulSearchState)
{
	return MAPI_E_NO_SUPPORT;
}

/////////////////////////////////////////////////
// IABContainer
//

HRESULT ZCABContainer::CreateEntry(ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulCreateFlags, LPMAPIPROP* lppMAPIPropEntry)
{
	return MAPI_E_NO_SUPPORT;
}

HRESULT ZCABContainer::CopyEntries(LPENTRYLIST lpEntries, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags)
{
	return MAPI_E_NO_SUPPORT;
}

HRESULT ZCABContainer::DeleteEntries(LPENTRYLIST lpEntries, ULONG ulFlags)
{
	return MAPI_E_NO_SUPPORT;
}

/** 
 * Resolve MAPI_UNRESOLVED items in lpAdrList and possebly add resolved
 * 
 * @param[in] lpPropTagArray properties to be included in lpAdrList
 * @param[in] ulFlags EMS_AB_ADDRESS_LOOKUP | MAPI_UNICODE
 * @param[in,out] lpAdrList 
 * @param[in,out] lpFlagList MAPI_AMBIGUOUS | MAPI_RESOLVED | MAPI_UNRESOLVED
 * 
 * @return 
 */
HRESULT ZCABContainer::ResolveNames(LPSPropTagArray lpPropTagArray, ULONG ulFlags, LPADRLIST lpAdrList, LPFlagList lpFlagList)
{
	HRESULT hr = hrSuccess;
	// only columns we can set from our contents table
	SizedSPropTagArray(7, sptaDefault) = {7, {PR_ADDRTYPE_A, PR_DISPLAY_NAME_A, PR_DISPLAY_TYPE, PR_EMAIL_ADDRESS_A, PR_ENTRYID, PR_INSTANCE_KEY, PR_OBJECT_TYPE}};
	SizedSPropTagArray(7, sptaUnicode) = {7, {PR_ADDRTYPE_W, PR_DISPLAY_NAME_W, PR_DISPLAY_TYPE, PR_EMAIL_ADDRESS_W, PR_ENTRYID, PR_INSTANCE_KEY, PR_OBJECT_TYPE}};
	ULONG i;
	SRowSetPtr	ptrRows;

	if (lpPropTagArray == NULL) {
		if(ulFlags & MAPI_UNICODE)
			lpPropTagArray = (LPSPropTagArray)&sptaUnicode;
		else
			lpPropTagArray = (LPSPropTagArray)&sptaDefault;
	}

	// in this container table, find given PR_DISPLAY_NAME

	if (m_lpFolders) {
		// return MAPI_E_NO_SUPPORT ? since you should not query on this level

		// @todo search in all folders, loop over self, since we want this providers entry ids
		MAPITablePtr ptrHierarchy;

		if (m_lpFolders->empty())
			goto exit;
		
		hr = this->GetHierarchyTable(0, &ptrHierarchy);
		if (hr != hrSuccess)
			goto exit;

		hr = ptrHierarchy->QueryRows(m_lpFolders->size(), 0, &ptrRows);
		if (hr != hrSuccess)
			goto exit;

		for (i = 0; i < ptrRows.size(); ++i) {
			ABContainerPtr ptrContainer;
			LPSPropValue lpEntryID = PpropFindProp(ptrRows[i].lpProps, ptrRows[i].cValues, PR_ENTRYID);
			ULONG ulObjType;

			if (!lpEntryID)
				continue;

			// this? provider?
			hr = this->OpenEntry(lpEntryID->Value.bin.cb, (LPENTRYID)lpEntryID->Value.bin.lpb, NULL, 0, &ulObjType, &ptrContainer);
			if (hr != hrSuccess)
				goto exit;

			hr = ptrContainer->ResolveNames(lpPropTagArray, ulFlags, lpAdrList, lpFlagList);
			if (hr != hrSuccess)
				goto exit;
		}
	} else if (m_lpContactFolder) {
		// only search within this folder and set entries in lpAdrList / lpFlagList
		MAPITablePtr ptrContents;
		std::set<ULONG> stProps;
		SPropTagArrayPtr ptrColumns;

		// make joint proptags
		std::copy(lpPropTagArray->aulPropTag, lpPropTagArray->aulPropTag + lpPropTagArray->cValues, std::inserter(stProps, stProps.begin()));
		for (i = 0; i < lpAdrList->aEntries[0].cValues; ++i)
			stProps.insert(lpAdrList->aEntries[0].rgPropVals[i].ulPropTag);
		hr = MAPIAllocateBuffer(CbNewSPropTagArray(stProps.size()), &ptrColumns);
		if (hr != hrSuccess)
			goto exit;
		ptrColumns->cValues = stProps.size();
		std::copy(stProps.begin(), stProps.end(), ptrColumns->aulPropTag);


		hr = this->GetContentsTable(ulFlags & MAPI_UNICODE, &ptrContents);
		if (hr != hrSuccess)
			goto exit;

		hr = ptrContents->SetColumns(ptrColumns, 0);
		if (hr != hrSuccess)
			goto exit;

		for (i = 0; i < lpAdrList->cEntries; ++i) {
			LPSPropValue lpDisplayNameA = PpropFindProp(lpAdrList->aEntries[i].rgPropVals, lpAdrList->aEntries[i].cValues, PR_DISPLAY_NAME_A);
			LPSPropValue lpDisplayNameW = PpropFindProp(lpAdrList->aEntries[i].rgPropVals, lpAdrList->aEntries[i].cValues, PR_DISPLAY_NAME_W);

			if (!lpDisplayNameA && !lpDisplayNameW)
				continue;

			ULONG ulResFlag = (ulFlags & EMS_AB_ADDRESS_LOOKUP) ? FL_FULLSTRING : FL_PREFIX | FL_IGNORECASE;
			ULONG ulStringType = lpDisplayNameW ? PT_UNICODE : PT_STRING8;
			SPropValue sProp = lpDisplayNameW ? *lpDisplayNameW : *lpDisplayNameA;

			ECOrRestriction resFind;
			ULONG ulSearchTags[] = {PR_DISPLAY_NAME, PR_EMAIL_ADDRESS, PR_ORIGINAL_DISPLAY_NAME};

			for (ULONG j = 0; j < arraySize(ulSearchTags); ++j) {
				sProp.ulPropTag = CHANGE_PROP_TYPE(ulSearchTags[j], ulStringType);
				resFind.append( ECContentRestriction(ulResFlag, CHANGE_PROP_TYPE(ulSearchTags[j], ulStringType), &sProp, ECRestriction::Cheap) );
			}

			SRestrictionPtr ptrRestriction;
			hr = resFind.CreateMAPIRestriction(&ptrRestriction);
			if (hr != hrSuccess)
				goto exit;

			hr = ptrContents->Restrict(ptrRestriction, 0);
			if (hr != hrSuccess)
				goto exit;

			hr = ptrContents->QueryRows(-1, MAPI_UNICODE, &ptrRows);
			if (hr != hrSuccess)
				goto exit;

			if (ptrRows.size() == 1) {
				lpFlagList->ulFlag[i] = MAPI_RESOLVED;

				// release old row
				MAPIFreeBuffer(lpAdrList->aEntries[i].rgPropVals);
				lpAdrList->aEntries[i].rgPropVals = NULL;

				hr = Util::HrCopySRow((LPSRow)&lpAdrList->aEntries[i], &ptrRows[0], NULL);
				if (hr != hrSuccess)
					goto exit;

			} else if (ptrRows.size() > 1) {
				lpFlagList->ulFlag[i] = MAPI_AMBIGUOUS;
			}
		}
	} else {
		// not supported within MAPI_DISTLIST container
		hr = MAPI_E_NO_SUPPORT;
	}

exit:
	return hr;
}

// IMAPIProp for m_lpDistList
HRESULT ZCABContainer::GetProps(LPSPropTagArray lpPropTagArray, ULONG ulFlags, ULONG FAR * lpcValues, LPSPropValue FAR * lppPropArray)
{
	HRESULT hr = MAPI_E_NO_SUPPORT;
	if (m_lpDistList)
		hr = m_lpDistList->GetProps(lpPropTagArray, ulFlags, lpcValues, lppPropArray);
	return hr;
}

HRESULT ZCABContainer::GetPropList(ULONG ulFlags, LPSPropTagArray FAR * lppPropTagArray)
{
	HRESULT hr = MAPI_E_NO_SUPPORT;
	if (m_lpDistList)
		hr = m_lpDistList->GetPropList(ulFlags, lppPropTagArray);
	return hr;
}


//////////////////////////////////
// Interface IUnknown
//
HRESULT ZCABContainer::xABContainer::QueryInterface(REFIID refiid , void** lppInterface)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::QueryInterface", "%s", DBGGUIDToString(refiid).c_str());
	METHOD_PROLOGUE_(ZCABContainer, ABContainer);
	HRESULT hr = pThis->QueryInterface(refiid,lppInterface);
	TRACE_MAPI(TRACE_RETURN, "IABContainer::QueryInterface", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

ULONG ZCABContainer::xABContainer::AddRef()
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::AddRef", "");
	METHOD_PROLOGUE_(ZCABContainer, ABContainer);
	return pThis->AddRef();
}

ULONG ZCABContainer::xABContainer::Release()
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::Release", "");
	METHOD_PROLOGUE_(ZCABContainer, ABContainer);
	ULONG ulRef = pThis->Release();
	TRACE_MAPI(TRACE_RETURN, "IABContainer::Release", "%d", ulRef);
	return ulRef;
}

//////////////////////////////////
// Interface IABContainer
//
HRESULT ZCABContainer::xABContainer::CreateEntry(ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulCreateFlags, LPMAPIPROP* lppMAPIPropEntry)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::CreateEntry", "");
	METHOD_PROLOGUE_(ZCABContainer, ABContainer);
	HRESULT hr = pThis->CreateEntry(cbEntryID, lpEntryID, ulCreateFlags, lppMAPIPropEntry);
	TRACE_MAPI(TRACE_RETURN, "IABContainer::CreateEntry", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ZCABContainer::xABContainer::CopyEntries(LPENTRYLIST lpEntries, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::CopyEntries", "");
	METHOD_PROLOGUE_(ZCABContainer, ABContainer);
	HRESULT hr = pThis->CopyEntries(lpEntries, ulUIParam, lpProgress, ulFlags);
	TRACE_MAPI(TRACE_RETURN, "IABContainer::CopyEntries", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ZCABContainer::xABContainer::DeleteEntries(LPENTRYLIST lpEntries, ULONG ulFlags)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::DeleteEntries", "");
	METHOD_PROLOGUE_(ZCABContainer, ABContainer);
	HRESULT hr = pThis->DeleteEntries(lpEntries, ulFlags);
	TRACE_MAPI(TRACE_RETURN, "IABContainer::DeleteEntries", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ZCABContainer::xABContainer::ResolveNames(LPSPropTagArray lpPropTagArray, ULONG ulFlags, LPADRLIST lpAdrList, LPFlagList lpFlagList)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::ResolveNames", "\nlpPropTagArray:\t%s\nlpAdrList:\t%s", PropNameFromPropTagArray(lpPropTagArray).c_str(), AdrRowSetToString(lpAdrList, lpFlagList).c_str() );
	METHOD_PROLOGUE_(ZCABContainer, ABContainer);
	HRESULT hr = pThis->ResolveNames(lpPropTagArray, ulFlags, lpAdrList, lpFlagList);
	TRACE_MAPI(TRACE_RETURN, "IABContainer::ResolveNames", "%s, lpadrlist=\n%s", GetMAPIErrorDescription(hr).c_str(), AdrRowSetToString(lpAdrList, lpFlagList).c_str() );
	return hr;
}

//////////////////////////////////
// Interface IMAPIContainer
//
HRESULT ZCABContainer::xABContainer::GetContentsTable(ULONG ulFlags, LPMAPITABLE *lppTable)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::GetContentsTable", "");
	METHOD_PROLOGUE_(ZCABContainer, ABContainer);
	HRESULT hr = pThis->GetContentsTable(ulFlags, lppTable);
	TRACE_MAPI(TRACE_RETURN, "IABContainer::GetContentsTable", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ZCABContainer::xABContainer::GetHierarchyTable(ULONG ulFlags, LPMAPITABLE *lppTable)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::GetHierarchyTable", ""); 
	METHOD_PROLOGUE_(ZCABContainer, ABContainer);
	HRESULT hr = pThis->GetHierarchyTable(ulFlags, lppTable);
	TRACE_MAPI(TRACE_RETURN, "IABContainer::GetHierarchyTable", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ZCABContainer::xABContainer::OpenEntry(ULONG cbEntryID, LPENTRYID lpEntryID, LPCIID lpInterface, ULONG ulFlags, ULONG *lpulObjType, LPUNKNOWN *lppUnk)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::OpenEntry", "interface=%s", (lpInterface)?DBGGUIDToString(*lpInterface).c_str():"NULL");
	METHOD_PROLOGUE_(ZCABContainer, ABContainer);
	HRESULT hr = pThis->OpenEntry(cbEntryID, lpEntryID, lpInterface, ulFlags, lpulObjType, lppUnk);
	TRACE_MAPI(TRACE_RETURN, "IABContainer::OpenEntry", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ZCABContainer::xABContainer::SetSearchCriteria(LPSRestriction lpRestriction, LPENTRYLIST lpContainerList, ULONG ulSearchFlags)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::SetSearchCriteria", "");
	METHOD_PROLOGUE_(ZCABContainer, ABContainer);
	HRESULT hr = pThis->SetSearchCriteria(lpRestriction, lpContainerList, ulSearchFlags);
	TRACE_MAPI(TRACE_RETURN, "IABContainer::SetSearchCriteria", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ZCABContainer::xABContainer::GetSearchCriteria(ULONG ulFlags, LPSRestriction *lppRestriction, LPENTRYLIST *lppContainerList, ULONG *lpulSearchState)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::GetSearchCriteria", "");
	METHOD_PROLOGUE_(ZCABContainer, ABContainer);
	HRESULT hr = pThis->GetSearchCriteria(ulFlags, lppRestriction, lppContainerList, lpulSearchState);
	TRACE_MAPI(TRACE_RETURN, "IABContainer::GetSearchCriteria", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

////////////////////////////////
// Interface IMAPIProp
//
HRESULT ZCABContainer::xABContainer::GetLastError(HRESULT hError, ULONG ulFlags, LPMAPIERROR * lppMapiError)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::GetLastError", "");
	HRESULT hr = MAPI_E_NO_SUPPORT;
	TRACE_MAPI(TRACE_RETURN, "IABContainer::GetLastError", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ZCABContainer::xABContainer::SaveChanges(ULONG ulFlags)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::SaveChanges", "");
	HRESULT hr = MAPI_E_NO_SUPPORT;
	TRACE_MAPI(TRACE_RETURN, "IABContainer::SaveChanges", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ZCABContainer::xABContainer::GetProps(LPSPropTagArray lpPropTagArray, ULONG ulFlags, ULONG FAR * lpcValues, LPSPropValue FAR * lppPropArray)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::GetProps", "%s", PropNameFromPropTagArray(lpPropTagArray).c_str());
	METHOD_PROLOGUE_(ZCABContainer, ABContainer);
	HRESULT hr = pThis->GetProps(lpPropTagArray, ulFlags, lpcValues, lppPropArray);
	TRACE_MAPI(TRACE_RETURN, "IABContainer::GetProps", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ZCABContainer::xABContainer::GetPropList(ULONG ulFlags, LPSPropTagArray FAR * lppPropTagArray)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::GetPropList", "");
	METHOD_PROLOGUE_(ZCABContainer, ABContainer);
	HRESULT hr = pThis->GetPropList(ulFlags, lppPropTagArray);
	TRACE_MAPI(TRACE_RETURN, "IABContainer::GetPropList", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ZCABContainer::xABContainer::OpenProperty(ULONG ulPropTag, LPCIID lpiid, ULONG ulInterfaceOptions, ULONG ulFlags, LPUNKNOWN FAR * lppUnk)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::OpenProperty", "PropTag=%s, lpiid=%s", PropNameFromPropTag(ulPropTag).c_str(), DBGGUIDToString(*lpiid).c_str());
	HRESULT hr = MAPI_E_NO_SUPPORT;
	TRACE_MAPI(TRACE_RETURN, "IABContainer::OpenProperty", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ZCABContainer::xABContainer::SetProps(ULONG cValues, LPSPropValue lpPropArray, LPSPropProblemArray FAR * lppProblems)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::SetProps", "");
	HRESULT hr = MAPI_E_NO_SUPPORT;
	TRACE_MAPI(TRACE_RETURN, "IABContainer::SetProps", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ZCABContainer::xABContainer::DeleteProps(LPSPropTagArray lpPropTagArray, LPSPropProblemArray FAR * lppProblems)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::DeleteProps", "%s", PropNameFromPropTagArray(lpPropTagArray).c_str());
	HRESULT hr = MAPI_E_NO_SUPPORT;
	TRACE_MAPI(TRACE_RETURN, "IABContainer::DeleteProps", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ZCABContainer::xABContainer::CopyTo(ULONG ciidExclude, LPCIID rgiidExclude, LPSPropTagArray lpExcludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray FAR * lppProblems)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::CopyTo", "");
	HRESULT hr = MAPI_E_NO_SUPPORT;
	TRACE_MAPI(TRACE_RETURN, "IABContainer::CopyTo", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ZCABContainer::xABContainer::CopyProps(LPSPropTagArray lpIncludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray FAR * lppProblems)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::CopyProps", "");
	HRESULT hr = MAPI_E_NO_SUPPORT;
	TRACE_MAPI(TRACE_RETURN, "IABContainer::CopyProps", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ZCABContainer::xABContainer::GetNamesFromIDs(LPSPropTagArray * pptaga, LPGUID lpguid, ULONG ulFlags, ULONG * pcNames, LPMAPINAMEID ** pppNames)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::GetNamesFromIDs", "");
	HRESULT hr = MAPI_E_NO_SUPPORT;
	TRACE_MAPI(TRACE_RETURN, "IABContainer::GetIDsFromNames", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ZCABContainer::xABContainer::GetIDsFromNames(ULONG cNames, LPMAPINAMEID * ppNames, ULONG ulFlags, LPSPropTagArray * pptaga)
{
	TRACE_MAPI(TRACE_ENTRY, "IABContainer::GetIDsFromNames", "");
	HRESULT hr = MAPI_E_NO_SUPPORT;
	TRACE_MAPI(TRACE_RETURN, "IABContainer::GetIDsFromNames", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

