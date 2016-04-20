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

#ifndef __M4L_COMMON_IMPL_H
#define __M4L_COMMON_IMPL_H

#include <pthread.h>

class M4LUnknown : public IUnknown {
private:
    ULONG ref;
    pthread_mutex_t mutex;
    
public:
    M4LUnknown();
    virtual ~M4LUnknown();

    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();
    virtual HRESULT __stdcall QueryInterface(REFIID refiid, void **lpvoid);
};

#endif
