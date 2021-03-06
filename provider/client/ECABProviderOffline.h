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

#ifndef ECABPROVIDEROFFLINE_H
#define ECABPROVIDEROFFLINE_H

#include <zarafa/zcdefs.h>
#include <zarafa/ECUnknown.h>
#include "ECABProvider.h"

class ECABProviderOffline : public ECABProvider
{
protected:
	ECABProviderOffline(void);

public:
	static  HRESULT Create(ECABProviderOffline **lppECABProvider);

	virtual HRESULT QueryInterface(REFIID refiid, void **lppInterface);

	class xABProvider _zcp_final : public IABProvider {
		// IUnknown
		virtual ULONG __stdcall AddRef(void) _zcp_override;
		virtual ULONG __stdcall Release(void) _zcp_override;
		virtual HRESULT __stdcall QueryInterface(REFIID refiid, void **lppInterface) _zcp_override;

		//IABProvider
		virtual HRESULT __stdcall Shutdown(ULONG * lpulFlags);
		virtual HRESULT __stdcall Logon(LPMAPISUP lpMAPISup, ULONG ulUIParam, LPTSTR lpszProfileName, ULONG ulFlags, ULONG * lpulcbSecurity, LPBYTE * lppbSecurity, LPMAPIERROR * lppMAPIError, LPABLOGON * lppABLogon);

	}m_xABProvider;
};

#endif // #ifndef ECABPROVIDEROFFLINE_H
