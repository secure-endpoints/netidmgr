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

// Applies to DisplayContainer
template<class T = DisplayContainer>
class NOINITVTABLE WithTooltips : public T {
    HWND hwnd_tooltip;
    std::wstring tt_text;
    Rect         tt_rect;
    bool         show_tt : 1;
    bool         shown_tt: 1;
    Point        pt_mouse;

    static const int TOOL_ID = 1;

 public:
    WithTooltips() : tt_rect(0,0,0,0) {
        show_tt = false;
        shown_tt = false;
    }

    ~WithTooltips() {
        if (hwnd_tooltip)
            ::DestroyWindow(hwnd_tooltip);
        hwnd_tooltip = NULL;
    }

    virtual BOOL OnCreate(LPVOID createParams) {
        hwnd_tooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS,
                                      NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,		
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                      hwnd, NULL, khm_hInstance, NULL);
        SetWindowPos(hwnd_tooltip, HWND_TOPMOST, 0, 0,
                     0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        TOOLINFO ti = { sizeof(TOOLINFO), TTF_ABSOLUTE | TTF_TRACK | TTF_TRANSPARENT,
                        hwnd, TOOL_ID, {0,0,0,0},
                        khm_hInstance, LPSTR_TEXTCALLBACK, 0 };
        ::GetClientRect(hwnd, &ti.rect);

        if (!::SendMessage(hwnd_tooltip, TTM_ADDTOOL, 0, (LPARAM) &ti)) {
            ti.cbSize -= sizeof(void *);
            ::SendMessage(hwnd_tooltip, TTM_ADDTOOL, 0, (LPARAM) &ti);
        }
        return T::OnCreate(createParams);
    }

    virtual void OnMouseHover(const Point& p, UINT keyflags) {
        if (mouse_element) {
            show_tt = mouse_element->OnShowToolTip(tt_text, tt_rect);
            if (show_tt) {
                TOOLINFO ti = { sizeof(TOOLINFO), 0, hwnd, TOOL_ID, {0,0,0,0},
                                NULL, NULL, 0 };
                RECT r = { tt_rect.GetLeft(), tt_rect.GetTop(),
                           tt_rect.GetRight(), tt_rect.GetBottom() };

                ::SendMessage(hwnd_tooltip, TTM_TRACKACTIVATE, TRUE, (LPARAM) &ti);
                ::SendMessage(hwnd_tooltip, TTM_ADJUSTRECT, TRUE, (LPARAM) &r);
                ::SendMessage(hwnd_tooltip, TTM_TRACKPOSITION, 0, MAKELONG(r.left, r.top));
                shown_tt = true;
                pt_mouse = p;
            }
        }
        T::OnMouseHover(p, keyflags);
    }

    virtual void OnMouseMove(const Point& pt_c, UINT keyflags) {
        if (shown_tt && !pt_mouse.Equals(pt_c)) {
            TOOLINFO ti = { sizeof(TOOLINFO), 0, hwnd, TOOL_ID, {0,0,0,0},
                            NULL, NULL, 0 };

            ::SendMessage(hwnd_tooltip, TTM_TRACKACTIVATE, FALSE, (LPARAM) &ti);
            shown_tt = false;
        }
        T::OnMouseMove(pt_c, keyflags);
    }

    LPARAM OnTooltipGetDispInfo(NMTTDISPINFO * pdi) {
        pdi->lpszText = (LPWSTR) tt_text.c_str();
        return !show_tt;
    }

    LPARAM OnTooltipShow(NMHDR * pnmh) {
        return 0;
    }

    LPARAM OnTooltipPop(NMHDR * pnmh) {
        return 0;
    }

    LPARAM OnTooltipNotify(int id, NMHDR * pnmh) {
        switch (pnmh->code) {
        case TTN_GETDISPINFO:
            return OnTooltipGetDispInfo(reinterpret_cast<NMTTDISPINFO *>(pnmh));

        case TTN_SHOW:
            return OnTooltipShow(pnmh);

        case TTN_POP:
            return OnTooltipPop(pnmh);
        }
        return 0;
    }

    virtual LPARAM OnNotify(int id, NMHDR * pnmh) {
        if (pnmh->hwndFrom == hwnd_tooltip)
            return OnTooltipNotify(id, pnmh);
        return T::OnNotify(id, pnmh);
    }
};


}
