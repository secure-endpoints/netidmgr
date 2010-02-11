/*
 * Copyright (c) 2009-2010 Secure Endpoints Inc.
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

#include "khmapp.h"
#include "NewCredWizard.hpp"

namespace nim {

void NewCredAdvancedPanel::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
    if (id == IDC_NC_ADVANCED && codeNotify == BN_CLICKED)
        NewCredWizard::FromNC(nc)->Navigate( NC_PAGE_CREDOPT_BASIC );
}

LRESULT NewCredAdvancedPanel::OnNotify(int id, NMHDR * pnmh)
{
    if (pnmh->code == TCN_SELCHANGE) {
        NewCredWizard::FromNC(nc)->m_privint.UpdateLayout();
    }
    return 0;
}

void NewCredAdvancedPanel::InitializeTabs()
{
    wchar_t desc[KCDB_MAXCCH_SHORT_DESC] = L"";
    khm_size i;
    HWND hw_tab = GetTabControl();
    khui_new_creds_privint_panel * p;

    TabCtrl_DeleteAllItems(hw_tab);

    khui_cw_lock_nc(nc);

    p = khui_cw_get_current_privint_panel(nc);

#pragma warning(push)
#pragma warning(disable: 4204)

    if (p != NULL) {
        TCITEM tci = { TCIF_PARAM | TCIF_TEXT, 0, 0, p->caption, 0, 0, NC_PRIVINT_PANEL };
        TabCtrl_InsertItem(hw_tab, 0, &tci);
    }

    for (i=0; i < nc->n_types; i++) {
        TCITEM tci = { TCIF_PARAM | TCIF_TEXT, 0, 0, nc->types[i].nct->name,
                       0, 0, i };

        /* All the enabled types are at the front of the list */
        if (nc->types[i].nct->flags & KHUI_NCT_FLAG_DISABLED)
            break;

        if (!tci.pszText) {
            khm_size cb;

            cb = sizeof(desc);
            kcdb_get_resource(KCDB_HANDLE_FROM_CREDTYPE(nc->types[i].nct->type),
                              KCDB_RES_DESCRIPTION, KCDB_RFS_SHORT,
                              NULL, NULL, desc, &cb);
            tci.pszText = desc;
        }

        TabCtrl_InsertItem(hw_tab, i + 1, &tci);
    }

#pragma warning(pop)

    nc->privint.initialized = TRUE;
    khui_cw_unlock_nc(nc);

    TabCtrl_SetCurSel(hw_tab, 0);
}

int  NewCredAdvancedPanel::GetCurrentTabId()
{
    TCITEM tci;
    int ctab;
    HWND hw_tab;

    hw_tab = GetTabControl();
    ctab = TabCtrl_GetCurSel(hw_tab);

    ZeroMemory(&tci, sizeof(tci));

    tci.mask = TCIF_PARAM;
    TabCtrl_GetItem(hw_tab, ctab, &tci);

    return (int) tci.lParam;
}

void  NewCredAdvancedPanel::GetTabPlacement(RECT& r)
{
    HWND hw_tab = GetTabControl();
    RECT r_t;

    GetWindowRect(hw_tab, &r_t);
    MapWindowRect(NULL, hwnd, &r_t);

    TabCtrl_AdjustRect(hw_tab, FALSE, &r_t);

    r = r_t;
}

}
