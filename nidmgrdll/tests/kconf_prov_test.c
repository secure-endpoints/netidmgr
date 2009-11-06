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

#include "tests.h"
#include <stdlib.h>
#include <wchar.h>
#include <strsafe.h>

extern khc_provider_interface test_provider;

typedef struct data {
    wchar_t   *name;
    khm_int32  type;

    union {
        khm_int32  i32;
        khm_int64  i64;
        wchar_t *  str;
        struct {
            void * bin;
            int    cb_bin;
        };
    };
} p_value;

enum {
    CIDX_p_init,
    CIDX_p_exit,
    CIDX_p_open,
    CIDX_p_close,
    CIDX_p_remove,
    CIDX_p_create,
    CIDX_p_begin_enum,
    CIDX_p_get_mtime,
    CIDX_p_read_value,
    CIDX_p_write_value,
    CIDX_p_remove_value,
    CIDX_N
};

const char * CIDX_names[] = {
    "p_init",
    "p_exit",

    "p_open",
    "p_close",
    "p_remove",

    "p_create",
    "p_begin_enum",
    "p_get_mtime",

    "p_read_value",
    "p_write_value",
    "p_remove_value"
};

typedef struct test_provider_data {
    khm_int32 magic;
#define MAGIC 0x1234abcd

    wchar_t * path;
    khm_int32  flags;

    int       open_count;
    khm_handle h;

    p_value * values;
    int       n_values;

    int       entry_count[CIDX_N];

    LDCL(struct test_provider_data);
} p_data;

p_data * all_data = NULL;
int n_all_data = 0;

p_data * get_data_for(const wchar_t * path, khm_int32 flags) {
    p_data * p;

    for (p = all_data; p; p = LNEXT(p)) {
        if (!wcscmp(p->path, path) &&
            (flags & (KCONF_FLAG_USER|KCONF_FLAG_MACHINE|KCONF_FLAG_SCHEMA)) ==
            (p->flags & (KCONF_FLAG_USER|KCONF_FLAG_MACHINE|KCONF_FLAG_SCHEMA)))
            return p;
    }
    return NULL;
}

#define ENTER(f)                                                        \
    do {                                                                \
        d->entry_count[ CIDX_ ## f ]++;                                 \
        log("%s(%d) :        Entering " #f " for [%S::%s] with count %d\n", \
            __FILE__, __LINE__,                                         \
            d->path,                                                    \
            ((d->flags & KCONF_FLAG_USER)? "USER":                      \
             (d->flags & KCONF_FLAG_MACHINE)? "MACHINE":                \
             (d->flags & KCONF_FLAG_SCHEMA)? "SCHEMA": "**UNKNOWN**"),  \
            d->entry_count[ CIDX_ ## f ]);                              \
    } while(0)

#define LEAVE(f)                                                       \
    do {                                                               \
        log("%s(%d) :        Leaving  " #f " for [%S::%s]\n",          \
            __FILE__, __LINE__,                                        \
            d->path,                                                   \
            ((d->flags & KCONF_FLAG_USER)? "USER":                     \
             (d->flags & KCONF_FLAG_MACHINE)? "MACHINE":               \
             (d->flags & KCONF_FLAG_SCHEMA)? "SCHEMA": "**UNKNOWN**")  \
            );                                                         \
    } while(0)

void free_value_contents(p_value * v)
{
    if (v->name) {
        free(v->name);
    }

    switch (v->type) {
    case KC_STRING:
        if (v->str)
            free(v->str);
        v->str = NULL;
        break;

    case KC_BINARY:
        if (v->bin)
            free(v->bin);
        v->bin = NULL;
        break;
    }

    memset(v, 0, sizeof(*v));
}

khm_int32 KHMCALLBACK p_init(khm_handle sp_handle,
                             const wchar_t * path, khm_int32 flags,
                             void * context, void ** r_nodeHandle)
{
    p_data * d = malloc(sizeof(*d));

    memset(d, 0, sizeof(*d));

    LINIT(d);
    d->magic = MAGIC;
    d->h = sp_handle;
    d->path = wcsdup(path);
    d->flags = flags;

    LPUSH(&all_data, d);
    n_all_data++;

    ENTER(p_init);

    *r_nodeHandle = d;

    CHECK(path != NULL);
    CHECK(context == NULL || !wcscmp(path, context));
    CHECK(d != NULL);
    CHECK(d->h != NULL);
    CHECK(flags & (KCONF_FLAG_USER|KCONF_FLAG_MACHINE|KCONF_FLAG_SCHEMA));
    CHECK(IS_POW2(flags & (KCONF_FLAG_USER|KCONF_FLAG_MACHINE|KCONF_FLAG_SCHEMA)));

    LEAVE(p_init);

    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK p_exit(void * nodeHandle)
{
    p_data * d = (p_data *) nodeHandle;
    CHECK(d && d->magic == MAGIC);

    ENTER(p_exit);

    CHECK(d->open_count >= 0);
    {
        int i;

        for (i=0; i < d->n_values; i++) {
            free_value_contents(&d->values[i]);
        }
        if (d->values)
            free(d->values);
    }
    {
        int i;

        log("    Invocation counts:\n");
        for (i=0; i < CIDX_N; i++) {
            log("        %s = %d\n", CIDX_names[i], d->entry_count[i]);
        }
    }

    n_all_data--;
    LDELETE(&all_data, d);

    LEAVE(p_exit);

    free (d);
    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK p_open(void * nodeHandle)
{
    p_data * d = (p_data *) nodeHandle;
    CHECK(d && d->magic == MAGIC);
    ENTER(p_open);

    d->open_count++;

    log("%s(%d) :        Handle count is %d\n", __FILE__, __LINE__, d->open_count);

    LEAVE(p_open);
    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK p_close(void * nodeHandle)
{
    p_data * d = (p_data *) nodeHandle;
    CHECK(d && d->magic == MAGIC);
    ENTER(p_close);

    d->open_count--;

    CHECK(d->open_count >= 0);

    if (d->open_count == 0)
        log("%s(%d) :        CLOSING. Handle count is 0\n", __FILE__, __LINE__);
    else
        log("%s(%d) :        Handle count is %d\n", __FILE__, __LINE__, d->open_count);

    LEAVE(p_close);
    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK p_remove(void * nodeHandle)
{
    p_data * d = (p_data *) nodeHandle;
    CHECK(d && d->magic == MAGIC);
    ENTER(p_remove);

    LEAVE(p_remove);
    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK p_create(void * nodeHandle, const wchar_t * name, khm_int32 flags)
{
    p_data * d = (p_data *) nodeHandle;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    CHECK(d && d->magic == MAGIC);
    ENTER(p_create);

    CHECK(name != NULL);
    CHECK(wcschr(name, L'\\') == NULL);
    CHECK(wcschr(name, L'/') == NULL);
    CHECK((flags & ~(KCONF_FLAG_USER|KCONF_FLAG_MACHINE|KCONF_FLAG_SCHEMA|KHM_FLAG_CREATE)) == 0);
    CHECK(IS_POW2(flags & (KCONF_FLAG_USER|KCONF_FLAG_MACHINE|KCONF_FLAG_SCHEMA)));

    if (flags & KHM_FLAG_CREATE) {

        CHECK(KHM_SUCCEEDED(khc_mount_provider(d->h, name, flags, &test_provider, NULL, NULL)));

        rv = KHM_ERROR_SUCCESS;
    } else {
        rv = KHM_ERROR_NOT_FOUND;
    }

    LEAVE(p_create);
    return rv;
}

khm_int32 KHMCALLBACK p_begin_enum(void * nodeHandle)
{
    p_data * d = (p_data *) nodeHandle;
    CHECK(d && d->magic == MAGIC);
    ENTER(p_begin_enum);

    LEAVE(p_begin_enum);
    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK p_get_mtime(void * nodeHandle, FILETIME * mtime)
{
    p_data * d = (p_data *) nodeHandle;
    CHECK(d && d->magic == MAGIC);
    ENTER(p_get_mtime);

    LEAVE(p_get_mtime);
    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK p_read_value(void * nodeHandle, const wchar_t * valuename,
                                   khm_int32 * vtype, void * buffer, khm_size * pcb)
{
    int i;
    khm_int32 rv = KHM_ERROR_SUCCESS;
    size_t cch;
    p_value * v = NULL;
    p_data * d = (p_data *) nodeHandle;
    CHECK(d && d->magic == MAGIC);
    ENTER(p_read_value);

    CHECK(valuename != NULL);
    CHECK(SUCCEEDED(StringCchLength(valuename, KCONF_MAXCCH_NAME, &cch)));
    CHECK(wcschr(valuename, L'\\') == NULL);
    CHECK(wcschr(valuename, L'/') == NULL);
    CHECK(vtype != NULL &&
          (*vtype == KC_NONE ||
           *vtype == KC_INT32 ||
           *vtype == KC_INT64 ||
           *vtype == KC_STRING ||
           *vtype == KC_BINARY));
    CHECK(buffer == NULL || pcb != NULL);

    for (i=0; i < d->n_values; i++) {
        if (valuename == d->values[i].name) {
            v = &d->values[i];
            break;
        }
    }

    if (v == NULL) {
        rv = KHM_ERROR_NOT_FOUND;
    } else if (*vtype != KC_NONE && *vtype != v->type) {
        rv = KHM_ERROR_TYPE_MISMATCH;
    } else if (buffer == NULL && pcb == NULL) {
        rv = KHM_ERROR_SUCCESS;
    } else {
        khm_size cb = 0;
        void * src = NULL;

        switch (v->type) {
        case KC_INT32:
            src = &v->i32;
            cb = sizeof(khm_int32);
            break;

        case KC_INT64:
            src = &v->i64;
            cb = sizeof(khm_int64);
            break;

        case KC_STRING:
            src = v->str;
            StringCbLength(v->str, KCONF_MAXCCH_STRING, &cb);
            cb += sizeof(wchar_t);
            break;

        case KC_BINARY:
            src = v->bin;
            cb = v->cb_bin;
            break;

        default:
            abort();
        }

        CHECK(src != NULL && cb != 0);
        if (buffer == NULL || *pcb < cb) {
            *pcb = cb;
            rv = KHM_ERROR_TOO_LONG;
        } else {
            *pcb = cb;
            memcpy(buffer, src, cb);
            rv = KHM_ERROR_SUCCESS;
        }
    }

    LEAVE(p_read_value);
    return rv;
}


khm_int32 KHMCALLBACK p_write_value(void * nodeHandle, const wchar_t * valuename,
                                    khm_int32 vtype, const void * buffer, khm_size cb)
{
    int i;
    p_value * v = NULL;
    size_t cch;
    p_data * d = (p_data *) nodeHandle;
    CHECK(d && d->magic == MAGIC);
    ENTER(p_write_value);

    CHECK(valuename != NULL);
    CHECK(SUCCEEDED(StringCchLength(valuename, KCONF_MAXCCH_NAME, &cch)));
    CHECK(vtype == KC_INT32 ||
          vtype == KC_INT64 ||
          vtype == KC_STRING ||
          vtype == KC_BINARY);
    CHECK(buffer != NULL);
    CHECK(wcschr(valuename, L'\\') == NULL);
    CHECK(wcschr(valuename, L'/') == NULL);

    for (i=0; i < d->n_values; i++) {
        if (!wcscmp(valuename, d->values[i].name)) {
            free_value_contents(&d->values[i]);
            v = &d->values[i];
            break;
        }
    }

    if (v == NULL) {
        v = realloc(d->values, sizeof(d->values[0]) * (d->n_values + 1));
        if (v == NULL) {
            abort();
        }
        d->values = v;
        v = &d->values[d->n_values++];
        memset(v, 0, sizeof(*v));
    }

    v->name = wcsdup(valuename);
    v->type = vtype;

    switch (v->type) {
    case KC_INT32:
        v->i32 = *((khm_int32 *)buffer);
        CHECK(cb == sizeof(khm_int32));
        break;

    case KC_INT64:
        v->i64 = *((khm_int64 *)buffer);
        CHECK(cb == sizeof(khm_int64));
        break;

    case KC_STRING:
        CHECK(SUCCEEDED(StringCchLength((wchar_t *) buffer, KCONF_MAXCCH_STRING, &cch)));
        CHECK(cb >= (++cch) * sizeof(wchar_t));
        v->str = wcsdup((wchar_t *) buffer);
        break;

    case KC_BINARY:
        CHECK(cb <= KCONF_MAXCB_STRING);
        v->bin = malloc(cb);
        v->cb_bin = cb;
        memcpy(v->bin, buffer, cb);
        break;

    default:
        CHECK(FALSE);
    }

    LEAVE(p_write_value);
    return KHM_ERROR_SUCCESS;
}


khm_int32 KHMCALLBACK p_remove_value(void * nodeHandle, const wchar_t * valuename)
{
    p_data * d = (p_data *) nodeHandle;
    CHECK(d && d->magic == MAGIC);
    ENTER(p_remove_value);

    LEAVE(p_remove_value);
    return KHM_ERROR_SUCCESS;
}

khc_provider_interface test_provider = {
    KHC_PROVIDER_V1,
    p_init,
    p_exit,

    p_open,
    p_close,
    p_remove,

    p_create,
    p_begin_enum,
    p_get_mtime,

    p_read_value,
    p_write_value,
    p_remove_value
};


static int prov_mount_test(void)
{
    khm_handle h;
    khm_handle h2;
    khm_handle h3;
    khm_int32 i32;

    /* Mounting on one store */
    IS(khc_mount_provider(NULL, L"TestProvider", KCONF_FLAG_USER,
                          &test_provider,
                          L"\\TestProvider", &h));
    CHECK(h != NULL);

    IS(khc_unmount_provider(h, KCONF_FLAG_USER));

    IS(khc_close_space(h));

    ISNT(khc_open_space(NULL, L"TestProvider", KCONF_FLAG_USER, &h));

    /* Mounting on multiple stores */
    IS(khc_mount_provider(NULL, L"TestProvider", KCONF_FLAG_USER|KCONF_FLAG_MACHINE|KCONF_FLAG_SCHEMA,
                          &test_provider,
                          L"\\TestProvider", &h));
    CHECK(h != NULL);
    CHECK(n_all_data == 3);

    IS(khc_unmount_provider(h, KCONF_FLAG_USER));

    IS(khc_open_space(NULL, L"TestProvider", KCONF_FLAG_MACHINE, &h2));
    IS(khc_close_space(h2));

    IS(khc_open_space(NULL, L"TestProvider", 0, &h2));
    IS(khc_close_space(h2));

    ISNT(khc_open_space(NULL, L"TestProvider", KCONF_FLAG_USER, &h2));

    IS(khc_unmount_provider(h, KCONF_FLAG_MACHINE|KCONF_FLAG_SCHEMA));

    IS(khc_close_space(h));

    ISNT(khc_open_space(NULL, L"TestProvider", KCONF_FLAG_USER, &h));

    ISNT(khc_open_space(NULL, L"TestProvider", 0, &h));

    /* Shouldn't be able to mount on space names with slashes in
       them. */
    CHECK(khc_mount_provider(NULL, L"TestProvider\\Foo", KCONF_FLAG_USER, &test_provider, NULL, NULL) == KHM_ERROR_INVALID_PARAM);
    CHECK(khc_mount_provider(NULL, L"TestProvider/Foo", KCONF_FLAG_USER, &test_provider, NULL, NULL) == KHM_ERROR_INVALID_PARAM);

    /* If a space is mounted and unmounted, the default configuration
       providers should be restored. */

    IS(khc_open_space(NULL, L"TestProvider", KCONF_FLAG_USER|KHM_FLAG_CREATE, &h));
    IS(khc_write_int32(h, L"TestValue", 7));
    i32 = 0; IS(khc_read_int32(h, L"TestValue", &i32));
    CHECK(i32 == 7);
    IS(khc_close_space(h));

    IS(khc_mount_provider(NULL, L"TestProvider", KCONF_FLAG_USER, &test_provider, NULL, &h));
    i32 = 0; ISNT(khc_read_int32(h, L"TestValue", &i32));
    IS(khc_unmount_provider(h, KCONF_FLAG_USER));
    i32 = 0; IS(khc_read_int32(h, L"TestValue", &i32));
    CHECK(i32 = 7);

    IS(khc_remove_space(h));
    IS(khc_close_space(h));

    /* This should also work if the child configuration space is
       mounted and dismounted while a parent configuration space is
       mounted.  The child should still revert to defaults. */

    IS(khc_open_space(NULL, L"TestProvider", KCONF_FLAG_USER|KHM_FLAG_CREATE, &h));
    IS(khc_open_space(h, L"Child", KCONF_FLAG_USER|KHM_FLAG_CREATE, &h2));
    IS(khc_write_int32(h2, L"TestValue", 8));
    i32 = 0; IS(khc_read_int32(h2, L"TestValue", &i32));
    CHECK(i32 == 8);

    IS(khc_mount_provider(NULL, L"TestProvider", KCONF_FLAG_USER, &test_provider, NULL, NULL));

    IS(khc_mount_provider(h, L"Child", KCONF_FLAG_USER, &test_provider, NULL, &h3));
    i32 = 0; ISNT(khc_read_int32(h2, L"TestValue", &i32));
    IS(khc_unmount_provider(h3, KCONF_FLAG_USER));
    i32 = 0; IS(khc_read_int32(h2, L"TestValue", &i32));
    CHECK(i32 == 8);
    IS(khc_close_space(h3));

    IS(khc_unmount_provider(h, KCONF_FLAG_USER));
    IS(khc_remove_space(h));
    IS(khc_close_space(h));
    IS(khc_close_space(h2));

    return 0;
}

static int prov_create_test(void)
{
    khm_handle h;
    khm_handle h2;

    IS(khc_mount_provider(NULL, L"TestProvider", KCONF_FLAG_USER,
                          &test_provider,
                          L"\\TestProvider", &h));
    CHECK(h != NULL);

    ISNT(khc_open_space(h, L"Foo", KCONF_FLAG_USER, &h2));
    ISNT(khc_open_space(h, L"Foo", 0, &h2));
    IS(khc_open_space(h, L"Foo", KCONF_FLAG_USER|KHM_FLAG_CREATE, &h2));

    CHECK(h2 != NULL);

    IS(khc_close_space(h2));

    /* If this causes a crash, then the provider data is inconsistent
       or the provider failed to mount. */
    CHECK(get_data_for(L"\\TestProvider", KCONF_FLAG_USER)->entry_count[CIDX_p_create] == 3);

    IS(khc_unmount_provider(h, KCONF_FLAG_USER));
    IS(khc_close_space(h));

    return 0;
}

static int prov_rd_wr_test(void)
{
    return 0;
}

static nim_test tests[] = {
    {"provmount", "Provider mount/unmount test", prov_mount_test},
    {"provcreate", "Provider create test", prov_create_test},
    {"provrdwr", "Provider read/write test", prov_rd_wr_test},
};

nim_test_suite kconf_prov_suite = {
    "KconfProv", "[KCONF] Provider Tests", NULL, NULL, ARRAYLENGTH(tests), tests
};
