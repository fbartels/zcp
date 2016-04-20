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

#ifndef IECTESTPROTOCOL_H
#define IECTESTPROTOCOL_H

class IECTestProtocol : public IUnknown {
public:
    virtual HRESULT __stdcall TestPerform(char *szCommand, unsigned int ulArgs, char **szArgs) = 0;
	virtual HRESULT __stdcall TestSet(char *szName, char *szValue) = 0;
	virtual HRESULT __stdcall TestGet(char *szName, char **szValue) = 0;
};

#endif
