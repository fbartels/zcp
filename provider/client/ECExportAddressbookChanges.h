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

#ifndef ECEXPORTADDRESSBOOKCHANGES_H
#define ECEXPORTADDRESSBOOKCHANGES_H

#include <zarafa/zcdefs.h>
#include <set>

#include "ECABContainer.h"

class IECImportAddressbookChanges;
class ECLogger;

class ECExportAddressbookChanges : public ECUnknown {
public:
	ECExportAddressbookChanges(ECMsgStore *lpContainer);
	virtual ~ECExportAddressbookChanges();

    virtual HRESULT QueryInterface(REFIID refiid, void **lppInterface);

	// IECExportAddressbookChanges
	virtual HRESULT	Config(LPSTREAM lpState, ULONG ulFlags, IECImportAddressbookChanges *lpCollector);
	virtual HRESULT Synchronize(ULONG *lpulSteps, ULONG *lpulProgress);
	virtual HRESULT UpdateState(LPSTREAM lpState);

private:
	static bool LeftPrecedesRight(const ICSCHANGE &left, const ICSCHANGE &right);

private:
	class xECExportAddressbookChanges _zcp_final : public IECExportAddressbookChanges {
		// IUnknown
		virtual ULONG __stdcall AddRef(void) _zcp_override;
		virtual ULONG __stdcall Release(void) _zcp_override;
		virtual HRESULT __stdcall QueryInterface(REFIID refiid, void **lppInterface) _zcp_override;
		
		// IECExportAddressbookChanges
		virtual HRESULT __stdcall Config(LPSTREAM lpState, ULONG ulFlags, IECImportAddressbookChanges *lpCollector) _zcp_override;
		virtual HRESULT __stdcall Synchronize(ULONG *lpulSteps, ULONG *lpulProgress) _zcp_override;
		virtual HRESULT __stdcall UpdateState(LPSTREAM lpState) _zcp_override;

	} m_xECExportAddressbookChanges;
	
private:
	IECImportAddressbookChanges *m_lpImporter;
	unsigned int				m_ulChangeId;
	ECMsgStore					*m_lpMsgStore;
	unsigned int				m_ulThisChange;
	ULONG						m_ulChanges;
	ULONG						m_ulMaxChangeId;
	ICSCHANGE					*m_lpRawChanges; // Raw data from server
	ICSCHANGE					*m_lpChanges;	 // Same data, but sorted (users, then groups)
	std::set<ULONG>				m_setProcessed;
	ECLogger					*m_lpLogger;
};

#endif
