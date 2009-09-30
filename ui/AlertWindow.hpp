#pragma once
#include "AlertContainer.hpp"
#include <list>

namespace nim {

    class AlertWindow;

    typedef std::list<AlertWindow *> AlertWindowList;

    class AlertWindow :
	public DialogWindow {

	AlertWindowList * m_owner;
	AlertContainer * m_alerts;

    public:
	AlertWindow() : DialogWindow(MAKEINTRESOURCE(IDD_ALERTS),
				     khm_hInstance) {
	    m_alerts = new AlertContainer();
	}

	~AlertWindow() {
	    m_alerts->Release();
	}

	bool Add(Alert & a) {
	    return m_alerts->Add(a);
	}

	bool IsEmpty() {
	    return m_alerts->IsEmpty();
	}

	bool BeginMonitoringAlert(Alert & a) {
	    return m_alerts->BeginMonitoringAlert(a);
	}

	void SetOwnerList(AlertWindowList * _owner);

    public: 			// event handlers
        virtual BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);

        virtual void OnCommand(int id, HWND hwndCtl, UINT codeNotify);

        virtual LRESULT OnHelp(HELPINFO * info);
    };
}

