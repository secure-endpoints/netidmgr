/*
 * Copyright (c) 2009-2010 Secure Endpoints Inc.
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
#include <stdio.h>

#define leave_if(e) if (e) { printf("Failed condition: " #e "\n"); __leave; } else { printf("Succeeded: !(" #e ")\n"); }

#define testk(e) do { khm_int32 rv; rv = e; if (KHM_FAILED(rv)) { printf ("Failed: " #e "\n  Return value : 0x%x", rv); __leave; } } while(FALSE)

#define testnk(e) do { khm_int32 rv; rv = e; if (KHM_SUCCEEDED(rv)) { printf ("Succeeded: " #e "\n  Should have failed", rv); __leave; } } while(FALSE)

#define test_if(e) do { if (!(e)) { printf ("False: " #e "\n"); __leave; } } while(FALSE)



void
hexdump(const unsigned char * buf, size_t cb)
{
    size_t i;
    static char nibbles[] =
        { '0', '1', '2', '3', '4', '5', '6', '7',
          '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

    printf("Hexdump of object %p (length: %d)\n", buf, cb);

    for (i=0; i < cb; i += 16) {
        size_t j;

        printf("%04X : ", i);

        for (j=i; j < cb && j < i+16; j++) {
            printf("%c%c ",
                   nibbles[(buf[j] >> 4) & 0xf],
                   nibbles[buf[j] & 0xf]);

            if ((j%16)==7)
                printf("- ");
        }

        for (; j < i+16; j++) {
            printf("   ");

            if ((j%16)==7)
                printf("- ");
        }

        printf("   ");

        for (j=i; j < cb && j < i+16; j++) {
            if (buf[j] > 32 && buf[j] < 127)
                printf("%c", buf[j]);
            else
                printf(".");
            if ((j%16)==7)
                printf("-");
        }

        printf("\n");
    }
}

void
dump_datablob(const datablob_t * d, char * prefix) {
    printf("%sDatablob at %p with %d bytes (%d allocated):\n", prefix,
           d, d->cb_data, d->cb_alloc);
    if (d->cb_data > 0)
        hexdump(d->data, d->cb_data);
}

void
dump_filetime(const FILETIME * ft, const char * prefix) {
    printf("%s", prefix);

    if (ft->dwLowDateTime == 0 && ft->dwHighDateTime == 0) {
        printf("{0,0}\n");
    } else {
        FILETIME lft;
        SYSTEMTIME st;

        FileTimeToLocalFileTime(ft, &lft);
        FileTimeToSystemTime(&lft, &st);

        printf("%02d:%02d:%02d.%03d %04d/%02d/%02d\n",
               (int) st.wHour, (int) st.wMinute,
               (int) st.wSecond, (int) st.wMilliseconds,
               (int) st.wYear, (int) st.wMonth, (int) st.wDay);
    }
}

void
dump_identkey(identkey_t * idk) {
    printf ("IdentKey at %p:\n", idk);

    printf ("  magic: 0x%x\n", idk->magic);
    printf ("  provider_name: [%S]\n", (idk->provider_name ? idk->provider_name : L"[null]"));
    printf ("  identity_name: [%S]\n", (idk->identity_name ? idk->identity_name : L"[null]"));
    printf ("  display_name: [%S]\n", (idk->display_name ? idk->display_name : L"[null]"));
    printf ("  key_description: [%S]\n", (idk->key_description ? idk->key_description : L"[null]"));

    dump_datablob(&idk->key_hash, "  key_hash:");
    dump_datablob(&idk->key, "  key:");
    dump_datablob(&idk->plain_key, "  plain_key:");

    printf("  flags: %d\n", idk->flags);
}


void
dump_keystore(keystore_t * ks) {
    khm_size i;

    printf("Keystore at %p:\n", ks);
    printf("  magic: 0x%x\n", ks->magic);
    printf("  display_name: [%S]\n", (ks->display_name ? ks->display_name : L"[null]"));
    printf("  description: [%S]\n", (ks->description ? ks->description : L"[null]"));
    dump_filetime(&ks->ft_expire, "  ft_expire:");
    dump_filetime(&ks->ft_ctime,  "  ft_ctime:");
    dump_filetime(&ks->ft_mtime,  "  ft_mtime:");

    {
        datablob_t db;

        db.data = ks->DBenc_key.pbData;
        db.cb_data = db.cb_alloc = ks->DBenc_key.cbData;
        dump_datablob(&db, "  enc_key:");
    }

    printf("  # of keys: %d (%d allocated)\n", ks->n_keys, ks->nc_keys);

    for (i=0; i < ks->n_keys; i++) {
        printf("\nKey #%d:\n", i);
        dump_identkey(ks->keys[i]);
    }
}

void KHMCALLBACK config_dump_cb(khm_int32 type, const wchar_t * name,
                                const void * data, khm_size cb, void * ctx)
{
    int *pi = (int *) ctx;
    int i;

    for (i=0; i < *pi; i++) {
        printf ("  ");
    }

    switch(type) {
    case KC_SPACE:
        printf("KC_SPACE: %S\n", name);
        (*pi)++;
        break;

    case KC_ENDSPACE:
        (*pi)--;
        printf("KC_ENDSPACE\n\n");
        break;

    case KC_INT32:
        printf("KC_INT32: %S=%d\n", name, *(khm_int32 *)data);
        break;

    case KC_INT64:
        printf("KC_INT64: %S=%i64d\n", name, *(khm_int64 *)data);
        break;

    case KC_STRING:
        printf("KC_STRING: %S=\"%S\"\n", name, (wchar_t *)data);
        break;

    case KC_MTIME:
        printf("KC_MTIME:\n");
        break;

    case KC_BINARY:
        printf("KC_BINARY:\n");
        hexdump(data, cb);
        break;

    default:
        printf("TYPE=%d\n", type);
        hexdump(data, cb);
    }
}

int main(int argc, char ** argv)
{
    keystore_t * ks = NULL;
    keystore_t * ksu = NULL;
    identkey_t * idk = NULL;
    khm_size cb;
    void * buf = NULL;
    void * cbuf = NULL;
    khm_handle h = NULL;

    __try {
        ks = ks_keystore_create_new();
        leave_if(ks == NULL);

        testk(ks_keystore_set_string(ks, KCDB_RES_DISPLAYNAME, L"Test keystore"));
        testk(ks_keystore_set_string(ks, KCDB_RES_DESCRIPTION, L"Some description"));
        testk(ks_keystore_set_key_password(ks, "foobarbaz", 10));

        idk = ks_identkey_create_new();
        leave_if(idk == NULL);

        idk->provider_name = _wcsdup(L"Foo provider");
        idk->identity_name = _wcsdup(L"My identity");
        idk->display_name = _wcsdup(L"My identity display name");

        testk(ks_keystore_add_identkey(ks, idk));
        ks_datablob_copy(&idk->plain_key, "This is a plain key", 20, 0);

        printf("\nBefore locking ======\n");
        dump_keystore(ks);

        testk(ks_keystore_lock(ks));

        printf("\nAfter locking ======\n");
        dump_keystore(ks);

        testk(ks_keystore_unlock(ks));

        printf("\nAfter unlocking ======\n");
        dump_keystore(ks);

        testk(ks_keystore_lock(ks));
        testnk(ks_keystore_set_key_password(ks, "wrongpassword", 14));

        cb = 0;
        test_if(ks_keystore_serialize(ks, NULL, &cb) == KHM_ERROR_TOO_LONG);
        printf("\nSize of serialized buffer: %d\n", cb);

        buf = malloc(cb);
        testk(ks_keystore_serialize(ks, buf, &cb));

        printf("Serialized buffer:\n");
        hexdump(buf, cb);

        testk(ks_keystore_unserialize(buf, cb, &ksu));

        printf("Unserialized buffer:\n");
        dump_keystore(ksu);

        testk(khc_memory_store_create(&h));

        {
            khm_int32 i32;

            khm_int64 i64;

            testk(khc_memory_store_add(h, L"A", KC_SPACE, NULL, 0));

            i32 = 12;
            testk(khc_memory_store_add(h, L"IValue", KC_INT32, &i32, sizeof(i32)));

            testk(khc_memory_store_add(h, L"SValue", KC_STRING, L"The rain in Spain stays mainly on the plane?", 1024));

            testk(khc_memory_store_add(h, L"BValue", KC_BINARY,
                                       "Lorem ipsum dolo"
                                       "r sit amet, cons"
                                       "ectetur adipisic"
                                       "ing elit, sed do"
                                       " eiusmod tempor", 79));

            testk(khc_memory_store_add(h, L"New space! This is a new space!", KC_SPACE, NULL, 0));

            testk(khc_memory_store_add(h, L"Fooblarg", KC_SPACE, NULL, 0));

            i64 = 0x9382393029348239i64;
            testk(khc_memory_store_add(h, L"64bit integer value", KC_INT64, &i64, sizeof(i64)));

            testk(khc_memory_store_add(h, L"Bonorum", KC_STRING,
                                       L"Sed ut perspiciatis unde omnis iste natus error sit voluptatem accusantium doloremque laudantium, totam rem aperiam, eaque ipsa quae ab illo inventore veritatis et quasi architecto beatae vitae dicta sunt explicabo. Nemo enim ipsam voluptatem quia voluptas sit aspernatur aut odit aut fugit, sed quia consequuntur magni dolores eos qui ratione voluptatem sequi nesciunt. Neque porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore magnam aliquam quaerat voluptatem. Ut enim ad minima veniam, quis nostrum exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? Quis autem vel eum iure reprehenderit qui in ea voluptate velit esse quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo voluptas nulla pariatur?",
                                       KCONF_MAXCB_STRING));

            testk(khc_memory_store_add(h, NULL, KC_ENDSPACE, NULL, 0));

            testk(khc_memory_store_add(h, NULL, KC_ENDSPACE, NULL, 0));

            testk(khc_memory_store_add(h, L"Another space! This is a new space!", KC_SPACE, NULL, 0));

            testk(khc_memory_store_add(h, NULL, KC_ENDSPACE, NULL, 0));

            testk(khc_memory_store_add(h, NULL, KC_ENDSPACE, NULL, 0));
        }

        cb = 0;
        test_if(ks_serialize_configuration(h, NULL, &cb) == KHM_ERROR_TOO_LONG);
        test_if(cb > 0);

        cbuf = malloc(cb);
        testk(ks_serialize_configuration(h, cbuf, &cb));

        printf("Serialized configuration:\n");
        hexdump(cbuf, cb);

        testk(khc_memory_store_release(h));

        testk(ks_unserialize_configuration(cbuf, cb, &h));

        {
            int i = 0;
            printf("\nDump of unserialized configuration space:\n");
            testk(khc_memory_store_enum(h, config_dump_cb, &i));
        }

        printf("Success!\n");

    } __finally {
        if (ks)
            ks_keystore_release(ks);
        if (ksu)
            ks_keystore_release(ksu);
        if (buf)
            free(buf);
        if (cbuf)
            free(cbuf);
        if (h)
            khc_memory_store_release(h);
    }

    return 0;
}
