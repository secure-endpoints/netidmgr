/*
 * Copyright (c) 2006 Secure Endpoints Inc.
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

/* Dialog procedures and support functions for handling configuration
   dialogs for per-identity configuration. When the configuration
   dialog is activated, an instance of this dialog will be created for
   each identity that the user touches. */

/* The structure that we use to hold state information for the
   dialog. */
typedef struct tag_config_id_dlg_data {
    khui_config_init_data cfg;  /* instance information for this
                                   dialog */

    khm_handle ident;           /* handle to the identity for this
                                   dialog */

    khm_int32  init_rv;         /* return value that should be passed
                                   in WMCFG_INIT_PANEL */

    HWND       child_panel;     /* handle to the child panel. */

    keystore_t * ks;            /* keystore corresponding to ident if
                                   ident is a keystore */

    keystore_t ** aks;          /* all keystores that contain keys for
                                   ident if ident is not a keystore */
    khm_size   n_aks;           /* number of keystores in aks */

    /* TODO: Add any fields for holding state here */
} config_id_dlg_data;

void
config_id_prompt_for_new_identity(HWND hwnd, config_id_dlg_data * d)
{
    khm_handle credset = NULL;
    khm_int32 rv;

    assert(is_keystore_t(d->ks));

    kcdb_credset_create(&credset);
    rv = khui_cw_collect_privileged_credentials(NULL, hwnd, NULL, credset);

    if (rv == KHUI_NC_RESULT_PROCESS) {
        add_identkeys_from_credset(d->ks, credset);
    } else {
        assert(FALSE);
    }

    kcdb_credset_delete(credset);
}

INT_PTR
config_id_ks_add_new_identkey(HWND hwnd, config_id_dlg_data * d)
{
    if (get_key_if_necessary(hwnd, d->ks, IDS_PWR_ADD)) {

        config_id_prompt_for_new_identity(hwnd, d);

        ks_keystore_lock(d->ks);
        save_keystore_with_identity(d->ks);
        creddlg_refresh_idlist(GetDlgItem(hwnd, IDC_IDLIST), d->ks);
        ks_keystore_release_key(d->ks);
    }

    return TRUE;
}

INT_PTR
config_id_ks_delete_identkey(HWND hwnd, config_id_dlg_data * d)
{
    HWND hw_list;
    int n_to_del;

    hw_list = GetDlgItem(hwnd, IDC_IDLIST);

    if ((n_to_del = ListView_GetSelectedCount(hw_list)) == 0)
        return TRUE;            /* nothing to do */

    if (get_key_if_necessary(hwnd, d->ks, IDS_PWR_DEL)) {
        int    idx = -1;

        {
            wchar_t title[64];
            wchar_t msg[128];
            wchar_t fmt[128];

            LoadString(hResModule, IDS_CONF_DEL_TITLE, title, ARRAYLENGTH(title));

            if (n_to_del == 1)
                LoadString(hResModule, IDS_CONF_DEL_FMT1, msg, ARRAYLENGTH(msg));
            else {
                LoadString(hResModule, IDS_CONF_DEL_FMT, fmt, ARRAYLENGTH(fmt));
                StringCbPrintf(msg, sizeof(msg), fmt, n_to_del);
            }

            if (MessageBox(hwnd, msg, title, MB_YESNO | MB_ICONQUESTION) != IDYES) {
                ks_keystore_release_key(d->ks);
                return TRUE;
            }
        }

        while ((idx = ListView_GetNextItem(hw_list, idx, LVNI_SELECTED)) != -1) {
            LVITEM lvi;

            lvi.mask = LVIF_PARAM;
            lvi.iItem = idx;
            lvi.iSubItem = 0;

            ListView_GetItem(hw_list, &lvi);

            ks_keystore_mark_remove_identkey(d->ks, lvi.lParam);
        }

        ks_keystore_purge_removed_identkeys(d->ks);
        ks_keystore_lock(d->ks);
        save_keystore_with_identity(d->ks);
        creddlg_refresh_idlist(hw_list, d->ks);
        ks_keystore_release_key(d->ks);
    }

    return TRUE;
}

INT_PTR
config_id_ks_configure_identkey(HWND hwnd, config_id_dlg_data * d)
{
    creddlg_prompt_for_configure(hwnd, NULL, d->ks);
    return TRUE;
}

INT_PTR CALLBACK
config_id_ks_dlgproc(HWND hwnd,
                     UINT uMsg,
                     WPARAM wParam,
                     LPARAM lParam)
{
    config_id_dlg_data * d;

    switch (uMsg) {
    case WM_INITDIALOG:
        {
            HWND hw_list;

            d = (config_id_dlg_data *) lParam;

#pragma warning(push)
#pragma warning(disable: 4244)
            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) d);
#pragma warning(pop)

            d->ks = find_keystore_for_identity(d->ident);
            if (d->ks == NULL) {
                d->init_rv = KHM_ERROR_NOT_FOUND;
                return FALSE;
            }

            hw_list = GetDlgItem(hwnd, IDC_IDLIST);
            creddlg_setup_idlist(hw_list);
            creddlg_refresh_idlist(hw_list, d->ks);

            KSLOCK(d->ks);
            SetDlgItemText(hwnd, IDC_NAME, d->ks->display_name);
            SetDlgItemText(hwnd, IDC_LOCATION, d->ks->location);
            KSUNLOCK(d->ks);
        }
        return FALSE;

    case WM_NOTIFY:
        {
            NMHDR *pnmh = (NMHDR *) lParam;

            if (pnmh->idFrom == IDC_IDLIST &&
                pnmh->code == LVN_ITEMCHANGED) {

                if (ListView_GetSelectedCount(pnmh->hwndFrom) != 0) {
                    EnableWindow(GetDlgItem(hwnd, IDC_REMOVE), TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDC_CONFIGURE), TRUE);
                } else {
                    EnableWindow(GetDlgItem(hwnd, IDC_REMOVE), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_CONFIGURE), FALSE);
                }
            }
        }
        return TRUE;

    case WM_COMMAND:
        d = (config_id_dlg_data *)
            GetWindowLongPtr(hwnd, DWLP_USER);
        if (d == NULL || d->ks == NULL)
            break;

        {
            int code = HIWORD(wParam);
            int id = LOWORD(wParam);

            if (code == BN_CLICKED && id == IDC_ADDNEW) {
                return config_id_ks_add_new_identkey(hwnd, d);
            }

            if (code == BN_CLICKED && id == IDC_REMOVE) {
                return config_id_ks_delete_identkey(hwnd, d);
            }

            if (code == BN_CLICKED && id == IDC_CONFIGURE) {
                return config_id_ks_configure_identkey(hwnd, d);
            }

            if (code == BN_CLICKED && id == IDC_SHOWPW) {
                if (IsDlgButtonChecked(hwnd, IDC_SHOWPW) == BST_CHECKED) {
                    if (get_key_if_necessary(hwnd, d->ks, IDS_PWR_SHOWP)) {
                        creddlg_show_passwords(GetDlgItem(hwnd, IDC_IDLIST), d->ks);
                        ks_keystore_release_key(d->ks);
                    }
                } else {
                    creddlg_hide_passwords(GetDlgItem(hwnd, IDC_IDLIST), d->ks);
                }
            }
        }

        return TRUE;
    }

    return FALSE;
}

void
config_id_o_setup_kslist(HWND hwlist)
{
    LVCOLUMN columns[] = {
        { LVCF_TEXT | LVCF_WIDTH, 0, 128, MAKEINTRESOURCE(IDS_CFG_KSLIST_C0), 0, 0, 0, 0 },
        { LVCF_TEXT | LVCF_WIDTH, 0, 128, MAKEINTRESOURCE(IDS_CFG_KSLIST_C1), 0, 0, 0, 0 },
    };
    int i;
    RECT r;
    wchar_t buf[128];
    HIMAGELIST hi;

    GetClientRect(hwlist, &r);
    for (i=0; i < ARRAYLENGTH(columns); i++) {
        columns[i].cx = columns[i].cx * (r.right - r.left) / 256;
        LoadString(hResModule, (UINT)(UINT_PTR) columns[i].pszText, buf, ARRAYLENGTH(buf));
        columns[i].pszText = buf;
        ListView_InsertColumn(hwlist, i, &columns[i]);
    }
    hi = ImageList_LoadBitmap(hResModule, MAKEINTRESOURCE(IDB_NC_STATE),
                              16, 4, RGB(255,0,255));
    ImageList_SetOverlayImage(hi, 1, 1);
    ImageList_SetOverlayImage(hi, 2, 2);
    ListView_SetImageList(hwlist, hi, LVSIL_SMALL);
}

void
config_id_o_refresh_kslist(HWND hwlist, keystore_t ** aks, khm_size n_aks)
{
    khm_size i;

    ListView_DeleteAllItems(hwlist);
    for (i=0; i < n_aks; i++) {
        LVITEM lvi_ks = {
            LVIF_PARAM | LVIF_TEXT,
            (int) i, 0,
            0,
            0,
            NULL, 0,
            0, i, 0, 0, 0, 0};
        LVITEM lvi_desc = {
            LVIF_TEXT,
            (int) i, 1, 0, 0,
            NULL, 0,
            0, 0, 0, 0, 0, 0};

        KSLOCK(aks[i]);
        lvi_ks.pszText = aks[i]->display_name;
        lvi_desc.pszText = aks[i]->location;

        ListView_InsertItem(hwlist, &lvi_ks);
        ListView_SetItem(hwlist, &lvi_desc);
        KSUNLOCK(aks[i]);
    }
}


INT_PTR CALLBACK
config_id_o_dlgproc(HWND hwnd,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
    config_id_dlg_data * d;

    switch (uMsg) {
    case WM_INITDIALOG:
        {
            HWND hw_list;

            d = (config_id_dlg_data *) lParam;

#pragma warning(push)
#pragma warning(disable: 4244)
            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) d);
#pragma warning(pop)

            d->n_aks = get_keystores_with_identkey(d->ident, &d->aks);
            if (d->n_aks == 0) {
                d->init_rv = KHM_ERROR_NOT_FOUND;
                return FALSE;
            }

            hw_list = GetDlgItem(hwnd, IDC_KSLIST);
            config_id_o_setup_kslist(hw_list);
            config_id_o_refresh_kslist(hw_list, d->aks, d->n_aks);
        }
        return FALSE;

    }

    return FALSE;
}

INT_PTR CALLBACK
config_id_dlgproc(HWND hwnd,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam) {

    config_id_dlg_data * d;

    switch (uMsg) {
    case WM_INITDIALOG:
        {
            d = malloc(sizeof(*d));
            assert(d);
            ZeroMemory(d, sizeof(*d));

            /* for subpanels, lParam is a pointer to a
               khui_config_init_data strucutre that provides the
               instance and context information.  It's not a
               persistent strucutre, so we have to make a copy. */
            d->cfg = *((khui_config_init_data *) lParam);

            /* The private data associated with a configuration node
               that represents an identity is a held identity
               handle. */
            d->ident = khui_cfg_get_data(d->cfg.ctx_node);
            assert(d->ident != NULL);
            kcdb_identity_hold(d->ident);

            if (d->cfg.ctx_node == d->cfg.ref_node) {
                d->init_rv = KHM_ERROR_GENERAL;
            } else {
                khm_handle prov = NULL;

                kcdb_identity_get_identpro(d->ident, &prov);

                if (prov == h_idprov) {
                    d->child_panel = CreateDialogParam(hResModule,
                                                       MAKEINTRESOURCE(IDD_CONFIG_ID_KS),
                                                       hwnd,
                                                       config_id_ks_dlgproc,
                                                       (LPARAM) d);
                } else {
                    d->child_panel = CreateDialogParam(hResModule,
                                                       MAKEINTRESOURCE(IDD_CONFIG_ID_O),
                                                       hwnd,
                                                       config_id_o_dlgproc,
                                                       (LPARAM) d);
                }

                SetWindowPos(d->child_panel, NULL, 0, 0, 0, 0,
                             SWP_NOACTIVATE | SWP_SHOWWINDOW |
                             SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);

                if (prov)
                    kcdb_identpro_release(prov);
            }

#pragma warning(push)
#pragma warning(disable: 4244)
            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) d);
#pragma warning(pop)
        }
        break;

    case KHUI_WM_CFG_NOTIFY:
        d = (config_id_dlg_data *)
            GetWindowLongPtr(hwnd, DWLP_USER);
        if (d == NULL)
            break;

        if (HIWORD(wParam) == WMCFG_APPLY) {
            /* TODO: apply changes */

            return TRUE;
        } else if (HIWORD(wParam) == WMCFG_INIT_PANEL) {
            khm_int32 * prv = (khm_int32 *) lParam;

            if (prv)
                *prv = d->init_rv;
        }
        break;

    case WM_DESTROY:
        {
            d = (config_id_dlg_data *)
                GetWindowLongPtr(hwnd, DWLP_USER);

            if (d) {
                if (d->child_panel)
                    DestroyWindow(d->child_panel);

                if (d->ks)
                    ks_keystore_release(d->ks);

                if (d->n_aks)
                    free_keystores_list(d->aks, d->n_aks);

                if (d->ident)
                    kcdb_identity_release(d->ident);

                free(d);
                SetWindowLongPtr(hwnd, DWLP_USER, 0);
            }
        }
        break;
    }

    return FALSE;

}
