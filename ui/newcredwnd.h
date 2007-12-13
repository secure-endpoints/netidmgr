/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
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

#ifndef __KHIMAIRA_NEWCREDWND_H
#define __KHIMAIRA_NEWCREDWND_H

#include<khuidefs.h>

#define KHUI_NEWCREDWND_CLASS L"KhmNewCredWnd"

typedef enum tag_nc_notification_types {
    NC_NOTIFY_NONE = 0,         /* no notification */
    NC_NOTIFY_MARQUEE,          /* marquee type notification */
    NC_NOTIFY_PROGRESS,         /* progress notification */
    NC_NOTIFY_MESSAGE,          /* a message */
} nc_notification_type;

typedef enum tag_nc_page {
    NC_PAGE_NONE = 0,
    NC_PAGE_IDSPEC,
    NC_PAGE_CREDOPT_BASIC,
    NC_PAGE_CREDOPT_ADV,
    NC_PAGE_PASSWORD,
    NC_PAGE_PROGRESS
} nc_page;

typedef struct tag_khui_nc_wnd_data {
    khui_new_creds * nc;

    /* Mode and sequence */
    nc_page          page;
    khm_boolean      enable_prev;
    khm_boolean      enable_next;

    /* Identity information */
    HICON            id_icon;
    wchar_t          id_display_string[KCDB_IDENT_MAXCCH_NAME];
    wchar_t          id_type_string[KCDB_MAXCCH_SHORT_DESC];
    wchar_t          id_status[KCDB_MAXCCH_SHORT_DESC];

    /* Privileged interaction */
    khui_new_creds_privint * privint; /* Current privileged interaction panel */

    /* Tab Control (only valid if in advanced mode) */
    khm_boolean      tab_initialized;
    HWND             last_tab_panel;

    /* Sizing the new credentials window */

    khm_boolean animation_enabled; /* Flag indicating whether
                                   animation is enabled for the dialg.
                                   If this flag is off, we don't
                                   animate size changes even if the
                                   configuration says so. */
    khm_boolean size_changing;  /* flag indicating that the size of
                                   the main window is being
                                   adjusted. */
    RECT sz_ch_source;          /* Source size, from which we are
                                   going towards target size in
                                   sz_ch_max steps. The RECT is self
                                   relative (i.e. left=0 and top=0)*/
    RECT sz_ch_target;          /* If we are doing an incremental size
                                   change, this holds the target size
                                   that we were going for.  Note that
                                   the target size might change while
                                   we are adjusting the size.  So this
                                   helps keep track of whether we need
                                   to start the size change again. The
                                   RECT is self relative (i.e. left=0
                                   and top=0). */
    int  sz_ch_increment;       /* Current step of the incremental
                                   size change operation. */
    int  sz_ch_max;             /* Max number of steps in the size
                                   change operation. */
    int  sz_ch_timeout;         /* Milliseconds between each increment */

    /* Behavior */

    khm_boolean flashing_enabled; /* The window maybe still flashing
                                   from the last call to
                                   FlashWindowEx(). */
    khm_boolean force_topmost;  /* Force New Credentials window to the
                                   top */

    /* Notification windows */

    nc_notification_type notif_type; /* Type of notification */
    HWND hwnd_notif_label;      /* Label for notifications */
    HWND hwnd_notif_aux;        /* Other control for notifications */

    /* Other windows */
    HWND hwnd_nav;              /* IDD_NC_NAV */
    HWND hwnd_idsel;            /* IDD_NC_IDSEL */
    HWND hwnd_idspec;           /* IDD_NC_IDSPEC */
    HWND hwnd_privint_basic;    /* IDD_NC_PRIVINT_BASIC */
    HWND hwnd_privint_advanced; /* IDD_NC_PRIVINT_ADVANCED */
    HWND hwnd_progress;         /* IDD_NC_PROGRESS */

    HWND hwnd_noprompts;        /* IDD_NC_NOPROMPTS */
} khui_nc_wnd_data;

void khm_register_newcredwnd_class(void);
void khm_unregister_newcredwnd_class(void);
HWND khm_create_newcredwnd(HWND parent, khui_new_creds * c);
void khm_prep_newcredwnd(HWND hwnd);
void khm_show_newcredwnd(HWND hwnd);

/* Control identifier for the tab control in the new credentials
   dialog. We declare this here since we will be creating the control
   manually. */
#define IDC_NC_TABS 8001

#define NC_BN_SET_DEF_ID 8012

/* the first control ID that may be used by an identity provider */
#define NC_IS_CTRL_ID_MIN 8016

/* the maximum number of controls that may be created by an identity
   provider*/
#define NC_IS_CTRL_MAX_CTRLS 8

/* the maximum control ID that may be used by an identity provider */
#define NC_IS_CTRL_ID_MAX (NC_IS_CTRL_ID_MIN + NC_IS_MAX_CTRLS - 1)

#define NC_SZ_STEPS_MIN 3
#define NC_SZ_STEPS_DEF 10
#define NC_SZ_STEPS_MAX 100

#define NC_SZ_TIMEOUT_MIN 5
#define NC_SZ_TIMEOUT_DEF 10
#define NC_SZ_TIMEOUT_MAX 500

#define NC_TIMER_SIZER         1001
#define NC_TIMER_ENABLEANIMATE 1002

#define ENABLEANIMATE_TIMEOUT  400

#endif
