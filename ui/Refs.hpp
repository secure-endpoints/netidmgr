/*
 * Copyright (c) 2009-2010 Secure Endpoints Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <assert.h>

namespace nim {

class RefCount {
    LONG refcount;
#ifdef DEBUG
    bool ok_to_dispose;
#endif

public:
    RefCount() : refcount(1)
#ifdef DEBUG
               , ok_to_dispose(false)
#endif
    { }

    RefCount(bool initially_held) : refcount((initially_held)? 1 : 0) { }

    virtual ~RefCount() { }

    void Hold() {
        InterlockedIncrement(&refcount);
    }

    void Release() {
        if (InterlockedDecrement(&refcount) == 0) {
            Dispose();
        }
    }

    virtual void Dispose() {
#ifdef DEBUG
        assert (ok_to_dispose);
#endif
        delete this;
    }

    enum RefCountType {
        HoldReference = 0,
        TakeOwnership = 1,
    };

#ifdef DEBUG
    void SetOkToDispose(bool is_ok) {
        ok_to_dispose = is_ok;
    }
#endif

};

template<class T>
class AutoRef {
    T *pT;

public:
    AutoRef(T *_pT, RefCount::RefCountType rtype = RefCount::HoldReference) {
        pT = _pT;
        if (pT && rtype == RefCount::HoldReference)
            pT->Hold();
    }

    AutoRef(const AutoRef<T>& that) {
        pT = that.pT;
        if (pT)
            pT->Hold();
    }

    ~AutoRef() {
        if (pT) {
            pT->Release();
            pT = NULL;
        }
    }

    T& operator * () {
        return *pT;
    }

    T* operator -> () {
        return pT;
    }

    T* operator = (T * _pT) {
        return Attach(_pT);
    }

    bool IsNull() {
        return pT == NULL;
    }

    T* Assign(T * _pT) {
        if (pT)
            pT->Release();
        pT = _pT;

        return pT;
    }

    T* Attach(T * _pT) {
        if (pT)
            pT->Release();
        pT = _pT;
        if (pT)
            pT->Hold();

        return pT;
    }
};

template <class T>
class AutoLock {
    T *pT;
    T *pT_aux;

public:
    AutoLock(T *_pT): pT(_pT), pT_aux(NULL) {
        pT->Lock();
    }

    AutoLock(T *_pT, T *_pT_aux): pT(_pT), pT_aux(_pT_aux) {
        if (pT->LockIndex() < pT_aux->LockIndex()) {
            pT->Lock();
            pT_aux->Lock();
        } else {
            pT_aux->Lock();
            pT->Lock();
        }
    }

    ~AutoLock() {
        pT->Unlock();
        if (pT_aux)
            pT_aux->Unlock();
    }
};

template <>
class AutoLock<CRITICAL_SECTION> {
    CRITICAL_SECTION *cs;

public:
    AutoLock(CRITICAL_SECTION * _cs) : cs(_cs) {
	EnterCriticalSection(cs);
    }

    ~AutoLock() {
	LeaveCriticalSection(cs);
    }
};

}
