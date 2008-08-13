/*
 * Copyright (c) 2006 Secure Endpoints Inc.
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

#define HT_IDENTITY_TO_KEYSTORE_SIZE 11
hashtable * ht_identity_to_keystore = NULL;

/* Functions for handling our credentials type.
*/

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

static khm_int32 hash_ptr(const void * key)
{
    return (khm_int32) key;
}

static khm_int32 comp_ptr(const void *k1, const void *k2)
{
    return ((khm_int32) k1) - ((khm_int32) k2);
}

static void add_ref_ks(const void *k, void * d)
{
    ks_keystore_hold((keystore_t *) d);
}

static void del_ref_ks(const void *k, void * d)
{
    ks_keystore_release((keystore_t *) d);
}

void
init_credtype(void)
{
    assert(ht_identity_to_keystore == NULL);
    ht_identity_to_keystore = hash_new_hashtable(HT_IDENTITY_TO_KEYSTORE_SIZE,
                                                 hash_ptr, comp_ptr,
                                                 add_ref_ks, del_ref_ks);
}

keystore_t *
find_keystore_for_identity(khm_handle identity)
{
    keystore_t *ks = NULL;

    EnterCriticalSection(&cs_ks);
    ks = (keystore_t *) hash_lookup(ht_identity_to_keystore, identity);
    if (ks)
        ks_keystore_hold(ks);
    LeaveCriticalSection(&cs_ks);

    return ks;
}

/* Obtains cs_ks and ks->cs */
void
associate_keystore_and_identity(keystore_t * ks, khm_handle identity)
{
    assert(is_keystore_t(ks));

    EnterCriticalSection(&cs_ks);
    KSLOCK(ks);

    if (ks->identity != NULL) {
        assert(kcdb_identity_is_equal(ks->identity, identity));
        assert((keystore_t *) hash_lookup(ht_identity_to_keystore, identity) == ks);
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

        hf = CreateFile(path + 5, GENERIC_WRITE,
                        FILE_SHARE_DELETE,
                        NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_ARCHIVE, NULL);

        if (hf == NULL)
            return;

        ks_keystore_serialize(ks, NULL, &cb_buffer);

        if (cb_buffer) {
            buffer = malloc(cb_buffer);
            ks_keystore_serialize(ks, buffer, &cb_buffer);
            WriteFile(hf, buffer, cb_buffer, &numwritten, NULL);
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

        hf = CreateFile(path + 5, GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING,
                        FILE_FLAG_SEQUENTIAL_SCAN, NULL);

        if (hf == NULL)
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

        if (!ReadFile(hf, buffer, cb_buffer, &cb_read, NULL))
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

    if (buffer)
        free(buffer);

    return ks;
}


keystore_t *
create_keystore_for_identity(khm_handle identity)
{
    khm_size cb;
    khm_int32 ctype;
    khm_handle csp_id = NULL;
    khm_handle csp_ks = NULL;
    keystore_t * ks = NULL;

    cb = sizeof(ctype);
    if (KHM_FAILED(kcdb_identity_get_attr(identity, KCDB_ATTR_TYPE,
                                          NULL, &ctype, &cb)) ||
        ctype != credtype_id)
        return NULL;

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
        associate_keystore_and_identity(ks, identity);
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

            ks = find_keystore_for_identity(identity);
            if (ks == NULL) {
                ks = create_keystore_for_identity(identity);
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

    LeaveCriticalSection(&cs_ks);
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
    identity = ks->identity;
    if (identity)
        kcdb_identity_hold(identity);
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
    kcdb_cred_set_attr(credential, KCDB_ATTR_ISSUE, &ks->ft_ctime, KCDB_CBSIZE_AUTO);
    kcdb_cred_set_attr(credential, KCDB_ATTR_EXPIRE, &ks->ft_expire, KCDB_CBSIZE_AUTO);
    KSUNLOCK(ks);

 done:
    if (identity)
        kcdb_identity_release(identity);

    return credential;
}

khm_handle
get_keystore_credential_for_identity(khm_handle identity)
{
    /* 1. find_keystore_for_identity()
       2. Extract the display name etc from the keystore
       3. Write out the fields to the credential
    */

    keystore_t * ks;
    khm_handle credential = NULL;

    ks = find_keystore_for_identity(identity);
    credential = get_keystore_credential(ks);
    if (ks)
        ks_keystore_release(ks);

    return credential;
}

khm_int32
save_keystore_with_identity(keystore_t * ks)
{
    khm_handle identity;
    khm_size cb;
    khm_int32 ctype;
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
            write_keystore_to_location(ks, path, csp_ks);
            khc_write_string(csp_ks, L"KeystorePath", path);
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

/* 

   When initially creating the identity for a keystore, we are going
   to use a UUID to identify the identity.  (as-in, the identity name
   would be a string representation of a UUID.

   Once we have a display name and a description for the keystore, we
   can update the identity information.  Until then, it would be
   "Unnamed Keystore #hhhhhhhh", where "#hhhhhhhh" are the last
   characters of the UUID.

   The keystore_t object should have a back pointer that identifies
   the identity that is associated with it.  This way, we can remove
   the objects from the hash table when they are being deallocated.

   Need to implement:

   create_identity_from_keystore() that will create the identity
   object which corresponds to the keystore.

   save_keystore() that will save the keystore to the registry
   associated with the identity or the file if the identity specifies
   a keystore location.

 */
