
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

    HWND NewCredPanels::UpdateLayout()
    {
        HWND hw_r_p = NULL;
        HWND hw_target = NULL;
        RECT r_p;
        RECT r_persist = {0,0,0,0};
        bool do_persist = false;
        khm_handle  parent_id = NULL;
        khui_new_creds_privint_panel * p;
        DialogWindow * dw;
        const NewCredPage page = NewCredWizard::FromNC(nc)->page;

        switch (page) {
        case NC_PAGE_CREDOPT_BASIC:
            dw = static_cast<DialogWindow *>(&m_basic);
            break;

        case NC_PAGE_CREDOPT_ADV:
            dw = static_cast<DialogWindow *>(&m_advanced);
            break;

        case NC_PAGE_CREDOPT_WIZ:
            dw = static_cast<DialogWindow *>(&m_cfgwiz);
            break;

        default:
            assert(false);
            return NULL;
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
            p->hwnd = khm_create_custom_prompter_dialog(nc, dw->hwnd, p);

        if (p && p->caption[0] == L'\0')
            LoadStringResource(p->caption, IDS_NC_IDENTITY);

        if (page == NC_PAGE_CREDOPT_ADV) {
            int panel_idx;

            /* Everytime there's a change in the order of the
               credentials providers (i.e. because the primary
               identity changed), the initialized flag is reset.  In
               which case we will re-initialize the tab control
               housing the panels. */

            if (!nc->privint.initialized)
                m_advanced.InitializeTabs();

            panel_idx = m_advanced.GetCurrentTabId();

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

        } if (page == NC_PAGE_CREDOPT_WIZ) {
            int panel_idx;

            khui_cw_lock_nc(nc);

            panel_idx = idx_current;

            for (; panel_idx >= 0 &&
                     panel_idx < (int) nc->n_types &&
                     (nc->types[panel_idx].nct->flags & KHUI_NCT_FLAG_DISABLED) ;
                 panel_idx++)
                ;

            if (panel_idx != NC_PRIVINT_PANEL &&
                (panel_idx < 0 || panel_idx >= (int) nc->n_types)) {
                panel_idx = NC_PRIVINT_PANEL;
            }

            if (panel_idx == NC_PRIVINT_PANEL) {
                hw_target = (p)? p->hwnd : NULL;
                if (p->caption)
                    m_cfgwiz.SetItemText(IDC_PANELNAME, p->caption);
                SetWindowPos(m_cfgwiz.GetItem(IDC_PANELNAME), NULL,
                             0, 0, 0, 0, SWP_SHOWONLY);
            } else {
                hw_target = nc->types[panel_idx].nct->hwnd_panel;
                do_persist = false;
                SetWindowPos(m_cfgwiz.GetItem(IDC_PANELNAME), NULL,
                             0, 0, 0, 0, SWP_HIDEONLY);
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

            LoadStringResource(format, IDS_NC_PERSIST);

            cb = sizeof(idname);
            kcdb_get_resource(nc->persist_identity, KCDB_RES_DISPLAYNAME, 0, NULL, NULL,
                              idname, &cb);
            StringCbPrintf(msgtext, sizeof(msgtext), format, idname);

            cb = sizeof(hicon);
            kcdb_get_resource(nc->persist_identity, KCDB_RES_ICON_NORMAL, 0, NULL, NULL,
                              &hicon, &cb);

            if (page == NC_PAGE_CREDOPT_ADV || page == NC_PAGE_CREDOPT_WIZ) {

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
            if (page == NC_PAGE_CREDOPT_ADV || page == NC_PAGE_CREDOPT_WIZ) {
                SetWindowPos(m_persist.hwnd, NULL, 0,0,0,0, SWP_HIDEONLY);
            } else {
                SetWindowPos(m_basic.GetItem(IDC_PERSIST), NULL, 0,0,0,0, SWP_HIDEONLY);
            }
        }

        if (hw_target == NULL) {
            hw_target = m_noprompts.hwnd;
        }

        hw_r_p = dw->GetItem(IDC_NC_R_PROMPTS);
        assert(hw_r_p != NULL);

        if (page == NC_PAGE_CREDOPT_ADV) {
            m_advanced.GetTabPlacement(r_p);
        } else {
            GetWindowRect(hw_r_p, &r_p);
            MapWindowRect(NULL, dw->hwnd, &r_p);
        }

        if (do_persist && page != NC_PAGE_CREDOPT_BASIC) {
            RECT r;

            r_persist = r_p;

            GetClientRect(m_persist.hwnd, &r);
            r_p.bottom -= r.bottom - r.top;
            r_persist.top = r_p.bottom;
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

        SetParent(hw_target, dw->hwnd);

        if (hw_target != m_noprompts.hwnd)
            SetWindowPos(m_noprompts.hwnd, NULL, 0, 0, 0, 0, SWP_HIDEONLY);

        SetWindowPos(hw_target, hw_r_p, rect_coords(r_p), SWP_MOVESIZEZ);
        if (page != NC_PAGE_CREDOPT_BASIC && do_persist) {
            SetParent(m_persist.hwnd, dw->hwnd);
            SetWindowPos(m_persist.hwnd, hw_target, rect_coords(r_persist), SWP_MOVESIZEZ);
        }

        {
            khm_int32 idf = 0;
            khm_handle vid = NULL;

            khui_cw_get_primary_id(nc, &vid);

            if (vid) {
                kcdb_identity_get_flags(vid, &idf);
                kcdb_identity_release(vid);
            }

            dw->CheckButton(IDC_NC_MAKEDEFAULT,
                            ((idf & KCDB_IDENT_FLAG_DEFAULT)? BST_CHECKED : BST_UNCHECKED));
            dw->EnableItem(IDC_NC_MAKEDEFAULT, !(idf & KCDB_IDENT_FLAG_DEFAULT));
        }

        /* if there are more privileged interaction panels to be
           displayed, we enable the Next button.  If not, we enable
           the Finish button. */

        NewCredWizard::FromNC(nc)->m_nav.CheckControls();

        if (hw_target != NULL)
            return GetNextDlgTabItem(hw_target, NULL, FALSE);
        return NULL;
    }

    void NewCredPanels::Create(HWND parent)
    {
        m_basic     .Create(parent);
        m_advanced  .Create(parent);
        m_cfgwiz    .Create(parent);
        m_noprompts .Create(parent);
        m_persist   .Create(parent);
    }
}
