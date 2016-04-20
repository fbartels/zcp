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

#ifndef WSMessageStreamExporter_INCLUDED
#define WSMessageStreamExporter_INCLUDED

#include <zarafa/ECUnknown.h>
#include "soapStub.h"
#include <zarafa/mapi_ptr.h>

#include <string>
#include <map>

class WSTransport;
class WSSerializedMessage;

/**
 * This object encapsulates an set of exported streams. It allows the user to request each individual stream. The
 * streams must be requested in the correct sequence.
 */
class WSMessageStreamExporter : public ECUnknown
{
public:
	static HRESULT Create(ULONG ulOffset, ULONG ulCount, const messageStreamArray &streams, WSTransport *lpTransport, WSMessageStreamExporter **lppStreamExporter);

	bool IsDone() const;
	HRESULT GetSerializedMessage(ULONG ulIndex, WSSerializedMessage **lppSerializedMessage);

private:
	WSMessageStreamExporter();
	~WSMessageStreamExporter();

	// Inhibit copying
	WSMessageStreamExporter(const WSMessageStreamExporter&);
	WSMessageStreamExporter& operator=(const WSMessageStreamExporter&);

private:
	typedef mapi_object_ptr<WSTransport> WSTransportPtr;

	struct StreamInfo {
		std::string	id;
		unsigned long	cbPropVals;
		SPropArrayPtr	ptrPropVals;
	};
	typedef std::map<ULONG, StreamInfo*>	StreamInfoMap;


	ULONG			m_ulExpectedIndex;
	ULONG			m_ulMaxIndex;
	WSTransportPtr	m_ptrTransport;
	StreamInfoMap	m_mapStreamInfo;
};

typedef mapi_object_ptr<WSMessageStreamExporter> WSMessageStreamExporterPtr;

#endif // ndef ECMessageStreamExporter_INCLUDED
