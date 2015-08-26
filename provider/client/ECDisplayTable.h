/*
 * Copyright 2005 - 2015  Zarafa B.V. and its licensors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation with the following
 * additional terms according to sec. 7:
 *
 * "Zarafa" is a registered trademark of Zarafa B.V.
 * The licensing of the Program under the AGPL does not imply a trademark
 * license. Therefore any rights, title and interest in our trademarks
 * remain entirely with us.
 *
 * Our trademark policy (see TRADEMARKS.txt) allows you to use our trademarks
 * in connection with Propagation and certain other acts regarding the Program.
 * In any case, if you propagate an unmodified version of the Program you are
 * allowed to use the term "Zarafa" to indicate that you distribute the Program.
 * Furthermore you may use our trademarks where it is necessary to indicate the
 * intended purpose of a product or service provided you use it in accordance
 * with honest business practices. For questions please contact Zarafa at
 * trademark@zarafa.com.
 *
 * The interactive user interface of the software displays an attribution
 * notice containing the term "Zarafa" and/or the logo of Zarafa.
 * Interactive user interfaces of unmodified and modified versions must
 * display Appropriate Legal Notices according to sec. 5 of the GNU Affero
 * General Public License, version 3, when you propagate unmodified or
 * modified versions of the Program. In accordance with sec. 7 b) of the GNU
 * Affero General Public License, version 3, these Appropriate Legal Notices
 * must retain the logo of Zarafa or display the words "Initial Development
 * by Zarafa" if the display of the logo is not reasonably feasible for
 * technical reasons.
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

#pragma once

#include <mapidefs.h>

#include "ECABProp.h"
#include "ECMAPITable.h"

#if defined(_WIN32) && !defined(WINCE)
	#include "dialogdefs.h"
#endif

/**
 * Helper class for creating the displaytable contents in the Outlook Address Book
 */
class ECDisplayTable
{
#if defined(WIN32) && !defined(WINCE)
public:
	/**
	 * Construct the DisplayTable which Outlook can use to build the details window
	 *
	 * @param[in]	ulPages
	 *					The number of pages provided through the lpPages argument
	 * @param[in]	lpPages
	 *					The list of pages which should be added to the details window
	 * @param[out]	lppTable
	 *					The table which Outlook will use to construct the details window
	 * @return HRESULT
	 */
	static HRESULT CreateDisplayTable(ULONG ulPages, DTPAGE *lpPages, IMAPITable **lppTable);

	/**
	 * Dummy function which creates an empty memtable. This can be used to provide Outlook
	 * with *something* rather then completely nothing (which can result in an error message in Outlook).
	 *
	 * @param[in]	lpABProp
	 *					The Address Book object for which the details window will be constructed
	 * @param[in]	lpiid
	 *					The interface which should be queried for the lppUnk argument
	 * @param[in]	ulInterfaceOptions
	 *					The interface flags requested by Outlook (e.g. for enabling UNICODE)
	 * @param[out]	lppUnk
	 *					The empty table with the lpiid interface
	 * @return HRESULT
	 */
	static HRESULT CreateTableEmpty(ECABProp *lpABProp, LPCIID lpiid, ULONG ulInterfaceOptions, LPUNKNOWN *lppUnk);

	/**
	 * Construct table from the contents of a property, this can be used for single and multi valued properties,
	 * the resulting table will contain a single column indicated with the ulColumnTag argument.
	 *
	 * @param[in]	lpABProp
	 *					The Address Book object for which the details window will be constructed
	 * @param[in]	lpiid
	 *					The interface which should be queried for the lppUnk argument
	 * @param[in]	ulInterfaceOptions
	 *					The interface flags requested by Outlook (e.g. for enabling UNICODE)
	 * @param[in]	ulColumnTag
	 *					The property tag which should be used as name for the column
	 * @param[in]	ulPropTag
	 *					The property which should be requested from lpABProp and which result will
	 *					be stored in the table. The type of this object should be PT_TSTRING or
	 *					PT_MV_TSTRING. If ulPropTag is of type PT_TSTRING the function will also
	 *					check the server for the availability of the PT_MV_TSTRING variant.
	 * @param[out]	lppUnk
	 *					The empty table with the lpiid interface
	 * @return HRESULT
	 */
	static HRESULT CreateTableFromProperty(ECABProp *lpABProp, LPCIID lpiid, ULONG ulInterfaceOptions, ULONG ulColumnTag, ULONG ulPropTag, LPUNKNOWN *lppUnk);

	/**
	 * Construct table by opening the contents of a property as AddressBook entries.
	 * This will create a table with the following columns (these are the minimal set of columns as required by Outlook):
	 *
	 * - PR_DISPLAY_NAME
	 * - PR_ENTRYID
	 * - PR_INSTANCE_KEY
	 * - PR_DISPLAY_TYPE
	 * - PR_SMTP_ADDRESS
	 * - PR_ACCOUNT
	 * .
	 *
	 * @param[in]	lpABProp
	 *					The Address Book object for which the details window will be constructed
	 * @param[in]	lpiid
	 *					The interface which should be queried for the lppUnk argument
	 * @param[in]	ulInterfaceOptions
	 *					The interface flags requested by Outlook (e.g. for enabling UNICODE)
	 * @param[in]	ulPropTag
	 *					The property which should be requested from lpABProp. The type of this
	 *					object can be PT_BINARY or PT_MV_BINARY (and must contain the entryid)
	 *					or can be PT_TSTRING or PT_MV_TSTRING (and must contain the name of the
	 *					object which can be resolved through the addressbook).
	 * @param[out]	lppUnk
	 *					The empty table with the lpiid interface
	 * @return HRESULT
	 */
	static HRESULT CreateTableFromResolved(ECABProp *lpABProp, LPCIID lpiid, ULONG ulInterfaceOptions, ULONG ulPropTag, LPUNKNOWN *lppUnk);

private:
	/**
	 * Request a set of properties from one or more entryids
	 *
	 * @param[in]	lpABContainer
	 *					The Address Book container from where OpenEntry() can be called to open the objects
	 * @param[in]	lpEntryId
	 *					The PT_BINARY or PT_MV_BINARY property which contains one or more entryids which should be opened
	 * @param[in]	lpGetProps
	 *					The properties which should be requested from the addressbook entry
	 * @param[out]	lppAddrList
	 *					The list of objects with their properties of all resolved objects. When not all objects could be
	 *					opened this list contains less results then what was requested.
	 * @return HRESULT
	 */
	static HRESULT ResolveFromEntryId(IABContainer *lpABContainer, LPSPropValue lpEntryId, LPSPropTagArray lpGetProps, LPADRLIST *lppAddrList);

	/**
	 * Request a set of properties from one or more names which should be resolved from the addressbook
	 *
	 * @param[in]	lpABContainer
	 *					The Address Book container from where ResolveNames() can be called to resolve the objects
	 * @param[in]	lpName
	 *					The PT_TSTRING or PT_MV_TSTRING property which contains one or more names which should be resolved
	 * @param[in]	lpGetProps
	 *					The properties which should be requested from the addressbook entry
	 * @param[out]	lppAddList
	 *					The list of objects with their properties of all resolved objects. When not all objects could be
	 *					opened this list contains less results then what was requested.
	 * @return HRESULT
	 */
	static HRESULT ResolveFromName(IABContainer *lpABContainer, LPSPropValue lpName, LPSPropTagArray lpGetProps, LPADRLIST *lppAddrList);

	/*
	 * Read property from Address Book entry and determine if the EntryID is provided by the server or the
	 * name should be provided.
	 * 
	 * @param[in]	lpABProp
	 *					The Address Book object for which the details window will be constructed
	 * @param[in]	ulPropTag
	 *					The property which should be read from the Address Book object. This must be of
	 *					the type PT_TSTRING or PT_MV_TSTRING. This function will also check the server
	 *					for the availability of the binary version of this property.
	 * @param[out]	lppAddList
	 *					The list of objects with their properties of all resolved objects.
	 * @return HRESULT
	 */
	static HRESULT ResolveFromProperty(ECABProp *lpABProp, ULONG ulPropTag, LPSPropTagArray lpGetProps, LPADRLIST *lppAddrList);
#endif
};
