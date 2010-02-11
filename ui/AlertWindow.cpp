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

#include "khmapp.h"
#include "AlertWindow.hpp"
#include <algorithm>
#include <assert.h>

namespace nim {

AlertWindow::AlertWindow() : DialogWindow(MAKEINTRESOURCE(IDD_ALERTS),
                                          khm_hInstance)
{
    m_alerts = PNEW AlertContainer();
    m_owner = NULL;
}

AlertWindow::~AlertWindow()
{
    SetOwnerList(NULL);
    m_alerts->Release();
}

BOOL AlertWindow::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
    HWND hw_container = GetItem(IDC_CONTAINER);
    RECT r_container;

    GetWindowRect(hw_container, &r_container);
    MapWindowRect(NULL, hwnd, &r_container);
    m_alerts->Create(hwnd, RectFromRECT(&r_container));

    return FALSE;
}

void AlertWindow::SetOwnerList(AlertWindowList * _owner)
{
    if (m_owner)
        m_owner->remove(this);

    m_owner = _owner;

    if (m_owner)
        m_owner->push_back(this);
}

void AlertWindow::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
    if (codeNotify == BN_CLICKED) {
        if (id == KHUI_PACTION_NEXT ||
            id == IDCANCEL ||
            id == IDOK) {

            EndDialog(id);
        } else {

            m_alerts->ProcessCommand(id);
        }
    }
}

LRESULT AlertWindow::OnHelp(HELPINFO * info)
{
    DoDefault();
    return 0;
}

void AlertWindow::OnDestroy(void)
{
    SetOwnerList(NULL);
    __super::OnDestroy();
}
}
