/*
* Copyright (c) 2005 Massachusetts Institute of Technology
* Copyright (c) 2006-2009 Secure Endpoints Inc.
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

DECLARE_ONCE(conf_once);
LONG volatile conf_status = 0;

#if defined(DEBUG) && (defined(KH_BUILD_PRIVATE) || defined(KH_BUILD_SPECIAL))

#include<stdio.h>

static void
khcint_dump_space(kconf_conf_space * sp)
{

    kconf_conf_space * sc;

    _RPTW3(_CRT_WARN, L"c12\t[%s]\t%d\t0x%x\n",
           sp->name,
           (int) sp->refcount,
           (int) sp->flags);

    sc = TFIRSTCHILD(sp);
    while(sc) {

        khcint_dump_space(sc);

        sc = LNEXT(sc);
    }
}

KHMEXP void KHMAPI
khcint_dump_handles(void)
{
    if (khc_is_config_running()) {
        kconf_handle * h, * sh;

        EnterCriticalSection(&cs_conf_global);

        _RPT0 (_CRT_WARN, "c00\t*** Active handles ***\n");
        _RPT0 (_CRT_WARN, "c01\tHandle\tName\tFlags\tRegpath\n");

        h = conf_handles;
        while(h) {
            kconf_conf_space * sp;

            sp = h->space;

            if (!khc_is_handle(h) || sp == NULL) {

                _RPT0(_CRT_WARN, "c02\t!!INVALID HANDLE!!\n");

            } else {

                _RPTW3(_CRT_WARN, L"c02\t0x%p\t[%s]\t0x%x\n",
                       h,
                       sp->name,
                       h->flags);

                sh = khc_shadow(h);

                while(sh) {

                    sp = sh->space;

                    if (!khc_is_handle(sh) || sp == NULL) {

                        _RPT2(_CRT_WARN, "c02\t0x%p:Shadow:0x%p\t[!!INVALID HANDLE!!]\n",
                              h, sh);

                    } else {

                        _RPTW4(_CRT_WARN, L"c02\t0x%p:Shadow:0x%p,[%s]\t0x%x\n",
                               h, sh,
                               sp->name,
                               sh->flags);

                    }

                    sh = khc_shadow(sh);
                }

            }

            h = LNEXT(h);
        }

        _RPT0(_CRT_WARN, "c03\t------  End ---------\n");

        _RPT0(_CRT_WARN, "c10\t*** Active Configuration Spaces ***\n");
        _RPT0(_CRT_WARN, "c11\tReg path\tName\tRefcount\tFlags\tLayers\n");

        khcint_dump_space(conf_root);

        _RPT0(_CRT_WARN, "c13\t------  End ---------\n");

        LeaveCriticalSection(&cs_conf_global);

    } else {
        _RPT0(_CRT_WARN, "c00\t------- KHC Configuration not running -------\n");
    }
}

#endif

/* obtains cs_conf_global */
kconf_handle * 
khcint_handle_from_space(kconf_conf_space * s, khm_int32 flags)
{
    kconf_handle * h;
    int n_added = 0;

    assert (flags & (KCONF_FLAG_USER | KCONF_FLAG_MACHINE | KCONF_FLAG_SCHEMA));

    EnterCriticalSection(&cs_conf_global);

    /* If the COpen() calls failed, we used to remove the relevant
       bits from flags, but this results in the handle not seeing
       those configuration store if they were created later on.  If
       the user specifies that the handle should see a specific store,
       it should see that store even if the store doesn't currently
       exist but will exist in the future. */

    if ((flags & KCONF_FLAG_USER) && KHM_SUCCEEDED(COpen(s,user)))
        n_added++;

    if ((flags & KCONF_FLAG_MACHINE) && KHM_SUCCEEDED(COpen(s,machine)))
        n_added++;

    if ((flags & KCONF_FLAG_SCHEMA) && KHM_SUCCEEDED(COpen(s,schema)))
        n_added++;

    if (n_added == 0) {
        /* Nothing is visible to this configuration handle */
        LeaveCriticalSection(&cs_conf_global);
        return NULL;
    }

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
    LeaveCriticalSection(&cs_conf_global);

    return h;
}

/* obtains cs_conf_global */
void 
khcint_handle_free(kconf_handle * h)
{
    kconf_handle * lower;

    EnterCriticalSection(&cs_conf_global);
#ifdef DEBUG
    /* check if the handle is actually in use */
    {
        kconf_handle * a;
        for (a = conf_handles; a; a = LNEXT(a)) {
            if(h == a)
                break;
        }

        if(a == NULL) {
            assert (FALSE);
            LeaveCriticalSection(&cs_conf_global);
            return;
        }
    }
#endif
    while(h) {
        LDELETE(&conf_handles, h);
        if(h->space) {

            if (khc_is_user_handle(h))
                CClose(h->space, user);

            if (khc_is_machine_handle(h))
                CClose(h->space, machine);

            if (khc_is_schema_handle(h))
                CClose(h->space, schema);

            khcint_space_release(h->space);
            h->space = NULL;
        }
        lower = h->lower;
        h->magic = 0;
        h->flags = 0;
        LPUSH(&conf_free_handles, h);
        h = lower;
    }
    LeaveCriticalSection(&cs_conf_global);
}

/* obains cs_conf_global */
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
    if (s->refcount == 0) {

        kconf_conf_space * p;

        p = TPARENT(s);

        if (p)
            TDELCHILD(p, s);

        khcint_free_space(s);

        if (p)
            khcint_space_release(p);
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
        /* even if the refcount is zero, we shouldn't free a
           configuration space just yet since that doesn't play well
           with the configuration space enumeration mechanism which
           expects the spaces to dangle around if there is a
           corresponding registry key or schema. */

        if (!khc_provider(s, user) && !khc_provider(s, machine) && !khc_provider(s, schema))
            khcint_try_free_space(s);
    }
    assert(l >= 0);

    LeaveCriticalSection(&cs_conf_global);
}

KHMEXP khm_int32 KHMAPI
khc_dup_space(khm_handle vh, khm_handle * pvh)
{
    kconf_handle * h;
    kconf_handle *h2;

    if (!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if (!khc_is_handle(vh) || pvh == NULL)
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_conf_global);

    h = (kconf_handle *) vh;
    h2 = khcint_handle_dup(h);

    LeaveCriticalSection(&cs_conf_global);

    *pvh = (khm_handle) h2;

    return KHM_ERROR_SUCCESS;
}

/* obtains cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_shadow_space(khm_handle upper, khm_handle lower)
{
    kconf_handle * h;

    if(!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if(!khc_is_handle(upper))
        return KHM_ERROR_INVALID_PARAM;

    h = (kconf_handle *) upper;

    EnterCriticalSection(&cs_conf_global);
    if(h->lower) {
        khcint_handle_free(h->lower);
        h->lower = NULL;
    }

    if(khc_is_handle(lower)) {
        kconf_handle * l;
        kconf_handle * lc;

        l = (kconf_handle *) lower;
        lc = khcint_handle_dup(l);
        h->lower = lc;
    }
    LeaveCriticalSection(&cs_conf_global);

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

    assert(TPARENT(r) == NULL);

    TPOPCHILD(r, &c);
    while(c) {
        khcint_free_space(c);
        TPOPCHILD(r, &c);
    }

    CExit(r, user);
    CExit(r, machine);
    CExit(r, schema);

    if(r->name)
        PFREE(r->name);

    PFREE(r);
}

static void
khcint_get_full_path(kconf_conf_space * s, wchar_t * buffer, khm_size cb)
{
    if (TPARENT(s)) {
        khcint_get_full_path(TPARENT(s), buffer, cb);
        StringCbCat(buffer, cb, L"\\");
        StringCbCat(buffer, cb, s->name);
    } else {
        StringCbCopy(buffer, cb, L"");
    }
}

static const khc_provider_interface *
khcint_find_effective_provider(kconf_conf_space * parent, khm_int32 flags, khm_boolean * pis_parent)
{
    kconf_conf_space * p;

    *pis_parent = !!(parent);

    for (p = parent; p; p = TPARENT(p)) {
        if ((flags & KCONF_FLAG_USER) && khc_provider(p, user))
            return khc_provider(p, user);

        if ((flags & KCONF_FLAG_MACHINE) && khc_provider(p, machine))
            return khc_provider(p, machine);

        if ((flags & KCONF_FLAG_SCHEMA) && khc_provider(p, schema))
            return khc_provider(p, schema);

        *pis_parent = FALSE;
    }

    if (flags & (KCONF_FLAG_USER | KCONF_FLAG_MACHINE))
        return &khc_reg_provider;
    else
        return &khc_schema_provider;
}

/* Called with cs_conf_global held */
static khm_int32
khcint_initialize_providers_for_space(kconf_conf_space * space, kconf_conf_space * parent,
                                      khm_int32 flags)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;
    khm_int32 create_user = ((flags & KCONF_FLAG_USER)? (flags & KHM_FLAG_CREATE) : 0);
    khm_int32 create_mach = ((flags & KCONF_FLAG_MACHINE) && !create_user ? (flags & KHM_FLAG_CREATE) : 0);
    khm_int32 create_schm = ((flags & KCONF_FLAG_SCHEMA) && !create_user && !create_mach? (flags & KHM_FLAG_CREATE) : 0);

    khm_boolean is_parent = FALSE;
    const khc_provider_interface * pint = NULL;
    wchar_t cpath[KCONF_MAXCCH_PATH];

    khcint_get_full_path(space, cpath, sizeof(cpath));

    if (!khc_provider(space, user)) {
        pint = NULL;
        if (parent && khc_provider(parent, user)) {
            CCreate(parent, user, space->name,
                    KCONF_FLAG_USER | create_user);
        } else if ((pint = khcint_find_effective_provider(parent, KCONF_FLAG_USER, &is_parent)) != NULL) {
            khc_provider(space, user) = pint;
            if (KHM_FAILED(CInit(space, user, cpath,
                                 KCONF_FLAG_USER|create_user, NULL)))
                khc_provider(space, user) = NULL;
        }

        if (!khc_provider(space, user) &&
            (flags & KCONF_FLAG_TRYDEFATULTS) &&
            pint != &khc_reg_provider) {
            khc_provider(space, user) = &khc_reg_provider;
            if (KHM_FAILED(CInit(space, user, cpath, KCONF_FLAG_USER, NULL)))
                khc_provider(space, user) = NULL;
        }
    }

    if (!khc_provider(space, machine)) {
        pint = NULL;
        if (parent && khc_provider(parent, machine)) {
            CCreate(parent, machine, space->name,
                    KCONF_FLAG_MACHINE | create_mach);
        } else if ((pint = khcint_find_effective_provider(parent, KCONF_FLAG_MACHINE, &is_parent)) != NULL) {
            khc_provider(space, machine) = pint;
            if (KHM_FAILED(CInit(space, machine, cpath,
                                 KCONF_FLAG_MACHINE|create_mach, NULL)))
                khc_provider(space, machine) = NULL;
        }

        if (!khc_provider(space, machine) &&
            (flags & KCONF_FLAG_TRYDEFATULTS) &&
            pint != &khc_reg_provider) {
            khc_provider(space, machine) = &khc_reg_provider;
            if (KHM_FAILED(CInit(space, machine, cpath, KCONF_FLAG_MACHINE, NULL)))
                khc_provider(space, machine) = NULL;
        }
    }

    if (!khc_provider(space, schema)) {
        pint = NULL;
        if (parent && khc_provider(parent, schema)) {
            CCreate(parent, schema, space->name,
                    KCONF_FLAG_SCHEMA | create_schm);
        } else if ((pint = khcint_find_effective_provider(parent, KCONF_FLAG_SCHEMA, &is_parent)) != NULL) {
            khc_provider(space, schema) = pint;
            if (KHM_FAILED(CInit(space, schema, cpath,
                                 KCONF_FLAG_SCHEMA|create_schm, NULL)))
                khc_provider(space, schema) = NULL;
        }
    }

    if (khc_provider(space, user))
        space->flags &= ~KCONF_SPACE_FLAG_DELETE_U;

    if (khc_provider(space, machine))
        space->flags &= ~KCONF_SPACE_FLAG_DELETE_M;

    if (khc_provider(space, user) ||
        khc_provider(space, machine) ||
        khc_provider(space, schema))

        return KHM_ERROR_SUCCESS;

    else
        return KHM_ERROR_NOT_FOUND;
}

/*! \addtogroup kconf
  @{

  \page kconf_sp_life Life and times of a configuration space

  \section kconf_sp_life_0 In the beginning

  The root configuration space ( ::conf_root ) is created when the
  configuration system is initialized in init_kconf().

  Each newly created configuration space is :

  - Added as a child to the parent configuration space.

  - Providers are initialized using
    khcint_initialize_providers_for_space().

  \section kconf_sp_life_1 Initializing providers

  If the configuration space was added using khc_mount_provider(),
  then the providers are already known.  Otherwise, a configuration
  node inherits the parent's providers for the configuration stores
  (user, machine and schema) for which there is no known provider.

  If ::KHM_FLAG_CREATE was specified during the opening of a
  configuration space, then:

  - For the first store (user,machine or schema) that the caller
    requests, an effective provider is located.  An effective provider
    is the one that has been assigned to this space and store or the
    provider associated with the same store for the closest ancestor.

    I.e.: A\\B\\C is being created with flags ::KHM_FLAG_CREATE,
    ::KCONFG_FLAG_MACHINE.  If there's no MACHINE provider for C, then
    a machine providers for B and A are looked up in turn.  The first
    found will be the provider for C.

  - Once the effective provider is located, and is the provider for
    the parent space, then the \a create method of the provider is
    called for the parent space to create the child space.

  - If an effective provider is located and is NOT the provider for
    the parent, then the node is initialized by setting the \a
    provider member of the new space to the ::khc_provider_interface
    structure and calling the \a init method with a \a NULL context
    and KHM_FLAG_CREATE|(store flags).

  - Afterwards, the remaining stores are opened as if no
    ::KHM_FLAG_CREATE was specified.

  One exception to the above steps is when no effective provider can
  be located for a store (becase the root configuration space does not
  have a provider for that store).  In this case, if the store is user
  or machine, the effective provider is always ::khc_reg_provider.
  The provider for schema is ::khc_schema_provider.

  If the ::KHM_FLAG_CREATE flag was not specified, then each of the
  parent providers will be called in turn invoking the
  khc_provider_interface::create() method to try and open the child
  configuration stores.

  @}
 */

/* obtains cs_conf_global */
khm_int32
khcint_open_space(kconf_conf_space * parent, 
                  const wchar_t * sname, size_t n_sname, 
                  khm_int32 flags, kconf_conf_space **result)
{
    kconf_conf_space * p;
    kconf_conf_space * c;
    wchar_t buf[KCONF_MAXCCH_NAME];

    p = ((parent) ? parent : conf_root);

    if (n_sname == 0 || FAILED(StringCchCopyN(buf, ARRAYLENGTH(buf), sname, n_sname)))
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_conf_global);
    for (c = TFIRSTCHILD(p); c; c = LNEXT(c))
        if(c->name && !wcscmp(c->name, buf))
            break;

    if(c) {
        if ((flags & KCONF_FLAG_NOINIT) ||
            KHM_SUCCEEDED(khcint_initialize_providers_for_space(c, p, flags)))
            *result = c;
        else
            *result = NULL;

        if (*result)
            khcint_space_hold(*result);
    }
    LeaveCriticalSection(&cs_conf_global);
    
    if (c)
        return (*result)? KHM_ERROR_SUCCESS : KHM_ERROR_NOT_FOUND;

    c = khcint_create_empty_space();
    
    /*SAFE: buf: is of known length < KCONF_MAXCCH_NAME */
    c->name = PWCSDUP(buf);

    EnterCriticalSection(&cs_conf_global);
    khcint_space_hold(c);
    TADDCHILD(p,c);

    if (!(flags & KCONF_FLAG_NOINIT) &&
        KHM_FAILED(khcint_initialize_providers_for_space(c, p, flags))) {
        TDELCHILD(p, c);
        khcint_free_space(c);
        c = NULL;
    } else {
        khcint_space_hold(p);
    }
    LeaveCriticalSection(&cs_conf_global);

    *result = c;
    return (c)? KHM_ERROR_SUCCESS : KHM_ERROR_NOT_FOUND;
}

/* obtains cs_conf_global */
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

    if (!result || !khc_is_any(parent) ||
        (cspace && FAILED(StringCbLength(cspace, KCONF_MAXCB_PATH, &cbsize)))) {
        return KHM_ERROR_INVALID_PARAM;
    }

    /* if none of these flags are specified, make it seem like all of
       them were */
    if ((flags & (KCONF_FLAG_USER |
                  KCONF_FLAG_MACHINE |
                  KCONF_FLAG_SCHEMA)) == 0)
        flags |= KCONF_FLAG_USER | KCONF_FLAG_MACHINE | KCONF_FLAG_SCHEMA;

    p = khc_space_from_any(parent);

    khcint_space_hold(p);

    if(cspace == NULL) {
        /* If space is NULL, the caller wants a new handle to the same
           configuration space with different options. */
        khcint_initialize_providers_for_space(p, TPARENT(p), flags);
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
        } else if (*result == NULL) {
            /* We may get here because although the configuration
               space exists, the configuration stores that were
               requested in 'flags' were not available. */
            rv = KHM_ERROR_NOT_FOUND;
        }
    } else {
        *result = NULL;
    }

    if (c)
        khcint_space_release(c);

    return rv;
}

/* obtains cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_close_space(khm_handle csp)
{
    if(!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if(!khc_is_handle(csp)) {
        return KHM_ERROR_INVALID_PARAM;
    }

    khcint_handle_free((kconf_handle *) csp);
    return KHM_ERROR_SUCCESS;
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
                               khm_int32 add_flags,
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
                                     KCONF_FLAG_TRAILINGVALUE |
                                     (pconf?khc_handle_flags(pconf):0) |
                                     add_flags, 
                                     ppconf)))
            return KHM_ERROR_INVALID_PARAM;

        *pvalue = ++value;
    } else {
        *pvalue = path;
        if (pconf != NULL &&
            (khc_handle_flags(pconf)|add_flags) == khc_handle_flags(pconf)) {
            *ppconf = pconf;
        } else {
            if (KHM_FAILED(khc_open_space(pconf, NULL,
                                          ((pconf)?khc_handle_flags(pconf):0) |
                                          add_flags,
                                          ppconf)))
                return KHM_ERROR_INVALID_PARAM;
        }
    }

    {
        size_t cch;

        if (FAILED(StringCchLength(*pvalue, KCONF_MAXCCH_NAME, &cch)))
            return KHM_ERROR_INVALID_PARAM;
    }

    return (*ppconf != NULL)? KHM_ERROR_SUCCESS : KHM_ERROR_NOT_FOUND;
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
    khm_int32 rv;
    khm_int32 vtype = expected_type;

    rv = khcint_get_parent_config_space(pconf, pvalue, 0, &conf, &value);
    if (KHM_FAILED(rv))
        return rv;

    assert(khc_is_handle(conf));

    c = khc_space_from_handle(conf);

    EnterCriticalSection(&cs_conf_global);
    assert(khc_is_space(c));

    rv = KHM_ERROR_NOT_FOUND;

    do {
        if (khc_is_user_handle(conf) &&
            khc_provider(c, user) &&
            (rv = CReadValue(c, user, value, &vtype, buf, bufsize)) != KHM_ERROR_NOT_FOUND &&
            rv != KHM_ERROR_NOT_READY)

            break;

        if (khc_is_machine_handle(conf) &&
            khc_provider(c, machine) &&
            (rv = CReadValue(c, machine, value, &vtype, buf, bufsize)) != KHM_ERROR_NOT_FOUND &&
            rv != KHM_ERROR_NOT_READY)

            break;

        if (khc_is_schema_handle(conf) &&
            khc_provider(c, schema) &&
            (rv = CReadValue(c, schema, value, &vtype, buf, bufsize)) != KHM_ERROR_NOT_FOUND &&
            rv != KHM_ERROR_NOT_READY)

            break;

    } while (FALSE);

    LeaveCriticalSection(&cs_conf_global);

    if(conf != pconf && conf != NULL)
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

    do {
        rv = khcint_read_value_from_cspace(pconf, pvalue, expected_type, buf, bufsize);
        if (rv != KHM_ERROR_NOT_FOUND && rv != KHM_ERROR_NOT_READY)
            break;

        if (khc_is_shadowed(pconf))
            pconf = khc_shadow(pconf);
        else
            break;
    } while(pconf);

    return rv;
}

/* obtains cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_read_string(khm_handle pconf, 
                const wchar_t * pvalue, 
                wchar_t * buf, 
                khm_size * bufsize) 
{
    return khcint_read_value(pconf, pvalue, KC_STRING, buf, bufsize);
}

/* obtains cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_read_int32(khm_handle pconf, const wchar_t * pvalue, khm_int32 * buf)
{
    khm_size cb = sizeof(khm_int32);
    return khcint_read_value(pconf, pvalue, KC_INT32, buf, &cb);
}

/* obtains cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_read_int64(khm_handle pconf, const wchar_t * pvalue, khm_int64 * buf)
{
    khm_size cb = sizeof(khm_int64);
    return khcint_read_value(pconf, pvalue, KC_INT64, buf, &cb);
}

/* obtaincs cs_conf_global */
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

        do {
            rv = khcint_read_value(pconf, pvalue, type, b_exist, &b_existsize);
            if (rv == KHM_ERROR_TOO_LONG) {
                if (b_exist != static_buf)
                    PFREE(b_exist);
                b_exist = PMALLOC(b_existsize);
            }
        } while (rv == KHM_ERROR_TOO_LONG);

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

    rv = khcint_get_parent_config_space(pconf, pvalue, KHM_FLAG_CREATE, &conf, &value);
    if (KHM_FAILED(rv))
        return rv;

    EnterCriticalSection(&cs_conf_global);
    c = khc_space_from_handle(conf);

    if (khc_is_user_handle(conf))
        rv = CWriteValue(c, user, value, type, buf, bufsize);
    else if (khc_is_machine_handle(conf))
        rv = CWriteValue(c, machine, value, type, buf, bufsize);
    else
        rv = KHM_ERROR_INVALID_PARAM;
    LeaveCriticalSection(&cs_conf_global);

    if (conf && conf != pconf)
        khc_close_space(conf);

    return rv;
}


/* obtains cs_conf_global */
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

/* obtaincs cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_write_int32(khm_handle pconf, 
                const wchar_t * pvalue, 
                khm_int32 val) 
{
    return khcint_write_value(pconf, pvalue, KC_INT32, &val, sizeof(val));
}

/* obtains cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_write_int64(khm_handle pconf, const wchar_t * pvalue, khm_int64 val)
{
    return khcint_write_value(pconf, pvalue, KC_INT64, &val, sizeof(val));
}

/* obtains cs_conf_global */
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
        if(buf && *bufsize > sizeof(wchar_t)) {
            buf[0] = L'\0';
            *bufsize = sizeof(wchar_t);
        } else {
            *bufsize = sizeof(wchar_t);
            rv = KHM_ERROR_TOO_LONG;
        }
        assert(FALSE);
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

/* obtains cs_conf_global */
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
    kconf_conf_space * c;
    khm_int32 type = KC_NONE;

    if(!khc_is_config_running())
        return KC_NONE;

    if(!khc_is_handle(conf))
        return KC_NONE;

    EnterCriticalSection(&cs_conf_global);
    c = khc_space_from_handle(conf);

    /* In this case, CReadValue() can only return KHM_ERROR_SUCCESS,
       KHM_ERROR_NOT_FOUND or KHM_ERROR_NOT_READY.  For all failure
       cases, we fail over to the next configuration store. */

    ((khc_is_user_handle(conf) &&
      KHM_SUCCEEDED(CReadValue(c, user, value, &type, NULL, NULL))) ||

     (khc_is_machine_handle(conf) &&
      KHM_SUCCEEDED(CReadValue(c, machine, value, &type, NULL, NULL))) ||

     (khc_is_schema_handle(conf) &&
      KHM_SUCCEEDED(CReadValue(c, schema, value, &type, NULL, NULL))));

    LeaveCriticalSection(&cs_conf_global);

    return type;
}

KHMEXP khm_int32 KHMAPI
khc_get_last_write_time(khm_handle conf, khm_int32 flags, FILETIME * plast_w_time)
{
    khm_int32 rv = KHM_ERROR_NOT_FOUND;
    kconf_conf_space * c;

    if (!khc_is_config_running() || !khc_is_handle(conf) || plast_w_time == NULL)
        return KHM_ERROR_INVALID_PARAM;

    if (flags == 0)
        flags = KCONF_FLAG_MACHINE | KCONF_FLAG_USER;

    EnterCriticalSection(&cs_conf_global);

    do {

        c = khc_space_from_handle(conf);

        if ((khc_is_user_handle(conf) &&
             (flags & KCONF_FLAG_USER) &&
             KHM_SUCCEEDED(CGetMTime(c, user, plast_w_time))) ||

            (khc_is_machine_handle(conf) &&
             (flags & KCONF_FLAG_MACHINE) &&
             KHM_SUCCEEDED(CGetMTime(c, machine, plast_w_time))) ||

            (khc_is_schema_handle(conf) &&
             (flags & KCONF_FLAG_SCHEMA) &&
             KHM_SUCCEEDED(CGetMTime(c, schema, plast_w_time)))) {

            rv = KHM_ERROR_SUCCESS;
            break;
        }

        if (khc_is_shadowed(conf))
            conf = khc_shadow(conf);
        else
            break;

    } while (conf);

    LeaveCriticalSection(&cs_conf_global);

    return rv;
}

/* obtains cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_value_exists(khm_handle conf, const wchar_t * value)
{
    kconf_conf_space * c;
    khm_int32 rv = 0;

    if(!khc_is_config_running() || !khc_is_handle(conf))
        return 0;

    EnterCriticalSection(&cs_conf_global);

    do {
        c = khc_space_from_handle(conf);

        if (khc_is_user_handle(conf) &&
            KHM_SUCCEEDED(CReadValue(c, user, value, NULL, NULL, NULL)))
            rv |= KCONF_FLAG_USER;

        if (khc_is_machine_handle(conf) &&
            KHM_SUCCEEDED(CReadValue(c, machine, value, NULL, NULL, NULL)))
            rv |= KCONF_FLAG_MACHINE;

        if (khc_is_schema_handle(conf) &&
            KHM_SUCCEEDED(CReadValue(c, schema, value, NULL, NULL, NULL)))
            rv |= KCONF_FLAG_SCHEMA;

        if (khc_is_shadowed(conf))
            conf = khc_shadow(conf);
        else
            break;
    } while (conf);

    LeaveCriticalSection(&cs_conf_global);

    return rv;
}

/* obtains cs_conf_global */
KHMEXP khm_int32 KHMAPI
khc_remove_value(khm_handle pconf, const wchar_t * pvalue, khm_int32 flags)
{
    kconf_conf_space * c;
    khm_int32 rvu = KHM_ERROR_SUCCESS;
    khm_int32 rvm = KHM_ERROR_SUCCESS;
    khm_int32 rvp;
    const wchar_t * value;
    khm_handle conf = NULL;

    if (!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if (!khc_is_handle(pconf) || (flags & ~(KCONF_FLAG_USER|KCONF_FLAG_MACHINE)))
        return KHM_ERROR_INVALID_PARAM;

    if (flags == 0)
        flags = KCONF_FLAG_USER | KCONF_FLAG_MACHINE;

    if ((khc_handle_flags(pconf) & flags) == 0)
        return KHM_ERROR_NOT_FOUND;

    rvp = khcint_get_parent_config_space(pconf, pvalue, 0, &conf, &value);
    if (KHM_FAILED(rvp))
        return rvp;

    EnterCriticalSection(&cs_conf_global);

    c = khc_space_from_handle(conf);

    if (khc_is_user_handle(conf) && (flags & KCONF_FLAG_USER))

        rvu = CRemoveValue(c, user, value);

    if (khc_is_machine_handle(conf) && (flags & KCONF_FLAG_MACHINE))

        rvm = CRemoveValue(c, machine, value);

    LeaveCriticalSection(&cs_conf_global);

    if (conf && conf != pconf)
        khc_close_space(conf);

    return
        (rvu == KHM_ERROR_SUCCESS || rvu == KHM_ERROR_NO_PROVIDER)?

        ((rvm == KHM_ERROR_SUCCESS || rvm == KHM_ERROR_NO_PROVIDER)? KHM_ERROR_SUCCESS :
         ((flags & KCONF_FLAG_USER)? KHM_ERROR_PARTIAL : rvm)) :

        ((rvm == KHM_ERROR_SUCCESS || rvm == KHM_ERROR_NO_PROVIDER)?
         ((flags & KCONF_FLAG_MACHINE)? KHM_ERROR_PARTIAL : rvu) :
         rvu);
}

/* called with cs_conf_global held */
khm_int32
khcint_remove_space(kconf_conf_space * c, khm_int32 flags)
{
    kconf_conf_space * cc;
    kconf_conf_space * cn;
    kconf_conf_space * p;
    khm_boolean free_c = FALSE;
    khm_int32 rv;

    p = TPARENT(c);

    /* We don't allow deleting top level keys.  They are
       predefined. */
    assert(p);

    if (!p)
        return KHM_ERROR_INVALID_OPERATION;

    khcint_space_hold(c);

    for (cc = TFIRSTCHILD(c); cc; cc = cn) {
        cn = LNEXT(cc);
        khcint_remove_space(cc, flags);
    }

    if (flags & KCONF_SPACE_FLAG_DELETE_U) {
        rv = CRemove(c, user);
        CExit(c, user);
        khc_provider(c, user) = NULL;
        c->flags |= KCONF_SPACE_FLAG_DELETE_U;
    }

    if (flags & KCONF_SPACE_FLAG_DELETE_M) {
        rv = CRemove(c, machine);
        CExit(c, machine);
        khc_provider(c, machine) = NULL;
        c->flags |= KCONF_SPACE_FLAG_DELETE_M;
    }

    khcint_space_release(c);

    return KHM_ERROR_SUCCESS;
}

/* obtains cs_conf_global */
KHMEXP khm_int32 KHMAPI
khc_remove_space(khm_handle conf)
{
    kconf_conf_space * c;
    khm_int32 rv = KHM_ERROR_SUCCESS;
    khm_int32 flags = 0;

    if (!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if (!khc_is_handle(conf))
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_conf_global);

    c = khc_space_from_handle(conf);

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

/* obtains cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_load_schema(khm_handle conf, const kconf_schema * schema)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if(!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if(conf && !khc_is_handle(conf))
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_conf_global);
    rv = khcint_load_schema(conf, schema, NULL);
    LeaveCriticalSection(&cs_conf_global);

    return rv;
}

/* obtains cs_conf_global */
KHMEXP khm_int32 KHMAPI 
khc_unload_schema(khm_handle conf, const kconf_schema * schema)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if(!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if(conf && !khc_is_handle(conf))
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_conf_global);
    rv = khcint_unload_schema(conf, schema);
    LeaveCriticalSection(&cs_conf_global);

    return rv;
}

/* obtaincs cs_conf_global */
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

    *next = NULL;

    EnterCriticalSection(&cs_conf_global);

    s = khc_space_from_handle(conf);

    if(prev == NULL) {
        CBeginEnum(s, user);
        CBeginEnum(s, machine);
        CBeginEnum(s, schema);
    }

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

    while (c && *next == NULL) {
        *next = khcint_handle_from_space(c, khc_handle_flags(conf));
        if (*next == NULL) {
            c = LNEXT(c);
        }
    }
    LeaveCriticalSection(&cs_conf_global);

    if(prev != NULL) {
        khc_close_space(prev);
        prev = NULL;
    }

    rv = ((*next != NULL)? KHM_ERROR_SUCCESS : KHM_ERROR_NOT_FOUND);

    return rv;
}

/* obtains cs_conf_global */
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

/* obtains cs_conf_global */
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

static khm_boolean
khcint_set_provider_for_space(kconf_conf_space * s, khm_int32 flags,
                              const khc_provider_interface * provider,
                              void * context)
{
    khm_boolean added = FALSE;

    if (flags & KCONF_FLAG_NOINIT) {

        if ((flags & KCONF_FLAG_USER) &&
            khc_provider(s, user) && khc_provider(s, user) != provider) {
            CExit(s, user);
            khc_provider(s, user) = NULL;
        }

        if ((flags & KCONF_FLAG_MACHINE) &&
            khc_provider(s, machine) && khc_provider(s, machine) != provider) {
            CExit(s, machine);
            khc_provider(s, machine) = NULL;
        }

        if ((flags & KCONF_FLAG_SCHEMA) &&
            khc_provider(s, schema) && khc_provider(s, schema) != provider) {
            CExit(s, schema);
            khc_provider(s, schema) = NULL;
        }

        khcint_initialize_providers_for_space(s, TPARENT(s), (flags & ~KHM_FLAG_CREATE));

    } else {
        wchar_t cpath[KCONF_MAXCCH_PATH];
        khm_int32 create = (flags & KHM_FLAG_CREATE);

        khcint_get_full_path(s, cpath, sizeof(cpath));

        if (flags & KCONF_FLAG_USER) {
            if (khc_provider(s, user)) {
                CExit(s, user);
                khc_provider(s, user) = NULL;
            }
            khc_provider(s, user) = provider;
            if (KHM_FAILED(CInit(s, user, cpath, KCONF_FLAG_USER|create, context))) {
                khc_provider(s, user) = NULL;
            } else {
                added = TRUE;
            }
        }

        if (flags & KCONF_FLAG_MACHINE) {
            if (khc_provider(s, machine)) {
                CExit(s, machine);
                khc_provider(s, machine) = NULL;
            }
            khc_provider(s, machine) = provider;
            if (KHM_FAILED(CInit(s, machine, cpath, KCONF_FLAG_MACHINE|create, context))) {
                khc_provider(s, machine) = NULL;
            } else {
                added = TRUE;
            }
        }

        if (flags & KCONF_FLAG_SCHEMA) {
            if (khc_provider(s, schema)) {
                CExit(s, schema);
                khc_provider(s, schema) = NULL;
            }
            khc_provider(s, schema) = provider;
            if (KHM_FAILED(CInit(s, schema, cpath, KCONF_FLAG_SCHEMA|create, context))) {
                khc_provider(s, schema) = NULL;
            } else {
                added = TRUE;
            }
        }
    }

    if (flags & KCONF_FLAG_RECURSIVE) {
        kconf_conf_space * c;

        for (c = TFIRSTCHILD(s); c; c = TNEXTSIBLING(c)) {
            khcint_set_provider_for_space(c, flags|KCONF_FLAG_NOINIT, provider, NULL);
        }
    }

    return added;
}

KHMEXP khm_int32 KHMAPI
khc_mount_provider(khm_handle conf, const wchar_t * name, khm_int32 flags,
                   const khc_provider_interface * provider,
                   void * context, khm_handle *ret_conf)
{
    kconf_conf_space * s;
    kconf_conf_space * p;
    khm_boolean added = FALSE;
    khm_boolean new_config_space = FALSE;

    if (!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if (!khc_is_any(conf) || name == NULL || provider == NULL ||
        !(flags & (KCONF_FLAG_USER|KCONF_FLAG_MACHINE|KCONF_FLAG_SCHEMA)))
        return KHM_ERROR_INVALID_PARAM;

    {
        khm_size cch;

        if (FAILED(StringCchLength(name, KCONF_MAXCCH_NAME, &cch)) ||
            wcschr(name, L'\\') || wcschr(name, L'/'))
            return KHM_ERROR_INVALID_PARAM;
    }

    EnterCriticalSection(&cs_conf_global);
    p = khc_space_from_any(conf);

    if (KHM_FAILED(khcint_open_space(p, name, KCONF_MAXCCH_NAME, KCONF_FLAG_NOINIT, &s))) {
        new_config_space = TRUE;

        s = khcint_create_empty_space();
        s->name = PWCSDUP(name);
        khcint_space_hold(s);
        TADDCHILD(p, s);
        khcint_space_hold(p);
    }

    if (khc_is_handle(conf))
        flags &=
            ((khc_handle_flags(conf) & (KCONF_FLAG_USER|
                                        KCONF_FLAG_MACHINE|
                                        KCONF_FLAG_SCHEMA)) |
             (KCONF_FLAG_RECURSIVE | KHM_FLAG_CREATE));

    added = khcint_set_provider_for_space(s, flags, provider, context);

    if (new_config_space) {
        if (KHM_FAILED(khcint_initialize_providers_for_space(s, p, 0))) {
            TDELCHILD(p, s);
            khcint_free_space(s);
            khcint_space_release(p);
            s = NULL;
            assert(!added);
        }
    }

    flags &= ~KHM_FLAG_CREATE;

    if (s && added && ret_conf)
        *ret_conf = khcint_handle_from_space(s, flags);

    if (s)
        khcint_space_release(s);
    LeaveCriticalSection(&cs_conf_global);

    return (added)? KHM_ERROR_SUCCESS: KHM_ERROR_NO_PROVIDER;
}

KHMEXP khm_int32 KHMAPI
khc_unmount_provider(khm_handle conf, const khc_provider_interface * provider,
                     khm_int32 flags)
{
    kconf_conf_space * s;
    khm_boolean removed = FALSE;

    if (!khc_is_config_running())
        return KHM_ERROR_NOT_READY;

    if (!khc_is_any(conf))
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_conf_global);
    s = khc_space_from_any(conf);
    khcint_space_hold(s);

    if ((!khc_is_handle(conf) || khc_is_user_handle(conf)) && (flags & KCONF_FLAG_USER)) {
        if (khc_provider(s, user) && khc_provider(s, user) == provider) {
            CExit(s, user);
            khc_provider(s, user) = NULL;
            removed = TRUE;
        }
    }

    if ((!khc_is_handle(conf) || khc_is_machine_handle(conf)) && (flags & KCONF_FLAG_MACHINE)) {
        if (khc_provider(s, machine) && khc_provider(s, machine) == provider) {
            CExit(s, machine);
            khc_provider(s, machine) = NULL;
            removed = TRUE;
        }
    }

    if ((!khc_is_handle(conf) || khc_is_schema_handle(conf)) && (flags & KCONF_FLAG_SCHEMA)) {
        if (khc_provider(s, schema) && khc_provider(s, schema) == provider) {
            CExit(s, schema);
            khc_provider(s, schema) = NULL;
            removed = TRUE;
        }
    }

    if (flags & KCONF_FLAG_RECURSIVE) {
        kconf_conf_space * c;
        kconf_conf_space * nc;

        for (c = TFIRSTCHILD(s); c; c = nc) {
            nc = LNEXT(c);

            if (KHM_SUCCEEDED(khc_unmount_provider(c, provider, flags)))
                removed = TRUE;
        }
    }

    khcint_initialize_providers_for_space(s, TPARENT(s), KCONF_FLAG_TRYDEFATULTS);

    khcint_space_release(s);

    LeaveCriticalSection(&cs_conf_global);

    return (removed)? KHM_ERROR_SUCCESS: KHM_ERROR_NO_PROVIDER;
}

void init_kconf(void)
{
    if(InitializeOnce(&conf_once)) {
        /* we are the first */
        InitializeCriticalSection(&cs_conf_global);

        EnterCriticalSection(&cs_conf_global);
        conf_root = khcint_create_empty_space();
        conf_root->name = PWCSDUP(L"Root");
        conf_root->refcount++;
        conf_root->user.provider = NULL;
        conf_root->machine.provider = NULL;
        conf_root->schema.provider = NULL;
        conf_status = 1;

        khcint_initialize_providers_for_space(conf_root, NULL, 0);

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
        LeaveCriticalSection(&cs_conf_global);
        DeleteCriticalSection(&cs_conf_global);
    }
}

