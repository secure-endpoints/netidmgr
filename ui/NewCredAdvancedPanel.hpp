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

class NewCredAdvancedPanel : public DialogWindow {
    khui_new_creds * nc;

private:
    HWND GetTabControl() {
        return GetItem(IDC_NC_TAB);
    }

public:
    NewCredAdvancedPanel(khui_new_creds * _nc):
        nc(_nc),
        DialogWindow(MAKEINTRESOURCE(IDD_NC_PRIVINT_ADVANCED), khm_hInstance)
    {}

    void InitializeTabs();

    int  GetCurrentTabId();

    void GetTabPlacement(RECT& r);

    void OnCommand(int id, HWND hwndCtl, UINT codeNotify);

    LRESULT OnNotify(int id, NMHDR * pnmh);
};

}
