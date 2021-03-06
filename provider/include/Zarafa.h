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

#ifndef ZARAFA_H
#define ZARAFA_H

#include <mapi.h>
#include <zarafa/ECTags.h>
#include <zarafa/Trace.h>

// We have 2 types of entryids: those of objects, and those of stores.
// Objects have a store-relative path, however they do have a GUID to make
// sure that we can differentiate 2 entryid's from different stores
//
// The ulType fields makes sure that we can read whether it is a store EID
// or an object EID. This is used when opening a store's properties.

// This is our EntryID struct. The first abFlags[] array is required
// by MAPI, other fields in the struct can be anything you like

// Version is our internal version so we can differentiate them in later
// versions

// The ulId field is simply the ID of the record in the database hierarchy
// table. For stores, this is the ID of the msgstore record in the database
// (also the hierarchy table). Each record in on the server has a different
// ulId, even across different stores.
//
// We can differentiate two EntryID's of 2 different servers with the same
// ulId because the guid is different. (This guid is different per server)
//
// When this is a store EID, the szServer field is also set, and the ulId
// points to the top-level object for the store. The other fields are the same.


#pragma pack(push,1)
// Entryid from version 6
// Entryid version 1
typedef struct EID {
	BYTE	abFlags[4];
	GUID	guid;			// StoreGuid
	ULONG	ulVersion;
	USHORT	usType;
	USHORT  usFlags;		// Before Zarafa 7.1, ulFlags did not exist, and ulType was ULONG
	GUID	uniqueId;		// Unique id
	CHAR	szServer[1];
	CHAR	szPadding[3];

	EID(USHORT usType, GUID guid, GUID uniqueId, USHORT usFlags = 0) {
		memset(this, 0, sizeof(EID));
		this->ulVersion = 1; //Always last version
		this->usType = usType;
		this->usFlags = usFlags;
		this->guid = guid;
		this->uniqueId = uniqueId;
	}

	EID(EID *oldEID) {
		memset(this, 0, sizeof(EID));
		this->ulVersion = oldEID->ulVersion;
		this->usType = oldEID->usType;
		this->guid = oldEID->guid;
		this->uniqueId = oldEID->uniqueId;
	}

	EID() {
		memset(this, 0, sizeof(EID));
		this->ulVersion = 1;
	}

} EID;


// The entryid from the begin of zarafa till 5.20
// Entryid version is zero
typedef struct EID_V0 {
	BYTE	abFlags[4];
	GUID	guid;			// StoreGuid
	ULONG	ulVersion;
	USHORT	usType;
	USHORT  usFlags;		// Before Zarafa 7.1, ulFlags did not exist, and ulType was ULONG
	ULONG	ulId;
	CHAR	szServer[1];
	CHAR	szPadding[3];

	EID_V0(USHORT usType, GUID guid, ULONG ulId) {
		memset(this, 0, sizeof(EID_V0));
		this->usType = usType;
		this->guid = guid;
		this->ulId = ulId;
	}

	EID_V0(EID_V0 *oldEID) {
		memset(this, 0, sizeof(EID_V0));
		this->usType = oldEID->usType;
		this->guid = oldEID->guid;
		this->ulId = oldEID->ulId;
	}

	EID_V0() {
		memset(this, 0, sizeof(EID_V0));
	}

} EID_V0;

#pragma pack(pop)

typedef struct ABEID {
	BYTE	abFlags[4];
	GUID	guid;
	ULONG	ulVersion;
	ULONG	ulType;
	ULONG	ulId;
	CHAR	szExId[1];
	CHAR	szPadding[3];

	ABEID(ULONG ulType, GUID guid, ULONG ulId) {
		memset(this, 0, sizeof(ABEID));
		this->ulType = ulType;
		this->guid = guid;
		this->ulId = ulId;
	}

	ABEID(ABEID *oldEID) {
		memset(this, 0, sizeof(ABEID));
		this->ulType = oldEID->ulType;
		this->guid = oldEID->guid;
		this->ulId = oldEID->ulId;
	}

	ABEID() {
		memset(this, 0, sizeof(ABEID));
	}

} ABEID, *PABEID;
#define _CbABEID(p)	((sizeof(ABEID)+strlen((char*)(p)->szExId))&~3)
#define CbABEID(p)	(sizeof(ABEID)>_CbABEID((p))?sizeof(ABEID):_CbABEID((p)))

#define _CbNewABEID(p) 	((sizeof(ABEID)+strlen((char*)(p)))&~3)
#define CbNewABEID(p)	(sizeof(ABEID)>_CbNewABEID((p))?sizeof(ABEID):_CbNewABEID((p)))

#define ABEID_TYPE(p)	((p) ? ((PABEID)p)->ulType : -1)
#define ABEID_ID(p)		((p) ? ((PABEID)p)->ulId : 0)

typedef struct SIEID {
	BYTE	abFlags[4];
	GUID	guid;
	ULONG	ulVersion;
	ULONG	ulType;
	ULONG	ulId;
	CHAR	szServerId[1];
	CHAR	szPadding[3];

	SIEID(ULONG ulType, GUID guid, ULONG ulId) {
		memset(this, 0, sizeof(SIEID));
		this->ulType = ulType;
		this->guid = guid;
		this->ulId = ulId;
	}

	SIEID(SIEID *oldEID) {
		memset(this, 0, sizeof(SIEID));
		this->ulType = oldEID->ulType;
		this->guid = oldEID->guid;
		this->ulId = oldEID->ulId;
	}

	SIEID() {
		memset(this, 0, sizeof(SIEID));
	}
} SIEID, *LPSIEID;

/* Bit definitions for abFlags[3] of ENTRYID */
#define	ZARAFA_FAVORITE		0x01		// Entryid from the favorits folder

// Indexes of the identity property array
enum
{
    XPID_NAME,              // Array Indexes
    XPID_EID,
    XPID_SEARCH_KEY,
	XPID_STORE_EID,
	XPID_ADDRESS,
	XPID_ADDRTYPE,
    NUM_IDENTITY_PROPS      // Array size
};

#define TRANSPORT_ADDRESS_TYPE_SMTP		_T("SMTP")
#define TRANSPORT_ADDRESS_TYPE_ZARAFA	_T("ZARAFA")
#define TRANSPORT_ADDRESS_TYPE_FAX		_T("FAX")

typedef EID * PEID;

#define CbEID(p)	(sizeof(EID)+strlen((char*)(p)->szServer))
#define CbNewEID(p) 	(sizeof(EID)+strlen(((char*)p)))

#define EID_TYPE_STORE		1
#define EID_TYPE_OBJECT		2

#ifndef STORE_HTML_OK
#define STORE_HTML_OK	0x00010000
#endif

//The message store supports properties containing ANSI (8-bit) characters
#ifndef STORE_ANSI_OK
#define STORE_ANSI_OK	0x00020000
#endif

//This flag is reserved and should not be used
#ifndef STORE_LOCALSTORE
#define STORE_LOCALSTORE	0x00080000
#endif

#ifndef STORE_UNICODE_OK
#define STORE_UNICODE_OK	0x00040000L
#endif

#ifndef STORE_PUSHER_OK
#define STORE_PUSHER_OK ((ULONG) 0x00800000)
#endif

// This is what we support for private store
#define EC_SUPPORTMASK_PRIVATE \
							STORE_ENTRYID_UNIQUE | \
							STORE_SEARCH_OK | \
							STORE_MODIFY_OK | \
							STORE_CREATE_OK | \
							STORE_ATTACH_OK | \
							STORE_OLE_OK | \
							STORE_NOTIFY_OK | \
							STORE_MV_PROPS_OK | \
							STORE_CATEGORIZE_OK | \
							STORE_RTF_OK | \
							STORE_RESTRICTION_OK | \
							STORE_SORT_OK | \
							STORE_HTML_OK | \
							STORE_UNICODE_OK | \
							STORE_SUBMIT_OK

// This is what we support for archive store
#define EC_SUPPORTMASK_ARCHIVE \
							STORE_ENTRYID_UNIQUE | \
							STORE_MODIFY_OK | \
							STORE_CREATE_OK | \
							STORE_ATTACH_OK | \
							STORE_OLE_OK | \
							STORE_NOTIFY_OK | \
							STORE_MV_PROPS_OK | \
							STORE_CATEGORIZE_OK | \
							STORE_RTF_OK | \
							STORE_RESTRICTION_OK | \
							STORE_SORT_OK | \
							STORE_HTML_OK | \
							STORE_UNICODE_OK

// This is what we support for delegate store
#define EC_SUPPORTMASK_DELEGATE \
							STORE_ENTRYID_UNIQUE | \
							STORE_MODIFY_OK | \
							STORE_CREATE_OK | \
							STORE_ATTACH_OK | \
							STORE_OLE_OK | \
							STORE_NOTIFY_OK | \
							STORE_MV_PROPS_OK | \
							STORE_CATEGORIZE_OK | \
							STORE_RTF_OK | \
							STORE_RESTRICTION_OK | \
							STORE_SORT_OK | \
							STORE_HTML_OK | \
							STORE_UNICODE_OK | \
							STORE_SUBMIT_OK

// This is what we support for public store
#define EC_SUPPORTMASK_PUBLIC \
							STORE_ENTRYID_UNIQUE | \
							STORE_MODIFY_OK | \
							STORE_CREATE_OK | \
							STORE_ATTACH_OK | \
							STORE_OLE_OK | \
							STORE_NOTIFY_OK | \
							STORE_MV_PROPS_OK | \
							STORE_CATEGORIZE_OK | \
							STORE_RTF_OK | \
							STORE_RESTRICTION_OK | \
							STORE_SORT_OK | \
							STORE_HTML_OK | \
							STORE_UNICODE_OK | \
							STORE_PUBLIC_FOLDERS


// This is the DLL name given to WrapStoreEntryID so MAPI can
// figure out which DLL to open for a given EntryID

// Note that the '32' is added by MAPI
#define ZARAFA_DLL_NAME "zarafa6client.dll"

// Default freebusy publish months
#define ECFREEBUSY_DEFAULT_PUBLISH_MONTHS		6

#define TABLE_CAP_STRING	255
#define TABLE_CAP_BINARY	511

//
// Capabilities bitmask, sent with ns__logon()
//
// test SOAP flag
#ifdef WITH_GZIP
#define ZARAFA_CAP_COMPRESSION			0x0001
#else
// no compression in soap
#define ZARAFA_CAP_COMPRESSION			0x0000
#endif
// Client has PR_MAILBOX_OWNER_* properties
#define ZARAFA_CAP_MAILBOX_OWNER		0x0002
// Server sends Mod. time and Create time in readProps() call
//#define ZARAFA_CAP_TIMES_IN_READPROPS	0x0004 //not needed since saveObject is introduced
#define ZARAFA_CAP_CRYPT				0x0008
// 64 bit session id's
#define ZARAFA_CAP_LARGE_SESSIONID		0x0010
// Includes license server
#define ZARAFA_CAP_LICENSE_SERVER		0x0020
// Client/Server understands ABEID (longer than 36 bytes)
#define ZARAFA_CAP_MULTI_SERVER				0x0040
// Server supports enhanced ICS operations
#define ZARAFA_CAP_ENHANCED_ICS				0x0100
// Support 'entryid' field in loadProp
#define ZARAFA_CAP_LOADPROP_ENTRYID		0x0080
// Client supports unicode
#define ZARAFA_CAP_UNICODE				0x0200
// Server side message locking
#define ZARAFA_CAP_MSGLOCK				0x0400
// ExportMessageChangeAsStream supports ulPropTag parameter
#define ZARAFA_CAP_EXPORT_PROPTAG		0x0800
// Support impersonation
#define ZARAFA_CAP_IMPERSONATION        0x1000
// Client stores the max ab changeid obtained from the server
#define ZARAFA_CAP_MAX_ABCHANGEID		0x2000
// Client can read and write binary anonymous ab properties
#define ZARAFA_CAP_EXTENDED_ANON		0x4000

// Do *not* use this from a client. This is just what the latest server supports.
#define ZARAFA_LATEST_CAPABILITIES		ZARAFA_CAP_CRYPT | ZARAFA_CAP_LICENSE_SERVER | ZARAFA_CAP_LOADPROP_ENTRYID | ZARAFA_CAP_EXPORT_PROPTAG | ZARAFA_CAP_IMPERSONATION

//
// Logon flags, sent with ns__logon()
//
// Don't allow uid based authentication (unix socket only)
#define ZARAFA_LOGON_NO_UID_AUTH		0x0001

// MTOM IDs
#define MTOM_ID_EXPORTMESSAGES			"idExportMessages"

#endif
