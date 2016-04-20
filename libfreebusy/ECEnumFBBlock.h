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

/**
 * @file
 * Free/busy data blocks
 *
 * @addtogroup libfreebusy
 * @{
 */

#ifndef ECENUMFBBLOCK_H
#define ECENUMFBBLOCK_H

#include <zarafa/zcdefs.h>
#include "freebusy.h"
#include <zarafa/ECUnknown.h>
#include <zarafa/Trace.h>
#include <zarafa/ECDebug.h>
#include <zarafa/ECGuid.h>
#include "freebusyguid.h"

#include "ECFBBlockList.h"

/**
 * Implementatie of the IEnumFBBlock interface
 */
class ECEnumFBBlock : public ECUnknown
{
private:
	ECEnumFBBlock(ECFBBlockList* lpFBBlock);
	~ECEnumFBBlock(void);
public:
	static HRESULT Create(ECFBBlockList* lpFBBlock, ECEnumFBBlock **lppECEnumFBBlock);
	
	virtual HRESULT QueryInterface(REFIID refiid, void** lppInterface);
	virtual HRESULT Next(LONG celt, FBBlock_1 *pblk, LONG *pcfetch);
	virtual HRESULT Skip(LONG celt);
	virtual HRESULT Reset();
	virtual HRESULT Clone(IEnumFBBlock **ppclone);
	virtual HRESULT Restrict(FILETIME ftmStart, FILETIME ftmEnd);

public:
	/* IEnumFBBlock wrapper class */
	class xEnumFBBlock _zcp_final : public IEnumFBBlock {
		public:
			// From IUnknown
			virtual HRESULT __stdcall QueryInterface(REFIID refiid, void **lppInterface) _zcp_override;
			virtual ULONG __stdcall AddRef(void) _zcp_override;
			virtual ULONG __stdcall Release(void) _zcp_override;

			// From IEnumFBBlock
			virtual HRESULT __stdcall Next(LONG celt, FBBlock_1 *pblk, LONG *pcfetch) _zcp_override;
			virtual HRESULT __stdcall Skip(LONG celt) _zcp_override;
			virtual HRESULT __stdcall Reset() _zcp_override;
			virtual HRESULT __stdcall Clone(IEnumFBBlock **ppclone) _zcp_override;
			virtual HRESULT __stdcall Restrict(FILETIME ftmStart, FILETIME ftmEnd);
	}m_xEnumFBBlock;

	ECFBBlockList	m_FBBlock; /**< Freebusy time blocks */
};

#endif // ECENUMFBBLOCK_H
/** @} */
