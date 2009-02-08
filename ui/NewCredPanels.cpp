
#include "khmapp.h"
#include "NewCredWizard.hpp"
#include <assert.h>

#define rect_coords(r) r.left, r.top, (r.right - r.left), (r.bottom - r.top)

namespace nim {

    void NewCredPanels::PurgeDeletedShownPanels()
    {
        khui_new_creds_privint_panel * p;
        khui_new_creds_privint_panel * np;

        for (p = QTOP(&nc->privint.shown); p; p = np) {
            np = QNEXT(p);

            if (!IsWindow(p->hwnd)) {
                QDEL(&nc->privint.shown, p);

                if (nc->privint.shown.current_panel == p)
                    nc->privint.shown.current_panel = NULL;

                if (hwnd_current == p->hwnd) {
                    hwnd_current = NULL;
                }

                p->hwnd = NULL;
                khui_cw_free_privint(p);
            }
        }
    }

    bool NewCredPanels::IsSavePasswordAllowed()
    {
        khm_int32 idf = 0;
        khm_handle parent_id = NULL;
        bool do_persist = false;

        /* Decide whether we want to allow saving privileged credentials.
           We do so if the all of the following is true:

           1. The primary identity has KCDB_IDENT_FLAG_KEY_EXPORT set.

           2. The primary identity has no parent identity.

           3. We are showing the first privileged interaction panel.

           4. There is exactly one identity with both
           KCDB_IDENT_FLAG_KEY_STORE, and KCDB_IDENT_FLAG_DEFAULT set
           and is not the primary identity.

           5. The type of the key store identity is a participant of this
           new credentials dialog.
        */

        khui_cw_lock_nc(nc);
        assert(nc->n_identities > 0);

        if (nc->n_identities > 0) {
            kcdb_identity_get_flags(nc->identities[0], &idf);
            kcdb_identity_get_parent(nc->identities[0], &parent_id);
        }
        khui_cw_unlock_nc(nc);

        do {
            kcdb_enumeration e = NULL;
            khm_handle h_ks = NULL;
            khm_int32  ks_type = KCDB_CREDTYPE_INVALID;
            khm_size n = 0;
            khm_size cb;
            khui_new_creds_by_type * t = NULL;

            /* 1. */
            if (!(idf & KCDB_IDENT_FLAG_KEY_EXPORT))
                break;

            /* 2. */
            if (parent_id != NULL)
                break;

            /* 3. */
            if (QTOP(&nc->privint.shown) != NULL &&
                QTOP(&nc->privint.shown) != nc->privint.shown.current_panel)
                break;

            /* 4. */
            if (KHM_SUCCEEDED(kcdb_identity_begin_enum(KCDB_IDENT_FLAG_KEY_STORE | KCDB_IDENT_FLAG_DEFAULT,
                                                       KCDB_IDENT_FLAG_KEY_STORE | KCDB_IDENT_FLAG_DEFAULT,
                                                       &e, &n))) {
                if (n != 1 || KHM_FAILED(kcdb_enum_next(e, &h_ks)) || h_ks == NULL) {
                    kcdb_enum_end(e);
                    break;
                }
                kcdb_enum_end(e);
            } else
                break;

            /* 5. */
            cb = sizeof(ks_type);
            if (KHM_FAILED(kcdb_identity_get_attr(h_ks, KCDB_ATTR_TYPE, NULL, &ks_type, &cb))) {
                kcdb_identity_release(h_ks);
                break;
            }

            if (KHM_FAILED(khui_cw_find_type(nc, ks_type, &t))) {
                kcdb_identity_release(h_ks);
                break;
            }

            do_persist = true;

            khui_cw_lock_nc(nc);
            if (nc->persist_identity)
                kcdb_identity_release(nc->persist_identity);

            nc->persist_identity = h_ks;
            khui_cw_unlock_nc(nc);

        } while (FALSE);

        if (parent_id) {
            kcdb_identity_release(parent_id);
        }

        return do_persist;
    }

    void NewCredPanels::InitializeTabControl(HWND hw_tab, khui_new_creds_privint_panel * p)
    {
        wchar_t desc[KCDB_MAXCCH_SHORT_DESC] = L"";
        khm_size i;

        TabCtrl_DeleteAllItems(hw_tab);

        khui_cw_lock_nc(nc);

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

    HWND NewCredPanels::UpdateLayout()
    {
        HWND hw;
        HWND hw_r_p = NULL;
        HWND hw_target = NULL;
        HWND hw_tab = NULL;
        RECT r_p;
	RECT r_persist = {0,0,0,0};
        bool adv;
        bool do_persist = false;
        khm_handle  parent_id = NULL;
        khui_new_creds_privint_panel * p;

        adv = (reinterpret_cast<NewCredWizard *>(nc->wizard)->page == NC_PAGE_CREDOPT_ADV);

        if (adv) {
            hw = m_advanced.hwnd;
            hw_tab = GetDlgItem(hw, IDC_NC_TAB);
            assert(hw_tab != NULL);
        } else {
            hw = m_basic.hwnd;
        }

        PurgeDeletedShownPanels();

        do_persist = IsSavePasswordAllowed();

        p = nc->privint.shown.current_panel;

        if (p == NULL) {
            khui_cw_get_next_privint(nc, &p);
            nc->privint.shown.current_panel = p;
        }

        /* Fill in some blanks */
        if (p && p->hwnd == NULL && p->use_custom)
            p->hwnd = khm_create_custom_prompter_dialog(nc, hw, p);

        if (p && p->caption[0] == L'\0')
            LoadStringResource(p->caption, IDS_NC_IDENTITY);

        if (adv) {
            TCITEM tci;
            int ctab;
            int panel_idx;

            /* Everytime there's a change in the order of the
               credentials providers (i.e. because the primary
               identity changed), the initialized flag is reset.  In
               which case we will re-initialize the tab control
               housing the panels. */

            if (!nc->privint.initialized)
                InitializeTabControl(hw_tab, p);

            ctab = TabCtrl_GetCurSel(hw_tab);

            ZeroMemory(&tci, sizeof(tci));

            tci.mask = TCIF_PARAM;
            TabCtrl_GetItem(hw_tab, ctab, &tci);

            panel_idx = (int) tci.lParam;

            khui_cw_lock_nc(nc);

            if (panel_idx == NC_PRIVINT_PANEL) {
                hw_target = (p)? p->hwnd : NULL;
            } else {
                /* We have to show the credentials options panel for some
                   credentials type */
                assert(panel_idx >= 0 && (khm_size) panel_idx < nc->n_types);
                hw_target = nc->types[panel_idx].nct->hwnd_panel;
                do_persist = false;
            }

            khui_cw_unlock_nc(nc);

            if (hw_target != hwnd_current && hwnd_current != NULL)
                SetWindowPos(hwnd_current, NULL, 0, 0, 0, 0, SWP_HIDEONLY);

            hwnd_current = hw_target;
            idx_current = panel_idx;

        } else {
            hw_target = (p)? p->hwnd : NULL;
            if (hw_target != hwnd_current && hwnd_current != NULL)
                SetWindowPos(hwnd_current, NULL, 0, 0, 0, 0, SWP_HIDEONLY);

            hwnd_current = hw_target;
            idx_current = NC_PRIVINT_PANEL;
            m_basic.SetItemText(IDC_BORDER, (p && p->caption)? p->caption: L"");
        }

        if (do_persist) {
            HICON hicon;
            wchar_t idname[KCDB_IDENT_MAXCCH_NAME] = L"";
            wchar_t msgtext[KCDB_MAXCCH_NAME];
            wchar_t format[64];
            khm_size cb;

            assert(nc->persist_identity);

            LoadString(khm_hInstance, IDS_NC_PERSIST, format, ARRAYLENGTH(format));
            cb = sizeof(idname);
            kcdb_get_resource(nc->persist_identity, KCDB_RES_DISPLAYNAME, 0, NULL, NULL,
                              idname, &cb);
            StringCbPrintf(msgtext, sizeof(msgtext), format, idname);

            cb = sizeof(hicon);
            kcdb_get_resource(nc->persist_identity, KCDB_RES_ICON_NORMAL, 0, NULL, NULL,
                              &hicon, &cb);

            if (adv) {

                m_persist.SetItemText(IDC_PERSIST, msgtext);

                m_persist.SendItemMessage(IDC_IDICON, STM_SETICON,
                                          (WPARAM) hicon, 0);

                m_persist.CheckButton(IDC_PERSIST,
                                      nc->persist_privcred ? BST_CHECKED : BST_UNCHECKED);
            } else {
                m_basic.SetItemText(IDC_PERSIST, msgtext);
                m_basic.CheckButton(IDC_PERSIST,
                                    nc->persist_privcred ? BST_CHECKED : BST_UNCHECKED);
                SetWindowPos(m_basic.GetItem(IDC_PERSIST), NULL, 0,0,0,0, SWP_SHOWONLY);
            }
        } else {
            if (adv) {
                SetWindowPos(m_persist.hwnd, NULL, 0,0,0,0, SWP_HIDEONLY);
            } else {
                SetWindowPos(m_basic.GetItem(IDC_PERSIST), NULL, 0,0,0,0, SWP_HIDEONLY);
            }
        }

        if (hw_target == NULL) {
            hw_target = m_noprompts.hwnd;
        }

        hw_r_p = GetDlgItem(hw, IDC_NC_R_PROMPTS);
        assert(hw_r_p != NULL);

        if (adv) {
            GetWindowRect(hw_tab, &r_p);
            MapWindowRect(NULL, hw, &r_p);

            TabCtrl_AdjustRect(hw_tab, FALSE, &r_p);

            if (do_persist) {
                RECT r;

                r_persist = r_p;

                GetClientRect(m_persist.hwnd, &r);
                r_p.bottom -= r.bottom - r.top;            
                r_persist.top = r_p.bottom;
            }
        } else {
            GetWindowRect(hw_r_p, &r_p);
            MapWindowRect(NULL, hw, &r_p);
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
                assert(FALSE);

                ws &= ~WS_POPUP;
                ws |= WS_CHILD;
                SetWindowLong(hw_target, GWL_STYLE, ws);
            }
        }

        SetParent(hw_target, hw);

        if (hw_target != m_noprompts.hwnd)
            SetWindowPos(m_noprompts.hwnd, NULL, 0, 0, 0, 0, SWP_HIDEONLY);

        SetWindowPos(hw_target, hw_r_p, rect_coords(r_p), SWP_MOVESIZEZ);
        if (adv && do_persist) {
            SetWindowPos(m_persist.hwnd, hw_target, rect_coords(r_persist), SWP_MOVESIZEZ);
        }

        CheckDlgButton(hw, IDC_NC_MAKEDEFAULT,
                       ((idf & KCDB_IDENT_FLAG_DEFAULT)? BST_CHECKED : BST_UNCHECKED));
        EnableWindow(GetDlgItem(hw, IDC_NC_MAKEDEFAULT),
                     !(idf & KCDB_IDENT_FLAG_DEFAULT));

        /* if there are more privileged interaction panels to be
           displayed, we enable the Next button.  If not, we enable
           the Finish button. */
        nc->nav.transitions &= ~(NC_TRANS_NEXT | NC_TRANS_PREV);
        if ((p && QNEXT(p)) || KHM_SUCCEEDED(khui_cw_peek_next_privint(nc, NULL)))
            nc->nav.transitions |= NC_TRANS_NEXT;
        if (p && QPREV(p))
            nc->nav.transitions |= NC_TRANS_PREV;
        if (nc->nav.state & NC_NAVSTATE_OKTOFINISH)
            nc->nav.transitions |= NC_TRANS_FINISH;

        if (hw_target != NULL && (hw = GetNextDlgTabItem(hw_target, NULL, FALSE)) != NULL)
            return hw;
        return NULL;
    }

}
