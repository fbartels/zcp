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

#ifndef ECSEARCHCLIENT_H
#define ECSEARCHCLIENT_H

#include <zarafa/zcdefs.h>
#include <map>
#include <set>
#include <string>

#include <soapH.h>
#include <zarafa/ZarafaCode.h>

#include "ECChannelClient.h"

typedef struct {
    std::string strTerm;
    std::set<unsigned int> setFields;
} SIndexedTerm;

typedef std::set<unsigned int> setindexprops_t;

class ECSearchClient _zcp_final : public ECChannelClient {
public:
	ECSearchClient(const char *szIndexerPath, unsigned int ulTimeOut);
	~ECSearchClient();

	ECRESULT GetProperties(setindexprops_t &mapProps);
	ECRESULT Query(GUID *lpServerGuid, GUID *lpStoreGUID, std::list<unsigned int> &lstFolders, std::list<SIndexedTerm> &lstSearches, std::list<unsigned int> &lstMatches);
	ECRESULT SyncRun();
	
private:
	ECRESULT Scope(const std::string &strServer, const std::string &strStore, const std::list<unsigned int> &ulFolders);
	ECRESULT Find(const std::set<unsigned int> &setFields, const std::string &strTerm);
	ECRESULT Query(std::list<unsigned int> &lstMatches);
};

#endif /* ECSEARCHCLIENT_H */
