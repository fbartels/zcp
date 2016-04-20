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

#include <string>
#include <list>
#include <memory>
#include <new>
#include <ostream>
#include "ArchiveManage.h"
#include "ArchiveManageImpl.h"
#include "ArchiverSession.h"
#include "helpers/StoreHelper.h"
#include <zarafa/charset/convert.h>
#include "ECACL.h"
#include <zarafa/ECConfig.h>
#include <zarafa/userutil.h>
#include "ArchiveStateUpdater.h"
#include <zarafa/ECRestriction.h>
#include <zarafa/MAPIErrors.h>
    
using namespace za::helpers;

inline UserEntry ArchiveManageImpl::MakeUserEntry(const std::string &strUser) {
	UserEntry entry;
	entry.UserName = strUser;
	return entry;
}

/**
 * Create an ArchiveManageImpl object.
 *
 * @param[in]	ptrSession
 *					Pointer to a Session object.
 * @param[in]	lpConfig
 * 					Pointer to an ECConfig object.
 * @param[in]	lpszUser
 *					The username of the user for which to create the archive manager.
 * @param[in]	lpLogger
 *					Pointer to an ECLogger object to which message will be logged.
 * @param[out]	lpptrArchiveManager
 *					Pointer to an ArchiveManagePtr that will be assigned the address of the returned object.
 *
 * @return HRESULT
 */
HRESULT ArchiveManageImpl::Create(ArchiverSessionPtr ptrSession, ECConfig *lpConfig, const TCHAR *lpszUser, ECLogger *lpLogger, ArchiveManagePtr *lpptrArchiveManage)
{
	HRESULT hr = hrSuccess;
	std::auto_ptr<ArchiveManageImpl> ptrArchiveManage;

	if (lpszUser == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}
	
	try {
		ptrArchiveManage.reset(new ArchiveManageImpl(ptrSession, lpConfig, lpszUser, lpLogger));
	} catch (std::bad_alloc &) {
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}
	
	hr = ptrArchiveManage->Init();
	if (hr != hrSuccess)
		goto exit;
	
	*lpptrArchiveManage = ptrArchiveManage;	// transfers ownership
	
exit:
	return hr;
}

HRESULT ArchiveManage::Create(LPMAPISESSION lpSession, ECLogger *lpLogger, const TCHAR *lpszUser, ArchiveManagePtr *lpptrManage)
{
	HRESULT hr = hrSuccess;
	ArchiverSessionPtr ptrArchiverSession;

	hr = ArchiverSession::Create(MAPISessionPtr(lpSession, true), NULL, lpLogger, &ptrArchiverSession);
	if (hr != hrSuccess)
		goto exit;

	hr = ArchiveManageImpl::Create(ptrArchiverSession, NULL, lpszUser, lpLogger, lpptrManage);

exit:
	return hr;
}

/**
 * Constructor
 *
 * @param[in]	ptrSession
 *					Pointer to a Session object.
 * @param[in]	lpConfig
 * 					Pointer to an ECConfig object.
 * @param[in]	lpszUser
 *					The username of the user for which to create the archive manager.
 * @param[in]	lpLogger
 *					Pointer to an ECLogger object to which message will be logged.
 */
ArchiveManageImpl::ArchiveManageImpl(ArchiverSessionPtr ptrSession, ECConfig *lpConfig, const tstring &strUser, ECLogger *lpLogger) :
	m_ptrSession(ptrSession),
	m_lpConfig(lpConfig),
	m_strUser(strUser)
{
	m_lpLogger = new(std::nothrow) ECArchiverLogger(lpLogger);
	if (m_lpLogger) {
		m_lpLogger->SetUser(strUser);
		if (lpConfig) {
			const char*	loglevelstring = lpConfig->GetSetting("log_level");
			if (loglevelstring) {
				unsigned int loglevel = strtoul(loglevelstring, NULL, 0);
				m_lpLogger->SetLoglevel(loglevel);
			}
		}
	}
}

/**
 *  Destructor
 */
ArchiveManageImpl::~ArchiveManageImpl()
{
	m_lpLogger->Release();
}

/**
 * Initialize the ArchiveManager object.
 */
HRESULT ArchiveManageImpl::Init()
{
	HRESULT hr = hrSuccess;

	hr = m_ptrSession->OpenStoreByName(m_strUser, &m_ptrUserStore);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to open user store '" TSTRING_PRINTF "' (hr=%s).", m_strUser.c_str(), stringify(hr, true).c_str());
		goto exit;
	}

exit:
	return hr;
}

/**
 * Attach an archive to the store of the user for which the ArchiveManger was created.
 *
 * @param[in]	lpszArchiveServer
 * 					If not NULL, this argument specifies the path of the server on which to create the archive.
 * @param[in]	lpszArchive
 *					The username of the non-active user that's the placeholder for the archive.
 * @param[in]	lpszFolder
 *					The name of the folder that will be used as the root of the archive. If ATT_USE_IPM_SUBTREE is passed
 *					in the ulFlags argument, the lpszFolder argument is ignored. If this argument is NULL, the username of
 *					the user is used as the root foldername.
 * @param[in]	ulFlags
 *					@ref flags specifying the options used for attaching the archive. 
 * @return HRESULT
 *
 * @section flags Flags
 * @li \b ATT_USE_IPM_SUBTREE	Use the IPM subtree of the archive store as the root of the archive.
 * @li \b ATT_WRITABLE			Make the archive writable for the user.
 */
eResult ArchiveManageImpl::AttachTo(const char *lpszArchiveServer, const TCHAR *lpszArchive, const TCHAR *lpszFolder, unsigned ulFlags)
{
	return MAPIErrorToArchiveError(AttachTo(lpszArchiveServer, lpszArchive, lpszFolder, ulFlags, ExplicitAttach));
}

HRESULT ArchiveManageImpl::AttachTo(const char *lpszArchiveServer, const TCHAR *lpszArchive, const TCHAR *lpszFolder, unsigned ulFlags, AttachType attachType)
{
	HRESULT	hr = hrSuccess;
	MsgStorePtr ptrArchiveStore;
	tstring strFoldername;
	abentryid_t sUserEntryId;
	ArchiverSessionPtr ptrArchiveSession(m_ptrSession);
	ArchiverSessionPtr ptrRemoteSession;

	hr = m_ptrSession->ValidateArchiverLicense();
	if (hr != hrSuccess)
		goto exit;

	// Resolve the requested user.
	hr = m_ptrSession->GetUserInfo(m_strUser, &sUserEntryId, &strFoldername, NULL);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to resolve user information for '" TSTRING_PRINTF "' (hr=%s).", m_strUser.c_str(), stringify(hr, true).c_str());
		goto exit;
	}
	
	
	if ((ulFlags & UseIpmSubtree) == UseIpmSubtree)
		strFoldername.clear();	// Empty folder name indicates tje IPM subtree.
	else if (lpszFolder)
		strFoldername.assign(lpszFolder);
		
	if (lpszArchiveServer) {
		hr = m_ptrSession->CreateRemote(lpszArchiveServer, m_lpLogger, &ptrRemoteSession);
		if (hr != hrSuccess) {
			m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to connect to archive server '%s' (hr=%s).", lpszArchiveServer, stringify(hr, true).c_str());
			goto exit;
		}
		
		ptrArchiveSession = ptrRemoteSession;
	}
	
	// Find the requested archive.
	hr = ptrArchiveSession->OpenStoreByName(lpszArchive, &ptrArchiveStore);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to open archive store '" TSTRING_PRINTF "' (hr=%s).", lpszArchive, stringify(hr, true).c_str());
		goto exit;
	}

	hr = AttachTo(ptrArchiveStore, strFoldername, lpszArchiveServer, sUserEntryId, ulFlags, attachType);

exit:
	return hr;
}

HRESULT ArchiveManageImpl::AttachTo(LPMDB lpArchiveStore, const tstring &strFoldername, const char *lpszArchiveServer, const abentryid_t &sUserEntryId, unsigned ulFlags, AttachType attachType)
{
	HRESULT hr = hrSuccess;
	ArchiveHelperPtr ptrArchiveHelper;
	abentryid_t sAttachedUserEntryId;
	StoreHelperPtr ptrStoreHelper;
	ObjectEntryList lstArchives;
	SObjectEntry objectEntry;
	bool bEqual = false;
	ArchiveType aType = UndefArchive;
	SPropValuePtr ptrArchiveName;
	SPropValuePtr ptrArchiveStoreId;
	
	// Check if we're not trying to attach a store to itself.
	hr = m_ptrSession->CompareStoreIds(m_ptrUserStore, lpArchiveStore, &bEqual);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to compare user and archive store (hr=%s).", stringify(hr, true).c_str());
		goto exit;
	}
	if (bEqual) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "User and archive store are the same.");
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	hr = HrGetOneProp(lpArchiveStore, PR_ENTRYID, &ptrArchiveStoreId);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to get archive store entryid (hr=%s).", stringify(hr, true).c_str());
		goto exit;
	}

	hr = StoreHelper::Create(m_ptrUserStore, &ptrStoreHelper);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to create store helper (hr=%s).", stringify(hr, true).c_str());
		goto exit;
	}
	
	hr = ptrStoreHelper->GetArchiveList(&lstArchives);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to get archive list (hr=%s).", stringify(hr, true).c_str());
		goto exit;	
	}

	// Find ptrArchiveStoreId in lstArchives
	for (ObjectEntryList::const_iterator i = lstArchives.begin(); i != lstArchives.end(); ++i) {
		bool bEqual;
		if (m_ptrSession->CompareStoreIds(i->sStoreEntryId, ptrArchiveStoreId->Value.bin, &bEqual) == hrSuccess && bEqual) {
			m_lpLogger->Log(EC_LOGLEVEL_FATAL, "An archive for this '" TSTRING_PRINTF "' is already present in this store.", m_strUser.c_str());
			hr = MAPI_E_UNABLE_TO_COMPLETE;
			goto exit;
		}
	}

	hr = ArchiveHelper::Create(lpArchiveStore, strFoldername, lpszArchiveServer, &ptrArchiveHelper);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to create archive helper (hr=%s).", stringify(hr, true).c_str());
		goto exit;
	}

	// Check if the archive is usable for the requested type
	hr = ptrArchiveHelper->GetArchiveType(&aType, NULL);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to get archive type (hr=%s).", stringify(hr, true).c_str());
		goto exit;
	}

	m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Archive Type: %d", (int)aType);
	if (aType == UndefArchive) {
		m_lpLogger->Log(EC_LOGLEVEL_NOTICE, "Preparing archive for first use");
		hr = ptrArchiveHelper->PrepareForFirstUse(m_lpLogger);
		if (hr != hrSuccess) {
			m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to prepare archive (hr=0x%08x).", hr);
			goto exit;
		}
	} else if (aType == SingleArchive && !strFoldername.empty()) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Attempted to create an archive folder in an archive store that has an archive in its root.");
		hr = MAPI_E_COLLISION;
		goto exit;
	} else if (aType == MultiArchive && strFoldername.empty()) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Attempted to create an archive in the root of an archive store that has archive folders.");
		hr = MAPI_E_COLLISION;
		goto exit;
	}

	// Check if the archive is attached yet.
	hr = ptrArchiveHelper->GetAttachedUser(&sAttachedUserEntryId);
	if (hr == MAPI_E_NOT_FOUND)
		hr = hrSuccess;

	else if ( hr == hrSuccess && (!sAttachedUserEntryId.empty() && sAttachedUserEntryId != sUserEntryId)) {
		tstring strUser;
		tstring strFullname;

		hr = m_ptrSession->GetUserInfo(sAttachedUserEntryId, &strUser, &strFullname);
		if (hr == hrSuccess)
			m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Archive is already used by " TSTRING_PRINTF " (" TSTRING_PRINTF ").", strUser.c_str(), strFullname.c_str());
		else
			m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Archive is already used (user entry: %s).", sAttachedUserEntryId.tostring().c_str());

		hr = MAPI_E_COLLISION;
		goto exit;
	} else if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to get attached user for the requested archive (hr=%s).", stringify(hr, true).c_str());
		goto exit;
	}
		
	// Add new archive to list of archives.
	hr = ptrArchiveHelper->GetArchiveEntry(true, &objectEntry);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to get archive entry (hr=%s).", stringify(hr, true).c_str());
		goto exit;	
	}

	lstArchives.push_back(objectEntry);
	lstArchives.sort();
	lstArchives.unique();
	
	hr = ptrStoreHelper->SetArchiveList(lstArchives);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to update archive list (hr=%s).", stringify(hr, true).c_str());
		goto exit;	
	}
		
		
	hr = ptrArchiveHelper->SetAttachedUser(sUserEntryId);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to mark archive used (hr=%s).", stringify(hr, true).c_str());
		goto exit;	
	}

	hr = ptrArchiveHelper->SetArchiveType(strFoldername.empty() ? SingleArchive : MultiArchive, attachType);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to set archive type to %d (hr=%s).", (int)(strFoldername.empty() ? SingleArchive : MultiArchive), stringify(hr, true).c_str());
		goto exit;
	}
	
	// Update permissions
	if (!lpszArchiveServer) {	// No need to set permissions on a remote archive.
		hr = ptrArchiveHelper->SetPermissions(sUserEntryId, (ulFlags & Writable) == Writable);
		if (hr != hrSuccess) {
			m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to set permissions on archive (hr=%s).", stringify(hr, true).c_str());
			goto exit;	
		}
	}

	
	// Create search folder
	hr = ptrStoreHelper->UpdateSearchFolders();
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to set search folders (hr=%s).", stringify(hr, true).c_str());
		goto exit;	
	}

	hr = HrGetOneProp(lpArchiveStore, PR_DISPLAY_NAME, &ptrArchiveName);
	if (hr != hrSuccess) {
		hr = hrSuccess;
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Successfully attached '" TSTRING_PRINTF "' in 'Unknown' for user '" TSTRING_PRINTF "'.", strFoldername.empty() ? _T("Root") : strFoldername.c_str(), m_strUser.c_str());
	} else
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Successfully attached '" TSTRING_PRINTF "' in '" TSTRING_PRINTF "' for user '" TSTRING_PRINTF "'.", strFoldername.empty() ? _T("Root") : strFoldername.c_str(), ptrArchiveName->Value.LPSZ, m_strUser.c_str());

exit:
	return hr;
}

/**
 * Detach an archive from a users store.
 *
 * @param[in]	lpszArchiveServer
 * 					If not NULL, this argument specifies the path of the server on which to create the archive.
 * @param[in]	lpszArchive
 *					The username of the non-active user that's the placeholder for the archive.
 * @param[in]	lpszFolder
 *					The name of the folder that's be used as the root of the archive. If this paramater
 *					is set to NULL and the user has only one archive in the archive store, which 
 *					is usually the case, that archive will be detached. If a user has multiple archives
 *					in the archive store, the exact folder need to be specified.
 *					If the archive root was placed in the IPM subtree of the archive store, this parameter
 *					must be set to NULL.
 *
 * @return HRESULT
 */
eResult ArchiveManageImpl::DetachFrom(const char *lpszArchiveServer, const TCHAR *lpszArchive, const TCHAR *lpszFolder)
{
	HRESULT hr = hrSuccess;
	entryid_t sUserEntryId;
	StoreHelperPtr ptrStoreHelper;
	ObjectEntryList lstArchives;
	ObjectEntryList::iterator iArchive;
	MsgStorePtr ptrArchiveStore;
	ArchiveHelperPtr ptrArchiveHelper;
	SPropValuePtr ptrArchiveStoreEntryId;
	MAPIFolderPtr ptrArchiveFolder;
	SPropValuePtr ptrDisplayName;
	ULONG ulType = 0;
	ArchiverSessionPtr ptrArchiveSession(m_ptrSession);
	ArchiverSessionPtr ptrRemoteSession;


	hr = StoreHelper::Create(m_ptrUserStore, &ptrStoreHelper);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to create store helper (hr=%s).", stringify(hr, true).c_str());
		goto exit;
	}
	
	hr = ptrStoreHelper->GetArchiveList(&lstArchives);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to get archive list (hr=%s).", stringify(hr, true).c_str());
		goto exit;	
	}



	if (lpszArchiveServer) {
		hr = m_ptrSession->CreateRemote(lpszArchiveServer, m_lpLogger, &ptrRemoteSession);
		if (hr != hrSuccess) {
			m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to connect to archive server '%s' (hr=%s).", lpszArchiveServer, stringify(hr, true).c_str());
			goto exit;
		}
		
		ptrArchiveSession = ptrRemoteSession;
	}

	hr = ptrArchiveSession->OpenStoreByName(lpszArchive, &ptrArchiveStore);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to open archive store '" TSTRING_PRINTF "' (hr=%s).", lpszArchive, stringify(hr, true).c_str());
		goto exit;
	}
	
	hr = HrGetOneProp(ptrArchiveStore, PR_ENTRYID, &ptrArchiveStoreEntryId);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to get archive entryid (hr=%s).", stringify(hr, true).c_str());
		goto exit;
	}


	// Find an archives on the passed store.
	iArchive = find_if(lstArchives.begin(), lstArchives.end(), StoreCompare(ptrArchiveStoreEntryId->Value.bin));
	if (iArchive == lstArchives.end()) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "'" TSTRING_PRINTF "' has no archive on '" TSTRING_PRINTF "'", m_strUser.c_str(), lpszArchive);
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}
	
	// If no folder name was passed and there are more archives for this user on this archive, we abort.
	if (lpszFolder == NULL) {
		ObjectEntryList::iterator iNextArchive(iArchive);
		++iNextArchive;

		if (find_if(iNextArchive, lstArchives.end(), StoreCompare(ptrArchiveStoreEntryId->Value.bin)) != lstArchives.end()) {
			m_lpLogger->Log(EC_LOGLEVEL_FATAL, "'" TSTRING_PRINTF "' has multiple archives on '" TSTRING_PRINTF "'", m_strUser.c_str(), lpszArchive);
			hr = MAPI_E_COLLISION;
			goto exit;
		}
	}
	
	// If a folder name was passed, we need to find the correct folder.
	if (lpszFolder) {
		while (iArchive != lstArchives.end()) {
			hr = ptrArchiveStore->OpenEntry(iArchive->sItemEntryId.size(), iArchive->sItemEntryId, &ptrArchiveFolder.iid, fMapiDeferredErrors, &ulType, &ptrArchiveFolder);
			if (hr != hrSuccess) {
				m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to open archive folder (hr=%s).", stringify(hr, true).c_str());
				goto exit;
			}
			
			hr = HrGetOneProp(ptrArchiveFolder, PR_DISPLAY_NAME, &ptrDisplayName);
			if (hr != hrSuccess) {
				m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to get archive folder name (hr=%s).", stringify(hr, true).c_str());
				goto exit;
			}
			
			if (_tcscmp(ptrDisplayName->Value.LPSZ, lpszFolder) == 0)
				break;
				
			iArchive = find_if(++iArchive, lstArchives.end(), StoreCompare(ptrArchiveStoreEntryId->Value.bin));
		}
		
		if (iArchive == lstArchives.end()) {
			m_lpLogger->Log(EC_LOGLEVEL_FATAL, "'" TSTRING_PRINTF "' has no archive named '" TSTRING_PRINTF "' on '" TSTRING_PRINTF "'", m_strUser.c_str(), lpszFolder, lpszArchive);
			hr = MAPI_E_NOT_FOUND;
			goto exit;				
		}
	}
	
	assert(iArchive != lstArchives.end());
	lstArchives.erase(iArchive);
	
	hr = ptrStoreHelper->SetArchiveList(lstArchives);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to update archive list (hr=%s).", stringify(hr, true).c_str());
		goto exit;	
	}

	// Update search folders
	hr = ptrStoreHelper->UpdateSearchFolders();
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to set search folders (hr=%s).", stringify(hr, true).c_str());
		goto exit;	
	}

	if (lpszFolder)
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Successfully detached '" TSTRING_PRINTF "' in '" TSTRING_PRINTF "' from '" TSTRING_PRINTF "'.", lpszFolder, lpszArchive, m_strUser.c_str());
	else
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Successfully detached '" TSTRING_PRINTF "' from '" TSTRING_PRINTF "'.", lpszArchive, m_strUser.c_str());

exit:
	return MAPIErrorToArchiveError(hr);
}

/**
 * Detach an archive from a users store based on its index
 *
 * @param[in]	ulArchive
 * 					The index of the archive in the list of archives.
 *
 * @return HRESULT
 */
eResult ArchiveManageImpl::DetachFrom(unsigned int ulArchive)
{
	HRESULT hr = hrSuccess;
	StoreHelperPtr ptrStoreHelper;
	ObjectEntryList lstArchives;
	ObjectEntryList::iterator iArchive;

	hr = StoreHelper::Create(m_ptrUserStore, &ptrStoreHelper);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to create store helper (hr=%s).", stringify(hr, true).c_str());
		goto exit;
	}

	hr = ptrStoreHelper->GetArchiveList(&lstArchives);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to get archive list (hr=%s).", stringify(hr, true).c_str());
		goto exit;
	}

	iArchive = lstArchives.begin();
	for (unsigned int i = 0; i < ulArchive && iArchive != lstArchives.end(); ++i, ++iArchive);
	if (iArchive == lstArchives.end()) {
		hr = MAPI_E_NOT_FOUND;
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Archive %u does not exist.", ulArchive);
		goto exit;
	}

	lstArchives.erase(iArchive);

	hr = ptrStoreHelper->SetArchiveList(lstArchives);
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to update archive list (hr=%s).", stringify(hr, true).c_str());
		goto exit;	
	}

	// Update search folders
	hr = ptrStoreHelper->UpdateSearchFolders();
	if (hr != hrSuccess) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Failed to set search folders (hr=%s).", stringify(hr, true).c_str());
		goto exit;	
	}

	m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Successfully detached archive %u from '" TSTRING_PRINTF "'.", ulArchive, m_strUser.c_str());
	
exit:
	return MAPIErrorToArchiveError(hr);
}

/**
 * List the attached archives for a user.
 *
 * @param[in]	ostr
 *					The std::ostream to which the list will be outputted.
 *
 * @return HRESULT
 */
eResult ArchiveManageImpl::ListArchives(std::ostream &ostr)
{
	eResult er = Success;
	ArchiveList	lstArchives;
	ULONG ulIdx = 0;

	er = ListArchives(&lstArchives, "Root Folder");
	if (er != Success)
		goto exit;

	ostr << "User '" << convert_to<std::string>(m_strUser) << "' has " << lstArchives.size() << " attached archives:" << std::endl;
	for (ArchiveList::const_iterator iArchive = lstArchives.begin(); iArchive != lstArchives.end(); ++iArchive, ++ulIdx) {
		ostr << "\t" << ulIdx
			 << ": Store: " << iArchive->StoreName
			 << ", Folder: " << iArchive->FolderName;

		if (iArchive->Rights != ARCHIVE_RIGHTS_ABSENT) {
			 ostr << ", Rights: ";
			if (iArchive->Rights == ROLE_OWNER)
				ostr << "Read Write";
			else if (iArchive->Rights == ROLE_REVIEWER)
				ostr << "Read Only";
			else
				ostr << "Modified: " << AclRightsToString(iArchive->Rights);
		}

		ostr << std::endl;
	}

exit:
	return er;
}

eResult ArchiveManageImpl::ListArchives(ArchiveList *lplstArchives, const char *lpszIpmSubtreeSubstitude)
{
	HRESULT hr = hrSuccess;
	StoreHelperPtr ptrStoreHelper;
	bool bAclCapable = true;
	ObjectEntryList lstArchives;
	ObjectEntryList::const_iterator iArchive;
	MsgStorePtr ptrArchiveStore;
	ULONG ulType = 0;
	ArchiveList lstEntries;

	hr = StoreHelper::Create(m_ptrUserStore, &ptrStoreHelper);
	if (hr != hrSuccess)
		goto exit;

	hr = m_ptrSession->GetUserInfo(m_strUser, NULL, NULL, &bAclCapable);
	if (hr != hrSuccess)
		goto exit;

	hr = ptrStoreHelper->GetArchiveList(&lstArchives);
	if (hr != hrSuccess)
		goto exit;
		
	for (iArchive = lstArchives.begin(); iArchive != lstArchives.end(); ++iArchive) {
		HRESULT hrTmp = hrSuccess;
		ULONG cStoreProps = 0;
		SPropArrayPtr ptrStoreProps;
		ArchiveEntry entry;
		MAPIFolderPtr ptrArchiveFolder;
		SPropValuePtr ptrPropValue;
		ULONG ulCompareResult = FALSE;

		SizedSPropTagArray(4, sptaStoreProps) = {4, {PR_DISPLAY_NAME_A, PR_MAILBOX_OWNER_ENTRYID, PR_IPM_SUBTREE_ENTRYID, PR_STORE_RECORD_KEY}};
		enum {IDX_DISPLAY_NAME, IDX_MAILBOX_OWNER_ENTRYID, IDX_IPM_SUBTREE_ENTRYID, IDX_STORE_RECORD_KEY};

		entry.Rights = ARCHIVE_RIGHTS_ERROR;

		hrTmp = m_ptrSession->OpenStore(iArchive->sStoreEntryId, &ptrArchiveStore);
		if (hrTmp != hrSuccess) {
			m_lpLogger->Log(EC_LOGLEVEL_ERROR, "Failed to open store (hr=%s)", stringify(hrTmp, true).c_str());
			entry.StoreName = "Failed id=" + iArchive->sStoreEntryId.tostring() + ", hr=" + stringify(hrTmp, true);
			lstEntries.push_back(entry);
			continue;
		}
		
		hrTmp = ptrArchiveStore->GetProps((LPSPropTagArray)&sptaStoreProps, 0, &cStoreProps, &ptrStoreProps);
		if (FAILED(hrTmp))
			entry.StoreName = entry.StoreOwner = "Unknown (" + stringify(hrTmp, true) + ")";
		else {
			if (ptrStoreProps[IDX_DISPLAY_NAME].ulPropTag == PR_DISPLAY_NAME_A)
				entry.StoreName = ptrStoreProps[IDX_DISPLAY_NAME].Value.lpszA;
			else
				entry.StoreName = "Unknown (" + stringify(ptrStoreProps[IDX_DISPLAY_NAME].Value.err, true) + ")";
				
			if (ptrStoreProps[IDX_MAILBOX_OWNER_ENTRYID].ulPropTag == PR_MAILBOX_OWNER_ENTRYID) {
				MAPIPropPtr ptrOwner;

				hrTmp = m_ptrSession->OpenMAPIProp(ptrStoreProps[IDX_MAILBOX_OWNER_ENTRYID].Value.bin.cb,
												  (LPENTRYID)ptrStoreProps[IDX_MAILBOX_OWNER_ENTRYID].Value.bin.lpb,
												  &ptrOwner);
				if (hrTmp == hrSuccess)
					hrTmp = HrGetOneProp(ptrOwner, PR_ACCOUNT_A, &ptrPropValue);

				if (hrTmp == hrSuccess)
					entry.StoreOwner = ptrPropValue->Value.lpszA;
				else
					entry.StoreOwner = "Unknown (" + stringify(hrTmp, true) + ")";
			} else
				entry.StoreOwner = "Unknown (" + stringify(ptrStoreProps[IDX_MAILBOX_OWNER_ENTRYID].Value.err, true) + ")";

			if (lpszIpmSubtreeSubstitude) {
				if (ptrStoreProps[IDX_IPM_SUBTREE_ENTRYID].ulPropTag != PR_IPM_SUBTREE_ENTRYID)
					hrTmp = MAPI_E_NOT_FOUND;
				else
					hrTmp = ptrArchiveStore->CompareEntryIDs(iArchive->sItemEntryId.size(), iArchive->sItemEntryId,
															 ptrStoreProps[IDX_IPM_SUBTREE_ENTRYID].Value.bin.cb, (LPENTRYID)ptrStoreProps[IDX_IPM_SUBTREE_ENTRYID].Value.bin.lpb,
															 0, &ulCompareResult);
				if (hrTmp != hrSuccess) {
					m_lpLogger->Log(EC_LOGLEVEL_ERROR, "Failed to compare entry ids (hr=%s)", stringify(hrTmp, true).c_str());
					ulCompareResult = FALSE;	// Let's assume it's not the IPM Subtree.
				}
			}

			if (ptrStoreProps[IDX_STORE_RECORD_KEY].ulPropTag == PR_STORE_RECORD_KEY) {
				entry.StoreGuid = bin2hex(ptrStoreProps[IDX_STORE_RECORD_KEY].Value.bin.cb, ptrStoreProps[IDX_STORE_RECORD_KEY].Value.bin.lpb);
			}
		}


		hrTmp = ptrArchiveStore->OpenEntry(iArchive->sItemEntryId.size(), iArchive->sItemEntryId, &ptrArchiveFolder.iid, fMapiDeferredErrors, &ulType, &ptrArchiveFolder);
		if (hrTmp != hrSuccess) {
			m_lpLogger->Log(EC_LOGLEVEL_ERROR, "Failed to open folder (hr=%s)", stringify(hrTmp, true).c_str());
			entry.FolderName = "Failed id=" + iArchive->sStoreEntryId.tostring() + ", hr=" + stringify(hrTmp, true);
			lstEntries.push_back(entry);
			continue;
		}
		
		if (lpszIpmSubtreeSubstitude && ulCompareResult == TRUE) {
			ASSERT(lpszIpmSubtreeSubstitude != NULL);
			entry.FolderName = lpszIpmSubtreeSubstitude;
		} else {
			hrTmp = HrGetOneProp(ptrArchiveFolder, PR_DISPLAY_NAME_A, &ptrPropValue);
			if (hrTmp != hrSuccess)
				entry.FolderName = "Unknown (" + stringify(hrTmp, true) + ")";
			else
				entry.FolderName = ptrPropValue->Value.lpszA ;
		}

		if (bAclCapable && !iArchive->sStoreEntryId.isWrapped()) {
			hrTmp = GetRights(ptrArchiveFolder, &entry.Rights);
			if (hrTmp != hrSuccess)
				m_lpLogger->Log(EC_LOGLEVEL_ERROR, "Failed to get archive rights (hr=%s)", stringify(hrTmp, true).c_str());
		} else
			entry.Rights = ARCHIVE_RIGHTS_ABSENT;

		lstEntries.push_back(entry);
	}

	lplstArchives->swap(lstEntries);
	
exit:
	return MAPIErrorToArchiveError(hr);
}



/**
 * Print a list of users with an attached archive store
 *
 * @param[in]  ostr
 *                     Output stream to write results to.
 *
 * @return eResult
*/
eResult ArchiveManageImpl::ListAttachedUsers(std::ostream &ostr)
{
	eResult er = Success;
	UserList lstUsers;

	er = ListAttachedUsers(&lstUsers);
	if (er != Success)
		goto exit;

	if (lstUsers.empty()) {
		ostr << "No users have an archive attached." << std::endl;
		goto exit;
	}

	ostr << "Users with an attached archive:" << std::endl;
	for (UserList::const_iterator iUser = lstUsers.begin(); iUser != lstUsers.end(); ++iUser)
		ostr << "\t" << iUser->UserName << std::endl;

exit:
	return er;
}

eResult ArchiveManageImpl::ListAttachedUsers(UserList *lplstUsers)
{
	HRESULT hr = hrSuccess;
	std::list<std::string> lstUsers;
	UserList lstUserEntries;

	if (lplstUsers == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	hr = GetArchivedUserList(m_lpLogger, 
							 m_ptrSession->GetMAPISession(),
							 m_ptrSession->GetSSLPath(),
							 m_ptrSession->GetSSLPass(),
							 &lstUsers);
	if (hr != hrSuccess)
		goto exit;

	std::transform(lstUsers.begin(), lstUsers.end(), std::back_inserter(lstUserEntries), &MakeUserEntry);
	lplstUsers->swap(lstUserEntries);

exit:
	return MAPIErrorToArchiveError(hr);
}

/**
 * Auto attach and detach archives to user stores based on the addressbook
 * settings.
 */
eResult ArchiveManageImpl::AutoAttach(unsigned int ulFlags)
{
	HRESULT hr = hrSuccess;
    m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "ArchiveManageImpl::AutoAttach(): function entry");
	ArchiveStateCollectorPtr ptrArchiveStateCollector;
	ArchiveStateUpdaterPtr ptrArchiveStateUpdater;

	if (ulFlags != ArchiveManage::Writable && ulFlags != ArchiveManage::ReadOnly && ulFlags != 0) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

    m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "ArchiveManageImpl::AutoAttach(): about to create ArchiveStateCollector");
	hr = ArchiveStateCollector::Create(m_ptrSession, m_lpLogger, &ptrArchiveStateCollector);
	if (hr != hrSuccess)
		goto exit;

    m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "ArchiveManageImpl::AutoAttach(): about to get ArchiveStateUpdater");
	hr = ptrArchiveStateCollector->GetArchiveStateUpdater(&ptrArchiveStateUpdater);
	if (hr != hrSuccess)
		goto exit;

	if (ulFlags == 0) {
		if (!m_lpConfig || parseBool(m_lpConfig->GetSetting("auto_attach_writable")))
			ulFlags = ArchiveManage::Writable;
		else
			ulFlags = ArchiveManage::ReadOnly;
	}

    m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "ArchiveManageImpl::AutoAttach(): about to call ArchiveStateUpdater::Update");
	hr = ptrArchiveStateUpdater->Update(m_strUser, ulFlags);

exit:
    m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "ArchiveManageImpl::AutoAttach(): function exit. Result: 0x%08X (%s)", hr, GetMAPIErrorMessage(hr));
	return MAPIErrorToArchiveError(hr);
}

/**
 * Obtain the rights for the user for which the instance of ArhiceveManageImpl
 * was created on the passed folder.
 *
 * @param[in]	lpFolder	The folder to get the rights from
 * @param[out]	lpulRights	The rights the current user has on the folder.
 */
HRESULT ArchiveManageImpl::GetRights(LPMAPIFOLDER lpFolder, unsigned *lpulRights)
{
	HRESULT hr = hrSuccess;
	SPropValuePtr ptrName;
	ExchangeModifyTablePtr ptrACLModifyTable;
	MAPITablePtr ptrACLTable;
	SPropValue sPropUser;
	ECPropertyRestriction res(RELOP_EQ, PR_MEMBER_NAME, &sPropUser, ECRestriction::Cheap);
	SRestrictionPtr ptrRes;
	SRowSetPtr ptrRows;

	SizedSPropTagArray(1, sptaTableProps) = {1, {PR_MEMBER_RIGHTS}};

	if (lpFolder == NULL || lpulRights == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	// In an ideal world we would use the user entryid for the restriction.
	// However, the ACL table is a client side table, which doesn't implement
	// comparing AB entryids correctly over multiple servers. Since we're
	// most likely dealing with multiple servers here, we'll use the users
	// fullname instead.
	hr = HrGetOneProp(m_ptrUserStore, PR_MAILBOX_OWNER_NAME, &ptrName);
	if (hr != hrSuccess)
		goto exit;

	hr = lpFolder->OpenProperty(PR_ACL_TABLE, &IID_IExchangeModifyTable, 0, 0, &ptrACLModifyTable);
	if (hr != hrSuccess)
		goto exit;

	hr = ptrACLModifyTable->GetTable(0, &ptrACLTable);
	if (hr != hrSuccess)
		goto exit;

	hr = ptrACLTable->SetColumns((LPSPropTagArray)&sptaTableProps, TBL_BATCH);
	if (hr != hrSuccess)
		goto exit;

	sPropUser.ulPropTag = PR_MEMBER_NAME;
	sPropUser.Value.LPSZ = ptrName->Value.LPSZ;

	hr = res.CreateMAPIRestriction(&ptrRes, ECRestriction::Cheap);
	if (hr != hrSuccess)
		goto exit;

	hr = ptrACLTable->FindRow(ptrRes, 0, BOOKMARK_BEGINNING);
	if (hr != hrSuccess)
		goto exit;

	hr = ptrACLTable->QueryRows(1, 0, &ptrRows);
	if (hr != hrSuccess)
		goto exit;

	if (ptrRows.empty()) {
		ASSERT(false);
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	*lpulRights = ptrRows[0].lpProps[0].Value.ul;

exit:
	return hr;
}
