/*
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
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AND
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* $Id$ */

#include "module.h"
#include <assert.h>

/* Protected by cs_ks */
khm_size n_keystores;
khm_size nc_keystores;
keystore_t ** keystores = NULL;
#define KEYSTORE_LIST_ALLOC_SIZE 4

key_type_map_ * key_type_map = NULL;
khm_size n_key_type_map = 0;
khm_size nc_key_type_map = 0;

#define HT_IDENTITY_TO_KEYSTORE_SIZE 11
hashtable * ht_identity_to_keystore = NULL;

/* Functions for handling our credentials type. */

khm_int32 KHMAPI
cred_is_equal(khm_handle cred1,
              khm_handle cred2,
              void * rock) {

    khm_int32 result;

    /* TODO: Check any additional fields to determine if the two
       credentials are equal or not. */

    /* Note that this is actually a comparison function.  It should
       return 0 if the credentials are found to be equal, and non-zero
       if they are not.  We just set this to 0 if we don't need to
       check any additional fields and accept the two credentials as
       being equal.  By the time this function is called, the
       identity, name and type of the credentials have already been
       found to be equal. */
    result = 0;

    return result;
}

khm_int32 KHMAPI
idk_cred_is_equal(khm_handle cred1,
                  khm_handle cred2,
                  void * rock) {

    khm_int32 result;

    result = 0;

    return result;
}


static khm_int32 hash_ptr(const void * key)
{
    return (khm_int32) (((khm_size)key >> 2) & 0x7fffffff);
}

static khm_int32 comp_ptr(const void *k1, const void *k2)
{
	return (k1 > k2)? -1 : ((k1 == k2)? 0 : 1);
}

static void add_ref_ks(const void *k, void * d)
{
    ks_keystore_hold((keystore_t *) d);
}

static void del_ref_ks(const void *k, void * d)
{
    ks_keystore_release((keystore_t *) d);
}

DECLARE_ONCE(ctype_init);

void
init_credtype(void)
{
    if (InitializeOnce(&ctype_init)) {
        ht_identity_to_keystore = hash_new_hashtable(HT_IDENTITY_TO_KEYSTORE_SIZE,
                                                     hash_ptr, comp_ptr,
                                                     add_ref_ks, del_ref_ks);
        InitializeOnceDone(&ctype_init);
    }
}

keystore_t *
find_keystore_for_identity(khm_handle identity)
{
    keystore_t *ks = NULL;

    init_credtype();

    EnterCriticalSection(&cs_ks);
    ks = (keystore_t *) hash_lookup(ht_identity_to_keystore, identity);
    if (ks)
        ks_keystore_hold(ks);
    LeaveCriticalSection(&cs_ks);

    return ks;
}

/* Obtains cs_ks and ks->cs */
khm_int32
associate_keystore_and_identity(keystore_t * ks, khm_handle identity)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;
    assert(is_keystore_t(ks));

    init_credtype();

    EnterCriticalSection(&cs_ks);
    KSLOCK(ks);

    if (ks->identity != NULL) {
        assert(kcdb_identity_is_equal(ks->identity, identity));
        assert((keystore_t *) hash_lookup(ht_identity_to_keystore, identity) == ks);

        if (ks->identity != identity) {
            rv = KHM_ERROR_DUPLICATE;
        }
    } else {
        hash_add(ht_identity_to_keystore, identity, ks);
        ks->identity = identity;
        kcdb_identity_hold(identity);

        if (n_keystores == nc_keystores) {
            nc_keystores = UBOUNDSS(n_keystores + 1, KEYSTORE_LIST_ALLOC_SIZE,
                                    KEYSTORE_LIST_ALLOC_SIZE);
            keystores = realloc(keystores, sizeof(keystores[0]) * nc_keystores);
        }
        keystores[n_keystores++] = ks;
        ks_keystore_hold(ks);
    }

    KSUNLOCK(ks);
    LeaveCriticalSection(&cs_ks);

    return rv;
}

khm_handle
create_default_keystore(void)
{
    keystore_t * ks = NULL;
    khm_handle identity = NULL;

    ks = ks_keystore_create_new();

    {
        wchar_t idname[KCDB_MAXCCH_NAME];
        LoadString(hResModule, IDS_DEF_KSNAME, idname, ARRAYLENGTH(idname));
        ks_keystore_set_string(ks, KCDB_RES_DISPLAYNAME, idname);
    }
    {
        wchar_t desc[KCDB_MAXCCH_SHORT_DESC];
        LoadString(hResModule, IDS_DEF_KSDESCF, desc, ARRAYLENGTH(desc));
        ks_keystore_set_string(ks, KCDB_RES_DESCRIPTION, desc);
    }
    ks_keystore_set_string(ks, KCDB_ATTR_LOCATION, KS_DEFAULT_KEYSTORE_LOCATION);
    ks_keystore_set_flags(ks, KS_FLAG_MODIFIED, KS_FLAG_MODIFIED);

    identity = create_identity_from_keystore(ks);

    kcdb_identity_set_attr(identity, KCDB_ATTR_LOCATION,
                           KS_DEFAULT_KEYSTORE_LOCATION, KCDB_CBSIZE_AUTO);

    save_keystore_with_identity(ks);

    ks_keystore_release(ks);

    kcdb_identity_set_default(identity);

    return identity;
}

/* MUST NOT be called with cs_ks held */
/* Obtains ks->cs */
khm_handle
create_identity_from_keystore(keystore_t * ks)
{
    khm_handle identity = NULL;

    KSLOCK(ks);
    if (ks->identity) {
        identity = ks->identity;
        kcdb_identity_hold(identity);
    }
    KSUNLOCK(ks);

    if (identity)
        return identity;

    /* if we didn't find an identity that matches the keystore, we
       have to create it. */
    assert(h_idprov != NULL);

    {
        UUID uuid;
        RPC_STATUS rst;

        rst = UuidCreate(&uuid);
        if (rst == RPC_S_OK || rst == RPC_S_UUID_LOCAL_ONLY) {
            wchar_t * uuidstr = NULL;

            rst = UuidToString(&uuid, &uuidstr);
            if (rst == RPC_S_OK) {
                kcdb_identity_create_ex(h_idprov, uuidstr, KCDB_IDENT_FLAG_CREATE,
                                        (void *) ks, &identity);
                RpcStringFree(&uuidstr);
            }
        }
    }

    return identity;
}

void
write_keystore_to_location(keystore_t * ks, const wchar_t * path, khm_handle csp)
{
    void * buffer = NULL;
    size_t cb_buffer = 0;

    if (!wcsncmp(path, L"FILE:", 5)) {
        HANDLE hf;
        DWORD numwritten;
        wchar_t filepath[MAX_PATH];

        if (ExpandEnvironmentStrings(path + 5, filepath, ARRAYLENGTH(filepath)) == 0) {
            return;
        }

        hf = CreateFile(filepath, GENERIC_WRITE,
                        FILE_SHARE_DELETE,
                        NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_ARCHIVE, NULL);

        if (hf == INVALID_HANDLE_VALUE)
            return;

        ks_keystore_serialize(ks, NULL, &cb_buffer);

        if (cb_buffer) {
            buffer = malloc(cb_buffer);
            ks_keystore_serialize(ks, buffer, &cb_buffer);
            WriteFile(hf, buffer, (DWORD) cb_buffer, &numwritten, NULL);
            assert(numwritten == cb_buffer);
            free(buffer);
            buffer = NULL;
        }

        CloseHandle(hf);
    } else if (!wcsncmp(path, L"REG:", 4)) {
        ks_keystore_serialize(ks, NULL, &cb_buffer);

        if (cb_buffer) {
            buffer = malloc(cb_buffer);
            ks_keystore_serialize(ks, buffer, &cb_buffer);
            khc_write_binary(csp, L"KeystoreData", buffer, cb_buffer);
            free(buffer);
            buffer = NULL;
        }
    } else {
        /* unknown location type */
        assert(FALSE);
    }

    ks_keystore_set_string(ks, KCDB_ATTR_LOCATION, path);
    ks_keystore_set_flags(ks, KS_FLAG_MODIFIED, 0);

    if (buffer)
        free(buffer);
}


keystore_t *
create_keystore_from_location(const wchar_t * path, khm_handle csp)
{
    keystore_t * ks = NULL;

    void * buffer = NULL;
    size_t cb_buffer = 0;

    if (!wcsncmp(path, L"FILE:", 5)) {
        HANDLE hf;
        DWORD cb_read;
        wchar_t filepath[MAX_PATH];

        if (ExpandEnvironmentStrings(path + 5, filepath, ARRAYLENGTH(filepath)) == 0) {
            return NULL;
        }

        hf = CreateFile(filepath, GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING,
                        FILE_FLAG_SEQUENTIAL_SCAN, NULL);

        if (hf == INVALID_HANDLE_VALUE)
            return NULL;

        /* Check for file size.  We only deal with files that are less
           than or equal to MAX_SERIALIZED_KEYSTORE_SIZE bytes. */
        {
            LARGE_INTEGER llsize;
            LARGE_INTEGER llmax;

            llmax.QuadPart = MAX_SERIALIZED_KEYSTORE_SIZE;

            if (!GetFileSizeEx(hf, &llsize) ||
                llsize.QuadPart > llmax.QuadPart) {
                goto done_file;
            }

            cb_buffer = (size_t) llsize.QuadPart;
            buffer = malloc(cb_buffer);

            if (buffer == NULL)
                goto done_file;
        }

        if (!ReadFile(hf, buffer, (DWORD) cb_buffer, &cb_read, NULL))
            goto done_file;

        ks_keystore_unserialize(buffer, cb_read, &ks);

    done_file:
        CloseHandle(hf);
    } else if (!wcsncmp(path, L"REG:", 4)) {
        if (khc_read_binary(csp, L"KeystoreData", NULL, &cb_buffer) != KHM_ERROR_TOO_LONG ||
            cb_buffer == 0)
            goto done_reg;

        buffer  = malloc(cb_buffer);
        if (buffer == NULL)
            goto done_reg;

        if (KHM_FAILED(khc_read_binary(csp, L"KeystoreData", buffer, &cb_buffer)))
            goto done_reg;

        ks_keystore_unserialize(buffer, cb_buffer, &ks);

    done_reg:
        ;
    } else {
        /* unknown location type */
        assert(FALSE);
    }

    if (ks) {
        khm_size i;

        ks_keystore_set_string(ks, KCDB_ATTR_LOCATION, path);

        /* Now see if we have opened the same keystore already.  If
           so, we return the previously opened keystore instead. */
        EnterCriticalSection(&cs_ks);
        for (i=0; i < n_keystores; i++) {
            RPC_STATUS st;

            if (!keystores[i])
                continue;
            KSLOCK(keystores[i]);
            if (UuidEqual(&keystores[i]->uuid, &ks->uuid, &st) &&
                CompareFileTime(&ks->ft_mtime, &keystores[i]->ft_mtime) == 0) {

                KSUNLOCK(keystores[i]);

                ks_keystore_release(ks);
                ks = keystores[i];
                ks_keystore_hold(ks);
                break;
            }
            KSUNLOCK(keystores[i]);
        }
        LeaveCriticalSection(&cs_ks);
    }

    if (buffer)
        free(buffer);

    return ks;
}


keystore_t *
create_keystore_for_identity(khm_handle identity)
{
    khm_size cb;
    khm_handle csp_id = NULL;
    khm_handle csp_ks = NULL;
    keystore_t * ks = NULL;

    if (KHM_FAILED(kcdb_identity_get_config(identity, 0, &csp_id)) ||
        KHM_FAILED(khc_open_space(csp_id, CREDPROV_NAMEW, 0, &csp_ks)))
        goto done;

    {
        wchar_t path[MAX_PATH];

        cb = sizeof(path);
        if (KHM_SUCCEEDED(khc_read_string(csp_ks, L"KeystorePath",
                                          path, &cb)))
            ks = create_keystore_from_location(path, csp_ks);
    }

    if (ks) {
        if (KHM_FAILED(associate_keystore_and_identity(ks, identity))) {
            ks_keystore_release(ks);
            ks = NULL;
        }
    }

    /* return ks held, if non-null */

 done:
    if (csp_id)
        khc_close_space(csp_id);
    if (csp_ks)
        khc_close_space(csp_ks);
    return ks;
}

/* Obtains cs_ks and ks->cs for all keystores in keystores list */
void
update_keystore_list(void)
{
    kcdb_enumeration e;
    khm_size i;
    khm_int32 rv_enum;
    int n_keystore_identities = 0;

    init_credtype();

    rv_enum = kcdb_identity_begin_enum(KCDB_IDENT_FLAG_CONFIG,
                                       KCDB_IDENT_FLAG_CONFIG,
                                       &e, NULL);

    EnterCriticalSection(&cs_ks);

    for (i=0; i < n_keystores; i++) {
        KSLOCK(keystores[i]);
        keystores[i]->flags &= ~KS_FLAG_SEEN;
        KSUNLOCK(keystores[i]);
    }

    if (KHM_SUCCEEDED(rv_enum)) {
        khm_handle identity = NULL;

        while (KHM_SUCCEEDED(kcdb_enum_next(e, &identity))) {
            khm_int32 idtype;
            khm_size cb;
            keystore_t * ks;

            cb = sizeof(idtype);
            if (KHM_FAILED(kcdb_identity_get_attr(identity, KCDB_ATTR_TYPE,
                                                  NULL, &idtype, &cb)) ||
                idtype != credtype_id)
                continue;

            n_keystore_identities++;

            ks = find_keystore_for_identity(identity);
            if (ks == NULL) {
                /* TODO: this should be handled more gracefully. */
                ks = create_keystore_for_identity(identity);
                assert(ks);
            }

            if (ks) {
                KSLOCK(ks);
                ks->flags |= KS_FLAG_SEEN;
                KSUNLOCK(ks);
                ks_keystore_release(ks);
            }
        }

        kcdb_enum_end(e);
    }

    for (i=0; i < n_keystores; i++) {
        KSLOCK(keystores[i]);
        /* We don't free keystores that have changes waiting to be
           saved.  Also, keystores that have just been created and not
           saved yet will also have the MODIFIED flag. */
        if (!(keystores[i]->flags & (KS_FLAG_SEEN |
                                     KS_FLAG_MODIFIED))) {
            keystore_t * ks;
            khm_size j;

            ks = keystores[i];

            for (j=i+1; j < n_keystores; j++)
                keystores[j-1] = keystores[j];
            n_keystores--;

            assert(ks->identity);

            hash_del(ht_identity_to_keystore, ks->identity);
            kcdb_identity_release(ks->identity);
            ks->identity = NULL;
            KSUNLOCK(ks);
            ks_keystore_release(ks);
            i--;
            continue;
        }
        KSUNLOCK(keystores[i]);
    }

    if (n_keystores == 0 && n_keystore_identities == 0) {
        khm_handle def_ks = NULL;

        LeaveCriticalSection(&cs_ks);
        def_ks = create_default_keystore();
        EnterCriticalSection(&cs_ks);

        if (def_ks)
            kcdb_identity_release(def_ks);
    }

    LeaveCriticalSection(&cs_ks);
}

khm_int32
get_provider_key_type(const wchar_t * provider_name)
{
    khm_size i;
    khm_int32 ctype = KCDB_CREDTYPE_INVALID;

    EnterCriticalSection(&cs_ks);
    for (i=0; i < n_key_type_map; i++) {
        if (!wcscmp(provider_name, key_type_map[i].provider_name))
            break;
    }

    if (i == n_key_type_map) {
        kcdb_credtype ct;
        wchar_t ctypename[KCDB_MAXCCH_NAME];
        wchar_t short_desc[KCDB_MAXCCH_SHORT_DESC];
        khm_size cb;
        khm_handle provider = NULL;

        if (n_key_type_map == nc_key_type_map) {
            nc_key_type_map = UBOUNDSS(n_key_type_map, 4, 4);
            key_type_map = PREALLOC(key_type_map, sizeof(key_type_map[0]) * nc_key_type_map); 
        }

        ZeroMemory(&key_type_map[i], sizeof(key_type_map[i]));

        key_type_map[i].provider_name = _wcsdup(provider_name);

        ZeroMemory(&ct, sizeof(ct));

        ct.id = KCDB_CREDTYPE_AUTO;
        ct.name = ctypename;
        ct.short_desc = short_desc;
        ct.long_desc = NULL;

        kmq_create_subscription(idk_credprov_msg_proc, &ct.sub);
        ct.is_equal = idk_cred_is_equal;

        StringCbPrintf(ctypename, sizeof(ctypename), L"KeyStoreKey_%s", provider_name);

        cb = sizeof(short_desc);
        if (KHM_FAILED(kcdb_identpro_find(provider_name, &provider)) ||
            KHM_FAILED(kcdb_get_resource(provider, KCDB_RES_INSTANCE, KCDB_RFS_SHORT, NULL,
                                         NULL, short_desc, &cb))) {
            StringCbCopy(short_desc, sizeof(short_desc), provider_name);
        }

        kcdb_credtype_register(&ct, &key_type_map[i].ctype);

        if (provider)
            kcdb_identpro_release(provider);
        n_key_type_map++;
    }

    ctype = key_type_map[i].ctype;
    LeaveCriticalSection(&cs_ks);

    return ctype;
}

khm_handle
get_identkey_credential(keystore_t * ks, identkey_t * idk)
{
    khm_handle credential = NULL;

    assert(is_identkey_t(idk));
    assert(is_keystore_t(ks));

    KSLOCK(ks);
    if (ks->identity == NULL)
        goto done;

    assert(idk->display_name);
    assert(idk->provider_name);
    assert(idk->identity_name);

    if (KHM_FAILED(kcdb_cred_create(idk->identity_name, ks->identity,
                                    get_provider_key_type(idk->provider_name),
                                    &credential)))
        goto done;

    if (ks->location)
        kcdb_cred_set_attr(credential, KCDB_ATTR_LOCATION, ks->location, KCDB_CBSIZE_AUTO);
    kcdb_cred_set_attr(credential, KCDB_ATTR_DISPLAY_NAME, idk->display_name, KCDB_CBSIZE_AUTO);
    kcdb_cred_set_attr(credential, KCDB_ATTR_ISSUE, &idk->ft_ctime, KCDB_CBSIZE_AUTO);
    if (FtToInt(&idk->ft_expire) != 0)
        kcdb_cred_set_attr(credential, KCDB_ATTR_EXPIRE, &idk->ft_expire, KCDB_CBSIZE_AUTO);

 done:
    KSUNLOCK(ks);

    return credential;
}

khm_handle
get_keystore_credential(keystore_t * ks)
{
    khm_handle credential = NULL;
    khm_handle identity = NULL;
    khm_size cb;

    if (ks == NULL)
        return NULL;

    KSLOCK(ks);
    if (ks_keystore_has_key(ks)) {
        identity = ks->identity;
        if (identity)
            kcdb_identity_hold(identity);
    }
    KSUNLOCK(ks);

    if (identity == NULL)
        return NULL;

    {
        wchar_t idname[KCDB_MAXCCH_NAME];

        cb = sizeof(idname);
        if (KHM_FAILED(kcdb_identity_get_name(identity, idname, &cb)))
            goto done;

        if (KHM_FAILED(kcdb_cred_create(idname, identity, credtype_id, &credential)))
            goto done;
    }

    {
        wchar_t display_name[KCDB_MAXCCH_SHORT_DESC];
        cb = sizeof(display_name);
        if (SUCCEEDED(kcdb_get_resource(identity, KCDB_RES_DISPLAYNAME,
                                        0, NULL, NULL, display_name, &cb))) {
            kcdb_cred_set_attr(credential, KCDB_ATTR_DISPLAY_NAME,
                               display_name, cb);
        }
    }

    KSLOCK(ks);
    kcdb_cred_set_attr(credential, KCDB_ATTR_ISSUE, &ks->ft_key_ctime, KCDB_CBSIZE_AUTO);
    if (FtToInt(&ks->ft_key_expire) != 0)
        kcdb_cred_set_attr(credential, KCDB_ATTR_EXPIRE, &ks->ft_key_expire, KCDB_CBSIZE_AUTO);
    if (ks->location)
        kcdb_cred_set_attr(credential, KCDB_ATTR_LOCATION, ks->location, KCDB_CBSIZE_AUTO);
    KSUNLOCK(ks);

 done:
    if (identity)
        kcdb_identity_release(identity);

    return credential;
}

khm_handle
get_keystore_credential_for_identity(khm_handle identity)
{
    keystore_t * ks;
    khm_handle credential = NULL;

    ks = find_keystore_for_identity(identity);
    credential = get_keystore_credential(ks);
    if (ks)
        ks_keystore_release(ks);

    return credential;
}

#define MAX_KS_LIST 16

/* The list that is returned must be freed using free_keystores_list */
khm_size
get_keystores_with_identkey(khm_handle s_identity, keystore_t *** pks)
{
    keystore_t *aks[MAX_KS_LIST];
    khm_size n_ks = 0;
    khm_size i, j;

    EnterCriticalSection(&cs_ks);
    for (i=0; i < n_keystores && n_ks < MAX_KS_LIST; i++) {
        keystore_t * ks = keystores[i];
        khm_boolean found = FALSE;

        KSLOCK(ks);
        for (j=0; j < ks->n_keys && !found; j++) {
            identkey_t * idk = ks->keys[j];
            khm_handle identpro = NULL;
            khm_handle identity = NULL;

            if (KHM_FAILED(kcdb_identpro_find(idk->provider_name, &identpro)) ||
                KHM_FAILED(kcdb_identity_create_ex(identpro, idk->identity_name,
                                                   KCDB_IDENT_FLAG_CREATE, NULL, &identity)))
                goto done_with_idk;

            if (kcdb_identity_is_equal(identity, s_identity)) {
                aks[n_ks++] = ks;
                ks_keystore_hold(ks);
                found = TRUE;
            }

        done_with_idk:
            if (identpro) kcdb_identpro_release(identpro);
            if (identity) kcdb_identity_release(identity);
        }
        KSUNLOCK(ks);
    }
    LeaveCriticalSection(&cs_ks);

    if (n_ks > 0) {
        *pks = malloc(sizeof((*pks)[0]) * n_ks);
        assert(*pks);
        for (i=0; i < n_ks; i++)
            (*pks)[i] = aks[i];
    } else {
        *pks = NULL;
    }

    return n_ks;
}

void
free_keystores_list(keystore_t ** aks, khm_size n_ks)
{
    khm_size i;

    for (i=0; i < n_ks; i++) {
        ks_keystore_release(aks[i]);
    }

    if (aks)
        free(aks);
}

khm_int32
save_keystore_with_identity(keystore_t * ks)
{
    khm_handle identity;
    khm_size cb;
    khm_handle csp_id = NULL;
    khm_handle csp_ks = NULL;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    assert(is_keystore_t(ks));
    KSLOCK(ks);
    identity = ks->identity;
    kcdb_identity_hold(identity);
    KSUNLOCK(ks);

    if (identity == NULL)
        return KHM_ERROR_NOT_FOUND;

    if (KHM_FAILED(rv = kcdb_identity_get_config(identity, KHM_FLAG_CREATE, &csp_id)) ||
        KHM_FAILED(rv = khc_open_space(csp_id, CREDPROV_NAMEW, KHM_FLAG_CREATE, &csp_ks)))
        goto done;

    {
        wchar_t path[MAX_PATH];

        cb = sizeof(path);
        if (KHM_SUCCEEDED(khc_read_string(csp_ks, L"KeystorePath",
                                          path, &cb)))
            write_keystore_to_location(ks, path, csp_ks);
        else if ((cb = sizeof(path)) != 0 &&
                 KHM_SUCCEEDED(kcdb_identity_get_attr(identity, KCDB_ATTR_LOCATION,
                                                      NULL, path, &cb))) {
            khc_write_string(csp_ks, L"KeystorePath", path);
            write_keystore_to_location(ks, path, csp_ks);
        } else {
            rv = KHM_ERROR_INVALID_PARAM;
            goto done;
        }
    }

 done:
    if (csp_id)
        khc_close_space(csp_id);
    if (csp_ks)
        khc_close_space(csp_ks);
    if (identity)
        kcdb_identity_release(identity);
    return rv;
}

khm_int32
destroy_keystore_identity(keystore_t * ks)
{
    khm_handle csp_id = NULL;
    khm_handle identity;
    khm_int32 rv = KHM_ERROR_SUCCESS;
    khm_handle csp_t = NULL;

    assert(is_keystore_t(ks));
    KSLOCK(ks);
    identity = ks->identity;
    kcdb_identity_hold(identity);
    KSUNLOCK(ks);

    if (identity == NULL)
        return KHM_ERROR_NOT_FOUND;

    if (KHM_FAILED(rv = kcdb_identity_get_config(identity, KHM_PERM_WRITE, &csp_id)))
        goto done;

    rv = khc_remove_space(csp_id);

    kcdb_identity_get_config(identity, KHM_PERM_READ, &csp_t);
    assert(csp_t == NULL);

 done:
    if (csp_id)
        khc_close_space(csp_id);
    if (identity)
        kcdb_identity_release(identity);
    return rv;
}

#define IFOK(X) (KHM_FAILED(rv) || (rv = (X)))

khm_int32
create_empty_memory_store_for_identity(khm_handle identity, khm_handle * ret_cfg)
{
    khm_handle cfg = NULL;
    wchar_t shortname[KCDB_IDENT_MAXCCH_NAME];
    khm_size cb = sizeof(shortname);
    khm_int32 rv = KHM_ERROR_SUCCESS;

    IFOK(kcdb_identity_get_short_name(identity, TRUE, shortname, &cb));

    IFOK(khc_memory_store_create(&cfg));

    IFOK(khc_memory_store_add(cfg, shortname, KC_SPACE, NULL, 0));

    IFOK(khc_memory_store_add(cfg, NULL, KC_ENDSPACE, NULL, 0));

    if (KHM_FAILED(rv) && cfg) {
        khc_memory_store_release(cfg);
        cfg = NULL;
    }

    *ret_cfg = cfg;

    return rv;
}

#if 0
/* TODO: Continue this

   This part of the code is necessary for permanently mounting
   keystore configuration onto an identity.  This way we can monitor
   and periodically save the configuration space as time permits.
 */
typedef struct idk_memstore_data {
    keystore_t * ks;
    identkey_t * idk;
} idk_memstore_data;

static void KHMCALLBACK handle_idk_init(void * ctx, khm_handle s)
{
}

static void KHMCALLBACK handle_idk_exit(void * ctx, khm_handle s)
{
}

static void KHMCALLBACK handle_idk_open(void * ctx, khm_handle s)
{
}

static void KHMCALLBACK handle_idk_close(void * ctx, khm_handle s)
{
}

static void KHMCALLBACK handle_idk_modify(void * ctx, khm_handle s)
{
}

khm_int32
prep_memory_store_for_identkey(identkey_t * idk)
{

}
#endif

khm_int32
mount_identkey_configuration(keystore_t * ks, khm_size idx)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;
    identkey_t * idk;

    assert(is_keystore_t(ks));

    KSLOCK(ks);

    __try {
        if (idx >= ks->n_keys)
            return KHM_ERROR_INVALID_PARAM;

        idk = ks->keys[idx];

        if (idk->flags & IDENTKEY_FLAG_CFGMOUNT)
            return KHM_ERROR_SUCCESS;

        if (idk->cfg_store == NULL &&
            idk->configuration.cb_data != 0) {

            rv = ks_unserialize_configuration(idk->configuration.data,
                                              idk->configuration.cb_data,
                                              &idk->cfg_store);

            if (KHM_FAILED(rv))
                return rv;
        }

        {
            khm_handle identpro = NULL;
            khm_handle identity = NULL;
            khm_handle csp_identity = NULL;
            khm_handle csp_idparent = NULL;

            IFOK(kcdb_identpro_find(idk->provider_name, &identpro));

            IFOK(kcdb_identity_create_ex(identpro, idk->identity_name,
                                         KCDB_IDENT_FLAG_CREATE, NULL, &identity));

            IFOK(khc_open_space(NULL, L"KCDB\\Identity",
                                KCONF_FLAG_USER|KCONF_FLAG_SCHEMA, &csp_idparent));

            (idk->cfg_store != NULL ||

             IFOK(create_empty_memory_store_for_identity(identity,
                                                         &idk->cfg_store)));

#if 0
            IFOK(prep_memory_store_for_identkey(idk));
#endif

            IFOK(khc_memory_store_mount(csp_idparent, KCONF_FLAG_USER|KCONF_FLAG_RECURSIVE,
                                        idk->cfg_store, NULL));

            IFOK(kcdb_identity_get_config(identity, KCONF_FLAG_USER, &csp_identity));

            assert(KHM_SUCCEEDED(rv));

            if (identpro) kcdb_identpro_release(identpro);
            if (identity) kcdb_identity_release(identity);
            if (csp_identity) khc_close_space(csp_identity);
            if (csp_idparent) khc_close_space(csp_idparent);

            if (KHM_SUCCEEDED(rv))
                idk->flags |= IDENTKEY_FLAG_CFGMOUNT;
        }
    } __finally {

        KSUNLOCK(ks);

    }

    return rv;
}

khm_int32
unmount_identkey_configuration(keystore_t * ks, khm_size idx)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;
    identkey_t * idk;

    assert(is_keystore_t(ks));

    KSLOCK(ks);

    __try {
        if (idx >= ks->n_keys)
            return KHM_ERROR_INVALID_PARAM;

        idk = ks->keys[idx];

        if (!(idk->flags & IDENTKEY_FLAG_CFGMOUNT))
            return KHM_ERROR_SUCCESS;

        if (idk->cfg_store == NULL) {
            assert(FALSE);
            return KHM_ERROR_INVALID_PARAM;
        }

        rv = khc_memory_store_unmount(idk->cfg_store);

        if (KHM_SUCCEEDED(rv))
            idk->flags &= ~IDENTKEY_FLAG_CFGMOUNT;

        {
            khm_size cb = 0;

            rv = ks_serialize_configuration(idk->cfg_store, NULL, &cb);
            if (rv == KHM_ERROR_TOO_LONG) {
                ks_datablob_alloc(&idk->configuration, cb);
                idk->configuration.cb_data = cb;

                rv = ks_serialize_configuration(idk->cfg_store,
                                                idk->configuration.data,
                                                &idk->configuration.cb_data);
                ks->flags |= KS_FLAG_MODIFIED;
            }
        }

    } __finally {

        KSUNLOCK(ks);
    }

    return rv;
}
