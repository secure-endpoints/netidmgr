/*
 * Copyright (c) 2008-2009 Secure Endpoints Inc.
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
    DrawStateSkipAux      = (1L << 7),
    DrawStateButton       = (1L << 8),

    DrawStateExpired      = (1L << 16),
    DrawStateCritial      = (1L << 17),
    DrawStateWarning      = (1L << 18),
    DrawStateBusy         = (1L << 19),
    DrawStatePostDated    = (1L << 20),
    DrawStateEmpty        = (1L << 21),
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
    HFONT   hf_normal;        // normal text
    HFONT   hf_header;        // header text
    HFONT   hf_select;        // selected text
    HFONT   hf_select_header; // selected header text

    Color   c_selection;    // Selection color
    Color   c_background;   // Background color
    Color   c_alert;	// Background color for alerts
    Color   c_suggestion;	// Background color for alert suggestion

    Color   c_normal;       // Normal background color
    Color   c_empty;        // Empty background color
    Color   c_warning;      // Warning background color
    Color   c_critical;     // Critical background color
    Color   c_expired;      // Expired background color

    Color   c_text;          // Normal text color
    Color   c_text_selected; // Selected text color
    Color   c_text_error;    // Error text color

    Color   c_tint;         // Tint color

    Image  *b_credwnd; // Credentials window widget images (small icon size)
    Image  *b_watermark;    // Credentials window watermark
    Image  *b_meter_state;  // Life meter state images
    Image  *b_meter_life;   // Life meter remainder images
    Image  *b_meter_renew;  // Life meter renewal animation images
    Bitmap *b_progress;     // Progress bar

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

public:

    KhmDraw();

    ~KhmDraw();

    void DrawImageByIndex(Graphics & g, Image * img, const Point& p, INT idx, const Size& sz);

    void DrawCredWindowImage(Graphics & g, CredWndImages img, const Point& p);

    void LoadTheme(const wchar_t * themename = NULL);

    void UnloadTheme(void);

    void DrawCredWindowBackground(Graphics & g, const Rect& extents,
                                  const Rect& clip);

    void DrawCredWindowOutline(Graphics& g, const Rect& extents, DrawState state);

    void DrawCredWindowOutlineWidget(Graphics& g, const Rect& extents, DrawState state);

    void DrawStarWidget(Graphics& g, const Rect& extents, DrawState state);

    void DrawStickyWidget(Graphics& g, const Rect& extents, DrawState state);

    void DrawCredWindowNormalBackground(Graphics& g, const Rect& extents, DrawState state);

    void DrawCredMeterState(Graphics& g, const Rect& extents, DrawState state, DWORD *ms_to_next);

    // index is from 0 ... 256
    void DrawCredMeterLife(Graphics& g, const Rect& extents, unsigned int index);

    void DrawCredMeterBusy(Graphics& g, const Rect& extents, DWORD *ms_to_next);

    // Progress is from 0 ... 256
    void DrawProgressBar(Graphics& g, const Rect& extents, int progress);

    void DrawFocusRect(Graphics& g, const Rect& extents);

    void DrawDropDownButton(HWND hwnd, HDC hdc,
                            DrawState state, RECT * client);

    void DrawIdentityItem(HDC hdc, const RECT& extents,
                          DrawState state,
                          HICON icon,
                          const std::wstring& title,
                          const std::wstring& subtitle,
                          const std::wstring& aux);

    void DrawAlertBackground(Graphics& g, const Rect& extents, DrawState state);

    void DrawAlertSuggestBackground(Graphics& g, const Rect& extents, DrawState state);

    void DrawImageNeutralBackground(Graphics& g, const Rect& extents);

    void DrawDragWidgets(Graphics& g, const RectF& rect, int corner);
};

typedef enum DrawTextStyle {
    DrawTextCredWndIdentity  = 1,
    DrawTextCredWndType,
    DrawTextCredWndStatus,
    DrawTextCredWndNormal
};

template <class T,
          HFONT KhmDraw::*font = &KhmDraw::hf_normal,
          INT format_flags = 0,
          StringTrimming trimming = StringTrimmingNone,
          StringAlignment alignment = StringAlignmentNear,
          StringAlignment line_alignment = StringAlignmentNear,
          Color KhmDraw::*clr_normal = &KhmDraw::c_text,
          Color KhmDraw::*clr_selected = &KhmDraw::c_text_selected
          >
class GenericTextFormatterT : public WithCachedFont< T > {
public:
    void GetStringFormat(StringFormat& sf) {
        sf.SetFormatFlags(format_flags);
        sf.SetTrimming(trimming);
        sf.SetAlignment(alignment);
        sf.SetLineAlignment(line_alignment);
    }

    Font * GetFontCreate(HDC hdc) {
        return new Font(hdc, g_theme->*font);
    }

    Color GetForegroundColor() {
        return (selected)? g_theme->*clr_selected : g_theme->*clr_normal;
    }
};

// Applies to WithTextDisplay<>
template <class T>
class HeaderTextBoxT :
        public GenericTextFormatterT<T, &KhmDraw::hf_header,
                                     StringFormatFlagsNoWrap, StringTrimmingEllipsisCharacter>
{};

// Applies to WithTextDisplay
template <class T>
class SubheaderTextBoxT :
        public GenericTextFormatterT<T, &KhmDraw::hf_normal,
                                     StringFormatFlagsNoWrap, StringTrimmingEllipsisCharacter>
{};

// Applies to WithTextDisplay
template <class T>
class IdentityStatusTextT :
        public GenericTextFormatterT<T>
{};

// Applies to WithTextDisplay
template <class T>
class IdentityCredentialListTextT :
        public GenericTextFormatterT<T, &KhmDraw::hf_normal,
                                     StringFormatFlagsNoWrap, StringTrimmingNone,
                                     StringAlignmentNear, StringAlignmentNear,
                                     &KhmDraw::c_tint>
{};

// Applies to WithTextDisplay
template <class T>
class ColumnCellTextT :
        public GenericTextFormatterT<T,
                                     &KhmDraw::hf_normal,
                                     StringFormatFlagsNoWrap,
                                     StringTrimmingEllipsisCharacter,
                                     StringAlignmentNear,
                                     StringAlignmentCenter>
{};

// Applies to WithTextDisplay
template <class T>
class GenericTextT :
        public GenericTextFormatterT<T>
{};

template <class T, void (KhmDraw::*DF)(Graphics&, const Rect&, DrawState)>
class BackgroundT : public T {
    virtual void PaintSelf(Graphics& g, const Rect& bounds, const Rect& clip) {
        (g_theme->*DF)(g, bounds, GetDrawState());
        __super::PaintSelf(g, bounds, clip);
    }

    virtual DrawState GetDrawState() {
        return DrawStateNone;
    }
};

extern KhmDraw * g_theme;

inline Rect RectFromRECT(const RECT * r) {
    return Rect(r->left, r->top, r->right - r->left, r->bottom - r->top);
}

inline RECT RECTFromRect(const Rect * r) {
    RECT R;

    R.left = r->X;
    R.top = r->Y;
    R.right = r->X + r->Width;
    R.bottom = r->Y + r->Height;

    return R;
}

inline Color operator * (const Color& left, BYTE right) {
    unsigned int a = (((unsigned int) left.GetAlpha()) * right) / 255;
    return Color((left.GetValue() & (Color::RedMask | Color::GreenMask | Color::BlueMask)) |
                 (a << Color::AlphaShift));
}

#define COMBINE_ALPHA(a1, a2)                   \
    (255 - ((255 - (a1)) * (255 - (a2))) / 255)
#define COMBINE_COMPONENT(c1, a1, c2, a2)                       \
    ((((c1) * (a1) * (255 - (a2))) / 255 + (c2) * (a2)) / 255)

inline Color& operator += (Color& left, const Color& right) {
    UINT la = (UINT) left.GetAlpha();
    UINT ra = (UINT) right.GetAlpha();
    UINT a = COMBINE_ALPHA(la, ra);
    UINT r = COMBINE_COMPONENT(left.GetRed(), la, right.GetRed(), ra);
    UINT g = COMBINE_COMPONENT(left.GetGreen(), la, right.GetGreen(), ra);
    UINT b = COMBINE_COMPONENT(left.GetBlue(), la, right.GetBlue(), ra);

    left.SetValue(Color::MakeARGB((BYTE) a,
                                  (BYTE) __min(r, 255),
                                  (BYTE) __min(g, 255),
                                  (BYTE) __min(b, 255)));
    return left;
}

inline Color operator + (const Color& left, const Color& right) {
    UINT la = (UINT) left.GetAlpha();
    UINT ra = (UINT) right.GetAlpha();
    UINT a = COMBINE_ALPHA(la, ra);
    UINT r = COMBINE_COMPONENT(left.GetRed(), la, right.GetRed(), ra);
    UINT g = COMBINE_COMPONENT(left.GetGreen(), la, right.GetGreen(), ra);
    UINT b = COMBINE_COMPONENT(left.GetBlue(), la, right.GetBlue(), ra);

    return Color(Color::MakeARGB((BYTE) a,
                                 (BYTE) __min(r, 255),
                                 (BYTE) __min(g, 255),
                                 (BYTE) __min(b, 255)));
}

#undef COMBINE_ALPHA
#undef COMBINE_COMPONENT

std::wstring
LoadStringResource(UINT res_id, HINSTANCE inst = khm_hInstance);

template <size_t cch> inline int
LoadStringResource(wchar_t (&str)[cch], UINT res_id, HINSTANCE inst = khm_hInstance) {
    return LoadString(inst, res_id, str, cch);
}

HICON
LoadIconResource(UINT res_id, bool small_icon = false,
                 bool shared = true, HINSTANCE inst = khm_hInstance);

HBITMAP
LoadImageResource(UINT res_id, bool shared = true,
                  HINSTANCE inst = khm_hInstance);

Image*
LoadImageResourceAsStream(LPCTSTR name, LPCTSTR type, HINSTANCE inst = khm_hInstance);

Bitmap *
GetBitmapFromHICON(HICON icon);

DrawState
GetIdentityDrawState(Identity& identity);

inline bool
IsRemoteSession() {
    return GetSystemMetrics(SM_REMOTESESSION) != 0;
}

bool
GetEncoderClsid(const wchar_t* format, CLSID* pClsid);
} /* namespace nim */


extern "C" {
#endif

    void khm_init_drawfuncs(void);

    void khm_exit_drawfuncs(void);

    void khm_reinit_drawfuncs(void);

#ifdef __cplusplus
}
#endif

#endif
