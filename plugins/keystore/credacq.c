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
};

INT_PTR
privint_WM_INITDIALOG(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    struct nc_dialog_data * d = NULL;

    d = (struct nc_dialog_data *) lParam;
    assert(d);

#pragma warning(push)
#pragma warning(disable: 4244)
    SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) d);
#pragma warning(pop)

    assert(d->ks);

    if (ks_is_keystore_locked(d->ks)) {
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
    return FALSE;
}

INT_PTR
privint_WM_DESTROY(HWND hwnd)
{
    SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) 0);

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
        LoadString(hResModule, (UINT) columns[i].pszText, buf, ARRAYLENGTH(buf));
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
            i, 0,
            INDEXTOOVERLAYMASK((ks->keys[i]->flags & IDENTKEY_FLAG_LOCKED)?2:1),
            LVIS_OVERLAYMASK,
            ks->keys[i]->display_name, 0,
            0, i, 0, 0, 0, 0};
        wchar_t idtype[KCDB_MAXCCH_NAME] = L"";
        LVITEM lvi_type = {
            LVIF_TEXT,
            i, 1, 0, 0,
            idtype, 0,
            0, 0, 0, 0, 0, 0};
        khm_size cb;
        khm_handle idpro;

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

    creddlg_setup_idlist(GetDlgItem(hwnd, IDC_IDLIST));

    return FALSE;
}

/* Note: This callback runs under the UI thread */
INT_PTR
creddlg_KHUI_WM_NC_NOTIFY(HWND hwnd, int notification, khui_new_creds * nc_in) {

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
            assert(ks);

            if (d->ks)
                ks_keystore_release(d->ks);
            d->ks = ks;
            /* leave ks held */

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
            }

            d->hw_privint = hw_privint;

            khui_cw_enable_type(d->nct.nc, credtype_id, TRUE);

            kcdb_identity_release(identity);

            creddlg_refresh_idlist(GetDlgItem(hwnd, IDC_IDLIST), d->ks);
            return TRUE;
        }

        break;
    }

    return TRUE;
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

            if (KHM_FAILED(kcdb_credset_get_cred(credset, i, &cred))) {
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
            ks_keystore_add_identkey(d->ks, idk);

            idk = NULL;
        done:
            if (cred) kcdb_cred_release(cred);
            if (identity) kcdb_identity_release(identity);
            if (identpro) kcdb_identpro_release(identpro);
            if (idk) ks_identkey_free(idk);
            if (data) free(data);
        }

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

#define HANDLE_KHUI_WM_NC_NOTIFY(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd),HIWORD(wParam),(khui_new_creds *) (lParam)))

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


/* Handler for KMSG_CRED_NEW_CREDS */
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
    d->nct.ordinal = -1;

    LoadString(hResModule, IDS_CT_SHORT_DESC,
               wshortdesc, ARRAYLENGTH(wshortdesc));
    StringCbLength(wshortdesc, sizeof(wshortdesc), &cb);
#ifdef DEBUG
    assert(cb > 0);
#endif
    cb += sizeof(wchar_t);

    d->nct.name = malloc(cb);
    StringCbCopy(d->nct.name, cb, wshortdesc);

    d->nct.h_module = hResModule;
    d->nct.dlg_proc = nc_dlg_proc;
    d->nct.dlg_template = MAKEINTRESOURCE(IDD_NEW_CREDS);

    khui_cw_add_type(nc, &d->nct);

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

    if (d == NULL || d->ks == NULL) {
        return KHM_ERROR_SUCCESS;
    }
    assert(is_keystore_t(d->ks));

    if (khui_cw_get_result(nc) == KHUI_NC_RESULT_PROCESS) {
        khm_size i;
        khm_boolean ks_was_locked;

        assert(d->hw_privint);

        ks_was_locked = ks_is_keystore_locked(d->ks);

        if (ks_was_locked) {
            wchar_t pw[KHUI_MAXCCH_PASSWORD] = L"";
            khm_size cb = 0;

            GetDlgItemText(d->hw_privint, IDC_PASSWORD, pw, ARRAYLENGTH(pw));
            StringCbLength(pw, sizeof(pw), &cb);

            if (cb == 0) {
                khui_cw_set_response(nc, credtype_id,
                                     KHUI_NC_RESPONSE_NOEXIT | KHUI_NC_RESPONSE_PENDING);
                show_message_for_edit_control(d->hw_privint, IDC_PASSWORD,
                                              IDS_TNOPASS, IDS_NOPASS, TTI_ERROR);
                return KHM_ERROR_SUCCESS;
            } if (KHM_FAILED(ks_keystore_set_key_password(d->ks, pw, cb))) {
                khui_cw_set_response(nc, credtype_id,
                                     KHUI_NC_RESPONSE_NOEXIT | KHUI_NC_RESPONSE_PENDING);
                show_message_for_edit_control(d->hw_privint, IDC_PASSWORD,
                                              IDS_TBADPASS, IDS_BADPASS, TTI_ERROR);
                SecureZeroMemory(pw, sizeof(pw));
                return KHM_ERROR_SUCCESS;
            } else {
                ks_keystore_lock(d->ks);
                if (ks_keystore_get_flags(d->ks) & KS_FLAG_MODIFIED)
                    save_keystore_with_identity(d->ks);
                khui_cw_set_response(nc, credtype_id,
                                     KHUI_NC_RESPONSE_EXIT | KHUI_NC_RESPONSE_SUCCESS);
                SecureZeroMemory(pw, sizeof(pw));
            }
        }

        if (IsDlgButtonChecked(d->hw_privint, IDC_CHPW) == BST_CHECKED) {

            /* Setting a new password */
            wchar_t pw1[KHUI_MAXCCH_PASSWORD] = L"";
            wchar_t pw2[KHUI_MAXCCH_PASSWORD] = L"";
            khm_size cb = 0;

            GetDlgItemText(d->hw_privint, IDC_NEWPW1, pw1, ARRAYLENGTH(pw1));
            GetDlgItemText(d->hw_privint, IDC_NEWPW1, pw2, ARRAYLENGTH(pw2));

            StringCbLength(pw1, sizeof(pw1), &cb);

            if (cb == 0) {
                khui_cw_set_response(nc, credtype_id,
                                     KHUI_NC_RESPONSE_NOEXIT | KHUI_NC_RESPONSE_PENDING);
                show_message_for_edit_control(d->hw_privint, IDC_NEWPW1,
                                              IDS_TNOPASS, IDS_NOPASS, TTI_ERROR);
                ks_keystore_reset_key(d->ks);
                return KHM_ERROR_SUCCESS;
            } else if (wcscmp(pw1, pw2)) {
                khui_cw_set_response(nc, credtype_id,
                                     KHUI_NC_RESPONSE_NOEXIT | KHUI_NC_RESPONSE_PENDING);
                show_message_for_edit_control(d->hw_privint, IDC_NEWPW1,
                                              IDS_TMISMPASS, IDS_MISMPASS, TTI_ERROR);
                SecureZeroMemory(pw1, sizeof(pw1));
                SecureZeroMemory(pw2, sizeof(pw2));
                ks_keystore_reset_key(d->ks);
                return KHM_ERROR_SUCCESS;
            }

            if (!ks_was_locked &&
                KHM_FAILED(ks_keystore_set_key_password(d->ks, pw1, cb))) {

                khui_cw_set_response(nc, credtype_id,
                                     KHUI_NC_RESPONSE_NOEXIT | KHUI_NC_RESPONSE_PENDING);
                show_message_for_edit_control(d->hw_privint, IDC_NEWPW1,
                                              IDS_TBADPASS, IDS_BADPASS, TTI_ERROR);
                SecureZeroMemory(pw1, sizeof(pw1));
                SecureZeroMemory(pw2, sizeof(pw2));
                ks_keystore_reset_key(d->ks);
                return KHM_ERROR_SUCCESS;
            } else if (ks_was_locked &&
                       KHM_FAILED(ks_keystore_change_key_password(d->ks, pw1, cb))) {

                khui_cw_set_response(nc, credtype_id,
                                     KHUI_NC_RESPONSE_NOEXIT | KHUI_NC_RESPONSE_PENDING);
                show_message_for_edit_control(d->hw_privint, IDC_NEWPW1,
                                              IDS_TUNSPASS, IDS_UNSPASS, TTI_ERROR);
                SecureZeroMemory(pw1, sizeof(pw1));
                SecureZeroMemory(pw2, sizeof(pw2));
                ks_keystore_reset_key(d->ks);
                return KHM_ERROR_SUCCESS;

            } else {
                ks_keystore_lock(d->ks);
                if (ks_keystore_get_flags(d->ks) & KS_FLAG_MODIFIED)
                    save_keystore_with_identity(d->ks);
                khui_cw_set_response(nc, credtype_id,
                                     KHUI_NC_RESPONSE_EXIT | KHUI_NC_RESPONSE_SUCCESS);
                SecureZeroMemory(pw1, sizeof(pw1));
                SecureZeroMemory(pw2, sizeof(pw2));
            }
        }

        /* Now go through and derive all possible identities */
        ks_keystore_unlock(d->ks);
        for (i=0; i < d->ks->n_keys; i++) {
            identkey_t * idk;
            khm_handle identpro = NULL;
            khm_handle identity = NULL;
            khm_handle credential = NULL;
            khm_handle credset = NULL;

            idk = d->ks->keys[i];

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

            khui_cw_derive_credentials(d->nct.nc, identity, credset);
            
        done_with_idk:
            if (identity) kcdb_identity_release(identity);
            if (identpro) kcdb_identpro_release(identpro);
            if (credset) kcdb_credset_delete(credset);
        }
        ks_keystore_lock(d->ks);
        ks_keystore_reset_key(d->ks);

        return KHM_ERROR_SUCCESS;

    } else {
        /* user cancelled */
        ks_keystore_set_flags(d->ks, KS_FLAG_MODIFIED, 0);
        khui_cw_set_response(nc, credtype_id,
                             KHUI_NC_RESPONSE_EXIT | KHUI_NC_RESPONSE_SUCCESS);
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
