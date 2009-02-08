
#include "khmapp.h"
#include "NewCredWizard.hpp"
#include <assert.h>

namespace nim {

    void NewCredIdentitySpecifier::Initialize(HWND hw_list)
    {
            khm_size i;
            HIMAGELIST ilist = NULL;

            ListView_DeleteAllItems(hw_list);
            ilist = ImageList_Create(GetSystemMetrics(SM_CXICON),
                                     GetSystemMetrics(SM_CYICON),
                                     ILC_COLORDDB|ILC_MASK,
                                     4, 4);

            {
                HIMAGELIST ilist_old = NULL;

                ilist_old = ListView_SetImageList(hw_list, ilist, LVSIL_NORMAL);

                if (ilist_old)
                    ImageList_Destroy(ilist_old);
            }
            
            LVITEM lvi;
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
            
            idx_current = -1;
            initialized = TRUE;
    }

    void NewCredIdentitySpecifier::UpdateLayout()
    {
        HWND hw_list;
        khm_ssize idx;

        hw_list = GetDlgItem(hwnd, IDC_IDPROVLIST);

        in_layout = TRUE;

        if (nc->n_providers == 0) {
            for (IdentityProvider::Enumeration e = IdentityProvider::Enum();
                 !e.AtEnd(); ++e) {

                khui_cw_add_provider(nc, *e);
            }
        }

        assert(nc->n_providers > 0);

        /* If the identity provider list hasn't been initialized, we
           should do so now. */
        if (!initialized)
            Initialize(hw_list);

        if (ListView_GetSelectedCount(hw_list) != 1 ||
            (idx = ListView_GetNextItem(hw_list, -1, LVNI_SELECTED)) == -1) {

            idx = -1;

            /* The number of selected items is not 1.  We have to
               pretend that there are no providers selected. */

        } else {
            LVITEM lvi;

            lvi.mask = LVIF_PARAM;
            lvi.iItem = (int) idx;
            lvi.iSubItem = 0;

            ListView_GetItem(hw_list, &lvi);
            idx = (int) lvi.lParam;

        }

        if (idx != idx_current) {

            if (idx_current >= 0 &&
                idx_current < (khm_ssize) nc->n_providers) {

                if (nc->providers[idx_current].hwnd_panel != NULL) {
                    SetWindowPos(nc->providers[idx_current].hwnd_panel, NULL,
                                 0, 0, 0, 0, SWP_HIDEONLY);
                }

            } else {

                SetWindowPos(GetDlgItem(hwnd, IDC_NC_R_IDSPEC), NULL,
                             0, 0, 0, 0, SWP_HIDEONLY);

            }

            if (idx >= 0 && idx < (khm_ssize) nc->n_providers) {
                HWND hw_r;
                RECT r;

                if (nc->providers[idx].hwnd_panel == NULL) {

                    if (nc->providers[idx].cb == NULL) {
                        kcdb_identpro_get_idsel_factory(nc->providers[idx].h,
                                                        &nc->providers[idx].cb);
                        assert(nc->providers[idx].cb != NULL);
                    }

                    if (nc->providers[idx].cb != NULL) {

                        idx_current = idx;
                        (*nc->providers[idx].cb)(hwnd,
                                                 &nc->providers[idx].hwnd_panel);
                        assert(nc->providers[idx].hwnd_panel);
                    }
                }

                hw_r = GetDlgItem(hwnd, IDC_NC_R_IDSPEC);
                GetWindowRect(hw_r, &r);
                MapWindowPoints(NULL, hwnd, (LPPOINT) &r, sizeof(r)/sizeof(POINT));

                SetWindowPos(nc->providers[idx].hwnd_panel, hw_r,
                             r.left, r.top, r.right - r.left, r.bottom - r.top,
                             SWP_MOVESIZEZ);
            } else {
                /* Show the placeholder window */
                SetWindowPos(GetDlgItem(hwnd, IDC_NC_R_IDSPEC), NULL,
                             0, 0, 0, 0, SWP_SHOWONLY);
            }

            idx_current = idx;
        }

        in_layout = FALSE;
    }

    bool NewCredIdentitySpecifier::ProcessNewIdentity()
    {
        khui_new_creds_idpro * p;

        if (idx_current >= 0 &&
            idx_current < (khm_ssize) nc->n_providers) {
            khm_handle ident = NULL;

            p = &nc->providers[idx_current];

            assert(p->hwnd_panel);

            SendMessage(p->hwnd_panel, KHUI_WM_NC_NOTIFY, MAKEWPARAM(0, WMNC_IDSEL_GET_IDENT),
                        (LPARAM) &ident);

            if (ident) {
                khui_cw_set_primary_id(nc, ident);
                kcdb_identity_release(ident);

                return true;
            }
        }

        return false;
    }

    LRESULT NewCredIdentitySpecifier::OnNotify(int id, NMHDR * pnmh)
    {
        if (pnmh->code == LVN_ITEMCHANGED && !in_layout) {
            NMLISTVIEW * nmlv = (NMLISTVIEW *) pnmh;

            if ((nmlv->uNewState ^ nmlv->uOldState) & LVIS_SELECTED)
                UpdateLayout();
        }
        return 0;
    }
}
