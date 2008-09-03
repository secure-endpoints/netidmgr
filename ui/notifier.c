/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
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
 * ACTION OF CONTRACT TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* $Id$ */

#define OEMRESOURCE

#include "khmapp.h"
#include<assert.h>

#define KHUI_NOTIFIER_CLASS         L"KhuiNotifierMsgWindowClass"
#define KHUI_ALERTER_CLASS          L"KhuiAlerterWindowClass"
#define KHUI_ALERTBIN_CLASS         L"KhuiAlertBinWindowClass"

#define KHUI_NOTIFIER_WINDOW        L"KhuiNotifierMsgWindow"


/* The commands that are available as default actions when the user
   clicks the notification icon. */

khm_int32 khm_notifier_actions[] = {
    KHUI_ACTION_OPEN_APP,
    KHUI_ACTION_NEW_CRED
};

khm_size  n_khm_notifier_actions = ARRAYLENGTH(khm_notifier_actions);

/* notifier message for notification icon */
#define KHUI_WM_NOTIFIER            WM_COMMAND

#define DRAWTEXTOPTIONS     (DT_NOPREFIX | DT_WORDBREAK)
#define MEASURETEXTOPTIONS  (DT_CALCRECT | DRAWTEXTOPTIONS)

/* are we showing an alert? */
#define ALERT_DISPLAYED() (balloon_alert != NULL || khui_alert_windows != NULL)

/* Forward declarations */

struct tag_alerter_wnd_data;
typedef struct tag_alerter_wnd_data alerter_wnd_data;

struct tag_alert_list;
typedef struct tag_alert_list alert_list;

static khm_int32
alert_show(khui_alert * a);

static khm_int32
alert_show_minimized(khui_alert * a);

static khm_int32
alert_show_normal(khui_alert * a);

static khm_int32
alert_show_list(alert_list * alist);

static khm_int32
alert_enqueue(khui_alert * a);

static khm_boolean
alert_is_equal(khui_alert * a1, khui_alert * a2);

static void
check_for_queued_alerts(void);

static void
show_queued_alerts(void);

static khm_int32
alert_consolidate(alert_list * alist,
                  khui_alert * alert,
                  khm_boolean add_from_queue);

static void
start_monitoring_alert(khui_alert * alert);

/* Globals */

/* window class registration atom for message only notifier window
   class */
ATOM atom_notifier = 0;

/* window class registration atom for alert windows */
ATOM atom_alerter = 0;
/* window class registration atom for the alert "bin", which is the
   window that holds all the alerts. */
ATOM atom_alert_bin = 0;

/* notifier message window */
HWND hwnd_notifier = NULL;

BOOL notifier_ready = FALSE;

/* The list of alert windows currently active */
alerter_wnd_data * khui_alert_windows = NULL;

/* Notification icon for when there are no alerts to be displayed */
int  iid_normal = IDI_NOTIFY_NONE;

/* Tooltip to use when there are no alerts to be displayed */
wchar_t tip_normal[128] = L"";

/* Current notifier severity level */
khm_int32 notifier_severity = KHERR_NONE;

/* The alert currently being displayed in a balloon */
khui_alert * balloon_alert = NULL;

/**********************************************************************
  Alert Queue

  The alert queue is the data structure that keeps track of all the
  alerts that are waiting to be displayed.  Alerts will be placed on
  the queue if they cannot be immediately displayed for some reason
  (e.g. another alert is being displayed, or the user is working in
  another window).
***********************************************************************/

#define KHUI_ALERT_QUEUE_MAX        64

khui_alert * alert_queue[KHUI_ALERT_QUEUE_MAX];
khm_int32    alert_queue_head = 0;
khm_int32    alert_queue_tail = 0;

#define is_alert_queue_empty() (alert_queue_head == alert_queue_tail)
#define is_alert_queue_full()  (((alert_queue_tail + 1) % KHUI_ALERT_QUEUE_MAX) == alert_queue_head)

/* NOTE: the alert queue functions are unsafe to call from any thread
   other than the UI thread. */

static void
alert_queue_put_alert(khui_alert * a) {
    if (is_alert_queue_full()) return;
    alert_queue[alert_queue_tail++] = a;
    khui_alert_hold(a);
    alert_queue_tail %= KHUI_ALERT_QUEUE_MAX;
}

/* the caller needs to release the alert that's returned  */
static khui_alert *
alert_queue_get_alert(void) {
    khui_alert * a;

    if (is_alert_queue_empty()) return NULL;
    a = alert_queue[alert_queue_head++];
    alert_queue_head %= KHUI_ALERT_QUEUE_MAX;

    return a;                   /* held */
}

static int
alert_queue_get_size(void) {
    if (is_alert_queue_empty())
        return 0;

    if (alert_queue_tail < alert_queue_head) {
        return (alert_queue_tail + KHUI_ALERT_QUEUE_MAX - alert_queue_head);
    } else {
        return alert_queue_tail - alert_queue_head;
    }
}

static khui_alert *
alert_queue_get_alert_by_pos(int pos) {
    khui_alert * a;

    if (is_alert_queue_empty() ||
        pos >= alert_queue_get_size() ||
        pos < 0) {
        return NULL;
    }

    a = alert_queue[(alert_queue_head + pos) % KHUI_ALERT_QUEUE_MAX];
    if (a) {
        khui_alert_hold(a);
    }
    return a;
}

static int
alert_queue_delete_alert(khui_alert * a) {
    int idx;
    int succ;

    idx = alert_queue_head;
    while(idx != alert_queue_tail) {
        if (alert_queue[idx] == a)
            break;

        idx = (idx + 1) % KHUI_ALERT_QUEUE_MAX;
    }

    if (idx == alert_queue_tail)
        return 0;

#ifdef DEBUG
    assert(alert_queue[idx]);
#endif
    khui_alert_release(alert_queue[idx]);

    succ = (idx + 1) % KHUI_ALERT_QUEUE_MAX;
    while(succ != alert_queue_tail) {
        alert_queue[idx] = alert_queue[succ];

        succ = (succ + 1) % KHUI_ALERT_QUEUE_MAX;
        idx = (idx + 1) % KHUI_ALERT_QUEUE_MAX;
    }

    alert_queue_tail = idx;
    return 1;
}

/* the caller needs to release the alert that's returned */
static khui_alert *
alert_queue_peek(void) {
    khui_alert * a;

    if (is_alert_queue_empty())
        return NULL;

    a = alert_queue[alert_queue_head];
    khui_alert_hold(a);

    return a;
}

/**********************************************************************
  Alert List

  A list of alerts.  Currently has a fixed upper limit, but the limit
  is high enough for now.
***********************************************************************/

typedef struct tag_alert_list {
    khui_alert * alerts[KHUI_ALERT_QUEUE_MAX];
    int          n_alerts;
    wchar_t      title[KHUI_MAXCCH_TITLE];
} alert_list;

static void
alert_list_init(alert_list * alist) {
    ZeroMemory(alist, sizeof(*alist));
}

static void
alert_list_set_title(alert_list * alist, wchar_t * title) {
    StringCbCopy(alist->title, sizeof(alist->title), title);
}

static khm_int32
alert_list_add_alert(alert_list * alist,
                     khui_alert * alert) {

    if (alist->n_alerts == ARRAYLENGTH(alist->alerts))
        return KHM_ERROR_NO_RESOURCES;

    khui_alert_hold(alert);
    alist->alerts[alist->n_alerts++] = alert;

    return KHM_ERROR_SUCCESS;
}

static void
alert_list_destroy(alert_list * alist) {
    int i;

    for (i=0; i < alist->n_alerts; i++) {
        if (alist->alerts[i] != NULL) {
            khui_alert_release(alist->alerts[i]);
            alist->alerts[i] = NULL;
        }
    }

    alist->n_alerts = 0;
}


/**********************************************************************
  Notifier Window

  The notifier window manages the notification icon and handles
  KMSG_ALERT messages sent from the UI library.  The window will exist
  for the lifetime of the application.
***********************************************************************/

/* These are defined for APPVER >= 0x501.  We are defining them here
   so that we can build with APPVER = 0x500 and use the same binaries
   with Win XP. */

#ifndef NIN_BALLOONSHOW
#define NIN_BALLOONSHOW (WM_USER + 2)
#endif

#ifndef NIN_BALLOONHIDE
#define NIN_BALLOONHIDE (WM_USER + 3)
#endif

#ifndef NIN_BALLOONTIMEOUT
#define NIN_BALLOONTIMEOUT (WM_USER + 4)
#endif

#ifndef NIN_BALLOONUSERCLICK
#define NIN_BALLOONUSERCLICK (WM_USER + 5)
#endif


static LRESULT CALLBACK
notifier_wnd_proc(HWND hwnd,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam)
{
    kmq_message * m;
    khm_int32 rv;

    if(uMsg == KMQ_WM_DISPATCH) {
        kmq_wm_begin(lParam, &m);
        rv = KHM_ERROR_SUCCESS;

        if(m->type == KMSG_ALERT) {
            /* handle notifier messages */
            switch(m->subtype) {
            case KMSG_ALERT_SHOW:
                {
                    khui_alert * a;

                    a = (khui_alert *) m->vparam;
#ifdef DEBUG
                    assert(a != NULL);
#endif
                    rv = alert_show(a);
                    khui_alert_release(a);
                }
                break;

            case KMSG_ALERT_QUEUE:
                {
                    khui_alert * a;

                    a = (khui_alert *) m->vparam;
#ifdef DEBUG
                    assert(a != NULL);
#endif
                    rv = alert_enqueue(a);
                    khui_alert_release(a);
                }
                break;

            case KMSG_ALERT_CHECK_QUEUE:
                check_for_queued_alerts();
                break;

            case KMSG_ALERT_SHOW_QUEUED:
                show_queued_alerts();
                break;

            case KMSG_ALERT_SHOW_MODAL:
                {
                    khui_alert * a;

                    a = (khui_alert *) m->vparam;
#ifdef DEBUG
                    assert(a != NULL);
#endif
                    khui_alert_lock(a);
                    a->flags |= KHUI_ALERT_FLAG_MODAL;
                    khui_alert_unlock(a);

                    rv = alert_show(a);

                    if (KHM_SUCCEEDED(rv)) {
                        khm_message_loop_int(&a->displayed);
                    }

                    khui_alert_release(a);
                }
                break;

            case KMSG_ALERT_MONITOR_PROGRESS:
                {
                    khui_alert * a;

                    a = (khui_alert *) m->vparam;
#ifdef DEBUG
                    assert(a != NULL);
#endif
                    start_monitoring_alert(a);
                }
                break;

            }
        } else if (m->type == KMSG_CRED &&
                   m->subtype == KMSG_CRED_ROOTDELTA) {

            KillTimer(hwnd, KHUI_REFRESH_TIMER_ID);
            SetTimer(hwnd, KHUI_REFRESH_TIMER_ID,
                     KHUI_REFRESH_TIMEOUT,
                     NULL);

        }

        return kmq_wm_end(m, rv);
    } else if (uMsg == KHUI_WM_NOTIFIER) {
        /* Handle events generated from the notification icon */

        /* wParam is the identifier of the notify icon, but we only
           have one. */
        switch(lParam) {
        case WM_CONTEXTMENU:
            {
                POINT pt;
                int menu_id;
                khui_menu_def * mdef;
                khui_action_ref * act = NULL;
                khm_size i, n;
                khm_int32 def_cmd;

                /* before we show the context menu, we need to make
                   sure that the default action for the notification
                   icon is present in the menu and that it is marked
                   as the default. */

                def_cmd = khm_get_default_notifier_action();

                if (khm_is_main_window_visible()) {
                    menu_id = KHUI_MENU_ICO_CTX_NORMAL;

                    if (def_cmd == KHUI_ACTION_OPEN_APP)
                        def_cmd = KHUI_ACTION_CLOSE_APP;
                } else {
                    menu_id = KHUI_MENU_ICO_CTX_MIN;
                }

                mdef = khui_find_menu(menu_id);

#ifdef DEBUG
                assert(mdef);
#endif
                n = khui_menu_get_size(mdef);
                for (i=0; i < n; i++) {
                    act = khui_menu_get_action(mdef, i);
                    if (!(act->flags & KHUI_ACTIONREF_PACTION) &&
                        (act->action == def_cmd))
                        break;
                }

                if (i < n) {
                    if (!(act->flags & KHUI_ACTIONREF_DEFAULT)) {
                        khui_menu_remove_action(mdef, i);
                        khui_menu_insert_action(mdef, i, def_cmd, KHUI_ACTIONREF_DEFAULT);
                    } else {
                        /* we are all set */
                    }
                } else {
                    /* the default action was not found on the context
                       menu */
#ifdef DEBUG
                    assert(FALSE);
#endif
                    khui_menu_insert_action(mdef, 0, def_cmd, KHUI_ACTIONREF_DEFAULT);
                }

                SetForegroundWindow(khm_hwnd_main);

                GetCursorPos(&pt);
                khm_menu_show_panel(menu_id, pt.x, pt.y);

                PostMessage(khm_hwnd_main, WM_NULL, 0, 0);
            }
            break;

        case NIN_SELECT:
            /* fall through */
        case NIN_KEYSELECT:
            /* If there were any alerts waiting to be shown, we show
               them.  Otherwise we perform the default action. */
            khm_notify_icon_activate();
            break;

        case NIN_BALLOONUSERCLICK:
            if (balloon_alert) {
                khui_alert * a;

                khm_notify_icon_change(KHERR_NONE);

                a = balloon_alert;
                balloon_alert = NULL;

                khui_alert_lock(a);
                a->displayed = FALSE;

                if ((a->flags & KHUI_ALERT_FLAG_DEFACTION) &&
                    !(a->flags & KHUI_ALERT_FLAG_REQUEST_WINDOW) &&
                    a->n_alert_commands > 0) {
                    PostMessage(khm_hwnd_main, WM_COMMAND,
                                MAKEWPARAM(a->alert_commands[0],
                                           0),
                                0);
                } else if (a->flags &
                           KHUI_ALERT_FLAG_REQUEST_WINDOW) {
                    khm_show_main_window();
                    alert_show_normal(a);
                }

                khui_alert_unlock(a);
                khui_alert_release(a);
            } else {
#ifdef DEBUG
                assert(FALSE);
#endif
            }
            break;

        case NIN_BALLOONHIDE:
        case NIN_BALLOONTIMEOUT:
            khm_notify_icon_change(KHERR_NONE);
            if (balloon_alert) {
                khui_alert * a;
                a = balloon_alert;
                balloon_alert = NULL;

                khui_alert_lock(a);
                a->displayed = FALSE;
                khui_alert_unlock(a);

                khui_alert_release(a);
            }
            break;
        }
    } else if (uMsg == WM_TIMER) {
        if (wParam == KHUI_TRIGGER_TIMER_ID) {
            KillTimer(hwnd, KHUI_TRIGGER_TIMER_ID);
            khm_timer_fire(hwnd);
        } else if (wParam == KHUI_REFRESH_TIMER_ID) {
            KillTimer(hwnd, KHUI_REFRESH_TIMER_ID);
            kcdb_identity_refresh_all();
            khm_timer_refresh(hwnd);
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

ATOM
khm_register_notifier_wnd_class(void)
{
    WNDCLASSEX wcx;

    ZeroMemory(&wcx, sizeof(wcx));

    wcx.cbSize = sizeof(wcx);
    wcx.style = 0;
    wcx.lpfnWndProc = notifier_wnd_proc;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = 0;
    wcx.hInstance = khm_hInstance;
    wcx.hIcon = NULL;
    wcx.hCursor = NULL;
    wcx.hbrBackground = NULL;
    wcx.lpszMenuName = NULL;
    wcx.lpszClassName = KHUI_NOTIFIER_CLASS;
    wcx.hIconSm = NULL;

    atom_notifier = RegisterClassEx(&wcx);

    return atom_notifier;
}

/*********************************************************************
  Alerter
**********************************************************************/

/*! \brief An alert that is being displayed to the user
*/
typedef struct tag_alerter_alert_data {
    khui_alert * alert;         /*!< The alert */

    BOOL         seen;          /*!< has the user seen this alert? */

    BOOL         has_commands;  /*!< we cache the value here.
                                   otherwise we'll have to get a lock
                                   on the alert each time we have to
                                   find out whether there are any
                                   commands for this alert. */

    RECT         r_alert;    /*!< the entire alert, relative to self. */

    /* the following rects are relative to the top left of r_alert. */

    RECT         r_title;       /*!< the title.  deflate by padding to
                                   get the text rect. */
    RECT         r_icon;        /*!< rect for icon */
    RECT         r_message;     /*!< rect for the text. no padding
                                   necessary. */
    RECT         r_suggestion;  /*!< rect for the suggestion.  deflate
                                   by padding to get the suggestion
                                   rect.  The suggestion rect includes
                                   space for the small icon on the
                                   left and padding between the icon
                                   and the text. The size of the small
                                   icon are as per system metrics
                                   SM_C{X,Y}SMICON. Padding is
                                   s_pad.cx vertical. */

    RECT         r_progress;    /*!< rect for progress control, if
                                   applicable. */

    int          n_cmd_buttons; /*!< number of command buttons in this
                                   alert. */

    RECT         r_buttons[KHUI_MAX_ALERT_COMMANDS];
                                /*!< rects for the command buttons. */

    HWND         hwnd_buttons[KHUI_MAX_ALERT_COMMANDS];
                                /*!< handles for the command buttons */

    HWND         hwnd_marker;
                                /*!< handle to the marker window used as
                                   a tab-stop target when there are
                                   not buttons associated with the
                                   alert. */

    HWND         hwnd_progress;
                                /*!< Handle to the progress control */

    khm_ui_4     progress_num, progress_denom;
                                /*!< Progress of the kherr_context
                                  associated with the alert. */
    BOOL         alert_ctx_done; /*!< The context associated with the
                                   alert has completed. */

    LDCL(struct tag_alerter_alert_data);
} alerter_alert_data;

/*! \brief Window data for an alert container */
typedef struct tag_alerter_wnd_data {
    khm_int32       magic;
#define ALERTER_WND_DATA_MAGIC 0x15e5b2bc

    HWND            hwnd;       /*!< Handle of container or parent window */
    HFONT           hfont;

    wchar_t         caption[KHUI_MAXCCH_TITLE]; /*!< the original
                                                   caption for the
                                                   dialog. */

    HWND            hw_bin;
    HWND            hw_scroll;
    HWND            hw_close;

    int             scroll_top;

    int             n_cmd_buttons; /*!< total number of command
                                      buttons in all the alerts being
                                      shown in this dialog. */

    int             c_alert;    /*!< current selected alert. */

    /* various metrics */
    /* calculated during WM_CREATE */
    SIZE            s_margin;
    SIZE            s_icon;
    SIZE            s_pad;
    SIZE            s_button;

    int             cx_wnd;
    int             cy_max_wnd;

    /* derived from the alert sizes */
    SIZE            s_alerts;
    RECT            r_client_area; /*!< Client area. Only used when
                                     the alert is a child window. */

    QDCL(alerter_alert_data);   /* queue of alerts that are being
                                   shown in this window. */

    LDCL(struct tag_alerter_wnd_data); /* for adding to
                                          khui_alert_windows list. */

    int             n_alerts;
    int             flags;      /*!< Combination of KHM_ALERTF_* values */

} alerter_wnd_data;

#define NTF_PARAM DWLP_USER

/* dialog sizes in base dialog units */

#define NTF_MARGIN          5
#define NTF_WIDTH           200
#define NTF_MAXHEIGHT       150

#define NTF_TEXT_PAD        2

#define NTF_BUTTON_WIDTH    50
#define NTF_BUTTON_HEIGHT   14

#define NTF_TIMEOUT 20000

#define ALERT_WINDOW_EX_SYLES (WS_EX_DLGMODALFRAME | WS_EX_CONTEXTHELP)
#define ALERT_WINDOW_STYLES   (WS_DLGFRAME | WS_POPUPWINDOW | WS_CLIPCHILDREN | DS_NOIDLEMSG)

/* Control ids */
#define IDC_NTF_ALERTBIN 998
#define IDC_NTF_CLOSE    999

#define IDC_NTF_CMDBUTTONS 1001
#define IDC_FROM_IDX(alert, bn) ((alert) * (KHUI_MAX_ALERT_COMMANDS + 2) + (bn) + 2 + IDC_NTF_CMDBUTTONS)
#define ALERT_FROM_IDC(idc)  (((idc) - IDC_NTF_CMDBUTTONS) / (KHUI_MAX_ALERT_COMMANDS + 2))
#define BUTTON_FROM_IDC(idc) (((idc) - IDC_NTF_CMDBUTTONS) % (KHUI_MAX_ALERT_COMMANDS + 2) - 2)

/* if the only command in an alert is "Close", we assume that the
   alert has no commands. */
#define ALERT_HAS_CMDS(a) ((a)->n_alert_commands > 1 || ((a)->n_alert_commands == 1 && (a)->alert_commands[0] != KHUI_PACTION_CLOSE))

#define SCROLL_LINE_SIZE(d) ((d)->cy_max_wnd / 12)

#define WMNC_ALERT_RECALC_LAYOUT 1
#define WMNC_ALERT_REFRESH_VIEW  2
#define WMNC_ALERT_NEW_CTX       3

static void
add_alert_to_wnd_data(alerter_wnd_data * d,
                      khui_alert * a) {
    alerter_alert_data * aiter;
    khm_boolean exists = 0;

    khui_alert_lock(a);

    /* check if the alert is already there */
    aiter = QTOP(d);
    while(aiter && !exists) {
        if (aiter->alert) {
            khui_alert_lock(aiter->alert);

            if (alert_is_equal(aiter->alert, a)) {
                exists = TRUE;
            }

            khui_alert_unlock(aiter->alert);
        }

        aiter = QNEXT(aiter);
    }

    a->flags |= KHUI_ALERT_FLAG_DISPLAY_WINDOW;

    if (!exists) {
        a->displayed = TRUE;
    }

    khui_alert_unlock(a);

    if (!exists) {
        alerter_alert_data * adata;

        adata = PMALLOC(sizeof(*adata));
        ZeroMemory(adata, sizeof(*adata));

        adata->alert = a;
        QPUT(d, adata);
        d->n_alerts ++;

        khui_alert_hold(a);
        khui_alert_lock(a);

        a->wnd_data = d;
        a->adata = adata;

        khui_alert_unlock(a);
    }
}

static alerter_wnd_data *
create_alerter_wnd_data(HWND hwnd, alert_list * l,
                        const RECT * rpos,
                        khm_int32 flags) {
    alerter_wnd_data * d;
    int i;
    LONG dlgb;

    d = PMALLOC(sizeof(*d));
    ZeroMemory(d, sizeof(*d));

    d->magic = ALERTER_WND_DATA_MAGIC;
    d->hwnd = hwnd;
    d->flags = flags;

    if (l != NULL) {
        for (i=0; i < l->n_alerts; i++) {
            add_alert_to_wnd_data(d, l->alerts[i]);
        }

        d->n_alerts = l->n_alerts;
    }

    if (!(flags & KHM_ALERTF_CHILD)) {

        GetWindowText(hwnd, d->caption, ARRAYLENGTH(d->caption));
        LPUSH(&khui_alert_windows, d);

    }

    /* Compute a few metrics first */

    dlgb = GetDialogBaseUnits();

#define DLG2SCNX(x) MulDiv((x), LOWORD(dlgb), 4)
#define DLG2SCNY(y) MulDiv((y), HIWORD(dlgb), 8)

    if (rpos) {

        d->cx_wnd = rpos->right - rpos->left;
        d->cy_max_wnd = rpos->bottom - rpos->top;
        d->r_client_area = *rpos;

    } else {

        d->cx_wnd = DLG2SCNX(NTF_WIDTH);
        d->cy_max_wnd = DLG2SCNY(NTF_MAXHEIGHT);

    }

    d->s_margin.cx = DLG2SCNX(NTF_MARGIN);
    d->s_margin.cy = DLG2SCNY(NTF_MARGIN);

    d->s_pad.cx = DLG2SCNX(NTF_TEXT_PAD);
    d->s_pad.cy = DLG2SCNY(NTF_TEXT_PAD);

    d->s_icon.cx = GetSystemMetrics(SM_CXICON);
    d->s_icon.cy = GetSystemMetrics(SM_CYICON);

    d->s_button.cx = DLG2SCNX(NTF_BUTTON_WIDTH);
    d->s_button.cy = DLG2SCNY(NTF_BUTTON_HEIGHT);

#undef DLG2SCNX
#undef DLG2SCNY

    d->c_alert = -1;

    return d;
}

static void
layout_alert(HDC hdc, alerter_wnd_data * d,
             alerter_alert_data * adata) {
    RECT r;
    int x;
    int y;
    int text_width;

    assert(adata->alert);

    khui_alert_lock(adata->alert);

    y = 0;
    x = d->s_margin.cx;
    /* Title */

    y += d->s_margin.cy;

    if (d->flags & KHM_ALERTF_COMPACT) {
        SetRect(&adata->r_icon, x, y,
                x + d->s_icon.cx,
                y + d->s_icon.cy);

        x = adata->r_icon.right + d->s_margin.cx;
    }

    /* If there is a title and it differs from the title of the
       alerter window or the alerter window is a child window, then we
       have to show the alert title separately. */
    if (adata->alert->title &&
        ((d->flags & KHM_ALERTF_CHILD) ||
         wcscmp(adata->alert->title, d->caption))) {

        SetRect(&adata->r_title, x, y, d->cx_wnd - d->s_margin.cx, y);

        khm_draw_text(hdc, adata->alert->title, KHM_FONT_NORMAL,
                      MEASURETEXTOPTIONS, &adata->r_title);

        adata->r_title.bottom += d->s_pad.cy * 2;

        y = adata->r_title.bottom + d->s_margin.cy;

    } else {

        SetRectEmpty(&adata->r_title);

    }

    /* Icon */

    if (!(d->flags & KHM_ALERTF_COMPACT)) {
        SetRect(&adata->r_icon, x, y,
                x + d->s_icon.cx,
                y + d->s_icon.cy);

        x = adata->r_icon.right + d->s_margin.cx;
    }

    text_width = (d->cx_wnd - d->s_margin.cx) - x;

    /* Message */

    if (adata->alert->message) {

        SetRect(&adata->r_message, x, y, text_width + x, y);
        khm_draw_text(hdc, adata->alert->message, KHM_FONT_NORMAL,
                      MEASURETEXTOPTIONS, &adata->r_message);

        y = adata->r_message.bottom + d->s_margin.cy;

    } else {

        SetRectEmpty(&adata->r_message);

    }

    /* Suggestion */

    if (adata->alert->suggestion) {

        int pad = d->s_pad.cx + GetSystemMetrics(SM_CXSMICON);

        SetRect(&r, x + pad, y, x + text_width, y);

        khm_draw_text(hdc, adata->alert->suggestion, KHM_FONT_NORMAL,
                      MEASURETEXTOPTIONS, &r);
        r.left -= pad;

        InflateRect(&r, d->s_pad.cx, d->s_pad.cy);
        OffsetRect(&r, 0, -r.top + y);
        CopyRect(&adata->r_suggestion, &r);

        y = r.bottom + d->s_margin.cy;

    } else {

        SetRectEmpty(&adata->r_suggestion);

    }

    y = max(y, adata->r_icon.bottom + d->s_margin.cy);

    /* Progress bar */
    if ((adata->alert->alert_type == KHUI_ALERTTYPE_PROGRESSACQ ||
         adata->alert->alert_type == KHUI_ALERTTYPE_PROGRESS) &&
        !adata->alert_ctx_done) {

        SetRect(&adata->r_progress, x, y, x + text_width, y + d->s_margin.cy);

        y += d->s_margin.cy * 2;

    } else {

        SetRectEmpty(&adata->r_progress);

    }

    /* Buttons */

    if (ALERT_HAS_CMDS(adata->alert)) {
        khm_int32 i;
        int bx, width;
        wchar_t caption[KHUI_MAXCCH_SHORT_DESC];
        size_t len;
        SIZE s;
        int skip_close;

        adata->has_commands = TRUE;

        d->s_button.cx = (d->cx_wnd - x) / KHUI_MAX_ALERT_COMMANDS - d->s_margin.cx;

        if (d->n_alerts > 1)
            skip_close = TRUE;
        else
            skip_close = FALSE;

#ifdef DEBUG
        assert(adata->alert->n_alert_commands <= KHUI_MAX_ALERT_COMMANDS);
#endif
        bx = x;

        for (i=0; i < adata->alert->n_alert_commands; i++) {

            if (adata->alert->alert_commands[i] == KHUI_PACTION_CLOSE && skip_close) {
                SetRectEmpty(&adata->r_buttons[i]);
                continue;
            }

            caption[0] = L'\0';
            len = 0;
            khm_get_action_caption(adata->alert->alert_commands[i],
                                   caption, sizeof(caption));
            StringCchLength(caption, ARRAYLENGTH(caption), &len);

            if (!GetTextExtentPoint32(hdc, caption, (int) len, &s)) {
                width = d->s_button.cx;
            } else {
                width = s.cx + d->s_margin.cx * 2;
            }

            if (width < d->s_button.cx)
                width = d->s_button.cx;
            else if (width > text_width)
                width = text_width;

            if (bx + width + d->s_margin.cx > d->cx_wnd) {
                /* new line */
                bx = x;
                y += d->s_button.cy + d->s_pad.cy;
            }

            SetRect(&adata->r_buttons[i], bx, y, bx + width, y + d->s_button.cy);

            bx += width + d->s_margin.cx;
        }

        y += d->s_button.cy + d->s_margin.cy;
    }

    khui_alert_unlock(adata->alert);

    /* Now set the rect for the whole alert */
    SetRect(&adata->r_alert, 0, 0, d->cx_wnd, y);

}

static void
pick_title_for_alerter_window(alerter_wnd_data * d) {
    alerter_alert_data * adata;
    wchar_t caption[KHUI_MAXCCH_TITLE];
    khm_boolean common_caption = TRUE;
    khui_alert_type ctype = KHUI_ALERTTYPE_NONE;
    khm_boolean common_type = TRUE;

    /* - If all the alerts have the same title, then we use the common
         title.

       - If all the alerts are of the same type, then we pick a title
         that is suitable for the type.

       - All else fails, we use a default caption for the window.
    */

    caption[0] = L'\0';
    adata = QTOP(d);
    while (adata && (common_caption || common_type)) {

        if (adata->alert) {
            khui_alert_lock(adata->alert);

            if (common_caption) {
                if (caption[0] == L'\0') {
                    if (adata->alert->title)
                        StringCbCopy(caption, sizeof(caption), adata->alert->title);
                } else if (adata->alert->title &&
                           wcscmp(caption, adata->alert->title)) {
                    common_caption = FALSE;
                }
            }

            if (common_type) {
                if (ctype == KHUI_ALERTTYPE_NONE)
                    ctype = adata->alert->alert_type;
                else if (ctype != adata->alert->alert_type)
                    common_type = FALSE;
            }

            khui_alert_unlock(adata->alert);
        }

        adata = QNEXT(adata);
    }

    /* just in case someone changes d->caption to a pointer from an
       array */
#ifdef DEBUG
    assert(sizeof(d->caption) > sizeof(wchar_t *));
#endif

    if (common_caption && caption[0] != L'\0') {
        StringCbCopy(d->caption, sizeof(d->caption), caption);
    } else if (common_type && ctype != KHUI_ALERTTYPE_NONE) {
        switch(ctype) {
        case KHUI_ALERTTYPE_PLUGIN:
            LoadString(khm_hInstance, IDS_ALERTTYPE_PLUGIN,
                       d->caption, ARRAYLENGTH(d->caption));
            break;

        case KHUI_ALERTTYPE_EXPIRE:
            LoadString(khm_hInstance, IDS_ALERTTYPE_EXPIRE,
                       d->caption, ARRAYLENGTH(d->caption));
            break;

        case KHUI_ALERTTYPE_RENEWFAIL:
            LoadString(khm_hInstance, IDS_ALERTTYPE_RENEWFAIL,
                       d->caption, ARRAYLENGTH(d->caption));
            break;

        case KHUI_ALERTTYPE_ACQUIREFAIL:
            LoadString(khm_hInstance, IDS_ALERTTYPE_ACQUIREFAIL,
                       d->caption, ARRAYLENGTH(d->caption));
            break;

        case KHUI_ALERTTYPE_CHPW:
            LoadString(khm_hInstance, IDS_ALERTTYPE_CHPW,
                       d->caption, ARRAYLENGTH(d->caption));
            break;

        default:
            LoadString(khm_hInstance, IDS_ALERT_DEFAULT,
                       d->caption, ARRAYLENGTH(d->caption));
        }
    } else {
        LoadString(khm_hInstance, IDS_ALERT_DEFAULT,
                   d->caption, ARRAYLENGTH(d->caption));
    }

    SetWindowText(d->hwnd, d->caption);
}

static void
estimate_alerter_wnd_sizes(alerter_wnd_data * d) {
    HDC hdc;
    HFONT hf_old;
    int height = 0;

    alerter_alert_data * adata;

    if (!(d->flags & KHM_ALERTF_CHILD))
        pick_title_for_alerter_window(d);

    hdc = GetDC(d->hwnd);
#ifdef DEBUG
    assert(hdc);
#endif

    if (d->hfont == NULL)
        d->hfont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);

#ifdef DEBUG
    assert(d->hfont);
#endif

    hf_old = SelectFont(hdc, d->hfont);

    adata = QTOP(d);
    while(adata) {
        layout_alert(hdc, d, adata);

        height += adata->r_alert.bottom;

        adata = QNEXT(adata);
    }

    SelectFont(hdc, hf_old);
    ReleaseDC(d->hwnd, hdc);

    d->s_alerts.cx = d->cx_wnd;
    d->s_alerts.cy = height;
}

static void
layout_command_buttons(alerter_wnd_data * d) {

    alerter_alert_data * adata;
    HDWP hdefer;
    int y;

    hdefer = BeginDeferWindowPos(d->n_cmd_buttons + d->n_alerts);

    y = 0;
    adata = QTOP(d);
    while (adata) {
        RECT r;
        int i;

        if (adata->hwnd_progress) {
            if (IsRectEmpty(&adata->r_progress)) {
                DestroyWindow(adata->hwnd_progress);
                adata->hwnd_progress = NULL;
            } else {
                CopyRect(&r, &adata->r_progress);
                OffsetRect(&r, 0, y-d->scroll_top);

                hdefer = DeferWindowPos(hdefer,
                                        adata->hwnd_progress, NULL,
                                        r.left, r.top, 0, 0,
                                        SWP_NOACTIVATE | SWP_NOOWNERZORDER |
                                        SWP_NOZORDER | SWP_NOSIZE);
            }
        }

        if (!adata->has_commands)
            goto done;

        for (i=0; i < adata->n_cmd_buttons; i++) {
            if (IsRectEmpty(&adata->r_buttons[i])) {
                /* the button is no longer needed */
                if (adata->hwnd_buttons[i] != NULL) {
                    DestroyWindow(adata->hwnd_buttons[i]);
                    adata->hwnd_buttons[i] = NULL;
                }

                continue;
            }

            if (adata->hwnd_buttons[i] == NULL) {
                continue;
            }

            CopyRect(&r, &adata->r_buttons[i]);
            OffsetRect(&r, 0, y - d->scroll_top);

            hdefer = DeferWindowPos(hdefer,
                                    adata->hwnd_buttons[i], NULL,
                                    r.left, r.top, 0, 0,
                                    SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER |
                                    SWP_NOSIZE);
        }

    done:
        y += adata->r_alert.bottom;
        adata = QNEXT(adata);
    }

    EndDeferWindowPos(hdefer);
}

static void
setup_alerter_window_controls(alerter_wnd_data * d) {

    RECT r_alerts;
    RECT r_window;
    RECT r_client;
    RECT r_parent;
    HWND hw_parent;
    HWND hw_focus = NULL;
    BOOL close_button = FALSE;
    const RECT *rpos;

    /* estimate_alerter_wnd_sizes() must be called before calling
       this. */
#ifdef DEBUG
    assert(d->s_alerts.cy >= 0);
#endif

    r_alerts.left = 0;
    r_alerts.top = 0;
    r_alerts.right = d->cx_wnd;

    if (d->s_alerts.cy > d->cy_max_wnd) {

        BOOL redraw = FALSE;

        if (d->flags & KHM_ALERTF_CHILD)
            r_alerts.right -= GetSystemMetrics(SM_CXVSCROLL);
        else
            r_alerts.right += GetSystemMetrics(SM_CXVSCROLL);
        r_alerts.bottom = d->cy_max_wnd;

        CopyRect(&r_client, &r_alerts);
        r_client.bottom += d->s_margin.cy + d->s_button.cy + d->s_pad.cy;
        close_button = TRUE;

        if (d->scroll_top > d->s_alerts.cy - d->cy_max_wnd)
            d->scroll_top = d->s_alerts.cy - d->cy_max_wnd;
    } else {
        r_alerts.bottom = d->s_alerts.cy;

        CopyRect(&r_client, &r_alerts);

        if (d->n_alerts == 1) {

            if (!QTOP(d)->has_commands) {
                r_client.bottom += d->s_margin.cy * 2 + d->s_button.cy;
                close_button = TRUE;
            }

        } else {

            r_client.bottom += d->s_margin.cy * 2 + d->s_button.cy;
            close_button = TRUE;
        }

        d->scroll_top = 0;
    }

    if (d->flags & KHM_ALERTF_CHILD)
        rpos = &d->r_client_area;
    else
        rpos = &r_alerts;

    if (d->hw_bin == NULL) {
        d->hw_bin = CreateWindowEx(WS_EX_CONTROLPARENT,
                                   MAKEINTATOM(atom_alert_bin),
                                   L"Alert Container",
                                   WS_CHILD | WS_CLIPCHILDREN |
                                   WS_VISIBLE | WS_VSCROLL,
                                   rpos->left, rpos->top,
                                   rpos->right - rpos->left,
                                   rpos->bottom - rpos->top,
                                   d->hwnd,
                                   (HMENU) IDC_NTF_ALERTBIN,
                                   khm_hInstance,
                                   (LPVOID) d);
    } else if (!(d->flags & KHM_ALERTF_CHILD)) {
        SetWindowPos(d->hw_bin, NULL,
                     rpos->left, rpos->top,
                     rpos->right - rpos->left,
                     rpos->bottom - rpos->top,
                     SWP_NOZORDER | SWP_NOOWNERZORDER);

    }

    {
        SCROLLINFO si;

        ZeroMemory(&si, sizeof(si));
        si.cbSize = sizeof(si);
        si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
        si.nMin = 0;
        si.nMax = d->s_alerts.cy;
        si.nPage = d->cy_max_wnd;
        si.nPos = d->scroll_top;

        SetScrollInfo(d->hw_bin, SB_VERT, &si, TRUE);
    }

    /* create the action buttons */
    {
        alerter_alert_data * adata;
        int y;
        int idx;
        HWND last_window = HWND_TOP;
        int n_buttons = 0;

        idx = 0;
        y = - d->scroll_top;
        adata = QTOP(d);
        while(adata) {
            if (adata->has_commands) {
                int i;
                wchar_t caption[KHUI_MAXCCH_SHORT_DESC];
                RECT r;

                if (adata->hwnd_marker) {
                    DestroyWindow(adata->hwnd_marker);
                    adata->hwnd_marker = NULL;
                }

                khui_alert_lock(adata->alert);

                adata->n_cmd_buttons = adata->alert->n_alert_commands;

                for (i=0; i < adata->alert->n_alert_commands; i++) {

                    n_buttons ++;

                    if (IsRectEmpty(&adata->r_buttons[i])) {
                        /* this button is not necessary */
                        if (adata->hwnd_buttons[i]) {
                            DestroyWindow(adata->hwnd_buttons[i]);
                            adata->hwnd_buttons[i] = NULL;
                        }

                        continue;
                    }

                    if (adata->hwnd_buttons[i] != NULL) {
                        /* already there */
                        CopyRect(&r, &adata->r_buttons[i]);
                        OffsetRect(&r, 0, y);

                        SetWindowPos(adata->hwnd_buttons[i], last_window,
                                     r.left, r.top,
                                     r.right - r.left,
                                     r.bottom - r.top,
                                     SWP_NOACTIVATE | SWP_NOOWNERZORDER |
                                     SWP_SHOWWINDOW);

                        last_window = adata->hwnd_buttons[i];

                        if (hw_focus == NULL)
                            hw_focus = adata->hwnd_buttons[i];

                        continue;
                    }

                    khm_get_action_caption(adata->alert->alert_commands[i],
                                           caption, sizeof(caption));

                    CopyRect(&r, &adata->r_buttons[i]);
                    OffsetRect(&r, 0, y);

                    adata->hwnd_buttons[i] =
                        CreateWindowEx(0,
                                       L"BUTTON",
                                       caption,
                                       WS_CHILD | WS_TABSTOP | BS_NOTIFY,
                                       r.left, r.top,
                                       r.right - r.left,
                                       r.bottom - r.top,
                                       d->hw_bin,
                                       (HMENU) (INT_PTR) IDC_FROM_IDX(idx, i),
                                       khm_hInstance,
                                       NULL);
#ifdef DEBUG
                    assert(adata->hwnd_buttons[i]);
#endif

                    if (d->hfont) {
                        SendMessage(adata->hwnd_buttons[i], WM_SETFONT,
                                    (WPARAM) d->hfont, FALSE);
                    }

                    SetWindowPos(adata->hwnd_buttons[i], last_window,
                                 0, 0, 0, 0,
                                 SWP_NOACTIVATE | SWP_NOOWNERZORDER |
                                 SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

                    last_window = adata->hwnd_buttons[i];

                    if (hw_focus == NULL)
                        hw_focus = adata->hwnd_buttons[i];
                }

                khui_alert_unlock(adata->alert);
            } else {
                int i;

                /* Destroy any buttons that belong to the alert. We
                   might have some left over, if there were command
                   belonging to the alert that were ignored.*/

                for (i=0; i < adata->n_cmd_buttons; i++) {
                    if (adata->hwnd_buttons[i]) {
                        DestroyWindow(adata->hwnd_buttons[i]);
                        adata->hwnd_buttons[i] = NULL;
                    }
                }

                adata->n_cmd_buttons = 0;

                if (adata->hwnd_marker == NULL) {
                    adata->hwnd_marker =
                        CreateWindowEx(0,
                                       L"BUTTON",
                                       L"Marker",
                                       WS_CHILD | WS_TABSTOP | WS_VISIBLE | BS_NOTIFY,
                                       -10, 0,
                                       5, 5,
                                       d->hw_bin,
                                       (HMENU) (INT_PTR) IDC_FROM_IDX(idx, -1),
                                       khm_hInstance,
                                       NULL);
#ifdef DEBUG
                    assert(adata->hwnd_marker);
#endif
                }

                SetWindowPos(adata->hwnd_marker, last_window,
                             0, 0, 0, 0,
                             SWP_NOACTIVATE | SWP_NOOWNERZORDER |
                             SWP_NOMOVE | SWP_NOSIZE);

                last_window = adata->hwnd_marker;

                /* We don't bother setting up navigation if there
                   aren't enough alerts to make it necessary. */

                if (d->s_alerts.cy > d->cy_max_wnd) {
                    EnableWindow(adata->hwnd_marker, TRUE);
                    if (hw_focus == NULL)
                        hw_focus = adata->hwnd_marker;
                } else {
                    EnableWindow(adata->hwnd_marker, FALSE);
                }
            }

            if (IsRectEmpty(&adata->r_progress)) {
                if (adata->hwnd_progress != NULL) {
                    DestroyWindow(adata->hwnd_progress);
                    adata->hwnd_progress = NULL;
                }
            } else {
                if (adata->hwnd_progress == NULL) {
                    adata->hwnd_progress =
                        CreateWindowEx(0,
                                       PROGRESS_CLASS,
                                       L"Progress meter",
                                       WS_CHILD | WS_VISIBLE,
                                       adata->r_progress.left,
                                       adata->r_progress.top,
                                       adata->r_progress.right - adata->r_progress.left,
                                       adata->r_progress.bottom - adata->r_progress.top,
                                       d->hw_bin,
                                       (HMENU) (INT_PTR) IDC_FROM_IDX(idx, -2),
                                       khm_hInstance,
                                       NULL);
                }

                SetWindowPos(adata->hwnd_progress, last_window,
                             0, 0, 0, 0,
                             SWP_NOACTIVATE | SWP_NOOWNERZORDER |
                             SWP_NOMOVE | SWP_NOSIZE);

                last_window = adata->hwnd_progress;

                SendMessage(adata->hwnd_progress, PBM_SETPOS,
                            ((adata->progress_denom == 0)? 0 :
                             (adata->progress_num * 100) / adata->progress_denom),
                            0);
            }

            y += adata->r_alert.bottom;
            adata = QNEXT(adata);
            idx++;
        }

        d->n_cmd_buttons = n_buttons;
    }

    if (close_button && !(d->flags & KHM_ALERTF_CHILD)) {
        if (d->hw_close == NULL) {
            wchar_t caption[256];

            khm_get_action_caption(KHUI_PACTION_CLOSE, caption, sizeof(caption));

            d->hw_close = CreateWindowEx(0,
                                         L"BUTTON",
                                         caption,
                                         WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP | BS_NOTIFY,
                                         0,0,100,100,
                                         d->hwnd,
                                         (HMENU) IDC_NTF_CLOSE,
                                         khm_hInstance,
                                         NULL);

#ifdef DEBUG
            assert(d->hw_close);
            assert(d->hfont);
#endif
            if (d->hfont)
                SendMessage(d->hw_close, WM_SETFONT, (WPARAM) d->hfont, FALSE);
        }

        {
            int x,y,width,height;

            x = d->s_icon.cx + d->s_margin.cx * 2;
            y = r_client.bottom - (d->s_margin.cy + d->s_button.cy);
            width = d->s_button.cx;
            height = d->s_button.cy;

            SetWindowPos(d->hw_close, NULL,
                         x, y, width, height,
                         SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER |
                         SWP_SHOWWINDOW);
        }

        if (hw_focus == NULL || d->n_cmd_buttons == 0)
            hw_focus = d->hw_close;

    } else {
        if (d->hw_close != NULL) {
            DestroyWindow(d->hw_close);
            d->hw_close = NULL;
        }
    }

    if (!(d->flags & KHM_ALERTF_CHILD)) {
        CopyRect(&r_window, &r_client);
        AdjustWindowRectEx(&r_window, ALERT_WINDOW_STYLES,
                           FALSE, ALERT_WINDOW_EX_SYLES);
        OffsetRect(&r_window, -r_window.left, -r_window.top);

        /* center the window above the parent window. */

        hw_parent = GetWindow(d->hwnd, GW_OWNER);
        GetWindowRect(hw_parent, &r_parent);

        {
            int x,y;

            x = (r_parent.left + r_parent.right - (r_window.right - r_window.left)) / 2;
            y = (r_parent.top + r_parent.bottom - (r_window.bottom - r_window.top)) / 2;

            SetWindowPos(d->hwnd,
                         HWND_TOP,
                         x, y,
                         r_window.right - r_window.left,
                         r_window.bottom - r_window.top,
                         SWP_SHOWWINDOW | SWP_NOOWNERZORDER);
        }

        if (hw_focus != NULL)
            PostMessage(d->hwnd, WM_NEXTDLGCTL, (WPARAM) hw_focus, MAKELPARAM(TRUE, 0));
    }
}

static void
scroll_to_position(alerter_wnd_data * d, int new_pos, khm_boolean redraw_scrollbar) {
    int delta;
    SCROLLINFO si;
    HWND hwnd = d->hw_bin;

    if (new_pos < 0)
        new_pos = 0;
    else if (new_pos > d->s_alerts.cy - d->cy_max_wnd)
        new_pos = d->s_alerts.cy - d->cy_max_wnd;

    if (new_pos == d->scroll_top)
        return;

    delta = d->scroll_top - new_pos;

    d->scroll_top -= delta;

    ScrollWindowEx(hwnd, 0, delta,
                   NULL, NULL, NULL, NULL,
                   SW_INVALIDATE | SW_ERASE);

    layout_command_buttons(d);

    ZeroMemory(&si, sizeof(si));

    si.fMask = SIF_POS;
    si.nPos = d->scroll_top;

    SetScrollInfo(hwnd, SB_VERT, &si, redraw_scrollbar);
}

static void
select_alert(alerter_wnd_data * d, int alert) {

    int y;
    RECT old_sel, new_sel;
    alerter_alert_data * adata;
    int idx;

    if (d->n_alerts == 1 ||
        alert < 0 ||
        alert > d->n_alerts)
        return;

    SetRectEmpty(&old_sel);
    SetRectEmpty(&new_sel);
    idx = 0; y = -d->scroll_top;
    adata = QTOP(d);
    while(adata && (idx <= d->c_alert || idx <= alert)) {

        if (idx == d->c_alert) {
            CopyRect(&old_sel, &adata->r_alert);
            OffsetRect(&old_sel, 0, y);
        }

        if (idx == alert) {
            CopyRect(&new_sel, &adata->r_alert);
            OffsetRect(&new_sel, 0, y);
        }

        y += adata->r_alert.bottom;
        idx ++;
        adata = QNEXT(adata);
    }

    d->c_alert = alert;
    if (!IsRectEmpty(&old_sel))
        InvalidateRect(d->hw_bin, &old_sel, TRUE);
    if (!IsRectEmpty(&new_sel))
        InvalidateRect(d->hw_bin, &new_sel, TRUE);
}

static void
ensure_command_is_visible(alerter_wnd_data * d, int id) {
    int alert_idx;
    int y = 0;
    alerter_alert_data * adata;
    int new_pos = 0;

    alert_idx = ALERT_FROM_IDC(id);

#ifdef DEBUG
    assert(alert_idx >= 0 && alert_idx < d->n_alerts);
#endif
    if (alert_idx >= d->n_alerts || alert_idx < 0)
        return;

    adata = QTOP(d);
    while(adata && alert_idx > 0) {
        y += adata->r_alert.bottom;
        alert_idx--;
        adata = QNEXT(adata);
    }

#ifdef DEBUG
    assert(alert_idx == 0);
    assert(adata);
    assert(adata->alert);
#endif
    if (adata == NULL || alert_idx != 0)
        return;

    new_pos = d->scroll_top;
    if (y < d->scroll_top) {
        new_pos = y;
    } else if (y + adata->r_alert.bottom > d->scroll_top + d->cy_max_wnd) {
        new_pos = y + adata->r_alert.bottom - d->cy_max_wnd;
    }

    if (new_pos != d->scroll_top)
        scroll_to_position(d, new_pos, TRUE);

    select_alert(d, ALERT_FROM_IDC(id));
}

static void
handle_mouse_select(alerter_wnd_data * d, int mouse_x, int mouse_y) {
    int y;
    alerter_alert_data * adata;

    y = -d->scroll_top;
    adata = QTOP(d);
    while(adata) {
        if (y <= mouse_y && (y + adata->r_alert.bottom) > mouse_y) {
            HWND hw = NULL;

            if (adata->n_cmd_buttons > 0)
                hw = adata->hwnd_buttons[0];
            else
                hw = adata->hwnd_marker;

            if (hw && !IsWindowEnabled(hw))
                hw = GetNextDlgTabItem(d->hwnd, hw, FALSE);

            if (hw)
                PostMessage(d->hwnd, WM_NEXTDLGCTL, (WPARAM) hw, MAKELPARAM(TRUE, 0));

            return;
        }

        y += adata->r_alert.bottom;
        adata = QNEXT(adata);
    }
}

static void
process_command_button(alerter_wnd_data * d, int id) {
    int alert_idx;
    int cmd_idx;
    khm_int32 flags = 0;
    khm_int32 cmd = 0;
    alerter_alert_data * adata;
    int i;

    alert_idx = ALERT_FROM_IDC(id);
    cmd_idx = BUTTON_FROM_IDC(id);

#ifdef DEBUG
    assert(alert_idx >= 0 && alert_idx < d->n_alerts);
#endif
    if (alert_idx >= d->n_alerts || alert_idx < 0)
        return;

    if (cmd_idx < 0) {
        /* the user selected a marker button.  Nothing to do. */
        return;
    }

    adata = QTOP(d);
    while(adata && alert_idx > 0) {
        alert_idx--;
        adata = QNEXT(adata);
    }

#ifdef DEBUG
    assert(alert_idx == 0);
    assert(adata);
    assert(adata->alert);
#endif
    if (adata == NULL || alert_idx != 0)
        return;

    khui_alert_lock(adata->alert);
#ifdef DEBUG
    assert(cmd_idx >= 0 && cmd_idx < adata->alert->n_alert_commands);
#endif

    if (cmd_idx >= 0 && cmd_idx < adata->alert->n_alert_commands) {
        cmd = adata->alert->alert_commands[cmd_idx];
    }

    flags = adata->alert->flags;

    adata->alert->response = cmd;

    khui_alert_unlock(adata->alert);

    /* if we were supposed to dispatch the command, do so */
    if (cmd != 0 &&
        cmd != KHUI_PACTION_CLOSE &&
        (flags & KHUI_ALERT_FLAG_DISPATCH_CMD)) {
        PostMessage(khm_hwnd_main, WM_COMMAND,
                    MAKEWPARAM(cmd, 0), 0);
    }

    /* if this was the only alert in the alert group and its close
       button was clicked, we close the alert window.  Otherwise, the
       alert window creates its own close button that closes the
       window. */
    if (d->n_alerts == 1) {
        PostMessage(d->hwnd, WM_CLOSE, 0, 0);
    }

    /* While we are at it, we should disable the buttons for this
       alert since we have already dispatched the command for it. */
    if (cmd != 0) {
        HWND hw_focus = GetFocus();
        khm_boolean focus_trapped = FALSE;

        for (i=0; i < adata->n_cmd_buttons; i++) {
            if (adata->hwnd_buttons[i]) {
                if (hw_focus == adata->hwnd_buttons[i])
                    focus_trapped = TRUE;

                EnableWindow(adata->hwnd_buttons[i], FALSE);
            }
        }

        if (focus_trapped) {
            hw_focus = GetNextDlgTabItem(d->hwnd, hw_focus, FALSE);
            if (hw_focus)
                PostMessage(d->hwnd, WM_NEXTDLGCTL, (WPARAM) hw_focus, MAKELPARAM(TRUE,0));
        }
    }
}

static void
destroy_alerter_wnd_data(alerter_wnd_data * d) {
    alerter_alert_data * adata;

    LDELETE(&khui_alert_windows, d);

    QGET(d, &adata);
    while(adata) {

        if (adata->alert) {

            khui_alert_lock(adata->alert);

            adata->alert->displayed = FALSE;

            khui_alert_unlock(adata->alert);

            khui_alert_release(adata->alert);
            adata->alert = NULL;
        }

        PFREE(adata);

        QGET(d, &adata);
    }

    PFREE(d);
}

/* both ref and to_add must be locked and held */
static khm_boolean
alert_can_consolidate(khui_alert * ref,
                      khui_alert * to_add,
                      alert_list * alist) {

    /* first check if we can add anything */
    if (alist->n_alerts == ARRAYLENGTH(alist->alerts))
        return FALSE;

#ifdef DEBUG
    assert(to_add != NULL);
#endif

    if (ref == NULL) {
        /* we are testing whether to_add should be added to the alist
           on its own. */
        if ((to_add->flags & KHUI_ALERT_FLAG_DISPLAY_BALLOON) &&
            !(to_add->flags & KHUI_ALERT_FLAG_DISPLAY_WINDOW)) {
            /* already displayed */
            return FALSE;
        }

        if ((to_add->flags & (KHUI_ALERT_FLAG_REQUEST_BALLOON |
                              KHUI_ALERT_FLAG_REQUEST_WINDOW)) == KHUI_ALERT_FLAG_REQUEST_BALLOON) {
            /* needs to be shown in a balloon */
            return FALSE;
        }

        return TRUE;
    }

    /* if the ref or to_add are marked for modal, then we can't
       consolidate them */
    if ((ref->flags & KHUI_ALERT_FLAG_MODAL) ||
        (to_add->flags & KHUI_ALERT_FLAG_MODAL))
        return FALSE;

    /* also, if either of them have requested to be exclusively shown
       in a balloon, then we can't consolidate them. */
    if (((ref->flags & (KHUI_ALERT_FLAG_REQUEST_BALLOON |
                        KHUI_ALERT_FLAG_REQUEST_WINDOW)) == KHUI_ALERT_FLAG_REQUEST_BALLOON)

        ||

        ((to_add->flags & (KHUI_ALERT_FLAG_REQUEST_BALLOON |
                           KHUI_ALERT_FLAG_REQUEST_WINDOW)) == KHUI_ALERT_FLAG_REQUEST_BALLOON))
        return FALSE;

    /* for now, all we check if whether they are of the same type. */
    if (ref->alert_type != KHUI_ALERTTYPE_NONE &&
        ref->alert_type == to_add->alert_type)
        return TRUE;
    else
        return FALSE;
}

/* both a1 and a2 must be locked */
static khm_boolean
alert_is_equal(khui_alert * a1, khui_alert * a2) {
    khm_int32 i;

    if ((a1->severity != a2->severity) ||
        (a1->n_alert_commands != a2->n_alert_commands) ||
        (a1->title && (!a2->title || wcscmp(a1->title, a2->title))) ||
        (!a1->title && a2->title) ||
        (a1->message && (!a2->message || wcscmp(a1->message, a2->message))) ||
        (!a1->message && a2->message) ||
        (a1->suggestion && (!a2->suggestion || wcscmp(a1->suggestion, a2->suggestion))) ||
        (!a1->suggestion && a2->suggestion)) {

        return FALSE;

    }

    for (i=0; i < a1->n_alert_commands; i++) {
        if (a1->alert_commands[i] != a2->alert_commands[i])
            return FALSE;
    }

    return TRUE;
}

/* the return value is the number of alerts added to alist */
static khm_int32
alert_consolidate(alert_list * alist,
                  khui_alert * alert,
                  khm_boolean add_from_queue) {

    khui_alert * listtop;
    int queue_size = 0;
    int i;
    khm_int32 n_added = 0;

#ifdef DEBUG
    assert(alist);
#endif

    if (alist->n_alerts == ARRAYLENGTH(alist->alerts)) {
        /* can't add anything */

        return 0;
    }

    /* if the list is empty, we just add one alert */
    if (alist->n_alerts == 0) {

        if (alert) {
            khui_alert_lock(alert);
            if (alert_can_consolidate(NULL, alert, alist)) {
                alert_list_add_alert(alist, alert);
                n_added ++;
                alert = NULL;
            }
            khui_alert_unlock(alert);
        }

        if (n_added == 0 && add_from_queue) {
            khui_alert * q;
            int i;

            queue_size = alert_queue_get_size();
            for (i=0; i < queue_size && n_added == 0; i++) {
                q = alert_queue_get_alert_by_pos(i);
                if (q) {
                    khui_alert_lock(q);
                    if (alert_can_consolidate(NULL, q, alist)) {
                        alert_list_add_alert(alist, q);
                        n_added++;
                        alert_queue_delete_alert(q);
                    }
                    khui_alert_unlock(q);
                    khui_alert_release(q);
                }
            }
        }

        if (n_added == 0) {
            /* nothing to add */
            return 0;
        }
    }

    /* at this point, the alert list is not empty */
#ifdef DEBUG
    assert(alist->n_alerts != 0);
    assert(alist->alerts[0]);
#endif

    listtop = alist->alerts[0];
    khui_alert_hold(listtop);
    khui_alert_lock(listtop);

    queue_size = alert_queue_get_size();

    if (alert) {
        khui_alert_lock(alert);
        if (alert_can_consolidate(listtop, alert, alist)) {
            alert_list_add_alert(alist, alert);
            n_added ++;
        }
        khui_alert_unlock(alert);
    }

    if (add_from_queue) {
        for (i=0; i < queue_size; i++) {
            khui_alert * a;

            a = alert_queue_get_alert_by_pos(i);
            if (a == NULL)
                continue;

            khui_alert_lock(a);
            if (alert_can_consolidate(listtop, a, alist)) {
                alert_queue_delete_alert(a);
                alert_list_add_alert(alist, a);
                n_added ++;

                queue_size--;
                i--;
#ifdef DEBUG
                assert(alert_queue_get_size() == queue_size);
#endif
            }
            khui_alert_unlock(a);
            khui_alert_release(a);
        }
    }

    khui_alert_unlock(listtop);
    khui_alert_release(listtop);

    return n_added;
}

static khm_int32
alert_check_consolidate_window(alerter_wnd_data * d, khui_alert * a) {
    alert_list alist;
    alerter_alert_data * adata;
    int n_added;

    alert_list_init(&alist);

    adata = QTOP(d);
    while(adata) {

#ifdef DEBUG
        assert(adata->alert);
#endif
        alert_list_add_alert(&alist, adata->alert);

        adata = QNEXT(adata);
    }

    n_added = alert_consolidate(&alist, a, FALSE);

    alert_list_destroy(&alist);

    return n_added;
}

static khm_int32
alert_show_minimized(khui_alert * a) {
    wchar_t tbuf[64];           /* corresponds to NOTIFYICONDATA::szInfoTitle[] */
    wchar_t mbuf[256];          /* corresponds to NOTIFYICONDATA::szInfo[] */

#ifdef DEBUG
    assert(a);
#endif
    if (a == NULL)
        return KHM_ERROR_INVALID_PARAM;

    khui_alert_lock(a);

    if (a->message == NULL)
        goto done;

    if (a->title == NULL) {
        LoadString(khm_hInstance, IDS_ALERT_DEFAULT,
                   tbuf, ARRAYLENGTH(tbuf));
    } else {
        StringCbCopy(tbuf, sizeof(tbuf), a->title);
    }

    if (FAILED(StringCbCopy(mbuf, sizeof(mbuf), a->message)) ||
        (!(a->flags & KHUI_ALERT_FLAG_DEFACTION) &&
         (a->n_alert_commands > 0 ||
          a->suggestion ||
          (a->flags & KHUI_ALERT_FLAG_VALID_ERROR)))) {
        /* if mbuf wasn't big enough, this should have copied a
           truncated version of it */
        size_t cch_m, cch_p;
        wchar_t postfix[256];

        cch_p = LoadString(khm_hInstance, IDS_ALERT_MOREINFO, postfix,
                           ARRAYLENGTH(postfix));
        cch_p++;                /* account for NULL */

        StringCchLength(mbuf, ARRAYLENGTH(mbuf), &cch_m);
        cch_m = min(cch_m, ARRAYLENGTH(mbuf) - cch_p);

        StringCchCopy(mbuf + cch_m, ARRAYLENGTH(mbuf) - cch_m,
                      postfix);

        a->flags |= KHUI_ALERT_FLAG_REQUEST_WINDOW;
    }

    a->flags |= KHUI_ALERT_FLAG_DISPLAY_BALLOON;

#ifdef DEBUG
    assert(balloon_alert == NULL);
#endif

    if (balloon_alert) {
        khui_alert_lock(balloon_alert);
        balloon_alert->displayed = FALSE;
        khui_alert_unlock(balloon_alert);
        khui_alert_release(balloon_alert);
        balloon_alert = NULL;
    }

    balloon_alert = a;
    khui_alert_hold(a);

    a->displayed = TRUE;

    khm_notify_icon_balloon(a->severity,
                            tbuf,
                            mbuf,
                            NTF_TIMEOUT);

 done:
    khui_alert_unlock(a);

    return KHM_ERROR_SUCCESS;
}

static khm_int32
alert_show_normal(khui_alert * a) {
    wchar_t buf[256];
    wchar_t * title;
    alert_list alist;

    khui_alert_lock(a);

    if(a->title == NULL) {
        LoadString(khm_hInstance, IDS_ALERT_DEFAULT,
                   buf, ARRAYLENGTH(buf));
        title = buf;
    } else
        title = a->title;

    khui_alert_unlock(a);

    alert_list_init(&alist);
    alert_list_set_title(&alist, title);
    alert_list_add_alert(&alist, a);

    alert_show_list(&alist);

    alert_list_destroy(&alist);

    return KHM_ERROR_SUCCESS;
}

static khm_int32
alert_show_list(alert_list * alist) {
    HWND hwa;

    /* we don't need to keep track of the window handle
       because the window procedure adds it to the dialog
       list automatically */

    hwa =
        CreateWindowEx(ALERT_WINDOW_EX_SYLES,
                       MAKEINTATOM(atom_alerter),
                       alist->title,
                       ALERT_WINDOW_STYLES,
                       0, 0, 300, 300, // bogus values
                       khm_hwnd_main,
                       (HMENU) NULL,
                       khm_hInstance,
                       (LPVOID) alist);

    ShowWindow(hwa, SW_SHOW);

    return (hwa != NULL);
}

static khm_int32
alert_show(khui_alert * a) {
    khm_boolean show_normal = FALSE;
    khm_boolean show_mini = FALSE;

    khui_alert_lock(a);

    /* is there an alert already?  If so, we just enqueue the message
       and let it sit. */
    if (ALERT_DISPLAYED() &&
        !(a->flags & KHUI_ALERT_FLAG_MODAL)) {
        khm_int32 rv;
        alerter_wnd_data * wdata;

        khui_alert_unlock(a);

        /* if there are any alerter windows displayed, check if this
           alert can be consolidated with any of them.  If so, we
           should consolidate it.  Otherwise, just enqueue it. */
        for(wdata = khui_alert_windows;
            wdata;
            wdata = LNEXT(wdata)) {
            if (alert_check_consolidate_window(wdata, a)) {

                add_alert_to_wnd_data(wdata, a);
                estimate_alerter_wnd_sizes(wdata);
                setup_alerter_window_controls(wdata);

                return KHM_ERROR_SUCCESS;

            }
        }

        rv = alert_enqueue(a);

        if (KHM_SUCCEEDED(rv))
            return KHM_ERROR_HELD;
        else
            return rv;
    }

    if((a->flags & KHUI_ALERT_FLAG_DISPLAY_WINDOW) ||
       ((a->flags & KHUI_ALERT_FLAG_DISPLAY_BALLOON) &&
        !(a->flags & KHUI_ALERT_FLAG_REQUEST_WINDOW))) {

        /* The alert has already been displayed. */

        show_normal = FALSE;
        show_mini = FALSE;

    } else {

        if(a->err_context != NULL) {
            a->flags |= KHUI_ALERT_FLAG_VALID_ERROR;
        }

        /* depending on the state of the main window, we
           need to either show a window or a balloon */
        if ((a->flags & KHUI_ALERT_FLAG_MODAL) ||
            (khm_is_main_window_active() &&
             !(a->flags & KHUI_ALERT_FLAG_REQUEST_BALLOON)) ||
            (a->flags & KHUI_ALERT_FLAG_REQUEST_WINDOW)) {

            show_normal = TRUE;

        } else {

            show_mini = TRUE;

        }
    }

    khui_alert_unlock(a);

    if (show_normal)
        return alert_show_normal(a);
    else if (show_mini)
        return alert_show_minimized(a);
    else
        return KHM_ERROR_SUCCESS;
}

static void
show_queued_alerts(void) {

    if (!ALERT_DISPLAYED()) {

        /* show next consolidated batch */
        alert_list alist;
        int n;

        alert_list_init(&alist);
        n = alert_consolidate(&alist, NULL, TRUE);

        if (n) {
            if (n == 1) {
                khui_alert_lock(alist.alerts[0]);

                if (alist.alerts[0]->title) {
                    alert_list_set_title(&alist, alist.alerts[0]->title);
                } else {
                    wchar_t title[KHUI_MAXCCH_TITLE];
                    LoadString(khm_hInstance, IDS_ALERT_DEFAULT,
                               title, ARRAYLENGTH(title));
                    alert_list_set_title(&alist, title);
                }

                khui_alert_unlock(alist.alerts[0]);
            } else {
                wchar_t title[KHUI_MAXCCH_TITLE];
                LoadString(khm_hInstance, IDS_ALERT_DEFAULT,
                           title, ARRAYLENGTH(title));
                alert_list_set_title(&alist, title);
            }

            alert_show_list(&alist);
        }

        alert_list_destroy(&alist);

        if (n == 0) {
            khui_alert * a;

            /* no alerts were shown above.  This maybe because none of
               the alerts were consolidatable or they were requested
               to be shown in a balloon.  In this case, we just take
               the first alert from the queue and show it manually. */

            a = alert_queue_get_alert();
            if (a) {
                alert_show(a);
                khui_alert_release(a);
            }
        }

        check_for_queued_alerts();
    }
}


static void
check_for_queued_alerts(void) {
    if (!is_alert_queue_empty()) {
        khui_alert * a;

        a = alert_queue_peek();

        khui_alert_lock(a);

        if (a->title) {
            HICON hi;
            int res;

            if (a->severity == KHERR_ERROR)
                res = OIC_ERROR;
            else if (a->severity == KHERR_WARNING)
                res = OIC_WARNING;
            else
                res = OIC_INFORMATION;

            hi = LoadImage(0, MAKEINTRESOURCE(res),
                           IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                           LR_SHARED);

            khm_statusbar_set_part(KHUI_SBPART_NOTICE,
                                   hi,
                                   a->title);
        } else {
            khm_statusbar_set_part(KHUI_SBPART_NOTICE,
                                   NULL, NULL);
#ifdef DEBUG
            DebugBreak();
#endif
        }

        khui_alert_unlock(a);
        khui_alert_release(a);

    } else {
        khm_statusbar_set_part(KHUI_SBPART_NOTICE,
                               NULL, NULL);
    }
}

static khm_int32
alert_enqueue(khui_alert * a) {
    if (is_alert_queue_full())
        return KHM_ERROR_NO_RESOURCES;

    alert_queue_put_alert(a);
    check_for_queued_alerts();

    return KHM_ERROR_SUCCESS;
}

/* the alerter window is actually a dialog */
static LRESULT CALLBACK
alerter_wnd_proc(HWND hwnd,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
    switch(uMsg) {
    case WM_CREATE:
        {
            LPCREATESTRUCT lpcs;
            alert_list * alist;
            alerter_wnd_data * d;

            lpcs = (LPCREATESTRUCT) lParam;
            alist = (alert_list *) lpcs->lpCreateParams;

            d = create_alerter_wnd_data(hwnd, alist, NULL, 0);

#pragma warning(push)
#pragma warning(disable: 4244)
            SetWindowLongPtr(hwnd, NTF_PARAM, (LONG_PTR) d);
#pragma warning(pop)

            khm_add_dialog(hwnd);
            khm_enter_modal(hwnd);

            estimate_alerter_wnd_sizes(d);
            setup_alerter_window_controls(d);

            if (d->hw_close) {
                SetFocus(d->hw_close);
            }

            return TRUE;
        }
        break; /* not reached */

    case WM_DESTROY:
        {
            alerter_wnd_data * d;

            /* khm_leave_modal() could be here, but instead it is in
               the WM_COMMAND handler.  This is because the modal loop
               has to be exited before DestroyWindow() is issued. */
            //khm_leave_modal();
            khm_del_dialog(hwnd);

            d = (alerter_wnd_data *)(LONG_PTR)
                GetWindowLongPtr(hwnd, NTF_PARAM);

            if (d) {
                SetWindowLongPtr(hwnd, NTF_PARAM, 0);
                SetWindowLongPtr(d->hw_bin, GWLP_USERDATA, 0);
                destroy_alerter_wnd_data(d);
            }

            return TRUE;
        }
        break;

    case WM_COMMAND:
        {
            alerter_wnd_data * d;

            d = (alerter_wnd_data *)(LONG_PTR)
                GetWindowLongPtr(hwnd, NTF_PARAM);

            if(HIWORD(wParam) == BN_CLICKED) {
                if (LOWORD(wParam) == IDC_NTF_CLOSE ||
                    LOWORD(wParam) == KHUI_PACTION_NEXT ||
                    LOWORD(wParam) == IDCANCEL ||
                    LOWORD(wParam) == IDOK) {

                    khm_leave_modal();
                    DestroyWindow(hwnd);
                    return 0;
                } else {
                    process_command_button(d, LOWORD(wParam));
                }
            } 
        }
        break;

    case WM_CLOSE:
        {
            khm_leave_modal();
            DestroyWindow(hwnd);
            return 0;
        }
    }

    /* Since this is a custom built dialog, we use DefDlgProc instead
       of DefWindowProc. */
    return DefDlgProc(hwnd, uMsg, wParam, lParam);
}



static LRESULT CALLBACK
alert_bin_wnd_proc(HWND hwnd,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    BOOL in_printclient = FALSE;

    switch(uMsg) {
    case WM_CREATE:
        {
            LPCREATESTRUCT lpcs;
            alerter_wnd_data * d;

            lpcs = (LPCREATESTRUCT) lParam;
            d = (alerter_wnd_data *) lpcs->lpCreateParams;

#pragma warning(push)
#pragma warning(disable: 4244)
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) d);
#pragma warning(pop)
        }
        return 0;

    case WM_ERASEBKGND:
        /* we erase the background when we are drawing the alerts
           anyway. */
        return 0;

    case WM_PRINTCLIENT:
        in_printclient = TRUE;
        /* fallthrough */
    case WM_PAINT:
        {
            HDC hdc;
            PAINTSTRUCT ps;
            RECT r;
            HFONT hf_old;
            int y;
            alerter_wnd_data * d;
            alerter_alert_data * adata;
            size_t len;
            int idx;
            HWND hwnd_focus;

            d = (alerter_wnd_data *) (LONG_PTR) GetWindowLongPtr(hwnd, GWLP_USERDATA);
#ifdef DEBUG
            assert(d);
#endif
            if (d == NULL)
                break;

            if (in_printclient) {
                hdc = (HDC) wParam;
                ZeroMemory(&ps, sizeof(ps));
            } else {
                hdc = BeginPaint(hwnd, &ps);
            }

#ifdef DEBUG
            assert(hdc);
            assert(d->hfont);
#endif

#ifdef ALERT_STATIC_BACKGROUND
            if (in_printclient || ps.fErase) {
                HBRUSH hb_background;

                hb_background = GetSysColorBrush(COLOR_BTNFACE);

                GetClientRect(hwnd, &r);
                FillRect(hdc, &r, hb_background);
            }
#endif

            hwnd_focus = GetFocus();

            SetBkMode(hdc, TRANSPARENT);

            hf_old = SelectFont(hdc, d->hfont);

            y = -d->scroll_top;
            idx = 0;
            /* go through the alerts and display them */
            adata = QTOP(d);
            while(adata) {
                khui_alert * a;
                khm_boolean we_have_focus;

                we_have_focus = (hwnd_focus &&
                                 (hwnd_focus == adata->hwnd_marker ||
                                  hwnd_focus == d->hw_bin));

#ifndef ALERT_STATIC_BACKGROUND
#define MIX_C(v1, v2, p) ((COLOR16)(((int)v1) * p + (((int) v2) * (256 - p))))
#define ALPHA 50
                if (in_printclient || ps.fErase) {
                    TRIVERTEX v[2];
                    GRADIENT_RECT gr;
                    COLORREF clr;
                    COLORREF clr2;

                    CopyRect(&r, &adata->r_alert);
                    OffsetRect(&r, 0, y);

                    v[0].x = r.left;
                    v[0].y = r.top;
                    v[0].Alpha = 0;

                    v[1].x = r.right;
                    v[1].y = r.bottom;
                    v[1].Alpha = 0;

                    if (idx == d->c_alert) {
                        clr = GetSysColor(COLOR_HOTLIGHT);

                        clr2 = GetSysColor(COLOR_BTNHIGHLIGHT);
                        v[0].Red =   MIX_C(GetRValue(clr), GetRValue(clr2), ALPHA);
                        v[0].Green = MIX_C(GetGValue(clr), GetGValue(clr2), ALPHA);
                        v[0].Blue =  MIX_C(GetBValue(clr), GetBValue(clr2), ALPHA);

                        clr2 = GetSysColor(COLOR_BTNFACE);
                        v[1].Red =   MIX_C(GetRValue(clr), GetRValue(clr2), ALPHA);
                        v[1].Green = MIX_C(GetGValue(clr), GetGValue(clr2), ALPHA);
                        v[1].Blue =  MIX_C(GetBValue(clr), GetBValue(clr2), ALPHA);
                    } else {
                        clr = GetSysColor(COLOR_BTNHIGHLIGHT);
                        v[0].Red =   (COLOR16) ((int)GetRValue(clr)) << 8;
                        v[0].Green = (COLOR16) ((int)GetGValue(clr)) << 8;
                        v[0].Blue =  (COLOR16) ((int)GetBValue(clr)) << 8;

                        clr = GetSysColor(COLOR_BTNFACE);
                        v[1].Red =   (COLOR16) ((int)GetRValue(clr)) << 8;
                        v[1].Green = (COLOR16) ((int)GetGValue(clr)) << 8;
                        v[1].Blue =  (COLOR16) ((int)GetBValue(clr)) << 8;
                    }

                    gr.UpperLeft = 0;
                    gr.LowerRight = 1;
                    GradientFill(hdc, v, 2, &gr, 1, GRADIENT_FILL_RECT_V);

                    if (we_have_focus) {
                        InflateRect(&r, -d->s_margin.cx / 2, -d->s_margin.cy / 2);
                        DrawFocusRect(hdc, &r);
                    }
                }
#undef ALPHA
#undef MIX_C
#endif

                a = adata->alert;
#ifdef DEBUG
                assert(a != NULL);
#endif
                khui_alert_lock(a);

                if (!IsRectEmpty(&adata->r_title)) {

                    CopyRect(&r, &adata->r_title);
                    OffsetRect(&r, 0, y);

                    StringCchLength(a->title, KHUI_MAXCCH_TITLE, &len);

                    DrawEdge(hdc, &r, EDGE_ETCHED, BF_RECT | BF_MIDDLE);

                    InflateRect(&r, -d->s_pad.cx, -d->s_pad.cy);

                    khm_draw_text(hdc, a->title, KHM_FONT_NORMAL, DRAWTEXTOPTIONS, &r);
                }

                {
                    HICON hicon = NULL;
                    int iid = 0;

                    CopyRect(&r, &adata->r_icon);
                    OffsetRect(&r, 0, y);

                    if ((a->alert_type == KHUI_ALERTTYPE_PROGRESSACQ ||
                         a->alert_type == KHUI_ALERTTYPE_PROGRESS) &&
                        a->severity >= KHERR_INFO) {

                        if (adata->alert_ctx_done)
                            iid = IDI_CHECK;
                        else if (!adata->alert_ctx_done)
                            iid = IDI_CLOCK;

                        if (iid != 0)
                            hicon = (HICON) LoadImage(khm_hInstance,
                                                      MAKEINTRESOURCE(iid),
                                                      IMAGE_ICON,
                                                      GetSystemMetrics(SM_CXICON),
                                                      GetSystemMetrics(SM_CYICON),
                                                      LR_SHARED);

                    }

                    if (hicon == NULL) {

                        if(a->severity == KHERR_ERROR)
                            iid = OIC_HAND;
                        else if(a->severity == KHERR_WARNING)
                            iid = OIC_BANG;
                        else
                            iid = OIC_NOTE;

                        hicon = (HICON) LoadImage(NULL,
                                                  MAKEINTRESOURCE(iid),
                                                  IMAGE_ICON,
                                                  GetSystemMetrics(SM_CXICON),
                                                  GetSystemMetrics(SM_CYICON),
                                                  LR_SHARED);
                    }

                    if (hicon)
                        DrawIcon(hdc, r.left, r.top, hicon);
                }

                if (a->message) {

                    CopyRect(&r, &adata->r_message);
                    OffsetRect(&r, 0, y);

                    khm_draw_text(hdc, a->message, KHM_FONT_NORMAL, DRAWTEXTOPTIONS, &r);
                }

                if (a->suggestion) {
                    HICON hicon;
                    SIZE sz;

                    CopyRect(&r, &adata->r_suggestion);
                    OffsetRect(&r, 0, y);

                    DrawEdge(hdc, &r, EDGE_SUNKEN, BF_RECT | BF_MIDDLE);

                    InflateRect(&r, -d->s_pad.cx, -d->s_pad.cy);

                    sz.cx = GetSystemMetrics(SM_CXSMICON);
                    sz.cy = GetSystemMetrics(SM_CYSMICON);

                    hicon = (HICON) LoadImage(NULL,
                                              MAKEINTRESOURCE(OIC_NOTE),
                                              IMAGE_ICON,
                                              sz.cx,
                                              sz.cy,
                                              LR_SHARED);

                    DrawIconEx(hdc, r.left, r.top, hicon, sz.cx, sz.cy, 0, NULL,
                               DI_NORMAL);

                    r.left += d->s_pad.cx + GetSystemMetrics(SM_CXSMICON);

                    StringCchLength(a->suggestion, KHUI_MAXCCH_SUGGESTION, &len);

                    khm_draw_text(hdc, a->suggestion, KHM_FONT_NORMAL, DRAWTEXTOPTIONS, &r);
                }

                khui_alert_unlock(a);

                y += adata->r_alert.bottom;
                idx++;

                adata = QNEXT(adata);
            }


#ifndef ALERT_STATIC_BACKGROUND
            GetClientRect(hwnd, &r);
            if (y < r.bottom) {
                HBRUSH hb_background;

                hb_background = GetSysColorBrush(COLOR_BTNFACE);

                r.top = y;
                FillRect(hdc, &r, hb_background);
            }
#endif

            SelectFont(hdc, hf_old);

            if (!in_printclient) {
                EndPaint(hwnd, &ps);
            }
        }
        return 0;

    case WM_VSCROLL:
        {
            alerter_wnd_data * d;
            int new_pos = 0;
            SCROLLINFO si;

            d = (alerter_wnd_data *) (LONG_PTR) GetWindowLongPtr(hwnd, GWLP_USERDATA);
#ifdef DEBUG
            assert(d);
#endif
            if (d == NULL)
                break;          /* we can't handle the message */

            ZeroMemory(&si, sizeof(si));

            switch(LOWORD(wParam)) {
            case SB_BOTTOM:
                new_pos = d->s_alerts.cy  - d->cy_max_wnd;
                break;

            case SB_LINEDOWN:
                new_pos = d->scroll_top + SCROLL_LINE_SIZE(d);
                break;

            case SB_LINEUP:
                new_pos = d->scroll_top - SCROLL_LINE_SIZE(d);
                break;

            case SB_PAGEDOWN:
                new_pos = d->scroll_top + d->cy_max_wnd;
                break;

            case SB_PAGEUP:
                new_pos = d->scroll_top - d->cy_max_wnd;
                break;

            case SB_THUMBPOSITION:
            case SB_THUMBTRACK:
                si.fMask = SIF_TRACKPOS;
                GetScrollInfo(hwnd, SB_VERT, &si);
                new_pos = si.nTrackPos;
                break;

            case SB_TOP:
                new_pos = 0;
                break;

            case SB_ENDSCROLL:
                si.fMask = SIF_POS;
                si.nPos = d->scroll_top;
                SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
                return 0;

            default:
                return 0;
            }

            scroll_to_position(d, new_pos, FALSE);
        }
        return 0;

    case WM_COMMAND:
        {
            alerter_wnd_data * d;

            d = (alerter_wnd_data *) (LONG_PTR) GetWindowLongPtr(hwnd, GWLP_USERDATA);
#ifdef DEBUG
            assert(d);
#endif
            if (d == NULL)
                break;

            if (HIWORD(wParam) == BN_CLICKED) {
                process_command_button(d, LOWORD(wParam));
                return 0;
            } else if (HIWORD(wParam) == BN_SETFOCUS ||
                       HIWORD(wParam) == BN_KILLFOCUS) {
                ensure_command_is_visible(d, LOWORD(wParam));
                return 0;
            } 
        }
        break;

    case WM_KILLFOCUS:
    case WM_SETFOCUS:
        {
            alerter_wnd_data * d;

            d = (alerter_wnd_data *) (LONG_PTR) GetWindowLongPtr(hwnd, GWLP_USERDATA);
#ifdef DEBUG
            assert(d);
#endif
            if (d == NULL)
                break;

            select_alert(d, d->c_alert);
        }
        break;

    case KHUI_WM_NC_NOTIFY:
        {
            alerter_wnd_data * d;

            d = (alerter_wnd_data *) (LONG_PTR) GetWindowLongPtr(hwnd, GWLP_USERDATA);
#ifdef DEBUG
            assert(d);
#endif
            if (d == NULL)
                break;

            switch (HIWORD(wParam)) {
            case WMNC_ALERT_REFRESH_VIEW:
                {
                    alerter_alert_data * adata;
                    RECT r;

                    SetRectEmpty(&r);

                    r.top = -d->scroll_top;
                    for (adata = QTOP(d);
                         adata;
                         adata = QNEXT(adata)) {
                        if (adata == (alerter_alert_data *) lParam) {
                            r.bottom = r.top + adata->r_alert.bottom;
                            r.right = adata->r_alert.right;
                            r.left = adata->r_alert.left;
                            break;
                        }
                        r.top += adata->r_alert.bottom;
                    }

                    if (adata == NULL) {
                        /* The target alert was not found. */
#ifdef DEBUG
                        assert(FALSE);
#endif
                        break;
                    }

                    if (adata->hwnd_progress != NULL) {
                        SendMessage(adata->hwnd_progress, PBM_SETPOS,
                                    ((adata->progress_denom == 0)? 0 :
                                     (adata->progress_num * 100) / adata->progress_denom),
                                    0);
                    }

                    InvalidateRect(hwnd, &r, TRUE);
                }
                break;

            case WMNC_ALERT_RECALC_LAYOUT:
                {
                    /* Recalculate everything */

                    estimate_alerter_wnd_sizes(d);
                    setup_alerter_window_controls(d);
                    layout_command_buttons(d);
                    InvalidateRect(hwnd, NULL, TRUE);
                }
                break;

            case WMNC_ALERT_NEW_CTX:
                {

                    kherr_context * child;
                    khui_alert * a = NULL;

                    child = (kherr_context *) lParam;

                    /* Add this context to the monitor list */

                    khui_alert_create_empty(&a);
                    khui_alert_set_type(a, KHUI_ALERTTYPE_PROGRESSACQ);
                    khui_alert_monitor_progress(a, child,
                                                KHUI_AMP_SHOW_EVT_ERR |
                                                KHUI_AMP_SHOW_EVT_WARN);

                    khui_alert_add_to_container(hwnd, a);

                    khui_alert_release(a);
                    kherr_release_context(child);

                    InvalidateRect(hwnd, NULL, TRUE);
                }
                break;
            }
        }
        break;

    case WM_LBUTTONUP:
        {
            alerter_wnd_data * d;
            int x,y;

            d = (alerter_wnd_data *) (LONG_PTR) GetWindowLongPtr(hwnd, GWLP_USERDATA);
#ifdef DEBUG
            assert(d);
#endif
            if (d == NULL)
                break;

            x = GET_X_LPARAM(lParam);
            y = GET_Y_LPARAM(lParam);

            handle_mouse_select(d, x, y);
        }
        break;

    case WM_SIZE:
        {
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;

    case WM_DESTROY:
        {
            /* nothing needs to be done here unless this was a child
               window */
            alerter_wnd_data * d;

            d = (alerter_wnd_data *) (LONG_PTR) GetWindowLongPtr(hwnd, GWLP_USERDATA);

            if (d != NULL && (d->flags & KHM_ALERTF_CHILD)) {
                destroy_alerter_wnd_data(d);
            }

            SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        }
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

struct alert_monitor_data {
    khm_int32 magic;
#define ALERT_MONITOR_DATA_MAGIC 0x2e5a2f12
    khui_alert * alert;
};

static void KHMAPI
alert_ctx_monitor_handler(enum kherr_ctx_event e,
                          kherr_context * c,
                          void * vparam)
{
    struct alert_monitor_data * d;

    d = vparam;

#ifdef DEBUG
    assert(d && d->magic == ALERT_MONITOR_DATA_MAGIC);
#endif

    switch (e) {
    case KHERR_CTX_DESCRIBE:
        {
            kherr_event * ev;

            ev = kherr_get_desc_event(c);
            if (ev) {
                if (ev->short_desc && ev->long_desc) {
                    khui_alert_set_title(d->alert, ev->long_desc);
                    khui_alert_set_message(d->alert, ev->short_desc);
                } else if (ev->long_desc) {
                    khui_alert_set_title(d->alert, ev->long_desc);
                } else if (ev->short_desc) {
                    khui_alert_set_title(d->alert, ev->short_desc);
                }

                if (d->alert->displayed) {
                    PostMessage(d->alert->wnd_data->hw_bin, KHUI_WM_NC_NOTIFY,
                                MAKEWPARAM(0, WMNC_ALERT_RECALC_LAYOUT),
                                (LPARAM) d->alert->adata);
                }
            }
        }
        break;

    case KHERR_CTX_END:
        if (d->alert->displayed) {
            d->alert->adata->alert_ctx_done = TRUE;
            PostMessage(d->alert->wnd_data->hw_bin, KHUI_WM_NC_NOTIFY,
                        MAKEWPARAM(0, WMNC_ALERT_RECALC_LAYOUT),
                        (LPARAM) d->alert->adata);

            kherr_remove_ctx_handler_param(alert_ctx_monitor_handler,
                                           c->serial, d);
            khui_alert_release(d->alert);
            PFREE(d);
        }
        break;

    case KHERR_CTX_EVTCOMMIT:
    case KHERR_CTX_ERROR:
        if (d->alert->displayed) {
            kherr_event * ev;

            ev = kherr_get_last_event(c);

            if (((d->alert->monitor_flags & KHUI_AMP_SHOW_EVT_ERR) &&
                 ev->severity <= KHERR_ERROR) ||
                ((d->alert->monitor_flags & KHUI_AMP_SHOW_EVT_WARN) &&
                 ev->severity <= KHERR_WARNING) ||
                ((d->alert->monitor_flags & KHUI_AMP_SHOW_EVT_INFO) &&
                 ev->severity <= KHERR_INFO) ||
                ((d->alert->monitor_flags & KHUI_AMP_SHOW_EVT_DEBUG) &&
                 ev->severity <= KHERR_DEBUG_3)) {

                khui_alert_set_severity(d->alert, ev->severity);

                if (!(ev->flags & KHERR_RF_STR_RESOLVED))
                    kherr_evaluate_event(ev);

                if (ev->long_desc) {
                    khui_alert_set_message(d->alert, ev->long_desc);
                } else if (ev->short_desc) {
                    khui_alert_set_message(d->alert, ev->short_desc);
                }

                PostMessage(d->alert->wnd_data->hw_bin, KHUI_WM_NC_NOTIFY,
                            MAKEWPARAM(0, WMNC_ALERT_RECALC_LAYOUT),
                            (LPARAM) d->alert->adata);
            }
        }
        break;

    case KHERR_CTX_NEWCHILD:
        if (d->alert->displayed) {
            kherr_context * child;

            child = kherr_get_first_context(c);

            if (child) {
                PostMessage(d->alert->wnd_data->hw_bin, KHUI_WM_NC_NOTIFY,
                            MAKEWPARAM(0, WMNC_ALERT_NEW_CTX),
                            (LPARAM) child);
            }
        }
        break;

    case KHERR_CTX_PROGRESS:
        if (d->alert->displayed) {
            kherr_get_progress_i(c,
                                 &d->alert->adata->progress_num,
                                 &d->alert->adata->progress_denom);

            PostMessage(d->alert->wnd_data->hw_bin, KHUI_WM_NC_NOTIFY,
                        MAKEWPARAM(0, WMNC_ALERT_REFRESH_VIEW),
                        (LPARAM) d->alert->adata);
        }
        break;

    }
}

/*! \brief Starts the process of monitoring a kherr_context via a khui_alert
 */
static void
start_monitoring_alert(khui_alert * alert)
{
    struct alert_monitor_data * d;
    kherr_event * ev;
    khm_int32 evt_flags;

    d = PMALLOC(sizeof(*d));
    memset(d, 0, sizeof(*d));

    d->magic = ALERT_MONITOR_DATA_MAGIC;
    d->alert = alert;
    khui_alert_hold(d->alert);

    evt_flags =
        KHERR_CTX_END |
        KHERR_CTX_DESCRIBE |
        KHERR_CTX_NEWCHILD |
        KHERR_CTX_PROGRESS;

    if (alert->monitor_flags & KHUI_AMP_SHOW_EVT_ERR)
        evt_flags |= KHERR_CTX_ERROR;

    if (alert->monitor_flags & (KHUI_AMP_SHOW_EVT_WARN |
                                KHUI_AMP_SHOW_EVT_INFO |
                                KHUI_AMP_SHOW_EVT_DEBUG))
        evt_flags |= KHERR_CTX_ERROR;

    kherr_add_ctx_handler_param(alert_ctx_monitor_handler,
                                evt_flags,
                                d->alert->err_context->serial, d);

    ev = kherr_get_desc_event(d->alert->err_context);
    if (ev) {
        if (ev->short_desc && ev->long_desc) {
            khui_alert_set_title(alert, ev->long_desc);
            khui_alert_set_message(alert, ev->short_desc);
        } else if (ev->long_desc) {
            khui_alert_set_title(alert, ev->long_desc);
        } else if (ev->short_desc) {
            khui_alert_set_title(alert, ev->short_desc);
        }
    }

    kherr_release_context(d->alert->err_context);
    d->alert->err_context = NULL;
}

/*! \brief Creates an alert container

    This is used to house a collection of alerts that should be
    displayed in a modal manner.  The parent window should be dialog
    box (i.e. respond to dialog messages for setting active and
    default controls etc.)

 */
HWND
khui_alert_create_container(HWND hwnd,
                            const RECT * pos,
                            khm_int32 flags)
{
    alerter_wnd_data * d;

    d = create_alerter_wnd_data(hwnd, NULL, pos,
                                flags | KHM_ALERTF_CHILD | KHM_ALERTF_COMPACT);

    estimate_alerter_wnd_sizes(d);
    setup_alerter_window_controls(d);

    /* The alerter will be destroyed when the alert bin is
       destroyed. */

    return d->hw_bin;
}

/*! \brief Add an alert to an alert container

    The alert container in \a hwnd_container must have been created
    using khui_alert_create_container().  The UI will be updated to
    display the new alert.
 */
void
khui_alert_add_to_container(HWND hwnd_container,
                            khui_alert * a)
{
    alerter_wnd_data * d;

    d = (alerter_wnd_data *) (LONG_PTR) GetWindowLongPtr(hwnd_container, GWLP_USERDATA);

#ifdef DEBUG
    assert(d && d->magic == ALERTER_WND_DATA_MAGIC);
#endif

    add_alert_to_wnd_data(d, a);
    estimate_alerter_wnd_sizes(d);
    setup_alerter_window_controls(d);
}

ATOM khm_register_alerter_wnd_class(void)
{
    WNDCLASSEX wcx;

    ZeroMemory(&wcx, sizeof(wcx));

    wcx.cbSize = sizeof(wcx);
    wcx.style =
        CS_OWNDC |
#if(_WIN32_WINNT >= 0x0501)
        ((IS_COMMCTL6())? CS_DROPSHADOW: 0) |
#endif
        0;
    wcx.lpfnWndProc = alerter_wnd_proc;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = DLGWINDOWEXTRA + sizeof(LONG_PTR);
    wcx.hInstance = khm_hInstance;
    wcx.hIcon = LoadIcon(khm_hInstance, MAKEINTRESOURCE(IDI_MAIN_APP));
    wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcx.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcx.lpszMenuName = NULL;
    wcx.lpszClassName = KHUI_ALERTER_CLASS;
    wcx.hIconSm = NULL;

    atom_alerter = RegisterClassEx(&wcx);

    return atom_alerter;
}

ATOM khm_register_alert_bin_wnd_class(void)
{
    WNDCLASSEX wcx;

    ZeroMemory(&wcx, sizeof(wcx));

    wcx.cbSize = sizeof(wcx);
    wcx.style = CS_OWNDC;

    wcx.lpfnWndProc = alert_bin_wnd_proc;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = sizeof(LONG_PTR);
    wcx.hInstance = khm_hInstance;
    wcx.hIcon = NULL;
    wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcx.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcx.lpszMenuName = NULL;
    wcx.lpszClassName = KHUI_ALERTBIN_CLASS;
    wcx.hIconSm = NULL;

    atom_alert_bin = RegisterClassEx(&wcx);

    return atom_alert_bin;
}

/**********************************************************************
  Notification Icon
***********************************************************************/

#define KHUI_NOTIFY_ICON_ID 0

void khm_notify_icon_add(void) {
    NOTIFYICONDATA ni;
    wchar_t buf[256];

    ZeroMemory(&ni, sizeof(ni));

    ni.cbSize = sizeof(ni);
    ni.hWnd = hwnd_notifier;
    ni.uID = KHUI_NOTIFY_ICON_ID;
    ni.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    ni.hIcon = LoadIcon(khm_hInstance, MAKEINTRESOURCE(iid_normal));
    ni.uCallbackMessage = KHUI_WM_NOTIFIER;
    LoadString(khm_hInstance, IDS_NOTIFY_PREFIX, buf, ARRAYLENGTH(buf));
    StringCbCopy(tip_normal, sizeof(tip_normal), buf);
    LoadString(khm_hInstance, IDS_NOTIFY_READY, buf, ARRAYLENGTH(buf));
    StringCbCat(tip_normal, sizeof(tip_normal), buf);

    StringCbCopy(ni.szTip, sizeof(ni.szTip), tip_normal);

    Shell_NotifyIcon(NIM_ADD, &ni);

    DestroyIcon(ni.hIcon);

    ni.cbSize = sizeof(ni);
    ni.uVersion = NOTIFYICON_VERSION;
    Shell_NotifyIcon(NIM_SETVERSION, &ni);
}

void
khm_notify_icon_balloon(khm_int32 severity,
                         wchar_t * title,
                         wchar_t * msg,
                         khm_int32 timeout) {
    NOTIFYICONDATA ni;
    int iid;

    if (!msg || !title)
        return;

    ZeroMemory(&ni, sizeof(ni));
    ni.cbSize = sizeof(ni);

    if (severity == KHERR_INFO) {
        ni.dwInfoFlags = NIIF_INFO;
        iid = IDI_NOTIFY_INFO;
    } else if (severity == KHERR_WARNING) {
        ni.dwInfoFlags = NIIF_WARNING;
        iid = IDI_NOTIFY_WARN;
    } else if (severity == KHERR_ERROR) {
        ni.dwInfoFlags = NIIF_ERROR;
        iid = IDI_NOTIFY_ERROR;
    } else {
        ni.dwInfoFlags = NIIF_NONE;
        iid = iid_normal;
    }

    ni.hWnd = hwnd_notifier;
    ni.uID = KHUI_NOTIFY_ICON_ID;
    ni.uFlags = NIF_INFO | NIF_ICON;
    ni.hIcon = LoadIcon(khm_hInstance, MAKEINTRESOURCE(iid));

    if (FAILED(StringCbCopy(ni.szInfo, sizeof(ni.szInfo), msg))) {
        /* too long? */
        StringCchCopyN(ni.szInfo, ARRAYLENGTH(ni.szInfo),
                       msg,
                       ARRAYLENGTH(ni.szInfo) - ARRAYLENGTH(ELLIPSIS));
        StringCchCat(ni.szInfo, ARRAYLENGTH(ni.szInfo),
                     ELLIPSIS);
    }

    if (FAILED(StringCbCopy(ni.szInfoTitle, sizeof(ni.szInfoTitle),
                            title))) {
        StringCchCopyN(ni.szInfoTitle, ARRAYLENGTH(ni.szInfoTitle),
                       title,
                       ARRAYLENGTH(ni.szInfoTitle) - ARRAYLENGTH(ELLIPSIS));
        StringCchCat(ni.szInfoTitle, ARRAYLENGTH(ni.szInfoTitle),
                     ELLIPSIS);
    }

    ni.uTimeout = timeout;

    Shell_NotifyIcon(NIM_MODIFY, &ni);

    DestroyIcon(ni.hIcon);
}

void khm_notify_icon_expstate(enum khm_notif_expstate expseverity) {
    int new_iid;

    if (expseverity == KHM_NOTIF_OK)
        new_iid = IDI_APPICON_OK;
    else if (expseverity == KHM_NOTIF_WARN)
        new_iid = IDI_APPICON_WARN;
    else if (expseverity == KHM_NOTIF_EXP)
        new_iid = IDI_APPICON_EXP;
    else
        new_iid = IDI_NOTIFY_NONE;

    if (iid_normal == new_iid)
        return;

    iid_normal = new_iid;

    if (balloon_alert == NULL)
        khm_notify_icon_change(KHERR_NONE);
}

void khm_notify_icon_change(khm_int32 severity) {
    NOTIFYICONDATA ni;
    wchar_t buf[256];
    int iid;

    if (severity == KHERR_INFO)
        iid = IDI_NOTIFY_INFO;
    else if (severity == KHERR_WARNING)
        iid = IDI_NOTIFY_WARN;
    else if (severity == KHERR_ERROR)
        iid = IDI_NOTIFY_ERROR;
    else
        iid = iid_normal;

    ZeroMemory(&ni, sizeof(ni));

    ni.cbSize = sizeof(ni);
    ni.hWnd = hwnd_notifier;
    ni.uID = KHUI_NOTIFY_ICON_ID;
    ni.uFlags = NIF_ICON | NIF_TIP;
    ni.hIcon = LoadIcon(khm_hInstance, MAKEINTRESOURCE(iid));

    if (severity == KHERR_NONE) {
        StringCbCopy(ni.szTip, sizeof(ni.szTip), tip_normal);
    } else {
        LoadString(khm_hInstance, IDS_NOTIFY_PREFIX, buf, ARRAYLENGTH(buf));
        StringCbCopy(ni.szTip, sizeof(ni.szTip), buf);
        LoadString(khm_hInstance, IDS_NOTIFY_ATTENTION, buf, ARRAYLENGTH(buf));
        StringCbCat(ni.szTip, sizeof(ni.szTip), buf);
    }

    Shell_NotifyIcon(NIM_MODIFY, &ni);

    DestroyIcon(ni.hIcon);

    notifier_severity = severity;
}

void khm_notify_icon_tooltip(wchar_t * s) {
    wchar_t buf[256];

    LoadString(khm_hInstance, IDS_NOTIFY_PREFIX, buf, ARRAYLENGTH(buf));
    StringCbCat(buf, sizeof(buf), s);

    StringCbCopy(tip_normal, sizeof(tip_normal), buf);

    if (notifier_severity == KHERR_NONE) {
        NOTIFYICONDATA ni;

        ZeroMemory(&ni, sizeof(ni));

        ni.cbSize = sizeof(ni);
        ni.hWnd = hwnd_notifier;
        ni.uID = KHUI_NOTIFY_ICON_ID;
        ni.uFlags = NIF_TIP;

        StringCbCopy(ni.szTip, sizeof(ni.szTip), tip_normal);

        Shell_NotifyIcon(NIM_MODIFY, &ni);
    }
}

void khm_notify_icon_remove(void) {
    NOTIFYICONDATA ni;

    ZeroMemory(&ni, sizeof(ni));

    ni.cbSize = sizeof(ni);
    ni.hWnd = hwnd_notifier;
    ni.uID = KHUI_NOTIFY_ICON_ID;

    Shell_NotifyIcon(NIM_DELETE, &ni);
}

khm_int32
khm_get_default_notifier_action(void) {
    khm_int32 def_cmd = KHUI_ACTION_OPEN_APP;
    khm_handle csp_cw = NULL;
    khm_size i;

    if (KHM_FAILED(khc_open_space(NULL, L"CredWindow", KHM_PERM_READ,
                                  &csp_cw)))
        def_cmd;

    khc_read_int32(csp_cw, L"NotificationAction", &def_cmd);

    khc_close_space(csp_cw);

    for (i=0; i < n_khm_notifier_actions; i++) {
        if (khm_notifier_actions[i] == def_cmd)
            break;
    }

    if (i < n_khm_notifier_actions)
        return def_cmd;
    else
        return KHUI_ACTION_OPEN_APP;
}

void khm_notify_icon_activate(void) {
    /* if there are any notifications waiting to be shown and there
       are no alerts already being shown, we show them.  Otherwise we
       execute the default action. */

    khm_notify_icon_change(KHERR_NONE);

    if (balloon_alert != NULL && khui_alert_windows == NULL) {

        khui_alert * a;
        khm_boolean alert_done = FALSE;

        a = balloon_alert;
        balloon_alert = NULL;

        khui_alert_lock(a);

        a->displayed = FALSE;

        if ((a->flags & KHUI_ALERT_FLAG_DEFACTION) &&
            (a->n_alert_commands > 0)) {

            PostMessage(khm_hwnd_main, WM_COMMAND,
                        MAKEWPARAM(a->alert_commands[0],
                                   0),
                        0);
            alert_done = TRUE;

        } else if (a->flags & KHUI_ALERT_FLAG_REQUEST_WINDOW) {

            alert_show_normal(a);
            alert_done = TRUE;

        }
        khui_alert_unlock(a);
        khui_alert_release(a);

        if (alert_done)
            return;
    }

    if (!is_alert_queue_empty() && !ALERT_DISPLAYED()) {

        khm_show_main_window();
        show_queued_alerts();

        return;
    }


    /* if none of the above applied, then we perform the default
       action for the notification icon. */
    {
        khm_int32 cmd = 0;

        cmd = khm_get_default_notifier_action();

        if (cmd == KHUI_ACTION_OPEN_APP) {
            if (khm_is_main_window_visible()) {
                khm_hide_main_window();
            } else {
                khm_show_main_window();
            }
        } else {
            khui_action_trigger(cmd, NULL);
        }

        check_for_queued_alerts();
    }
}

/*********************************************************************
  Initialization
**********************************************************************/

void khm_init_notifier(void)
{
    if(!khm_register_notifier_wnd_class())
        return;

    if(!khm_register_alerter_wnd_class())
        return;

    if(!khm_register_alert_bin_wnd_class())
        return;

    hwnd_notifier = CreateWindowEx(0,
                                   MAKEINTATOM(atom_notifier),
                                   KHUI_NOTIFIER_WINDOW,
                                   0,
                                   0,0,0,0,
                                   HWND_MESSAGE,
                                   NULL,
                                   khm_hInstance,
                                   NULL);

    if(hwnd_notifier != NULL) {
        kmq_subscribe_hwnd(KMSG_ALERT, hwnd_notifier);
        kmq_subscribe_hwnd(KMSG_CRED, hwnd_notifier);
        notifier_ready = TRUE;

        khm_notify_icon_add();
    } else {
#ifdef DEBUG
        assert(hwnd_notifier != NULL);
#endif
    }
    khm_timer_init();

    khm_addr_change_notifier_init();
}

void khm_exit_notifier(void)
{
    khm_addr_change_notifier_exit();

    khm_timer_exit();

    if(hwnd_notifier != NULL) {
        khm_notify_icon_remove();
        kmq_unsubscribe_hwnd(KMSG_ALERT, hwnd_notifier);
        kmq_unsubscribe_hwnd(KMSG_CRED, hwnd_notifier);
        DestroyWindow(hwnd_notifier);
        hwnd_notifier = NULL;
    }

    if(atom_notifier != 0) {
        UnregisterClass(MAKEINTATOM(atom_notifier), khm_hInstance);
        atom_notifier = 0;
    }

    if(atom_alerter != 0) {
        UnregisterClass(MAKEINTATOM(atom_alerter), khm_hInstance);
        atom_alerter = 0;
    }

    if(atom_alert_bin != 0) {
        UnregisterClass(MAKEINTATOM(atom_alert_bin), khm_hInstance);
        atom_alert_bin = 0;
    }

    notifier_ready = FALSE;
}

