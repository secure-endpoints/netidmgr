/*
 * Copyright (c) 2009 Secure Endpoints Inc.
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
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <sys/stat.h>
#include <stdio.h>
#include <wchar.h>

const char * ks_filename = NULL;

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
dump_filetime_interval(const FILETIME * ft, const char * prefix) {
    wchar_t srep[1024];
    khm_size cb;

    printf("%s", prefix);

    cb = sizeof(srep);
    FtIntervalToString(ft, srep, &cb);

    wprintf(L"%s\n", srep);
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
    dump_datablob(&idk->configuration, "  configuration:");

    {
        khm_handle h = NULL;
        int i = 0;

        if (idk->configuration.cb_data != 0) {
            printf ("\n  Configuration:\n");

            if (KHM_SUCCEEDED(ks_unserialize_configuration(idk->configuration.data,
                                                           idk->configuration.cb_data,
                                                           &h))) {
                khc_memory_store_enum(h, config_dump_cb, &i);

                khc_memory_store_release(h);
            } else {
                printf ("!Error decoding configuration data!");
            }
        }
    }

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
    dump_filetime_interval(&ks->ft_key_lifetime, "  ft_key_lifetime:");

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

void usage(const char * pn)
{
    printf("Usage: %s filename\n"
           "  Dumps the contents of a Keystore\n", pn);
}

int main(int argc, char ** argv)
{
    keystore_t * ks = NULL;
    int fd = -1;
    long len;
    void * buffer = NULL;
    int read_len;
    khm_int32 rv;

    __try {
        if (argc != 2) {
            usage(argv[0]);
            return 0;
        }

        fprintf(stderr, "Trying to open file %s\n", argv[1]);

        _sopen_s(&fd, argv[1], _O_RDONLY | _O_BINARY | _O_SEQUENTIAL, _SH_DENYNO, _S_IREAD);

        if (fd == -1) {
            perror("_open");
            return 1;
        }

        len = _lseek(fd, 0, SEEK_END);
        if (len == -1) {
            perror("_lseek");
            return 1;
        }

        _lseek(fd, 0, SEEK_SET);

        buffer = malloc(len);

        read_len = _read(fd, buffer, len);

        if (read_len == -1) {
            perror("_read");
            return 1;
        }

        if (read_len != len) {
            fprintf(stderr, "Expected read len %ld.  Actual len %d\n", len, read_len);
            return 1;
        }

        if (KHM_FAILED(rv = ks_keystore_unserialize(buffer, len, &ks))) {
            fprintf(stderr, "ks_keystore_unserialize() == %d\n", rv);
            return 1;
        }

        dump_keystore(ks);

    } __finally {
        if (fd != -1)
            _close(fd);

        if (buffer != NULL)
            free(buffer);

        if (ks != NULL)
            ks_keystore_release(ks);
    }

    return 0;
}
