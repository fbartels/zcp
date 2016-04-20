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

#ifndef ECSERIALIZER_H
#define ECSERIALIZER_H

#include <zarafa/zcdefs.h>
#include <zarafa/ZarafaCode.h>

class ECFifoBuffer;
class IStream;

#ifdef DEBUG
#define STR_DEF_TIMEOUT 0
#else
#define STR_DEF_TIMEOUT 600000
#endif

class ECSerializer
{
public:
	virtual ~ECSerializer() {};

	virtual ECRESULT SetBuffer(void *lpBuffer) = 0;

	virtual ECRESULT Write(const void *ptr, size_t size, size_t nmemb) = 0;
	virtual ECRESULT Read(void *ptr, size_t size, size_t nmemb) = 0;

	virtual ECRESULT Skip(size_t size, size_t nmemb) = 0;
	virtual ECRESULT Flush() = 0;
	
	virtual ECRESULT Stat(ULONG *lpulRead, ULONG *lpulWritten) = 0;
};

class ECStreamSerializer _zcp_final : public ECSerializer
{
public:
	ECStreamSerializer(IStream *lpBuffer);
	ECRESULT SetBuffer(void *lpBuffer) _zcp_override;
	ECRESULT Write(const void *ptr, size_t size, size_t nmemb) _zcp_override;
	ECRESULT Read(void *ptr, size_t size, size_t nmemb) _zcp_override;
	ECRESULT Skip(size_t size, size_t nmemb) _zcp_override;
	ECRESULT Flush(void) _zcp_override;
	ECRESULT Stat(ULONG *lpulRead, ULONG *lpulWritten) _zcp_override;

private:
	IStream *m_lpBuffer;
	ULONG m_ulRead;
	ULONG m_ulWritten;
};

class ECFifoSerializer _zcp_final : public ECSerializer
{
public:
	enum eMode { serialize, deserialize };

	ECFifoSerializer(ECFifoBuffer *lpBuffer, eMode mode);
	virtual ~ECFifoSerializer(void);
	ECRESULT SetBuffer(void *lpBuffer) _zcp_override;
	ECRESULT Write(const void *ptr, size_t size, size_t nmemb) _zcp_override;
	ECRESULT Read(void *ptr, size_t size, size_t nmemb) _zcp_override;
	ECRESULT Skip(size_t size, size_t nmemb) _zcp_override;
	ECRESULT Flush(void) _zcp_override;
	ECRESULT Stat(ULONG *lpulRead, ULONG *lpulWritten) _zcp_override;

private:
	ECFifoBuffer *m_lpBuffer;
	eMode m_mode;
	ULONG m_ulRead;
	ULONG m_ulWritten;
};

#endif /* ECSERIALIZER_H */
