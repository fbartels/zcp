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

#include <zarafa/platform.h>
#include "ECArchiverLogger.h"
#include "deleter.h"

using namespace std;

namespace za { namespace operations {

/**
 * Constructor
 *
 * @param[in]	lpLogger
 *					Pointer to the logger.
 *
 * @return HRESULT
 */
Deleter::Deleter(ECArchiverLogger *lpLogger, int ulAge, bool bProcessUnread)
: ArchiveOperationBaseEx(lpLogger, ulAge, bProcessUnread, ARCH_NEVER_DELETE)
{ }

/**
 * Destructor
 */
Deleter::~Deleter()
{
	PurgeQueuedMessages();
}

HRESULT Deleter::EnterFolder(LPMAPIFOLDER)
{
	return hrSuccess;
}

HRESULT Deleter::LeaveFolder()
{
	// Folder is still available, purge now!
	return PurgeQueuedMessages();
}

HRESULT Deleter::DoProcessEntry(ULONG cProps, const LPSPropValue &lpProps)
{
	HRESULT hr = hrSuccess;
	LPSPropValue lpEntryId = NULL;

	lpEntryId = PpropFindProp(lpProps, cProps, PR_ENTRYID);
	if (lpEntryId == NULL) {
		Logger()->Log(EC_LOGLEVEL_FATAL, "PR_ENTRYID missing");
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}
	
	if (m_lstEntryIds.size() >= 50) {
		hr = PurgeQueuedMessages();
		if (hr != hrSuccess)
			goto exit;
	}

	m_lstEntryIds.push_back(lpEntryId->Value.bin);
	
exit:
	return hr;
}

/**
 * Delete the messages that are queued for deletion.
 * @return HRESULT
 */
HRESULT Deleter::PurgeQueuedMessages()
{
	HRESULT hr = hrSuccess;
	EntryListPtr ptrEntryList;
	list<entryid_t>::const_iterator iEntryId;
	ULONG ulIdx = 0;
	
	if (m_lstEntryIds.empty())
		goto exit;
	
	hr = MAPIAllocateBuffer(sizeof(ENTRYLIST), &ptrEntryList);
	if (hr != hrSuccess)
		goto exit;
	
	hr = MAPIAllocateMore(m_lstEntryIds.size() * sizeof(SBinary), ptrEntryList, (LPVOID*)&ptrEntryList->lpbin);
	if (hr != hrSuccess)
		goto exit;
		
	ptrEntryList->cValues = m_lstEntryIds.size();
	for (iEntryId = m_lstEntryIds.begin(); iEntryId != m_lstEntryIds.end(); ++iEntryId, ++ulIdx) {
		ptrEntryList->lpbin[ulIdx].cb = iEntryId->size();
		ptrEntryList->lpbin[ulIdx].lpb = *iEntryId;
	}
	
	hr = CurrentFolder()->DeleteMessages(ptrEntryList, 0, NULL, 0);
	if (hr != hrSuccess) {
		Logger()->Log(EC_LOGLEVEL_FATAL, "Failed to delete %u messages. (hr=%s)", ptrEntryList->cValues, stringify(hr, true).c_str());
		goto exit;
	}
	
	m_lstEntryIds.clear();
	
exit:
	return hr;
}

}} // namespaces 
