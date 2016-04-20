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

#ifndef IECUNKNOWN_H
#define IECUNKNOWN_H

/* needed for windows */
#ifndef METHOD_PROLOGUE_
#define METHOD_PROLOGUE_(theClass, localClass) \
	theClass* pThis = \
		((theClass*)((BYTE*)this - offsetof(theClass, m_x##localClass))); \
	pThis; 
#endif

// Our public IECUnknown interface

class IECUnknown {
public:
	virtual ~IECUnknown() {};

	virtual ULONG AddRef() = 0;
	virtual ULONG Release() = 0;
	virtual HRESULT QueryInterface(REFIID refiid, void **lpvoid) = 0;
};

#endif // IECUNKOWN_H
