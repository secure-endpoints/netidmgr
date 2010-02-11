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

namespace nim {

// Applies to DisplayElement
template <class T = DisplayElement>
class NOINITVTABLE WithHostedControl : public T {
 protected:
    HWND hwnd_hosted;

 public:
    WithHostedControl() : hwnd_hosted(NULL) { }

    ~WithHostedControl() {
        if (hwnd_hosted) {
            DestroyWindow(hwnd_hosted);
        }
    }

    void SetWindow(HWND _hwnd) {
        hwnd_hosted = _hwnd;
    }

    HWND GetWindow() {
        return hwnd_hosted;
    }

    virtual void NotifyLayout(void) {
        Point p_client;

        assert(owner);

        if (hwnd_hosted && owner) {
            p_client = owner->VirtualToClient(owner->MapFromDescendant(this, Point(0,0)));

            SetWindowPos(hwnd_hosted, NULL, p_client.X, p_client.Y,
                         0, 0, SWP_MOVEONLY);
        }
    }

    virtual void UpdateLayoutPost(Graphics& g, const Rect& layout) {
        if (hwnd_hosted) {
            RECT r;

            if (visible) {

                ::GetClientRect(hwnd_hosted, &r);

                extents.Width = r.right - r.left;
                extents.Height = r.bottom - r.top;

            } else {

                extents.Width = 0;
                extents.Height = 0;

                SetWindowPos(hwnd_hosted, NULL, 0, 0, 0, 0, SWP_HIDEONLY);
            }

        } else {

            extents.Width = 0;
            extents.Height = 0;

        }
    }
};
}
