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

class NewCredIdentitySpecifier : public DialogWindow {
    khui_new_creds *nc;

    khm_boolean     initialized; 
    /*< Has the identity provider list
      been initialized? */

    khm_boolean     in_layout;
    /*< Are we in nc_layout_idspec()?
      This field is used to suppress event
      handling when we are modifying some
      of the controls. */

    khm_ssize       idx_current;
    /*< Index of last provider */

    NewCredPage     prev_page;
    /*< Previous page, if we allow back
      navigation */

    friend class NewCredWizard;

public:
    NewCredIdentitySpecifier(khui_new_creds * _nc):
        nc(_nc),
        DialogWindow(MAKEINTRESOURCE(IDD_NC_IDSPEC), khm_hInstance),
        initialized(false),
        in_layout(false),
        prev_page(NC_PAGE_NONE)
    {}

    void Initialize(HWND hwnd);

    HWND UpdateLayout();

    bool ProcessNewIdentity();

    LRESULT OnNotify(int id, NMHDR * pnmh);
};

}
