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

#include "tests.h"
#include <stdlib.h>
#include <wchar.h>
#include <strsafe.h>

struct conf_record {
    khm_int32 type;
    const wchar_t * name;
    const void * data;
    size_t cb_data;
};

const khm_int32 i32_10 = 10;
const khm_int32 i32_11 = 11;
const khm_int32 i32_20 = 20;
const khm_int32 i32_21 = 21;
const wchar_t   str_1[] = L"Foo string";
const wchar_t   str_2[] = L"Different string this time";
const char      bin_1[] = "Some string that will be used as a binary blob";

struct conf_record simple[] = {
    { KC_SPACE, L"Foo", NULL, 0 },
    { KC_INT32, L"IntVal", &i32_10, sizeof(i32_10) },
    { KC_ENDSPACE, NULL, NULL, 0}
};

struct conf_record with_subspace[] = {
    { KC_SPACE, L"Foo", NULL, 0 },
    { KC_INT32, L"IntVal", &i32_10, sizeof(i32_10) },
    { KC_SPACE, L"Bar", NULL, 0 },
    { KC_INT32, L"Some other int val", &i32_11, sizeof(i32_11)},
    { KC_STRING, L"sTRING val", str_1, sizeof(str_1)},
    { KC_ENDSPACE, NULL, NULL, 0},
    { KC_ENDSPACE, NULL, NULL, 0}
};

struct conf_record with_subsubspace[] = {
    { KC_SPACE, L"Foo", NULL, 0 },
    { KC_INT32, L"IntVal", &i32_10, sizeof(i32_10) },
    { KC_SPACE, L"Bar", NULL, 0 },
    { KC_INT32, L"Some other int val", &i32_11, sizeof(i32_11)},
    { KC_STRING, L"sTRING val", str_1, sizeof(str_1)},
    { KC_SPACE, L"Baz", NULL, 0 },
    { KC_INT32, L"Yet another int val", &i32_11, sizeof(i32_11)},
    { KC_BINARY, L"Binary val", bin_1, sizeof(str_1)},
    { KC_ENDSPACE, NULL, NULL, 0},
    { KC_ENDSPACE, NULL, NULL, 0},
    { KC_ENDSPACE, NULL, NULL, 0}
};

struct conf_record with_subsubspace_m1[] = {
    { KC_SPACE, L"Foo", NULL, 0 },
    { KC_INT32, L"IntVal", &i32_20, sizeof(i32_20) },

    { KC_SPACE, L"Bar", NULL, 0 },
    { KC_INT32, L"Some other int val", &i32_21, sizeof(i32_21)},
    { KC_STRING, L"sTRING val", str_1, sizeof(str_1)},

    { KC_SPACE, L"Baz", NULL, 0 },
    { KC_INT32, L"Yet another int val", &i32_11, sizeof(i32_11)},
    { KC_BINARY, L"Binary val", bin_1, sizeof(str_1)},

    { KC_ENDSPACE, NULL, NULL, 0},
    { KC_ENDSPACE, NULL, NULL, 0},
    { KC_ENDSPACE, NULL, NULL, 0}
};

struct conf_record with_subsubspace_m2[] = {
    { KC_SPACE, L"Foo", NULL, 0 },
    { KC_INT32, L"IntVal", &i32_20, sizeof(i32_20) },

    { KC_SPACE, L"Bar", NULL, 0 },
    { KC_INT32, L"Some other int val", &i32_21, sizeof(i32_21)},
    { KC_STRING, L"sTRING val", str_1, sizeof(str_1)},

    { KC_SPACE, L"Baz", NULL, 0 },
    { KC_INT32, L"Yet another int val", &i32_11, sizeof(i32_11)},
    { KC_BINARY, L"Binary val", bin_1, sizeof(str_1)},

    { KC_INT32, L"New integer value", &i32_20, sizeof(i32_20)},
    { KC_STRING, L"New string value", str_2, sizeof(str_2)},

    { KC_ENDSPACE, NULL, NULL, 0},
    { KC_ENDSPACE, NULL, NULL, 0},
    { KC_ENDSPACE, NULL, NULL, 0}
};

struct conf_record with_subsubspace_m3[] = {
    { KC_SPACE, L"Foo", NULL, 0 },
    { KC_INT32, L"IntVal", &i32_20, sizeof(i32_20) },

    { KC_SPACE, L"Bar", NULL, 0 },
    { KC_INT32, L"Some other int val", &i32_21, sizeof(i32_21)},
    { KC_STRING, L"sTRING val", str_1, sizeof(str_1)},

    { KC_SPACE, L"Baz", NULL, 0 },

    { KC_INT32, L"New integer value", &i32_20, sizeof(i32_20)},
    { KC_STRING, L"New string value", str_2, sizeof(str_2)},

    { KC_ENDSPACE, NULL, NULL, 0},
    { KC_ENDSPACE, NULL, NULL, 0},
    { KC_ENDSPACE, NULL, NULL, 0}
};

struct conf_record with_subsubspace_m4[] = {
    { KC_SPACE, L"Foo", NULL, 0 },
    { KC_INT32, L"IntVal", &i32_20, sizeof(i32_20) },

    { KC_SPACE, L"Bar", NULL, 0 },
    { KC_INT32, L"Some other int val", &i32_21, sizeof(i32_21)},
    { KC_STRING, L"sTRING val", str_1, sizeof(str_1)},

    { KC_ENDSPACE, NULL, NULL, 0},
    { KC_ENDSPACE, NULL, NULL, 0}
};

static void make_space(khm_handle h_mem, struct conf_record * rec, int n)
{
    int i;

    for (i=0; i < n; i++) {
        IS(khc_memory_store_add(h_mem, rec[i].name, rec[i].type, rec[i].data, rec[i].cb_data));
    }
}

struct conf_checker {
    struct conf_record * rec;
    int i;
    int n;
};

static void KHMCALLBACK checker_cb(khm_int32 type, const wchar_t * name,
                                   const void * data, khm_size cb, void * ctx)
{
    struct conf_checker * chk = (struct conf_checker *) ctx;
    int i = chk->i;

    CHECK(chk != NULL);
    CHECK(chk->i < chk->n);

    if (chk->i >= chk->n)
        return;

    if (type == KC_MTIME) {
        return;
    }

    CHECK(chk->rec[i].type == type);
    CHECK((chk->rec[i].name == NULL && name == NULL) ||
          (chk->rec[i].name != NULL && name != NULL && !wcscmp(chk->rec[i].name, name)));
    CHECK(chk->rec[i].cb_data == cb);
    if (chk->rec[i].cb_data > 0) {
        CHECK(!memcmp(chk->rec[i].data, data, cb));
    }

    chk->i++;
}

static void check_space(khm_handle h_mem, struct conf_record * rec, int n)
{
    struct conf_checker chk;

    chk.rec = rec;
    chk.n = n;
    chk.i = 0;

    IS(khc_memory_store_enum(h_mem, checker_cb, &chk));
}

static int mem_create(void)
{
    khm_handle h_mem = NULL;

    IS(khc_memory_store_create(&h_mem));

    IS(khc_memory_store_release(h_mem));

    h_mem = NULL;

    IS(khc_memory_store_create(&h_mem));

    make_space(h_mem, simple, ARRAYLENGTH(simple));

    IS(khc_memory_store_release(h_mem));

    return 0;
}

static int mem_enum(void)
{
    khm_handle h_mem = NULL;

    IS(khc_memory_store_create(&h_mem));
    make_space(h_mem, simple, ARRAYLENGTH(simple));
    check_space(h_mem, simple, ARRAYLENGTH(simple));
    IS(khc_memory_store_release(h_mem));

    IS(khc_memory_store_create(&h_mem));
    make_space(h_mem, with_subspace, ARRAYLENGTH(with_subspace));
    check_space(h_mem, with_subspace, ARRAYLENGTH(with_subspace));
    IS(khc_memory_store_release(h_mem));

    IS(khc_memory_store_create(&h_mem));
    make_space(h_mem, with_subsubspace, ARRAYLENGTH(with_subsubspace));
    check_space(h_mem, with_subsubspace, ARRAYLENGTH(with_subsubspace));
    IS(khc_memory_store_release(h_mem));

    return 0;
}

static int mem_mount(void)
{
    khm_handle h_mem = NULL;
    khm_handle h_csp = NULL;
    khm_handle h_csp2 = NULL;
    khm_int32 i32;

    IS(khc_memory_store_create(&h_mem));
    make_space(h_mem, simple, ARRAYLENGTH(simple));
    ISNT(khc_memory_store_mount(NULL, KCONF_FLAG_SCHEMA, h_mem, &h_csp));
    ISNT(khc_memory_store_mount(NULL, KCONF_FLAG_SCHEMA|KCONF_FLAG_MACHINE, h_mem, &h_csp));
    ISNT(khc_memory_store_mount(NULL, KCONF_FLAG_USER|KCONF_FLAG_MACHINE, h_mem, &h_csp));
    IS(khc_memory_store_mount(NULL, KCONF_FLAG_USER, h_mem, &h_csp));
    ISNT(khc_memory_store_mount(NULL, KCONF_FLAG_USER, h_mem, &h_csp2));

    i32 = 0;
    IS(khc_read_int32(h_csp, L"IntVal", &i32));
    CHECK(i32 == 10);

    IS(khc_memory_store_release(h_mem));

    ISNT(khc_read_int32(h_csp, L"IntVal", &i32));
    ISNT(khc_read_int32(NULL, L"Foo\\Bar\\Some other int val", &i32));
    ISNT(khc_read_int32(h_csp, L"Bar\\Baz\\Yet another int val", &i32));

    IS(khc_close_space(h_csp));

    IS(khc_memory_store_create(&h_mem));
    make_space(h_mem, with_subsubspace, ARRAYLENGTH(with_subsubspace));
    IS(khc_memory_store_mount(NULL, KCONF_FLAG_USER, h_mem, &h_csp));
    ISNT(khc_memory_store_mount(NULL, KCONF_FLAG_USER, h_mem, &h_csp2));

    i32 = 0;
    IS(khc_read_int32(h_csp, L"IntVal", &i32));
    CHECK(i32 == 10);

    i32 = 0;
    IS(khc_read_int32(NULL, L"Foo\\Bar\\Some other int val", &i32));
    CHECK(i32 == 11);

    i32 = 0;
    IS(khc_read_int32(h_csp, L"Bar\\Baz\\Yet another int val", &i32));
    CHECK(i32 == 11);

    IS(khc_memory_store_release(h_mem));

    ISNT(khc_read_int32(h_csp, L"IntVal", &i32));
    ISNT(khc_read_int32(NULL, L"Foo\\Bar\\Some other int val", &i32));
    ISNT(khc_read_int32(h_csp, L"Bar\\Baz\\Yet another int val", &i32));

    IS(khc_close_space(h_csp));

    return 0;
}

static int mem_rw(void)
{
    khm_handle h_mem = NULL;
    khm_handle h_csp = NULL;
    khm_handle h_csp2 = NULL;

    IS(khc_memory_store_create(&h_mem));
    make_space(h_mem, with_subsubspace, ARRAYLENGTH(with_subsubspace));
    IS(khc_memory_store_mount(NULL, KCONF_FLAG_USER, h_mem, &h_csp));

    IS(khc_write_int32(h_csp, L"IntVal", i32_20));
    IS(khc_write_int32(h_csp, L"Bar\\Some other int val", i32_21));

    check_space(h_mem, with_subsubspace_m1, ARRAYLENGTH(with_subsubspace_m1));

    IS(khc_write_int32(h_csp, L"Bar\\Baz\\New integer value", i32_20));
    IS(khc_write_string(NULL, L"Foo\\Bar\\Baz\\New string value", str_2));

    check_space(h_mem, with_subsubspace_m2, ARRAYLENGTH(with_subsubspace_m2));

    IS(khc_remove_value(h_csp, L"Bar\\Baz\\Yet another int val", KCONF_FLAG_USER));
    IS(khc_remove_value(h_csp, L"Bar\\Baz\\Binary val", KCONF_FLAG_USER));

    check_space(h_mem, with_subsubspace_m3, ARRAYLENGTH(with_subsubspace_m3));

    IS(khc_open_space(h_csp, L"Bar\\Baz", 0, &h_csp2));
    IS(khc_remove_space(h_csp2));

    check_space(h_mem, with_subsubspace_m4, ARRAYLENGTH(with_subsubspace_m4));

    IS(khc_memory_store_release(h_mem));
    IS(khc_close_space(h_csp));

    return 0;
}

typedef struct notify_test_data {
    int magic;
#define NOTIFY_TEST_DATA_MAGIC 0x1a2b3c4d
    int inits;
    int exits;
    int opens;
    int closes;
    int modifys;                /* yes. it's not 'modifies' */
} notify_test_data;

static void KHMCALLBACK n_init(void * ctx, khm_handle s)
{
    struct notify_test_data * d = (notify_test_data *) ctx;

    CHECK(d && d->magic == NOTIFY_TEST_DATA_MAGIC);

    d->inits++;
}

static void KHMCALLBACK n_exit(void * ctx, khm_handle s)
{
    struct notify_test_data * d = (notify_test_data *) ctx;

    CHECK(d && d->magic == NOTIFY_TEST_DATA_MAGIC);

    d->exits++;
}

static void KHMCALLBACK n_open(void * ctx, khm_handle s)
{
    struct notify_test_data * d = (notify_test_data *) ctx;

    CHECK(d && d->magic == NOTIFY_TEST_DATA_MAGIC);

    d->opens++;
}

static void KHMCALLBACK n_close(void * ctx, khm_handle s)
{
    struct notify_test_data * d = (notify_test_data *) ctx;

    CHECK(d && d->magic == NOTIFY_TEST_DATA_MAGIC);

    d->closes++;
}

static void KHMCALLBACK n_modify(void * ctx, khm_handle s)
{
    struct notify_test_data * d = (notify_test_data *) ctx;

    CHECK(d && d->magic == NOTIFY_TEST_DATA_MAGIC);

    d->modifys++;
}

static int mem_notify(void)
{
    khc_memory_store_notify notify = {
        n_init,
        n_exit,
        n_open,
        n_close,
        n_modify
    };

    khm_handle h_mem = NULL;
    khm_handle h_csp = NULL;
    khm_handle h_csp2 = NULL;

    struct notify_test_data td;

    /* Basic open */
    IS(khc_memory_store_create(&h_mem));
    ISNT(khc_memory_store_set_notify_interface(h_mem, &notify, &td));
    make_space(h_mem, with_subsubspace, ARRAYLENGTH(with_subsubspace));
    memset(&td, 0, sizeof(td));
    td.magic = NOTIFY_TEST_DATA_MAGIC;
    IS(khc_memory_store_set_notify_interface(h_mem, &notify, &td));
    IS(khc_memory_store_mount(NULL, KCONF_FLAG_USER, h_mem, &h_csp));
    IS(khc_memory_store_release(h_mem));
    IS(khc_close_space(h_csp));

    log(KHERR_DEBUG_1, "init = %d, exit = %d, open = %d, close =%d, modify = %d\n",
        td.inits, td.exits, td.opens, td.closes, td.modifys);
    CHECK(td.inits == 1 && td.exits == 1 && td.opens == 1 && td.closes == 0 && td.modifys == 0);

    IS(khc_memory_store_create(&h_mem));
    make_space(h_mem, with_subsubspace, ARRAYLENGTH(with_subsubspace));
    memset(&td, 0, sizeof(td));
    td.magic = NOTIFY_TEST_DATA_MAGIC;
    IS(khc_memory_store_set_notify_interface(h_mem, &notify, &td));
    IS(khc_memory_store_mount(NULL, KCONF_FLAG_USER, h_mem, NULL));

    log(KHERR_DEBUG_1, "init = %d, exit = %d, open = %d, close =%d, modify = %d\n",
        td.inits, td.exits, td.opens, td.closes, td.modifys);
    CHECK(td.inits == 1 && td.exits == 0 && td.opens == 0 && td.closes == 0 && td.modifys == 0);

    IS(khc_open_space(NULL, L"Foo", KCONF_FLAG_USER, &h_csp));
    log(KHERR_DEBUG_1, "init = %d, exit = %d, open = %d, close =%d, modify = %d\n",
        td.inits, td.exits, td.opens, td.closes, td.modifys);
    CHECK(td.inits == 1 && td.exits == 0 && td.opens == 1 && td.closes == 0 && td.modifys == 0);

    IS(khc_close_space(h_csp));
    log(KHERR_DEBUG_1, "init = %d, exit = %d, open = %d, close =%d, modify = %d\n",
        td.inits, td.exits, td.opens, td.closes, td.modifys);
    CHECK(td.inits == 1 && td.exits == 0 && td.opens == 1 && td.closes == 1 && td.modifys == 0);

    IS(khc_open_space(NULL, L"Foo\\Bar", KCONF_FLAG_USER, &h_csp));
    log(KHERR_DEBUG_1, "init = %d, exit = %d, open = %d, close =%d, modify = %d\n",
        td.inits, td.exits, td.opens, td.closes, td.modifys);
    CHECK(td.inits == 2 && td.exits == 0 && td.opens == 2 && td.closes == 1 && td.modifys == 0);

    IS(khc_close_space(h_csp));
    log(KHERR_DEBUG_1, "init = %d, exit = %d, open = %d, close =%d, modify = %d\n",
        td.inits, td.exits, td.opens, td.closes, td.modifys);
    CHECK(td.inits == 2 && td.exits == 0 && td.opens == 2 && td.closes == 2 && td.modifys == 0);

    IS(khc_open_space(NULL, L"Foo\\Bar\\Baz", KCONF_FLAG_USER, &h_csp));
    log(KHERR_DEBUG_1, "init = %d, exit = %d, open = %d, close =%d, modify = %d\n",
        td.inits, td.exits, td.opens, td.closes, td.modifys);
    CHECK(td.inits == 3 && td.exits == 0 && td.opens == 3 && td.closes == 2 && td.modifys == 0);

    IS(khc_open_space(NULL, L"Foo\\Bar\\Boo", KCONF_FLAG_USER|KHM_FLAG_CREATE, &h_csp2));
    log(KHERR_DEBUG_1, "init = %d, exit = %d, open = %d, close =%d, modify = %d\n",
        td.inits, td.exits, td.opens, td.closes, td.modifys);
    CHECK(td.inits == 4 && td.exits == 0 && td.opens == 4 && td.closes == 2 && td.modifys == 1);

    IS(khc_close_space(h_csp2));
    log(KHERR_DEBUG_1, "init = %d, exit = %d, open = %d, close =%d, modify = %d\n",
        td.inits, td.exits, td.opens, td.closes, td.modifys);
    CHECK(td.inits == 4 && td.exits == 0 && td.opens == 4 && td.closes == 3 && td.modifys == 1);

    IS(khc_open_space(NULL, L"Foo\\Bar", KCONF_FLAG_USER, &h_csp2));
    log(KHERR_DEBUG_1, "init = %d, exit = %d, open = %d, close =%d, modify = %d\n",
        td.inits, td.exits, td.opens, td.closes, td.modifys);
    CHECK(td.inits == 4 && td.exits == 0 && td.opens == 5 && td.closes == 3 && td.modifys == 1);

    IS(khc_write_int32(h_csp2, L"Blah", 10));
    log(KHERR_DEBUG_1, "init = %d, exit = %d, open = %d, close =%d, modify = %d\n",
        td.inits, td.exits, td.opens, td.closes, td.modifys);
    CHECK(td.inits == 4 && td.exits == 0 && td.opens == 5 && td.closes == 3 && td.modifys == 2);

    IS(khc_write_int32(h_csp, L"BlahBlah", 10));
    log(KHERR_DEBUG_1, "init = %d, exit = %d, open = %d, close =%d, modify = %d\n",
        td.inits, td.exits, td.opens, td.closes, td.modifys);
    CHECK(td.inits == 4 && td.exits == 0 && td.opens == 5 && td.closes == 3 && td.modifys == 3);

    IS(khc_remove_value(h_csp2, L"Blah", KCONF_FLAG_USER));
    log(KHERR_DEBUG_1, "init = %d, exit = %d, open = %d, close =%d, modify = %d\n",
        td.inits, td.exits, td.opens, td.closes, td.modifys);
    CHECK(td.inits == 4 && td.exits == 0 && td.opens == 5 && td.closes == 3 && td.modifys == 4);

    ISNT(khc_remove_value(h_csp2, L"Blah", KCONF_FLAG_USER));
    log(KHERR_DEBUG_1, "init = %d, exit = %d, open = %d, close =%d, modify = %d\n",
        td.inits, td.exits, td.opens, td.closes, td.modifys);
    CHECK(td.inits == 4 && td.exits == 0 && td.opens == 5 && td.closes == 3 && td.modifys == 4);

    IS(khc_remove_value(h_csp, L"BlahBlah", KCONF_FLAG_USER));
    log(KHERR_DEBUG_1, "init = %d, exit = %d, open = %d, close =%d, modify = %d\n",
        td.inits, td.exits, td.opens, td.closes, td.modifys);
    CHECK(td.inits == 4 && td.exits == 0 && td.opens == 5 && td.closes == 3 && td.modifys == 5);

    IS(khc_close_space(h_csp));
    IS(khc_close_space(h_csp2));
    log(KHERR_DEBUG_1, "init = %d, exit = %d, open = %d, close =%d, modify = %d\n",
        td.inits, td.exits, td.opens, td.closes, td.modifys);
    CHECK(td.inits == 4 && td.exits == 0 && td.opens == 5 && td.closes == 5 && td.modifys == 5);

    IS(khc_memory_store_unmount(h_mem));
    log(KHERR_DEBUG_1, "init = %d, exit = %d, open = %d, close =%d, modify = %d\n",
        td.inits, td.exits, td.opens, td.closes, td.modifys);
    CHECK(td.inits == 4 && td.exits == 4 && td.opens == 5 && td.closes == 5 && td.modifys == 5);

    IS(khc_memory_store_release(h_mem));

    return 0;
}

static nim_test tests[] = {
    {"memcreate", "Create test", mem_create},
    {"memenum", "Enum test", mem_enum},
    {"memmount", "Mount test", mem_mount},
    {"memrw", "Read/write test", mem_rw},
    {"memnotify", "Notify test", mem_notify},
};

nim_test_suite kconf_mem_suite = {
    "KconfMem", "[KCONF] Memory Provider Tests", NULL, NULL, ARRAYLENGTH(tests), tests
};
