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

namespace nim {

class NewCredNavigation : public DialogWindow {
    khui_new_creds * nc;

    int             m_controls; /*!< Combination of enum Controls */

    int             m_state; /*!< State flags */

public:

    enum Controls {
        Next        = (1L<<0),
        Prev        = (1L<<1),
        Finish      = (1L<<2),
        Abort       = (1L<<3),
        ShowCloseIf = (1L<<4),
        Close       = (1L<<5),
        Retry       = (1L<<6)
    };

    enum States {
        PreEnd      = (1L<<0),
        NoClose     = (1L<<1),
        Cancelled   = (1L<<2)
    };

public:
    NewCredNavigation( khui_new_creds * _nc ) :
        DialogWindow(MAKEINTRESOURCE(IDD_NC_NAV), khm_hInstance),
        nc(_nc),
        m_controls(0),
        m_state(0)
    {}

    void CheckControls();

    khm_int32 EnableControl(int t) {
        m_controls |= t;
        return m_controls;
    }

    khm_int32 DisableControl(int t) {
        m_controls &= ~t;
        return m_controls;
    }

    khm_int32 SetAllControls(int t) {
        m_controls = t;
        return m_controls;
    }

    bool IsControlEnabled(int t) {
        return (m_controls & t) == t;
    }

    khm_int32 EnableState(int s) {
        m_state |= s;
        return m_state;
    }

    khm_int32 DisableState(int s) {
        m_state &= ~s;
        return m_state;
    }

    khm_int32 SetAllState(int s) {
        return (m_state = s);
    }

    bool IsState(int t) {
        return (m_state & t) == t;
    }

    HWND UpdateLayout();

    void OnCommand(int id, HWND hwndCtl, UINT codeNotify);
};
}
