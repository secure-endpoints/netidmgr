#pragma once

namespace nim {

    class NotificationIcon {
	AutoRef<ControlWindow> m_notify;
	std::wstring m_tooltip;
	UINT m_iid;
	khm_int32 m_severity;

    public:
	NotificationIcon(ControlWindow * cw_notify):
	    m_notify(cw_notify),
	    m_iid(IDI_NOTIFY_NONE),
	    m_severity(KHERR_NONE)
	{}

	~NotificationIcon() {
	    Remove();
	}

	void Add();

	void ShowBalloon(khm_int32 severity,
			 const wchar_t * title,
			 const wchar_t * msg,
			 int timeout);

	void SetExpiryState(enum khm_notif_expstate expseverity);

	void SetSeverity(khm_int32 severity);

	void SetTooltip(const wchar_t * tooltip);

	void SetProgress(int progress);

	void Remove();
    };
}
