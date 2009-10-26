/*
 * Copyright (c) 2009 Secure Endpoints Inc.
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

#define OEMRESOURCE

#include "khmapp.h"
#include "Notifier.hpp"

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

#define NTF_TIMEOUT 20000

#define ALERT_RECALC_LAYOUT (WM_USER + 256)
#define ALERT_REFRESH_VIEW  (WM_USER + 257)
#define ALERT_NEW_CTX       (WM_USER + 258)
#define ALERT_ADD_ALERT     (WM_USER + 259)

namespace nim {

    Notifier::Notifier() {
        m_icon = PNEW NotificationIcon(this);
    }

    Notifier::~Notifier() {
        delete m_icon;
    }

    bool Notifier::IsAlertDisplayed() const {
        return !m_balloon_alert.IsNull() || m_alert_windows.size() != 0;
    }

    khm_int32 Notifier::ShowAlertNormal(Alert& a) {
        AlertWindow * w = PNEW AlertWindow();
        AutoRef<AlertWindow> r_w(w, RefCount::TakeOwnership);
        bool is_modal = false;

        if (!w->Add(a))
            return KHM_ERROR_INVALID_PARAM;

        w->SetOwnerList(&m_alert_windows);
        {
            AutoLock<Alert> a_lock(&a);
            is_modal = !!(a->flags & KHUI_ALERT_FLAG_MODAL);
        }
        if (is_modal) {
            w->DoModal(khm_hwnd_main);
        } else {
            w->Create(khm_hwnd_main);
            w->ShowWindow();
        }

        UpdateAlertState();

        return KHM_ERROR_SUCCESS;
    }

    khm_int32 Notifier::ShowAlertMini(Alert& a) {
        wchar_t tbuf[64];           /* corresponds to NOTIFYICONDATA::szInfoTitle[] */
        wchar_t mbuf[256];          /* corresponds to NOTIFYICONDATA::szInfo[] */

        AutoLock<Alert> a_lock(&a);

        if (a->message == NULL)
            return KHM_ERROR_INVALID_PARAM;

        if (a->title == NULL) {
            LoadStringResource(tbuf, IDS_ALERT_DEFAULT);
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

            cch_p = LoadStringResource(postfix, IDS_ALERT_MOREINFO);
            cch_p++;                /* account for NULL */

            StringCchLength(mbuf, ARRAYLENGTH(mbuf), &cch_m);
            cch_m = min(cch_m, ARRAYLENGTH(mbuf) - cch_p);

            StringCchCopy(mbuf + cch_m, ARRAYLENGTH(mbuf) - cch_m,
                          postfix);

            a->flags |= KHUI_ALERT_FLAG_REQUEST_WINDOW;
        }

        a->flags |= KHUI_ALERT_FLAG_DISPLAY_BALLOON;

        assert(m_balloon_alert.IsNull());

        if (!m_balloon_alert.IsNull()) {
            AutoLock<Alert> b_lock(&m_balloon_alert);
            m_balloon_alert->displayed = FALSE;
        }

        m_balloon_alert = a;
        a->displayed = TRUE;

        m_icon->ShowBalloon(a->severity, tbuf, mbuf, NTF_TIMEOUT);

        UpdateAlertState();

        return KHM_ERROR_SUCCESS;
    }

    khm_int32 Notifier::ShowAlert(Alert& a, bool ok_to_enqueue) {
        enum what_to_do {
            Nothing,
            Show_in_balloon,
            Show_in_window,
            Consolidate_with_shown_window,
        } to_do = Nothing;

        do {
            AutoLock<Alert> a_lock(&a);

            if ((a->flags & KHUI_ALERT_FLAG_DISPLAY_WINDOW) ||

                ((a->flags & KHUI_ALERT_FLAG_DISPLAY_BALLOON) &&
                 !(a->flags & KHUI_ALERT_FLAG_REQUEST_WINDOW))

                ) {

                // The alert has already been displayed.
                return KHM_ERROR_DUPLICATE;

            }

            if (IsAlertDisplayed()) {

                if (a->flags & KHUI_ALERT_FLAG_MODAL) {
                    to_do = Show_in_window;
                } else {
                    to_do = Consolidate_with_shown_window;
                }

            } else if ((khm_is_main_window_active() &&
                        !(a->flags & KHUI_ALERT_FLAG_REQUEST_BALLOON)) ||

                       (a->flags & KHUI_ALERT_FLAG_REQUEST_WINDOW)) {

                to_do = Show_in_window;

            } else {

                to_do = Show_in_balloon;

            }

        } while (false);


        switch (to_do) {

        case Consolidate_with_shown_window:

            for (AlertWindowList::iterator i = m_alert_windows.begin();
                 i != m_alert_windows.end();
                 ++i) {

                if ((*i)->Add(a))
                    return KHM_ERROR_SUCCESS;

            }

            if (ok_to_enqueue)
                EnqueueAlert(a);
            return KHM_ERROR_HELD;

        case Show_in_window:
            return ShowAlertNormal(a);

        case Show_in_balloon:
            return ShowAlertMini(a);
        }

        return KHM_ERROR_UNKNOWN;
    }

    void Notifier::EnqueueAlert(Alert& a) {
        m_alert_list.push_back (a);
        UpdateAlertState();
    }

    void Notifier::ShowQueuedAlerts() {
        if (IsAlertDisplayed())
            return;

        if (m_alert_list.empty())
            return;

        AlertWindow * w = PNEW AlertWindow();

        for (AlertList::iterator i = m_alert_list.begin();
             i != m_alert_list.end();) {

            if (w->Add(*i)) {
                i = m_alert_list.erase(i);
            } else {
                ++i;
            }
        }

        if (w->IsEmpty()) {
            for (AlertList::iterator i = m_alert_list.begin();
                 i != m_alert_list.end(); ++i) {

                if (KHM_SUCCEEDED(ShowAlert(*i, false /* Don't enqueue */))) {
                    m_alert_list.erase(i);
                    break;
                }
            }
        } else {
            w->SetOwnerList(&m_alert_windows);
            w->Create(khm_hwnd_main);
            w->ShowWindow();
        }

        w->Release();        // ControlWindows are self-disposing.
        // No need to free.
        UpdateAlertState();
    }

    void Notifier::UpdateAlertState() {

        if (m_alert_list.empty()) {

            m_icon->SetSeverity(KHERR_NONE);
            khm_statusbar_set_part(KHUI_SBPART_NOTICE, NULL, NULL);

        } else {

            Alert &a = m_alert_list.front();

            {
                AutoLock<Alert> a_lock(&a);

                HICON hi;
                int res;

                if (a->severity == KHERR_ERROR)
                    res = OIC_ERROR;
                else if (a->severity == KHERR_WARNING)
                    res = OIC_WARNING;
                else
                    res = OIC_INFORMATION;

                hi = (HICON)
                    LoadImage(0, MAKEINTRESOURCE(res),
                              IMAGE_ICON,
                              GetSystemMetrics(SM_CXSMICON),
                              GetSystemMetrics(SM_CYSMICON),
                              LR_SHARED);

                khm_statusbar_set_part(KHUI_SBPART_NOTICE,
                                       hi,
                                       a->title);
                m_icon->SetSeverity(a->severity);
            }
        }
    }

    void Notifier::BeginMonitoringAlert(Alert& alert) {
        AutoLock<Alert> a_lock(&alert);

        if (alert->displayed) {
            for (AlertWindowList::iterator i = m_alert_windows.begin();
                 i != m_alert_windows.end(); ++i) {
                if ((*i)->BeginMonitoringAlert(alert))
                    return;
            }
        }
    }

    khm_int32 Notifier::GetDefaultNotifierAction() {
        khm_int32 def_cmd = KHUI_ACTION_OPEN_APP;

        {
            ConfigSpace cw(L"CredWindow", KHM_PERM_READ);
            def_cmd = cw.GetInt32(L"NotificationAction", def_cmd);
        }

        for (khm_size i = 0; i < n_khm_notifier_actions; ++i) {
            if (khm_notifier_actions[i] == def_cmd)
                return def_cmd;
        }
        return KHUI_ACTION_OPEN_APP;
    }

    void Notifier::NotificationIconActivate() {
        /* if there are any notifications waiting to be shown and
           there are no alerts already being shown, we show them.
           Otherwise we execute the default action. */

        if (!m_balloon_alert.IsNull() &&
            m_alert_windows.size() == 0) {

            Alert a(m_balloon_alert);
            khm_boolean alert_done = FALSE;

            m_balloon_alert = (khui_alert *) NULL;

            {
                AutoLock<Alert> a_lock(&a);

                a->displayed = FALSE;

                if ((a->flags & KHUI_ALERT_FLAG_DEFACTION) &&
                    (a->n_alert_commands > 0)) {

                    ::PostMessage(khm_hwnd_main, WM_COMMAND,
                                  MAKEWPARAM(a->alert_commands[0], 0),
                                  0);
                    UpdateAlertState();
                    alert_done = TRUE;

                } else if (a->flags & KHUI_ALERT_FLAG_REQUEST_WINDOW) {

                    ShowAlertNormal(a);
                    alert_done = TRUE;

                }
            }

            if (alert_done) {
                return;
            }
        }

        if (m_alert_list.size() != 0 && !IsAlertDisplayed()) {

            khm_show_main_window();
            ShowQueuedAlerts();

            return;
        }

        /* if none of the above applied, then we perform the default
           action for the notification icon. */
        {
            khm_int32 cmd = 0;

            cmd = GetDefaultNotifierAction();

            if (cmd == KHUI_ACTION_OPEN_APP) {
                if (khm_is_main_window_visible()) {
                    khm_hide_main_window();
                } else {
                    khm_show_main_window();
                }
            } else {
                khui_action_trigger(cmd, NULL);
            }

            UpdateAlertState();
        }
    }

    khm_int32 Notifier::OnMsgAlertShow(khui_alert * _a) {
        Alert a(_a, true);
        assert(!a.IsNull());
        return ShowAlert(a);
    }

    khm_int32 Notifier::OnMsgAlertQueue(khui_alert * _a) {
        Alert a(_a, true);
        assert(!a.IsNull());
        EnqueueAlert(a);
        return KHM_ERROR_SUCCESS;
    }

    khm_int32 Notifier::OnMsgAlertCheckQueue() {
        UpdateAlertState();
        return KHM_ERROR_SUCCESS;
    }

    khm_int32 Notifier::OnMsgAlertShowQueued() {
        ShowQueuedAlerts();
        return KHM_ERROR_SUCCESS;
    }

    khm_int32 Notifier::OnMsgAlertShowModal(khui_alert * _a) {
        Alert a(_a, true);
        {
            AutoLock<Alert> a_lock(&a);
            a->flags |= KHUI_ALERT_FLAG_MODAL;
        }
        return ShowAlert(a);
    }

    khm_int32 Notifier::OnMsgAlertMonitorProgress(khui_alert * _a) {
        Alert a(_a, true);
        BeginMonitoringAlert(a);
        return KHM_ERROR_SUCCESS;
    }

    khm_int32 Notifier::OnMsgCredRootDelta() {
        KillTimer (hwnd, KHUI_REFRESH_TIMER_ID);
        SetTimer (hwnd, KHUI_REFRESH_TIMER_ID,
                  KHUI_REFRESH_TIMEOUT,
                  NULL);
        return KHM_ERROR_SUCCESS;
    }

    void Notifier::OnNotifyContextMenu() {
        POINT pt;
        int menu_id;
        khui_menu_def * mdef;
        khui_action_ref * act = NULL;
        khm_size i, n;
        khm_int32 def_cmd;

        /* before we show the context menu, we need to make sure that
           the default action for the notification icon is present in
           the menu and that it is marked as the default. */

        def_cmd = GetDefaultNotifierAction();

        if (khm_is_main_window_visible()) {
            menu_id = KHUI_MENU_ICO_CTX_NORMAL;

            if (def_cmd == KHUI_ACTION_OPEN_APP)
                def_cmd = KHUI_ACTION_CLOSE_APP;
        } else {
            menu_id = KHUI_MENU_ICO_CTX_MIN;
        }

        mdef = khui_find_menu(menu_id);

        assert(mdef);

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
            /* the default action was not found on the context menu */
            assert(FALSE);
            khui_menu_insert_action(mdef, 0, def_cmd, KHUI_ACTIONREF_DEFAULT);
        }

        SetForegroundWindow(khm_hwnd_main);

        GetCursorPos(&pt);
        khm_menu_show_panel(menu_id, pt.x, pt.y);

        ::PostMessage(khm_hwnd_main, WM_NULL, 0, 0);
    }

    void Notifier::OnNotifySelect() {
        NotificationIconActivate();
    }

    void Notifier::OnNotifyBalloonUserClick() {
        if (!m_balloon_alert.IsNull()) {
            Alert a(m_balloon_alert);
            bool show_normal = false;

            m_balloon_alert = (khui_alert *) NULL;

            {
                AutoLock<Alert> a_lock(&a);

                a->displayed = FALSE;

                if ((a->flags & KHUI_ALERT_FLAG_DEFACTION) &&
                    !(a->flags & KHUI_ALERT_FLAG_REQUEST_WINDOW) &&
                    a->n_alert_commands > 0) {
                    ::PostMessage(khm_hwnd_main, WM_COMMAND,
                                  MAKEWPARAM(a->alert_commands[0], 0),
                                  0);
                } else if (a->flags &
                           KHUI_ALERT_FLAG_REQUEST_WINDOW) {
                    show_normal = true;
                }
            }

            if (show_normal) {
                khm_show_main_window();
                ShowAlertNormal(a);
            }

        } else {
            assert(FALSE);
        }
    }

    void Notifier::OnNotifyBalloonHide() {
        m_icon->SetSeverity(KHERR_NONE);

        if (!m_balloon_alert.IsNull()) {
            AutoLock<Alert> a_lock(&m_balloon_alert);
            m_balloon_alert->displayed = FALSE;
        }
        m_balloon_alert = (khui_alert *) NULL;
    }

    void Notifier::OnTriggerTimer() {
        KillTimer(hwnd, KHUI_TRIGGER_TIMER_ID);
        khm_timer_fire(hwnd);
    }

    void Notifier::OnRefreshTimer() {
        KillTimer(hwnd, KHUI_REFRESH_TIMER_ID);
        kcdb_identity_refresh_all();
        khm_timer_refresh(hwnd);
    }

    khm_int32 Notifier::OnMsgAlert(khm_int32 msg_subtype, khm_ui_4 uparam, void * vparam) {
        switch (msg_subtype) {
        case KMSG_ALERT_SHOW:
            return OnMsgAlertShow((khui_alert *) vparam);

        case KMSG_ALERT_QUEUE:
            return OnMsgAlertQueue((khui_alert *) vparam);

        case KMSG_ALERT_CHECK_QUEUE:
            return OnMsgAlertCheckQueue();

        case KMSG_ALERT_SHOW_QUEUED:
            return OnMsgAlertShowQueued();

        case KMSG_ALERT_SHOW_MODAL:
            return OnMsgAlertShowModal((khui_alert *) vparam);

        case KMSG_ALERT_MONITOR_PROGRESS:
            return OnMsgAlertMonitorProgress((khui_alert *) vparam);
        }
        return KHM_ERROR_SUCCESS;
    }

    khm_int32 Notifier::OnMsgCred(khm_int32 msg_subtype, khm_ui_4 uparam, void * vparam) {
        if (msg_subtype == KMSG_CRED_ROOTDELTA) {
            return OnMsgCredRootDelta();
        }
        return KHM_ERROR_SUCCESS;
    }

    khm_int32 Notifier::OnWmDispatch(khm_int32 msg_type, khm_int32 msg_subtype,
                                     khm_ui_4 uparam, void * vparam) {
        switch (msg_type) {
        case KMSG_ALERT:
            return OnMsgAlert(msg_subtype, uparam, vparam);

        case KMSG_CRED:
            return OnMsgCred(msg_subtype, uparam, vparam);
        }

        return KHM_ERROR_SUCCESS;
    }

    BOOL Notifier::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT * lr) {
        if (uMsg == NotificationIcon::NOTIFICATION_MSG) {
            switch (lParam) {
            case WM_CONTEXTMENU:
                OnNotifyContextMenu();
                break;

            case NIN_SELECT:
            case NIN_KEYSELECT:
                OnNotifySelect();
                break;

            case NIN_BALLOONUSERCLICK:
                OnNotifyBalloonUserClick();
                break;

            case NIN_BALLOONHIDE:
            case NIN_BALLOONTIMEOUT:
                OnNotifyBalloonHide();
                break;
            }

            return TRUE;
        }

        return FALSE;
    }

    void Notifier::OnWmTimer(UINT_PTR id) {
        switch (id) {
        case KHUI_TRIGGER_TIMER_ID:
            OnTriggerTimer();
            break;

        case KHUI_REFRESH_TIMER_ID:
            OnRefreshTimer();
            break;
        }
    }

    BOOL Notifier::OnCreate(LPVOID createParams) {
        kmq_subscribe_hwnd(KMSG_ALERT, hwnd);
        kmq_subscribe_hwnd(KMSG_CRED, hwnd);
        return TRUE;
    }

    Notifier * g_notifier = NULL;

    extern "C" HWND hwnd_notifier = NULL;

    extern "C" const khm_int32 khm_notifier_actions[] = {
        KHUI_ACTION_OPEN_APP,
        KHUI_ACTION_NEW_CRED
    };

    extern "C" const khm_size n_khm_notifier_actions = ARRAYLENGTH(khm_notifier_actions);

    extern "C" void khm_init_notifier()
    {
        ControlWindow::RegisterWindowClass();

        g_notifier = PNEW Notifier();

        hwnd_notifier = g_notifier->Create(HWND_MESSAGE, Rect(0,0,0,0));

        g_notifier->m_icon->Add();

        khm_timer_init();
        khm_addr_change_notifier_init();
    }

    extern "C" void khm_exit_notifier()
    {
        khm_addr_change_notifier_exit();
        khm_timer_exit();

        g_notifier->m_icon->Remove();
        g_notifier->DestroyWindow();

        ControlWindow::UnregisterWindowClass();
    }

    // Legacy

    extern "C" khm_int32 khm_get_default_notifier_action(void)
    {
        return g_notifier->GetDefaultNotifierAction();
    }
};
