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

#include <zarafa/platform.h>
#include <memory.h>
#include <mapi.h>
#include <mapiutil.h>
#include <mapispi.h>


#include <zarafa/ECGuid.h>
#include "edkguid.h"

#include "Trace.h"
#include "ECDebug.h"

#include "ECTags.h"

#include "ECABProviderOffline.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif


ECABProviderOffline::ECABProviderOffline(void) : ECABProvider(EC_PROVIDER_OFFLINE, "ECABProviderOffline")
{
}

HRESULT ECABProviderOffline::Create(ECABProviderOffline **lppECABProvider)
{
	HRESULT hr = hrSuccess;

	ECABProviderOffline *lpECABProvider = new ECABProviderOffline();

	hr = lpECABProvider->QueryInterface(IID_ECABProvider, (void **)lppECABProvider);

	if(hr != hrSuccess)
		delete lpECABProvider;

	return hr;
}

HRESULT ECABProviderOffline::QueryInterface(REFIID refiid, void **lppInterface)
{
	REGISTER_INTERFACE(IID_ECABProvider, this);
	REGISTER_INTERFACE(IID_ECUnknown, this);

	REGISTER_INTERFACE(IID_IABProvider, &this->m_xABProvider);
	REGISTER_INTERFACE(IID_IUnknown, &this->m_xABProvider);

	REGISTER_INTERFACE(IID_ISelectUnicode, &this->m_xUnknown);

	return MAPI_E_INTERFACE_NOT_SUPPORTED;
}

HRESULT __stdcall ECABProviderOffline::xABProvider::QueryInterface(REFIID refiid, void ** lppInterface)
{
	TRACE_MAPI(TRACE_ENTRY, "ECABProviderOffline::QueryInterface", "%s", DBGGUIDToString(refiid).c_str());
	METHOD_PROLOGUE_(ECABProviderOffline , ABProvider);
	HRESULT hr = pThis->QueryInterface(refiid, lppInterface);
	TRACE_MAPI(TRACE_RETURN, "ECABProviderOffline::QueryInterface", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

ULONG __stdcall ECABProviderOffline::xABProvider::AddRef()
{
	TRACE_MAPI(TRACE_ENTRY, "ECABProviderOffline::AddRef", "");
	METHOD_PROLOGUE_(ECABProviderOffline , ABProvider);
	return pThis->AddRef();
}

ULONG __stdcall ECABProviderOffline::xABProvider::Release()
{
	TRACE_MAPI(TRACE_ENTRY, "ECABProviderOffline::Release", "");
	METHOD_PROLOGUE_(ECABProviderOffline , ABProvider);
	return pThis->Release();
}

HRESULT ECABProviderOffline::xABProvider::Shutdown(ULONG *lpulFlags)
{
	TRACE_MAPI(TRACE_ENTRY, "ECABProviderOffline::Shutdown", "");
	METHOD_PROLOGUE_(ECABProviderOffline , ABProvider);
	return pThis->Shutdown(lpulFlags);
}

HRESULT ECABProviderOffline::xABProvider::Logon(LPMAPISUP lpMAPISup, ULONG ulUIParam, LPTSTR lpszProfileName, ULONG ulFlags, ULONG * lpulcbSecurity, LPBYTE * lppbSecurity, LPMAPIERROR * lppMAPIError, LPABLOGON * lppABLogon)
{
	TRACE_MAPI(TRACE_ENTRY, "ECABProviderOffline::Logon", "");
	METHOD_PROLOGUE_(ECABProviderOffline , ABProvider);
	HRESULT hr = pThis->Logon(lpMAPISup, ulUIParam, lpszProfileName, ulFlags, lpulcbSecurity, lppbSecurity, lppMAPIError, lppABLogon);
	TRACE_MAPI(TRACE_RETURN, "ECABProviderOffline::Logon", "%s", GetMAPIErrorDescription(hr).c_str());
	return  hr;
}
