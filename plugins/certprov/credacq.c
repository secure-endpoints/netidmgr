/*
 * Copyright (c) 2006-2009 Secure Endpoints Inc.
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

#include "module.h"
#include<assert.h>

/* This file provides handlers for the credentials acquisition
   messages including handling the user interface for the new
   credentials dialogs. */

/*********************************************************************

These are stubs for the Window message for the dialog panel.  This
dialog panel is the one that is added to the new credentials window
for obtaining new credentials.

Note that all the UI callbacks run under the UI thread.

 *********************************************************************/

/* This structure will hold all the state information we will need to
   access from the new credentials panel for our credentials type. */
struct nc_dialog_data {
    khui_new_creds_by_type nct;

    /* TODO: add any other state information here */
};

/* Note: This callback runs under the UI thread */
INT_PTR
handle_wm_initdialog(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    khui_new_creds * nc = NULL;
    struct nc_dialog_data * d = NULL;

    nc = (khui_new_creds *) lParam;
    khui_cw_find_type(nc, credtype_id, (khui_new_creds_by_type **) &d);

    assert(d);

#pragma warning(push)
#pragma warning(disable: 4244)
    SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM) d);
#pragma warning(pop)

    /* TODO: Perform any additional initialization here */

    return FALSE;
}

/* Note: This callback runs under the UI thread */
INT_PTR
handle_khui_wm_nc_notify(HWND hwnd, WPARAM wParam, LPARAM lParam) {

    struct nc_dialog_data * d;

    /* Refer to the khui_wm_nc_notifications enumeration in the
       NetIDMgr SDK for the full list of notification messages that
       can be sent. */

    d = (struct nc_dialog_data *) GetWindowLongPtr(hwnd, DWLP_USER);

    if (!d)
        return TRUE;

    /* this should be set by now */
    assert(d->nct.nc);

    switch (HIWORD(wParam)) {

    case WMNC_IDENTITY_CHANGE:
        /* Sent when the primary identity associated with the new
           credentials operation has changed. */

        /* TODO: Handle this message */

        assert(h_idprov != NULL);

        {
            khm_handle ident = NULL;

            khui_cw_get_primary_id(d->nct.nc, &ident);

            if (ident &&
                kcdb_identity_by_provider(ident, IDPROV_NAMEW)) {
                khui_cw_enable_type(d->nct.nc, credtype_id, TRUE);

                khui_cw_notify_identity_state(d->nct.nc, NULL, L"",
                                              KHUI_CWNIS_VALIDATED |
                                              KHUI_CWNIS_READY, 0);
            } else {
                khui_cw_enable_type(d->nct.nc, credtype_id, FALSE);
            }

            if (ident)
                kcdb_identity_release(ident);
        }

        break;

    case WMNC_DIALOG_PREPROCESS:
        /* Sent before KMSG_CRED_PROCESS messages are dispatched. */

        /* TODO: Handle this message */
        break;
    }

    return TRUE;
}

/* Note: This callback runs under the UI thread */
INT_PTR
handle_wm_command(HWND hwnd, WPARAM wParam, LPARAM lParam) {

    struct nc_dialog_data * d;

    d = (struct nc_dialog_data *) GetWindowLongPtr(hwnd, DWLP_USER);
    if (d == NULL)
        return FALSE;

    /* TODO: handle WM_COMMAND */

    return FALSE;
}

/* Note: This callback runs under the UI thread */
INT_PTR
handle_wm_destroy(HWND hwnd, WPARAM wParam, LPARAM lParam) {

    SetWindowLongPtr(hwnd, DWLP_USER, 0);

    /* TODO: Perform any additional uninitialization */

    return FALSE;
}

/* Dialog procedure for the new credentials panel for our credentials
   type.  We just dispatch messages here to other functions here.

   Note that this procedure runs under the UI thread.
 */
INT_PTR CALLBACK
nc_dlg_proc(HWND hwnd,
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam) {

    switch (uMsg) {
    case WM_INITDIALOG:
        return handle_wm_initdialog(hwnd, wParam, lParam);

    case WM_COMMAND:
        return handle_wm_command(hwnd, wParam, lParam);

    case KHUI_WM_NC_NOTIFY:
        return handle_khui_wm_nc_notify(hwnd, wParam, lParam);

    case WM_DESTROY:
        return handle_wm_destroy(hwnd, wParam, lParam);

        /* TODO: add code for handling other windows messages here. */
    }

    return FALSE;
}

/*******************************************************************

The following section contains function stubs for each of the
credentials messages that a credentials provider is likely to want to
handle.  It doesn't include a few messages, but they should be easy to
add.  Please see the documentation for each of the KMSG_CRED_*
messages for documentation on how to handle each of the messages.

********************************************************************/


/* Handler for KMSG_CRED_NEW_CREDS */
khm_int32
handle_kmsg_cred_new_creds(khui_new_creds * nc) {

    wchar_t wshortdesc[KHUI_MAXCCH_SHORT_DESC];
    size_t cb = 0;
    struct nc_dialog_data * d;

    /* This is a minimal handler that just adds a dialog pane to the
       new credentials window to handle new credentials acquisition
       for this credentials type. */

    /* TODO: add additional initialization etc. as needed */

    d = malloc(sizeof(*d));
    ZeroMemory(d, sizeof(*d));

    d->nct.type = credtype_id;
    d->nct.ordinal = -1;

    LoadString(hResModule, IDS_CT_SHORT_DESC,
               wshortdesc, ARRAYLENGTH(wshortdesc));
    StringCbLength(wshortdesc, sizeof(wshortdesc), &cb);
#ifdef DEBUG
    assert(cb > 0);
#endif
    cb += sizeof(wchar_t);

    d->nct.name = malloc(cb);
    StringCbCopy(d->nct.name, cb, wshortdesc);

    d->nct.h_module = hResModule;
    d->nct.dlg_proc = nc_dlg_proc;
    d->nct.dlg_template = MAKEINTRESOURCE(IDD_NEW_CREDS);

    khui_cw_add_type(nc, &d->nct);

    khui_cw_add_selector(nc, idsel_factory, NULL);

    return KHM_ERROR_SUCCESS;
}

/* Handler for KMSG_CRED_RENEW_CREDS */
khm_int32
handle_kmsg_cred_renew_creds(khui_new_creds * nc) {

    struct nc_dialog_data * d;

    /* This is a minimal handler that just adds this credential type
       to the list of credential types that are participating in this
       renewal operation. */

    /* TODO: add additional initialization etc. as needed */

    d = malloc(sizeof(*d));
    ZeroMemory(d, sizeof(*d));

    d->nct.type = credtype_id;

    khui_cw_add_type(nc, &d->nct);

    return KHM_ERROR_SUCCESS;
}

/* Handler for KMSG_CRED_DIALOG_PRESTART */
khm_int32
handle_kmsg_cred_dialog_prestart(khui_new_creds * nc) {
    /* TODO: Handle this message */

    /* The message is sent after the dialog has been created.  The
       window handle for the created dialog can be accessed through
       the hwnd_panel member of the khui_new_creds_by_type structure
       that was added for this credentials type. */
    return KHM_ERROR_SUCCESS;
}

/* Handler for KMSG_CRED_DIALOG_NEW_IDENTITY */
/* Not a message sent out by NetIDMgr.  See documentation of
   KMSG_CRED_DIALOG_NEW_IDENTITY  */
khm_int32
handle_kmsg_cred_dialog_new_identity(khm_ui_4 uparam,
                                     void *   vparam) {

    /* TODO: Handle this message */
    return KHM_ERROR_SUCCESS;
}

/* Handler for KMSG_CRED_DIALOG_NEW_OPTIONS */
/* Not a message sent out by NetIDMgr.  See documentation of
   KMSG_CRED_DIALOG_NEW_OPTIONS */
khm_int32
handle_kmsg_cred_dialog_new_options(khm_ui_4 uparam,
                                    void *   vparam) {
    /* TODO: Handle this message */
    return KHM_ERROR_SUCCESS;
}

/* Handler for KMSG_CRED_PROCESS */
khm_int32
handle_kmsg_cred_process(khui_new_creds * nc) {
    /* TODO: Handle this message */

    /* This is where the credentials acquisition should be performed
       as determined by the UI.  Note that this message is sent even
       when the user clicks 'cancel'.  The value of nc->result should
       be checked before performing any credentials acquisition.  If
       the value is KHUI_NC_RESULT_CANCEL, then no credentials should
       be acquired.  Otherwise, the value would be
       KHUI_NC_RESULT_PROCESS. */

    return KHM_ERROR_SUCCESS;
}

/* Handler for KMSG_CRED_END */
khm_int32
handle_kmsg_cred_end(khui_new_creds * nc) {

    struct nc_dialog_data * d;

    /* TODO: Perform any additional uninitialization as needed. */

    khui_cw_find_type(nc, credtype_id, (khui_new_creds_by_type **) &d);

    if (d) {

        khui_cw_del_type(nc, credtype_id);

        if (d->nct.name)
            free(d->nct.name);

        free(d);
    }

    return KHM_ERROR_SUCCESS;
}

/* Handler for KMSG_CRED_IMPORT */
khm_int32
handle_kmsg_cred_import(void) {

    /* TODO: Handle this message */

    return KHM_ERROR_SUCCESS;
}


/******************************************************
 Dispatch each message to individual handlers above.
 */
khm_int32 KHMAPI
handle_cred_acq_msg(khm_int32 msg_type,
                    khm_int32 msg_subtype,
                    khm_ui_4  uparam,
                    void *    vparam) {

    khm_int32 rv = KHM_ERROR_SUCCESS;

    switch(msg_subtype) {
    case KMSG_CRED_NEW_CREDS:
        return handle_kmsg_cred_new_creds((khui_new_creds *) vparam);

    case KMSG_CRED_RENEW_CREDS:
        return handle_kmsg_cred_renew_creds((khui_new_creds *) vparam);

    case KMSG_CRED_DIALOG_PRESTART:
        return handle_kmsg_cred_dialog_prestart((khui_new_creds *) vparam);

    case KMSG_CRED_PROCESS:
        return handle_kmsg_cred_process((khui_new_creds *) vparam);

    case KMSG_CRED_DIALOG_NEW_IDENTITY:
        return handle_kmsg_cred_dialog_new_identity(uparam, vparam);

    case KMSG_CRED_DIALOG_NEW_OPTIONS:
        return handle_kmsg_cred_dialog_new_options(uparam, vparam);

    case KMSG_CRED_END:
        return handle_kmsg_cred_end((khui_new_creds *) vparam);

    case KMSG_CRED_IMPORT:
        return handle_kmsg_cred_import();
    }

    return rv;
}
