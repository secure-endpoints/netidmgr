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

#pragma once

#include "AlertContainer.hpp"
#include "AlertWindow.hpp"
#include "NotificationIcon.hpp"

namespace nim {

    // Notifier
    class Notifier : public ControlWindow {

	AlertWindowList m_alert_windows;

	AlertList m_alert_list;

	Alert m_balloon_alert;

    public:
	NotificationIcon *m_icon;

	Notifier();

	~Notifier();

    public:			// Alert show methods

	bool IsAlertDisplayed() const;

	khm_int32 ShowAlertNormal(Alert& a);

	khm_int32 ShowAlertMini(Alert& a);

	khm_int32 ShowAlert(Alert& a, bool ok_to_enqueue = true);

	void EnqueueAlert(Alert& a);

	void ShowQueuedAlerts();

	void UpdateAlertState();

	void BeginMonitoringAlert(Alert& alert);

    public:			// Utility

	khm_int32 GetDefaultNotifierAction();

	void NotificationIconActivate();

    private:			// KMSG_ALERT handlers

	khm_int32 OnMsgAlertShow(khui_alert * _a);

	khm_int32 OnMsgAlertQueue(khui_alert * _a);

	khm_int32 OnMsgAlertCheckQueue();

	khm_int32 OnMsgAlertShowQueued();

	khm_int32 OnMsgAlertShowModal(khui_alert * _a);

	khm_int32 OnMsgAlertMonitorProgress(khui_alert * _a);

    private: 			// KMSG_CRED handlers
	khm_int32 OnMsgCredRootDelta();

    private:			// Notification icon messages
	void OnNotifyContextMenu();

	void OnNotifySelect();

	void OnNotifyBalloonUserClick();

	void OnNotifyBalloonHide();

    private: 			// Timer messages
	void OnTriggerTimer();

	void OnRefreshTimer();

    private:			// Dispatchers
	khm_int32 OnMsgAlert(khm_int32 msg_subtype, khm_ui_4 uparam, void * vparam);

	khm_int32 OnMsgCred(khm_int32 msg_subtype, khm_ui_4 uparam, void * vparam);

	virtual khm_int32 OnWmDispatch(khm_int32 msg_type, khm_int32 msg_subtype,
				       khm_ui_4 uparam, void * vparam);

	virtual void OnWmTimer(UINT_PTR id);

	virtual BOOL OnCreate(LPVOID createParams);

        virtual BOOL HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT * lr);
    };

    extern Notifier * g_notifier;

    extern "C" void khm_init_notifier();

    extern "C" void khm_exit_notifier();
}
