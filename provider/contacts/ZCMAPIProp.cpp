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
#include "ZCMAPIProp.h"
#include "ZCABData.h"

#include <mapidefs.h>
#include <mapiguid.h>
#include <mapicode.h>
#include <mapiutil.h>

#include <zarafa/Util.h>
#include <zarafa/ECGuid.h>
#include <zarafa/mapi_ptr.h>
#include <zarafa/namedprops.h>
#include <zarafa/mapiguidext.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

ZCMAPIProp::ZCMAPIProp(ULONG ulObjType, const char *szClassName) :
    ECUnknown(szClassName), m_ulObject(ulObjType)
{
	m_base = NULL;
	empty[0] = 0;
}

ZCMAPIProp::~ZCMAPIProp()
{
	MAPIFreeBuffer(m_base);
}

#define ADD_PROP_OR_EXIT(dest, src, base, propid) {						\
	if (src) {															\
		hr = Util::HrCopyProperty(&dest, src, base);					\
		if (hr != hrSuccess)											\
			goto exit;													\
		dest.ulPropTag = propid;										\
		m_mapProperties.insert(std::make_pair(PROP_ID(propid), dest));	\
		src = NULL;														\
	} \
}

/** 
 * Add properties required to make an IMailUser. Properties are the
 * same list as Outlook Contacts Provider does.
 * 
 * @param[in] cValues number of props in lpProps
 * @param[in] lpProps properties of the original contact
 * 
 * @return 
 */
HRESULT ZCMAPIProp::ConvertMailUser(LPSPropTagArray lpNames, ULONG cValues, LPSPropValue lpProps, ULONG ulIndex)
{
	HRESULT hr = hrSuccess;
	LPSPropValue lpProp = NULL;
	SPropValue sValue, sSource;
	std::string strSearchKey;
	convert_context converter;

	lpProp = PpropFindProp(lpProps, cValues, PR_BODY);
	if (lpProp) {
		ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_BODY);
	} else {
		sValue.ulPropTag = PR_BODY;
		sValue.Value.lpszW = empty;
		m_mapProperties.insert(std::make_pair(PROP_ID(PR_BODY), sValue));
	}

	lpProp = PpropFindProp(lpProps, cValues, PR_BUSINESS_ADDRESS_CITY);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_BUSINESS_ADDRESS_CITY);

	lpProp = PpropFindProp(lpProps, cValues, PR_BUSINESS_ADDRESS_STATE_OR_PROVINCE);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_BUSINESS_ADDRESS_STATE_OR_PROVINCE);

	lpProp = PpropFindProp(lpProps, cValues, PR_BUSINESS_FAX_NUMBER);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_BUSINESS_FAX_NUMBER);

	lpProp = PpropFindProp(lpProps, cValues, PR_COMPANY_NAME);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_COMPANY_NAME);

	if (lpNames)
		lpProp = PpropFindProp(lpProps, cValues, CHANGE_PROP_TYPE(lpNames->aulPropTag[0], PT_UNICODE));
	if (!lpProp)
		lpProp = PpropFindProp(lpProps, cValues, PR_DISPLAY_NAME);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_DISPLAY_NAME);

	sValue.ulPropTag = PR_DISPLAY_TYPE;
	sValue.Value.ul = DT_MAILUSER;
	m_mapProperties.insert(std::make_pair(PROP_ID(PR_DISPLAY_TYPE), sValue));

	if (lpNames)
		lpProp = PpropFindProp(lpProps, cValues, CHANGE_PROP_TYPE(lpNames->aulPropTag[1], PT_UNICODE));
	if (!lpProp)
		lpProp = PpropFindProp(lpProps, cValues, PR_ADDRTYPE);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_ADDRTYPE);

	if (lpNames)
		lpProp = PpropFindProp(lpProps, cValues, CHANGE_PROP_TYPE(lpNames->aulPropTag[2], PT_UNICODE));
	if (!lpProp)
		lpProp = PpropFindProp(lpProps, cValues, PR_EMAIL_ADDRESS);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_EMAIL_ADDRESS);

	lpProp = PpropFindProp(lpProps, cValues, PR_GIVEN_NAME);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_GIVEN_NAME);

	lpProp = PpropFindProp(lpProps, cValues, PR_MIDDLE_NAME);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_MIDDLE_NAME);

	lpProp = PpropFindProp(lpProps, cValues, PR_NORMALIZED_SUBJECT);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_NORMALIZED_SUBJECT);

	sValue.ulPropTag = PR_OBJECT_TYPE;
	sValue.Value.ul = MAPI_MAILUSER;
	m_mapProperties.insert(std::make_pair(PROP_ID(PR_OBJECT_TYPE), sValue));

	if (lpNames)
		lpProp = PpropFindProp(lpProps, cValues, CHANGE_PROP_TYPE(lpNames->aulPropTag[3], PT_UNICODE));
	if (!lpProp)
		lpProp = PpropFindProp(lpProps, cValues, PR_DISPLAY_NAME);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_ORIGINAL_DISPLAY_NAME);

	if (lpNames)
		lpProp = PpropFindProp(lpProps, cValues, CHANGE_PROP_TYPE(lpNames->aulPropTag[4], PT_BINARY));
	if (!lpProp)
		lpProp = PpropFindProp(lpProps, cValues, PR_ENTRYID);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_ORIGINAL_ENTRYID);

	lpProp = PpropFindProp(lpProps, cValues, PR_RECORD_KEY);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_RECORD_KEY);

	if (lpNames) {
		lpProp = PpropFindProp(lpProps, cValues, CHANGE_PROP_TYPE(lpNames->aulPropTag[1], PT_UNICODE));
		if (lpProp) {
			strSearchKey += converter.convert_to<std::string>(lpProp->Value.lpszW) + ":";
		} else {
			strSearchKey += "SMTP:";
		}
		lpProp = PpropFindProp(lpProps, cValues, CHANGE_PROP_TYPE(lpNames->aulPropTag[2], PT_UNICODE));
		if (lpProp)
			strSearchKey += converter.convert_to<std::string>(lpProp->Value.lpszW);

		sSource.ulPropTag = PR_SEARCH_KEY;
		sSource.Value.bin.cb = strSearchKey.size();
		sSource.Value.bin.lpb = (LPBYTE)strSearchKey.data();
		lpProp = &sSource;
		ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_SEARCH_KEY);
	}

	lpProp = PpropFindProp(lpProps, cValues, PR_TITLE);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_TITLE);

	lpProp = PpropFindProp(lpProps, cValues, PR_TRANSMITABLE_DISPLAY_NAME);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_TRANSMITABLE_DISPLAY_NAME);

	lpProp = PpropFindProp(lpProps, cValues, PR_ENTRYID);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_ZC_ORIGINAL_ENTRYID);

	lpProp = PpropFindProp(lpProps, cValues, PR_PARENT_ENTRYID);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_ZC_ORIGINAL_PARENT_ENTRYID);

	lpProp = PpropFindProp(lpProps, cValues, PR_SOURCE_KEY);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_ZC_ORIGINAL_SOURCE_KEY);

	lpProp = PpropFindProp(lpProps, cValues, PR_PARENT_SOURCE_KEY);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_ZC_ORIGINAL_PARENT_SOURCE_KEY);

	lpProp = PpropFindProp(lpProps, cValues, PR_CHANGE_KEY);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_ZC_ORIGINAL_CHANGE_KEY);

exit:
	return hr;
}

HRESULT ZCMAPIProp::ConvertDistList(LPSPropTagArray lpNames, ULONG cValues, LPSPropValue lpProps)
{
	HRESULT hr = hrSuccess;
	LPSPropValue lpProp = NULL;
	SPropValue sValue, sSource;

	sSource.ulPropTag = PR_ADDRTYPE;
	sSource.Value.lpszW = const_cast<wchar_t *>(L"MAPIPDL");
	lpProp = &sSource;
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_ADDRTYPE);

	lpProp = PpropFindProp(lpProps, cValues, PR_DISPLAY_NAME);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_DISPLAY_NAME);

	sValue.ulPropTag = PR_DISPLAY_TYPE;
	sValue.Value.ul = DT_PRIVATE_DISTLIST;
	m_mapProperties.insert(std::make_pair(PROP_ID(PR_DISPLAY_TYPE), sValue));

	sValue.ulPropTag = PR_OBJECT_TYPE;
	sValue.Value.ul = MAPI_DISTLIST;
	m_mapProperties.insert(std::make_pair(PROP_ID(PR_OBJECT_TYPE), sValue));

	lpProp = PpropFindProp(lpProps, cValues, PR_RECORD_KEY);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_RECORD_KEY);

	// above are all the props present in the Outlook Contact Provider ..
	// but I need the props below too

	// one off members in 0x81041102
	lpProp = PpropFindProp(lpProps, cValues, 0x81041102);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, 0x81041102);

	// real eid members in 0x81051102 (gab, maybe I only need this one)
	lpProp = PpropFindProp(lpProps, cValues, 0x81051102);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, 0x81051102);

	lpProp = PpropFindProp(lpProps, cValues, PR_ENTRYID);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_ZC_ORIGINAL_ENTRYID);

	lpProp = PpropFindProp(lpProps, cValues, PR_PARENT_ENTRYID);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_ZC_ORIGINAL_PARENT_ENTRYID);

	lpProp = PpropFindProp(lpProps, cValues, PR_SOURCE_KEY);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_ZC_ORIGINAL_SOURCE_KEY);

	lpProp = PpropFindProp(lpProps, cValues, PR_PARENT_SOURCE_KEY);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_ZC_ORIGINAL_PARENT_SOURCE_KEY);

	lpProp = PpropFindProp(lpProps, cValues, PR_CHANGE_KEY);
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_ZC_ORIGINAL_CHANGE_KEY);

exit:
	return hr;
}

/** 
 * Get Props from the contact and remembers them is this object.
 * 
 * @param[in] lpContact 
 * @param[in] ulIndex index in named properties
 * 
 * @return 
 */
HRESULT ZCMAPIProp::ConvertProps(IMAPIProp *lpContact, ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulIndex)
{
	HRESULT hr = hrSuccess;
	ULONG cValues = 0;
	SPropValuePtr ptrContactProps;
	SPropValue sValue, sSource;
	LPSPropValue lpProp = NULL;

	// named properties
	SPropTagArrayPtr ptrNameTags;
	LPMAPINAMEID *lppNames = NULL;
	ULONG ulNames = 5;
	MAPINAMEID mnNamedProps[5] = {
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidEmail1DisplayName}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidEmail1AddressType}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidEmail1Address}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidEmail1OriginalDisplayName}},
		{(LPGUID)&PSETID_Address, MNID_ID, {dispidEmail1OriginalEntryID}}
	};

	hr = MAPIAllocateBuffer(sizeof(LPMAPINAMEID) * (ulNames), (void**)&lppNames);
	if (hr != hrSuccess)
		goto exit;

	if (ulIndex < 3) {
		for (ULONG i = 0; i < ulNames; ++i) {
			mnNamedProps[i].Kind.lID += (ulIndex * 0x10);
			lppNames[i] = &mnNamedProps[i];
		}

		hr = lpContact->GetIDsFromNames(ulNames, lppNames, MAPI_CREATE, &ptrNameTags);
		if (FAILED(hr))
			goto exit;
	}

	hr = lpContact->GetProps(NULL, MAPI_UNICODE, &cValues, &ptrContactProps);
	if (FAILED(hr))
		goto exit;

	hr = MAPIAllocateBuffer(sizeof(SPropValue), (void**)&m_base);
	if (hr != hrSuccess)
		goto exit;

	sSource.ulPropTag = PR_ENTRYID;
	sSource.Value.bin.cb = cbEntryID;
	sSource.Value.bin.lpb = (LPBYTE)lpEntryID;
	lpProp = &sSource;
	ADD_PROP_OR_EXIT(sValue, lpProp, m_base, PR_ENTRYID);

	if (m_ulObject == MAPI_MAILUSER)
		hr = ConvertMailUser(ptrNameTags, cValues, ptrContactProps, ulIndex);
	else
		hr = ConvertDistList(ptrNameTags, cValues, ptrContactProps);

exit:
	MAPIFreeBuffer(lppNames);
	return hr;
}

HRESULT ZCMAPIProp::Create(IMAPIProp *lpContact, ULONG cbEntryID, LPENTRYID lpEntryID, ZCMAPIProp **lppZCMAPIProp)
{
	HRESULT	hr = hrSuccess;
	ZCMAPIProp *lpZCMAPIProp = NULL;
	cabEntryID *lpCABEntryID = (cabEntryID*)lpEntryID;

	if (lpCABEntryID->ulObjType != MAPI_MAILUSER && lpCABEntryID->ulObjType != MAPI_DISTLIST) {
		hr = MAPI_E_INVALID_OBJECT;
		goto exit;
	}

	try {
		lpZCMAPIProp = new ZCMAPIProp(lpCABEntryID->ulObjType);
	}
	catch (...) {
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}

	hr = lpZCMAPIProp->ConvertProps(lpContact, cbEntryID, lpEntryID, lpCABEntryID->ulOffset);
	if (hr != hrSuccess)
		goto exit;

	hr = lpZCMAPIProp->QueryInterface(IID_ZCMAPIProp, (void**)lppZCMAPIProp);

exit:
	if (hr != hrSuccess)
		delete lpZCMAPIProp;

	return hr;
}

HRESULT ZCMAPIProp::QueryInterface(REFIID refiid, void **lppInterface)
{
	REGISTER_INTERFACE(IID_ZCMAPIProp, this);
	REGISTER_INTERFACE(IID_ECUnknown, this);

	REGISTER_INTERFACE(IID_IMAPIProp, &this->m_xMAPIProp);
	REGISTER_INTERFACE(IID_IUnknown, &this->m_xMAPIProp);

	if (m_ulObject == MAPI_MAILUSER) {
		REGISTER_INTERFACE(IID_IMailUser, &this->m_xMAPIProp);
	}

	return MAPI_E_INTERFACE_NOT_SUPPORTED;
}

HRESULT ZCMAPIProp::GetLastError(HRESULT hResult, ULONG ulFlags, LPMAPIERROR * lppMAPIError)
{
	return MAPI_E_NO_SUPPORT;
}

HRESULT ZCMAPIProp::SaveChanges(ULONG ulFlags)
{
	return MAPI_E_NO_SUPPORT;
}

HRESULT ZCMAPIProp::CopyOneProp(convert_context &converter, ULONG ulFlags,
    const std::map<short, SPropValue>::const_iterator &i, LPSPropValue lpProp,
    LPSPropValue lpBase)
{
	HRESULT hr = hrSuccess;

	if ((ulFlags & MAPI_UNICODE) == 0 && PROP_TYPE(i->second.ulPropTag) == PT_UNICODE) {
		std::string strAnsi;
		// copy from unicode to string8
		lpProp->ulPropTag = CHANGE_PROP_TYPE(i->second.ulPropTag, PT_STRING8);
		strAnsi = converter.convert_to<std::string>(i->second.Value.lpszW);
		hr = MAPIAllocateMore(strAnsi.size() + 1, lpBase, (void**)&lpProp->Value.lpszA);
		if (hr != hrSuccess)
			goto exit;
		strcpy(lpProp->Value.lpszA, strAnsi.c_str());
	} else {
		hr = Util::HrCopyProperty(lpProp, &i->second, lpBase);
		if (hr != hrSuccess)
			goto exit;
	}

exit:
	return hr;
}


/** 
 * return property data of this object
 * 
 * @param[in] lpPropTagArray requested properties or NULL for all
 * @param[in] ulFlags MAPI_UNICODE when lpPropTagArray is NULL
 * @param[out] lpcValues number of properties returned 
 * @param[out] lppPropArray properties
 * 
 * @return MAPI Error code
 */
HRESULT ZCMAPIProp::GetProps(LPSPropTagArray lpPropTagArray, ULONG ulFlags, ULONG * lpcValues, LPSPropValue * lppPropArray)
{
	HRESULT hr = hrSuccess;
	std::map<short, SPropValue>::const_iterator i;
	ULONG j = 0;
	LPSPropValue lpProps = NULL;
	convert_context converter;

	if((lpPropTagArray != NULL && lpPropTagArray->cValues == 0) || Util::ValidatePropTagArray(lpPropTagArray) == false)
		return MAPI_E_INVALID_PARAMETER;

	if (lpPropTagArray == NULL) {
		// check ulFlags for MAPI_UNICODE
		hr = MAPIAllocateBuffer(sizeof(SPropValue) * m_mapProperties.size(), (void**)&lpProps);
		if (hr != hrSuccess)
			goto exit;

		for (i = m_mapProperties.begin(), j = 0; i != m_mapProperties.end(); ++i, ++j) {
			hr = CopyOneProp(converter, ulFlags, i, &lpProps[j], lpProps);
			if (hr != hrSuccess)
				goto exit;				
		}
		
		*lpcValues = m_mapProperties.size();
	} else {
		// check lpPropTagArray->aulPropTag[x].ulPropTag for PT_UNICODE or PT_STRING8
		hr = MAPIAllocateBuffer(sizeof(SPropValue) * lpPropTagArray->cValues, (void**)&lpProps);
		if (hr != hrSuccess)
			goto exit;

		for (ULONG n = 0, j = 0; n < lpPropTagArray->cValues; ++n, ++j) {
			i = m_mapProperties.find(PROP_ID(lpPropTagArray->aulPropTag[n]));
			if (i == m_mapProperties.end()) {
				lpProps[j].ulPropTag = CHANGE_PROP_TYPE(lpPropTagArray->aulPropTag[n], PT_ERROR);
				lpProps[j].Value.ul = MAPI_E_NOT_FOUND;
				continue;
			}

			hr = CopyOneProp(converter, ulFlags, i, &lpProps[j], lpProps);
			if (hr != hrSuccess)
				goto exit;
		}

		*lpcValues = lpPropTagArray->cValues;
	}

	*lppPropArray = lpProps;
	lpProps = NULL;

exit:
	MAPIFreeBuffer(lpProps);
	return hr;
}

/** 
 * Return a list of all properties on this object
 * 
 * @param[in] ulFlags 0 for PT_STRING8, MAPI_UNICODE for PT_UNICODE properties
 * @param[out] lppPropTagArray list of all existing properties
 * 
 * @return MAPI Error code
 */
HRESULT ZCMAPIProp::GetPropList(ULONG ulFlags, LPSPropTagArray * lppPropTagArray)
{
	HRESULT hr = hrSuccess;
	LPSPropTagArray lpPropTagArray = NULL;
	std::map<short, SPropValue>::const_iterator i;
	ULONG j = 0;

	hr = MAPIAllocateBuffer(CbNewSPropTagArray(m_mapProperties.size()), (void**)&lpPropTagArray);
	if (hr != hrSuccess)
		goto exit;

	lpPropTagArray->cValues = m_mapProperties.size();
	for (i = m_mapProperties.begin(); i != m_mapProperties.end(); ++i, ++j) {
		lpPropTagArray->aulPropTag[j] = i->second.ulPropTag;
		if ((ulFlags & MAPI_UNICODE) == 0 && PROP_TYPE(lpPropTagArray->aulPropTag[j]) == PT_UNICODE)
			lpPropTagArray->aulPropTag[j] = CHANGE_PROP_TYPE(lpPropTagArray->aulPropTag[j], PT_STRING8);
	}

	*lppPropTagArray = lpPropTagArray;

exit:
	return hr;
}

HRESULT ZCMAPIProp::OpenProperty(ULONG ulPropTag, LPCIID lpiid, ULONG ulInterfaceOptions, ULONG ulFlags, LPUNKNOWN * lppUnk)
{
	return  MAPI_E_NO_SUPPORT;
}

HRESULT ZCMAPIProp::SetProps(ULONG cValues, LPSPropValue lpPropArray, LPSPropProblemArray * lppProblems)
{
	return  MAPI_E_NO_SUPPORT;
}

HRESULT ZCMAPIProp::DeleteProps(LPSPropTagArray lpPropTagArray, LPSPropProblemArray * lppProblems)
{
	return  MAPI_E_NO_SUPPORT;
}

HRESULT ZCMAPIProp::CopyTo(ULONG ciidExclude, LPCIID rgiidExclude, LPSPropTagArray lpExcludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray * lppProblems)
{
	return MAPI_E_NO_SUPPORT;
}

HRESULT ZCMAPIProp::CopyProps(LPSPropTagArray lpIncludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray * lppProblems)
{
	return MAPI_E_NO_SUPPORT;
}

HRESULT ZCMAPIProp::GetNamesFromIDs(LPSPropTagArray * lppPropTags, LPGUID lpPropSetGuid, ULONG ulFlags, ULONG * lpcPropNames, LPMAPINAMEID ** lpppPropNames)
{
	return MAPI_E_NO_SUPPORT;
}
 
HRESULT ZCMAPIProp::GetIDsFromNames(ULONG cPropNames, LPMAPINAMEID * lppPropNames, ULONG ulFlags, LPSPropTagArray * lppPropTags)
{
	return MAPI_E_NO_SUPPORT;
}


////////////////////////////////////////////
// Interface IMAPIProp
//

HRESULT __stdcall ZCMAPIProp::xMAPIProp::QueryInterface(REFIID refiid, void ** lppInterface)
{
	METHOD_PROLOGUE_(ZCMAPIProp , MAPIProp);
	return pThis->QueryInterface(refiid, lppInterface);
}

ULONG __stdcall ZCMAPIProp::xMAPIProp::AddRef()
{
	METHOD_PROLOGUE_(ZCMAPIProp , MAPIProp);
	return pThis->AddRef();
}

ULONG __stdcall ZCMAPIProp::xMAPIProp::Release()
{
	METHOD_PROLOGUE_(ZCMAPIProp , MAPIProp);
	return pThis->Release();
}

HRESULT __stdcall ZCMAPIProp::xMAPIProp::GetLastError(HRESULT hError, ULONG ulFlags, LPMAPIERROR * lppMapiError)
{
	METHOD_PROLOGUE_(ZCMAPIProp , MAPIProp);
	return pThis->GetLastError(hError, ulFlags, lppMapiError);
}

HRESULT __stdcall ZCMAPIProp::xMAPIProp::SaveChanges(ULONG ulFlags)
{
	METHOD_PROLOGUE_(ZCMAPIProp , MAPIProp);
	return pThis->SaveChanges(ulFlags);
}

HRESULT __stdcall ZCMAPIProp::xMAPIProp::GetProps(LPSPropTagArray lpPropTagArray, ULONG ulFlags, ULONG * lpcValues, LPSPropValue * lppPropArray)
{
	METHOD_PROLOGUE_(ZCMAPIProp , MAPIProp);
	return pThis->GetProps(lpPropTagArray, ulFlags, lpcValues, lppPropArray);
}

HRESULT __stdcall ZCMAPIProp::xMAPIProp::GetPropList(ULONG ulFlags, LPSPropTagArray * lppPropTagArray)
{
	METHOD_PROLOGUE_(ZCMAPIProp , MAPIProp);
	return pThis->GetPropList(ulFlags, lppPropTagArray);
}

HRESULT __stdcall ZCMAPIProp::xMAPIProp::OpenProperty(ULONG ulPropTag, LPCIID lpiid, ULONG ulInterfaceOptions, ULONG ulFlags, LPUNKNOWN * lppUnk)
{
	METHOD_PROLOGUE_(ZCMAPIProp , MAPIProp);
	return pThis->OpenProperty(ulPropTag, lpiid, ulInterfaceOptions, ulFlags, lppUnk);
}

HRESULT __stdcall ZCMAPIProp::xMAPIProp::SetProps(ULONG cValues, LPSPropValue lpPropArray, LPSPropProblemArray * lppProblems)
{
	METHOD_PROLOGUE_(ZCMAPIProp , MAPIProp);
	return pThis->SetProps(cValues, lpPropArray, lppProblems);
}

HRESULT __stdcall ZCMAPIProp::xMAPIProp::DeleteProps(LPSPropTagArray lpPropTagArray, LPSPropProblemArray * lppProblems)
{
	METHOD_PROLOGUE_(ZCMAPIProp , MAPIProp);
	return pThis->DeleteProps(lpPropTagArray, lppProblems);
}

HRESULT __stdcall ZCMAPIProp::xMAPIProp::CopyTo(ULONG ciidExclude, LPCIID rgiidExclude, LPSPropTagArray lpExcludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray * lppProblems)
{
	METHOD_PROLOGUE_(ZCMAPIProp , MAPIProp);
	return pThis->CopyTo(ciidExclude, rgiidExclude, lpExcludeProps, ulUIParam, lpProgress, lpInterface, lpDestObj, ulFlags, lppProblems);
}

HRESULT __stdcall ZCMAPIProp::xMAPIProp::CopyProps(LPSPropTagArray lpIncludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray * lppProblems)
{
	METHOD_PROLOGUE_(ZCMAPIProp , MAPIProp);
	return pThis->CopyProps(lpIncludeProps, ulUIParam, lpProgress, lpInterface, lpDestObj, ulFlags, lppProblems);
}

HRESULT __stdcall ZCMAPIProp::xMAPIProp::GetNamesFromIDs(LPSPropTagArray * pptaga, LPGUID lpguid, ULONG ulFlags, ULONG * pcNames, LPMAPINAMEID ** pppNames)
{
	METHOD_PROLOGUE_(ZCMAPIProp , MAPIProp);
	return pThis->GetNamesFromIDs(pptaga, lpguid, ulFlags, pcNames, pppNames);
}

HRESULT __stdcall ZCMAPIProp::xMAPIProp::GetIDsFromNames(ULONG cNames, LPMAPINAMEID * ppNames, ULONG ulFlags, LPSPropTagArray * pptaga)
{
	METHOD_PROLOGUE_(ZCMAPIProp , MAPIProp);
	return pThis->GetIDsFromNames(cNames, ppNames, ulFlags, pptaga);
}
