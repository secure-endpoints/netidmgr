
#include "khmapp.h"

namespace {

    BOOL NewCredWizard::OnInitDialog(HWND hwndFocus, LPARAM lParam)
    {
        int x, y;
        int width, height;
        RECT r;
        HWND hwnd_parent = NULL;

        nc->hwnd = hwnd;

        assert(nc != NULL);
        assert(nc->subtype == KHUI_NC_SUBTYPE_NEW_CREDS ||
               nc->subtype == KHUI_NC_SUBTYPE_PASSWORD ||
               nc->subtype == KHUI_NC_SUBTYPE_IDSPEC ||
               nc->subtype == KHUI_NC_SUBTYPE_ACQPRIV_ID ||
               nc->subtype == KHUI_NC_SUBTYPE_CONFIG_ID ||
               nc->subtype == KHUI_NC_SUBTYPE_ACQDERIVED);

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

        nc->privint.hwnd_noprompts =
            CreateDialogParam(khm_hInstance,
                              MAKEINTRESOURCE(IDD_NC_NOPROMPTS),
                              hwnd, nc_noprompts_dlg_proc, (LPARAM) nc);

        nc->privint.hwnd_persist =
            CreateDialogParam(khm_hInstance,
                              MAKEINTRESOURCE(IDD_NC_PERSIST),
                              nc->privint.hwnd_advanced, nc_persist_dlg_proc, (LPARAM) nc);
#if _WIN32_WINNT >= 0x0501
        EnableThemeDialogTexture(nc->privint.hwnd_persist, ETDT_ENABLETAB);
#endif

        nc->idspec.hwnd =
            CreateDialogParam(khm_hInstance,
                              MAKEINTRESOURCE(IDD_NC_IDSPEC),
                              hwnd, nc_idspec_dlg_proc, (LPARAM) nc);

        nc->progress.hwnd =
            CreateDialogParam(khm_hInstance,
                              MAKEINTRESOURCE(IDD_NC_PROGRESS),
                              hwnd, nc_progress_dlg_proc, (LPARAM) nc);

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

        if (nc->is_modal) {
            switch (nc->subtype) {
            case KHUI_NC_SUBTYPE_ACQPRIV_ID:
                if (khm_cred_begin_new_cred_op()) {
                    kmq_post_message(KMSG_CRED, KMSG_CRED_ACQPRIV_ID, 0,
                                     (void *) nc);
                }
                break;

            default:
                assert(FALSE);
            }
        } else {
            /* add this to the dialog chain.  Doing so allows the main
               message dispatcher to use IsDialogMessage() to dispatch
               our messages. */
            khm_add_dialog(hwnd);
        }

        SetWindowText(hwnd, nc->window_title);

        return TRUE;
    }

    void NewCredWizard::OnDestroy(void)
    {
        khm_del_dialog(hwnd);
        delete this;
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

        HWND hw = NULL;
        HWND hw_ctrl;

        if (nc->subtype != KHUI_NC_SUBTYPE_NEW_CREDS &&
            nc->subtype != KHUI_NC_SUBTYPE_PASSWORD)
            return TRUE;

        if (hlp->iContextType != HELPINFO_WINDOW)
            return TRUE;

        if (hlp->hItemHandle != NULL &&
            hlp->hItemHandle != hwnd) {
            DWORD id;
            int i;

            hw_ctrl = hlp->hItemHandle;

            id = GetWindowLong(hw_ctrl, GWL_ID);
            for (i=0; ctxids[i] != 0; i += 2)
                if (ctxids[i] == id)
                    break;

            if (ctxids[i] != 0)
                hw = khm_html_help(hw_ctrl,
                                   L"::popups_newcreds.txt",
                                   HH_TP_HELP_WM_HELP,
                                   (DWORD_PTR) ctxids);
        }

        if (hw == NULL) {
            khm_html_help(hwnd, NULL, HH_HELP_CONTEXT,
                          ((nc->subtype == KHUI_NC_SUBTYPE_NEW_CREDS)?
                           IDH_ACTION_NEW_ID: IDH_ACTION_PASSWD_ID));
        }

        return TRUE;
    }

    void NewCredWizard::OnClose()
    {
        Navigate(NC_PAGET_CANCEL);
    }

    void NewCredWizard::OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
    {
        if (state == WA_ACTIVE || state == WA_CLICKACTIVE) {

            FLASHWINFO fi;
            DWORD_PTR ex_style;

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
    }

    void NewCredWizard::OnDeriveFromPrivCred(khui_collect_privileged_creds_data * pcd)
    {
        khui_new_creds * nc_child;
        khm_handle cs_privcred = NULL;

        khui_cw_create_cred_blob(&nc_child);
        nc_child->subtype = KHUI_NC_SUBTYPE_ACQDERIVED;
        khui_context_create(&nc_child->ctx,
                            KHUI_SCOPE_IDENT,
                            pcd->target_identity,
                            -1, NULL);
        kcdb_credset_create(&cs_privcred);
        kcdb_credset_collect(cs_privcred, pcd->dest_credset, NULL, KCDB_CREDTYPE_ALL, NULL);
        khui_cw_set_privileged_credential_collector(nc_child, cs_privcred);
        nc_child->parent = nc;
        nc->n_children++;
        if (khm_cred_begin_new_cred_op()) {
            kmq_post_message(KMSG_CRED, KMSG_CRED_RENEW_CREDS, 0, (void *) nc_child);
        } else {
            khui_cw_destroy_cred_blob(nc_child);
        }
    }

    void NewCredWizard::OnCollectPrivCred(khui_collect_privileged_creds_data * pcd)
    {
        khui_new_creds * nc_child;

        pcd = (khui_collect_privileged_creds_data *) vParam;

        khui_cw_create_cred_blob(&nc_child);
        nc_child->subtype = KHUI_NC_SUBTYPE_ACQPRIV_ID;
        khui_context_create(&nc_child->ctx,
                            KHUI_SCOPE_IDENT,
                            pcd->target_identity,
                            -1, NULL);
        nc->persist_privcred = TRUE;
        khui_cw_set_privileged_credential_collector(nc_child, pcd->dest_credset);
        khm_do_modal_newcredwnd(nc->hwnd, nc_child);
    }

    void NewCredWizard::OnProcessComplete(int has_error)
    {
        if (nc->n_children != 0)
            return TRUE;

        nc->response &= ~KHUI_NC_RESPONSE_PROCESSING;

        if (nc->response & KHUI_NC_RESPONSE_NOEXIT) {

            EnableControls( TRUE );

            /* reset state */
            nc->result = KHUI_NC_RESULT_CANCEL;

            NotifyTypes( WMNC_DIALOG_PROCESS_COMPLETE, 0, TRUE);

            if (has_error) {
                nc->nav.transitions = NC_TRANS_RETRY | NC_TRANS_CLOSE | NC_TRANS_PREV;
                nc_layout_nav(nc);
                return TRUE;
            } if (nc->subtype == KHUI_NC_SUBTYPE_NEW_CREDS ||
                  nc->subtype == KHUI_NC_SUBTYPE_ACQPRIV_ID) {
                Navigate( NC_PAGE_CREDOPT_BASIC);
            } else if (nc->subtype == KHUI_NC_SUBTYPE_PASSWORD) {
                Navigate( NC_PAGE_PASSWORD);
            } else {
                assert(FALSE);
            }
        }

        Navigate( NC_PAGET_END);
    }

    void NewCredWizard::OnSetPrompts()
    {
        nc->privint.initialized = FALSE;

        if (nc->page == NC_PAGE_PASSWORD ||
            nc->page == NC_PAGE_CREDOPT_ADV ||
            nc->page == NC_PAGE_CREDOPT_BASIC) {

            UpdateLayout();
        }
    }

    void NewCredWizard::OnIdentityStateChange(const nc_identity_state_notification * notif)
    {
        khm_int32 idflags;
        wchar_t buf[80];
        khm_int32 nflags;

        nflags = notif->flags;

        if (nc->n_identities == 0) {
            /* no identities */

            assert(FALSE);

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

        if (nflags & KHUI_CWNIS_READY) {
            nc->nav.transitions |= NC_TRANS_FINISH;
            nc->nav.state |= NC_NAVSTATE_OKTOFINISH;
            nc_layout_nav(nc);
        } else {
            nc->nav.transitions &= ~NC_TRANS_FINISH;
            nc->nav.state &= ~NC_NAVSTATE_OKTOFINISH;
            nc_layout_nav(nc);
        }

    }

    void NewCredWizard::OnNewIdentity()
    {
        NotifyNewIdentity( TRUE );
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
                /* If there is a primary identity, then we can start
                   in the credentials options page */

                khm_int32 idflags = 0;
                khm_handle ident;

                khm_handle parent = NULL;

                ident = nc->identities[0];

                if (KHM_SUCCEEDED(kcdb_identity_get_parent(ident, &parent)) &&
                    parent != NULL) {

                    kcdb_identity_release(ident);
                    ident = nc->identities[0] = parent;

                }

                NotifyNewIdentity( FALSE );

                assert(ident != NULL);
                kcdb_identity_get_flags(ident, &idflags);

                /* Check if this identity has a configuration.  If so,
                   we can continue in basic mode.  Otherwise we should
                   start in advanced mode so that the user can specify
                   identity options to be used the next time. */
                if (idflags & KCDB_IDENT_FLAG_CONFIG) {
                    Navigate( NC_PAGE_CREDOPT_BASIC);
                } else {
                    Navigate( NC_PAGE_CREDOPT_ADV);
                }
            } else {
                /* No primary identity.  We have to open with the
                   identity specification page */
                Navigate( NC_PAGE_IDSPEC);
            }

            break;

        case KHUI_NC_SUBTYPE_PASSWORD:
            if (nc->n_identities > 0) {
                NotifyNewIdentity( FALSE );
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

        ShowWindow(hwnd, SW_SHOWNORMAL);

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

                nc->flashing_enabled = TRUE;
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

        assert(nc->privint.hwnd_advanced != NULL);

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
        if (nc == NULL || nc->privint.hwnd_current == NULL)
            return FALSE;

        SendMessage(nc->privint.hwnd_current, KHUI_WM_NC_NOTIFY, 
                    MAKEWPARAM(0, WMNC_DIALOG_MOVE), (LPARAM) nc);
    }

    void NewCredWizard::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
    {
        if (code == BN_CLICKED) {
            switch (id) {
            case IDCANCEL:
                Navigate( NC_PAGET_CANCEL);
                return TRUE;

            case IDOK:
                Navigate( NC_PAGET_DEFAULT);
                return TRUE;
            }
        }
    
        if (hwndCtl != 0) {
            HWND owner = GetParent(hwndCtl);
            if (owner != NULL && owner != hwnd) {
                FORWARD_WM_COMMAND(owner, id, hwndCtl, code, PostMessage);
                return TRUE;
            }
        }
    }

    void NewCredWizard::Navigate( NewCredPage new_page )
    {
    /* All the page transitions in the new credentials wizard happen
       here. */

        switch (new_page) {
        case NC_PAGE_NONE:
            /* Do nothing */
            assert(FALSE);
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
            nc->nav.transitions &= ~(NC_TRANS_ABORT | NC_TRANS_SHOWCLOSEIF);
            nc->page = NC_PAGE_CREDOPT_BASIC;
            break;

        case NC_PAGE_CREDOPT_ADV:
            nc->nav.transitions &= ~(NC_TRANS_ABORT | NC_TRANS_SHOWCLOSEIF);
            nc->page = NC_PAGE_CREDOPT_ADV;
            break;

        case NC_PAGE_PASSWORD:
            nc->nav.transitions &= ~(NC_TRANS_ABORT | NC_TRANS_SHOWCLOSEIF);
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
                        nc->nav.transitions &= ~(NC_TRANS_ABORT | NC_TRANS_SHOWCLOSEIF);
                        nc->page = NC_PAGE_CREDOPT_ADV;
                        break;
                        
                    case KHUI_NC_SUBTYPE_ACQPRIV_ID:
                        nc->nav.transitions &= ~(NC_TRANS_ABORT | NC_TRANS_SHOWCLOSEIF);
                        nc->page = NC_PAGE_CREDOPT_BASIC;
                        break;

                    case KHUI_NC_SUBTYPE_PASSWORD:
                        nc->nav.transitions &= ~(NC_TRANS_ABORT | NC_TRANS_SHOWCLOSEIF);
                        nc->page = NC_PAGE_PASSWORD;
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

                    p = nc->privint.shown.current_panel;
                    if (p) {
                        p->processed = TRUE;
                        p = QNEXT(p);
                    }
                    if (p == NULL)
                        khui_cw_get_next_privint(nc, &p);
                    nc->privint.shown.current_panel = p;
                    nc->nav.transitions &= ~(NC_TRANS_ABORT | NC_TRANS_SHOWCLOSEIF);
                }
                break;

            default:
                assert(FALSE);
            }
            break;

        case NC_PAGET_PREV:
            switch (nc->page) {
            case NC_PAGE_IDSPEC:
                if (nc->idspec.prev_page != NC_PAGE_NONE) {
                    nc->page = nc->idspec.prev_page;
                    nc->nav.transitions &= ~(NC_TRANS_ABORT | NC_TRANS_SHOWCLOSEIF);
                }
                break;

            case NC_PAGE_CREDOPT_ADV:
            case NC_PAGE_CREDOPT_BASIC:
            case NC_PAGE_PASSWORD:
                {
                    khui_new_creds_privint_panel * p;

                    p = nc->privint.shown.current_panel;
                    if (p)
                        p = QPREV(p);
                    nc->privint.shown.current_panel = p;
                    nc->nav.transitions &= ~(NC_TRANS_ABORT | NC_TRANS_SHOWCLOSEIF);
                }
                break;

            case NC_PAGE_PROGRESS:
                {
                    if (nc->subtype == KHUI_NC_SUBTYPE_NEW_CREDS ||
                        nc->subtype == KHUI_NC_SUBTYPE_ACQPRIV_ID)
                        nc->page = NC_PAGE_CREDOPT_BASIC;
                    else if (nc->subtype == KHUI_NC_SUBTYPE_PASSWORD)
                        nc->page = NC_PAGE_PASSWORD;
                    else {
                        assert(FALSE);
                    }

                    nc->nav.transitions = 0;
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
            case KHUI_NC_SUBTYPE_ACQPRIV_ID:
                nc->result = KHUI_NC_RESULT_PROCESS;
                NotifyTypes( WMNC_DIALOG_PREPROCESS, (LPARAM) nc, TRUE);
                khm_cred_dispatch_process_message(nc);
                nc->nav.transitions = NC_TRANS_ABORT | NC_TRANS_SHOWCLOSEIF;
                nc->page = NC_PAGE_PROGRESS;
                break;

            default:
                assert(FALSE);
            }
            break;

        case NC_PAGET_CANCEL:
            if (nc->nav.state & NC_NAVSTATE_CANCELLED)
                break;

            if (nc->nav.state & NC_NAVSTATE_PREEND) {

                nc->nav.state &= ~NC_NAVSTATE_NOCLOSE;
                Navigate( NC_PAGET_END);
                return;

            } else {

                nc->result = KHUI_NC_RESULT_CANCEL;
                NotifyTypes( WMNC_DIALOG_PREPROCESS, (LPARAM) nc, TRUE);
                khm_cred_dispatch_process_message(nc);
                nc->nav.transitions = 0;
                nc->page = NC_PAGE_PROGRESS;
                nc->nav.state |= NC_NAVSTATE_CANCELLED;
            }
            break;

        case NC_PAGET_END:
            if ((nc->nav.state & NC_NAVSTATE_NOCLOSE) &&
                !(nc->nav.state & NC_NAVSTATE_CANCELLED)) {

                nc->nav.state |= NC_NAVSTATE_PREEND;
                nc->nav.transitions = NC_TRANS_CLOSE;
                nc->page = NC_PAGE_PROGRESS;

            } else {
                khm_size i;
                if (nc->is_modal) {
                    if (nc->subtype == KHUI_NC_SUBTYPE_ACQPRIV_ID) {
                        /* prevent the collector credential set from being
                           destroyed */
                        khui_cw_set_privileged_credential_collector(nc, NULL);
                    }
                    for (i=0; i < nc->n_types; i++)
                        if (nc->types[i].nct->hwnd_panel) {
                            DestroyWindow(nc->types[i].nct->hwnd_panel);
                            nc->types[i].nct->hwnd_panel = NULL;
                        }
                    EndDialog(nc->hwnd, nc->result);
                } else {
                    DestroyWindow(nc->hwnd);
                }
                kmq_post_message(KMSG_CRED, KMSG_CRED_END, 0, (void *) nc);
                return;

            }
            break;

        case NC_PAGET_DEFAULT:
            {
                new_page = 
                    (nc->nav.transitions & NC_TRANS_NEXT)?   NC_PAGET_NEXT :
                    (nc->nav.transitions & NC_TRANS_FINISH)? NC_PAGET_FINISH :
                    (nc->nav.transitions & NC_TRANS_RETRY)?  NC_PAGET_FINISH :
                    (nc->nav.transitions & NC_TRANS_CLOSE)?  NC_PAGET_CANCEL :
                    (nc->nav.transitions & NC_TRANS_ABORT)?  NC_PAGET_CANCEL :
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
        khm_handle  k5idpro = NULL;
        khm_handle  idpro = NULL;
        khm_boolean default_off;
        khm_boolean isk5 = FALSE;

        /* For backwards compatibility, we assume that if there is no
           primary identity or if the primary identity is not from the
           Kerberos v5 identity provider, then the default for all
           plug-ins is to be disabled and must be explicitly enabled. */

        kcdb_identpro_find(L"Krb5Ident", &k5idpro);

        /* All the privileged interaction panels are assumed to no longer
           be valid */
        khui_cw_clear_all_privints(nc);

        khui_cw_lock_nc(nc);

        if (k5idpro == NULL ||
            nc->n_identities == 0 ||
            KHM_FAILED(kcdb_identity_get_identpro(nc->identities[0], &idpro)) ||
            !kcdb_identpro_is_equal(idpro, k5idpro))

            default_off = TRUE;

        else {

            isk5 = TRUE;
            default_off = FALSE;

        }

        if (k5idpro) {
            kcdb_identpro_release(k5idpro);
            k5idpro = NULL;
        }

        if (idpro) {
            kcdb_identpro_release(idpro);
            idpro = NULL;
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
        nc->nav.state &= ~NC_NAVSTATE_OKTOFINISH;

        if (nc->subtype == KHUI_NC_SUBTYPE_ACQPRIV_ID)
            nc->persist_privcred = TRUE;
        else
            nc->persist_privcred = FALSE;

        if (nc->persist_identity)
            kcdb_identity_release(nc->persist_identity);
        nc->persist_identity = NULL;

        khui_cw_unlock_nc(nc);

        NotifyTypes( WMNC_IDENTITY_CHANGE, (LPARAM) nc, TRUE);
        PrepCredTypes();

        khui_cw_lock_nc(nc);

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

    void NewCredWizard::NotifyTypes(khui_wm_nc_notifications N, LPARAM lParam, bool sync)
    {
        khm_size i;
        
        for (i=0; i < c->n_types; i++) {
            if (nc->types[i].nct->hwnd_panel == NULL)
                continue;

            if (sync) {
                SendMessage(nc->types[i].nct->hwnd_panel, KHUI_WM_NC_NOTIFY,
                            MAKEWPARAM(0, N), lParam);
            } else {
                PostMessage(nc->types[i].nct->hwnd_panel, KHUI_WM_NC_NOTIFY,
                            MAKEWPARAM(0, N), lParam);
            }
        }
    }

    void NewCredWizard::EnableControls(bool enable)
    {
        EnableWindow(nc->nav.hwnd, enable);
        EnableWindow(nc->idsel.hwnd, enable);
    }

    void NewCredWizard::UpdateLayout()
    {
        RECT r_idsel;
        RECT r_main;
        RECT r_main_lg;
        RECT r_nav_lg;
        RECT r_nav_sm;

        RECT r;
        HDWP dwp = NULL;
        BOOL drv;
        HWND nextctl = NULL;

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
            ResizeWindow();

            khui_cw_unlock_nc(nc);

            nextctl = nc_layout_idspec(nc);
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
            ResizeWindow();

            khui_cw_unlock_nc(nc);

            nc_layout_idsel(nc);
            nextctl = nc_layout_privint(nc);
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
            ResizeWindow();

            khui_cw_unlock_nc(nc);

            nc_layout_idsel(nc);
            nextctl = nc_layout_privint(nc);
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
            ResizeWindow();

            khui_cw_unlock_nc(nc);

            nc_layout_idsel(nc);
            nextctl = nc_layout_privint(nc);
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
            ResizeWindow();

            khui_cw_unlock_nc(nc);

            nc_layout_idsel(nc);
            nc_layout_progress(nc);
            nextctl = nc_layout_nav(nc);

            break;

        default:
            assert(FALSE);
        }

#undef dlg_item

        if (nextctl)
            PostMessage(nc->hwnd, WM_NEXTDLGCTL, (WPARAM) nextctl, TRUE);
    }

    void NewCredWizard::ResizeWindow()
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
