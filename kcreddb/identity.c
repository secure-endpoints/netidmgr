/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
 * Copyright (c) 2007-2009 Secure Endpoints Inc.
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
#include<assert.h>

CRITICAL_SECTION cs_ident;

hashtable *     kcdb_identities_namemap = NULL;

khm_int32       kcdb_n_identities = 0;

kcdb_identity * kcdb_identities = NULL;

khm_ui_8        kcdb_ident_serial = 0;

khm_ui_4        kcdb_ident_refresh_cycle = 0;

khm_boolean     kcdb_checked_config = FALSE;

khm_boolean     kcdb_checking_config = FALSE;

HANDLE          kcdb_config_check_event = NULL;

/* forward dcl */
static khm_int32
hash_identity(const void *);

static khm_int32
hash_identity_comp(const void *, const void *);

void 
kcdbint_ident_init(void)
{
    InitializeCriticalSection(&cs_ident);
    kcdb_identities_namemap =
        hash_new_hashtable(KCDB_IDENT_HASHTABLE_SIZE,
                           hash_identity,
                           hash_identity_comp,
                           NULL, NULL);
    kcdb_config_check_event = CreateEvent(NULL, TRUE, FALSE, L"KCDB Config Check Completion Event");
}

void 
kcdbint_ident_exit(void)
{
    EnterCriticalSection(&cs_ident);
    hash_del_hashtable(kcdb_identities_namemap);
    LeaveCriticalSection(&cs_ident);
    DeleteCriticalSection(&cs_ident);
    CloseHandle(kcdb_config_check_event);
}

/* message completion routine */
void 
kcdbint_ident_msg_completion(kmq_message * m)
{
    kcdb_identity_release(m->vparam);
}

/*! \note cs_ident must be available. */
void 
kcdbint_ident_post_message(khm_int32 op, kcdb_identity * id)
{
    kcdb_identity_hold(id);
    kmq_post_message(KMSG_KCDB, KMSG_KCDB_IDENT, op, (void *) id);
}

/* hashtable callbacks */
static khm_int32
hash_identity(const void * vkey)
{
    const kcdb_identity * key = (const kcdb_identity *) vkey;

    assert(kcdb_is_identity(key));

    return (hash_string(key->name) << 5) + (khm_int32)((UINT_PTR)key->id_pro >> 3);
}

static khm_int32
hash_identity_comp(const void * vkey1, const void * vkey2)
{
    const kcdb_identity * key1 = (const kcdb_identity *) vkey1;
    const kcdb_identity * key2 = (const kcdb_identity *) vkey2;

#ifdef DEBUG
    assert(kcdb_is_identity(key1));
    assert(kcdb_is_identity(key2));
#endif

    if (key1->id_pro == key2->id_pro) {
        return wcscmp(key1->name, key2->name);
    } else {
        return (key1->id_pro < key2->id_pro)? -1 : 1;
    }
}

KHMEXP khm_boolean KHMAPI
kcdb_identity_is_equal(khm_handle identity1,
                       khm_handle identity2)
{
    return (identity1 == identity2);
}

KHMEXP khm_boolean KHMAPI
kcdb_handle_is_identity(khm_handle h)
{
    return kcdb_is_identity(h);
}

#pragma warning(push)
#pragma warning(disable: 4995)

/* NOT called with cs_ident held */
KHMEXP khm_boolean KHMAPI 
kcdb_identity_is_valid_name(const wchar_t * name)
{
    khm_int32 rv;

    /* special case.  Note since the string we are comparing with is
       of a known length we don't need to check the length of name. */
    if (!wcscmp(name, L"_Schema"))
        return FALSE;

    rv = kcdb_identpro_validate_name(name);

    if(rv == KHM_ERROR_NO_PROVIDER ||
       rv == KHM_ERROR_NOT_IMPLEMENTED)
        return TRUE;
    else
        return KHM_SUCCEEDED(rv);
}

#pragma warning(pop)

/* Called with cs_ident held */
static void
kcdbint_refresh_identity_timer_thresholds(kcdb_identity * id, khm_handle csp_id)
{
    khm_int32 t;
    FILETIME ft;

    if (KHM_FAILED(khc_get_last_write_time(csp_id, 0, &ft))) {
        ft = IntToFt(0);
    }

    if ((id->ft_thr_last_update.dwHighDateTime != 0 ||
         id->ft_thr_last_update.dwLowDateTime != 0) &&

        (FtToInt(&ft) == 0 ||
         CompareFileTime(&ft, &id->ft_thr_last_update) <= 0))

        return;

    if ((khc_value_exists(csp_id, L"Name") & (KCONF_FLAG_USER | KCONF_FLAG_SCHEMA))
        != (KCONF_FLAG_SCHEMA | KCONF_FLAG_USER))
        return;

    if (FtToInt(&ft) != 0)
        id->ft_thr_last_update = ft;
    else
        GetSystemTimeAsFileTime(&id->ft_thr_last_update);

    if (KHM_FAILED(khc_read_int32(csp_id, L"Monitor", &t)) || t == 0) {
        id->ft_thr_renew = IntToFt(0);
        id->ft_thr_warn = id->ft_thr_renew;
        id->ft_thr_crit = id->ft_thr_renew;
        return;
    }

    if (KHM_SUCCEEDED(khc_read_int32(csp_id, L"AllowAutoRenew", &t)) && t != 0) {
        if (KHM_SUCCEEDED(khc_read_int32(csp_id, L"RenewAtHalfLife", &t)) && t != 0) {
            id->ft_thr_renew.dwHighDateTime = 0;
            id->ft_thr_renew.dwLowDateTime = 1;
        } else {
            t = 0;
            khc_read_int32(csp_id, L"AutoRenewThreshold", &t);
            TimetToFileTimeInterval(t, &id->ft_thr_renew);
        }
    } else {
        id->ft_thr_renew = IntToFt(0);
    }

    if (KHM_SUCCEEDED(khc_read_int32(csp_id, L"AllowWarn", &t)) && t != 0) {
        t = 0;
        khc_read_int32(csp_id, L"WarnThreshold", &t);
        TimetToFileTimeInterval(t, &id->ft_thr_warn);
    } else {
        id->ft_thr_warn = IntToFt(0);
    }

    if (KHM_SUCCEEDED(khc_read_int32(csp_id, L"AllowCritical", &t)) && t != 0) {
        t = 0;
        khc_read_int32(csp_id, L"CriticalThreshold", &t);
        TimetToFileTimeInterval(t, &id->ft_thr_crit);
    } else {
        id->ft_thr_crit = IntToFt(0);
    }
}

KHMEXP khm_int32 KHMAPI
kcdb_identity_create_ex(khm_handle vidpro,
                        const wchar_t * name,
                        khm_int32 flags,
                        void * vparam,
                        khm_handle * result)
{
    kcdb_identity * id = NULL;
    kcdb_identity * id_tmp = NULL;
    kcdb_identity   id_query;
    khm_boolean     create_config = FALSE;
    khm_boolean     create_ident = FALSE;

    if (!result || !name || !kcdb_is_active_identpro(vidpro))
        return KHM_ERROR_INVALID_PARAM;

    *result = NULL;

    EnterCriticalSection(&cs_ident);

    /* is it there already? */
    ZeroMemory(&id_query, sizeof(id_query));
    id_query.magic = KCDB_IDENT_MAGIC;
    id_query.name = name;
    id_query.id_pro = kcdb_identpro_from_handle(vidpro);
    id = hash_lookup(kcdb_identities_namemap, (void *) &id_query);
    if(id)
        kcdb_identity_hold((khm_handle) id);
    LeaveCriticalSection(&cs_ident);

    if(id) {
        *result = (khm_handle) id;
        return KHM_ERROR_SUCCESS;
    } else if(!(flags & KCDB_IDENT_FLAG_CREATE)) {
        return KHM_ERROR_NOT_FOUND;
    }

    create_config = !!(flags & KCDB_IDENT_FLAG_CONFIG);
    create_ident = !!(flags & KCDB_IDENT_FLAG_CREATE);
    flags &= ~(KCDB_IDENT_FLAG_CREATE | KCDB_IDENT_FLAG_CONFIG);

    /* nope. create it */
    if((flags & ~KCDB_IDENT_FLAGMASK_RDWR) ||
       (flags & (KCDB_IDENT_FLAG_DEFAULT |
                 KCDB_IDENT_FLAG_SEARCHABLE |
                 KCDB_IDENT_FLAG_STICKY))) {
        /* can't specify this flag in create */
        return KHM_ERROR_INVALID_PARAM;
    }

    if (KHM_FAILED(kcdb_identpro_validate_name_ex(vidpro, name))) {
        return KHM_ERROR_INVALID_NAME;
    }

    EnterCriticalSection(&cs_identpro);
    if (kcdb_is_active_identpro(vidpro)) {
        kcdb_identpro_hold(vidpro);
    } else {
        LeaveCriticalSection(&cs_identpro);
        return KHM_ERROR_NO_PROVIDER;
    }
    LeaveCriticalSection(&cs_identpro);

    id = PMALLOC(sizeof(kcdb_identity));
    ZeroMemory(id, sizeof(kcdb_identity));
    id->magic = KCDB_IDENT_MAGIC;
    id->name = PWCSDUP(name);

    id->id_pro = kcdb_identpro_from_handle(vidpro);
    /* vidpro is already held from above */

    id->flags = (flags & KCDB_IDENT_FLAGMASK_RDWR);
    id->flags |=
        KCDB_IDENT_FLAG_EMPTY |
        KCDB_IDENT_FLAG_NO_NOTIFY;
    LINIT(id);

    EnterCriticalSection(&cs_ident);
    id_tmp = hash_lookup(kcdb_identities_namemap, (void *) &id_query);
    if(id_tmp) {
        /* lost a race */
        LeaveCriticalSection(&cs_ident);

        kcdb_identity_hold((khm_handle) id_tmp);
        *result = (khm_handle) id_tmp;

        PFREE((void *) id->name);
        kcdb_identpro_release(vidpro);
        PFREE(id);
        id = NULL;
    } else {
        khm_handle h_cfg;
        khm_handle h_kcdb;

        /* This is also the place where we check and update the
           identity serial number */
        h_kcdb = kcdb_get_config();
        if (kcdb_ident_serial == 0) {
            khm_int64 t = 0;
            khc_read_int64(h_kcdb, L"IdentSerial", &t);
            kcdb_ident_serial = (khm_ui_8) t;
        }

        if (id->serial == 0)
            id->serial = ++kcdb_ident_serial;

        hash_add(kcdb_identities_namemap, (void *) id, (void *) id);
        kcdb_identity_hold((khm_handle) id);

        if (KHM_SUCCEEDED(kcdb_identity_get_config((khm_handle) id,
                                                   KCONF_FLAG_SHADOW |
                                                   ((create_config)? KHM_FLAG_CREATE : 0) |
                                                   ((create_ident)? KHM_PERM_WRITE : 0),
                                                   &h_cfg))) {
            /* don't need to set the KCDB_IDENT_FLAG_CONFIG flags
               since kcdb_identity_get_config() sets it for us. */
            khm_int32 sticky;

            if (KHM_SUCCEEDED(khc_read_int32(h_cfg, L"Sticky", &sticky)) &&
                sticky) {
                id->flags |= KCDB_IDENT_FLAG_STICKY;
            }

            kcdbint_refresh_identity_timer_thresholds(id, h_cfg);

            khc_close_space(h_cfg);
        }
        
        if (id->serial == kcdb_ident_serial) {
            khc_write_int64(h_kcdb, L"IdentSerial", kcdb_ident_serial);
        }

        if (vparam)
            kcdb_identity_set_attr((khm_handle) id, KCDB_ATTR_PARAM,
                                   &vparam, sizeof(vparam));

        LeaveCriticalSection(&cs_ident);

        *result = (khm_handle) id;

        {
            khm_int32 rv;
            rv = kcdb_identpro_notify_create((khm_handle) id);
            assert(rv != KHM_ERROR_NO_PROVIDER);
        }

        if (vparam)
            kcdb_identity_set_attr((khm_handle) id, KCDB_ATTR_PARAM, NULL, 0);

        EnterCriticalSection(&cs_ident);
        LPUSH(&kcdb_identities, id);
        id->flags |= KCDB_IDENT_FLAG_ACTIVE;
        id->flags &= ~KCDB_IDENT_FLAG_NO_NOTIFY;
        LeaveCriticalSection(&cs_ident);

        kcdbint_ident_post_message(KCDB_OP_INSERT, id);

        if (h_kcdb)
            khc_close_space(h_kcdb);
    }

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI 
kcdb_identity_create(const wchar_t *name, 
                     khm_int32 flags, 
                     khm_handle * result)
{
    wchar_t * pc;
    wchar_t id_name[KCDB_IDENT_MAXCCH_NAME];
    khm_handle vidpro = NULL;
    khm_int32 rv;

    if (name == NULL)
        return KHM_ERROR_INVALID_PARAM;

    pc = wcschr(name, L':');

    if (pc == NULL) {
        if (KHM_FAILED(kcdb_identpro_find(L"Krb5Ident", &vidpro))) {
            /* TODO: This needs to be handled in a more graceful manner */
            return KHM_ERROR_NO_PROVIDER;
        }

        StringCbCopy(id_name, sizeof(id_name), name);
    } else {
        wchar_t idprov_name[KCDB_MAXCCH_NAME];

        StringCchCopy(id_name, sizeof(id_name), pc + 1);
        StringCchCopyN(idprov_name, sizeof(idprov_name), name, pc - name);

        /* Check for known prefixes */
        if (!wcscmp(idprov_name, L"NIMIdentity")) {
            khm_ui_8 serial;
            kcdb_identity * id;

            /* This is an identity name generated by
               kcdb_identity_get_short_name().  The name looks like
               "NIMIdentity:<id-serial>", where id-serial is the
               serial number of the identity in decimal. */

            serial = _wtoi64(id_name);

            EnterCriticalSection(&cs_ident);
            for (id = kcdb_identities; id != NULL; id = LNEXT(id)) {
                if ((id->flags & KCDB_IDENT_FLAG_ACTIVE) == KCDB_IDENT_FLAG_ACTIVE &&
                    id->serial == serial)
                    break;
            }

            if (id) {
                kcdb_identity_hold((khm_handle) id);
            }
            LeaveCriticalSection(&cs_ident);

            if (id) {
                *result = id;
                return KHM_ERROR_SUCCESS;
            } else {
                return KHM_ERROR_NOT_FOUND;
            }

        } else if (KHM_FAILED(kcdb_identpro_find(idprov_name, &vidpro)))
            return KHM_ERROR_NO_PROVIDER;
    }

    rv = kcdb_identity_create_ex(vidpro, id_name, flags, NULL, result);

    kcdb_identpro_release(vidpro);

    return rv;
}

khm_int32
escape_short_name_chars(wchar_t * name, khm_size cb_max)
{
    wchar_t *c, *d;
    size_t cch;
    size_t cch_escape;          /* number of characters to be escaped */
    size_t cch_max = cb_max / sizeof(wchar_t);

    if (FAILED(StringCchLength(name, cch_max, &cch))) {
#ifdef DEBUG
        assert(FALSE);
#endif
        return KHM_ERROR_INVALID_PARAM;
    }
    cch++;

    cch_escape = 0;
    for (c = name; *c; c++) {
        if (*c == L'\\' || *c == L'#')
            cch_escape++;
    }

    if (cch_escape == 0)
        return KHM_ERROR_SUCCESS;

    if (cch + cch_escape > cch_max)
        return KHM_ERROR_TOO_LONG;

    for (c = name + cch, d = name + (cch + cch_escape);
         cch_escape > 0;
         c--) {
        if (*c == L'\\') {
            *d-- = L'b'; *d-- = L'#';
            cch_escape--;
        } else if (*c == L'#') {
            *d-- = L'#'; *d-- = L'#';
            cch_escape--;
        } else {
            *d-- = *c;
        }
    }

#ifdef DEBUG
    assert(d == c);
    assert(c >= name);
#endif

    return KHM_ERROR_SUCCESS;
}

/* \internal
   \brief Retrieves a short name representing an identity

   The generated name is used by several different functions that
   place several constraints on the generated name.  To satisfy these,
   the following rules are followed:

   1. If the identity was provided by a provider named "Krb5Ident",
      then the generated name will be just the identity name.  If the
      generated name is longer than KCDB_MAXCCH_NAME, then rule #3 is
      applied.

   2. If the identity was not provided by "Krb5Ident", then the
      generated name will be of the form
      "<provider-name>:<identity-name>".  If the generated name
      exceeds KCDB_MAXCCH_NAME, then rule #3 is applied.

   3. If #1 and #2 fail to produce a suitable name, then the generated
      name will be of the form "NIMIdentity:<serial-number>", where
      "<serial-number>" will be the upper-case hexadecimal
      representation of the serial number of the identity.

   The current constraints are:

   - The name should uniquely identify an identity.

   - Passing an unescaped named into kcdb_identity_create() should
     open the identity if it is still active.

   - The escaped name should be usable as a configuration space name.

   - The unescaped name should be usable as a configuration node name.

   \param[in] vid Handle to the identity

   \param[in] escape_chars Set to \a TRUE if special characters in the
       generated name should be escaped.  The special characters that
       are currently recognized are '#' and '\'.

   \param[out] buf Receives the generated short name.

   \param[in,out] pcb_buf On entry, specifies the size of \a buf in
       bytes. On exit, specifies how many bytes were used (including
       the trailing NULL).

 */
KHMEXP khm_int32 KHMAPI
kcdb_identity_get_short_name(khm_handle vid, khm_boolean escape_chars,
                             wchar_t * buf, khm_size * pcb_buf)
{
    kcdb_identity * id;
    wchar_t name[KCDB_MAXCCH_NAME];
    khm_size cb;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    EnterCriticalSection(&cs_ident);
    if (!kcdb_is_identity(vid)) {
        rv = KHM_ERROR_INVALID_PARAM;
        goto _exit;
    }

    id = (kcdb_identity *) vid;
#ifdef DEBUG
    assert(id->id_pro != NULL);
    assert(id->id_pro->name != NULL);
    assert(id->name);
#endif

    /* #1 */
    if (id->id_pro && id->id_pro->name && !wcscmp(id->id_pro->name, L"Krb5Ident") &&
        SUCCEEDED(StringCbCopy(name, sizeof(name), id->name)) &&
        (!escape_chars || KHM_SUCCEEDED(escape_short_name_chars(name, sizeof(name)))))
        goto _have_name;

    /* #2 */
    if (id->name && id->id_pro && id->id_pro->name &&
        SUCCEEDED(StringCbPrintf(name, sizeof(name), L"%s:%s", id->id_pro->name, id->name)) &&
        (!escape_chars || KHM_SUCCEEDED(escape_short_name_chars(name, sizeof(name)))))
        goto _have_name;

    /* #3 */
    StringCbPrintf(name, sizeof(name), L"NIMIdentity:%I64d", id->serial);

 _have_name:
    if (FAILED(StringCbLength(name, sizeof(name), &cb))) {
        rv = KHM_ERROR_UNKNOWN;
    } else {
        cb += sizeof(wchar_t);
        if (buf == NULL || *pcb_buf < cb) {
            *pcb_buf = cb;
            rv = KHM_ERROR_TOO_LONG;
        } else {
            StringCbCopy(buf, *pcb_buf, name);
            *pcb_buf = cb;
            rv = KHM_ERROR_SUCCESS;
        }
    }

 _exit:
    LeaveCriticalSection(&cs_ident);

    return rv;
}

KHMEXP khm_int32 KHMAPI
khui_cache_del_by_owner(khm_handle owner);


KHMEXP khm_int32 KHMAPI 
kcdb_identity_delete(khm_handle vid)
{
    kcdb_identity * id;
    kcdb_identity * parent = NULL;
    khm_int32 code = KHM_ERROR_SUCCESS;

    EnterCriticalSection(&cs_ident);
    if(!kcdb_is_identity(vid)) {
        code = KHM_ERROR_INVALID_PARAM;
        goto _exit;
    }

    id = (kcdb_identity *) vid;

    if (kcdb_is_active_identity(vid)) {

        id->flags &= ~KCDB_IDENT_FLAG_ACTIVE;

        hash_del(kcdb_identities_namemap, (void *) id);

        LeaveCriticalSection(&cs_ident);

        kcdbint_ident_post_message(KCDB_OP_DELETE, id);

        /* Once everybody finishes dealing with the identity deletion,
           we will get called again. */
        return KHM_ERROR_SUCCESS;
    } else if (id->refcount == 0) {

        parent = id->parent;
        id->parent = NULL;

        khui_cache_del_by_owner(vid);

        /* If the identity is not active, it is not in the hashtable
           either */
        LDELETE(&kcdb_identities, id);

        if (id->name)
            PFREE((void *) id->name);
        id->magic = 0;
        PFREE(id);
    }

    /* else, we have an identity that is not active, but has
       outstanding references.  We have to wait until those references
       are freed.  Once they are released, kcdb_identity_delete() will
       be called again. */

 _exit:
    LeaveCriticalSection(&cs_ident);

    /* parent must be released outside cs_ident since it can also
       trigger an identity delete. */
    if (parent) {
        kcdb_identity_release(parent);
    }

    return code;
}

KHMEXP khm_int32 KHMAPI 
kcdb_identity_set_flags(khm_handle vid, 
                        khm_int32 flag,
                        khm_int32 mask)
{
    kcdb_identity * id;
    khm_int32 oldflags;
    khm_int32 newflags;
    khm_int32 delta = 0;
    khm_int32 rv;

    if (mask == 0)
        return KHM_ERROR_SUCCESS;

    if(!kcdb_is_identity(vid))
        return KHM_ERROR_INVALID_PARAM;

    id = (kcdb_identity *) vid;

    flag &= mask;

    if((mask & ~KCDB_IDENT_FLAGMASK_RDWR) ||
       ((flag & KCDB_IDENT_FLAG_INVALID) && (flag & KCDB_IDENT_FLAG_VALID)))
        return KHM_ERROR_INVALID_PARAM;

    if((mask & KCDB_IDENT_FLAG_DEFAULT) &&
       (flag & KCDB_IDENT_FLAG_DEFAULT)) {
        /* kcdb_identity_set_default already does checking for
           redundant transitions */
        rv = kcdb_identity_set_default(vid);

        if(KHM_FAILED(rv))
            return rv;

        mask &= ~KCDB_IDENT_FLAG_DEFAULT;
        flag &= ~KCDB_IDENT_FLAG_DEFAULT;
        delta |= KCDB_IDENT_FLAG_DEFAULT;
    }

    EnterCriticalSection(&cs_ident);

    if(mask & KCDB_IDENT_FLAG_SEARCHABLE) {
        if(!(flag & KCDB_IDENT_FLAG_SEARCHABLE)) {
            if(id->flags & KCDB_IDENT_FLAG_SEARCHABLE) {
                LeaveCriticalSection(&cs_ident);
                rv = kcdb_identpro_set_searchable(vid, FALSE);
                EnterCriticalSection(&cs_ident);
                if (rv == KHM_ERROR_NO_PROVIDER ||
                    KHM_SUCCEEDED(rv)) {
                    id->flags &= ~KCDB_IDENT_FLAG_SEARCHABLE;
                    delta |= KCDB_IDENT_FLAG_SEARCHABLE;
                }
            }
        } else {
            if(!(id->flags & KCDB_IDENT_FLAG_SEARCHABLE)) {
                LeaveCriticalSection(&cs_ident);
                rv = kcdb_identpro_set_searchable(vid, TRUE);
                EnterCriticalSection(&cs_ident);
                if(rv == KHM_ERROR_NO_PROVIDER ||
                    KHM_SUCCEEDED(rv)) {
                    id->flags |= KCDB_IDENT_FLAG_SEARCHABLE;
                    delta |= KCDB_IDENT_FLAG_SEARCHABLE;
                }
            }
        }

        flag &= ~KCDB_IDENT_FLAG_SEARCHABLE;
        mask &= ~KCDB_IDENT_FLAG_SEARCHABLE;
    }

    if (mask & KCDB_IDENT_FLAG_STICKY) {
        if ((flag ^ id->flags) & KCDB_IDENT_FLAG_STICKY) {
            khm_handle h_conf;

            if (KHM_SUCCEEDED(kcdb_identity_get_config(vid, 
                                                       KHM_FLAG_CREATE, 
                                                       &h_conf))) {
                khc_write_int32(h_conf, L"Sticky",
                                !!(flag & KCDB_IDENT_FLAG_STICKY));
                khc_close_space(h_conf);
            }

            id->flags =
                ((id->flags & ~KCDB_IDENT_FLAG_STICKY) |
                 (flag & KCDB_IDENT_FLAG_STICKY));

            delta |= KCDB_IDENT_FLAG_STICKY;
        }

        flag &= ~KCDB_IDENT_FLAG_STICKY;
        mask &= ~KCDB_IDENT_FLAG_STICKY;
    }

    /* deal with every other flag */

    oldflags = id->flags;

    id->flags = (id->flags & ~mask) | (flag & mask);

    if (flag & KCDB_IDENT_FLAG_VALID) {
        id->flags &= ~(KCDB_IDENT_FLAG_INVALID | KCDB_IDENT_FLAG_UNKNOWN);
    }
    if (flag & KCDB_IDENT_FLAG_INVALID) {
        id->flags &= ~(KCDB_IDENT_FLAG_VALID | KCDB_IDENT_FLAG_UNKNOWN);
    }

    /* If the NO_NOTIFY flag is turned off and the NEED_NOTIFY flag is
       present, then a notification should be triggered. */
    if ((mask & KCDB_IDENT_FLAG_NO_NOTIFY) &&
        !(flag & KCDB_IDENT_FLAG_NO_NOTIFY)) {
        id->flags &= ~KCDB_IDENT_FLAG_NEED_NOTIFY;
    }

    newflags = id->flags;

    LeaveCriticalSection(&cs_ident);

    delta |= newflags ^ oldflags;
    delta &= ~KCDB_IDENT_FLAG_NO_NOTIFY; /* Simply turning on
                                            notifications shouldn't
                                            cause a notification. */

    if((delta & KCDB_IDENT_FLAG_HIDDEN)) {
        kcdbint_ident_post_message((newflags & KCDB_IDENT_FLAG_HIDDEN)?KCDB_OP_HIDE:KCDB_OP_UNHIDE, 
                                   vid);
    }

    if((delta & KCDB_IDENT_FLAG_SEARCHABLE)) {
        kcdbint_ident_post_message((newflags & KCDB_IDENT_FLAG_SEARCHABLE)?
                                   KCDB_OP_SETSEARCH:KCDB_OP_UNSETSEARCH, 
                                   vid);
    }

    if(delta != 0 && !(newflags & KCDB_IDENT_FLAG_NO_NOTIFY))
        kcdbint_ident_post_message(KCDB_OP_MODIFY, vid);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
kcdb_identity_get_parent(khm_handle vid,
                         khm_handle *pvparent)
{
    kcdb_identity * id;

    if (!kcdb_is_identity(vid))
        return KHM_ERROR_INVALID_PARAM;

    id = (kcdb_identity *) vid;

    EnterCriticalSection(&cs_ident);
    *pvparent = (khm_handle) id->parent;
    if (*pvparent)
        kcdb_identity_hold(*pvparent);
    LeaveCriticalSection(&cs_ident);

    return (*pvparent) ? KHM_ERROR_SUCCESS : KHM_ERROR_NOT_FOUND;
}

KHMEXP khm_int32 KHMAPI
kcdb_identity_set_parent(khm_handle vid,
                         khm_handle vparent)
{
    kcdb_identity * id;
    kcdb_identity * old_parent = NULL;
    khm_boolean notify = FALSE;

    if (!kcdb_is_identity(vid) ||
        (vparent != NULL && !kcdb_is_identity(vparent)))
        return KHM_ERROR_INVALID_PARAM;

    id = (kcdb_identity *) vid;

    EnterCriticalSection(&cs_ident);
    old_parent = id->parent;
    id->parent = (kcdb_identity *) vparent;
    if (vparent)
        kcdb_identity_hold(vparent);
    notify = !(id->flags & KCDB_IDENT_FLAG_NO_NOTIFY);
    if (!notify)
        id->flags |= KCDB_IDENT_FLAG_NEED_NOTIFY;
    LeaveCriticalSection(&cs_ident);

    if (old_parent)
        kcdb_identity_release(old_parent);

    if (notify)
        kcdbint_ident_post_message(KCDB_OP_MODIFY, id);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI 
kcdb_identity_get_flags(khm_handle vid, 
                        khm_int32 * flags) {
    kcdb_identity * id;

    *flags = 0;

    if(!kcdb_is_identity(vid))
        return KHM_ERROR_INVALID_PARAM;

    id = (kcdb_identity *) vid;

    EnterCriticalSection(&cs_ident);
    *flags = id->flags;
    LeaveCriticalSection(&cs_ident);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
kcdb_identity_get_serial(khm_handle vid, khm_ui_8 * pserial)
{
    kcdb_identity * id;
    khm_int32 rv;

    EnterCriticalSection(&cs_ident);
    if (kcdb_is_identity(vid)) {
        id = (kcdb_identity *) vid;
        *pserial = id->serial;
        rv = KHM_ERROR_SUCCESS;
    } else {
        rv = KHM_ERROR_INVALID_PARAM;
    }
    LeaveCriticalSection(&cs_ident);

    return rv;
}

KHMEXP khm_int32 KHMAPI 
kcdb_identity_get_name(khm_handle vid, 
                       wchar_t * buffer, 
                       khm_size * pcbsize)
{
    size_t namesize;
    kcdb_identity * id;

    if(!kcdb_is_identity(vid) || !pcbsize)
        return KHM_ERROR_INVALID_PARAM;

    id = (kcdb_identity *) vid;

    if(FAILED(StringCbLength(id->name, KCDB_IDENT_MAXCB_NAME, &namesize)))
        return KHM_ERROR_UNKNOWN;

    namesize += sizeof(wchar_t);

    if(!buffer || namesize > *pcbsize) {
        *pcbsize = namesize;
        return KHM_ERROR_TOO_LONG;
    }

    StringCbCopy(buffer, *pcbsize, id->name);
    *pcbsize = namesize;

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
kcdb_identity_get_identpro(khm_handle h_ident,
                           khm_handle * ph_identpro)
{
    kcdb_identity * id;

    if (!kcdb_is_identity(h_ident) ||
        ph_identpro == NULL)
        return KHM_ERROR_INVALID_PARAM;

    id = (kcdb_identity *) h_ident;

    *ph_identpro = kcdb_handle_from_identpro(id->id_pro);

    if (*ph_identpro)
        kcdb_identpro_hold(*ph_identpro);

    return (*ph_identpro != NULL)? KHM_ERROR_SUCCESS : KHM_ERROR_NO_PROVIDER;
}

#pragma warning(push)
#pragma warning(disable: 4995)

KHMEXP khm_int32 KHMAPI 
kcdb_identity_get_default(khm_handle * pvid)
{
    khm_handle idpro = NULL;
    khm_int32 rv;

    if (KHM_FAILED(kcdb_identpro_get_default(&idpro)))
        return KHM_ERROR_NO_PROVIDER;

    rv = kcdb_identpro_get_default_identity(idpro, pvid);

    kcdb_identpro_release(idpro);

    return rv;
}

#pragma warning(pop)

KHMEXP khm_int32 KHMAPI
kcdb_identity_get_default_ex(khm_handle vidpro, khm_handle * pvid)
{
    return kcdb_identpro_get_default_identity(vidpro, pvid);
}

KHMEXP khm_int32 KHMAPI 
kcdb_identity_set_default(khm_handle vid) {
    return kcdb_identpro_set_default_identity(vid, TRUE);
}

KHMEXP khm_int32 KHMAPI
kcdb_identity_set_default_int(khm_handle vid) {
    return kcdb_identpro_set_default_identity(vid, FALSE);
}

/* May be called with cs_ident held. */
KHMEXP khm_int32 KHMAPI
kcdb_identity_get_config(khm_handle vid, 
                         khm_int32 flags,
                         khm_handle * result)
{
    khm_handle hkcdb;
    khm_handle hidents = NULL;
    khm_handle hident = NULL;
    khm_int32 rv;
    kcdb_identity * id;
    wchar_t cfname[KCONF_MAXCCH_NAME];
    khm_size cb;
    khm_boolean need_to_notify_provider = FALSE;

    if(kcdb_is_identity(vid)) {
        id = (kcdb_identity *) vid;
    } else {
        return KHM_ERROR_INVALID_PARAM;
    }

    cb = sizeof(cfname);
    if (KHM_FAILED(rv = kcdb_identity_get_short_name(vid, TRUE, cfname, &cb)))
        return rv;

    if (flags & KHM_FLAG_CREATE) {
        /* Identity configuration should only be created in the user
           store and should use the schema. */
        flags |= KCONF_FLAG_USER | KCONF_FLAG_SCHEMA;
    }

    EnterCriticalSection(&cs_ident);

    hkcdb = kcdb_get_config();
    if(hkcdb) {
        rv = khc_open_space(hkcdb, L"Identity", 0, &hidents);
        if(KHM_FAILED(rv))
            goto _exit;

        rv = khc_open_space(hidents, cfname, flags, &hident);

        if(KHM_FAILED(rv)) {
            if ((id->flags & KCDB_IDENT_FLAG_CONFIG) &&
                ((flags & KCONF_FLAG_USER) || flags == 0)) {
                id->flags &= ~KCDB_IDENT_FLAG_CONFIG;
                kcdbint_ident_post_message(KCDB_OP_DELCONFIG, id);
            }
            goto _exit;
        }

        if (flags & (KHM_FLAG_CREATE | KHM_PERM_WRITE)) {
            khm_size cb;

            if (khc_value_exists(hident, L"Name") & KCONF_FLAG_USER) {
                cb = sizeof(cfname);
                if (KHM_FAILED(khc_read_string(hident, L"Name", cfname, &cb)) ||
                    wcscmp(cfname, id->name)) {
                    /* For some reason, there's a mismatch between the
                       name of identity and the name specified in the
                       configuration.  We should abort. */
                    assert(FALSE);
                    khc_close_space(hident);
                    hident = NULL;
                    rv = KHM_ERROR_UNKNOWN;
                    goto _exit;
                }
            } else {
                /* Name doesn't exist. Create. */
                khc_write_string(hident, L"Name", id->name);
            }

            if (khc_value_exists(hident, L"IDProvider") & KCONF_FLAG_USER) {
                cb = sizeof(cfname);
                if (KHM_FAILED(khc_read_string(hident, L"IDProvider", cfname, &cb)) ||
                    wcscmp(cfname, id->id_pro->name)) {
                    /* Mismatch between provider of this identity and
                       the provider specified in the configuration.
                       Abort. */
                    assert(FALSE);
                    khc_close_space(hident);
                    hident = NULL;
                    rv = KHM_ERROR_UNKNOWN;
                    goto _exit;
                }
            } else {
                /* IDProvider doesn't exist. Create. */
                khc_write_string(hident, L"IDProvider", id->id_pro->name);

                /* If the IDProvider value is missing, we also assume
                   that the identity provider wasn't notified of the
                   configuration space creation and has not had a
                   chance to initialize it properly. */

                need_to_notify_provider = TRUE;
            }

            if (!khc_value_exists(hident, L"IdentSerial")) {
                /* The serial number field doesn't exist. Create */
                khc_write_int64(hident, L"IdentSerial", id->serial);
            }
        } /* flags & KHM_FLAG_CREATE | KHM_PERM_WRITE */

        if (khc_value_exists(hident, L"Name") & KCONF_FLAG_USER) {
            id->flags |= KCDB_IDENT_FLAG_CONFIG;

            if (flags & KCONF_FLAG_SHADOW)
                kcdbint_refresh_identity_timer_thresholds(id, hident);
        }

        *result = hident;
        rv = KHM_ERROR_SUCCESS;
    } else
        rv = KHM_ERROR_UNKNOWN;

 _exit:
    LeaveCriticalSection(&cs_ident);

    if (need_to_notify_provider) {
        kcdb_identpro_notify_config_create(vid);
    }

    if(hidents)
        khc_close_space(hidents);
    if(hkcdb)
        khc_close_space(hkcdb);
    return rv;
}

/*! \note cs_ident must be available. */
KHMEXP khm_int32 KHMAPI 
kcdb_identity_hold(khm_handle vid)
{
    kcdb_identity * id;

    EnterCriticalSection(&cs_ident);
    if(kcdb_is_identity(vid)) {
        id = vid;
        id->refcount++;
    } else {
        LeaveCriticalSection(&cs_ident);
        return KHM_ERROR_INVALID_PARAM;
    }
    LeaveCriticalSection(&cs_ident);
    return ERROR_SUCCESS;
}

/*! \note cs_ident must be available. */
KHMEXP khm_int32 KHMAPI 
kcdb_identity_release(khm_handle vid)
{
    kcdb_identity * id;
    khm_int32 refcount;

    EnterCriticalSection(&cs_ident);
    if(kcdb_is_identity(vid)) {
        id = vid;
        refcount = --id->refcount;
        if(refcount == 0) {
            /* We only delete identities which do not have a
               configuration. */
            if (id->refcount == 0 &&
                !(id->flags & KCDB_IDENT_FLAG_CONFIG))
                kcdb_identity_delete(vid);
        }
    } else {
        LeaveCriticalSection(&cs_ident);
        return KHM_ERROR_INVALID_PARAM;
    }
    LeaveCriticalSection(&cs_ident);
    return ERROR_SUCCESS;
}

struct kcdb_idref_result {
    kcdb_identity * ident;
    khm_int32 flags;

    khm_size  n_credentials;
    khm_size  n_id_credentials;
    khm_size  n_init_credentials;
};

/* Identity refresh callback.  Itereated over all the credentials in
   the root credentials set. */
static khm_int32 KHMAPI 
kcdbint_idref_proc(khm_handle cred, void * r)
{
    khm_handle vid;
    struct kcdb_idref_result *result;
    khm_int32 flags;
    khm_int32 ctype;

    result = (struct kcdb_idref_result *) r;

    if (KHM_SUCCEEDED(kcdb_cred_get_identity(cred, &vid))) {
        if (result->ident == (kcdb_identity *) vid) {

            result->n_credentials++;

            kcdb_cred_get_flags(cred, &flags);
            kcdb_cred_get_type(cred, &ctype);

            if (flags & KCDB_CRED_FLAG_RENEWABLE) {
                result->flags |= KCDB_IDENT_FLAG_CRED_RENEW;
            }

            if (ctype == result->ident->id_pro->cred_type) {
                result->n_id_credentials++;

                if (flags & KCDB_CRED_FLAG_INITIAL) {
                    result->n_init_credentials++;
                    result->flags |= KCDB_IDENT_FLAG_VALID;
                }

                if ((flags & (KCDB_CRED_FLAG_RENEWABLE |
                              KCDB_CRED_FLAG_INITIAL)) ==
                    (KCDB_CRED_FLAG_RENEWABLE |
                     KCDB_CRED_FLAG_INITIAL)) {
                    result->flags |= KCDB_IDENT_FLAG_RENEWABLE;
                }
            }
        }

        kcdb_identity_release(vid);
    } else {
#ifdef DEBUG
        assert(FALSE);
#endif
    }

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI 
kcdb_identity_refresh(khm_handle vid)
{
    kcdb_identity * ident = NULL;
    khm_int32 code = KHM_ERROR_SUCCESS;
    struct kcdb_idref_result result;
    khm_boolean notify = FALSE;

    EnterCriticalSection(&cs_ident);

    if (!kcdb_is_active_identity(vid)) {
        code = KHM_ERROR_INVALID_PARAM;
        goto _exit;
    }

    ident = (kcdb_identity *) vid;

    result.ident = ident;
    result.flags = 0;
    result.n_credentials = 0;
    result.n_id_credentials = 0;
    result.n_init_credentials = 0;

    notify = !(ident->flags & KCDB_IDENT_FLAG_NO_NOTIFY);
    ident->flags |= KCDB_IDENT_FLAG_NO_NOTIFY;

    LeaveCriticalSection(&cs_ident);

    kcdb_credset_apply(NULL, kcdbint_idref_proc, &result);

    if (result.n_credentials == 0)
        result.flags |= KCDB_IDENT_FLAG_EMPTY;

    kcdb_identity_set_flags(vid, result.flags,
                            KCDB_IDENT_FLAGMASK_RDWR &
                            ~(KCDB_IDENT_FLAG_DEFAULT |
                              KCDB_IDENT_FLAG_SEARCHABLE |
                              KCDB_IDENT_FLAG_STICKY));

    EnterCriticalSection(&cs_ident);

    ident->refresh_cycle = kcdb_ident_refresh_cycle;
    ident->n_credentials = result.n_credentials;
    ident->n_id_credentials = result.n_id_credentials;
    ident->n_init_credentials = result.n_init_credentials;
    ident->flags &= ~KCDB_IDENT_FLAG_NEEDREFRESH;

    if (!notify)
        ident->flags |= KCDB_IDENT_FLAG_NEED_NOTIFY;
    else
        ident->flags &= ~KCDB_IDENT_FLAG_NO_NOTIFY;

 _exit:
    LeaveCriticalSection(&cs_ident);

    if (code == 0)
        code = kcdb_identpro_update(vid);

    if (ident != 0 && code == 0 && notify)
        kcdbint_ident_post_message(KCDB_OP_MODIFY, ident);

    return code;
}

KHMEXP khm_int32 KHMAPI 
kcdb_identity_refresh_all(void)
{
    kcdb_identity * ident;
    kcdb_identity * next;
    khm_int32 code = KHM_ERROR_SUCCESS;
    int hit_count;

    EnterCriticalSection(&cs_ident);

    kcdb_ident_refresh_cycle++;

    /* The do-while loop is here to account for race conditions.  We
       release cs_ident in the for loop, so we don't actually have a
       guarantee that we traversed the whole identity list at the end.
       We repeat until all the identities are uptodate. */

    do {
        hit_count = 0;

        for (ident = kcdb_identities; 
             ident != NULL;
             ident = next) {

            if (!kcdb_is_active_identity(ident) ||
                ident->refresh_cycle == kcdb_ident_refresh_cycle ||
                !(ident->flags & KCDB_IDENT_FLAG_NEEDREFRESH)) {

                ident->refresh_cycle = kcdb_ident_refresh_cycle;
                next = LNEXT(ident);
                continue;
            }

            kcdb_identity_hold((khm_handle) ident);
            next = LNEXT(ident);

            LeaveCriticalSection(&cs_ident);

            kcdb_identity_refresh((khm_handle) ident);

            EnterCriticalSection(&cs_ident);

            kcdb_identity_release((khm_handle) ident);

            hit_count++;
        }

    } while (hit_count > 0);

    LeaveCriticalSection(&cs_ident);

    return code;
}


/*****************************************/
/* Custom property functions             */

khm_int32
kcdbint_ident_attr_cb(khm_handle h,
                      khm_int32 attr,
                      void * buf,
                      khm_size *pcb_buf)
{
    kcdb_identity * id;

    id = (kcdb_identity *) h;

    switch (attr) {
    case KCDB_ATTR_NAME:
        return kcdb_identity_get_name(h, buf, pcb_buf);

    case KCDB_ATTR_TYPE:
        if (id->id_pro == NULL ||
            id->id_pro->cred_type <= 0)
            return KHM_ERROR_NOT_FOUND;

        if (buf && *pcb_buf >= sizeof(khm_int32)) {
            *pcb_buf = sizeof(khm_int32);
            *((khm_int32 *) buf) = id->id_pro->cred_type;
            return KHM_ERROR_SUCCESS;
        } else {
            *pcb_buf = sizeof(khm_int32);
            return KHM_ERROR_TOO_LONG;
        }

    case KCDB_ATTR_TYPE_NAME:
        if (id->id_pro == NULL ||
            id->id_pro->cred_type <= 0)
            return KHM_ERROR_NOT_FOUND;

        return kcdb_credtype_describe(id->id_pro->cred_type, buf, 
                                      pcb_buf, KCDB_TS_SHORT);

    case KCDB_ATTR_LIFETIME:
        {
            khm_int32 rv = KHM_ERROR_SUCCESS;
            khm_size slot_issue;
            khm_size slot_exp;

            EnterCriticalSection(&cs_ident);

            if((slot_issue = kcdb_buf_slot_by_id(&id->buf, (khm_ui_2) KCDB_ATTR_ISSUE))
               == KCDB_BUF_INVALID_SLOT ||
               !kcdb_buf_val_exist(&id->buf, slot_issue) ||
               (slot_exp = kcdb_buf_slot_by_id(&id->buf, (khm_ui_2) KCDB_ATTR_EXPIRE))
               == KCDB_BUF_INVALID_SLOT ||
               !kcdb_buf_val_exist(&id->buf, slot_exp)) {

                rv = KHM_ERROR_NOT_FOUND;
            } else if (!buf || *pcb_buf < sizeof(FILETIME)) {
                *pcb_buf = sizeof(FILETIME);
                rv = KHM_ERROR_TOO_LONG;
            } else {
                *((FILETIME *) buf) =
                    FtSub((FILETIME *) kcdb_buf_get(&id->buf,slot_exp),
                          (FILETIME *) kcdb_buf_get(&id->buf,slot_issue));
                *pcb_buf = sizeof(FILETIME);
            }
            LeaveCriticalSection(&cs_ident);

            return rv;
        }

    case KCDB_ATTR_TIMELEFT:
        {
            khm_int32 rv = KHM_ERROR_SUCCESS;
            khm_size slot;

            EnterCriticalSection(&cs_ident);

            if((slot = kcdb_buf_slot_by_id(&id->buf, (khm_ui_2) KCDB_ATTR_EXPIRE))
               == KCDB_BUF_INVALID_SLOT ||
               !kcdb_buf_val_exist(&id->buf, slot)) {

                rv = KHM_ERROR_NOT_FOUND;
            } else if (!buf || *pcb_buf < sizeof(FILETIME)) {
                *pcb_buf = sizeof(FILETIME);
                rv = KHM_ERROR_TOO_LONG;
            } else {
                FILETIME ftc;
                khm_int64 iftc;

                GetSystemTimeAsFileTime(&ftc);
                iftc = FtToInt(&ftc);

                *((FILETIME *) buf) =
                    IntToFt(FtToInt((FILETIME *) 
                                    kcdb_buf_get(&id->buf,slot))
                            - iftc);
                *pcb_buf = sizeof(FILETIME);
            }
            LeaveCriticalSection(&cs_ident);

            return rv;
        }

    case KCDB_ATTR_RENEW_TIMELEFT:
        {
            khm_int32 rv = KHM_ERROR_SUCCESS;
            khm_size slot;
                
            EnterCriticalSection(&cs_ident);
            if((slot = kcdb_buf_slot_by_id(&id->buf, (khm_ui_2) KCDB_ATTR_RENEW_EXPIRE))
               == KCDB_BUF_INVALID_SLOT ||
               !kcdb_buf_val_exist(&id->buf, slot)) {

                rv = KHM_ERROR_NOT_FOUND;
            } else if(!buf || *pcb_buf < sizeof(FILETIME)) {
                *pcb_buf = sizeof(FILETIME);
                rv = KHM_ERROR_TOO_LONG;
            } else {
                FILETIME ftc;
                khm_int64 i_re;
                khm_int64 i_ct;

                GetSystemTimeAsFileTime(&ftc);

                i_re = FtToInt(((FILETIME *)
                                kcdb_buf_get(&id->buf, slot)));
                i_ct = FtToInt(&ftc);

                if (i_re > i_ct)
                    *((FILETIME *) buf) =
                        IntToFt(i_re - i_ct);
                else
                    *((FILETIME *) buf) =
                        IntToFt(0);

                *pcb_buf = sizeof(FILETIME);
            }
            LeaveCriticalSection(&cs_ident);

            return rv;
        }

    case KCDB_ATTR_FLAGS:
        if(buf && *pcb_buf >= sizeof(khm_int32)) {
            *pcb_buf = sizeof(khm_int32);

            EnterCriticalSection(&cs_ident);
            *((khm_int32 *) buf) = id->flags;
            LeaveCriticalSection(&cs_ident);

            return KHM_ERROR_SUCCESS;
        } else {
            *pcb_buf = sizeof(khm_int32);
            return KHM_ERROR_TOO_LONG;
        }
        
    case KCDB_ATTR_LAST_UPDATE:
        if (buf && *pcb_buf >= sizeof(FILETIME)) {

            *pcb_buf = sizeof(FILETIME);
            EnterCriticalSection(&cs_ident);
            *((FILETIME *) buf) = id->ft_lastupdate;
            LeaveCriticalSection(&cs_ident);

            return KHM_ERROR_SUCCESS;
        } else {
            *pcb_buf = sizeof(FILETIME);
            return KHM_ERROR_TOO_LONG;
        }

    case KCDB_ATTR_N_CREDS:
        if (buf && *pcb_buf >= sizeof(khm_int32)) {
            *pcb_buf = sizeof(khm_int32);
            EnterCriticalSection(&cs_ident);
            *((khm_int32 *) buf) = (khm_int32) id->n_credentials;
            LeaveCriticalSection(&cs_ident);

            return KHM_ERROR_SUCCESS;
        } else {
            *pcb_buf = sizeof(khm_int32);
            return KHM_ERROR_TOO_LONG;
        }

    case KCDB_ATTR_N_IDCREDS:
        if (buf && *pcb_buf >= sizeof(khm_int32)) {
            *pcb_buf = sizeof(khm_int32);
            EnterCriticalSection(&cs_ident);
            *((khm_int32 *) buf) = (khm_int32) id->n_id_credentials;
            LeaveCriticalSection(&cs_ident);

            return KHM_ERROR_SUCCESS;
        } else {
            *pcb_buf = sizeof(khm_int32);
            return KHM_ERROR_TOO_LONG;
        }

    case KCDB_ATTR_N_INITCREDS:
        if (buf && *pcb_buf >= sizeof(khm_int32)) {
            *pcb_buf = sizeof(khm_int32);
            EnterCriticalSection(&cs_ident);
            *((khm_int32 *) buf) = (khm_int32) id->n_init_credentials;
            LeaveCriticalSection(&cs_ident);

            return KHM_ERROR_SUCCESS;
        } else {
            *pcb_buf = sizeof(khm_int32);
            return KHM_ERROR_TOO_LONG;
        }

    case KCDB_ATTR_THR_RENEW:
        if (buf && *pcb_buf >= sizeof(FILETIME)) {

            *pcb_buf = sizeof(FILETIME);
            EnterCriticalSection(&cs_ident);
            *((FILETIME *) buf) = id->ft_thr_renew;
            LeaveCriticalSection(&cs_ident);

            return KHM_ERROR_SUCCESS;
        } else {
            *pcb_buf = sizeof(FILETIME);
            return KHM_ERROR_TOO_LONG;
        }

    case KCDB_ATTR_THR_WARN:
        if (buf && *pcb_buf >= sizeof(FILETIME)) {

            *pcb_buf = sizeof(FILETIME);
            EnterCriticalSection(&cs_ident);
            *((FILETIME *) buf) = id->ft_thr_warn;
            LeaveCriticalSection(&cs_ident);

            return KHM_ERROR_SUCCESS;
        } else {
            *pcb_buf = sizeof(FILETIME);
            return KHM_ERROR_TOO_LONG;
        }

    case KCDB_ATTR_THR_CRIT:
        if (buf && *pcb_buf >= sizeof(FILETIME)) {

            *pcb_buf = sizeof(FILETIME);
            EnterCriticalSection(&cs_ident);
            *((FILETIME *) buf) = id->ft_thr_crit;
            LeaveCriticalSection(&cs_ident);

            return KHM_ERROR_SUCCESS;
        } else {
            *pcb_buf = sizeof(FILETIME);
            return KHM_ERROR_TOO_LONG;
        }

    default:
        return KHM_ERROR_NOT_FOUND;
    }
}

KHMEXP khm_int32 KHMAPI 
kcdb_identity_set_attr(khm_handle vid,
                       khm_int32 attr_id,
                       const void *buffer,
                       khm_size cbbuf)
{
    kcdb_identity * id = NULL;
    kcdb_attrib * attrib = NULL;
    kcdb_type * type = NULL;
    khm_size slot;
    khm_size cbdest;
    khm_int32 code = KHM_ERROR_SUCCESS;
    khm_boolean notify = FALSE;

    EnterCriticalSection(&cs_ident);
    if(!kcdb_is_identity(vid)) {
        LeaveCriticalSection(&cs_ident);
        return KHM_ERROR_INVALID_PARAM;
    }

    id = (kcdb_identity *) vid;

    if(!(id->flags & KCDB_IDENT_FLAG_ATTRIBS)) {
        kcdb_buf_new(&id->buf, KCDB_BUF_DEFAULT);
        id->flags |= KCDB_IDENT_FLAG_ATTRIBS;
    }

    if(KHM_FAILED(kcdb_attrib_get_info(attr_id, &attrib))) {
        LeaveCriticalSection(&cs_ident);
        return KHM_ERROR_INVALID_PARAM;
    }

    if(attrib->flags & KCDB_ATTR_FLAG_COMPUTED)
    {
        LeaveCriticalSection(&cs_ident);
        kcdb_attrib_release_info(attrib);
        return KHM_ERROR_INVALID_OPERATION;
    }

    if (buffer == NULL) {
        /* we are removing a value */
        slot = kcdb_buf_slot_by_id(&id->buf, (khm_ui_2) attr_id);
        if (slot != KCDB_BUF_INVALID_SLOT &&
            kcdb_buf_exist(&id->buf, slot)) {
            kcdb_buf_alloc(&id->buf, slot, (khm_ui_2) attr_id, 0);
            notify = TRUE;
        }
        code = KHM_ERROR_SUCCESS;
        goto _exit;
    }

    if(KHM_FAILED(kcdb_type_get_info(attrib->type, &type))) {
        LeaveCriticalSection(&cs_ident);
        kcdb_attrib_release_info(attrib);
        return KHM_ERROR_INVALID_PARAM;
    }

    if(!(type->isValid(buffer,cbbuf))) {
        code = KHM_ERROR_TYPE_MISMATCH;
        goto _exit;
    }

    if((type->dup(buffer, cbbuf, NULL, &cbdest)) != KHM_ERROR_TOO_LONG) {
        code = KHM_ERROR_INVALID_PARAM;
        goto _exit;
    }

    kcdb_buf_alloc(&id->buf, KCDB_BUF_APPEND, (khm_ui_2) attr_id, cbdest);
    slot = kcdb_buf_slot_by_id(&id->buf, (khm_ui_2) attr_id);
    if(slot == KCDB_BUF_INVALID_SLOT || !kcdb_buf_exist(&id->buf, slot)) {
        code = KHM_ERROR_NO_RESOURCES;
        goto _exit;
    }

    if(KHM_FAILED(code =
        type->dup(buffer, cbbuf, kcdb_buf_get(&id->buf, slot), &cbdest)))
    {
        kcdb_buf_alloc(&id->buf, slot, (khm_ui_2) attr_id, 0);
        goto _exit;
    }

    kcdb_buf_set_value_flag(&id->buf, slot);
    notify = TRUE;

_exit:
    LeaveCriticalSection(&cs_ident);

    if(attrib)
        kcdb_attrib_release_info(attrib);
    if(type)
        kcdb_type_release_info(type);
    if (notify && id)
        kcdbint_ident_post_message(KCDB_OP_MODIFY, id);

    return code;
}

KHMEXP khm_int32 KHMAPI 
kcdb_identity_set_attrib(khm_handle vid,
                         const wchar_t * attr_name,
                         const void * buffer,
                         khm_size cbbuf)
{
    khm_int32 attr_id = -1;

    if(KHM_FAILED(kcdb_attrib_get_id(attr_name, &attr_id)))
        return KHM_ERROR_INVALID_PARAM;

    return kcdb_identity_set_attr(
        vid,
        attr_id,
        buffer,
        cbbuf);
}

KHMEXP khm_int32 KHMAPI 
kcdb_identity_get_attr(khm_handle vid,
                       khm_int32 attr_id,
                       khm_int32 * attr_type,
                       void * buffer,
                       khm_size * pcbbuf)
{
    khm_int32 code = KHM_ERROR_SUCCESS;
    kcdb_identity * id = NULL;
    kcdb_attrib * attrib = NULL;
    kcdb_type * type = NULL;
    khm_size slot = 0;

    if(KHM_FAILED(kcdb_attrib_get_info(attr_id, &attrib))) {
        return KHM_ERROR_INVALID_PARAM;
    }

    if(KHM_FAILED(kcdb_type_get_info(attrib->type, &type))) {
        kcdb_attrib_release_info(attrib);
        return KHM_ERROR_UNKNOWN;
    }

    if(attr_type)
        *attr_type = attrib->type;

    EnterCriticalSection(&cs_ident);

    if(!kcdb_is_identity(vid)) {
        code = KHM_ERROR_INVALID_PARAM;
        goto _exit;
    }

    id = (kcdb_identity *) vid;

    if (!(attrib->flags & KCDB_ATTR_FLAG_COMPUTED) &&
        (!(id->flags & KCDB_IDENT_FLAG_ATTRIBS) ||
         (slot = kcdb_buf_slot_by_id(&id->buf, (khm_ui_2) attr_id)) == KCDB_BUF_INVALID_SLOT ||
         !kcdb_buf_val_exist(&id->buf, slot))) {
        code = KHM_ERROR_NOT_FOUND;
        goto _exit;
    }

    if(!buffer && !pcbbuf) {
        /* in this case the caller is only trying to determine if the
           field contains data.  If we get here, then the value
           exists. */
        code = KHM_ERROR_SUCCESS;
        goto _exit;
    }

    if(attrib->flags & KCDB_ATTR_FLAG_COMPUTED) {
        code = attrib->compute_cb(vid, attr_id, buffer, pcbbuf);
    } else {
        code = type->dup(kcdb_buf_get(&id->buf, slot),
                         kcdb_buf_size(&id->buf, slot),
                         buffer, pcbbuf);
    }

_exit:
    LeaveCriticalSection(&cs_ident);
    if(type)
        kcdb_type_release_info(type);
    if(attrib)
        kcdb_attrib_release_info(attrib);

    return code;
}

KHMEXP khm_int32 KHMAPI 
kcdb_identity_get_attrib(khm_handle vid,
                         const wchar_t * attr_name,
                         khm_int32 * attr_type,
                         void * buffer,
                         khm_size * pcbbuf)
{
    khm_int32 attr_id = -1;

    if(KHM_FAILED(kcdb_attrib_get_id(attr_name, &attr_id)))
        return KHM_ERROR_NOT_FOUND;

    return kcdb_identity_get_attr(vid,
                                  attr_id,
                                  attr_type,
                                  buffer,
                                  pcbbuf);
}

KHMEXP khm_int32 KHMAPI 
kcdb_identity_get_attr_string(khm_handle vid,
                              khm_int32 attr_id,
                              wchar_t * buffer,
                              khm_size * pcbbuf,
                              khm_int32 flags)
{
    khm_int32 code = KHM_ERROR_SUCCESS;
    kcdb_identity * id = NULL;
    kcdb_attrib * attrib = NULL;
    kcdb_type * type = NULL;
    khm_size slot = 0;

    if(KHM_FAILED(kcdb_attrib_get_info(attr_id, &attrib))) {
        return KHM_ERROR_INVALID_PARAM;
    }

    if(KHM_FAILED(kcdb_type_get_info(attrib->type, &type))) {
        kcdb_attrib_release_info(attrib);
        return KHM_ERROR_UNKNOWN;
    }

    EnterCriticalSection(&cs_ident);

    if(!kcdb_is_identity(vid)) {
        code = KHM_ERROR_INVALID_PARAM;
        goto _exit;
    }

    id = (kcdb_identity *) vid;

    if (!(attrib->flags & KCDB_ATTR_FLAG_COMPUTED) &&
        (!(id->flags & KCDB_IDENT_FLAG_ATTRIBS) ||
         (slot = kcdb_buf_slot_by_id(&id->buf, (khm_ui_2) attr_id)) == KCDB_BUF_INVALID_SLOT ||
         !kcdb_buf_val_exist(&id->buf, slot))) {
        code = KHM_ERROR_NOT_FOUND;
        goto _exit;
    }

    if(!buffer && !pcbbuf) {
        /* in this case the caller is only trying to determine if the
           field contains data.  If we get here, then the value
           exists.  We assume that computed values always exist. */
        code = KHM_ERROR_SUCCESS;
        goto _exit;
    }

    if(attrib->flags & KCDB_ATTR_FLAG_COMPUTED) {
        khm_size cbbuf;

        code = attrib->compute_cb(vid, attr_id, NULL, &cbbuf);
        if (code == KHM_ERROR_TOO_LONG) {
            wchar_t vbuf[KCDB_MAXCCH_NAME];
            void * buf;

            if (cbbuf < sizeof(vbuf))
                buf = vbuf;
            else
                buf = PMALLOC(cbbuf);

            code = attrib->compute_cb(vid, attr_id, buf, &cbbuf);
            if (KHM_SUCCEEDED(code)) {
                code = type->toString(buf, cbbuf, buffer, pcbbuf, flags);
            }

            if (buf != vbuf)
                PFREE(buf);
        }
    } else {
        if (kcdb_buf_exist(&id->buf, slot)) {
            code = type->toString(kcdb_buf_get(&id->buf, slot),
                                  kcdb_buf_size(&id->buf, slot),
                                  buffer, pcbbuf, flags);
        } else
            code = KHM_ERROR_NOT_FOUND;
    }

 _exit:
    LeaveCriticalSection(&cs_ident);
    if(type)
        kcdb_type_release_info(type);
    if(attrib)
        kcdb_attrib_release_info(attrib);

    return code;
}

KHMEXP khm_int32 KHMAPI 
kcdb_identity_get_attrib_string(khm_handle vid,
                                const wchar_t * attr_name,
                                wchar_t * buffer,
                                khm_size * pcbbuf,
                                khm_int32 flags)
{
    khm_int32 attr_id = -1;

    if(KHM_FAILED(kcdb_attrib_get_id(attr_name, &attr_id)))
        return KHM_ERROR_NOT_FOUND;

    return kcdb_identity_get_attr_string(
        vid,
        attr_id,
        buffer,
        pcbbuf,
        flags);
}

/* Called with cs_ident held */
static void
check_config_for_identities(void)
{

 check_again:

    if (!kcdb_checking_config && !kcdb_checked_config) {
        khm_handle h_kcdb = NULL;
        khm_handle h_idents = NULL;
        khm_handle h_ident = NULL;
        khm_boolean try_again = FALSE;

        kcdb_checking_config = TRUE;

        h_kcdb = kcdb_get_config();
        if (!h_kcdb)
            goto _config_check_cleanup;
        if(KHM_FAILED(khc_open_space(h_kcdb, L"Identity", 0, &h_idents)))
            goto _config_check_cleanup;

        while (KHM_SUCCEEDED(khc_enum_subspaces(h_idents,
                                                h_ident,
                                                &h_ident))) {

            wchar_t wname[KCDB_IDENT_MAXCCH_NAME];
            khm_size cb;
            khm_handle t_id;

            if (khc_value_exists(h_ident, L"Name") &&
                khc_value_exists(h_ident, L"IDProvider")) {

                wchar_t wprov[KCDB_MAXCCH_NAME];
                khm_handle idpro = NULL;

                cb = sizeof(wprov);
                if (KHM_FAILED(khc_read_string(h_ident, L"IDProvider", wprov, &cb)))
                    continue;

                cb = sizeof(wname);
                if (KHM_FAILED(khc_read_string(h_ident, L"Name", wname, &cb)))
                    continue;

                LeaveCriticalSection(&cs_ident);

                if (KHM_FAILED(kcdb_identpro_find(wprov, &idpro))) {
                    EnterCriticalSection(&cs_ident);

                    /* If we try to enumerate identities before the
                       specific identity provider has been loaded,
                       then we will probably fail to create the
                       corresponding identities. */
                    try_again = TRUE;
                    continue;
                }

                if (KHM_SUCCEEDED(kcdb_identity_create_ex(idpro, wname,
                                                          KCDB_IDENT_FLAG_CREATE,
                                                          NULL, &t_id)))
                    kcdb_identity_release(t_id);

                kcdb_identpro_release(idpro);

                EnterCriticalSection(&cs_ident);

            } else {
                cb = sizeof(wname);
                if (KHM_FAILED(khc_get_config_space_name(h_ident, wname, &cb)))
                    continue;

                LeaveCriticalSection(&cs_ident);

                if (KHM_SUCCEEDED(kcdb_identity_create(wname,
                                                       KCDB_IDENT_FLAG_CREATE |
                                                       KCDB_IDENT_FLAG_CONFIG,
                                                       &t_id)))
                    kcdb_identity_release(t_id);
                else
                    try_again = TRUE;

                EnterCriticalSection(&cs_ident);
            }
        }

    _config_check_cleanup:
        if (h_kcdb)
            khc_close_space(h_kcdb);
        if (h_idents)
            khc_close_space(h_idents);

        kcdb_checking_config = FALSE;
        kcdb_checked_config = !try_again;

        SetEvent(kcdb_config_check_event);

    } else if (kcdb_checking_config) {
        /* some other thread is already checking the configuration.
           We should wait for it to complete */
        LeaveCriticalSection(&cs_ident);
        WaitForSingleObject(kcdb_config_check_event, INFINITE);
        EnterCriticalSection(&cs_ident);

        goto check_again;
    } else {
        /* The configuration has already been checked.  We don't need
           to do anything. */
    }
}

#pragma warning(push)
#pragma warning(disable: 4995)

KHMEXP khm_int32 KHMAPI 
kcdb_identity_enum(khm_int32 and_flags,
                   khm_int32 eq_flags,
                   wchar_t * name_buf,
                   khm_size * pcb_buf,
                   khm_size * pn_idents)
{
    kcdb_identity * id;
    khm_int32 rv = KHM_ERROR_SUCCESS;
    khm_size cb_req = 0;
    khm_size n_idents = 0;
    size_t cb_curr;
    size_t cch_left;

    if ((name_buf == NULL && pcb_buf == NULL && pn_idents == NULL) ||
        (name_buf != NULL && pcb_buf == NULL))
        return KHM_ERROR_INVALID_PARAM;

    eq_flags &= and_flags;

    EnterCriticalSection(&cs_ident);

    check_config_for_identities();

    for ( id = kcdb_identities;
          id != NULL;
          id = LNEXT(id) ) {

        if ((id->flags & KCDB_IDENT_FLAG_ACTIVE) == KCDB_IDENT_FLAG_ACTIVE &&
            (id->flags & and_flags) == eq_flags) {

            n_idents ++;

            rv = kcdb_identity_get_short_name((khm_handle) id, FALSE, NULL, &cb_curr);
            assert(rv == KHM_ERROR_TOO_LONG);

            cb_req += cb_curr;
        }
    }

    cb_req += sizeof(wchar_t);

    if (pn_idents != NULL)
        *pn_idents = n_idents;

    if (pcb_buf != NULL && (name_buf == NULL || *pcb_buf < cb_req)) {
        *pcb_buf = cb_req;

        rv = KHM_ERROR_TOO_LONG;
    } else if(name_buf != NULL) {
        cch_left = (*pcb_buf) / sizeof(wchar_t);

        for (id = kcdb_identities;
             id != NULL;
             id = LNEXT(id)) {

            if ((id->flags & KCDB_IDENT_FLAG_ACTIVE) == KCDB_IDENT_FLAG_ACTIVE &&
                (id->flags & and_flags) == eq_flags) {

                cb_curr = cch_left * sizeof(wchar_t);
                rv = kcdb_identity_get_short_name((khm_handle) id, FALSE,
                                                  name_buf, &cb_curr);
                assert(KHM_SUCCEEDED(rv));
                cch_left -= cb_curr / sizeof(wchar_t);
                name_buf += cb_curr / sizeof(wchar_t);
            }
        }

        *name_buf = L'\0';
        *pcb_buf = cb_req;
    }

    LeaveCriticalSection(&cs_ident);

    return rv;
}

#pragma warning(pop)

KHMEXP khm_int32 KHMAPI
kcdb_identity_begin_enum(khm_int32 and_flags,
                         khm_int32 eq_flags,
                         kcdb_enumeration * pe,
                         khm_size * pn_identites)
{
    kcdb_identity * id;
    kcdb_enumeration e = NULL;
    khm_size n_ids = 0;
    khm_size i;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if (pe == NULL)
        return KHM_ERROR_INVALID_PARAM;

    eq_flags &= and_flags;

    EnterCriticalSection(&cs_ident);

    check_config_for_identities();

    n_ids = 0;
    for (id = kcdb_identities; id != NULL; id = LNEXT(id))
        if ((id->flags & KCDB_IDENT_FLAG_ACTIVE) == KCDB_IDENT_FLAG_ACTIVE &&
            (id->flags & and_flags) == eq_flags)
            n_ids ++;

    if (n_ids == 0) {
        rv = KHM_ERROR_NOT_FOUND;
        goto _exit;
    }

    kcdbint_enum_create(&e);
    kcdbint_enum_alloc(e, n_ids);

    for (i=0, id = kcdb_identities; id != NULL; id = LNEXT(id))
        if ((id->flags & KCDB_IDENT_FLAG_ACTIVE) == KCDB_IDENT_FLAG_ACTIVE &&
            (id->flags & and_flags) == eq_flags) {

            assert(i < n_ids);

            if (i >= n_ids)
                break;

            e->objs[i++] = (khm_handle) id;
            kcdb_identity_hold(id);
        }

 _exit:
    LeaveCriticalSection(&cs_ident);

    *pe = e;

    if (pn_identites)
        *pn_identites = n_ids;

    return rv;
}

