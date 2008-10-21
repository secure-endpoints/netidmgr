/*
 * Copyright (c) 2008 Secure Endpoints Inc.
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

#include "module.h"
#include <assert.h>

static void
destroy_encryption_key(keystore_t * ks);

void
ks_datablob_init(datablob_t * pd)
{
    memset(pd, 0, sizeof(*pd));
    pd->data = NULL;
}

void
ks_datablob_free(datablob_t * pd)
{
    if (pd->data) {
        SecureZeroMemory(pd->data, pd->cb_data);
        free(pd->data);
        pd->data = NULL;
    }
    pd->cb_data = 0;
    pd->cb_alloc = 0;
}

datablob_t *
ks_datablob_copy(datablob_t *dest, const void * data, khm_size cb_data, khm_size align)
{
    ks_datablob_free(dest);
    if (align != 0)
        dest->cb_alloc = UBOUNDSS(cb_data, align, align);
    else
        dest->cb_alloc = cb_data;

    dest->data = malloc(cb_data);
    assert(dest->data != NULL);

    if (dest->data == NULL) {
        ks_datablob_free(dest);
        return NULL;
    }

    dest->cb_data = cb_data;
    memcpy(dest->data, data, cb_data);

    if (dest->cb_alloc > dest->cb_data)
        memset(BYTEOFFSET(dest->data, dest->cb_data), 0, dest->cb_alloc - dest->cb_data);

    return dest;
}

datablob_t *
ks_datablob_dup(datablob_t * dest, const datablob_t * src)
{
    ks_datablob_free(dest);
    if (datablob_is_empty(src))
        return dest;

    dest->cb_data = src->cb_data;
    dest->cb_alloc = src->cb_alloc;
    dest->data = malloc(src->cb_alloc);
    assert(dest->data);

    if (dest->data == NULL) {
        ks_datablob_free(dest);
        return NULL;
    }

    memcpy(dest->data, src->data, src->cb_data);
    if (dest->cb_alloc > dest->cb_data)
        memset(BYTEOFFSET(dest->data, dest->cb_data), 0, dest->cb_alloc - dest->cb_data);

    return dest;
}

datablob_t *
ks_datablob_alloc(datablob_t * dest, khm_size cb)
{
    ks_datablob_free(dest);

    dest->data = malloc(cb);
    if (dest->data != NULL) {
        dest->cb_alloc = cb;
    }

    return dest;
}

identkey_t *
ks_identkey_create_new(void)
{
    identkey_t * idk;

    idk = malloc(sizeof(*idk));
    assert(idk != NULL);
    memset(idk, 0, sizeof(*idk));

    idk->magic = IDENTKEY_MAGIC;
    idk->provider_name = NULL;
    idk->identity_name = NULL;
    idk->display_name = NULL;
    idk->key_description = NULL;

    ks_datablob_init(&idk->key_hash);
    ks_datablob_init(&idk->key);
    ks_datablob_init(&idk->plain_key);

    idk->flags = 0;

    return idk;
}

khm_int32
ks_identkey_free(identkey_t * idk)
{
    assert(is_identkey_t(idk));
    if (!is_identkey_t(idk))
        return KHM_ERROR_INVALID_PARAM;

    if (idk->provider_name)
        free(idk->provider_name);

    if (idk->identity_name)
        free(idk->identity_name);

    if (idk->display_name)
        free(idk->display_name);

    if (idk->key_description)
        free(idk->key_description);

    ks_datablob_free(&idk->key);
    ks_datablob_free(&idk->key_hash);
    ks_datablob_free(&idk->plain_key);

    memset(idk, 0, sizeof(*idk));
    free(idk);

    return KHM_ERROR_SUCCESS;
}

keystore_t *
ks_keystore_create_new(void)
{
    keystore_t * ks;

    ks = malloc(sizeof(*ks));
    memset(ks, 0, sizeof(*ks));

    ks->magic = KEYSTORE_MAGIC;
    ks->display_name = NULL;
    ks->description = NULL;
    ks->location = NULL;
    ks->keys = NULL;
    ks->DBenc_key.pbData = NULL;
    UuidCreate(&ks->uuid);

    GetSystemTimeAsFileTime(&ks->ft_ctime);
    ks->ft_mtime = ks->ft_ctime;
    ks->ft_key_lifetime = IntToFt(KS_DEFAULT_KEY_LIFETIME);

    InitializeCriticalSection(&ks->cs);
    ks->refcount = 1;           /* initially held */

    return ks;
}

void
ks_keystore_free(keystore_t * ks)
{
    khm_size i;

    assert(is_keystore_t(ks));

    KSLOCK(ks);
    if (!is_keystore_t(ks))
        return;

    if (ks->display_name)
        free(ks->display_name);

    if (ks->description)
        free(ks->description);

    if (ks->location)
        free(ks->location);

    destroy_encryption_key(ks);

    for (i=0; i < ks->n_keys; i++)
        if (ks->keys[i] != NULL) {
            ks_identkey_free(ks->keys[i]);
            ks->keys[i] = NULL;
        }

    if (ks->keys)
        free(ks->keys);

    if (ks->identity)
        kcdb_identity_release(ks->identity);

    KSUNLOCK(ks);
    DeleteCriticalSection(&ks->cs);

    memset(ks, 0, sizeof(*ks));
    free(ks);
}

void
ks_keystore_hold(keystore_t * ks)
{
    InterlockedIncrement(&ks->refcount);
}

void
ks_keystore_release(keystore_t * ks)
{
    if (InterlockedDecrement(&ks->refcount) == 0)
        ks_keystore_free(ks);
}

khm_int32
ks_keystore_add_identkey(keystore_t * ks, identkey_t * idk)
{
    khm_size i;

    assert(is_keystore_t(ks));
    assert(is_identkey_t(idk));

    if (idk->provider_name == NULL ||
        idk->identity_name == NULL ||
        idk->display_name == NULL) {
        assert(FALSE);
        return KHM_ERROR_INVALID_PARAM;
    }

    KSLOCK(ks);

    if (idk->version == 0)
        idk->version = 1;

    for (i=0; i < ks->n_keys; i++) {
        if (!wcscmp(idk->provider_name, ks->keys[i]->provider_name) &&
            !wcscmp(idk->identity_name, ks->keys[i]->identity_name)) {
            /* we don't allow a locked key to be replaced by an
               unlocked key or vice versa. */
            assert(!((ks->keys[i]->flags ^ idk->flags) & IDENTKEY_FLAG_LOCKED));
            idk->version = ks->keys[i]->version + 1;
            ks_identkey_free(ks->keys[i]);
            ks->keys[i] = NULL;
            break;
        }
    }

    if (i == ks->nc_keys) {
        /* need to allocate more */
        ks->nc_keys = UBOUNDSS(ks->n_keys + 1, KS_KEY_ALLOC_INCR,
                               KS_KEY_ALLOC_INCR);
        ks->keys = realloc(ks->keys, sizeof(ks->keys[0]) * ks->nc_keys);
    }

    ks->keys[i] = idk;
    ks->flags |= KS_FLAG_MODIFIED;
    GetSystemTimeAsFileTime(&ks->ft_mtime);

    if (i == ks->n_keys)
        ks->n_keys++;

    KSUNLOCK(ks);

    return KHM_ERROR_SUCCESS;
}

khm_int32
ks_keystore_remove_identkey(keystore_t * ks, khm_size idx)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;

    assert(is_keystore_t(ks));

    KSLOCK(ks);

    if (idx < ks->n_keys) {
        ks_identkey_free(ks->keys[idx]);
        ks->n_keys--;
        while (idx < ks->n_keys) {
            ks->keys[idx] = ks->keys[idx + 1];
            idx++;
        }
    } else {
        rv = KHM_ERROR_NOT_FOUND;
    }
    ks->flags |= KS_FLAG_MODIFIED;
    GetSystemTimeAsFileTime(&ks->ft_mtime);

    KSUNLOCK(ks);

    return rv;
}

khm_int32
ks_keystore_set_flags(keystore_t * ks, khm_int32 mask, khm_int32 flags)
{
    assert(is_keystore_t(ks));

    KSLOCK(ks);
    ks->flags &= ~mask;
    ks->flags |= (flags & mask);
    KSUNLOCK(ks);
    return KHM_ERROR_SUCCESS;
}

khm_int32
ks_keystore_get_flags(keystore_t * ks)
{
    khm_int32 f;

    assert(is_keystore_t(ks));

    KSLOCK(ks);
    f = ks->flags;
    KSUNLOCK(ks);
    return f;
}


khm_int32
ks_keystore_set_string(keystore_t * ks, kcdb_resource_id r_id, const wchar_t * buffer)
{
    khm_size cch = 0;
    wchar_t ** dest = NULL;
    khm_size maxcch = 0;
    khm_boolean set_modified = FALSE;

    assert(is_keystore_t(ks));
    if (!is_keystore_t(ks))
        return KHM_ERROR_INVALID_PARAM;

    KSLOCK(ks);
    switch(r_id) {
    case KCDB_RES_DISPLAYNAME:
        dest = &ks->display_name;
        maxcch = KCDB_MAXCCH_NAME;
        set_modified = TRUE;
        break;

    case KCDB_RES_DESCRIPTION:
        dest = &ks->description;
        maxcch = KCDB_MAXCCH_SHORT_DESC;
        set_modified = TRUE;
        break;

    case KCDB_ATTR_LOCATION:
        dest = &ks->location;
        maxcch = MAX_PATH;
        set_modified = FALSE;
        break;

    default:
        KSUNLOCK(ks);
        return KHM_ERROR_INVALID_PARAM;
    }

    if ((*dest == NULL && buffer == NULL || buffer[0] == L'\0') ||
        (*dest != NULL && buffer != NULL && !wcscmp(*dest, buffer)) ||
        FAILED(StringCchLength(buffer, maxcch, &cch))) {
        goto done;
    }

    if (*dest) {
        free(*dest);
        *dest = NULL;
    }
    if (buffer && buffer[0])
        *dest = _wcsdup(buffer);
    if (set_modified) {
        ks->flags |= KS_FLAG_MODIFIED;
        GetSystemTimeAsFileTime(&ks->ft_mtime);
    }

 done:

    KSUNLOCK(ks);
    return KHM_ERROR_SUCCESS;
}

khm_int32
ks_keystore_get_string(keystore_t * ks, kcdb_resource_id r_id,
              wchar_t * buffer, khm_size *pcb_buffer)
{
    const wchar_t * src = NULL;
    size_t maxlen;

    assert(is_keystore_t(ks));

    KSLOCK(ks);
    if (!is_keystore_t(ks))
        return KHM_ERROR_INVALID_PARAM;

    switch (r_id) {
    case KCDB_RES_DISPLAYNAME:
        src = ks->display_name;
        maxlen = KCDB_MAXCCH_SHORT_DESC;
        break;

    case KCDB_RES_DESCRIPTION:
        src = ks->description;
        maxlen = KCDB_MAXCCH_LONG_DESC;
        break;

    case KCDB_ATTR_LOCATION:
        src = ks->location;
        maxlen = MAX_PATH;
        break;

    }

    if (src == NULL) {
        KSUNLOCK(ks);
        return KHM_ERROR_NOT_FOUND;
    }

    if (FAILED(StringCbCopy(buffer, *pcb_buffer, src))) {
        if (FAILED(StringCbLength(src, maxlen, pcb_buffer))) {
            KSUNLOCK(ks);
            return KHM_ERROR_INVALID_PARAM;
        }
        *pcb_buffer += sizeof(wchar_t);
        KSUNLOCK(ks);
        return KHM_ERROR_TOO_LONG;
    }

    StringCbLength(buffer, maxlen, pcb_buffer);
    *pcb_buffer += sizeof(wchar_t);
    KSUNLOCK(ks);

    return KHM_ERROR_SUCCESS;
}

typedef struct crypt_op {
    HCRYPTPROV hProv;
    HCRYPTKEY  hKey;
} crypt_op;

#define CCall(f) if (!f) goto done;

static khm_boolean
lock_encryption_key(keystore_t * ks, const void * data, size_t cb_data) {
    DATA_BLOB db_in;
    khm_boolean rv;

    assert(is_keystore_t(ks));

    if (cb_data == 0)
        return FALSE;

    db_in.pbData = (void *) data;
    db_in.cbData = cb_data;

    rv = CryptProtectData(&db_in, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN,
                          &ks->DBenc_key);
    if (rv) {
        GetSystemTimeAsFileTime(&ks->ft_key_ctime);
        ks->ft_key_expire = FtAdd(&ks->ft_key_ctime, &ks->ft_key_lifetime);
    }

    return rv;
}

static khm_boolean
unlock_encryption_key(keystore_t * ks, datablob_t * db_out) {
    DATA_BLOB db_tmp;
    khm_boolean rv;

    rv = CryptUnprotectData(&ks->DBenc_key, NULL, NULL, NULL, NULL,
                            CRYPTPROTECT_UI_FORBIDDEN, &db_tmp);
    if (rv) {
        ks_datablob_copy(db_out, db_tmp.pbData, db_tmp.cbData, 0);
        SecureZeroMemory(db_tmp.pbData, db_tmp.cbData);
        LocalFree(db_tmp.pbData);
        db_tmp.pbData = NULL;
    }

    return rv;
}

static void
destroy_encryption_key(keystore_t * ks) {
    if (ks->DBenc_key.pbData) {
        SecureZeroMemory(ks->DBenc_key.pbData, ks->DBenc_key.cbData);
        LocalFree(ks->DBenc_key.pbData);
        ks->DBenc_key.pbData = NULL;
        ks->DBenc_key.cbData = 0;
        ks->ft_key_ctime = IntToFt(0);
        ks->ft_key_expire = IntToFt(0);
    }
}

static khm_boolean
has_encryption_key(keystore_t * ks) {
    FILETIME ftc;

    if (ks->DBenc_key.pbData == NULL)
        return FALSE;

    GetSystemTimeAsFileTime(&ftc);
    if (CompareFileTime(&ftc, &ks->ft_key_expire) > 0) {
        destroy_encryption_key(ks);
        return FALSE;
    }

    return TRUE;
}


static khm_boolean
begin_crypt_op(const keystore_t * ks, crypt_op * op)
{
    memset(op, 0, sizeof(*op));

    return CryptAcquireContext(&op->hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL,
                               CRYPT_VERIFYCONTEXT | CRYPT_SILENT);
}

static void
end_crypt_op(crypt_op * op)
{
    if (op->hKey)
        CryptDestroyKey(op->hKey);

    if (op->hProv)
        CryptReleaseContext(op->hProv, 0);

    memset(op, 0, sizeof(*op));
}

static khm_boolean
derive_new_key_from_data(crypt_op * op, const void * data, khm_size cb_key)
{
    HCRYPTHASH hHash = 0;
    khm_boolean rv = FALSE;

    if (op->hKey != 0) {
        CryptDestroyKey(op->hKey);
        op->hKey = 0;
    }

    CCall(CryptCreateHash(op->hProv, CALG_SHA1, 0, 0, &hHash));
    CCall(CryptHashData(hHash, data, cb_key, 0));
    CCall(CryptDeriveKey(op->hProv, CALG_RC4, hHash, CRYPT_NO_SALT, &op->hKey));

    rv = TRUE;

 done:
    if (hHash)
        CryptDestroyHash(hHash);

    return rv;
}

static khm_boolean
encrypt_data(crypt_op * op, const datablob_t * plain,
             datablob_t * encrypted,
             datablob_t * hash)
{
    khm_boolean rv = FALSE;
    HCRYPTHASH hEncHash = 0;
    DWORD cbData;
    DWORD cbHash;

    CCall(CryptCreateHash(op->hProv, CALG_SHA1, 0, 0, &hEncHash));

    cbData = plain->cb_data;
    CCall(CryptEncrypt(op->hKey, hEncHash, TRUE, 0, NULL, &cbData, 0));

    ks_datablob_alloc(encrypted, max(cbData, plain->cb_data));
    memcpy(encrypted->data, plain->data, plain->cb_data);
    cbData = plain->cb_data;
    CCall(CryptEncrypt(op->hKey, hEncHash, TRUE, 0, encrypted->data, &cbData,
                       encrypted->cb_alloc));
    encrypted->cb_data = cbData;

    cbData = sizeof(cbHash);
    CCall(CryptGetHashParam(hEncHash, HP_HASHSIZE,
                            (BYTE *) &cbHash, &cbData, 0));

    ks_datablob_alloc(hash, cbHash);
    CCall(CryptGetHashParam(hEncHash, HP_HASHVAL,
                            hash->data, &cbHash, 0));
    hash->cb_data = cbHash;

    rv = TRUE;

 done:
    if (hEncHash)
        CryptDestroyHash(hEncHash);

    return rv;
}

static khm_boolean
decrypt_data(crypt_op * op,
             const datablob_t * encrypted,
             const datablob_t * hash,
             datablob_t * plain)
{
    khm_boolean rv = FALSE;
    HCRYPTHASH hDecHash = 0;
    DWORD cbData;
    DWORD cbHash;
    datablob_t new_hash;

    ks_datablob_init(&new_hash);

    CCall(CryptCreateHash(op->hProv, CALG_SHA1, 0, 0, &hDecHash));

    ks_datablob_alloc(plain, encrypted->cb_data);
    memcpy(plain->data, encrypted->data, encrypted->cb_data);
    cbData = encrypted->cb_data;
    CCall(CryptDecrypt(op->hKey, hDecHash, TRUE, 0, plain->data, &cbData));
    plain->cb_data = cbData;

    cbData = sizeof(cbHash);
    CCall(CryptGetHashParam(hDecHash, HP_HASHSIZE,
                            (BYTE *) &cbHash, &cbData, 0));

    ks_datablob_alloc(&new_hash, cbHash);
    CCall(CryptGetHashParam(hDecHash, HP_HASHVAL,
                            new_hash.data, &cbHash, 0));
    new_hash.cb_data = cbHash;

    if (new_hash.cb_data != hash->cb_data ||
        memcmp(new_hash.data, hash->data, hash->cb_data))
        rv = FALSE;
    else
        rv = TRUE;

 done:
    if (hDecHash)
        CryptDestroyHash(hDecHash);

    ks_datablob_free(&new_hash);

    if (!rv)
        ks_datablob_free(plain);

    return rv;
}

static khm_boolean
verify_key_on_keystore(keystore_t * ks, const void * key, khm_size cb_key)
{
    crypt_op op;
    khm_boolean rv = FALSE;
    khm_size i;
    datablob_t p;

    ks_datablob_init(&p);

    assert(is_keystore_t(ks));

    begin_crypt_op(ks, &op);

    CCall(derive_new_key_from_data(&op, key, cb_key));

    for (i=0; i < ks->n_keys; i++) {
        if (!(ks->keys[i]->flags & IDENTKEY_FLAG_LOCKED))
            continue;

        CCall(decrypt_data(&op, &ks->keys[i]->key,
                           &ks->keys[i]->key_hash, &p));
    }

    rv = TRUE;
 done:
    end_crypt_op(&op);

    ks_datablob_free(&p);
    return rv;
}

khm_int32
ks_keystore_set_key_password(keystore_t * ks, const void * key, khm_size cb_key)
{
    khm_int32 rv = KHM_ERROR_GENERAL;

    assert(is_keystore_t(ks));

    KSLOCK(ks);
    if (verify_key_on_keystore(ks, key, cb_key)) {
        lock_encryption_key(ks, key, cb_key);
        rv = KHM_ERROR_SUCCESS;
    }
    KSUNLOCK(ks);
    return rv;
}

khm_int32
ks_keystore_change_key_password(keystore_t * ks, const void * key, khm_size cb_key)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;

    assert(is_keystore_t(ks));

    KSLOCK(ks);
    rv = ks_keystore_unlock(ks);
    if (KHM_FAILED(rv)) goto done;

    lock_encryption_key(ks, key, cb_key);

 done:
    KSUNLOCK(ks);
    return rv;
}

khm_int32
ks_keystore_reset_key(keystore_t * ks)
{
    assert(is_keystore_t(ks));

    KSLOCK(ks);
    destroy_encryption_key(ks);
    KSUNLOCK(ks);

    return KHM_ERROR_SUCCESS;
}


khm_int32
ks_keystore_unlock(keystore_t * ks)
{
    crypt_op op;
    khm_int32 rv = KHM_ERROR_GENERAL;
    khm_size i;

    assert(is_keystore_t(ks));

    KSLOCK(ks);

    /* This operation vacously suceeds if there are no locked private
       keys */
    for (i=0; i < ks->n_keys; i++) {
        if ((ks->keys[i]->flags & IDENTKEY_FLAG_LOCKED) == IDENTKEY_FLAG_LOCKED)
            break;
    }

    if (i >= ks->n_keys) {
        rv = KHM_ERROR_SUCCESS;
        goto done2;
    }

    if (!has_encryption_key(ks)) {
        rv = KHM_ERROR_NOT_READY;
        goto done2;
    }

    begin_crypt_op(ks, &op);

    {
        datablob_t enc_key;
        khm_boolean rv_dnk;

        ks_datablob_init(&enc_key);

        unlock_encryption_key(ks, &enc_key);
        rv_dnk = derive_new_key_from_data(&op, enc_key.data, enc_key.cb_data);
        ks_datablob_free(&enc_key);

        if (!rv_dnk)
            goto done;
    }

    for (i=0; i < ks->n_keys; i++) {
        if ((ks->keys[i]->flags & IDENTKEY_FLAG_LOCKED) != IDENTKEY_FLAG_LOCKED)
            continue;

        CCall(decrypt_data(&op,
                           &ks->keys[i]->key,
                           &ks->keys[i]->key_hash,
                           &ks->keys[i]->plain_key));

        ks->keys[i]->flags &= ~IDENTKEY_FLAG_LOCKED;
    }

    rv = KHM_ERROR_SUCCESS;
 done:
    end_crypt_op(&op);
 done2:
    KSUNLOCK(ks);

    return rv;
}

khm_int32
ks_keystore_lock(keystore_t * ks)
{
    crypt_op op;
    khm_int32 rv = KHM_ERROR_GENERAL;
    khm_size i;

    assert(is_keystore_t(ks));

    KSLOCK(ks);

    if (!has_encryption_key(ks)) {
        rv = KHM_ERROR_NOT_READY;
        goto done2;
    }

    begin_crypt_op(ks, &op);

    {
        datablob_t enc_key;
        khm_boolean rv_dnk;

        ks_datablob_init(&enc_key);
        unlock_encryption_key(ks, &enc_key);
        rv_dnk = derive_new_key_from_data(&op, enc_key.data, enc_key.cb_data);
        ks_datablob_free(&enc_key);

        if (!rv_dnk)
            goto done;
    }

    for (i=0; i < ks->n_keys; i++) {
        if ((ks->keys[i]->flags & IDENTKEY_FLAG_LOCKED) == IDENTKEY_FLAG_LOCKED)
            continue;

        CCall(encrypt_data(&op, &ks->keys[i]->plain_key,
                           &ks->keys[i]->key,
                           &ks->keys[i]->key_hash));
        ks_datablob_free(&ks->keys[i]->plain_key);
        ks->keys[i]->flags |= IDENTKEY_FLAG_LOCKED;
        ks->flags |= KS_FLAG_MODIFIED;
        GetSystemTimeAsFileTime(&ks->ft_mtime);
    }

    rv = KHM_ERROR_SUCCESS;
 done:
    end_crypt_op(&op);
 done2:
    KSUNLOCK(ks);
    return rv;
}


khm_boolean
ks_is_keystore_locked(keystore_t * ks)
{
    khm_boolean is_locked = FALSE;
    khm_size i;

    assert(is_keystore_t(ks));

    KSLOCK(ks);
    for (i=0; i < ks->n_keys; i++) {
        if ((ks->keys[i]->flags & IDENTKEY_FLAG_LOCKED) == IDENTKEY_FLAG_LOCKED) {
            is_locked = TRUE;
            break;
        }
    }
    KSUNLOCK(ks);

    return is_locked;
}

khm_boolean
ks_keystore_has_key(keystore_t * ks)
{
    khm_boolean has_key = FALSE;

    assert(is_keystore_t(ks));
    KSLOCK(ks);
    has_key = has_encryption_key(ks);
    KSUNLOCK(ks);

    return has_key;
}
