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
 * Free/busy data for one user
 *
 * @addtogroup libfreebusy
 * @{
 */

#ifndef ECFREEBUSYDATA_H
#define ECFREEBUSYDATA_H

#include <zarafa/zcdefs.h>
#include "freebusy.h"
#include "freebusyguid.h"

#include <zarafa/ECUnknown.h>
#include <zarafa/Trace.h>
#include <zarafa/ECDebug.h>
#include <zarafa/ECGuid.h>

#include "ECFBBlockList.h"

/**
 * Implementatie of the IFreeBusyData interface
 */
class ECFreeBusyData : public ECUnknown
{
private:
	ECFreeBusyData();
	~ECFreeBusyData(void);
public:
	static HRESULT Create(ECFreeBusyData **lppECFreeBusyData);

	HRESULT Init(LONG rtmStart, LONG rtmEnd, ECFBBlockList* lpfbBlockList);
	
	virtual HRESULT QueryInterface(REFIID refiid, void** lppInterface);
	virtual HRESULT Reload(void*);
	virtual HRESULT EnumBlocks(IEnumFBBlock **ppenumfb, FILETIME ftmStart, FILETIME ftmEnd);
	virtual HRESULT Merge(void *);
	virtual HRESULT GetDeligateInfo(void *);
	virtual HRESULT FindFreeBlock(LONG, LONG, LONG, BOOL, LONG, LONG, LONG, FBBlock_1 *);
	virtual HRESULT InterSect(void *, LONG, void *);
	virtual HRESULT SetFBRange(LONG rtmStart, LONG rtmEnd);
	virtual HRESULT NextFBAppt(void *, ULONG, void *, ULONG, void *, void *);
	virtual HRESULT GetFBPublishRange(LONG *prtmStart, LONG *prtmEnd);

public:
	class xFreeBusyData _zcp_final : public IFreeBusyData
	{
		public:
			// From IUnknown
			virtual HRESULT __stdcall QueryInterface(REFIID refiid , void **lppInterface) _zcp_override;
			virtual ULONG __stdcall AddRef(void) _zcp_override;
			virtual ULONG __stdcall Release(void) _zcp_override;

			// From IFreeBusyData
			virtual HRESULT __stdcall Reload(void *) _zcp_override;
			virtual HRESULT __stdcall EnumBlocks(IEnumFBBlock **ppenumfb, FILETIME ftmStart, FILETIME ftmEnd) _zcp_override;
			virtual HRESULT __stdcall Merge(void *) _zcp_override;
			virtual HRESULT __stdcall GetDeligateInfo(void *) _zcp_override;
			virtual HRESULT __stdcall FindFreeBlock(LONG, LONG, LONG, BOOL, LONG, LONG, LONG, FBBlock_1 *) _zcp_override;
			virtual HRESULT __stdcall InterSect(void *, LONG, void *) _zcp_override;
			virtual HRESULT __stdcall SetFBRange(LONG rtmStart, LONG rtmEnd) _zcp_override;
			virtual HRESULT __stdcall NextFBAppt(void *, ULONG, void *, ULONG, void *, void *) _zcp_override;
			virtual HRESULT __stdcall GetFBPublishRange(LONG *prtmStart, LONG *prtmEnd) _zcp_override;
	}m_xFreeBusyData;

private:
	ECFBBlockList	m_fbBlockList;
	LONG			m_rtmStart; // PR_FREEBUSY_START_RANGE
	LONG			m_rtmEnd; // PR_FREEBUSY_END_RANGE
};

#endif // ECFREEBUSYDATA_H

/** @} */
