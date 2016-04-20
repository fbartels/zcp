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

#ifndef ECPARENTSTORAGE_H
#define ECPARENTSTORAGE_H

/* This PropStorate class writes the data to the parent object, so this is only used in attachments and msg-in-msg objects
   It reads from the saved data in the parent
*/

#include <zarafa/zcdefs.h>
#include <zarafa/ECUnknown.h>
#include "IECPropStorage.h"

#include "ECGenericProp.h"
#include "WSMAPIPropStorage.h"

#include <zarafa/ZarafaCode.h>
#include "soapZarafaCmdProxy.h"

#include <mapi.h>
#include <mapispi.h>
#include <pthread.h>

class ECParentStorage : public ECUnknown
{
	/*
	  lpParentObject:	The property object of the parent (eg. ECMessage for ECAttach)
	  ulUniqueId:		A unique client-side to find the object in the children list on the parent (PR_ATTACH_NUM (attachments) or PR_ROWID (recipients))
	  ulObjId:			The hierarchy id on the server (0 for a new item)
	  lpServerStorage:	A WSMAPIPropStorage interface which has the communication line to the server
	 */
protected:
	ECParentStorage(ECGenericProp *lpParentObject, ULONG ulUniqueId, ULONG ulObjId, IECPropStorage *lpServerStorage);
	virtual ~ECParentStorage();

public:
	static HRESULT Create(ECGenericProp *lpParentObject, ULONG ulUniqueId, ULONG ulObjId, IECPropStorage *lpServerStorage, ECParentStorage **lppParentStorage);

	virtual HRESULT QueryInterface(REFIID refiid, void **lppInterface);

private:

	// Get a list of the properties
	virtual HRESULT HrReadProps(LPSPropTagArray *lppPropTags, ULONG *cValues, LPSPropValue *ppValues);

	// Get a single (large) property
	virtual HRESULT HrLoadProp(ULONG ulObjId, ULONG ulPropTag, LPSPropValue *lppsPropValue);

	// Not implemented
	virtual	HRESULT	HrWriteProps(ULONG cValues, LPSPropValue pValues, ULONG ulFlags = 0);

	// Not implemented
	virtual HRESULT HrDeleteProps(LPSPropTagArray lpsPropTagArray);

	// Save complete object, deletes/adds/modifies/creates
	virtual HRESULT HrSaveObject(ULONG ulFlags, MAPIOBJECT *lpsMapiObject);

	// Load complete object, deletes/adds/modifies/creates
	virtual HRESULT HrLoadObject(MAPIOBJECT **lppsMapiObject);
	
	// Returns the correct storage which can connect to the server
	virtual IECPropStorage* GetServerStorage();

public:
	class xECPropStorage _zcp_final : public IECPropStorage {
		public:
			// IECUnknown
			virtual ULONG AddRef(void) _zcp_override;
			virtual ULONG Release(void) _zcp_override;
			virtual HRESULT QueryInterface(REFIID refiid, void **lppInterface) _zcp_override;

			// IECPropStorage
			virtual HRESULT HrReadProps(LPSPropTagArray *lppPropTags,ULONG *cValues, LPSPropValue *lppValues);
			virtual HRESULT HrLoadProp(ULONG ulObjId, ULONG ulPropTag, LPSPropValue *lppsPropValue);
			virtual	HRESULT	HrWriteProps(ULONG cValues, LPSPropValue lpValues, ULONG ulFlags = 0);
			virtual HRESULT HrDeleteProps(LPSPropTagArray lpsPropTagArray);
			virtual HRESULT HrSaveObject(ULONG ulFlags, MAPIOBJECT *lpsMapiObject);
			virtual HRESULT HrLoadObject(MAPIOBJECT **lppsMapiObject);
			virtual IECPropStorage* GetServerStorage();
	} m_xECPropStorage;

private:
	ECGenericProp *m_lpParentObject;
	ULONG m_ulObjId;
	ULONG m_ulUniqueId;
	IECPropStorage *m_lpServerStorage;
};


#endif
