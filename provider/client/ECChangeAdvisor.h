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

#ifndef ECCHANGEADVISOR_H
#define ECCHANGEADVISOR_H

#include <zarafa/zcdefs.h>
#include <mapidefs.h>
#include <mapispi.h>

#include <pthread.h>	

#include <zarafa/ECUnknown.h>
#include <IECChangeAdvisor.h>
#include "ECICS.h"
#include <zarafa/ZarafaCode.h>

#include <map>

class ECMsgStore;
class ECLogger;

/**
 * ECChangeAdvisor: Implementation IECChangeAdvisor, which allows one to register for 
 *                  change notifications on folders.
 */
class ECChangeAdvisor _zcp_final : public ECUnknown
{
protected:
	/**
	 * Construct the ChangeAdvisor.
	 *
	 * @param[in]	lpMsgStore
	 *					The message store that contains the folder to be registered for
	 *					change notifications.
	 */
	ECChangeAdvisor(ECMsgStore *lpMsgStore);

	/**
	 * Destructor.
	 */
	virtual ~ECChangeAdvisor();

public:
	/**
	 * Construct the ChangeAdvisor.
	 *
	 * @param[in]	lpMsgStore
	 *					The message store that contains the folder to be registered for
	 *					change notifications.
	 * @param[out]	lppChangeAdvisor
	 *					The new change advisor.
	 * @return HRESULT
	 */
	static	HRESULT Create(ECMsgStore *lpMsgStore, ECChangeAdvisor **lppChangeAdvisor);

	/**
	 * Get an alternate interface to the same object.
	 *
	 * @param[in]	refiid
	 *					A reference to the IID of the requested interface.
	 *					Supported interfaces:
	 *						- IID_ECChangeAdvisor
	 *						- IID_IECChangeAdvisor
	 * @param[out]	lpvoid
	 *					Pointer to a pointer of the requested type, casted to a void pointer.
	 */
	virtual HRESULT QueryInterface(REFIID refiid, void **lpvoid);

	// IECChangeAdvisor
	virtual HRESULT GetLastError(HRESULT hResult, ULONG ulFlags, LPMAPIERROR *lppMAPIError);
	virtual HRESULT Config(LPSTREAM lpStream, LPGUID lpGUID, IECChangeAdviseSink *lpAdviseSink, ULONG ulFlags);
	virtual HRESULT UpdateState(LPSTREAM lpStream);
	virtual HRESULT AddKeys(LPENTRYLIST lpEntryList);
	virtual HRESULT RemoveKeys(LPENTRYLIST lpEntryList);
	virtual HRESULT IsMonitoringSyncId(syncid_t ulSyncId);
	virtual HRESULT UpdateSyncState(syncid_t ulSyncId, changeid_t ulChangeId);

private:
	typedef std::map<syncid_t, connection_t> ConnectionMap;
	typedef std::map<syncid_t, changeid_t> SyncStateMap;

	/**
	 * Get the sync id from a ConnectionMap entry.
	 *
	 * @param[in]	sConnection
	 *					The ConnectionMap entry from which to extract the sync id.
	 * @return The sync id extracted from the the ConnectionMap entry.
	 */
	static ULONG					GetSyncId(const ConnectionMap::value_type &sConnection);

	/**
	 * Create a SyncStateMap entry from an SSyncState structure.
	 *
	 * @param[in]	sSyncState
	 *					The SSyncState structure to be converted to a SyncStateMap entry.
	 * @return A new SyncStateMap entry.
	 */
	static SyncStateMap::value_type	ConvertSyncState(const SSyncState &sSyncState);

	static SSyncState				ConvertSyncStateMapEntry(const SyncStateMap::value_type &sMapEntry);

	/**
	 * Compare the sync ids from a ConnectionMap entry and a SyncStateMap entry.
	 *
	 * @param[in]	sConnection
	 *					The ConnectionMap entry to compare the sync id with.
	 * @param[in]	sSyncState
	 *					The SyncStateMap entry to compare the sync id with.
	 * @return true if the sync ids are equal, false otherwise.
	 */
	static bool						CompareSyncId(const ConnectionMap::value_type &sConnection, const SyncStateMap::value_type &sSyncState);

	/**
	 * Reload the change notifications.
	 *
	 * @param[in]	lpParam
	 *					The parameter passed to AddSessionReloadCallback.
	 * @param[in]	newSessionId
	 *					The sessionid of the new session.
	 *
	 * @return HRESULT.
	 */
	static HRESULT					Reload(void *lpParam, ECSESSIONID newSessionId);

	/**
	 * Purge all unused connections from advisor.
	 * @return HRESULT
	 */
	HRESULT							PurgeStates();

	class xECChangeAdvisor _zcp_final : public IECChangeAdvisor {
		// IUnknown
		virtual ULONG __stdcall AddRef(void) _zcp_override;
		virtual ULONG __stdcall Release(void) _zcp_override;
		virtual HRESULT __stdcall QueryInterface(REFIID refiid, void **pInterface) _zcp_override;

		// IECChangeAdvisor
		virtual HRESULT __stdcall GetLastError(HRESULT hResult, ULONG ulFlags, LPMAPIERROR *lppMAPIError) _zcp_override;
		virtual HRESULT __stdcall Config(LPSTREAM lpStream, LPGUID lpGUID, IECChangeAdviseSink *lpAdviseSink, ULONG ulFlags) _zcp_override;
		virtual HRESULT __stdcall UpdateState(LPSTREAM lpStream) _zcp_override;
		virtual HRESULT __stdcall AddKeys(LPENTRYLIST lpEntryList) _zcp_override;
		virtual HRESULT __stdcall RemoveKeys(LPENTRYLIST lpEntryList) _zcp_override;
		virtual HRESULT __stdcall IsMonitoringSyncId(ULONG ulSyncId) _zcp_override;
		virtual HRESULT __stdcall UpdateSyncState(ULONG ulSyncId, ULONG ulChangeId) _zcp_override;
	} m_xECChangeAdvisor;


	ECMsgStore				*m_lpMsgStore;
	IECChangeAdviseSink *m_lpChangeAdviseSink;
	ULONG					m_ulFlags;
	pthread_mutex_t			m_hConnectionLock;
	ConnectionMap			m_mapConnections;
	SyncStateMap			m_mapSyncStates;
	ECLogger				*m_lpLogger;
	ULONG					m_ulReloadId;
};

#endif // ndef ECCHANGEADVISOR_H
