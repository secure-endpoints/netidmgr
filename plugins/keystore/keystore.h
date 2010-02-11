/*
 * Copyright (c) 2008-2010 Secure Endpoints Inc.
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

#pragma once

typedef struct tag_keystore keystore_t;

typedef struct tag_datablob {
    void *       data;
    khm_size     cb_data;
    khm_size     cb_alloc;
} datablob_t;

#define datablob_is_empty(d) ((d)->cb_data == 0 || (d)->data == NULL)

typedef struct tag_identkey {
    khm_int32    magic;

    wchar_t     *provider_name;
    wchar_t     *identity_name;
    wchar_t     *display_name;
    wchar_t     *key_description;

    datablob_t   key_hash;
    datablob_t   key;

    datablob_t   plain_key;

    datablob_t   configuration;

    khm_int32    version;
    FILETIME     ft_ctime;
    FILETIME     ft_expire;

    khm_int32    flags;
#define IDENTKEY_FLAG_LOCKED   0x00000001
#define IDENTKEY_FLAG_DELETED  0x00000002
#define IDENTKEY_FLAG_CFGMOUNT 0x00000004

    khm_handle   cfg_store;     /*!< Handle to configuration store. */

    khc_provider_interface provider;

} identkey_t;

#define IDENTKEY_MAGIC 0x58b67ff2

#define is_identkey_t(idk) ((idk) && (idk)->magic == IDENTKEY_MAGIC)

typedef struct tag_keystore {
    khm_int32    magic;

    UUID         uuid;

    wchar_t     *display_name;
    wchar_t     *description;
    wchar_t     *location;

    khm_handle   identity;      /*!< Identity handle.  If known */

    FILETIME     ft_expire;     /*!< Expiration time for keystore */
    FILETIME     ft_ctime;      /*!< Creation time of keystore */
    FILETIME     ft_mtime;      /*!< Time of last modification */

    FILETIME     ft_key_lifetime; /*!< Lifetime of key */

    /* Default lifetime of a keystore master key is 30 minutes */
#define KS_DEFAULT_KEY_LIFETIME SECONDS_TO_FT(1800)

    /* Minimum lifetime of a keystore master key is 5 minutes */
#define KS_MIN_KEY_LIFETIME SECONDS_TO_FT(300)

    /* Maximum lifetime of a keystore master key is 7 days */
#define KS_MAX_KEY_LIFETIME SECONDS_TO_FT(60 * 60 * 24 * 7)

    /* Infinity is a couple of days under 8 years */
#define KS_INF_KEY_LIFETIME SECONDS_TO_FT(60 * 60 * 24 * 365 * 8)

    LONG         key_refcount;  /*!< Number of key references.  If the
                                  reference count is non-zero, the key
                                  isn't discarded. */

    FILETIME     ft_key_ctime;    /*!< Creation time of key */
    FILETIME     ft_key_expire; /*!< Expiration time for encryption key */

    khm_size     n_keys;
    khm_size     nc_keys;
    identkey_t **keys;
#define KS_KEY_ALLOC_INCR 4
#define KS_MAX_KEYS       256

    khm_int32    flags;
#define KS_FLAG_SEEN     0x00000001
#define KS_FLAG_MODIFIED 0x00000002

    DATA_BLOB    DBenc_key;     /*!< Encryption key */
    CRITICAL_SECTION cs;
    LONG         refcount;
} keystore_t;

#define KEYSTORE_MAGIC 0x83d65bdd

#define is_keystore_t(ks) ((ks) && (ks)->magic == KEYSTORE_MAGIC)

#define KSLOCK(ks)   EnterCriticalSection(&(ks)->cs)
#define KSUNLOCK(ks) LeaveCriticalSection(&(ks)->cs)





extern void
ks_datablob_init(datablob_t * pd);

extern void
ks_datablob_free(datablob_t * pd);

extern datablob_t *
ks_datablob_copy(datablob_t *dest, const void * data, khm_size cb_data,
                 khm_size align);

extern datablob_t *
ks_datablob_dup(datablob_t * dest, const datablob_t * src);

extern datablob_t *
ks_datablob_alloc(datablob_t * dest, khm_size cb);




extern identkey_t *
ks_identkey_create_new(void);

extern khm_int32
ks_identkey_free(identkey_t * idk);




extern keystore_t *
ks_keystore_create_new(void);

extern void
ks_keystore_hold(keystore_t * ks);

extern void
ks_keystore_release(keystore_t * ks);

extern khm_int32
ks_keystore_add_identkey(keystore_t * ks, identkey_t * idk);

extern khm_int32
ks_keystore_remove_identkey(keystore_t * ks, khm_size idx);

extern khm_int32
ks_keystore_mark_remove_identkey(keystore_t * ks, khm_size idx);

extern khm_int32
ks_keystore_purge_removed_identkeys(keystore_t * ks);

extern khm_int32
ks_keystore_get_identkey(keystore_t * ks, khm_size idx, identkey_t **pidk);

extern khm_int32
ks_keystore_set_string(keystore_t * ks, kcdb_resource_id r_id,
                       const wchar_t * buffer);

extern khm_int32
ks_keystore_get_string(keystore_t * ks, kcdb_resource_id r_id,
                       wchar_t * buffer, khm_size *pcb_buffer);

extern khm_int32
ks_keystore_set_flags(keystore_t * ks, khm_int32 mask, khm_int32 flags);

extern khm_int32
ks_keystore_get_flags(keystore_t * ks);

extern khm_int32
ks_keystore_set_key_password(keystore_t * ks, const void * key, khm_size cb_key);

extern khm_int32
ks_keystore_change_key_password(keystore_t * ks, const void * newkey, khm_size cb_newkey);

extern khm_int32
ks_keystore_reset_key(keystore_t * ks);

extern khm_boolean
ks_keystore_has_key(keystore_t * ks);

extern khm_boolean
ks_keystore_hold_key(keystore_t * ks);

extern void
ks_keystore_release_key(keystore_t * ks);

extern khm_boolean
ks_keystore_reset_key_timer(keystore_t * ks);

extern khm_int32
ks_keystore_unlock(keystore_t * ks);

extern khm_int32
ks_keystore_lock(keystore_t * ks);

extern khm_boolean
ks_is_keystore_locked(keystore_t * ks);

extern khm_int32
ks_keystore_serialize(const keystore_t * ks, void * buffer, khm_size * pcb_buffer);

extern khm_int32
ks_keystore_unserialize(const void * buffer, khm_size cb_buffer, keystore_t ** pks);


/* Must always be less than maximum value of size_t */
#define MAX_SERIALIZED_KEYSTORE_SIZE 262144L

extern khm_int32
ks_serialize_credential(khm_handle credential, void * buffer, khm_size * pcb_buffer);

extern khm_int32
ks_unserialize_credential(const void * buffer, khm_size cb_buffer,
                          khm_handle *pcredential);

khm_int32
ks_serialize_configuration(khm_handle conf, void * buffer, khm_size * pcb_buffer);

khm_int32
ks_unserialize_configuration(const void * buffer, khm_size cb_buffer,
                             khm_handle * conf);
