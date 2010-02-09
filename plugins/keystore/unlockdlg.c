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

/* $Id$ */

#include "module.h"
#include <commctrl.h>
#include <assert.h>

struct key_prompt_dlg_data {
    keystore_t * ks;
    int res_reason;
    khm_boolean got_pw;
};

INT_PTR CALLBACK get_key_dlg_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    struct key_prompt_dlg_data * d = NULL;

    switch (uMsg) {
    case WM_INITDIALOG:
        d = (struct key_prompt_dlg_data *)(LONG_PTR) lParam;
        SetWindowLongPtr(hwnd, DWLP_USER, lParam);

        if (d->res_reason) {
            wchar_t reason[128] = L"";

            LoadString(hResModule, d->res_reason, reason, ARRAYLENGTH(reason));
            SetDlgItemText(hwnd, IDC_REASON, reason);
        }

        {
            khm_handle identity = create_identity_from_keystore(d->ks);
            if (identity) {
                HICON icon = NULL;
                khm_size cb = sizeof(icon);
                wchar_t idname[KCDB_IDENT_MAXCCH_NAME] = L"";

                kcdb_get_resource(identity, KCDB_RES_ICON_NORMAL, 0, NULL, NULL,
                                  &icon, &cb);

                if (icon) {
                    SendDlgItemMessage(hwnd, IDC_IDICON, STM_SETICON, (WPARAM) icon, 0);
                }

                cb = sizeof(idname);
                kcdb_get_resource(identity, KCDB_RES_DISPLAYNAME, 0, NULL, NULL,
                                  idname, &cb);
                if (idname[0]) {
                    SetDlgItemText(hwnd, IDC_NAME, idname);
                }

                kcdb_identity_release(identity);
            }
        }

        SendDlgItemMessage(hwnd, IDC_PASSWORD, EM_LIMITTEXT, KHUI_MAXCCH_PASSWORD, 0);
        return TRUE;

    case WM_COMMAND:
        d = (struct key_prompt_dlg_data *)(LONG_PTR) GetWindowLongPtr(hwnd, DWLP_USER);
        if (d == NULL)
            return FALSE;

        switch (wParam) {
        case MAKEWPARAM(IDOK, BN_CLICKED):
            {
                wchar_t pw[KHUI_MAXCCH_PASSWORD];
                khm_size cb = 0;

                GetDlgItemText(hwnd, IDC_PASSWORD, pw, ARRAYLENGTH(pw));
                StringCbLength(pw, KHUI_MAXCB_PASSWORD, &cb);

                if (KHM_SUCCEEDED(ks_keystore_set_key_password(d->ks, pw, cb))) {
                    d->got_pw = TRUE;
                    EndDialog(hwnd, 1);
                    list_credentials();
                    return TRUE;
                } else {
                    MessageBox(hwnd, L"Incorrect password", L"Bad password", MB_OK);
                    return TRUE;
                }
            }

        case MAKEWPARAM(IDCANCEL, BN_CLICKED):
            EndDialog(hwnd, 0);
            break;
        }
        return TRUE;

    case WM_CLOSE:
        EndDialog(hwnd, 0);
        return TRUE;
    }

    return FALSE;
}


khm_boolean
get_key_if_necessary(HWND hwnd, keystore_t * ks, int res_reason)
{
    struct key_prompt_dlg_data d;

    if (ks_keystore_hold_key(ks))
        return TRUE;

    d.ks = ks;
    d.res_reason = res_reason;
    d.got_pw = FALSE;

    DialogBoxParam(hResModule, MAKEINTRESOURCE(IDD_KEYPROMPT), hwnd, get_key_dlg_proc,
                   (LPARAM) &d);

    return ks_keystore_hold_key(ks);
}
