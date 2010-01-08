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

#include <shlwapi.h>
#include "kconfiginternal.h"
#include <assert.h>

typedef struct reg_node {
    khm_int32  magic;
    HKEY       h_key;
    HKEY       h_root;
    int        open_count;
    wchar_t *  regpath;
    khm_handle h;
} reg_node;

#define REG_NODE_MAGIC 0xadb3e53d
#define is_reg_node(x) ((x) && ((reg_node *) (x))->magic == REG_NODE_MAGIC)

#define KCONF_SPACE_FLAG_NO_HKCU  0x00000200
#define KCONF_SPACE_FLAG_NO_HKLM  0x00000400

#define CONFIG_REGPATHW L"Software\\MIT\\NetIDMgr"

static const DWORD kc_type_to_reg_type[] = {
    0,
    0,
    0,
    REG_DWORD,
    REG_QWORD,
    REG_SZ,
    REG_BINARY
};

static HKEY
open_key(reg_node * node, khm_boolean create)
{
    int nflags = 0;
    DWORD disp;

    if (node->h_key != NULL)
        return node->h_key;

    if (RegOpenKeyEx(node->h_root, node->regpath, 0, 
                     KEY_READ | KEY_WRITE, &node->h_key) == ERROR_ACCESS_DENIED &&

        RegOpenKeyEx(node->h_root, node->regpath, 0,
                     KEY_READ, &node->h_key) == ERROR_SUCCESS) {

        nflags = KHM_PERM_READ;
    }

    if(node->h_key == NULL && create) {

        RegCreateKeyEx(node->h_root,
                       node->regpath,
                       0, NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE,
                       NULL, &node->h_key, &disp);
    }

    return node->h_key;
}

static void
close_key(reg_node * node)
{
    if (node->h_key) {
        RegCloseKey(node->h_key);
        node->h_key = NULL;
    }
}

khm_int32 KHMCALLBACK
reg_init(khm_handle sp_handle,
         const wchar_t * path, khm_int32 flags,
         void * context, void ** r_nodeHandle)
{
    wchar_t regpath[8192];      /* Each registry API call can only
                                   address 32 levels of keys, with
                                   each key being 255 characters
                                   max. */
    reg_node * r_node;
    reg_node node;

    StringCbCopy(regpath, sizeof(regpath), CONFIG_REGPATHW);
    StringCbCat(regpath, sizeof(regpath), path);

    memset(&node, 0, sizeof(node));
    node.magic = REG_NODE_MAGIC;
    node.h_key = NULL;
    node.h_root = ((flags & KCONF_FLAG_USER)? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE);
    node.regpath = regpath;
    node.h = sp_handle;
    node.open_count = 0;

    if (open_key(&node, !!(flags & KHM_FLAG_CREATE)) != NULL) {
        r_node = PMALLOC(sizeof(*r_node));
        *r_node = node;
        r_node->regpath = PWCSDUP(regpath);

        *r_nodeHandle = r_node;
        return KHM_ERROR_SUCCESS;
    } else {
        *r_nodeHandle = NULL;
        return KHM_ERROR_NOT_FOUND;
    }
}

khm_int32 KHMCALLBACK
reg_exit(void * nodeHandle)
{
    reg_node * node = (reg_node *) nodeHandle;
    assert(is_reg_node(node));

    close_key(node);
    if (node->regpath)
        PFREE(node->regpath);
    memset(node, 0, sizeof(*node));

    PFREE(node);

    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK
reg_open(void * nodeHandle)
{
    reg_node * node = (reg_node *) nodeHandle;
    assert(is_reg_node(node));

    ++(node->open_count);

    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK
reg_close(void * nodeHandle)
{
    reg_node * node = (reg_node *) nodeHandle;
    assert(is_reg_node(node));

    if (--(node->open_count) == 0)
        close_key(node);

    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK
reg_remove(void * nodeHandle)
{
    reg_node * node = (reg_node *) nodeHandle;
    assert(is_reg_node(node));

    close_key(node);
    if (SHDeleteKey(node->h_root, node->regpath) == ERROR_SUCCESS) {
        return KHM_ERROR_SUCCESS;
    } else {
        return KHM_ERROR_GENERAL;
    }
}

khm_int32 KHMCALLBACK
reg_create(void * nodeHandle, const wchar_t * name, khm_int32 flags)
{
    reg_node * node = (reg_node *) nodeHandle;
    khm_int32 rv;

    assert(is_reg_node(node));
    rv = khc_mount_provider(node->h, name, flags, &khc_reg_provider,
                            NULL, NULL);
    return rv;
}

khm_int32 KHMCALLBACK
reg_begin_enum(void * nodeHandle)
{
    reg_node * node = (reg_node *) nodeHandle;
    assert(is_reg_node(node));

    if (open_key(node, FALSE)) {
        wchar_t name[KCONF_MAXCCH_NAME];
        khm_handle h;
        int idx;

        idx = 0;
        while(RegEnumKey(node->h_key, idx, 
                         name, ARRAYLENGTH(name)) == ERROR_SUCCESS) {
            wchar_t * tilde;

            tilde = wcschr(name, L'~');
            if (tilde)
                *tilde = 0;
            if (KHM_SUCCEEDED(khc_open_space(node->h, name, 0, &h)))
                khc_close_space(h);
            idx++;
        }
    }

    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK
reg_get_mtime(void * nodeHandle, FILETIME * mtime)
{
    FILETIME ft;
    reg_node * node = (reg_node *) nodeHandle;
    assert(is_reg_node(node));

    if (open_key(node, FALSE) &&

        (RegQueryInfoKey(node->h_key, NULL, NULL, NULL,
                         NULL, NULL, NULL, NULL,
                         NULL, NULL, NULL, &ft) == ERROR_SUCCESS)) {
        *mtime = ft;

        return KHM_ERROR_SUCCESS;
    }

    return KHM_ERROR_NOT_FOUND;
}

khm_int32 KHMCALLBACK
reg_read_value(void * nodeHandle, const wchar_t * valuename,
               khm_int32 * vtype, void * buffer, khm_size * pcb_buffer)
{
    reg_node * node = (reg_node *) nodeHandle;
    DWORD size;
    DWORD type;
    LONG hr;

    assert(is_reg_node(node));

    if (!open_key(node, FALSE)) {
        return KHM_ERROR_NOT_FOUND;
    }

    if (pcb_buffer)
        size = (DWORD) *pcb_buffer;
    else
        size = 0;

    hr = RegQueryValueEx(node->h_key, valuename, NULL, &type, (LPBYTE) buffer, &size);

    if (hr == ERROR_SUCCESS) {

        if (vtype != NULL && *vtype != KC_NONE && type != kc_type_to_reg_type[*vtype])
            return KHM_ERROR_TYPE_MISMATCH;

        if (vtype != NULL)
            *vtype = ((type == REG_DWORD)? KC_INT32 :
                      (type == REG_QWORD)? KC_INT64 :
                      (type == REG_SZ || type == REG_EXPAND_SZ)? KC_STRING :
                      KC_BINARY);

        if (pcb_buffer == NULL)         /* Only checking if the value
                                           exists */
            return KHM_ERROR_SUCCESS;

        if ((type == REG_SZ || type == REG_EXPAND_SZ) && buffer != NULL &&
            ((wchar_t *) buffer)[size / sizeof(wchar_t) - 1] != L'\0') {

            /* if the target buffer is not large enough to store the
               terminating NUL, RegQueryValueEx() will return
               ERROR_SUCCESS without terminating the string. */

            *pcb_buffer = size + sizeof(wchar_t);
            return KHM_ERROR_TOO_LONG;

        }

        *pcb_buffer = size;

        /* if buffer==NULL, RegQueryValueEx will return success and
           just return the required buffer size in 'size' */
        return (buffer)? KHM_ERROR_SUCCESS: KHM_ERROR_TOO_LONG;

    } else if(hr == ERROR_MORE_DATA) {

        *pcb_buffer = size;
        return KHM_ERROR_TOO_LONG;

    } else {

        return KHM_ERROR_NOT_FOUND;

    }
}

khm_int32 KHMCALLBACK
reg_write_value(void * nodeHandle, const wchar_t * valuename,
                khm_int32 vtype, const void * buffer, khm_size cb)
{
    reg_node * node = (reg_node *) nodeHandle;
    assert(is_reg_node(node));

    if (open_key(node, FALSE)) {
        LONG hr;

        hr = RegSetValueEx(node->h_key, valuename, 0,
                           kc_type_to_reg_type[vtype],
                           (LPBYTE) buffer,
                           (DWORD) cb);

        if (hr == ERROR_SUCCESS)
            return KHM_ERROR_SUCCESS;
        else
            return KHM_ERROR_READONLY;
    } else {
        assert(FALSE);
        return KHM_ERROR_NOT_READY;
    }
}

khm_int32 KHMCALLBACK
reg_remove_value(void * nodeHandle, const wchar_t * valuename)
{
    reg_node * node = (reg_node *) nodeHandle;
    assert(is_reg_node(node));

    if (open_key(node, FALSE)) {
        LONG hr;

        hr = RegDeleteValue(node->h_key, valuename);

        if (hr == ERROR_SUCCESS)
            return KHM_ERROR_SUCCESS;
    }

    return KHM_ERROR_NOT_FOUND;
}

const khc_provider_interface khc_reg_provider = {
    KHC_PROVIDER_V1,
    reg_init,
    reg_exit,

    reg_open,
    reg_close,
    reg_remove,
    reg_create,
    reg_begin_enum,
    reg_get_mtime,

    reg_read_value,
    reg_write_value,
    reg_remove_value,
};
