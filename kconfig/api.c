/*
* Copyright (c) 2005 Massachusetts Institute of Technology
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

/* $Id$ */

#include<shlwapi.h>
#include "kconfiginternal.h"
#include<netidmgr_intver.h>
#include<assert.h>

kconf_conf_space * conf_root = NULL;
kconf_handle * conf_handles = NULL;
kconf_handle * conf_free_handles = NULL;

CRITICAL_SECTION cs_conf_global;
CRITICAL_SECTION cs_conf_handle;

DECLARE_ONCE(conf_once);
LONG volatile conf_status = 0;


void init_kconf(void)
{
    if(InitializeOnce(&conf_once)) {
        /* we are the first */
        InitializeCriticalSection(&cs_conf_global);

        EnterCriticalSection(&cs_conf_global);
        conf_root = khcint_create_empty_space();
        conf_root->name = PWCSDUP(L"Root");
        conf_root->regpath = PWCSDUP(CONFIG_REGPATHW);
        conf_root->refcount++;
        conf_status = 1;
        InitializeCriticalSection(&cs_conf_handle);
        LeaveCriticalSection(&cs_conf_global);

        InitializeOnceDone(&conf_once);
    }
}

void exit_kconf(void)
{
    if(khc_is_config_running()) {
        kconf_handle * h;

        EnterCriticalSection(&cs_conf_global);

        conf_status = 0;

        khcint_free_space(conf_root);

        LeaveCriticalSection(&cs_conf_global);
        DeleteCriticalSection(&cs_conf_global);

        EnterCriticalSection(&cs_conf_handle);
        while(conf_free_handles) {
            LPOP(&conf_free_handles, &h);
            if(h) {
                PFREE(h);
            }
        }

        while(conf_handles) {
            LPOP(&conf_handles, &h);
            if(h) {
                PFREE(h);
            }
        }
        LeaveCriticalSection(&cs_conf_handle);
        DeleteCriticalSection(&cs_conf_handle);
    }
}

#if defined(DEBUG) && (defined(KH_BUILD_PRIVATE) || defined(KH_BUILD_SPECIAL))

#include<stdio.h>

static void
khcint_dump_space(FILE * f, kconf_conf_space * sp)
{

    kconf_conf_space * sc;

    fprintf(f, "c12\t[%S]\t[%S]\t%d\t0x%x\tWin(%s|%s)|%s\n",
            ((sp->regpath) ? sp->regpath : L"!No Reg path"),
            sp->name,
            (int) sp->refcount,
            (int) sp->flags,
            ((sp->regkey_user)? "HKCU" : ""),
            ((sp->regkey_machine)? "HKLM" : ""),
            ((sp->schema)? "Schema" : ""));


    sc = TFIRSTCHILD(sp);
    while(sc) {

        khcint_dump_space(f, sc);

        sc = LNEXT(sc);
    }
}

KHMEXP void KHMAPI
khcint_dump_handles(FILE * f)
{
    if (khc_is_config_running()) {
        kconf_handle * h, * sh;

        EnterCriticalSection(&cs_conf_handle);
        EnterCriticalSection(&cs_conf_global);

        fprintf(f, "c00\t*** Active handles ***\n");
        fprintf(f, "c01\tHandle\tName\tFlags\tRegpath\n");

        h = conf_handles;
        while(h) {
            kconf_conf_space * sp;

            sp = h->space;

            if (!khc_is_handle(h) || sp == NULL) {

                fprintf(f, "c02\t!!INVALID HANDLE!!\n");

            } else {

                fprintf(f, "c02\t0x%p\t[%S]\t0x%x\t[%S]\n",
                        h,
                        sp->name,
                        h->flags,
                        sp->regpath);

                sh = khc_shadow(h);

                while(sh) {

                    sp = sh->space;

                    if (!khc_is_handle(sh) || sp == NULL) {

                        fprintf(f, "c02\t0x%p:Shadow:0x%p\t[!!INVALID HANDLE!!]\n",
                                h, sh);

                    } else {

                        fprintf(f, "c02\t0x%p:Shadow:0x%p,[%S]\t0x%x\t[%S]\n",
                                h, sh,
                                sp->name,
                                sh->flags,
                                sp->regpath);

                    }

                    sh = khc_shadow(sh);
                }

            }

            h = LNEXT(h);
        }

        fprintf(f, "c03\t------  End ---------\n");

        fprintf(f, "c10\t*** Active Configuration Spaces ***\n");
        fprintf(f, "c11\tReg path\tName\tRefcount\tFlags\tLayers\n");

        khcint_dump_space(f, conf_root);

        fprintf(f, "c13\t------  End ---------\n");

        LeaveCriticalSection(&cs_conf_global);
        LeaveCriticalSection(&cs_conf_handle);

    } else {
        fprintf(f, "c00\t------- KHC Configuration not running -------\n");
    }
}

#endif

/* obtains cs_conf_handle/cs_conf_global */
kconf_handle * 
khcint_handle_from_space(kconf_conf_space * s, khm_int32 flags)
{
    kconf_handle * h;

    EnterCriticalSection(&cs_conf_handle);
    LPOP(&conf_free_handles, &h);
    if(!h) {
        h = PMALLOC(sizeof(kconf_handle));
        assert(h != NULL);
    }
    ZeroMemory((void *) h, sizeof(kconf_handle));

    h->magic = KCONF_HANDLE_MAGIC;
    khcint_space_hold(s);
    h->space = s;
    h->flags = flags;

    LPUSH(&conf_handles, h);
    LeaveCriticalSection(&cs_conf_handle);

    return h;
}

/* obtains cs_conf_handle/cs_conf_global */
void 
khcint_handle_free(kconf_handle * h)
{
    kconf_handle * lower;

    EnterCriticalSection(&cs_conf_handle);
#ifdef DEBUG
    /* check if the handle is actually in use */
    {
        kconf_handle * a;
        a = conf_handles;
        while(a) {
            if(h == a)
                break;
            a = LNEXT(a);
        }

        if(a == NULL) {
            DebugBreak();

            /* hmm.  the handle was not in the in-use list */
            LeaveCriticalSection(&cs_conf_handle);
            return;
        }
    }
#endif
    while(h) {
        LDELETE(&conf_handles, h);
        if(h->space) {
            khcint_space_release(h->space);
            h->space = NULL;
        }
        lower = h->lower;
        h->magic = 0;
        LPUSH(&conf_free_handles, h);
        h = lower;
    }
    LeaveCriticalSection(&cs_conf_handle);
}

/* obains cs_conf_handle/cs_conf_global */
kconf_handle * 
khcint_handle_dup(kconf_handle * o)
{
    kconf_handle * h;
    kconf_handle * r;

    r = khcint_handle_from_space(o->space, o->flags);
    h = r;

    while(o->lower) {
        h->lower = khcint_handle_from_space(o->lower->space, o->lower->flags);

        o = o->lower;
        h = h->lower;
    }

    return r;
}

/* called with cs_conf_global */
void
khcint_try_free_space(kconf_conf_space * s)
{
    if (TFIRSTCHILD(s) == NULL &&
        s->refcount == 0 &&
        s->schema == NULL) {

        kconf_conf_space * p;

        p = TPARENT(s);

        if (p == NULL)
            return;

        TDELCHILD(p, s);

        khcint_free_space(s);

        khcint_try_free_space(p);
    }
}

/* obtains cs_conf_global */
void 
khcint_space_hold(kconf_conf_space * s)
{
    InterlockedIncrement(&s->refcount);
}

/* obtains cs_conf_global */
void 
khcint_space_release(kconf_conf_space * s)
{
    LONG l;

    EnterCriticalSection(&cs_conf_global);

    l = InterlockedDecrement(&s->refcount);
    if (l == 0) {
        if(s->regkey_machine)
            RegCloseKey(s->regkey_machine);
        if(s->regkey_user)
            RegCloseKey(s->regkey_user);
        s->regkey_machine = NULL;
        s->regkey_user = NULL;

        if (s->flags &
            (KCONF_SPACE_FLAG_DELETE_M |
             KCONF_SPACE_FLAG_DELETE_U)) {
            khcint_remove_space(s, s->flags);
        } else {
#ifdef USE_TRY_FREE
            /* even if the refcount is zero, we shouldn't free a
               configuration space just yet since that doesn't play
               well with the configuration space enumeration mechanism
               which expects the spaces to dangle around if there is a
               corresponding registry key or schema. */
            khcint_try_free_space(s);
#endif
        }
    }

    LeaveCriticalSection(&cs_conf_global);
}

/* obtains cs_conf_global */
HKEY 
khcint_space_open_key(kconf_conf_space * s, khm_int32 flags)
{
    HKEY hk_root;
    HKEY hk = NULL;
    int nflags = 0;
    DWORD disp;
    khm_boolean per_machine = !!(flags & KCONF_FLAG_MACHINE);
    khm_boolean skip = FALSE;

    EnterCriticalSection(&cs_conf_global);
    if (per_machine) {
        hk = s->regkey_machine;
        hk_root = HKEY_LOCAL_MACHINE;
        skip = !!(s->flags & KCONF_SPACE_FLAG_NO_HKLM);
    } else {
        hk = s->regkey_user;
        hk_root = HKEY_CURRENT_USER;
        skip = !!(s->flags & KCONF_SPACE_FLAG_NO_HKCU);
    }
    LeaveCriticalSection(&cs_conf_global);

    if (hk || (skip && !(flags & KHM_FLAG_CREATE)))
        return hk;

    if((RegOpenKeyEx(hk_root, s->regpath, 0, 
                     KEY_READ | KEY_WRITE, &hk) != ERROR_SUCCESS) &&

       !(flags & KHM_PERM_WRITE)) {

        if(RegOpenKeyEx(hk_root, s->regpath, 0, 
                        KEY_READ, &hk) == ERROR_SUCCESS) {
            nflags = KHM_PERM_READ;
        }
    }

    if(!hk && (flags & KHM_FLAG_CREATE)) {

        RegCreateKeyEx(hk_root, 
                       s->regpath, 
                       0, NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE,
                       NULL, &hk, &disp);
    }

    EnterCriticalSection(&cs_conf_global);
    if(hk) {
        if (per_machine) {
            s->regkey_machine = hk;
            s->flags &= ~KCONF_SPACE_FLAG_NO_HKLM;
        } else {
            s->regkey_user = hk;
            s->flags &= ~KCONF_SPACE_FLAG_NO_HKCU;
        }
    } else {
        if (per_machine)
            s->flags |= KCONF_SPACE_FLAG_NO_HKLM;
        else
            s->flags |= KCONF_SPACE_FLAG_NO_HKCU;
    }
    LeaveCriticalSection(&cs_conf_global);

    return hk;
}

KHMEXP khm_int32 KHMAPI
khc_dup_space(khm_handle vh, khm_handle * pvh)
{
    kconf_handle * h;
    kconf_handle *h2;

    if (!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if (!khc_is_handle(vh) || pvh == NULL) {
        return KHM_ERROR_INVALID_PARAM;
    }

    h = (kconf_handle *) vh;
    h2 = khcint_handle_dup(h);

    *pvh = (khm_handle) h2;

    return KHM_ERROR_SUCCESS;
}

/* obtains cs_conf_handle/cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_shadow_space(khm_handle upper, khm_handle lower)
{
    kconf_handle * h;

    if(!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if(!khc_is_handle(upper)) {
#ifdef DEBUG
        DebugBreak();
#endif
        return KHM_ERROR_INVALID_PARAM;
    }

    h = (kconf_handle *) upper;

    EnterCriticalSection(&cs_conf_handle);
    if(h->lower) {
        EnterCriticalSection(&cs_conf_global);
        khcint_handle_free(h->lower);
        LeaveCriticalSection(&cs_conf_global);
        h->lower = NULL;
    }

    if(khc_is_handle(lower)) {
        kconf_handle * l;
        kconf_handle * lc;

        l = (kconf_handle *) lower;
        lc = khcint_handle_dup(l);
        h->lower = lc;
    }
    LeaveCriticalSection(&cs_conf_handle);

    return KHM_ERROR_SUCCESS;
}

/* no locks */
kconf_conf_space * 
khcint_create_empty_space(void)
{
    kconf_conf_space * r;

    r = PMALLOC(sizeof(kconf_conf_space));
    assert(r != NULL);
    ZeroMemory(r,sizeof(kconf_conf_space));
    r->magic = KCONF_SPACE_MAGIC;

    return r;
}

/* called with cs_conf_global */
void 
khcint_free_space(kconf_conf_space * r)
{
    kconf_conf_space * c;

    if(!r)
        return;

    TPOPCHILD(r, &c);
    while(c) {
        khcint_free_space(c);
        TPOPCHILD(r, &c);
    }

    if(r->name)
        PFREE(r->name);

    if(r->regpath)
        PFREE(r->regpath);

    if(r->regkey_machine)
        RegCloseKey(r->regkey_machine);

    if(r->regkey_user)
        RegCloseKey(r->regkey_user);

    PFREE(r);
}

/* obtains cs_conf_global */
khm_int32
khcint_open_space(kconf_conf_space * parent, 
                  const wchar_t * sname, size_t n_sname, 
                  khm_int32 flags, kconf_conf_space **result)
{
    kconf_conf_space * p;
    kconf_conf_space * c;
    wchar_t buf[KCONF_MAXCCH_NAME];
    size_t cb_regpath = 0;

    if(!parent)
        p = conf_root;
    else
        p = parent;

    if(n_sname >= KCONF_MAXCCH_NAME || n_sname <= 0)
        return KHM_ERROR_INVALID_PARAM;

    StringCchCopyN(buf, ARRAYLENGTH(buf), sname, n_sname);

    /* see if there is already a config space by this name. if so,
       return it.  Note that if the configuration space is specified
       in a schema, we would find it here. */
    EnterCriticalSection(&cs_conf_global);
    c = TFIRSTCHILD(p);
    while(c) {
        if(c->name && !wcscmp(c->name, buf))
            break;

        c = LNEXT(c);
    }

    if (c)
        khcint_space_hold(c);
    LeaveCriticalSection(&cs_conf_global);

    if(c) {

        if (c->flags & KCONF_SPACE_FLAG_DELETED) {
            if (flags & KHM_FLAG_CREATE) {
                c->flags &= ~(KCONF_SPACE_FLAG_DELETED |
                              KCONF_SPACE_FLAG_DELETE_M |
                              KCONF_SPACE_FLAG_DELETE_U);
            } else {
                khcint_space_release(c);
                *result = NULL;
                return KHM_ERROR_NOT_FOUND;
            }
        }

        *result = c;
        return KHM_ERROR_SUCCESS;
    }

    if(!(flags & KHM_FLAG_CREATE)) {
        HKEY pkey = NULL;
        HKEY ckey = NULL;

        /* we are not creating the space, so it must exist in the form
           of a registry key in HKLM or HKCU.  If it existed as a
           schema, we would have already retured it above. */

        if (flags & KCONF_FLAG_USER)
            pkey = khcint_space_open_key(p, KHM_PERM_READ | KCONF_FLAG_USER);

        if((pkey == NULL ||
            (RegOpenKeyEx(pkey, buf, 0, KEY_READ, &ckey) != ERROR_SUCCESS))
           && (flags & KCONF_FLAG_MACHINE)) {

            pkey = khcint_space_open_key(p, KHM_PERM_READ | KCONF_FLAG_MACHINE);

            if(pkey == NULL ||
               (RegOpenKeyEx(pkey, buf, 0, KEY_READ, &ckey) != ERROR_SUCCESS)) {

                *result = NULL;
                return KHM_ERROR_NOT_FOUND;
            }
        }

        if(ckey) {
            RegCloseKey(ckey);
            ckey = NULL;
        }
    }

    c = khcint_create_empty_space();
    
    /*SAFE: buf: is of known length < KCONF_MAXCCH_NAME */
    c->name = PWCSDUP(buf);

    /*SAFE: p->regpath: is valid since it was set using this same
      function. */
    /*SAFE: buf: see above */
    cb_regpath = (wcslen(p->regpath) + wcslen(buf) + 2) * sizeof(wchar_t);
    c->regpath = PMALLOC(cb_regpath);

    assert(c->regpath != NULL);

    /*SAFE: c->regpath: allocated above to be big enough */
    /*SAFE: p->regpath: see above */
    StringCbCopy(c->regpath, cb_regpath, p->regpath);
    StringCbCat(c->regpath, cb_regpath, L"\\");

    /*SAFE: buf: see above */
    StringCbCat(c->regpath, cb_regpath, buf);

    khcint_space_hold(c);

    EnterCriticalSection(&cs_conf_global);
    TADDCHILD(p,c);
    LeaveCriticalSection(&cs_conf_global);

    *result = c;
    return KHM_ERROR_SUCCESS;
}

/* obtains cs_conf_handle/cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_open_space(khm_handle parent, const wchar_t * cspace, khm_int32 flags, 
               khm_handle * result)
{
    kconf_conf_space * p;
    kconf_conf_space * c = NULL;
    size_t cbsize;
    const wchar_t * str;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if (!khc_is_config_running()) {
        return KHM_ERROR_NOT_READY;
    }

    if (!result || (parent && !khc_is_handle(parent)) ||
        (cspace && FAILED(StringCbLength(cspace, KCONF_MAXCB_PATH, &cbsize)))) {
        return KHM_ERROR_INVALID_PARAM;
    }

    /* if none of these flags are specified, make it seem like all of
       them were */
    if ((flags & (KCONF_FLAG_USER |
                  KCONF_FLAG_MACHINE |
                  KCONF_FLAG_SCHEMA)) == 0)
        flags |= KCONF_FLAG_USER | KCONF_FLAG_MACHINE | KCONF_FLAG_SCHEMA;

    if(!parent)
        p = conf_root;
    else
        p = khc_space_from_handle(parent);

    khcint_space_hold(p);

    if(cspace == NULL) {
        /* If space is NULL, the caller wants a new handle to the same
           configuration space with different options. */
        *result = (khm_handle) khcint_handle_from_space(p, flags);
        khcint_space_release(p);
        return KHM_ERROR_SUCCESS;
    }

    str = cspace;
    while(TRUE) {
        const wchar_t * end = NULL;
        khm_boolean last = FALSE;

        /* Loop invariants:

           str = pointer to rest of configuration path to parse
           p   = configuration space in which to evaluate str

           On loop exit:

           c   = configuration space we want
           p   = parent of c, if non NULL

           e.g.:

           khc_open_space(h, L"foo\\bar\\baz", ...);
           -> (p = h, str = "foo\\bar\\baz")
           -> (p = h\foo, str = "bar\\baz")
           -> (p = h\foo\bar, str = "baz")
           -> exit with c = h\foo\bar\baz
         */

        end = wcschr(str, L'\\');

        if (end == NULL) {
            if (flags & KCONF_FLAG_TRAILINGVALUE) {
                c = p;
                p = NULL;
                break;
            } else {
                end = str + wcslen(str);
                last = TRUE;
            }
        } else {
            if (flags & KCONF_FLAG_TRAILINGVALUE) {
                if (wcschr(end + 1, L'\\') == NULL)
                    last = TRUE;
            }
        }

        rv = khcint_open_space(p, str, end - str, flags, &c);

        if (KHM_SUCCEEDED(rv) && !last) {
            khcint_space_release(p);
            p = c;
            c = NULL;
            str = end + 1;
        } else if (last && rv == KHM_ERROR_NOT_FOUND &&
                   (flags & KCONF_FLAG_SHADOW)) {
            
            rv = khcint_open_space(p, L"_Schema", 7,
                                   flags, &c);
            flags &= ~KCONF_FLAG_SHADOW;
            flags |= KCONF_FLAG_READONLY;
            khcint_space_release(p);
            p = NULL;
            break;

        } else {

            break;

        }
    }

    if (p)
        khcint_space_release(p);

    if(KHM_SUCCEEDED(rv)) {
        *result = khcint_handle_from_space(c, flags);

        if (*result && (flags & KCONF_FLAG_SHADOW)) {
            p = TPARENT(c);
            if (p) {

                kconf_conf_space * shadow;

                khcint_space_hold(p);

                flags &= ~KHM_FLAG_CREATE;
                rv = khcint_open_space(p, L"_Schema", 7,
                                       flags, &shadow);

                if (KHM_SUCCEEDED(rv)) {
                    khm_handle tshadow = NULL;

                    tshadow = khcint_handle_from_space(shadow,
                                                       flags | KCONF_FLAG_READONLY);

                    khc_shadow_space(*result, tshadow);

                    khc_close_space(tshadow);

                    khcint_space_release(shadow);
                }

                khcint_space_release(p);

                rv = KHM_ERROR_SUCCESS;
            }
        }
    } else {
        *result = NULL;
    }

    if (c)
        khcint_space_release(c);

    return rv;
}

/* obtains cs_conf_handle/cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_close_space(khm_handle csp)
{
    if(!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if(!khc_is_handle(csp)) {
#ifdef DEBUG
        DebugBreak();
#endif
        return KHM_ERROR_INVALID_PARAM;
    }

    khcint_handle_free((kconf_handle *) csp);
    return KHM_ERROR_SUCCESS;
}


static const DWORD kc_type_to_reg_type[] = {
    0,
    0,
    0,
    REG_DWORD,
    REG_QWORD,
    REG_SZ,
    REG_BINARY
};

khm_int32
khcint_read_value_from_registry(HKEY hk, const wchar_t * value,
                                khm_int32 expected_type,
                                void * buf, khm_size * bufsize)
{
    DWORD size;
    DWORD type;
    LONG hr;

    if (hk == NULL)
        return KHM_ERROR_NOT_FOUND;

    size = (DWORD) *bufsize;

    hr = RegQueryValueEx(hk, value, NULL, &type, (LPBYTE) buf, &size);

    if(hr == ERROR_SUCCESS) {
        if(type != kc_type_to_reg_type[expected_type]) {

            return KHM_ERROR_TYPE_MISMATCH;

        } else if (type == REG_SZ && buf != NULL &&
                   ((wchar_t *) buf)[size / sizeof(wchar_t) - 1] != L'\0') {

            /* if the target buffer is not large enough to store the
               terminating NUL, RegQueryValueEx() will return
               ERROR_SUCCESS without terminating the string. */

            *bufsize = size + sizeof(wchar_t);
            return KHM_ERROR_TOO_LONG;

        } else {

            *bufsize = size;

            /* if buf==NULL, RegQueryValueEx will return success and
               just return the required buffer size in 'size' */
            return (buf)? KHM_ERROR_SUCCESS: KHM_ERROR_TOO_LONG;
        }
    } else if(hr == ERROR_MORE_DATA) {

        *bufsize = size;
        return KHM_ERROR_TOO_LONG;

    } else {

        return KHM_ERROR_NOT_FOUND;

    }
}

khm_int32
khcint_read_value_from_schema(kconf_conf_space * c,
                              const wchar_t * value,
                              khm_int32 expected_type,
                              void * buf, khm_size * bufsize)
{
    int i;

    for (i = 0; i < c->nSchema; i++) {

        if (c->schema[i].type == expected_type &&

            !wcscmp(value, c->schema[i].name)) {

            khm_int32 i32 = 0;
            khm_int64 i64 = 0;
            size_t cbsize = 0;
            void * src = NULL;
            const kconf_schema * s = &c->schema[i];

            switch (s->type) {
            case KC_INT32:
                cbsize = sizeof(khm_int32);
                i32 = (khm_int32) s->value;
                src = &i32;
                break;

            case KC_INT64:
                cbsize = sizeof(khm_int64);
                i64 = (khm_int64) s->value;
                src = &i64;
                break;

            case KC_STRING:
                if(FAILED(StringCbLength((wchar_t *) s->value, KCONF_MAXCB_STRING, &cbsize))) {
                    break;
                }
                cbsize += sizeof(wchar_t);
                src = (wchar_t *) s->value;
                break;
            }

            if (src == NULL)
                return KHM_ERROR_NOT_FOUND;

            if (buf == NULL || *bufsize < cbsize) {
                *bufsize = cbsize;
                return KHM_ERROR_TOO_LONG;
            }

            memcpy(buf, src, cbsize);
            *bufsize = cbsize;
            return KHM_ERROR_SUCCESS;
        }
    }

    return KHM_ERROR_NOT_FOUND;
}


/*! \internal
  \brief Get the actual parent configuration space handle and value

  The value name passed into some of the kconf functions can include
  configuration space components.  I.e.:

  \code
  khc_read_int32(h, L"foo\\bar", &i);
  \endcode

  In such cases, the caller is referring to a value named "bar" in a
  configuration space named "foo" that is a child of the space
  represented by \a h.  This function takes a configuration space
  handle \a pconf and a \a path and returns a handle to the
  configuration space containing the value that is being referenced.
 */
khm_int32
khcint_get_parent_config_space(khm_handle pconf,
                               const wchar_t * path,
                               khm_handle *ppconf,
                               const wchar_t ** pvalue)
{
    wchar_t * value = NULL;

    /* If the value contains a '\\', we consider it to be of the form
       "relative\\path\\valuename".  Therefore we have to obtain a
       handle to the configuration space which holds the value. */
    if((value = wcsrchr(path, L'\\')) != NULL) {

        if(KHM_FAILED(khc_open_space(pconf,
                                     path,
                                     KCONF_FLAG_TRAILINGVALUE | (pconf?khc_handle_flags(pconf):0), 
                                     ppconf)))
            return KHM_ERROR_INVALID_PARAM;

        *pvalue = ++value;
    } else {
        *pvalue = path;
        *ppconf = pconf;
    }

    return KHM_ERROR_SUCCESS;
}

khm_int32
khcint_read_value_from_cspace(khm_handle pconf,
                              const wchar_t * pvalue,
                              khm_int32 expected_type,
                              void * buf, khm_size * bufsize)
{
    kconf_conf_space * c = NULL;
    const wchar_t * value = NULL;
    khm_handle conf = NULL;
    khm_int32 rv = KHM_ERROR_NOT_FOUND;

    rv = khcint_get_parent_config_space(pconf, pvalue, &conf, &value);
    if (KHM_FAILED(rv))
        goto done;

    assert(khc_is_handle(conf));

    c = khc_space_from_handle(conf);

    if (khc_is_user_handle(conf) &&

        (rv = khcint_read_value_from_registry(khcint_space_open_key(c, KHM_PERM_READ),
                                               value, expected_type,
                                               buf, bufsize)) != KHM_ERROR_NOT_FOUND) {
        goto done;
    }

    if (khc_is_machine_handle(conf) &&
        (rv = khcint_read_value_from_registry
         (khcint_space_open_key(c,
                                KHM_PERM_READ | KCONF_FLAG_MACHINE),
          value, expected_type,
          buf, bufsize)) != KHM_ERROR_NOT_FOUND) {
        goto done;
    }

    if (khc_is_schema_handle(conf) &&
        (rv = khcint_read_value_from_schema(c, value, expected_type,
                                             buf, bufsize)) != KHM_ERROR_NOT_FOUND) {
        goto done;
    }

 done:
    if(conf != pconf)
        khc_close_space(conf);

    return rv;
}


khm_int32
khcint_read_value(khm_handle pconf,
                  const wchar_t * pvalue,
                  khm_int32 expected_type,
                  void * buf, khm_size * bufsize)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if(!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if (expected_type < 0 || expected_type >= ARRAYLENGTH(kc_type_to_reg_type))
        return KHM_ERROR_INVALID_PARAM;

    do {
        rv = khcint_read_value_from_cspace(pconf, pvalue, expected_type, buf, bufsize);
        if (rv != KHM_ERROR_NOT_FOUND)
            break;

        if (khc_is_shadowed(pconf))
            pconf = khc_shadow(pconf);
        else
            break;
    } while(pconf);

    return rv;
}

/* obtains cs_conf_handle/cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_read_string(khm_handle pconf, 
                const wchar_t * pvalue, 
                wchar_t * buf, 
                khm_size * bufsize) 
{
    return khcint_read_value(pconf, pvalue, KC_STRING, buf, bufsize);
}

/* obtains cs_conf_handle/cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_read_int32(khm_handle pconf, const wchar_t * pvalue, khm_int32 * buf)
{
    khm_size cb = sizeof(khm_int32);
    return khcint_read_value(pconf, pvalue, KC_INT32, buf, &cb);
}

/* obtains cs_conf_handle/cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_read_int64(khm_handle pconf, const wchar_t * pvalue, khm_int64 * buf)
{
    khm_size cb = sizeof(khm_int64);
    return khcint_read_value(pconf, pvalue, KC_INT64, buf, &cb);
}

/* obtaincs cs_conf_handle/cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_read_binary(khm_handle pconf, const wchar_t * pvalue, 
                void * buf, khm_size * bufsize)
{
    return khcint_read_value(pconf, pvalue, KC_BINARY, buf, bufsize);
}

khm_int32
khcint_write_value(khm_handle pconf,
                   const wchar_t * pvalue,
                   khm_int32 type,
                   const void * buf,
                   khm_size bufsize)
{
    kconf_conf_space * c = NULL;
    khm_handle conf = NULL;
    wchar_t * value = NULL;
    HKEY pk = NULL;
    LONG hr = 0;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if (!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if (pconf && !khc_is_machine_handle(pconf) && !khc_is_user_handle(pconf))
        return KHM_ERROR_INVALID_OPERATION;

    if (pconf && khc_is_readonly(pconf))
        return KHM_ERROR_READONLY;

    if (type != KC_STRING && type != KC_INT32 && type != KC_INT64 && type != KC_BINARY)
        return KHM_ERROR_INVALID_PARAM;

    if (pconf && (khc_handle_flags(pconf) & KCONF_FLAG_WRITEIFMOD)) {
        khm_int32 static_buf[128];
        void * b_exist = static_buf;
        size_t b_existsize = sizeof(static_buf);
        khm_boolean is_equal = FALSE;
        khm_int32 rv;

        rv = khcint_read_value(pconf, pvalue, type, b_exist, &b_existsize);
        if (rv == KHM_ERROR_TOO_LONG) {
            b_exist = PMALLOC(b_existsize);
            rv = khcint_read_value(pconf, pvalue, type, b_exist, &b_existsize);
        }

        if (KHM_SUCCEEDED(rv) && b_existsize == bufsize) {
            if (type == KC_STRING && (khc_handle_flags(pconf) & KCONF_FLAG_IFMODCI))
                is_equal = !lstrcmpi(b_exist, buf);
            else
                is_equal = !memcmp(b_exist, buf, bufsize);
        }

        if (b_exist != static_buf)
            PFREE(b_exist);

        if (is_equal)
            return KHM_ERROR_SUCCESS;
    }

    rv = khcint_get_parent_config_space(pconf, pvalue, &conf, &value);
    if (KHM_FAILED(rv))
        goto done;

    c = khc_space_from_handle(conf);

    if (khc_is_user_handle(conf)) {
        pk = khcint_space_open_key(c, KHM_PERM_WRITE | KHM_FLAG_CREATE);
    } else {
        pk = khcint_space_open_key(c, KHM_PERM_WRITE | KCONF_FLAG_MACHINE | KHM_FLAG_CREATE);
    }

    hr = RegSetValueEx(pk, value, 0,
                       kc_type_to_reg_type[type],
                       (LPBYTE) buf,
                       (DWORD) bufsize);

    if (hr == ERROR_SUCCESS)
        rv = KHM_ERROR_SUCCESS;
    else
        rv = KHM_ERROR_INVALID_OPERATION;

 done:
    if (conf && conf != pconf)
        khc_close_space(conf);
    return rv;
}


/* obtains cs_conf_handle/cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_write_string(khm_handle pconf, 
                 const wchar_t * pvalue, 
                 const wchar_t * buf) 
{
    size_t cb;

    if (FAILED(StringCbLength(buf, KCONF_MAXCB_STRING, &cb)))
        return KHM_ERROR_INVALID_PARAM;

    cb += sizeof(wchar_t);

    return khcint_write_value(pconf, pvalue, KC_STRING, buf, cb);
}

/* obtaincs cs_conf_handle/cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_write_int32(khm_handle pconf, 
                const wchar_t * pvalue, 
                khm_int32 val) 
{
    return khcint_write_value(pconf, pvalue, KC_INT32, &val, sizeof(val));
}

/* obtains cs_conf_handle/cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_write_int64(khm_handle pconf, const wchar_t * pvalue, khm_int64 val)
{
    return khcint_write_value(pconf, pvalue, KC_INT64, &val, sizeof(val));
}

/* obtains cs_conf_handle/cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_write_binary(khm_handle pconf, 
                 const wchar_t * pvalue, 
                 const void * buf, khm_size bufsize)
{
    return khcint_write_value(pconf, pvalue, KC_BINARY, buf, bufsize);
}

/* no locks */
KHMEXP khm_int32 KHMAPI 
khc_get_config_space_name(khm_handle conf, 
                          wchar_t * buf, khm_size * bufsize)
{
    kconf_conf_space * c;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if(!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if(!khc_is_handle(conf))
        return KHM_ERROR_INVALID_PARAM;

    c = khc_space_from_handle(conf);

    if(!c->name) {
        if(buf && *bufsize > 0)
            buf[0] = L'\0';
        else {
            *bufsize = sizeof(wchar_t);
            rv = KHM_ERROR_TOO_LONG;
        }
    } else {
        size_t cbsize;

        if(FAILED(StringCbLength(c->name, KCONF_MAXCB_NAME, &cbsize)))
            return KHM_ERROR_UNKNOWN;

        cbsize += sizeof(wchar_t);

        if(!buf || cbsize > *bufsize) {
            *bufsize = cbsize;
            rv = KHM_ERROR_TOO_LONG;
        } else {
            StringCbCopy(buf, *bufsize, c->name);
            *bufsize = cbsize;
        }
    }

    return rv;
}

/* obtains cs_conf_handle/cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_get_config_space_parent(khm_handle conf, khm_handle * parent)
{
    kconf_conf_space * c;

    if(!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if(!khc_is_handle(conf))
        return KHM_ERROR_INVALID_PARAM;

    c = khc_space_from_handle(conf);

    if(c == conf_root || c->parent == conf_root)
        *parent = NULL;
    else
        *parent = khcint_handle_from_space(c->parent, khc_handle_flags(conf));

    return KHM_ERROR_SUCCESS;
}

/* obtains cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_get_type(khm_handle conf, const wchar_t * value)
{
    HKEY hkm = NULL;
    HKEY hku = NULL;
    kconf_conf_space * c;
    khm_int32 rv;
    LONG hr = ERROR_SUCCESS;
    DWORD type = 0;

    if(!khc_is_config_running())
        return KC_NONE;

    if(!khc_is_handle(conf))
        return KC_NONE;

    c = khc_space_from_handle(conf);

    if(!khc_is_machine_handle(conf))
        hku = khcint_space_open_key(c, KHM_PERM_READ);
    hkm = khcint_space_open_key(c, KHM_PERM_READ | KCONF_FLAG_MACHINE);

    if(hku)
        hr = RegQueryValueEx(hku, value, NULL, &type, NULL, NULL);
    if(!hku || hr != ERROR_SUCCESS)
        hr = RegQueryValueEx(hkm, value, NULL, &type, NULL, NULL);
    if(((!hku && !hkm) || hr != ERROR_SUCCESS) && c->schema) {
        int i;

        for(i=0; i<c->nSchema; i++) {
            if(!wcscmp(c->schema[i].name, value)) {
                return c->schema[i].type;
            }
        }

        return KC_NONE;
    }

    switch(type) {
        case REG_MULTI_SZ:
        case REG_SZ:
            rv = KC_STRING;
            break;
        case REG_DWORD:
            rv = KC_INT32;
            break;
        case REG_QWORD:
            rv = KC_INT64;
            break;
        case REG_BINARY:
            rv = KC_BINARY;
            break;
        default:
            rv = KC_NONE;
    }

    return rv;
}

KHMEXP khm_int32 KHMAPI
khc_get_last_write_time(khm_handle conf, khm_int32 flags, FILETIME * last_w_time)
{
    kconf_conf_space * c;
    HKEY hku = NULL;
    HKEY hkm = NULL;
    FILETIME ftr = {0,0};

    if (!khc_is_config_running() || !khc_is_handle(conf) || last_w_time == NULL)
        return KHM_ERROR_INVALID_PARAM;

    if (flags == 0)
        flags = KCONF_FLAG_MACHINE | KCONF_FLAG_USER;

    do {
        FILETIME ft = {0,0};

        hku = NULL;
        hkm = NULL;
        c = khc_space_from_handle(conf);

        if (khc_is_user_handle(conf) &&

            (flags & KCONF_FLAG_USER) &&

            (hku = khcint_space_open_key(c, KHM_PERM_READ)) != NULL &&

            (RegQueryInfoKey(hku, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, &ft) == ERROR_SUCCESS) &&

            ((ftr.dwLowDateTime == 0 && ftr.dwHighDateTime == 0) ||
             CompareFileTime(&ftr, &ft) < 0)) {

            ftr = ft;
        }

        if (khc_is_machine_handle(conf) &&

            (flags & KCONF_FLAG_MACHINE) &&

            (hkm = khcint_space_open_key(c, KHM_PERM_READ | KCONF_FLAG_MACHINE)) != NULL &&

            (RegQueryInfoKey(hkm, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, &ft) == ERROR_SUCCESS) &&

            ((ftr.dwLowDateTime == 0 && ftr.dwHighDateTime == 0) ||
             CompareFileTime(&ftr, &ft) < 0)) {

            ftr = ft;
        }

        if (khc_is_shadowed(conf))
            conf = khc_shadow(conf);
        else
            break;

    } while(conf);

    if (ftr.dwLowDateTime == 0 && ftr.dwHighDateTime == 0)
        return KHM_ERROR_NOT_FOUND;
    else {
        *last_w_time = ftr;
        return KHM_ERROR_SUCCESS;
    }
}

/* obtains cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_value_exists(khm_handle conf, const wchar_t * value)
{
    HKEY hku = NULL;
    HKEY hkm = NULL;
    kconf_conf_space * c;
    khm_int32 rv = 0;
    DWORD t;
    int i;

    if(!khc_is_config_running() || !khc_is_handle(conf))
        return 0;

    do {
        hku = NULL;
        hkm = NULL;
        c = khc_space_from_handle(conf);

        if (khc_is_user_handle(conf))
            hku = khcint_space_open_key(c, KHM_PERM_READ);
        if (khc_is_machine_handle(conf))
            hkm = khcint_space_open_key(c, KHM_PERM_READ | KCONF_FLAG_MACHINE);

        if(hku && (RegQueryValueEx(hku, value, NULL, &t, NULL, NULL) == ERROR_SUCCESS))
            rv |= KCONF_FLAG_USER;
        if(hkm && (RegQueryValueEx(hkm, value, NULL, &t, NULL, NULL) == ERROR_SUCCESS))
            rv |= KCONF_FLAG_MACHINE;

        if(c->schema && khc_is_schema_handle(conf)) {
            for(i=0; i<c->nSchema; i++) {
                if(!wcscmp(c->schema[i].name, value)) {
                    rv |= KCONF_FLAG_SCHEMA;
                    break;
                }
            }
        }

        /* if the value is not found at this level and the handle is
           shadowed, try the next level down. */
        if (rv == 0 && khc_is_shadowed(conf))
            conf = khc_shadow(conf);
        else
            break;
    } while (conf);

    return rv;
}

/* obtains cs_conf_global */
KHMEXP khm_int32 KHMAPI
khc_remove_value(khm_handle conf, const wchar_t * value, khm_int32 flags)
{
    HKEY hku = NULL;
    HKEY hkm = NULL;
    kconf_conf_space * c;
    khm_int32 rv = KHM_ERROR_NOT_FOUND;
    DWORD t;
    LONG l;

    if (!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if (!khc_is_handle(conf))
        return KHM_ERROR_INVALID_PARAM;

    c = khc_space_from_handle(conf);

    if (khc_is_user_handle(conf))
        hku = khcint_space_open_key(c, KHM_PERM_WRITE);
    if (khc_is_machine_handle(conf))
        hkm = khcint_space_open_key(c, KHM_PERM_WRITE | KCONF_FLAG_MACHINE);

    if ((flags == 0 ||
         (flags & KCONF_FLAG_USER)) &&
        hku && (RegQueryValueEx(hku, value, NULL, 
                                &t, NULL, NULL) == ERROR_SUCCESS)) {
        l = RegDeleteValue(hku, value);
        if (l == ERROR_SUCCESS)
            rv = KHM_ERROR_SUCCESS;
        else
            rv = KHM_ERROR_UNKNOWN;
    }
    if ((flags == 0 ||
         (flags & KCONF_FLAG_MACHINE)) &&
        hkm && (RegQueryValueEx(hkm, value, NULL, 
                                &t, NULL, NULL) == ERROR_SUCCESS)) {
        l = RegDeleteValue(hkm, value);
        if (l == ERROR_SUCCESS)
            rv = (rv == KHM_ERROR_UNKNOWN)?KHM_ERROR_PARTIAL: 
                KHM_ERROR_SUCCESS;
        else
            rv = (rv == KHM_ERROR_SUCCESS)?KHM_ERROR_PARTIAL:
                KHM_ERROR_UNKNOWN;
    }

    return rv;
}

/* called with cs_conf_global held */
khm_int32
khcint_remove_space(kconf_conf_space * c, khm_int32 flags)
{
    kconf_conf_space * cc;
    kconf_conf_space * cn;
    kconf_conf_space * p;
    khm_boolean free_c = FALSE;

    p = TPARENT(c);

    /* We don't allow deleting top level keys.  They are
       predefined. */
#ifdef DEBUG
    assert(p);
#endif
    if (!p)
        return KHM_ERROR_INVALID_OPERATION;

    cc = TFIRSTCHILD(c);
    while (cc) {
        cn = LNEXT(cc);

        khcint_remove_space(cc, flags);

        cc = cn;
    }

    cc = TFIRSTCHILD(c);
    if (!cc && !c->schema && c->refcount == 0) {
        TDELCHILD(p, c);
        free_c = TRUE;
    } else {
        c->flags |= (flags &
                     (KCONF_SPACE_FLAG_DELETE_M |
                      KCONF_SPACE_FLAG_DELETE_U));

        /* if all the registry spaces have been marked as deleted and
           there is no schema, we should mark the space as deleted as
           well.  Note that ideally we only need to check for stores
           which have data corresponding to this configuration space,
           but this is a bit problematic since we don't monitor the
           registry for changes. */
        if ((c->flags &
             (KCONF_SPACE_FLAG_DELETE_M |
              KCONF_SPACE_FLAG_DELETE_U)) ==
            (KCONF_SPACE_FLAG_DELETE_M |
             KCONF_SPACE_FLAG_DELETE_U) &&
            (!c->schema || c->nSchema == 0))

            c->flags |= KCONF_SPACE_FLAG_DELETED;
    }

    if (c->regpath && p->regpath) {
        HKEY hk;

        if (flags & KCONF_SPACE_FLAG_DELETE_U) {
            hk = khcint_space_open_key(p, KCONF_FLAG_USER);

            if (hk)
                RegDeleteKey(hk, c->name);
        }
        if (flags & KCONF_SPACE_FLAG_DELETE_M) {
            hk = khcint_space_open_key(p, KCONF_FLAG_MACHINE);

            if (hk)
                RegDeleteKey(hk, c->name);
        }
    }

    if (free_c) {
        khcint_free_space(c);
    }

    return KHM_ERROR_SUCCESS;
}

/* obtains cs_conf_global */
KHMEXP khm_int32 KHMAPI
khc_remove_space(khm_handle conf)
{

    /*
      - mark this space as well as all child spaces as
        'delete-on-close' using flags.  Mark should indicate which
        repository to delete the space from. (user/machine)

      - When each subspace is released, check if it has been marked
        for deletion.  If so, delete the marked spaces as well as
        removing the space from kconf space tree.

      - When removing a subspace from a space, check if the parent
        space has any children left.  If there are none, check if the
        parent space is also marked for deletion.
    */
    kconf_conf_space * c;
    khm_int32 rv = KHM_ERROR_SUCCESS;
    khm_int32 flags = 0;

    if (!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if (!khc_is_handle(conf))
        return KHM_ERROR_INVALID_PARAM;

    c = khc_space_from_handle(conf);

    EnterCriticalSection(&cs_conf_global);

    if (khc_is_machine_handle(conf))
        flags |= KCONF_SPACE_FLAG_DELETE_M;
    if (khc_is_user_handle(conf))
        flags |= KCONF_SPACE_FLAG_DELETE_U;

    rv = khcint_remove_space(c, flags);

    LeaveCriticalSection(&cs_conf_global);

    return rv;
}

/* no locks */
khm_boolean 
khcint_is_valid_name(wchar_t * name)
{
    size_t cbsize;
    if(FAILED(StringCbLength(name, KCONF_MAXCB_NAME, &cbsize)))
        return FALSE;
    return TRUE;
}

/* no locks */
khm_int32 
khcint_validate_schema(const kconf_schema * schema,
                       int begin,
                       int *end)
{
    int i;
    int state = 0;
    int end_found = 0;

    i = begin;
    while (!end_found) {
        switch (state) {
        case 0: /* initial.  this record should start a config space */
            if(!khcint_is_valid_name(schema[i].name) ||
               schema[i].type != KC_SPACE)
                return KHM_ERROR_INVALID_PARAM;
            state = 1;
            break;

        case 1: /* we are inside a config space, in the values area */
            if (!khcint_is_valid_name(schema[i].name))
                return KHM_ERROR_INVALID_PARAM;
            if (schema[i].type == KC_SPACE) {
                if(KHM_FAILED(khcint_validate_schema(schema, i, &i)))
                    return KHM_ERROR_INVALID_PARAM;
                state = 2;
            } else if (schema[i].type == KC_ENDSPACE) {
                end_found = 1;
                if (end)
                    *end = i;
            } else {
                if (schema[i].type != KC_STRING &&
                    schema[i].type != KC_INT32 &&
                    schema[i].type != KC_INT64 &&
                    schema[i].type != KC_BINARY)
                    return KHM_ERROR_INVALID_PARAM;
            }
            break;

        case 2: /* we are inside a config space, in the subspace area */
            if (schema[i].type == KC_SPACE) {
                if (KHM_FAILED(khcint_validate_schema(schema, i, &i)))
                    return KHM_ERROR_INVALID_PARAM;
            } else if (schema[i].type == KC_ENDSPACE) {
                end_found = 1;
                if (end)
                    *end = i;
            } else {
                return KHM_ERROR_INVALID_PARAM;
            }
            break;
            
        default:
            /* unreachable */
            return KHM_ERROR_INVALID_PARAM;
        }
        i++;
    }

    return KHM_ERROR_SUCCESS;
}

/* obtains cs_conf_handle/cs_conf_global; called with cs_conf_global */
khm_int32 
khcint_load_schema_i(khm_handle parent, const kconf_schema * schema, 
                     int begin, int * end)
{
    int i;
    int state = 0;
    int end_found = 0;
    kconf_conf_space * thisconf = NULL;
    khm_handle h = NULL;

    i=begin;
    while(!end_found) {
        switch(state) {
            case 0: /* initial.  this record should start a config space */
                LeaveCriticalSection(&cs_conf_global);
                if(KHM_FAILED(khc_open_space(parent, schema[i].name, 
                                             KHM_FLAG_CREATE, &h))) {
                    EnterCriticalSection(&cs_conf_global);
                    return KHM_ERROR_INVALID_PARAM;
                }
                EnterCriticalSection(&cs_conf_global);
                thisconf = khc_space_from_handle(h);
                thisconf->schema = schema + (begin + 1);
                thisconf->nSchema = 0;
                state = 1;
                break;

            case 1: /* we are inside a config space, in the values area */
                if(schema[i].type == KC_SPACE) {
                    thisconf->nSchema = i - (begin + 1);
                    if(KHM_FAILED(khcint_load_schema_i(h, schema, i, &i)))
                        return KHM_ERROR_INVALID_PARAM;
                    state = 2;
                } else if(schema[i].type == KC_ENDSPACE) {
                    thisconf->nSchema = i - (begin + 1);
                    end_found = 1;
                    if(end)
                        *end = i;
                    LeaveCriticalSection(&cs_conf_global);
                    khc_close_space(h);
                    EnterCriticalSection(&cs_conf_global);
                }
                break;

            case 2: /* we are inside a config space, in the subspace area */
                if(schema[i].type == KC_SPACE) {
                    if(KHM_FAILED(khcint_load_schema_i(h, schema, i, &i)))
                        return KHM_ERROR_INVALID_PARAM;
                } else if(schema[i].type == KC_ENDSPACE) {
                    end_found = 1;
                    if(end)
                        *end = i;
                    LeaveCriticalSection(&cs_conf_global);
                    khc_close_space(h);
                    EnterCriticalSection(&cs_conf_global);
                } else {
                    return KHM_ERROR_INVALID_PARAM;
                }
                break;

            default:
                /* unreachable */
                return KHM_ERROR_INVALID_PARAM;
        }
        i++;
    }

    return KHM_ERROR_SUCCESS;
}

/* obtains cs_conf_handle/cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_load_schema(khm_handle conf, const kconf_schema * schema)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if(!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if(conf && !khc_is_handle(conf))
        return KHM_ERROR_INVALID_PARAM;

    if(KHM_FAILED(khcint_validate_schema(schema, 0, NULL)))
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_conf_global);
    rv = khcint_load_schema_i(conf, schema, 0, NULL);        
    LeaveCriticalSection(&cs_conf_global);

    return rv;
}

/* obtains cs_conf_handle/cs_conf_global; called with cs_conf_global */
khm_int32 
khcint_unload_schema_i(khm_handle parent, const kconf_schema * schema, 
                       int begin, int * end)
{
    int i;
    int state = 0;
    int end_found = 0;
    kconf_conf_space * thisconf = NULL;
    khm_handle h = NULL;

    i=begin;
    while(!end_found) {
        switch(state) {
            case 0: /* initial.  this record should start a config space */
                LeaveCriticalSection(&cs_conf_global);
                if(KHM_FAILED(khc_open_space(parent, schema[i].name, 0, &h))) {
                    EnterCriticalSection(&cs_conf_global);
                    return KHM_ERROR_INVALID_PARAM;
                }
                EnterCriticalSection(&cs_conf_global);
                thisconf = khc_space_from_handle(h);
                if(thisconf->schema == (schema + (begin + 1))) {
                    thisconf->schema = NULL;
                    thisconf->nSchema = 0;
                }
                state = 1;
                break;

            case 1: /* we are inside a config space, in the values area */
                if(schema[i].type == KC_SPACE) {
                    if(KHM_FAILED(khcint_unload_schema_i(h, schema, i, &i)))
                        return KHM_ERROR_INVALID_PARAM;
                    state = 2;
                } else if(schema[i].type == KC_ENDSPACE) {
                    end_found = 1;
                    if(end)
                        *end = i;
                    LeaveCriticalSection(&cs_conf_global);
                    khc_close_space(h);
                    EnterCriticalSection(&cs_conf_global);
                }
                break;

            case 2: /* we are inside a config space, in the subspace area */
                if(schema[i].type == KC_SPACE) {
                    if(KHM_FAILED(khcint_unload_schema_i(h, schema, i, &i)))
                        return KHM_ERROR_INVALID_PARAM;
                } else if(schema[i].type == KC_ENDSPACE) {
                    end_found = 1;
                    if(end)
                        *end = i;
                    LeaveCriticalSection(&cs_conf_global);
                    khc_close_space(h);
                    EnterCriticalSection(&cs_conf_global);
                } else {
                    return KHM_ERROR_INVALID_PARAM;
                }
                break;

            default:
                /* unreachable */
                return KHM_ERROR_INVALID_PARAM;
        }
        i++;
    }

    return KHM_ERROR_SUCCESS;
}

/* obtains cs_conf_handle/cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_unload_schema(khm_handle conf, const kconf_schema * schema)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if(!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if(conf && !khc_is_handle(conf))
        return KHM_ERROR_INVALID_PARAM;

    if(KHM_FAILED(khcint_validate_schema(schema, 0, NULL)))
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_conf_global);
    rv = khcint_unload_schema_i(conf, schema, 0, NULL);
    LeaveCriticalSection(&cs_conf_global);

    return rv;
}

/* obtaincs cs_conf_handle/cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_enum_subspaces(khm_handle conf,
                   khm_handle prev,
                   khm_handle * next)
{
    kconf_conf_space * s;
    kconf_conf_space * c;
    kconf_conf_space * p;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if (!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if (!khc_is_handle(conf) || next == NULL ||
        (prev != NULL && !khc_is_handle(prev)))
        return KHM_ERROR_INVALID_PARAM;

    s = khc_space_from_handle(conf);

    if(prev == NULL) {
        /* first off, we enumerate all the registry spaces regardless
           of whether the handle is applicable for some registry space
           or not. */

        /* go through the user hive first */
        {
            HKEY hk_conf;

            hk_conf = khcint_space_open_key(s, 0);
            if(hk_conf) {
                wchar_t name[KCONF_MAXCCH_NAME];
                khm_handle h;
                int idx;

                idx = 0;
                while(RegEnumKey(hk_conf, idx, 
                                 name, ARRAYLENGTH(name)) == ERROR_SUCCESS) {
                    wchar_t * tilde;
                    tilde = wcschr(name, L'~');
                    if (tilde)
                        *tilde = 0;
                    if(KHM_SUCCEEDED(khc_open_space(conf, name, 0, &h)))
                        khc_close_space(h);
                    idx++;
                }
            }
        }

        /* go through the machine hive next */
        {
            HKEY hk_conf;

            hk_conf = khcint_space_open_key(s, KCONF_FLAG_MACHINE);
            if(hk_conf) {
                wchar_t name[KCONF_MAXCCH_NAME];
                khm_handle h;
                int idx;

                idx = 0;
                while(RegEnumKey(hk_conf, idx, 
                                 name, ARRAYLENGTH(name)) == ERROR_SUCCESS) {
                    wchar_t * tilde;
                    tilde = wcschr(name, L'~');
                    if (tilde)
                        *tilde = 0;

                    if(KHM_SUCCEEDED(khc_open_space(conf, name, 
                                                    KCONF_FLAG_MACHINE, &h)))
                        khc_close_space(h);
                    idx++;
                }
            }
        }

        /* don't need to go through schema, because that was already
           done when the schema was loaded. */
    }

    /* at last we are now ready to return the results */
    EnterCriticalSection(&cs_conf_global);
    if(prev == NULL) {
        c = TFIRSTCHILD(s);
        rv = KHM_ERROR_SUCCESS;
    } else {
        p = khc_space_from_handle(prev);
        if(TPARENT(p) == s)
            c = LNEXT(p);
        else
            c = NULL;
    }
    LeaveCriticalSection(&cs_conf_global);

    if(prev != NULL)
        khc_close_space(prev);

    if(c) {
        *next = khcint_handle_from_space(c, khc_handle_flags(conf));
        rv = KHM_ERROR_SUCCESS;
    } else {
        *next = NULL;
        rv = KHM_ERROR_NOT_FOUND;
    }

    return rv;
}

/* obtains cs_conf_handle/cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_write_multi_string(khm_handle conf, const wchar_t * value, wchar_t * buf)
{
    size_t cb;
    wchar_t vbuf[KCONF_MAXCCH_STRING];
    wchar_t *tb;
    khm_int32 rv;

    if(!khc_is_config_running())
        return KHM_ERROR_NOT_READY;
    if(!khc_is_handle(conf) || buf == NULL || value == NULL)
        return KHM_ERROR_INVALID_PARAM;

    if(multi_string_to_csv(NULL, &cb, buf) != KHM_ERROR_TOO_LONG)
        return KHM_ERROR_INVALID_PARAM;

    if (cb < sizeof(vbuf))
        tb = vbuf;
    else
        tb = PMALLOC(cb);

    assert(tb != NULL);

    multi_string_to_csv(tb, &cb, buf);
    rv = khc_write_string(conf, value, tb);

    if (tb != vbuf)
        PFREE(tb);
    return rv;
}

/* obtains cs_conf_handle/cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_read_multi_string(khm_handle conf, const wchar_t * value, 
                      wchar_t * buf, khm_size * bufsize)
{
    wchar_t vbuf[KCONF_MAXCCH_STRING];
    wchar_t * tb;
    khm_size cbbuf;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if(!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if(!bufsize)
        return KHM_ERROR_INVALID_PARAM;

    rv = khc_read_string(conf, value, NULL, &cbbuf);
    if(rv != KHM_ERROR_TOO_LONG)
        return rv;

    if (cbbuf < sizeof(vbuf))
        tb = vbuf;
    else
        tb = PMALLOC(cbbuf);

    assert(tb != NULL);

    rv = khc_read_string(conf, value, tb, &cbbuf);

    if(KHM_FAILED(rv))
        goto _exit;

    rv = csv_to_multi_string(buf, bufsize, tb);

 _exit:
    if (tb != vbuf)
        PFREE(tb);

    return rv;
}
