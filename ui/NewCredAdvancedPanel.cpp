
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
