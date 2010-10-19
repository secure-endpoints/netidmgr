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

static khm_boolean in_dialog = FALSE;
static CRITICAL_SECTION cs_dialog;
static HANDLE in_dialog_evt = NULL;
static khm_int32 dialog_result = 0;
static wchar_t dialog_identity[KCDB_IDENT_MAXCCH_NAME];
static khui_new_creds * dialog_nc = NULL;

DECLARE_ONCE(dialog_init_once);

static void
dialog_sync_init(void) {
    if (InitializeOnce(&dialog_init_once)) {
        InitializeCriticalSection(&cs_dialog);
        in_dialog_evt = CreateEvent(NULL, TRUE, TRUE,
                                    L"DialogCompletionEvent");

        InitializeOnceDone(&dialog_init_once);
    }
}

BOOL
khm_cred_begin_dialog(void) {
    BOOL rv;

    dialog_sync_init();

    EnterCriticalSection(&cs_dialog);

    if (in_dialog) {
        rv = FALSE;

        /* if a dialog is being displayed and we got a another request
           to show one, we bring the existing one to the
           foreground. */
        if (dialog_nc && dialog_nc->hwnd) {
            khm_int32 t = 0;

            if (KHM_SUCCEEDED(khc_read_int32(NULL,
                                             L"CredWindow\\Windows\\NewCred\\ForceToTop",
                                             &t)) &&
                t != 0) {

                khm_activate_main_window();

                SetWindowPos(dialog_nc->hwnd, HWND_TOP, 0, 0, 0, 0,
                             (SWP_NOMOVE | SWP_NOSIZE));
            }
        }

    } else {
        rv = TRUE;
        in_dialog = TRUE;
        ResetEvent(in_dialog_evt);
    }

    LeaveCriticalSection(&cs_dialog);
    return rv;
}

void
khm_cred_end_dialog(khui_new_creds * nc) {
    dialog_sync_init();

    EnterCriticalSection(&cs_dialog);
    if (in_dialog) {
        in_dialog = FALSE;
        SetEvent(in_dialog_evt);
    }
    if (nc != NULL) {
        dialog_result = nc->result;
        assert(dialog_nc == nc);
        dialog_nc = NULL;
        if ((nc->subtype == KHUI_NC_SUBTYPE_NEW_CREDS ||
             nc->subtype == KHUI_NC_SUBTYPE_IDSPEC) &&
            nc->n_identities > 0 &&
            nc->identities[0]) {
            khm_size cb;

            cb = sizeof(dialog_identity);
            if (KHM_FAILED(kcdb_identity_get_short_name(nc->identities[0],
                                                        FALSE,
                                                        dialog_identity, &cb)))
                dialog_identity[0] = 0;
        } else {
            dialog_identity[0] = 0;
        }
    } else {
        assert(dialog_nc == NULL);
    }
    LeaveCriticalSection(&cs_dialog);
}

BOOL
khm_cred_is_in_dialog(void) {
    BOOL rv;

    dialog_sync_init();

    EnterCriticalSection(&cs_dialog);
    rv = in_dialog;
    LeaveCriticalSection(&cs_dialog);

    return rv;
}

khm_int32
khm_cred_wait_for_dialog(DWORD timeout, khm_int32 * result,
                         wchar_t * ident, khm_size cb_ident) {
    khm_int32 rv;

    dialog_sync_init();

    EnterCriticalSection(&cs_dialog);
    if (!in_dialog)
        rv = KHM_ERROR_NOT_FOUND;
    else {
        DWORD dw;

        do {
            LeaveCriticalSection(&cs_dialog);

            dw = WaitForSingleObject(in_dialog_evt, timeout);

            EnterCriticalSection(&cs_dialog);

            if (!in_dialog) {
                rv = KHM_ERROR_SUCCESS;
                if (result) {
                    *result = dialog_result;
                }
                if (ident) {
                    StringCbCopy(ident, cb_ident, dialog_identity);
                }
                break;
            } else if(dw == WAIT_TIMEOUT) {
                rv = KHM_ERROR_TIMEOUT;
                break;
            }
        } while(TRUE);
    }
    LeaveCriticalSection(&cs_dialog);

    return rv;
}

static volatile LONG pending_new_cred_ops = 0;

khm_boolean
khm_cred_begin_new_cred_op(void)
{
    InterlockedIncrement(&pending_new_cred_ops);
    if (khm_exiting_application()) {
        khm_cred_end_new_cred_op();
        return FALSE;
    }
    return TRUE;
}

void
khm_cred_end_new_cred_op(void)
{
    LONG pending;
    khm_boolean exiting;

    exiting = khm_exiting_application();
    pending = InterlockedDecrement(&pending_new_cred_ops);
    assert(pending >= 0);
    if (pending == 0) {
        if (exiting)
            khm_exit_application();
        else if (khm_startup.processing)
            kmq_post_message(KMSG_ACT, KMSG_ACT_CONTINUE_CMDLINE, 0, 0);
    }
}

khm_boolean
khm_new_cred_ops_pending(void)
{
    return (pending_new_cred_ops > 0);
}

void KHMCALLBACK
khm_new_cred_progress_broadcast(enum kherr_ctx_event evt,
                                kherr_ctx_event_data * data,
                                void * vparam)
{
    khui_new_creds * nc = (khui_new_creds *) vparam;
    khm_handle identity;

    if (nc->n_identities > 0)
        identity = nc->identities[0];
    else
        identity = nc->ctx.identity;

    switch (evt) {
    case KHERR_CTX_BEGIN:
        kmq_post_message(KMSG_CREDP, KMSG_CREDP_BEGIN_NEWCRED, nc->subtype,
                         identity);
        break;

    case KHERR_CTX_END:
        kmq_post_message(KMSG_CREDP, KMSG_CREDP_END_NEWCRED, nc->subtype,
                         identity);
        break;

    case KHERR_CTX_PROGRESS:
        kmq_post_message(KMSG_CREDP, KMSG_CREDP_PROG_NEWCRED,
                         ((((khm_ui_4)nc->subtype) << 16)|(data->data.progress)),
                         identity);
        break;
    }
}

/* Completion handler for KMSG_CRED messages.  We control the overall
   logic of credentials acquisition and other operations here.  Once a
   credentials operation is triggered, each successive message
   completion notification will be used to dispatch the messages for
   the next step in processing the operation. */
void KHMAPI
kmsg_cred_completion(kmq_message *m)
{
    khui_new_creds * nc;

#ifdef DEBUG
    assert(m->type == KMSG_CRED);
#else
    if(m->type != KMSG_CRED)
        return; /* huh? */
#endif

    switch(m->subtype) {
    case KMSG_CRED_PASSWORD:
        /* fallthrough */
    case KMSG_CRED_NEW_CREDS:
        /* fallthrough */
    case KMSG_CRED_ACQPRIV_ID:
        /* Cred types have attached themselves.  Trigger the next
           phase. */
        kmq_post_message(KMSG_CRED, KMSG_CRED_DIALOG_SETUP, 0,
                         m->vparam);
        break;

    case KMSG_CRED_IDSPEC:
        nc = (khui_new_creds *) m->vparam;
        khm_show_newcredwnd(nc->hwnd);
        break;

    case KMSG_CRED_RENEW_CREDS:
        nc = (khui_new_creds *) m->vparam;

        /* khm_cred_dispatch_process_message() deals with the case
           where there are no credential types that wants to
           participate in this operation. */
        if (nc->subtype == KHUI_NC_SUBTYPE_ACQDERIVED)
            kmq_post_message(KMSG_CRED, KMSG_CRED_PREPROCESS_ID, 0,
                             m->vparam);
        else
            khm_cred_dispatch_process_message(nc);
        break;

    case KMSG_CRED_PREPROCESS_ID:
        nc = (khui_new_creds *) m->vparam;

        nc->original_subtype = KHUI_NC_SUBTYPE_ACQDERIVED;
        nc->subtype = KHUI_NC_SUBTYPE_RENEW_CREDS;

        khm_cred_dispatch_process_message(nc);
        break;

    case KMSG_CRED_DIALOG_SETUP:
        nc = (khui_new_creds *) m->vparam;

        khm_prep_newcredwnd(nc->hwnd);

        /* all the controls have been created.  Now initialize them */
        if (nc->n_types > 0) {
            kmq_post_subs_msg(nc->type_subs,
                              nc->n_types,
                              KMSG_CRED,
                              KMSG_CRED_DIALOG_PRESTART,
                              0,
                              m->vparam);
        } else {
            PostMessage(nc->hwnd, KHUI_WM_NC_NOTIFY,
                        MAKEWPARAM(0, WMNC_DIALOG_PROCESS_COMPLETE), 0);
        }
        break;

    case KMSG_CRED_DIALOG_PRESTART:
        /* all prestart stuff is done.  Now to activate the dialog */
        nc = (khui_new_creds *) m->vparam;
        khm_show_newcredwnd(nc->hwnd);

        kmq_post_subs_msg(nc->type_subs,
                          nc->n_types,
                          KMSG_CRED,
                          KMSG_CRED_DIALOG_START,
                          0,
                          m->vparam);
        /* at this point, the dialog window takes over.  We let it run
           the show until KMSG_CRED_DIALOG_END is posted by the dialog
           procedure. */
        break;

    case KMSG_CRED_PROCESS:
        /* a wave of these messages have completed.  We should check
           if there's more */
        nc = (khui_new_creds *) m->vparam;

        /* if we are done processing all the plug-ins, then check if
           there were any errors reported.  Otherwise we dispatch
           another set of messages. */
        if(!khm_cred_dispatch_process_level(nc)) {

            khm_boolean has_error;

            has_error = khm_cred_conclude_processing(nc);

            if (nc->subtype == KMSG_CRED_RENEW_CREDS) {
                if (nc->n_children == 0)
                    kmq_post_message(KMSG_CRED, KMSG_CRED_END, 0,
                                     m->vparam);
            } else {
                PostMessage(nc->hwnd, KHUI_WM_NC_NOTIFY,
                            MAKEWPARAM(has_error, WMNC_DIALOG_PROCESS_COMPLETE),
                            0);
            }
        }
        break;

    case KMSG_CRED_END:
        /* all is done. */
        {
            khui_new_creds * nc;

            nc = (khui_new_creds *) m->vparam;

            switch (nc->subtype) {
            case KHUI_NC_SUBTYPE_NEW_CREDS:
            case KHUI_NC_SUBTYPE_PASSWORD:

                khm_cred_end_dialog(nc);

                /* fallthrough */

            case KHUI_NC_SUBTYPE_ACQPRIV_ID:

                khm_cred_end_new_cred_op();

                break;

            case KHUI_NC_SUBTYPE_IDSPEC:

                /* IDSPEC subtypes are only spawned by
                   khm_cred_prompt_for_identity_modal() and requires
                   that nc be kept alive after the dialog dies.  They
                   do not post KMSG_CRED_END messages. */

                assert(FALSE);
                return;

            case KHUI_NC_SUBTYPE_RENEW_CREDS:

                /* if this is a renewal that was triggered while we
                   were processing the commandline, then we need to
                   update the pending renewal count. */

                khm_cred_end_new_cred_op();

                break;
            }

            if (nc->parent) {
                nc->parent->n_children--;
                if (nc->parent->n_children == 0) {
                    if (nc->parent->hwnd != NULL)
                        PostMessage(nc->parent->hwnd, KHUI_WM_NC_NOTIFY,
                                    MAKEWPARAM(0, WMNC_DIALOG_PROCESS_COMPLETE), 0);
                    else
                        kmq_post_message(KMSG_CRED, KMSG_CRED_END, 0,
                                         nc->parent);
                }
            }

            khui_cw_destroy_cred_blob(nc);

            kmq_post_message(KMSG_CRED, KMSG_CRED_REFRESH, 0, 0);
        }
        break;

        /* property sheet stuff */

    case KMSG_CRED_PP_BEGIN:

        /* all the pages should have been added by now.  Just send out
           the precreate message */
        kmq_post_message(KMSG_CRED, KMSG_CRED_PP_PRECREATE, 0,
                         m->vparam);
        break;

    case KMSG_CRED_PP_END:
        kmq_post_message(KMSG_CRED, KMSG_CRED_PP_DESTROY, 0,
                         m->vparam);
        break;

    case KMSG_CRED_DESTROY_CREDS:
#ifdef DEBUG
        assert(m->vparam != NULL);
#endif
        khui_context_release((khui_action_context *) m->vparam);
        PFREE(m->vparam);

        kmq_post_message(KMSG_CRED, KMSG_CRED_REFRESH, 0, 0);

        kmq_post_message(KMSG_ACT, KMSG_ACT_CONTINUE_CMDLINE, 0, 0);
        break;

    case KMSG_CRED_IMPORT:
        {
            /* once an import operation ends, we have to trigger a
               renewal so that other plug-ins that didn't participate
               in the import operation can have a chance at getting
               the necessary credentials.

               If we are in the middle of processing the commandline,
               we have to be a little bit careful.  We can't issue a
               commandline conituation message right now because the
               import action is still ongoing (since the renewals are
               part of the action).  Once the renewals have completed,
               the completion handler will automatically issue a
               commandline continuation message.  However, if there
               were no identities to renew, then we have to issue the
               message ourselves.
            */

            khm_cred_renew_all_identities();

            khm_cred_end_new_cred_op();
        }
        break;

    case KMSG_CRED_REFRESH:
        kcdb_identity_refresh_all();

        if (m->vparam == &khm_startup) {

            /* We should start processing the command-line */

            kmq_post_message(KMSG_ACT, KMSG_ACT_BEGIN_CMDLINE, 0, 0);
        }
        break;
    }
}

void khm_cred_import(void)
{
    if (khm_cred_begin_new_cred_op()) {
        _begin_task(KHERR_CF_TRANSITIVE);
        _report_sr0(KHERR_NONE, IDS_CTX_IMPORT);
        _describe();

        kmq_post_message(KMSG_CRED, KMSG_CRED_IMPORT, 0, 0);

        _end_task();
    }
}

void khm_cred_set_default(void)
{
    khui_action_context ctx;
    khm_int32 rv;

    khui_context_get(&ctx);

    if (ctx.identity) {
        rv = kcdb_identity_set_default(ctx.identity);
    }

    khui_context_release(&ctx);
}

void khm_cred_set_default_identity(khm_handle identity)
{
    kcdb_identity_set_default(identity);
}

void khm_cred_show_identity_options()
{
    khui_action_context ctx;

    khui_context_get(&ctx);

    if (ctx.identity)
        khm_show_identity_config_pane(ctx.identity);

    khui_context_release(&ctx);
}

void khm_cred_prompt_for_identity_modal(const wchar_t * w_title,
                                        khm_handle *pidentity)
{
    khui_new_creds * nc;

    if (!khm_cred_begin_dialog())
        return;

    khui_cw_create_cred_blob(&nc);
    nc->subtype = KHUI_NC_SUBTYPE_IDSPEC;
    dialog_nc = nc;

    if (w_title)
        nc->window_title = PWCSDUP(w_title);

    if (*pidentity)
        khui_cw_set_primary_id(nc, *pidentity);

    khm_do_modal_newcredwnd(khm_hwnd_main, nc);

    if (nc->n_identities > 0) {
        if (!kcdb_identity_is_equal(nc->identities[0], *pidentity)) {
            if (*pidentity)
                kcdb_identity_release(*pidentity);
            *pidentity = nc->identities[0];
            kcdb_identity_hold(*pidentity);
        }
    } else {
        if (*pidentity)
            kcdb_identity_release(*pidentity);
    }

    khui_cw_destroy_cred_blob(nc);
}

void khm_cred_destroy_creds(khm_boolean sync, khm_boolean quiet)
{
    khui_action_context * pctx;

    pctx = PMALLOC(sizeof(*pctx));
#ifdef DEBUG
    assert(pctx);
#endif

    khui_context_get(pctx);

    if(pctx->scope == KHUI_SCOPE_NONE && !quiet) {
        /* this really shouldn't be necessary once we start enabling
           and disbling actions based on context */
        wchar_t title[256];
        wchar_t message[256];

        LoadString(khm_hInstance,
                   IDS_ALERT_NOSEL_TITLE,
                   title,
                   ARRAYLENGTH(title));

        LoadString(khm_hInstance,
                   IDS_ALERT_NOSEL,
                   message,
                   ARRAYLENGTH(message));

        khui_alert_show_simple(title,
                               message,
                               KHERR_WARNING);

        khui_context_release(pctx);
        PFREE(pctx);

        return;
    }

    _begin_task(KHERR_CF_TRANSITIVE);
    _report_sr0(KHERR_NONE, IDS_CTX_DESTROY_CREDS);
    _describe();

    if (sync)
        kmq_send_message(KMSG_CRED,
                         KMSG_CRED_DESTROY_CREDS,
                         0,
                         (void *) pctx);
    else
        kmq_post_message(KMSG_CRED,
                         KMSG_CRED_DESTROY_CREDS,
                         0,
                         (void *) pctx);

    _end_task();
}

void khm_cred_destroy_identity(khm_handle identity)
{
    khui_action_context * pctx;
    wchar_t idname[KCDB_IDENT_MAXCCH_NAME];
    khm_size cb;

    if (identity == NULL)
        return;

    pctx = PMALLOC(sizeof(*pctx));
#ifdef DEBUG
    assert(pctx);
#endif

    khui_context_create(pctx,
                        KHUI_SCOPE_IDENT,
                        identity,
                        KCDB_CREDTYPE_INVALID,
                        NULL);

    cb = sizeof(idname);
    kcdb_identity_get_name(identity, idname, &cb);

    _begin_task(KHERR_CF_TRANSITIVE);
    _report_sr1(KHERR_NONE, IDS_CTX_DESTROY_ID, _dupstr(idname));
    _describe();

    kmq_post_message(KMSG_CRED,
                     KMSG_CRED_DESTROY_CREDS,
                     0,
                     (void *) pctx);

    _end_task();
}

void khm_cred_renew_all_identities(void)
{
    kcdb_enumeration e;
    khm_size n_idents = 0;
    khm_handle h_ident = NULL;

    if (KHM_FAILED(kcdb_identity_begin_enum(KCDB_IDENT_FLAG_EMPTY, 0,
                                            &e, &n_idents)))
        return;

    while (KHM_SUCCEEDED(kcdb_enum_next(e, &h_ident))) {
        khm_cred_renew_identity(h_ident);
    }

    kcdb_enum_end(e);
}

/*! \internal
  \brief Determine whether an identity should be renewed
 */
khm_boolean should_identity_be_renewed(khm_handle identity)
{
    khm_int32 flags = 0;

    if (FAILED(kcdb_identity_get_flags(identity, &flags)))
        return FALSE;

    if ((flags & KCDB_IDENT_FLAG_RENEWABLE) == 0)
        return FALSE;

    /* TODO: Ideally we wouldn't try to renew an identity that has a
     * parent identity that is itself renewable.  If the parent is
     * renewed, then the child would also be renewed as a side-effect.
     * However, if we prevent such an identity from being renewed, we
     * should assure that:
     *
     * - The parent identity is renewed in time before the child
     *   expires.
     *
     * - If the parent fails to renew for some reason, then the child
     *   should be renewed independently of the parent.
     *
     * Since this logic isn't in place yet, we renew any renewable
     * identity for the moment.
     */

    return TRUE;
}

void khm_cred_renew_identity(khm_handle identity)
{
    khui_new_creds * c;

    if (!should_identity_be_renewed(identity)) {
        wchar_t idname[KCDB_IDENT_MAXCCH_NAME] = L"";
        khm_size cb = sizeof(idname);

        kcdb_identity_get_short_name(identity, FALSE, idname, &cb);
        _reportf(L"Skipping renewal of identity %s", idname);
        return;
    }

    if (khm_cred_begin_new_cred_op()) {
        khui_cw_create_cred_blob(&c);

        c->subtype = KMSG_CRED_RENEW_CREDS;
        c->result = KHUI_NC_RESULT_PROCESS;
        khui_context_create(&c->ctx,
                            KHUI_SCOPE_IDENT,
                            identity,
                            KCDB_CREDTYPE_INVALID,
                            NULL);

        _begin_task(KHERR_CF_TRANSITIVE);
        _report_sr0(KHERR_NONE, IDS_CTX_RENEW_CREDS);
        _describe();

        kmq_post_message(KMSG_CRED, KMSG_CRED_RENEW_CREDS, 0, (void *) c);

        _end_task();
    }
}

void khm_cred_renew_cred(khm_handle cred)
{
    khui_new_creds * c;

    if (khm_cred_begin_new_cred_op()) {
        khui_cw_create_cred_blob(&c);

        c->subtype = KMSG_CRED_RENEW_CREDS;
        c->result = KHUI_NC_RESULT_PROCESS;
        khui_context_create(&c->ctx,
                            KHUI_SCOPE_CRED,
                            NULL,
                            KCDB_CREDTYPE_INVALID,
                            cred);

        _begin_task(KHERR_CF_TRANSITIVE);
        _report_sr0(KHERR_NONE, IDS_CTX_RENEW_CREDS);
        _describe();

        kmq_post_message(KMSG_CRED, KMSG_CRED_RENEW_CREDS, 0, (void *) c);

        _end_task();
    }
}

void khm_cred_renew_creds(void)
{
    khui_new_creds * c;

    if (khm_cred_begin_new_cred_op()) {
        khui_cw_create_cred_blob(&c);
        c->subtype = KMSG_CRED_RENEW_CREDS;
        c->result = KHUI_NC_RESULT_PROCESS;
        khui_context_get(&c->ctx);

        _begin_task(KHERR_CF_TRANSITIVE);
        _report_sr0(KHERR_NONE, IDS_CTX_RENEW_CREDS);
        _describe();

        kmq_post_message(KMSG_CRED, KMSG_CRED_RENEW_CREDS, 0, (void *) c);

        _end_task();
    }
}

void khm_cred_change_password(wchar_t * title)
{
    khui_new_creds * nc;
    LPNETID_DLGINFO pdlginfo;
    khm_size cb;

    if (!khm_cred_begin_dialog())
        return;

    if (!khm_cred_begin_new_cred_op()) {
        khm_cred_end_dialog(NULL);
        return;
    }

    khui_cw_create_cred_blob(&nc);
    nc->subtype = KMSG_CRED_PASSWORD;
    dialog_nc = nc;

    khui_context_get(&nc->ctx);

    if (title) {

        if (SUCCEEDED(StringCbLength(title, KHUI_MAXCB_TITLE, &cb))) {
            cb += sizeof(wchar_t);

            nc->window_title = PMALLOC(cb);
            assert(nc->window_title);
            StringCbCopy(nc->window_title, cb, title);
        }
    } else if (nc->ctx.cb_vparam == sizeof(NETID_DLGINFO) &&
               (pdlginfo = nc->ctx.vparam) &&
               pdlginfo->size == NETID_DLGINFO_V1_SZ &&
               pdlginfo->in.title[0] &&
               SUCCEEDED(StringCchLength(pdlginfo->in.title,
                                         NETID_TITLE_SZ,
                                         &cb))) {
        cb = (cb + 1) * sizeof(wchar_t);
        nc->window_title = PMALLOC(cb);
        assert(nc->window_title);
        StringCbCopy(nc->window_title, cb, pdlginfo->in.title);
    }

    if (nc->ctx.identity != NULL) {
        khui_cw_set_primary_id(nc, nc->ctx.identity);
    }

    khm_create_newcredwnd(khm_hwnd_main, nc);

    if (nc->hwnd != NULL) {
        _begin_task(KHERR_CF_TRANSITIVE);
        _report_sr0(KHERR_NONE, IDS_CTX_PASSWORD);
        _describe();

        kmq_post_message(KMSG_CRED, KMSG_CRED_PASSWORD, 0,
                         (void *) nc);

        _end_task();
    } else {
        khui_cw_destroy_cred_blob(nc);
    }
}

LRESULT
khm_cred_configure_identity(khui_configure_identity_data * pcid)
{
    khm_show_identity_config_pane(pcid->target_identity);
    return 0;
}

void
khm_cred_obtain_new_creds_for_ident(khm_handle ident, wchar_t * title)
{
    khui_action_context ctx;

    if (ident == NULL) {
        khm_cred_obtain_new_creds(title);
        return;
    }

    khui_context_get(&ctx);

    khui_context_set(KHUI_SCOPE_IDENT,
                     ident,
                     KCDB_CREDTYPE_INVALID,
                     NULL,
                     NULL,
                     0,
                     NULL);

    khm_cred_obtain_new_creds(title);

    khui_context_set_indirect(&ctx);

    khui_context_release(&ctx);
}

void khm_cred_obtain_new_creds(wchar_t * title)
{
    khui_new_creds * nc;
    LPNETID_DLGINFO pdlginfo;
    khm_size cb;
    khm_handle def_idpro = NULL;

    if (!khm_cred_begin_dialog())
        return;

    if (!khm_cred_begin_new_cred_op()) {
        khm_cred_end_dialog(NULL);
        return;
    }

    khui_cw_create_cred_blob(&nc);
    nc->subtype = KHUI_NC_SUBTYPE_NEW_CREDS;
    dialog_nc = nc;

    khui_context_get(&nc->ctx);

    if (KHM_FAILED(kcdb_identpro_get_default(&def_idpro))) {
        wchar_t title[256];
        wchar_t msg[512];
        wchar_t suggestion[512];
        khui_alert * a;

        LoadString(khm_hInstance, IDS_ERR_TITLE_NO_IDENTPRO,
                   title, ARRAYLENGTH(title));
        LoadString(khm_hInstance, IDS_ERR_MSG_NO_IDENTPRO,
                   msg, ARRAYLENGTH(msg));
        LoadString(khm_hInstance, IDS_ERR_SUGG_NO_IDENTPRO,
                   suggestion, ARRAYLENGTH(suggestion));

        khui_alert_create_simple(title,
                                 msg,
                                 KHERR_ERROR,
                                 &a);
        khui_alert_set_suggestion(a, suggestion);

        khui_alert_show(a);

        khui_alert_release(a);

        khui_context_release(&nc->ctx);
        nc->result = KHUI_NC_RESULT_CANCEL;
        khm_cred_end_dialog(nc);
        khm_cred_end_new_cred_op();
        khui_cw_destroy_cred_blob(nc);
        return;
    }

    if (title) {
        if (SUCCEEDED(StringCbLength(title, KHUI_MAXCB_TITLE, &cb))) {
            cb += sizeof(wchar_t);

            nc->window_title = PMALLOC(cb);
            assert(nc->window_title);
            StringCbCopy(nc->window_title, cb, title);
        }
    } else if (nc->ctx.cb_vparam == sizeof(NETID_DLGINFO) &&
               (pdlginfo = nc->ctx.vparam) &&
               pdlginfo->size == NETID_DLGINFO_V1_SZ &&
               pdlginfo->in.title[0] &&
               SUCCEEDED(StringCchLength(pdlginfo->in.title,
                                         NETID_TITLE_SZ,
                                         &cb))) {

        cb = (cb + 1) * sizeof(wchar_t);
        nc->window_title = PMALLOC(cb);
        assert(nc->window_title);
        StringCbCopy(nc->window_title, cb, pdlginfo->in.title);
    }

    /* Preselect primary identity */

    {
        khm_handle ident = NULL;
        khm_int32 f = 0;

        if (nc->ctx.identity) {
            ident = nc->ctx.identity;
            kcdb_identity_hold(ident);
        } else {
            /* Use the default identity */
            kcdb_identity_get_default_ex(def_idpro, &ident);
        }

        if (ident && KHM_SUCCEEDED(kcdb_identity_get_flags(ident, &f)) &&
            !(f & KCDB_IDENT_FLAG_CRED_INIT)) {
            kcdb_identity_release(ident);
            ident = NULL;
        }

        khui_cw_set_primary_id(nc, ident);

        if (ident)
            kcdb_identity_release(ident);
    }

    khm_create_newcredwnd(khm_hwnd_main, nc);

    if (nc->hwnd != NULL) {
        _begin_task(KHERR_CF_TRANSITIVE);
        _report_sr0(KHERR_NONE, IDS_CTX_NEW_CREDS);
        _describe();

        kmq_post_message(KMSG_CRED, KMSG_CRED_NEW_CREDS, 0,
                         (void *) nc);
        _end_task();
    } else {
        khui_context_release(&nc->ctx);
        nc->result = KHUI_NC_RESULT_CANCEL;
        khm_cred_end_dialog(nc);
        khm_cred_end_new_cred_op();
        khui_cw_destroy_cred_blob(nc);
    }

    if (def_idpro) {
        kcdb_identpro_release(def_idpro);
        def_idpro = NULL;
    }
}

/*! \brief Describe the result of a new credentials operation

  This is used when concluding error contexts that are being monitored
  remotely.  In this case, we add a final additional event to the
  primary event context that describes the result of the operation.
 */
static void
describe_new_creds_result(khui_new_creds * nc, khm_boolean has_error)
{
    wchar_t wsinsert[512];
    khm_size cbsize;

    kherr_severity severity = (has_error) ? KHERR_ERROR : KHERR_NONE;

    if(nc->subtype == KHUI_NC_SUBTYPE_NEW_CREDS ||
       nc->subtype == KHUI_NC_SUBTYPE_ACQPRIV_ID) {

        cbsize = sizeof(wsinsert);
        kcdb_get_resource(nc->identities[0],
                          KCDB_RES_DISPLAYNAME,
                          0, NULL, NULL, wsinsert, &cbsize);

        _report_sr1(severity,  (has_error ? IDS_CTX_PROC_NEW_CREDS_RF :
                                IDS_CTX_PROC_NEW_CREDS_R),
                    _cstr(wsinsert));

    } else if (nc->subtype == KMSG_CRED_RENEW_CREDS) {
        cbsize = sizeof(wsinsert);

        if (nc->ctx.scope == KHUI_SCOPE_IDENT)
            kcdb_get_resource(nc->ctx.identity, KCDB_RES_DISPLAYNAME, 0, NULL, NULL,
                              wsinsert, &cbsize);
        else if (nc->ctx.scope == KHUI_SCOPE_CREDTYPE) {
            if (nc->ctx.identity != NULL)
                kcdb_get_resource(nc->ctx.identity, KCDB_RES_DISPLAYNAME, 0, NULL, NULL,
                                  wsinsert, &cbsize);
            else
                kcdb_get_resource(KCDB_HANDLE_FROM_CREDTYPE(nc->ctx.cred_type),
                                  KCDB_RES_DISPLAYNAME, 0, NULL, NULL,
                                  wsinsert, &cbsize);
        } else if (nc->ctx.scope == KHUI_SCOPE_CRED) {
            kcdb_get_resource(nc->ctx.cred, KCDB_RES_DISPLAYNAME, 0, NULL, NULL,
                              wsinsert, &cbsize);
        } else {
            StringCbCopy(wsinsert, sizeof(wsinsert), L"(?)");
        }

        if (has_error) {
            _report_sr1(severity, ((nc->original_subtype == KHUI_NC_SUBTYPE_ACQDERIVED) ?
                                   IDS_CTX_PROC_NEW_CREDS_RF :IDS_CTX_PROC_RENEW_CREDS_RF),
                        _cstr(wsinsert));
        } else {
            _report_sr1(severity, ((nc->original_subtype == KHUI_NC_SUBTYPE_ACQDERIVED) ?
                                   IDS_CTX_PROC_NEW_CREDS_R :IDS_CTX_PROC_RENEW_CREDS_R),
                        _cstr(wsinsert));
        }
    } else if (nc->subtype == KMSG_CRED_PASSWORD) {
        cbsize = sizeof(wsinsert);
        kcdb_identity_get_name(nc->identities[0], wsinsert, &cbsize);

        _report_sr1(severity, (has_error ? IDS_CTX_PROC_PASSWORD_RF :
                               IDS_CTX_PROC_PASSWORD_R),
                    _cstr(wsinsert));
    } else {
        assert(FALSE);
    }

    _resolve();
}

/*! \brief Figure out a good description of the credentials operation we are going to perform

 */
static void
describe_new_creds_operation(khui_new_creds * nc)
{
    wchar_t wsinsert[512];
    khm_size cbsize;

    if(nc->subtype == KHUI_NC_SUBTYPE_NEW_CREDS ||
       nc->subtype == KHUI_NC_SUBTYPE_ACQPRIV_ID) {

        cbsize = sizeof(wsinsert);
        kcdb_get_resource(nc->identities[0],
                          KCDB_RES_DISPLAYNAME,
                          0, NULL, NULL, wsinsert, &cbsize);

        _report_sr1(KHERR_NONE,  IDS_CTX_PROC_NEW_CREDS,
                    _cstr(wsinsert));

    } else if (nc->subtype == KMSG_CRED_RENEW_CREDS) {
        cbsize = sizeof(wsinsert);

        if (nc->ctx.scope == KHUI_SCOPE_IDENT)
            kcdb_get_resource(nc->ctx.identity, KCDB_RES_DISPLAYNAME, 0, NULL, NULL,
                              wsinsert, &cbsize);
        else if (nc->ctx.scope == KHUI_SCOPE_CREDTYPE) {
            if (nc->ctx.identity != NULL)
                kcdb_get_resource(nc->ctx.identity, KCDB_RES_DISPLAYNAME, 0, NULL, NULL,
                                  wsinsert, &cbsize);
            else
                kcdb_get_resource(KCDB_HANDLE_FROM_CREDTYPE(nc->ctx.cred_type),
                                  KCDB_RES_DISPLAYNAME, 0, NULL, NULL,
                                  wsinsert, &cbsize);
        } else if (nc->ctx.scope == KHUI_SCOPE_CRED) {
            kcdb_get_resource(nc->ctx.cred, KCDB_RES_DISPLAYNAME, 0, NULL, NULL,
                              wsinsert, &cbsize);
        } else {
            StringCbCopy(wsinsert, sizeof(wsinsert), L"(?)");
        }

        _report_sr1(KHERR_NONE, ((nc->original_subtype == KHUI_NC_SUBTYPE_ACQDERIVED) ?
                                 IDS_CTX_PROC_NEW_CREDS :IDS_CTX_PROC_RENEW_CREDS),
                    _cstr(wsinsert));
    } else if (nc->subtype == KMSG_CRED_PASSWORD) {
        cbsize = sizeof(wsinsert);
        kcdb_identity_get_name(nc->identities[0], wsinsert, &cbsize);

        _report_sr1(KHERR_NONE, IDS_CTX_PROC_PASSWORD,
                    _cstr(wsinsert));
    } else {
        assert(FALSE);
    }

    _describe();

}

/*! \brief Conclude processing of a new credentials operation

  Once the new credentials operation has concluded, this function is
  called to check if there are any errors to be reported to the user.
*/
khm_boolean
khm_cred_conclude_processing(khui_new_creds * nc)
{
    khm_boolean has_error = kherr_is_error();

    khm_new_cred_progress_broadcast(KHERR_CTX_END, NULL, nc);

    if (nc->ignore_errors) {
        describe_new_creds_result(nc, has_error);
        return has_error;
    }

    if(has_error) {
        khui_alert * alert;
        kherr_event * evt;
        kherr_context * ctx;
        wchar_t ws_tfmt[512];
        wchar_t w_idname[KCDB_IDENT_MAXCCH_NAME];
        wchar_t ws_title[ARRAYLENGTH(ws_tfmt) + KCDB_IDENT_MAXCCH_NAME];
        khm_size cb;

        /* For renewals, we suppress the error message for the
           following case:

           - The renewal was for an identity

           - There are no identity credentials for the identity (no
             credentials that have the same type as the identity
             provider). */

        if (nc->subtype == KMSG_CRED_RENEW_CREDS &&
            nc->ctx.scope == KHUI_SCOPE_IDENT &&
            nc->ctx.identity != NULL) {
            khm_int32 count = 0;

            cb = sizeof(count);
            kcdb_identity_get_attr(nc->ctx.identity, KCDB_ATTR_N_IDCREDS, NULL,
                                   &count, &cb);

            if (count == 0) {
                return has_error;
            }
        }

        ctx = kherr_peek_context();
        evt = kherr_get_err_event(ctx);
        kherr_evaluate_event(evt);

        khui_alert_create_empty(&alert);

        if (nc->subtype == KHUI_NC_SUBTYPE_NEW_CREDS ||
            nc->subtype == KMSG_CRED_ACQPRIV_ID) {

            khui_alert_set_type(alert, KHUI_ALERTTYPE_ACQUIREFAIL);

            cb = sizeof(w_idname);
            if (nc->n_identities == 0 ||
                KHM_FAILED(kcdb_get_resource(nc->identities[0],
                                             KCDB_RES_DISPLAYNAME,
                                             0, NULL, NULL,
                                             w_idname, &cb))) {
                /* an identity could not be determined */
                LoadString(khm_hInstance, IDS_NC_FAILED_TITLE,
                           ws_title, ARRAYLENGTH(ws_title));
            } else {
                LoadString(khm_hInstance, IDS_NC_FAILED_TITLE_I,
                           ws_tfmt, ARRAYLENGTH(ws_tfmt));
                StringCbPrintf(ws_title, sizeof(ws_title),
                               ws_tfmt, w_idname);
                khui_alert_set_ctx(alert,
                                   KHUI_SCOPE_IDENT,
                                   nc->identities[0],
                                   KCDB_CREDTYPE_INVALID,
                                   NULL);
            }

        } else if (nc->subtype == KMSG_CRED_PASSWORD) {

            khui_alert_set_type(alert, KHUI_ALERTTYPE_CHPW);

            cb = sizeof(w_idname);
            if (nc->n_identities == 0 ||
                KHM_FAILED(kcdb_get_resource(nc->identities[0],
                                             KCDB_RES_DISPLAYNAME,
                                             0, NULL, NULL,
                                             w_idname, &cb))) {
                LoadString(khm_hInstance, IDS_NC_PWD_FAILED_TITLE,
                           ws_title, ARRAYLENGTH(ws_title));
            } else {
                LoadString(khm_hInstance, IDS_NC_PWD_FAILED_TITLE_I,
                           ws_tfmt, ARRAYLENGTH(ws_tfmt));
                StringCbPrintf(ws_title, sizeof(ws_title),
                               ws_tfmt, w_idname);
                khui_alert_set_ctx(alert,
                                   KHUI_SCOPE_IDENT,
                                   nc->identities[0],
                                   KCDB_CREDTYPE_INVALID,
                                   NULL);
            }

        } else if (nc->subtype == KMSG_CRED_RENEW_CREDS) {

            khui_alert_set_type(alert, KHUI_ALERTTYPE_RENEWFAIL);

            cb = sizeof(w_idname);
            if (nc->ctx.identity == NULL ||
                KHM_FAILED(kcdb_get_resource(nc->ctx.identity,
                                             KCDB_RES_DISPLAYNAME,
                                             0, NULL, NULL,
                                             w_idname, &cb))) {
                LoadString(khm_hInstance, IDS_NC_REN_FAILED_TITLE,
                           ws_title, ARRAYLENGTH(ws_title));
            } else {
                LoadString(khm_hInstance, IDS_NC_REN_FAILED_TITLE_I,
                           ws_tfmt, ARRAYLENGTH(ws_tfmt));
                StringCbPrintf(ws_title, sizeof(ws_title),
                               ws_tfmt, w_idname);
                khui_alert_set_ctx(alert,
                                   KHUI_SCOPE_IDENT,
                                   nc->ctx.identity,
                                   KCDB_CREDTYPE_INVALID,
                                   NULL);
            }

        } else {
            assert(FALSE);
        }

        khui_alert_set_title(alert, ws_title);
        khui_alert_set_severity(alert, evt->severity);

        if(!evt->long_desc)
            khui_alert_set_message(alert, evt->short_desc);
        else
            khui_alert_set_message(alert, evt->long_desc);

        if(evt->suggestion)
            khui_alert_set_suggestion(alert, evt->suggestion);

        if (nc->subtype == KMSG_CRED_RENEW_CREDS &&
            nc->ctx.identity != NULL) {

            khm_int32 n_cmd;

            n_cmd = khm_get_identity_new_creds_action(nc->ctx.identity);

            if (n_cmd != 0) {
                khui_alert_add_command(alert, n_cmd);
                khui_alert_add_command(alert, KHUI_PACTION_CLOSE);

                khui_alert_set_flags(alert, KHUI_ALERT_FLAG_DISPATCH_CMD,
                                     KHUI_ALERT_FLAG_DISPATCH_CMD);
            }
        }

        khui_alert_show(alert);
        khui_alert_release(alert);

        kherr_release_context(ctx);

        kherr_clear_error();
    }

    return has_error;
}

/**
 * Check if there are any credential types that have pending operations
 *
 * Returns true if any work will be done if a
 * khm_cred_dispatch_process_message() will be called.  Naturally, the
 * logic here has to reflect the logic in
 * khm_cred_dispatch_process_message().
 */
BOOL khm_cred_is_new_creds_pending(khui_new_creds * nc)
{
    BOOL pending = FALSE;
    khm_size i;

    EnterCriticalSection(&nc->cs);

    for (i=0; i < nc->n_types && !pending; i++) {
        khm_size j;
        khui_new_creds_by_type *t;
        khui_new_creds_by_type *dep;

        t = nc->types[i].nct;

        if (t->flags & (KHUI_NCT_FLAG_PROCESSED |
                        KHUI_NC_RESPONSE_COMPLETED |
                        KHUI_NCT_FLAG_DISABLED))
            continue;

        pending = TRUE;

        for(j=0; j < t->n_type_deps && pending; j++) {

            if(KHM_FAILED(khui_cw_find_type(nc, t->type_deps[j], &dep))) {
                /* Type has a dependency on a non-existent or
                 * non-participating type. */
                pending = FALSE;
            }
        }
    }

    LeaveCriticalSection(&nc->cs);

    return pending;
}

/* this is called by khm_cred_dispatch_process_message and the
   kmsg_cred_completion to initiate and continue checked broadcasts of
   KMSG_CRED_DIALOG_PROCESS messages.

   Returns TRUE if more KMSG_CRED_DIALOG_PROCESS messages were
   posted. */
BOOL khm_cred_dispatch_process_level(khui_new_creds *nc)
{
    khm_size i,j;
    khm_handle subs[KHUI_MAX_NCTYPES];
    int n_subs = 0;
    BOOL cont = FALSE;
    khui_new_creds_by_type *t, *d;

    _reportf(L"Preparing to dispatch batch of KMSG_CRED_PROCESS messages");

    /* at each level, we dispatch a wave of notifications to plug-ins
       who's dependencies are all satisfied */
    EnterCriticalSection(&nc->cs);

    if (nc->last_dispatch != NULL)
        nc->last_dispatch = NULL;

    for(i=0; i<nc->n_types && nc->dispatch_state != KHUI_NC_DISPATCH_STATE_ABORTED; i++) {
        t = nc->types[i].nct;

        if (t->flags & (KHUI_NCT_FLAG_PROCESSED |
                        KHUI_NC_RESPONSE_COMPLETED |
                        KHUI_NCT_FLAG_DISABLED)) {
            _reportf(L"Skipping credtype %s(%d).  Marked as %s",
                     nc->types[i].display_name, t->type,
                     ((t->flags & KHUI_NCT_FLAG_DISABLED)? L"disabled" :
                      (t->flags & KHUI_NCT_FLAG_PROCESSED)? L"processed" :
                      (t->flags & KHUI_NC_RESPONSE_COMPLETED)? L"completed": L"(unknown)"));
            continue;
        }

        for(j=0; j<t->n_type_deps; j++) {
            if(KHM_FAILED(khui_cw_find_type(nc, t->type_deps[j], &d))) {
                _reportf(L"Credtype %s(%d) has invalid dependency."
                         L"  Depends on type %d, that is not participating"
                         L" in the current operation.",
                         nc->types[i].display_name, t->type, t->type_deps[j]);
                break;
            }

            if (d->flags & KHUI_NCT_FLAG_DISABLED) {
                _reportf(L"Credtype %s(%d) waiting for disabled credtype %s(%d)."
                         L" Ignoring dependency.",
                         nc->types[i].display_name, t->type, d->name, d->type);
                continue;
            }

            if (!(d->flags & KHUI_NC_RESPONSE_COMPLETED)) {
                _reportf(L"Credtype %s(%d) waiting for completion of %s(%d)",
                         nc->types[i].display_name, t->type, d->name, d->type);
                break;
            }

            if (d->flags & KHUI_NC_RESPONSE_FAILED) {
                _reportf(L"Credtype %s(%d) waiting on failed type %s(%d)",
                         nc->types[i].display_name, t->type, d->name, d->type);
                break;
            }
        }

        if (j < t->n_type_deps) /* there are unmet dependencies */
            continue;

        /* all dependencies for this type have been met. */
        _reportf(L"Queuing credtype %s(%d) for processing",
                 nc->types[i].display_name, t->type);
        subs[n_subs++] = kcdb_credtype_get_sub(t->type);
        t->flags |= KHUI_NCT_FLAG_PROCESSED;
        cont = TRUE;

        if (n_subs == KHUI_MAX_NCTYPES)
            break;
    }

    LeaveCriticalSection(&nc->cs);

    /* the reason why we are posting messages in batches is because
       when the message has completed we know that all the types that
       have the KHUI_NCT_FLAG_PROCESSED set have completed processing.
       Otherwise we have to individually track each message and update
       the type */
    if(n_subs > 0) {
        kmq_call call = NULL;

        kmq_post_subs_msg_ex(subs, n_subs, KMSG_CRED, KMSG_CRED_PROCESS, 0,
                             (void *) nc, &call);

        EnterCriticalSection(&nc->cs);
        if (nc->dispatch_state != KHUI_NC_DISPATCH_STATE_ABORTED)
            nc->dispatch_state = KHUI_NC_DISPATCH_STATE_INCALL;
        else
            kmq_abort_call(call);
        nc->last_dispatch = call;
        kmq_free_call(call);

        /* We don't keep a reference to 'call'.  An implicit reference
           exists for the call until after the call completion handler
           runs and we will relinquish our reference before then. */

        LeaveCriticalSection(&nc->cs);
    } else {
        _reportf(L"Done with processing");
        EnterCriticalSection(&nc->cs);
        assert(nc->last_dispatch == NULL);
        nc->dispatch_state = KHUI_NC_DISPATCH_STATE_NONE;
        LeaveCriticalSection(&nc->cs);
    }

    return cont;
}

/*! \page idnotif Identity Event Notification

 * When a lengthy credentials operation begins for an identity, the
   following happens:

   - khm_cred_dispatch_process_message() calls
     khm_nc_track_progress_of_this_task() which begins monitoring the
     new credentials operation.

   - khm_new_cred_progress_broadcast() is added to the context istener
     list of the current error context.

   - khm_new_cred_progress_broadcast() is called with ::KHERR_CTX_BEGIN.

   - If the operation is ending, khm_new_cred_progress_broadcast() is
     called with ::KHERR_CTX_END.

   - When progress is made, khm_new_cred_progress_broadcast() will be
     called with ::KHERR_CTX_PROGRESS.

 * When khm_new_cred_progress_broadcast() is called with
   ::KHERR_CTX_BEGIN, ::KHERR_CTX_PROGRESS or ::KHERR_CTX_END, it posts a
   ::KMSG_CREDP message:

   - ::KMSG_CREDP_BEGIN_NEWCRED

   - ::KMSG_CREDP_PROG_NEWCRED

   - ::KMSG_CREDP_END_NEWCRED
*/

/**
 * Reset the PROCESSED and COMPLETED flags for credential types as appropriate
 *
 * All credential types will have PROCESSSED bit reset.  If any
 * credential type doesn't have DISABLED and COMPLETED set, then any
 * type that depends on it with COMPLETED set will have COMPLETED
 * reset.
 *
 * \note: Must be called with nc->cs held.
 */
static void
reset_completion_state_for_types(khui_new_creds * nc)
{
    khm_size i;
    khm_boolean retry;

    do {
        retry = FALSE;

        for (i=0; i < nc->n_types; i++) {
            khui_new_creds_by_type * t = nc->types[i].nct;
            khui_new_creds_by_type * dep;

            t->flags &= ~KHUI_NCT_FLAG_PROCESSED;

            if (t->flags & KHUI_NC_RESPONSE_COMPLETED) {
                khm_size j;

                for(j=0; j<t->n_type_deps; j++) {
                    if(KHM_FAILED(khui_cw_find_type(nc, t->type_deps[j], &dep))) {
                        break;
                    }

                    if (dep->flags & KHUI_NCT_FLAG_DISABLED) {
                        continue;
                    }

                    if (!(dep->flags & KHUI_NC_RESPONSE_COMPLETED)) {
                        t->flags &= ~KHUI_NC_RESPONSE_COMPLETED;
                        retry = TRUE;
                        break;
                    }
                }
            }
        }
    } while (retry);
}

void
khm_cred_dispatch_process_message(khui_new_creds *nc)
{
    BOOL pending;

    /* see if there's anything to do.  We can check this without
       obtaining a lock */
    if(nc->n_types == 0 ||

       ((nc->subtype == KHUI_NC_SUBTYPE_NEW_CREDS ||
         nc->subtype == KHUI_NC_SUBTYPE_ACQPRIV_ID) &&
        nc->n_identities == 0) ||

       (nc->subtype == KMSG_CRED_PASSWORD &&
        nc->n_identities == 0))

        goto _terminate_job;

    /* check dependencies and stuff first */
    EnterCriticalSection(&nc->cs);

    /* We shouldn't start a dispatch if one is already in progress. */
    if (nc->dispatch_state != KHUI_NC_DISPATCH_STATE_NONE) {
        assert(FALSE);
        LeaveCriticalSection(&nc->cs);
        goto _terminate_job;
    }

    reset_completion_state_for_types(nc);
    nc->response = 0;

    if (nc->persist_privcred) {
        if (nc->cs_privcred)
            kcdb_credset_flush(nc->cs_privcred);
        else
            kcdb_credset_create(&nc->cs_privcred);
    }
    LeaveCriticalSection(&nc->cs);

    /* Consindering all that can go wrong here and the desire to
       handle errors here separately from others, we create a new task
       for the purpose of tracking the credentials acquisition
       process. */
    _begin_task(KHERR_CF_TRANSITIVE);

    khm_nc_track_progress_of_this_task(nc);
    khm_new_cred_progress_broadcast(KHERR_CTX_BEGIN, NULL, nc);
    kherr_add_ctx_handler_param(khm_new_cred_progress_broadcast,
                                KHERR_CTX_PROGRESS,
                                KHERR_SERIAL_CURRENT,
                                nc);

    describe_new_creds_operation(nc);

    pending = khm_cred_dispatch_process_level(nc);

    _end_task();

    if(!pending)
        goto _terminate_job;

    return;

 _terminate_job:
    khm_new_cred_progress_broadcast(KHERR_CTX_END, NULL, nc);

    if (nc->subtype == KMSG_CRED_RENEW_CREDS)
        kmq_post_message(KMSG_CRED, KMSG_CRED_END, 0, (void *) nc);
    else
        PostMessage(nc->hwnd, KHUI_WM_NC_NOTIFY,
                    MAKEWPARAM(0, WMNC_DIALOG_PROCESS_COMPLETE), 0);
}

khm_int32
khm_cred_abort_process_message(khui_new_creds * nc)
{
    khm_int32 rv;

    EnterCriticalSection(&nc->cs);
    switch (nc->dispatch_state) {
    case KHUI_NC_DISPATCH_STATE_NONE:
        rv = KHM_ERROR_NOT_READY;
        break;

    case KHUI_NC_DISPATCH_STATE_INCALL:
        nc->dispatch_state = KHUI_NC_DISPATCH_STATE_ABORTED;
        if (nc->last_dispatch)
            kmq_abort_call(nc->last_dispatch);
        rv = KHM_ERROR_SUCCESS;
        break;

    case KHUI_NC_DISPATCH_STATE_ABORTED:
        rv = KHM_ERROR_DUPLICATE;
        break;

    default:
        rv = KHM_ERROR_INVALID_PARAM;
        assert(FALSE);
    }
    LeaveCriticalSection(&nc->cs);
    return rv;
}

void
khm_cred_refresh(void) {
    kmq_post_message(KMSG_CRED, KMSG_CRED_REFRESH, 0, NULL);
}

void
khm_cred_addr_change(void) {
    khm_handle csp_cw = NULL;
    khm_int32 check_net = 0;
    kcdb_enumeration e = NULL;
    khm_handle ident = NULL;
    khm_size cb;
    khm_size n_idents;
    FILETIME ft_now;
    FILETIME ft_exp;
    FILETIME ft_issue;

    if (KHM_SUCCEEDED(khc_open_space(NULL, L"CredWindow",
                                     0, &csp_cw))) {
        khc_read_int32(csp_cw, L"AutoDetectNet", &check_net);

        khc_close_space(csp_cw);
    }

    if (!check_net)
        return;

    if (KHM_FAILED(kcdb_identity_begin_enum(KCDB_IDENT_FLAG_VALID |
                                            KCDB_IDENT_FLAG_RENEWABLE,
                                            KCDB_IDENT_FLAG_VALID |
                                            KCDB_IDENT_FLAG_RENEWABLE,
                                            &e, &n_idents)))
        return;

    GetSystemTimeAsFileTime(&ft_now);

    while (KHM_SUCCEEDED(kcdb_enum_next(e, &ident))) {

        cb = sizeof(ft_issue);

        if (KHM_SUCCEEDED
            (kcdb_identity_get_attr(ident, KCDB_ATTR_ISSUE, NULL,
                                    &ft_issue, &cb)) &&

            (cb = sizeof(ft_exp)) &&
            KHM_SUCCEEDED
            (kcdb_identity_get_attr(ident, KCDB_ATTR_EXPIRE, NULL,
                                    &ft_exp, &cb)) &&

            CompareFileTime(&ft_now, &ft_exp) < 0) {

            khm_int64 i_issue;
            khm_int64 i_exp;
            khm_int64 i_now;

            i_issue = FtToInt(&ft_issue);
            i_exp = FtToInt(&ft_exp);
            i_now = FtToInt(&ft_now);

            if (i_now > (i_issue + i_exp) / 2) {

                khm_cred_renew_identity(ident);

            }
        }
    }

    kcdb_enum_end(e);
}

LRESULT
khm_cred_collect_privileged_creds(khui_collect_privileged_creds_data * pcpcd)
{
    khui_new_creds * nc_child;

    khui_cw_create_cred_blob(&nc_child);
    nc_child->subtype = KHUI_NC_SUBTYPE_ACQPRIV_ID;
    khui_context_create(&nc_child->ctx,
                        KHUI_SCOPE_IDENT,
                        pcpcd->target_identity,
                        -1, NULL);
    nc_child->persist_privcred = TRUE;
    khui_cw_set_privileged_credential_collector(nc_child, pcpcd->dest_credset);
    return khm_do_modal_newcredwnd(pcpcd->hwnd_parent, nc_child);
}

khm_int32
khm_cred_derive_identity_from_privileged_creds(khui_collect_privileged_creds_data * pcd)
{
    khui_new_creds * nc_child = NULL;
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
    nc_child->parent = pcd->nc;
    khui_cw_lock_nc(pcd->nc);
    pcd->nc->n_children++;
    khui_cw_unlock_nc(pcd->nc);
    if (khm_cred_begin_new_cred_op()) {
        kmq_post_message(KMSG_CRED, KMSG_CRED_RENEW_CREDS, 0, (void *) nc_child);
        return KHM_ERROR_SUCCESS;
    } else {
        khui_cw_destroy_cred_blob(nc_child);
        return KHM_ERROR_NOT_READY;
    }
}

void
khm_cred_process_startup_actions(void) {
    khm_handle defident = NULL;

    if (!khm_startup.processing)
        return;

    _begin_task(0);
    _reportf(L"Processing commandline");
    _describe();

    if (khm_startup.init ||
        khm_startup.renew ||
        khm_startup.destroy ||
        khm_startup.autoinit) {
        khm_handle idpro;

        if (KHM_SUCCEEDED(kcdb_identpro_get_default(&idpro))) {
            wchar_t name[KCDB_MAXCCH_NAME];
            khm_size cb;
            cb = sizeof(name);

            kcdb_get_resource(idpro, KCDB_RES_DISPLAYNAME, 0, NULL, NULL,
                              name, &cb);
            _reportf(L"Selected identity provider [%s]", name);

            kcdb_identpro_get_default_identity(idpro, &defident);
            kcdb_identpro_release(idpro);

            if (defident) {
                cb = sizeof(name);
                kcdb_get_resource(defident, KCDB_RES_DISPLAYNAME, 0, NULL, NULL,
                                  name, &cb);
                _reportf(L"Default identity [%s]", name);
            } else {
                _reportf(L"No default identity");
            }
        }
    }

    /* For asynchronous actions, we trigger the action and then exit
       the loop.  Once the action completes, the completion handler
       will trigger a continuation message which will result in this
       function getting called again.  Then we can proceed with the
       rest of the startup actions. */
    do {
        if (khm_startup.init) {

            khm_cred_obtain_new_creds_for_ident(defident, NULL);
            khm_startup.init = FALSE;
            break;
        }

        if (khm_startup.import) {
            _reportf(L"Importing...");

            khm_cred_import();
            khm_startup.import = FALSE;

            /* we also set the renew command to false here because we
               trigger a renewal for all the identities at the end of
               the import operation anyway. */
            khm_startup.renew = FALSE;
            break;
        }

        if (khm_startup.renew) {

            _reportf(L"Renewing...");

            /* if there are no credentials, we just skip over the
               renew action. */

            khm_startup.renew = FALSE;

            if (khm_cred_begin_new_cred_op()) {

                khm_cred_renew_all_identities();

                khm_cred_end_new_cred_op();
            }

            break;
        }

        if (khm_startup.destroy) {

            _reportf(L"Destroying credentials ...");

            khm_startup.destroy = FALSE;

            if (defident) {
                khm_cred_destroy_identity(defident);
                break;
            }
        }

        if (khm_startup.autoinit) {
            khm_int32 count = 0;
            khm_size cb;

            _reportf(L"Autoinit ...");

            khm_startup.autoinit = FALSE;

            if (defident) {
                cb = sizeof(count);
                kcdb_identity_get_attr(defident, KCDB_ATTR_N_IDCREDS, NULL, &count, &cb);
            }

            if (count == 0) {

                _reportf(L"No credentials for default identity.  Invoking new credentials wizard");

                if (defident)
                    khui_context_set(KHUI_SCOPE_IDENT,
                                     defident,
                                     KCDB_CREDTYPE_INVALID,
                                     NULL, NULL, 0,
                                     NULL);
                else
                    khui_context_reset();

                khm_cred_obtain_new_creds(NULL);
                break;
            }
        }

        if (khm_startup.exit) {
            _reportf(L"Exiting...");

            PostMessage(khm_hwnd_main,
                        WM_COMMAND,
                        MAKEWPARAM(KHUI_ACTION_EXIT, 0), 0);
            khm_startup.exit = FALSE;
            break;
        }

        if (khm_startup.display & SOPTS_DISPLAY_HIDE) {
            khm_hide_main_window();
        } else if (khm_startup.display & SOPTS_DISPLAY_SHOW) {
            khm_show_main_window();
        }
        khm_startup.display = 0;

        /* when we get here, then we are all done with the command
           line stuff */
        khm_startup.processing = FALSE;
        khm_startup.remote = FALSE;

        kmq_post_message(KMSG_ACT, KMSG_ACT_END_CMDLINE, 0, 0);
    } while(FALSE);

    if (defident)
        kcdb_identity_release(defident);

    _end_task();
}

void
khm_cred_begin_startup_actions(void) {
    khm_handle csp_cw;

    if (khm_startup.seen)
        return;

    if (!khm_startup.remote &&
        KHM_SUCCEEDED(khc_open_space(NULL, L"CredWindow", 0, &csp_cw))) {

        khm_int32 t = 0;

        khc_read_int32(csp_cw, L"Autoinit", &t);
        if (t)
            khm_startup.autoinit = TRUE;

        t = 0;
        khc_read_int32(csp_cw, L"AutoImport", &t);
        if (t)
            khm_startup.import = TRUE;

        khc_close_space(csp_cw);

    }

    /* if this is a remote request, and no specific options were
       specified other than --renew, then we perform the default
       action, as if the user clicked on the tray icon. */
    if (khm_startup.remote &&
        !khm_startup.exit &&
        !khm_startup.destroy &&
        !khm_startup.autoinit &&
        !khm_startup.init &&
        !khm_startup.remote_exit &&
        !khm_startup.import &&
        !khm_startup.display) {

        khm_int32 def_action = khm_get_default_notifier_action();

        if (def_action > 0) {
            khui_action_trigger(def_action, NULL);
        }
    }

    khm_startup.seen = TRUE;
    khm_startup.processing = TRUE;

    khm_cred_process_startup_actions();
}

