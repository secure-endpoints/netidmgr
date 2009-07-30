
#include "khmapp.h"
#include "NewCredWizard.hpp"
#include <assert.h>

namespace nim {

    void        NewCredIdentitySelector::UpdateLayout()
    {
        khui_cw_lock_nc(nc);

        /* If there are no identities selected, we should clear out all
           the display data that we have cached. */

        if (nc->n_identities > 0) {
            Identity primary_id(nc->identities[0], false);

            if (m_current &&
                m_current->identity != primary_id) {
                delete m_current;
                m_current = NULL;
            }

            m_current = new IdentityDisplayData(primary_id);
        } else {
            if (m_current) {
                delete m_current;
                m_current = NULL;
            }
        }
        khui_cw_unlock_nc(nc);

        EnableWindow(GetItem(IDC_IDSEL),
                     NewCredWizard::FromNC(nc)->page != NC_PAGE_PROGRESS);

        Invalidate();
    }

    void        NewCredIdentitySelector::SetStatus(const wchar_t * status)
    {
        if (m_current) {
            if (status)
                m_current->status = status;
            else
                m_current->status = L"";

            Invalidate();
        }
    }

    void        NewCredIdentitySelector::ShowIdentitySelector()
    {
        IdentityDisplayData ** ids = NULL;
        khm_size n_ids, i;
        bool seen_current = false;
        HMENU hm = NULL;

        Identity::Enumeration e = Identity::Enum(KCDB_IDENT_FLAG_CONFIG,
                                                 KCDB_IDENT_FLAG_CONFIG);
        n_ids = e.Size() + 2;
        ids = reinterpret_cast<IdentityDisplayData **>(PMALLOC(n_ids * sizeof(ids[0])));
        if (ids == NULL) {
            assert(FALSE);
            return;
        }

        i = 0;

        ids[0] = new IdentityDisplayData();

        ids[0]->icon = LoadIconResource(IDI_ID_NEW);
        ids[0]->display_string = LoadStringResource(IDS_NC_NEW_IDENT);
        ids[0]->status = LoadStringResource(IDS_NC_NEW_IDENT_D);
        ids[0]->type_string = LoadStringResource(IDS_NC_IDSEL_PROMPT);

        i++;

        Identity current_id( (nc->n_identities > 0)? nc->identities[0] : NULL, false );

        for (; !e.AtEnd(); ++e) {
            assert(i < n_ids);

            ids[i++] = new IdentityDisplayData(*e);

            if (*e == current_id)
                seen_current = true;
        }

        if (!seen_current && current_id.IsValid()) {
            ids[i++] = new IdentityDisplayData(current_id);
        }

        n_ids = i;

        hm = CreatePopupMenu();

        for (i=0; i < n_ids; i++) {
            UINT flags = 0;

            if (i > 0 && ids[i]->identity == current_id)
                flags = MF_DISABLED;

            AppendMenu(hm, (flags | MF_OWNERDRAW), (UINT_PTR) i+1, (LPCTSTR) ids[i]);
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
            khui_cw_set_primary_id(nc, ids[i]->identity);
        } else if (i == 1) {
            /* Navigate to the identity specification window if the user
               wants to specify a new identity. */
            reinterpret_cast<NewCredWizard *>(nc->wizard)->Navigate( NC_PAGE_IDSPEC );
        }

    _cleanup:
        if (ids != NULL) {
            for (i=0; i < n_ids; i++) {
                delete ids[i];
            }
            PFREE(ids);
            ids = NULL;
        }
    }

    LRESULT     NewCredIdentitySelector::OnNotifyIdentitySelector(NMHDR * pnmh)
    {
        LPNMCUSTOMDRAW pcd;

        switch (pnmh->code) {
        case NM_CUSTOMDRAW:

            /* We are drawing the identity selector button.  The
               button face contains an icon representing the selected
               primary identity, the display stirng for the primary
               identity, a status string and a chevron. */

            pcd = (LPNMCUSTOMDRAW) pnmh;
            switch (pcd->dwDrawStage) {
            case CDDS_PREERASE:
            case CDDS_PREPAINT:
                return (CDRF_NOTIFYITEMDRAW |
                        CDRF_NOTIFYPOSTPAINT |
                        CDRF_NOTIFYPOSTERASE);

            case CDDS_POSTERASE:
                return 0;

            case CDDS_POSTPAINT:
                DrawState ds =
                    static_cast<DrawState>
                    (DrawStateNoBackground |
                     ((pcd->uItemState & CDIS_SELECTED)? DrawStateSelected : DrawStateNone) |
                     ((pcd->uItemState & CDIS_HOT)? DrawStateHotTrack : DrawStateHotTrack));
                RECT r;

                ::GetClientRect(hwnd, &r);

                g_theme->DrawDropDownButton(hwnd, pcd->hdc, ds, &r);
                if (m_current)
                    g_theme->DrawIdentityItem(pcd->hdc, r, ds,
                                              m_current->icon,
                                              m_current->display_string,
                                              m_current->status,
                                              m_current->type_string);
                return 0;
            }
            return 0;
        }
        return 0;
    }

    LRESULT     NewCredIdentitySelector::OnNotify(int id, NMHDR * pnmh)
    {
        if (pnmh->idFrom == IDC_IDSEL)
            return OnNotifyIdentitySelector(pnmh);
        return 0;
    }

    void        NewCredIdentitySelector::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
    {
        if (id == IDC_IDSEL && codeNotify == BN_CLICKED)
            ShowIdentitySelector();
    }

    LRESULT     NewCredIdentitySelector::OnDrawItem( const DRAWITEMSTRUCT * lpd )
    {
        IdentityDisplayData * d;
        RECT r, tr;
        SIZE margin;
        HFONT hf_old = NULL;
        DrawState ds;

        d = reinterpret_cast<IdentityDisplayData *>( lpd->itemData );

        CopyRect(&r, &lpd->rcItem);

        if (lpd->itemID == 1) {
            HBRUSH hbr;

            margin.cx = margin.cy = (r.bottom - r.top) / 6;
            CopyRect(&tr, &r);
            r.top += margin.cy * 2;
            tr.bottom = r.top;

            hbr = GetSysColorBrush(COLOR_MENU);
            FillRect(lpd->hDC, &tr, hbr);

            if (g_theme->hf_normal) {
                hf_old = SelectFont(lpd->hDC, g_theme->hf_normal);
            }

            SetTextColor(lpd->hDC, GetSysColor(COLOR_MENUTEXT));
            DrawText(lpd->hDC, d->type_string.c_str(), (int) d->type_string.length(),
                     &tr, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
        } else {
            margin.cx = margin.cy = (r.bottom - r.top) / 4;
        }

        ds = static_cast<DrawState>
            (((lpd->itemState & ODS_DISABLED)? DrawStateDisabled : DrawStateNone) |
             ((lpd->itemState & ODS_HOTLIGHT)? DrawStateHotTrack : DrawStateNone) |
             ((lpd->itemState & ODS_SELECTED)? DrawStateSelected : DrawStateNone));

        g_theme->DrawIdentityItem(lpd->hDC, r, ds,
                                  d->icon, d->display_string, d->status,
                                  d->type_string);

        if (hf_old != NULL)
            SelectFont(lpd->hDC, hf_old);

        return TRUE;
    }

    LRESULT     NewCredIdentitySelector::OnMeasureItem( MEASUREITEMSTRUCT * lpm )
    {
        HWND hwc;
        RECT r;

        hwc = GetDlgItem(hwnd, IDC_IDSEL);
        assert(hwc != NULL);
        ::GetClientRect(hwc, &r);

        lpm->itemWidth = r.right - (r.bottom / 3);
        lpm->itemHeight = r.bottom - (r.bottom / 3);

        if (lpm->itemID == 1)
            lpm->itemHeight += lpm->itemHeight / 2;

        return TRUE;
    }
}