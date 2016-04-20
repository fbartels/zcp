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

#ifndef WSMessageStreamImporter_INCLUDED
#define WSMessageStreamImporter_INCLUDED

#include <zarafa/zcdefs.h>
#include <zarafa/ECUnknown.h>
#include <zarafa/mapi_ptr.h>
#include "soapStub.h"
#include "ECFifoBuffer.h"
#include <zarafa/ECThreadPool.h>
#include "WSTransport.h"

class ECMAPIFolder;
typedef mapi_object_ptr<WSTransport> WSTransportPtr;

class WSMessageStreamImporter;

/**
 * This class represents the data sink into which the stream data can be written.
 * It is returned from WSMessageStreamImporter::StartTransfer.
 */
class WSMessageStreamSink _zcp_final : public ECUnknown
{
public:
	static HRESULT Create(ECFifoBuffer *lpFifoBuffer, ULONG ulTimeout, WSMessageStreamImporter *lpImporter, WSMessageStreamSink **lppSink);
	HRESULT Write(LPVOID lpData, ULONG cbData);

private:
	WSMessageStreamSink(ECFifoBuffer *lpFifoBuffer, ULONG ulTimeout, WSMessageStreamImporter *lpImporter);
	~WSMessageStreamSink();

private:
	ECFifoBuffer	*m_lpFifoBuffer;
	WSMessageStreamImporter *m_lpImporter;
};

typedef mapi_object_ptr<WSMessageStreamSink> WSMessageStreamSinkPtr;


/**
 * This class is used to perform an message stream import to the server.
 * The actual import call to the server is deferred until StartTransfer is called. When that
 * happens, the actual transfer is done on a worker thread so the calling thread can start writing
 * data in the returned WSMessageStreamSink. Once the returned stream is deleted, GetAsyncResult can
 * be used to wait for the worker and obtain its return values.
 */
class WSMessageStreamImporter _zcp_final : public ECUnknown, private ECWaitableTask
{
public:
	static HRESULT Create(ULONG ulFlags, ULONG ulSyncId, ULONG cbEntryID, LPENTRYID lpEntryID, ULONG cbFolderEntryID, LPENTRYID lpFolderEntryID, bool bNewMessage, LPSPropValue lpConflictItems, WSTransport *lpTransport, WSMessageStreamImporter **lppStreamImporter);

	HRESULT StartTransfer(WSMessageStreamSink **lppSink);
	HRESULT GetAsyncResult(HRESULT *lphrResult);

private:
	WSMessageStreamImporter(ULONG ulFlags, ULONG ulSyncId, const entryId &sEntryId, const entryId &sFolderEntryId, bool bNewMessage, const propVal &sConflictItems, WSTransport *lpTransport, ULONG ulBufferSize, ULONG ulTimeout);
	~WSMessageStreamImporter();

	void run();

	static void  *StaticMTOMReadOpen(struct soap *soap, void *handle, const char *id, const char *type, const char *description);
	static size_t StaticMTOMRead(struct soap *soap, void *handle, char *buf, size_t len);
	static void   StaticMTOMReadClose(struct soap *soap, void *handle);

	void  *MTOMReadOpen(struct soap *soap, void *handle, const char *id, const char *type, const char *description);
	size_t MTOMRead(struct soap *soap, void *handle, char *buf, size_t len);
	void   MTOMReadClose(struct soap *soap, void *handle);


private:
	ULONG m_ulFlags;
	ULONG m_ulSyncId;
	entryId m_sEntryId;
	entryId m_sFolderEntryId;
	bool m_bNewMessage;
	propVal m_sConflictItems;
	WSTransportPtr m_ptrTransport;

	HRESULT m_hr;
	ECFifoBuffer m_fifoBuffer;
	ECThreadPool m_threadPool;
	ULONG m_ulTimeout;
};

typedef mapi_object_ptr<WSMessageStreamImporter> WSMessageStreamImporterPtr;

#endif // ndef WSMessageStreamImporter_INCLUDED
