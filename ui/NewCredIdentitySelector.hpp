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

class NewCredWizard;

class NewCredIdentitySelector : public DialogWindow {
    class IdentityDisplayData {
    public:
        Identity            identity;
        std::wstring        display_string;
        std::wstring        status;
        std::wstring        type_string;
        HICON               icon;

        IdentityDisplayData(const Identity& _identity):
            identity(_identity),
            display_string( _identity.GetResourceString(KCDB_RES_DISPLAYNAME) ),
            type_string( _identity.GetProvider().GetResourceString(KCDB_RES_INSTANCE) ),
            icon( _identity.GetResourceIcon(KCDB_RES_ICON_NORMAL) )
        {}

        IdentityDisplayData()
        {}
    };

    IdentityDisplayData     *m_current;

    khui_new_creds          *nc;

public:
    NewCredIdentitySelector(khui_new_creds * _nc):
        nc(_nc),
        m_current(NULL),
        DialogWindow(MAKEINTRESOURCE(IDD_NC_IDSEL), khm_hInstance)
    {}

    ~NewCredIdentitySelector() {
        if (m_current) {
            delete m_current;
            m_current = NULL;
        }
    }

    void    UpdateLayout();

    void    SetStatus(const wchar_t * status);

    void    ShowIdentitySelector();

    LRESULT OnNotifyIdentitySelector(NMHDR * pnmh);

    LRESULT OnNotify(int id, NMHDR * pnmh);

    void    OnCommand(int id, HWND hwndCtl, UINT codeNotify);

    LRESULT OnDrawItem( const DRAWITEMSTRUCT * lpDrawItem );

    LRESULT OnMeasureItem( MEASUREITEMSTRUCT * lpMeasureItem );
};
}
