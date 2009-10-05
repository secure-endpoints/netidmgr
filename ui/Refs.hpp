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
            if (pT)
                pT->Release();
            pT = _pT;
            if (pT)
                pT->Hold();

            return pT;
        }

        bool IsNull() {
            return pT == NULL;
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
}
