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
#include <windowsx.h>
#include <assert.h>

/* This file provides handlers for the credentials acquisition
   messages including handling the user interface for the new
   credentials dialogs. */

/*********************************************************************

These are stubs for the Window message for the dialog panel.  This
dialog panel is the one that is added to the new credentials window
for obtaining new credentials.

Note that all the UI callbacks run under the UI thread.

 *********************************************************************/

/* This structure will hold all the state information we will need to
   access from the new credentials panel for our credentials type. */
struct nc_dialog_data {
    khui_new_creds_by_type nct;
    HWND hw_privint;

    keystore_t * ks;
    khm_boolean  foreign_id;

    /* for password changing */
    khm_size n_ks;
    keystore_t **aks;
    HWND *hw_privints;
    khm_size cur_idx;
};

INT_PTR
privint_WM_INITDIALOG(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    struct nc_dialog_data * d = NULL;
    keystore_t * ks;

    d = (struct nc_dialog_data *) lParam;
    assert(d);

#pragma warning(push)
#pragma warning(disable: 4244)
    SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) d);
#pragma warning(pop)
    SetProp(hwnd, L"KeyStoreIndex", (HANDLE) d->cur_idx);

    if (khui_cw_get_subtype(d->nct.nc) == KHUI_NC_SUBTYPE_NEW_CREDS)
        ks = d->ks;
    else {
        wchar_t desc[KHUI_MAXCCH_SHORT_DESC];
        wchar_t fmt[64];
        khm_handle identity;

        ks = d->aks[d->cur_idx];
        LoadString(hResModule, IDS_NC_PWDTITLE, fmt, ARRAYLENGTH(fmt));
        StringCbPrintf(desc, sizeof(desc), fmt, ks->display_name);
        SetDlgItemText(hwnd, IDC_LBL_KSNAME, desc);

        identity = create_identity_from_keystore(ks);
        if (identity) {
            HICON icon;
            khm_size cb = sizeof(icon);
            if (KHM_SUCCEEDED(kcdb_get_resource(identity, KCDB_RES_ICON_NORMAL, 0, NULL, NULL,
                                                &icon, &cb))) {
                Static_SetIcon(GetDlgItem(hwnd, IDC_IDICON), icon);
            }
        }
    }

    if (ks == NULL) {
        EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_NEWPW1), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_NEWPW2), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_LBL_NEWPW1), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_LBL_NEWPW2), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_CHPW), FALSE);

        return FALSE;        
    }

    if (ks_is_keystore_locked(ks)) {
        EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD), TRUE);
        ShowWindow(GetDlgItem(hwnd, IDC_NEWPW1), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDC_NEWPW2), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDC_LBL_NEWPW1), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDC_LBL_NEWPW2), SW_HIDE);
        Button_SetCheck(GetDlgItem(hwnd, IDC_CHPW), BST_UNCHECKED);
        EnableWindow(GetDlgItem(hwnd, IDC_CHPW), TRUE);
    } else {
        EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD), FALSE);
        ShowWindow(GetDlgItem(hwnd, IDC_NEWPW1), SW_SHOW);
        ShowWindow(GetDlgItem(hwnd, IDC_NEWPW2), SW_SHOW);
        ShowWindow(GetDlgItem(hwnd, IDC_LBL_NEWPW1), SW_SHOW);
        ShowWindow(GetDlgItem(hwnd, IDC_LBL_NEWPW2), SW_SHOW);
        Button_SetCheck(GetDlgItem(hwnd, IDC_CHPW), BST_CHECKED);
        EnableWindow(GetDlgItem(hwnd, IDC_CHPW), FALSE);
    }

    return FALSE;
}

void
privint_CheckIfReady(HWND hwnd)
{
    struct nc_dialog_data * d;
    khm_boolean is_ready = FALSE;
    keystore_t * ks;

    d = (struct nc_dialog_data *)GetWindowLongPtr(hwnd, DWLP_USER);
    assert(d);

    if (khui_cw_get_subtype(d->nct.nc) == KHUI_NC_SUBTYPE_NEW_CREDS)
        ks = d->ks;
    else
        ks = d->aks[(int) GetProp(hwnd, L"KeyStoreIndex")];

    do {
        if (ks_is_keystore_locked(ks) &&
            GetWindowTextLength(GetDlgItem(hwnd, IDC_PASSWORD)) <= 0)
            break;

        if (IsDlgButtonChecked(hwnd, IDC_CHPW) == BST_CHECKED) {
            wchar_t pw1[KHUI_MAXCCH_PASSWORD] = L"";
            wchar_t pw2[KHUI_MAXCCH_PASSWORD] = L"";

            if (GetDlgItemText(hwnd, IDC_NEWPW1, pw1, ARRAYLENGTH(pw1)) == 0 ||
                GetDlgItemText(hwnd, IDC_NEWPW2, pw2, ARRAYLENGTH(pw2)) == 0 ||
                wcscmp(pw1, pw2))
                break;
        }

        is_ready = TRUE;
    } while (FALSE);

    khui_cw_notify_identity_state(d->nct.nc, hwnd, NULL,
                                  ((is_ready)? KHUI_CWNIS_READY : 0) | KHUI_CWNIS_NOPROGRESS, 0);
}

INT_PTR
privint_WM_COMMAND(HWND hwnd, int id, HWND hwCtl, UINT code)
{
    if (id == IDC_CHPW && code == BN_CLICKED) {
        if (IsDlgButtonChecked(hwnd, IDC_CHPW) == BST_CHECKED) {
            ShowWindow(GetDlgItem(hwnd, IDC_NEWPW1), SW_SHOW);
            ShowWindow(GetDlgItem(hwnd, IDC_NEWPW2), SW_SHOW);
            ShowWindow(GetDlgItem(hwnd, IDC_LBL_NEWPW1), SW_SHOW);
            ShowWindow(GetDlgItem(hwnd, IDC_LBL_NEWPW2), SW_SHOW);
        } else {
            ShowWindow(GetDlgItem(hwnd, IDC_NEWPW1), SW_HIDE);
            ShowWindow(GetDlgItem(hwnd, IDC_NEWPW2), SW_HIDE);
            ShowWindow(GetDlgItem(hwnd, IDC_LBL_NEWPW1), SW_HIDE);
            ShowWindow(GetDlgItem(hwnd, IDC_LBL_NEWPW2), SW_HIDE);
        }
    }

    if (code == EN_CHANGE)
        privint_CheckIfReady(hwnd);

    return FALSE;
}

INT_PTR
privint_WM_DESTROY(HWND hwnd)
{
    SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) 0);
    RemoveProp(hwnd, L"KeyStoreIndex");

    return TRUE;
}

/* Dialog procedure for the privileged interaction panel for a keystore */
INT_PTR CALLBACK
nc_privint_dlg_proc(HWND hwnd,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
    switch (uMsg) {
#define PHANDLE_MSG(m) \
    case m: return HANDLE_##m((hwnd), (wParam), (lParam), privint_##m)
        PHANDLE_MSG(WM_INITDIALOG);
        PHANDLE_MSG(WM_DESTROY);
        PHANDLE_MSG(WM_COMMAND);
#undef  PHANDLE_MSG
    }
    return FALSE;
}

void
creddlg_setup_idlist(HWND hwlist)
{
    LVCOLUMN columns[] = {
        { LVCF_TEXT | LVCF_WIDTH, 0, 192, MAKEINTRESOURCE(IDS_NCLC_ID), 0, 0, 0, 0 },
        { LVCF_TEXT | LVCF_WIDTH, 0, 64, MAKEINTRESOURCE(IDS_NCLC_TYPE), 0, 0, 0, 0}
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
creddlg_refresh_idlist(HWND hwlist, keystore_t * ks)
{
    khm_size i;

    ListView_DeleteAllItems(hwlist);
    KSLOCK(ks);
    for (i=0; i < ks->n_keys; i++) {
        LVITEM lvi_id = {
            LVIF_PARAM | LVIF_STATE | LVIF_TEXT | LVIF_IMAGE,
            (int) i, 0,
            INDEXTOOVERLAYMASK((ks->keys[i]->flags & IDENTKEY_FLAG_LOCKED)?2:1),
            LVIS_OVERLAYMASK,
            ks->keys[i]->display_name, 0,
            0, i, 0, 0, 0, 0};
        wchar_t idtype[KCDB_MAXCCH_NAME] = L"";
        LVITEM lvi_type = {
            LVIF_TEXT,
            (int) i, 1, 0, 0,
            NULL, 0,
            0, 0, 0, 0, 0, 0};
        khm_size cb;
        khm_handle idpro;

        lvi_type.pszText = idtype;

        if (KHM_SUCCEEDED(kcdb_identpro_find(ks->keys[i]->provider_name, &idpro))) {
            cb = sizeof(idtype);
            kcdb_get_resource(idpro, KCDB_RES_INSTANCE, 0, NULL, NULL, idtype, &cb);
            kcdb_identpro_release(idpro);
        }

        ListView_InsertItem(hwlist, &lvi_id);
        ListView_SetItem(hwlist, &lvi_type);
    }
    KSUNLOCK(ks);
}

/* Note: This callback runs under the UI thread */
INT_PTR
creddlg_WM_INITDIALOG(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    khui_new_creds * nc = NULL;
    struct nc_dialog_data * d = NULL;

    nc = (khui_new_creds *) lParam;
    khui_cw_find_type(nc, credtype_id, (khui_new_creds_by_type **) &d);

    assert(d);

#pragma warning(push)
#pragma warning(disable: 4244)
    SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) d);
#pragma warning(pop)

    if (khui_cw_get_subtype(nc) == KHUI_NC_SUBTYPE_NEW_CREDS)
        creddlg_setup_idlist(GetDlgItem(hwnd, IDC_IDLIST));

    return FALSE;
}

INT_PTR
creddlg_WMNC_IDENTITY_CHANGE_new_creds(HWND hwnd, struct nc_dialog_data * d)
{
    khm_int32 ctype;
    khm_size cb;
    khm_handle identity = NULL;
    keystore_t * ks = NULL;
    HWND hw_privint;

    cb = sizeof(ctype);
    if (KHM_FAILED(khui_cw_get_primary_id(d->nct.nc, &identity)) ||
        KHM_FAILED(kcdb_identity_get_attr(identity, KCDB_ATTR_TYPE,
                                          NULL, &ctype, &cb)) ||
        ctype != credtype_id) {
        khui_cw_enable_type(d->nct.nc, credtype_id, FALSE);
        kcdb_identity_release(identity);
        return TRUE;
    }

    ks = find_keystore_for_identity(identity);

    if (d->ks)
        ks_keystore_release(d->ks);
    d->ks = ks;
    /* leave ks held */

    if (d->ks == NULL) {
        /* There is no valid keystore for this identity. */
        wchar_t status[KHUI_MAXCCH_MESSAGE] = L"";
        khm_size cb = sizeof(status);

        kcdb_identity_get_attr(identity, KCDB_ATTR_STATUS, NULL, status, &cb);
        khui_cw_notify_identity_state(d->nct.nc, d->nct.hwnd_panel,
                                      status, KHUI_CWNIS_VALIDATED | KHUI_CWNIS_NOPROGRESS, 0);
        kcdb_identity_release(identity);

        return TRUE;
    }

    hw_privint = CreateDialogParam(hResModule,
                                   MAKEINTRESOURCE(IDD_NC_PRIV),
                                   GetParent(hwnd),
                                   nc_privint_dlg_proc,
                                   (LPARAM) d);
    {
        wchar_t caption[256];
        LoadString(hResModule, IDS_NCPRIV_CAPTION, caption,
                   ARRAYLENGTH(caption));

        khui_cw_show_privileged_dialog(d->nct.nc, credtype_id,
                                       hw_privint, caption);
        privint_CheckIfReady(hw_privint);
    }

    d->hw_privint = hw_privint;

    khui_cw_enable_type(d->nct.nc, credtype_id, TRUE);

    kcdb_identity_release(identity);

    khui_cw_notify_identity_state(d->nct.nc, d->nct.hwnd_panel,
                                  NULL, KHUI_CWNIS_READY | KHUI_CWNIS_NOPROGRESS, 0);
    creddlg_refresh_idlist(GetDlgItem(hwnd, IDC_IDLIST), d->ks);
    d->foreign_id = FALSE;

    return TRUE;
}

INT_PTR
creddlg_WMNC_IDENTITY_CHANGE_password(HWND hwnd, struct nc_dialog_data * d)
{
    khm_handle identity = NULL;
    khm_size i;
    khm_int32 id_type = KCDB_CREDTYPE_INVALID;
    khm_size cb = sizeof(id_type);
    khm_handle credset = NULL;

    for (i=0; i < d->n_ks; i++) {
        ks_keystore_release(d->aks[i]);
    }
    if (d->aks)
        free(d->aks);
    if (d->hw_privints)
        free(d->hw_privints);

    d->aks = NULL; d->n_ks = 0; d->hw_privints = NULL;

    if (KHM_FAILED(khui_cw_get_primary_id(d->nct.nc, &identity)) ||
        KHM_FAILED(kcdb_identity_get_attr(identity, KCDB_ATTR_TYPE, NULL, &id_type, &cb)) ||
        (d->n_ks = get_keystores_with_identkey(identity, &d->aks)) == 0) {
        khui_cw_enable_type(d->nct.nc, credtype_id, FALSE);
        kcdb_identity_release(identity);
        return TRUE;
    }

    d->hw_privints = malloc(sizeof(d->hw_privints[0]) * d->n_ks);
    for (i=0; i < d->n_ks; i++) {
        d->cur_idx = i;
        d->hw_privints[i] = CreateDialogParam(hResModule,
                                              MAKEINTRESOURCE(IDD_NC_PRIV_PASSWORD),
                                              GetParent(hwnd),
                                              nc_privint_dlg_proc,
                                              (LPARAM) d);
        khui_cw_show_privileged_dialog(d->nct.nc, credtype_id,
                                       d->hw_privints[i], d->aks[i]->display_name);
        privint_CheckIfReady(d->hw_privints[i]);
    }

    d->nct.type_deps[0] = id_type;
    d->nct.n_type_deps = 1;
    khui_cw_enable_type(d->nct.nc, credtype_id, TRUE);

    if (KHM_FAILED(khui_cw_get_privileged_credential_collector(d->nct.nc, &credset))) {
        kcdb_credset_create(&credset);
        khui_cw_set_privileged_credential_collector(d->nct.nc, credset);
    } else {
        kcdb_credset_flush(credset);
    }

    kcdb_identity_release(identity);
    return TRUE;
}

INT_PTR
creddlg_WMNC_COLLECT_PRIVCRED(HWND hwnd, struct nc_dialog_data * d)
{
    if (khui_cw_get_persist_private_data(d->nct.nc)) {
        khm_handle identity = NULL;
        keystore_t * ks = NULL;
        HWND hw_privint;

        khui_cw_revoke_privileged_dialogs(d->nct.nc, credtype_id);

        khui_cw_get_privileged_credential_store(d->nct.nc, &identity);
        if (identity == NULL) {
            khui_cw_enable_type(d->nct.nc, credtype_id, FALSE);
            return TRUE;
        }

        ks = find_keystore_for_identity(identity);

        if (d->ks)
            ks_keystore_release(d->ks);
        d->ks = ks;
        /* leave ks held */

        if (d->ks == NULL) {
            /* No keystore? */
            assert(FALSE);
            khui_cw_enable_type(d->nct.nc, credtype_id, FALSE);
            kcdb_identity_release(identity);
            return TRUE;
        }

        hw_privint = CreateDialogParam(hResModule,
                                       MAKEINTRESOURCE(IDD_NC_PRIV_STORE),
                                       GetParent(hwnd),
                                       nc_privint_dlg_proc,
                                       (LPARAM) d);
        {
            wchar_t caption[256];
            LoadString(hResModule, IDS_NCPRIV_CAPTION, caption,
                       ARRAYLENGTH(caption));

            khui_cw_show_privileged_dialog(d->nct.nc, credtype_id,
                                           hw_privint, caption);
            privint_CheckIfReady(hw_privint);
        }

        d->hw_privint = hw_privint;

        khui_cw_enable_type(d->nct.nc, credtype_id, TRUE);

        kcdb_identity_release(identity);

        creddlg_refresh_idlist(GetDlgItem(hwnd, IDC_IDLIST), d->ks);

        d->foreign_id = TRUE;

        return TRUE;
    } else {
        khui_cw_revoke_privileged_dialogs(d->nct.nc, credtype_id);
        khui_cw_enable_type(d->nct.nc, credtype_id, FALSE);
        return TRUE;
    }
}

INT_PTR
creddlg_WMNC_DIALOG_PREPROCESS(HWND hwnd, struct nc_dialog_data * d)
{
    khm_handle pid = NULL;
    khm_handle persist_id = NULL;
    khm_int32 ctype_pid = KCDB_CREDTYPE_INVALID;
    khm_int32 ctype_store = KCDB_CREDTYPE_INVALID;
    khm_size cb = sizeof(ctype_pid);

    if (khui_cw_get_subtype(d->nct.nc) == KHUI_NC_SUBTYPE_NEW_CREDS &&

        KHM_SUCCEEDED(khui_cw_get_primary_id(d->nct.nc, &pid)) &&

        KHM_SUCCEEDED(kcdb_identity_get_attr(pid, KCDB_ATTR_TYPE, NULL, &ctype_pid, &cb)) &&

        ctype_pid != credtype_id &&

        khui_cw_get_persist_private_data(d->nct.nc) &&

        KHM_SUCCEEDED(khui_cw_get_privileged_credential_store(d->nct.nc, &persist_id)) &&

        KHM_SUCCEEDED(kcdb_identity_get_attr(persist_id, KCDB_ATTR_TYPE, NULL, &ctype_store, &cb)) &&

        ctype_store == credtype_id) {

        d->nct.n_type_deps = 1;
        d->nct.type_deps[0] = ctype_pid;

    }

    if (pid)
        kcdb_identity_release(pid);

    if (persist_id)
        kcdb_identity_release(persist_id);

    return TRUE;
}

/* Note: This callback runs under the UI thread */
INT_PTR
creddlg_KHUI_WM_NC_NOTIFY(HWND hwnd, khui_wm_nc_notification notification,
                          int sParam, void * vParam)
{
    struct nc_dialog_data * d;

    /* Refer to the khui_wm_nc_notifications enumeration in the
       NetIDMgr SDK for the full list of notification messages that
       can be sent. */

    d = (struct nc_dialog_data *) GetWindowLongPtr(hwnd, DWLP_USER);

    if (!d)
        return TRUE;

    /* this should be set by now */
    assert(d->nct.nc);

    switch (notification) {

    case WMNC_IDENTITY_CHANGE:
        /* Sent when the primary identity associated with the new
           credentials operation has changed. */

        if (khui_cw_get_subtype(d->nct.nc) == KHUI_NC_SUBTYPE_NEW_CREDS)
            return creddlg_WMNC_IDENTITY_CHANGE_new_creds(hwnd, d);
        else
            return creddlg_WMNC_IDENTITY_CHANGE_password(hwnd, d);

    case WMNC_COLLECT_PRIVCRED:
        return creddlg_WMNC_COLLECT_PRIVCRED(hwnd, d);

    case WMNC_DIALOG_PREPROCESS:
        return creddlg_WMNC_DIALOG_PREPROCESS(hwnd, d);

    }

    return TRUE;
}

void
add_identkeys_from_credset(keystore_t * ks, khm_handle credset)
{
    khm_size n_creds = 0;
    khm_size i;

    kcdb_credset_get_size(credset, &n_creds);

    for (i=0; i < n_creds; i++) {
        khm_handle cred = NULL;
        khm_handle identity = NULL;
        khm_handle identpro = NULL;
        wchar_t buf[KCDB_IDENT_MAXCCH_NAME];
        khm_size cb;
        identkey_t * idk = NULL;
        void * data = NULL;

        if (KHM_FAILED(kcdb_credset_get_cred(credset, (khm_int32) i, &cred))) {
            assert(FALSE);
            continue;
        }

        kcdb_cred_get_identity(cred, &identity);
        kcdb_identity_get_identpro(identity, &identpro);

        assert(cred && identity && identpro);

        idk = ks_identkey_create_new();

        cb = sizeof(buf);
        if (KHM_FAILED(kcdb_identity_get_name(identity, buf, &cb)))
            goto done;
        idk->identity_name = _wcsdup(buf);

        cb = sizeof(buf);
        if (KHM_FAILED(kcdb_get_resource(identity, KCDB_RES_DISPLAYNAME,
                                         0, NULL, NULL, buf, &cb)))
            goto done;
        idk->display_name = _wcsdup(buf);

        cb = sizeof(buf);
        if (KHM_FAILED(kcdb_identpro_get_name(identpro, buf, &cb)))
            goto done;
        idk->provider_name = _wcsdup(buf);

        cb = sizeof(idk->ft_ctime);
        kcdb_cred_get_attr(cred, KCDB_ATTR_ISSUE, NULL, &idk->ft_ctime, &cb);
        
        cb = sizeof(idk->ft_expire);
        kcdb_cred_get_attr(cred, KCDB_ATTR_EXPIRE, NULL, &idk->ft_expire, &cb);

        ks_serialize_credential(cred, NULL, &cb);
        if (cb == 0)
            goto done;
        data = malloc(cb);

        ks_serialize_credential(cred, data, &cb);
        ks_datablob_copy(&idk->plain_key, data, cb, 0);

        idk->flags = 0;
        ks_keystore_add_identkey(ks, idk);

        idk = NULL;
    done:
        if (cred) kcdb_cred_release(cred);
        if (identity) kcdb_identity_release(identity);
        if (identpro) kcdb_identpro_release(identpro);
        if (idk) ks_identkey_free(idk);
        if (data) free(data);
    }
}

void
creddlg_prompt_for_new_identity(HWND hwnd, struct nc_dialog_data * d)
{
    khm_handle credset = NULL;
    khm_int32 rv;

    assert(is_keystore_t(d->ks));

    kcdb_credset_create(&credset);
    rv = khui_cw_collect_privileged_credentials(d->nct.nc, NULL, credset);

    if (rv == KHUI_NC_RESULT_PROCESS) {
        add_identkeys_from_credset(d->ks, credset);
    } else {
        assert(FALSE);          /* for debugging */
    }

    kcdb_credset_delete(credset);
}

void
creddlg_prompt_for_configure(HWND hwnd, struct nc_dialog_data * d)
{
}

void
creddlg_remove_selected_identkeys(HWND hwnd, struct nc_dialog_data *d)
{
    int idx = -1;
    HWND hwlist = GetDlgItem(hwnd, IDC_IDLIST);

    if (ListView_GetSelectedCount(hwlist) == 0)
        return;

    /* TODO: show what's going to be removed */

    assert(d->ks);
    KSLOCK(d->ks);
    while ((idx = ListView_GetNextItem(hwlist, idx, LVNI_SELECTED)) != -1) {
        LVITEM lvi = { LVIF_PARAM, idx, 0, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0 };
        ListView_GetItem(hwlist, &lvi);

        ks_keystore_remove_identkey(d->ks, lvi.lParam);
    }
    KSUNLOCK(d->ks);
    creddlg_refresh_idlist(GetDlgItem(hwnd, IDC_IDLIST), d->ks);
}

/* Note: This callback runs under the UI thread */
INT_PTR
creddlg_WM_COMMAND(HWND hwnd, int id, HWND hwCtl, UINT code)
{
    struct nc_dialog_data * d;

    d = (struct nc_dialog_data *) GetWindowLongPtr(hwnd, DWLP_USER);
    if (d == NULL)
        return FALSE;

    if (id == IDC_ADDNEW && code == BN_CLICKED) {
        creddlg_prompt_for_new_identity(hwnd, d);
        creddlg_refresh_idlist(GetDlgItem(hwnd, IDC_IDLIST), d->ks);
        return TRUE;
    }

    if (id == IDC_CONFIGURE && code == BN_CLICKED) {
        creddlg_prompt_for_configure(hwnd, d);
        return TRUE;
    }

    if (id == IDC_REMOVE && code == BN_CLICKED) {
        creddlg_remove_selected_identkeys(hwnd, d);
        return TRUE;
    }

    return FALSE;
}

INT_PTR
creddlg_WM_NOTIFY(HWND hwnd, int wParam, NMHDR * pnmh)
{
    if (pnmh->idFrom == IDC_IDLIST &&
        pnmh->code == LVN_ITEMCHANGED) {

        if (ListView_GetSelectedCount(pnmh->hwndFrom) != 0) {
            EnableWindow(GetDlgItem(hwnd, IDC_REMOVE), TRUE);
            EnableWindow(GetDlgItem(hwnd, IDC_CONFIGURE), TRUE);
        } else {
            EnableWindow(GetDlgItem(hwnd, IDC_REMOVE), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_CONFIGURE), FALSE);
        }

        return TRUE;
    }

    return FALSE;
}

/* Note: This callback runs under the UI thread */
INT_PTR
creddlg_WM_DESTROY(HWND hwnd)
{
    SetWindowLongPtr(hwnd, DWLP_USER, 0);

    return FALSE;
}

/* Dialog procedure for the new credentials panel for our credentials
   type.  We just dispatch messages here to other functions here.

   Note that this procedure runs under the UI thread.
 */
INT_PTR CALLBACK
nc_dlg_proc(HWND hwnd,
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam) {

#ifndef HANDLE_KHUI_WM_NC_NOTIFY
#define HANDLE_KHUI_WM_NC_NOTIFY(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd),HIWORD(wParam),(khui_new_creds *) (lParam)))
#endif

    switch (uMsg) {
#define PHANDLE_MSG(m) \
    case m: return HANDLE_##m((hwnd), (wParam), (lParam), creddlg_##m)
        PHANDLE_MSG(WM_INITDIALOG);
        PHANDLE_MSG(WM_COMMAND);
        PHANDLE_MSG(KHUI_WM_NC_NOTIFY);
        PHANDLE_MSG(WM_DESTROY);
        PHANDLE_MSG(WM_NOTIFY);
#undef PHANDLE_MSG
    }

    return FALSE;
}

/*******************************************************************

The following section contains function stubs for each of the
credentials messages that a credentials provider is likely to want to
handle.  It doesn't include a few messages, but they should be easy to
add.  Please see the documentation for each of the KMSG_CRED_*
messages for documentation on how to handle each of the messages.

********************************************************************/


/* Handler for KMSG_CRED_NEW_CREDS and KMSG_CRED_PASSWORD */
khm_int32
handle_kmsg_cred_new_creds(khui_new_creds * nc) {

    wchar_t wshortdesc[KHUI_MAXCCH_SHORT_DESC];
    size_t cb = 0;
    struct nc_dialog_data * d;

    /* This is a minimal handler that just adds a dialog pane to the
       new credentials window to handle new credentials acquisition
       for this credentials type. */

    /* TODO: add additional initialization etc. as needed */

    d = malloc(sizeof(*d));
    ZeroMemory(d, sizeof(*d));

    d->nct.type = credtype_id;
    d->nct.ordinal = (khm_size) -1;

    LoadString(hResModule, IDS_CT_SHORT_DESC,
               wshortdesc, ARRAYLENGTH(wshortdesc));
    StringCbLength(wshortdesc, sizeof(wshortdesc), &cb);
    assert(cb > 0);
    cb += sizeof(wchar_t);

    d->nct.name = malloc(cb);
    StringCbCopy(d->nct.name, cb, wshortdesc);

    d->nct.h_module = hResModule;
    d->nct.dlg_proc = nc_dlg_proc;
    d->nct.dlg_template = 
        (khui_cw_get_subtype(nc) == KHUI_NC_SUBTYPE_NEW_CREDS)?
        MAKEINTRESOURCE(IDD_NEW_CREDS):
        MAKEINTRESOURCE(IDD_PASSWORD);

    khui_cw_add_type(nc, &d->nct);

    khui_cw_add_selector(nc, idsel_factory, NULL);

    return KHM_ERROR_SUCCESS;
}

/* Handler for KMSG_CRED_RENEW_CREDS */
khm_int32
handle_kmsg_cred_renew_creds(khui_new_creds * nc) {

    struct nc_dialog_data * d;

    /* This is a minimal handler that just adds this credential type
       to the list of credential types that are participating in this
       renewal operation. */

    /* TODO: add additional initialization etc. as needed */

    d = malloc(sizeof(*d));
    ZeroMemory(d, sizeof(*d));

    d->nct.type = credtype_id;

    khui_cw_add_type(nc, &d->nct);

    return KHM_ERROR_SUCCESS;
}

/* Handler for KMSG_CRED_DIALOG_PRESTART */
khm_int32
handle_kmsg_cred_dialog_prestart(khui_new_creds * nc) {
    /* TODO: Handle this message */

    /* The message is sent after the dialog has been created.  The
       window handle for the created dialog can be accessed through
       the hwnd_panel member of the khui_new_creds_by_type structure
       that was added for this credentials type. */
    return KHM_ERROR_SUCCESS;
}

/* Handler for KMSG_CRED_DIALOG_NEW_IDENTITY */
/* Not a message sent out by NetIDMgr.  See documentation of
   KMSG_CRED_DIALOG_NEW_IDENTITY  */
khm_int32
handle_kmsg_cred_dialog_new_identity(khm_ui_4 uparam,
                                     void *   vparam) {
    /* TODO: Handle this message */
    return KHM_ERROR_SUCCESS;
}

/* Handler for KMSG_CRED_DIALOG_NEW_OPTIONS */
/* Not a message sent out by NetIDMgr.  See documentation of
   KMSG_CRED_DIALOG_NEW_OPTIONS */
khm_int32
handle_kmsg_cred_dialog_new_options(khm_ui_4 uparam,
                                    void *   vparam) {
    /* TODO: Handle this message */
    return KHM_ERROR_SUCCESS;
}

void
show_message_for_edit_control(HWND dlg, UINT id_edit,
                              UINT id_caption, UINT id_message, UINT icon)
{
    EDITBALLOONTIP bt;
    wchar_t caption[KHUI_MAXCCH_TITLE];
    wchar_t message[KHUI_MAXCCH_MESSAGE];

    bt.cbStruct = sizeof(bt);
    LoadString(hResModule, id_caption, caption, ARRAYLENGTH(caption));
    LoadString(hResModule, id_message, message, ARRAYLENGTH(message));
    bt.pszTitle = caption;
    bt.pszText = message;
    bt.ttiIcon = icon;

    SendDlgItemMessage(dlg, id_edit, EM_SHOWBALLOONTIP, 0, (LPARAM) &bt);
}

khm_int32
process_keystore_new_credentials(khui_new_creds * nc, HWND hw_privint, keystore_t * ks,
                                 khm_handle key_source, khm_boolean derive_new)
{
    khm_size i;
    khm_boolean ks_was_locked;

    assert(hw_privint);

    ks_was_locked = ks_is_keystore_locked(ks);

    _begin_task(0);
    _report_sr1(KHERR_INFO, IDS_S_NEW_CREDS, _dupstr(ks->display_name));
    _describe();

    if (ks_was_locked) {
        wchar_t pw[KHUI_MAXCCH_PASSWORD] = L"";
        khm_size cb = 0;

        GetDlgItemText(hw_privint, IDC_PASSWORD, pw, ARRAYLENGTH(pw));
        StringCbLength(pw, sizeof(pw), &cb);

        if (cb == 0 && ks_keystore_has_key(ks)) {
            /* Nothing to do.  We already have a key for this keystore
               and the user wants us to use it. */
        } else if (cb == 0) {

            khui_cw_set_response(nc, credtype_id,
                                 KHUI_NC_RESPONSE_NOEXIT | KHUI_NC_RESPONSE_PENDING);
            show_message_for_edit_control(hw_privint, IDC_PASSWORD,
                                          IDS_TNOPASS, IDS_NOPASS, TTI_ERROR);
            _report_sr0(KHERR_ERROR, IDS_TNOPASS);
            _end_task();
            return KHM_ERROR_INVALID_PARAM;

        } else if (KHM_FAILED(ks_keystore_set_key_password(ks, pw, cb))) {

            khui_cw_set_response(nc, credtype_id,
                                 KHUI_NC_RESPONSE_NOEXIT | KHUI_NC_RESPONSE_PENDING);
            show_message_for_edit_control(hw_privint, IDC_PASSWORD,
                                          IDS_TBADPASS, IDS_BADPASS, TTI_ERROR);
            SecureZeroMemory(pw, sizeof(pw));
            _report_sr0(KHERR_ERROR, IDS_TBADPASS);
            _end_task();
            return KHM_ERROR_INVALID_PARAM;
        }

        if (key_source) {
            ks_keystore_unlock(ks);
            add_identkeys_from_credset(ks, key_source);
        }
        ks_keystore_lock(ks);
        if (ks_keystore_get_flags(ks) & KS_FLAG_MODIFIED)
            save_keystore_with_identity(ks);
        khui_cw_set_response(nc, credtype_id,
                             KHUI_NC_RESPONSE_EXIT | KHUI_NC_RESPONSE_SUCCESS);
        SecureZeroMemory(pw, sizeof(pw));
    }

    if (IsDlgButtonChecked(hw_privint, IDC_CHPW) == BST_CHECKED) {

        /* Setting a new password */
        wchar_t pw1[KHUI_MAXCCH_PASSWORD] = L"";
        wchar_t pw2[KHUI_MAXCCH_PASSWORD] = L"";
        khm_size cb = 0;

        GetDlgItemText(hw_privint, IDC_NEWPW1, pw1, ARRAYLENGTH(pw1));
        GetDlgItemText(hw_privint, IDC_NEWPW1, pw2, ARRAYLENGTH(pw2));

        StringCbLength(pw1, sizeof(pw1), &cb);

        if (cb == 0) {
            khui_cw_set_response(nc, credtype_id,
                                 KHUI_NC_RESPONSE_NOEXIT | KHUI_NC_RESPONSE_PENDING);
            show_message_for_edit_control(hw_privint, IDC_NEWPW1,
                                          IDS_TNOPASS, IDS_NOPASS, TTI_ERROR);
            _report_sr0(KHERR_ERROR, IDS_TNOPASS);
            _end_task();
            return KHM_ERROR_INVALID_PARAM;
        } else if (wcscmp(pw1, pw2)) {
            khui_cw_set_response(nc, credtype_id,
                                 KHUI_NC_RESPONSE_NOEXIT | KHUI_NC_RESPONSE_PENDING);
            show_message_for_edit_control(hw_privint, IDC_NEWPW1,
                                          IDS_TMISMPASS, IDS_MISMPASS, TTI_ERROR);
            SecureZeroMemory(pw1, sizeof(pw1));
            SecureZeroMemory(pw2, sizeof(pw2));
            _report_sr0(KHERR_ERROR, IDS_TMISMPASS);
            _end_task();
            return KHM_ERROR_INVALID_PARAM;
        }

        if (!ks_was_locked &&
            KHM_FAILED(ks_keystore_set_key_password(ks, pw1, cb))) {

            khui_cw_set_response(nc, credtype_id,
                                 KHUI_NC_RESPONSE_NOEXIT | KHUI_NC_RESPONSE_PENDING);
            show_message_for_edit_control(hw_privint, IDC_NEWPW1,
                                          IDS_TBADPASS, IDS_BADPASS, TTI_ERROR);
            SecureZeroMemory(pw1, sizeof(pw1));
            SecureZeroMemory(pw2, sizeof(pw2));
            _report_sr0(KHERR_ERROR, IDS_TBADPASS);
            _end_task();
            return KHM_ERROR_INVALID_PARAM;
        } else if (ks_was_locked &&
                   KHM_FAILED(ks_keystore_change_key_password(ks, pw1, cb))) {

            khui_cw_set_response(nc, credtype_id,
                                 KHUI_NC_RESPONSE_NOEXIT | KHUI_NC_RESPONSE_PENDING);
            show_message_for_edit_control(hw_privint, IDC_NEWPW1,
                                          IDS_TUNSPASS, IDS_UNSPASS, TTI_ERROR);
            SecureZeroMemory(pw1, sizeof(pw1));
            SecureZeroMemory(pw2, sizeof(pw2));
            _report_sr0(KHERR_ERROR, IDS_TUNSPASS);
            _end_task();
            return KHM_ERROR_INVALID_PARAM;

        } else {
            if (!ks_was_locked && key_source) {
                add_identkeys_from_credset(ks, key_source);
            }
            ks_keystore_lock(ks);
            if (ks_keystore_get_flags(ks) & KS_FLAG_MODIFIED)
                save_keystore_with_identity(ks);
            khui_cw_set_response(nc, credtype_id,
                                 KHUI_NC_RESPONSE_EXIT | KHUI_NC_RESPONSE_SUCCESS);
            SecureZeroMemory(pw1, sizeof(pw1));
            SecureZeroMemory(pw2, sizeof(pw2));
        }
    }

    /* if we are done with the dialog, we can destroy it so that if
       there are any other privileged interaction panels waiting, they
       can take over */
    DestroyWindow(hw_privint);

    if (!derive_new)
        goto done;

    /* Now go through and derive all possible identities */
    ks_keystore_unlock(ks);
    for (i=0; i < ks->n_keys; i++) {
        identkey_t * idk;
        khm_handle identpro = NULL;
        khm_handle identity = NULL;
        khm_handle credential = NULL;
        khm_handle credset = NULL;

        _progress((khm_ui_4) i, (khm_ui_4) ks->n_keys);

        idk = ks->keys[i];

        if (idk->plain_key.cb_data == 0)
            continue;

        kcdb_identpro_find(idk->provider_name, &identpro);
        if (identpro == NULL) goto done_with_idk;

        kcdb_identity_create_ex(identpro, idk->identity_name,
                                KCDB_IDENT_FLAG_CREATE, NULL, &identity);
        if (identity == NULL) goto done_with_idk;

        kcdb_credset_create(&credset);

        ks_unserialize_credential(idk->plain_key.data, idk->plain_key.cb_data,
                                  &credential);

        if (credset == NULL || credential == NULL)
            goto done_with_idk;

        kcdb_credset_add_cred(credset, credential, -1);

        khui_cw_derive_credentials(nc, identity, credset);
            
    done_with_idk:
        if (identity) kcdb_identity_release(identity);
        if (identpro) kcdb_identpro_release(identpro);
        if (credset) kcdb_credset_delete(credset);
    }
    ks_keystore_lock(ks);

    _progress(1,1);

 done:
    _end_task();

    return KHM_ERROR_SUCCESS;
}

/* Handler for KMSG_CRED_PROCESS */
khm_int32
handle_kmsg_cred_process(khui_new_creds * nc) {
    /* TODO: Handle this message */

    /* This is where the credentials acquisition should be performed
       as determined by the UI.  Note that this message is sent even
       when the user clicks 'cancel'.  The value of nc->result should
       be checked before performing any credentials acquisition.  If
       the value is KHUI_NC_RESULT_CANCEL, then no credentials should
       be acquired.  Otherwise, the value would be
       KHUI_NC_RESULT_PROCESS. */

    struct nc_dialog_data * d;

    khui_cw_find_type(nc, credtype_id, (khui_new_creds_by_type **) &d);

    if (d == NULL) {
        return KHM_ERROR_SUCCESS;
    }

    if (khui_cw_get_result(nc) == KHUI_NC_RESULT_PROCESS) {
        if (khui_cw_get_subtype(nc) == KHUI_NC_SUBTYPE_NEW_CREDS) {
            if (d->ks != NULL) {
                if (d->foreign_id) {
                    khm_handle cs_source = NULL;

                    khui_cw_get_privileged_credential_collector(nc, &cs_source);
                    process_keystore_new_credentials(nc, d->hw_privint, d->ks, cs_source, FALSE);
                } else {
                    process_keystore_new_credentials(nc, d->hw_privint, d->ks, NULL, TRUE);
                }
            }
            return KHM_ERROR_SUCCESS;
        } else if (khui_cw_get_subtype(nc) == KHUI_NC_SUBTYPE_PASSWORD) {
            khm_size i;
            khm_handle credset = NULL;
            khm_boolean failed = FALSE;

            khui_cw_get_privileged_credential_collector(nc, &credset);
            for (i=0; i < d->n_ks; i++) {
                if (KHM_FAILED(process_keystore_new_credentials(nc, d->hw_privints[i],
                                                                d->aks[i], credset, FALSE)))
                    failed = TRUE;
            }

            if (failed)
                khui_cw_set_response(nc, credtype_id,
                                     KHUI_NC_RESPONSE_NOEXIT |
                                     KHUI_NC_RESPONSE_PENDING);
            return KHM_ERROR_SUCCESS;
        }
    } else {
        /* user cancelled */
        if (d->ks)
            ks_keystore_set_flags(d->ks, KS_FLAG_MODIFIED, 0);
        khui_cw_set_response(nc, credtype_id,
                             KHUI_NC_RESPONSE_EXIT | KHUI_NC_RESPONSE_SUCCESS);
        return KHM_ERROR_SUCCESS;
    }

    return KHM_ERROR_SUCCESS;
}

/* Handler for KMSG_CRED_END */
khm_int32
handle_kmsg_cred_end(khui_new_creds * nc) {

    struct nc_dialog_data * d;

    /* TODO: Perform any additional uninitialization as needed. */

    khui_cw_find_type(nc, credtype_id, (khui_new_creds_by_type **) &d);

    if (d) {
        khui_cw_del_type(nc, credtype_id);

        if (d->nct.name)
            free(d->nct.name);

        if (d->ks) {
            ks_keystore_release(d->ks);
            d->ks = NULL;
        }

        free(d);
    }

    return KHM_ERROR_SUCCESS;
}

/* Handler for KMSG_CRED_IMPORT */
khm_int32
handle_kmsg_cred_import(void) {

    /* TODO: Handle this message */

    return KHM_ERROR_SUCCESS;
}


/******************************************************
 Dispatch each message to individual handlers above.
 */
khm_int32 KHMAPI
handle_cred_acq_msg(khm_int32 msg_type,
                    khm_int32 msg_subtype,
                    khm_ui_4  uparam,
                    void *    vparam) {

    khm_int32 rv = KHM_ERROR_SUCCESS;

    switch(msg_subtype) {
    case KMSG_CRED_PASSWORD:
    case KMSG_CRED_NEW_CREDS:
        return handle_kmsg_cred_new_creds((khui_new_creds *) vparam);

    case KMSG_CRED_RENEW_CREDS:
        return handle_kmsg_cred_renew_creds((khui_new_creds *) vparam);

    case KMSG_CRED_DIALOG_PRESTART:
        return handle_kmsg_cred_dialog_prestart((khui_new_creds *) vparam);

    case KMSG_CRED_PROCESS:
        return handle_kmsg_cred_process((khui_new_creds *) vparam);

    case KMSG_CRED_DIALOG_NEW_IDENTITY:
        return handle_kmsg_cred_dialog_new_identity(uparam, vparam);

    case KMSG_CRED_DIALOG_NEW_OPTIONS:
        return handle_kmsg_cred_dialog_new_options(uparam, vparam);

    case KMSG_CRED_END:
        return handle_kmsg_cred_end((khui_new_creds *) vparam);

    case KMSG_CRED_IMPORT:
        return handle_kmsg_cred_import();
    }

    return rv;
}
