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

#ifndef __MAPI_SVC_H
#define __MAPI_SVC_H

#include <zarafa/zcdefs.h>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <mapidefs.h>
#include <mapispi.h>

typedef std::map<std::string, std::string> inf_section;
typedef std::map<std::string, inf_section> inf;

/* MAPI Providers EntryPoint functions */
typedef HRESULT(__stdcall *SVC_MSGServiceEntry)(HINSTANCE hInst, LPMALLOC lpMalloc, LPMAPISUP psup, ULONG ulUIParam, ULONG ulFlags,
												ULONG ulContext, ULONG cvals, LPSPropValue pvals, LPPROVIDERADMIN lpAdminProviders,
												MAPIERROR **lppMapiError);

typedef HRESULT(__cdecl *SVC_MSProviderInit)(HINSTANCE hInstance, LPMALLOC pmalloc,
											 LPALLOCATEBUFFER pfnAllocBuf, LPALLOCATEMORE pfnAllocMore, LPFREEBUFFER pfnFreeBuf,
											 ULONG ulFlags, ULONG ulMAPIVersion, ULONG * pulMDBVersion, LPMSPROVIDER * ppmsp);

typedef HRESULT(__cdecl *SVC_ABProviderInit)(HINSTANCE hInstance, LPMALLOC lpMalloc, LPALLOCATEBUFFER lpAllocateBuffer,
											 LPALLOCATEMORE lpAllocateMore, LPFREEBUFFER lpFreeBuffer, ULONG ulFlags, ULONG ulMAPIVer,
											 ULONG * lpulProviderVer, LPABPROVIDER * lppABProvider);


class INFLoader _zcp_final {
public:
	INFLoader();
	~INFLoader();

	HRESULT LoadINFs();
	const inf_section* GetSection(const std::string& strSectionName) const;

	HRESULT MakeProperty(const std::string& strTag, const std::string& strData, void *base, LPSPropValue lpProp) const;

private:
	std::vector<std::string> GetINFPaths();
	HRESULT LoadINF(const char *filename);

	inf m_mapSections;

	ULONG DefinitionFromString(const std::string& strDef, bool bProp) const;
	std::map<std::string, unsigned int> m_mapDefs;
};

class SVCProvider _zcp_final {
public:
	/* ZARAFA6_ABP, ZARAFA6_MSMDB_private, ZARAFA6_MSMDB_public */
	SVCProvider();
	~SVCProvider();

	HRESULT Init(const INFLoader& cINF, const inf_section* infService);
	void GetProps(ULONG *lpcValues, LPSPropValue *lppPropValues);

private:
	ULONG m_cValues;
	LPSPropValue m_lpProps; /* PR_* tags from file */
};

class SVCService _zcp_final {
public:
	/* ZARAFA6, ZCONTACTS */
	SVCService();
	~SVCService();

	HRESULT Init(const INFLoader& cINF, const inf_section* infService);

	HRESULT CreateProviders(IProviderAdmin *lpProviderAdmin);

	LPSPropValue GetProp(ULONG ulPropTag);

	SVCProvider* GetProvider(LPTSTR lpszProvider, ULONG ulFlags);
	std::vector<SVCProvider*> GetProviders();

	SVC_MSGServiceEntry MSGServiceEntry();
	/* move to SVCProvider ? */
	SVC_MSProviderInit MSProviderInit();
	SVC_ABProviderInit ABProviderInit();

private:
	DLIB m_dl;
	SVC_MSGServiceEntry m_fnMSGServiceEntry;
	SVC_MSProviderInit m_fnMSProviderInit;
	SVC_ABProviderInit m_fnABProviderInit;

	/* PR_* tags from file */
	LPSPropValue m_lpProps;
	ULONG m_cValues;

	std::map<std::string, SVCProvider*> m_sProviders;
};

class MAPISVC _zcp_final {
public:
	MAPISVC();
	~MAPISVC();

	HRESULT Init();

	HRESULT GetService(LPTSTR lpszService, ULONG ulFlags, SVCService **lppService);
	HRESULT GetService(char* lpszDLLName, SVCService **lppService);

private:
	std::map<std::string, SVCService*> m_sServices;
};

#endif
