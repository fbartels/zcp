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

#include <pthread.h>
#include "LDAPCache.h"
#include "LDAPUserPlugin.h"
#include <zarafa/stringutil.h>

LDAPCache::LDAPCache()
{
	pthread_mutexattr_init(&m_hMutexAttrib);
	pthread_mutexattr_settype(&m_hMutexAttrib, PTHREAD_MUTEX_RECURSIVE);

	pthread_mutex_init(&m_hMutex, &m_hMutexAttrib);

	m_lpCompanyCache = std::auto_ptr<dn_cache_t>(new dn_cache_t());
	m_lpGroupCache = std::auto_ptr<dn_cache_t>(new dn_cache_t());
	m_lpUserCache = std::auto_ptr<dn_cache_t>(new dn_cache_t());
	m_lpAddressListCache = std::auto_ptr<dn_cache_t>(new dn_cache_t());
}

LDAPCache::~LDAPCache()
{
	pthread_mutex_destroy(&m_hMutex);
	pthread_mutexattr_destroy(&m_hMutexAttrib);
}

bool LDAPCache::isObjectTypeCached(objectclass_t objclass)
{
	bool bCached = false;

	pthread_mutex_lock(&m_hMutex);

	switch (objclass) {
	case OBJECTCLASS_USER:
	case ACTIVE_USER:
	case NONACTIVE_USER:
	case NONACTIVE_ROOM:
	case NONACTIVE_EQUIPMENT:
	case NONACTIVE_CONTACT:
		bCached = !m_lpUserCache->empty();
		break;
	case OBJECTCLASS_DISTLIST:
	case DISTLIST_GROUP:
	case DISTLIST_SECURITY:
	case DISTLIST_DYNAMIC:
		bCached = !m_lpGroupCache->empty();
		break;
	case CONTAINER_COMPANY:
		bCached = !m_lpCompanyCache->empty();
		break;
	case CONTAINER_ADDRESSLIST:
		bCached = !m_lpAddressListCache->empty();
		break;
	default:
		break;
	}

	pthread_mutex_unlock(&m_hMutex);

	return bCached;
}

void LDAPCache::setObjectDNCache(objectclass_t objclass, std::auto_ptr<dn_cache_t> lpCache)
{
	/*
	 * Always merge caches rather then overwritting them.
	 */
	std::auto_ptr<dn_cache_t> lpTmp = getObjectDNCache(NULL, objclass);
	// cannot use insert() because it does not override existing entries
	for (dn_cache_t::const_iterator i = lpCache->begin();
	     i != lpCache->end(); ++i)
		(*lpTmp)[i->first] = i->second;
	lpCache = lpTmp;

	pthread_mutex_lock(&m_hMutex);

	switch (objclass) {
	case OBJECTCLASS_USER:
	case ACTIVE_USER:
	case NONACTIVE_USER:
	case NONACTIVE_ROOM:
	case NONACTIVE_EQUIPMENT:
	case NONACTIVE_CONTACT:
		m_lpUserCache = lpCache;
		break;
	case OBJECTCLASS_DISTLIST:
	case DISTLIST_GROUP:
	case DISTLIST_SECURITY:
	case DISTLIST_DYNAMIC:
		m_lpGroupCache = lpCache;
		break;
	case CONTAINER_COMPANY:
		m_lpCompanyCache = lpCache;
		break;
	case CONTAINER_ADDRESSLIST:
		m_lpAddressListCache = lpCache;
		break;
	default:
		break;
	}

	pthread_mutex_unlock(&m_hMutex);
}

std::auto_ptr<dn_cache_t> LDAPCache::getObjectDNCache(LDAPUserPlugin *lpPlugin, objectclass_t objclass)
{
	std::auto_ptr<dn_cache_t> cache;

	pthread_mutex_lock(&m_hMutex);

	/* If item was not yet cached, make sure it is done now. */
	if (!isObjectTypeCached(objclass) && lpPlugin)
		lpPlugin->getAllObjects(objectid_t(), objclass); // empty company, so request all objects of type

	switch (objclass) {
	case OBJECTCLASS_USER:
	case ACTIVE_USER:
	case NONACTIVE_USER:
	case NONACTIVE_ROOM:
	case NONACTIVE_EQUIPMENT:
	case NONACTIVE_CONTACT:
		cache.reset(new dn_cache_t(*m_lpUserCache.get()));
		break;
	case OBJECTCLASS_DISTLIST:
	case DISTLIST_GROUP:
	case DISTLIST_SECURITY:
	case DISTLIST_DYNAMIC:
		cache.reset(new dn_cache_t(*m_lpGroupCache.get()));
		break;
	case CONTAINER_COMPANY:
		cache.reset(new dn_cache_t(*m_lpCompanyCache.get()));
		break;
	case CONTAINER_ADDRESSLIST:
		cache.reset(new dn_cache_t(*m_lpAddressListCache.get()));
		break;
	default:
		break;
	}

	pthread_mutex_unlock(&m_hMutex);

	return cache;
}

objectid_t LDAPCache::getParentForDN(const std::auto_ptr<dn_cache_t> &lpCache, const std::string &dn)
{
	objectid_t entry;
	std::string parent_dn;

	if (lpCache->empty())
		goto exit;

	// @todo make sure we find the largest DN match
	for (dn_cache_t::const_iterator it = lpCache->begin();
	     it != lpCache->end(); ++it)
		/* Key should be larger then current guess, but has to be smaller then the userobject dn */
		/* If key matches the end of the userobject dn, we have a positive match */
		if (it->second.size() > parent_dn.size() && it->second.size() < dn.size() &&
		    stricmp(dn.c_str() + (dn.size() - it->second.size()), it->second.c_str()) == 0) {
			parent_dn = it->second;
			entry = it->first;
		}

exit:
	/* Either empty, or the correct result */
	return entry;
}

std::auto_ptr<dn_list_t> LDAPCache::getChildrenForDN(const std::auto_ptr<dn_cache_t> &lpCache, const std::string &dn)
{
	std::auto_ptr<dn_list_t> list = std::auto_ptr<dn_list_t>(new dn_list_t());

	/* Find al DNs which are hierarchically below the given dn */
	for (dn_cache_t::const_iterator iter = lpCache->begin();
	     iter != lpCache->end(); ++iter)
		/* Key should be larger then root DN */
		/* If key matches the end of the root dn, we have a positive match */
		if (iter->second.size() > dn.size() &&
		    stricmp(iter->second.c_str() + (iter->second.size() - dn.size()), dn.c_str()) == 0)
			list->push_back(iter->second);

	return list;
}

std::string LDAPCache::getDNForObject(const std::auto_ptr<dn_cache_t> &lpCache, const objectid_t &externid)
{
	dn_cache_t::const_iterator it = lpCache->find(externid);
	return it == lpCache->end() ? std::string() : it->second;
}

bool LDAPCache::isDNInList(const std::auto_ptr<dn_list_t> &lpList, const std::string &dn)
{
	/* We were given an DN, check if a parent of that dn is listed as filterd */
	for (dn_list_t::const_iterator iter = lpList->begin();
	     iter != lpList->end(); ++iter)
		/* Key should be larger or equal then user DN */
		/* If key matches the end of the user dn, we have a positive match */
		if (iter->size() <= dn.size() &&
		    stricmp(dn.c_str() + (dn.size() - iter->size()), iter->c_str()) == 0)
			return true;

	return false;
}
