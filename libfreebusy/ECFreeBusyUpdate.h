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
 * Updates the freebusy data
 *
 * @addtogroup libfreebusy
 * @{
 */

#ifndef ECFREEBUSYUPDATE_H
#define ECFREEBUSYUPDATE_H

#include <zarafa/zcdefs.h>
#include "freebusy.h"
#include "freebusyguid.h"
#include <zarafa/ECUnknown.h>
#include <zarafa/Trace.h>
#include <zarafa/ECDebug.h>
#include <zarafa/ECGuid.h>

#include <mapi.h>
#include <mapidefs.h>

#include "ECFBBlockList.h"

/**
 * Implementatie of the IFreeBusyUpdate interface
 */
class ECFreeBusyUpdate : public ECUnknown
{
private:
	ECFreeBusyUpdate(IMessage* lpMessage);
	~ECFreeBusyUpdate(void);
public:
	static HRESULT Create(IMessage* lpMessage, ECFreeBusyUpdate **lppECFreeBusyUpdate);
	
	virtual HRESULT QueryInterface(REFIID refiid, void** lppInterface);

	virtual HRESULT Reload();
	virtual HRESULT PublishFreeBusy(FBBlock_1 *lpBlocks, ULONG nBlocks);
	virtual HRESULT RemoveAppt();
	virtual HRESULT ResetPublishedFreeBusy();
	virtual HRESULT ChangeAppt();
	virtual HRESULT SaveChanges(FILETIME ftStart, FILETIME ftEnd);
	virtual HRESULT GetFBTimes();
	virtual HRESULT Intersect();

public:
	class xFreeBusyUpdate _zcp_final : public IFreeBusyUpdate {
		public:
		// From IUnknown
		virtual HRESULT __stdcall QueryInterface(REFIID refiid, void **lppInterface) _zcp_override;
		virtual ULONG __stdcall AddRef(void) _zcp_override;
		virtual ULONG __stdcall Release(void) _zcp_override;

		// From IFreeBusyUpdate
		virtual HRESULT __stdcall Reload(void) _zcp_override;
		virtual HRESULT __stdcall PublishFreeBusy(FBBlock_1 *lpBlocks, ULONG nBlocks) _zcp_override;
		virtual HRESULT __stdcall RemoveAppt(void) _zcp_override;
		virtual HRESULT __stdcall ResetPublishedFreeBusy(void) _zcp_override;
		virtual HRESULT __stdcall ChangeAppt(void) _zcp_override;
		virtual HRESULT __stdcall SaveChanges(FILETIME ftBegin, FILETIME ftEnd) _zcp_override;
		virtual HRESULT __stdcall GetFBTimes(void) _zcp_override;
		virtual HRESULT __stdcall Intersect(void) _zcp_override;

	}m_xFreeBusyUpdate;

private:
	IMessage*		m_lpMessage; /**< Pointer to the free/busy message received from GetFreeBusyMessage */
	ECFBBlockList	m_fbBlockList; /**< Freebusy time blocks */

};

#endif // ECFREEBUSYUPDATE_H

/** @} */
