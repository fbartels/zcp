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

#include <mapitags.h>
#include <mapidefs.h>
#include <mapicode.h>

#include <list>

#include "Mem.h"
#include "ECNamedProp.h"
#include "WSTransport.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

/*
 * How our named properties work
 *
 * Basically, named properties in objects with the same PR_MAPPING_SIGNATURE should have the same
 * mapping of named properties to property ID's and vice-versa. We can use this information, together
 * with the information that Outlook mainly uses a fixed set of named properties to speed up things
 * considerably;
 *
 * Normally, each GetIDsFromNames calls would have to consult the server for an ID, and then cache
 * and return the value to the client. This is a rather time-consuming thing to do as Outlook requests
 * quite a few different named properties at startup. We can make this much faster by hard-wiring 
 * a bunch of known named properties into the CLIENT-side DLL. This makes sure that most (say 90%) of
 * GetIDsFromNames calls can be handled locally without any reference to the server, while any other
 * (new) named properties can be handled in the standard way. This reduces client-server communications
 * dramatically, resulting in both a faster client as less datacommunication between client and server.
 *
 * In fact, most of the time, the named property mechanism will work even if the server is down...
 */

/*
 * Currently, serverside named properties are cached locally in a map<> object,
 * however, in the future, a bimap<> may be used to speed up reverse lookups (ie
 * getNamesFromIDs) but this is not used frequently so we can leave it like 
 * this for now
 *
 * For the most part, this implementation is rather fast, (possible faster than
 * Exchange) due to the fact that we know which named properties are likely to be
 * requested. This means that we have 
 */


/* Our local names
 *
 * It is VERY IMPORTANT that these values are NOT MODIFIED, otherwise the mapping of
 * named properties will change, which will BREAK THINGS BADLY
 *
 * Special constraints for this structure:
 * - The ulMappedId's must not overlap the previous row
 * - The ulMappedId's must be in ascending order
 */

struct _sLocalNames {
	GUID guid;
	LONG ulMin;
	LONG ulMax;
	ULONG ulMappedId; // mapped ID of the FIRST property in the range
} sLocalNames[] = 	{{{ 0x62002, 0x0, 0x0, { 0xC0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x46 } }, 0x8200, 0x826F, 0x8000 },
					{{ 0x62003, 0x0, 0x0, { 0xC0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x46 } }, 0x8100, 0x813F, 0x8070 },
					{{ 0x62004, 0x0, 0x0, { 0xC0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x46 } }, 0x8000, 0x80EF, 0x80B0 },
					{{ 0x62008, 0x0, 0x0, { 0xC0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x46 } }, 0x8500, 0x85FF, 0x81A0 },
					{{ 0x6200A, 0x0, 0x0, { 0xC0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x46 } }, 0x8700, 0x871F, 0x82A0 },
					{{ 0x6200B, 0x0, 0x0, { 0xC0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x46 } }, 0x8800, 0x881F, 0x82C0 },
					{{ 0x6200E, 0x0, 0x0, { 0xC0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x46 } }, 0x8B00, 0x8B1F, 0x82E0 },
					{{ 0x62013, 0x0, 0x0, { 0xC0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x46 } }, 0x8D00, 0x8D1F, 0x8300 },
					{{ 0x62014, 0x0, 0x0, { 0xC0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x46 } }, 0x8F00, 0x8F1F, 0x8320 },
					{{ 0x6ED8DA90, 0x450B, 0x101B, { 0x98, 0xDA, 0x00, 0xAA, 0x00, 0x3F, 0x13, 0x05} } , 0x0000, 0x003F, 0x8340}};

#define SERVER_NAMED_OFFSET	0x8500

ECNamedProp::ECNamedProp(WSTransport *lpTransport)
{
	this->lpTransport = lpTransport;

	lpTransport->AddRef();
}

ECNamedProp::~ECNamedProp()
{
	std::map<MAPINAMEID *, ULONG, ltmap>::const_iterator iterMap;

	// Clear all the cached names
	for (iterMap = mapNames.begin(); iterMap != mapNames.end(); ++iterMap)
		if(iterMap->first)
			ECFreeBuffer(iterMap->first);
	if(lpTransport)
		lpTransport->Release();
}

HRESULT ECNamedProp::GetNamesFromIDs(LPSPropTagArray FAR * lppPropTags, LPGUID lpPropSetGuid, ULONG ulFlags, ULONG FAR * lpcPropNames, LPMAPINAMEID FAR * FAR * lpppPropNames)
{
	HRESULT			hr = hrSuccess;
	unsigned int	i = 0;
	LPSPropTagArray	lpsPropTags = NULL;
	LPMAPINAMEID*	lppPropNames = NULL;
	LPSPropTagArray	lpsUnresolved = NULL;
	LPMAPINAMEID*	lppResolved = NULL;
	ULONG			cResolved = 0;
	ULONG			cUnresolved = 0;

	// Exchange doesn't support this, so neither do we
	if(lppPropTags == NULL || *lppPropTags == NULL) {
		hr = MAPI_E_TOO_BIG;
		goto exit;
	}

	lpsPropTags = *lppPropTags;

	// Allocate space for properties
	ECAllocateBuffer(sizeof(LPMAPINAMEID) * lpsPropTags->cValues, (void **)&lppPropNames);

	// Pass 1, local reverse mapping (FAST)
	for (i = 0; i < lpsPropTags->cValues; ++i)
		if (ResolveReverseLocal(PROP_ID(lpsPropTags->aulPropTag[i]),
		    lpPropSetGuid, ulFlags, lppPropNames,
		    &lppPropNames[i]) != hrSuccess)
			lppPropNames[i] = NULL;

	// Pass 2, cache reverse mapping (FAST)
	for (i = 0; i < lpsPropTags->cValues; ++i) {
		if(lppPropNames[i] == NULL) {
			if(PROP_ID(lpsPropTags->aulPropTag[i]) > SERVER_NAMED_OFFSET) {
				ResolveReverseCache(PROP_ID(lpsPropTags->aulPropTag[i]), lpPropSetGuid, ulFlags, lppPropNames, &lppPropNames[i]);
			} else {
				// Hmmm, so here is a named property, which is < SERVER_NAMED_OFFSET, but CANNOT be 
				// resolved internally. Looks like somebody's pulling our leg ... We just leave it unknown
			}
		}
	}

	ECAllocateBuffer(CbNewSPropTagArray(lpsPropTags->cValues), (void **)&lpsUnresolved);

	cUnresolved = 0;
	// Pass 3, server reverse lookup (SLOW)
	for (i = 0; i < lpsPropTags->cValues; ++i)
		if (lppPropNames[i] == NULL)
			if(PROP_ID(lpsPropTags->aulPropTag[i]) > SERVER_NAMED_OFFSET) {
				lpsUnresolved->aulPropTag[cUnresolved] = PROP_ID(lpsPropTags->aulPropTag[i]) - SERVER_NAMED_OFFSET;
				++cUnresolved;
			}
	lpsUnresolved->cValues = cUnresolved;

	if(cUnresolved > 0) {
		hr = lpTransport->HrGetNamesFromIDs(lpsUnresolved, &lppResolved, &cResolved);

		if(hr != hrSuccess)
			goto exit;

		// Put the resolved values from the server into the cache
		if(cResolved != cUnresolved) { 
			hr = MAPI_E_CALL_FAILED;
			goto exit;
		}

		for (i = 0; i < cResolved; ++i)
			if(lppResolved[i] != NULL)
				UpdateCache(lpsUnresolved->aulPropTag[i] + SERVER_NAMED_OFFSET, lppResolved[i]);

		// re-scan the cache
		for (i = 0; i < lpsPropTags->cValues; ++i)
			if (lppPropNames[i] == NULL)
				if (PROP_ID(lpsPropTags->aulPropTag[i]) > SERVER_NAMED_OFFSET)
					ResolveReverseCache(PROP_ID(lpsPropTags->aulPropTag[i]), lpPropSetGuid, ulFlags, lppPropNames, &lppPropNames[i]);
	}

	// Check for errors
	for (i = 0; i < lpsPropTags->cValues; ++i)
		if(lppPropNames[i] == NULL)
			hr = MAPI_W_ERRORS_RETURNED;

	*lpppPropNames = lppPropNames;
	*lpcPropNames = lpsPropTags->cValues;
	lppPropNames = NULL;

exit:
	if(lppPropNames)
		ECFreeBuffer(lppPropNames);

	if(lpsUnresolved)
		ECFreeBuffer(lpsUnresolved);

	if(lppResolved)
		ECFreeBuffer(lppResolved);

	return hr;
}

HRESULT ECNamedProp::GetIDsFromNames(ULONG cPropNames, LPMAPINAMEID FAR * lppPropNames, ULONG ulFlags, LPSPropTagArray FAR * lppPropTags)
{
	HRESULT			hr = hrSuccess;
	unsigned int	i=0;
	LPSPropTagArray	lpsPropTagArray = NULL;
	LPMAPINAMEID*	lppPropNamesUnresolved = NULL;
	ULONG			cUnresolved = 0;
	ULONG*			lpServerIDs = NULL;

	// Exchange doesn't support this, so neither do we
	if(cPropNames == 0 || lppPropNames == NULL) {
		hr = MAPI_E_TOO_BIG;
		goto exit;
	}

	// Sanity check input
	for (i = 0; i < cPropNames; ++i) {
		if(lppPropNames[i] == NULL) {
			hr = MAPI_E_INVALID_PARAMETER;
			goto exit;
		}
	}

	// Allocate memory for the return structure
	hr = ECAllocateBuffer(CbNewSPropTagArray(cPropNames), (void **)&lpsPropTagArray);
	if(hr != hrSuccess)
		goto exit;

	lpsPropTagArray->cValues = cPropNames;


	// Pass 1, resolve static (local) names (FAST)
	for (i = 0; i < cPropNames; ++i)
		if(lppPropNames[i] == NULL || ResolveLocal(lppPropNames[i], &lpsPropTagArray->aulPropTag[i]) != hrSuccess)
			lpsPropTagArray->aulPropTag[i] = PROP_TAG(PT_ERROR, 0);

	// Pass 2, resolve names from local cache (FAST)
	for (i = 0; i < cPropNames; ++i)
		if (lppPropNames[i] != NULL && lpsPropTagArray->aulPropTag[i] == PROP_TAG(PT_ERROR, 0))
			ResolveCache(lppPropNames[i], &lpsPropTagArray->aulPropTag[i]);

	// Pass 3, resolve names from server (SLOW, but decreases in frequency with store lifetime)

	lppPropNamesUnresolved = new MAPINAMEID * [lpsPropTagArray->cValues]; // over-allocated

	// Get a list of unresolved names
	for (i = 0; i < cPropNames; ++i)
		if(lpsPropTagArray->aulPropTag[i] == PROP_TAG(PT_ERROR, 0) && lppPropNames[i] != NULL ) {
			lppPropNamesUnresolved[cUnresolved] = lppPropNames[i];
			++cUnresolved;
		}

	if(cUnresolved) {
		// Let the server resolve these names 
		hr = lpTransport->HrGetIDsFromNames(lppPropNamesUnresolved, cUnresolved, ulFlags, &lpServerIDs);

		if(hr != hrSuccess)
			goto exit;

		// Put the names into the local cache for all the IDs the server gave us
		for (i = 0; i < cUnresolved; ++i)
			if(lpServerIDs[i] != 0)
				UpdateCache(lpServerIDs[i] + SERVER_NAMED_OFFSET, lppPropNamesUnresolved[i]);

		// Pass 4, re-resolve from local cache (FAST)
		for (i = 0; i < cPropNames; ++i)
			if (lppPropNames[i] != NULL &&
			    lpsPropTagArray->aulPropTag[i] == PROP_TAG(PT_ERROR, 0))
				ResolveCache(lppPropNames[i], &lpsPropTagArray->aulPropTag[i]);
	}
	
	// Finally, check for any errors left in the returned structure
	hr = hrSuccess;

	for (i = 0; i < cPropNames; ++i)
		if(lpsPropTagArray->aulPropTag[i] == PROP_TAG(PT_ERROR, 0)) {
			hr = MAPI_W_ERRORS_RETURNED;
			break;
		}

	*lppPropTags = lpsPropTagArray;
	lpsPropTagArray = NULL;

exit:
	if(lpsPropTagArray)
		ECFreeBuffer(lpsPropTagArray);

	delete[] lppPropNamesUnresolved;
	if(lpServerIDs)
		ECFreeBuffer(lpServerIDs);

	return hr;
}

HRESULT ECNamedProp::ResolveLocal(MAPINAMEID *lpName, ULONG *ulPropTag)
{
	// We can only locally resolve MNID_ID types of named properties
	if (lpName->ulKind != MNID_ID)
		return MAPI_E_NOT_FOUND;

	// Loop through our local names to see if the named property is in there
	for (size_t i = 0; i < ARRAY_SIZE(sLocalNames); ++i) {
		if(memcmp(&sLocalNames[i].guid,lpName->lpguid,sizeof(GUID))==0 && sLocalNames[i].ulMin <= lpName->Kind.lID && sLocalNames[i].ulMax >= lpName->Kind.lID) {
			// Found it, calculate the ID and return it.
			*ulPropTag = PROP_TAG(PT_UNSPECIFIED, sLocalNames[i].ulMappedId + lpName->Kind.lID - sLocalNames[i].ulMin);
			return hrSuccess;
		}
	}

	// Couldn't find it ...
	return MAPI_E_NOT_FOUND;
}

HRESULT ECNamedProp::ResolveReverseCache(ULONG ulId, LPGUID lpGuid, ULONG ulFlags, void *lpBase, MAPINAMEID **lppName)
{
	HRESULT hr = MAPI_E_NOT_FOUND;
	std::map<MAPINAMEID *, ULONG, ltmap>::const_iterator iterMap;

	// Loop through the map to find the reverse-lookup of the named property. This could be speeded up by
	// used a bimap (bi-directional map)

	for (iterMap = mapNames.begin(); iterMap != mapNames.end(); ++iterMap)
		if(iterMap->second == ulId) { // FIXME match GUID
			if(lpGuid) {
				ASSERT(memcmp(lpGuid, iterMap->first->lpguid, sizeof(GUID)) == 0); // TEST michel
			}
			// found it
			hr = HrCopyNameId(iterMap->first, lppName, lpBase);
			break;
		}

	return hr;
}

HRESULT ECNamedProp::ResolveReverseLocal(ULONG ulId, LPGUID lpGuid, ULONG ulFlags, void *lpBase, MAPINAMEID **lppName)
{
	HRESULT		hr = hrSuccess;
	MAPINAMEID*	lpName = NULL; 

	// Local mapping is only for MNID_ID
	if(ulFlags & MAPI_NO_IDS) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	// Loop through the local names to see if we can reverse-map the id
	for (size_t i = 0; i < ARRAY_SIZE(sLocalNames); ++i) {
		if((lpGuid == NULL || memcmp(&sLocalNames[i].guid, lpGuid, sizeof(GUID)) == 0) && ulId >= sLocalNames[i].ulMappedId && ulId < sLocalNames[i].ulMappedId + (sLocalNames[i].ulMax - sLocalNames[i].ulMin + 1)) {
			// Found it !
			ECAllocateMore(sizeof(MAPINAMEID), lpBase, (void **)&lpName);
			ECAllocateMore(sizeof(GUID), lpBase, (void **)&lpName->lpguid);

			lpName->ulKind = MNID_ID;
			memcpy(lpName->lpguid, &sLocalNames[i].guid, sizeof(GUID));
			lpName->Kind.lID = sLocalNames[i].ulMin + (ulId - sLocalNames[i].ulMappedId);

			break;
		}
	}

	if(lpName)
		*lppName = lpName;
	else
		hr = MAPI_E_NOT_FOUND;
exit:
	return hr;
}


// Update the cache with the given data
HRESULT ECNamedProp::UpdateCache(ULONG ulId, MAPINAMEID *lpName)
{
	HRESULT		hr = hrSuccess;
	MAPINAMEID*	lpNameCopy = NULL;

	if(mapNames.find(lpName) != mapNames.end()) {
		// Already in the cache!
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	hr = HrCopyNameId(lpName, &lpNameCopy, NULL);

	if(hr != hrSuccess)
		goto exit;

	mapNames[lpNameCopy] = ulId;

exit:
	if(hr != hrSuccess && lpNameCopy)
		ECFreeBuffer(lpNameCopy);

	return hr;
}

HRESULT ECNamedProp::ResolveCache(MAPINAMEID *lpName, ULONG *lpulPropTag)
{
	std::map<MAPINAMEID *, ULONG, ltmap>::const_iterator iterMap;

	iterMap = mapNames.find(lpName);

	if (iterMap == mapNames.end())
		return MAPI_E_NOT_FOUND;
	*lpulPropTag = PROP_TAG(PT_UNSPECIFIED, iterMap->second);
	return hrSuccess;
}

/* This copies a MAPINAMEID struct using ECAllocate* functions. Therefore, the
 * memory allocated here is *not* traced by the debug functions. Make sure you
 * release all memory allocated from this function! (or make sure the client application
 * does)
 */
HRESULT ECNamedProp::HrCopyNameId(LPMAPINAMEID lpSrc, LPMAPINAMEID *lppDst, void *lpBase)
{
	HRESULT			hr = hrSuccess;
	LPMAPINAMEID	lpDst = NULL;

	if(lpBase == NULL)
		hr = ECAllocateBuffer(sizeof(MAPINAMEID), (void **) &lpDst);
	else
		hr = ECAllocateMore(sizeof(MAPINAMEID), lpBase, (void **) &lpDst);

	if(hr != hrSuccess)
		goto exit;

	lpDst->ulKind = lpSrc->ulKind;

	if(lpSrc->lpguid) {
		if(lpBase) 
			hr = ECAllocateMore(sizeof(GUID), lpBase, (void **) &lpDst->lpguid);
		else 
			hr = ECAllocateMore(sizeof(GUID), lpDst, (void **) &lpDst->lpguid);

		if(hr != hrSuccess)
			goto exit;

		memcpy(lpDst->lpguid, lpSrc->lpguid, sizeof(GUID));
	} else {
		lpDst->lpguid = NULL;
	}

	switch(lpSrc->ulKind) {
	case MNID_ID:
		lpDst->Kind.lID = lpSrc->Kind.lID;
		break;
	case MNID_STRING:
		if(lpBase)
			ECAllocateMore(wcslen(lpSrc->Kind.lpwstrName) * sizeof(WCHAR) + sizeof(WCHAR), lpBase, (void **)&lpDst->Kind.lpwstrName);
		else
			ECAllocateMore(wcslen(lpSrc->Kind.lpwstrName) * sizeof(WCHAR) + sizeof(WCHAR), lpDst, (void **)&lpDst->Kind.lpwstrName);

		wcscpy(lpDst->Kind.lpwstrName, lpSrc->Kind.lpwstrName);
		break;
	default:
		hr = MAPI_E_INVALID_TYPE;
		goto exit;
	}

	*lppDst = lpDst;

exit:
	if(hr != hrSuccess && !lpBase && lpDst)
		ECFreeBuffer(lpDst);

	return hr;
}
