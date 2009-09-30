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

	virtual void OnCommand(int id, HWND hwndCtl, UINT codeNotify);

	virtual void OnWmTimer(UINT id);

	virtual BOOL OnCreate(LPVOID createParams);
    };

    extern Notifier * g_notifier;

    extern "C" void khm_init_notifier();

    extern "C" void khm_exit_notifier();
}
