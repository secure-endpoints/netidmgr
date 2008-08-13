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

/* Encoding and decoding */

typedef struct Codec {
    khm_size  cb_used;
    khm_size  cb_free;
    void *    base;
    void *    current;
    khm_int32 last_error;
} Codec;

#define ENCOK(e) ((e) && KHM_SUCCEEDED((e)->last_error))

#define KSFF_VERSION 1
#define KSFF_DATA_MAGIC          0x7118b33b
#define KSFF_COUNTEDSTRING_MAGIC 0xa4d16aef
#define KSFF_IDENTKEY_MAGIC      0x981cb3ef
#define KSFF_HEADER_MAGIC        0x33f0a303

#define KSFF_ALIGNMENT 4

void *
declare_serial_buffer(Codec * e, khm_size allocsize, khm_boolean init_to_zero)
{
    if (allocsize == 0 || (!ENCOK(e) && e->last_error != KHM_ERROR_TOO_LONG))
        return NULL;

    allocsize = UBOUNDSS(allocsize, KSFF_ALIGNMENT, KSFF_ALIGNMENT);

    if (allocsize > e->cb_free) {

        e->cb_free = 0;
        e->cb_used += allocsize;
        e->last_error = KHM_ERROR_TOO_LONG;

        return NULL;

    } else {
        void * rbuf;

        rbuf = e->current;
        e->current = BYTEOFFSET(e->current, allocsize);
        e->cb_free -= allocsize;
        e->cb_used += allocsize;

        if (init_to_zero)
            memset(rbuf, 0, allocsize);

        return rbuf;
    }
}

void
encode_KS_Size(khm_ui_4 s, Codec * e)
{
    khm_ui_4 * ps = declare_serial_buffer(e, sizeof(s), TRUE);
    if (ps) {
        memcpy(ps, &s, sizeof(s));
    }
}

khm_ui_4
decode_KS_Size(Codec * e)
{
    khm_ui_4 * ps = declare_serial_buffer(e, sizeof(khm_ui_4), FALSE);
    if (ps) {
        khm_ui_4 r;
        memcpy(&r, ps, sizeof(r));
        return r;
    } else
        return 0;
}

void
encode_KS_Int(khm_int32 i, Codec * e)
{
    khm_int32 * pi = declare_serial_buffer(e, sizeof(i), TRUE);
    if (pi) {
        memcpy(pi, &i, sizeof(i));
    }
}

khm_int32
decode_KS_Int(Codec * e)
{
    khm_int32 *pi = declare_serial_buffer(e, sizeof(khm_int32), FALSE);
    if (pi) {
        khm_int32 r;
        memcpy(&r, pi, sizeof(r));
        return *pi;
    } else
        return 0;
}

void
encode_KS_Time(FILETIME ft, Codec * e)
{
    FILETIME * pft = declare_serial_buffer(e, sizeof(FILETIME), TRUE);
    if (pft) {
        memcpy(pft, &ft, sizeof(ft));
    }
}

FILETIME
decode_KS_Time(Codec * e)
{
    FILETIME ft;
    FILETIME * pft = declare_serial_buffer(e, sizeof(ft), FALSE);

    if (pft) {
        memcpy(&ft, pft, sizeof(ft));
    } else {
        ft.dwLowDateTime = 0;
        ft.dwHighDateTime = 0;
    }

    return ft;
}

void
encode_KS_Data(const datablob_t * dsrc,
               Codec * e)
{
    void * db;

    encode_KS_Int(KSFF_DATA_MAGIC, e);
    encode_KS_Size(dsrc->cb_data, e);

    db = declare_serial_buffer(e, dsrc->cb_data, TRUE);
    if (db) {
        memcpy(db, dsrc->data, dsrc->cb_data);
    }
}

datablob_t
decode_KS_Data(Codec * e)
{
    void * data;
    datablob_t d;
    size_t cb;

    ks_datablob_init(&d);

    if (!ENCOK(e) ||
        decode_KS_Int(e) != KSFF_DATA_MAGIC) {

        e->last_error = KHM_ERROR_INVALID_PARAM;
        return d;

    }

    cb = decode_KS_Size(e);
    if (!ENCOK(e) ||
        (data = declare_serial_buffer(e, cb, FALSE)) == NULL ||
        !ENCOK(e)) {
        e->last_error = KHM_ERROR_INVALID_PARAM;
        return d;
    }

    ks_datablob_alloc(&d, cb);
    if (d.data == NULL) {
        e->last_error = KHM_ERROR_NO_RESOURCES;
        return d;
    }

    memcpy(d.data, data, cb);
    d.cb_data = cb;

    return d;
}

void
encode_KS_CountedString(const wchar_t * wstr, khm_size cch_max,
                        Codec * e)
{
    size_t cch = 0;
    wchar_t * s;

    if (FAILED(StringCchLength(wstr, cch_max, &cch)))
        cch = 0;

    encode_KS_Int(KSFF_COUNTEDSTRING_MAGIC, e);
    encode_KS_Size(cch, e);

    s = declare_serial_buffer(e, cch * sizeof(wchar_t), TRUE);
    if (s) {
        memcpy(s, wstr, cch * sizeof(wchar_t)); /* not NULL terminated */
    }
}

wchar_t *
decode_KS_CountedString(Codec * e, khm_size maxcch)
{
    const wchar_t * strsrc;
    wchar_t * str;
    khm_size cch;

    if (!ENCOK(e) ||
        decode_KS_Int(e) != KSFF_COUNTEDSTRING_MAGIC ||
        !ENCOK(e)) {
        e->last_error = KHM_ERROR_INVALID_PARAM;
        return NULL;
    }

    cch = decode_KS_Size(e);
    strsrc = declare_serial_buffer(e, cch * sizeof(wchar_t), FALSE);
    if (cch >= maxcch || !ENCOK(e)) {
        e->last_error = KHM_ERROR_INVALID_PARAM;
        return NULL;
    }

    if (cch > 0) {
        str = malloc((cch + 1) * sizeof(wchar_t));
        if (str == NULL) {
            e->last_error = KHM_ERROR_NO_RESOURCES;
            return NULL;
        }
        StringCchCopyN(str, cch+1, strsrc, cch);
    } else {
        str = NULL;
    }

    return str;
}

void
encode_KS_IdentKey(const identkey_t * idk, Codec * e)
{
    assert(is_identkey_t(idk));

    encode_KS_Int(KSFF_IDENTKEY_MAGIC, e);
    encode_KS_CountedString(idk->provider_name, KCDB_MAXCCH_NAME, e);
    encode_KS_CountedString(idk->identity_name, KCDB_IDENT_MAXCCH_NAME, e);
    encode_KS_CountedString(idk->display_name, KCDB_MAXCCH_SHORT_DESC, e);
    encode_KS_CountedString(idk->key_description, KCDB_MAXCCH_LONG_DESC, e);
    encode_KS_Data(&idk->key_hash, e);
    encode_KS_Data(&idk->key, e);
}

identkey_t *
decode_KS_IdentKey(Codec * e)
{
    identkey_t * idk;

    if (!ENCOK(e) ||
        decode_KS_Int(e) != KSFF_IDENTKEY_MAGIC) {
        e->last_error = KHM_ERROR_INVALID_PARAM;
        return NULL;
    }

    idk = ks_identkey_create_new();
    if (idk == NULL) {
        e->last_error = KHM_ERROR_NO_RESOURCES;
        return NULL;
    }
    idk->provider_name = decode_KS_CountedString(e, KCDB_MAXCCH_NAME);
    idk->identity_name = decode_KS_CountedString(e, KCDB_IDENT_MAXCCH_NAME);
    idk->display_name = decode_KS_CountedString(e, KCDB_MAXCCH_SHORT_DESC);
    idk->key_description = decode_KS_CountedString(e, KCDB_MAXCCH_LONG_DESC);
    idk->key_hash = decode_KS_Data(e);
    idk->key = decode_KS_Data(e);
    idk->flags = IDENTKEY_FLAG_LOCKED;

    if (ENCOK(e))
        return idk;
    ks_identkey_free(idk);
    return NULL;
}

void
encode_KS_Header(const keystore_t * ks, Codec * e)
{
    khm_size i;

    assert(is_keystore_t(ks));

    encode_KS_Int(KSFF_HEADER_MAGIC, e);
    encode_KS_Int(KSFF_VERSION, e);
    encode_KS_CountedString(ks->display_name, KCDB_MAXCCH_NAME, e);
    encode_KS_CountedString(ks->description, KCDB_MAXCCH_LONG_DESC, e);
    encode_KS_Time(ks->ft_expire, e);
    encode_KS_Time(ks->ft_ctime, e);
    encode_KS_Time(ks->ft_mtime, e);
    encode_KS_Size(ks->n_keys, e);
    for (i=0; i < ks->n_keys; i++) {
        encode_KS_IdentKey(ks->keys[i], e);
    }
}

keystore_t *
decode_KS_Header(Codec * e)
{
    keystore_t * ks;

    if (!ENCOK(e) ||
        decode_KS_Int(e) != KSFF_HEADER_MAGIC ||
        decode_KS_Int(e) != KSFF_VERSION ||
        !ENCOK(e)) {

        e->last_error = KHM_ERROR_INVALID_PARAM;
        return NULL;

    }

    ks = ks_keystore_create_new();
    if (ks == NULL) { e->last_error = KHM_ERROR_NO_RESOURCES; return NULL; }
    ks->display_name = decode_KS_CountedString(e, KCDB_MAXCCH_NAME);
    ks->description = decode_KS_CountedString(e, KCDB_MAXCCH_LONG_DESC);
    ks->ft_expire = decode_KS_Time(e);
    ks->ft_ctime = decode_KS_Time(e);
    ks->ft_mtime = decode_KS_Time(e);
    ks->n_keys = decode_KS_Size(e);

    if (ks->n_keys > KS_MAX_KEYS) {
        e->last_error = KHM_ERROR_INVALID_PARAM;
        goto done;
    }

    if (ks->n_keys > 0) {
        khm_size i;

        ks->nc_keys = UBOUNDSS(ks->n_keys, KS_KEY_ALLOC_INCR, KS_KEY_ALLOC_INCR);
        ks->keys = calloc(ks->nc_keys, sizeof(ks->keys[0]));
        if (ks->keys == NULL) {
            e->last_error = KHM_ERROR_NO_RESOURCES;
            goto done;
        }

        for (i=0; i < ks->n_keys; i++) {
            ks->keys[i] = decode_KS_IdentKey(e);
        }
    }

 done:
    if (ENCOK(e))
        return ks;

    ks_keystore_release(ks);
    return NULL;
}

khm_int32
ks_keystore_serialize(const keystore_t * ks, void * buffer, khm_size * pcb_buffer)
{
    Codec encoding;

    encoding.cb_used = 0;
    encoding.cb_free = ((buffer != NULL)? *pcb_buffer : 0);
    encoding.current = encoding.base = buffer;
    encoding.last_error = KHM_ERROR_SUCCESS;

    encode_KS_Header(ks, &encoding);

    *pcb_buffer = encoding.cb_used;
    return encoding.last_error;
}

khm_int32
ks_keystore_unserialize(const void * buffer, khm_size cb_buffer, keystore_t ** pks)
{
    Codec decoding;

    decoding.cb_used = 0;
    decoding.cb_free = cb_buffer;
    decoding.current = decoding.base = (void *) buffer;
    decoding.last_error = KHM_ERROR_SUCCESS;

    *pks = decode_KS_Header(&decoding);

    return decoding.last_error;
}

#ifndef NO_NIM_DEPENDENCIES

#define KSFF_CREDENTIAL_MAGIC 0xb662dc04
#define KSFF_CREDENTIAL_VERSION 1

void
encode_KS_Credential(khm_handle credential, Codec * e)
{
    khm_size n_attrs = 0;
    khm_size n_defined_attrs = 0;
    khm_size i;
    khm_int32 rv = KHM_ERROR_SUCCESS;
    khm_int32 * attrlist = NULL;

    #define AND_FLAGS KCDB_ATTR_FLAG_COMPUTED
    #define EQ_FLAGS  0

    rv = kcdb_attrib_get_count(AND_FLAGS, EQ_FLAGS, &n_attrs);
    if (KHM_FAILED(rv))
        goto done;

    attrlist = malloc(sizeof(khm_int32) * n_attrs);
    rv = kcdb_attrib_get_ids(AND_FLAGS, EQ_FLAGS, attrlist, &n_attrs);
    if (KHM_FAILED(rv))
        goto done;

    /*[1] Magic */
    encode_KS_Int(KSFF_CREDENTIAL_MAGIC, e);
    /*[2] Version */
    encode_KS_Int(KSFF_CREDENTIAL_VERSION, e);

    {
        wchar_t buffer[KCDB_IDENT_MAXCCH_NAME];
        khm_int32 ctype = KCDB_CREDTYPE_INVALID;
        khm_size cb;
        khm_handle identity = NULL;
        khm_handle ident_pro = NULL;

        /*[3] Credential name */
        cb = sizeof(buffer);
        rv = kcdb_cred_get_name(credential, buffer, &cb);
        if (KHM_FAILED(rv)) goto done_with_names;
        encode_KS_CountedString(buffer, KCDB_MAXCCH_NAME, e);

        /*[4] Credential type name */
        rv = kcdb_cred_get_type(credential, &ctype);
        if (KHM_FAILED(rv)) goto done_with_names;
        cb = sizeof(buffer);
        rv = kcdb_credtype_get_name(ctype, buffer, &cb);
        if (KHM_FAILED(rv)) goto done_with_names;
        encode_KS_CountedString(buffer, KCDB_MAXCCH_NAME, e);

        kcdb_cred_get_identity(credential, &identity);
        kcdb_identity_get_identpro(identity, &ident_pro);

        if (identity == NULL || ident_pro == NULL) {
            rv = KHM_ERROR_INVALID_PARAM;
            goto done_with_names;
        }

        /*[5] Identity name */
        cb = sizeof(buffer);
        rv = kcdb_identity_get_name(identity, buffer, &cb);
        if (KHM_FAILED(rv)) goto done_with_names;
        encode_KS_CountedString(buffer, KCDB_IDENT_MAXCCH_NAME, e);

        /*[6] Identity provider name */
        cb = sizeof(buffer);
        rv = kcdb_identpro_get_name(ident_pro, buffer, &cb);
        if (KHM_FAILED(rv)) goto done_with_names;
        encode_KS_CountedString(buffer, KCDB_MAXCCH_NAME, e);

    done_with_names:
        if (identity)
            kcdb_identity_release(identity);
        if (ident_pro)
            kcdb_identpro_release(ident_pro);
        if (KHM_FAILED(rv))
            goto done;
    }

    for (i=0; i < n_attrs; i++) {
        if (KHM_SUCCEEDED(kcdb_cred_get_attr(credential, attrlist[i], NULL, NULL, NULL)))
            n_defined_attrs++;
    }

    /*[7] Number of attributes */
    encode_KS_Size(n_defined_attrs, e);

    for (i=0; i < n_attrs; i++) {
        khm_size cb_data;
        kcdb_attrib * info = NULL;
        datablob_t db;

        rv = kcdb_cred_get_attr(credential, attrlist[i], NULL, NULL, &cb_data);
        if (rv != KHM_ERROR_TOO_LONG)
            continue;

        rv = kcdb_attrib_get_info(attrlist[i], &info);
        if (KHM_FAILED(rv)) {
            assert(FALSE);
            goto done;
        }

        ks_datablob_init(&db);
        ks_datablob_alloc(&db, cb_data);
        db.cb_data = cb_data;

        rv = kcdb_cred_get_attr(credential, attrlist[i], NULL, db.data, &db.cb_data);
        if (KHM_FAILED(rv)) {
            assert(FALSE);
            ks_datablob_free(&db);
            goto done;
        }

        /*[7 + 2j] Attribute name */
        encode_KS_CountedString(info->name, KCDB_MAXCCH_NAME, e);

        /*[7 + 2j + 1] Attribute value */
        encode_KS_Data(&db, e);

        ks_datablob_free(&db);
        kcdb_attrib_release_info(info);

        n_defined_attrs--;
    }

    assert(n_defined_attrs == 0);
    if (n_defined_attrs != 0) {
        /* Bad credential.  We don't consider this case to be a valid
           encoding */
        rv = KHM_ERROR_INVALID_PARAM;
        goto done;
    }

 done:
    if (attrlist)
        free(attrlist);
    if (KHM_FAILED(rv))
        e->last_error = rv;
}

khm_handle
decode_KS_Credential(Codec * e)
{
    khm_handle credential = NULL;
    khm_size n_defined_attrs = 0;
    khm_size i;

    if (!ENCOK(e) ||
        decode_KS_Int(e) != KSFF_CREDENTIAL_MAGIC ||
        decode_KS_Int(e) != KSFF_CREDENTIAL_VERSION ||
        !ENCOK(e)) {
        e->last_error = KHM_ERROR_INVALID_PARAM;
        return NULL;
    }

    {
        wchar_t * cred_name =       decode_KS_CountedString(e, KCDB_MAXCCH_NAME);
        wchar_t * cred_type_name =  decode_KS_CountedString(e, KCDB_MAXCCH_NAME);
        wchar_t * ident_name =      decode_KS_CountedString(e, KCDB_IDENT_MAXCCH_NAME);
        wchar_t * ident_prov_name = decode_KS_CountedString(e, KCDB_MAXCCH_NAME);
        khm_handle identity = NULL;
        khm_handle identpro = NULL;
        khm_int32 ctype;

        if (!ENCOK(e)) goto done_with_names;

        e->last_error = KHM_ERROR_INVALID_PARAM;

        if (KHM_FAILED(kcdb_identpro_find(ident_prov_name, &identpro)))
            goto done_with_names;
        if (KHM_FAILED(kcdb_identity_create_ex(identpro, ident_name,
                                               KCDB_IDENT_FLAG_CREATE,
                                               NULL, &identity)))
            goto done_with_names;
        if (KHM_FAILED(kcdb_credtype_get_id(cred_type_name, &ctype)))
            goto done_with_names;
        if (KHM_FAILED(kcdb_cred_create(cred_name, identity, ctype, &credential)))
            goto done_with_names;

        e->last_error = KHM_ERROR_SUCCESS;

    done_with_names:
        if (identity) kcdb_identity_release(identity);
        if (identpro) kcdb_identpro_release(identpro);
        if (cred_name) free(cred_name);
        if (cred_type_name) free(cred_type_name);
        if (ident_name) free(ident_name);
        if (ident_prov_name) free(ident_prov_name);
    }

    if (!ENCOK(e)) goto done;

    n_defined_attrs = decode_KS_Size(e);

    for (i=0; i < n_defined_attrs && ENCOK(e); i++) {
        wchar_t * attrib_name;
        datablob_t db;

        attrib_name = decode_KS_CountedString(e, KCDB_MAXCCH_NAME);
        db = decode_KS_Data(e);

        if (ENCOK(e))
            kcdb_cred_set_attrib(credential, attrib_name, db.data, db.cb_data);

        if (attrib_name)
            free(attrib_name);
        ks_datablob_free(&db);
    }
    
 done:
    if (ENCOK(e))
        return credential;

    if (credential)
        kcdb_cred_release(credential);
    return NULL;
}

khm_int32
ks_serialize_credential(khm_handle credential, void * buffer, khm_size * pcb_buffer)
{
    Codec encoding;

    encoding.cb_used = 0;
    encoding.cb_free = ((buffer != NULL)? *pcb_buffer : 0);
    encoding.current = encoding.base = buffer;
    encoding.last_error = KHM_ERROR_SUCCESS;

    encode_KS_Credential(credential, &encoding);

    *pcb_buffer = encoding.cb_used;
    return encoding.last_error;
}

khm_int32
ks_unserialize_credential(const void * buffer, khm_size cb_buffer,
                          khm_handle *pcredential)
{
    Codec decoding;

    decoding.cb_used = 0;
    decoding.cb_free = cb_buffer;
    decoding.current = decoding.base = (void *) buffer;
    decoding.last_error = KHM_ERROR_SUCCESS;

    *pcredential = decode_KS_Credential(&decoding);

    return decoding.last_error;
}
#endif
