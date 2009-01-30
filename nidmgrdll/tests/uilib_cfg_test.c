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

/* $Id$ */

#define NIMPRIVATE
#include "tests.h"

/* The configuration node tree used for testing is:

   root
    |
    +-- A (0)
    |
    +-- B (0)
    |   +-- B1 (0)
    |   +-- B2 (0)
    |   +-- C  (0)
    |
    ++- C (SORT_CHILDREN)
        +-- CS1 (SUBPANEL)
        +-- CS2 (SUBPANEL)
        +-- CP1 (SUBPANEL|PLURAL)
        +-- CP2 (SUBPANEL|PLURAL)
        +-- CC1 (0)
        +-- CC2 (0)
        +-- CC3 (0)
 */

BOOL config_name_is(khui_config_node node, const wchar_t * sname)
{
    wchar_t name[KHUI_MAXCCH_NAME];
    khm_size cb;
    khm_int32 rv;
    BOOL eq;

    cb = sizeof(name);
    if (KHM_FAILED(rv = khui_cfg_get_name(node, name, &cb))) {
        log("Failed to get name. Retval = 0x%x\n", rv);
        return FALSE;
    }

    eq = !wcscmp(sname, name);
    if (!eq) {
        log("Config names are not equal.  Looking for [%S]. Found [%S]\n", sname, name);
    }

    return eq;
}

int reg(void) {
    khui_config_node_reg reg;
    khui_config_node parent;

    ZeroMemory(&reg, sizeof(reg));

    reg.name = reg.short_desc = reg.long_desc = L"A";
    reg.flags = 0;
    CHECK(KHM_SUCCEEDED(khui_cfg_register(NULL, &reg)));

    reg.name = reg.short_desc = reg.long_desc = L"B";
    reg.flags = 0;
    CHECK(KHM_SUCCEEDED(khui_cfg_register(NULL, &reg)));
    
    reg.name = reg.short_desc = reg.long_desc = L"C";
    reg.flags = KHUI_CNFLAG_SORT_CHILDREN;
    CHECK(KHM_SUCCEEDED(khui_cfg_register(NULL, &reg)));

    parent = NULL;
    CHECK(KHM_SUCCEEDED(khui_cfg_open(NULL, L"B", &parent)));
    if (parent) {
        reg.name = reg.short_desc = reg.long_desc = L"B1";
        reg.flags = 0;
        CHECK(KHM_SUCCEEDED(khui_cfg_register(parent, &reg)));

        reg.name = reg.short_desc = reg.long_desc = L"B2";
        reg.flags = 0;
        CHECK(KHM_SUCCEEDED(khui_cfg_register(parent, &reg)));

        reg.name = reg.short_desc = reg.long_desc = L"C";
        reg.flags = 0;
        CHECK(KHM_SUCCEEDED(khui_cfg_register(parent, &reg)));

        khui_cfg_release(parent);
    }

    parent = NULL;
    CHECK(KHM_SUCCEEDED(khui_cfg_open(NULL, L"C", &parent)));
    if (parent) {
        /* Intentionally unsorted.  The order of registration
           shouldn't matter. */
        reg.name = reg.short_desc = reg.long_desc = L"CP2";
        reg.flags = KHUI_CNFLAG_SUBPANEL | KHUI_CNFLAG_INSTANCE;
        CHECK(KHM_SUCCEEDED(khui_cfg_register(parent, &reg)));

        reg.name = reg.short_desc = reg.long_desc = L"CS1";
        reg.flags = KHUI_CNFLAG_SUBPANEL;
        CHECK(KHM_SUCCEEDED(khui_cfg_register(parent, &reg)));

        reg.name = reg.short_desc = reg.long_desc = L"CC1";
        reg.flags = 0;
        CHECK(KHM_SUCCEEDED(khui_cfg_register(parent, &reg)));

        reg.name = reg.short_desc = reg.long_desc = L"CS2";
        reg.flags = KHUI_CNFLAG_SUBPANEL;
        CHECK(KHM_SUCCEEDED(khui_cfg_register(parent, &reg)));

        reg.name = reg.short_desc = reg.long_desc = L"CC3";
        reg.flags = 0;
        CHECK(KHM_SUCCEEDED(khui_cfg_register(parent, &reg)));

        reg.name = reg.short_desc = reg.long_desc = L"CP1";
        reg.flags = KHUI_CNFLAG_SUBPANEL | KHUI_CNFLAG_INSTANCE;
        CHECK(KHM_SUCCEEDED(khui_cfg_register(parent, &reg)));

        reg.name = reg.short_desc = reg.long_desc = L"CC2";
        reg.flags = 0;
        CHECK(KHM_SUCCEEDED(khui_cfg_register(parent, &reg)));

        khui_cfg_release(parent);
    }

    /* These ones should fail */

    reg.name = reg.short_desc = reg.long_desc = L"C";
    reg.flags = 0;
    CHECKX((khui_cfg_register(NULL, &reg) == KHM_ERROR_DUPLICATE), "Duplicate name registration test");

    reg.name = reg.short_desc = reg.long_desc = NULL;
    reg.flags = 0;
    CHECKX((khui_cfg_register(NULL, &reg) == KHM_ERROR_INVALID_PARAM), "NULL name registration test");

    reg.name = L"D"; reg.short_desc = reg.long_desc = NULL;
    reg.flags = 0;
    CHECKX((khui_cfg_register(NULL, &reg) == KHM_ERROR_INVALID_PARAM), "NULL description registration test");

    /* 256 characters + NULL terminator = 257 > KHUI_MAXCCH_NAME*/
    reg.name = reg.short_desc = reg.long_desc =
        L"0123456789ABCDEF" L"0123456789ABCDEF" L"0123456789ABCDEF" L"0123456789ABCDEF"
        L"0123456789ABCDEF" L"0123456789ABCDEF" L"0123456789ABCDEF" L"0123456789ABCDEF"
        L"0123456789ABCDEF" L"0123456789ABCDEF" L"0123456789ABCDEF" L"0123456789ABCDEF"
        L"0123456789ABCDEF" L"0123456789ABCDEF" L"0123456789ABCDEF" L"0123456789ABCDEF"
        ;
    reg.flags = 0;
    CHECKX((khui_cfg_register(NULL, &reg) == KHM_ERROR_INVALID_PARAM), "Too long name registration test");

    /* 255 + NULL == KHUI_MAXCCH_NAME, should pass */
    reg.name = reg.short_desc = reg.long_desc =
        L"0123456789ABCDEF" L"0123456789ABCDEF" L"0123456789ABCDEF" L"0123456789ABCDEF"
        L"0123456789ABCDEF" L"0123456789ABCDEF" L"0123456789ABCDEF" L"0123456789ABCDEF"
        L"0123456789ABCDEF" L"0123456789ABCDEF" L"0123456789ABCDEF" L"0123456789ABCDEF"
        L"0123456789ABCDEF" L"0123456789ABCDEF" L"0123456789ABCDEF" L"0123456789ABCDE";
    reg.flags = 0;
    CHECKX(KHM_SUCCEEDED(khui_cfg_register(NULL, &reg)), "Long name registration test");
    CHECK(KHM_SUCCEEDED(khui_cfg_open(NULL, reg.name, &parent)));
    CHECK(KHM_SUCCEEDED(khui_cfg_remove(parent)));
    khui_cfg_release(parent);
    parent = NULL;

    return 0;
}

int unreg(void) {
    khui_config_node n, c;

    n = NULL;
    CHECK(KHM_SUCCEEDED(khui_cfg_open(NULL, L"A", &n)));
    CHECK(KHM_SUCCEEDED(khui_cfg_remove(n)));
    khui_cfg_release(n);
    CHECK((khui_cfg_open(NULL, L"A", &n) == KHM_ERROR_NOT_FOUND));

    n = NULL;
    CHECK(KHM_SUCCEEDED(khui_cfg_open(NULL, L"B", &n)));
    CHECK(KHM_SUCCEEDED(khui_cfg_remove(n)));
    /* n should still be held and valid */

    c = NULL;
    CHECK(KHM_SUCCEEDED(khui_cfg_open(n, L"B1", &c)));
    CHECK(KHM_SUCCEEDED(khui_cfg_remove(c)));
    khui_cfg_release(c);
    CHECK((khui_cfg_open(n, L"B1", &c) == KHM_ERROR_NOT_FOUND));

    c = NULL;
    CHECK(KHM_SUCCEEDED(khui_cfg_open(n, L"B2", &c)));
    CHECK(KHM_SUCCEEDED(khui_cfg_remove(c)));
    khui_cfg_release(c);
    CHECK((khui_cfg_open(n, L"B2", &c) == KHM_ERROR_NOT_FOUND));

    c = NULL;
    CHECK(KHM_SUCCEEDED(khui_cfg_open(n, L"C", &c)));
    CHECK(KHM_SUCCEEDED(khui_cfg_remove(c)));
    khui_cfg_release(c);
    CHECK((khui_cfg_open(n, L"C", &c) == KHM_ERROR_NOT_FOUND));

    CHECK((khui_cfg_open(NULL, L"B", &n) == KHM_ERROR_NOT_FOUND));
    khui_cfg_release(n);

    n = NULL;
    CHECK(KHM_SUCCEEDED(khui_cfg_open(NULL, L"C", &n)));
    CHECK(KHM_SUCCEEDED(khui_cfg_remove(n)));

    c = NULL;
    CHECK(KHM_SUCCEEDED(khui_cfg_open(n, L"CS1", &c)));
    CHECK(KHM_SUCCEEDED(khui_cfg_remove(c)));
    khui_cfg_release(c);
    CHECK((khui_cfg_open(n, L"CS1", &c) == KHM_ERROR_NOT_FOUND));

    c = NULL;
    CHECK(KHM_SUCCEEDED(khui_cfg_open(n, L"CS2", &c)));
    CHECK(KHM_SUCCEEDED(khui_cfg_remove(c)));
    khui_cfg_release(c);
    CHECK((khui_cfg_open(n, L"CS2", &c) == KHM_ERROR_NOT_FOUND));

    c = NULL;
    CHECK(KHM_SUCCEEDED(khui_cfg_open(n, L"CP1", &c)));
    CHECK(KHM_SUCCEEDED(khui_cfg_remove(c)));
    khui_cfg_release(c);
    CHECK((khui_cfg_open(n, L"CP1", &c) == KHM_ERROR_NOT_FOUND));

    c = NULL;
    CHECK(KHM_SUCCEEDED(khui_cfg_open(n, L"CP2", &c)));
    CHECK(KHM_SUCCEEDED(khui_cfg_remove(c)));
    khui_cfg_release(c);
    CHECK((khui_cfg_open(n, L"CP2", &c) == KHM_ERROR_NOT_FOUND));

    c = NULL;
    CHECK(KHM_SUCCEEDED(khui_cfg_open(n, L"CC1", &c)));
    CHECK(KHM_SUCCEEDED(khui_cfg_remove(c)));
    khui_cfg_release(c);
    CHECK((khui_cfg_open(n, L"CC1", &c) == KHM_ERROR_NOT_FOUND));

    c = NULL;
    CHECK(KHM_SUCCEEDED(khui_cfg_open(n, L"CC2", &c)));
    CHECK(KHM_SUCCEEDED(khui_cfg_remove(c)));
    khui_cfg_release(c);
    CHECK((khui_cfg_open(n, L"CC2", &c) == KHM_ERROR_NOT_FOUND));

    c = NULL;
    CHECK(KHM_SUCCEEDED(khui_cfg_open(n, L"CC3", &c)));
    CHECK(KHM_SUCCEEDED(khui_cfg_remove(c)));
    khui_cfg_release(c);
    CHECK((khui_cfg_open(n, L"CC3", &c) == KHM_ERROR_NOT_FOUND));

    CHECK((khui_cfg_open(NULL, L"C", &n) == KHM_ERROR_NOT_FOUND));
    khui_cfg_release(n);

    return 0;
}

int ienum(void) {
    khui_config_node parent;
    khui_config_node parent2;
    khui_config_node child1;
    khui_config_node subpanel;
    khui_config_node child2;

    CHECK(KHM_SUCCEEDED(khui_cfg_open(NULL, L"A", &parent)));
    CHECK(khui_cfg_get_first_child(parent, &child1) == KHM_ERROR_NOT_FOUND);
    CHECK(khui_cfg_get_first_subpanel(parent, &subpanel) == KHM_ERROR_NOT_FOUND);
    khui_cfg_release(parent);

    CHECK(KHM_SUCCEEDED(khui_cfg_open(NULL, L"B", &parent)));
    CHECK(KHM_SUCCEEDED(khui_cfg_get_first_child(parent, &child1)));
    CHECK(config_name_is(child1, L"C"));
    CHECK(KHM_SUCCEEDED(khui_cfg_get_next(child1, &child2)));
    CHECK(config_name_is(child2, L"B2"));
    khui_cfg_release(child1);

    CHECK(KHM_SUCCEEDED(khui_cfg_get_parent(child1, &parent2)));
    CHECK(parent2 == parent);
    khui_cfg_release(parent2);

    CHECK(KHM_SUCCEEDED(khui_cfg_get_next(child2, &child1)));
    CHECK(config_name_is(child1, L"B1"));
    khui_cfg_release(child2);
    CHECK(khui_cfg_get_next(child1, &child2) == KHM_ERROR_NOT_FOUND);
    khui_cfg_release(child1);
    khui_cfg_release(parent);

    child1=child2=parent = NULL;

    CHECK(KHM_SUCCEEDED(khui_cfg_open(NULL, L"C", &parent)));
    CHECK(KHM_SUCCEEDED(khui_cfg_get_first_child(parent, &child1)));
    CHECK(config_name_is(child1, L"CC2"));
    CHECK(KHM_SUCCEEDED(khui_cfg_get_next_release(&child1)));
    CHECK(config_name_is(child1, L"CC3"));

    CHECK(KHM_SUCCEEDED(khui_cfg_get_parent(child1, &parent2)));
    CHECK(parent2 == parent);
    khui_cfg_release(parent2);

    CHECK(KHM_SUCCEEDED(khui_cfg_get_next_release(&child1)));
    CHECK(config_name_is(child1, L"CC1"));

    CHECK(KHM_SUCCEEDED(khui_cfg_get_parent(child1, &parent2)));
    CHECK(parent2 == parent);
    khui_cfg_release(parent2);

    CHECK(khui_cfg_get_next_release(&child1) == KHM_ERROR_NOT_FOUND);
    CHECK(child1 == NULL);
    child1 = NULL;
    CHECK(KHM_SUCCEEDED(khui_cfg_get_first_subpanel(parent, &subpanel)));
    CHECK(config_name_is(subpanel, L"CP1"));

    CHECK(KHM_SUCCEEDED(khui_cfg_get_parent(subpanel, &parent2)));
    CHECK(parent2 == parent);
    khui_cfg_release(parent2);

    CHECK(KHM_SUCCEEDED(khui_cfg_get_next_release(&subpanel)));
    CHECK(config_name_is(subpanel, L"CS2"));
    CHECK(KHM_SUCCEEDED(khui_cfg_get_next_release(&subpanel)));
    CHECK(config_name_is(subpanel, L"CS1"));

    CHECK(KHM_SUCCEEDED(khui_cfg_get_parent(subpanel, &parent2)));
    CHECK(parent2 == parent);
    khui_cfg_release(parent2);

    CHECK(KHM_SUCCEEDED(khui_cfg_get_next_release(&subpanel)));
    CHECK(config_name_is(subpanel, L"CP2"));
    CHECK(khui_cfg_get_next_release(&subpanel) == KHM_ERROR_NOT_FOUND);
    CHECK(subpanel == NULL);
    subpanel = NULL;
    khui_cfg_release(parent);
    parent = NULL;

    return 0;
}

void
map_children(khui_config_node node,
             void (*pf)(khui_config_node))
{
    khui_config_node child = NULL;

    if (KHM_FAILED(khui_cfg_get_first_child(node, &child)))
        return;
    do {
        (*pf)(child);
        map_children(child, pf);
    } while(KHM_SUCCEEDED(khui_cfg_get_next_release(&child)));
}

void
map_subpanels(khui_config_node node,
              void (*pf)(khui_config_node, khui_config_node))
{
    khui_config_node subpanel = NULL;
    khui_config_node child = NULL;

    if (KHM_SUCCEEDED(khui_cfg_get_first_subpanel(node, &subpanel))) {
        do {
            if (khui_cfg_get_flags(subpanel) & KHUI_CNFLAG_INSTANCE) {
                if (KHM_SUCCEEDED(khui_cfg_get_first_child(node, &child))) {
                    do {
                        (*pf)(subpanel, child);
                    } while (KHM_SUCCEEDED(khui_cfg_get_next_release(&child)));
                }
            } else {
                (*pf)(subpanel, node);
            }
        } while(KHM_SUCCEEDED(khui_cfg_get_next_release(&subpanel)));
    }

    if (KHM_SUCCEEDED(khui_cfg_get_first_child(node, &child))) {
        do {
            map_subpanels(child, pf);
        } while (KHM_SUCCEEDED(khui_cfg_get_next_release(&child)));
    }
}

int test_count;
khm_boolean read_test;

void gs_test_node(khui_config_node node)
{
    wchar_t name[KHUI_MAXCCH_NAME];
    khm_size cb;
    HWND hwnd;
    LPARAM lparam;

    cb = sizeof(name);
    CHECK(KHM_SUCCEEDED(khui_cfg_get_name(node, name, &cb)));
    log("Looking at node [%S]\n", name);

    hwnd = (HWND) node;
    lparam = (LPARAM) node;

    if (!read_test) {
        khui_cfg_set_hwnd(node, hwnd);
        khui_cfg_set_param(node, lparam);
    }

    CHECK(khui_cfg_get_hwnd(node) == hwnd);
    CHECK(khui_cfg_get_param(node) == lparam);

    test_count++;
}

void gsi_test_node(khui_config_node node, khui_config_node ctx)
{
    wchar_t name[KHUI_MAXCCH_NAME];
    khm_size cb;
    HWND hwnd;
    LPARAM lparam;

    cb = sizeof(name);
    CHECK(KHM_SUCCEEDED(khui_cfg_get_name(node, name, &cb)));
    log("Looking at node [%S]\n", name);

    hwnd = (HWND) ((khm_ssize)ctx ^ ((khm_ssize)node >> 2));
    lparam = (LPARAM) ((khm_ssize)ctx ^ ((khm_ssize)node >> 2));

    if (!read_test) {
        khui_cfg_set_hwnd_inst(node, ctx, hwnd);
        khui_cfg_set_param_inst(node, ctx, lparam);
    }

    CHECK(khui_cfg_get_hwnd_inst(node, ctx) == hwnd);
    CHECK(khui_cfg_get_param_inst(node, ctx) == lparam);

    test_count++;
}

int gs_test(void) {

    test_count = 0;
    read_test = FALSE;
    map_children(NULL, gs_test_node);
    CHECK(test_count == 9);
    read_test = TRUE;
    map_children(NULL, gs_test_node);

    return 0;
}

int gsi_test(void) {

    test_count = 0;
    read_test = FALSE;
    map_subpanels(NULL, gsi_test_node);
    CHECK(test_count == 8);
    read_test = TRUE;
    map_subpanels(NULL, gsi_test_node);

    return 0;
}

int gsf_test(void) {
    return 0;
}

int _plural;
int _single;

khm_int32 _pl_before[] = {0,0,0,0,KHUI_CNFLAG_MODIFIED,0};
khm_int32 _pl_after[]  = {0,KHUI_CNFLAG_MODIFIED,0,0,KHUI_CNFLAG_MODIFIED|KHUI_CNFLAG_APPLIED,KHUI_CNFLAG_APPLIED};

void gsfi_test_node(khui_config_node node, khui_config_node ctx)
{
    wchar_t name[KHUI_MAXCCH_NAME];
    khm_size cb;
    khui_config_init_data d;

    cb = sizeof(name);
    CHECK(KHM_SUCCEEDED(khui_cfg_get_name(node, name, &cb)));
    log("Looking at node [%S]\n", name);

    if (khui_cfg_get_flags(node) & KHUI_CNFLAG_INSTANCE) {
        if (_plural >= ARRAYLENGTH(_pl_before)) {
            CHECKX(FALSE, "_plural out of bounds\n");
            return;
        }

        CHECK((khui_cfg_get_flags(ctx) & KHUI_CNFLAGMASK_DYNAMIC) == _pl_before[_plural]);
        CHECK(KHM_SUCCEEDED(khui_cfg_get_parent(node, &d.ref_node)));
        d.ctx_node = ctx;
        d.this_node = node;
        khui_cfg_set_flags_inst(&d,
                                ((_plural == 1)?KHUI_CNFLAG_MODIFIED: ((_plural > 3)?KHUI_CNFLAG_APPLIED:0)),
                                KHUI_CNFLAG_APPLIED|KHUI_CNFLAG_MODIFIED);
        CHECK((khui_cfg_get_flags(ctx) & KHUI_CNFLAGMASK_DYNAMIC) == _pl_after[_plural]);
        khui_cfg_release(d.ref_node);
        _plural++;
    } else {
        d.ref_node = d.ctx_node = ctx;
        d.this_node = node;
        if (_single == 0)
            khui_cfg_set_flags_inst(&d, KHUI_CNFLAG_MODIFIED, KHUI_CNFLAG_MODIFIED);
        else {
            khui_cfg_set_flags_inst(&d, KHUI_CNFLAG_APPLIED, KHUI_CNFLAG_APPLIED);
            CHECK((khui_cfg_get_flags(ctx) & KHUI_CNFLAGMASK_DYNAMIC) == (KHUI_CNFLAG_APPLIED|KHUI_CNFLAG_MODIFIED));
        }
        _single++;
    }

    test_count++;
}

int gsfi_test(void) {

    test_count = 0;
    _single = 0;
    _plural = 0;
    map_subpanels(NULL, gsfi_test_node);
    CHECK(test_count == 8);

    return 0;
}

static nim_test tests[] = {
    {"REG", "khui_cfg_register_node()", reg},
    {"ENUM", "khui_cfg_get_first_child(), khui_cfg_get_first_subpanel(), etc.", ienum},
    {"GS", "khui_cfg_{get,set}_{hwnd,param,data}()", gs_test},
    {"GSI", "khui_cfg_{get,set}_{hwnd,param}_inst()", gsi_test},
    {"GSF", "khui_cfg_{get,set}_flags()", gsf_test},
    {"GSFI","khui_cfg_{get,set}_flags_inst()", gsfi_test},
    {"UNREG", "khui_cfg_remove()", unreg}
};

nim_test_suite uilib_cfg_suite = {
    "UILibCfg", "[uilib] Configuration UI tests", NULL, NULL, ARRAYLENGTH(tests), tests
};
