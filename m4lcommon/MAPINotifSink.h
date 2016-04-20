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

#ifndef MAPINOTIFSINK_H
#define MAPINOTIFSINK_H

#include <zarafa/zcdefs.h>
#include <list>
#include <mapi.h>
#include <mapix.h>
#include <mapidefs.h>
#include <pthread.h>
#include <zarafa/ECUnknown.h>

class MAPINotifSink _zcp_final : public IMAPIAdviseSink {
public:
    static HRESULT Create(MAPINotifSink **lppSink);
    
    virtual ULONG 	__stdcall 	AddRef(void) _zcp_override;
    virtual ULONG 	__stdcall 	Release(void) _zcp_override;
    
    virtual HRESULT __stdcall	QueryInterface(REFIID iid, void **lpvoid) _zcp_override;
    
    virtual ULONG 	__stdcall 	OnNotify(ULONG cNotif, LPNOTIFICATION lpNotifications) _zcp_override;
    virtual HRESULT __stdcall 	GetNotifications(ULONG *lpcNotif, LPNOTIFICATION *lppNotifications, BOOL fNonBlock, ULONG timeout);

private:
    MAPINotifSink();
    virtual ~MAPINotifSink();

    pthread_mutex_t m_hMutex;
    pthread_cond_t 	m_hCond;
    bool			m_bExit;
    std::list<NOTIFICATION *> m_lstNotifs;
    
    unsigned int	m_cRef;
};


HRESULT MAPICopyUnicode(WCHAR *lpSrc, void *lpBase, WCHAR **lpDst);
HRESULT MAPICopyString(char *lpSrc, void *lpBase, char **lpDst);

#endif

