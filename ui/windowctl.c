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
    HWND hwnd_next;
    BOOL active;

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
        d->hwnd_next = NULL;
        d->active = TRUE;
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

khui_property_sheet *_ui_propsheets[MAX_UI_PROPSHEETS];
int _n_ui_propsheets = 0;

void khm_add_property_sheet(khui_property_sheet * s) {
    if(_n_ui_propsheets < MAX_UI_PROPSHEETS)
        _ui_propsheets[_n_ui_propsheets++] = s;
    else {
        assert(FALSE);
    }
}

void khm_del_property_sheet(khui_property_sheet * s) {
    int i;

    for(i=0;i < _n_ui_propsheets; i++) {
        if(_ui_propsheets[i] == s)
            break;
    }

    if(i < _n_ui_propsheets)
        _n_ui_propsheets--;
    else
        return;

    for(;i < _n_ui_propsheets; i++) {
        _ui_propsheets[i] = _ui_propsheets[i+1];
    }
}

BOOL khm_check_ps_message(LPMSG pmsg) {
    int i;
    khui_property_sheet * ps;
    for(i=0;i<_n_ui_propsheets;i++) {
        if(khui_ps_check_message(_ui_propsheets[i], pmsg)) {
            if(_ui_propsheets[i]->status == KHUI_PS_STATUS_DONE) {
                ps = _ui_propsheets[i];

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

    for (i=0; i < _n_ui_propsheets; i++) {
        if (_ui_propsheets[i]->ctx.scope == pctx->scope &&
            kcdb_identity_is_equal(_ui_propsheets[i]->identity, pctx->identity) &&
            ((pctx->cred == NULL && _ui_propsheets[i]->cred == NULL) ||
             kcdb_creds_is_equal(pctx->cred, _ui_propsheets[i]->cred)) &&
            _ui_propsheets[i]->status != KHUI_PS_STATUS_DONE &&
            _ui_propsheets[i]->status != KHUI_PS_STATUS_DESTROY) {
            /* found */
            SetActiveWindow(_ui_propsheets[i]->hwnd);
            return TRUE;
        }
    }

    return FALSE;
}
