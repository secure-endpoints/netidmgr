/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
 * Copyright (c) 2006-2010 Secure Endpoints Inc.
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
#include<commctrl.h>
#include<strsafe.h>
#include<krb5.h>
#include<assert.h>

#define NC_UNCHANGE_TIMEOUT 3000
#define NC_UNCHANGE_TIMER   2
#define NC_REALMCHANGE_TIMEOUT NC_UNCHANGE_TIMEOUT
#define NC_REALMCHANGE_TIMER 3

khm_handle k5_identpro = NULL;     /* Handle to self */

extern HANDLE h_idprov_event;

/************************************************************/
/*                Identity Selector Control                 */
/************************************************************/

struct idsel_dlg_data {
    khm_int32 magic;            /* Always IDSEL_DLG_DATA_MAGIC */
    khm_handle ident;           /* Current selection */
};

#define IDSEL_DLG_DATA_MAGIC 0xa1fbcc16

static
void
trim_str(wchar_t * s, khm_size cch) {
    wchar_t * c, * last_ws;

    for (c = s; *c && iswspace(*c) && ((khm_size)(c - s)) < cch; c++);

    if (((khm_size)(c - s)) >= cch)
        return;

    if (c != s && ((khm_size)(c - s)) < cch) {
#if _MSC_VER >= 1400 && __STDC_WANT_SECURE_LIB__
        wmemmove_s(s, cch, c, cch - ((khm_size)(c - s)));
#else
        memmove(s, c, (cch - ((khm_size)(c - s))) * sizeof(wchar_t));
#endif
    }

    last_ws = NULL;
    for (c = s; *c && ((khm_size)(c - s)) < cch; c++) {
        if (!iswspace(*c))
            last_ws = NULL;
        else if (last_ws == NULL)
            last_ws = c;
    }

    if (last_ws)
        *last_ws = L'\0';
}

/* Display a balloon prompt for the edit control in a combo box.  If
   title and text are NULL, then hides the ballon instead.

   Requires XP.

   TODO: Make it backwards compatible with 2000.
 */
static void
show_combobox_balloon(HWND combobox, const wchar_t * title,
                      const wchar_t * text, int iicon)
{
    COMBOBOXINFO ci;
    EDITBALLOONTIP bt;

    ZeroMemory(&ci, sizeof(ci));
    ci.cbSize = sizeof(ci);

    if (!GetComboBoxInfo(combobox, &ci) || ci.hwndItem == NULL)
        return;

    if (text != NULL) {
        ZeroMemory(&bt, sizeof(bt));

        bt.cbStruct = sizeof(bt);
        bt.pszTitle = title;
        bt.pszText = text;
        bt.ttiIcon = iicon;

        Edit_ShowBalloonTip(ci.hwndItem, &bt);
    } else {
        Edit_HideBalloonTip(ci.hwndItem);
    }
}

/* Get the identity that has been selected in the UI.
 */
static void
set_identity_from_ui(HWND hwnd) {
    struct idsel_dlg_data * d;
    wchar_t un[KCDB_IDENT_MAXCCH_NAME];
    wchar_t * realm;
    khm_size cch;
    khm_size cch_left;
    khm_handle ident = NULL;
    LRESULT idx = CB_ERR;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    d = (struct idsel_dlg_data *) (LONG_PTR) GetWindowLongPtr(hwnd, DWLP_USER);
#ifdef DEBUG
    assert(d && d->magic == IDSEL_DLG_DATA_MAGIC);
#endif

    cch = GetDlgItemText(hwnd, IDC_NC_UN, un, ARRAYLENGTH(un));
    if (cch == 0) {
        rv = KHM_ERROR_INVALID_NAME;
        goto _set_ident;
    }
    trim_str(un, ARRAYLENGTH(un));

    realm = khm_get_realm_from_princ(un);
    if (realm)          /* realm was specified */
        goto _create_ident;

    if (cch >= KCDB_IDENT_MAXCCH_NAME - 3) {
        /* has to allow space for the '@' and at least a single
           character realm, and the NULL terminator. */
        rv = KHM_ERROR_TOO_LONG;
        goto _set_ident;
    }

    realm = un + cch;   /* now points at terminating NULL */
    cch_left = KCDB_IDENT_MAXCCH_NAME - cch;

    *realm++ = L'@';
    *realm = L'\0';
    cch_left--;

    cch = GetDlgItemText(hwnd, IDC_NC_REALM, realm, (int) cch_left);
    if (cch == 0) {
        rv = KHM_ERROR_INVALID_NAME;
        goto _set_ident;
    }

    trim_str(realm, cch_left);

 _create_ident:
    if (KHM_FAILED(rv = kcdb_identity_create_ex(k5_identpro, un,
                                                KCDB_IDENT_FLAG_CREATE,
                                                NULL, &ident))) {
        ident = NULL;
    }

 _set_ident:

    if (d->ident != NULL && kcdb_identity_is_equal(d->ident, ident)) {
        goto _cleanup;
    }

    if (d->ident != NULL)
        kcdb_identity_release(d->ident);

    d->ident = ident;

    if (ident) {
        HICON icon = NULL;
        khm_size cb;

        cb = sizeof(icon);
        kcdb_get_resource(ident, KCDB_RES_ICON_NORMAL, 0, NULL, NULL, &icon, &cb);

        SendDlgItemMessage(hwnd, IDC_NC_ICON, STM_SETICON, (WPARAM) icon, 0);
    } else {
        SendDlgItemMessage(hwnd, IDC_NC_ICON, STM_SETICON, 0, 0);
    }

    ident = NULL;

    {
        HWND parent;

        parent = GetParent(hwnd);
        if (parent) {
            PostMessage(parent, KHUI_WM_NC_NOTIFY, MAKEWPARAM(0, WMNC_IDENTITY_CHANGE),
                        (LPARAM) hwnd);
        }
    }

 _cleanup:
    if (ident) {
        kcdb_identity_release(ident);
    }

    return;
}

/* runs in the UI thread */
static BOOL
update_crossfeed(HWND hwnd, khm_boolean from_uname) {
    wchar_t un[KCDB_IDENT_MAXCCH_NAME];
    wchar_t * un_realm;
    wchar_t realm[KCDB_IDENT_MAXCCH_NAME];
    khm_size cch;
    khm_size cch_left;
    int idx;
    HWND hw_realm;
    HWND hw_un;

    hw_realm = GetDlgItem(hwnd, IDC_NC_REALM);
    hw_un = GetDlgItem(hwnd, IDC_NC_UN);

#ifdef DEBUG
    assert(hw_realm != NULL);
    assert(hw_un != NULL);
#endif

    un[0] = L'\0';
    GetWindowText(hw_un, un, ARRAYLENGTH(un));
    trim_str(un, ARRAYLENGTH(un));

    un_realm = khm_get_realm_from_princ(un);

    if (un_realm == NULL) {
        EnableWindow(hw_realm, TRUE);
        return FALSE;
    }

    if (from_uname) {

        idx = (int)SendMessage(hw_realm, CB_FINDSTRINGEXACT,
                               (WPARAM) -1, (LPARAM) un_realm);

        if (idx != CB_ERR) {
            wchar_t srealm[KCDB_IDENT_MAXCCH_NAME];

            cch = SendMessage(hw_realm, CB_GETLBTEXTLEN, idx, 0);

#ifdef DEBUG
            assert(cch < ARRAYLENGTH(srealm) - 1);
#endif
            SendMessage(hw_realm, CB_GETLBTEXT, idx, (LPARAM) srealm);

            if (!_wcsicmp(srealm, un_realm) && wcscmp(srealm, un_realm)) {
                /* differ only by case */

                StringCchCopy(un_realm, ARRAYLENGTH(un) - (un_realm - un),
                              srealm);

                SetWindowText(hw_un, un);
            }
        }

        SendMessage(hw_realm, CB_SELECTSTRING, (WPARAM) -1, (LPARAM) un_realm);

        SetWindowText(hw_realm, un_realm);

        if (GetFocus() == hw_realm) {
            HWND hw_next = NULL;

            hw_next = GetNextDlgTabItem(hwnd, hw_realm, FALSE);
            if (hw_next)
                SetFocus(hw_next);
        }

        EnableWindow(hw_realm, FALSE);

        return TRUE;
    }
    /* else... */

    cch_left = KCDB_IDENT_MAXCCH_NAME - (un_realm - un);

    cch = (khm_size) GetWindowTextLength(hw_realm);

#ifdef DEBUG
    assert(cch < KCDB_IDENT_MAXCCH_NAME);
#endif
    if (cch == 0)
        return FALSE;

    GetWindowText(hw_realm, realm, ARRAYLENGTH(realm));
    trim_str(realm, ARRAYLENGTH(realm));

    idx = (int)SendMessage(hw_realm, CB_FINDSTRINGEXACT,
                           (WPARAM) -1, (LPARAM) realm);

    if (idx != CB_ERR) {
        wchar_t srealm[KCDB_IDENT_MAXCCH_NAME];

        SendMessage(hw_realm, CB_GETLBTEXT, idx, (LPARAM) srealm);

        if (!_wcsicmp(srealm, realm) && wcscmp(srealm, realm)) {
            StringCbCopy(realm, sizeof(realm), srealm);

            SetWindowText(hw_realm, srealm);
        }
    }

    StringCchCopy(un_realm, cch_left, realm);

    SendMessage(hw_un, CB_SELECTSTRING, (WPARAM) -1, (LPARAM) un);

    SetWindowText(hw_un, un);

    return TRUE;
}

/* Dialog procedure for IDD_NC_KRB5_IDSEL

   runs in UI thread */
static INT_PTR CALLBACK
k5_idspec_dlg_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg) {
    case WM_INITDIALOG:
        {
            struct idsel_dlg_data * d;

            wchar_t defident[KCDB_IDENT_MAXCCH_NAME];
            wchar_t * ms = NULL;
            wchar_t * t = NULL;
            wchar_t * defrealm = NULL;
            LRESULT lr;
            khm_size cb_ms;
            khm_size cb;
            khm_int32 rv;
            khm_handle hident = NULL;

            HWND hw_realm;
            HWND hw_un;

            d = PMALLOC(sizeof(*d));
            ZeroMemory(d, sizeof(*d));
            d->magic = IDSEL_DLG_DATA_MAGIC;

#pragma warning(push)
#pragma warning(disable: 4244)
            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) d);
#pragma warning(pop)

            hw_realm = GetDlgItem(hwnd, IDC_NC_REALM);
            hw_un = GetDlgItem(hwnd, IDC_NC_UN);

#ifdef DEBUG
            assert(hw_realm != NULL);
            assert(hw_un != NULL);
#endif

            SendMessage(hw_un, CB_LIMITTEXT, (KCDB_IDENT_MAXCCH_NAME - 1), 0);
            SendMessage(hw_un, CB_SETEXTENDEDUI, TRUE, 0);

            SendMessage(hw_realm, CB_LIMITTEXT, (KCDB_IDENT_MAXCCH_NAME - 1), 0);
            SendMessage(hw_realm, CB_SETEXTENDEDUI, TRUE, 0);

            defident[0] = L'\0';

            /* add the LRU realms and principals to the dropdown
               lists */
            rv = khc_read_multi_string(csp_params, L"LRUPrincipals", NULL, &cb_ms);

            if (rv != KHM_ERROR_TOO_LONG || cb_ms <= sizeof(wchar_t) * 2)
                goto _add_lru_realms;

            ms = PMALLOC(cb_ms);
            assert(ms != NULL);

            cb = cb_ms;
            rv = khc_read_multi_string(csp_params, L"LRUPrincipals", ms, &cb);

            assert(KHM_SUCCEEDED(rv));

            /* the first of these is considered the default identity
               if no other default is known */
            StringCbCopy(defident, sizeof(defident), ms);

            for (t = ms; t && *t; t = multi_string_next(t)) {
                SendMessage(hw_un, CB_ADDSTRING, 0, (LPARAM) t);
            }

        _add_lru_realms:
            /* add the default realm first */
            defrealm = khm_krb5_get_default_realm();
            if (defrealm) {
                SendMessage(hw_realm, CB_ADDSTRING, 0, (LPARAM) defrealm);
            }

            rv = khc_read_multi_string(csp_params, L"LRURealms", NULL, &cb);

            if (rv != KHM_ERROR_TOO_LONG)
                goto _done_adding_lru;

            if (ms != NULL) {
                if (cb_ms < cb) {
                    PFREE(ms);
                    ms = PMALLOC(cb);
                    assert(ms);
                    cb_ms = cb;
                }
            } else {
                ms = PMALLOC(cb);
                cb_ms = cb;
            }

            rv = khc_read_multi_string(csp_params, L"LRURealms", ms, &cb);

            assert(KHM_SUCCEEDED(rv));

            for (t = ms; t && *t; t = multi_string_next(t)) {
                lr = SendMessage(hw_realm, CB_FINDSTRINGEXACT, (WPARAM) -1, (LPARAM) t);
                if (lr != CB_ERR)
                    continue;

                SendMessage(hw_realm, CB_ADDSTRING, 0, (LPARAM) t);
            }
	  _done_adding_lru:

            {
                khm_int32 inc_realms = 0;

                if (KHM_FAILED(khc_read_int32(csp_params,
                                              L"UseFullRealmList",
                                              &inc_realms)) ||
                    !inc_realms)
                    goto _done_adding_all_realms;
            }

	    if(ms)
		PFREE(ms);

	    ms = khm_krb5_get_realm_list();
	    if(ms) {
		for (t = ms; t && *t; t = multi_string_next(t)) {
		    lr = SendMessage(hw_realm, CB_FINDSTRINGEXACT, (WPARAM) -1, (LPARAM) t);
		    if (lr != CB_ERR)
			continue;

		    SendMessage(hw_realm, CB_ADDSTRING, 0, (LPARAM) t);
		}
	    }

        _done_adding_all_realms:

            /* set the current selection of the realms list */
            if (defrealm) {
                SendMessage(hw_realm, CB_SELECTSTRING, (WPARAM) -1, (LPARAM) defrealm);
            } else {
                SendMessage(hw_realm, CB_SETCURSEL, 0, 0);
            }

            if (defrealm) {
                PFREE(defrealm);
                defrealm = NULL;
            }

            if (ms) {
                PFREE(ms);
                ms = NULL;
            }

            if (KHM_SUCCEEDED(kcdb_identity_get_default_ex(k5_identpro, &hident))) {
                cb = sizeof(defident);
                kcdb_identity_get_name(hident, defident, &cb);
                kcdb_identity_release(hident);
                hident = NULL;
            }

            if (defident[0] == L'\0') {
                DWORD dw;

                dw = ARRAYLENGTH(defident);
                GetUserName(defident, &dw);
            }

            t = khm_get_realm_from_princ(defident);
            if (t) {
                /* there is a realm */
                assert(t != defident);
                *--t = L'\0';
                t++;

                SendMessage(hw_realm, CB_SELECTSTRING, (WPARAM) -1, (LPARAM) t);
                SetWindowText(hw_realm, t);
            }

            if (defident[0] != L'\0') {
                /* there is a username */
                SendMessage(hw_un, CB_SELECTSTRING, (WPARAM) -1, (LPARAM) defident);
                SetWindowText(hw_un, defident);
            }

            set_identity_from_ui(hwnd);
        }
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
                if (d->ident) {
                    kcdb_identity_release(d->ident);
                    d->ident = NULL;
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
        switch(wParam) {
        case MAKEWPARAM(IDC_NC_UN, CBN_EDITCHANGE):
            /* the username has changed.  Instead of handling this
               for every keystroke, set a timer that elapses some
               time afterwards and then handle the event. */
            SetTimer(hwnd, NC_UNCHANGE_TIMER,
                     NC_UNCHANGE_TIMEOUT, NULL);
            return TRUE;

        case MAKEWPARAM(IDC_NC_UN, CBN_KILLFOCUS):
        case MAKEWPARAM(IDC_NC_UN, CBN_CLOSEUP):
            KillTimer(hwnd, NC_UNCHANGE_TIMER);

            update_crossfeed(hwnd, TRUE);
            set_identity_from_ui(hwnd);
            return TRUE;

        case MAKEWPARAM(IDC_NC_REALM,CBN_EDITCHANGE):
            SetTimer(hwnd, NC_REALMCHANGE_TIMER,
                     NC_REALMCHANGE_TIMEOUT, NULL);
            return TRUE;

        case MAKEWPARAM(IDC_NC_REALM,CBN_KILLFOCUS):
        case MAKEWPARAM(IDC_NC_REALM,CBN_CLOSEUP):
            KillTimer(hwnd, NC_REALMCHANGE_TIMER);

            update_crossfeed(hwnd, FALSE);
            set_identity_from_ui(hwnd);
            return TRUE;
        }
        break;

    case WM_TIMER:
        if(wParam == NC_UNCHANGE_TIMER) {
            KillTimer(hwnd, NC_UNCHANGE_TIMER);

            update_crossfeed(hwnd, TRUE);
            set_identity_from_ui(hwnd);
            return TRUE;
        } else if (wParam == NC_REALMCHANGE_TIMER) {
            KillTimer(hwnd, NC_REALMCHANGE_TIMER);

            update_crossfeed(hwnd, FALSE);
            set_identity_from_ui(hwnd);
            return TRUE;
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

                set_identity_from_ui(hwnd);
                if (ph) {
                    *ph = d->ident;
                    if (d->ident)
                        kcdb_identity_hold(d->ident);
                }
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
k5_idselector_factory(HWND hwnd_parent, khui_identity_selector * u) {

    if (hwnd_parent) {
        HWND hw_dlg;
        wchar_t display_name[KHUI_MAXCCH_NAME] = L"";

        hw_dlg = CreateDialog(hResModule, MAKEINTRESOURCE(IDD_NC_KRB5_IDSEL),
                              hwnd_parent, k5_idspec_dlg_proc);

        assert(hw_dlg);

        LoadString(hResModule, IDS_ID_INSTANCE, display_name, ARRAYLENGTH(display_name));

        u->hwnd_selector = hw_dlg;
        u->icon = LoadImage(hResModule, MAKEINTRESOURCE(IDI_KERBEROS),
                            IMAGE_ICON, 0, 0,
                            LR_DEFAULTSIZE | LR_DEFAULTCOLOR);
        u->display_name = PWCSDUP(display_name);

        return (hw_dlg ? KHM_ERROR_SUCCESS : KHM_ERROR_UNKNOWN);

    } else {

        if (u->display_name) {
            PFREE(u->display_name);
            u->display_name = NULL;
        }

        if (u->icon) {
            DestroyIcon(u->icon);
            u->icon = NULL;
        }

        return KHM_ERROR_SUCCESS;
    }
}

/************************************************************/
/*         Message Handler for KMSG_IDENT messages          */
/************************************************************/

static khm_int32
k5_validate_name(const wchar_t * name)
{
    krb5_principal princ = NULL;
    char princ_name[KCDB_IDENT_MAXCCH_NAME];
    krb5_error_code code;
    wchar_t * atsign;

    if(UnicodeStrToAnsi(princ_name, sizeof(princ_name), name) == 0) {
        return KHM_ERROR_INVALID_NAME;
    }

    assert(k5_identpro_ctx != NULL);

    code = pkrb5_parse_name(k5_identpro_ctx,
                            princ_name,
                            &princ);

    if (code) {
        return KHM_ERROR_INVALID_NAME;
    }

    if (princ != NULL)
        pkrb5_free_principal(k5_identpro_ctx, princ);

    /* krb5_parse_name() accepts principal names with no realm or an
       empty realm.  We don't. */
    atsign = wcschr(name, L'@');
    if (atsign == NULL || atsign[1] == L'\0') {
        return KHM_ERROR_INVALID_NAME;
    }

    return KHM_ERROR_SUCCESS;
}

static khm_int32
k5_ident_validate_name(khm_int32 msg_type,
                      khm_int32 msg_subtype,
                      khm_ui_4 uparam,
                      void * vparam) {
    kcdb_ident_name_xfer * nx;

    nx = (kcdb_ident_name_xfer *) vparam;

    nx->result = k5_validate_name(nx->name_src);

    return KHM_ERROR_SUCCESS;
}

static void
k5_update_last_default_identity(khm_handle ident) {
    wchar_t idname[KCDB_IDENT_MAXCCH_NAME];
    khm_size cb;

    cb = sizeof(idname);
    if (KHM_FAILED(kcdb_identity_get_name(ident, idname, &cb)))
        return;

    assert(csp_params);

    khc_write_string(csp_params, L"LastDefaultIdent", idname);
}

static khm_int32
k5_ident_set_default_int(khm_handle def_ident) {
    wchar_t id_ccname[KRB5_MAXCCH_CCNAME];
    khm_size cb;
    DWORD dw;
    LONG l;
    HKEY hk_ccname;
    DWORD dwType;
    DWORD dwSize;
    wchar_t reg_ccname[KRB5_MAXCCH_CCNAME];

#ifdef DEBUG
    assert(def_ident != NULL);
#endif

    cb = sizeof(id_ccname);
    if (KHM_FAILED(kcdb_identity_get_attr(def_ident, attr_id_krb5_ccname, NULL,
                                          id_ccname, &cb))) {
        _reportf(L"The specified identity does not have the Krb5CCName property");

        cb = sizeof(id_ccname);
        if (KHM_FAILED(khm_krb5_get_identity_default_ccache(def_ident, id_ccname, &cb))) {
            return KHM_ERROR_INVALID_PARAM;
        }
    }

    _reportf(L"Found Krb5CCName property : %s", id_ccname);

    StringCbLength(id_ccname, sizeof(id_ccname), &cb);
    cb += sizeof(wchar_t);

    _reportf(L"Setting default CC name in the registry");

    l = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\MIT\\kerberos5", 0,
                     KEY_READ | KEY_WRITE, &hk_ccname);

    if (l != ERROR_SUCCESS)
        l = RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\MIT\\kerberos5", 0,
                           NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                           NULL, &hk_ccname, &dw);

    if (l != ERROR_SUCCESS) {
        _reportf(L"Can't create registry key : %d", l);
        _end_task();
        return KHM_ERROR_UNKNOWN;
    }

    dwSize = sizeof(reg_ccname);

    l = RegQueryValueEx(hk_ccname, L"ccname", NULL, &dwType, (LPBYTE) reg_ccname,
                        &dwSize);

    if (l != ERROR_SUCCESS ||
        dwType != REG_SZ ||
        khm_krb5_cc_name_cmp(reg_ccname, id_ccname)) {

        /* we have to write the new value in */

        l = RegSetValueEx(hk_ccname, L"ccname", 0, REG_SZ, (BYTE *) id_ccname,
                          (DWORD) cb);
    }

    RegCloseKey(hk_ccname);

    if (l == ERROR_SUCCESS) {
        _reportf(L"Successfully set the default ccache");
        k5_update_last_default_identity(def_ident);
        return KHM_ERROR_SUCCESS;
    } else {
        _reportf(L"Can't set the registry value : %d", l);
        return KHM_ERROR_UNKNOWN;
    }
}

static khm_int32
k5_ident_set_default(khm_int32 msg_type,
                     khm_int32 msg_subtype,
                     khm_ui_4 uparam,
                     void * vparam) {

    /*
       Currently, setting the default identity simply sets the
       "ccname" registry value at "Software\MIT\kerberos5".
    */

    if (uparam) {
        /* an identity is being made default */
        khm_handle def_ident = (khm_handle) vparam;
        khm_int32 rv;

#ifdef DEBUG
        assert(def_ident != NULL);
#endif

        {
            wchar_t idname[KCDB_IDENT_MAXCCH_NAME];
            khm_size cb;

            cb = sizeof(idname);
            kcdb_identity_get_name(def_ident, idname, &cb);

            _begin_task(0);
            _report_cs1(KHERR_DEBUG_1, L"Setting default identity [%1!s!]", _cstr(idname));
            _describe();
        }

        rv = k5_ident_set_default_int(def_ident);

        _end_task();

        return rv;

    } else {
        /* the default identity is being forgotten */

        /* we don't really do anything about this case */
    }

    return KHM_ERROR_SUCCESS;
}

static khm_int32
k5_ident_notify_create(khm_int32 msg_type,
                       khm_int32 msg_subtype,
                       khm_ui_4 uparam,
                       void * vparam) {

    /* a new identity has been created.  What we want to do at
       this point is to check if the identity belongs to krb5
       and to see if it is the default. */

    krb5_ccache cc = NULL;
    krb5_error_code code;
    krb5_principal princ = NULL;
    char * princ_nameA = NULL;
    wchar_t princ_nameW[KCDB_IDENT_MAXCCH_NAME];
    wchar_t id_nameW[KCDB_IDENT_MAXCCH_NAME];
    char id_nameA[KCDB_IDENT_MAXCCH_NAME];
    khm_size cb;
    khm_handle ident;

    assert(k5_identpro_ctx != NULL);

    ident = (khm_handle) vparam;

    /* As a convention, we use the principal name as the identity name
       and the display name */
    cb = sizeof(id_nameW);

    if (KHM_FAILED(kcdb_identity_get_name(ident,
                                          id_nameW,
                                          &cb))) {
        return KHM_ERROR_INVALID_PARAM;
    }

    kcdb_identity_set_attr(ident, KCDB_ATTR_DISPLAY_NAME,
                           id_nameW, KCDB_CBSIZE_AUTO);
    kcdb_identity_set_flags(ident, KCDB_IDENT_FLAG_CRED_INIT,
                            KCDB_IDENT_FLAG_CRED_INIT);

    /* Get name and realm */
    {
        UnicodeStrToAnsi(id_nameA, sizeof(id_nameA), id_nameW);

        code = pkrb5_parse_name(k5_identpro_ctx, id_nameA, &princ);
        assert(code == 0);

        if (code == 0) {
            wchar_t realmW[K5_MAXCCH_REALM] = L"";
            wchar_t fcW[KCDB_MAXCCH_NAME] = L"";
            wchar_t idW[KCDB_IDENT_MAXCCH_NAME] = L"";
            int l;

            l = MultiByteToWideChar(CP_UTF8, 0,
                                    krb5_princ_realm(k5_identpro_ctx, princ)->data,
                                    krb5_princ_realm(k5_identpro_ctx, princ)->length,
                                    realmW, ARRAYLENGTH(realmW));
            realmW[max(l, ARRAYLENGTH(realmW) - 1)] = L'\0';

            kcdb_identity_set_attr(ident, KCDB_ATTR_NAME_DOMAIN,
                                   realmW, KCDB_CBSIZE_AUTO);

            l = MultiByteToWideChar(CP_UTF8, 0,
                                    krb5_princ_name(k5_identpro_ctx, princ)->data,
                                    krb5_princ_name(k5_identpro_ctx, princ)->length,
                                    fcW, ARRAYLENGTH(fcW));

            fcW[max(l, ARRAYLENGTH(fcW) - 1)] = L'\0';

            StringCbPrintf(idW, sizeof(idW), L"%s@%s", fcW, realmW);

            kcdb_identity_set_attr(ident, KCDB_ATTR_NAME_EMAIL,
                                   idW, KCDB_CBSIZE_AUTO);

            pkrb5_free_principal(k5_identpro_ctx, princ);
            princ = NULL;
        }
    }

    /* if there is a default identity already, we assume we don't need
       to check this one. */
    {
        khm_handle def_ident;

        if (KHM_SUCCEEDED(kcdb_identity_get_default_ex(k5_identpro, &def_ident))) {
            kcdb_identity_release(def_ident);

            return KHM_ERROR_SUCCESS;
        }
    }

    code = pkrb5_cc_default(k5_identpro_ctx, &cc);
    if (code)
        goto _nc_cleanup;

    code = pkrb5_cc_get_principal(k5_identpro_ctx,
                                  cc,
                                  &princ);
    if (code)
        goto _nc_cleanup;

    code = pkrb5_unparse_name(k5_identpro_ctx,
                              princ,
                              &princ_nameA);
    if (code)
        goto _nc_cleanup;

    AnsiStrToUnicode(princ_nameW,
                     sizeof(princ_nameW),
                     princ_nameA);

    if (!wcscmp(id_nameW, princ_nameW)) {
        kcdb_identity_set_default_int(ident);
    }

 _nc_cleanup:
    if (princ_nameA)
        pkrb5_free_unparsed_name(k5_identpro_ctx,
                                 princ_nameA);
    if (princ)
        pkrb5_free_principal(k5_identpro_ctx,
                             princ);
    if (cc)
        pkrb5_cc_close(k5_identpro_ctx, cc);

    return KHM_ERROR_SUCCESS;
}

struct k5_ident_update_data {
    khm_handle identity;

    FILETIME   ft_expire;      /* expiration */
    FILETIME   ft_issue;       /* issue */
    FILETIME   ft_rexpire;     /* renew expiration */
    wchar_t    ccname[KRB5_MAXCCH_CCNAME];
    khm_int32  k5_flags;
};

/* The logic here has to reflect the logic in khm_krb5_list_tickets().
   We use this to handle an identity update request because some other
   plug-in or maybe NetIDMgr itself is about to do something
   important(tm) with the identity and needs to make sure that the
   properties of the identity are up-to-date. */
static khm_int32 KHMAPI
k5_ident_update_apply_proc(khm_handle cred,
                           void * rock) {
    struct k5_ident_update_data * d = (struct k5_ident_update_data *) rock;
    khm_handle ident = NULL;
    khm_int32 t;
    khm_int32 flags;
    FILETIME t_cexpire;
    FILETIME t_rexpire;
    khm_size cb;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if (KHM_FAILED(kcdb_cred_get_type(cred, &t)) ||
        t != credtype_id_krb5 ||
        KHM_FAILED(kcdb_cred_get_identity(cred, &ident)))

        return KHM_ERROR_SUCCESS;

    if (!kcdb_identity_is_equal(ident,d->identity))

        goto _cleanup;

    if (KHM_FAILED(kcdb_cred_get_flags(cred, &flags)))

        flags = 0;

    if (flags & KCDB_CRED_FLAG_INITIAL) {
        cb = sizeof(t_cexpire);
        if (KHM_SUCCEEDED(kcdb_cred_get_attr(cred,
                                             KCDB_ATTR_EXPIRE,
                                             NULL,
                                             &t_cexpire,
                                             &cb))) {
            if ((d->ft_expire.dwLowDateTime == 0 &&
                 d->ft_expire.dwHighDateTime == 0) ||
                CompareFileTime(&t_cexpire, &d->ft_expire) > 0) {
                goto update_identity;
            }
        }
    }

    goto _cleanup;

 update_identity:

    d->ft_expire = t_cexpire;

    cb = sizeof(d->ccname);
    if (KHM_FAILED(kcdb_cred_get_attr(cred, KCDB_ATTR_LOCATION, NULL, d->ccname, &cb))) {
        d->ccname[0] = L'\0';
    }

    cb = sizeof(d->k5_flags);
    if (KHM_FAILED(kcdb_cred_get_attr(cred, attr_id_krb5_flags, NULL,
                                         &d->k5_flags, &cb))) {
        d->k5_flags = 0;
    }

    cb = sizeof(d->ft_issue);
    if (KHM_FAILED(kcdb_cred_get_attr(cred, KCDB_ATTR_ISSUE, NULL, &d->ft_issue, &cb))) {
        ZeroMemory(&d->ft_issue, sizeof(d->ft_issue));
    }

    cb = sizeof(t_rexpire);
    if ((d->k5_flags & TKT_FLG_RENEWABLE) &&
        KHM_SUCCEEDED(kcdb_cred_get_attr(cred,
                                         KCDB_ATTR_RENEW_EXPIRE,
                                         NULL,
                                         &t_rexpire,
                                         &cb))) {
        d->ft_rexpire = t_rexpire;
    } else {
        ZeroMemory(&d->ft_rexpire, sizeof(d->ft_rexpire));
    }

 _cleanup:
    if (ident)
        kcdb_identity_release(ident);

    return rv;
}

static khm_int32
k5_ident_update(khm_int32 msg_type,
                khm_int32 msg_subtype,
                khm_ui_4 uparam,
                void * vparam) {

#if 0
    struct k5_ident_update_data d;
#endif
    khm_handle ident;
    khm_handle tident;
    krb5_ccache cc = NULL;
    char * ccname;
    krb5_error_code code;
    khm_size cb;
    wchar_t wid_ccname[MAX_PATH];
    wchar_t w_ccname[MAX_PATH];

    ident = (khm_handle) vparam;
    if (ident == NULL)
        return KHM_ERROR_SUCCESS;

#if 0
    /* we are going to skip doing this here since
       khm_krb5_list_tickets() performs this function for us each time
       we enumerate tickets.  Since it also gets run each time our
       list of tickets changes and since we are basing this operation
       on existing tickets, we are unlikely to find anything new
       here.  */
    ZeroMemory(&d, sizeof(d));
    d.identity = ident;

    kcdb_credset_apply(NULL,
                       k5_ident_update_apply_proc,
                       (void *) &d);

    if (d.ft_expire.dwLowDateTime != 0 ||
        d.ft_expire.dwHighDateTime != 0) {

        /* we found a TGT */

        kcdb_identity_set_attr(ident, KCDB_ATTR_EXPIRE,
                               &d.ft_expire, sizeof(d.ft_expire));
        if (d.ft_issue.dwLowDateTime != 0 ||
            d.ft_issue.dwHighDateTime != 0)
            kcdb_identity_set_attr(ident, KCDB_ATTR_ISSUE,
                                   &d.ft_issue, sizeof(d.ft_issue));
        else
            kcdb_identity_set_attr(ident, KCDB_ATTR_ISSUE, NULL, 0);

        if (d.ft_rexpire.dwLowDateTime != 0 ||
            d.ft_rexpire.dwHighDateTime != 0)
            kcdb_identity_set_attr(ident, KCDB_ATTR_RENEW_EXPIRE,
                                   &d.ft_rexpire, sizeof(d.ft_rexpire));
        else
            kcdb_identity_set_attr(ident, KCDB_ATTR_RENEW_EXPIRE, NULL, 0);

        kcdb_identity_set_attr(ident, attr_id_krb5_flags,
                               &d.k5_flags, sizeof(d.k5_flags));

        if (d.ccname[0])
            kcdb_identity_set_attr(ident, attr_id_krb5_ccname,
                                   d.ccname, KCDB_CBSIZE_AUTO);
        else
            kcdb_identity_set_attr(ident, attr_id_krb5_ccname, NULL, 0);

    } else {
        /* Clear out the attributes.  We don't have any information
           about this identity */
        kcdb_identity_set_attr(ident, KCDB_ATTR_EXPIRE, NULL, 0);
        kcdb_identity_set_attr(ident, KCDB_ATTR_ISSUE, NULL, 0);
        kcdb_identity_set_attr(ident, KCDB_ATTR_RENEW_EXPIRE, NULL, 0);
        kcdb_identity_set_attr(ident, attr_id_krb5_flags, NULL, 0);
        kcdb_identity_set_attr(ident, attr_id_krb5_ccname, NULL, 0);
    }
#endif

    if (KHM_SUCCEEDED(kcdb_identity_get_default_ex(k5_identpro, &tident))) {
        kcdb_identity_release(tident);
        goto _iu_cleanup;
    }

    cb = sizeof(wid_ccname);
    if (KHM_FAILED(kcdb_identity_get_attr(ident,
                                          attr_id_krb5_ccname,
                                          NULL,
                                          wid_ccname,
                                          &cb)))
        goto _iu_cleanup;

    if(k5_identpro_ctx == NULL)
        goto _iu_cleanup;

    code = pkrb5_cc_default(k5_identpro_ctx, &cc);
    if (code)
        goto _iu_cleanup;

    ccname = pkrb5_cc_get_name(k5_identpro_ctx, cc);
    if (ccname == NULL)
        goto _iu_cleanup;

    AnsiStrToUnicode(w_ccname, sizeof(w_ccname), ccname);

    khm_krb5_canon_cc_name(w_ccname, sizeof(w_ccname));
    khm_krb5_canon_cc_name(wid_ccname, sizeof(wid_ccname));

    if (!_wcsicmp(w_ccname, wid_ccname))
        kcdb_identity_set_default_int(ident);

 _iu_cleanup:
    if (cc && k5_identpro_ctx)
        pkrb5_cc_close(k5_identpro_ctx, cc);

    return KHM_ERROR_SUCCESS;
}

static khm_boolean
k5_refresh_default_identity(krb5_context ctx) {
    /* just like notify_create, except now we set the default identity
       based on what we find in the configuration */
    krb5_ccache cc = NULL;
    krb5_error_code code;
    krb5_principal princ = NULL;
    char * princ_nameA = NULL;
    wchar_t princ_nameW[KCDB_IDENT_MAXCCH_NAME];
    char * ccname = NULL;
    khm_handle ident = NULL;
    khm_boolean found_default = FALSE;

    assert(ctx != NULL);

    _begin_task(0);
    _report_cs0(KHERR_DEBUG_1, L"Refreshing default identity");
    _describe();

    code = pkrb5_cc_default(ctx, &cc);
    if (code) {
        _reportf(L"Can't open default ccache. code=%d", code);
        goto _nc_cleanup;
    }

    ccname = pkrb5_cc_get_name(ctx, cc);
    _reportf(L"CC name is [%S]", (ccname ? ccname : "[Unknown cache name]"));

    code = pkrb5_cc_get_principal(ctx, cc, &princ);
    if (code) {
        /* try to determine the identity from the ccache name */
        if (ccname) {
            char * namepart = strchr(ccname, ':');

            if (namepart == NULL)
                namepart = ccname;
            else
                namepart++;

            _reportf(L"Checking if [%S] is a valid identity name", namepart);

            AnsiStrToUnicode(princ_nameW, sizeof(princ_nameW), namepart);
            if (KHM_SUCCEEDED(k5_validate_name(princ_nameW))) {
                kcdb_identity_create_ex(k5_identpro, princ_nameW,
                                        KCDB_IDENT_FLAG_CREATE, NULL, &ident);
                if (ident) {
                    _reportf(L"Setting [%S] as the default identity", namepart);
                    kcdb_identity_set_default_int(ident);
                    found_default = TRUE;
                }
            }
        } else {
            _reportf(L"Can't determine ccache name");
        }

        goto _nc_cleanup;
    }

    code = pkrb5_unparse_name(ctx, princ, &princ_nameA);
    if (code)
        goto _nc_cleanup;

    AnsiStrToUnicode(princ_nameW, sizeof(princ_nameW), princ_nameA);

    _reportf(L"Found principal [%s]", princ_nameW);

    if (KHM_FAILED(kcdb_identity_create_ex(k5_identpro, princ_nameW,
                                           KCDB_IDENT_FLAG_CREATE, NULL, &ident))) {
        _reportf(L"Failed to create identity");
        goto _nc_cleanup;
    }

    _reportf(L"Setting default identity to [%s]", princ_nameW);
    kcdb_identity_set_default_int(ident);

    found_default = TRUE;

 _nc_cleanup:

    _end_task();

    if (princ_nameA)
        pkrb5_free_unparsed_name(ctx, princ_nameA);

    if (princ)
        pkrb5_free_principal(ctx, princ);

    if (cc)
        pkrb5_cc_close(ctx, cc);

    if (ident)
        kcdb_identity_release(ident);

    return found_default;
}

static khm_int32
k5_ident_init(khm_int32 msg_type,
              khm_int32 msg_subtype,
              khm_ui_4 uparam,
              void * vparam) {

    khm_boolean found_default = FALSE;
    khm_handle ident = NULL;

    /* First get a handle to self.  Something is very wrong if we
       can't do that. */
    if (KHM_FAILED(kcdb_identpro_find(L"Krb5Ident", &k5_identpro))) {
        return KHM_ERROR_UNKNOWN;
    }

    /* And set the primary credentials type. */
    kcdb_identity_set_type(credtype_id_krb5);

    found_default = k5_refresh_default_identity(k5_identpro_ctx);

    if (!found_default) {
        wchar_t widname[KCDB_IDENT_MAXCCH_NAME];
        khm_size cb;

        cb = sizeof(widname);

        assert(csp_params);

        if (KHM_SUCCEEDED(khc_read_string(csp_params, L"LastDefaultIdent",
                                          widname, &cb))) {
            ident = NULL;
            kcdb_identity_create_ex(k5_identpro, widname,
                                    KCDB_IDENT_FLAG_CREATE, NULL, &ident);
            if (ident) {
                kcdb_identity_set_default_int(ident);
                kcdb_identity_release(ident);
                ident = NULL;

                found_default = TRUE;
            }
        }
    }

    if (!found_default) {

        /* There was no default ccache and we don't have a
           "LastDefaultIdent" value. Next we see if there are any
           identities that have credentials which have a Krb5CCName
           property (i.e. an identity that has a Kerberos 5 TGT), and
           make it the default.

           Note that since the Krb5Ident plug-in has a dependency on
           Krb5Cred, by the time this code runs, we already have a
           listing of Kerberos 5 tickets and identities. */

        khm_size cb = 0;
        khm_size n_idents = 0;
        khm_int32 rv;
        wchar_t ccname[KRB5_MAXCCH_CCNAME];
        FILETIME ft_expire;
        FILETIME ft_now;
        FILETIME ft_threshold;
        BOOL match_all = FALSE;
        kcdb_enumeration e = NULL;

        TimetToFileTimeInterval(5 * 60, &ft_threshold);
        GetSystemTimeAsFileTime(&ft_now);
        ft_now = FtAdd(&ft_now, &ft_threshold);

        rv = kcdb_identity_begin_enum(0, 0, &e, &n_idents);

        if (KHM_SUCCEEDED(rv)) {

            /* first we try to find an identity that has a valid TGT.
               If that fails, then we try to find an identity with
               *any* TGT. */

        try_again:

#ifdef DEBUG
            assert(ident == NULL);
#endif
            ident = NULL;

            while (KHM_SUCCEEDED(kcdb_enum_next(e, &ident))) {
                khm_size cb_ft = sizeof(FILETIME);
                cb = sizeof(ccname);

                if (KHM_SUCCEEDED(kcdb_identity_get_attr(ident, attr_id_krb5_ccname,
                                                         NULL, ccname, &cb)) &&
                    (match_all ||
                     (KHM_SUCCEEDED(kcdb_identity_get_attr(ident, KCDB_ATTR_EXPIRE,
                                                           NULL, &ft_expire, &cb_ft)) &&
                      CompareFileTime(&ft_expire, &ft_now) > 0))) {

                    /* found one */
                    k5_ident_set_default_int(ident);
                    kcdb_identity_set_default_int(ident);
                    found_default = TRUE;
                }
            }

            if (!found_default && !match_all) {
                match_all = TRUE;
                goto try_again;
            }

            kcdb_enum_end(e);
        }
    }

    /* It is now safe to resume the credentials provider message
       pump. */
    assert(h_idprov_event);
    if (h_idprov_event)
        SetEvent(h_idprov_event);

    return KHM_ERROR_SUCCESS;
}

static khm_int32
k5_ident_exit(khm_int32 msg_type,
              khm_int32 msg_subtype,
              khm_ui_4 uparam,
              void * vparam) {
    /* don't really do anything */
    return KHM_ERROR_SUCCESS;
}

/* forward dcl */
khm_int32 KHMAPI
k5_ident_name_comp_func(const void * dl, khm_size cb_dl,
                        const void * dr, khm_size cb_dr);

static khm_int32
k5_ident_compare_name(khm_int32 msg_type,
                      khm_int32 msg_subtype,
                      khm_ui_4 uparam,
                      void * vparam) {
    kcdb_ident_name_xfer *px;

    px = (kcdb_ident_name_xfer *) vparam;

    /* note that k5_ident_name_comp_func() ignores the size
       specifiers.  So we can just pass in 0's. */
    px->result = k5_ident_name_comp_func(px->name_src, 0,
                                         px->name_alt, 0);

    return KHM_ERROR_SUCCESS;
}

static khm_int32
k5_ident_resource_req(kcdb_resource_request * preq)
{
    wchar_t buf[KCDB_MAXCCH_SHORT_DESC];
    HICON   hicon;
    BOOL found = TRUE;
    size_t cb = 0;

    buf[0] = L'\0';
    hicon = NULL;

    if (preq->h_obj != NULL) {

        /* This is a resource request for a specific identity
           specified by h_obj */
        switch(preq->res_id) {

        case KCDB_RES_ICON_DISABLED:
        case KCDB_RES_ICON_NORMAL:
            hicon = LoadImage(hResModule, MAKEINTRESOURCE(IDI_KERBEROS),
                              IMAGE_ICON, 0, 0,
                              LR_DEFAULTSIZE | LR_DEFAULTCOLOR);
            break;

        case KCDB_RES_INSTANCE:
            LoadString(hResModule, IDS_ID_INSTANCE, buf, ARRAYLENGTH(buf));
            break;

            /* failover to NIM */
        case KCDB_RES_DESCRIPTION:
            return KHM_ERROR_SUCCESS;

        default:
            assert(FALSE);
        }

    } else {

        /* This is a resource request for the identity provider */
        switch(preq->res_id) {
        case KCDB_RES_DISPLAYNAME:
            LoadString(hResModule, IDS_ID_DISPLAYNAME, buf, ARRAYLENGTH(buf));
            break;

        case KCDB_RES_DESCRIPTION:
            LoadString(hResModule, IDS_ID_DESCRIPTION, buf, ARRAYLENGTH(buf));
            break;

        case KCDB_RES_TOOLTIP:
            LoadString(hResModule, IDS_ID_TOOLTIP, buf, ARRAYLENGTH(buf));
            break;

        case KCDB_RES_INSTANCE:
            LoadString(hResModule, IDS_ID_INSTANCE, buf, ARRAYLENGTH(buf));
            break;

        case KCDB_RES_ICON_NORMAL:
            hicon = LoadImage(hResModule, MAKEINTRESOURCE(IDI_KERBEROS),
                              IMAGE_ICON, 0, 0,
                              LR_DEFAULTSIZE | LR_DEFAULTCOLOR);
            break;

        default:
            found = FALSE;
        }
    }

    if (found && buf[0] != L'\0' &&
        SUCCEEDED(StringCbLength(buf, sizeof(buf), &cb))) {
        cb += sizeof(wchar_t);
        if (preq->buf == NULL || preq->cb_buf < cb) {
            preq->cb_buf = cb;
            preq->code = KHM_ERROR_TOO_LONG;
        } else {
            StringCbCopy(preq->buf, preq->cb_buf, buf);
            preq->cb_buf = cb;
            preq->code = KHM_ERROR_SUCCESS;
        }
    } else if (found && hicon != NULL) {
        if (preq->buf == NULL || preq->cb_buf < sizeof(HICON)) {
            preq->cb_buf = sizeof(HICON);
            preq->code = KHM_ERROR_TOO_LONG;
        } else {
            *((HICON *) preq->buf) = hicon;
            preq->cb_buf = sizeof(HICON);
            preq->code = KHM_ERROR_SUCCESS;
        }
    }

    return KHM_ERROR_SUCCESS;
}

#if 0
/* copy and paste template for ident provider messages */
static khm_int32
k5_ident_(khm_int32 msg_type,
          khm_int32 msg_subtype,
          khm_ui_4 uparam,
          void * vparam) {
}
#endif

khm_int32 KHMAPI
k5_msg_ident(khm_int32 msg_type,
               khm_int32 msg_subtype,
               khm_ui_4 uparam,
               void * vparam)
{
    switch(msg_subtype) {
    case KMSG_IDENT_INIT:
        return k5_ident_init(msg_type,
                             msg_subtype,
                             uparam,
                             vparam);

    case KMSG_IDENT_EXIT:
        return k5_ident_exit(msg_type,
                             msg_subtype,
                             uparam,
                             vparam);

    case KMSG_IDENT_VALIDATE_NAME:
        return k5_ident_validate_name(msg_type,
                                      msg_subtype,
                                      uparam,
                                      vparam);

    case KMSG_IDENT_VALIDATE_IDENTITY:
        /* TODO: handle KMSG_IDENT_VALIDATE_IDENTITY */
        break;

    case KMSG_IDENT_CANON_NAME:
        /* TODO: handle KMSG_IDENT_CANON_NAME */
        break;

    case KMSG_IDENT_COMPARE_NAME:
        return k5_ident_compare_name(msg_type,
                                     msg_subtype,
                                     uparam,
                                     vparam);

    case KMSG_IDENT_SET_DEFAULT:
        return k5_ident_set_default(msg_type,
                                    msg_subtype,
                                    uparam,
                                    vparam);

    case KMSG_IDENT_SET_SEARCHABLE:
        /* TODO: handle KMSG_IDENT_SET_SEARCHABLE */
        break;

    case KMSG_IDENT_GET_INFO:
        /* TODO: handle KMSG_IDENT_GET_INFO */
        break;

    case KMSG_IDENT_UPDATE:
        return k5_ident_update(msg_type,
                               msg_subtype,
                               uparam,
                               vparam);

    case KMSG_IDENT_ENUM_KNOWN:
        /* TODO: handle KMSG_IDENT_ENUM_KNOWN */
        break;

    case KMSG_IDENT_NOTIFY_CREATE:
        return k5_ident_notify_create(msg_type,
                                      msg_subtype,
                                      uparam,
                                      vparam);

    case KMSG_IDENT_RESOURCE_REQ:
        return k5_ident_resource_req(vparam);
    }

    return KHM_ERROR_SUCCESS;
}

/* note that we are ignoring the size specifiers.  We can do that
   because we are guaranteed that dl and dr point to NULL terminated
   unicode strings when used with credential data buffers.  We also
   use the fact that we are ignoring the size specifiers when we call
   this function from k5_ident_compare_name() to avoid calculating the
   length of the string. */
khm_int32 KHMAPI
k5_ident_name_comp_func(const void * dl, khm_size cb_dl,
                        const void * dr, khm_size cb_dr) {
    wchar_t * idl = (wchar_t *) dl;
    wchar_t * idr = (wchar_t *) dr;
    wchar_t * rl;
    wchar_t * rr;
    khm_int32 r;

    rl = khm_get_realm_from_princ(idl);
    rr = khm_get_realm_from_princ(idr);

    if (rl == NULL && rr == NULL)
        return wcscmp(idl, idr);
    else if (rl == NULL)
        return 1;
    else if (rr == NULL)
        return -1;

    r = wcscmp(rl, rr);
    if (r == 0)
        return wcscmp(idl, idr);
    else
        return r;
}


/* Identity change notification thread */

static HANDLE h_ccname_exit_event;
static HANDLE h_ccname_thread;

unsigned __stdcall k5_ccname_monitor_thread(void * lpParameter) {

    HKEY hk_ccname;
    HANDLE h_notify;
    HANDLE h_waits[2];

    khm_int32 rv = KHM_ERROR_SUCCESS;
    DWORD dwType;
    DWORD dwSize;
    DWORD dwDisp;
    wchar_t reg_ccname[KRB5_MAXCCH_CCNAME];
    LONG l;

    PDESCTHREAD(L"Krb5 CCName Monitor", L"Krb5");

    l = RegOpenKeyEx(HKEY_CURRENT_USER,
                     L"Software\\MIT\\kerberos5",
                     0,
                     KEY_READ | KEY_WRITE,
                     &hk_ccname);

    if (l != ERROR_SUCCESS)
        l = RegCreateKeyEx(HKEY_CURRENT_USER,
                           L"Software\\MIT\\kerberos5",
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_READ | KEY_WRITE,
                           NULL,
                           &hk_ccname,
                           &dwDisp);

    if (l != ERROR_SUCCESS) {
        rv = KHM_ERROR_UNKNOWN;
        goto _exit;
    }

    dwSize = sizeof(reg_ccname);

    l = RegQueryValueEx(hk_ccname,
                        L"ccname",
                        NULL,
                        &dwType,
                        (LPBYTE) reg_ccname,
                        &dwSize);

    if (l != ERROR_SUCCESS ||
        dwType != REG_SZ) {

        reg_ccname[0] = L'\0';
    }

    h_notify = CreateEvent(NULL, FALSE, FALSE, L"Local\\Krb5CCNameChangeNotifier");

    if (h_notify == NULL)
        goto _exit_0;

    /* begin wait loop */

    h_waits[0] = h_ccname_exit_event;
    h_waits[1] = h_notify;

    do {
        DWORD dwrv;

        l = RegNotifyChangeKeyValue(hk_ccname, FALSE,
                                    REG_NOTIFY_CHANGE_LAST_SET,
                                    h_notify, TRUE);

        if (l != ERROR_SUCCESS) {
            rv = KHM_ERROR_UNKNOWN;
            break;
        }

        dwrv = WaitForMultipleObjects(2, h_waits, FALSE, INFINITE);

        if (dwrv == WAIT_OBJECT_0) {
            /* exit! */
            break;

        } else if (dwrv == WAIT_OBJECT_0 + 1) {
            /* change notify! */
            wchar_t new_ccname[KRB5_MAXCCH_CCNAME];

            dwSize = sizeof(new_ccname);

            l = RegQueryValueEx(hk_ccname,
                                L"ccname",
                                NULL,
                                &dwType,
                                (LPBYTE) new_ccname,
                                &dwSize);

            if (l != ERROR_SUCCESS ||
                dwType != REG_SZ) {
                new_ccname[0] = L'\0';
            }

            if (_wcsicmp(new_ccname, reg_ccname)) {
                krb5_context ctx = NULL;

                l = pkrb5_init_context(&ctx);
                if (l == 0 && ctx != NULL) {
                    k5_refresh_default_identity(ctx);
                    StringCbCopy(reg_ccname, sizeof(reg_ccname), new_ccname);
                }

                if (ctx)
                    pkrb5_free_context(ctx);
            }

        } else {
            /* something went wrong */
            rv = KHM_ERROR_UNKNOWN;
            break;
        }

    } while (TRUE);

    CloseHandle(h_notify);

 _exit_0:

    RegCloseKey(hk_ccname);

 _exit:
    return (unsigned) rv;
}

khm_int32
k5_msg_system_idpro(khm_int32 msg_type, khm_int32 msg_subtype,
                    khm_ui_4 uparam, void * vparam) {

    switch(msg_subtype) {
    case KMSG_SYSTEM_INIT:
        {

            pkrb5_init_context(&k5_identpro_ctx);

            h_ccname_exit_event = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (h_ccname_exit_event) {
                h_ccname_thread = (HANDLE) _beginthreadex(NULL, 8 * 4096,
                                                          k5_ccname_monitor_thread,
                                                          NULL, 0, NULL);
            } else {
                h_ccname_thread = NULL;
            }
        }
        break;

    case KMSG_SYSTEM_EXIT:
        {

            if (h_ccname_thread) {
                SetEvent(h_ccname_exit_event);
                WaitForSingleObject(h_ccname_thread, INFINITE);
                CloseHandle(h_ccname_thread);
                CloseHandle(h_ccname_exit_event);

                h_ccname_exit_event = NULL;
                h_ccname_thread = NULL;
            }

            if (k5_identpro_ctx) {
                pkrb5_free_context(k5_identpro_ctx);
                k5_identpro_ctx = NULL;
            }
        }
        break;
    }

    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMAPI
k5_ident_callback(khm_int32 msg_type, khm_int32 msg_subtype,
                  khm_ui_4 uparam, void * vparam) {
    switch(msg_type) {
    case KMSG_SYSTEM:
        return k5_msg_system_idpro(msg_type, msg_subtype, uparam, vparam);

    case KMSG_IDENT:
        return k5_msg_ident(msg_type, msg_subtype, uparam, vparam);
    }

    return KHM_ERROR_SUCCESS;
}
