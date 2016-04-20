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
#include <zarafa/ECABEntryID.h>
#include <zarafa/ECGuid.h>

#include <mapicode.h>

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/* This is a copy from the definition in Zarafa.h. It's for internal use only as we
 * don't want to expose the format of the entry id. */
typedef struct ABEID {
	BYTE	abFlags[4];
	GUID	guid;
	ULONG	ulVersion;
	ULONG	ulType;
	ULONG	ulId;
	char	szExId[1];
	char	szPadding[3];

	ABEID(ULONG ulType, GUID guid, ULONG ulId) {
		memset(this, 0, sizeof(ABEID));
		this->ulType = ulType;
		this->guid = guid;
		this->ulId = ulId;
	}
} ABEID, *PABEID;

static ABEID		g_sDefaultEid(MAPI_MAILUSER, MUIDECSAB, 0);
unsigned char		*g_lpDefaultEid = (unsigned char*)&g_sDefaultEid;
const unsigned int	g_cbDefaultEid = sizeof(g_sDefaultEid);

static ABEID		g_sEveryOneEid(MAPI_DISTLIST, MUIDECSAB, 1);
unsigned char		*g_lpEveryoneEid = (unsigned char*)&g_sEveryOneEid;
const unsigned int	g_cbEveryoneEid = sizeof(g_sEveryOneEid);

static ABEID		g_sSystemEid(MAPI_MAILUSER, MUIDECSAB, 2);
unsigned char		*g_lpSystemEid = (unsigned char*)&g_sSystemEid;
const unsigned int	g_cbSystemEid = sizeof(g_sSystemEid);

static HRESULT CheckEntryId(unsigned int cbEntryId, const ENTRYID *lpEntryId,
    unsigned int ulId, unsigned int ulType, bool *lpbResult)
{
	bool	bResult = true;
	PABEID	lpEid = NULL;

	if (cbEntryId < sizeof(ABEID) || lpEntryId == NULL || lpbResult == NULL)
		return MAPI_E_INVALID_PARAMETER;

	lpEid = (PABEID)lpEntryId;
	if (lpEid->ulId != ulId)
		bResult = false;
		
	else if (lpEid->ulType != ulType)
		bResult = false;

	else if (lpEid->ulVersion == 1 && lpEid->szExId[0])
		bResult = false;

	*lpbResult = bResult;
	return hrSuccess;
}

HRESULT EntryIdIsDefault(unsigned int cbEntryId, const ENTRYID *lpEntryId,
    bool *lpbResult)
{
	return CheckEntryId(cbEntryId, lpEntryId, 0, MAPI_MAILUSER, lpbResult);
}

HRESULT EntryIdIsSystem(unsigned int cbEntryId, const ENTRYID *lpEntryId,
    bool *lpbResult)
{
	return CheckEntryId(cbEntryId, lpEntryId, 2, MAPI_MAILUSER, lpbResult);
}

HRESULT EntryIdIsEveryone(unsigned int cbEntryId, const ENTRYID *lpEntryId,
    bool *lpbResult)
{
	return CheckEntryId(cbEntryId, lpEntryId, 1, MAPI_DISTLIST, lpbResult);
}

HRESULT GetNonPortableObjectId(unsigned int cbEntryId,
    const ENTRYID *lpEntryId, unsigned int *lpulObjectId)
{
	if (cbEntryId < sizeof(ABEID) || lpEntryId == NULL || lpulObjectId == NULL)
		return MAPI_E_INVALID_PARAMETER;
	*lpulObjectId = ((PABEID)lpEntryId)->ulId;
	return hrSuccess;
}

HRESULT GetNonPortableObjectType(unsigned int cbEntryId,
    const ENTRYID *lpEntryId, ULONG *lpulObjectType)
{
	if (cbEntryId < sizeof(ABEID) || lpEntryId == NULL || lpulObjectType == NULL)
		return MAPI_E_INVALID_PARAMETER;
	*lpulObjectType = ((PABEID)lpEntryId)->ulType;
	return hrSuccess;
}

HRESULT GeneralizeEntryIdInPlace(unsigned int cbEntryId,
    const ENTRYID *lpEntryId)
{
	PABEID	lpAbeid = NULL;

	if (cbEntryId < sizeof(ABEID) || lpEntryId == NULL)
		return MAPI_E_INVALID_PARAMETER;

	lpAbeid = (PABEID)lpEntryId;
	switch (lpAbeid->ulVersion) {
		// A version 0 entry id is generalized by nature as it's not used to be shared
		// between servers.
		case 0:
			break;

		// A version 1 entry id can be understood by all servers in a cluster, but they
		// cannot be compared with memcpy because the ulId field is server specific.
		// However we can zero this field as the server doesn't need it to locate the
		// object referenced with the entry id.
		// An exception on this rule are SYSTEM en EVERYONE as they don't have an external
		// id. However their ulId fields are specified as 1 and 2 respectively. So every
		// server will understand the entry id, regardless of the version number. We will
		// downgrade anyway be as compatible as possible in that case.
		case 1:
			if (lpAbeid->szExId[0])	// remove the 'legacy ulId field'
				lpAbeid->ulId = 0;			
			else {								// downgrade to version 0
				ASSERT(cbEntryId == sizeof(ABEID));
				lpAbeid->ulVersion = 0;
			}
			break;

		default:
			ASSERT(FALSE);
			break;
	}
	return hrSuccess;
}
