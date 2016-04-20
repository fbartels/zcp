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

#ifndef LDAPCACHE_H
#define LDAPCACHE_H

#include <memory>
#include <list>
#include <map>
#include <string>

#include <zarafa/ECDefs.h>
#include <zarafa/ZarafaUser.h>

class LDAPUserPlugin;

/**
 * @defgroup userplugin_ldap_cache LDAP user plugin cache
 * @ingroup userplugin_ldap
 * @{
 */


/**
 * Cache type, std::string is LDAP DN sting
 */
typedef std::map<objectid_t, std::string> dn_cache_t;
typedef std::list<std::string> dn_list_t;

/**
 * LDAP Cache which collects DNs with the matching
 * objectid and name.
 */
class LDAPCache {
private:
	/* Protect mutex from being overriden */
	pthread_mutex_t m_hMutex;
	pthread_mutexattr_t m_hMutexAttrib;

	std::auto_ptr<dn_cache_t> m_lpCompanyCache;		/* CONTAINER_COMPANY */
	std::auto_ptr<dn_cache_t> m_lpGroupCache;		/* OBJECTCLASS_DISTLIST */
	std::auto_ptr<dn_cache_t> m_lpUserCache;		/* OBJECTCLASS_USER */
	std::auto_ptr<dn_cache_t> m_lpAddressListCache; /* CONTAINER_ADDRESSLIST */

public:
	/**
	 * Default constructor
	 */
	LDAPCache();

	/**
	 * Destructor
	 */
	~LDAPCache();

	/**
	 * Check if the requested objclass class is cached.
	 *
	 * @param[in]	objclass
	 *					The objectclass which could be cached.
	 * @return TRUE if the object class is cached.
	 */
	bool isObjectTypeCached(objectclass_t objclass);

	/**
	 * Add new entries for the object cache.
	 *
	 * @param[in]	objclass
	 *					The objectclass which should be cached.
	 * @param[in]	lpCache
	 *					The data to add to the cache.
	 */
	void setObjectDNCache(objectclass_t objclass, std::auto_ptr<dn_cache_t> lpCache);

	/**
	 * Obtain the cached data
	 *
	 * @param[in]	lpPlugin
	 *					Pointer to the plugin, if the objectclass was not cached,
	 *					lpPlugin->getAllObjects() will be called to fill the cache.
	 * @param[in]	objclass
	 *						The objectclass for which the cache is requested
	 * @return The cache data
	 */
	std::auto_ptr<dn_cache_t> getObjectDNCache(LDAPUserPlugin *lpPlugin, objectclass_t objclass);

	/**
	 * Helper function: Search the cache for the direct parent for a DN.
	 * If the DN has multiple parents only the parent closest to the DN will be returned.
	 *
	 * @param[in]	lpCache
	 *					The cache which should be checked.
	 * @param[in]	dn
	 *					The DN for which the parent should be found.
	 * @return The cache entry of the parent. Contents will be empty if no parent was found.
	 */
	static objectid_t getParentForDN(const std::auto_ptr<dn_cache_t> &lpCache, const std::string &dn);

	/**
	 * Helper function: List all DNs which are hierarchially below the given DN.
	 *
	 * @param[in]	lpCache
	 *					The cache which should be checked.
	 * @param[in]	dn
	 *					The DN for which the children should be found
	 * @return The list of children for the DN
	 */
	static std::auto_ptr<dn_list_t> getChildrenForDN(const std::auto_ptr<dn_cache_t> &lpCache, const std::string &dn);

	/**
	 * Search the cache to obtain the DN for an object based on the object id.
	 *
	 * @param[in]	lpCache
	 *					The cache which should be checked.
	 * @param[in]	externid
	 *					The objectid which should be found in the cache
	 * @return the DN for the object id
	 */
	static std::string getDNForObject(const std::auto_ptr<dn_cache_t> &lpCache, const objectid_t &externid);

	/**
	 * Check if the given DN is present in the DN list
	 *
	 * @param[in]	lpList
	 *					The list which should be checked
	 * @param[in]	dn
	 *					The DN which should be found in the list
	 * @return TRUE if the DN is found in the list
	 */
	static bool isDNInList(const std::auto_ptr<dn_list_t> &lpList, const std::string &dn);
};

/** @} */

#endif /* LDAPCACHE_H */
