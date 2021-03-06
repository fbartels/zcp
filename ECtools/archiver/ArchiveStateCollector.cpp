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

#include <zarafa/zcdefs.h>
#include <zarafa/platform.h>
#include <zarafa/userutil.h>
#include "ArchiveStateCollector.h"
#include <zarafa/CommonUtil.h>
#include "ArchiverSession.h"
#include "ArchiveStateUpdater.h"
#include "ECIterators.h"
#include "HrException.h"
#include <zarafa/ECRestriction.h>

namespace details {

	/**
	 * Subclass of DataCollector that is used to get the current state
	 * through the MailboxTable.
	 */
	class MailboxDataCollector _zcp_final : public DataCollector {
	public:
		MailboxDataCollector(ArchiveStateCollector::ArchiveInfoMap &mapArchiveInfo, ECLogger *lpLogger);
		~MailboxDataCollector();

		HRESULT GetRequiredPropTags(LPMAPIPROP lpProp, LPSPropTagArray *lppPropTagArray) const _zcp_override;
		HRESULT CollectData(LPMAPITABLE lpStoreTable) _zcp_override;

	private:
		ArchiveStateCollector::ArchiveInfoMap &m_mapArchiveInfo;
		ECLogger *m_lpLogger;
	};

	MailboxDataCollector::MailboxDataCollector(ArchiveStateCollector::ArchiveInfoMap &mapArchiveInfo, ECLogger *lpLogger): m_mapArchiveInfo(mapArchiveInfo), m_lpLogger(lpLogger)
	{
		m_lpLogger->AddRef();
	}

	MailboxDataCollector::~MailboxDataCollector()
	{
		m_lpLogger->Release();
	}

	HRESULT MailboxDataCollector::GetRequiredPropTags(LPMAPIPROP lpProp, LPSPropTagArray *lppPropTagArray) const
	{
		HRESULT hr = hrSuccess;
		SPropTagArrayPtr ptrPropTagArray;

		PROPMAP_START
			PROPMAP_NAMED_ID(STORE_ENTRYIDS, PT_MV_BINARY, PSETID_Archive, "store-entryids")
			PROPMAP_NAMED_ID(ITEM_ENTRYIDS, PT_MV_BINARY, PSETID_Archive, "item-entryids")
		PROPMAP_INIT(lpProp);

		hr = MAPIAllocateBuffer(CbNewSPropTagArray(4), &ptrPropTagArray);
		if (hr != hrSuccess)
			goto exit;

		ptrPropTagArray->cValues = 4;
		ptrPropTagArray->aulPropTag[0] = PR_ENTRYID;
		ptrPropTagArray->aulPropTag[1] = PR_MAILBOX_OWNER_ENTRYID;
		ptrPropTagArray->aulPropTag[2] = PROP_STORE_ENTRYIDS;
		ptrPropTagArray->aulPropTag[3] = PROP_ITEM_ENTRYIDS;

		*lppPropTagArray = ptrPropTagArray.release();

	exit:
		return hr;
	}

	HRESULT MailboxDataCollector::CollectData(LPMAPITABLE lpStoreTable)
	{
		HRESULT hr = hrSuccess;
		SRowSetPtr ptrRows;

		enum {IDX_ENTRYID, IDX_MAILBOX_OWNER_ENTRYID, IDX_STORE_ENTRYIDS, IDX_ITEM_ENTRYIDS, IDX_MAX};

		while (true) {
			hr = lpStoreTable->QueryRows(50, 0, &ptrRows);
			if (hr != hrSuccess)
				goto exit;

			if (ptrRows.size() == 0)
				break;

			for (SRowSetPtr::size_type i = 0; i < ptrRows.size(); ++i) {
				std::pair<ArchiveStateCollector::ArchiveInfoMap::iterator, bool> res;
				bool bComplete = true;
				abentryid_t userId;

				for (unsigned j = 0; bComplete && j < IDX_MAX; ++j) {
					if (PROP_TYPE(ptrRows[i].lpProps[j].ulPropTag) == PT_ERROR) {
						m_lpLogger->Log(EC_LOGLEVEL_WARNING, "Got uncomplete row, row %u, column %u contains error 0x%08x", i, j, ptrRows[i].lpProps[j].Value.err);
						bComplete = false;
					}
				}
						
				if (!bComplete)
					continue;

				if (ptrRows[i].lpProps[IDX_STORE_ENTRYIDS].Value.MVbin.cValues != ptrRows[i].lpProps[IDX_ITEM_ENTRYIDS].Value.MVbin.cValues) {
					m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Mismatch in archive prop count, %u vs. %u", ptrRows[i].lpProps[IDX_STORE_ENTRYIDS].Value.MVbin.cValues, ptrRows[i].lpProps[IDX_ITEM_ENTRYIDS].Value.MVbin.cValues);
					continue;
				}

				userId.assign(ptrRows[i].lpProps[IDX_MAILBOX_OWNER_ENTRYID].Value.bin);
				res = m_mapArchiveInfo.insert(std::make_pair(userId, ArchiveStateCollector::ArchiveInfo()));
				if (res.second == true)
					m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Inserting row for user id %s", userId.tostring().c_str());
				else
					m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Updating row for user '" TSTRING_PRINTF "'", res.first->second.userName.c_str());

				// Assign entryid
				res.first->second.storeId.assign(ptrRows[i].lpProps[IDX_ENTRYID].Value.bin);

				// Assign archives
				m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Adding %u archive(s)", ptrRows[i].lpProps[IDX_STORE_ENTRYIDS].Value.MVbin.cValues);
				for (ULONG j = 0; j < ptrRows[i].lpProps[IDX_STORE_ENTRYIDS].Value.MVbin.cValues; ++j) {
					SObjectEntry objEntry;
					objEntry.sStoreEntryId.assign(entryid_t(ptrRows[i].lpProps[IDX_STORE_ENTRYIDS].Value.MVbin.lpbin[j]));
					objEntry.sItemEntryId.assign(entryid_t(ptrRows[i].lpProps[IDX_ITEM_ENTRYIDS].Value.MVbin.lpbin[j]));
					res.first->second.lstArchives.push_back(objEntry);
				}
			}
		}

	exit:
		return hr;
	}

}


/**
 * Create an ArchiveStateCollector instance.
 * @param[in]	ArchiverSessionPtr		The archive session
 * @param[in]	lpLogger		The logger.
 * @param[out]	lpptrCollector	The new ArchiveStateCollector instance.
 */
HRESULT ArchiveStateCollector::Create(const ArchiverSessionPtr &ptrSession, ECLogger *lpLogger, ArchiveStateCollectorPtr *lpptrCollector)
{
	HRESULT hr = hrSuccess;
	ArchiveStateCollectorPtr ptrCollector;
	
	try {
		ptrCollector = ArchiveStateCollectorPtr(new ArchiveStateCollector(ptrSession, lpLogger));
	} catch (const std::bad_alloc &) {
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}

	*lpptrCollector = ptrCollector;

exit:
	return hr;
}

/**
 * Constructor
 * @param[in]	ArchiverSessionPtr		The archive session
 * @param[in]	lpLogger		The logger.
 */
ArchiveStateCollector::ArchiveStateCollector(const ArchiverSessionPtr &ptrSession, ECLogger *lpLogger)
: m_ptrSession(ptrSession)
, m_lpLogger(new ECArchiverLogger(lpLogger))
{ }

ArchiveStateCollector::~ArchiveStateCollector()
{
	m_lpLogger->Release();
}

/**
 * Return an ArchiveStateUpdater instance that can update the current state
 * to the required state.
 * @param[out]	lpptrUpdate		The new ArchiveStateUpdater instance.
 */
HRESULT ArchiveStateCollector::GetArchiveStateUpdater(ArchiveStateUpdaterPtr *lpptrUpdater)
{
	HRESULT hr = hrSuccess;
	details::MailboxDataCollector mdc(m_mapArchiveInfo, m_lpLogger);

	hr = PopulateUserList();
	if (hr != hrSuccess)
		goto exit;

	hr = GetMailboxData(m_lpLogger, m_ptrSession->GetMAPISession(), m_ptrSession->GetSSLPath(), m_ptrSession->GetSSLPass(), false, &mdc);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Failed to get mailbox data. hr=0x%08x", hr);
		goto exit;
	}

	hr = ArchiveStateUpdater::Create(m_ptrSession, m_lpLogger, m_mapArchiveInfo, lpptrUpdater);
	if (hr != hrSuccess)
		goto exit;

	m_mapArchiveInfo.clear();

exit:
	return hr;
}

/**
 * Populate the user list through the GAL.
 * When this method completes, a list will be available of all users that
 * should have one or more archives attached to their primary store.
 */
HRESULT ArchiveStateCollector::PopulateUserList()
{
	HRESULT hr = hrSuccess;
	ABContainerPtr ptrABContainer;

	hr = m_ptrSession->GetGAL(&ptrABContainer);
	if (hr != hrSuccess)
		goto exit;

	hr = PopulateFromContainer(ptrABContainer);
	if (hr != hrSuccess)
		goto exit;

	try {
		for (ECABContainerIterator iter(ptrABContainer, 0); iter != ECABContainerIterator(); ++iter) {
			hr = PopulateFromContainer(*iter);
			if (hr != hrSuccess)
				goto exit;
		}
	} catch (const HrException &he) {
		hr = he.hr();
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to iterate addressbook containers. (hr=0x%08x)", hr);
		goto exit;
	}

exit:
	return hr;
}

/**
 * Populate the user list through one AB container.
 * When this method completes, the userlist will be available for all users
 * from the passed container that should have one or more archives attached to
 * their primary store.
 * @param[in]	lpContainer		The addressbook container to process.
 */
HRESULT ArchiveStateCollector::PopulateFromContainer(LPABCONT lpContainer)
{
	HRESULT hr = hrSuccess;
	SPropValue sPropObjType;
	SPropValue sPropDispType;
	SRestrictionPtr ptrRestriction;
	MAPITablePtr ptrTable;
	SRowSetPtr ptrRows;

	SizedSPropTagArray(4, sptaUserProps) = {4, {PR_ENTRYID, PR_ACCOUNT, PR_EC_ARCHIVE_SERVERS, PR_EC_ARCHIVE_COUPLINGS}};
	enum {IDX_ENTRYID, IDX_ACCOUNT, IDX_EC_ARCHIVE_SERVERS, IDX_EC_ARCHIVE_COUPLINGS};

	ECAndRestriction resFilter(
		ECPropertyRestriction(RELOP_EQ, PR_OBJECT_TYPE, &sPropObjType, ECRestriction::Cheap) +
		ECPropertyRestriction(RELOP_EQ, PR_DISPLAY_TYPE, &sPropDispType, ECRestriction::Cheap) +
		ECOrRestriction(
			ECExistRestriction(PR_EC_ARCHIVE_SERVERS) +
			ECExistRestriction(PR_EC_ARCHIVE_COUPLINGS)
		)
	);

	sPropObjType.ulPropTag = PR_OBJECT_TYPE;
	sPropObjType.Value.ul = MAPI_MAILUSER;
	sPropDispType.ulPropTag = PR_DISPLAY_TYPE;
	sPropDispType.Value.ul = DT_MAILUSER;;

	hr = resFilter.CreateMAPIRestriction(&ptrRestriction, ECRestriction::Cheap);
	if (hr != hrSuccess)
		goto exit;

	hr = lpContainer->GetContentsTable(0, &ptrTable);
	if (hr != hrSuccess)
		goto exit;

	hr = ptrTable->SetColumns((LPSPropTagArray)&sptaUserProps, TBL_BATCH);
	if (hr != hrSuccess)
		goto exit;

	hr = ptrTable->Restrict(ptrRestriction, TBL_BATCH);
	if (hr != hrSuccess)
		goto exit;

	while (true) {
		hr = ptrTable->QueryRows(50, 0, &ptrRows);
		if (hr != hrSuccess)
			goto exit;

		if (ptrRows.size() == 0)
			break;

		for (SRowSetPtr::size_type i = 0; i < ptrRows.size(); ++i) {
			ArchiveInfoMap::iterator iterator;
			
			if (ptrRows[i].lpProps[IDX_ENTRYID].ulPropTag != PR_ENTRYID) {
				m_lpLogger->Log(EC_LOGLEVEL_ERROR, "Unable to get entryid from address list. hr=0x%08x", ptrRows[i].lpProps[IDX_ACCOUNT].Value.err);
				continue;
			}

			if (ptrRows[i].lpProps[IDX_ACCOUNT].ulPropTag != PR_ACCOUNT) {
				m_lpLogger->Log(EC_LOGLEVEL_ERROR, "Unable to get username from address list. hr=0x%08x", ptrRows[i].lpProps[IDX_ACCOUNT].Value.err);
				continue;
			}

			m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Inserting row for user '" TSTRING_PRINTF "'", ptrRows[i].lpProps[IDX_ACCOUNT].Value.LPSZ);
			iterator = m_mapArchiveInfo.insert(std::make_pair(abentryid_t(ptrRows[i].lpProps[IDX_ENTRYID].Value.bin), ArchiveInfo())).first;

			iterator->second.userName.assign(ptrRows[i].lpProps[IDX_ACCOUNT].Value.LPSZ);

			if (ptrRows[i].lpProps[IDX_EC_ARCHIVE_SERVERS].ulPropTag == PR_EC_ARCHIVE_SERVERS) {
				m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Adding %u archive server(s)", ptrRows[i].lpProps[IDX_EC_ARCHIVE_SERVERS].Value.MVSZ.cValues);
				for (ULONG j = 0; j < ptrRows[i].lpProps[IDX_EC_ARCHIVE_SERVERS].Value.MVSZ.cValues; ++j)
					iterator->second.lstServers.push_back(ptrRows[i].lpProps[IDX_EC_ARCHIVE_SERVERS].Value.MVSZ.LPPSZ[j]);
			}

			if (ptrRows[i].lpProps[IDX_EC_ARCHIVE_COUPLINGS].ulPropTag == PR_EC_ARCHIVE_COUPLINGS) {
				m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Adding %u archive coupling(s)", ptrRows[i].lpProps[IDX_EC_ARCHIVE_COUPLINGS].Value.MVSZ.cValues);
				for (ULONG j = 0; j < ptrRows[i].lpProps[IDX_EC_ARCHIVE_COUPLINGS].Value.MVSZ.cValues; ++j)
					iterator->second.lstCouplings.push_back(ptrRows[i].lpProps[IDX_EC_ARCHIVE_COUPLINGS].Value.MVSZ.LPPSZ[j]);
			}
		}
	}

exit:
	return hr;
}
