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

namespace nim {

    class NotificationIcon {
	AutoRef<ControlWindow> m_notify;
	std::wstring m_tooltip;
	UINT m_iid;
	khm_int32 m_severity;

    public:
        enum {
            NOTIFICATION_MSG = WM_USER + 1
        };

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
