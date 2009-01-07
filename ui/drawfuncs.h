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
        DrawStateHotTrack     = (1L << 0),
        DrawStateActive       = (1L << 1),
        DrawStateSelected     = (1L << 2),
        DrawStateChecked      = (1L << 3),
        DrawStateDisabled     = (1L << 4),
        DrawStateFocusRect    = (1L << 5),
        DrawStateNoBackground = (1L << 6),

        DrawStateExpired      = (1L << 16),
        DrawStateCritial      = (1L << 17),
        DrawStateWarning      = (1L << 18),
        DrawStateBusy         = (1L << 19),
        DrawStatePostDated    = (1L << 20),
    } DrawState;

    typedef enum DrawElement {
        DrawNone             = 0,
        DrawCwBackground,
        DrawCwOutlineBackground,
        DrawCwOutlineWidget,
        DrawCwNormalBackground,
    } DrawElement;

    class KhmDraw {
    public:
        HFONT   hf_normal;          // normal text
        HFONT   hf_header;          // header text
        HFONT   hf_select;          // selected text
        HFONT   hf_select_header;   // selected header text

        Color   c_selection;        // Selection color
        Color   c_background;       // Background color
        Color   c_header;           // Header color

        Color   c_warning;          // Warning color
        Color   c_critical;         // Critical color
        Color   c_expired;          // Expired color

        Color   c_text;             // Normal text color
        Color   c_text_selected;    // Selected text color

        Image  *b_credwnd;          // Credentials window widget images (small icon size)
        Image  *b_watermark;        // Credentials window watermark
        Image  *b_meter_state;      // Life meter state images
        Image  *b_meter_life;       // Life meter remainder images
        Image  *b_meter_renew;      // Life meter renewal animation images

        bool    isThemeLoaded;      // TRUE if a theme was loaded

        Size    sz_icon;        // Size of large icon
        Size    sz_icon_sm;     // Size of small icon
        Size    sz_margin;      // Size of small margin
        Size    sz_meter;       // Size of meter images

        int     line_thickness;

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
            ImgStarHi,
            ImgStar,
            ImgStarEmptyHi,
            ImgStarEmpty
        } CredWndImages;

    protected:
        void DrawImageByIndex(Graphics & g, Image * img, const Point& p, INT idx, const Size& sz);
        void DrawCredWindowImage(Graphics & g, CredWndImages img, const Point& p);

    public:
        KhmDraw();
        ~KhmDraw();

        void LoadTheme(const wchar_t * themename = NULL);
        void UnloadTheme(void);

        void DrawCredWindowBackground(Graphics & g, const Rect& extents,
                                      const Rect& clip);

        void DrawCredWindowOutline(Graphics& g, const Rect& extents, DrawState state);

        void DrawCredWindowOutlineWidget(Graphics& g, const Rect& extents, DrawState state);

        void DrawStarWidget(Graphics& g, const Rect& extents, DrawState state);

        void DrawCredWindowNormalBackground(Graphics& g, const Rect& extents, DrawState state);

        void DrawCredMeterState(Graphics& g, const Rect& extents, DrawState state, DWORD *ms_to_next);

        // index is from 0 ... 255
        void DrawCredMeterLife(Graphics& g, const Rect& extents, unsigned int index);

        void DrawCredMeterBusy(Graphics& g, const Rect& extents, DWORD *ms_to_next);
    };

    typedef enum DrawTextStyle {
        DrawTextCredWndIdentity  = 1,
        DrawTextCredWndType,
        DrawTextCredWndStatus,
        DrawTextCredWndNormal
    };

    // Applies to WithTextDisplay<>
    template <class T>
    class HeaderTextBoxT : public WithCachedFont< T > {
        void GetStringFormat(StringFormat& sf) {
            sf.SetFormatFlags(StringFormatFlagsNoWrap);
            sf.SetTrimming(StringTrimmingEllipsisCharacter);
        }

        Font * GetFontCreate(HDC hdc) {
            return new Font(hdc, g_theme->hf_header);
        }

        Color GetForegroundColor() {
            return (selected)? g_theme->c_text_selected : g_theme->c_text;
        }
    };

    // Applies to WithTextDisplay
    template <class T>
    class SubheaderTextBoxT : public WithCachedFont< T > {
        void GetStringFormat(StringFormat& sf) {
            sf.SetFormatFlags(StringFormatFlagsNoWrap);
            sf.SetTrimming(StringTrimmingEllipsisCharacter);
        }

        Font * GetFontCreate(HDC hdc) {
            return new Font(hdc, g_theme->hf_normal);
        }

        Color GetForegroundColor() {
            return (selected)? g_theme->c_text_selected : g_theme->c_text;
        }
    };

    // Applies to WithTextDisplay
    template <class T>
    class IdentityStatusTextT : public WithCachedFont< T > {
        void GetStringFormat(StringFormat& sf) {
            sf.SetFormatFlags(0);
            sf.SetAlignment(StringAlignmentNear);
        }

        Font * GetFontCreate(HDC hdc) {
            return new Font(hdc, g_theme->hf_normal);
        }

        Color GetForegroundColor() {
            return (selected)? g_theme->c_text_selected : g_theme->c_text;
        }
    };

    template <class T>
    class ColumnCellTextT : public WithCachedFont< T > {
        void GetStringFormat(StringFormat& sf) {
            sf.SetFormatFlags(StringFormatFlagsNoWrap);
            sf.SetAlignment(StringAlignmentNear);
            sf.SetLineAlignment(StringAlignmentCenter);
            sf.SetTrimming(StringTrimmingEllipsisCharacter);
        }

        Font * GetFontCreate(HDC hdc) {
            return new Font(hdc, g_theme->hf_normal);
        }

        Color GetForegroundColor() {
            return (selected)? g_theme->c_text_selected : g_theme->c_text;
        }
    };

    template <class T>
    class GenericTextT : public WithCachedFont< T > {
        void GetStringFormat(StringFormat& sf) {
            sf.SetFormatFlags(0);
            sf.SetAlignment(StringAlignmentNear);
        }

        Font * GetFontCreate(HDC hdc) {
            return new Font(hdc, g_theme->hf_normal);
        }

        Color GetForegroundColor() {
            return (selected)? g_theme->c_text_selected : g_theme->c_text;
        }
    };

    extern KhmDraw * g_theme;

    inline Rect RectFromRECT(const RECT * r) {
        return Rect(r->left, r->top, r->right - r->left, r->bottom - r->top);
    }

    inline Color operator * (const Color& left, BYTE right) {
        unsigned int a = (((unsigned int) left.GetAlpha()) * right) / 255;
        return Color((left.GetValue() & (Color::RedMask | Color::GreenMask | Color::BlueMask)) |
                     (a << Color::AlphaShift));
    }

    inline Color& operator += (Color& left, const Color& right) {
        UINT la = (UINT) left.GetAlpha();
        UINT ra = (UINT) right.GetAlpha();
        UINT a = la + ((255 - la) * ra) / 255;
        UINT r = (left.GetRed() * la + right.GetRed() * ra) / 255;
        UINT g = (left.GetGreen() * la + right.GetGreen() * ra) / 255;
        UINT b = (left.GetBlue() * la + right.GetBlue() * ra) / 255;

        left.SetValue(Color::MakeARGB(a, __min(r, 255), __min(g, 255), __min(b, 255)));
        return left;
    }

    inline Color operator + (const Color& left, const Color& right) {
        UINT la = (UINT) left.GetAlpha();
        UINT ra = (UINT) right.GetAlpha();
        UINT a = la + ((255 - la) * ra) / 255;
        UINT r = (left.GetRed() * la + right.GetRed() * ra) / 255;
        UINT g = (left.GetGreen() * la + right.GetGreen() * ra) / 255;
        UINT b = (left.GetBlue() * la + right.GetBlue() * ra) / 255;

        return Color(Color::MakeARGB(a, __min(r, 255), __min(g, 255), __min(b, 255)));
    }

    std::wstring
    LoadStringResource(UINT res_id, HINSTANCE inst = khm_hInstance);

    HICON
    LoadIconResource(UINT res_id, bool small_icon,
                     bool shared = true, HINSTANCE inst = khm_hInstance);

    HBITMAP
    LoadImageResource(UINT res_id, bool shared = true,
                      HINSTANCE inst = khm_hInstance);

    Image*
    LoadImageResourceAsStream(LPCTSTR name, LPCTSTR type, HINSTANCE inst = khm_hInstance);

} /* namespace nim */
#else  /* not __cplusplus */

void khm_init_drawfuncs(void);

void khm_exit_drawfuncs(void);

#endif

#endif
