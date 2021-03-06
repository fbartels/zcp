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

// -*- Mode: c++ -*-
#ifndef LDAPUSERPLUGIN_H
#define LDAPUSERPLUGIN_H

#include <stdexcept>
#include <string>
#include <zarafa/ECIConv.h>

#include <set>

// OpenLDAP
#include <ldap.h>
// win32 defines this because of clashing types, so we use this define outside win32 too
#define LDAP_TIMEVAL struct timeval

#include "plugin.h"
#include "LDAPCache.h"

/**
 * @defgroup userplugin_ldap LDAP userplugin
 * @ingroup userplugin
 * @{
 */


/** 
 * LDAP user plugin
 *
 * User management based on LDAP.
 *
 * @todo update documentation with the right exception, some function can throw more exceptions!
 */
class LDAPUserPlugin: public UserPlugin {
public:
	/** 
	 * Create a connection to the LDAP server and do some
	 * initialization.
	 *
	 * Configuration parameters: \c ldap_url
	 *
	 * Possible configuration parameters: \c ldap_use_tls, \c
	 * ldap_bind_user, \c ldap_bind_passwd
	 *
	 * @param[in]	pluginlock
	 *					The plugin mutex
	 * @param[in]   shareddata
	 *					The singleton shared plugin data.
	 * @throw std::runtime_error on failure, such as being unable to
	 * connect to the LDAP server, or when the bind credentials are
	 * incorrect.
	 *
	 * @todo The constructor sets the maximum size of query results to
	 * infinite, instead it'd be better to use ldap_search instead of
	 * ldap_search_s.
	 */
	LDAPUserPlugin(pthread_mutex_t *pluginlock, ECPluginSharedData *shareddata);

	/**
	 * Destructor
	 */
	virtual ~LDAPUserPlugin();

    /**
	 * Initialize plugin
	 *
	 * @throw std::exception
	 */
	virtual void InitPlugin();

	/**
	 * Resolve name and company to objectsignature
	 *
	 * @param[in]	objclass
	 *					The objectclass of the name which should be resolved.
	 *					The objectclass can be partially unknown (OBJECTCLASS_UNKNOWN, MAILUSER_UNKNOWN, ...)
	 * @param[in]	name
	 *					The name which should be resolved. name should be in charset windows-1252
	 * @param[in]	company
	 *					The company beneath which the name should be searched
	 *					This objectid can be empty.
	 * @return The object signature of the resolved object
	 * @throw runtime_error When an unsupported objclass was requested
	 * @throw objectnotfound When no object was found with the requested name or objectclass
	 * @throw collison_error When more then one object was found with the requested name
	 */
	virtual objectsignature_t resolveName(objectclass_t objclass, const string &name, const objectid_t &company);

	/**
	 * Authenticate user with username and password
	 *
	 * Depending on the authentication type this will call
	 * LDAPUserPlugin::authenticateUserPassword() or LDAPUserPlugin::authenticateUserBind()
	 *
	 * @param[in]	username
	 *					The username of the user to be authenticated. username should be in charset windows-1252
	 * @param[in]	password
	 *					The password of the user to be authenticated. password should be in charset windows-1252
	 * @param[in]	company
	 *					The objectid of the company to which the user belongs.
	 *					This objectid can be empty.
	 * @return The objectsignature of the authenticated user
	 */
	virtual objectsignature_t authenticateUser(const string &username, const string &password, const objectid_t &company);

	/**
	 * Request a list of objects for a particular company and specified objectclass.
	 *
	 * @param[in]	company
	 *					The company beneath which the objects should be listed.
	 *					This objectid can be empty.
	 * @param[in]	objclass
	 *					The objectclass of the objects which should be returned.
	 *					The objectclass can be partially unknown (OBJECTCLASS_UNKNOWN, MAILUSER_UNKNOWN, ...)
	 * @return The list of object signatures of all objects which were found
	 */
	virtual auto_ptr<signatures_t> getAllObjects(const objectid_t &company, objectclass_t objclass);

	/**
	 * Obtain the object details for the given object
	 *
	 * This will call LDAPUserPlugin::getObjectDetails(const list<objectid_t> &objectids)
	 *
	 * @param[in]	objectid
	 *					The objectid for which is details are requested
	 * @return The objectdetails for the given objectid
	 * @throw objectnotfound When the object was not found
	 */
	virtual auto_ptr<objectdetails_t> getObjectDetails(const objectid_t &objectid);

	/**
	 * Obtain the object details for the given objects
	 *
	 * @param[in]	objectids
	 *					The list of object signatures for which the details are requested
	 * @return A map of objectid with the matching objectdetails
	 * @throw runtime_error When the LDAP query failed
	 *
	 * @remarks The methode returns a whole set of objectdetails but user may be missing if the user
	 * 			details cannot be retrieved for some reason.
	 */
	virtual auto_ptr<map<objectid_t, objectdetails_t> > getObjectDetails(const list<objectid_t> &objectids);

	/**
	 * Get all children for a parent for a given relation type.
	 * For example all users in a group
	 *
	 * @param[in]	relation
	 *					The relation type which connects the child and parent object
	 * @param[in]	parentobject
	 *					The parent object for which the children are requested
	 * @return A list of object signatures of the children of the parent.
	 * @throw When an unsupported object relation was requested
	 */
	virtual auto_ptr<signatures_t> getSubObjectsForObject(userobject_relation_t relation, const objectid_t &parentobject);

	/**
	 * Request all parents for a childobject for a given relation type.
	 * For example all groups for a user
	 *
	 * @param[in]	relation
	 *					The relation type which connects the child and parent object
	 * @param[in]	childobject
	 *					The childobject for which the parents are requested
	 * @return A list of object signatures of the parents of the child.
	 * @throw runtime_error When an unsupported object relation was requested
	 */
	virtual auto_ptr<signatures_t> getParentObjectsForObject(userobject_relation_t relation, const objectid_t &childobject);

	/**
	 * Search for all objects which match the given string,
	 * the name and email address should be compared for this search.
	 *
	 * @param[in]	match
	 *					The string which should be found
	 * @param[in]	ulFlags
	 *					If EMS_AB_ADDRESS_LOOKUP the string must exactly match the name or email address
	 *					otherwise a partial match is allowed.
	 * @return List of object signatures which match the given string
	 * @throw objectnotfound When no objects were found
	 */
	virtual auto_ptr<signatures_t> searchObject(const string &match, unsigned int ulFlags);

	/**
	 * Obtain details for the public store
	 *
	 * @return The public store details
	 * @throw runtime_error When LDAP query failed or mandatory attributes are missing
	 * @throw objectnotfound When no public store was found
	 * @throw toomanyobjects When more then one public store has been found
	 */
	virtual auto_ptr<objectdetails_t> getPublicStoreDetails();

	/**
	 * Obtain the objectdetails for a server
	 *
	 * @param[in]	server
	 *					The externid of the server
	 * @return The server details
	 * @throw runtime_error When LDAP query failed or mandatory attributes are missing
	 * @throw objectnotfound When no server has been found with the given name
	 * @throw toomanyobjects When more then one server have been found with the given name
	 */
	virtual auto_ptr<serverdetails_t> getServerDetails(const string &server);

	/**
	 * Obtain server list
	 *
	 * @return list of servers
	 * @throw runtime_error LDAP query failure
	 */
	virtual auto_ptr<serverlist_t> getServers();

	/**
	 * Update an object with new details
	 *
	 * @note This function is not supported and will always throw an exception
	 *
	 * @param[in]	id
	 *					The object id of the object which should be updated.
	 * @param[in]	details
	 *					The objectdetails which should be written to the object.
	 * @param[in]	lpRemove
	 *					List of configuration names which should be removed from the object
	 * @throw notimplemented Always when the function is called.
	 */
	virtual void changeObject(const objectid_t &id, const objectdetails_t &details, const std::list<std::string> *lpRemove);

	/**
	 * Create object in plugin
	 *
	 * @note This function is not supported and will always throw an exception
	 *
	 * @param[in]	details
	 *					The object details of the new object.
	 * @return The objectsignature of the created object.
	 * @throw notimplemented Always when the function is called.
	 */
	virtual objectsignature_t createObject(const objectdetails_t &details);

    /**
	 * Delete object from plugin
	 *
	 * @note This function is not supported and will always throw an exception
	 *
	 * @param[in]	id
	 *					The objectid which should be deleted
	 * @throw notimplemented Always when the function is called.
	 */
	virtual void deleteObject(const objectid_t &id);

	/**
	 * Modify id of object in plugin
	 *
	 * @note This function is not supported by this plugin and will always throw an exception
	 *
	 * @param[in]	oldId
	 *					The old objectid
	 * @param[in]	newId
	 *					The new objectid
	 * @throw notsupported Always when this function is called
	 */
	virtual void modifyObjectId(const objectid_t &oldId, const objectid_t &newId);

	/**
	 * Add relation between child and parent. This can be used
	 * for example to add a user to a group or add
	 * permission relations on companies.
	 *
	 * @note This function is not supported and will always throw an exception
	 *
	 * @param[in]   relation
	 *					The relation type which should connect the
	 *					child and parent.
	 * @param[in]	parentobject
	 *					The parent object.
	 * @param[in]	childobject
	 *					The child object.
	 * @throw notimplemented Always when the function is called.
	 */
	virtual void addSubObjectRelation(userobject_relation_t relation,
									  const objectid_t &parentobject, const objectid_t &childobject);

	/**
	 * Delete relation between child and parent, this can be used
	 * for example to delete a user from a group or delete
	 * permission relations on companies.
	 *
	 * @note This function is not supported and will always throw an exception
	 *
	 * @param[in]	relation
	 *					The relation type which connected the child and parent.
	 * @param[in]	parentobject
	 *					The parent object.
	 * @param[in]	childobject
	 *					The child object.
	 * @throw notimplemented Always when the function is called.
	 */
	virtual void deleteSubObjectRelation(userobject_relation_t relation,
										 const objectid_t& parentobject, const objectid_t &childobject);

	/**
	 * Get quota information from object.
	 * There are two quota types, normal quota and userdefault quota,
	 * the first quota is the quote for the object itself while the userdefault
	 * quota can only be requested on containers (i.e. groups or companies) and
	 * is the quota for the members of that container.
	 *
	 * @param[in]	id
	 *					The objectid from which the quota should be read
	 * @param[in]	bGetUserDefault
	 *					Boolean to indicate if the userdefault quota must be requested.
	 * @throw runtime_error When the LDAP query failed
	 */	 
	virtual auto_ptr<quotadetails_t> getQuota(const objectid_t &id, bool bGetUserDefault);

    /**
	 * Set quota information on object
	 *
	 * @note This function is not supported and will always throw an exception
	 *
	 * @param[in]	id
	 *					The objectid which should be updated
	 * @param[in]	quotadetails
	 *					The quota information which should be written to the object
	 * @throw notimplemented Always when the function is called.
	 */
	virtual void setQuota(const objectid_t &id, const quotadetails_t &quotadetails);

	/**
	 * Get extra properties which are set in the object details for the addressbook
	 *
	 * @note It is not mandatory to implement this function
	 *
	 * @return	a list of properties
	 */
	virtual auto_ptr<abprops_t> getExtraAddressbookProperties();

	virtual void removeAllObjects(objectid_t except);

protected:
	/**
	 * Pointer to the LDAP state struct.
	 */
	LDAP *m_ldap;

	/**
	 * converter FROM ldap TO zarafa-server
	 */
	ECIConv *m_iconv;

	/**
	 * converter FROM zarafa-server TO ldap
	 */
	ECIConv *m_iconvrev;

	static auto_ptr<LDAPCache> m_lpCache;

	LDAP_TIMEVAL m_timeout;

private:
	/**
	 * Get the value of the given attribute from the search results.
	 *
	 * This is a convenience function that uses
	 * getLDAPAttributeValues.  Parse the result of an LDAP search
	 * query, and retrieve exactly one string.  If the attribute does
	 * not occur in the search results, or if there are no values for
	 * the attribute, returns an empty string.
	 *
	 * @param[in]	attribute
	 *					The name of the attribute.
	 * @param[in]	entry
	 *					The entry result from \c ldap_first_entry &c.
	 * @returns The first value of the attribute, or an empty string
	 * if the attribute or a value was not found.
	 */
	std::string getLDAPAttributeValue(char *attribute, LDAPMessage *entry);

	/**
	 * Get multiple values of the given attribute from the search results.
	 *
	 * Parse the result of an LDAP search query, and retrieve all the
	 * values for the given attribute.
	 *
	 * @param[in]	attribute
	 *					The name of the attribute.
	 * @param[in]	entry
	 *					The entry result from \c ldap_first_entry &c.
	 * @return list of strings containing the attribute values
	 */
	std::list<std::string> getLDAPAttributeValues(char *attribute, LDAPMessage *entry);

	/**
	 * Get DN for given entry
	 *
	 * @param[in]	entry
	 *					The entry result form the \c ldap_first_entry &c.
	 * @return The DN for the entry
	 */
	std::string GetLDAPEntryDN(LDAPMessage *entry);

	/**
	 * Set one attribute of \c dn to \c value
	 *
	 * @param[in]	dn
	 *					The DN to modify.
	 * @param[in]	attribute
	 *					The name of the attribute to modify.
	 * @param[in]	value
	 *					The new value of the attribute.
	 * @return 0 When the modification was successful, 1 otherwise (error).
	 */
	int changeAttribute(const char *dn, char *attribute, const char *value);

	/**
	 * Set one attribute of \c dn to the values in \c values
	 *
	 * @param[in]	dn
	 *					The DN to modify.
	 * @param[in]	attribute
	 *					The name of the attribute to modify.
	 * @param[in]	values
	 *					A list of new values. Any old values are overwritten or removed.
	 * @retval 0 When the modification was successful, 1 Otherwise (error).
	 */
	int changeAttribute(const char *dn, char *attribute, const std::list<std::string> &values);

	/**
	 * Connect to the LDAP server
	 *
	 * @param[in]	bind_dn
	 *					The DN for the administrator
	 * @param[in]	bind_pw
	 *					The password for the administrator
	 * @return LDAP pointer
	 * @throw ldap_error When no connection could be established
	 */
	LDAP *ConnectLDAP(const char *bind_dn, const char *bind_pw);

	/**
	 * Authenticate by user bind
	 *
	 * @param[in]	username
	 *					The username in charset windows-1252
	 * @param[in]	password
	 *					The password for the username in charset windows-1252
	 * @param[in]	company
	 *					The company to which the user belongs (optional argument)
     * @return The object signature of the authenticated user
	 * @throw runtime_error When the LDAP query fails.
	 * @throw login_error When the username and password are incorrect.
	 */
	objectsignature_t authenticateUserBind(const string &username, const string &password,
										   const objectid_t &company = objectid_t(CONTAINER_COMPANY));

	/**
	 * Authenticate by username and password
	 *
	 * @param[in]	username
	 *					The username in charset windows-1252
	 * @param[in]	password
	 *					The password for the username in charset windows-1252
	 * @param[in]	company
	 *					The company to which the user belongs (optional argument)
	 * @return The object signature of the authenticated user
	 * @throw runtime_error When the LDAP query fails.
	 * @throw objectnotfound When no user with the given name has been found.
	 * @throw login_error When the username and password are incorrect.
	 */
	objectsignature_t authenticateUserPassword(const string &username, const string &password,
											   const objectid_t &company = objectid_t(CONTAINER_COMPANY));

	/**
	 * Convert objectid to a DN
	 *
	 * @param[in]	uniqueid
	 *					The unique id which should be converted
	 * @return the DN for the object
	 * @throw runtime_error When an error occured during the LDAP query.
	 * @throw objectnotfound When no object was found with the given objectid.
	 * @throw toomanyobjects When more then one object was returned with the objectid.
	 */
	string objectUniqueIDtoObjectDN(const objectid_t &uniqueid, bool cache = true);

	/**
	 * Convert a DN to an object signature
	 *
	 * @param[in]	objclass
	 *					The objectclass to which this search should be restricted.
	 *					The objectclass can be partially unknown (OBJECTCLASS_UNKNOWN, MAILUSER_UNKNOWN, ...)
	 * @param[in]	dn
	 *					The DN to convert
	 * @return The objectsignature
	 * @throw objectnotfound When the DN does not exist or does not match the object class.
	 * @throw toomanyobjects When more then one object was found
	 */
	objectsignature_t objectDNtoObjectSignature(objectclass_t objclass, const string &dn);

	/**
	 * Convert a list of DN's to a list of object signatures
	 *
	 * @param[in]	objclass
	 *					The objectclass to which this search should be restricted.
	 *					The objectclass can be partially unknown (OBJECTCLASS_UNKNOWN, MAILUSER_UNKNOWN, ...)
	 * @param[in]	dn
	 *					List of DN's
	 * @return The list of objectsignatures
	 */
	auto_ptr<signatures_t> objectDNtoObjectSignatures(objectclass_t objclass, const list<string> &dn);

	/**
	 * Escape binary data to escaped string
	 *
	 * @param[in]	lpdata
	 *					The binary data
	 * @param[in]	size
	 *					The length of the binary data
	 * @param[out]	lpEscaped
	 *					Escaped string
	 * @return HRESULT
	 */
	HRESULT BintoEscapeSequence(const char* lpdata, size_t size, string* lpEscaped);

	/**
	 * Escape binary data to escaped string
	 *
	 * @param[in]	lpdata
	 *					The binary data
	 * @param[in]	size
	 *					The length of the binary data
	 * @return Escaped string
	 */
	std::string StringEscapeSequence(const char* lpdata, size_t size);

	/**
	 * Escape binary data to escaped string
	 *
	 * @param[in]	strData
	 *					The binary data
	 * @return Escaped string
	 */
	std::string StringEscapeSequence(const string &strData);

	/**
	 * Determine the search base for a LDAP query
	 *
	 * @param[in]	company
	 *					Optional argument, the company for which the base should be found
	 * @return The search base
	 * @throw runtime_error When the configuration option ldap_search_base is empty.
	 */
	string getSearchBase(const objectid_t &company = objectid_t(CONTAINER_COMPANY));

	/**
	 * Create a search filter for servers
	 *
	 * @return The server search filter
	 */
	string getServerSearchFilter();

	/**
	 * Create LDAP search filter based on the object class.
	 *
	 * @param[in]	objclass
	 *					The objectclass for which the filter should be created
	 *					The objectclass can be partially unknown (OBJECTCLASS_UNKNOWN, MAILUSER_UNKNOWN, ...)
	 * @return The search filter for the specified object class
	 * @throw runtime_error when an invalid objectclass is requested or configuration options are missing.
	 */
	string getSearchFilter(objectclass_t objclass = OBJECTCLASS_UNKNOWN);

	/**
	 * Create LDAP search filter based on the object data and the attribute in which the date should
	 * be found. If attr is NULL this function will return an empty string,
	 *
	 * @param[in]	data
	 *					The object data
	 * @param[in]	attr
	 *					Optional argument, The attribute in which the data should be found.
	 * @param[in]	attr_type
	 *					Optional argument, The attribute type (text, DN, binary, ...)
	 * @return The LDAP Search filter.
	 */
	string getSearchFilter(const string &data,  const char *attr = NULL, const char *attr_type = NULL);

	/**
	 * Create LDAP search filter based on the object id and the attribute in which the object id should
	 * be found. If attr is empty the object class will be used to discover the unique attribute for
	 * that object class.
	 *
	 * @param[in]	id
	 *					The object id for which the LDAP filter should be created
	 * @param[in]	attr
	 *					Optional argument, The attribute in which the object id should be found.
	 * @param[in]	attr_type
	 *					Optional argument, The attribute type (text, DN, binary, ...)
	 * @return The LDAP Search filter.
	 * @throw runtime_error When an invalid objectclass was requested
	 */
	string getObjectSearchFilter(const objectid_t &id, const char *attr = NULL, const char *attr_type = NULL);

	/**
	 * Resolve object from attribute data
	 *
	 * This will call LDAPUserPlugin::resolveObjectsFromAttribute()
	 *
	 * @param[in]	objclass
	 *					The objectclass to which this search should be restricted.
	 *					The objectclass can be partially unknown (OBJECTCLASS_UNKNOWN, MAILUSER_UNKNOWN, ...)
	 * @param[in]	AttrData
	 *					The contents of the attribute
	 * @param[in]	lpAttr
	 *					The attribute which should contain the AttrData. lpAttr must be in charset which defined 
	 *					in config value ldap_server_charset
	 * @param[in]	company
	 *					Optional argument, The company where the possible object should belong.
	 * @return The object signature which was found
	 * @throw objectnotfound When no object was found with the attribute data
	 * @throw toomanyobjects When more then one object was found with the attribute data
	 */
	objectsignature_t resolveObjectFromAttribute(objectclass_t objclass,
												 const string &AttrData, const char* lpAttr,
												 const objectid_t &company = objectid_t(CONTAINER_COMPANY));

	/**
	 * Resolve objects from attribute data
	 *
	 * This will call LDAPUserPlugin::resolveObjectsFromAttributes()
	 *
	 * @param[in]	objclass
	 *					The objectclass to which this search should be restricted.
	 *					The objectclass can be partially unknown (OBJECTCLASS_UNKNOWN, MAILUSER_UNKNOWN, ...)
	 * @param[in]	objects
	 *					The list of atribute data
	 * @param[in]	lpAttr
	 *					The attribute which should contain the AttrData
	 * @param[in]	company
	 *					Optional argument, The company where the possible object should belong.
	 * @return The list of object signatures which were found
	 */
	auto_ptr<signatures_t> resolveObjectsFromAttribute(objectclass_t objclass,
													   const list<string> &objects, const char* lpAttr,
													   const objectid_t &company = objectid_t(CONTAINER_COMPANY));

	/**
	 * Resolve objects from attribute data by checking if the data contains
	 * in any of the provided attributes.
	 *
	 * This will call LDAPUserPlugin::getAllObjectsByFilter()
	 *
	 * @param[in]	objclass
	 *					The objectclass to which this search should be restricted.
	 *					The objectclass can be partially unknown (OBJECTCLASS_UNKNOWN, MAILUSER_UNKNOWN, ...)
	 * @param[in]	objects
	 *					The list of atribute data
	 * @param[in]	lppAttr
	 *					The attributes which should contain the AttrData
	 * @param[in]	company
	 *					Optional argument, The company where the possible object should belong.
	 * @return The list of object signatures which were found
	 */
	auto_ptr<signatures_t> resolveObjectsFromAttributes(objectclass_t objclass,
														const list<string> &objects, const char** lppAttr,
														const objectid_t &company = objectid_t(CONTAINER_COMPANY));

	/**
	 * Resolve object from attribute data depending on the attribute type
	 *
	 * This will call LDAPUserPlugin::resolveObjectsFromAttributeType()
	 *
	 * @param[in]	objclass
	 *					The objectclass to which this search should be restricted.
	 *					The objectclass can be partially unknown (OBJECTCLASS_UNKNOWN, MAILUSER_UNKNOWN, ...)
	 * @param[in]	AttrData
	 *					The contents of the attribute
	 * @param[in]	lpAttr
	 *					The attribute which should contain the AttrData
	 * @param[in]	lpAttrType
	 *					The attribute type of the attribte, can be DN, text or binary
	 * @param[in]	company
	 *					Optional argument, The company where the possible object should belong.
	 * @return The object signature which was found
	 * @throw objectnotfound When no object was found with the attribute data
	 * @throw toomanyobjects When more then one object was found with the attribute data
	 */
	objectsignature_t resolveObjectFromAttributeType(objectclass_t objclass,
													 const string &AttrData, const char* lpAttr, const char* lpAttrType,
													 const objectid_t &company = objectid_t(CONTAINER_COMPANY));

	/**
	 * Resolve objects from attribute data depending on the attribute type
	 *
	 * This will call LDAPUserPlugin::resolveObjectsFromAttributes()
	 *
	 * @param[in]	objclass
	 *					The objectclass to which this search should be restricted.
	 *					The objectclass can be partially unknown (OBJECTCLASS_UNKNOWN, MAILUSER_UNKNOWN, ...)
	 * @param[in]	objects
	 *					The list of atribute data
	 * @param[in]	lpAttr
	 *					The attribute which should contain the AttrData
	 * @param[in]	lpAttrType
	 *					The attribute type of the attribtes, can be DN, text or binary
	 * @param[in]	company
	 *					Optional argument, The company where the possible object should belong.
	 * @return The list of object signatures which were found
	 */
	auto_ptr<signatures_t> resolveObjectsFromAttributeType(objectclass_t objclass,
														   const list<string> &objects, const char* lpAttr, const char* lpAttrType,
														   const objectid_t &company = objectid_t(CONTAINER_COMPANY));

	/**
	 * Resolve objects from attribute data by checking if the data contains
	 * in any of the provided attributes depending on the attribute type.
	 *
	 * This will call LDAPUserPlugin::objectDNtoObjectSignatures() or
	 * LDAPUserPlugin::resolveObjectsFromAttributes() depending on the attribute type.
	 *
	 * @param[in]	objclass
	 *					The objectclass to which this search should be restricted.
	 *					The objectclass can be partially unknown (OBJECTCLASS_UNKNOWN, MAILUSER_UNKNOWN, ...)
	 * @param[in]	objects
	 *					The list of atribute data
	 * @param[in]	lppAttr
	 *					The attributes which should contain the AttrData
	 * @param[in]	lpAttrType
	 *					The attribute type of the attribtes, can be DN, text or binary
	 * @param[in]	company
	 *					Optional argument, The company where the possible object should belong.
	 * @return The list of object signatures which were found
	 */
	auto_ptr<signatures_t> resolveObjectsFromAttributesType(objectclass_t objclass,
															const list<string> &objects, const char** lppAttr, const char* lpAttrType,
															const objectid_t &company = objectid_t(CONTAINER_COMPANY));

	/**
	 * Determine attribute data for a specific object id
	 *
	 * @param[in]	uniqueid
	 *					The object id which should be converted
	 * @param[in]	lpAttr
	 *					he LDAP attribute which should be read from the object
	 * @return The attribute data from lpAtrr in the DN
	 * @throw runtime_error When the LDAP query failed
	 * @throw objectnotfound When DN does not point to an existing object
	 * @throw toomanyobjects When multiple objects were found
	 * @throw data_error When the requested attribute does not exist on the object of uniqueid
	 */
	string objectUniqueIDtoAttributeData(const objectid_t &uniqueid, const char* lpAttr);

	/**
	 * Determine attribute data for a specific DN
	 *
	 * @param[in]	dn
	 *					The DN which should be converted
	 * @param[in]	lpAttr
	 *					The LDAP attribute which should be read from the DN
	 * @return The attribute data from lpAtrr in the DN
	 * @throw runtime_error When the LDAP query failed
	 * @throw objectnotfound When DN does not point to an existing object
	 * @throw toomanyobjects When multiple objects were found
	 */
	string objectDNtoAttributeData(const string &dn, const char *lpAttr);

	/**
	 * Apply filter to LDAP and request all object signatures
	 * of the objects which were returned by the filter.
	 *
	 * @param[in]	basedn
	 *					The LDAP base from where the filter should be applied
	 * @param[in]	scope
	 *					Search scope (SUB, ONE, BASE)
	 * @param[in]	search_filter
	 *					The LDAP search filter which should be applied
	 * @param[in]	strCompanyDN
	 *					Optional argument, the company to which all returned
	 *					objects should belong. This DN must be the same as basedn.
	 * @param[in]	bCache
	 *					Set to true if this query should update the LDAPCache.
	 * @return The list of object signatures for all found objects
	 * @throw runtime_error When the LDAP query failed
	 */
	auto_ptr<signatures_t> getAllObjectsByFilter(const string &basedn, const int scope, const string &search_filter,
												 const string &strCompanyDN, bool bCache);

	/**
	 * Detecmine object id from LDAP result entry
	 *
	 * This function must be called when the following attributes were requested:
	 * - ldap_object_type_attribute
	 * - ldap_nonactive_attribute
	 * - ldap_resource_type_attribute
	 * - ldap_group_security_attribute
	 * - ldap_group_security_attribute_type
	 * - ldap_user_unique_attribute
	 * - ldap_group_unique_attribute
	 * - ldap_company_unique_attribute
	 * - ldap_addresslist_unique_attribute
	 * - ldap_dynamicgroup_unique_attribute
	 *
	 * This will determine the object ID by first determining the object type from the 
	 * objectClass and possibly other attributes, and then get the object's unique ID 
	 * from the attribute list. 
	 *
	 * @remarks The caller is responsible for making sure all required attributes are 
	 *			available in the LDAP result entry.
	 *
	 * @param[in]	entry
	 *					The LDAPMessage which contains the requested
	 *					attributes from LDAP
	 * @return The Object id.
	 */
	objectid_t GetObjectIdForEntry(LDAPMessage *entry);

	/**
	 * Wrapper function for ldap_search_s which has reconnect features
	 *
	 * @param[in]	base
	 *					The LDAP search base
	 * @param[in]	scope
	 *					Search scope (SUB, ONE, BASE)
	 * @param[in]	filter
	 *					The LDAP search filter
	 * @param[in]	attrs
	 *					The attributes which should be requested from the objects
	 * @param[in]	attrsonly
	 *					Set to 1 to request attribute types only. Set to 0 to request
	 *					both attributes types and attribute values.
	 * @param[out]	lppres
	 *					Contains the result of the synchronous search operation
	 * @throw ldap_error When the LDAP query failed
	 *
	 * @todo return value lppres
	 */
	void my_ldap_search_s(char *base, int scope, char *filter, char *attrs[], int attrsonly, LDAPMessage **lppres, LDAPControl **serverControls = NULL);


	/**
	 * Get list of object classes from object class settings string
	 *
	 * Object classes must be separated by comma (,) and are trimmed for whitespace surrounding the
	 * classes.
	 *
	 * @param[in] 	lpszClasses
	 * 					String from settings with classes separated by comma (,)
	 * @return std::list List of classes converted from settings
	 */
	std::list<std::string> GetClasses(const char *lpszClasses);

	/**
	 * Returns TRUE if all classes in lstClasses are set in setClasses
	 *
	 * Due to case-insensitivity of object classes, the object classes in setClasses must
	 * be provided in UPPER CASE, while the classes in lstClasses need not be.
	 *
	 * @param[in]	setClasses
	 * 					Set of classes to look in (UPPER CASE)
	 * @param[in]	lstClasses
	 * 					Set of classes that must be in setClasses
	 * @return boolean TRUE if all classes in lstClasses are available in setClasses
	 */
	bool MatchClasses(std::set<std::string> setClasses, std::list<std::string> lstClasses);

	/**
	 * Creates an LDAP object class filter for a list of object classes
	 *
	 * Takes the list of object classes passed and converts them into an LDAP
	 * filter that matches entries which have all the passed object classes.
	 *
	 * @param[in]	lpszObjectClassAttr
	 *					Name of the objectClass attribute that should be matched
	 * @param[in]	lpszClasses
	 *					String with classes separated by comma (,) that should be in 
	 *					the filter. The string will be convert in a list. See GetClasses
	 * @return std::string Filter
	 */
	std::string GetObjectClassFilter(const char *lpszObjectClassAttr, const char *lpszClasses);

	long unsigned int ldapServerIndex; // index of the last ldap server to which we could connect
	std::vector<std::string> ldap_servers;
};

extern "C" {
	extern UserPlugin* getUserPluginInstance(pthread_mutex_t*, ECPluginSharedData*);
	extern void deleteUserPluginInstance(UserPlugin*);
	extern int getUserPluginVersion();
}
/** @} */
#endif
