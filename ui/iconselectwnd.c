/*
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

#include "khmapp.h"
#ifdef DEBUG
#include<assert.h>
#endif

struct ch_icon_data {
    khm_int32   magic;          /* CH_ICON_DATA_MAGIC */
    wchar_t     pathname[MAX_PATH];
    khm_int32   flags;
    khm_restype restype;
    khm_int32   index;          /* Currently selected index */
};

#define CH_ICON_DATA_MAGIC 0x02dd1a65

#define BMP_OFFSET 0x1000

static void
parse_resource_path(struct ch_icon_data * c, const wchar_t * respath)
{
    size_t len;
    const wchar_t * colon;
    const wchar_t * ppath;
    const wchar_t * comma;

    c->magic = CH_ICON_DATA_MAGIC;

    if (FAILED(StringCchLength(respath, MAX_PATH, &len)))
        return;

    colon = wcschr(respath, L':');
    if (colon == NULL)
        return;

    ppath = colon + 1;

    comma = wcsrchr(ppath, L',');

    if (comma)
        len = comma - ppath;
    else
        len -= ppath - respath;

    StringCchCopyN(c->pathname, ARRAYLENGTH(c->pathname), ppath, len);

    if (comma)
        c->index = _wtoi(comma + 1);
    else
        c->index = 0;

    if (wcsncmp(respath, KHUI_PREFIX_ICO, ARRAYLENGTH(KHUI_PREFIX_ICO) - 1) == 0 ||
        wcsncmp(respath, KHUI_PREFIX_ICODLL, ARRAYLENGTH(KHUI_PREFIX_ICODLL) - 1) == 0)
        c->restype = KHM_RESTYPE_ICON;
    else
        c->restype = KHM_RESTYPE_BITMAP;
}

static UINT_PTR
ofn_load_icons(struct ch_icon_data * c, HWND hwnd, HWND hw_ofn)
{
    HWND hw_lst;                /* icon list view */

    HIMAGELIST  ilist = NULL;   /* Image list */
    khm_size    n_icons = 0;    /* Number of icons */
    khm_size    n_bitmaps = 0;  /* Number of bitmaps */
    khm_restype restype;        /* resource type */
    khm_int32   flags;     /* flags for khui_load_icons_from_path() */
    HICON     * picons = NULL;  /* icons list */
    
    HICON       icbuf[32];      /* icon buffer */
    wchar_t     path[MAX_PATH];

    khm_size    i;

    hw_lst = GetDlgItem(hwnd, IDC_LIST);
    assert(hw_lst);

    ListView_DeleteAllItems(hw_lst);
    c->index = -1;              /* also signals that the user hasn't
                                   selected an icon.  When
                                   c->index==-1, the dialog won't
                                   process IDOK. */

    if (!SendMessage(hw_ofn, CDM_GETFILEPATH, ARRAYLENGTH(path),
                     (LPARAM) path))
        return TRUE;

    {
        wchar_t * pext;

        pext = wcsrchr(path, L'.');
        if (pext == NULL)
            return FALSE;
        else if (!_wcsicmp(pext, L".ico")) {
            restype = KHM_RESTYPE_ICON;
            flags = 0; n_icons = 1;
        } else if (!_wcsicmp(pext, L".bmp")) {
            restype = KHM_RESTYPE_BITMAP;
            flags = 0; n_icons = 1;
        } else if (!_wcsicmp(pext, L".dll") ||
                   !_wcsicmp(pext, L".exe")) {
            khm_size n_bitmaps = 0;

            restype = KHM_RESTYPE_ICON;
            flags = KHUI_LIFR_FROMLIB;

            n_icons = 0;
            khui_load_icons_from_path(path, KHM_RESTYPE_ICON,
                                      0, KHUI_LIFR_FROMLIB,
                                      NULL, &n_icons);
            
            khui_load_icons_from_path(path, KHM_RESTYPE_BITMAP,
                                      0, KHUI_LIFR_FROMLIB,
                                      NULL, &n_bitmaps);
            n_icons += n_bitmaps;
            if (n_icons == 0)
                return TRUE;
        } else {
            return TRUE;
        }
    }

    c->restype = restype;
    c->flags = flags;
    StringCchCopy(c->pathname, ARRAYLENGTH(c->pathname), path);

    if (n_icons > ARRAYLENGTH(icbuf))
        picons = PMALLOC(sizeof(HICON) * n_icons);
    else
        picons = icbuf;

    ZeroMemory(picons, sizeof(HICON) * n_icons);

    {
        khm_size t;

        t = n_icons;

        khui_load_icons_from_path(path, restype, 0, flags,
                                  picons, &t);

        if ((flags & KHUI_LIFR_FROMLIB) &&
            n_icons > t) {
            khm_size t2;
            khm_int32 rv;

            t2 = n_icons - t;
            rv = khui_load_icons_from_path(path, KHM_RESTYPE_BITMAP,
                                           0, flags,
                                           picons + t, &t2);
            assert(KHM_SUCCEEDED(rv));
            if (KHM_FAILED(rv))
                goto _cleanup_selchange;

            n_icons = t + t2;
            n_bitmaps = t2;
        } else {
            n_icons = t;
            n_bitmaps = 0;
        }
    }

    ilist = ImageList_Create(GetSystemMetrics(SM_CXICON),
                             GetSystemMetrics(SM_CYICON),
                             ILC_COLORDDB|ILC_MASK,
                             (int) n_icons, 4 /* Growth factor */);

    {
        HIMAGELIST ilist_old;

        ilist_old = ListView_SetImageList(hw_lst, ilist, LVSIL_NORMAL);
        if (ilist_old) {
            ImageList_Destroy(ilist_old);
        }
    }

    {
        int x, y;
        RECT r;
        SIZE s;

        GetClientRect(hw_lst, &r);
        s.cx = GetSystemMetrics(SM_CXICON);
        s.cy = GetSystemMetrics(SM_CYICON);
        r.left += s.cx / 2;
        r.right -= s.cx / 2;
        r.top += s.cy / 2;
        x = r.left; y = r.top;

        for (i = 0; i < n_icons; i++) {
            LVITEM lvi;
            int idx_new;

            assert(picons[i]);
            if (picons[i] == NULL)
                continue;

            lvi.mask = LVIF_IMAGE|LVIF_PARAM;
            lvi.iItem = 0;
            lvi.iSubItem = 0;

            lvi.iImage = ImageList_AddIcon(ilist, picons[i]);

            DestroyIcon(picons[i]);
            picons[i] = NULL;

            lvi.lParam =
                (i >= n_icons - n_bitmaps)?BMP_OFFSET + (i - (n_icons - n_bitmaps)): i;

            idx_new = ListView_InsertItem(hw_lst, &lvi);
            ListView_SetItemPosition(hw_lst, idx_new, x, y);

            x += s.cx + s.cx / 2;
            if (x + s.cx > r.right) {
                x = r.left;
                y += s.cy + s.cy / 2;
            }
        }
    }

    ListView_SetItemState(hw_lst, 0, LVIS_SELECTED, LVIS_SELECTED);

 _cleanup_selchange:

    if (picons != icbuf)
        PFREE(picons);

    return TRUE;
}

static UINT_PTR CALLBACK
ofn_hook(HWND hwnd,
         UINT uiMsg,
         WPARAM wParam,
         LPARAM lParam)
{
    struct ch_icon_data * c;

    switch (uiMsg) {
    case WM_INITDIALOG:
        {
            OPENFILENAME * ofn;

            ofn = (OPENFILENAME *) lParam;
            c = (struct ch_icon_data *) ofn->lCustData;
            assert(c->magic == CH_ICON_DATA_MAGIC);

#pragma warning(push)
#pragma warning(disable: 4244)
            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) c);
#pragma warning(pop)
        }
        return FALSE;

    case WM_NOTIFY:
        {
            NMHDR * pnmh;
            HWND hw_ofn = NULL; /* OpenFile dialog */
            HWND hw_lst = NULL; /* list view control */

            c = (struct ch_icon_data *)(LONG_PTR) GetWindowLongPtr(hwnd, DWLP_USER);
            if (c == NULL)
                return FALSE;

            assert(c && c->magic == CH_ICON_DATA_MAGIC);

            pnmh = (NMHDR *) lParam;
            hw_ofn = GetParent(hwnd);

            if (pnmh->idFrom == IDC_LIST) {
                hw_lst = GetDlgItem(hwnd, IDC_LIST);
                assert(hw_lst);

                /* Notifications from the icon list */
                switch (pnmh->code) {
                case LVN_ITEMCHANGED:
                    {
                        int sel_idx;
                        LVITEM lvi;

                        sel_idx = ListView_GetNextItem(hw_lst, -1, LVNI_SELECTED);
                        if (sel_idx == -1) {
                            c->index = -1;
                            return TRUE;
                        }

                        lvi.mask = LVIF_PARAM;
                        lvi.iItem = sel_idx;
                        lvi.iSubItem = 0;

                        ListView_GetItem(hw_lst, &lvi);
                        c->index = (int) lvi.lParam;
                    }
                    break;
                }

                return FALSE;
            } else {
                OFNOTIFY * ofn;

                /* Notifications from the OpenFile dialog */
                ofn = (OFNOTIFY *) pnmh;

                switch (pnmh->code) {
                case CDN_FILEOK:
                    {
                        if (c->index == -1) {
                            SetWindowLong(hwnd, DWL_MSGRESULT, TRUE);
                            return TRUE;
                        }
                    }
                    return FALSE;

                case CDN_SELCHANGE:
                    return ofn_load_icons(c, hwnd, hw_ofn);
                }
            }
        }
        return FALSE;
    }

    return 0;
}

khm_boolean
khm_show_select_icon_dialog(HWND hw_parent, wchar_t * path, khm_size cb_path)
{
    OPENFILENAME ofn;
    wchar_t filter[128];
    struct ch_icon_data icondata;
    khm_boolean bSetIcon = FALSE;

    ZeroMemory(&icondata, sizeof(icondata));
    icondata.magic = CH_ICON_DATA_MAGIC;

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hw_parent;
    ofn.hInstance = khm_hInstance;

    {
        wchar_t tfilter[128];
        khm_size cb;

        LoadString(khm_hInstance, IDS_ICON_FILTER,
                   tfilter, ARRAYLENGTH(tfilter));
        cb = sizeof(filter);
        csv_to_multi_string(filter, &cb, tfilter);
        ofn.lpstrFilter = filter;
    }

    parse_resource_path(&icondata, path);

    ofn.lpstrFile = icondata.pathname;
    ofn.nMaxFile = ARRAYLENGTH(icondata.pathname);
    ofn.Flags = OFN_ENABLEHOOK | OFN_ENABLETEMPLATE |
        OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
        OFN_HIDEREADONLY | OFN_NONETWORKBUTTON;
    ofn.lCustData = (LPARAM) &icondata;
    ofn.lpfnHook = ofn_hook;
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_ICON_OFN);

    bSetIcon = GetOpenFileName(&ofn);

    if (bSetIcon) {
        wchar_t resstring[MAX_PATH + MAX_PATH];
        const wchar_t * prefix;
        void unexpand_env_var_prefix(wchar_t * s, size_t cb_s);

        if (icondata.index >= BMP_OFFSET) {
            icondata.restype = KHM_RESTYPE_BITMAP;
            icondata.index -= BMP_OFFSET;
        }

        if (icondata.flags & KHUI_LIFR_FROMLIB) {
            if (icondata.restype == KHM_RESTYPE_BITMAP) {
                prefix = KHUI_PREFIX_IMGDLL;
            } else {
                prefix = KHUI_PREFIX_ICODLL;
            }
        } else {
            if (icondata.restype == KHM_RESTYPE_BITMAP) {
                prefix = KHUI_PREFIX_IMG;
            } else {
                prefix = KHUI_PREFIX_ICO;
            }
        }

        unexpand_env_var_prefix(icondata.pathname, sizeof(icondata.pathname));

        if (icondata.index > 0) {
            StringCbPrintf(resstring, sizeof(resstring), L"%s%s,%d",
                           prefix, icondata.pathname, icondata.index);
        } else {
            StringCbPrintf(resstring, sizeof(resstring), L"%s%s",
                           prefix, icondata.pathname);
        }

        if (FAILED(StringCbCopy(path, cb_path, resstring)))
            bSetIcon = FALSE;
    }

    return bSetIcon;
}

static const wchar_t * _exp_env_vars[] = {
    L"TMP",
    L"TEMP",
    L"USERPROFILE",
    L"ALLUSERSPROFILE",
    L"SystemRoot"
#if 0
    /* Replacing this is not typically useful */
    L"SystemDrive"
#endif
};

const int _n_exp_env_vars = ARRAYLENGTH(_exp_env_vars);

static int
check_and_replace_env_prefix(wchar_t * s, size_t cb_s, const wchar_t * env) {
    wchar_t evalue[MAX_PATH];
    DWORD len;
    size_t sz;

    evalue[0] = L'\0';
    len = GetEnvironmentVariable(env, evalue, ARRAYLENGTH(evalue));
    if (len > ARRAYLENGTH(evalue) || len == 0)
        return 0;

    if (_wcsnicmp(s, evalue, len))
        return 0;

    if (SUCCEEDED(StringCbPrintf(evalue, sizeof(evalue), L"%%%s%%%s",
                                 env, s + len)) &&
        SUCCEEDED(StringCbLength(evalue, sizeof(evalue), &sz)) &&
        sz <= cb_s) {
        StringCbCopy(s, cb_s, evalue);
    }

    return 1;
}

/* We use this instead of PathUnexpandEnvStrings() because that API
   doesn't take process TMP and TEMP.  The string is modified
   in-place.  The usual case is to call it with a buffer big enough to
   hold MAX_PATH characters. */
void
unexpand_env_var_prefix(wchar_t * s, size_t cb_s) {
    int i;

    for (i=0; i < _n_exp_env_vars; i++) {
        if (check_and_replace_env_prefix(s, cb_s, _exp_env_vars[i]))
            return;
    }
}
