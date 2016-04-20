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

#ifdef LINUX
#include <sys/un.h>
#include <sys/socket.h>
#endif

#include <zarafa/base64.h>
#include <zarafa/ECChannel.h>
#include <zarafa/ECDefs.h>
#include <zarafa/stringutil.h>

#include "ECSearchClient.h"

ECSearchClient::ECSearchClient(const char *szIndexerPath, unsigned int ulTimeOut)
	: ECChannelClient(szIndexerPath, ":;")
{
	m_ulTimeout = ulTimeOut;
}

ECSearchClient::~ECSearchClient()
{
}

ECRESULT ECSearchClient::GetProperties(setindexprops_t &setProps)
{
	ECRESULT er;
	std::vector<std::string> lstResponse;
	std::vector<std::string> lstProps;
	std::vector<std::string>::const_iterator iter;

	er = DoCmd("PROPS", lstResponse);
	if (er != erSuccess)
		return er;

	setProps.clear();
	if (lstResponse.empty())
		return erSuccess; // No properties

	lstProps = tokenize(lstResponse[0], " ");

	for (iter = lstProps.begin(); iter != lstProps.end(); ++iter)
		setProps.insert(atoui(iter->c_str()));
	return erSuccess;
}

/**
 * Output SCOPE command
 *
 * Specifies the scope of the search.
 *
 * @param strServer[in] Server GUID to search in
 * @param strStore[in] Store GUID to search in
 * @param lstFolders[in] List of folders to search in. As a special case, no folders means 'all folders'
 * @return result
 */
ECRESULT ECSearchClient::Scope(const std::string &strServer,
    const std::string &strStore, const std::list<unsigned int> &lstFolders)
{
	ECRESULT er;
	std::vector<std::string> lstResponse;
	std::string strScope;

	er = Connect();
	if (er != erSuccess)
		return er;

	strScope = "SCOPE " + strServer + " " + strStore;
	for (std::list<unsigned int>::const_iterator i = lstFolders.begin();
	     i != lstFolders.end(); ++i)
		strScope += " " + stringify(*i);

	er = DoCmd(strScope, lstResponse);
	if (er != erSuccess)
		return er;
	if (!lstResponse.empty())
		return ZARAFA_E_BAD_VALUE;
	return erSuccess;
}

/**
 * Output FIND command
 *
 * The FIND command specifies which term to look for in which fields. When multiple
 * FIND commands are issued, items must match ALL of the terms.
 *
 * @param setFields[in] Fields to match (may match any of these fields)
 * @param strTerm[in] Term to match (utf-8 encoded)
 * @return result
 */
ECRESULT ECSearchClient::Find(const std::set<unsigned int> &setFields,
    const std::string &strTerm)
{
	ECRESULT er;
	std::vector<std::string> lstResponse;
	std::string strFind;

	strFind = "FIND";
	for (std::set<unsigned int>::const_iterator i = setFields.begin();
	     i != setFields.end(); ++i)
		strFind += " " + stringify(*i);
		
	strFind += ":";
	
	strFind += strTerm;

	er = DoCmd(strFind, lstResponse);
	if (er != erSuccess)
		return er;
	if (!lstResponse.empty())
		return ZARAFA_E_BAD_VALUE;
	return erSuccess;
}

/**
 * Run the search query
 *
 * @param lstMatches[out] List of matches as hierarchy IDs
 * @return result
 */
ECRESULT ECSearchClient::Query(std::list<unsigned int> &lstMatches)
{
	ECRESULT er;
	std::vector<std::string> lstResponse;
	std::vector<std::string> lstResponseIds;
	
	lstMatches.clear();

	er = DoCmd("QUERY", lstResponse);
	if (er != erSuccess)
		return er;
		
	if (lstResponse.empty())
		return erSuccess; /* no matches */

	lstResponseIds = tokenize(lstResponse[0], " ");

	for (unsigned int i = 0; i < lstResponseIds.size(); ++i)
		lstMatches.push_back(atoui(lstResponseIds[i].c_str()));
	return erSuccess;
}

/**
 * Do a full search query
 *
 * This function actually executes a number of commands:
 *
 * SCOPE <serverid> <storeid> <folder1> ... <folderN>
 * FIND <field1> ... <fieldN> : <term>
 * QUERY
 *
 * @param lpServerGuid[in] Server GUID to search in
 * @param lpStoreGuid[in] Store GUID to search in
 * @param lstFolders[in] List of folders to search in
 * @param lstSearches[in] List of searches that items should match (AND)
 * @param lstMatches[out] Output of matching items
 * @return result
 */
 
ECRESULT ECSearchClient::Query(GUID *lpServerGuid, GUID *lpStoreGuid, std::list<unsigned int>& lstFolders, std::list<SIndexedTerm> &lstSearches, std::list<unsigned int> &lstMatches)
{
	ECRESULT er;
	std::string strServer = bin2hex(sizeof(GUID), (unsigned char *)lpServerGuid);
	std::string strStore = bin2hex(sizeof(GUID), (unsigned char *)lpStoreGuid);

	er = Scope(strServer, strStore, lstFolders);
	if (er != erSuccess)
		return er;

	for (std::list<SIndexedTerm>::const_iterator i = lstSearches.begin();
	     i != lstSearches.end(); ++i)
		Find(i->setFields, i->strTerm);

	return Query(lstMatches);
}

ECRESULT ECSearchClient::SyncRun()
{
	std::vector<std::string> lstVoid;
	return DoCmd("SYNCRUN", lstVoid);
}

