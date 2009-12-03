/*
 * Copyright (c) 2007-2009 Secure Endpoints Inc.
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

/* we support up to 32 simutaneous dialogs.  In reality, more than two
   is pretty unlikely.  Property sheets are special and are handled
   separately. */
#define MAX_UI_DIALOGS 128

typedef struct khui_dialog {
    HWND hwnd;

    LDCL(struct khui_dialog);
} khui_dialog;

static khui_dialog * khui_dialogs = NULL;
static int n_khui_dialogs = 0;
static BOOL khui_main_window_active;

/* should only be called from the UI thread */
void khm_add_dialog(HWND dlg) {
    if (n_khui_dialogs < MAX_UI_DIALOGS) {
        khui_dialog * d = PMALLOC(sizeof(*d));

        d->hwnd = dlg;
        LINIT(d);

        LPUSH(&khui_dialogs, d);
        n_khui_dialogs++;
    } else {
        assert(FALSE);
    }
}

/* should only be called from the UI thread */
void khm_del_dialog(HWND dlg) {
    khui_dialog * d;

    for (d = khui_dialogs; d; d = LNEXT(d)) {
        if (d->hwnd == dlg) {
            LDELETE(&khui_dialogs, d);
            n_khui_dialogs--;
        }
    }
}

BOOL khm_check_dlg_message(LPMSG pmsg) {
    BOOL found = FALSE;
    khui_dialog *d;

    for(d = khui_dialogs; d; d = LNEXT(d)) {
        if(IsDialogMessage(d->hwnd, pmsg)) {
            found = TRUE;
            break;
        }
    }

    return found;
}

BOOL khm_is_dialog_active(void) {
    HWND hwnd;
    khui_dialog * d;

    hwnd = GetForegroundWindow();

    for (d = khui_dialogs; d; d = LNEXT(d)) {
        if (d->hwnd == hwnd)
            return TRUE;
    }

    return FALSE;
}

/* We support at most 256 property sheets simultaneously.  256
   property sheets should be enough for everybody. */
#define MAX_UI_PROPSHEETS 256

static khui_property_sheet *ui_propsheets[MAX_UI_PROPSHEETS];
static int n_ui_propsheets = 0;

void khm_add_property_sheet(khui_property_sheet * s) {
    if(n_ui_propsheets < MAX_UI_PROPSHEETS)
        ui_propsheets[n_ui_propsheets++] = s;
    else {
        assert(FALSE);
    }
}

void khm_del_property_sheet(khui_property_sheet * s) {
    int i;

    for(i=0;i < n_ui_propsheets; i++) {
        if(ui_propsheets[i] == s)
            break;
    }

    if(i < n_ui_propsheets)
        n_ui_propsheets--;
    else
        return;

    for(;i < n_ui_propsheets; i++) {
        ui_propsheets[i] = ui_propsheets[i+1];
    }
}

BOOL khm_check_ps_message(LPMSG pmsg) {
    int i;
    khui_property_sheet * ps;
    for(i=0;i<n_ui_propsheets;i++) {
        if(khui_ps_check_message(ui_propsheets[i], pmsg)) {
            if(ui_propsheets[i]->status == KHUI_PS_STATUS_DONE) {
                ps = ui_propsheets[i];

                ps->status = KHUI_PS_STATUS_DESTROY;
                kmq_post_message(KMSG_CRED, KMSG_CRED_PP_END, 0, (void *) ps);

                return TRUE;
            }
            return TRUE;
        }
    }

    return FALSE;
}

BOOL khm_find_and_activate_property_sheet(khui_action_context * pctx)
{
    int i;

    for (i=0; i < n_ui_propsheets; i++) {
        if (ui_propsheets[i]->ctx.scope == pctx->scope &&
            kcdb_identity_is_equal(ui_propsheets[i]->identity, pctx->identity) &&
            ((pctx->cred == NULL && ui_propsheets[i]->cred == NULL) ||
             kcdb_creds_is_equal(pctx->cred, ui_propsheets[i]->cred)) &&
            ui_propsheets[i]->status != KHUI_PS_STATUS_DONE &&
            ui_propsheets[i]->status != KHUI_PS_STATUS_DESTROY) {
            /* found */
            SetActiveWindow(ui_propsheets[i]->hwnd);
            return TRUE;
        }
    }

    return FALSE;
}

typedef struct modal_dialog {
    HWND modal;
    HWND *enabled;
    int n_enabled;

    LDCL(struct modal_dialog);
} modal_dialog;

modal_dialog * modal_dialogs = NULL;

void khm_enter_modal(HWND hwnd)
{
    HWND enabled[MAX_UI_PROPSHEETS + MAX_UI_DIALOGS];
    int n_enabled = 0;
    int i;
    khui_dialog * d;
    modal_dialog * md;

    for (d = khui_dialogs; d; d = LNEXT(d)) {
        if (hwnd != d->hwnd && IsWindowEnabled(d->hwnd))
            enabled[n_enabled++] = d->hwnd;
    }

    for (i=0; i < n_ui_propsheets; i++) {
        if (hwnd != ui_propsheets[i]->hwnd &&
            IsWindowEnabled(ui_propsheets[i]->hwnd))
            enabled[n_enabled++] = ui_propsheets[i]->hwnd;
    }

    assert(n_enabled < MAX_UI_PROPSHEETS + MAX_UI_DIALOGS);

    md = PMALLOC(sizeof(*md));
    memset(md, 0, sizeof(*md));

    md->modal = hwnd;
    md->n_enabled = n_enabled;
    if (n_enabled) {
        md->enabled = PMALLOC(sizeof(HWND) * n_enabled);
        memcpy(md->enabled, enabled, sizeof(HWND) * n_enabled);
    } else {
        md->enabled = NULL;
    }

    LPUSH(&modal_dialogs, md);

    for (i = 0; i < n_enabled; i++)
        EnableWindow(enabled[i], FALSE);
}

void khm_leave_modal(void)
{
    modal_dialog * md = NULL;
    int i;

    LPOP(&modal_dialogs, &md);

    assert(md);

    if (md == NULL)
        return;

    for (i = 0; i < md->n_enabled; i++) {
        EnableWindow(md->enabled[i], TRUE);
    }

    if (md->enabled)
        PFREE(md->enabled);
    PFREE(md);
}
