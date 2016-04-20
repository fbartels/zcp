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

// ECMailUser.cpp: implementation of the ECMailUser class.
//
//////////////////////////////////////////////////////////////////////
#include <zarafa/platform.h>

#include "resource.h"
#include <mapiutil.h>
#include "Zarafa.h"
#include <zarafa/CommonUtil.h>
#include "ECMailUser.h"

#include "Mem.h"
#include <zarafa/ECGuid.h>
#include <zarafa/ECDebug.h>

#include "ECDisplayTable.h"

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ECMailUser::ECMailUser(void* lpProvider, BOOL fModify) : ECABProp(lpProvider, MAPI_MAILUSER, fModify, "IMailUser")
{
	// since we have no OpenProperty / abLoadProp, remove the 8k prop limit
	this->m_ulMaxPropSize = 0;
}

ECMailUser::~ECMailUser()
{

}

HRESULT ECMailUser::Create(void* lpProvider, BOOL fModify, ECMailUser** lppMailUser)
{

	HRESULT hr = hrSuccess;
	ECMailUser *lpMailUser = NULL;

	lpMailUser = new ECMailUser(lpProvider, fModify);

	hr = lpMailUser->QueryInterface(IID_ECMailUser, (void **)lppMailUser);

	if(hr != hrSuccess)
		delete lpMailUser;

	return hr;
}

HRESULT	ECMailUser::QueryInterface(REFIID refiid, void **lppInterface) 
{
	REGISTER_INTERFACE(IID_ECMailUser, this);
	REGISTER_INTERFACE(IID_ECABProp, this);
	REGISTER_INTERFACE(IID_ECUnknown, this);

	REGISTER_INTERFACE(IID_IMailUser, &this->m_xMailUser);
	REGISTER_INTERFACE(IID_IMAPIProp, &this->m_xMailUser);
	REGISTER_INTERFACE(IID_IUnknown, &this->m_xMailUser);

	return MAPI_E_INTERFACE_NOT_SUPPORTED;
}

HRESULT ECMailUser::TableRowGetProp(void* lpProvider, struct propVal *lpsPropValSrc, LPSPropValue lpsPropValDst, void **lpBase, ULONG ulType)
{
	return MAPI_E_NOT_FOUND;
}

HRESULT ECMailUser::OpenProperty(ULONG ulPropTag, LPCIID lpiid, ULONG ulInterfaceOptions, ULONG ulFlags, LPUNKNOWN FAR * lppUnk)
{
	HRESULT			hr = MAPI_E_NOT_FOUND;

	if (lpiid == NULL)
		return MAPI_E_INVALID_PARAMETER;

	if (ulFlags & MAPI_CREATE)
		// Don't support creating any sub-objects
		return MAPI_E_NO_ACCESS;

	switch(ulPropTag) {
#if defined(_WIN32) && !defined(WINCE)
	case PR_DETAILS_TABLE:
		if (*lpiid != IID_IMAPITable)
			return MAPI_E_INTERFACE_NOT_SUPPORTED;

		hr = ECDisplayTable::CreateDisplayTable(arraySize(rgdtIMailUserPage), rgdtIMailUserPage, (LPMAPITABLE *) lppUnk);
		if (hr != hrSuccess)
			return hr;

		break;
	case PR_BUSINESS2_TELEPHONE_NUMBER_O:
		if (*lpiid != IID_IMAPITable)
			return MAPI_E_INTERFACE_NOT_SUPPORTED;

		hr = ECDisplayTable::CreateTableFromProperty(this, lpiid, ulInterfaceOptions, PR_BUSINESS2_TELEPHONE_NUMBER, PR_BUSINESS2_TELEPHONE_NUMBER, lppUnk);
		if (hr != hrSuccess)
			return hr;

		break;
	case PR_HOME2_TELEPHONE_NUMBER_O:
		if (*lpiid != IID_IMAPITable)
			return MAPI_E_INTERFACE_NOT_SUPPORTED;

		hr = ECDisplayTable::CreateTableFromProperty(this, lpiid, ulInterfaceOptions, PR_HOME2_TELEPHONE_NUMBER, PR_HOME2_TELEPHONE_NUMBER, lppUnk);
		if (hr != hrSuccess)
			return hr;

		break;
	case PR_EMS_AB_MANAGER_O:
		if (*lpiid != IID_IMAPITable)
			return MAPI_E_INTERFACE_NOT_SUPPORTED;

		hr = ECDisplayTable::CreateTableFromResolved(this, lpiid, ulInterfaceOptions, CHANGE_PROP_TYPE(PR_EMS_AB_MANAGER, PT_BINARY), lppUnk);
		if (hr != hrSuccess)
			return hr;

		break;
	case PR_EMS_AB_PROXY_ADDRESSES_O:
		if (*lpiid != IID_IMAPITable)
			return MAPI_E_INTERFACE_NOT_SUPPORTED;

		hr = ECDisplayTable::CreateTableFromProperty(this, lpiid, ulInterfaceOptions, PR_SMTP_ADDRESS, PR_EMS_AB_PROXY_ADDRESSES, lppUnk);
		if (hr != hrSuccess)
			return hr;

		break;
	case PR_EMS_AB_IS_MEMBER_OF_DL_O:
		if (*lpiid != IID_IMAPITable)
			return MAPI_E_INTERFACE_NOT_SUPPORTED;

		hr = ECDisplayTable::CreateTableFromResolved(this, lpiid, ulInterfaceOptions, PR_EMS_AB_IS_MEMBER_OF_DL_T, lppUnk);
		if (hr != hrSuccess)
			return hr;

		break;
	case PR_EMS_AB_REPORTS_O:
		if (*lpiid != IID_IMAPITable)
			return MAPI_E_INTERFACE_NOT_SUPPORTED;

		hr = ECDisplayTable::CreateTableFromResolved(this, lpiid, ulInterfaceOptions, PR_EMS_AB_REPORTS_T, lppUnk);
		if (hr != hrSuccess)
			return hr;

		break;
#endif
	default:
		hr = ECABProp::OpenProperty(ulPropTag, lpiid, ulInterfaceOptions, ulFlags, lppUnk);
		break;
	}
	return hr;
}

HRESULT ECMailUser::CopyTo(ULONG ciidExclude, LPCIID rgiidExclude, LPSPropTagArray lpExcludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray FAR * lppProblems)
{
	return this->GetABStore()->m_lpMAPISup->DoCopyTo(&IID_IMailUser, &this->m_xMailUser, ciidExclude, rgiidExclude, lpExcludeProps, ulUIParam, lpProgress, lpInterface, lpDestObj, ulFlags, lppProblems);
}

HRESULT ECMailUser::CopyProps(LPSPropTagArray lpIncludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray FAR * lppProblems)
{
	return this->GetABStore()->m_lpMAPISup->DoCopyProps(&IID_IMailUser, &this->m_xMailUser, lpIncludeProps, ulUIParam, lpProgress, lpInterface, lpDestObj, ulFlags, lppProblems);
}

//////////////////////////////////////////////
// IMailUser
//

HRESULT ECMailUser::xMailUser::QueryInterface(REFIID refiid , void** lppInterface)
{
	TRACE_MAPI(TRACE_ENTRY, "IMailUser::QueryInterface", "%s", DBGGUIDToString(refiid).c_str());
	METHOD_PROLOGUE_(ECMailUser, MailUser);
	HRESULT hr = pThis->QueryInterface(refiid, lppInterface);
	TRACE_MAPI(TRACE_RETURN, "IMailUser::QueryInterface", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

ULONG ECMailUser::xMailUser::AddRef()
{
	TRACE_MAPI(TRACE_ENTRY, "IMailUser::AddRef", "");
	METHOD_PROLOGUE_(ECMailUser, MailUser);
	return pThis->AddRef();
}

ULONG ECMailUser::xMailUser::Release()
{
	TRACE_MAPI(TRACE_ENTRY, "IMailUser::Release", "");
	METHOD_PROLOGUE_(ECMailUser, MailUser);
	return pThis->Release();
}

HRESULT ECMailUser::xMailUser::GetLastError(HRESULT hError, ULONG ulFlags, LPMAPIERROR * lppMapiError)
{
	TRACE_MAPI(TRACE_ENTRY, "IMailUser::GetLastError", "herror=0x%08x", hError);
	METHOD_PROLOGUE_(ECMailUser, MailUser);
	HRESULT hr = pThis->GetLastError(hError, ulFlags, lppMapiError);
	TRACE_MAPI(TRACE_RETURN, "IMailUser::GetLastError", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ECMailUser::xMailUser::SaveChanges(ULONG ulFlags)
{
	TRACE_MAPI(TRACE_ENTRY, "IMailUser::SaveChanges", "");
	METHOD_PROLOGUE_(ECMailUser, MailUser);
	HRESULT hr = pThis->SaveChanges(ulFlags);
	TRACE_MAPI(TRACE_RETURN, "IMailUser::SaveChanges", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ECMailUser::xMailUser::GetProps(LPSPropTagArray lpPropTagArray, ULONG ulFlags, ULONG FAR * lpcValues, LPSPropValue FAR * lppPropArray)
{
	TRACE_MAPI(TRACE_ENTRY, "IMailUser::GetProps", "%s, flags=%08X", PropNameFromPropTagArray(lpPropTagArray).c_str(), ulFlags);
	METHOD_PROLOGUE_(ECMailUser, MailUser);
	HRESULT hr = pThis->GetProps(lpPropTagArray, ulFlags, lpcValues, lppPropArray);
	TRACE_MAPI(TRACE_RETURN, "IMailUser::GetProps", "%s, %s", GetMAPIErrorDescription(hr).c_str(), (lpcValues && lppPropArray)?PropNameFromPropArray(*lpcValues, *lppPropArray).c_str():"NULL");
	return hr;
}

HRESULT ECMailUser::xMailUser::GetPropList(ULONG ulFlags, LPSPropTagArray FAR * lppPropTagArray)
{
	TRACE_MAPI(TRACE_ENTRY, "IMailUser::GetPropList", "");
	METHOD_PROLOGUE_(ECMailUser, MailUser);
	HRESULT hr = pThis->GetPropList(ulFlags, lppPropTagArray);
	TRACE_MAPI(TRACE_RETURN, "IMailUser::GetPropList", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ECMailUser::xMailUser::OpenProperty(ULONG ulPropTag, LPCIID lpiid, ULONG ulInterfaceOptions, ULONG ulFlags, LPUNKNOWN FAR * lppUnk)
{
	TRACE_MAPI(TRACE_ENTRY, "IMailUser::OpenProperty", "PropTag=%s, lpiid=%s", PropNameFromPropTag(ulPropTag).c_str(), DBGGUIDToString(*lpiid).c_str());
	METHOD_PROLOGUE_(ECMailUser, MailUser);
	HRESULT hr = pThis->OpenProperty(ulPropTag, lpiid, ulInterfaceOptions, ulFlags, lppUnk);
	TRACE_MAPI(TRACE_RETURN, "IMailUser::OpenProperty", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ECMailUser::xMailUser::SetProps(ULONG cValues, LPSPropValue lpPropArray, LPSPropProblemArray FAR * lppProblems)
{
	TRACE_MAPI(TRACE_ENTRY, "IMailUser::SetProps", "");
	METHOD_PROLOGUE_(ECMailUser, MailUser);
	HRESULT hr = pThis->SetProps(cValues, lpPropArray, lppProblems);
	TRACE_MAPI(TRACE_RETURN, "IMailUser::SetProps", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ECMailUser::xMailUser::DeleteProps(LPSPropTagArray lpPropTagArray, LPSPropProblemArray FAR * lppProblems)
{
	TRACE_MAPI(TRACE_ENTRY, "IMailUser::DeleteProps", "%s", PropNameFromPropTagArray(lpPropTagArray).c_str());
	METHOD_PROLOGUE_(ECMailUser, MailUser);
	HRESULT hr = pThis->DeleteProps(lpPropTagArray, lppProblems);
	TRACE_MAPI(TRACE_RETURN, "IMailUser::DeleteProps", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ECMailUser::xMailUser::CopyTo(ULONG ciidExclude, LPCIID rgiidExclude, LPSPropTagArray lpExcludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray FAR * lppProblems)
{
	TRACE_MAPI(TRACE_ENTRY, "IMailUser::CopyTo", "");
	METHOD_PROLOGUE_(ECMailUser, MailUser);
	HRESULT hr = pThis->CopyTo(ciidExclude, rgiidExclude, lpExcludeProps, ulUIParam, lpProgress, lpInterface, lpDestObj, ulFlags, lppProblems);;
	TRACE_MAPI(TRACE_RETURN, "IMailUser::CopyTo", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ECMailUser::xMailUser::CopyProps(LPSPropTagArray lpIncludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray FAR * lppProblems)
{
	TRACE_MAPI(TRACE_ENTRY, "IMailUser::CopyProps", "");
	METHOD_PROLOGUE_(ECMailUser, MailUser);
	HRESULT hr = pThis->CopyProps(lpIncludeProps, ulUIParam, lpProgress, lpInterface, lpDestObj, ulFlags, lppProblems);
	TRACE_MAPI(TRACE_RETURN, "IMailUser::CopyProps", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ECMailUser::xMailUser::GetNamesFromIDs(LPSPropTagArray * pptaga, LPGUID lpguid, ULONG ulFlags, ULONG * pcNames, LPMAPINAMEID ** pppNames)
{
	TRACE_MAPI(TRACE_ENTRY, "IMailUser::GetNamesFromIDs", "");
	METHOD_PROLOGUE_(ECMailUser, MailUser);
	HRESULT hr = pThis->GetNamesFromIDs(pptaga, lpguid, ulFlags, pcNames, pppNames);
	TRACE_MAPI(TRACE_RETURN, "IMailUser::GetIDsFromNames", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT ECMailUser::xMailUser::GetIDsFromNames(ULONG cNames, LPMAPINAMEID * ppNames, ULONG ulFlags, LPSPropTagArray * pptaga)
{
	TRACE_MAPI(TRACE_ENTRY, "IMailUser::GetIDsFromNames", "");
	METHOD_PROLOGUE_(ECMailUser, MailUser);
	HRESULT hr = pThis->GetIDsFromNames(cNames, ppNames, ulFlags, pptaga);
	TRACE_MAPI(TRACE_RETURN, "IMailUser::GetIDsFromNames", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}
