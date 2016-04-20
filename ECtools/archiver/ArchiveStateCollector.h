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

#ifndef ARCHIVESTATECOLLECTOR_H_INCLUDED
#define ARCHIVESTATECOLLECTOR_H_INCLUDED

#include <boost/smart_ptr.hpp>
#include <map>
#include "archivestateupdater_fwd.h"
#include "ArchiverSessionPtr.h"     // For ArchiverSessionPtr
#include <zarafa/tstring.h>
#include <zarafa/archiver-common.h>
#include "ECArchiverLogger.h"

class ArchiveStateCollector;
typedef boost::shared_ptr<ArchiveStateCollector> ArchiveStateCollectorPtr;

/**
 * The ArchiveStateCollector will construct the current archive state, which
 * is the set of currently attached archives for each primary store, and the
 * should-be archive state, which is the set of attached archives for each
 * primary store as specified in LDAP/ADS.
 */
class ArchiveStateCollector {
public:
	static HRESULT Create(const ArchiverSessionPtr &ptrSession, ECLogger *lpLogger, ArchiveStateCollectorPtr *lpptrCollector);

	~ArchiveStateCollector();
	HRESULT GetArchiveStateUpdater(ArchiveStateUpdaterPtr *lpptrUpdater);

public:
	struct ArchiveInfo {
		tstring userName;
		entryid_t storeId;
		std::list<tstring> lstServers;
		std::list<tstring> lstCouplings;
		ObjectEntryList lstArchives;
	};
	typedef std::map<abentryid_t, ArchiveInfo> ArchiveInfoMap;

private:
	ArchiveStateCollector(const ArchiverSessionPtr &ptrSession, ECLogger *lpLogger);
	HRESULT PopulateUserList();
	HRESULT PopulateFromContainer(LPABCONT lpContainer);

private:
	ArchiverSessionPtr m_ptrSession;
	ECArchiverLogger *m_lpLogger;

	ArchiveInfoMap	m_mapArchiveInfo;
};

#endif // !defined ARCHIVESTATECOLLECTOR_H_INCLUDED
