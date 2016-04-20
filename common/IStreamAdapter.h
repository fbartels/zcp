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

#ifndef ISTREAMADAPTER_H
#define ISTREAMADAPTER_H

#include <zarafa/zcdefs.h>

class IStreamAdapter _zcp_final : public IStream {
public:
    IStreamAdapter(std::string& str);
    ~IStreamAdapter();
    
    virtual HRESULT QueryInterface(REFIID iid, void **pv);
    virtual ULONG AddRef();
    virtual ULONG Release();
    virtual HRESULT Read(void *pv, ULONG cb, ULONG *pcbRead);
    virtual HRESULT Write(const void *pv, ULONG cb, ULONG *pcbWritten);
    virtual HRESULT Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    virtual HRESULT SetSize(ULARGE_INTEGER libNewSize);
    virtual HRESULT CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    virtual HRESULT Commit(DWORD grfCommitFlags);
    virtual HRESULT Revert(void);
    virtual HRESULT LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    virtual HRESULT UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    virtual HRESULT Stat(STATSTG *pstatstg, DWORD grfStatFlag);
    virtual HRESULT Clone(IStream **ppstm);
    
    IStream * get();
private:
    size_t	m_pos;
    std::string& m_str;
};

#endif

