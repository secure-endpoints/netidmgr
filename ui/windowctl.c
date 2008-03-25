/*
 * Copyright (c) 2007 Secure Endpoints Inc.
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

/* we support up to 16 simutaneous dialogs.  In reality, more than two
   is pretty unlikely.  Property sheets are special and are handled
   separately. */
#define MAX_UI_DIALOGS 16

typedef struct tag_khui_dialog {
    HWND hwnd;
    HWND hwnd_next;
    BOOL active;
} khui_dialog;

static khui_dialog khui_dialogs[MAX_UI_DIALOGS];
static int n_khui_dialogs = 0;
static HWND khui_modal_dialog = NULL;
static BOOL khui_main_window_active;

/* should only be called from the UI thread */
void khm_add_dialog(HWND dlg) {
    if(n_khui_dialogs < MAX_UI_DIALOGS - 1) {
        khui_dialogs[n_khui_dialogs].hwnd = dlg;
        khui_dialogs[n_khui_dialogs].hwnd_next = NULL;
        khui_dialogs[n_khui_dialogs].active = TRUE;
        n_khui_dialogs++;
    } else {
#if DEBUG
          assert(FALSE);
#endif
    }
}

/* should only be called from the UI thread */
void khm_enter_modal(HWND hwnd) {
    int i;

    if (khui_modal_dialog) {

        /* we are already in a modal loop. */

#ifdef DEBUG
        assert(hwnd != khui_modal_dialog);
#endif

        for (i=0; i < n_khui_dialogs; i++) {
            if (khui_dialogs[i].hwnd == khui_modal_dialog) {
                khui_dialogs[i].active = TRUE;
                EnableWindow(khui_modal_dialog, FALSE);
                break;
            }
        }

#ifdef DEBUG
        assert(i < n_khui_dialogs);
#endif

        for (i=0; i < n_khui_dialogs; i++) {
            if (khui_dialogs[i].hwnd == hwnd) {
                khui_dialogs[i].hwnd_next = khui_modal_dialog;
                break;
            }
        }

#ifdef DEBUG
        assert(i < n_khui_dialogs);
#endif

        khui_modal_dialog = hwnd;

    } else {

        /* we are entering a modal loop.  preserve the active state of
           the overlapped dialogs and proceed with the modal
           dialog. */

        for (i=0; i < n_khui_dialogs; i++) {
            if(khui_dialogs[i].hwnd != hwnd) {
                khui_dialogs[i].active = IsWindowEnabled(khui_dialogs[i].hwnd);
                EnableWindow(khui_dialogs[i].hwnd, FALSE);
            }
        }

        khui_main_window_active = khm_is_main_window_active();
        EnableWindow(khm_hwnd_main, FALSE);

        khui_modal_dialog = hwnd;

        SetForegroundWindow(hwnd);
    }
}

/* should only be called from the UI thread */
void khm_leave_modal(void) {
    int i;

    for (i=0; i < n_khui_dialogs; i++) {
        if (khui_dialogs[i].hwnd == khui_modal_dialog)
            break;
    }

#ifdef DEBUG
    assert(i < n_khui_dialogs);
#endif

    if (i < n_khui_dialogs && khui_dialogs[i].hwnd_next) {

        /* we need to proceed to the next one down the modal dialog
           chain.  We are not exiting a modal loop. */

        khui_modal_dialog = khui_dialogs[i].hwnd_next;
        khui_dialogs[i].hwnd_next = FALSE;

        EnableWindow(khui_modal_dialog, TRUE);

    } else {

        HWND last_dialog = NULL;

        /* we are exiting a modal loop. */

        for (i=0; i < n_khui_dialogs; i++) {
            if(khui_dialogs[i].hwnd != khui_modal_dialog) {
                EnableWindow(khui_dialogs[i].hwnd, khui_dialogs[i].active);
                last_dialog = khui_dialogs[i].hwnd;
            }
        }

        EnableWindow(khm_hwnd_main, TRUE);

        khui_modal_dialog = NULL;

        if(last_dialog)
            SetActiveWindow(last_dialog);
        else
            SetActiveWindow(khm_hwnd_main);
    }
}

/* should only be called from the UI thread */
void khm_del_dialog(HWND dlg) {
    int i;
    for(i=0;i < n_khui_dialogs; i++) {
        if(khui_dialogs[i].hwnd == dlg)
            break;
    }
    
    if(i < n_khui_dialogs)
        n_khui_dialogs--;
    else
        return;

    for(;i < n_khui_dialogs; i++) {
        khui_dialogs[i] = khui_dialogs[i+1];
    }
}

BOOL khm_check_dlg_message(LPMSG pmsg) {
    int i;
    BOOL found = FALSE;
    for(i=0;i<n_khui_dialogs;i++) {
        if(IsDialogMessage(khui_dialogs[i].hwnd, pmsg)) {
            found = TRUE;
            break;
        }
    }

    return found;
}

BOOL khm_is_dialog_active(void) {
    HWND hwnd;
    int i;

    hwnd = GetForegroundWindow();

    for (i=0; i<n_khui_dialogs; i++) {
        if (khui_dialogs[i].hwnd == hwnd)
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
#ifdef DEBUG
        assert(FALSE);
#endif
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

