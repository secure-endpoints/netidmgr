/*
 * Copyright (c) 2007 Massachusetts Institute of Technology
 * Copyright (c) 2007-2010 Secure Endpoints Inc.
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

#include "kcreddbinternal.h"
#include<kmm.h>
#include<assert.h>

CRITICAL_SECTION cs_identpro;

/*!< \brief The list of identity providers

  The first element in this list is the default provider.
 */
static struct {
    QDCL(kcdb_identpro_i);
} id_providers;

khm_size n_id_providers;

#define DEFAULT_PROVIDER QTOP(&id_providers)

static kcdb_identpro_i * deleted_id_providers;

/* forward declarations */
static void
identpro_check_and_deleteL(kcdb_identpro_i * p);

void
kcdbint_identpro_init(void)
{
    InitializeCriticalSection(&cs_identpro);
    QINIT(&id_providers);
    deleted_id_providers = NULL;
    n_id_providers = 0;
}

void
kcdbint_identpro_exit(void)
{
    /* TODO: Check if the provider lists are empty */
    DeleteCriticalSection(&cs_identpro);
}

static void
identpro_hold(kcdb_identpro_i * p)
{
    if (!kcdb_is_identpro(p))
        return;

    EnterCriticalSection(&cs_identpro);
    if (kcdb_is_identpro(p))
        p->refcount++;
    LeaveCriticalSection(&cs_identpro);
}

static void
identpro_release(kcdb_identpro_i * p)
{
    if (!kcdb_is_identpro(p))
        return;

    EnterCriticalSection(&cs_identpro);
    p->refcount--;
#ifdef DEBUG
    assert(p->refcount >= 0);
#endif
    if (p->refcount == 0 &&
        (p->flags & KCDB_IDENTPRO_FLAG_DELETED))
        identpro_check_and_deleteL(p);
    LeaveCriticalSection(&cs_identpro);
}

static kcdb_identpro_i *
identpro_find_by_name(const wchar_t * name)
{
    kcdb_identpro_i * p;

    EnterCriticalSection(&cs_identpro);
    for (p = QTOP(&id_providers); p; p = QNEXT(p)) {
        if (!wcscmp(p->name, name))
            break;
    }

    if (p)
        identpro_hold(p);
    LeaveCriticalSection(&cs_identpro);

    return p;
}

static kcdb_identpro_i *
identpro_find_by_plugin(kmm_plugin plugin)
{
    kcdb_identpro_i * p;

    EnterCriticalSection(&cs_identpro);
    for (p = QTOP(&id_providers); p; p = QNEXT(p)) {
        if (p->plugin == plugin)
            break;
    }

    if (p)
        identpro_hold(p);
    LeaveCriticalSection(&cs_identpro);

    return p;
}

/* called with cs_identpro held */
static void
identpro_check_and_set_default_provider(void)
{
    wchar_t * p_order = NULL;
    khm_size cb;
    khm_handle csp_kcdb = NULL;
    khm_int32 rv;
    kcdb_identpro_i * p = NULL;
    wchar_t * e;

    if (KHM_SUCCEEDED(khc_open_space(NULL, L"KCDB", KHM_PERM_READ, &csp_kcdb))) {
        rv = khc_read_multi_string(csp_kcdb, L"IdentityProviderOrder", NULL, &cb);
        if (rv == KHM_ERROR_TOO_LONG && cb > sizeof(wchar_t) * 2) {
            p_order = PMALLOC(cb);
            rv = khc_read_multi_string(csp_kcdb, L"IdentityProviderOrder", p_order, &cb);
            if (KHM_FAILED(rv)) {
                PFREE(p_order);
                p_order = NULL;
            }
        }

        khc_close_space(csp_kcdb);
        csp_kcdb = NULL;
    }

    for (e = p_order;
         e && e[0];
         e = multi_string_next(e)) {

        p = identpro_find_by_name(e);
        if (p)
            break;
    }

    if (p != NULL) {
        if (DEFAULT_PROVIDER != p) {
            QDEL(&id_providers, p);
            QPUSH(&id_providers, p);
#ifdef DEBUG
            assert(DEFAULT_PROVIDER == p);
#endif
            kcdbint_identpro_post_message(KCDB_OP_NEW_DEFAULT, DEFAULT_PROVIDER);
        }
    }

    if (p)
        identpro_release(p);
    if (p_order)
        PFREE(p_order);
}

static kcdb_identpro_i *
identpro_create_with_plugin(kmm_plugin plugin, khm_handle sub)
{
    kmm_plugin_info pi;
    kcdb_identpro_i * p;
    size_t len;

    ZeroMemory(&pi, sizeof(pi));
    if (KHM_FAILED(kmm_get_plugin_info_i(plugin, &pi)))
        return NULL;

    if (pi.reg.type != KHM_PITYPE_IDENT) {
        kmm_release_plugin_info_i(&pi);
#ifdef DEBUG
        assert(FALSE);
#endif
        return NULL;
    }

    if (FAILED(StringCbLength(pi.reg.name, KMM_MAXCB_NAME, &len)))
        return NULL;
    len += sizeof(wchar_t);

    /* we need to check if the plug-in is already there, both in the
       current and the deleted lists */
    EnterCriticalSection(&cs_identpro);
    for (p = QTOP(&id_providers); p; p = QNEXT(p)) {
        if (p->plugin == plugin)
            break;
    }

    if (p == NULL)
        for (p = deleted_id_providers; p; p = LNEXT(p)) {
            if (p->plugin == plugin ||
                !wcscmp(p->name, pi.reg.name))
                break;
        }

    if (p)
        identpro_hold(p);

    if (p == NULL) {
        p = PMALLOC(sizeof(*p));
        ZeroMemory(p, sizeof(*p));

        p->magic = KCDB_IDENTPRO_MAGIC;
        p->name = PMALLOC(len);
        StringCbCopy(p->name, len, pi.reg.name);
        p->sub = sub;
        p->plugin = plugin;
        kmm_hold_plugin(plugin);

        p->refcount = 1;            /* initially held */

        QPUT(&id_providers, p);

        n_id_providers++;
        kcdbint_identpro_post_message(KCDB_OP_INSERT, p);

    } else if (p->flags & KCDB_IDENTPRO_FLAG_DELETED) {

        LDELETE(&deleted_id_providers, p);
        p->flags &= ~KCDB_IDENTPRO_FLAG_DELETED;
        if (p->plugin != plugin) {
            /* can happen if the plug-in was reloaded */
            if (p->plugin)
                kmm_release_plugin_info_i(p->plugin);
            p->plugin = plugin;
            kmm_hold_plugin(plugin);
        }

        if (p->sub) {
            kmq_delete_subscription(p->sub);
        }

        p->sub = sub;

        QPUT(&id_providers, p);
        n_id_providers++;
    }

    identpro_check_and_set_default_provider();
    LeaveCriticalSection(&cs_identpro);

    kmm_release_plugin_info_i(&pi);

    return p;
}

static void
identpro_mark_for_deletion(kcdb_identpro_i * p)
{
    khm_boolean is_default = FALSE;

    if (!kcdb_is_identpro(p))
        return;

    EnterCriticalSection(&cs_identpro);
    if (DEFAULT_PROVIDER == p)
        is_default = TRUE;

    if (!(p->flags & KCDB_IDENTPRO_FLAG_DELETED)) {
        QDEL(&id_providers, p);
        n_id_providers--;
        LPUSH(&deleted_id_providers, p);
        p->flags |= KCDB_IDENTPRO_FLAG_DELETED;
	p->flags &= ~KCDB_IDENTPRO_FLAG_READY;

        kcdbint_identpro_post_message(KCDB_OP_DELETE, p);
    }

    if (p->refcount == 0)
        identpro_check_and_deleteL(p);

    if (is_default)
        identpro_check_and_set_default_provider();

    LeaveCriticalSection(&cs_identpro);
}

/* called with cs_identpro held */
static void
identpro_check_and_deleteL(kcdb_identpro_i * p)
{
    if (!(p->flags & KCDB_IDENTPRO_FLAG_DELETED) ||
        p->refcount > 0)
        return;

    p->magic = 0;
    if (p->name)
        PFREE(p->name);
    if (p->sub)
        kmq_delete_subscription(p->sub);
    if (p->plugin)
        kmm_release_plugin(p->plugin);
    LDELETE(&deleted_id_providers, p);
    ZeroMemory(p, sizeof(*p));
    PFREE(p);
}

static kcdb_identpro_i *
identpro_get_provider_for_identity(khm_handle videntity)
{
    kcdb_identity * ident;
    kcdb_identpro_i * p = NULL;

    EnterCriticalSection(&cs_ident);
    if (kcdb_is_identity(videntity)) {

        ident = (kcdb_identity *) videntity;

        p = ident->id_pro;
    }
    LeaveCriticalSection(&cs_ident);

    return p;
}

static khm_handle
identpro_get_sub_for_identity(khm_handle videntity)
{
    khm_handle sub = NULL;
    kcdb_identpro_i * p;

    p = identpro_get_provider_for_identity(videntity);
    EnterCriticalSection(&cs_identpro);
    if (kcdb_is_active_identpro(p))
        sub = p->sub;
    LeaveCriticalSection(&cs_identpro);

    return sub;
}

static khm_handle
identpro_get_sub(khm_handle vidpro) {
    kcdb_identpro_i * p;
    khm_handle sub;

    EnterCriticalSection(&cs_identpro);
    p = kcdb_identpro_from_handle(vidpro);
    if (!kcdb_is_active_identpro(vidpro)) {
        sub = NULL;
    } else {
        sub = p->sub;
    }
    LeaveCriticalSection(&cs_identpro);

    return sub;
}

KHMEXP khm_boolean KHMAPI
kcdb_handle_is_identpro(khm_handle h)
{
    return kcdb_is_active_identpro(h);
}

KHMEXP khm_boolean KHMAPI
kcdb_identpro_is_equal(khm_handle idp1, khm_handle idp2)
{
#ifdef DEBUG
    assert(kcdb_is_identpro(idp1));
    assert(kcdb_is_identpro(idp2));
#endif

    return idp1 == idp2;
}

KHMEXP khm_int32 KHMAPI
kcdb_identity_set_provider(khm_handle sub)
{
    kmm_plugin plugin = NULL;
    kcdb_identpro_i * p;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    plugin = kmm_this_plugin();
    if (plugin == NULL) {
#ifdef DEBUG
        assert(FALSE);
#endif
        return KHM_ERROR_INVALID_OPERATION;
    }

    p = identpro_find_by_plugin(plugin);
    if (p != NULL && sub != NULL) {
        rv = KHM_ERROR_DUPLICATE;
#ifdef DEBUG
        assert(FALSE);
#endif
        goto _exit;
    }

#ifdef DEBUG
    assert(sub != NULL || p != NULL);
#endif

    if (p == NULL)
        p = identpro_create_with_plugin(plugin, sub);

    if (p == NULL) {
        rv = KHM_ERROR_UNKNOWN;
        goto _exit;
    }

    if (sub) {
        rv = kmq_send_sub_msg(sub, KMSG_IDENT, KMSG_IDENT_INIT, 0, 0);

        if (KHM_FAILED(rv)) {
            kmq_send_sub_msg(sub, KMSG_IDENT, KMSG_IDENT_EXIT, 0, 0);
            identpro_mark_for_deletion(p);
        } else {
	    EnterCriticalSection(&cs_identpro);
	    p->flags |= KCDB_IDENTPRO_FLAG_READY;
	    LeaveCriticalSection(&cs_identpro);
	}
    } else {
        EnterCriticalSection(&cs_identpro);
        sub = p->sub;
        LeaveCriticalSection(&cs_identpro);
        kmq_send_sub_msg(sub, KMSG_IDENT, KMSG_IDENT_EXIT, 0, 0);
        identpro_mark_for_deletion(p);
    }

 _exit:
    if (p)
        identpro_release(p);

    if (plugin)
        kmm_release_plugin(plugin);

    return rv;
}


KHMEXP khm_int32 KHMAPI
kcdb_identity_set_type(khm_int32 cred_type)
{
    kmm_plugin plugin = NULL;
    kcdb_identpro_i * idp = NULL;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    plugin = kmm_this_plugin();
    if (plugin == NULL) {
#ifdef DEBUG
        assert(FALSE);
#endif
        rv = KHM_ERROR_INVALID_OPERATION;
        goto _exit;
    }

    idp = identpro_find_by_plugin(plugin);
    if (idp == NULL) {
#ifdef DEBUG
        assert(FALSE);
#endif
        rv = KHM_ERROR_INVALID_OPERATION;
        goto _exit;
    }

    EnterCriticalSection(&cs_identpro);
    idp->cred_type = cred_type;

    identpro_release(idp);
    LeaveCriticalSection(&cs_identpro);

 _exit:

    if (plugin)
        kmm_release_plugin(plugin);

    return KHM_ERROR_SUCCESS;
}

/* The following two functions are deprecated, but are provided for
   backwards compatibility.  We have to suppress the deprecated
   warning.  */
#pragma warning(push)
#pragma warning(disable: 4995)

KHMEXP khm_int32 KHMAPI
kcdb_identity_get_provider(khm_handle * sub)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if (sub == NULL)
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_identpro);
    if (DEFAULT_PROVIDER == NULL) {
        rv = KHM_ERROR_NOT_FOUND;
    } else {
        *sub = DEFAULT_PROVIDER->sub;
    }
    LeaveCriticalSection(&cs_identpro);

    return rv;
}

KHMEXP khm_int32 KHMAPI
kcdb_identity_get_type(khm_int32 * ptype)
{
    if (ptype != NULL)
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_identpro);
    if (DEFAULT_PROVIDER != NULL) {
        *ptype = DEFAULT_PROVIDER->cred_type;
    }
    LeaveCriticalSection(&cs_identpro);

    if (*ptype > 0)
        return KHM_ERROR_SUCCESS;
    else
        return KHM_ERROR_NOT_FOUND;
}

#pragma warning(pop)

KHMEXP khm_int32 KHMAPI
kcdb_identpro_find(const wchar_t * name, khm_handle * vidpro)
{
    kcdb_identpro_i * p;

    p = identpro_find_by_name(name);
    if (p == NULL) {
        *vidpro = NULL;
        return KHM_ERROR_NOT_FOUND;
    } else {
        *vidpro = kcdb_handle_from_identpro(p);
        return KHM_ERROR_SUCCESS;
    }
}

KHMEXP khm_int32 KHMAPI
kcdb_identpro_hold(khm_handle vidpro)
{
    if (!kcdb_is_identpro(vidpro))
        return KHM_ERROR_INVALID_PARAM;

    identpro_hold(kcdb_identpro_from_handle(vidpro));
    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
kcdb_identpro_release(khm_handle vidpro)
{
    if (!kcdb_is_identpro(vidpro))
        return KHM_ERROR_INVALID_PARAM;

    identpro_release(kcdb_identpro_from_handle(vidpro));
    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
kcdb_identpro_begin_enum(kcdb_enumeration * pe, khm_size * pn_providers)
{
    kcdb_enumeration e;
    kcdb_identpro_i * p;
    khm_size i;

    EnterCriticalSection(&cs_identpro);

    if (n_id_providers == 0) {
        LeaveCriticalSection(&cs_identpro);
        if (pn_providers)
            *pn_providers = 0;
        *pe = NULL;
        return KHM_ERROR_NOT_FOUND;
    }

    kcdbint_enum_create(&e);
    kcdbint_enum_alloc(e, n_id_providers);

    for (i = 0, p = QTOP(&id_providers); p; p = QNEXT(p)) {
#ifdef DEBUG
        assert(i < n_id_providers);
#endif
        e->objs[i++] = (khm_handle) p;
        identpro_hold(p);
    }
    LeaveCriticalSection(&cs_identpro);

    *pe = e;
    if (pn_providers)
        *pn_providers = e->n;

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
kcdb_identpro_validate_name_ex(khm_handle vidpro, const wchar_t * name)
{
    kcdb_ident_name_xfer namex;
    khm_handle sub;
    khm_size cch;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    /* we need to verify the length of the string before calling the
       identity provider */
    if(FAILED(StringCchLength(name, KCDB_IDENT_MAXCCH_NAME, &cch)))
        return KHM_ERROR_TOO_LONG;

    sub = identpro_get_sub(vidpro);

    if(sub != NULL) {
        ZeroMemory(&namex, sizeof(namex));

        namex.name_src = name;
        namex.result = KHM_ERROR_NOT_IMPLEMENTED;

        kmq_send_sub_msg(sub,
                         KMSG_IDENT,
                         KMSG_IDENT_VALIDATE_NAME,
                         0,
                         (void *) &namex);

        rv = namex.result;
    } else {
        rv = KHM_ERROR_NO_PROVIDER;
    }

    return rv;
}

KHMEXP khm_int32 KHMAPI
kcdb_identpro_get_default(khm_handle * pvidpro)
{
    if (!pvidpro)
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_identpro);
    *pvidpro = DEFAULT_PROVIDER;
    if (*pvidpro)
        identpro_hold(*pvidpro);
    LeaveCriticalSection(&cs_identpro);

    if (*pvidpro)
        return KHM_ERROR_SUCCESS;
    else
        return KHM_ERROR_NOT_FOUND;
}

KHMEXP khm_int32 KHMAPI
kcdb_identpro_get_default_identity(khm_handle vidpro, khm_handle * pvident)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if (!pvident)
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_identpro);
    if (kcdb_is_active_identpro(vidpro)) {
        kcdb_identpro_i * p;

        p = kcdb_identpro_from_handle(vidpro);
        *pvident = p->default_id;
    } else {
        *pvident = NULL;
        rv = KHM_ERROR_NO_PROVIDER;
    }
    LeaveCriticalSection(&cs_identpro);

    if (*pvident == NULL ||
        KHM_FAILED(kcdb_identity_hold(*pvident)))
        rv = KHM_ERROR_NOT_FOUND;

    return rv;
}

#pragma warning(push)
#pragma warning(disable: 4995)

/* NOT called with cs_ident held */
KHMEXP khm_int32 KHMAPI
kcdb_identpro_validate_name(const wchar_t * name)
{
    kcdb_identpro_i * p;
    khm_int32 rv;

    EnterCriticalSection(&cs_identpro);
    p = DEFAULT_PROVIDER;
    if (p)
        identpro_hold(p);
    LeaveCriticalSection(&cs_identpro);

    if (p) {
        rv = kcdb_identpro_validate_name_ex(DEFAULT_PROVIDER, name);
        identpro_release(p);
    } else {
        rv = KHM_ERROR_NO_PROVIDER;
    }

    return rv;
}

#pragma warning(pop)

#if 0
KHMEXP khm_int32 KHMAPI
kcdb_identpro_validate_identity(khm_handle videntity)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;
    khm_handle sub;

    if(!kcdb_is_active_identity(videntity))
        return KHM_ERROR_INVALID_PARAM;

    sub = identpro_get_sub_for_identity(videntity);

    if(sub != NULL) {
        rv = kmq_send_sub_msg(sub,
                              KMSG_IDENT,
                              KMSG_IDENT_VALIDATE_IDENTITY,
                              0,
                              (void *) identity);
    } else {
        rv = KHM_ERROR_NO_PROVIDER;
    }

    return rv;
}
#endif

KHMEXP khm_int32 KHMAPI
kcdb_identpro_canon_name_ex(khm_handle vidpro,
                            const wchar_t * name_in,
                            wchar_t * name_out,
                            khm_size * cb_name_out)
{
    khm_handle sub;
    kcdb_ident_name_xfer namex;
    wchar_t name_tmp[KCDB_IDENT_MAXCCH_NAME];
    khm_int32 rv = KHM_ERROR_SUCCESS;
    khm_size cch;

    if(cb_name_out == 0 ||
       FAILED(StringCchLength(name_in, KCDB_IDENT_MAXCCH_NAME, &cch)))
        return KHM_ERROR_INVALID_NAME;

    sub = identpro_get_sub(vidpro);

    if(sub != NULL) {
        ZeroMemory(&namex, sizeof(namex));
        ZeroMemory(name_tmp, sizeof(name_tmp));

        namex.name_src = name_in;
        namex.name_dest = name_tmp;
        namex.cb_name_dest = sizeof(name_tmp);
        namex.result = KHM_ERROR_NOT_IMPLEMENTED;

        rv = kmq_send_sub_msg(sub,
                              KMSG_IDENT,
                              KMSG_IDENT_CANON_NAME,
                              0,
                              (void *) &namex);

        if(KHM_SUCCEEDED(namex.result)) {
            const wchar_t * name_result;
            khm_size cb;

            if(name_in[0] != 0 && name_tmp[0] == 0)
                name_result = name_tmp;
            else
                name_result = name_in;
			
            if(FAILED(StringCbLength(name_result, KCDB_IDENT_MAXCB_NAME, &cb)))
                rv = KHM_ERROR_UNKNOWN;
            else {
                cb += sizeof(wchar_t);
                if(name_out == 0 || *cb_name_out < cb) {
                    rv = KHM_ERROR_TOO_LONG;
                    *cb_name_out = cb;
                } else {
                    StringCbCopy(name_out, *cb_name_out, name_result);
                    *cb_name_out = cb;
                    rv = KHM_ERROR_SUCCESS;
                }
            }
        }
    } else {
        rv = KHM_ERROR_NO_PROVIDER;
    }

    return rv;
}

#pragma warning(push)
#pragma warning(disable: 4995)

KHMEXP khm_int32 KHMAPI
kcdb_identpro_canon_name(const wchar_t * name_in,
                         wchar_t * name_out,
                         khm_size * cb_name_out)
{
    kcdb_identpro_i * p;
    khm_int32 rv;

    EnterCriticalSection(&cs_identpro);
    p = DEFAULT_PROVIDER;
    if (p)
        identpro_hold(p);
    LeaveCriticalSection(&cs_identpro);

    if (p) {
        rv = kcdb_identpro_canon_name_ex(kcdb_handle_from_identpro(p),
                                         name_in,
                                         name_out,
                                         cb_name_out);
        identpro_release(p);
    } else {
        rv = KHM_ERROR_NO_PROVIDER;
    }

    return rv;
}

#pragma warning(pop)

KHMEXP khm_int32 KHMAPI
kcdb_identpro_compare_name_ex(khm_handle vidpro,
                              const wchar_t * name1,
                              const wchar_t * name2)
{
    khm_handle sub;
    kcdb_ident_name_xfer namex;
    khm_int32 rv = 0;

    /* Generally in kcdb_identpro_* functions we don't emulate
       any behavior if the provider is not available, but lacking
       a way to make this known, we emulate here */
    rv = wcscmp(name1, name2);

    sub = identpro_get_sub(vidpro);

    if(sub != NULL) {
        ZeroMemory(&namex, sizeof(namex));
        namex.name_src = name1;
        namex.name_alt = name2;
        namex.result = rv;

        kmq_send_sub_msg(sub,
                         KMSG_IDENT,
                         KMSG_IDENT_COMPARE_NAME,
                         0,
                         (void *) &namex);

        rv = namex.result;
    }

    return rv;
}

#pragma warning(push)
#pragma warning(disable: 4995)

KHMEXP khm_int32 KHMAPI
kcdb_identpro_compare_name(const wchar_t * name1,
                           const wchar_t * name2)
{
    kcdb_identpro_i * p;
    khm_int32 rv;

    EnterCriticalSection(&cs_identpro);
    p = DEFAULT_PROVIDER;
    if (p)
        identpro_hold(p);
    LeaveCriticalSection(&cs_identpro);

    rv = kcdb_identpro_compare_name_ex(kcdb_handle_from_identpro(p),
                                       name1, name2);
    if (p)
        identpro_release(p);

    return rv;
}

#pragma warning(pop)

KHMEXP khm_int32 KHMAPI
kcdb_identpro_get_name(khm_handle vidpro, wchar_t * pname, khm_size * pcb_name)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;
    kcdb_identpro_i * p;
    size_t cb;

    if (!kcdb_is_active_identpro(vidpro) || !pcb_name)
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_identpro);
    if (kcdb_is_active_identpro(vidpro)) {
        p = kcdb_identpro_from_handle(vidpro);
#ifdef DEBUG
        assert(p->name);
#endif

        if (p->name) {
            StringCbLength(p->name, KCDB_MAXCB_NAME, &cb);
            cb += sizeof(wchar_t);
        } else {
            cb = sizeof(wchar_t);
        }

        if (pname && *pcb_name >= cb) {
            if (p->name)
                StringCbCopy(pname, *pcb_name, p->name);
            else
                *pname = L'\0';
            *pcb_name = cb;
        } else {
            *pcb_name = cb;
            rv = KHM_ERROR_TOO_LONG;
        }
    } else {
        rv = KHM_ERROR_INVALID_PARAM;
    }
    LeaveCriticalSection(&cs_identpro);

    return rv;
}

KHMEXP khm_int32 KHMAPI
kcdb_identpro_get_type(khm_handle vidpro, khm_int32 * ptype)
{
    kcdb_identpro_i * p;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if (!kcdb_is_active_identpro(vidpro) || !ptype)
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_identpro);
    if (kcdb_is_active_identpro(vidpro)) {
        p = kcdb_identpro_from_handle(vidpro);
        *ptype = p->cred_type;
    } else {
        *ptype = KCDB_CREDTYPE_INVALID;
        rv = KHM_ERROR_INVALID_PARAM;
    }
    LeaveCriticalSection(&cs_identpro);

    return rv;
}

KHMEXP khm_int32 KHMAPI
kcdb_identpro_set_default_identity(khm_handle videntity, khm_boolean ask_idpro)
{
    khm_handle sub = NULL;
    khm_int32 rv = KHM_ERROR_SUCCESS;
    kcdb_identpro_i * p;

    if(!kcdb_is_identity(videntity))
        return KHM_ERROR_INVALID_PARAM;

    p = identpro_get_provider_for_identity(videntity);

    EnterCriticalSection(&cs_identpro);
    if (!kcdb_is_active_identpro(p)) {
        rv = KHM_ERROR_INVALID_PARAM;
    } else {
        if (kcdb_identity_is_equal(videntity, p->default_id))
            rv = KHM_ERROR_DUPLICATE;
    }
    LeaveCriticalSection(&cs_identpro);

    if (KHM_FAILED(rv)) {
        return (rv == KHM_ERROR_DUPLICATE)? KHM_ERROR_SUCCESS : rv;
    }

    if (ask_idpro) {
        sub = identpro_get_sub_for_identity(videntity);

        if(sub != NULL) {
            rv = kmq_send_sub_msg(sub, KMSG_IDENT, KMSG_IDENT_SET_DEFAULT,
                                  TRUE, (void *) videntity);
        } else {
#ifdef DEBUG
            assert(FALSE);
#endif
            rv = KHM_ERROR_NO_PROVIDER;
        }
    }

    if (KHM_SUCCEEDED(rv)) {
        khm_handle h_old_id = NULL;
        int new_default = FALSE;
        kcdb_identity * id;

        EnterCriticalSection(&cs_identpro);
        if (!kcdb_identity_is_equal(videntity, p->default_id)) {
            h_old_id = p->default_id;
            p->default_id = videntity;
            new_default = TRUE;
        }
        LeaveCriticalSection(&cs_identpro);

        EnterCriticalSection(&cs_ident);
        if (new_default)
            kcdb_identity_hold(videntity);
        id = (kcdb_identity *) videntity;
        id->flags |= KCDB_IDENT_FLAG_DEFAULT;

        if (h_old_id) {
            id = (kcdb_identity *) h_old_id;
            id->flags &= ~KCDB_IDENT_FLAG_DEFAULT;
            kcdb_identity_release(h_old_id);
        }
        LeaveCriticalSection(&cs_ident);

        if (new_default) {
            kcdb_identity_hold(videntity);
            kcdbint_ident_post_message(KCDB_OP_NEW_DEFAULT, (kcdb_identity *) videntity);
        }
    }

    return rv;
}

KHMEXP khm_int32 KHMAPI
kcdb_identpro_set_searchable(khm_handle videntity,
                             khm_boolean searchable)
{
    khm_handle sub;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if(!kcdb_is_identity(videntity))
        return KHM_ERROR_INVALID_PARAM;

    sub = identpro_get_sub_for_identity(videntity);

    if(sub != NULL) {
        rv = kmq_send_sub_msg(sub,
                              KMSG_IDENT,
                              KMSG_IDENT_SET_SEARCHABLE,
                              searchable,
                              (void *) videntity);
    } else {
        rv = KHM_ERROR_NO_PROVIDER;
    }

    return rv;
}


KHMEXP khm_int32 KHMAPI
kcdb_identpro_update(khm_handle identity)
{
    khm_handle sub;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if(!kcdb_is_active_identity(identity))
        return KHM_ERROR_INVALID_PARAM;

    sub = identpro_get_sub_for_identity(identity);

    if(sub != NULL) {
        rv = kmq_send_sub_msg(sub,
                              KMSG_IDENT,
                              KMSG_IDENT_UPDATE,
                              0,
                              (void *) identity);
    } else {
        rv = KHM_ERROR_NO_PROVIDER;
    }

    return rv;
}

KHMEXP khm_int32 KHMAPI
kcdb_identpro_notify_create(khm_handle identity)
{
    khm_handle sub;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if(!kcdb_is_identity(identity))
        return KHM_ERROR_INVALID_PARAM;

    sub = identpro_get_sub_for_identity(identity);

    if(sub != NULL) {
        rv = kmq_send_sub_msg(sub,
                              KMSG_IDENT, KMSG_IDENT_NOTIFY_CREATE,
                              0, (void *) identity);
    } else {
        rv = KHM_ERROR_NO_PROVIDER;
    }

    return rv;
}

KHMEXP khm_int32 KHMAPI
kcdb_identpro_notify_config_create(khm_handle identity)
{
    khm_handle sub;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if(!kcdb_is_identity(identity))
        return KHM_ERROR_INVALID_PARAM;

    sub = identpro_get_sub_for_identity(identity);

    if(sub != NULL) {
        rv = kmq_send_sub_msg(sub,
                              KMSG_IDENT, KMSG_IDENT_NOTIFY_CONFIG,
                              0, (void *) identity);
    } else {
        rv = KHM_ERROR_NO_PROVIDER;
    }

    return rv;
}

#pragma warning(push)
#pragma warning(disable: 4995)

KHMEXP khm_int32 KHMAPI
kcdb_identpro_get_ui_cb(void * rock)
{
    kcdb_identpro_i * p;
    khm_handle sub;
    khm_int32 rv;

    EnterCriticalSection(&cs_identpro);
    p = DEFAULT_PROVIDER;
    if (p)
        identpro_hold(p);
    LeaveCriticalSection(&cs_identpro);

    if (p) {
        sub = identpro_get_sub(kcdb_handle_from_identpro(p));

        rv = kmq_send_sub_msg(sub, KMSG_IDENT, KMSG_IDENT_GET_UI_CALLBACK,
                              0, rock);
    } else {
        rv = KHM_ERROR_NO_PROVIDER;
    }

    if (p)
        identpro_release(p);

    return rv;
}

#pragma warning(pop)

void
kcdbint_identpro_post_message(khm_int32 op, kcdb_identpro_i * p) {
    identpro_hold(p);
    kmq_post_message(KMSG_KCDB, KMSG_KCDB_IDENTPRO, op, (void *) p);
}

void
kcdbint_identpro_msg_completion(kmq_message * m) {
    identpro_release((kcdb_identpro_i *) m->vparam);
}
