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
#include <memory.h>
#include <mapi.h>
#include <mapiutil.h>
#include <mapispi.h>


#include <zarafa/ECGuid.h>
#include <edkguid.h>

#include <zarafa/Trace.h>
#include <zarafa/ECDebug.h>

#include <zarafa/ECTags.h>

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
