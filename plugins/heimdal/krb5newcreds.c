/*
 * Copyright (c) 2010 Secure Endpoints Inc.
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

#include "krbcred.h"
#include<strsafe.h>

#include<commctrl.h>

#include<assert.h>


INT_PTR
k5_handle_wm_initdialog(HWND hwnd,
                        WPARAM wParam,
                        LPARAM lParam)
{
    HWND hw;
    k5_dlg_data * d;
    khui_new_creds * nc;

    /* lParam is a pointer to a khui_new_creds structure */
    nc = (khui_new_creds *) lParam;
    khui_cw_find_type(nc, credtype_id_krb5, (khui_new_creds_by_type **) &d);

#pragma warning(push)
#pragma warning(disable: 4244)
    SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) d);
#pragma warning(pop)

    if (khui_cw_get_subtype(d->nct.nc) == KHUI_NC_SUBTYPE_NEW_CREDS ||
        khui_cw_get_subtype(d->nct.nc) == KHUI_NC_SUBTYPE_ACQPRIV_ID) {
        khui_tracker_initialize(&d->tc_lifetime);
        khui_tracker_initialize(&d->tc_renew);

        hw = GetDlgItem(hwnd, IDC_NCK5_LIFETIME_EDIT);
        khui_tracker_install(hw, &d->tc_lifetime);

        hw = GetDlgItem(hwnd, IDC_NCK5_RENEW_EDIT);
        khui_tracker_install(hw, &d->tc_renew);
    }
    return TRUE;
}

INT_PTR
k5_handle_wm_destroy(HWND hwnd,
                     WPARAM wParam,
                     LPARAM lParam)
{
    k5_dlg_data * d;

    d = (k5_dlg_data *) (LONG_PTR)
        GetWindowLongPtr(hwnd, DWLP_USER);
    if (!d)
        return TRUE;

    if (khui_cw_get_subtype(d->nct.nc) == KHUI_NC_SUBTYPE_NEW_CREDS ||
        khui_cw_get_subtype(d->nct.nc) == KHUI_NC_SUBTYPE_ACQPRIV_ID) {
        khui_tracker_kill_controls(&d->tc_renew);
        khui_tracker_kill_controls(&d->tc_lifetime);
    }

    SetWindowLongPtr(hwnd, DWLP_USER, 0);

    return TRUE;
}

LRESULT
k5_force_password_change(k5_dlg_data * d) {
    /* we are turning this dialog into a change password dialog... */
    wchar_t wbuf[KHUI_MAXCCH_BANNER];

    khui_cw_clear_prompts(d->nct.nc);

    LoadString(hResModule, IDS_NC_PWD_BANNER,
               wbuf, ARRAYLENGTH(wbuf));
    khui_cw_begin_custom_prompts(d->nct.nc, 3, NULL, wbuf);

    LoadString(hResModule, IDS_NC_PWD_PWD,
               wbuf, ARRAYLENGTH(wbuf));
    khui_cw_add_prompt(d->nct.nc, KHUI_NCPROMPT_TYPE_PASSWORD,
                       wbuf, NULL, KHUI_NCPROMPT_FLAG_HIDDEN);

    LoadString(hResModule, IDS_NC_PWD_NPWD,
               wbuf, ARRAYLENGTH(wbuf));
    khui_cw_add_prompt(d->nct.nc, KHUI_NCPROMPT_TYPE_NEW_PASSWORD,
                       wbuf, NULL, KHUI_NCPROMPT_FLAG_HIDDEN);

    LoadString(hResModule, IDS_NC_PWD_NPWD_AGAIN,
               wbuf, ARRAYLENGTH(wbuf));
    khui_cw_add_prompt(d->nct.nc, KHUI_NCPROMPT_TYPE_NEW_PASSWORD_AGAIN,
                       wbuf, NULL, KHUI_NCPROMPT_FLAG_HIDDEN);

    d->pwd_change = TRUE;
    d->sync = TRUE;

    khui_cw_notify_identity_state(d->nct.nc, d->nct.hwnd_panel, NULL,
                                  KHUI_CWNIS_READY | KHUI_CWNIS_NOPROGRESS, 0);

    return TRUE;
}

INT_PTR
k5_handle_wmnc_notify(HWND hwnd,
                      WPARAM wParam,
                      LPARAM lParam)
{
    k5_dlg_data * d;

    d = (k5_dlg_data *)(LONG_PTR)
        GetWindowLongPtr(hwnd, DWLP_USER);

    if (d == NULL)
        return TRUE;

    switch(HIWORD(wParam)) {
    case WMNC_DIALOG_MOVE:
        {
            if (khui_cw_get_subtype(d->nct.nc) == KHUI_NC_SUBTYPE_NEW_CREDS ||
                khui_cw_get_subtype(d->nct.nc) == KHUI_NC_SUBTYPE_ACQPRIV_ID) {
                khui_tracker_reposition(&d->tc_lifetime);
                khui_tracker_reposition(&d->tc_renew);
            }

            return TRUE;
        }
        break;

    case WMNC_DIALOG_SETUP:
        {
            BOOL old_sync;

            if (khui_cw_get_subtype(d->nct.nc) == KMSG_CRED_PASSWORD)
                return TRUE;

            /* we save the value of the 'sync' field here because some
               of the notifications that are generated while setting
               the controls overwrite the field. */
            old_sync = d->sync;

            SendDlgItemMessage(hwnd, IDC_NCK5_RENEWABLE,
                               BM_SETCHECK,
			       (d->params.renewable? BST_CHECKED : BST_UNCHECKED),
                               0);
            EnableWindow(GetDlgItem(hwnd, IDC_NCK5_RENEW_EDIT),
                         !!d->params.renewable);

            khui_tracker_refresh(&d->tc_lifetime);
            khui_tracker_refresh(&d->tc_renew);

            SendDlgItemMessage(hwnd, IDC_NCK5_FORWARDABLE,
                               BM_SETCHECK,
                               (d->params.forwardable ? BST_CHECKED : BST_UNCHECKED),
                               0);

            SendDlgItemMessage(hwnd, IDC_NCK5_PROXIABLE,
                               BM_SETCHECK,
                               (d->params.proxiable ? BST_CHECKED : BST_UNCHECKED),
                               0);

            SendDlgItemMessage(hwnd, IDC_NCK5_ADDRESS,
                               BM_SETCHECK,
                               (d->params.addressless ? BST_CHECKED : BST_UNCHECKED),
                               0);

            SendDlgItemMessage(hwnd, IDC_NCK5_PUBLICIP,
                               IPM_SETADDRESS,
                               0, d->params.publicIP);

            EnableWindow(GetDlgItem(hwnd, IDC_NCK5_PUBLICIP), !d->params.addressless);

            d->sync = old_sync;
        }
        break;

    case WMNC_CREDTEXT_LINK:
        {
            khui_htwnd_link * l;
            khui_new_creds * nc;
            wchar_t linktext[128];

            nc = d->nct.nc;
            l = (khui_htwnd_link *) lParam;

            if (!l)
                break;

            StringCchCopyN(linktext, ARRAYLENGTH(linktext),
                           l->id, l->id_len);

            if (!wcscmp(linktext, L"Krb5Cred:!Passwd")) {
                return k5_force_password_change(d);
            }
        }
        break;

    case WMNC_IDENTITY_CHANGE:
        {
            /* There has been a change of identity */
            if (khui_cw_get_subtype(d->nct.nc) == KHUI_NC_SUBTYPE_PASSWORD) {
                khm_handle p_identity = NULL;

                if (KHM_FAILED(khui_cw_get_primary_id(d->nct.nc, &p_identity)) ||
                    !kcdb_identity_by_provider(p_identity, L"Krb5Ident")) {
                    khui_cw_enable_type(d->nct.nc, credtype_id_krb5, FALSE);
                } else {
                    k5_force_password_change(d);
                }

                if (p_identity)
                    kcdb_identity_release(p_identity);
            } else {
                kmq_post_sub_msg(k5_sub, KMSG_CRED,
                                 KMSG_CRED_DIALOG_NEW_IDENTITY,
                                 0, (void *) d->nct.nc);
            }
        }
        break;

    case WMNC_DIALOG_PREPROCESS:
        {
            if(!d->sync && khui_cw_get_result(d->nct.nc) == KHUI_NC_RESULT_PROCESS) {
                kmq_post_sub_msg(k5_sub, KMSG_CRED,
                                 KMSG_CRED_DIALOG_NEW_OPTIONS,
                                 0, (void *) d->nct.nc);
            }
        }
        break;
    }

    return 0;
}

INT_PTR
k5_handle_wm_notify(HWND hwnd,
                    WPARAM wParam,
                    LPARAM lParam)
{
    LPNMHDR pnmh;
    k5_dlg_data * d;

    pnmh = (LPNMHDR) lParam;
    if (pnmh->idFrom == IDC_NCK5_PUBLICIP &&
        pnmh->code == IPN_FIELDCHANGED) {

        d = (k5_dlg_data *) (LONG_PTR) GetWindowLongPtr(hwnd, DWLP_USER);
        if (d == NULL)
            return 0;

        SendDlgItemMessage(hwnd, IDC_NCK5_PUBLICIP,
                           IPM_GETADDRESS,
                           0, (LPARAM) &d->params.publicIP);

        d->dirty = TRUE;
        d->sync = FALSE;

        return TRUE;
    }

    return 0;
}

INT_PTR
k5_handle_wm_command(HWND hwnd,
                     WPARAM wParam,
                     LPARAM lParam)
{
    k5_dlg_data * d;
    int c;

    d = (k5_dlg_data *)(LONG_PTR) GetWindowLongPtr(hwnd, DWLP_USER);
    if (d == NULL)
        return FALSE;

    switch(wParam) {
    case MAKEWPARAM(IDC_NCK5_RENEWABLE, BN_CLICKED):

        c = (int) SendDlgItemMessage(hwnd, IDC_NCK5_RENEWABLE,
                                     BM_GETCHECK, 0, 0);
        d->params.renewable = (c == BST_CHECKED);
        EnableWindow(GetDlgItem(hwnd, IDC_NCK5_RENEW_EDIT), d->params.renewable);
        break;

    case MAKEWPARAM(IDC_NCK5_FORWARDABLE, BN_CLICKED):

        c = (int) SendDlgItemMessage(hwnd, IDC_NCK5_FORWARDABLE,
                                     BM_GETCHECK, 0, 0);
        d->params.forwardable = (c == BST_CHECKED);
        break;

    case MAKEWPARAM(IDC_NCK5_PROXIABLE, BN_CLICKED):

        c = (int) SendDlgItemMessage(hwnd, IDC_NCK5_PROXIABLE,
                                     BM_GETCHECK, 0, 0);
        d->params.proxiable = (c == BST_CHECKED);
        break;

    case MAKEWPARAM(IDC_NCK5_ADDRESS, BN_CLICKED):

        c = (int) SendDlgItemMessage(hwnd, IDC_NCK5_ADDRESS,
                                     BM_GETCHECK, 0, 0);
        d->params.addressless = (c == BST_CHECKED);
        EnableWindow(GetDlgItem(hwnd, IDC_NCK5_PUBLICIP), !d->params.addressless);
        break;

    case MAKEWPARAM(IDC_NCK5_RENEW_EDIT, EN_CHANGE):
    case MAKEWPARAM(IDC_NCK5_LIFETIME_EDIT, EN_CHANGE):
        break;

    default:
        return FALSE;
    }

    d->dirty = TRUE;
    d->sync = FALSE;

    return TRUE;
}

/*  Dialog procedure for the Krb5 credentials type panel.

    NOTE: Runs in the context of the UI thread
*/
INT_PTR CALLBACK
k5_nc_dlg_proc(HWND hwnd,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    switch(uMsg) {
    case WM_INITDIALOG:
        return k5_handle_wm_initdialog(hwnd, wParam, lParam);

    case WM_COMMAND:
        return k5_handle_wm_command(hwnd, wParam, lParam);

    case KHUI_WM_NC_NOTIFY:
        return k5_handle_wmnc_notify(hwnd, wParam, lParam);

    case WM_NOTIFY:
        return k5_handle_wm_notify(hwnd, wParam, lParam);

    case WM_DESTROY:
        return k5_handle_wm_destroy(hwnd, wParam, lParam);
    }
    return FALSE;
}

void
k5_read_dlg_params(k5_dlg_data * d, khm_handle identity)
{
    khui_action_context * pctx = NULL;

    khm_krb5_get_identity_params(identity, &d->params);

    d->tc_lifetime.current = d->params.lifetime;
    d->tc_lifetime.max = d->params.lifetime_max;
    d->tc_lifetime.min = d->params.lifetime_min;

    d->tc_renew.current = d->params.renew_life;
    d->tc_renew.max = d->params.renew_life_max;
    d->tc_renew.min = d->params.renew_life_min;

    /* however, if this has externally supplied defaults, we have to
       use them too. */
    if (d->nct.nc &&
        (pctx = khui_cw_get_ctx(d->nct.nc)) != NULL &&
        pctx->vparam &&
        pctx->cb_vparam == sizeof(NETID_DLGINFO)) {
        LPNETID_DLGINFO pdlginfo;

        pdlginfo = (LPNETID_DLGINFO) pctx->vparam;
        if (pdlginfo->size == NETID_DLGINFO_V1_SZ &&
            pdlginfo->in.use_defaults == 0) {
            d->params.forwardable = pdlginfo->in.forwardable;
            d->params.addressless = pdlginfo->in.noaddresses;
            d->tc_lifetime.current = pdlginfo->in.lifetime;
            d->tc_renew.current = pdlginfo->in.renew_till;

            if (pdlginfo->in.renew_till == 0)
                d->params.renewable = FALSE;
            else
                d->params.renewable = TRUE;

            d->params.proxiable = pdlginfo->in.proxiable;
            d->params.publicIP = pdlginfo->in.publicip;
        }
    }

    /* once we read the new data, in, it is no longer considered
       dirty */
    d->dirty = FALSE;
    d->sync = TRUE;
}

void
k5_ensure_identity_ccache_is_watched(khm_handle identity, char * ccache)
{
    /* if we used a FILE: ccache, we should add it to FileCCList.
       Otherwise the tickets are not going to get listed. */
    do {
        wchar_t thisccache[MAX_PATH];
        wchar_t * ccpath;
        khm_size cb_cc;
        wchar_t * mlist = NULL;
        khm_size cb_mlist;
        khm_int32 rv;
        khm_size t;

        if (ccache != NULL &&
            strncmp(ccache, "FILE:", 5) != 0)
            break;

        if (ccache == NULL) {
            cb_cc = sizeof(thisccache);
            rv = khm_krb5_get_identity_default_ccache(identity, thisccache, &cb_cc);
#ifdef DEBUG
            assert(KHM_SUCCEEDED(rv));
#endif
        } else {
            thisccache[0] = L'\0';
            AnsiStrToUnicode(thisccache, sizeof(thisccache), ccache);
        }

        if (wcsncmp(thisccache, L"FILE:", 5))
            break;

        /* the FileCCList is a list of paths.  We have to strip out
           the FILE: prefix. */
        ccpath = thisccache + 5;
        unexpand_env_var_prefix(ccpath, sizeof(thisccache) - sizeof(wchar_t) * 5);

        _reportf(L"Checking if ccache [%s] is in FileCCList", ccpath);

        StringCbLength(ccpath, sizeof(thisccache) - sizeof(wchar_t) * 5, &cb_cc);
        cb_cc += sizeof(wchar_t);

        rv = khc_read_multi_string(csp_params, L"FileCCList", NULL, &cb_mlist);
        if (rv == KHM_ERROR_TOO_LONG && cb_mlist > sizeof(wchar_t) * 2) {
            wchar_t * cc = NULL;

            cb_mlist += cb_cc;
            mlist = PMALLOC(cb_mlist);

            t = cb_mlist;
            rv = khc_read_multi_string(csp_params, L"FileCCList", mlist, &t);
#ifdef DEBUG
            assert(KHM_SUCCEEDED(rv));
#endif
            if (KHM_FAILED(rv))
                goto failed_filecclist;

            for (cc = mlist;
                 cc && *cc;
                 cc = multi_string_next(cc)) {

                wchar_t tcc[MAX_PATH];

                StringCbCopy(tcc, sizeof(tcc), cc);
                unexpand_env_var_prefix(tcc, sizeof(tcc));

                if (!_wcsicmp(tcc, ccpath))
                    break;
            }

            if (cc == NULL || !*cc) {
                t = cb_mlist;
                multi_string_append(mlist, &t, ccpath);

                khc_write_multi_string(csp_params, L"FileCCList", mlist);
                _reportf(L"Added CCache to list");
            } else {
                _reportf(L"The CCache is already in the list");
            }
        } else {
            cb_mlist = cb_cc + sizeof(wchar_t);
            mlist = PMALLOC(cb_mlist);

            multi_string_init(mlist, cb_mlist);
            t = cb_mlist;
            multi_string_append(mlist, &t, ccpath);

            khc_write_multi_string(csp_params, L"FileCCList", mlist);

            _reportf(L"FileCCList was empty.  Added CCache");
        }

    failed_filecclist:

        if (mlist)
            PFREE(mlist);

    } while(FALSE);
}

void
k5_write_dlg_params(k5_dlg_data * d, khm_handle identity, char * ccache)
{

    d->params.source_reg = K5PARAM_FM_ALL; /* we want to write all the
                                              settings to the
                                              registry, if
                                              necessary. */

    d->params.lifetime = (krb5_deltat) d->tc_lifetime.current;
    d->params.renew_life = (krb5_deltat) d->tc_renew.current;

    khm_krb5_set_identity_params(identity, &d->params);

    k5_ensure_identity_ccache_is_watched(identity, ccache);

    /* as in k5_read_dlg_params, once we write the data in, the local
       data is no longer dirty */
    d->dirty = FALSE;
}


static khm_int32 KHMAPI
k5_find_tgt_filter(khm_handle cred,
                   khm_int32 flags,
                   void * rock) {
    khm_handle ident = (khm_handle) rock;
    khm_handle cident = NULL;
    khm_int32 f;
    khm_int32 rv;

    if (KHM_SUCCEEDED(kcdb_cred_get_identity(cred,
                                             &cident)) &&
        cident == ident &&
        KHM_SUCCEEDED(kcdb_cred_get_flags(cred, &f)) &&
        (f & KCDB_CRED_FLAG_INITIAL) &&
        !(f & KCDB_CRED_FLAG_EXPIRED))
        rv = 1;
    else
        rv = 0;

    if (cident)
        kcdb_identity_release(cident);

    return rv;
}

khm_int32
k5_remove_from_LRU(khm_handle identity)
{
    wchar_t * wbuf = NULL;
    wchar_t idname[KCDB_IDENT_MAXCCH_NAME];
    khm_size cb;
    khm_size cb_ms;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    cb = sizeof(idname);
    rv = kcdb_identity_get_name(identity, idname, &cb);
    assert(rv == KHM_ERROR_SUCCESS);

    rv = khc_read_multi_string(csp_params, L"LRUPrincipals", NULL, &cb_ms);
    if (rv != KHM_ERROR_TOO_LONG)
        cb_ms = sizeof(wchar_t) * 2;

    wbuf = PMALLOC(cb_ms);
    assert(wbuf);

    cb = cb_ms;

    if (rv == KHM_ERROR_TOO_LONG) {
        rv = khc_read_multi_string(csp_params, L"LRUPrincipals", wbuf, &cb);
        assert(KHM_SUCCEEDED(rv));

        if (multi_string_find(wbuf, idname, KHM_CASE_SENSITIVE) != NULL) {
            multi_string_delete(wbuf, idname, KHM_CASE_SENSITIVE);
        }
    } else {
        multi_string_init(wbuf, cb_ms);
    }

    rv = khc_write_multi_string(csp_params, L"LRUPrincipals", wbuf);

    if (wbuf)
        PFREE(wbuf);

    return rv;
}

khm_int32
k5_update_LRU(khm_handle identity)
{
    wchar_t * wbuf = NULL;
    wchar_t * idname = NULL;
    wchar_t * realm = NULL;
    khm_size cb;
    khm_size cb_ms;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    rv = kcdb_identity_get_name(identity, NULL, &cb);
    assert(rv == KHM_ERROR_TOO_LONG);

    idname = PMALLOC(cb);
    assert(idname);

    rv = kcdb_identity_get_name(identity, idname, &cb);
    assert(KHM_SUCCEEDED(rv));

    rv = khc_read_multi_string(csp_params, L"LRUPrincipals", NULL, &cb_ms);
    if (rv != KHM_ERROR_TOO_LONG)
        cb_ms = cb + sizeof(wchar_t);
    else
        cb_ms += cb + sizeof(wchar_t);

    wbuf = PMALLOC(cb_ms);
    assert(wbuf);

    cb = cb_ms;

    if (rv == KHM_ERROR_TOO_LONG) {
        rv = khc_read_multi_string(csp_params, L"LRUPrincipals", wbuf, &cb);
        assert(KHM_SUCCEEDED(rv));

        if (multi_string_find(wbuf, idname, KHM_CASE_SENSITIVE) != NULL) {
            /* it's already there.  We remove it here and add it at
               the top of the LRU list. */
            multi_string_delete(wbuf, idname, KHM_CASE_SENSITIVE);
        }
    } else {
        multi_string_init(wbuf, cb_ms);
    }

    cb = cb_ms;
    rv = multi_string_prepend(wbuf, &cb, idname);
    assert(KHM_SUCCEEDED(rv));

    rv = khc_write_multi_string(csp_params, L"LRUPrincipals", wbuf);

    realm = khm_get_realm_from_princ(idname);
    if (realm == NULL || *realm == L'\0')
        goto _done_with_LRU;

    cb = cb_ms;
    rv = khc_read_multi_string(csp_params, L"LRURealms", wbuf, &cb);

    if (rv == KHM_ERROR_TOO_LONG) {
        PFREE(wbuf);
        wbuf = PMALLOC(cb);
        assert(wbuf);

        cb_ms = cb;

        rv = khc_read_multi_string(csp_params, L"LRURealms", wbuf, &cb);

        assert(KHM_SUCCEEDED(rv));
    } else if (rv == KHM_ERROR_SUCCESS) {
        if (multi_string_find(wbuf, realm, KHM_CASE_SENSITIVE) != NULL) {
            /* remove the realm and add it at the top later. */
            multi_string_delete(wbuf, realm, KHM_CASE_SENSITIVE);
        }
    } else {
        multi_string_init(wbuf, cb_ms);
    }

    cb = cb_ms;
    rv = multi_string_prepend(wbuf, &cb, realm);

    if (rv == KHM_ERROR_TOO_LONG) {
        wbuf = PREALLOC(wbuf, cb);

        rv = multi_string_prepend(wbuf, &cb, realm);

        assert(KHM_SUCCEEDED(rv));
    }

    rv = khc_write_multi_string(csp_params, L"LRURealms", wbuf);

    assert(KHM_SUCCEEDED(rv));

 _done_with_LRU:

    if (wbuf)
        PFREE(wbuf);
    if (idname)
        PFREE(idname);

    return rv;
}

void
k5_reply_to_acqpriv_id_request(khui_new_creds * nc,
                               krb5_data * privdata)
{
    khm_handle credset = NULL;
    khm_handle cred = NULL;
    khm_handle identity = NULL;
    wchar_t idname[KCDB_IDENT_MAXCCH_NAME];
    khm_size cb;
    FILETIME ft;

    khui_cw_get_privileged_credential_collector(nc, &credset);
    if (credset == NULL)
        return;

    khui_cw_get_primary_id(nc, &identity);
    assert(identity);

    cb = sizeof(idname);
    kcdb_identity_get_name(identity, idname, &cb);

    kcdb_cred_create(idname, identity, credtype_id_krb5, &cred);
    GetSystemTimeAsFileTime(&ft);
    kcdb_cred_set_attr(cred, KCDB_ATTR_ISSUE, &ft, sizeof(ft));
    kcdb_cred_set_attr(cred, attr_id_krb5_pkeyv1, privdata->data, privdata->length);

    kcdb_credset_flush(credset);
    kcdb_credset_add_cred(credset, cred, -1);

    kcdb_cred_release(cred);
    kcdb_identity_release(identity);
}

void
k5_handle_process_password(khui_new_creds * nc,
                           k5_dlg_data * d);

void
k5_handle_process_new_creds(khui_new_creds *nc,
                            k5_dlg_data    *d)
{
    khm_handle ident = NULL;
    khm_int32 r = 0;

    if (d->pwd_change) {
        /* we are forcing a password change */

        k5_handle_process_password(nc, d);
        return;
    }

    _begin_task(0);
    _report_mr0(KHERR_NONE, MSG_CTX_INITAL_CREDS);
    _describe();

    _progress(0,1);

    if(khui_cw_get_result(nc) == KHUI_NC_RESULT_CANCEL) {
        _reportf(L"Cancelling");
        k5_kinit_task_abort_and_release(d->kinit_task);
        d->kinit_task = NULL;
    } else if (khui_cw_get_result(nc) == KHUI_NC_RESULT_PROCESS) {
        if (d->kinit_task == NULL ||
            KHM_FAILED(k5_kinit_task_confirm_and_wait(d->kinit_task))) {

            d->kinit_task = NULL;
        } else {

            _progress(1,2);

            assert(d->kinit_task->state == K5_KINIT_STATE_WAIT ||
                   d->kinit_task->state == K5_KINIT_STATE_DONE);

            if (d->kinit_task->state == K5_KINIT_STATE_WAIT) {
                /* We are showing another set of prompts.  We can't
                   proceed with credentials acquisition yet. */

                _reportf(L"Further prompting required.");

                khui_cw_set_response(nc, credtype_id_krb5,
                                     KHUI_NC_RESPONSE_NOEXIT |
                                     KHUI_NC_RESPONSE_PENDING);
                goto done;
            }
        }
    }

    khui_cw_get_primary_id(nc, &ident);

    /* special case: if there was no password entered, and if there is
       a valid TGT we allow the credential acquisition to go
       through */
    if (khui_cw_get_subtype(nc) == KHUI_NC_SUBTYPE_NEW_CREDS &&
        d->kinit_task &&
        d->kinit_task->kinit_code != 0 &&
        d->kinit_task->is_null_password &&
        ident != NULL &&
        KHM_SUCCEEDED(kcdb_credset_find_filtered (NULL, -1, k5_find_tgt_filter,
                                                  ident, NULL, NULL))) {
        _reportf(L"No password entered, but a valid TGT exists. Continuing");
        d->kinit_task->kinit_code = 0;
    } else if (d->kinit_task &&
               d->kinit_task->kinit_code == 0 &&
               ident != NULL) {

        /* we had a password and we used it to get tickets.  We should
           reset the IMPORTED flag now since the tickets are not
           imported. */

        khm_krb5_set_identity_flags(ident, K5IDFLAG_IMPORTED, 0);
    }

    if(d->kinit_task && d->kinit_task->kinit_code != 0) {
        wchar_t tbuf[1024];
        DWORD suggestion = 0;
        kherr_suggestion suggest_code = 0;

        khm_err_describe(d->kinit_task->context,
			 d->kinit_task->kinit_code,
                         tbuf, sizeof(tbuf),
                         &suggestion, &suggest_code);

        _report_cs0(KHERR_ERROR, tbuf);
        if (suggestion != 0)
            _suggest_mr(suggestion, suggest_code);

        _resolve();

        r = KHUI_NC_RESPONSE_FAILED;

        if (suggest_code == KHERR_SUGGEST_RETRY) {
            r |= KHUI_NC_RESPONSE_NOEXIT |
                KHUI_NC_RESPONSE_PENDING;
        }

        if (d->kinit_task->is_valid_principal && ident) {
            /* the principal was valid, so we can go ahead and update
               the LRU */
            k5_update_LRU(ident);
        }

    } else if (d->kinit_task &&
               khui_cw_get_result(nc) == KHUI_NC_RESULT_PROCESS) {
        krb5_context ctx = NULL;

        _reportf(L"Tickets successfully acquired");

        r = KHUI_NC_RESPONSE_SUCCESS |
            KHUI_NC_RESPONSE_EXIT;

        /* if we successfully obtained credentials, we should save the
           current settings in the identity config space */

        assert(ident != NULL);

        k5_write_dlg_params(d, ident, d->kinit_task->ccache);

        /* We should also quickly refresh the credentials so that the
           identity flags and ccache properties reflect the current
           state of affairs.  This has to be done here so that other
           credentials providers which depend on Krb5 can properly
           find the initial creds to obtain their respective creds. */

        khm_krb5_list_tickets(&ctx);

        if (khui_cw_get_use_as_default(nc)) {
            _reportf(L"Setting default identity");
            kcdb_identity_set_default(ident);
        }

        /* If there is no default identity, then make this the default */
        kcdb_identity_refresh(ident);
        {
            khm_handle idpro = NULL;
            khm_handle tdefault = NULL;

            _progress(3,4);

            if (KHM_SUCCEEDED(kcdb_identpro_find(L"Krb5Ident", &idpro)) &&
                KHM_SUCCEEDED(kcdb_identity_get_default_ex(idpro, &tdefault))) {
                kcdb_identity_release(tdefault);
            } else {
                _reportf(L"There was no default identity.  Setting default");
                kcdb_identity_set_default(ident);
            }

            _progress(4,5);

            if (idpro != NULL)
                kcdb_identpro_release(idpro);
        }

        khm_krb5_sync_default_id_with_mslsa();

        /* and update the LRU */
        k5_update_LRU(ident);

        if (ctx != NULL)
            krb5_free_context(ctx);
    } else if (d->kinit_task == NULL) {
        /* the user cancelled the operation */
        r = KHUI_NC_RESPONSE_EXIT |
            KHUI_NC_RESPONSE_SUCCESS;
    }

    khui_cw_set_response(nc, credtype_id_krb5, r);

    if (d->kinit_task) {
        k5_kinit_task_abort_and_release(d->kinit_task);
        d->kinit_task = NULL;
    }

    if (r & KHUI_NC_RESPONSE_NOEXIT) {
        /* if we are retrying the call, we should restart the kinit
           thread */
#ifdef DEBUG
        assert(r & KHUI_NC_RESPONSE_PENDING);
#endif
        d->kinit_task = k5_kinit_task_create(nc);
    }

 done:

    if (ident)
        kcdb_identity_release(ident);
    ident = NULL;

    _progress(1,1);

    _end_task();
}

void
k5_handle_process_renew_creds(khui_new_creds *nc,
                              k5_dlg_data    *d)
{
    khm_handle ident = NULL;
    khm_int32 r = 0;
    FILETIME ftidexp = {0,0};
    FILETIME ftcurrent;
    khm_size cb;
    khui_action_context * pctx = NULL;

    GetSystemTimeAsFileTime(&ftcurrent);

    _begin_task(0);
    _report_mr0(KHERR_NONE, MSG_CTX_RENEW_CREDS);
    _describe();

    _progress(0,1);

    pctx = khui_cw_get_ctx(nc);

    if (pctx->scope == KHUI_SCOPE_IDENT ||

        (pctx->scope == KHUI_SCOPE_CREDTYPE &&
         pctx->cred_type == credtype_id_krb5) ||

        (pctx->scope == KHUI_SCOPE_CRED &&
         pctx->cred_type == credtype_id_krb5)) {
        int code;

        if (pctx->scope == KHUI_SCOPE_CRED &&
            pctx->cred != NULL) {

            /* get the expiration time for the identity first. */
            cb = sizeof(ftidexp);
#ifdef DEBUG
            assert(pctx->identity != NULL);
#endif
            kcdb_identity_get_attr(pctx->identity,
                                   KCDB_ATTR_EXPIRE,
                                   NULL,
                                   &ftidexp,
                                   &cb);

            code = khm_krb5_renew_cred(pctx->cred);

        } else if (pctx->scope == KHUI_SCOPE_IDENT &&
                   pctx->identity != 0) {
            /* get the current identity expiration time */
            cb = sizeof(ftidexp);

            kcdb_identity_get_attr(pctx->identity,
                                   KCDB_ATTR_EXPIRE,
                                   NULL,
                                   &ftidexp,
                                   &cb);

            code = khm_krb5_renew_ident(pctx->identity);
        } else {

            _reportf(L"No identity specified.  Can't renew Kerberos tickets");

            code = 1; /* it just has to be non-zero */
        }

        _progress(1,2);

        if (code == 0) {
            _reportf(L"Tickets successfully renewed");

            khui_cw_set_response(nc, credtype_id_krb5,
                                 KHUI_NC_RESPONSE_EXIT |
                                 KHUI_NC_RESPONSE_SUCCESS);
        } else if (pctx->identity == NULL) {

            _report_mr0(KHERR_ERROR, MSG_ERR_NO_IDENTITY);

            khui_cw_set_response(nc, credtype_id_krb5,
                                 KHUI_NC_RESPONSE_EXIT |
                                 KHUI_NC_RESPONSE_FAILED);
        } else if (CompareFileTime(&ftcurrent, &ftidexp) < 0) {
            wchar_t tbuf[1024];
            DWORD suggestion;
            kherr_suggestion sug_id;

            /* if we failed to get new tickets, but the identity is
               still valid, then we assume that the current tickets
               are still good enough for other credential types to
               obtain their credentials. */

            khm_err_describe(NULL, code, tbuf, sizeof(tbuf),
                             &suggestion, &sug_id);

            _report_cs0(KHERR_WARNING, tbuf);
            if (suggestion)
                _suggest_mr(suggestion, sug_id);

            _resolve();

            khui_cw_set_response(nc, credtype_id_krb5,
                                 KHUI_NC_RESPONSE_EXIT |
                                 KHUI_NC_RESPONSE_SUCCESS);
        } else {
            wchar_t tbuf[1024];
            DWORD suggestion;
            kherr_suggestion sug_id;

            khm_err_describe(NULL, code, tbuf, sizeof(tbuf),
                             &suggestion, &sug_id);

            _report_cs0(KHERR_ERROR, tbuf);
            if (suggestion)
                _suggest_mr(suggestion, sug_id);

            _resolve();

            khui_cw_set_response(nc, credtype_id_krb5,
                                 ((sug_id == KHERR_SUGGEST_RETRY)?KHUI_NC_RESPONSE_NOEXIT:KHUI_NC_RESPONSE_EXIT) |
                                 KHUI_NC_RESPONSE_FAILED);
        }
    } else {
        khui_cw_set_response(nc, credtype_id_krb5,
                             KHUI_NC_RESPONSE_EXIT |
                             KHUI_NC_RESPONSE_SUCCESS);
    }

    _progress(1,1);

    _end_task();
}

void
k5_handle_process_password(khui_new_creds * nc,
                           k5_dlg_data    * d)
{
    khm_handle ident;

    _begin_task(0);
    _report_mr0(KHERR_NONE, MSG_CTX_PASSWD);
    _describe();

    ident = NULL;
    khui_cw_get_primary_id(nc, &ident);

    if (khui_cw_get_result(nc) == KHUI_NC_RESULT_CANCEL) {

        khui_cw_set_response(nc, credtype_id_krb5,
                             KHUI_NC_RESPONSE_SUCCESS |
                             KHUI_NC_RESPONSE_EXIT);

    } else if (ident == NULL) {
        _report_mr0(KHERR_ERROR, MSG_PWD_NO_IDENTITY);
        _suggest_mr(MSG_PWD_S_NO_IDENTITY, KHERR_SUGGEST_RETRY);

        khui_cw_set_response(nc, credtype_id_krb5,
                             KHUI_NC_RESPONSE_FAILED |
                             KHUI_NC_RESPONSE_NOEXIT);
    } else if (!kcdb_identity_by_provider(ident, L"Krb5Ident")) {
        khui_cw_set_response(nc, credtype_id_krb5,
                             KHUI_NC_RESPONSE_SUCCESS |
                             KHUI_NC_RESPONSE_EXIT);
    } else {
        wchar_t   widname[KCDB_IDENT_MAXCCH_NAME];
        char      idname[KCDB_IDENT_MAXCCH_NAME];
        wchar_t   wpwd[KHUI_MAXCCH_PASSWORD];
        char      pwd[KHUI_MAXCCH_PASSWORD];
        wchar_t   wnpwd[KHUI_MAXCCH_PASSWORD];
        char      npwd[KHUI_MAXCCH_PASSWORD];
        wchar_t   wnpwd2[KHUI_MAXCCH_PASSWORD];
        wchar_t * wresult;
        char    * result;
        khm_size n_prompts = 0;
        khm_size cb;
        khm_int32 rv = KHM_ERROR_SUCCESS;
        long code = 0;

        _progress(0,1);

        khui_cw_sync_prompt_values(nc);

        khui_cw_get_prompt_count(nc, &n_prompts);
        assert(n_prompts == 3);

        cb = sizeof(widname);
        rv = kcdb_identity_get_name(ident, widname, &cb);
        if (KHM_FAILED(rv)) {
            assert(FALSE);
            _report_mr0(KHERR_ERROR, MSG_PWD_UNKNOWN);
            goto _pwd_exit;
        }

        cb = sizeof(wpwd);
        rv = khui_cw_get_prompt_value(nc, 0, wpwd, &cb);
        if (KHM_FAILED(rv)) {
            assert(FALSE);
            _report_mr0(KHERR_ERROR, MSG_PWD_UNKNOWN);
            goto _pwd_exit;
        }

        cb = sizeof(wnpwd);
        rv = khui_cw_get_prompt_value(nc, 1, wnpwd, &cb);
        if (KHM_FAILED(rv)) {
            assert(FALSE);
            _report_mr0(KHERR_ERROR, MSG_PWD_UNKNOWN);
            goto _pwd_exit;
        }

        cb = sizeof(wnpwd2);
        rv = khui_cw_get_prompt_value(nc, 2, wnpwd2, &cb);
        if (KHM_FAILED(rv)) {
            assert(FALSE);
            _report_mr0(KHERR_ERROR, MSG_PWD_UNKNOWN);
            goto _pwd_exit;
        }

        if (wcscmp(wnpwd, wnpwd2)) {
            rv = KHM_ERROR_INVALID_PARAM;
            _report_mr0(KHERR_ERROR, MSG_PWD_NOT_SAME);
            _suggest_mr(MSG_PWD_S_NOT_SAME, KHERR_SUGGEST_INTERACT);
            goto _pwd_exit;
        }

        if (!wcscmp(wpwd, wnpwd)) {
            rv = KHM_ERROR_INVALID_PARAM;
            _report_mr0(KHERR_ERROR, MSG_PWD_SAME);
            _suggest_mr(MSG_PWD_S_SAME, KHERR_SUGGEST_INTERACT);
            goto _pwd_exit;
        }

        UnicodeStrToAnsi(idname, sizeof(idname), widname);
        UnicodeStrToAnsi(pwd, sizeof(pwd), wpwd);
        UnicodeStrToAnsi(npwd, sizeof(npwd), wnpwd);

        result = NULL;

        _progress(1, 2);

        code = khm_krb5_changepwd(idname, pwd, npwd, &result);

        if (code)
            rv = KHM_ERROR_UNKNOWN;
        else {
            khm_handle csp_idcfg = NULL;
            krb5_context ctx = NULL;

            /* if there is anyone requesting privileged data, give it
               to them */
            {
                krb5_data pd;
                pd.data = npwd;
                pd.length = (unsigned int) strlen(npwd) + 1;
                k5_reply_to_acqpriv_id_request(nc, &pd);
            }

            /* We don't need the prompts around anymore */
            khui_cw_clear_prompts(nc);

            /* we set a new password.  now we need to get initial
               credentials. */

            if (d == NULL) {
                rv = KHM_ERROR_UNKNOWN;
                goto _pwd_exit;
            }

            if (khui_cw_get_subtype(nc) == KMSG_CRED_PASSWORD) {
                /* since this was just a password change, we need to
                   load new credentials options from the configuration
                   store. */

                k5_read_dlg_params(d, ident);
            }

            /* the password change phase is now done */
            d->pwd_change = FALSE;

#ifdef DEBUG
            _reportf(L"Calling khm_krb5_kinit()");
#endif
            code = khm_krb5_kinit(NULL,   /* context (create one) */
                                  idname, /* principal_name */
                                  npwd,   /* new password */
                                  NULL, /* ccache name (figure out the identity cc)*/
                                  d->params.lifetime,
                                  d->params.forwardable,
                                  d->params.proxiable,
                                  (d->params.renewable)?d->params.renew_life:0,
                                  d->params.addressless, /* addressless */
                                  d->params.publicIP, /* public IP */
                                  NULL,               /* prompter */
                                  NULL           /* prompter data */);

            if (code) {
                rv = KHM_ERROR_UNKNOWN;
                goto _pwd_exit;
            }

            /* save the settings that we used for obtaining the
               ticket. */
            if (khui_cw_get_subtype(nc) == KHUI_NC_SUBTYPE_NEW_CREDS ||
                khui_cw_get_subtype(d->nct.nc) == KHUI_NC_SUBTYPE_ACQPRIV_ID) {

                k5_write_dlg_params(d, ident, NULL);

                /* and then update the LRU too */
                k5_update_LRU(ident);
            }

            /* and do a quick refresh of the krb5 tickets so that
               other plug-ins that depend on krb5 can look up tickets
               inside NetIDMgr */
            khm_krb5_list_tickets(&ctx);

            /* if there was no default identity, we make this one the
               default. */
            kcdb_identity_refresh(ident);
            {
                khm_handle idpro = NULL;
                khm_handle tdefault = NULL;

                if (KHM_SUCCEEDED(kcdb_identpro_find(L"Krb5Ident", &idpro)) &&
                    KHM_SUCCEEDED(kcdb_identity_get_default_ex(idpro, &tdefault))) {
                    kcdb_identity_release(tdefault);
                } else {
                    _reportf(L"There was no default identity.  Setting default");
                    kcdb_identity_set_default(ident);
                }

                if (idpro)
                    kcdb_identpro_release(idpro);
            }

            if (ctx != NULL)
                krb5_free_context(ctx);

            if (khui_cw_get_subtype(nc) == KMSG_CRED_PASSWORD) {
                /* if we obtained new credentials as a result of
                   successfully changing the password, we also
                   schedule an identity renewal for this identity.
                   This allows the other credential types to obtain
                   credentials for this identity. */
                khui_action_context ctx;

                _reportf(L"Scheduling renewal of [%s] after password change",
                         widname);

                khui_context_create(&ctx,
                                    KHUI_SCOPE_IDENT,
                                    ident,
                                    KCDB_CREDTYPE_INVALID,
                                    NULL);
                khui_action_trigger(KHUI_ACTION_RENEW_CRED,
                                    &ctx);

                khui_context_release(&ctx);
            }
        }

        /* result is only set when code != 0 */
        if (code && result) {
            size_t len;

            StringCchLengthA(result, KHERR_MAXCCH_STRING, &len);
            wresult = PMALLOC((len + 1) * sizeof(wchar_t));
            assert(wresult);
            AnsiStrToUnicode(wresult, (len + 1) * sizeof(wchar_t), result);

            _report_cs1(KHERR_ERROR, L"%1!s!", _cstr(wresult));
            _resolve();

            PFREE(result);
            PFREE(wresult);

            /* we don't need to report anything more */
            code = 0;
        }

    _pwd_exit:
        if (KHM_FAILED(rv)) {
            if (code) {
                wchar_t tbuf[1024];
                DWORD suggestion;
                kherr_suggestion sug_id;

                khm_err_describe(NULL, code, tbuf, sizeof(tbuf),
                                 &suggestion, &sug_id);
                _report_cs0(KHERR_ERROR, tbuf);

                if (suggestion)
                    _suggest_mr(suggestion, sug_id);

                _resolve();
            }

            khui_cw_set_response(nc, credtype_id_krb5,
                                 KHUI_NC_RESPONSE_NOEXIT |
                                 KHUI_NC_RESPONSE_FAILED);
        } else {
            khui_cw_set_response(nc, credtype_id_krb5,
                                 KHUI_NC_RESPONSE_SUCCESS |
                                 KHUI_NC_RESPONSE_EXIT);
        }
    }

    if (ident)
        kcdb_identity_release(ident);

    _progress(1,1);

    _end_task();
}

/* Handler for CRED type messages

    Runs in the context of the Krb5 plugin
*/
khm_int32 KHMAPI
k5_msg_cred_dialog(khm_int32 msg_type,
                   khm_int32 msg_subtype,
                   khm_ui_4 uparam,
                   void * vparam)
{
    khui_new_creds *nc = NULL;
    k5_dlg_data    *d  = NULL;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    switch(msg_subtype) {

    case KMSG_CRED_PASSWORD:
    case KMSG_CRED_NEW_CREDS:
    case KMSG_CRED_ACQPRIV_ID:
        {
            wchar_t wbuf[256];

            nc = (khui_new_creds *) vparam;

            d = PMALLOC(sizeof(*d));
            ZeroMemory(d, sizeof(*d));

            d->nct.type = credtype_id_krb5;

            LoadString(hResModule, IDS_KRB5_NC_NAME,
                       wbuf, ARRAYLENGTH(wbuf));
            d->nct.name = PWCSDUP(wbuf);
            d->nct.h_module = hResModule;
            d->nct.dlg_proc = k5_nc_dlg_proc;
            if (khui_cw_get_subtype(nc) == KMSG_CRED_PASSWORD)
                d->nct.dlg_template = MAKEINTRESOURCE(IDD_NC_KRB5_PASSWORD);
            else
                d->nct.dlg_template = MAKEINTRESOURCE(IDD_NC_KRB5);

            khui_cw_add_type(nc, &d->nct);

            khui_cw_add_selector(nc, k5_idselector_factory, NULL);
        }
        break;

    case KMSG_CRED_IDSPEC:
        {
            nc = (khui_new_creds *) vparam;

            khui_cw_add_selector(nc, k5_idselector_factory, NULL);
        }
        break;

    case KMSG_CRED_RENEW_CREDS:
        {
            nc = (khui_new_creds *) vparam;

            d = PMALLOC(sizeof(*d));
            ZeroMemory(d, sizeof(*d));

            d->nct.type = credtype_id_krb5;

            khui_cw_add_type(nc, &d->nct);
        }
        break;

    case KMSG_CRED_DIALOG_PRESTART:
        {
            nc = (khui_new_creds *) vparam;

            khui_cw_find_type(nc, credtype_id_krb5, (khui_new_creds_by_type **) &d);

            /* this can be NULL if the dialog was closed while the
               plug-in thread was processing. */
            if (d == NULL)
                break;

            if (khui_cw_get_subtype(nc) == KHUI_NC_SUBTYPE_NEW_CREDS ||
                khui_cw_get_subtype(d->nct.nc) == KHUI_NC_SUBTYPE_ACQPRIV_ID) {
                k5_read_dlg_params(d, NULL);
            }

            PostMessage(d->nct.hwnd_panel, KHUI_WM_NC_NOTIFY,
                        MAKEWPARAM(0,WMNC_DIALOG_SETUP), 0);
        }
        break;

    case KMSG_CRED_DIALOG_NEW_IDENTITY:
        {
            khm_handle ident = NULL;

            nc = (khui_new_creds *) vparam;

            khui_cw_find_type(nc, credtype_id_krb5, (khui_new_creds_by_type **) &d);
            if (d == NULL)
                break;

	    /* ?: It might be better to not load identity defaults if
	       the user has already changed options in the dialog. */
            if((khui_cw_get_subtype(nc) == KHUI_NC_SUBTYPE_NEW_CREDS ||
                khui_cw_get_subtype(d->nct.nc) == KHUI_NC_SUBTYPE_ACQPRIV_ID) &&
               KHM_SUCCEEDED(khui_cw_get_primary_id(nc, &ident))) {
                k5_read_dlg_params(d, ident);

                PostMessage(d->nct.hwnd_panel, KHUI_WM_NC_NOTIFY,
                            MAKEWPARAM(0,WMNC_DIALOG_SETUP), 0);

                kcdb_identity_release(ident);
                ident = NULL;
            }

            /* reset the force-password-change flag if this is a new
               identity. */
            d->pwd_change = FALSE;
        }

        /* fallthrough */
    case KMSG_CRED_DIALOG_NEW_OPTIONS:
        {
            khm_handle ident = NULL;
            khm_boolean foreign_identity = FALSE;

            nc = (khui_new_creds *) vparam;

            khui_cw_find_type(nc, credtype_id_krb5, (khui_new_creds_by_type **) &d);
            if (d == NULL)
                break;

            if (khui_cw_get_subtype(nc) == KMSG_CRED_PASSWORD) {
                k5_force_password_change(d);
                return KHM_ERROR_SUCCESS;
            }
            /* else; nc->subtype == KHUI_NC_SUBTYPE_NEW_CREDS */

            assert(khui_cw_get_subtype(nc) == KHUI_NC_SUBTYPE_NEW_CREDS ||
                   khui_cw_get_subtype(d->nct.nc) == KHUI_NC_SUBTYPE_ACQPRIV_ID);

            /* If we are forcing a password change, then we don't do
               anything here.  Note that if the identity changed, then
               this field would have been reset, so we would proceed
               as usual. */
            if (d->pwd_change)
                return KHM_ERROR_SUCCESS;

#if 0
            /* Clearing the prompts at this point is a bad idea since
               the prompter depends on the prompts to know if this set
               of prompts is the same as the new set and if so, use
               the values entered in the old prompts as responses to
               the new one. */
            khui_cw_clear_prompts(nc);
#endif

            khui_cw_get_primary_id(nc, &ident);

            if (!kcdb_identity_by_provider(ident, L"Krb5Ident")) {
                /* This is not an identity we want to deal with */

                foreign_identity = TRUE;
            }

            /* If we already have a k5_kinit_task object, we want to
               get rid of it and create a new one. */
            if(d->kinit_task != NULL) {
                khm_boolean clear_prompts = TRUE;

                EnterCriticalSection(&d->kinit_task->cs);
                if (kcdb_identity_is_equal(ident, d->kinit_task->identity))
                    clear_prompts = FALSE;
                LeaveCriticalSection(&d->kinit_task->cs);

                k5_kinit_task_abort_and_release(d->kinit_task);
                d->kinit_task = NULL;

                if (clear_prompts) {
                    khui_cw_clear_prompts(nc);
                    if (!foreign_identity)
                        khui_cw_notify_identity_state(nc, d->nct.hwnd_panel,
                                                      NULL, 0, KHUI_CWNIS_MARQUEE);
                }
            } else {
                khui_cw_clear_prompts(nc);
                if (!foreign_identity)
                    khui_cw_notify_identity_state(nc, d->nct.hwnd_panel,
                                                  NULL, 0, KHUI_CWNIS_MARQUEE);
            }

            if(ident && !foreign_identity) {
                khui_cw_lock_nc(nc);
                d->kinit_task = k5_kinit_task_create(nc);
                d->sync = TRUE;
                khui_cw_unlock_nc(nc);
                kcdb_identity_release(ident);
            } else {
                khui_cw_clear_prompts(nc);
            }
        }
        break;

    case KMSG_CRED_PREPROCESS_ID:
        {
            khui_action_context * ctx = NULL;
            khm_handle credset = NULL;
            khm_int32 code = 0;
            krb5_context k5ctx = 0;
            khm_handle cred = NULL;
            char * pwd = NULL;
            khm_size cb_pwd = 0;
            wchar_t widname[KCDB_IDENT_MAXCCH_NAME];
            char idname[KCDB_IDENT_MAXCCH_NAME];
            khm_size cb;

            nc = (khui_new_creds *) vparam;

            khui_cw_find_type(nc, credtype_id_krb5, (khui_new_creds_by_type **) &d);
            if (d == NULL) {
                assert(FALSE);
                break;
            }

            khui_cw_get_privileged_credential_collector(nc, &credset);

            ctx = khui_cw_get_ctx(nc);

            assert(credset);
            assert(ctx);
            assert(ctx->scope == KHUI_SCOPE_IDENT);
            assert(ctx->identity != NULL);

            cb = sizeof(widname);
            kcdb_identity_get_name(ctx->identity, widname, &cb);
            UnicodeStrToAnsi(idname, sizeof(idname), widname);

            k5_read_dlg_params(d, ctx->identity);

            if (KHM_SUCCEEDED(kcdb_credset_get_cred(credset, 0, &cred))) {
                if (KHM_SUCCEEDED(kcdb_cred_get_attr(cred, attr_id_krb5_pkeyv1,
                                                     NULL, NULL, NULL))) {
                    kcdb_cred_get_attr(cred, attr_id_krb5_pkeyv1, NULL, NULL, &cb_pwd);
                    pwd = malloc(cb_pwd + 1);
                    ZeroMemory(pwd, cb_pwd + 1);
                    kcdb_cred_get_attr(cred, attr_id_krb5_pkeyv1, NULL, pwd, &cb_pwd);
                }
                kcdb_cred_release(cred);
            }

            if (pwd) {
                code = khm_krb5_kinit(NULL,   /* context (create one) */
                                      idname, /* principal_name */
                                      pwd,   /* new password */
                                      NULL, /* ccache name (figure out the identity cc)*/
                                      d->params.lifetime,
                                      d->params.forwardable,
                                      d->params.proxiable,
                                      (d->params.renewable)?d->params.renew_life:0,
                                      d->params.addressless, /* addressless */
                                      d->params.publicIP, /* public IP */
                                      NULL,               /* prompter */
                                      NULL           /* prompter data */);

                SecureZeroMemory(pwd, cb_pwd);
                free (pwd);
            }

            if (code)
                break;

            khm_krb5_list_tickets(&k5ctx);

            if (k5ctx != NULL)
                krb5_free_context(k5ctx);
        }
        break;

    case KMSG_CRED_PROCESS:
        {
            khm_handle ident = NULL;
            khm_int32 r = 0;

            nc = (khui_new_creds *) vparam;

            khui_cw_find_type(nc, credtype_id_krb5, (khui_new_creds_by_type **) &d);
            if(d == NULL)
                break;

            switch (khui_cw_get_subtype(nc)) {
            case KHUI_NC_SUBTYPE_NEW_CREDS:
            case KHUI_NC_SUBTYPE_ACQPRIV_ID:
                k5_handle_process_new_creds(nc, d);
                break;

            case KHUI_NC_SUBTYPE_RENEW_CREDS:
                k5_handle_process_renew_creds(nc, d);
                break;

            case KHUI_NC_SUBTYPE_PASSWORD:
                k5_handle_process_password(nc, d);
                break;
            }
        }
        break;

    case KMSG_CRED_END:
        {
            nc = (khui_new_creds *) vparam;
            khui_cw_find_type(nc, credtype_id_krb5, (khui_new_creds_by_type **) &d);
            if(d == NULL)
                break;

            khui_cw_del_type(nc, credtype_id_krb5);

            if (d->nct.name)
                PFREE(d->nct.name);
            if (d->nct.credtext)
                PFREE(d->nct.credtext);

            if (d->kinit_task)
                k5_kinit_task_abort_and_release(d->kinit_task);

            ZeroMemory(d, sizeof(*d));
            PFREE(d);
        }
        break;

    case KMSG_CRED_IMPORT:
        {
            khm_int32 t = 0;

#ifdef DEBUG
            assert(csp_params);
#endif
            khc_read_int32(csp_params, L"MsLsaImport", &t);

            if (t != K5_LSAIMPORT_NEVER) {
                krb5_context ctx = NULL;
                khm_handle idpro = NULL;
                khm_handle id_default = NULL;
                khm_handle id_imported = NULL;
                BOOL imported;

                imported = khm_krb5_ms2mit(NULL, (t == K5_LSAIMPORT_MATCH), TRUE,
                                           &id_imported);
                if (imported) {
                    if (id_imported)
                        k5_ensure_identity_ccache_is_watched(id_imported, NULL);

                    khm_krb5_list_tickets(&ctx);

                    if (ctx)
                        krb5_free_context(ctx);

                    kcdb_identity_refresh(id_imported);

                    if (KHM_SUCCEEDED(kcdb_identpro_find(L"Krb5Ident", &idpro)) &&
                        KHM_SUCCEEDED(kcdb_identity_get_default_ex(idpro, &id_default))) {
                        kcdb_identity_release(id_default);
                        id_default = NULL;
                    } else {
                        _reportf(L"There was no default identity.  Setting default");
                        kcdb_identity_set_default(id_imported);
                    }

                    if (idpro)
                        kcdb_identpro_release(idpro);

                    /* and update the LRU */
                    k5_update_LRU(id_imported);
                }

                if (id_imported)
                    kcdb_identity_release(id_imported);
            }
        }
        break;
    }

    return rv;
}
