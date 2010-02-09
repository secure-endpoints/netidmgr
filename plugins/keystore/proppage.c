/*
 * Copyright (c) 2006-2009 Secure Endpoints Inc.
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

#include "module.h"
#include <windowsx.h>
#include <commctrl.h>
#include <assert.h>

extern void creddlg_setup_idlist(HWND);
extern void creddlg_refresh_idlist(HWND, keystore_t *);

/* Dialog procedure and support code for displaying property sheets
   for credentials of type MyCred. */

BOOL pp_WM_INITDIALOG(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    khui_property_sheet * ps;
    PROPSHEETPAGE * p;
    keystore_t * ks;
    p = (PROPSHEETPAGE *) lParam;
    ps = (khui_property_sheet *) p->lParam;

    ks = find_keystore_for_identity(ps->identity);

    assert(ks != NULL);

    Edit_LimitText(GetDlgItem(hwnd, IDC_NAME), KHUI_MAXCCH_NAME);
    Edit_LimitText(GetDlgItem(hwnd, IDC_DESCRIPTION), KHUI_MAXCCH_SHORT_DESC);
    creddlg_setup_idlist(GetDlgItem(hwnd, IDC_IDLIST));

    KSLOCK(ks);
    SetDlgItemText(hwnd, IDC_NAME, ks->display_name);
    SetDlgItemText(hwnd, IDC_DESCRIPTION, ks->description);

    creddlg_refresh_idlist(GetDlgItem(hwnd, IDC_IDLIST), ks);
    KSUNLOCK(ks);

    ks_keystore_release(ks);

#pragma warning(push)
#pragma warning(disable: 4047)
    SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) ps);
#pragma warning(pop)

    return FALSE;
}

void pp_WM_COMMAND(HWND hwnd, int id, HWND hwCtrl, UINT code)
{
    khui_property_sheet * ps;
    keystore_t * ks;

    ps = (khui_property_sheet *) (LONG_PTR) GetWindowLongPtr(hwnd, DWLP_USER);
    if (ps == NULL)
        return;

    ks = find_keystore_for_identity(ps->identity);
    if (ks == NULL)
        return;

    if (code == EN_CHANGE) {
        wchar_t text[KHUI_MAXCCH_SHORT_DESC];
        BOOL changed = TRUE;

        GetDlgItemText(hwnd, id, text, ARRAYLENGTH(text));

        KSLOCK(ks);
        switch (id) {
        case IDC_NAME:
            if (ks->display_name &&
                !wcscmp(ks->display_name, text))
                changed = FALSE;
            break;

        case IDC_DESCRIPTION:
            if (ks->description &&
                !wcscmp(ks->description, text))
                changed = FALSE;
            break;
        }
        KSUNLOCK(ks);

        if (changed)
            PropSheet_Changed(ps->hwnd, hwnd);
        else
            PropSheet_UnChanged(ps->hwnd, hwnd);
    }

    ks_keystore_release(ks);
}

BOOL pp_WM_NOTIFY(HWND hwnd, WPARAM wParam, NMHDR * pnmh)
{
    khui_property_sheet * ps;
    keystore_t * ks;
    wchar_t text[KHUI_MAXCCH_SHORT_DESC];

    ps = (khui_property_sheet *) (LONG_PTR) GetWindowLongPtr(hwnd, DWLP_USER);
    if (ps == NULL)
        return FALSE;

    ks = find_keystore_for_identity(ps->identity);
    if (ks == NULL)
        return FALSE;

    switch (pnmh->code) {
    case PSN_APPLY:

        KSLOCK(ks);

        GetDlgItemText(hwnd, IDC_NAME, text, ARRAYLENGTH(text));
        if (!ks->display_name ||
            wcscmp(ks->display_name, text))
            ks_keystore_set_string(ks, KCDB_RES_DISPLAYNAME, text);

        GetDlgItemText(hwnd, IDC_DESCRIPTION, text, ARRAYLENGTH(text));
        if (!ks->description ||
            wcscmp(ks->description, text))
            ks_keystore_set_string(ks, KCDB_RES_DESCRIPTION, text);

        KSUNLOCK(ks);

        if (ks_keystore_get_flags(ks) & KS_FLAG_MODIFIED) {
            save_keystore_with_identity(ks);
            list_credentials();
        }

        SetDlgMsgResult(hwnd, WM_NOTIFY, PSNRET_NOERROR);

        break;
    }

    ks_keystore_release(ks);

    return TRUE;
}

void pp_WM_DESTROY(HWND hwnd)
{
    SetWindowLongPtr(hwnd, DWLP_USER, 0);
}

#define PP_HANDLE_MSG(msg) \
    case msg: return HANDLE_##msg(hwnd, wParam, lParam, pp_##msg)

/* Dialog procedure for the property sheet.  This will run under the
   UI thread when a property sheet is being displayed for one of our
   credentials.. */
INT_PTR CALLBACK
pp_cred_dlg_proc(HWND hwnd,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam) {

    switch (uMsg) {
        PP_HANDLE_MSG(WM_INITDIALOG);
        PP_HANDLE_MSG(WM_DESTROY);
        PP_HANDLE_MSG(WM_COMMAND);
        PP_HANDLE_MSG(WM_NOTIFY);
    }

    return FALSE;
}

#undef PP_HANDLE_MSG
