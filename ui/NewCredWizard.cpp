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
#if _WIN32_WINNT >= 0x0501
#include <uxtheme.h>
#endif
#include <assert.h>

namespace nim {

void NewCredWizard::PositionSelf()
{
    RECT r;
    HWND hwnd_parent = NULL;
    int x, y;
    int width, height;

    /* Position the dialog */

    GetWindowRect(hwnd, &r);

    width = r.right - r.left;
    height = r.bottom - r.top;

    /* if the parent window is visible, we center the new
       credentials dialog over the parent.  Otherwise, we center
       it on the primary display. */

    hwnd_parent = GetParent(hwnd);

    if (IsWindowVisible(hwnd_parent)) {
        GetWindowRect(hwnd_parent, &r);
    } else {
        if (!SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID) &r, 0)) {
            /* failover to the window coordinates */
            GetWindowRect(hwnd_parent, &r);
        }
    }
    x = (r.right + r.left)/2 - width / 2;
    y = (r.top + r.bottom)/2 - height / 2;

    /* we want to check if the entire rect is visible on the
       screen.  If the main window is visible and in basic mode,
       we might end up with a rect that is partially outside the
       screen. */
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
}

BOOL NewCredWizard::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
    khui_cw_lock_nc(nc);
    nc->hwnd = hwnd;

    assert(nc != NULL);
    assert(nc->subtype == KHUI_NC_SUBTYPE_NEW_CREDS ||
           nc->subtype == KHUI_NC_SUBTYPE_PASSWORD ||
           nc->subtype == KHUI_NC_SUBTYPE_IDSPEC ||
           nc->subtype == KHUI_NC_SUBTYPE_ACQPRIV_ID ||
           nc->subtype == KHUI_NC_SUBTYPE_CONFIG_ID ||
           nc->subtype == KHUI_NC_SUBTYPE_ACQDERIVED);
    khui_cw_unlock_nc(nc);

    m_nav     .Create(hwnd);
    m_idsel   .Create(hwnd);
    m_privint .Create(hwnd);
    m_idspec  .Create(hwnd);
    m_progress.Create(hwnd);

    PositionSelf();

    if (is_modal) {
        switch (khui_cw_get_subtype(nc)) {
        case KHUI_NC_SUBTYPE_ACQPRIV_ID:
            if (khm_cred_begin_new_cred_op()) {
                kmq_post_message(KMSG_CRED, KMSG_CRED_ACQPRIV_ID, 0,
                                 (void *) nc);
            }
            break;

        case KHUI_NC_SUBTYPE_IDSPEC:
            kmq_post_message(KMSG_CRED, KMSG_CRED_IDSPEC, 0,
                             (void *) nc);
            break;

        default:
            assert(FALSE);
        }
    }

    khui_cw_lock_nc(nc);
    SetWindowText(hwnd, nc->window_title);
    khui_cw_unlock_nc(nc);

    return TRUE;
}

void NewCredWizard::OnDestroy(void)
{
    khm_del_dialog(hwnd);

    __super::OnDestroy();
}

LRESULT NewCredWizard::OnHelp(HELPINFO * hlp)
{
    /* Context mapping:

       This is a list of pairs of DWORDs.  The first DWORD
       specifies the control ID, and the second DWORD specifies
       the help context ID.  The list ends with a 0.

    */
    static DWORD ctxids[] = {
        IDC_IDSEL, IDH_NC_IDSEL,
        IDC_IDPROVLIST, IDH_NC_IDPROVLIST,
        IDC_CLOSEIF, IDH_NC_CLOSEIF,
        IDC_BACK, IDH_NC_BACK,
        IDC_NEXT, IDH_NC_NEXT,
        IDC_FINISH, IDH_NC_FINISH,
        IDC_NC_ABORT, IDH_NC_ABORT,
        IDC_NC_CLOSE, IDH_NC_CLOSE,
        IDC_RETRY, IDH_NC_RETRY,
        IDCANCEL, IDH_NC_CANCEL,
        IDC_PERSIST, IDH_NC_PERSIST,
        IDC_NC_MAKEDEFAULT, IDH_NC_MAKEDEFAULT,
        IDC_NC_ADVANCED, IDH_NC_ADVANCED,
        0
    };

    khui_nc_subtype subtype = khui_cw_get_subtype(nc);

    if (subtype != KHUI_NC_SUBTYPE_NEW_CREDS &&
        subtype != KHUI_NC_SUBTYPE_PASSWORD)
        return TRUE;

    return khm_handle_wm_help(hlp, L"::/popups_newcreds.txt", ctxids,
                              ((nc->subtype == KHUI_NC_SUBTYPE_NEW_CREDS)?
                               IDH_ACTION_NEW_ID: IDH_ACTION_PASSWD_ID));
}

void NewCredWizard::OnClose()
{
    Navigate(NC_PAGET_CANCEL);
}

void NewCredWizard::OnActivate(UINT state, HWND hwndActDeact, BOOL fMinimized)
{
    if (state == WA_ACTIVE || state == WA_CLICKACTIVE) {

        FLASHWINFO fi;
        DWORD_PTR ex_style;

        if (nc && flashing_enabled) {
            ZeroMemory(&fi, sizeof(fi));

            fi.cbSize = sizeof(fi);
            fi.hwnd = hwnd;
            fi.dwFlags = FLASHW_STOP;

            FlashWindowEx(&fi);

            flashing_enabled = false;
        }

        ex_style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

        if (ex_style & WS_EX_TOPMOST) {
            ex_style &= ~WS_EX_TOPMOST;
            SetWindowLongPtr(hwnd, GWL_EXSTYLE, ex_style);
            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE);
        }
    }
}

void NewCredWizard::OnDeriveFromPrivCred(khui_collect_privileged_creds_data * pcd)
{
    khm_cred_derive_identity_from_privileged_creds(pcd);
}

void NewCredWizard::OnCollectPrivCred(khui_collect_privileged_creds_data * pcd)
{
    khm_cred_collect_privileged_creds(pcd);
}

void NewCredWizard::OnProcessComplete(int has_error)
{
    khui_cw_lock_nc(nc);

    if (nc->n_children != 0) {
        khui_cw_unlock_nc(nc);
        return;
    }

    nc->response &= ~KHUI_NC_RESPONSE_PROCESSING;

    if ((nc->response & KHUI_NC_RESPONSE_NOEXIT) || has_error) {

        EnableControls( TRUE );

        /* reset state */
        nc->result = KHUI_NC_RESULT_CANCEL;

        khui_cw_unlock_nc(nc);

        NotifyTypes( WMNC_DIALOG_PROCESS_COMPLETE, 0, TRUE);

        if (has_error) {
            m_nav.SetAllControls(NewCredNavigation::Retry |
                                 NewCredNavigation::Close |
                                 NewCredNavigation::Prev);
            m_nav.UpdateLayout();
            return;
        }

        khui_nc_subtype subtype = khui_cw_get_subtype(nc);

        if (subtype == KHUI_NC_SUBTYPE_NEW_CREDS ||
            subtype == KHUI_NC_SUBTYPE_ACQPRIV_ID) {
            Navigate( NC_PAGE_CREDOPT_BASIC);
        } else if (nc->subtype == KHUI_NC_SUBTYPE_PASSWORD) {
            Navigate( NC_PAGE_PASSWORD);
        } else {
            assert(FALSE);
        }
    } else {
        khui_cw_unlock_nc(nc);
        Navigate( NC_PAGET_END);
    }
}

void NewCredWizard::OnSetPrompts()
{
    khui_cw_lock_nc(nc);
    nc->privint.initialized = FALSE;
    khui_cw_unlock_nc(nc);

    if (page == NC_PAGE_PASSWORD ||
        page == NC_PAGE_CREDOPT_ADV ||
        page == NC_PAGE_CREDOPT_WIZ ||
        page == NC_PAGE_CREDOPT_BASIC) {

        UpdateLayout();
    }
}

void NewCredWizard::OnIdentityStateChange(const nc_identity_state_notification * notif)
{
    khm_int32 nflags;

    nflags = notif->flags;

    // If there are no identities, we don't accept identity state notifications.
    khui_cw_lock_nc(nc);

    if (nc->n_identities == 0) {
        wchar_t buf[80];

        assert(FALSE);
        LoadStringResource(buf, IDS_NC_NPR_CHOOSE);
        m_privint.m_noprompts.SetText(KHERR_WARNING, buf);
        m_privint.m_noprompts.SetProgress(0, false);
        khui_cw_unlock_nc(nc);
        goto done;

    }

    khui_cw_unlock_nc(nc);

    // If the notification is from the identity credentials type,
    // then we update the identity state on the m_noprompts
    // dialog.  This way, the provider can notify the user of the
    // identity validation status.

    if (notif->credtype_panel != NULL && notif->credtype_panel->is_id_credtype) {

        // Finished validating the identity
        if (nflags & KHUI_CWNIS_VALIDATED) {

            khm_int32 idflags = 0;

            kcdb_identity_get_flags(nc->identities[0], &idflags);

            if (idflags & KCDB_IDENT_FLAG_VALID) {
                wchar_t buf[80];

                LoadStringResource(buf, IDS_NC_NPR_CLICKFINISH);
                m_privint.m_noprompts.SetText(KHERR_NONE, buf);
                m_idsel.SetStatus( (notif->state_string)? notif->state_string : L"");

                if (auto_configure && page == NC_PAGE_CREDOPT_BASIC) {
                    m_privint.m_noprompts.SetProgress(0, false);
                    Navigate( NC_PAGE_CREDOPT_WIZ );
                    return;
                }

            } else {
                wchar_t buf[80];

                /* The identity may have KCDB_IDENT_FLAG_INVALID
                   or KCDB_IDENT_FLAG_UNKNOWN set. */

                LoadStringResource(buf,
                                   ((idflags & KCDB_IDENT_FLAG_INVALID)?
                                    IDS_NC_INVALIDID : IDS_NC_UNKNOWNID));
                m_idsel.SetStatus(buf);
                m_privint.m_noprompts.SetText(KHERR_ERROR, notif->state_string);
                nflags &= ~KHUI_CWNIS_VALIDATED;
            }

            m_privint.m_noprompts.SetProgress(0, false);

        } else {
            wchar_t buf[80];

            LoadStringResource(buf, IDS_NC_NPR_VALIDATING);
            m_privint.m_noprompts.SetText(KHERR_INFO, buf);
            if (nflags & KHUI_CWNIS_NOPROGRESS) {
                m_privint.m_noprompts.SetProgress(0, false);
            } else {
                m_privint.m_noprompts.SetProgress(notif->progress, true);
            }
        }
    }

 done:

    m_privint.UpdateLayout();
    m_nav.UpdateLayout();

}

void NewCredWizard::OnNewIdentity()
{
    NotifyNewIdentity( TRUE );

    {
        khm_handle identity = NULL;
        khm_int32 flags = 0;

        if (KHM_SUCCEEDED(khui_cw_get_primary_id(nc, &identity))) {
            kcdb_identity_get_flags(identity, &flags);
            kcdb_identity_release(identity);
        }

        if ((flags & (KCDB_IDENT_FLAG_CONFIG |
                      KCDB_IDENT_FLAG_INVALID)) ||
            !(flags & KCDB_IDENT_FLAG_CRED_INIT)) {
            Navigate( NC_PAGE_CREDOPT_BASIC );
        }
    }
}

void NewCredWizard::OnDialogActivate()
{
    khm_int32 t;

    /* About to activate the new credentials dialog.  We need to
       set up the wizard. */
    switch (nc->subtype) {
    case KHUI_NC_SUBTYPE_ACQPRIV_ID:
    case KHUI_NC_SUBTYPE_NEW_CREDS:

        if (nc->n_identities > 0) {
            /* If there is a primary identity, then we can start in
               the credentials options page */

            khm_int32 idflags = 0;
            khm_handle ident = NULL;

            khm_handle parent = NULL;

            khui_cw_lock_nc(nc);

            khui_cw_get_primary_id(nc, &ident);

            if (KHM_SUCCEEDED(kcdb_identity_get_parent(ident, &parent)) &&
                parent != NULL) {

                khui_cw_set_primary_id_no_notify(nc, parent);
                khui_cw_add_identity(nc, ident);

                kcdb_identity_release(ident);
                ident = parent;
                parent = NULL;
            }

            khui_cw_unlock_nc(nc);

            NotifyNewIdentity( FALSE );

            assert(ident != NULL);
            assert(parent == NULL);

            kcdb_identity_get_flags(ident, &idflags);

            /* Check if this identity has a configuration.  If so,
               we can continue in basic mode.  Otherwise we should
               start in advanced mode so that the user can specify
               identity options to be used the next time. */
            if ((idflags & (KCDB_IDENT_FLAG_CONFIG |
                            KCDB_IDENT_FLAG_INVALID)) ||
                !(idflags & KCDB_IDENT_FLAG_CRED_INIT)) {
                Navigate( NC_PAGE_CREDOPT_BASIC);
            } else {
                Navigate( NC_PAGE_CREDOPT_WIZ);
            }

            kcdb_identity_release(ident);
        } else {
            /* No primary identity.  We have to open with the
               identity specification page */
            Navigate( NC_PAGE_IDSPEC);
        }

        break;

    case KHUI_NC_SUBTYPE_PASSWORD:
        if (nc->n_identities > 0) {
            NotifyNewIdentity( TRUE );
            Navigate( NC_PAGE_PASSWORD);
        } else {
            Navigate( NC_PAGE_IDSPEC);
        }
        break;

    case KHUI_NC_SUBTYPE_IDSPEC:
        Navigate( NC_PAGE_IDSPEC);
        break;

    default:
        assert(FALSE);
    }

    ShowWindow(SW_SHOWNORMAL);

    t = 0;
    /* bring the window to the top, if necessary */
    if (KHM_SUCCEEDED(khc_read_int32(NULL,
                                     L"CredWindow\\Windows\\NewCred\\ForceToTop",
                                     &t)) &&

        t != 0) {

        BOOL sfw = FALSE;

        /* it used to be that the above condition also called
           !khm_is_dialog_active() to find out whether there was a
           dialog active.  If there was, we wouldn't try to bring
           the new cred window to the foreground. But that was not
           the behavior we want. */

        /* if the main window is not visible, then the
           SetWindowPos() call is sufficient to bring the new
           creds window to the top.  However, if the main window
           is visible but not active, the main window needs to be
           activated before a child window can be activated. */

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

            flashing_enabled = true;
        }

    } else {
        SetFocus(hwnd);
    }
}

void NewCredWizard::OnDialogSetup()
{

    /* At this point, all the credential types that are interested
       in the new credentials operation have attached themselves.

       Each credentials type that wants to present a UI would
       provide us with a dialog template and a dialog procedure.
       We should now create the dialogs using these.
    */

    assert(m_privint.m_advanced.hwnd != NULL);

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
                                      m_privint.m_advanced.hwnd,
                                      t->dlg_proc, (LPARAM) nc);

                assert(t->hwnd_panel);
#if _WIN32_WINNT >= 0x0501
                if (t->hwnd_panel) {
                    EnableThemeDialogTexture(t->hwnd_panel,
                                             ETDT_ENABLETAB);
                }
#endif
            }
        }
    }
}

BOOL NewCredWizard::OnPosChanging(LPWINDOWPOS wpos)
{
    if (nc == NULL || m_privint.hwnd_current == NULL)
        return FALSE;

    ::SendMessage(m_privint.hwnd_current, KHUI_WM_NC_NOTIFY, 
                  MAKEWPARAM(0, WMNC_DIALOG_MOVE), (LPARAM) nc);

    return FALSE;
}

void NewCredWizard::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
    if (codeNotify == BN_CLICKED) {
        switch (id) {
        case IDCANCEL:
            Navigate( NC_PAGET_CANCEL);
            return;

        case IDOK:
            Navigate( NC_PAGET_DEFAULT);
            return;
        }
    }
    
    if (hwndCtl != NULL) {
        HWND owner = GetParent(hwndCtl);
        if (owner != NULL && owner != hwnd) {
            FORWARD_WM_COMMAND(owner, id, hwndCtl, codeNotify, ::PostMessage);
            return;
        }
    }
}

bool NewCredWizard::HaveValidIdentity()
{
    khm_handle identity = NULL;
    khm_int32 flags = 0;
    bool is_valid = false;

    if (KHM_SUCCEEDED(khui_cw_get_primary_id(nc, &identity)) &&
        KHM_SUCCEEDED(kcdb_identity_get_flags(identity, &flags)) &&
        (flags & KCDB_IDENT_FLAG_VALID)) {
        is_valid = true;
    }

    if (identity)
        kcdb_identity_release(identity);
    return is_valid;
}

void NewCredWizard::Navigate( NewCredPage new_page )
{
    auto_configure = false;

    /* All the page transitions in the new credentials wizard
       happen here. */

    switch (new_page) {
    case NC_PAGE_NONE:
        /* Do nothing */
        assert(FALSE);
        return;

    case NC_PAGE_IDSPEC:
        m_idspec.prev_page = page;
        page = NC_PAGE_IDSPEC;
        if (nc->subtype == KHUI_NC_SUBTYPE_IDSPEC) {
            m_nav.SetAllControls(NewCredNavigation::Finish);
        } else {
            m_nav.SetAllControls(NewCredNavigation::Next);
        }
        if (m_idspec.prev_page != NC_PAGE_NONE)
            m_nav.EnableControl(NewCredNavigation::Prev);
        break;

    case NC_PAGE_CREDOPT_BASIC:
        m_nav.DisableControl(NewCredNavigation::Abort | NewCredNavigation::ShowCloseIf);
        page = NC_PAGE_CREDOPT_BASIC;
        break;

    case NC_PAGE_CREDOPT_ADV:
        m_nav.DisableControl(NewCredNavigation::Abort | NewCredNavigation::ShowCloseIf);
        page = NC_PAGE_CREDOPT_ADV;
        break;

    case NC_PAGE_CREDOPT_WIZ:
        m_nav.DisableControl(NewCredNavigation::Abort | NewCredNavigation::ShowCloseIf);
        page = NC_PAGE_CREDOPT_WIZ;
        m_privint.idx_current = 0;
        break;

    case NC_PAGE_PASSWORD:
        m_nav.DisableControl(NewCredNavigation::Abort | NewCredNavigation::ShowCloseIf);
        page = NC_PAGE_PASSWORD;
        break;

    case NC_PAGE_PROGRESS:
        m_nav.SetAllControls(NewCredNavigation::Abort);
        page = NC_PAGE_PROGRESS;
        break;

    case NC_PAGET_NEXT:
        switch (page) {
        case NC_PAGE_IDSPEC:
            if (m_idspec.ProcessNewIdentity()) {

                switch (nc->subtype) {
                case KHUI_NC_SUBTYPE_NEW_CREDS:
                    m_nav.DisableControl(NewCredNavigation::Abort | NewCredNavigation::ShowCloseIf);
                    m_privint.idx_current = 0;
                    if (HaveValidIdentity()) {
                        page = NC_PAGE_CREDOPT_WIZ;
                    } else {
                        page = NC_PAGE_CREDOPT_BASIC;
                        auto_configure = true;
                    }
                    break;

                case KHUI_NC_SUBTYPE_ACQPRIV_ID:
                    m_nav.DisableControl(NewCredNavigation::Abort | NewCredNavigation::ShowCloseIf);
                    page = NC_PAGE_CREDOPT_BASIC;
                    break;

                case KHUI_NC_SUBTYPE_PASSWORD:
                    m_nav.DisableControl(NewCredNavigation::Abort | NewCredNavigation::ShowCloseIf);
                    page = NC_PAGE_PASSWORD;
                    break;

                default:
                    assert(FALSE);
                    break;
                }

                break;
            }
            return;

        case NC_PAGE_CREDOPT_ADV:
        case NC_PAGE_CREDOPT_BASIC:
        case NC_PAGE_PASSWORD:
            {
                khui_new_creds_privint_panel * p;

                p = khui_cw_get_current_privint_panel(nc);
                if (p) {
                    p->processed = TRUE;
                    p = QNEXT(p);
                }
                if (p == NULL)
                    khui_cw_get_next_privint(nc, &p);
                khui_cw_set_current_privint_panel(nc, p);
                m_nav.DisableControl(NewCredNavigation::Abort | NewCredNavigation::ShowCloseIf);
            }
            break;

        case NC_PAGE_CREDOPT_WIZ:
            {
                int idx;

                idx = m_privint.idx_current;

                if (idx == NC_PRIVINT_PANEL) {
                    khui_new_creds_privint_panel * p;

                    p = khui_cw_get_current_privint_panel(nc);
                    if (p) {
                        p->processed = TRUE;
                        p = QNEXT(p);
                    }
                    if (p == NULL)
                        khui_cw_get_next_privint(nc, &p);
                    khui_cw_set_current_privint_panel(nc, p);
                    m_nav.DisableControl(NewCredNavigation::Abort | NewCredNavigation::ShowCloseIf);
                } else {
                    if (idx + 1 < (int) nc->n_types &&
                        !(nc->types[idx + 1].nct->flags & KHUI_NCT_FLAG_DISABLED))
                        idx ++;
                    else
                        idx = NC_PRIVINT_PANEL;
                    m_nav.DisableControl(NewCredNavigation::Abort | NewCredNavigation::ShowCloseIf);
                }

                m_privint.idx_current = idx;
            }
            break;

        default:
            assert(FALSE);
        }
        break;

    case NC_PAGET_PREV:
        switch (page) {
        case NC_PAGE_IDSPEC:
            if (m_idspec.prev_page != NC_PAGE_NONE) {
                page = m_idspec.prev_page;
                m_nav.DisableControl(NewCredNavigation::Abort | NewCredNavigation::ShowCloseIf);
            }
            break;

        case NC_PAGE_CREDOPT_ADV:
        case NC_PAGE_CREDOPT_BASIC:
        case NC_PAGE_PASSWORD:
            {
                khui_new_creds_privint_panel * p;

                p = khui_cw_get_current_privint_panel(nc);
                if (p)
                    p = QPREV(p);
                khui_cw_set_current_privint_panel(nc, p);
                m_nav.DisableControl(NewCredNavigation::Abort | NewCredNavigation::ShowCloseIf);
            }
            break;

        case NC_PAGE_CREDOPT_WIZ:
            {
                int idx = m_privint.idx_current;

                if (idx == NC_PRIVINT_PANEL) {
                    khui_new_creds_privint_panel * p;

                    p = khui_cw_get_current_privint_panel(nc);
                    if (p)
                        p = QPREV(p);
                    if (p == NULL) {
                        int i;

                        for (i = 0; i < (int) nc->n_types; i++)
                            if (nc->types[i].nct->flags & KHUI_NCT_FLAG_DISABLED)
                                break;

                        i--;
                        m_privint.idx_current = i;
                    } else {
                        khui_cw_set_current_privint_panel(nc, p);
                    }
                } else {
                    if (idx > 0)
                        idx --;

                    m_privint.idx_current = idx;
                }

                m_nav.DisableControl(NewCredNavigation::Abort | NewCredNavigation::ShowCloseIf);
            }
            break;

        case NC_PAGE_PROGRESS:
            {
                if (nc->subtype == KHUI_NC_SUBTYPE_NEW_CREDS ||
                    nc->subtype == KHUI_NC_SUBTYPE_ACQPRIV_ID)
                    page = NC_PAGE_CREDOPT_BASIC;
                else if (nc->subtype == KHUI_NC_SUBTYPE_PASSWORD)
                    page = NC_PAGE_PASSWORD;
                else {
                    assert(FALSE);
                }

                m_nav.SetAllControls(0);
            }
            break;

        default:
            assert(FALSE);
        }
        break;

    case NC_PAGET_FINISH:
        switch (nc->subtype) {
        case KHUI_NC_SUBTYPE_IDSPEC:
            assert(page == NC_PAGE_IDSPEC);
            if (m_idspec.ProcessNewIdentity()) {
                nc->result = KHUI_NC_RESULT_PROCESS;
                khm_cred_dispatch_process_message(nc);
            }
            break;

        case KHUI_NC_SUBTYPE_NEW_CREDS:
        case KHUI_NC_SUBTYPE_PASSWORD:
        case KHUI_NC_SUBTYPE_ACQPRIV_ID:
            nc->result = KHUI_NC_RESULT_PROCESS;
            NotifyTypes( WMNC_DIALOG_PREPROCESS, (LPARAM) nc, TRUE);
            khm_cred_dispatch_process_message(nc);
            m_nav.SetAllControls(NewCredNavigation::Abort | NewCredNavigation::ShowCloseIf);
            page = NC_PAGE_PROGRESS;
            break;

        default:
            assert(FALSE);
        }
        break;

    case NC_PAGET_CANCEL:
        if (m_nav.IsState(NewCredNavigation::Cancelled))
            break;

        if (m_nav.IsState(NewCredNavigation::PreEnd)) {

            m_nav.DisableState(NewCredNavigation::NoClose);
            Navigate( NC_PAGET_END);
            return;

        } else {

            if (khm_cred_abort_process_message(nc) == KHM_ERROR_NOT_READY) {
                khui_cw_lock_nc(nc);
                nc->result = KHUI_NC_RESULT_CANCEL;
                khui_cw_unlock_nc(nc);
                NotifyTypes( WMNC_DIALOG_PREPROCESS, (LPARAM) nc, TRUE);
                khm_cred_dispatch_process_message(nc);
            }
            m_nav.SetAllControls(0);
            page = NC_PAGE_PROGRESS;
            m_nav.EnableState(NewCredNavigation::Cancelled);
        }
        break;

    case NC_PAGET_END:
        if (m_nav.IsState(NewCredNavigation::NoClose) &&
            !m_nav.IsState(NewCredNavigation::Cancelled)) {

            m_nav.EnableState(NewCredNavigation::PreEnd);
            m_nav.SetAllControls(NewCredNavigation::Close);
            page = NC_PAGE_PROGRESS;

        } else {
            khm_size i;

            Hold();
            if (is_modal) {
                if (nc->subtype == KHUI_NC_SUBTYPE_ACQPRIV_ID) {
                    /* prevent the collector credential set from being
                       destroyed */
                    khui_cw_set_privileged_credential_collector(nc, NULL);
                }
                for (i=0; i < nc->n_types; i++)
                    if (nc->types[i].nct->hwnd_panel) {
                        ::DestroyWindow(nc->types[i].nct->hwnd_panel);
                        nc->types[i].nct->hwnd_panel = NULL;
                    }

                EndDialog(nc->result);
            } else {
                DestroyWindow();
            }
            if (nc->subtype == KHUI_NC_SUBTYPE_IDSPEC)
                khm_cred_end_dialog(nc);
            else
                kmq_post_message(KMSG_CRED, KMSG_CRED_END, 0, (void *) nc);
            Release();

            return;

        }
        break;

    case NC_PAGET_DEFAULT:
        {
            new_page = 
                (m_nav.IsControlEnabled(NewCredNavigation::Next))?   NC_PAGET_NEXT :
                (m_nav.IsControlEnabled(NewCredNavigation::Finish))? NC_PAGET_FINISH :
                (m_nav.IsControlEnabled(NewCredNavigation::Retry))?  NC_PAGET_FINISH :
                (m_nav.IsControlEnabled(NewCredNavigation::Close))?  NC_PAGET_CANCEL :
                (m_nav.IsControlEnabled(NewCredNavigation::Abort))?  NC_PAGET_CANCEL :
                NC_PAGET_CANCEL;
            Navigate( new_page);
            return;
        }
        break;

    default:
        assert(FALSE);
    }

    UpdateLayout();
}

// Preprare the wizard to handle a new primary identity.  Should
// NOT be called with nc locked.
void NewCredWizard::NotifyNewIdentity(bool notify_ui)
{
    bool default_off = true;
    bool isk5 = false;

    /* For backwards compatibility, we assume that if there is no
       primary identity or if the primary identity is not from the
       Kerberos v5 identity provider, then the default for all
       plug-ins is to be disabled and must be explicitly enabled. */

    /* All the privileged interaction panels are assumed to no longer
       be valid */
    khui_cw_clear_all_privints(nc);

    khui_cw_lock_nc(nc);

    if (nc->n_identities == 0 ||
        !kcdb_identity_by_provider(nc->identities[0], L"Krb5Ident"))

        default_off = true;

    else {

        isk5 = true;
        default_off = false;

    }

    if (default_off) {
        unsigned i;

        for (i=0; i < nc->n_types; i++) {
            nc->types[i].nct->flags |= KHUI_NCT_FLAG_DISABLED;
        }
    } else if (isk5) {
        unsigned i;

        for (i=0; i < nc->n_types; i++) {
            nc->types[i].nct->flags &= ~KHUI_NCT_FLAG_DISABLED;
        }
    }

    nc->privint.initialized = FALSE;

    if (nc->subtype == KHUI_NC_SUBTYPE_ACQPRIV_ID)
        nc->persist_privcred = TRUE;
    else
        nc->persist_privcred = FALSE;

    if (nc->persist_identity)
        kcdb_identity_release(nc->persist_identity);
    nc->persist_identity = NULL;

    khui_cw_unlock_nc(nc);

    NotifyTypes( WMNC_IDENTITY_CHANGE, (LPARAM) nc, TRUE);

    khui_cw_lock_nc(nc);

    PrepCredTypes();

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

        if (!(f & KCDB_IDENT_FLAG_CRED_INIT)) {

            std::wstring cant = LoadStringResource(IDS_NC_CANTINIT);
            m_privint.m_noprompts.SetText(KHERR_ERROR, cant.c_str());
        }
    }

    /* The m_noprompts panel will be shown if the provider does
       not provide us with a password change dialog.  The default
       text would need to be changed to indicate that the identity
       does not support password changes. */
    if (nc->subtype == KHUI_NC_SUBTYPE_PASSWORD &&
        nc->n_identities > 0 &&
        nc->identities[0]) {

        std::wstring wont = LoadStringResource(IDS_NC_NOPWCHANGE);
        m_privint.m_noprompts.SetText(KHERR_ERROR, wont.c_str());
    }

    khui_cw_unlock_nc(nc);

    if (notify_ui) {

        m_idsel.UpdateLayout();

        if (page == NC_PAGE_CREDOPT_ADV ||
            page == NC_PAGE_CREDOPT_BASIC ||
            page == NC_PAGE_CREDOPT_WIZ ||
            page == NC_PAGE_PASSWORD) {

            m_privint.UpdateLayout();

        }
    }
}

void NewCredWizard::NotifyTypes(khui_wm_nc_notifications N, LPARAM lParam, bool sync)
{
    khm_size i;
        
    for (i=0; i < nc->n_types; i++) {
        if (nc->types[i].nct->hwnd_panel == NULL)
            continue;

        if (sync) {
            ::SendMessage(nc->types[i].nct->hwnd_panel, KHUI_WM_NC_NOTIFY,
                          MAKEWPARAM(0, N), lParam);
        } else {
            ::PostMessage(nc->types[i].nct->hwnd_panel, KHUI_WM_NC_NOTIFY,
                          MAKEWPARAM(0, N), lParam);
        }
    }
}

void NewCredWizard::EnableControls(bool enable)
{
    //EnableWindow(m_nav.hwnd, enable);
    //EnableWindow(m_idsel.hwnd, enable);
}

NewCredWizard::NewCredLayout::NewCredLayout(NewCredWizard * _w, int nWindows)
{
    dwp = BeginDeferWindowPos(nWindows);
    w = _w;
    SetRectEmpty(&r_pos);
    hwnd_last = HWND_TOP;
}

void NewCredWizard::NewCredLayout::AddPanel(ControlWindow * cwnd, Slot slot)
{
    RECT r = {0, 0, 0, 0};

    assert(dwp != NULL);

    switch (slot) {
    case Hidden:
        break;              // nothing to do

    case Header:
        ::GetWindowRect(w->GetItem(IDC_NC_R_IDSEL), &r);
        break;

    case ContentNoHeader:
        {
            RECT r_1, r_2;

            ::GetWindowRect(w->GetItem(IDC_NC_R_IDSEL), &r_1);
            ::GetWindowRect(w->GetItem(IDC_NC_R_MAIN), &r_2);
            UnionRect(&r, &r_1, &r_2);
        }
        break;

    case ContentNormal:
        ::GetWindowRect(w->GetItem(IDC_NC_R_MAIN), &r);
        break;

    case ContentLarge:
        ::GetWindowRect(w->GetItem(IDC_NC_R_MAIN_LG), &r);
        break;

    case Footer:
        ::GetWindowRect(w->GetItem(IDC_NC_R_NAV), &r);
        break;

    default:
        assert(FALSE);
    }

    OffsetRect(&r, r_pos.left - r.left, r_pos.bottom - r.top);

    r_pos.right = __max(r_pos.right, r.right);
    r_pos.bottom = r.bottom;

    dwp = DeferWindowPos(dwp, cwnd->hwnd, hwnd_last,
                         r.left, r.top, r.right - r.left, r.bottom - r.top,
                         ((slot == Hidden)? SWP_HIDEONLY : SWP_MOVESIZEZ));
    if (slot != Hidden)
        hwnd_last = cwnd->hwnd;
}

BOOL NewCredWizard::NewCredLayout::Commit()
{
    BOOL rv;
    DWORD style;
    DWORD exstyle;

    assert(dwp != NULL);

    rv = EndDeferWindowPos(dwp);
    dwp = NULL;

    style = GetWindowLong(w->hwnd, GWL_STYLE);
    exstyle = GetWindowLong(w->hwnd, GWL_EXSTYLE);

    AdjustWindowRectEx(&r_pos, style, FALSE, exstyle);

    SetWindowPos(w->hwnd, NULL, 0, 0,
                 r_pos.right - r_pos.left, r_pos.bottom - r_pos.top,
                 SWP_SIZEONLY);

    return rv;
}

void NewCredWizard::UpdateLayout()
{
    HWND nextctl = NULL;
    NewCredLayout layout(this, 7);

    switch (page) {
    case NC_PAGE_IDSPEC:

        khui_cw_lock_nc(nc);
        nc->mode = KHUI_NC_MODE_MINI;

        layout.AddPanel(&m_idspec, NewCredLayout::ContentNoHeader);
        layout.AddPanel(&m_nav, NewCredLayout::Footer);
        layout.HidePanel(&m_idsel);
        layout.HidePanel(&m_privint.m_basic);
        layout.HidePanel(&m_privint.m_advanced);
        layout.HidePanel(&m_privint.m_cfgwiz);
        layout.HidePanel(&m_progress);
        layout.Commit();

        khui_cw_unlock_nc(nc);

        nextctl = m_idspec.UpdateLayout();
        m_nav.UpdateLayout();

        break;

    case NC_PAGE_CREDOPT_BASIC:

        khui_cw_lock_nc(nc);
        nc->mode = KHUI_NC_MODE_MINI;

        layout.AddPanel(&m_idsel, NewCredLayout::Header);
        layout.AddPanel(&m_privint.m_basic, NewCredLayout::ContentNormal);
        layout.AddPanel(&m_nav, NewCredLayout::Footer);
        layout.HidePanel(&m_privint.m_advanced);
        layout.HidePanel(&m_privint.m_cfgwiz);
        layout.HidePanel(&m_idspec);
        layout.HidePanel(&m_progress);
        layout.Commit();

        khui_cw_unlock_nc(nc);

        m_idsel.UpdateLayout();
        nextctl = m_privint.UpdateLayout();
        m_nav.UpdateLayout();

        break;

    case NC_PAGE_CREDOPT_ADV:

        khui_cw_lock_nc(nc);
        nc->mode = KHUI_NC_MODE_EXPANDED;

        layout.AddPanel(&m_idsel, NewCredLayout::Header);
        layout.AddPanel(&m_privint.m_advanced, NewCredLayout::ContentLarge);
        layout.AddPanel(&m_nav, NewCredLayout::Footer);
        layout.HidePanel(&m_privint.m_basic);
        layout.HidePanel(&m_privint.m_cfgwiz);
        layout.HidePanel(&m_idspec);
        layout.HidePanel(&m_progress);
        layout.Commit();

        khui_cw_unlock_nc(nc);

        m_idsel.UpdateLayout();
        nextctl = m_privint.UpdateLayout();
        m_nav.UpdateLayout();

        break;

    case NC_PAGE_CREDOPT_WIZ:

        khui_cw_lock_nc(nc);

        nc->mode = KHUI_NC_MODE_EXPANDED;

        layout.AddPanel(&m_idsel, NewCredLayout::Header);
        layout.AddPanel(&m_privint.m_cfgwiz, NewCredLayout::ContentLarge);
        layout.AddPanel(&m_nav, NewCredLayout::Footer);
        layout.HidePanel(&m_privint.m_basic);
        layout.HidePanel(&m_privint.m_advanced);
        layout.HidePanel(&m_idspec);
        layout.HidePanel(&m_progress);
        layout.Commit();

        khui_cw_unlock_nc(nc);

        m_idsel.UpdateLayout();
        nextctl = m_privint.UpdateLayout();
        m_nav.UpdateLayout();

        break;

    case NC_PAGE_PASSWORD:

        khui_cw_lock_nc(nc);
        nc->mode = KHUI_NC_MODE_MINI;

        layout.AddPanel(&m_idsel, NewCredLayout::Header);
        layout.AddPanel(&m_privint.m_basic, NewCredLayout::ContentNormal);
        layout.AddPanel(&m_nav, NewCredLayout::Footer);
        layout.HidePanel(&m_privint.m_advanced);
        layout.HidePanel(&m_privint.m_cfgwiz);
        layout.HidePanel(&m_idspec);
        layout.HidePanel(&m_progress);
        layout.Commit();

        khui_cw_unlock_nc(nc);

        m_idsel.UpdateLayout();
        nextctl = m_privint.UpdateLayout();
        m_nav.UpdateLayout();

        break;

    case NC_PAGE_PROGRESS:

        khui_cw_lock_nc(nc);
        nc->mode = KHUI_NC_MODE_MINI;

        layout.AddPanel(&m_idsel, NewCredLayout::Header);
        layout.AddPanel(&m_progress, NewCredLayout::ContentNormal);
        layout.AddPanel(&m_nav, NewCredLayout::Footer);
        layout.HidePanel(&m_privint.m_basic);
        layout.HidePanel(&m_privint.m_advanced);
        layout.HidePanel(&m_privint.m_cfgwiz);
        layout.HidePanel(&m_idspec);
        layout.Commit();

        khui_cw_unlock_nc(nc);

        m_idsel.UpdateLayout();
        m_progress.UpdateLayout();
        nextctl = m_nav.UpdateLayout();

        break;

    default:
        assert(FALSE);
    }

    if (nextctl)
        ::PostMessage(hwnd, WM_NEXTDLGCTL, (WPARAM) nextctl, TRUE);
}

static int __cdecl
tab_sort_func(const void * v1, const void * v2)
{
    /* v1 and v2 and of type : khui_new_creds_by_type ** */
    khui_new_creds_type_int *t1, *t2;

    t1 = ((khui_new_creds_type_int *) v1);
    t2 = ((khui_new_creds_type_int *) v2);

    assert(t1 != NULL && t2 != NULL && t1 != t2);

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
   beginning and the ordinals are updated to reflect the actual
   order of the types.
*/
void NewCredWizard::PrepCredTypes()
{
    khm_size i;
    khm_handle idpro = NULL;
    khm_int32 ctype = KCDB_CREDTYPE_INVALID;

    /* if we have an identity, we should make sure that the identity
       credentials type is at the top of the list.  So we mark the
       identity credentials type and let the compare function take
       over. */
    if (nc->n_identities > 0) {
        khm_size cb;

        cb = sizeof(ctype);
        kcdb_identity_get_attr(nc->identities[0], KCDB_ATTR_TYPE,
                               NULL, &ctype, &cb);
    }

    for (i=0; i < nc->n_types; i++) {
        nc->types[i].is_id_credtype = (ctype != KCDB_CREDTYPE_INVALID &&
                                       nc->types[i].nct->type == ctype);
    }

    qsort(nc->types, nc->n_types, sizeof(*(nc->types)), tab_sort_func);

    for (i=0; i < nc->n_types; i++) {
        nc->types[i].nct->ordinal = i+1;
    }
}
}
