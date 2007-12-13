/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
 * Copyright (c) 2007 Secure Endpoints Inc.
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

/* Include the OEMRESOURCE constants for locating standard icon
   resources. */
#define OEMRESOURCE

#include<khmapp.h>
#if _WIN32_WINNT >= 0x0501
#include<uxtheme.h>
#endif
#include<assert.h>

static void
nc_set_dlg_data(HWND hwnd, khui_nc_wnd_data * d)
{
#pragma warning(push)
#pragma warning(disable: 4244)
    SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) ncd);
#pragma warning(pop)
}

static khui_nc_wnd_data *
nc_get_dlg_data(HWND hwnd)
{
    return (khui_nc_wnd_data *)(LONG_PTR) GetWindowLongPtr(hwnd, DWLP_USER);
}

/* Flag combinations for SetWindowPos */

/* Move+Size+Zorder */
#define SWP_MOVESIZEZ (SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_SHOWWINDOW)

/* Size */
#define SWP_SIZEONLY (SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER)

/* Hide */
#define SWP_HIDEONLY (SWP_NOACTIVATE|SWP_HIDEWINDOW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOOWNERZORDER|SWP_NOZORDER)

/* Show */
#define SWP_SHOWONLY (SWP_NOACTIVATE|SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOOWNERZORDER|SWP_NOZORDER)

/* Layout functions */

/* Layout the ID Selector dialog.  This should be called whenever the
   primary identity changes. */
static void
nc_layout_idsel(khui_nc_wnd_data * ncd)
{
    khm_handle ident;
    khm_handle idpro = NULL;
    khm_size cb;

    khui_cw_lock_nc(ncd->nc);
#ifdef DEBUG
    assert(ncd->nc->n_identities > 0);
#endif

    ncd->id_icon = NULL;
    ncd->id_display_string[0] = L'\0';
    ncd->id_status[0] = L'\0';
    ncd->id_type_string[0] = L'\0';

    if (ncd->nc->n_identities == 0) {
        goto _exit;
    }

    ident = ncd->nc->identities[0];

    cb = sizeof(ncd->id_icon);
    kcdb_get_resource(ident, KCDB_RES_ICON_NORMAL, 0, NULL, NULL,
                      &cb, &ncd->id_icon);

    cb = sizeof(ncd->id_display_string);
    kcdb_get_resource(ident, KCDB_RES_DISPLAYNAME, KCDB_RFS_SHORT, NULL, NULL,
                      &cb, ncd->id_display_string);

    kcdb_identity_get_identpro(ident, &idpro);

    cb = sizeof(ncd->id_type_string);
    kcdb_get_resource(idpro, KCDB_RES_INSTANCE, KCDB_RFS_SHORT, NULL, NULL,
                      &cb, ncd->id_type_string);

 _exit:

    khui_cw_unlock_nc(ncd->nc);
}

static void
nc_layout_idspec(khui_nc_wnd_data * ncd)
{
}

static void
nc_layout_nav(khui_nc_wnd_data * ncd)
{
    HDWP dwp;
    dwp = BeginDeferWindowPos(3);

    DeferWindowPos(dwp, GetDlgItem(ncd->hwnd_nav, ID_WIZBACK), NULL, 0, 0, 0, 0,
                   (ncd->enable_prev ? SWP_SHOWONLY : SWP_HIDEONLY));
    DeferWindowPos(dwp, GetDlgItem(ncd->hwnd_nav, ID_WIZNEXT), NULL, 0, 0, 0, 0,
                   (ncd->enable_next ? SWP_SHOWONLY : SWP_HIDEONLY));
    DeferWindowPos(dwp, GetDlgItem(ncd->hwnd_nav, ID_WIZFINISH), NULL, 0, 0, 0, 0,
                   ((ncd->enable_next || ncd->page == NC_PAGE_PROGRESS)? SWP_HIDEONLY: SWP_SHOWONLY));

    EndDeferWindowPos(dwp);
}

/* Credential type panel comparison function.  Tabs are sorted based
   on the following criteria:

   1) By ordinal - Panels with ordinal -1 will be ranked after panels
      whose ordinal is not -1.

   2) By name - Case insensitive comparison of the name.  If the panel
      does not have a name (i.e. the ->name member is NULL, it will be
      ranked after panels which have a name.
 */
static int __cdecl
nc_tab_sort_func(const void * v1, const void * v2)
{
    /* v1 and v2 and of type : khui_new_creds_by_type ** */
    khui_new_creds_type_int *t1, *t2;

    t1 = *((khui_new_creds_type_int **) v1);
    t2 = *((khui_new_creds_type_int **) v2);

    if(t1->nct->ordinal !=  -1) {
        if(t2->nct->ordinal != -1) {
            if(t1->nct->ordinal == t2->ordinal) {
                if (t1->nct->name && t2->nct->name)
                    return _wcsicmp(t1->nct->name, t2->nct->name);
                else if (t1->nct->name)
                    return -1;
                else if (t2->nct->name)
                    return 1;
                else
                    return 0;
            } else {
                /* safe to convert to an int here */
                return (int) (t1->nct->ordinal - t2->nct->ordinal);
            }
        } else
            return -1;
    } else {
        if(t2->nct->ordinal != -1)
            return 1;
        else if (t1->nct->name && t2->nct->name)
            return wcscmp(t1->nct->name, t2->nct->name);
        else if (t1->nct->name)
            return -1;
        else if (t2->nct->name)
            return 1;
        else
            return 0;
    }
}

/* Called with ncd->nc locked

   Prepares the credentials type list in a new credentials object
   after an identity has been selected and the types have been
   notified.

   The list is sorted with the identity credential type at the
   beginning and the ordinals are updated to reflect the actual order
   of the types.
*/
static void
nc_prep_cred_types(khui_nc_wnd_data * ncd)
{
    khm_size i;
    khm_handle idpro = NULL;
    khm_int32 ctype;

    /* if we have an identity, we should make sure that the identity
       credentials type is at the top of the list */
    if (ncd->nc->n_identities > 0 &&
        KHM_SUCCEEDED(kcdb_identity_get_identpro(ncd->nc->identities[0],
                                                 &idpro)) &&
        KHM_SUCCEEDED(kcdb_identpro_get_type(idpro, &ctype))) {

        for (i=0; i < ncd->nc->n_types; i++) {
            if (ncd->nc->types[i]->nct->type == ctype) {
                ncd->nc->types[i]->nct->ordinal = 0;
            } else if (ncd->nc->types[i]->nct->ordinal == 0) {
                ncd->nc->types[i]->nct->ordinal = 1;
            }
        }
    }

    qsort(ncd->nc->types, ncd->nc->n_types, sizeof(*(ncd->nc->types)), nc_tab_sort_func);

    for (i=0; i < ncd->nc->n_types; i++) {
        ncd->nc->types[i]->nct->ordinal = i+1;
    }
}

/* Layout the privileged interaction panel in either the basic or the
   advanced mode. */
static void
nc_layout_privint(khui_nc_wnd_data * ncd, khm_boolean adv)
{
    HWND hw;
    HWND hw_r_p = NULL;
    HWND hw_target = NULL;
    HWND hw_tab = NULL;
    RECT r, r_p;
    khm_int32 idf = 0;

    if (adv) {
        hw = ncd->hwnd_privint_advanced;
        hw_tab = GetDlgItem(hw, IDC_NC_TAB);
    } else {
        hw = ncd->hwnd_privint_basic;
    }

    if (adv && !ncd->tab_initialized) {
        /* We have to populate the Tab control */

        wchar_t desc[KCDB_MAXCCH_SHORT_DESC];
        TCITEM tci;
        khm_size i;
        int idx = 0;

        ZeroMemory(&tci, sizeof(tci));
#ifdef DEBUG
        assert(hw_tab != NULL);
#endif
        TabCtrl_DeleteAllItems(hw_tab);

        khui_cw_lock_nc(ncd->nc);
        nc_prep_cred_types(ncd);

        if (ncd->privint == NULL)
            khui_cw_get_next_privint(ncd->nc, &ncd->privint);

        if (ncd->privint) {
            tci.mask = TCIF_PARAM | TCIF_TEXT;
            tci.pszText = ncd->privint->caption;
            tci.lParam = -1;

            TabCtrl_InsertItem(hw_tab, 0, &tci);
            idx = 1;
        }

        for (i=0; i < ncd->nc->n_types; i++)
            if (!(ncd->nc->types[i].nct->flags & KHUI_NCT_FLAG_DISABLED)) {
                tci.mask = TCIF_PARAM | TCIF_TEXT;
                if (ncd->nc->types[i].nct->name) {
                    tci.pszText = ncd->nc->types[i].nct->name;
                } else {
                    khm_size cb;

                    desc[0] = L'\0';
                    cb = sizeof(desc);
                    kcdb_get_resource((khm_handle) ncd->nc->types[i].nct->type,
                                      KCDB_RES_DESCRIPTION, KCDB_RFS_SHORT,
                                      NULL, NULL, &cb, desc);
                    tci.pszText = desc;
                }
                tci.lParam = i;

                TabCtrl_InsertItem(hw_tab, idx, &tci);
                idx++;
            }
        khui_cw_unlock_nc(ncd->nc);

        ncd->tab_initialized = TRUE;

        TabCtrl_SetCurSel(hw_tab, ncd->current_tab);

#ifdef DEBUG
        assert(idx > 0);
#endif
    }

    if (adv) {
        TCITEM tci;
        int current_panel;

        current_panel = TabCtrl_GetCurSel(hw_tab);

        ZeroMemory(&tci, sizeof(tci));

        tci.mask = TCIF_PARAM;
        TabCtrl_GetItem(hw_tab, current_panel, &tci);

        khui_cw_lock_nc(ncd->nc);

        if (tci.lParam == -1) {
            /* selected the privileged interaction panel */
#ifdef DEBUG
            assert(ncd->privint);
#endif
            hw_target = ncd->privint->hwnd;
        } else {
#ifdef DEBUG
            assert(tci.lParam >= 0 && tci.lParam < ncd->nc->n_types);
#endif
            hw_target = ncd->nc->types[tci.lParam].nct->hwnd_panel;
        }

        khui_cw_unlock_nc(ncd->nc);

        if (hw_target != ncd->last_tab_panel && ncd->last_tab_panel != NULL) {
            SetWindowPos(ncd->last_tab_panel, NULL, 0, 0, 0, 0, SWP_HIDEONLY);
        }

    } else {
        if (ncd->privint == NULL) {
            khui_cw_get_next_privint(ncd->nc, &ncd->privint);
        }

        if (ncd->privint == NULL)
            hw_target = ncd->hwnd_noprompts;
        else
            hw_target = ncd->privint->hwnd;
    }

#ifdef DEBUG
    assert(hw_target != NULL);
#endif

    hw_r_p = GetDlgItem(hw, IDC_NC_R_PROMPTS);
#ifdef DEBUG
    assert(hw_r_p != NULL);
#endif

    if (!adv) {
        GetWindowRect(hw, &r);
        GetWindowRect(hw_r_p, &r_p);
        OffsetRect(&r_p, -r.left, -r.top);
    } else {
#ifdef DEBUG
        assert(hw_tab != NULL);
#endif
        GetWindowRect(hw, &r);
        GetWindowRect(hw_tab, &r_p);
        OffsetRect(&r_p, -r.left, -r.top);

        TabCtrl_AdjustRect(hw_tab, FALSE, &r_p);
    }

    SetParent(hw_target, hw);
    SetWindowPos(hw_target, hw_r_p, r_p.left, r_p.top,
                 r_p.right - r_p.left, r_p.bottom - r_p.top,
                 SWP_MOVESIZEZ);

    khui_cw_lock_nc(ncd->nc);
#ifdef DEBUG
    assert(ncd->nc->n_identities > 0);
#endif
    if (ncd->nc->n_identities > 0) {
        kcdb_identity_get_flags(ncd->nc->identities[0], &idf);
    }
    khui_cw_unlock_nc(ncd->nc);

    CheckDlgButton(hw, IDC_NC_MAKEDEFAULT,
                   ((idf & KCDB_IDENT_FLAG_DEFAULT)? BST_CHECKED : BST_UNCHECKED));
    EnableWindow(GetDlgItem(hw, IDC_NC_MAKEDEFAULT),
                 !(idf & KCDB_IDENT_FLAG_DEFAULT));
}

static void
nc_layout_progress(khui_nc_wnd_data * ncd)
{
}

static void
nc_size_container(khui_nc_wnd_data * ncd, HDWP dwp)
{
    RECT r;
    DWORD style;
    DWORD exstyle;

    if (ncd->nc->mode == KHUI_NC_MODE_MINI) {
        int t;
        RECT r1, r2;

        GetWindowRect(GetDlgItem(ncd->nc->hwnd, IDC_NC_R_IDSEL), &r1);
        GetWindowRect(GetDlgItem(ncd->nc->hwnd, IDC_NC_R_MAIN_LG), &r2);
        t = r2.top;
        GetWindowRect(GetDlgItem(ncd->nc->hwnd, IDC_NC_R_NAV), &r2);
        OffsetRect(&r2, 0, t - r2.top);

        UnionRect(&r, &r1, &r2);
    } else {
        RECT r1, r2;

        GetWindowRect(GetDlgItem(ncd->nc->hwnd, IDC_NC_R_IDSEL), &r1);
        GetWindowRect(GetDlgItem(ncd->nc->hwnd, IDC_NC_R_NAV), &r2);

        UnionRect(&r, &r1, &r2);
    }

    style = GetWindowLong(ncd->nc->hwnd, GWL_STYLE);
    exstyle = GetWindowLong(ncd->nc->hwnd, GWL_EXSTYLE);

    AdjustWindowRectEx(&r, style, FALSE, exstyle);

    DeferWindowPos(dwp, ncd->nc->hwnd, NULL, 0, 0,
                   r.right - r.left, r.bottom - r.top,
                   SWP_SIZEONLY);
}

static void
nc_layout_container(khui_nc_wnd_data * ncd)
{
    RECT r_idsel;
    RECT r_main;
    RECT r_main_lg;
    RECT r_nav_lg;
    RECT r_nav_sm;
    RECT r;
    HDWP dwp = NULL;

#define get_wnd_rect(id,pv) \
    GetWindowRect(GetDlgItem(ncd->nc->hwnd, id), pv); OffsetRect(pv, -r.left, -r.top)

#define rect_coords(r) r.left, r.top, (r.right - r.left), (r.bottom - r.top)

#define dlg_item(id) GetDlgItem(ncd->nc->hwnd, id)

    GetWindowRect(ncd->nc->hwnd, &r);

    get_wnd_rect(IDC_NC_R_IDSEL, &r_idsel);
    get_wnd_rect(IDC_NC_R_MAIN, &r_main);
    get_wnd_rect(IDC_NC_R_MAIN_LG, &r_main_lg);
    get_wnd_rect(IDC_NC_R_NAV, &r_nav_lg);

    CopyRect(&r_nav_sm, &r_nav_lg);
    OffsetRect(&r_nav_sm, 0, r_main_lg.top - r_nav_sm.top);
    UnionRect(&r, &r_main, &r_main_lg);
    CopyRect(&r_main_lg, &r);

    switch (ncd->page) {
    case NC_PAGE_IDSPEC:

        khui_cw_lock_nc(ncd->nc);

        dwp = BeginDeferWindowPos(7);

        UnionRect(&r, &r_idsel, &r_main);
        DeferWindowPos(dwp, ncd->hwnd_idspec,
                       dlg_item(IDC_NC_R_IDSEL),
                       rect_coords(r), SWP_MOVESIZEZ);

        DeferWindowPos(dwp, ncd->hwnd_nav,
                       dlg_item(IDC_NC_R_NAV),
                       rect_coords(r_nav_sm), SWP_MOVESIZEZ);

        DeferWindowPos(dwp, ncd->hwnd_idsel, NULL, 0, 0, 0, 0,
                       SWP_HIDEONLY);

        DeferWindowPos(dwp, ncd->hwnd_privint_basic, NULL, 0, 0, 0, 0,
                       SWP_HIDEONLY);

        DeferWindowPos(dwp, ncd->hwnd_privint_advanced, NULL, 0, 0, 0, 0,
                       SWP_HIDEONLY);

        DeferWindowPos(dwp, ncd->hwnd_progress, NULL, 0, 0, 0, 0,
                       SWP_HIDEONLY);

        ncd->nc->mode = KHUI_NC_MODE_MINI;
        nc_size_container(ncd, dwp);

        EndDeferWindowPos(dwp);

        khui_cw_unlock_nc(ncd->nc);

        nc_layout_idspec(ncd);
        nc_layout_nav(ncd);

        break;

    case NC_PAGE_CREDOPT_BASIC:

        khui_cw_lock_nc(ndc->nc);

        dwp = BeginDeferWindowPos(7);

        DeferWindowPos(dwp, ncd->hwnd_idsel,
                       dlg_item(IDC_NC_R_IDSEL),
                       rect_coords(r_idsel), SWP_MOVESIZEZ);

        DeferWindowPos(dwp, ncd->hwnd_privint_basic,
                       dlg_item(IDC_NC_R_MAIN),
                       rect_coords(r_main), SWP_MOVESIZEZ);

        DeferWindowPos(dwp, ncd->hwnd_nav,
                       dlg_item(IDC_NC_R_NAV),
                       rect_coords(r_nav_sm), SWP_MOVESIZEZ);

        DeferWindowPos(dwp, ncd->hwnd_privint_advanced, NULL, 0, 0, 0, 0,
                       SWP_HIDEONLY);

        DeferWindowPos(dwp, ncd->hwnd_idspec, NULL, 0, 0, 0, 0,
                       SWP_HIDEONLY);

        DeferWindowPos(dwp, ncd->hwnd_progress, NULL, 0, 0, 0, 0,
                       SWP_HIDEONLY);

        ncd->nc->mode = KHUI_NC_MODE_MINI;
        nc_size_container(ncd, dwp);

        EndDeferWindowPos(dwp);

        khui_cw_unlock_nc(ncd->nc);

        nc_layout_idsel(ncd);
        nc_layout_privint(ncd, FALSE);
        nc_layout_nav(ncd);

        break;

    case NC_PAGE_CREDOPT_ADV:

        khui_cw_lock_nc(ncd->nc);

        dwp = BeginDeferWindowPos(7);

        DeferWindowPos(dwp, ncd->hwnd_idsel,
                       dlg_item(IDC_NC_R_IDSEL),
                       rect_coords(r_idsel), SWP_MOVESIZEZ);

        DeferWindowPos(dwp, ncd->hwnd_privint_advanced,
                       dlg_item(IDC_NC_R_MAIN),
                       rect_coords(r_main_lg), SWP_MOVESIZEZ);

        DeferWindowPos(dwp, ncd->hwnd_nav,
                       dlg_item(IDC_NC_R_NAV),
                       rect_coords(r_nav_lg), SWP_MOVESIZEZ);

        DeferWindowPos(dwp, ncd->hwnd_privint_basic, NULL, 0, 0, 0, 0,
                       SWP_HIDEONLY);

        DeferWindowPos(dwp, ncd->hwnd_idspec, NULL, 0, 0, 0, 0,
                       SWP_HIDEONLY);

        DeferWindowPos(dwp, ncd->hwnd_progress, NULL, 0, 0, 0, 0,
                       SWP_HIDEONLY);

        ncd->nc = KHUI_NC_MODE_EXPANDED;
        nc_size_container(ncd, dwp);

        EndDeferWindowPos(dwp);

        khui_cw_unlock_nc(ncd->nc);

        nc_layout_idsel(ncd);
        nc_layout_privint(ncd, TRUE);
        nc_layout_nav(ncd);

        break;

    case NC_PAGE_PASSWORD:

        khui_cw_lock_nc(ncd->nc);

        dwp = BeginDeferWindowPos(7);

        DeferWindowPos(dwp, ncd->hwnd_idsel,
                       dlg_item(IDC_NC_R_IDSEL),
                       rect_coords(r_idsel), SWP_MOVESIZEZ);

        DeferWindowPos(dwp, ncd->hwnd_privint_basic,
                       dlg_item(IDC_NC_R_MAIN),
                       rect_coords(r_main), SWP_MOVESIZEZ);

        DeferWindowPos(dwp, ncd->hwnd_nav,
                       dlg_item(IDC_NC_R_NAV),
                       rect_coords(r_nav_sm), SWP_MOVESIZEZ);

        DeferWindowPos(dwp, ncd->hwnd_privint_advanced, NULL, 0, 0, 0, 0,
                       SWP_HIDEONLY);

        DeferWindowPos(dwp, ncd->hwnd_idspec, NULL, 0, 0, 0, 0,
                       SWP_HIDEONLY);

        DeferWindowPos(dwp, ncd->hwnd_progress, NULL, 0, 0, 0, 0,
                       SWP_HIDEONLY);

        ncd->nc->mode = KHUI_NC_MODE_MINI;
        nc_size_container(ncd, dwp);

        EndDeferWindowPos(dwp);

        khui_cw_unlock_nc(ncd->nc);

        nc_layout_idsel(ncd);
        nc_layout_privint(ncd, FALSE);
        nc_layout_nav(ncd);

        break;

    case NC_PAGE_PROGRESS:

        khui_cw_lock_nc(ncd->nc);

        dwp = BeginDeferWindowPos(7);

        DeferWindowPos(dwp, ncd->hwnd_idsel,
                       dlg_item(IDC_NC_R_IDSEL),
                       rect_coords(r_idsel), SWP_MOVESIZEZ);

        DeferWindowPos(dwp, ncd->hwnd_progress,
                       dlg_item(IDC_NC_R_MAIN),
                       rect_coords(r_main), SWP_MOVESIZEZ);

        DeferWindowPos(dwp, ncd->hwnd_nav,
                       dlg_item(IDC_NC_R_NAV),
                       rect_coords(r_nav_sm), SWP_MOVESIZEZ);

        DeferWindowPos(dwp, ncd->hwnd_privint_basic, NULL, 0, 0, 0, 0,
                       SWP_HIDEONLY);

        DeferWindowPos(dwp, ncd->hwnd_privint_advanced, NULL, 0, 0, 0, 0,
                       SWP_HIDEONLY);

        DeferWindowPos(dwp, ncd->hwnd_idspec, NULL, 0, 0, 0, 0,
                       SWP_HIDEONLY);

        ncd->nc->mode = KHUI_NC_MODE_MINI;
        nc_size_container(ncd, dwp);

        EndDeferWindowPos(dwp);

        khui_cw_unlock_nc(ncd->nc);

        nc_layout_idsel(ncd);
        nc_layout_progress(ncd);
        nc_layout_nav(ncd);

        break;

    default:
#ifdef DEBUG
        assert(FALSE);
#endif
    }

#undef get_wnd_rect
#undef rect_coords
#undef dlg_item
}

/* Dialog procedures for child dialogs */

/* Navigation dialog.  IDD_NC_NAV */
static LRESULT
nc_nav_dlg_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    khui_nc_wnd_data * ncd;

    switch (uMsg) {
    case WM_INITDIALOG:
        ncd = (khui_nc_wnd_data *) lParam;
        nc_set_dlg_data(hwnd, ncd);
        return TRUE;

    default:
        return FALSE;
    }
}

/* Identity specification.  IDD_NC_IDSPEC */
static LRESULT
nc_idspec_dlg_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    khui_nc_wnd_data * ncd;

    switch (uMsg) {
    case WM_INITDIALOG:
        ncd = (khui_nc_wnd_data *) lParam;
        nc_set_dlg_data(hwnd, ncd);
        return TRUE;

    default:
        return FALSE;
    }
}

/* Privileged Interaction (Basic). IDD_NC_PRIVINT_BASIC */
static LRESULT
nc_privint_basic_dlg_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    khui_nc_wnd_data * ncd;

    switch (uMsg) {
    case WM_INITDIALOG:
        ncd = (khui_nc_wnd_data *) lParam;
        nc_set_dlg_data(hwnd, ncd);
        return TRUE;

    default:
        return FALSE;
    }
}

/* Privileged Interaction (Advanced). IDD_NC_PRIVINT_ADVANCED */
static LRESULT
nc_privint_advanced_dlg_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    khui_nc_wnd_data * ncd;

    switch (uMsg) {
    case WM_INITDIALOG:
        ncd = (khui_nc_wnd_data *) lParam;
        nc_set_dlg_data(hwnd, ncd);
        return TRUE;

    default:
        return FALSE;
    }
}

/* Identity Selection.  IDD_NC_IDSEL */
static LRESULT
nc_idsel_dlg_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    khui_nc_wnd_data * ncd;

    switch (uMsg) {
    case WM_INITDIALOG:
        ncd = (khui_nc_wnd_data *) lParam;
        nc_set_dlg_data(hwnd, ncd);
        return TRUE;

    default:
        return FALSE;
    }
}

/* Progress. IDD_NC_PROGRESS */
static LRESULT
nc_progress_dlg_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    khui_nc_wnd_data * ncd;

    switch (uMsg) {
    case WM_INITDIALOG:
        ncd = (khui_nc_wnd_data *) lParam;
        nc_set_dlg_data(hwnd, ncd);
        return TRUE;

    default:
        return FALSE;
    }
}

/* No-Prompt Placeholder.  IDD_NC_NOPROMPTS */
static LRESULT
nc_noprompts_dlg_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    khui_nc_wnd_data * ncd;

    switch (uMsg) {
    case WM_INITDIALOG:
        ncd = (khui_nc_wnd_data *) lParam;
        nc_set_dlg_data(hwnd, ncd);
        return TRUE;

    default:
        return FALSE;
    }
}


/* Message handlers for the wizard. IDD_NC_CONTAINER*/



static LRESULT 
nc_handle_wm_initdialog(HWND hwnd,
                        UINT uMsg,
                        WPARAM wParam,
                        LPARAM lParam)
{
    khui_new_creds * c;
    khui_nc_wnd_data * ncd;
    int x, y;
    int width, height;
    RECT r;
    HFONT hf_main;

    ncd = (khui_nc_wnd_data *) lParam;
#ifdef debug
    assert(ncd);
#endif
    c = ncd->nc;
    c->hwnd = hwnd;

#ifdef DEBUG
    assert(c != NULL);
    assert(c->subtype == KMSG_CRED_NEW_CREDS ||
           c->subtype == KMSG_CRED_PASSWORD);
#endif
    nc_set_dlg_data(hwnd, ncd);

    ncd->hwnd_nav =
        CreateDialogIndirectParam(khm_hInstance,
                                  MAKEINTRESOURCE(IDD_NC_NAV),
                                  hwnd, nc_nav_dlg_proc, (LPARAM) ncd);
    ncd->hwnd_idsel =
        CreateDialogIndirectParam(khm_hInstance,
                                  MAKEINTRESOURCE(IDD_NC_IDSPEC),
                                  hwnd, nc_idsel_dlg_proc, (LPARAM) ncd);
    ncd->hwnd_privint_basic =
        CreateDialogIndirectParam(khm_hInstance,
                                  MAKEINTRESOURCE(IDD_NC_PRIVINT_BASIC),
                                  hwnd, nc_privint_basic_dlg_proc, (LPARAM) ncd);

    ncd->hwnd_privint_advanced =
        CreateDialogIndirectParam(khm_hInstance,
                                  MAKEINTRESOURCE(IDD_NC_PRIVINT_ADVANCED),
                                  hwnd, nc_privint_advanced_dlg_proc, (LPARAM) ncd);

    ncd->hwnd_idspec =
        CreateDialogIndirectParam(khm_hInstance,
                                  MAKEINTRESOURCE(IDD_NC_IDSEL),
                                  hwnd, nc_idspec_dlg_proc, (LPARAM) ncd);

    ncd->hwnd_progress =
        CreateDialogIndirectParam(khm_hInstance,
                                  MAKEINTRESOURCE(IDD_NC_PROGRESS),
                                  hwnd, nc_progress_dlg_proc, (LPARAM) ncd);

    ncd->hwnd_noprompts =
        CreateDialogIndirectParam(khm_hInstance,
                                  MAKEINTRESOURCE(IDD_NC_NOPROMPTS),
                                  hwnd, nc_noprompts_dlg_proc, (LPARAM) ncd);

    /* Position the dialog */

    GetWindowRect(hwnd, &r);

    width = r.right - r.left;
    height = r.bottom - r.top;

    /* if the parent window is visible, we center the new credentials
       dialog over the parent.  Otherwise, we center it on the primary
       display. */

    if (IsWindowVisible(lpc->hwndParent)) {
        GetWindowRect(lpc->hwndParent, &r);
    } else {
        if(!SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID) &r, 0)) {
            /* failover to the window coordinates */
            GetWindowRect(lpc->hwndParent, &r);
        }
    }
    x = (r.right + r.left)/2 - width / 2;
    y = (r.top + r.bottom)/2 - height / 2;

    /* we want to check if the entire rect is visible on the screen.
       If the main window is visible and in basic mode, we might end
       up with a rect that is partially outside the screen. */
    {
        RECT r;

        SetRect(&r, x, y, x + width, y + height);
        khm_adjust_window_dimensions_for_display(&r, 0);

        x = r.left;
        y = r.top;
        width = r.right - r.left;
        height = r.bottom - r.top;
    }

    MoveWindow(hwnd, x, y, width, height, FALSE);

    /* add this to the dialog chain */
    khm_add_dialog(hwnd);

    return TRUE;
}


static LRESULT 
nc_handle_wm_destroy(HWND hwnd,
                     UINT uMsg,
                     WPARAM wParam,
                     LPARAM lParam)
{
    khui_nc_wnd_data * d;

    /* remove self from dialog chain */
    khm_del_dialog(hwnd);

    d = nc_get_dlg_data(hwnd);
    if (d == NULL)
        return TRUE;

    d->nc->ident_cb(d->nc, WMNC_IDENT_EXIT, NULL, 0, 0, 0);

    if (d->hwnd_notif_label)
        DestroyWindow(d->hwnd_notif_label);
    if (d->hwnd_notif_aux)
        DestroyWindow(d->hwnd_notif_aux);

    if(d->dlg_bb)
        DestroyWindow(d->dlg_bb);
    if(d->dlg_main)
        DestroyWindow(d->dlg_main);

    d->dlg_bb = NULL;
    d->dlg_main = NULL;

    PFREE(d);
    SetWindowLongPtr(hwnd, CW_PARAM, 0);

    return TRUE;
}

static LRESULT 
nc_handle_wm_command(HWND hwnd,
                     UINT uMsg,
                     WPARAM wParam,
                     LPARAM lParam)
{
    khui_nc_wnd_data * d;

    d = nc_get_dlg_data(hwnd);
    if (d == NULL)
        return 0;

    switch(HIWORD(wParam)) {
    case BN_CLICKED:
        switch(LOWORD(wParam)) {

        case IDOK:
            d->nc->result = KHUI_NC_RESULT_PROCESS;

            /* fallthrough */

        case IDCANCEL:
            /* the default value for d->nc->result is set to
               KHUI_NC_RESULT_CANCEL */
            d->nc->response = KHUI_NC_RESPONSE_PROCESSING;

            nc_enable_controls(d, FALSE);

            nc_notify_types(d->nc, 
                            KHUI_WM_NC_NOTIFY, 
                            MAKEWPARAM(0,WMNC_DIALOG_PREPROCESS), 
                            (LPARAM) d->nc,
                            TRUE);

            khui_cw_sync_prompt_values(d->nc);

            khm_cred_dispatch_process_message(d->nc);

            /* we won't know whether to abort or not until we get
               feedback from the plugins, even if the command was
               to cancel */
            {
                HWND hw;

                hw = GetDlgItem(d->dlg_main, IDOK);
                EnableWindow(hw, FALSE);
                hw = GetDlgItem(d->dlg_main, IDCANCEL);
                EnableWindow(hw, FALSE);
                hw = GetDlgItem(d->dlg_main, IDC_NC_ADVANCED);
                EnableWindow(hw, FALSE);
                hw = GetDlgItem(d->dlg_bb, IDOK);
                EnableWindow(hw, FALSE);
                hw = GetDlgItem(d->dlg_bb, IDCANCEL);
                EnableWindow(hw, FALSE);
            }
            return FALSE;

        case IDC_NC_HELP:
            khm_html_help(hwnd, NULL, HH_HELP_CONTEXT, IDH_ACTION_NEW_ID);
            return FALSE;

        case IDC_NC_BASIC:
        case IDC_NC_ADVANCED: 
            /* the Options button in the main window was clicked.  we
               respond by expanding the dialog. */
            PostMessage(hwnd, KHUI_WM_NC_NOTIFY, 
                        MAKEWPARAM(0, WMNC_DIALOG_EXPAND), 0);
            return FALSE;

        case IDC_NC_CREDTEXT: /* credtext link activated */
            {
                khui_htwnd_link * l;
                wchar_t sid[KHUI_MAXCCH_HTLINK_FIELD];
                wchar_t sparam[KHUI_MAXCCH_HTLINK_FIELD];
                wchar_t * colon;

                l = (khui_htwnd_link *) lParam;

                /* do we have a valid link? */
                if(l->id == NULL || l->id_len >= ARRAYLENGTH(sid))
                    return TRUE; /* nope */

                StringCchCopyN(sid, ARRAYLENGTH(sid), l->id, l->id_len);
                sid[l->id_len] = L'\0'; /* just make sure */

                if(l->param != NULL && 
                   l->param_len < ARRAYLENGTH(sparam) &&
                   l->param_len > 0) {

                    StringCchCopyN(sparam, ARRAYLENGTH(sparam),
                                   l->param, l->param_len);
                    sparam[l->param_len] = L'\0';

                } else {
                    sparam[0] = L'\0';
                }

                /* If the ID is of the form '<credtype>:<link_tag>'
                   and <credtype> is a valid name of a credentials
                   type that is participating in the credentials
                   acquisition process, then we forward the message to
                   the panel that is providing the UI for that cred
                   type.  We also switch to that panel first, unless
                   the link is of the form '<credtype>:!<link_tag>'. */

                colon = wcschr(sid, L':');
                if (colon != NULL) {
                    khm_int32 credtype;
                    khui_new_creds_by_type * t;

                    *colon = L'\0';
                    if (KHM_SUCCEEDED(kcdb_credtype_get_id(sid, &credtype)) &&
                        KHM_SUCCEEDED(khui_cw_find_type(d->nc, credtype, &t))){
                        *colon = L':';

                        if (t->ordinal != d->current_panel &&
                            *(colon + 1) != L'!')
                            PostMessage(hwnd,
                                        KHUI_WM_NC_NOTIFY,
                                        MAKEWPARAM(t->ordinal,
                                                   WMNC_DIALOG_SWITCH_PANEL),
                                        0);

                        return SendMessage(t->hwnd_panel,
                                           KHUI_WM_NC_NOTIFY,
                                           MAKEWPARAM(0, WMNC_CREDTEXT_LINK),
                                           lParam);
                    } else {
                        *colon = L':';
                    }
                }

                /* if it was for us, then we need to process the message */
                if(!_wcsicmp(sid, CTLINKID_SWITCH_PANEL)) {
                    khm_int32 credtype;
                    khui_new_creds_by_type * t;

                    if (KHM_SUCCEEDED(kcdb_credtype_get_id(sparam, 
                                                           &credtype)) &&
                        KHM_SUCCEEDED(khui_cw_find_type(d->nc,
                                                        credtype, &t))) {
                        if (t->ordinal != d->current_panel)
                            PostMessage(hwnd,
                                        KHUI_WM_NC_NOTIFY,
                                        MAKEWPARAM(t->ordinal,
                                                   WMNC_DIALOG_SWITCH_PANEL),
                                        0);
                    }
                } else if (!_wcsicmp(sid, L"NotDef")) {
                    d->nc->set_default = FALSE;
                    nc_update_credtext(d);
                } else if (!_wcsicmp(sid, L"MakeDef")) {
                    d->nc->set_default = TRUE;
                    nc_update_credtext(d);
                }
            }
            return FALSE;

#if 0
        case NC_BN_SET_DEF_ID:
            {
                d->nc->set_default =
                    (IsDlgButtonChecked(d->dlg_main, NC_BN_SET_DEF_ID)
                     == BST_CHECKED);
            }
            return FALSE;
#endif
        }
        break;
    }

    return TRUE;
}

static LRESULT nc_handle_wm_moving(HWND hwnd,
                                   UINT uMsg,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
    khui_nc_wnd_data * d;

    d = nc_get_dlg_data(hwnd);
    if (d == NULL)
        return FALSE;

    nc_notify_types(d->nc, KHUI_WM_NC_NOTIFY, 
                    MAKEWPARAM(0, WMNC_DIALOG_MOVE), (LPARAM) d->nc, TRUE);

    return FALSE;
}

static LRESULT nc_handle_wm_nc_notify(HWND hwnd,
                               UINT uMsg,
                               WPARAM wParam,
                               LPARAM lParam)
{
    khui_nc_wnd_data * d;
    int id;

    d = nc_get_dlg_data(hwnd);
    if (d == NULL)
        return FALSE;

    switch(HIWORD(wParam)) {

    case WMNC_DIALOG_SWITCH_PANEL:
        id = LOWORD(wParam);
        if(id >= 0 && id <= (int) d->nc->n_types) {
            /* one of the tab buttons were pressed */
            if(d->current_panel == id) {
                return TRUE; /* nothing to do */
            }

            d->current_panel = id;

            TabCtrl_SetCurSel(d->tab_wnd, id);
        }

        if(d->nc->mode == KHUI_NC_MODE_EXPANDED) {
            nc_layout_new_cred_window(d);
            return TRUE;
        }
        /*else*/
        /* fallthrough */

    case WMNC_DIALOG_EXPAND:
        /* we are switching from basic to advanced or vice versa */

        if (d->nc->mode == KHUI_NC_MODE_EXPANDED) {

            if (d->current_panel != 0) {
                d->current_panel = 0;
                TabCtrl_SetCurSel(d->tab_wnd, 0);
                nc_layout_new_cred_window(d);
            }

            d->nc->mode = KHUI_NC_MODE_MINI;
        } else {
            d->nc->mode = KHUI_NC_MODE_EXPANDED;
        }

        /* if we are switching to the advanced mode, we clear any
           notifications because we now have a credential text area
           for that. */
        if (d->nc->mode == KHUI_NC_MODE_EXPANDED)
            nc_notify_clear(d);

        nc_layout_main_panel(d);

        nc_layout_new_cred_window(d);

        break;

    case WMNC_DIALOG_SETUP:

        if(d->nc->n_types > 0) {
            khm_size i;
            for(i=0; i < d->nc->n_types;i++) {

                if (d->nc->types[i]->dlg_proc == NULL) {
                    d->nc->types[i]->hwnd_panel = NULL;
                } else {
                    /* Create the dialog panel */
                    d->nc->types[i]->hwnd_panel = 
                        CreateDialogParam(d->nc->types[i]->h_module,
                                          d->nc->types[i]->dlg_template,
                                          d->nc->hwnd,
                                          d->nc->types[i]->dlg_proc,
                                          (LPARAM) d->nc);

#ifdef DEBUG
                    assert(d->nc->types[i]->hwnd_panel);
#endif
#if _WIN32_WINNT >= 0x0501
                    if (d->nc->types[i]->hwnd_panel) {
                        EnableThemeDialogTexture(d->nc->types[i]->hwnd_panel,
                                                 ETDT_ENABLETAB);
                    }
#endif
                }
            }
        }

        break;

    case WMNC_DIALOG_ACTIVATE:
        {
            wchar_t wname[KCDB_MAXCCH_NAME];
            TCITEM tabitem;
            khm_int32 t;

            /* About to activate the window. We should add all the
               panels to the tab control.  */

#ifdef DEBUG
            assert(d->tab_wnd != NULL);
#endif

            ZeroMemory(&tabitem, sizeof(tabitem));

            tabitem.mask = TCIF_PARAM | TCIF_TEXT;

            LoadString(khm_hInstance, IDS_NC_IDENTITY, 
                       wname, ARRAYLENGTH(wname));

            tabitem.pszText = wname;
            tabitem.lParam = 0; /* ordinal */

            TabCtrl_InsertItem(d->tab_wnd, 0, &tabitem);

            khui_cw_lock_nc(d->nc);

            if(d->nc->n_types > 0) {
                khm_size i;

                /* We should sort the tabs first.  See
                   nc_tab_sort_func() for sort criteria. */
                qsort(d->nc->types, 
                      d->nc->n_types, 
                      sizeof(*(d->nc->types)), 
                      nc_tab_sort_func);

                for(i=0; i < d->nc->n_types;i++) {

                    d->nc->types[i]->ordinal = i + 1;

                    if(d->nc->types[i]->name)
                        tabitem.pszText = d->nc->types[i]->name;
                    else {
                        khm_size cbsize;

                        cbsize = sizeof(wname);

                        if(KHM_FAILED
                           (kcdb_credtype_describe
                            (d->nc->types[i]->type, 
                             wname, 
                             &cbsize, 
                             KCDB_TS_SHORT))) {

#ifdef DEBUG
                            assert(FALSE);
#endif
                            wname[0] = L'\0';

                        }

                        tabitem.pszText = wname;

                    }

                    tabitem.lParam = d->nc->types[i]->ordinal;

                    TabCtrl_InsertItem(d->tab_wnd, d->nc->types[i]->ordinal,
                                       &tabitem);
                }
            }

            khui_cw_unlock_nc(d->nc);

            nc_update_credtext(d);

            TabCtrl_SetCurSel(d->tab_wnd, 0); /* the first selected
                                                 tab is the main
                                                 panel. */

            /* we don't enable animations until a specific timeout
               elapses after showing the window.  We don't need to
               animate any size changes if the user has barely had a
               chance to notice the original size. This prevents the
               new cred window from appearing in an animated state. */
            SetTimer(hwnd, NC_TIMER_ENABLEANIMATE, ENABLEANIMATE_TIMEOUT, NULL);

            ShowWindow(hwnd, SW_SHOWNORMAL);

            /* bring the window to the top, if necessary */
            if (KHM_SUCCEEDED(khc_read_int32(NULL,
                                             L"CredWindow\\Windows\\NewCred\\ForceToTop",
                                             &t)) &&

                t != 0) {

                BOOL sfw = FALSE;

                /* it used to be that the above condition also called
                   !khm_is_dialog_active() to find out whether there
                   was a dialog active.  If there was, we wouldn't try
                   to bring the new cred window to the foreground. But
                   that was not the behavior we want. */

                /* if the main window is not visible, then the SetWindowPos()
                   call is sufficient to bring the new creds window to the
                   top.  However, if the main window is visible but not
                   active, the main window needs to be activated before a
                   child window can be activated. */

                SetActiveWindow(hwnd);

                sfw = SetForegroundWindow(hwnd);

                if (!sfw) {
                    FLASHWINFO fi;

                    ZeroMemory(&fi, sizeof(fi));

                    fi.cbSize = sizeof(fi);
                    fi.hwnd = hwnd;
                    fi.dwFlags = FLASHW_ALL;
                    fi.uCount = 3;
                    fi.dwTimeout = 0; /* use the default cursor blink rate */

                    FlashWindowEx(&fi);

                    d->flashing_enabled = TRUE;
                }

            } else {
                SetFocus(hwnd);
            }

            if (d->nc->n_identities == 0)
                break;
            /* else */
            /*   fallthrough */
        }

    case WMNC_IDENTITY_CHANGE:
        {
            BOOL okEnable = FALSE;

            nc_notify_types(d->nc, KHUI_WM_NC_NOTIFY,
                            MAKEWPARAM(0, WMNC_IDENTITY_CHANGE), (LPARAM) d->nc,
                            TRUE);

            if (d->nc->subtype == KMSG_CRED_NEW_CREDS &&
                d->nc->n_identities > 0 &&
                d->nc->identities[0]) {
                khm_int32 f = 0;

                kcdb_identity_get_flags(d->nc->identities[0], &f);

                if (!(f & KCDB_IDENT_FLAG_DEFAULT)) {
                    d->nc->set_default = FALSE;
                }
            }

            nc_update_credtext(d);

        }
        break;

    case WMNC_TYPE_STATE:
        /* fallthrough */
    case WMNC_UPDATE_CREDTEXT:
        nc_update_credtext(d);
        break;

    case WMNC_CLEAR_PROMPTS:
        {
            khm_size i;

            khui_cw_lock_nc(d->nc);

            if(d->hwnd_banner != NULL) {
                DestroyWindow(d->hwnd_banner);
                d->hwnd_banner = NULL;
            }

            if(d->hwnd_name != NULL) {
                DestroyWindow(d->hwnd_name);
                d->hwnd_name = NULL;
            }

            for(i=0;i<d->nc->n_prompts;i++) {
                if(!(d->nc->prompts[i]->flags & 
                     KHUI_NCPROMPT_FLAG_STOCK)) {
                    if(d->nc->prompts[i]->hwnd_static != NULL)
                        DestroyWindow(d->nc->prompts[i]->hwnd_static);

                    if(d->nc->prompts[i]->hwnd_edit != NULL)
                        DestroyWindow(d->nc->prompts[i]->hwnd_edit);
                }

                d->nc->prompts[i]->hwnd_static = NULL;
                d->nc->prompts[i]->hwnd_edit = NULL;
            }

            khui_cw_unlock_nc(d->nc);

            SetRectEmpty(&d->r_custprompt);

            nc_layout_main_panel(d);

            nc_layout_new_cred_window(d);
        }
        break;

    case WMNC_SET_PROMPTS:
        {
            khm_size i;
            int  y;
            HWND hw, hw_prev;
            HFONT hf, hfold;
            HDC hdc;
            BOOL use_large_lables = FALSE;

            /* we assume that WMNC_CLEAR_PROMPTS has already been
               received */

#ifdef DEBUG
            assert(IsRectEmpty(&d->r_custprompt));
#endif

            khui_cw_lock_nc(d->nc);

#if 0
            /* special case, we have one prompt and it is a password
               prompt.  very common */
            if(d->nc->n_prompts == 1 && 
               d->nc->prompts[0]->type == KHUI_NCPROMPT_TYPE_PASSWORD) {

                hw = GetDlgItem(d->dlg_main, IDC_NC_PASSWORD);
                EnableWindow(hw, TRUE);

                d->nc->prompts[0]->flags |= KHUI_NCPROMPT_FLAG_STOCK;
                d->nc->prompts[0]->hwnd_edit = hw;
                d->nc->prompts[0]->hwnd_static = NULL; /* don't care */

                khui_cw_unlock_nc(d->nc);
                break;
            }
#endif
            /* for everything else */

            y = d->r_idspec.bottom;

            d->r_custprompt.left = d->r_area.left;
            d->r_custprompt.right = d->r_area.right;
            d->r_custprompt.top = y;

            hf = (HFONT) SendMessage(d->dlg_main, WM_GETFONT, 0, 0);

            if (d->nc->pname != NULL) {
                hw =
                    CreateWindowEx
                    (0,
                     L"STATIC",
                     d->nc->pname,
                     SS_SUNKEN | WS_CHILD,
                     d->r_area.left, y,
                     d->r_row.right, 
                     d->r_n_label.bottom - d->r_n_label.top,
                     d->dlg_main,
                     NULL,
                     khm_hInstance,
                     NULL);

#ifdef DEBUG
                assert(hw);
#endif
                d->hwnd_name = hw;
                SendMessage(hw, WM_SETFONT, (WPARAM)hf, (LPARAM) TRUE);
                ShowWindow(hw, SW_SHOW);

                y += d->r_n_label.bottom - d->r_n_label.top;
            }

            if (d->nc->banner != NULL) {
                hw = 
                    CreateWindowEx
                    (0,
                     L"STATIC",
                     d->nc->banner,
                     WS_CHILD,
                     d->r_area.left, y,
                     d->r_row.right, d->r_row.bottom,
                     d->dlg_main,
                     NULL,
                     khm_hInstance,
                     NULL);
#ifdef DEBUG
                assert(hw);
#endif
                d->hwnd_banner = hw;
                SendMessage(hw, WM_SETFONT, (WPARAM)hf, (LPARAM)TRUE);
                ShowWindow(hw, SW_SHOW);
                y += d->r_row.bottom;
            }

            hw_prev = d->hwnd_last_idspec;

            hdc = GetWindowDC(d->dlg_main);
            hfold = SelectObject(hdc,hf);

            /* first do a trial run and see if we should use the
               larger text labels or not.  This is so that all the
               labels and input controls align properly. */
            for (i=0; i < d->nc->n_prompts; i++) {
                if (d->nc->prompts[i]->prompt != NULL) {
                    SIZE s;

                    GetTextExtentPoint32(hdc, 
                                         d->nc->prompts[i]->prompt, 
                                         (int) wcslen(d->nc->prompts[i]->prompt),
                                         &s);

                    if(s.cx >= d->r_n_label.right - d->r_n_label.left) {
                        use_large_lables = TRUE;
                        break;
                    }
                }
            }

            for(i=0; i<d->nc->n_prompts; i++) {
                RECT pr, er;
                SIZE s;
                int dy;

                if(d->nc->prompts[i]->prompt != NULL) {
                    GetTextExtentPoint32(hdc, 
                                         d->nc->prompts[i]->prompt, 
                                         (int) wcslen(d->nc->prompts[i]->prompt),
                                         &s);
                    if(s.cx < d->r_n_label.right - d->r_n_label.left &&
                       !use_large_lables) {
                        CopyRect(&pr, &d->r_n_label);
                        CopyRect(&er, &d->r_n_input);
                        dy = d->r_row.bottom;
                    } else if(s.cx <
                              d->r_e_label.right - d->r_e_label.left) {
                        CopyRect(&pr, &d->r_e_label);
                        CopyRect(&er, &d->r_e_input);
                        dy = d->r_row.bottom;
                    } else {
                        /* oops. the prompt doesn't fit in our
                           controls.  we need to use up two lines */
                        pr.left = 0;
                        pr.right = d->r_row.right;
                        pr.top = 0;
                        pr.bottom = d->r_n_label.bottom - 
                            d->r_n_label.top;
                        CopyRect(&er, &d->r_n_input);
                        OffsetRect(&er, 0, pr.bottom);
                        dy = er.bottom + (d->r_row.bottom - 
                                          d->r_n_input.bottom);
                    }
                } else {
                    SetRectEmpty(&pr);
                    CopyRect(&er, &d->r_n_input);
                    dy = d->r_row.bottom;
                }

                if(IsRectEmpty(&pr)) {
                    d->nc->prompts[i]->hwnd_static = NULL;
                } else {
                    OffsetRect(&pr, d->r_area.left, y);

                    hw = CreateWindowEx
                        (0,
                         L"STATIC",
                         d->nc->prompts[i]->prompt,
                         WS_CHILD,
                         pr.left, pr.top,
                         pr.right - pr.left, pr.bottom - pr.top,
                         d->dlg_main,
                         NULL,
                         khm_hInstance,
                         NULL);
#ifdef DEBUG
                    assert(hw);
#endif

                    SendMessage(hw, WM_SETFONT, 
                                (WPARAM) hf, (LPARAM) TRUE);

                    SetWindowPos(hw, hw_prev,
                                 0, 0, 0, 0,
                                 SWP_NOACTIVATE | SWP_NOMOVE |
                                 SWP_NOOWNERZORDER | SWP_NOSIZE |
                                 SWP_SHOWWINDOW);

                    d->nc->prompts[i]->hwnd_static = hw;
                    hw_prev = hw;
                }

                OffsetRect(&er, d->r_area.left, y);

                hw = CreateWindowEx
                    (0,
                     L"EDIT",
                     (d->nc->prompts[i]->def ? 
                      d->nc->prompts[i]->def : L""),
                     WS_CHILD | WS_TABSTOP |
                     WS_BORDER |
                     ((d->nc->prompts[i]->flags & 
                       KHUI_NCPROMPT_FLAG_HIDDEN)? ES_PASSWORD:0),
                     er.left, er.top,
                     er.right - er.left, er.bottom - er.top,
                     d->dlg_main,
                     NULL,
                     khm_hInstance,
                     NULL);

#ifdef DEBUG
                assert(hw);
#endif

                SendMessage(hw, WM_SETFONT, 
                            (WPARAM) hf, (LPARAM) TRUE);

                SetWindowPos(hw, hw_prev,
                             0, 0, 0, 0, 
                             SWP_NOACTIVATE | SWP_NOMOVE | 
                             SWP_NOOWNERZORDER | SWP_NOSIZE | 
                             SWP_SHOWWINDOW);

                SendMessage(hw, EM_SETLIMITTEXT,
                            KHUI_MAXCCH_PROMPT_VALUE -1,
                            0);

                d->nc->prompts[i]->hwnd_edit = hw;

                hw_prev = hw;

                y += dy;
            }

            if (d->nc->n_prompts > 0 &&
                d->nc->prompts[0]->hwnd_edit) {

                PostMessage(d->dlg_main, WM_NEXTDLGCTL,
                            (WPARAM) d->nc->prompts[0]->hwnd_edit,
                            MAKELPARAM(TRUE, 0));

            }

            SelectObject(hdc, hfold);
            ReleaseDC(d->dlg_main, hdc);

            khui_cw_unlock_nc(d->nc);

            d->r_custprompt.bottom = y;

            if (d->r_custprompt.bottom == d->r_custprompt.top)
                SetRectEmpty(&d->r_custprompt);

            nc_layout_main_panel(d);

            nc_layout_new_cred_window(d);
        }
        break;

    case WMNC_DIALOG_PROCESS_COMPLETE:
        {
            khui_new_creds * nc;

            nc = d->nc;

            nc->response &= ~KHUI_NC_RESPONSE_PROCESSING;

            if(nc->response & KHUI_NC_RESPONSE_NOEXIT) {
                HWND hw;

                nc_enable_controls(d, TRUE);

                /* reset state */
                nc->result = KHUI_NC_RESULT_CANCEL;

                hw = GetDlgItem(d->dlg_main, IDOK);
                EnableWindow(hw, TRUE);
                hw = GetDlgItem(d->dlg_main, IDCANCEL);
                EnableWindow(hw, TRUE);
                hw = GetDlgItem(d->dlg_main, IDC_NC_ADVANCED);
                EnableWindow(hw, TRUE);
                hw = GetDlgItem(d->dlg_bb, IDOK);
                EnableWindow(hw, TRUE);
                hw = GetDlgItem(d->dlg_bb, IDCANCEL);
                EnableWindow(hw, TRUE);

                nc_clear_password_fields(d);

                return TRUE;
            }

            DestroyWindow(hwnd);

            kmq_post_message(KMSG_CRED, KMSG_CRED_END, 0, (void *) nc);
        }
        break;

        /* MUST be called with SendMessage */
    case WMNC_ADD_CONTROL_ROW:
        {
            khui_control_row * row;

            row = (khui_control_row *) lParam;

#ifdef DEBUG
            assert(row->label);
            assert(row->input);
#endif

            nc_add_control_row(d, row->label, row->input, row->size);
        }
        break;

    case WMNC_UPDATE_LAYOUT:
        {

            RECT r_client;
            khm_int32 animate;
            khm_int32 steps;
            khm_int32 timeout;

            /* We are already adjusting the size of the window.  The
               next time the timer fires, it will notice if the target
               size has changed. */
            if (d->size_changing)
                return TRUE;

            GetClientRect(hwnd, &r_client);

            if ((r_client.right - r_client.left ==
                 d->r_required.right - d->r_required.left) &&
                (r_client.bottom - r_client.top ==
                 d->r_required.bottom - d->r_required.top)) {

                /* the window is already at the right size */
                return TRUE;

            }

            if (!IsWindowVisible(hwnd)) {
                /* The window is not visible yet.  There's no need to
                   animate anything. */

                animate = FALSE;

            } else if (KHM_FAILED(khc_read_int32(NULL,
                                                 L"CredWindow\\Windows\\NewCred\\AnimateSizeChanges",
                                                 &animate))) {
#ifdef DEBUG
                assert(FALSE);
#endif
                animate = TRUE;
            }

            /* if we aren't animating the window resize, then we just
               do it in one call. */
            if (!animate || !d->animation_enabled) {
                RECT r_window;

                CopyRect(&r_window, &d->r_required);
                AdjustWindowRectEx(&r_window, NC_WINDOW_STYLES, FALSE,
                                   NC_WINDOW_EX_STYLES);

                SetWindowPos(hwnd, NULL, 0, 0,
                             r_window.right - r_window.left,
                             r_window.bottom - r_window.top,
                             SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER |
                             SWP_NOZORDER);

                return TRUE;
            }

            if (KHM_FAILED(khc_read_int32(NULL,
                                          L"CredWindow\\Windows\\NewCred\\AnimationSteps",
                                          &steps))) {
#ifdef DEBUG
                assert(FALSE);
#endif
                steps = NC_SZ_STEPS_DEF;
            } else {

                if (steps < NC_SZ_STEPS_MIN)
                    steps = NC_SZ_STEPS_MIN;
                else if (steps > NC_SZ_STEPS_MAX)
                    steps = NC_SZ_STEPS_MAX;

            }

            if (KHM_FAILED(khc_read_int32(NULL,
                                          L"CredWindow\\Windows\\NewCred\\AnimationStepTimeout",
                                          &timeout))) {
#ifdef DEBUG
                assert(FALSE);
#endif
                timeout = NC_SZ_TIMEOUT_DEF;
            } else {

                if (timeout < NC_SZ_TIMEOUT_MIN)
                    timeout = NC_SZ_TIMEOUT_MIN;
                else if (timeout > NC_SZ_TIMEOUT_MAX)
                    timeout = NC_SZ_TIMEOUT_MAX;

            }

            CopyRect(&d->sz_ch_source, &r_client);
            OffsetRect(&d->sz_ch_source, -d->sz_ch_source.left, -d->sz_ch_source.top);
            CopyRect(&d->sz_ch_target, &d->r_required);
            OffsetRect(&d->sz_ch_target, -d->sz_ch_target.left, -d->sz_ch_target.top);
            d->sz_ch_increment = 0;
            d->sz_ch_max = steps;
            d->sz_ch_timeout = timeout;
            d->size_changing = TRUE;

            SetTimer(hwnd, NC_TIMER_SIZER, timeout, NULL);
        }
        break;
    } /* switch(HIWORD(wParam)) */

    return TRUE;
}

static LRESULT nc_handle_wm_timer(HWND hwnd,
                                  UINT uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam) {
    khui_nc_wnd_data * d;

    d = nc_get_dlg_data(hwnd);
    if (d == NULL)
        return FALSE;

    if (wParam == NC_TIMER_SIZER) {

        RECT r_now;

        /* are we done with this sizing operation? */
        if (!d->size_changing ||
            d->sz_ch_increment >= d->sz_ch_max) {

            d->size_changing = FALSE;
            KillTimer(hwnd, NC_TIMER_SIZER);
            return 0;
        }

        /* have the requirements changed while we were processing the
           sizing operation? */
        if ((d->r_required.right - d->r_required.left !=
             d->sz_ch_target.right)

            ||

            (d->r_required.bottom - d->r_required.top !=
             d->sz_ch_target.bottom)) {

            /* the target size has changed.  we need to restart the
               sizing operation. */

            RECT r_client;

            GetClientRect(hwnd, &r_client);

            CopyRect(&d->sz_ch_source, &r_client);
            OffsetRect(&d->sz_ch_source, -d->sz_ch_source.left, -d->sz_ch_source.top);
            CopyRect(&d->sz_ch_target, &d->r_required);
            OffsetRect(&d->sz_ch_target, -d->sz_ch_target.left, -d->sz_ch_target.top);
            d->sz_ch_increment = 0;

            /* leave the other fields alone */

#ifdef DEBUG
            assert(d->sz_ch_max >= NC_SZ_STEPS_MIN);
            assert(d->sz_ch_max <= NC_SZ_STEPS_MAX);
            assert(d->sz_ch_timeout >= NC_SZ_TIMEOUT_MIN);
            assert(d->sz_ch_timeout <= NC_SZ_TIMEOUT_MAX);
            assert(d->size_changing);
#endif
        }

        /* we are going to do the next increment */
        d->sz_ch_increment ++;

        /* now, figure out the size of the client area for this
           step */

        r_now.left = 0;
        r_now.top = 0;

#define PROPORTION(v1, v2, i, s) (((v1) * ((s) - (i)) + (v2) * (i)) / (s))

        r_now.right = PROPORTION(d->sz_ch_source.right, d->sz_ch_target.right,
                                 d->sz_ch_increment, d->sz_ch_max);

        r_now.bottom = PROPORTION(d->sz_ch_source.bottom, d->sz_ch_target.bottom,
                                  d->sz_ch_increment, d->sz_ch_max);

#undef  PROPORTION

#ifdef DEBUG
        {
            long dx = ((r_now.right - r_now.left) - d->sz_ch_target.right) *
                (d->sz_ch_source.right - d->sz_ch_target.right);

            long dy = ((r_now.bottom - r_now.top) - d->sz_ch_target.bottom) *
                (d->sz_ch_source.bottom - d->sz_ch_target.bottom);

            if (dx < 0 || dy < 0) {
                KillTimer(hwnd, NC_TIMER_SIZER);
                assert(dx >= 0);
                assert(dy >= 0);
                SetTimer(hwnd, NC_TIMER_SIZER, d->sz_ch_timeout, NULL);
            }
        }
#endif

        AdjustWindowRectEx(&r_now, NC_WINDOW_STYLES, FALSE,
                           NC_WINDOW_EX_STYLES);

        {
            RECT r;

            GetWindowRect(hwnd, &r);
            OffsetRect(&r_now, r.left - r_now.left, r.top - r_now.top);
        }

        khm_adjust_window_dimensions_for_display(&r_now, 0);

        SetWindowPos(hwnd, NULL,
                     r_now.left, r_now.top,
                     r_now.right - r_now.left,
                     r_now.bottom - r_now.top,
                     SWP_NOACTIVATE | SWP_NOOWNERZORDER |
                     SWP_NOZORDER);

        /* and now we wait for the next timer message */

        return 0;
    } else if (wParam == NC_TIMER_ENABLEANIMATE) {

        d->animation_enabled = TRUE;
        KillTimer(hwnd, NC_TIMER_ENABLEANIMATE);
    }

    return 0;
}

static LRESULT nc_handle_wm_notify(HWND hwnd,
                                   UINT uMsg,
                                   WPARAM wParam,
                                   LPARAM lParam) {

    LPNMHDR nmhdr;
    khui_nc_wnd_data * d;

    d = nc_get_dlg_data(hwnd);
    if (d == NULL)
        return FALSE;

    nmhdr = (LPNMHDR) lParam;

    if (nmhdr->code == TCN_SELCHANGE) {
        /* the current tab has changed. */
        int idx;
        TCITEM tcitem;

        idx = TabCtrl_GetCurSel(d->tab_wnd);
        ZeroMemory(&tcitem, sizeof(tcitem));

        tcitem.mask = TCIF_PARAM;
        TabCtrl_GetItem(d->tab_wnd, idx, &tcitem);

        d->current_panel = (int) tcitem.lParam;

        nc_layout_new_cred_window(d);

        return TRUE;
    }

    return FALSE;
}

static LRESULT nc_handle_wm_help(HWND hwnd,
                                 UINT uMsg,
                                 WPARAM wParam,
                                 LPARAM lParam) {
    static DWORD ctxids[] = {
        NC_TS_CTRL_ID_MIN, IDH_NC_TABMAIN,
        NC_TS_CTRL_ID_MIN + 1, IDH_NC_TABBUTTON,
        NC_TS_CTRL_ID_MIN + 2, IDH_NC_TABBUTTON,
        NC_TS_CTRL_ID_MIN + 3, IDH_NC_TABBUTTON,
        NC_TS_CTRL_ID_MIN + 4, IDH_NC_TABBUTTON,
        NC_TS_CTRL_ID_MIN + 5, IDH_NC_TABBUTTON,
        NC_TS_CTRL_ID_MIN + 6, IDH_NC_TABBUTTON,
        NC_TS_CTRL_ID_MIN + 7, IDH_NC_TABBUTTON,
        IDOK, IDH_NC_OK,
        IDCANCEL, IDH_NC_CANCEL,
        IDC_NC_HELP, IDH_NC_HELP,
        IDC_NC_ADVANCED, IDH_NC_ADVANCED,
        IDC_NC_CREDTEXT, IDH_NC_CREDWND,
        0
    };

    HELPINFO * hlp;
    HWND hw = NULL;
    HWND hw_ctrl;
    khui_nc_wnd_data * d;

    d = nc_get_dlg_data(hwnd);
    if (d == NULL)
        return FALSE;

    hlp = (HELPINFO *) lParam;

    if (d->nc->subtype != KMSG_CRED_NEW_CREDS &&
        d->nc->subtype != KMSG_CRED_PASSWORD)
        return TRUE;

    if (hlp->iContextType != HELPINFO_WINDOW)
        return TRUE;

    if (hlp->hItemHandle != NULL &&
        hlp->hItemHandle != hwnd) {
        DWORD id;
        int i;

        hw_ctrl =hlp->hItemHandle;

        id = GetWindowLong(hw_ctrl, GWL_ID);
        for (i=0; ctxids[i] != 0; i += 2)
            if (ctxids[i] == id)
                break;

        if (ctxids[i] != 0)
            hw = khm_html_help(hw_ctrl,
                               ((d->nc->subtype == KMSG_CRED_NEW_CREDS)?
                                L"::popups_newcreds.txt":
                                L"::popups_password.txt"),
                               HH_TP_HELP_WM_HELP,
                               (DWORD_PTR) ctxids);
    }

    if (hw == NULL) {
        khm_html_help(hwnd, NULL, HH_HELP_CONTEXT,
                      ((d->nc->subtype == KMSG_CRED_NEW_CREDS)?
                       IDH_ACTION_NEW_ID: IDH_ACTION_PASSWD_ID));
    }

    return TRUE;
}

static LRESULT nc_handle_wm_activate(HWND hwnd,
                                     UINT uMsg,
                                     WPARAM wParam,
                                     LPARAM lParam) {
    if (uMsg == WM_MOUSEACTIVATE ||
        wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE) {

        FLASHWINFO fi;
        khui_nc_wnd_data * d;
        DWORD_PTR ex_style;

        d = nc_get_dlg_data(hwnd);

        if (d && d->flashing_enabled) {
            ZeroMemory(&fi, sizeof(fi));

            fi.cbSize = sizeof(fi);
            fi.hwnd = hwnd;
            fi.dwFlags = FLASHW_STOP;

            FlashWindowEx(&fi);

            d->flashing_enabled = FALSE;
        }

        ex_style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

        if (ex_style & WS_EX_TOPMOST) {
            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }

    return (uMsg == WM_MOUSEACTIVATE)? MA_ACTIVATE : 0;
}

INT_PTR CALLBACK nc_dlg_proc(HWND hwnd,
                             UINT uMsg,
                             WPARAM wParam,
                             LPARAM lParam)
{
    switch(uMsg) {
    case WM_INITDIALOG:
        return nc_handle_wm_initdialog(hwnd, uMsg, wParam, lParam);

    case WM_CLOSE:
        return nc_handle_wm_close(hwnd, uMsg, wParam, lParam);

    case WM_DESTROY:
        return nc_handle_wm_destroy(hwnd, uMsg, wParam, lParam);

    case WM_COMMAND:
        return nc_handle_wm_command(hwnd, uMsg, wParam, lParam);

    case WM_MOUSEACTIVATE:
    case WM_ACTIVATE:
        return nc_handle_wm_activate(hwnd, uMsg, wParam, lParam);

    case WM_NOTIFY:
        return nc_handle_wm_notify(hwnd, uMsg, wParam, lParam);

    case WM_MOVE:
    case WM_MOVING:
        return nc_handle_wm_moving(hwnd, uMsg, wParam, lParam);

    case WM_TIMER:
        return nc_handle_wm_timer(hwnd, uMsg, wParam, lParam);

    case WM_HELP:
        return nc_handle_wm_help(hwnd, uMsg, wParam, lParam);

    case KHUI_WM_NC_NOTIFY:
        return nc_handle_wm_nc_notify(hwnd, uMsg, wParam, lParam);
    }

    return FALSE;
}

void khm_register_newcredwnd_class(void)
{
    /* Nothing to do */
}

void khm_unregister_newcredwnd_class(void)
{
    /* Nothing to do */
}


HWND khm_create_newcredwnd(HWND parent, khui_new_creds * c)
{
    wchar_t wtitle[256];
    HWND hwnd;
    khui_nc_wnd_data * d;

    d = (khui_nc_wnd_data *) PMALLOC(sizeof(*d));
#ifdef DEBUG
    assert(d != NULL);
#endif
    ZeroMemory(d, sizeof(*d));

    d->nc = c;

    khui_cw_lock_nc(c);

    if (c->window_title == NULL) {
        size_t t = 0;

        wtitle[0] = L'\0';

        if (c->subtype == KMSG_CRED_PASSWORD)
            LoadString(khm_hInstance, 
                       IDS_WT_PASSWORD,
                       wtitle,
                       ARRAYLENGTH(wtitle));
        else
            LoadString(khm_hInstance, 
                       IDS_WT_NEW_CREDS,
                       wtitle,
                       ARRAYLENGTH(wtitle));

        StringCchLength(wtitle, ARRAYLENGTH(wtitle), &t);
        if (t > 0) {
            t = (t + 1) * sizeof(wchar_t);
            c->window_title = PMALLOC(t);
            StringCbCopy(c->window_title, t, wtitle);
        }
    }

    khui_cw_unlock_nc(c);

    khc_read_int32(NULL, L"CredWindow\\Windows\\NewCred\\ForceToTop", &d->force_topmost);

    hwnd = CreateDialogParam(khm_hInstance, MAKEINTRESOURCE(IDD_NC_CONTAINER),
                             parent, nc_dlg_proc,  (LPVOID) d);

#ifdef DEBUG
    assert(hwnd != NULL);
#endif

    /* note that the window is not visible yet.  That's because, at
       this point we don't know what the panels are */

    return hwnd;
}

void khm_prep_newcredwnd(HWND hwnd)
{
    SendMessage(hwnd, KHUI_WM_NC_NOTIFY, 
                MAKEWPARAM(0, WMNC_DIALOG_SETUP), 0);
}

void khm_show_newcredwnd(HWND hwnd)
{
    /* add all the panels in and prep UI */
    PostMessage(hwnd, KHUI_WM_NC_NOTIFY, 
                MAKEWPARAM(0, WMNC_DIALOG_ACTIVATE), 0);
}
