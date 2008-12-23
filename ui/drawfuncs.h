/*
 * Copyright (c) 2008 Secure Endpoints Inc.
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

/* $Id$ */

#ifndef __NETIDMGR_DRAWFUNCS_H
#define __NETIDMGR_DRAWFUNCS_H

#ifdef __cplusplus

#include <gdiplus.h>

using namespace Gdiplus;

namespace nim {

    typedef enum DrawState {
        DrawStateNone         = 0,
        DrawStateNoBackground = (1L << 0),
        DrawStateSelected     = (1L << 1),
        DrawStateHotTrack     = (1L << 2),
        DrawStateFocusRect    = (1L << 3),

        DrawStateExpired      = (1L << 16),
        DrawStateCritial      = (1L << 17),
        DrawStateWarning      = (1L << 18),
    } DrawState;

    typedef enum DrawElement {
        DrawNone             = 0,
        DrawOutlineBackground,
        DrawOutlineWidget,
    } DrawElement;

    class KhmDraw {
    public:
        HFONT   hf_normal;          // normal text
        HFONT   hf_header;          // header text
        HFONT   hf_select;          // selected text
        HFONT   hf_select_header;   // selected header text

        Color   c_selection;        // Selection color
        Color   c_background;       // Background color

        Color   c_warning;          // Warning color
        Color   c_critical;         // Critical color
        Color   c_expired;          // Expired color

        Color   c_text;             // Normal text color
        Color   c_text_selected;    // Selected text color

        Bitmap *b_credwnd;          // Credentials window widget images
        Bitmap *b_watermark;        // Credentials window watermark

        bool    isThemeLoaded;      // TRUE if a theme was loaded

        Size    sz_icon;        // Size of large icon
        Size    sz_icon_sm;     // Size of small icon
        Size    sz_margin;      // Size of small margin

        Point   pt_margin_cx;
        Point   pt_margin_cy;
        Point   pt_icon_cx;
        Point   pt_icon_cy;
        Point   pt_icon_sm_cx;
        Point   pt_icon_sm_cy;

    private:
        typedef enum CredWndImages {
            ImgExpand = 1,
            ImgExpandHi,
            ImgCollapse,
            ImgCollapseHi,
            ImgStick,
            ImgStickHi,
            ImgStuck,
            ImgStuckHi,
            ImgFlagNormal,
            ImgFlagRenewable,
            ImgFlagWarning,
            ImgFlagCritical,
            ImgFlagExpired,
            Img_Reserved,       // currently for the flag icon, though we don't want that
            ImgIdentity,
            ImgIdentityDisabled,
        } CredWndImages;

    protected:
        void DrawImageByIndex(Graphics & g, Image * img, const Point& p, INT idx);
        void DrawCredWindowImage(Graphics & g, CredWndImages img, const Point& p);

    public:
        KhmDraw();
        ~KhmDraw();

        void LoadTheme(const wchar_t * themename = NULL);
        void UnloadTheme(void);

        void DrawCredWindowBackground(Graphics & g, const Rect& extents, const Rect& clip);
        void DrawCredWindowElement(Graphics & g, const Rect& bounds, DrawElement elem, DrawState state );

        // Utilities
        Rect& VCenter(Rect & r, const Rect & ref) {
            r.Y = (ref.GetTop() + ref.GetBottom() - r.Height) / 2;
            return r;
        }
    };

    extern KhmDraw * g_theme;

    inline Rect RectFromRECT(const RECT * r) {
        return Rect(r->left, r->top, r->right - r->left, r->bottom - r->top);
    }

    std::wstring
        LoadStringResource(UINT res_id, HINSTANCE inst = khm_hInstance);

    HICON
        LoadIconResource(UINT res_id, bool small_icon,
                         bool shared = true, HINSTANCE inst = khm_hInstance);

    HBITMAP
        LoadImageResource(UINT res_id, bool shared = true,
                          HINSTANCE inst = khm_hInstance);

} /* namespace nim */
#endif

#endif
