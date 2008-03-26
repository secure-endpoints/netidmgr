/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
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

/* Include the OEMRESOURCE constants for locating standard icon
   resources. */
#define OEMRESOURCE



#include "khmapp.h"
#if _WIN32_WINNT >= 0x0501
#include<uxtheme.h>
#endif
#include<assert.h>



static HWND
nc_create_custom_prompter_dialog(khui_new_creds * nc,
                                 HWND parent,
                                 khui_new_creds_privint_panel * p);

static void
nc_notify_new_identity(khui_new_creds * nc, BOOL notify_ui);

static void
nc_navigate(khui_new_creds * nc, nc_page new_page);

#define rect_coords(r) r.left, r.top, (r.right - r.left), (r.bottom - r.top)






/* Set dialog window data */
static void
nc_set_dlg_data(HWND hwnd, khui_new_creds * nc)
{
#pragma warning(push)
#pragma warning(disable: 4244)
    SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) nc);
#pragma warning(pop)
}

/* Get dialog window data */
static khui_new_creds *
nc_get_dlg_data(HWND hwnd)
{
    return (khui_new_creds *)(LONG_PTR) GetWindowLongPtr(hwnd, DWLP_USER);
}

/* Set the return value for a dialog */
static void
nc_set_dlg_retval(HWND hwnd, LONG_PTR  rv)
{
#pragma warning(push)
#pragma warning(disable: 4244)
    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, rv);
#pragma warning(pop)
}





/* Flag combinations for SetWindowPos/DeferWindowPos */

/* Move+Size+ZOrder */
#define SWP_MOVESIZEZ (SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_SHOWWINDOW)

/* Move+Size */
#define SWP_MOVESIZE  (SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_SHOWWINDOW|SWP_NOZORDER)

/* Size */
#define SWP_SIZEONLY (SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER)

/* Hide */
#define SWP_HIDEONLY (SWP_NOACTIVATE|SWP_HIDEWINDOW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOOWNERZORDER|SWP_NOZORDER)

/* Show */
#define SWP_SHOWONLY (SWP_NOACTIVATE|SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOOWNERZORDER|SWP_NOZORDER)




/**********************************************************************
 *  Identity Selector Dialog (IDD_NC_IDSEL)
 *  
 *  This block also holds the code that displays the identity selector
 *  menu.
 **********************************************************************/

static void
get_ident_display_data(nc_ident_display * d, khm_handle ident)
{
    khm_size cb;
    khm_handle idpro = NULL;

    if (d->hident)
        kcdb_identity_release(d->hident);

    ZeroMemory(d, sizeof(*d));

    if (ident == NULL)
        return;

    cb = sizeof(d->icon);
    kcdb_get_resource(ident, KCDB_RES_ICON_NORMAL, 0, NULL, NULL,
                      &d->icon, &cb);

    cb = sizeof(d->display_string);
    kcdb_get_resource(ident, KCDB_RES_DISPLAYNAME, KCDB_RFS_SHORT, NULL, NULL,
                      d->display_string, &cb);

    kcdb_identity_get_identpro(ident, &idpro);

    cb = sizeof(d->type_string);
    kcdb_get_resource(idpro, KCDB_RES_INSTANCE, KCDB_RFS_SHORT, NULL, NULL,
                      d->type_string, &cb);

    kcdb_identpro_release(idpro);

    d->hident = ident;
    kcdb_identity_hold(ident);
}

/* Layout the ID Selector dialog.  This should be called whenever the
   primary identity changes. */
static void
nc_layout_idsel(khui_new_creds * nc)
{
    khui_cw_lock_nc(nc);
#ifdef DEBUG
    assert(nc->n_identities > 0);
#endif
    if (nc->n_identities > 0) {
        get_ident_display_data(&nc->idsel.id, nc->identities[0]);
    }
    khui_cw_unlock_nc(nc);

    InvalidateRect(nc->idsel.hwnd, NULL, TRUE);
}



static void
nc_idsel_set_status_string(khui_new_creds * nc,
                           const wchar_t * status)
{
    if (status) {
        StringCbCopy(nc->idsel.id.status, sizeof(nc->idsel.id.status), status);
    } else {
        nc->idsel.id.status[0] = L'\0';
    }

    InvalidateRect(nc->idsel.hwnd, NULL, TRUE);
}



static void
nc_idsel_draw_identity_item(khui_new_creds * nc, HDC hdc,
                            const RECT * pr, const SIZE * pmargin,
                            UINT state, khm_boolean erase,
                            khm_boolean skip_type,
                            const nc_ident_display * d)
{
    RECT r, tr;
    size_t cch;
    SIZE margin, s;
    HFONT hf_old = NULL;
    COLORREF cr_type, cr_idname, cr_status;

    r = *pr;
    margin = *pmargin;

    if ((state & (ODS_HOTLIGHT|ODS_SELECTED)) &&
        !(state & ODS_DISABLED)) {
        cr_type = cr_idname = cr_status = GetSysColor(COLOR_HIGHLIGHTTEXT);

        if (erase) {
            HBRUSH hbr;

            hbr = GetSysColorBrush(COLOR_HIGHLIGHT);
            FillRect(hdc, &r, hbr);
        }
    } else {
        if (erase) {
            HBRUSH hbr;

            hbr = GetSysColorBrush(COLOR_MENU);
            FillRect(hdc, &r, hbr);
        }

        if (state & ODS_DISABLED) {
            cr_type = cr_idname = cr_status = GetSysColor(COLOR_GRAYTEXT);
        } else {
            khm_int32 idflags;

            kcdb_identity_get_flags(d->hident, &idflags);

            if ((idflags & KCDB_IDENT_FLAG_INVALID) ||
                (idflags & KCDB_IDENT_FLAG_UNKNOWN)) {

                cr_idname = nc->idsel.cr_idname_dis;
                cr_status = nc->idsel.cr_status_err;
            } else {
                cr_idname = nc->idsel.cr_idname;
                cr_status = nc->idsel.cr_status;
            }
            cr_type = nc->idsel.cr_type;
        }
    }

    /* Identity icon */
    if (d->icon) {
        DrawIconEx(hdc, r.left, r.top, d->icon, 0, 0, 0,
                   NULL, DI_DEFAULTSIZE | DI_NORMAL);
    }

    r.left += GetSystemMetrics(SM_CXICON) + margin.cx;

    SetBkMode(hdc, TRANSPARENT);

    CopyRect(&tr, &r);
    tr.bottom = (r.bottom + r.top) / 2;

    /* Identity Type String */
    if (d->type_string[0] && !skip_type) {
#ifdef DEBUG
        assert(nc->idsel.hf_type);
#endif
        if (nc->idsel.hf_type) {
            if (hf_old == NULL)
                hf_old = SelectFont(hdc, nc->idsel.hf_type);
            else
                SelectFont(hdc, nc->idsel.hf_type);
        }

        StringCchLength(d->type_string, KCDB_MAXCCH_SHORT_DESC, &cch);
        SetTextColor(hdc, cr_type);
        DrawText(hdc, d->type_string, (int) cch,
                 &tr, DT_SINGLELINE | DT_RIGHT | DT_VCENTER);

        GetTextExtentPoint32(hdc, d->type_string, (int) cch, &s);

        tr.right -= s.cx + margin.cx;
    }

    /* Display String */
    if (d->display_string[0]) {
#ifdef DEBUG
        assert(nc->idsel.hf_idname);
#endif
        if (nc->idsel.hf_idname) {
            if (hf_old == NULL)
                hf_old = SelectFont(hdc, nc->idsel.hf_idname);
            else
                SelectFont(hdc, nc->idsel.hf_idname);
        }

        StringCchLength(d->display_string, KCDB_IDENT_MAXCCH_NAME, &cch);
        SetTextColor(hdc, cr_idname);
        DrawText(hdc, d->display_string, (int) cch,
                 &tr, DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS);
    }

    /* Status String */
    if (d->status[0]) {
#ifdef DEBUG
        assert(nc->idsel.hf_status);
#endif
        if (nc->idsel.hf_status) {
            if (hf_old == NULL)
                hf_old = SelectFont(hdc, nc->idsel.hf_status);
            else
                SelectFont(hdc, nc->idsel.hf_status);
        }

        CopyRect(&tr, &r);
        tr.top += (r.bottom - r.top) / 2;

        StringCchLength(d->status, KCDB_MAXCCH_SHORT_DESC, &cch);
        SetTextColor(hdc, cr_status);
        DrawText(hdc, d->status, (int) cch,
                 &tr, DT_SINGLELINE | DT_LEFT | DT_WORD_ELLIPSIS);
    }

    if (hf_old != NULL) {
        SelectFont(hdc, hf_old);
    }
}

static INT_PTR
nc_idsel_measureitem(HWND hwnd, LPMEASUREITEMSTRUCT lpm)
{
    HWND hwc;
    RECT r;

    hwc = GetDlgItem(hwnd, IDC_IDSEL);
    assert(hwc != NULL);
    GetClientRect(hwc, &r);

    lpm->itemWidth = r.right - (r.bottom / 3);
    lpm->itemHeight = r.bottom - (r.bottom / 3);

    if (lpm->itemID == 1)
        lpm->itemHeight += lpm->itemHeight / 2;

    nc_set_dlg_retval(hwnd, TRUE);
    return TRUE;
}

static INT_PTR
nc_idsel_draw_menu_item(HWND hwnd, LPDRAWITEMSTRUCT lpd)
{
    khui_new_creds * nc;
    nc_ident_display * d;
    RECT r, tr;
    size_t cch;
    SIZE margin;
    HFONT hf_old = NULL;

    nc = nc_get_dlg_data(hwnd);
    d = (nc_ident_display *) lpd->itemData;
    assert(d != NULL);

    CopyRect(&r, &lpd->rcItem);

    if (lpd->itemID == 1) {
        HBRUSH hbr;

        margin.cx = margin.cy = (r.bottom - r.top) / 6;
        CopyRect(&tr, &r);
        r.top += margin.cy * 2;
        tr.bottom = r.top;

        hbr = GetSysColorBrush(COLOR_MENU);
        FillRect(lpd->hDC, &tr, hbr);

        if (nc->idsel.hf_status) {
            hf_old = SelectFont(lpd->hDC, nc->idsel.hf_status);
        }

        StringCchLength(d->type_string, KCDB_MAXCCH_SHORT_DESC, &cch);
        SetTextColor(lpd->hDC, GetSysColor(COLOR_MENUTEXT));
        DrawText(lpd->hDC, d->type_string, (int) cch,
                 &tr, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
    } else {
        margin.cx = margin.cy = (r.bottom - r.top) / 4;
    }

    nc_idsel_draw_identity_item(nc, lpd->hDC, &r, &margin,
                                lpd->itemState, TRUE,
                                (lpd->itemID == 1)?TRUE:FALSE, d);

    if (hf_old != NULL)
        SelectFont(lpd->hDC, hf_old);

    return TRUE;
}

static void
nc_show_identity_selector(HWND hwnd)
{
    khui_new_creds * nc;
    kcdb_enumeration e = NULL;
    khm_size n_ids, i;
    nc_ident_display * ids = NULL;
    khm_handle ident;
    khm_boolean seen_current = FALSE;
    HMENU hm = NULL;
    khm_size cb;

    nc = nc_get_dlg_data(hwnd);
#ifdef DEBUG
    assert(nc != NULL);
#endif

    if (KHM_FAILED(kcdb_identity_begin_enum(KCDB_IDENT_FLAG_CONFIG,
                                            KCDB_IDENT_FLAG_CONFIG,
                                            &e, &n_ids))) {
        n_ids = 0;
#ifdef DEBUG
        assert(e == NULL);
#endif
        e = NULL;
    }

    n_ids ++;

    ids = PMALLOC(n_ids * sizeof(ids[0]));
    if (ids == NULL) {
#ifdef DEBUG
        assert(FALSE);
#endif
        goto _cleanup;
    }
    ZeroMemory(ids, n_ids * sizeof(ids[0]));

    i = 0;

    cb = sizeof(ids[i].icon);
    if (KHM_FAILED(khui_cache_get_resource(khm_hInstance, IDI_ID_NEW, KHM_RESTYPE_ICON,
                                           &ids[i].icon, &cb))) {
        ids[i].icon = (HICON) LoadImage(khm_hInstance, MAKEINTRESOURCE(IDI_ID_NEW),
                                        IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_DEFAULTSIZE);
#ifdef DEBUG
        assert(ids[i].icon);
#endif
        if (ids[i].icon) {
            khui_cache_add_resource(khm_hInstance, IDI_ID_NEW, KHM_RESTYPE_ICON,
                                    &ids[i].icon, sizeof(ids[i].icon));
        }
    }

    LoadString(khm_hInstance, IDS_NC_NEW_IDENT, ids[i].display_string,
               ARRAYLENGTH(ids[i].display_string));
    LoadString(khm_hInstance, IDS_NC_NEW_IDENT_D, ids[i].status,
               ARRAYLENGTH(ids[i].status));
    LoadString(khm_hInstance, IDS_NC_IDSEL_PROMPT, ids[i].type_string,
               ARRAYLENGTH(ids[i].type_string));
    i++;

    ident = NULL;
    while (e && KHM_SUCCEEDED(kcdb_enum_next(e, &ident))) {
#ifdef DEBUG
        assert(i < n_ids);
#endif

        get_ident_display_data(&ids[i], ident);

        if (nc->n_identities > 0 &&
            kcdb_identity_is_equal(ident, nc->identities[0]))
            seen_current = TRUE;

        i++;
    }

    if (!seen_current && nc->n_identities > 0) {
        /* The current identity wasn't seen in the enumeration.  We
           must add it manually. */
        i = n_ids++;

        ids = PREALLOC(ids, n_ids * sizeof(ids[0]));
        if (ids == NULL) {
#ifdef DEBUG
            assert(FALSE);
#endif
            goto _cleanup;
        }

        get_ident_display_data(&ids[i], nc->identities[0]);
    }

    hm = CreatePopupMenu();

    for (i=0; i < n_ids; i++) {
        UINT flags;

        if (nc->n_identities > 0 &&
            kcdb_identity_is_equal(ids[i].hident, nc->identities[0]))
            flags = MF_DISABLED;
        else
            flags = 0;

        AppendMenu(hm, (flags | MF_OWNERDRAW), (UINT_PTR) i+1, (LPCTSTR) &ids[i]);
    }

    {
        TPMPARAMS tp;

        ZeroMemory(&tp, sizeof(tp));
        tp.cbSize = sizeof(tp);
        GetWindowRect(hwnd, &tp.rcExclude);

        i = TrackPopupMenuEx(hm,
                             TPM_LEFTALIGN | TPM_RETURNCMD | TPM_LEFTBUTTON |
                             TPM_VERPOSANIMATION,
                             tp.rcExclude.left, tp.rcExclude.bottom,
                             hwnd, &tp);
    }

    if (i == 0)
        /* The user cancelled the menu or otherwise didn't make any
           selection */
        goto _cleanup;
    else if (i >= 2 && i <= n_ids + 1) {
        i--;
        khui_cw_set_primary_id(nc, ids[i].hident);
        //nc_notify_new_identity(nc, TRUE);
    } else if (i == 1) {
        /* Navigate to the identity specification window if the user
           wants to specify a new identity. */
        nc_navigate(nc, NC_PAGE_IDSPEC);
    }

 _cleanup:
    if (ids != NULL) {
        for (i=0; i < n_ids; i++) {
            if (ids[i].hident) {
                kcdb_identity_release(ids[i].hident);
                ids[i].hident = NULL;
            }
        }
        PFREE(ids);
        ids = NULL;
    }

    if (e)
        kcdb_enum_end(e);
}

static INT_PTR
nc_idsel_handle_wm_notify_for_idsel(HWND hwnd, khui_new_creds * nc, LPNMHDR pnmh)
{
    LPNMCUSTOMDRAW pcd;

    switch (pnmh->code) {
    case NM_CUSTOMDRAW:

        /* We are drawing the identity selector button.  The button
           face contains an icon representing the selected primary
           identity, the display stirng for the primary identity, a
           status string and a chevron. */

        pcd = (LPNMCUSTOMDRAW) pnmh;
        switch (pcd->dwDrawStage) {
        case CDDS_PREERASE:
        case CDDS_PREPAINT:
            nc_set_dlg_retval(hwnd, (LONG)(CDRF_NOTIFYITEMDRAW |
                                           CDRF_NOTIFYPOSTPAINT |
                                           CDRF_NOTIFYPOSTERASE));
            return TRUE;

        case CDDS_POSTERASE:
            nc_set_dlg_retval(hwnd, 0);
            return TRUE;

        case CDDS_POSTPAINT:
            {
                RECT r;
                SIZE margin;

                GetClientRect(hwnd, &r);
                margin.cx = margin.cy = r.bottom / 6;
                SetRect(&r, margin.cx, margin.cy, r.right - margin.cx, r.bottom - margin.cy);

                if (pcd->uItemState & CDIS_SELECTED) {
                    OffsetRect(&r, margin.cx / 2, margin.cy / 2);
                }

                nc_idsel_draw_identity_item(nc, pcd->hdc, &r, &margin,
                                            0, FALSE, FALSE, &nc->idsel.id);

                /* TODO: Also draw a chevron */

                nc_set_dlg_retval(hwnd, 0);
            }
            return TRUE;
        } /* switch drawStage */

        return FALSE;
    }

    return FALSE;
}

/* Dialog procedure */
static INT_PTR CALLBACK
nc_idsel_dlg_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    khui_new_creds * nc;

    switch (uMsg) {
    case WM_INITDIALOG:
        nc = (khui_new_creds *) lParam;
        nc_set_dlg_data(hwnd, nc);
        nc->idsel.hf_type = khm_get_element_font(KHM_FONT_AUX);
        nc->idsel.hf_idname = khm_get_element_font(KHM_FONT_TITLE);
        nc->idsel.hf_status = khm_get_element_font(KHM_FONT_NORMAL);
        nc->idsel.cr_idname = khm_get_element_color(KHM_CLR_TEXT_HEADER);
        nc->idsel.cr_idname_dis = khm_get_element_color(KHM_CLR_TEXT_HEADER_DIS);
        nc->idsel.cr_type = khm_get_element_color(KHM_CLR_ACCENT);
        nc->idsel.cr_status = khm_get_element_color(KHM_CLR_TEXT);
        nc->idsel.cr_status_err = khm_get_element_color(KHM_CLR_TEXT_ERR);
        return TRUE;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh;

            pnmh = (LPNMHDR) lParam;
            nc = nc_get_dlg_data(hwnd);

            if (pnmh->idFrom == IDC_IDSEL) {
                return nc_idsel_handle_wm_notify_for_idsel(hwnd, nc, pnmh);
            }
        }
        break;

    case WM_COMMAND:
        if (MAKEWPARAM(IDC_IDSEL, BN_CLICKED) == wParam) {
            nc_show_identity_selector(hwnd);
            return TRUE;
        }
        break;

    case WM_MEASUREITEM:
        return nc_idsel_measureitem(hwnd, (LPMEASUREITEMSTRUCT) lParam);

    case WM_DRAWITEM:
        return nc_idsel_draw_menu_item(hwnd, (LPDRAWITEMSTRUCT) lParam);

    case WM_DESTROY:
        break;
    }

    return FALSE;
}


/************************************************************
 * Identity Specification Window
 ************************************************************/

static void
nc_layout_idspec(khui_new_creds * nc)
{
    HWND hw_list;
    LVITEM lvi;
    khm_ssize idx;

    hw_list = GetDlgItem(nc->idspec.hwnd, IDC_IDPROVLIST);

    nc->idspec.in_layout = TRUE;

    if (nc->n_providers == 0) {
        kcdb_enumeration e = NULL;
        khm_handle idpro = NULL;

        if (KHM_SUCCEEDED(kcdb_identpro_begin_enum(&e, NULL))) {
            khui_cw_lock_nc(nc);

            while (KHM_SUCCEEDED(kcdb_enum_next(e, &idpro))) {
                khui_cw_add_provider(nc, idpro);
            }

            khui_cw_unlock_nc(nc);
        }

        if (e)
            kcdb_enum_end(e);
    }

#ifdef DEBUG
    assert(nc->n_providers > 0);
#endif

    /* If the identity provider list hasn't been initialized, we
       should do so now. */
    if (!nc->idspec.initialized) {
        khm_size i;
        HIMAGELIST ilist = NULL;
        HIMAGELIST ilist_old = NULL;

        ListView_DeleteAllItems(hw_list);
        ilist = ImageList_Create(GetSystemMetrics(SM_CXICON),
                                 GetSystemMetrics(SM_CYICON),
                                 ILC_COLORDDB|ILC_MASK,
                                 4, 4);
        ilist_old = ListView_SetImageList(hw_list, ilist, LVSIL_NORMAL);

        if (ilist_old) {
            ImageList_Destroy(ilist_old);
            ilist_old = NULL;
        }

        ZeroMemory(&lvi, sizeof(lvi));

        for (i=0; i < nc->n_providers; i++) {
            HICON icon;
            wchar_t caption[KHUI_MAXCCH_SHORT_DESC];
            khm_size cb;

            lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;

            cb = sizeof(icon);
            if (KHM_FAILED(kcdb_get_resource(nc->providers[i].h,
                                             KCDB_RES_ICON_NORMAL,
                                             0, NULL, NULL, &icon, &cb)))
                lvi.mask &= ~LVIF_IMAGE;
            else {
                lvi.iImage = ImageList_AddIcon(ilist, icon);
            }

            cb = sizeof(caption);
            if (KHM_FAILED(kcdb_get_resource(nc->providers[i].h,
                                             KCDB_RES_DISPLAYNAME,
                                             KCDB_RFS_SHORT, NULL, NULL, caption, &cb)))
                caption[0] = L'\0';
            lvi.pszText = caption;

            lvi.lParam = i;

            lvi.iItem = (int) i;
            lvi.iSubItem = 0;

            ListView_InsertItem(hw_list, &lvi);
        }

        /* And select the first item */

        lvi.mask = LVIF_STATE;
        lvi.stateMask = LVIS_SELECTED;
        lvi.state = LVIS_SELECTED;
        lvi.iItem = 0;
        lvi.iSubItem = 0;

        ListView_SetItem(hw_list, &lvi);

        nc->idspec.idx_current = -1;
        nc->idspec.initialized = TRUE;
    }

    if (ListView_GetSelectedCount(hw_list) != 1 ||
        (idx = ListView_GetNextItem(hw_list, -1, LVNI_SELECTED)) == -1) {

        idx = -1;

        /* The number of selected items is not 1.  We have to pretend
           that there are no providers selected. */

    } else {

        lvi.mask = LVIF_PARAM;
        lvi.iItem = (int) idx;
        lvi.iSubItem = 0;

        ListView_GetItem(hw_list, &lvi);
        idx = (int) lvi.lParam;

    }

    if (idx != nc->idspec.idx_current) {

        if (nc->idspec.idx_current >= 0 &&
            nc->idspec.idx_current < (khm_ssize) nc->n_providers) {
            khm_ssize current;

            current = nc->idspec.idx_current;

            if (nc->providers[current].hwnd_panel != NULL) {
                SetWindowPos(nc->providers[current].hwnd_panel, NULL,
                             0, 0, 0, 0, SWP_HIDEONLY);
            }
        } else {
            SetWindowPos(GetDlgItem(nc->idspec.hwnd, IDC_NC_R_IDSPEC), NULL,
                         0, 0, 0, 0, SWP_HIDEONLY);
        }

        if (idx >= 0 && idx < (khm_ssize) nc->n_providers) {
            HWND hw_r;
            RECT r;

            if (nc->providers[idx].hwnd_panel == NULL) {

                if (nc->providers[idx].cb == NULL) {
                    kcdb_identpro_get_idsel_factory(nc->providers[idx].h,
                                                    &nc->providers[idx].cb);
#ifdef DEBUG
                    assert(nc->providers[idx].cb != NULL);
#endif
                }

                if (nc->providers[idx].cb != NULL) {

                    nc->idspec.idx_current = idx;
                    (*nc->providers[idx].cb)(nc->idspec.hwnd,
                                             &nc->providers[idx].hwnd_panel);
#ifdef DEBUG
                    assert(nc->providers[idx].hwnd_panel);
#endif
                }
            }

            hw_r = GetDlgItem(nc->idspec.hwnd, IDC_NC_R_IDSPEC);
            GetWindowRect(hw_r, &r);
            MapWindowPoints(NULL, nc->idspec.hwnd, (LPPOINT) &r, sizeof(r)/sizeof(POINT));

            SetWindowPos(nc->providers[idx].hwnd_panel, hw_r,
                         r.left, r.top, r.right - r.left, r.bottom - r.top,
                         SWP_MOVESIZEZ);
        } else {
            /* Show the placeholder window */
            SetWindowPos(GetDlgItem(nc->idspec.hwnd, IDC_NC_R_IDSPEC), NULL,
                         0, 0, 0, 0, SWP_SHOWONLY);
        }

        nc->idspec.idx_current = idx;
    }

    nc->idspec.in_layout = FALSE;
}

static khm_boolean
nc_idspec_process(khui_new_creds * nc)
{
    khui_new_creds_idpro * p;

    if (nc->idspec.idx_current >= 0 &&
        nc->idspec.idx_current < (khm_ssize) nc->n_providers) {
        khm_handle ident = NULL;

        p = &nc->providers[nc->idspec.idx_current];

#ifdef DEBUG
        assert(p->hwnd_panel);
#endif

        SendMessage(p->hwnd_panel, KHUI_WM_NC_NOTIFY, MAKEWPARAM(0, WMNC_IDSEL_GET_IDENT),
                    (LPARAM) &ident);

        if (ident) {
            khui_cw_set_primary_id(nc, ident);
            kcdb_identity_release(ident);

            return TRUE;
        }
    }

    return FALSE;
}

/* Identity specification.  IDD_NC_IDSPEC */
static INT_PTR CALLBACK
nc_idspec_dlg_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    khui_new_creds * nc;

    switch (uMsg) {
    case WM_INITDIALOG:
        nc = (khui_new_creds *) lParam;
        nc_set_dlg_data(hwnd, nc);
        return TRUE;

    case WM_NOTIFY:
        {
            NMHDR * nmh;

            nc = nc_get_dlg_data(hwnd);
            nmh = (NMHDR *) lParam;

            if (nmh->code == LVN_ITEMCHANGED && !nc->idspec.in_layout) {
                NMLISTVIEW * nmlv = (NMLISTVIEW *) nmh;

                if ((nmlv->uNewState ^ nmlv->uOldState) & LVIS_SELECTED)
                    nc_layout_idspec(nc);
                return TRUE;
            }
        }
        return FALSE;

    default:
        return FALSE;
    }
}



/************************************************************
 * Navigation Pane
 ************************************************************/

static void
nc_layout_nav(khui_new_creds * nc)
{
    HDWP dwp;
    dwp = BeginDeferWindowPos(6);

    dwp = DeferWindowPos(dwp, GetDlgItem(nc->nav.hwnd, IDC_BACK), NULL, 0, 0, 0, 0,
                         ((nc->nav.transitions & NC_TRANS_PREV) ?
                          SWP_SHOWONLY : SWP_HIDEONLY));
    dwp = DeferWindowPos(dwp, GetDlgItem(nc->nav.hwnd, IDC_NEXT), NULL, 0, 0, 0, 0,
                         ((nc->nav.transitions & NC_TRANS_NEXT) ?
                          SWP_SHOWONLY : SWP_HIDEONLY));
    dwp = DeferWindowPos(dwp, GetDlgItem(nc->nav.hwnd, IDC_FINISH), NULL, 0, 0, 0, 0,
                         ((nc->nav.transitions & NC_TRANS_FINISH) ?
                          SWP_SHOWONLY : SWP_HIDEONLY));
    dwp = DeferWindowPos(dwp, GetDlgItem(nc->nav.hwnd, IDC_NC_ABORT), NULL, 0, 0, 0, 0,
                         ((nc->nav.transitions & NC_TRANS_ABORT) ?
                          SWP_SHOWONLY : SWP_HIDEONLY));
    dwp = DeferWindowPos(dwp, GetDlgItem(nc->nav.hwnd, IDCANCEL), NULL, 0, 0, 0, 0,
                         (!((nc->nav.transitions & NC_TRANS_ABORT)) ?
                          SWP_SHOWONLY : SWP_HIDEONLY));
    dwp = DeferWindowPos(dwp, GetDlgItem(nc->nav.hwnd, IDC_NC_DETAILS), NULL, 0, 0, 0, 0,
                         ((nc->nav.transitions & NC_TRANS_SHOWDETAILS) ?
                          SWP_SHOWONLY : SWP_HIDEONLY));

    EndDeferWindowPos(dwp);
}




/************************************************************
 * Privileged Interaction Dialog
 ************************************************************/

/* Layout the privileged interaction panel in either the basic or the
   advanced mode. */
static void
nc_layout_privint(khui_new_creds * nc)
{
    HWND hw;
    HWND hw_r_p = NULL;
    HWND hw_target = NULL;
    HWND hw_tab = NULL;
    RECT r, r_p;
    khm_int32 idf = 0;
    khm_boolean adv;

    adv = (nc->page == NC_PAGE_CREDOPT_ADV);

    if (adv) {
        hw = nc->privint.hwnd_advanced;
        hw_tab = GetDlgItem(hw, IDC_NC_TAB);
    } else {
        hw = nc->privint.hwnd_basic;
    }

    /* Everytime there's a change in the order of the credentials
       providers (i.e. because the primary identity changed), the
       nc->privint.initialized flag is reset.  In which case we will
       re-initialize the tab control housing the panels. */

    if (adv && !nc->privint.initialized) {
        wchar_t desc[KCDB_MAXCCH_SHORT_DESC];
        TCITEM tci;
        khm_size i;
        khui_new_creds_privint_panel * p;

        ZeroMemory(&tci, sizeof(tci));
        assert(hw_tab != NULL);

        TabCtrl_DeleteAllItems(hw_tab);

        khui_cw_lock_nc(nc);

        if (nc->privint.shown.show_blank)
            p = NULL;
        else
            p = QBOTTOM(&nc->privint.shown);

        if (p != NULL) {
            tci.mask = TCIF_PARAM | TCIF_TEXT;
            tci.pszText = p->caption;
            tci.lParam = NC_PRIVINT_PANEL;

            if (p->caption[0] == L'\0') {
                LoadString(khm_hInstance, IDS_NC_IDENTITY,
                           p->caption, ARRAYLENGTH(p->caption));
            }

            TabCtrl_InsertItem(hw_tab, 0, &tci);
        }

        for (i=0; i < nc->n_types; i++) {
            /* All the enabled types are at the front of the list */
            if (nc->types[i].nct->flags & KHUI_NCT_FLAG_DISABLED)
                break;

            tci.mask = TCIF_PARAM | TCIF_TEXT;
            if (nc->types[i].nct->name) {
                tci.pszText = nc->types[i].nct->name;
            } else {
                khm_size cb;

                desc[0] = L'\0';
                cb = sizeof(desc);
                kcdb_get_resource((khm_handle)(khm_ssize) nc->types[i].nct->type,
                                  KCDB_RES_DESCRIPTION, KCDB_RFS_SHORT,
                                  NULL, NULL, desc, &cb);
                tci.pszText = desc;
            }
            tci.lParam = i;

            TabCtrl_InsertItem(hw_tab, i + 1, &tci);
        }

        nc->privint.initialized = TRUE;
        khui_cw_unlock_nc(nc);

        TabCtrl_SetCurSel(hw_tab, 0);
    }

    if (adv) {
        TCITEM tci;
        int ctab;
        int panel_idx;

        ctab = TabCtrl_GetCurSel(hw_tab);

        ZeroMemory(&tci, sizeof(tci));

        tci.mask = TCIF_PARAM;
        TabCtrl_GetItem(hw_tab, ctab, &tci);

        panel_idx = (int) tci.lParam;

        khui_cw_lock_nc(nc);

        if (panel_idx == NC_PRIVINT_PANEL) {
            /* We have to show the privileged interaction panel */
            if (nc->privint.shown.show_blank || QBOTTOM(&nc->privint.shown) == NULL) {
                hw_target = NULL;
            } else {
                khui_new_creds_privint_panel * p;

                p = QBOTTOM(&nc->privint.shown);

                if (p->hwnd == NULL && p->use_custom) {
                    nc_create_custom_prompter_dialog(nc, nc->privint.hwnd_advanced, p);
                }

#ifdef DEBUG
                assert(p->hwnd != NULL);
#endif

                hw_target = p->hwnd;
            }
        } else {
            /* We have to show the credentials options panel for some
               credentials type */
            assert(panel_idx >= 0 && (khm_size) panel_idx < nc->n_types);
            hw_target = nc->types[panel_idx].nct->hwnd_panel;
        }

        khui_cw_unlock_nc(nc);

        if (hw_target != nc->privint.hwnd_current && nc->privint.hwnd_current != NULL) {
            SetWindowPos(nc->privint.hwnd_current, NULL, 0, 0, 0, 0, SWP_HIDEONLY);
        }

        nc->privint.hwnd_current = hw_target;
        nc->privint.idx_current = panel_idx;

    } else {
        if (nc->privint.shown.show_blank || QBOTTOM(&nc->privint.shown) == NULL) {
            hw_target = NULL;
        } else {
            khui_new_creds_privint_panel * p;

            p = QBOTTOM(&nc->privint.shown);

            if (p->hwnd == NULL && p->use_custom) {
                nc_create_custom_prompter_dialog(nc, nc->privint.hwnd_basic, p);
            }

            hw_target = p->hwnd;

            if (hw_target != nc->privint.hwnd_current && nc->privint.hwnd_current != NULL) {
                SetWindowPos(nc->privint.hwnd_current, NULL, 0, 0, 0, 0, SWP_HIDEONLY);
            }
        }

        nc->privint.hwnd_current = hw_target;
        nc->privint.idx_current = NC_PRIVINT_PANEL;
    }

    if (hw_target == NULL) {
        hw_target = nc->privint.hwnd_noprompts;
    }

    hw_r_p = GetDlgItem(hw, IDC_NC_R_PROMPTS);
    assert(hw_r_p != NULL);

    if (adv) {
        assert(hw_tab != NULL);

        GetWindowRect(hw, &r);
        GetWindowRect(hw_tab, &r_p);
        OffsetRect(&r_p, -r.left, -r.top);

        TabCtrl_AdjustRect(hw_tab, FALSE, &r_p);
    } else {
        GetWindowRect(hw, &r);
        GetWindowRect(hw_r_p, &r_p);
        OffsetRect(&r_p, -r.left, -r.top);
    }

    /* One thing to be careful about when dealing with third-party
       plug-ins: If the target dialog has the WS_POPUP style set or
       doesn't have WS_CHILD style set, we may be in for a world of
       hurt.  So we play it safe and set the style bits properly
       beforehand. */
    {
        LONG ws;

        ws = GetWindowLong(hw_target, GWL_STYLE);
        if ((ws & (WS_CHILD|WS_POPUP)) != WS_CHILD) {
#ifdef DEBUG
            assert(FALSE);
#endif
            ws &= ~WS_POPUP;
            ws |= WS_CHILD;
            SetWindowLong(hw_target, GWL_STYLE, ws);
        }
    }

    SetParent(hw_target, hw);

    if (hw_target != nc->privint.hwnd_noprompts) {
        SetWindowPos(nc->privint.hwnd_noprompts, NULL, 0, 0, 0, 0, SWP_HIDEONLY);
    }

    SetWindowPos(hw_target, hw_r_p, rect_coords(r_p), SWP_MOVESIZEZ);

    khui_cw_lock_nc(nc);
    assert(nc->n_identities > 0);

    if (nc->n_identities > 0) {
        kcdb_identity_get_flags(nc->identities[0], &idf);
    }
    khui_cw_unlock_nc(nc);

    CheckDlgButton(hw, IDC_NC_MAKEDEFAULT,
                   ((idf & KCDB_IDENT_FLAG_DEFAULT)? BST_CHECKED : BST_UNCHECKED));
    EnableWindow(GetDlgItem(hw, IDC_NC_MAKEDEFAULT),
                 !(idf & KCDB_IDENT_FLAG_DEFAULT));
}


/* Privileged Interaction (Basic). IDD_NC_PRIVINT_BASIC */
static INT_PTR CALLBACK
nc_privint_basic_dlg_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    khui_new_creds * nc;

    switch (uMsg) {
    case WM_INITDIALOG:
        nc = (khui_new_creds *) lParam;
        nc_set_dlg_data(hwnd, nc);
        return TRUE;

    case WM_COMMAND:
        nc = nc_get_dlg_data(hwnd);
        if (wParam == MAKEWPARAM(IDC_NC_ADVANCED, BN_CLICKED)) {
            nc_navigate(nc, NC_PAGE_CREDOPT_ADV);
        }
        return FALSE;

    default:
        return FALSE;
    }
}

/* Privileged Interaction (Advanced). IDD_NC_PRIVINT_ADVANCED */
static INT_PTR CALLBACK
nc_privint_advanced_dlg_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    khui_new_creds * nc;

    switch (uMsg) {
    case WM_INITDIALOG:
        nc = (khui_new_creds *) lParam;
        nc_set_dlg_data(hwnd, nc);
        return TRUE;

    case WM_NOTIFY:
        {
            LPNMHDR nmhdr;

            nc = nc_get_dlg_data(hwnd);
            if (nc == NULL)
                return FALSE;

            nmhdr = (LPNMHDR) lParam;

            if (nmhdr->code == TCN_SELCHANGE) {
                nc_layout_privint(nc);
                return TRUE;
            }
        }
        return FALSE;

    case WM_COMMAND:
        nc = nc_get_dlg_data(hwnd);
        if (wParam == MAKEWPARAM(IDC_NC_ADVANCED, BN_CLICKED)) {
            nc_navigate(nc, NC_PAGE_CREDOPT_BASIC);
        }
        return FALSE;

    default:
        return FALSE;
    }
}

#define MARQUEE_TIMEOUT 100

static void
nc_privint_set_progress(khui_new_creds * nc, int progress, BOOL show)
{

#if _WIN32_WINNT >= 0x0501
    /* The marquee mode of the progress bar control is only supported
       on Windows XP or later.  If you are running an older OS, you
       get to stare at a progress bar that isn't moving.  */

    if (show && progress == KHUI_CWNIS_MARQUEE) {
        /* enabling marquee */
        if (!(nc->privint.noprompt_flags & NC_NPF_MARQUEE)) {
            HWND hw;

            hw = GetDlgItem(nc->privint.hwnd_noprompts, IDC_PROGRESS);
            SetWindowLong(hw, GWL_STYLE, WS_CHILD|PBS_MARQUEE);
            SendMessage(hw, PBM_SETMARQUEE, TRUE, MARQUEE_TIMEOUT);
            nc->privint.noprompt_flags |= NC_NPF_MARQUEE;
        }
    } else {
        /* disabling marquee */
        if (nc->privint.noprompt_flags & NC_NPF_MARQUEE) {
            SendDlgItemMessage(nc->privint.hwnd_noprompts, IDC_PROGRESS,
                               PBM_SETMARQUEE, FALSE, MARQUEE_TIMEOUT);
            nc->privint.noprompt_flags &= ~NC_NPF_MARQUEE;
        }
    }
#endif

    if (show && progress >= 0 && progress <= 100) {
        SendDlgItemMessage(nc->privint.hwnd_noprompts, IDC_PROGRESS,
                           PBM_SETPOS, progress, 0);
    }

    ShowWindow(GetDlgItem(nc->privint.hwnd_noprompts, IDC_PROGRESS), (show?SW_SHOW:SW_HIDE));
}

static void
nc_privint_update_identity_state(khui_new_creds * nc,
                                 const nc_identity_state_notification * notif)
{
    khm_int32 idflags;
    wchar_t buf[80];
    khm_int32 nflags;

    nflags = notif->flags;

    if (nc->n_identities == 0) {
        /* no identities */

#ifdef DEBUG
        assert(FALSE);
#endif
        LoadString(khm_hInstance, IDS_NC_NPR_CHOOSE, buf, ARRAYLENGTH(buf));
        SetDlgItemText(nc->privint.hwnd_noprompts, IDC_TEXT, buf);
        SetDlgItemText(nc->privint.hwnd_noprompts, IDC_TEXT2, L"");

        nflags &= ~KHUI_CWNIS_READY;

        nc_privint_set_progress(nc, 0, FALSE);

    } else if (nflags & KHUI_CWNIS_VALIDATED) {

        kcdb_identity_get_flags(nc->identities[0], &idflags);

        if (idflags & KCDB_IDENT_FLAG_VALID) {

            LoadString(khm_hInstance, IDS_NC_NPR_CLICKFINISH, buf, ARRAYLENGTH(buf));
            SetDlgItemText(nc->privint.hwnd_noprompts, IDC_TEXT, buf);
            SetDlgItemText(nc->privint.hwnd_noprompts, IDC_TEXT2, L"");
            if (notif->state_string)
                nc_idsel_set_status_string(nc, notif->state_string);
            else
                nc_idsel_set_status_string(nc, L"");

        } else {
            /* The identity may have KCDB_IDENT_FLAG_INVALID or
               KCDB_IDENT_FLAG_UNKNOWN set. */

            if ((idflags & KCDB_IDENT_FLAG_INVALID) == KCDB_IDENT_FLAG_INVALID) {
                LoadString(khm_hInstance, IDS_NC_INVALIDID, buf, ARRAYLENGTH(buf));
            } else {
                LoadString(khm_hInstance, IDS_NC_UNKNOWNID, buf, ARRAYLENGTH(buf));
            }
            nc_idsel_set_status_string(nc, buf);

            if (notif->state_string)
                SetDlgItemText(nc->privint.hwnd_noprompts, IDC_TEXT, notif->state_string);
            else
                SetDlgItemText(nc->privint.hwnd_noprompts, IDC_TEXT, L"");
            SetDlgItemText(nc->privint.hwnd_noprompts, IDC_TEXT2, L"");

            nflags &= ~KHUI_CWNIS_VALIDATED;
        }

        nc_privint_set_progress(nc, 0, FALSE);
    } else {
        LoadString(khm_hInstance, IDS_NC_NPR_VALIDATING, buf, ARRAYLENGTH(buf));
        SetDlgItemText(nc->privint.hwnd_noprompts, IDC_TEXT, buf);

        if (notif->state_string)
            SetDlgItemText(nc->privint.hwnd_noprompts, IDC_TEXT2, notif->state_string);
        else
            SetDlgItemText(nc->privint.hwnd_noprompts, IDC_TEXT2, L"");

        if (nflags & KHUI_CWNIS_NOPROGRESS) {
            nc_privint_set_progress(nc, 0, FALSE);
        } else {
            nc_privint_set_progress(nc, notif->progress, TRUE);
        }
    }

    if ((nflags & KHUI_CWNIS_READY) && !(nc->nav.transitions & NC_TRANS_FINISH)) {
        nc->nav.transitions |= NC_TRANS_FINISH;
        nc_layout_nav(nc);
    } else if (!(nflags & KHUI_CWNIS_READY) && (nc->nav.transitions & NC_TRANS_FINISH)) {
        nc->nav.transitions &= ~NC_TRANS_FINISH;
        nc_layout_nav(nc);
    }
}


/************************************************************
 * Generic Window Layout Functions
 ************************************************************/

static int __cdecl
nc_tab_sort_func(const void * v1, const void * v2)
{
    /* v1 and v2 and of type : khui_new_creds_by_type ** */
    khui_new_creds_type_int *t1, *t2;

    t1 = ((khui_new_creds_type_int *) v1);
    t2 = ((khui_new_creds_type_int *) v2);

#ifdef DEBUG
    assert(t1 != NULL && t2 != NULL && t1 != t2);
#endif

    /* Identity credentials type always sorts first */
    if (t1->is_id_credtype)
        return -1;
    if (t2->is_id_credtype)
        return 1;

    /* Enabled types sort first */
    if (!(t1->nct->flags & KHUI_NCT_FLAG_DISABLED) &&
        (t2->nct->flags & KHUI_NCT_FLAG_DISABLED))
        return -1;
    if ((t1->nct->flags & KHUI_NCT_FLAG_DISABLED) &&
        !(t2->nct->flags & KHUI_NCT_FLAG_DISABLED))
        return 1;

    /* Then sort by name, if both types have names */
    if (t1->display_name != NULL && t2->display_name != NULL)
        return _wcsicmp(t1->display_name, t2->display_name);
    if (t1->display_name)
        return -1;
    if (t2->display_name)
        return 1;

    return 0;
}

/* Called with nc locked

   Prepares the credentials type list in a new credentials object
   after an identity has been selected and the types have been
   notified.

   The list is sorted with the identity credential type at the
   beginning and the ordinals are updated to reflect the actual order
   of the types.
*/
static void
nc_prep_cred_types(khui_new_creds * nc)
{
    khm_size i;
    khm_handle idpro = NULL;
    khm_int32 ctype = KCDB_CREDTYPE_INVALID;

    /* if we have an identity, we should make sure that the identity
       credentials type is at the top of the list */
    if (nc->n_identities > 0 &&
        KHM_SUCCEEDED(kcdb_identity_get_identpro(nc->identities[0],
                                                 &idpro))) {
        kcdb_identpro_get_type(idpro, &ctype);
    }

    for (i=0; i < nc->n_types; i++) {
        nc->types[i].is_id_credtype = (ctype != KCDB_CREDTYPE_INVALID &&
                                       nc->types[i].nct->type == ctype);
    }

    qsort(nc->types, nc->n_types, sizeof(*(nc->types)), nc_tab_sort_func);

    for (i=0; i < nc->n_types; i++) {
        nc->types[i].nct->ordinal = i+1;
    }
}

static void
nc_layout_progress(khui_new_creds * nc)
{
}

static void
nc_size_container(khui_new_creds * nc)
{
    RECT r;
    DWORD style;
    DWORD exstyle;

    if (nc->mode == KHUI_NC_MODE_MINI) {
        int t;
        RECT r1, r2;

        GetWindowRect(GetDlgItem(nc->hwnd, IDC_NC_R_IDSEL), &r1);
        GetWindowRect(GetDlgItem(nc->hwnd, IDC_NC_R_MAIN_LG), &r2);
        t = r2.top;
        GetWindowRect(GetDlgItem(nc->hwnd, IDC_NC_R_NAV), &r2);
        OffsetRect(&r2, 0, t - r2.top);

        UnionRect(&r, &r1, &r2);
    } else {
        RECT r1, r2;

        GetWindowRect(GetDlgItem(nc->hwnd, IDC_NC_R_IDSEL), &r1);
        GetWindowRect(GetDlgItem(nc->hwnd, IDC_NC_R_NAV), &r2);

        UnionRect(&r, &r1, &r2);
    }

    style = GetWindowLong(nc->hwnd, GWL_STYLE);
    exstyle = GetWindowLong(nc->hwnd, GWL_EXSTYLE);

    AdjustWindowRectEx(&r, style, FALSE, exstyle);

    SetWindowPos(nc->hwnd, NULL, 0, 0,
                 r.right - r.left, r.bottom - r.top,
                 SWP_SIZEONLY);
}

/* Should be called on every page transition */
static void
nc_layout_container(khui_new_creds * nc)
{
    RECT r_idsel;
    RECT r_main;
    RECT r_main_lg;
    RECT r_nav_lg;
    RECT r_nav_sm;

    RECT r;
    HDWP dwp = NULL;
    BOOL drv;

#define dlg_item(id) GetDlgItem(nc->hwnd, id)

    GetWindowRect(dlg_item(IDC_NC_R_IDSEL), &r_idsel);
    MapWindowPoints(NULL, nc->hwnd, (LPPOINT) &r_idsel, 2);
    GetWindowRect(dlg_item(IDC_NC_R_MAIN), &r_main);
    MapWindowPoints(NULL, nc->hwnd, (LPPOINT) &r_main, 2);
    GetWindowRect(dlg_item(IDC_NC_R_MAIN_LG), &r_main_lg);
    MapWindowPoints(NULL, nc->hwnd, (LPPOINT) &r_main_lg, 2);
    GetWindowRect(dlg_item(IDC_NC_R_NAV), &r_nav_lg);
    MapWindowPoints(NULL, nc->hwnd, (LPPOINT) &r_nav_lg, 2);

    CopyRect(&r_nav_sm, &r_nav_lg);
    OffsetRect(&r_nav_sm, 0, r_main_lg.top - r_nav_sm.top);
    UnionRect(&r, &r_main, &r_main_lg);
    CopyRect(&r_main_lg, &r);

    switch (nc->page) {
    case NC_PAGE_IDSPEC:

        khui_cw_lock_nc(nc);

        dwp = BeginDeferWindowPos(7);

        UnionRect(&r, &r_idsel, &r_main);
        dwp = DeferWindowPos(dwp, nc->idspec.hwnd,
                             dlg_item(IDC_NC_R_IDSEL),
                             rect_coords(r), SWP_MOVESIZEZ);

        dwp = DeferWindowPos(dwp, nc->nav.hwnd,
                             dlg_item(IDC_NC_R_NAV),
                             rect_coords(r_nav_sm), SWP_MOVESIZEZ);

        dwp = DeferWindowPos(dwp, nc->idsel.hwnd, NULL, 0, 0, 0, 0,
                             SWP_HIDEONLY);

        dwp = DeferWindowPos(dwp, nc->privint.hwnd_basic, NULL, 0, 0, 0, 0,
                             SWP_HIDEONLY);

        dwp = DeferWindowPos(dwp, nc->privint.hwnd_advanced, NULL, 0, 0, 0, 0,
                             SWP_HIDEONLY);

        dwp = DeferWindowPos(dwp, nc->progress.hwnd, NULL, 0, 0, 0, 0,
                             SWP_HIDEONLY);

        drv = EndDeferWindowPos(dwp);

        nc->mode = KHUI_NC_MODE_MINI;
        nc_size_container(nc);

        khui_cw_unlock_nc(nc);

        nc_layout_idspec(nc);
        nc_layout_nav(nc);

        break;

    case NC_PAGE_CREDOPT_BASIC:

        khui_cw_lock_nc(nc);

        dwp = BeginDeferWindowPos(7);

        dwp = DeferWindowPos(dwp, nc->idsel.hwnd,
                             dlg_item(IDC_NC_R_IDSEL),
                             rect_coords(r_idsel), SWP_MOVESIZEZ);

        dwp = DeferWindowPos(dwp, nc->privint.hwnd_basic,
                             dlg_item(IDC_NC_R_MAIN),
                             rect_coords(r_main), SWP_MOVESIZEZ);

        dwp = DeferWindowPos(dwp, nc->nav.hwnd,
                             dlg_item(IDC_NC_R_NAV),
                             rect_coords(r_nav_sm), SWP_MOVESIZEZ);

        dwp = DeferWindowPos(dwp, nc->privint.hwnd_advanced, NULL, 0, 0, 0, 0,
                             SWP_HIDEONLY);

        dwp = DeferWindowPos(dwp, nc->idspec.hwnd, NULL, 0, 0, 0, 0,
                             SWP_HIDEONLY);

        dwp = DeferWindowPos(dwp, nc->progress.hwnd, NULL, 0, 0, 0, 0,
                             SWP_HIDEONLY);

        drv = EndDeferWindowPos(dwp);

        nc->mode = KHUI_NC_MODE_MINI;
        nc_size_container(nc);

        khui_cw_unlock_nc(nc);

        nc_layout_idsel(nc);
        nc_layout_privint(nc);
        nc_layout_nav(nc);

        break;

    case NC_PAGE_CREDOPT_ADV:

        khui_cw_lock_nc(nc);

        dwp = BeginDeferWindowPos(7);

        dwp = DeferWindowPos(dwp, nc->idsel.hwnd,
                             dlg_item(IDC_NC_R_IDSEL),
                             rect_coords(r_idsel), SWP_MOVESIZEZ);

        dwp = DeferWindowPos(dwp, nc->privint.hwnd_advanced,
                             dlg_item(IDC_NC_R_MAIN),
                             rect_coords(r_main_lg), SWP_MOVESIZEZ);

        dwp = DeferWindowPos(dwp, nc->nav.hwnd,
                             dlg_item(IDC_NC_R_NAV),
                             rect_coords(r_nav_lg), SWP_MOVESIZEZ);

        dwp = DeferWindowPos(dwp, nc->privint.hwnd_basic, NULL, 0, 0, 0, 0,
                             SWP_HIDEONLY);

        dwp = DeferWindowPos(dwp, nc->idspec.hwnd, NULL, 0, 0, 0, 0,
                             SWP_HIDEONLY);

        dwp = DeferWindowPos(dwp, nc->progress.hwnd, NULL, 0, 0, 0, 0,
                             SWP_HIDEONLY);

        drv = EndDeferWindowPos(dwp);

        nc->mode = KHUI_NC_MODE_EXPANDED;
        nc_size_container(nc);

        khui_cw_unlock_nc(nc);

        nc_layout_idsel(nc);
        nc_layout_privint(nc);
        nc_layout_nav(nc);

        break;

    case NC_PAGE_PASSWORD:

        khui_cw_lock_nc(nc);

        dwp = BeginDeferWindowPos(7);

        dwp = DeferWindowPos(dwp, nc->idsel.hwnd,
                             dlg_item(IDC_NC_R_IDSEL),
                             rect_coords(r_idsel), SWP_MOVESIZEZ);

        dwp = DeferWindowPos(dwp, nc->privint.hwnd_basic,
                             dlg_item(IDC_NC_R_MAIN),
                             rect_coords(r_main), SWP_MOVESIZEZ);

        dwp = DeferWindowPos(dwp, nc->nav.hwnd,
                             dlg_item(IDC_NC_R_NAV),
                             rect_coords(r_nav_sm), SWP_MOVESIZEZ);

        dwp = DeferWindowPos(dwp, nc->privint.hwnd_advanced, NULL, 0, 0, 0, 0,
                             SWP_HIDEONLY);

        dwp = DeferWindowPos(dwp, nc->idspec.hwnd, NULL, 0, 0, 0, 0,
                             SWP_HIDEONLY);
        
        dwp = DeferWindowPos(dwp, nc->progress.hwnd, NULL, 0, 0, 0, 0,
                             SWP_HIDEONLY);

        drv = EndDeferWindowPos(dwp);

        nc->mode = KHUI_NC_MODE_MINI;
        nc_size_container(nc);

        khui_cw_unlock_nc(nc);

        nc_layout_idsel(nc);
        nc_layout_privint(nc);
        nc_layout_nav(nc);

        break;

    case NC_PAGE_PROGRESS:

        khui_cw_lock_nc(nc);

        dwp = BeginDeferWindowPos(7);

        dwp = DeferWindowPos(dwp, nc->idsel.hwnd,
                             dlg_item(IDC_NC_R_IDSEL),
                             rect_coords(r_idsel), SWP_MOVESIZEZ);

        dwp = DeferWindowPos(dwp, nc->progress.hwnd,
                             dlg_item(IDC_NC_R_MAIN),
                             rect_coords(r_main), SWP_MOVESIZEZ);

        dwp = DeferWindowPos(dwp, nc->nav.hwnd,
                             dlg_item(IDC_NC_R_NAV),
                             rect_coords(r_nav_sm), SWP_MOVESIZEZ);

        dwp = DeferWindowPos(dwp, nc->privint.hwnd_basic, NULL, 0, 0, 0, 0,
                             SWP_HIDEONLY);

        dwp = DeferWindowPos(dwp, nc->privint.hwnd_advanced, NULL, 0, 0, 0, 0,
                             SWP_HIDEONLY);

        dwp = DeferWindowPos(dwp, nc->idspec.hwnd, NULL, 0, 0, 0, 0,
                             SWP_HIDEONLY);

        drv = EndDeferWindowPos(dwp);

        nc->mode = KHUI_NC_MODE_MINI;
        nc_size_container(nc);

        khui_cw_unlock_nc(nc);

        nc_layout_idsel(nc);
        nc_layout_progress(nc);
        nc_layout_nav(nc);

        break;

    default:
#ifdef DEBUG
        assert(FALSE);
#endif
    }

#undef dlg_item
}


/************************************************************
 *                       Utilities                          *
 ************************************************************/



/* Enable or disable all the controls in the new credentials wizard */
static void
nc_enable_controls(khui_new_creds * nc, BOOL enable)
{
    EnableWindow(nc->nav.hwnd, enable);
    EnableWindow(nc->idsel.hwnd, enable);
}



/* Send or post a message to all the credential type windows */
static void 
nc_notify_types(khui_new_creds * c, enum khui_wm_nc_notifications N,
                LPARAM lParam, BOOL sync)
{
    khm_size i;

    for (i=0; i < c->n_types; i++) {
        if (c->types[i].nct->hwnd_panel == NULL)
            continue;

        if (sync)
            SendMessage(c->types[i].nct->hwnd_panel, KHUI_WM_NC_NOTIFY,
                        MAKEWPARAM(0, N), lParam);
        else
            PostMessage(c->types[i].nct->hwnd_panel, KHUI_WM_NC_NOTIFY,
                        MAKEWPARAM(0, N), lParam);
    }
}


/* Prepare the wizard to handle a new primary identity.  Should NOT be
   called with nc locked. */
static void
nc_notify_new_identity(khui_new_creds * nc, BOOL notify_ui)
{
    nc_notify_types(nc, WMNC_IDENTITY_CHANGE, (LPARAM) nc, TRUE);
    nc_prep_cred_types(nc);

    khui_cw_lock_nc(nc);

    nc->privint.initialized = FALSE;

    /* The currently selected privileged interaction panel also need
       to be reset.  However, we let nc_layout_privint() handle that
       since it may need to hide the window. */

    if (nc->subtype == KHUI_NC_SUBTYPE_NEW_CREDS &&
        nc->n_identities > 0 &&
        nc->identities[0]) {
        khm_int32 f = 0;

        kcdb_identity_get_flags(nc->identities[0], &f);

        if (!(f & KCDB_IDENT_FLAG_DEFAULT)) {
            nc->set_default = FALSE;
        }
    }

    khui_cw_unlock_nc(nc);

    if (notify_ui) {

        nc_layout_idsel(nc);

        if (nc->page == NC_PAGE_CREDOPT_ADV ||
            nc->page == NC_PAGE_CREDOPT_BASIC ||
            nc->page == NC_PAGE_PASSWORD) {

            nc_layout_privint(nc);

        }
    }
}

static void
nc_navigate(khui_new_creds * nc, nc_page new_page)
{
    /* All the page transitions in the new credentials wizard happen
       here. */

    switch (new_page) {
    case NC_PAGE_NONE:
        /* Do nothing */
#ifdef DEBUG
        assert(FALSE);
#endif
        return;

    case NC_PAGE_IDSPEC:
        nc->idspec.prev_page = nc->page;
        nc->page = NC_PAGE_IDSPEC;
        if (nc->subtype == KHUI_NC_SUBTYPE_IDSPEC) {
            nc->nav.transitions = NC_TRANS_FINISH;
        } else {
            nc->nav.transitions = NC_TRANS_NEXT;
        }
        if (nc->idspec.prev_page != NC_PAGE_NONE)
            nc->nav.transitions |= NC_TRANS_PREV;
        break;

    case NC_PAGE_CREDOPT_BASIC:
        nc->nav.transitions = NC_TRANS_FINISH;
        nc->page = NC_PAGE_CREDOPT_BASIC;
        break;

    case NC_PAGE_CREDOPT_ADV:
        nc->nav.transitions = NC_TRANS_FINISH;
        nc->page = NC_PAGE_CREDOPT_ADV;
        break;

    case NC_PAGE_PASSWORD:
        nc->nav.transitions = NC_TRANS_FINISH;
        nc->page = NC_PAGE_PASSWORD;
        break;

    case NC_PAGE_PROGRESS:
        nc->nav.transitions = NC_TRANS_ABORT;
        nc->page = NC_PAGE_PROGRESS;
        break;

    case NC_PAGET_NEXT:
        switch (nc->page) {
        case NC_PAGE_IDSPEC:
            if (nc_idspec_process(nc)) {

                switch (nc->subtype) {
                case KHUI_NC_SUBTYPE_NEW_CREDS:
                    nc->page = NC_PAGE_CREDOPT_ADV;
                    break;

                case KHUI_NC_SUBTYPE_PASSWORD:
                    nc->page = NC_PAGE_PASSWORD;
                    break;

                default:
                    assert(FALSE);
                    break;
                }

                break;
            }

            return;

        default:
            assert(FALSE);
        }
        break;

    case NC_PAGET_PREV:
        switch (nc->page) {
        case NC_PAGE_IDSPEC:
            if (nc->idspec.prev_page != NC_PAGE_NONE) {
                nc->page = nc->idspec.prev_page;
            }
            break;

        default:
            assert(FALSE);
        }
        break;

    case NC_PAGET_FINISH:
        switch (nc->subtype) {
        case KHUI_NC_SUBTYPE_IDSPEC:
            assert(nc->page == NC_PAGE_IDSPEC);
            if (nc_idspec_process(nc)) {
                nc->result = KHUI_NC_RESULT_PROCESS;
                khm_cred_dispatch_process_message(nc);
            }
            break;

        case KHUI_NC_SUBTYPE_NEW_CREDS:
        case KHUI_NC_SUBTYPE_PASSWORD:
            nc->result = KHUI_NC_RESULT_PROCESS;
            nc_notify_types(nc, WMNC_DIALOG_PREPROCESS, (LPARAM) nc, TRUE);
            khm_cred_dispatch_process_message(nc);
            nc->nav.transitions = NC_TRANS_ABORT;
            nc->page = NC_PAGE_PROGRESS;
            break;

        default:
            assert(FALSE);
        }
        break;

    case NC_PAGET_CANCEL:
        nc->result = KHUI_NC_RESULT_CANCEL;
        nc_notify_types(nc, WMNC_DIALOG_PREPROCESS, (LPARAM) nc, TRUE);
        khm_cred_dispatch_process_message(nc);
        nc->nav.transitions = 0;
        nc->page = NC_PAGE_PROGRESS;
        break;

    default:
        assert(FALSE);
    }

    nc_layout_container(nc);
}


/************************************************************
 *          Dialog procedures for child dialogs             *
 ************************************************************/

/* Navigation dialog.  IDD_NC_NAV */
static INT_PTR CALLBACK
nc_nav_dlg_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    khui_new_creds * nc;

    switch (uMsg) {
    case WM_INITDIALOG:
        nc = (khui_new_creds *) lParam;
        nc_set_dlg_data(hwnd, nc);
        return TRUE;

    case WM_COMMAND:
        nc = nc_get_dlg_data(hwnd);

        switch (wParam) {
        case MAKEWPARAM(IDC_BACK, BN_CLICKED):
            nc_navigate(nc, NC_PAGET_PREV);
            return TRUE;

        case MAKEWPARAM(IDC_NEXT, BN_CLICKED):
            nc_navigate(nc, NC_PAGET_NEXT);
            return TRUE;

        case MAKEWPARAM(IDC_FINISH, BN_CLICKED):
            nc_navigate(nc, NC_PAGET_FINISH);
            return TRUE;

        case MAKEWPARAM(IDCANCEL, BN_CLICKED):
            nc_navigate(nc, NC_PAGET_CANCEL);
            return TRUE;
        }
        return FALSE;

    default:
        return FALSE;
    }
}

/* Progress. IDD_NC_PROGRESS */
static INT_PTR CALLBACK
nc_progress_dlg_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    khui_new_creds * nc;

    switch (uMsg) {
    case WM_INITDIALOG:
        nc = (khui_new_creds *) lParam;
        nc_set_dlg_data(hwnd, nc);
        return TRUE;

    default:
        return FALSE;
    }
}

/* No-Prompt Placeholder.  IDD_NC_NOPROMPTS */
static INT_PTR CALLBACK
nc_noprompts_dlg_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    khui_new_creds * nc;

    switch (uMsg) {
    case WM_INITDIALOG:
        nc = (khui_new_creds *) lParam;
        nc_set_dlg_data(hwnd, nc);
        return TRUE;

    default:
        return FALSE;
    }
}


/************************************************************
 *                   Custom prompter                        *
 ************************************************************/

static void
nc_layout_custom_prompter(HWND hwnd, khui_new_creds_privint_panel * p, BOOL create)
{
    SIZE window;
    SIZE margin;
    SIZE row;
    RECT r_sm_lbl;
    RECT r_sm_input;
    RECT r_lg_lbl;
    RECT r_lg_input;
    SIZE banner;

    HFONT hf = NULL;
    HFONT hf_old = NULL;
    HDC  hdc = NULL;
    HDWP hdwp = NULL;
    HWND hw;

    RECT r;
    SIZE s;
    size_t cch;
    khm_size i;

    int x,y;

    assert(p->use_custom);

    {
        RECT r_row;

        GetWindowRect(hwnd, &r);

        window.cx = r.right - r.left;
        window.cy = r.bottom - r.top;

        GetWindowRect(GetDlgItem(hwnd, IDC_NC_TPL_ROW), &r_row);

        margin.cx = r_row.left - r.left;
        margin.cy = r_row.top - r.top;

        row.cy = r_row.bottom - r_row.top;
        row.cx = window.cx - margin.cx * 2;

        GetWindowRect(GetDlgItem(hwnd, IDC_NC_TPL_LABEL), &r_sm_lbl);
        OffsetRect(&r_sm_lbl, -r_row.left, -r_row.top);

        GetWindowRect(GetDlgItem(hwnd, IDC_NC_TPL_INPUT), &r_sm_input);
        OffsetRect(&r_sm_input, -r_row.left, -r_row.top);
        r_sm_input.right = row.cx;

        GetWindowRect(GetDlgItem(hwnd, IDC_NC_TPL_ROW_LG), &r_row);

        GetWindowRect(GetDlgItem(hwnd, IDC_NC_TPL_LABEL_LG), &r_lg_lbl);
        OffsetRect(&r_lg_lbl, -r_row.left, -r_row.top);

        GetWindowRect(GetDlgItem(hwnd, IDC_NC_TPL_INPUT_LG), &r_lg_input);
        OffsetRect(&r_lg_input, -r_row.left, -r_row.top);
        r_lg_input.right = row.cx;

        banner.cx = row.cx;
        banner.cy = r_sm_lbl.bottom - r_sm_lbl.top;
    }

    x = margin.cx;
    y = margin.cy;

    hf = (HFONT) SendMessage(GetDlgItem(hwnd, IDC_NC_TPL_LABEL), WM_GETFONT, 0, 0);
    hdc = GetDC(hwnd);
    hf_old = SelectFont(hdc, hf);

    if (!create) {
        hdwp = BeginDeferWindowPos((int) (2 + p->n_prompts * 2));
    }

    if (p->pname) {
        StringCchLength(p->pname, KHUI_MAXCCH_PNAME, &cch);
        SetRect(&r, 0, 0, banner.cx - margin.cx, 0);
        DrawText(hdc, p->pname, (int) cch, &r, DT_CALCRECT | DT_WORDBREAK);
        assert(r.bottom > 0);

        r.right = banner.cx;
        r.bottom += margin.cy;
        OffsetRect(&r, x, y);

        if (create) {
            hw = CreateWindow(L"STATIC", p->pname,
                              SS_CENTER | SS_SUNKEN | WS_CHILD,
                              r.left, r.top, r.right - r.left, r.bottom - r.top,
                              hwnd, (HMENU) IDC_NCC_PNAME,
                              khm_hInstance, NULL);
            assert(hw != NULL);
            SendMessage(hw, WM_SETFONT, (WPARAM) hf, 0);
        } else {
            hw = GetDlgItem(hwnd, IDC_NCC_PNAME);
            assert(hw != NULL);
            hdwp = DeferWindowPos(hdwp, hw, NULL, rect_coords(r), SWP_MOVESIZE);
        }

        y = r.bottom;
    }

    if (p->banner) {
        StringCchLength(p->banner, KHUI_MAXCCH_BANNER, &cch);
        SetRect(&r, 0, 0, banner.cx - margin.cx, 0);
        DrawText(hdc, p->banner, (int) cch, &r, DT_CALCRECT | DT_WORDBREAK);
        assert(r.bottom > 0);

        r.right = banner.cx;
        r.bottom += margin.cy;
        OffsetRect(&r, x, y);

        if (create) {
            hw = CreateWindow(L"STATIC", p->banner,
                              SS_LEFT | WS_CHILD,
                              r.left, r.top, r.right - r.left, r.bottom - r.top,
                              hwnd, (HMENU) IDC_NCC_BANNER,
                              khm_hInstance, NULL);
            assert(hw != NULL);
            SendMessage(hw, WM_SETFONT, (WPARAM) hf, 0);
        } else {
            hw = GetDlgItem(hwnd, IDC_NCC_BANNER);
            assert(hw);
            hdwp = DeferWindowPos(hdwp, hw, NULL, rect_coords(r), SWP_MOVESIZE);
        }

        y = r.bottom;
    }

    for (i=0; i < p->n_prompts; i++) {
        khui_new_creds_prompt * pr;
        RECT rlbl, rinp;

        pr = p->prompts[i];

        assert(pr);
        if (pr == NULL)
            continue;

        s.cx = 0;
        StringCchLength(pr->prompt, KHUI_MAXCCH_PROMPT, &cch);
        GetTextExtentPoint32(hdc, pr->prompt, (int) cch, &s);
        assert(s.cx > 0);

        if (s.cx < r_sm_lbl.right - r_sm_lbl.left) {
            CopyRect(&rlbl, &r_sm_lbl);
            OffsetRect(&rlbl, x, y);
            CopyRect(&rinp, &r_sm_input);
            OffsetRect(&rinp, x, y);
            y += row.cy;
        } else if (s.cx < r_lg_lbl.right - r_lg_lbl.left) {
            CopyRect(&rlbl, &r_lg_lbl);
            OffsetRect(&rlbl, x, y);
            CopyRect(&rinp, &r_lg_input);
            OffsetRect(&rinp, x, y);
            y += row.cy;
        } else {
            SetRect(&rlbl, x, y, x + banner.cx, y + banner.cy);
            y += banner.cy;
            CopyRect(&rinp, &r_sm_input);
            OffsetRect(&rinp, x, y);
            y += row.cy;
        }

        if (create) {
            assert(pr->hwnd_edit == NULL);
            assert(pr->hwnd_static == NULL);

            pr->hwnd_static = CreateWindow(L"STATIC", pr->prompt,
                                           SS_LEFT | WS_CHILD,
                                           rlbl.left, rlbl.top,
                                           rlbl.right - rlbl.left, rlbl.bottom - rlbl.top,
                                           hwnd,
                                           (HMENU) (IDC_NCC_CTL + i*2),
                                           khm_hInstance, NULL);
            assert(pr->hwnd_static != NULL);
            SendMessage(pr->hwnd_static, WM_SETFONT, (WPARAM) hf, 0);

            pr->hwnd_edit = CreateWindow(L"EDIT", ((pr->def)?pr->def:L""),
                                         ((pr->flags & KHUI_NCPROMPT_FLAG_HIDDEN)?
                                          ES_PASSWORD: 0) | WS_CHILD | WS_TABSTOP |
                                         WS_BORDER,
                                         rinp.left, rinp.top,
                                         rinp.right - rinp.left, rinp.bottom - rinp.top,
                                         hwnd, (HMENU) (IDC_NCC_CTL + 1 + i*2),
                                         khm_hInstance, NULL);
            assert(pr->hwnd_edit != NULL);
            SendMessage(pr->hwnd_edit, WM_SETFONT, (WPARAM) hf, 0);
        } else {
            hw = GetDlgItem(hwnd, (int)(IDC_NCC_CTL + i*2));
            assert(hw != NULL);
            hdwp = DeferWindowPos(hdwp, hw, NULL, rect_coords(rlbl), SWP_MOVESIZE);

            hw = GetDlgItem(hwnd, (int)(IDC_NCC_CTL + 1 + i*2));
            assert(hw != NULL);
            hdwp = DeferWindowPos(hdwp, hw, NULL, rect_coords(rinp), SWP_MOVESIZE);
        }
    }

    SelectFont(hdc, hf_old);
    ReleaseDC(hwnd, hdc);
    if (!create) {
        EndDeferWindowPos(hdwp);
    }
}

static INT_PTR CALLBACK
nc_prompter_dlg_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    khui_new_creds_privint_panel * p;

    switch (uMsg) {
    case WM_INITDIALOG:
        {
            p = (khui_new_creds_privint_panel *) lParam;
            assert(p->magic == KHUI_NEW_CREDS_PRIVINT_PANEL_MAGIC);

#pragma warning(push)
#pragma warning(disable: 4244)
            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) p);
#pragma warning(pop)

            assert(p->hwnd == NULL);
            p->hwnd = hwnd;

            nc_layout_custom_prompter(hwnd, p, TRUE);
        }
        return TRUE;

    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS * wp;

            p = (khui_new_creds_privint_panel *)(LONG_PTR) GetWindowLongPtr(hwnd, DWLP_USER);
            assert(p->magic == KHUI_NEW_CREDS_PRIVINT_PANEL_MAGIC);

            wp = (WINDOWPOS *) lParam;

            if (!(wp->flags & SWP_NOMOVE) ||
                !(wp->flags & SWP_NOSIZE))
                nc_layout_custom_prompter(hwnd, p, FALSE);
        }
        return TRUE;

    case WM_CLOSE:
        {
            DestroyWindow(hwnd);
        }
        return TRUE;

    case WM_DESTROY:
        {
            p = (khui_new_creds_privint_panel *)(LONG_PTR) GetWindowLongPtr(hwnd, DWLP_USER);
            assert(p->magic == KHUI_NEW_CREDS_PRIVINT_PANEL_MAGIC);

            p->hwnd = NULL;
        }
        return TRUE;

    default:
        return FALSE;
    }
}


static HWND
nc_create_custom_prompter_dialog(khui_new_creds * nc,
                                 HWND parent,
                                 khui_new_creds_privint_panel * p)
{
    HWND hw;

#ifdef DEBUG
    assert(p->magic == KHUI_NEW_CREDS_PRIVINT_PANEL_MAGIC);
    assert(p->hwnd == NULL);
#endif

    p->nc = nc;
    hw = CreateDialogParam(khm_hInstance,
                           MAKEINTRESOURCE(IDD_NC_PROMPTS),
                           parent,
                           nc_prompter_dlg_proc,
                           (LPARAM) p);
    assert(hw != NULL);

    return hw;
}


/************************************************************
 *    Message handlers for the wizard. IDD_NC_CONTAINER     *
 ************************************************************/

static LRESULT 
nc_handle_wm_initdialog(HWND hwnd,
                        UINT uMsg,
                        WPARAM wParam,
                        LPARAM lParam)
{
    khui_new_creds * nc;
    int x, y;
    int width, height;
    RECT r;
    HWND hwnd_parent = NULL;

    nc = (khui_new_creds *) lParam;
#ifdef debug
    assert(nc);
#endif
    nc->hwnd = hwnd;

#ifdef DEBUG
    assert(nc != NULL);
    assert(nc->subtype == KHUI_NC_SUBTYPE_NEW_CREDS ||
           nc->subtype == KHUI_NC_SUBTYPE_PASSWORD ||
           nc->subtype == KHUI_NC_SUBTYPE_IDSPEC);
#endif
    nc_set_dlg_data(hwnd, nc);

    nc->nav.hwnd =
        CreateDialogParam(khm_hInstance,
                          MAKEINTRESOURCE(IDD_NC_NAV),
                          hwnd, nc_nav_dlg_proc, (LPARAM) nc);
    nc->idsel.hwnd =
        CreateDialogParam(khm_hInstance,
                          MAKEINTRESOURCE(IDD_NC_IDSEL),
                          hwnd, nc_idsel_dlg_proc, (LPARAM) nc);
    nc->privint.hwnd_basic =
        CreateDialogParam(khm_hInstance,
                          MAKEINTRESOURCE(IDD_NC_PRIVINT_BASIC),
                          hwnd, nc_privint_basic_dlg_proc, (LPARAM) nc);

    nc->privint.hwnd_advanced =
        CreateDialogParam(khm_hInstance,
                          MAKEINTRESOURCE(IDD_NC_PRIVINT_ADVANCED),
                          hwnd, nc_privint_advanced_dlg_proc, (LPARAM) nc);
    nc->idspec.hwnd =
        CreateDialogParam(khm_hInstance,
                          MAKEINTRESOURCE(IDD_NC_IDSPEC),
                          hwnd, nc_idspec_dlg_proc, (LPARAM) nc);
    nc->progress.hwnd =
        CreateDialogParam(khm_hInstance,
                          MAKEINTRESOURCE(IDD_NC_PROGRESS),
                          hwnd, nc_progress_dlg_proc, (LPARAM) nc);
    nc->privint.hwnd_noprompts =
        CreateDialogParam(khm_hInstance,
                          MAKEINTRESOURCE(IDD_NC_NOPROMPTS),
                          hwnd, nc_noprompts_dlg_proc, (LPARAM) nc);

    /* Position the dialog */

    GetWindowRect(hwnd, &r);

    width = r.right - r.left;
    height = r.bottom - r.top;

    /* if the parent window is visible, we center the new credentials
       dialog over the parent.  Otherwise, we center it on the primary
       display. */

    hwnd_parent = GetParent(hwnd);

    if (IsWindowVisible(hwnd_parent)) {
        GetWindowRect(hwnd_parent, &r);
    } else {
        if(!SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID) &r, 0)) {
            /* failover to the window coordinates */
            GetWindowRect(hwnd_parent, &r);
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

    /* add this to the dialog chain.  Doing so allows the main message
       dispatcher to use IsDialogMessage() to dispatch our
       messages. */
    khm_add_dialog(hwnd);

    return TRUE;
}


static LRESULT 
nc_handle_wm_destroy(HWND hwnd,
                     UINT uMsg,
                     WPARAM wParam,
                     LPARAM lParam)
{
    khui_new_creds * nc;

    /* remove self from dialog chain */
    khm_del_dialog(hwnd);

    nc = nc_get_dlg_data(hwnd);
    if (nc == NULL)
        return TRUE;

    nc_set_dlg_data(hwnd, NULL);

    return TRUE;
}

static LRESULT 
nc_handle_wm_command(HWND hwnd,
                     UINT uMsg,
                     WPARAM wParam,
                     LPARAM lParam)
{
    khui_new_creds * nc;

    nc = nc_get_dlg_data(hwnd);
    if (nc == NULL)
        return 0;

    switch (wParam) {
    case MAKEWPARAM(IDCANCEL, BN_CLICKED):
        nc_navigate(nc, NC_PAGET_CANCEL);
        break;

    case MAKEWPARAM(IDOK, BN_CLICKED):
        nc_navigate(nc, NC_PAGET_FINISH);
        break;
    }

    return TRUE;
}

static LRESULT
nc_handle_wm_moving(HWND hwnd,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
    khui_new_creds * nc;

    nc = nc_get_dlg_data(hwnd);
    if (nc == NULL || nc->privint.hwnd_current == NULL)
        return FALSE;

    SendMessage(nc->privint.hwnd_current, KHUI_WM_NC_NOTIFY, 
                MAKEWPARAM(0, WMNC_DIALOG_MOVE), (LPARAM) nc);

    return FALSE;
}




static LRESULT
nc_handle_wm_nc_notify(HWND hwnd,
                       UINT uMsg,
                       WPARAM wParam,
                       LPARAM lParam)
{
    khui_new_creds * nc;

    nc = nc_get_dlg_data(hwnd);
    if (nc == NULL)
        return FALSE;

    switch(HIWORD(wParam)) {

    case WMNC_DIALOG_SETUP:

        /* At this point, all the credential types that are interested
           in the new credentials operation have attached themselves.
           We should create the dialogs. */

#ifdef DEBUG
        assert(nc->privint.hwnd_advanced != NULL);
#endif

        if(nc->n_types > 0) {
            khm_size i;
            for(i=0; i < nc->n_types;i++) {
                khui_new_creds_by_type * t;

                t = nc->types[i].nct;

                if (t->dlg_proc == NULL) {
                    t->hwnd_panel = NULL;
                } else {
                    /* Create the dialog panel */
                    t->hwnd_panel =
                        CreateDialogParam(t->h_module, t->dlg_template,
                                          nc->privint.hwnd_advanced,
                                          t->dlg_proc, (LPARAM) nc);

#ifdef DEBUG
                    assert(t->hwnd_panel);
#endif
#if _WIN32_WINNT >= 0x0501
                    if (t->hwnd_panel) {
                        EnableThemeDialogTexture(t->hwnd_panel,
                                                 ETDT_ENABLETAB);
                    }
#endif
                }
            }
        }

        break;

    case WMNC_DIALOG_ACTIVATE:
        {
            khm_int32 t;

            /* About to activate the new credentials dialog.  We need
               to set up the wizard. */
            switch (nc->subtype) {
            case KHUI_NC_SUBTYPE_NEW_CREDS:

                if (nc->n_identities > 0) {
                    /* If there is a primary identity, then we can
                       start in the credentials options page */

                    khm_int32 idflags = 0;
                    khm_handle ident;

                    nc_notify_new_identity(nc, FALSE);

                    ident = nc->identities[0];
#ifdef DEBUG
                    assert(ident != NULL);
#endif
                    kcdb_identity_get_flags(ident, &idflags);

                    /* Check if this identity has a configuration.  If
                       so, we can continue in basic mode.  Otherwise
                       we should start in advanced mode so that the
                       user can specify identity options to be used
                       the next time. */
                    if (idflags & KCDB_IDENT_FLAG_CONFIG) {
                        nc_navigate(nc, NC_PAGE_CREDOPT_BASIC);
                    } else {
                        nc_navigate(nc, NC_PAGE_CREDOPT_ADV);
                    }
                } else {
                    /* No primary identity.  We have to open with the
                       identity specification page */
                    nc_navigate(nc, NC_PAGE_IDSPEC);
                }

                break;

            case KHUI_NC_SUBTYPE_PASSWORD:
                if (nc->n_identities > 0) {
                    nc_notify_new_identity(nc, FALSE);
                    nc_navigate(nc, NC_PAGE_PASSWORD);
                } else {
                    nc_navigate(nc, NC_PAGE_IDSPEC);
                }
                break;

            case KHUI_NC_SUBTYPE_IDSPEC:
                nc_navigate(nc, NC_PAGE_IDSPEC);
                break;

            default:
#ifdef DEBUG
                assert(FALSE);
#endif
            }

            ShowWindow(hwnd, SW_SHOWNORMAL);

            t = 0;
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

                    nc->flashing_enabled = TRUE;
                }

            } else {
                SetFocus(hwnd);
            }
        }
        break;

    case WMNC_IDENTITY_CHANGE:
        {
            nc_notify_new_identity(nc, TRUE);
        }
        break;

    case WMNC_DIALOG_SWITCH_PANEL:
#if 0
        id = LOWORD(wParam);
        if(id >= 0 && id <= (int) nc->n_types) {
            /* one of the tab buttons were pressed */
            if(nc->privint.idx_current == id) {
                return TRUE; /* nothing to do */
            }

            nc->privint.idx_current = id;

            TabCtrl_SetCurSel(nc->tab_wnd, id);
        }

        if(nc->mode == KHUI_NC_MODE_EXPANDED) {
            nc_layout_new_cred_window(nc);
            return TRUE;
        }
#endif
        break;

    case WMNC_DIALOG_EXPAND:
#ifdef DEBUG
        assert(FALSE);
#endif
        break;

    case WMNC_IDENTITY_STATE:
        {
            nc_privint_update_identity_state(nc, (nc_identity_state_notification *) lParam);
        }
        break;

    case WMNC_SET_PROMPTS:
        khui_cw_get_next_privint(nc, NULL);
        nc->privint.initialized = FALSE;

        if (nc->page == NC_PAGE_PASSWORD ||
            nc->page == NC_PAGE_CREDOPT_ADV ||
            nc->page == NC_PAGE_CREDOPT_BASIC) {

            nc_layout_container(nc);
        }
        break;

    case WMNC_DIALOG_PROCESS_COMPLETE:
        {
            nc->response &= ~KHUI_NC_RESPONSE_PROCESSING;

            if(nc->response & KHUI_NC_RESPONSE_NOEXIT) {

                nc_enable_controls(nc, TRUE);

                /* reset state */
                nc->result = KHUI_NC_RESULT_CANCEL;

                nc_notify_types(nc, WMNC_DIALOG_PROCESS_COMPLETE, 0, TRUE);

                if (nc->subtype == KHUI_NC_SUBTYPE_NEW_CREDS)
                    nc_navigate(nc, NC_PAGE_CREDOPT_BASIC);
                else if (nc->subtype == KHUI_NC_SUBTYPE_PASSWORD)
                    nc_navigate(nc, NC_PAGE_PASSWORD);
                else {
                    assert(FALSE);
                }
                return TRUE;
            }

            DestroyWindow(hwnd);
            kmq_post_message(KMSG_CRED, KMSG_CRED_END, 0, (void *) nc);
        }
        break;


    case WMNC_UPDATE_LAYOUT:
#ifdef DEBUG
        assert(FALSE);
#endif
        break;
    } /* switch(HIWORD(wParam)) */

    return TRUE;
}

static LRESULT nc_handle_wm_help(HWND hwnd,
                                 UINT uMsg,
                                 WPARAM wParam,
                                 LPARAM lParam) {
#if 0
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
    khui_new_creds * nc;

    nc = nc_get_dlg_data(hwnd);
    if (nc == NULL)
        return FALSE;

    hlp = (HELPINFO *) lParam;

    if (nc->subtype != KHUI_NC_SUBTYPE_NEW_CREDS &&
        nc->subtype != KHUI_NC_SUBTYPE_PASSWORD)
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
                               ((nc->subtype == KHUI_NC_SUBTYPE_NEW_CREDS)?
                                L"::popups_newcreds.txt":
                                L"::popups_password.txt"),
                               HH_TP_HELP_WM_HELP,
                               (DWORD_PTR) ctxids);
    }

    if (hw == NULL) {
        khm_html_help(hwnd, NULL, HH_HELP_CONTEXT,
                      ((nc->subtype == KHUI_NC_SUBTYPE_NEW_CREDS)?
                       IDH_ACTION_NEW_ID: IDH_ACTION_PASSWD_ID));
    }

#endif

    return TRUE;
}

static LRESULT
nc_handle_wm_close(HWND hwnd, UINT uMsg,
                   WPARAM wParam, LPARAM lParam)
{
    khui_new_creds * nc;

    nc = nc_get_dlg_data(hwnd);
    nc_navigate(nc, NC_PAGET_CANCEL);

    return TRUE;
}


static LRESULT nc_handle_wm_activate(HWND hwnd,
                                     UINT uMsg,
                                     WPARAM wParam,
                                     LPARAM lParam) {
    if (uMsg == WM_MOUSEACTIVATE || uMsg == WM_ACTIVATEAPP ||
        wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE) {

        FLASHWINFO fi;
        khui_new_creds * nc;
        DWORD_PTR ex_style;

        nc = nc_get_dlg_data(hwnd);

        if (nc && nc->flashing_enabled) {
            ZeroMemory(&fi, sizeof(fi));

            fi.cbSize = sizeof(fi);
            fi.hwnd = hwnd;
            fi.dwFlags = FLASHW_STOP;

            FlashWindowEx(&fi);

            nc->flashing_enabled = FALSE;
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
    case WM_ACTIVATEAPP:
        return nc_handle_wm_activate(hwnd, uMsg, wParam, lParam);

    case WM_MOVE:
    case WM_MOVING:
        return nc_handle_wm_moving(hwnd, uMsg, wParam, lParam);

    case WM_HELP:
        return nc_handle_wm_help(hwnd, uMsg, wParam, lParam);

    case KHUI_WM_NC_NOTIFY:
        return nc_handle_wm_nc_notify(hwnd, uMsg, wParam, lParam);
    }

    return FALSE;
}


HWND khm_create_newcredwnd(HWND parent, khui_new_creds * c)
{
    wchar_t wtitle[256];
    HWND hwnd;

    khui_cw_lock_nc(c);

    if (c->window_title == NULL) {
        size_t t = 0;

        wtitle[0] = L'\0';

        switch (c->subtype) {
        case KHUI_NC_SUBTYPE_PASSWORD:
            LoadString(khm_hInstance, 
                       IDS_WT_PASSWORD,
                       wtitle,
                       ARRAYLENGTH(wtitle));
            break;

        case KHUI_NC_SUBTYPE_NEW_CREDS:
            LoadString(khm_hInstance, 
                       IDS_WT_NEW_CREDS,
                       wtitle,
                       ARRAYLENGTH(wtitle));
            break;

        case KHUI_NC_SUBTYPE_IDSPEC:
            LoadString(khm_hInstance,
                       IDS_WT_IDSPEC,
                       wtitle,
                       ARRAYLENGTH(wtitle));
            break;

        default:
#ifdef DEBUG
            assert(FALSE);
#endif
        }

        StringCchLength(wtitle, ARRAYLENGTH(wtitle), &t);
        if (t > 0) {
            t = (t + 1) * sizeof(wchar_t);
            c->window_title = PMALLOC(t);
            StringCbCopy(c->window_title, t, wtitle);
        }
    }

    khui_cw_unlock_nc(c);

    khc_read_int32(NULL, L"CredWindow\\Windows\\NewCred\\ForceToTop", &c->force_topmost);

    hwnd = CreateDialogParam(khm_hInstance, MAKEINTRESOURCE(IDD_NC_CONTAINER),
                             parent, nc_dlg_proc,  (LPARAM) c);

#ifdef DEBUG
    assert(hwnd != NULL);
#endif

    return hwnd;
}

void khm_prep_newcredwnd(HWND hwnd)
{
    SendMessage(hwnd, KHUI_WM_NC_NOTIFY, 
                MAKEWPARAM(0, WMNC_DIALOG_SETUP), 0);
}

void khm_show_newcredwnd(HWND hwnd)
{
    PostMessage(hwnd, KHUI_WM_NC_NOTIFY, 
                MAKEWPARAM(0, WMNC_DIALOG_ACTIVATE), 0);
}


void khm_register_newcredwnd_class(void)
{
    /* Nothing to do */
}


void khm_unregister_newcredwnd_class(void)
{
    /* Nothing to do */
}
