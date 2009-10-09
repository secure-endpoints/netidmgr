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

#include <list>
#include "generic_widgets.hpp"
#include "AlertElement.hpp"
#include "AlertContextMonitor.hpp"

namespace nim {

    typedef std::vector<Alert> AlertList;

    /*! \brief A display container for alerts
     */
    class AlertContainer :
	public
    WithVerticalLayout < 
	WithAcceleratorTranslation < 
	    WithNavigation < DisplayContainer > > >
    {
	AlertList m_alerts;
	std::wstring m_title;
        int m_unseen;

    private:

	void PickTitle();

        virtual void UpdateLayoutPre(Graphics & g, Rect & layout);

    public:			// Constants and stuff
	enum {
	    IDC_NTF_CMDBUTTONS = 1001,
	    //!< Starting control identifier for generated command buttons

	    IDC_NTF_ERRCTXMONITOR = 8001
	    //!< Control identifier for messages originating from error
	    // context event listeners (AlertContextMonitor objects)
	};

	static UINT ControlIdFromIndex(int alert, int button) {
	    return alert * (KHUI_MAX_ALERT_COMMANDS + 2) + button + 2 + IDC_NTF_CMDBUTTONS;
	}

	static int AlertFromControlId(UINT idc) {
	    return (idc - IDC_NTF_CMDBUTTONS) / (KHUI_MAX_ALERT_COMMANDS + 2);
	}

	static int ButtonFromControlId(UINT idc) {
	    return (idc - IDC_NTF_CMDBUTTONS) % (KHUI_MAX_ALERT_COMMANDS + 2) - 2;
	}

        bool IsCommandButtonControlId(UINT idc) {
            unsigned alert = AlertFromControlId(idc);
            unsigned button = ButtonFromControlId(idc);

            return (alert >= 0 && alert < m_alerts.size() &&
                    button >= 0 && button < (unsigned) m_alerts[alert]->n_alert_commands);
        }

    public:
	AlertContainer() : m_unseen(0) {}

	~AlertContainer() {}

	bool Add(Alert &alert);

	void ProcessCommand(int id);

	bool IsEmpty() {
	    return m_alerts.size() == 0;
	}

	bool BeginMonitoringAlert(Alert& a);

	virtual void AlertContainer::OnCommand(int id, HWND hwndCtl, UINT codeNotify);

	virtual DWORD GetStyleEx() {
	    return WS_EX_CONTROLPARENT;
	}

	virtual bool TranslateAccelerator(LPMSG pMsg);

    public:
	static AlertElement *GetAlertElement(Alert& alert, DisplayElement *container);
    };
}
