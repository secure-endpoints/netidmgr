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

#include "khmapp.h"
#include<assert.h>
#if _WIN32_WINNT >= 0x0501
#include<uxtheme.h>
#endif

static khui_config_node
get_window_node(HWND hwnd)
{
    return (khui_config_node) (LONG_PTR)
        GetWindowLongPtr(hwnd, DWLP_USER);
}

static void
set_window_node(HWND hwnd, khui_config_node node)
{
#pragma warning(push)
#pragma warning(disable: 4244)
    SetWindowLongPtr(hwnd, DWLP_USER,
                     (LONG_PTR) node);
#pragma warning(pop)
}

static void
add_subpanels(HWND hwnd,
              khui_config_node ctx_node,
              khui_config_node ref_node)
{
    HWND hw_tab;
    HWND hw_target;
    khui_config_node sub;
    khui_config_node_reg reg;
    khui_config_init_data idata;
    int idx;

    hw_tab = GetDlgItem(hwnd, IDC_CFG_TAB);
    hw_target = GetDlgItem(hwnd, IDC_CFG_TARGET);
    assert(hw_tab);
    assert(hw_target);

    if (KHM_FAILED(khui_cfg_get_first_subpanel(ref_node, &sub))) {
        assert(FALSE);
        return;
    }

    idx = 0;
    while(sub) {
        HWND hwnd_panel;
        TCITEM tci;
        int iid;
        khm_int32 init_rc;

        khui_cfg_get_reg(sub, &reg);

        if ((ctx_node == ref_node && (reg.flags & KHUI_CNFLAG_INSTANCE)) ||
            (ctx_node != ref_node && !(reg.flags & KHUI_CNFLAG_INSTANCE)))
            goto _next_node;

        idata.ctx_node = ctx_node;
        idata.this_node = sub;
        idata.ref_node = ref_node;

        hwnd_panel = CreateDialogParam(reg.h_module,
                                       reg.dlg_template,
                                       hwnd,
                                       reg.dlg_proc,
                                       (LPARAM) &idata);

        assert(hwnd_panel);

        /* For backwards compatibility, we assume that if the
           configuration node is a Kerberos 5 identity or the identity
           globals node, then all panels default to 'Allow' if the
           WMCFG_INIT_PANEL message isn't handled and 'Disallow'
           otherwise. */
        if (ctx_node != ref_node) {
            khm_handle ident = khui_cfg_get_data(ctx_node);

            if (ident == NULL) {
                assert(FALSE);  /* Shouldn't happen */
                init_rc = KHM_ERROR_UNKNOWN;
            } else {
                khm_int32 ident_type = KCDB_CREDTYPE_INVALID;
                khm_int32 krb5_type = KCDB_CREDTYPE_INVALID;

                khm_size cb = sizeof(ident_type);

                kcdb_identity_get_attr(ident, KCDB_ATTR_TYPE, NULL, &ident_type, &cb);
                kcdb_credtype_get_id(L"Krb5Cred", &krb5_type);

                if (ident_type == krb5_type)
                    init_rc = KHM_ERROR_SUCCESS;
                else
                    init_rc = KHM_ERROR_NOT_IMPLEMENTED;
            }
        } else {
            /* identity globals node */
            init_rc = KHM_ERROR_SUCCESS;
        }

        /* If the panel doesn't initialize properly or determines that
           the subpanel does not apply to the context node, we have to
           skip to the next subpanel node. */

        SendMessage(hwnd_panel, KHUI_WM_CFG_NOTIFY,
                    MAKEWPARAM(0, WMCFG_INIT_PANEL), (LPARAM) &init_rc);

        if (KHM_FAILED(init_rc)) {
            DestroyWindow(hwnd_panel);
            goto _next_node;
        }

#if _WIN32_WINNT >= 0x0501
        EnableThemeDialogTexture(hwnd_panel, ETDT_ENABLETAB);
#endif

        ShowWindow(hwnd_panel, SW_HIDE);

        ZeroMemory(&tci, sizeof(tci));

        tci.mask = TCIF_PARAM | TCIF_TEXT;
        tci.lParam = (LPARAM) sub;
        tci.pszText = (LPWSTR) reg.short_desc;

        iid = TabCtrl_InsertItem(hw_tab, 0, &tci);
        idx++;

        if (reg.flags & KHUI_CNFLAG_INSTANCE) {
            khui_cfg_set_param_inst(sub, ctx_node, iid);
            khui_cfg_set_hwnd_inst(sub, ctx_node, hwnd_panel);
        } else {
            khui_cfg_set_param(sub, iid);
            khui_cfg_set_hwnd(sub, hwnd_panel);
        }

    _next_node:

        khui_cfg_get_next_release(&sub);
    }

    TabCtrl_SetCurSel(hw_tab, 0);
}

static void
apply_all(HWND hwnd,
          HWND hw_tab,
          khui_config_node noderef)
{
    TCITEM tci;
    HWND hw;
    khui_config_node_reg reg;
    int idx;
    int count;
    khm_boolean cont = TRUE;

    count = TabCtrl_GetItemCount(hw_tab);

    for (idx = 0; idx < count && cont; idx++) {

        ZeroMemory(&tci, sizeof(tci));

        tci.mask = TCIF_PARAM;
        TabCtrl_GetItem(hw_tab,
                        idx,
                        &tci);

        assert(tci.lParam);
        khui_cfg_get_reg((khui_config_node) tci.lParam, &reg);
        if (reg.flags & KHUI_CNFLAG_INSTANCE)
            hw = khui_cfg_get_hwnd_inst((khui_config_node) tci.lParam,
                                        noderef);
        else
            hw = khui_cfg_get_hwnd((khui_config_node) tci.lParam);
        assert(hw);

        SendMessage(hw, KHUI_WM_CFG_NOTIFY,
                    MAKEWPARAM(0, WMCFG_APPLY), (LPARAM) &cont);
    }
}

static void
show_tab_panel(HWND hwnd,
               khui_config_node node,
               HWND hw_tab,
               int idx,
               khm_boolean show)
{
    TCITEM tci;
    HWND hw;
    HWND hw_target;
    HWND hw_firstctl;
    RECT r;
    RECT rref;
    khui_config_node_reg reg;

    ZeroMemory(&tci, sizeof(tci));

    tci.mask = TCIF_PARAM;
    TabCtrl_GetItem(hw_tab,
                    idx,
                    &tci);

    assert(tci.lParam);
    khui_cfg_get_reg((khui_config_node) tci.lParam, &reg);
    if (reg.flags & KHUI_CNFLAG_INSTANCE)
        hw = khui_cfg_get_hwnd_inst((khui_config_node) tci.lParam,
                                    node);
    else
        hw = khui_cfg_get_hwnd((khui_config_node) tci.lParam);
    assert(hw);

    if (!show) {
        ShowWindow(hw, SW_HIDE);
        return;
    }

    hw_target = GetDlgItem(hwnd, IDC_CFG_TARGET);
    assert(hw_target);

    GetWindowRect(hwnd, &rref);
    GetWindowRect(hw_target, &r);

    OffsetRect(&r, -rref.left, -rref.top);

    SetWindowPos(hw,
                 hw_tab,
                 r.left, r.top,
                 r.right - r.left, r.bottom - r.top,
                 SWP_NOACTIVATE | SWP_NOOWNERZORDER |
                 SWP_SHOWWINDOW);

    hw_firstctl = GetNextDlgTabItem(hw, NULL, FALSE);
    if (hw_firstctl) {
        SetFocus(hw_firstctl);
    }
}

static INT_PTR
handle_cfg_notify(HWND hwnd,
                  WPARAM wParam,
                  LPARAM lParam) {
    khui_config_node node;
    HWND hw;

    node = get_window_node(hwnd);
    if (node == NULL)
        return TRUE;

    if (HIWORD(wParam) == WMCFG_APPLY) {

        hw = GetDlgItem(hwnd, IDC_CFG_TAB);

        apply_all(hwnd,
                  hw,
                  node);
    }

    return TRUE;
}

static INT_PTR
handle_notify(HWND hwnd,
              WPARAM wParam,
              LPARAM lParam)
{
    LPNMHDR lpnm;
    int i;


    khui_config_node node;

    lpnm = (LPNMHDR) lParam;
    node = get_window_node(hwnd);
    if (node == NULL)
        return FALSE;

    if (lpnm->idFrom == IDC_CFG_TAB) {
        switch(lpnm->code) {
        case TCN_SELCHANGING:
            i = TabCtrl_GetCurSel(lpnm->hwndFrom);

            show_tab_panel(hwnd,
                           node,
                           lpnm->hwndFrom,
                           i,
                           FALSE);
            break;

        case TCN_SELCHANGE:
            i = TabCtrl_GetCurSel(lpnm->hwndFrom);

            show_tab_panel(hwnd,
                           node,
                           lpnm->hwndFrom,
                           i,
                           TRUE);
            break;
        }
        return TRUE;
    } else {
        return FALSE;
    }
}

typedef struct tag_ident_props {
    khm_boolean monitor;
    khm_boolean auto_renew;
    khm_boolean sticky;
} ident_props;

typedef struct tag_ident_data {
    khm_handle  ident;
    int lv_idx;

    khm_boolean removed;               /* this identity was marked for removal */
    khm_boolean applied;
    khm_boolean purged;                /* this identity was actually removed */

    khm_int32   flags;

    ident_props saved;
    ident_props work;

    HWND hwnd;
} ident_data;

typedef struct tag_global_props {
    khm_boolean monitor;
    khm_boolean auto_renew;
    khm_boolean sticky;
} global_props;

typedef struct tag_idents_data {
    khm_boolean valid;

    ident_data *idents;
    khm_size    n_idents;
    khm_size    nc_idents;
#define IDENTS_DATA_ALLOC_INCR 8

    /* global options */
    global_props        saved;
    global_props        work;
    khm_boolean         applied;

    int         refcount;

    HIMAGELIST  hi_status;
    int         idx_id;
    int         idx_default;
    int         idx_modified;
    int         idx_applied;
    int         idx_deleted;

    HWND        hwnd;
    khui_config_init_data       cfg;
} idents_data;

static idents_data cfg_idents = {
    FALSE,
    NULL, 0, 0,
    {0, 0, 0},
    {0, 0, 0},
    FALSE,

    0,

    NULL, 0, 0, 0, 0, 0,

    NULL,
    {NULL, NULL, NULL}
};

static void
read_params_ident(ident_data * d)
{
    khm_handle csp_ident;
    khm_int32 t;

    if (KHM_FAILED(kcdb_identity_get_config(d->ident,
                                            KHM_PERM_READ | KCONF_FLAG_SHADOW,
                                            &csp_ident))) {
        csp_ident = NULL;
    }

    if (KHM_FAILED(khc_read_int32(csp_ident, L"Monitor", &t))) {
        assert(FALSE);
        d->saved.monitor = TRUE;
    } else {
        d->saved.monitor = !!t;
    }

    if (KHM_FAILED(khc_read_int32(csp_ident, L"AllowAutoRenew", &t))) {
        assert(FALSE);
        d->saved.auto_renew = TRUE;
    } else {
        d->saved.auto_renew = !!t;
    }

    if (KHM_FAILED(khc_read_int32(csp_ident, L"Sticky", &t))) {
        d->saved.sticky = FALSE;
    } else {
        d->saved.sticky = !!t;
    }

    khc_close_space(csp_ident);

    d->work = d->saved;
    d->applied = FALSE;
}

static void
write_params_ident(ident_data * d)
{
    khm_handle csp_ident;

    if (d->saved.monitor == d->work.monitor &&
        d->saved.auto_renew == d->work.auto_renew &&
        d->saved.sticky == d->work.sticky &&
        !d->removed)
        return;

    if (KHM_FAILED(kcdb_identity_get_config(d->ident,
                                            KHM_PERM_WRITE |
                                            ((!d->removed)? (KHM_FLAG_CREATE |
                                                             KCONF_FLAG_SHADOW |
                                                             KCONF_FLAG_WRITEIFMOD) : 0),
                                            &csp_ident))) {
        assert(FALSE);
        return;
    }

    if (d->removed) {
        khm_handle h = NULL;
        khm_int32 flags = 0;

        khc_remove_space(csp_ident);

        /* calling kcdb_identity_get_config() will update the
           KCDB_IDENT_FLAG_CONFIG flag for the identity to reflect the
           fact that it nolonger has a configuration. */
        kcdb_identity_get_config(d->ident, 0, &h);
        if (h) {
            /* what the ? */
            assert(FALSE);
            khc_close_space(h);
        }
        kcdb_identity_get_flags(d->ident, &flags);
        assert(!(flags & KCDB_IDENT_FLAG_CONFIG));

        d->purged = TRUE;

    } else {

        khc_write_int32(csp_ident, L"Monitor", !!d->work.monitor);
        khc_write_int32(csp_ident, L"AllowAutoRenew",
                        !!d->work.auto_renew);

        if (d->saved.sticky != d->work.sticky) {
            kcdb_identity_set_flags(d->ident,
                                    (d->work.sticky)?KCDB_IDENT_FLAG_STICKY:0,
                                    KCDB_IDENT_FLAG_STICKY);
        }
    }

    khc_close_space(csp_ident);

    d->saved = d->work;

    d->applied = TRUE;

    if (d->hwnd && !d->removed)
        PostMessage(d->hwnd, KHUI_WM_CFG_NOTIFY,
                    MAKEWPARAM(0, WMCFG_UPDATE_STATE), 0);

    khm_refresh_config();
}

static void
write_params_idents(void)
{
    khm_handle csp_kcdb = NULL;

    if (KHM_SUCCEEDED(khc_open_space(NULL, L"KCDB",
                                     KHM_FLAG_CREATE, &csp_kcdb))) {
        if (cfg_idents.work.monitor != cfg_idents.saved.monitor) {
            khc_write_int32(csp_kcdb, L"DefaultMonitor",
                            !!cfg_idents.work.monitor);
            cfg_idents.saved.monitor = cfg_idents.work.monitor;
            cfg_idents.applied = TRUE;
        }
        if (cfg_idents.work.auto_renew != cfg_idents.saved.auto_renew) {
            khc_write_int32(csp_kcdb, L"DefaultAllowAutoRenew",
                            !!cfg_idents.work.auto_renew);
            cfg_idents.saved.auto_renew = cfg_idents.work.auto_renew;
            cfg_idents.applied = TRUE;
        }
        if (cfg_idents.work.sticky != cfg_idents.saved.sticky) {
            khc_write_int32(csp_kcdb, L"DefaultSticky",
                            !!cfg_idents.work.sticky);
            cfg_idents.saved.sticky = cfg_idents.work.sticky;
            cfg_idents.applied = TRUE;
        }

        khc_close_space(csp_kcdb);
        csp_kcdb = NULL;
    }

    if (cfg_idents.hwnd)
        PostMessage(cfg_idents.hwnd, KHUI_WM_CFG_NOTIFY,
                    MAKEWPARAM(0, WMCFG_UPDATE_STATE), 0);
}

static void
init_idents_data(void)
{
    kcdb_enumeration e;
    khm_handle ident;
    khm_int32 rv;
    int n_tries = 0;
    int i;
    khm_handle csp_kcdb = NULL;

    if (cfg_idents.valid)
        return;

    assert(cfg_idents.idents == NULL);
    assert(cfg_idents.n_idents == 0);
    assert(cfg_idents.nc_idents == 0);

    if (KHM_SUCCEEDED(khc_open_space(NULL, L"KCDB", 0, &csp_kcdb))) {
        khm_int32 t;

        if (KHM_SUCCEEDED(khc_read_int32(csp_kcdb, L"DefaultMonitor", &t)))
            cfg_idents.saved.monitor = !!t;
        else
            cfg_idents.saved.monitor = TRUE;

        if (KHM_SUCCEEDED(khc_read_int32(csp_kcdb, L"DefaultAllowAutoRenew", &t)))
            cfg_idents.saved.auto_renew = !!t;
        else
            cfg_idents.saved.auto_renew = TRUE;

        if (KHM_SUCCEEDED(khc_read_int32(csp_kcdb, L"DefaultSticky", &t)))
            cfg_idents.saved.sticky = !!t;
        else
            cfg_idents.saved.sticky = FALSE;

        khc_close_space(csp_kcdb);
        csp_kcdb = NULL;

    } else {

        cfg_idents.saved.monitor = TRUE;
        cfg_idents.saved.auto_renew = TRUE;
        cfg_idents.saved.sticky = FALSE;

    }

    cfg_idents.work = cfg_idents.saved;
    cfg_idents.applied = FALSE;

    rv = kcdb_identity_begin_enum(KCDB_IDENT_FLAG_CONFIG,
                                  KCDB_IDENT_FLAG_CONFIG,
                                  &e, &cfg_idents.n_idents);

    if (KHM_FAILED(rv)) {
        cfg_idents.n_idents = 0;
        goto _cleanup;
    }

    cfg_idents.idents = PMALLOC(sizeof(*cfg_idents.idents) *
                                cfg_idents.n_idents);
    assert(cfg_idents.idents);
    ZeroMemory(cfg_idents.idents,
               sizeof(*cfg_idents.idents) * cfg_idents.n_idents);
    cfg_idents.nc_idents = cfg_idents.n_idents;

    i = 0;
    ident = NULL;
    while (KHM_SUCCEEDED(kcdb_enum_next(e, &ident))) {

        cfg_idents.idents[i].ident = ident;
        kcdb_identity_hold(ident);
        cfg_idents.idents[i].removed = FALSE;

        kcdb_identity_get_flags(ident, &cfg_idents.idents[i].flags);
        assert(cfg_idents.idents[i].flags & KCDB_IDENT_FLAG_CONFIG);

        read_params_ident(&cfg_idents.idents[i]);

        i++;
    }

    kcdb_enum_end(e);

 _cleanup:

    cfg_idents.valid = TRUE;
}

static void
free_idents_data(void)
{
    int i;

    if (!cfg_idents.valid)
        return;

    for (i=0; i< (int) cfg_idents.n_idents; i++) {
        if (cfg_idents.idents[i].ident)
            kcdb_identity_release(cfg_idents.idents[i].ident);
    }

    if (cfg_idents.idents)
        PFREE(cfg_idents.idents);

    cfg_idents.idents = NULL;
    cfg_idents.n_idents = 0;
    cfg_idents.nc_idents = 0;
    cfg_idents.valid = FALSE;
}

static void
hold_idents_data(void)
{
    if (!cfg_idents.valid)
        init_idents_data();
    assert(cfg_idents.valid);
    cfg_idents.refcount++;
}

static void
release_idents_data(void)
{
    assert(cfg_idents.valid);
    cfg_idents.refcount--;

    if (cfg_idents.refcount == 0)
        free_idents_data();
}


static void
refresh_data_idents(HWND hwnd)
{
    cfg_idents.work.monitor =
        (IsDlgButtonChecked(hwnd, IDC_CFG_MONITOR) == BST_CHECKED);
    cfg_idents.work.auto_renew =
        (IsDlgButtonChecked(hwnd, IDC_CFG_RENEW) == BST_CHECKED);
    cfg_idents.work.sticky =
        (IsDlgButtonChecked(hwnd, IDC_CFG_STICKY) == BST_CHECKED);
}

static void
refresh_view_idents_state(HWND hwnd)
{
    khm_boolean modified;
    khm_boolean applied;
    khm_int32 flags = 0;

    applied = cfg_idents.applied;
    modified = (cfg_idents.work.monitor != cfg_idents.saved.monitor ||
                cfg_idents.work.auto_renew != cfg_idents.saved.auto_renew ||
                cfg_idents.work.sticky != cfg_idents.saved.sticky);

    if (modified)
        flags |= KHUI_CNFLAG_MODIFIED;
    if (applied)
        flags |= KHUI_CNFLAG_APPLIED;

    khui_cfg_set_flags_inst(&cfg_idents.cfg, flags,
                            KHUI_CNFLAG_APPLIED | KHUI_CNFLAG_MODIFIED);
}

/* dialog procedure for the "general" pane of the "identities"
   configuration node. */
INT_PTR CALLBACK
khm_cfg_ids_tab_proc(HWND hwnd,
                    UINT umsg,
                    WPARAM wParam,
                    LPARAM lParam)
{

    switch(umsg) {
    case WM_INITDIALOG:
        {
            HICON hicon;

            hold_idents_data();

            cfg_idents.hwnd = hwnd;
            cfg_idents.cfg = *((khui_config_init_data *) lParam);

            /* add the status icons */
            if (cfg_idents.hi_status)
                goto _done_with_icons;

            cfg_idents.hi_status =
                ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                 GetSystemMetrics(SM_CYSMICON),
                                 ILC_COLOR8 | ILC_MASK,
                                 4,4);

            hicon =
                LoadImage(khm_hInstance, MAKEINTRESOURCE(IDI_ID),
                          IMAGE_ICON,
                          GetSystemMetrics(SM_CXSMICON),
                          GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

            cfg_idents.idx_id = ImageList_AddIcon(cfg_idents.hi_status,
                                                  hicon);

            DestroyIcon(hicon);

            hicon = LoadImage(khm_hInstance, MAKEINTRESOURCE(IDI_CFG_DEFAULT),
                              IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                              GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

            cfg_idents.idx_default = ImageList_AddIcon(cfg_idents.hi_status,
                                                       hicon) + 1;

            DestroyIcon(hicon);

            hicon = LoadImage(khm_hInstance, MAKEINTRESOURCE(IDI_CFG_MODIFIED),
                              IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                              GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

            cfg_idents.idx_modified = ImageList_AddIcon(cfg_idents.hi_status,
                                                        hicon) + 1;

            DestroyIcon(hicon);

            hicon = LoadImage(khm_hInstance, MAKEINTRESOURCE(IDI_CFG_APPLIED),
                              IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                              GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

            cfg_idents.idx_applied = ImageList_AddIcon(cfg_idents.hi_status,
                                                       hicon) + 1;

            DestroyIcon(hicon);

            hicon = LoadImage(khm_hInstance, MAKEINTRESOURCE(IDI_CFG_DELETED),
                              IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                              GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

            cfg_idents.idx_deleted = ImageList_AddIcon(cfg_idents.hi_status,
                                                       hicon) + 1;

            DestroyIcon(hicon);

        _done_with_icons:

            CheckDlgButton(hwnd, IDC_CFG_MONITOR,
                           (cfg_idents.work.monitor)?BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_CFG_RENEW,
                           (cfg_idents.work.auto_renew)?BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_CFG_STICKY,
                           (cfg_idents.work.sticky)?BST_CHECKED:BST_UNCHECKED);

        }
        return FALSE;

    case WM_COMMAND:

        if (HIWORD(wParam) == BN_CLICKED) {
            UINT ctrl = LOWORD(wParam);

            switch(ctrl) {
            case IDC_CFG_MONITOR:
            case IDC_CFG_RENEW:
            case IDC_CFG_STICKY:
                refresh_data_idents(hwnd);
                break;

            case IDC_CFG_ADDIDENT:
                {
                    khm_handle identity = NULL;
                    khm_handle csp_id = NULL;

                    khm_cred_prompt_for_identity_modal(NULL, &identity);

                    if (identity == NULL)
                        break;

                    if (KHM_SUCCEEDED(kcdb_identity_get_config(identity,
                                                               KHM_FLAG_CREATE,
                                                               &csp_id))) {
                        khc_close_space(csp_id);

                        khm_refresh_config();
                    } else {
                        wchar_t err_title[64];
                        wchar_t err_msg[256];

                        LoadString(khm_hInstance, IDS_CFG_IDNAME_PRB,
                                   err_title, ARRAYLENGTH(err_title));
                        LoadString(khm_hInstance, IDS_CFG_IDNAME_CCC,
                                   err_msg, ARRAYLENGTH(err_msg));

                        MessageBox(hwnd, err_msg, err_title, MB_OK | MB_ICONSTOP);
                    }

                    kcdb_identity_release(identity);
                }
                break;
            }

            refresh_view_idents_state(hwnd);
        }

        return SetDlgMsgResult(hwnd, WM_COMMAND, 0);

    case WM_HELP:
        khm_html_help(khm_hwnd_main, NULL, HH_HELP_CONTEXT, IDH_CFG_IDENTITIES);
        return TRUE;


    case KHUI_WM_CFG_NOTIFY:
        {
            switch(HIWORD(wParam)) {
            case WMCFG_APPLY:
                write_params_idents();
                break;

            case WMCFG_UPDATE_STATE:
                refresh_view_idents_state(hwnd);
                break;

            case WMCFG_INIT_PANEL:
                {
                    khm_int32 *prv = (khm_int32 *) lParam;
                    if (prv)
                        *prv = KHM_ERROR_SUCCESS;
                }
                break;
            }
        }
        return TRUE;

    case WM_DESTROY:
        cfg_idents.hwnd = NULL;

        if (cfg_idents.hi_status != NULL) {
            ImageList_Destroy(cfg_idents.hi_status);
            cfg_idents.hi_status = NULL;
        }
        release_idents_data();

        return SetDlgMsgResult(hwnd, WM_DESTROY, 0);
    }

    return FALSE;
}

/* dialog procedure for the "Identities" configuration node */
INT_PTR CALLBACK
khm_cfg_identities_proc(HWND hwnd,
                        UINT uMsg,
                        WPARAM wParam,
                        LPARAM lParam)
{
    HWND hw;
    switch(uMsg) {
    case WM_INITDIALOG:
        set_window_node(hwnd, (khui_config_node) lParam);
        add_subpanels(hwnd, (khui_config_node) lParam,
                      (khui_config_node) lParam);
        hw = GetDlgItem(hwnd, IDC_CFG_TAB);
        show_tab_panel(hwnd,
                       (khui_config_node) lParam,
                       hw,
                       TabCtrl_GetCurSel(hw),
                       TRUE);
        return FALSE;

    case WM_DESTROY:
        return 0;

    case KHUI_WM_CFG_NOTIFY:
        return handle_cfg_notify(hwnd, wParam, lParam);

    case WM_NOTIFY:
        return handle_notify(hwnd, wParam, lParam);

    case WM_HELP:
        khm_html_help(khm_hwnd_main, NULL, HH_HELP_CONTEXT, IDH_CFG_IDENTITIES);
        return TRUE;
    }

    return FALSE;
}

static ident_data *
find_ident_by_node(khui_config_node node)
{
    int i;
    khm_handle ident = NULL;

    ident = khui_cfg_get_data(node);
    if (ident == NULL)
        return NULL;

    kcdb_identity_hold(ident);

    for (i=0; i < (int)cfg_idents.n_idents; i++) {
        if (kcdb_identity_is_equal(cfg_idents.idents[i].ident, ident))
            break;
    }

    if (i < (int)cfg_idents.n_idents) {
        if (cfg_idents.idents[i].purged) {
            /* we are re-creating a purged identity */
            cfg_idents.idents[i].purged = FALSE;
            cfg_idents.idents[i].removed = FALSE;
            cfg_idents.idents[i].applied = FALSE;

            read_params_ident(&cfg_idents.idents[i]);
        }
        return &cfg_idents.idents[i];
    }

    /* there is no identity data for this configuration node.  We try
       to create it. */

    if (cfg_idents.n_idents >= cfg_idents.nc_idents) {
        cfg_idents.nc_idents = UBOUNDSS(cfg_idents.n_idents + 1,
                                        IDENTS_DATA_ALLOC_INCR,
                                        IDENTS_DATA_ALLOC_INCR);
        assert(cfg_idents.nc_idents > cfg_idents.n_idents);
        cfg_idents.idents = PREALLOC(cfg_idents.idents,
                                     sizeof(*cfg_idents.idents) *
                                     cfg_idents.nc_idents);
        assert(cfg_idents.idents);
        ZeroMemory(&(cfg_idents.idents[cfg_idents.n_idents]),
                   sizeof(*cfg_idents.idents) *
                   (cfg_idents.nc_idents - cfg_idents.n_idents));
    }

    i = (int) cfg_idents.n_idents;

    cfg_idents.idents[i].ident = ident;
    cfg_idents.idents[i].removed = FALSE;

    kcdb_identity_get_flags(ident, &cfg_idents.idents[i].flags);
    assert(cfg_idents.idents[i].flags & KCDB_IDENT_FLAG_CONFIG);

    read_params_ident(&cfg_idents.idents[i]);

    cfg_idents.n_idents++;

    /* leave ident held. */

    return &cfg_idents.idents[i];
}

static void
refresh_view_ident(HWND hwnd, khui_config_node node)
{
    ident_data * d;

    d = find_ident_by_node(node);
    assert(d);

    CheckDlgButton(hwnd, IDC_CFG_MONITOR,
                   (d->work.monitor? BST_CHECKED: BST_UNCHECKED));
    CheckDlgButton(hwnd, IDC_CFG_RENEW,
                   (d->work.auto_renew? BST_CHECKED: BST_UNCHECKED));
    CheckDlgButton(hwnd, IDC_CFG_STICKY,
                   (d->work.sticky? BST_CHECKED: BST_UNCHECKED));
}

static void
mark_remove_ident(HWND hwnd, khui_config_init_data * idata)
{
    ident_data * d;

    d = find_ident_by_node(idata->ctx_node);
    assert(d);

    if (d->removed)
        return;

    d->removed = TRUE;

    khui_cfg_set_flags_inst(idata, KHUI_CNFLAG_MODIFIED,
                            KHUI_CNFLAG_MODIFIED);

    EnableWindow(GetDlgItem(hwnd, IDC_CFG_REMOVE), FALSE);
}

static void
refresh_data_ident(HWND hwnd, khui_config_init_data * idata)
{
    ident_data * d;

    d = find_ident_by_node(idata->ctx_node);
    assert(d);

    if (IsDlgButtonChecked(hwnd, IDC_CFG_MONITOR) == BST_CHECKED)
        d->work.monitor = TRUE;
    else
        d->work.monitor = FALSE;

    if (IsDlgButtonChecked(hwnd, IDC_CFG_RENEW) == BST_CHECKED)
        d->work.auto_renew = TRUE;
    else
        d->work.auto_renew = FALSE;

    if (IsDlgButtonChecked(hwnd, IDC_CFG_STICKY) == BST_CHECKED)
        d->work.sticky = TRUE;
    else
        d->work.sticky = FALSE;

    if (d->work.monitor != d->saved.monitor ||
        d->work.auto_renew != d->saved.auto_renew ||
        d->work.sticky != d->saved.sticky) {

        khui_cfg_set_flags_inst(idata, KHUI_CNFLAG_MODIFIED,
                                KHUI_CNFLAG_MODIFIED);

    } else {
        khui_cfg_set_flags_inst(idata, 0,
                                KHUI_CNFLAG_MODIFIED);
    }
}

static void
change_icon_ident (HWND hwnd, khui_config_init_data * idata)
{
    ident_data * d;
    HICON hicon = NULL;
    khm_size cb = sizeof(hicon);

    d = find_ident_by_node(idata->ctx_node);
    assert(d);

    if (d == NULL)
        return;

    khm_select_icon_for_identity(hwnd, d->ident);

    kcdb_get_resource(d->ident, KCDB_RES_ICON_NORMAL, KCDB_RF_SKIPCACHE,
                      NULL, NULL, &hicon, &cb);
    assert(hicon != NULL);
    SendDlgItemMessage(hwnd, IDC_CFG_ICON, STM_SETICON,
                       (WPARAM) hicon, 0);
    InvalidateRect(GetDlgItem(hwnd, IDC_CFG_ICON), NULL, TRUE);
}

static void
reset_icon_ident (HWND hwnd, khui_config_init_data * idata)
{
    ident_data * d;
    khm_handle csp_id = NULL;
    HICON hicon = NULL;
    khm_size cb;

    d = find_ident_by_node(idata->ctx_node);
    assert(d);

    if (d == NULL)
        return;

    if (KHM_FAILED(kcdb_identity_get_config(d->ident, KHM_PERM_WRITE, &csp_id)))
        return;

    khc_remove_value(csp_id, L"IconNormal", 0);

    cb = sizeof(hicon);
    kcdb_get_resource(d->ident, KCDB_RES_ICON_DISABLED, KCDB_RF_SKIPCACHE | KCDB_RFI_SMALL,
                      NULL, NULL, &hicon, &cb);
    kcdb_get_resource(d->ident, KCDB_RES_ICON_NORMAL, KCDB_RF_SKIPCACHE | KCDB_RFI_SMALL,
                      NULL, NULL, &hicon, &cb);
    kcdb_get_resource(d->ident, KCDB_RES_ICON_DISABLED, KCDB_RF_SKIPCACHE,
                      NULL, NULL, &hicon, &cb);
    kcdb_get_resource(d->ident, KCDB_RES_ICON_NORMAL, KCDB_RF_SKIPCACHE,
                      NULL, NULL, &hicon, &cb);
    assert(hicon != NULL);
    SendDlgItemMessage(hwnd, IDC_CFG_ICON, STM_SETICON,
                       (WPARAM) hicon, 0);
    InvalidateRect(GetDlgItem(hwnd, IDC_CFG_ICON), NULL, TRUE);

    kcdb_identity_hold(d->ident);
    kmq_post_message(KMSG_KCDB, KMSG_KCDB_IDENT, KCDB_OP_RESUPDATE, (void *) d->ident);
    /* d->ident will be automatically released when the message
       handling is completed. */

    if (csp_id)
        khc_close_space(csp_id);
}



/* dialog procedure for the "general" pane of individual identity
   configuration nodes. */
INT_PTR CALLBACK
khm_cfg_id_tab_proc(HWND hwnd,
                    UINT umsg,
                    WPARAM wParam,
                    LPARAM lParam)
{

    khui_config_init_data * idata;

    switch(umsg) {
    case WM_INITDIALOG:
        {
            ident_data * d;

            hold_idents_data();

            idata = (khui_config_init_data *) lParam;

            khui_cfg_init_dialog_data(hwnd, idata, 0, NULL, NULL);

            refresh_view_ident(hwnd, idata->ctx_node);

            d = find_ident_by_node(idata->ctx_node);
            if (d == NULL) {
                assert(FALSE);
                return FALSE;
            }

            d->hwnd = hwnd;

            /* Set a few permanent properties in the dialog */
            {
                HICON hicon;
                wchar_t buf[KCDB_MAXCCH_LONG_DESC];
                khm_size cb;
                khm_handle idpro = NULL;

                cb = sizeof(hicon);
                if (KHM_SUCCEEDED(kcdb_get_resource(d->ident,
                                                    KCDB_RES_ICON_NORMAL, 0,
                                                    NULL, NULL,
                                                    &hicon, &cb))) {
                    SendDlgItemMessage(hwnd, IDC_CFG_ICON, STM_SETICON,
                                       (WPARAM) hicon, 0);
                }

                cb = sizeof(buf);
                if (KHM_SUCCEEDED(kcdb_get_resource(d->ident,
                                                    KCDB_RES_DESCRIPTION, 0,
                                                    NULL, NULL,
                                                    buf, &cb))) {
                    SetDlgItemText(hwnd, IDC_CFG_DESC, buf);
                }

                cb = sizeof(buf);
                if (KHM_SUCCEEDED(kcdb_identity_get_identpro(d->ident, &idpro)) &&
                    KHM_SUCCEEDED(kcdb_get_resource(idpro,
                                                    KCDB_RES_INSTANCE, 0,
                                                    NULL, NULL, buf, &cb))) {
                    SetDlgItemText(hwnd, IDC_CFG_TYPE, buf);
                }

                if (idpro)
                    kcdb_identpro_release(idpro);
            }
        }
        return FALSE;

    case WM_COMMAND:
        khui_cfg_get_dialog_data(hwnd, &idata, NULL);

        if (HIWORD(wParam) == BN_CLICKED) {
            switch(LOWORD(wParam)) {
            case IDC_CFG_MONITOR:
            case IDC_CFG_RENEW:
            case IDC_CFG_STICKY:

                refresh_data_ident(hwnd, idata);
                if (cfg_idents.hwnd)
                    PostMessage(cfg_idents.hwnd, KHUI_WM_CFG_NOTIFY,
                                MAKEWPARAM(1, WMCFG_UPDATE_STATE), 0);
                break;

            case IDC_CFG_REMOVE:
                {
                    wchar_t title[KCDB_IDENT_MAXCCH_NAME];
                    wchar_t text[KCDB_IDENT_MAXCCH_NAME + 256];
                    wchar_t fmt[256];
                    wchar_t idname[KCDB_IDENT_MAXCCH_NAME] = L"";
                    ident_data * d;
                    khm_size cb;

                    d = find_ident_by_node(idata->ctx_node);
                    if (d == NULL)
                        break;

                    cb = sizeof(idname);
                    kcdb_get_resource(d->ident, KCDB_RES_DISPLAYNAME, 0, NULL, NULL, idname, &cb);

                    LoadString(khm_hInstance, IDS_MBT_IDREMOVE, fmt, ARRAYLENGTH(fmt));
                    StringCbPrintf(title, sizeof(title), fmt, idname);

                    LoadString(khm_hInstance, IDS_MB_IDREMOVE, fmt, ARRAYLENGTH(fmt));
                    StringCbPrintf(text, sizeof(text), fmt, idname);

                    if (MessageBox(hwnd, text, title, MB_YESNO | MB_ICONWARNING) != IDYES)
                        break;
                }

                mark_remove_ident(hwnd, idata);
                if (cfg_idents.hwnd)
                    PostMessage(cfg_idents.hwnd, KHUI_WM_CFG_NOTIFY,
                                MAKEWPARAM(1, WMCFG_UPDATE_STATE), 0);
                break;

            case IDC_CFG_CHICON:
                change_icon_ident(hwnd, idata);
                break;

            case IDC_CFG_RESETICON:
                reset_icon_ident(hwnd, idata);
                break;
            }
        }

        return SetDlgMsgResult(hwnd, WM_COMMAND, 0);

    case WM_DESTROY:
        {
            ident_data * d;

            khui_cfg_get_dialog_data(hwnd, &idata, NULL);

            d = find_ident_by_node(idata->ctx_node);
            if (d)
                d->hwnd = NULL;

            release_idents_data();
            khui_cfg_free_dialog_data(hwnd);
        }
        return SetDlgMsgResult(hwnd, WM_DESTROY, 0);

    case WM_HELP:
        khm_html_help(khm_hwnd_main, NULL, HH_HELP_CONTEXT, IDH_CFG_IDENTITY);
        return TRUE;

    case KHUI_WM_CFG_NOTIFY:
        {
            ident_data * d;
            khm_boolean * cont;

            khui_cfg_get_dialog_data(hwnd, &idata, NULL);

            switch (HIWORD(wParam)) {
            case WMCFG_APPLY:
                cont = (khm_boolean *) lParam;
                d = find_ident_by_node(idata->ctx_node);
                write_params_ident(d);
                if (d->removed) {
                    if (cont)
                        *cont = FALSE;
                } else {
                    khui_cfg_set_flags_inst(idata, KHUI_CNFLAG_APPLIED,
                                            KHUI_CNFLAG_APPLIED |
                                            KHUI_CNFLAG_MODIFIED);
                }
                break;

            case WMCFG_UPDATE_STATE:
                refresh_view_ident(hwnd, idata->ctx_node);
                refresh_data_ident(hwnd, idata);
                break;

            case WMCFG_INIT_PANEL:
                {
                    khm_int32 * prv = (khm_int32 *) lParam;
                    if (prv)
                        *prv = KHM_ERROR_SUCCESS;
                }
                break;
            }
        }
        return TRUE;
    }

    return FALSE;
}

/* dialog procedure for individual identity configuration nodes */
INT_PTR CALLBACK
khm_cfg_identity_proc(HWND hwnd,
                      UINT uMsg,
                      WPARAM wParam,
                      LPARAM lParam)
{
    HWND hw;

    switch(uMsg) {
    case WM_INITDIALOG:
        {
            khui_config_node refnode = NULL;

            set_window_node(hwnd, (khui_config_node) lParam);

            khui_cfg_open(NULL, L"KhmIdentities", &refnode);
            assert(refnode != NULL);

            add_subpanels(hwnd,
                          (khui_config_node) lParam,
                          refnode);

            hw = GetDlgItem(hwnd, IDC_CFG_TAB);

            show_tab_panel(hwnd,
                           (khui_config_node) lParam,
                           hw,
                           TabCtrl_GetCurSel(hw),
                           TRUE);

            khui_cfg_release(refnode);
        }
        return FALSE;

    case WM_DESTROY:
        return 0;

    case KHUI_WM_CFG_NOTIFY:
        return handle_cfg_notify(hwnd, wParam, lParam);

    case WM_NOTIFY:
        return handle_notify(hwnd, wParam, lParam);

    case WM_HELP:
        khm_html_help(khm_hwnd_main, NULL, HH_HELP_CONTEXT, IDH_CFG_IDENTITIES);
        return TRUE;
    }
    return FALSE;
}


