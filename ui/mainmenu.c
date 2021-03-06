/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
 * Copyright (c) 2007-2010 Secure Endpoints Inc.
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

HWND khui_main_menu_toolbar;
int mm_last_hot_item = -1;
int mm_next_hot_item = -1;
BOOL mm_hot_track = FALSE;

#define MAX_ILIST 256
/* not the same as MENU_SIZE_ICON_* */
#define ILIST_ICON_X 16
#define ILIST_ICON_Y 15

khui_ilist * il_icon;
int il_icon_id[MAX_ILIST];

void khui_init_menu(void) {
    int i;

    il_icon = khui_create_ilist(ILIST_ICON_X, 
                                ILIST_ICON_Y, 
                                MAX_ILIST, 5, 0);
    for(i=0;i<MAX_ILIST;i++)
        il_icon_id[i] = -1;

    khm_refresh_identity_menus();
}

void khui_exit_menu(void) {
    khui_delete_ilist(il_icon);
}

int khui_get_icon_index(int id) {
    int i;
    HBITMAP hbm;

    for(i=0;i<MAX_ILIST;i++)
        if(il_icon_id[i] == id) {
            return i;
        }

    hbm = LoadImage(khm_hInstance, 
                    MAKEINTRESOURCE(id), 
                    IMAGE_BITMAP, 
                    ILIST_ICON_X, ILIST_ICON_Y, 
                    LR_DEFAULTCOLOR);
    i = khui_ilist_add_masked(il_icon, hbm, KHUI_TOOLBAR_BGCOLOR);
    il_icon_id[i] = id;
    DeleteObject(hbm);

    return i;
}

void khm_get_action_caption(khm_int32 action, wchar_t * buf, khm_size cb_buf) {
    khui_action * act;

    StringCbCopy(buf, cb_buf, L"");

    khui_action_lock();
    act = khui_find_action(action);

    if (act == NULL)
        goto done;

    if (act->caption) {
        StringCbCopy(buf, cb_buf, act->caption);
    } else if (act->is_caption) {
        LoadString(khm_hInstance, act->is_caption,
                   buf, (int)(cb_buf / sizeof(wchar_t)));
    }

 done:
    khui_action_unlock();
}

void khm_get_action_tooltip(khm_int32 action, wchar_t * buf, khm_size cb_buf) {
    khui_action * act;

    StringCbCopy(buf, cb_buf, L"");

    khui_action_lock();
    act = khui_find_action(action);

    if (act == NULL)
        goto done;

    if (act->tooltip) {
        StringCbCopy(buf, cb_buf, act->tooltip);
    } else if (act->is_tooltip) {
        LoadString(khm_hInstance, act->is_tooltip,
                   buf, (int) (cb_buf / sizeof(wchar_t)));
    }

 done:
    khui_action_unlock();
}

void add_action_to_menu(HMENU hm, khui_action * act, 
                        int idx, int flags) {
    MENUITEMINFO mii;
    wchar_t buf[MAX_RES_STRING] = L"";
    wchar_t accel[MAX_RES_STRING] = L"";

    assert(!act || act->cmd);

    mii.cbSize = sizeof(mii);
    mii.fMask = 0;

    if(act == NULL) {
        mii.fMask = MIIM_FTYPE;
        mii.fType = MFT_SEPARATOR;
    } else {
        khui_menu_def * def;

        if (act->type == KHUI_ACTIONTYPE_IDENTITY) {
            mii.fMask = MIIM_FTYPE | MIIM_ID | MIIM_DATA;
            mii.fType = MFT_OWNERDRAW;

            mii.dwTypeData = 0;
            mii.dwItemData = 0;
        } else {
            khm_get_action_caption(act->cmd, buf, sizeof(buf));

            if(khui_get_cmd_accel_string(act->cmd, accel, 
                                         ARRAYLENGTH(accel))) {
                StringCbCat(buf, sizeof(buf), L"\t");
                StringCbCat(buf, sizeof(buf), accel);
            }

            mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
            mii.fType = MFT_STRING;

            mii.dwTypeData = buf;
            mii.cch = (int) wcslen(buf);
        }

        mii.wID = act->cmd;

        if(act->state & KHUI_ACTIONSTATE_DISABLED) {
            mii.fMask |= MIIM_STATE;
            mii.fState = MFS_DISABLED;
        } else {
            mii.fState = 0;
        }

        if((act->type & KHUI_ACTIONTYPE_TOGGLE) && 
           (act->state & KHUI_ACTIONSTATE_CHECKED)) {
            mii.fMask |= MIIM_STATE;
            mii.fState |= MFS_CHECKED;
        }

        if(act->ib_icon) {
            mii.fMask |= MIIM_BITMAP;
            mii.hbmpItem = HBMMENU_CALLBACK;
        }

        if (flags & KHUI_ACTIONREF_SUBMENU) {
            def = khui_find_menu(act->cmd);
            if(def) {
                mii.fMask |= MIIM_SUBMENU;
                mii.hSubMenu = mm_create_menu_from_def(def, FALSE);
            }
        }

        if(flags & KHUI_ACTIONREF_DEFAULT) {
            mii.fMask |= MIIM_STATE;
            mii.fState |= MFS_DEFAULT;
        }
    }

    InsertMenuItem(hm,idx,TRUE,&mii);
}

static void refresh_menu(HMENU hm, khui_menu_def * def);

static int refresh_menu_item(HMENU hm, khui_action * act, 
                             int idx, int flags) {
    MENUITEMINFO mii;
    khui_menu_def * def;

    mii.cbSize = sizeof(mii);
    mii.fMask = 0;

    if (flags & KHUI_ACTIONREF_END) {
        /* we have been asked to assert that the menu doesn't have
           more than idx items */
        mii.fMask = MIIM_FTYPE;
        while (GetMenuItemInfo(hm, idx, TRUE, &mii)) {
            RemoveMenu(hm, idx, MF_BYPOSITION);
            mii.fMask = MIIM_FTYPE;
        }

        return 0;
    }

    if (act == NULL)
        return 0;

    /* Check if the menu item is there.  Otherwise we need to add
       it. */
    mii.fMask = MIIM_STATE | MIIM_ID | MIIM_FTYPE;
    if (!GetMenuItemInfo(hm, idx, TRUE, &mii) ||
        ((flags & KHUI_ACTIONREF_SEP) && !(mii.fType & MFT_SEPARATOR)) ||
        (!(flags & KHUI_ACTIONREF_SEP) && mii.wID != (WORD) act->cmd)) {
        add_action_to_menu(hm, ((flags & KHUI_ACTIONREF_SEP)? NULL : act),
                           idx, flags);
        return 0;
    }

    if (flags & KHUI_ACTIONREF_SEP)
        return 0;

#ifdef DEBUG
    assert(act);
#endif
    if (!act)
        return 0;

    if (flags & KHUI_ACTIONREF_DEFAULT) {
        if (!(mii.fState & MFS_DEFAULT)) {
            mii.fMask |= MIIM_STATE;
            mii.fState |= MFS_DEFAULT;
        }
    } else {
        if (mii.fState & MFS_DEFAULT) {
            RemoveMenu(hm, idx, MF_BYPOSITION);
            add_action_to_menu(hm, act, idx, flags);
            return 0;
        }
    }

    mii.fMask = 0;

    if(act->state & KHUI_ACTIONSTATE_DISABLED) {
        mii.fMask |= MIIM_STATE;
        mii.fState &= ~MFS_ENABLED;
        mii.fState |= MFS_DISABLED;
    } else {
        mii.fMask |= MIIM_STATE;
        mii.fState &= ~MFS_DISABLED;
        mii.fState |= MFS_ENABLED;
    }

    if(act->type & KHUI_ACTIONTYPE_TOGGLE) {
        mii.fMask |= MIIM_STATE;
        if (act->state & KHUI_ACTIONSTATE_CHECKED) {
            mii.fState &= ~MFS_UNCHECKED;
            mii.fState |= MFS_CHECKED;
        } else {
            mii.fState &= ~MFS_CHECKED;
            mii.fState |= MFS_UNCHECKED;
        }
    }

    SetMenuItemInfo(hm, act->cmd, FALSE, &mii);

    def = khui_find_menu(act->cmd);

    if(def) {
        MENUITEMINFO mii2;

        mii2.cbSize = sizeof(mii2);
        mii2.fMask = MIIM_SUBMENU;

        if (GetMenuItemInfo(hm, act->cmd, FALSE, &mii2)) {
            refresh_menu(mii2.hSubMenu, def);
        }
    }

    return 0;
}


static void refresh_menu(HMENU hm, khui_menu_def * def) {
    khui_action_ref * act;
    khm_size i, n;

    for(i = 0, n = khui_menu_get_size(def); i < n; i++) {
        act = khui_menu_get_action(def, i);
        refresh_menu_item(hm, khui_find_action(act->action), (int) i, act->flags);
    }

    refresh_menu_item(hm, NULL, (int) i, KHUI_ACTIONREF_END);
}

static HMENU mm_create_menu_from_def(khui_menu_def * def, BOOL main) {
    HMENU hm;
    khui_action_ref * act;
    khm_size i, n;

    if (main)
        hm = CreateMenu();
    else
        hm = CreatePopupMenu();

    for (i = 0, n = khui_menu_get_size(def); i < n; i++) {
        act = khui_menu_get_action(def, i);
        add_action_to_menu(hm, khui_find_action(act->action), (int) i, act->flags);
    }

    return hm;
}

void mm_begin_hot_track(void);
void mm_end_hot_track(void);

void khm_menu_show_panel_def(khui_menu_def * def, LONG x, LONG y)
{
    HMENU hm;

    hm = mm_create_menu_from_def(def, FALSE);

    mm_hot_track = (mm_last_hot_item >= 0);

    if (mm_hot_track)
        mm_begin_hot_track();

    TrackPopupMenuEx(hm, 
                     TPM_LEFTALIGN | TPM_TOPALIGN | 
                     TPM_VERPOSANIMATION, 
                     x, y, khm_hwnd_main, NULL);

    mm_last_hot_item = -1;

    if (mm_hot_track)
        mm_end_hot_track();

    mm_hot_track = FALSE;

    DestroyMenu(hm);
}

void khm_menu_show_panel(int id, LONG x, LONG y) {
    khui_menu_def * def;

    def = khui_find_menu(id);
    if(!def)
        return;

    khm_menu_show_panel_def(def, x, y);
}

LRESULT khm_menu_activate(int menu_id) {
    khui_menu_def * mmdef;
    int nmm;

    mmdef = khui_find_menu(KHUI_MENU_MAIN);
    nmm = (int) khui_action_list_length(mmdef->items);

    if(menu_id == MENU_ACTIVATE_DEFAULT) {
        if (mm_last_hot_item != -1)
            menu_id = mm_last_hot_item;
        else
            menu_id = 0;
    } else if(menu_id == MENU_ACTIVATE_LEFT) {
        menu_id = (mm_last_hot_item > 0)? 
            mm_last_hot_item - 1: 
            ((mm_last_hot_item == 0)? nmm - 1: 0);
    } else if(menu_id == MENU_ACTIVATE_RIGHT) {
        menu_id = (mm_last_hot_item >=0 && mm_last_hot_item < nmm - 1)? 
            mm_last_hot_item + 1: 
            0;
    } else if(menu_id == MENU_ACTIVATE_NONE) {
        menu_id = -1;
    }
    
    SendMessage(khui_main_menu_toolbar,
                TB_SETHOTITEM,
                menu_id,
                0);

    khm_menu_track_current();

    return TRUE;
}

LRESULT khm_menu_measure_item(WPARAM wParam, LPARAM lParam) {
    LPMEASUREITEMSTRUCT lpm = (LPMEASUREITEMSTRUCT) lParam;
    khui_action * act;

    act = khui_find_action(lpm->itemID);
    if (act && act->type == KHUI_ACTIONTYPE_IDENTITY) {
        khm_measure_identity_menu_item(khm_hwnd_main_cred, lpm, act);
    } else {
        lpm->itemWidth = MENU_SIZE_ICON_X;
        lpm->itemHeight = MENU_SIZE_ICON_Y;
    }
    return TRUE;
}

LRESULT khm_menu_draw_item(WPARAM wParam, LPARAM lParam) {
    LPDRAWITEMSTRUCT lpd;
    khui_action * act;

    lpd = (LPDRAWITEMSTRUCT) lParam;
    act = khui_find_action(lpd->itemID);

    if (act && act->type == KHUI_ACTIONTYPE_IDENTITY) {

        khm_draw_identity_menu_item(khm_hwnd_main_cred, lpd, act);

    } else {
        int resid;
        int iidx;
        UINT style;

        resid = 0;
        if((lpd->itemState & ODS_DISABLED) || (lpd->itemState & ODS_GRAYED)) {
            resid = act->ib_icon_dis;
        }
        if(!resid)
            resid = act->ib_icon;

        if(!resid) /* nothing to draw */
            return TRUE;

    
        iidx = khui_get_icon_index(resid);
        if(iidx == -1)
            return TRUE;


        style = ILD_TRANSPARENT;
        if(lpd->itemState & ODS_HOTLIGHT || lpd->itemState & ODS_SELECTED) {
            style |= ILD_SELECTED;
        }
    
        khui_ilist_draw(il_icon, 
                        iidx, 
                        lpd->hDC, 
                        lpd->rcItem.left, lpd->rcItem.top, style);
    }

    return TRUE;
}

void khm_track_menu(int menu) {
    TBBUTTON bi;
    RECT r;
    RECT wr;

    if (menu != -1)
        mm_last_hot_item = menu;

    if (mm_last_hot_item != -1) {
        SendMessage(khui_main_menu_toolbar,
                    TB_GETBUTTON,
                    mm_last_hot_item,
                    (LPARAM) &bi);

        SendMessage(khui_main_menu_toolbar,
                    TB_GETITEMRECT,
                    mm_last_hot_item,
                    (LPARAM) &r);

        GetWindowRect(khui_main_menu_toolbar, &wr);

        khm_menu_show_panel(bi.idCommand, wr.left + r.left, wr.top + r.bottom);

        r.left = 0;

        if (mm_next_hot_item != -1) {
            mm_last_hot_item = mm_next_hot_item;
            mm_next_hot_item = -1;

            PostMessage(khm_hwnd_main, WM_COMMAND, 
                        MAKEWPARAM(KHUI_PACTION_MENU,0),
                        MAKELPARAM(mm_last_hot_item,1));
        }
    }
}

void khm_menu_track_current(void) {
    khm_track_menu(-1);
}

LRESULT khm_menu_handle_select(WPARAM wParam, LPARAM lParam) {
    if((HIWORD(wParam) == 0xffff && lParam == 0) || 
       (HIWORD(wParam) & MF_POPUP)) {
        /* the menu was closed */
        khm_statusbar_set_part(KHUI_SBPART_INFO, NULL, NULL);
    } else {
        khui_action * act;
        int id;
        wchar_t buf[MAX_RES_STRING] = L"";

        id = LOWORD(wParam);
        act = khui_find_action(id);
        if(act == NULL || (act->is_tooltip == 0 && act->tooltip == NULL))
            khm_statusbar_set_part(KHUI_SBPART_INFO, NULL, NULL);
        else {
            khm_get_action_tooltip(act->cmd, buf, sizeof(buf));

            khm_statusbar_set_part(KHUI_SBPART_INFO, NULL, buf);
        }
    }
    return 0;
}

HHOOK mm_hevt_hook = NULL;
HWND mm_hwnd_menu_panel = NULL;

LRESULT CALLBACK mm_event_filter(int code,
                                 WPARAM wParam,
                                 LPARAM lParam) {
    MSG * m;
    RECT r;
    int x,y;

    if (code == MSGF_MENU) {
        /* do stuff */
        m = (MSG *) lParam;
        GetWindowRect(khui_main_menu_toolbar, &r);

        if (m->hwnd != khm_hwnd_main)
            mm_hwnd_menu_panel = m->hwnd;

        switch(m->message) {
        case WM_MOUSEMOVE:

            x = GET_X_LPARAM(m->lParam);
            y = GET_Y_LPARAM(m->lParam);
            x -= r.left;
            y -= r.top;

            SendMessage(khui_main_menu_toolbar,
                        m->message,
                        m->wParam,
                        MAKELPARAM(x,y));
            break;
        }
    }

    return CallNextHookEx(mm_hevt_hook, code, wParam, lParam);
}


void mm_begin_hot_track(void) {

    if (mm_hevt_hook)
        UnhookWindowsHookEx(mm_hevt_hook);

    mm_hevt_hook = SetWindowsHookEx(WH_MSGFILTER,
                                    mm_event_filter,
                                    NULL,
                                    GetCurrentThreadId());
}

void mm_end_hot_track(void) {
    if (mm_hevt_hook)
        UnhookWindowsHookEx(mm_hevt_hook);

    mm_hevt_hook = NULL;
    mm_hwnd_menu_panel = NULL;
}

void mm_cancel_menu(void) {
    if (mm_hwnd_menu_panel)
        SendMessage(mm_hwnd_menu_panel, WM_CANCELMODE, 0, 0);
}

LRESULT khm_menu_notify_main(LPNMHDR notice) {
    LPNMTOOLBAR nmt;
    LRESULT ret = FALSE;
    RECT r;
    khui_menu_def * mmdef;
    khui_action_ref * mm;
    int nmm;

    mmdef = khui_find_menu(KHUI_MENU_MAIN);
    mm = mmdef->items;
    nmm = (int) khui_action_list_length(mm);

    GetWindowRect(khui_main_menu_toolbar, &r);

    nmt = (LPNMTOOLBAR) notice;
    switch(notice->code) {
    case TBN_DROPDOWN:
        khm_track_menu(-1);
        /*
        khm_menu_show_panel(nmt->iItem, 
                        r.left + nmt->rcButton.left, 
                        r.top + nmt->rcButton.bottom);
        */
        ret = TBDDRET_DEFAULT;
        break;

    case TBN_HOTITEMCHANGE:
        {
            LPNMTBHOTITEM nmhi;
            int new_item = -1;

            nmhi = (LPNMTBHOTITEM) notice;

            if(nmhi->dwFlags & HICF_LEAVING)
                new_item = -1;
            else {
                int i;
                for(i=0; i < nmm; i++) {
                    if(mm[i].action == nmhi->idNew) {
                        new_item = i;
                        break;
                    }
                }
            }

            if (mm_hot_track && 
                new_item != mm_last_hot_item &&
                new_item != -1 &&
                mm_last_hot_item != -1) {

                EndMenu();
                mm_next_hot_item = new_item;

            }

            ret = 0;

            if (!mm_hot_track || new_item != -1)
                mm_last_hot_item = new_item;

        } break;

    default:
        /* hmm. what to do */
        ret = FALSE;
    }
    return ret;
}

struct identity_action_map {
    khm_handle identity;
    khm_int32  renew_cmd;
    khm_int32  destroy_cmd;
    khm_int32  new_cmd;
    khm_int32  setdef_cmd;
    khm_int32  config_cmd;
    int        refreshcycle;
};

#define IDMAP_ALLOC_INCR 8

struct identity_action_map * id_action_map = NULL;
khm_size n_id_action_map  = 0;
khm_size nc_id_action_map = 0;

int idcmd_refreshcycle = 0;

static struct identity_action_map *
create_identity_cmd_map(khm_handle ident) {

    struct identity_action_map * actmap;
    wchar_t idshortname[KCDB_IDENT_MAXCCH_NAME];
    wchar_t displayname[KCDB_IDENT_MAXCCH_NAME];
    wchar_t fmt[128];
    wchar_t caption[KHUI_MAXCCH_SHORT_DESC];
    wchar_t tooltip[KHUI_MAXCCH_SHORT_DESC];
    wchar_t actionname[KHUI_MAXCCH_NAME];
    khm_size cb;

    if (n_id_action_map + 1 > nc_id_action_map) {
        nc_id_action_map = UBOUNDSS(n_id_action_map + 1,
                                    IDMAP_ALLOC_INCR,
                                    IDMAP_ALLOC_INCR);
#ifdef DEBUG
        assert(nc_id_action_map > n_id_action_map + 1);
#endif
        id_action_map = PREALLOC(id_action_map,
                                 nc_id_action_map * sizeof(id_action_map[0]));
#ifdef DEBUG
        assert(id_action_map);
#endif
        ZeroMemory(&id_action_map[n_id_action_map],
                   sizeof(id_action_map[0]) * (nc_id_action_map - n_id_action_map));
    }

    actmap = &id_action_map[n_id_action_map];
    n_id_action_map++;

    cb = sizeof(displayname);
    kcdb_get_resource(ident, KCDB_RES_DISPLAYNAME, 0, NULL, NULL, displayname, &cb);

    cb = sizeof(idshortname);
    kcdb_identity_get_short_name(ident, FALSE, idshortname, &cb);

    actmap->identity = ident;
    kcdb_identity_hold(ident);

#define GETFORMAT(I) do { fmt[0] = L'\0'; LoadString(khm_hInstance, I, fmt, ARRAYLENGTH(fmt)); } while(0)
#define EXPFORMAT(d,s) do { StringCbPrintf(d, sizeof(d), fmt, s); } while(0)
    /* renew */

    GETFORMAT(IDS_IDACTIONT_RENEW);
    EXPFORMAT(tooltip, displayname);

    GETFORMAT(IDS_IDACTION_RENEW);
    EXPFORMAT(caption, displayname);

    StringCbPrintf(actionname, sizeof(actionname), L"R:%s", idshortname);

    actmap->renew_cmd =
        khui_action_create(actionname, caption, tooltip, NULL,
                           KHUI_ACTIONTYPE_TRIGGER, NULL);

    /* destroy */

    GETFORMAT(IDS_IDACTIONT_DESTROY);
    EXPFORMAT(tooltip, displayname);

    GETFORMAT(IDS_IDACTION_DESTROY);
    EXPFORMAT(caption, displayname);

    StringCbPrintf(actionname, sizeof(actionname), L"D:%s", idshortname);

    actmap->destroy_cmd =
        khui_action_create(actionname, caption, tooltip, NULL,
                           KHUI_ACTIONTYPE_TRIGGER, NULL);

    /* new */

    GETFORMAT(IDS_IDACTIONT_NEW);
    EXPFORMAT(tooltip, displayname);

    GETFORMAT(IDS_IDACTION_NEW);
    EXPFORMAT(caption, displayname);

    StringCbPrintf(actionname, sizeof(actionname), L"N:%s", idshortname);

    actmap->new_cmd =
        khui_action_create(actionname, caption, tooltip, NULL,
                           KHUI_ACTIONTYPE_TRIGGER, NULL);

    /* set default */
    GETFORMAT(IDS_IDACTIONT_SETDEF);
    EXPFORMAT(tooltip, displayname);

    GETFORMAT(IDS_IDACTION_SETDEF);
    EXPFORMAT(caption, displayname);

    StringCbPrintf(actionname, sizeof(actionname), L"E:%s", idshortname);

    actmap->setdef_cmd =
        khui_action_create(actionname, caption, tooltip, ident,
                           KHUI_ACTIONTYPE_IDENTITY, NULL);

    /* config */
    GETFORMAT(IDS_IDACTIONT_CONFIG);
    EXPFORMAT(tooltip, displayname);

    GETFORMAT(IDS_IDACTION_CONFIG);
    EXPFORMAT(caption, displayname);

    StringCbPrintf(actionname, sizeof(actionname), L"C:%s", idshortname);

    actmap->config_cmd =
        khui_action_create(actionname, caption, tooltip, ident,
                           KHUI_ACTIONTYPE_IDENTITY, NULL);

    actmap->refreshcycle = idcmd_refreshcycle;

#undef GETFORMAT
#undef EXPFORMAT

    return actmap;
}

static void
purge_identity_cmd_map(void) {
    khm_size i;

    for (i=0; i < n_id_action_map; i++) {
        khm_handle ident;

        if (id_action_map[i].refreshcycle != idcmd_refreshcycle) {
            ident = id_action_map[i].identity;
            id_action_map[i].identity = NULL;
            kcdb_identity_release(ident);

            khui_action_delete(id_action_map[i].renew_cmd);
            khui_action_delete(id_action_map[i].destroy_cmd);
            khui_action_delete(id_action_map[i].new_cmd);
            khui_action_delete(id_action_map[i].setdef_cmd);
            khui_action_delete(id_action_map[i].config_cmd);

            id_action_map[i].renew_cmd = 0;
            id_action_map[i].destroy_cmd = 0;
            id_action_map[i].new_cmd = 0;
            id_action_map[i].setdef_cmd = 0;
            id_action_map[i].config_cmd = 0;
        }
    }
}

static struct identity_action_map *
get_identity_cmd_map(khm_handle ident) {
    khm_size i;

    for (i=0; i < n_id_action_map; i++) {
        if (kcdb_identity_is_equal(id_action_map[i].identity,
                                   ident))
            break;
    }

    if (i < n_id_action_map) {
        id_action_map[i].refreshcycle = idcmd_refreshcycle;
        return &id_action_map[i];
    } else {
        return create_identity_cmd_map(ident);
    }
}

khm_int32
khm_get_identity_renew_action(khm_handle ident) {
    struct identity_action_map * map;

    map = get_identity_cmd_map(ident);

    if (map)
        return map->renew_cmd;
    else
        return 0;
}

khm_int32
khm_get_identity_destroy_action(khm_handle ident) {
    struct identity_action_map * map;

    map = get_identity_cmd_map(ident);

    if (map)
        return map->destroy_cmd;
    else
        return 0;
}

khm_int32
khm_get_identity_setdef_action(khm_handle ident) {
    struct identity_action_map * map;

    map = get_identity_cmd_map(ident);

    if (map)
        return map->setdef_cmd;
    else
        return 0;
}

khm_int32
khm_get_identity_new_creds_action(khm_handle ident) {
    struct identity_action_map * map;

    map = get_identity_cmd_map(ident);

    if (map)
        return map->new_cmd;
    else
        return 0;
}

khm_int32
khm_get_identity_config_action(khm_handle ident) {
    struct identity_action_map * map;

    map = get_identity_cmd_map(ident);

    if (map)
        return map->config_cmd;
    else
        return 0;
}

static khm_boolean KHMCALLBACK filter_for_viable_identities(khm_handle h,
                                                            void * v)
{
    khm_int32 flags = 0;

    return (KHM_SUCCEEDED(kcdb_identity_get_flags(h, &flags)) &&
            (!(flags & KCDB_IDENT_FLAG_EMPTY) ||
             (flags & KCDB_IDENT_FLAG_CONFIG) ||
             (flags & KCDB_IDENT_FLAG_DEFAULT)));
}

void
khm_refresh_identity_menus(void) {
    khui_menu_def * renew_def = NULL;
    khui_menu_def * dest_def = NULL;
    khui_menu_def * setdef_def = NULL;
    khui_menu_def * new_def = NULL;
    kcdb_enumeration id_enum = NULL;
    khm_size cb = 0;
    khm_size n_idents = 0;
    khm_size t;
    khm_int32 rv = KHM_ERROR_SUCCESS;
    khm_handle csp_cw = NULL;
    khm_int32 idflags;
    khm_int32 def_sticky = 0;
    khm_int32 all_identities = 0;
    khm_boolean sticky_done = FALSE;
    khm_boolean added_dest = FALSE;
    khm_boolean added_setdef = FALSE;
    khm_handle identity = NULL;

    if (KHM_SUCCEEDED(khc_open_space(NULL, L"CredWindow", 0, &csp_cw))) {
        khc_read_int32(NULL, L"KCDB\\DefaultSticky", &def_sticky);
        khc_read_int32(csp_cw, L"ViewAllIdents", &all_identities);
        khc_close_space(csp_cw);
        csp_cw = NULL;
    }

    kcdb_identity_refresh_all();

    khui_action_lock();

    idcmd_refreshcycle++;

    if (KHM_FAILED(kcdb_identity_begin_enum(KCDB_IDENT_FLAG_ACTIVE,
                                            KCDB_IDENT_FLAG_ACTIVE,
                                            &id_enum, &n_idents)))
        n_idents = 0;

    if (n_idents > 0) {
        kcdb_enum_filter(id_enum, filter_for_viable_identities, NULL);
        kcdb_enum_get_size(id_enum, &n_idents);
    }

    renew_def = khui_find_menu(KHUI_MENU_RENEW_CRED);
    dest_def = khui_find_menu(KHUI_MENU_DESTROY_CRED);
    setdef_def = khui_find_menu(KHUI_MENU_SETDEF);
    new_def = khui_find_menu(KHUI_MENU_NEW_CRED);
    assert(renew_def);
    assert(dest_def);
    assert(setdef_def);
    assert(new_def);

    t = khui_menu_get_size(renew_def);
    while(t) {
        khui_menu_remove_action(renew_def, 0);
        t--;
    }

    t = khui_menu_get_size(dest_def);
    while(t) {
        khui_menu_remove_action(dest_def, 0);
        t--;
    }

    t = khui_menu_get_size(setdef_def);
    while(t) {
        khui_menu_remove_action(setdef_def, 0);
        t--;
    }

    t = khui_menu_get_size(new_def);
    while (t) {
        khui_menu_remove_action(new_def, 0);
        t--;
    }

    if (n_idents > 1) {
        khui_menu_insert_action(renew_def, 0, KHUI_ACTION_RENEW_ALL, 0);
        khui_menu_insert_action(renew_def, 1, KHUI_MENU_SEP, 0);

        khui_menu_insert_action(dest_def, 0, KHUI_ACTION_DESTROY_ALL, 0);
        khui_menu_insert_action(dest_def,  1, KHUI_MENU_SEP, 0);
    }

    khui_menu_insert_action(new_def, 0, KHUI_ACTION_NEW_CRED, 0);
    khui_menu_insert_action(new_def, 1, KHUI_MENU_SEP, 0);

    while (id_enum && KHM_SUCCEEDED(kcdb_enum_next(id_enum, &identity))) {

        kcdb_identity_get_flags(identity, &idflags);

        if (!(idflags & KCDB_IDENT_FLAG_STICKY) && def_sticky) {
            kcdb_identity_set_flags(identity,
                                    KCDB_IDENT_FLAG_STICKY,
                                    KCDB_IDENT_FLAG_STICKY);
            idflags |= KCDB_IDENT_FLAG_STICKY;
            sticky_done = TRUE;
        }

        if (!(idflags & KCDB_IDENT_FLAG_EMPTY)) {
            if (idflags & KCDB_IDENT_FLAG_RENEWABLE)
                khui_menu_insert_action(renew_def, 1000,
                                        khm_get_identity_renew_action(identity),
                                        0);

            khui_menu_insert_action(dest_def, 1000,
                                    khm_get_identity_destroy_action(identity),
                                    0);
            added_dest = TRUE;
        }

        if (all_identities ||
            (idflags & KCDB_IDENT_FLAG_STICKY) ||
            (idflags & KCDB_IDENT_FLAG_DEFAULT)) {

            khui_menu_insert_action(setdef_def, 1000,
                                    khm_get_identity_setdef_action(identity),
                                    0);
            khui_menu_insert_action(new_def, 1000,
                                    khm_get_identity_new_creds_action(identity),
                                    0);
            added_setdef = TRUE;
        }

        /* For each active identity, we should maintain the new
           credentials action (and the action map entry).  Otherwise
           new credentials actions used in menus and alerts would stop
           working when the credentials associated with an identity
           are destroyed. */
        khm_get_identity_new_creds_action(identity);
    }

    if (added_dest) {
        khui_enable_action(KHUI_MENU_RENEW_CRED, TRUE);
        khui_enable_action(KHUI_MENU_DESTROY_CRED, TRUE);
        khui_enable_action(KHUI_ACTION_RENEW_CRED, TRUE);
        khui_enable_action(KHUI_ACTION_DESTROY_CRED, TRUE);
    } else {
        khui_enable_action(KHUI_MENU_RENEW_CRED, FALSE);
        khui_enable_action(KHUI_MENU_DESTROY_CRED, FALSE);
        khui_enable_action(KHUI_ACTION_RENEW_CRED, FALSE);
        khui_enable_action(KHUI_ACTION_DESTROY_CRED, FALSE);
    }

    if (added_setdef) {
        khui_enable_action(KHUI_MENU_SETDEF, TRUE);
    } else {
        khui_enable_action(KHUI_MENU_SETDEF, FALSE);
    }

    purge_identity_cmd_map();

    khui_action_unlock();

    khui_refresh_actions();

    if (sticky_done) {
        InvalidateRect(khm_hwnd_main_cred, NULL, TRUE);
    }

    if (id_enum != NULL)
        kcdb_enum_end(id_enum);
}

khm_boolean
khm_check_identity_menu_action(khm_int32 act_id) {

    if (act_id == KHUI_ACTION_DESTROY_ALL) {
        khm_size i;

        for (i=0; i < n_id_action_map; i++) {
            if (id_action_map[i].identity != NULL) {
                khm_cred_destroy_identity(id_action_map[i].identity);
            }
        }

        return TRUE;
    } else if (act_id == KHUI_ACTION_RENEW_ALL) {
        khm_size i;

        for (i=0; i < n_id_action_map; i++) {
            if (id_action_map[i].identity != NULL) {
                khm_cred_renew_identity(id_action_map[i].identity);
            }
        }

        return TRUE;
    } else {
        khm_size i;

        for (i=0; i < n_id_action_map; i++) {
            if (id_action_map[i].identity == NULL)
                continue;

            if (id_action_map[i].renew_cmd == act_id) {
                khm_cred_renew_identity(id_action_map[i].identity);
                return TRUE;
            }

            if (id_action_map[i].destroy_cmd == act_id) {
                khm_cred_destroy_identity(id_action_map[i].identity);
                return TRUE;
            }

            if (id_action_map[i].new_cmd == act_id) {
                khm_cred_obtain_new_creds_for_ident(id_action_map[i].identity,
                                                    NULL);
                return TRUE;
            }

            if (id_action_map[i].setdef_cmd == act_id) {
                khm_cred_set_default_identity(id_action_map[i].identity);

                return TRUE;
            }

            if (id_action_map[i].config_cmd == act_id) {
                khm_show_identity_config_pane(id_action_map[i].identity);

                return TRUE;
            }
        }
    }

    return FALSE;
}


HMENU khui_hmenu_main = NULL;

void khm_menu_refresh_items(void) {
    khui_menu_def * def;

    if (!khui_hmenu_main)
        return;

    khui_action_lock();

    def = khui_find_menu(KHUI_MENU_MAIN);

    refresh_menu(khui_hmenu_main, def);

    khui_action_unlock();

    DrawMenuBar(khm_hwnd_main);
}

void khm_menu_create_main(HWND parent) {
    HMENU hmenu;
    khui_menu_def * def;

    def = khui_find_menu(KHUI_MENU_MAIN);

    hmenu = mm_create_menu_from_def(def, TRUE);

    SetMenu(parent, hmenu);

    khui_hmenu_main = hmenu;

    return;
}
