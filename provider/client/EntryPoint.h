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

#ifndef ENTRYPOINT_H
#define ENTRYPOINT_H

#include <mapispi.h>

#include "ProviderUtil.h"
#include <zarafa/tstring.h>

extern "C" {

HRESULT __cdecl MSProviderInit(HINSTANCE hInstance, LPMALLOC pmalloc, LPALLOCATEBUFFER pfnAllocBuf, LPALLOCATEMORE pfnAllocMore, LPFREEBUFFER pfnFreeBuf, ULONG ulFlags, ULONG ulMAPIVersion, ULONG * pulMDBVersion, LPMSPROVIDER * ppmsp);
HRESULT __stdcall MSGServiceEntry(HINSTANCE hInst, LPMALLOC lpMalloc, LPMAPISUP psup, ULONG ulUIParam, ULONG ulSEFlags, ULONG ulContext, ULONG cvals, LPSPropValue pvals, LPPROVIDERADMIN lpAdminProviders, LPMAPIERROR *lppMapiError);
HRESULT __cdecl XPProviderInit(HINSTANCE hInstance, LPMALLOC lpMalloc, LPALLOCATEBUFFER lpAllocateBuffer, LPALLOCATEMORE lpAllocateMore, LPFREEBUFFER lpFreeBuffer, ULONG ulFlags, ULONG ulMAPIVer, ULONG * lpulProviderVer, LPXPPROVIDER * lppXPProvider);
HRESULT  __cdecl ABProviderInit(HINSTANCE hInstance, LPMALLOC lpMalloc, LPALLOCATEBUFFER lpAllocateBuffer, LPALLOCATEMORE lpAllocateMore, LPFREEBUFFER lpFreeBuffer, ULONG ulFlags, ULONG ulMAPIVer, ULONG * lpulProviderVer, LPABPROVIDER * lppABProvider);

#ifdef WIN32
HRESULT __stdcall MergeWithMAPISVC();
HRESULT __stdcall RemoveFromMAPISVC();
#endif

}

HRESULT InitializeProvider(LPPROVIDERADMIN lpAdminProvider, IProfSect *lpProfSect, sGlobalProfileProps sProfileProps, ULONG *lpcStoreID, LPENTRYID *lppStoreID);

// Global values
extern tstring	g_strCommonFilesZarafa;
extern tstring	g_strUserLocalAppDataZarafa;
extern tstring	g_strZarafaDirectory;
extern ECMapProvider g_mapProviders;
extern tstring		g_strManufacturer;
extern tstring		g_strProductName;
extern tstring		g_strProductNameShort;
extern bool g_isOEM;
extern ULONG g_ulLoadsim;


#endif // ENTRYPOINT_H
