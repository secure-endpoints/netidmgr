/*
 * Copyright (c) 2009-2010 Secure Endpoints Inc.
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
#include <list>

namespace nim {

class AlertWindow;

typedef std::list<AlertWindow *> AlertWindowList;

class AlertWindow :
	virtual public DialogWindow {

    AlertWindowList * m_owner;
    AlertContainer * m_alerts;

public:
    AlertWindow();

    ~AlertWindow();

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

    virtual void OnDestroy(void);

    virtual LRESULT OnHelp(HELPINFO * info);
};
}

