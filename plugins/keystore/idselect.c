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

#include <shlwapi.h>
#include "module.h"
#include <commctrl.h>
#include <assert.h>

/*  */

/************************************************************/
/*                Identity Selector Control                 */
/************************************************************/

struct idsel_dlg_data {
    khm_int32 magic;            /* Always IDSEL_DLG_DATA_MAGIC */
    khm_handle identity;           /* Current selection */
};

#define IDSEL_DLG_DATA_MAGIC 0x5967d973

static INT_PTR
on_browse(HWND hwnd)
{
    OPENFILENAME ofn;
    wchar_t path[MAX_PATH] = L"";
    wchar_t filter[128];
    wchar_t title[64];

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = hResModule;
    ofn.lpstrFilter = filter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_DONTADDTORECENT | OFN_NOREADONLYRETURN;
    ofn.lpstrDefExt = L"keystore";

    GetDlgItemText(hwnd, IDC_PATH, path, ARRAYLENGTH(path));
    LoadString(hResModule, IDS_NC_BROW_TITLE, title, ARRAYLENGTH(title));
    LoadString(hResModule, IDS_NC_BROW_FILTER, filter, ARRAYLENGTH(filter));
    {
        wchar_t * c = filter;
        for (; *c; c++) {
            if (*c == L'%') *c = L'\0';
        }
    }

    if (GetOpenFileName(&ofn)) {
        keystore_t * ks = NULL;
        wchar_t fpath[MAX_PATH+5];

        SetDlgItemText(hwnd, IDC_PATH, path);

        if (PathFileExists(path)) {
            StringCbCopy(fpath, sizeof(fpath), L"FILE:");
            StringCbCat(fpath, sizeof(fpath), path);

            ks = create_keystore_from_location(fpath, NULL);
            if (ks) {
                wchar_t buf[KCDB_MAXCCH_SHORT_DESC];
                khm_size cb;

                cb = sizeof(buf);
                if (KHM_SUCCEEDED(ks_keystore_get_string(ks, KCDB_RES_DISPLAYNAME,
                                                         buf, &cb))) {
                    SetDlgItemText(hwnd, IDC_NAME, buf);
                }
                if (KHM_SUCCEEDED(ks_keystore_get_string(ks, KCDB_RES_DESCRIPTION,
                                                         buf, &cb))) {
                    SetDlgItemText(hwnd, IDC_DESCRIPTION, buf);
                }

                ks_keystore_release(ks);
            }
        }
    }

    return TRUE;
}

static void
on_get_ident(HWND hwnd, struct idsel_dlg_data *d, khm_handle *ph)
{
    khm_handle identity = NULL;
    keystore_t * ks = NULL;
    wchar_t desc[KCDB_MAXCCH_SHORT_DESC];
    wchar_t path[MAX_PATH+5];
    khm_boolean try_open = FALSE;

    if (IsDlgButtonChecked(hwnd, IDC_FILE) == BST_CHECKED) {
        StringCbCopy(path, sizeof(path), L"FILE:");
        GetDlgItemText(hwnd, IDC_PATH, path + 5, ARRAYLENGTH(path) - 5);
        if (PathFileExists(path+5)) {
            /* If the path exists, then we should try to create the
               keystore using the path before */
            try_open = TRUE;
        }
    } else {
        StringCbCopy(path, sizeof(path), L"REG:");
    }

    if (try_open)
        ks = create_keystore_from_location(path, NULL);

    if (ks == NULL)
        ks = ks_keystore_create_new();

    GetDlgItemText(hwnd, IDC_NAME, desc, ARRAYLENGTH(desc));
    ks_keystore_set_string(ks, KCDB_RES_DISPLAYNAME, desc);
    GetDlgItemText(hwnd, IDC_DESCRIPTION, desc, ARRAYLENGTH(desc));
    ks_keystore_set_string(ks, KCDB_RES_DESCRIPTION, desc);
    ks_keystore_set_flags(ks, KS_FLAG_MODIFIED, KS_FLAG_MODIFIED);

    identity = create_identity_from_keystore(ks);

    kcdb_identity_set_attr(identity, KCDB_ATTR_LOCATION, path, KCDB_CBSIZE_AUTO);

    *ph = identity;
    /* leave identity held */
}


/* Dialog procedure for IDD_IDSEL

   runs in UI thread */
static INT_PTR CALLBACK
idspec_dlg_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg) {
    case WM_INITDIALOG:
        {
            struct idsel_dlg_data * d;

            d = PMALLOC(sizeof(*d));
            ZeroMemory(d, sizeof(*d));
            d->magic = IDSEL_DLG_DATA_MAGIC;
            d->identity = NULL;

#pragma warning(push)
#pragma warning(disable: 4244)
            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) d);
#pragma warning(pop)

            /* TODO: Initialize controls etc. */

        }

        CheckRadioButton(hwnd, IDC_REGISTRY, IDC_FILE, IDC_REGISTRY);
        EnableWindow(GetDlgItem(hwnd, IDC_PATH), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), FALSE);

        SendDlgItemMessage(hwnd, IDC_NAME, EM_SETLIMITTEXT, KCDB_MAXCCH_NAME, 0);
        SendDlgItemMessage(hwnd, IDC_DESCRIPTION, EM_SETLIMITTEXT, KCDB_MAXCCH_SHORT_DESC, 0);
        SendDlgItemMessage(hwnd, IDC_PATH, EM_SETLIMITTEXT, MAX_PATH, 0);

        {
            wchar_t cuebanner[KCDB_MAXCCH_NAME];
            LoadString(hResModule, IDS_CUE_NAME, cuebanner, ARRAYLENGTH(cuebanner));
            SendDlgItemMessage(hwnd, IDC_NAME, EM_SETCUEBANNER, 0, (LPARAM)cuebanner);
            LoadString(hResModule, IDS_CUE_DESC, cuebanner, ARRAYLENGTH(cuebanner));
            SendDlgItemMessage(hwnd, IDC_DESCRIPTION, EM_SETCUEBANNER, 0, (LPARAM)cuebanner);
        }

        /* We return FALSE here because this is an embedded modeless
           dialog and we don't want to steal focus from the
           container. */
        return FALSE;

    case WM_DESTROY:
        {
            struct idsel_dlg_data * d;

            d = (struct idsel_dlg_data *)(LONG_PTR)
                GetWindowLongPtr(hwnd, DWLP_USER);

#ifdef DEBUG
            assert(d != NULL);
            assert(d->magic == IDSEL_DLG_DATA_MAGIC);
#endif
            if (d && d->magic == IDSEL_DLG_DATA_MAGIC) {
                if (d->identity) {
                    kcdb_identity_release(d->identity);
                    d->identity = NULL;
                }

                d->magic = 0;
                PFREE(d);
            }
#pragma warning(push)
#pragma warning(disable: 4244)
            SetWindowLongPtr(hwnd, DWLP_USER, 0);
#pragma warning(pop)
        }
        return TRUE;

    case WM_COMMAND:
        {
            switch (wParam) {
            case MAKEWPARAM(IDC_REGISTRY, BN_CLICKED):
                EnableWindow(GetDlgItem(hwnd, IDC_PATH), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), FALSE);
                return TRUE;

            case MAKEWPARAM(IDC_FILE, BN_CLICKED):
                EnableWindow(GetDlgItem(hwnd, IDC_PATH), TRUE);
                EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), TRUE);
                return TRUE;

            case MAKEWPARAM(IDC_BROWSE, BN_CLICKED):
                return on_browse(hwnd);
            }
        }
        break;

    case KHUI_WM_NC_NOTIFY:
        {
            struct idsel_dlg_data * d;
            khm_handle * ph;

            d = (struct idsel_dlg_data *)(LONG_PTR)
                GetWindowLongPtr(hwnd, DWLP_USER);

            switch (HIWORD(wParam)) {
            case WMNC_IDSEL_GET_IDENT:
                ph = (khm_handle *) lParam;

                on_get_ident(hwnd, d, ph);
                break;
            }
        }
        return TRUE;
    }
    return FALSE;
}

/* Identity Selector control factory

   Runs in UI thread */
khm_int32 KHMAPI 
idsel_factory(HWND hwnd_parent, HWND * phwnd_return) {

    HWND hw_dlg;

    hw_dlg = CreateDialog(hResModule, MAKEINTRESOURCE(IDD_IDSPEC),
                          hwnd_parent, idspec_dlg_proc);

#ifdef DEBUG
    assert(hw_dlg);
#endif
    *phwnd_return = hw_dlg;

    return (hw_dlg ? KHM_ERROR_SUCCESS : KHM_ERROR_UNKNOWN);
}

khm_int32
handle_kmsg_ident_get_idsel_factory(kcdb_idsel_factory * pcb)
{
    *pcb = idsel_factory;

    return KHM_ERROR_SUCCESS;
}

