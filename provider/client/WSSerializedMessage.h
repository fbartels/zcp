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

#ifndef WSSerializedMessage_INCLUDED
#define WSSerializedMessage_INCLUDED

#include <zarafa/ECUnknown.h>
#include "soapStub.h"
#include <zarafa/mapi_ptr.h>
#include <string>

/**
 * This object represents one exported message stream. It is responsible for requesting the MTOM attachments from soap.
 */
class WSSerializedMessage : public ECUnknown
{
public:
	WSSerializedMessage(soap *lpSoap, const std::string strStreamId, ULONG cbProps, LPSPropValue lpProps);

	HRESULT GetProps(ULONG *lpcbProps, LPSPropValue *lppProps);
	HRESULT CopyData(LPSTREAM lpDestStream);
	HRESULT DiscardData();

private:
	HRESULT DoCopyData(LPSTREAM lpDestStream);

	static void*	StaticMTOMWriteOpen(struct soap *soap, void *handle, const char *id, const char *type, const char *description, enum soap_mime_encoding encoding);
	static int		StaticMTOMWrite(struct soap *soap, void *handle, const char *buf, size_t len);
	static void		StaticMTOMWriteClose(struct soap *soap, void *handle);

	void*	MTOMWriteOpen(struct soap *soap, void *handle, const char *id, const char *type, const char *description, enum soap_mime_encoding encoding);
	int		MTOMWrite(struct soap *soap, void *handle, const char *buf, size_t len);
	void	MTOMWriteClose(struct soap *soap, void *handle);

private:
	soap				*m_lpSoap;
	const std::string	m_strStreamId;
	ULONG				m_cbProps;
	LPSPropValue		m_lpProps;	//	Points to data from parent object.

	bool				m_bUsed;
	StreamPtr			m_ptrDestStream;
	HRESULT				m_hr;
};

typedef mapi_object_ptr<WSSerializedMessage> WSSerializedMessagePtr;

#endif // ndef WSSerializedMessage_INCLUDED
