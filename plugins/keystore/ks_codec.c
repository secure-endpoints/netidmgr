/*
 * Copyright (c) 2008-2009 Secure Endpoints Inc.
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

typedef struct OptRecord {
    khm_int32 signature;
    khm_int32 cb_optional;
} OptRecord;

typedef struct OptArg {
    OptRecord *optRecord;
    khm_size   cb_used;
} OptArg;

#define ENCOK(e) ((e) && KHM_SUCCEEDED((e)->last_error))

#define KSFF_VERSION 3
#define KSFF_DATA_MAGIC          0x7118b33b
#define KSFF_COUNTEDSTRING_MAGIC 0xa4d16aef
#define KSFF_IDENTKEY_MAGIC      0x981cb3ef
#define KSFF_HEADER_MAGIC        0x33f0a303
#define KSFF_OPTSIG_MAGIC        0xfefd0000
#define KSFF_OPTHDR_MAGIC        0xeafd00a3

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
begin_encode_opt_header(Codec * e)
{
    khm_int32 i = KSFF_OPTHDR_MAGIC;
    khm_int32 * pi = declare_serial_buffer(e, sizeof(i), TRUE);
    if (pi) {
        memcpy(pi, &i, sizeof(i));
    }
}

khm_boolean
begin_decode_opt_header(Codec * e)
{
    khm_int32 *pi = e->current;
    if (e->cb_free >= sizeof(khm_int32) && *pi == KSFF_OPTHDR_MAGIC) {
        declare_serial_buffer(e, sizeof(khm_int32), FALSE);
        return TRUE;
    }
    return FALSE;
}

void
begin_encode_optional(khm_ui_2 ordinal, OptArg * opt, Codec * e)
{
    opt->optRecord = declare_serial_buffer(e, sizeof(OptRecord), TRUE);
    if (opt->optRecord)
        opt->optRecord->signature = (KSFF_OPTSIG_MAGIC | (khm_int32) ordinal);
    opt->cb_used = e->cb_used;
}

void
end_encode_optional(OptArg * opt, Codec * e)
{
    if (opt->optRecord) {
        opt->optRecord->cb_optional = (khm_int32) (e->cb_used - opt->cb_used);
    }
}

khm_boolean
begin_decode_optional(khm_ui_2 ordinal, OptArg * opt, Codec * e)
{
    opt->optRecord = e->current;
    if (e->cb_free >= sizeof(OptRecord) &&
        opt->optRecord->signature == (khm_int32) (KSFF_OPTSIG_MAGIC | (khm_int32) ordinal)) {
        declare_serial_buffer(e, sizeof(OptRecord), FALSE);
        return TRUE;
    }
    return FALSE;
}

void
end_decode_optional(OptArg * opt, Codec * e)
{
    /* nothing to do */
}

#define E_OPT(o) { OptArg oa; begin_encode_optional(o, &oa, e);
#define E_ENDOPT() end_encode_optional(&oa, e); }

#define D_OPT(o) { OptArg oa; if (begin_decode_optional(o, &oa, e)) {
#define D_ENDOPT() end_decode_optional(&oa, e); } }

void
skip_optional(Codec * e)
{
    OptRecord * optr;

    while ((optr = e->current) != NULL &&
           e->cb_free >= sizeof(OptRecord) &&
           (optr->signature & 0xffff0000) == KSFF_OPTSIG_MAGIC) {
        declare_serial_buffer(e, sizeof(OptRecord), FALSE);
        declare_serial_buffer(e, optr->cb_optional, FALSE);
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
        return *ps;
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
        return *pi;
    } else
        return 0;
}

void
encode_KS_Int64(khm_int64 i, Codec * e)
{
    khm_int64 * pi = declare_serial_buffer(e, sizeof(i), TRUE);
    if (pi) {
        memcpy(pi, &i, sizeof(i));
    }
}

khm_int64
decode_KS_Int64(Codec * e)
{
    khm_int64 *pi = declare_serial_buffer(e, sizeof(khm_int64), FALSE);
    if (pi) {
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
    encode_KS_Size((khm_ui_4) dsrc->cb_data, e);

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

    if (cb == 0 && ENCOK(e))
        return d;

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
encode_KS_UUID(const UUID * uuid, Codec * e)
{
    UUID * p = declare_serial_buffer(e, sizeof(UUID), TRUE);
    if (p) {
        *p = *uuid;
    }
}

UUID
decode_KS_UUID(Codec * e)
{
    UUID *p = declare_serial_buffer(e, sizeof(UUID), FALSE);
    if (p) {
        return *p;
    } else {
        UUID dummy = {0,0,0,{0,0,0,0,0,0,0,0}};
        return dummy;
    }
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
    encode_KS_Size((khm_ui_4) cch, e);

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
        str = PMALLOC((cch + 1) * sizeof(wchar_t));
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
    E_OPT(7); encode_KS_Int(idk->version, e); E_ENDOPT();
    E_OPT(8); encode_KS_Time(idk->ft_ctime, e); E_ENDOPT();
    E_OPT(9); encode_KS_Time(idk->ft_expire, e); E_ENDOPT();
    E_OPT(10); encode_KS_Data(&idk->configuration, e); E_ENDOPT();
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
    D_OPT(7); idk->version = decode_KS_Int(e); D_ENDOPT();
    D_OPT(8); idk->ft_ctime = decode_KS_Time(e); D_ENDOPT();
    D_OPT(9); idk->ft_expire = decode_KS_Time(e); D_ENDOPT();
    D_OPT(10); idk->configuration = decode_KS_Data(e); D_ENDOPT();
    skip_optional(e);
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
    encode_KS_UUID(&ks->uuid, e);
    encode_KS_CountedString(ks->display_name, KCDB_MAXCCH_NAME, e);
    encode_KS_CountedString(ks->description, KCDB_MAXCCH_LONG_DESC, e);
    encode_KS_Time(ks->ft_expire, e);
    encode_KS_Time(ks->ft_ctime, e);
    encode_KS_Time(ks->ft_mtime, e);
    encode_KS_Size((khm_ui_4) ks->n_keys, e);
    for (i=0; i < ks->n_keys; i++) {
        encode_KS_IdentKey(ks->keys[i], e);
    }
}

keystore_t *
decode_KS_Header(Codec * e)
{
    keystore_t * ks;
    khm_size n_keys;

    if (!ENCOK(e) ||
        decode_KS_Int(e) != KSFF_HEADER_MAGIC ||
        decode_KS_Int(e) != KSFF_VERSION ||
        !ENCOK(e)) {

        e->last_error = KHM_ERROR_INVALID_PARAM;
        return NULL;

    }

    ks = ks_keystore_create_new();
    if (ks == NULL) { e->last_error = KHM_ERROR_NO_RESOURCES; return NULL; }
    ks->uuid = decode_KS_UUID(e);
    ks->display_name = decode_KS_CountedString(e, KCDB_MAXCCH_NAME);
    ks->description = decode_KS_CountedString(e, KCDB_MAXCCH_LONG_DESC);
    ks->ft_expire = decode_KS_Time(e);
    ks->ft_ctime = decode_KS_Time(e);
    ks->ft_mtime = decode_KS_Time(e);
    n_keys = decode_KS_Size(e);

    if (n_keys > KS_MAX_KEYS) {
        e->last_error = KHM_ERROR_INVALID_PARAM;
        goto done;
    }

    if (n_keys > 0) {
        khm_size i;

        for (i=0; i < n_keys; i++) {
            identkey_t * idkey;

            idkey = decode_KS_IdentKey(e);
            if (idkey) {
                if (KHM_FAILED(ks_keystore_add_identkey(ks, idkey)))
                    ks_identkey_free(idkey);
            }
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

    attrlist = PMALLOC(sizeof(khm_int32) * n_attrs);
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
    encode_KS_Size((khm_ui_4) n_defined_attrs, e);

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
        PFREE(attrlist);
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
        if (cred_name) PFREE(cred_name);
        if (cred_type_name) PFREE(cred_type_name);
        if (ident_name) PFREE(ident_name);
        if (ident_prov_name) PFREE(ident_prov_name);
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
            PFREE(attrib_name);
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

#define KSFF_CONFIGURATION_MAGIC 0x1fca108d

#define KSFF_CONFIGURATION_EOF   0x08bd6865

static void KHMCALLBACK config_enum_cb(khm_int32 type, const wchar_t * name,
                                       const void * data, khm_size cb, void * ctx)
{
    Codec * e = (Codec *) ctx;

    encode_KS_Int(type, e);

    switch(type) {
    case KC_SPACE:
        encode_KS_CountedString(name, KCONF_MAXCCH_NAME, e);
        break;

    case KC_ENDSPACE:
        break;

    case KC_INT32:
        encode_KS_CountedString(name, KCONF_MAXCCH_NAME, e);
        encode_KS_Int(*((const khm_int32 *) data), e);
        break;

    case KC_INT64:
        encode_KS_CountedString(name, KCONF_MAXCCH_NAME, e);
        encode_KS_Int64(*((const khm_int64 *) data), e);
        break;

    case KC_STRING:
        encode_KS_CountedString(name, KCONF_MAXCCH_NAME, e);
        encode_KS_CountedString((const wchar_t *) data, cb / sizeof(wchar_t), e);
        break;

    case KC_BINARY:
        {
            datablob_t d;

            ks_datablob_init(&d);
            d.data = (void *) data;
            d.cb_data = cb;

            encode_KS_CountedString(name, KCONF_MAXCCH_NAME, e);
            encode_KS_Data(&d, e);
        }
        break;

    case KC_MTIME:
        encode_KS_Time(*((const FILETIME *) data), e);
        break;

    default:
        assert(FALSE);
    }
}

void
encode_KS_Configuration(khm_handle conf, Codec * e)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;

    encode_KS_Int(KSFF_CONFIGURATION_MAGIC, e);

    rv = khc_memory_store_enum(conf, config_enum_cb, e);
    if (KHM_FAILED(rv))
        goto done;

    encode_KS_Int(KSFF_CONFIGURATION_EOF, e);

 done:
    if (KHM_FAILED(rv))
        e->last_error = rv;
}

khm_handle
decode_KS_Configuration(Codec * e)
{
    khm_handle conf = NULL;

    if (!ENCOK(e) ||
        decode_KS_Int(e) != KSFF_CONFIGURATION_MAGIC ||

        KHM_FAILED(khc_memory_store_create(&conf))) {

        e->last_error = KHM_ERROR_INVALID_PARAM;
        return NULL;
    }

    while (TRUE) {
        khm_int32 rec;
        khm_int32 rv = KHM_ERROR_INVALID_PARAM;
        wchar_t * name = NULL;

        rec = decode_KS_Int(e);

        if (!ENCOK(e) || rec == KSFF_CONFIGURATION_EOF)
            break;

        switch (rec) {
        case KC_SPACE:
            {
                name = decode_KS_CountedString(e, KCONF_MAXCCH_NAME);
                if (ENCOK(e))
                    rv = khc_memory_store_add(conf, name, KC_SPACE, NULL, 0);
            }
            break;

        case KC_ENDSPACE:
            rv = khc_memory_store_add(conf, NULL, KC_ENDSPACE, NULL, 0);
            break;

        case KC_INT32:
            {
                khm_int32 i;
                name = decode_KS_CountedString(e, KCONF_MAXCCH_NAME);
                i = decode_KS_Int(e);
                if (ENCOK(e))
                    rv = khc_memory_store_add(conf, name, KC_INT32, &i, sizeof(i));
            }
            break;

        case KC_INT64:
            {
                khm_int64 i;
                name = decode_KS_CountedString(e, KCONF_MAXCCH_NAME);
                i = decode_KS_Int64(e);
                if (ENCOK(e))
                    rv = khc_memory_store_add(conf, name, KC_INT64, &i, sizeof(i));
            }
            break;

        case KC_STRING:
            {
                wchar_t * val;
                name = decode_KS_CountedString(e, KCONF_MAXCCH_NAME);
                val = decode_KS_CountedString(e, KCONF_MAXCCH_STRING);
                if (ENCOK(e))
                    rv = khc_memory_store_add(conf, name, KC_STRING, val, KCONF_MAXCB_STRING);
                if (val)
                    PFREE(val);
            }
            break;

        case KC_BINARY:
            {
                datablob_t d;

                name = decode_KS_CountedString(e, KCONF_MAXCCH_NAME);
                d = decode_KS_Data(e);

                if (ENCOK(e))
                    rv = khc_memory_store_add(conf, name, KC_BINARY, d.data, d.cb_data);
                ks_datablob_free(&d);
            }
            break;

        case KC_MTIME:
            {
                FILETIME ft;

                ft = decode_KS_Time(e);
                if (ENCOK(e))
                    rv = khc_memory_store_add(conf, NULL, KC_MTIME, &ft, sizeof(ft));
            }
            break;

        default:
            assert(FALSE);
        }

        if (KHM_FAILED(rv))
            e->last_error = rv;

        if (!ENCOK(e))
            break;
    }

    if (ENCOK(e))
        return conf;

    if (conf)
        khc_memory_store_release(conf);
    return NULL;
}

khm_int32
ks_serialize_configuration(khm_handle conf, void * buffer, khm_size * pcb_buffer)
{
    Codec encoding;

    encoding.cb_used = 0;
    encoding.cb_free = ((buffer != NULL)? *pcb_buffer : 0);
    encoding.current = encoding.base = buffer;
    encoding.last_error = KHM_ERROR_SUCCESS;

    encode_KS_Configuration(conf, &encoding);

    *pcb_buffer = encoding.cb_used;
    return encoding.last_error;
}

khm_int32
ks_unserialize_configuration(const void * buffer, khm_size cb_buffer,
                             khm_handle * conf)
{
    Codec decoding;

    decoding.cb_used = 0;
    decoding.cb_free = cb_buffer;
    decoding.current = decoding.base = (void *) buffer;
    decoding.last_error = KHM_ERROR_SUCCESS;

    *conf = decode_KS_Configuration(&decoding);

    return decoding.last_error;
}

#endif
