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

#ifndef IECSECURITY_H
#define IECSECURITY_H

#include <zarafa/IECUnknown.h>
#include <zarafa/ECDefs.h>

#include <mapidefs.h>

class IECSecurity : public IECUnknown {
public:
	virtual HRESULT GetOwner(ULONG *lpcbOwner, LPENTRYID *lppOwner) = 0;
	virtual HRESULT GetUserList(ULONG cbCompanyId, LPENTRYID lpCompanyId, ULONG ulFlags, ULONG *lpcUsers, ECUSER **lppsUsers) = 0;
	virtual HRESULT GetGroupList(ULONG cbCompanyId, LPENTRYID lpCompanyId, ULONG ulFlags, ULONG *lpcGroups, ECGROUP **lppsGroups) = 0;
	virtual HRESULT GetCompanyList(ULONG ulFlags, ULONG *lpcCompanies, ECCOMPANY **lppsCompanies) = 0;

	virtual HRESULT GetPermissionRules(int ulType, ULONG* lpcPermissions, ECPERMISSION **lppECPermissions) = 0;
	virtual HRESULT SetPermissionRules(ULONG cPermissions, ECPERMISSION *lpECPermissions) = 0;
};

#endif
