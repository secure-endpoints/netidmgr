#include "khmapp.h"
#include "NotificationIcon.hpp"

namespace nim {

#define KHUI_NOTIFY_ICON_ID 0

    void
    NotificationIcon::Add()
    {
	NOTIFYICONDATA ni;
	wchar_t buf[256];

	ZeroMemory(&ni, sizeof(ni));

	ni.cbSize = sizeof(ni);
	ni.hWnd = m_notify->hwnd;
	ni.uID = KHUI_NOTIFY_ICON_ID;
	ni.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	ni.hIcon = LoadIconResource(m_iid, true, false); // small_icon = true, shared = false
	ni.uCallbackMessage = NOTIFICATION_MSG;

	LoadStringResource(buf, IDS_NOTIFY_PREFIX);
	m_tooltip = buf;

	LoadStringResource(buf, IDS_NOTIFY_READY);
	m_tooltip += buf;

	StringCbCopy(ni.szTip, sizeof(ni.szTip), m_tooltip.c_str());

	Shell_NotifyIcon(NIM_ADD, &ni);

	DestroyIcon(ni.hIcon);

	ni.cbSize = sizeof(ni);
	ni.uVersion = NOTIFYICON_VERSION;
	Shell_NotifyIcon(NIM_SETVERSION, &ni);
    }

    void
    NotificationIcon::ShowBalloon(khm_int32 severity,
				  const wchar_t * title,
				  const wchar_t * msg,
				  int timeout)
    {
	NOTIFYICONDATA ni;
	int iid;

	if (!msg || !title)
	    return;

	ZeroMemory(&ni, sizeof(ni));

	ni.cbSize = sizeof(ni);

	switch (severity) {
	case KHERR_INFO:
	    ni.dwInfoFlags = NIIF_INFO;
	    iid = IDI_NOTIFY_INFO;
	    break;

	case KHERR_WARNING:
	    ni.dwInfoFlags = NIIF_WARNING;
	    iid = IDI_NOTIFY_WARN;
	    break;

	case KHERR_ERROR:
	    ni.dwInfoFlags = NIIF_ERROR;
	    iid = IDI_NOTIFY_ERROR;
	    break;

	default:
	    ni.dwInfoFlags = NIIF_NONE;
	    iid = m_iid;
	}

	ni.hWnd = m_notify->hwnd;
	ni.uID = KHUI_NOTIFY_ICON_ID;
	ni.uFlags = NIF_INFO | NIF_ICON;
	ni.hIcon = LoadIconResource(iid, true, false);

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

    void
    NotificationIcon::SetExpiryState(enum khm_notif_expstate expseverity)
    {
	int new_iid;
	
	if (expseverity == KHM_NOTIF_OK)
	    new_iid = IDI_APPICON_OK;
	else if (expseverity == KHM_NOTIF_WARN)
	    new_iid = IDI_APPICON_WARN;
	else if (expseverity == KHM_NOTIF_EXP)
	    new_iid = IDI_APPICON_EXP;
	else
	    new_iid = IDI_NOTIFY_NONE;

	if (m_iid == new_iid)
	    return;

	m_iid = new_iid;
    }

    void
    NotificationIcon::SetSeverity(khm_int32 severity)
    {
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
	    iid = m_iid;

	ZeroMemory(&ni, sizeof(ni));

	ni.cbSize = sizeof(ni);
	ni.hWnd = m_notify->hwnd;
	ni.uID = KHUI_NOTIFY_ICON_ID;
	ni.uFlags = NIF_ICON | NIF_TIP;
	ni.hIcon = LoadIconResource(iid, true, false);

	if (severity == KHERR_NONE) {
	    StringCbCopy(ni.szTip, sizeof(ni.szTip), m_tooltip.c_str());
	} else {
	    LoadStringResource(buf, IDS_NOTIFY_PREFIX);
	    StringCbCopy(ni.szTip, sizeof(ni.szTip), buf);
	    LoadStringResource(buf, IDS_NOTIFY_ATTENTION);
	    StringCbCat(ni.szTip, sizeof(ni.szTip), buf);
	}

	Shell_NotifyIcon(NIM_MODIFY, &ni);

	DestroyIcon(ni.hIcon);

	m_severity = severity;
    }

    void
    NotificationIcon::SetTooltip(const wchar_t * tooltip)
    {
	m_tooltip = LoadStringResource(IDS_NOTIFY_PREFIX);
	m_tooltip += tooltip;

	if (m_severity == KHERR_NONE) {
	    NOTIFYICONDATA ni;

	    ZeroMemory(&ni, sizeof(ni));

	    ni.cbSize = sizeof(ni);
	    ni.hWnd = m_notify->hwnd;
	    ni.uID = KHUI_NOTIFY_ICON_ID;
	    ni.uFlags = NIF_TIP;

	    StringCbCopy(ni.szTip, sizeof(ni.szTip), m_tooltip.c_str());

	    Shell_NotifyIcon(NIM_MODIFY, &ni);
	}
    }

    void
    NotificationIcon::SetProgress(int progress)
    {
    }

    void
    NotificationIcon::Remove()
    {
	NOTIFYICONDATA ni;

	ZeroMemory(&ni, sizeof(ni));

	ni.cbSize = sizeof(ni);
	ni.hWnd = m_notify->hwnd;
	ni.uID = KHUI_NOTIFY_ICON_ID;

	Shell_NotifyIcon(NIM_DELETE, &ni);
    }
}
