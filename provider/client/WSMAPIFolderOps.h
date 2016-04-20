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

#ifndef WSMAPIFOLDEROPS_H
#define WSMAPIFOLDEROPS_H

#include <zarafa/ECUnknown.h>
#include "Zarafa.h"
#include <zarafa/ZarafaCode.h>
#include "soapZarafaCmdProxy.h"
#include "ECICS.h"

#include <vector>

#include <mapi.h>
#include <mapispi.h>
#include <pthread.h>

class WSTransport;
class utf8string;

class WSMAPIFolderOps : public ECUnknown
{
protected:
	WSMAPIFolderOps(ZarafaCmd *lpCmd, pthread_mutex_t *lpDataLock, ECSESSIONID ecSessionId, ULONG cbEntryId, LPENTRYID lpEntryId, WSTransport *lpTransport);
	virtual ~WSMAPIFolderOps();

public:
	static HRESULT Create(ZarafaCmd *lpCmd, pthread_mutex_t *lpDataLock, ECSESSIONID ecSessionId, ULONG cbEntryId, LPENTRYID lpEntryId, WSTransport *lpTransport, WSMAPIFolderOps **lppFolderOps);

	virtual HRESULT QueryInterface(REFIID refiid, void **lppInterface);
	
	// Creates a folder object with only a PR_DISPLAY_NAME and type
	virtual HRESULT HrCreateFolder(ULONG ulFolderType, const utf8string &strFolderName, const utf8string &strComment, BOOL fOpenIfExists, ULONG ulSyncId, LPSBinary lpsSourceKey, ULONG cbNewEntryId, LPENTRYID lpNewEntryId, ULONG* lpcbEntryId, LPENTRYID* lppEntryId);

	// Completely remove a folder, the messages in it, the folders in it or any combination
	virtual HRESULT HrDeleteFolder(ULONG cbEntryId, LPENTRYID lpEntryId, ULONG ulFlags, ULONG ulSyncId);

	// Empty folder (ie delete all folders and messages in folder)
	virtual HRESULT HrEmptyFolder(ULONG ulFlags, ULONG ulSyncId);

	// Set read/unread flags on messages
	virtual HRESULT HrSetReadFlags(ENTRYLIST *lpMsgList, ULONG ulFlags, ULONG ulSyncId);

	// Set / Get search criteria
	virtual HRESULT HrSetSearchCriteria(ENTRYLIST *lpMsgList, SRestriction *lpRestriction, ULONG ulFlags);
	virtual HRESULT HrGetSearchCriteria(ENTRYLIST **lppMsgList, LPSRestriction *lppRestriction, ULONG *lpulFlags);

	// Move or copy a folder
	virtual HRESULT HrCopyFolder(ULONG cbEntryFrom, LPENTRYID lpEntryFrom, ULONG cbEntryDest, LPENTRYID lpEntryDest, const utf8string &strNewFolderName, ULONG ulFlags, ULONG ulSyncId);

	// Move or copy a message
	virtual HRESULT HrCopyMessage(ENTRYLIST *lpMsgList, ULONG cbEntryDest, LPENTRYID lpEntryDest, ULONG ulFlags, ULONG ulSyncId);
	
	// Message status
	virtual HRESULT HrGetMessageStatus(ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulFlags, ULONG *lpulMessageStatus);
	virtual HRESULT HrSetMessageStatus(ULONG cbEntryID, LPENTRYID lpEntryID, ULONG ulNewStatus, ULONG ulNewStatusMask, ULONG ulSyncId, ULONG *lpulOldStatus);
	
	// Streaming Support
	virtual HRESULT HrGetChangeInfo(ULONG cbEntryID, LPENTRYID lpEntryID, LPSPropValue *lppPropPCL, LPSPropValue *lppPropCK);

	// Reload callback
	static HRESULT Reload(void *lpParam, ECSESSIONID sessionid);

private:
	virtual HRESULT LockSoap();
	virtual HRESULT UnLockSoap();

private:
	entryId			m_sEntryId;		// Entryid of the folder
	ZarafaCmd*		lpCmd;			// command object
	pthread_mutex_t *lpDataLock;		//
	ECSESSIONID		ecSessionId;	// Id of the session
	ULONG			m_ulSessionReloadCallback;
	WSTransport *	m_lpTransport;
};

#endif
