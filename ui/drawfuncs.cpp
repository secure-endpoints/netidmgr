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

#include "khmapp.h"
#include <assert.h>

namespace nim
{

    KhmDraw * g_theme = NULL;

    KhmDraw::KhmDraw()
    {
        isThemeLoaded = FALSE;
    }

    KhmDraw::~KhmDraw()
    {
        UnloadTheme();
    }

    void KhmDraw::LoadTheme(const wchar_t * themename)
    {
        if (themename)
            khm_load_theme(themename);

        hf_normal        = khm_get_element_font(KHM_FONT_NORMAL);
        hf_header        = khm_get_element_font(KHM_FONT_HEADER);
        hf_select        = khm_get_element_font(KHM_FONT_SELECT);
        hf_select_header = khm_get_element_font(KHM_FONT_HEADERSEL);

        c_selection    .SetFromCOLORREF(khm_get_element_color(KHM_CLR_SELECTION));
        c_background   .SetFromCOLORREF(khm_get_element_color(KHM_CLR_BACKGROUND));
        c_warning      .SetFromCOLORREF(khm_get_element_color(KHM_CLR_HEADER_WARN));
        c_critical     .SetFromCOLORREF(khm_get_element_color(KHM_CLR_HEADER_CRIT));
        c_expired      .SetFromCOLORREF(khm_get_element_color(KHM_CLR_HEADER_EXP));
        c_text         .SetFromCOLORREF(khm_get_element_color(KHM_CLR_TEXT));
        c_text_selected.SetFromCOLORREF(khm_get_element_color(KHM_CLR_TEXT_SEL));

        b_credwnd =   Bitmap::FromResource(khm_hInstance, MAKEINTRESOURCE(IDB_CREDWND_IMAGELIST));
        b_watermark = Bitmap::FromResource(khm_hInstance, MAKEINTRESOURCE(IDB_LOGO_SHADE));

        sz_icon.Width     = GetSystemMetrics(SM_CXICON);
        sz_icon.Height    = GetSystemMetrics(SM_CYICON);
        sz_icon_sm.Width  = GetSystemMetrics(SM_CXSMICON);
        sz_icon_sm.Height = GetSystemMetrics(SM_CYSMICON);
        sz_margin.Width   = sz_icon_sm.Width / 2;
        sz_margin.Height  = sz_icon_sm.Height / 2;

#define CXCY_FROM_SIZE(p,s)                     \
        p ## _cx.X = s.Width;                   \
        p ## _cx.Y = 0;                         \
        p ## _cy.X = 0;                         \
        p ## _cy.Y = s.Height

        CXCY_FROM_SIZE(pt_margin, sz_margin);
        CXCY_FROM_SIZE(pt_icon, sz_icon);
        CXCY_FROM_SIZE(pt_icon_sm, sz_icon_sm);

#undef  CXCY_FROM_SIZE

        isThemeLoaded = TRUE;
    }

    void KhmDraw::UnloadTheme(void)
    {
        if (b_credwnd) {
            delete b_credwnd;
            b_credwnd = NULL;
        }

        if (b_watermark) {
            delete b_watermark;
            b_watermark = NULL;
        }

        isThemeLoaded = FALSE;
    }

    void KhmDraw::DrawImageByIndex(Graphics & g, Image * img, const Point& p, INT idx)
    {
        if (idx < 0 || idx * img->GetHeight() > img->GetWidth())
            return;

        g.DrawImage(img, p.X, p.Y,
                    img->GetHeight() * idx, 0,
                    img->GetHeight(), img->GetHeight(),
                    UnitPixel);
    }

    void
    KhmDraw::DrawCredWindowImage(Graphics & g, CredWndImages img, const Point& p)
    {
        int idx = static_cast<int>(img);

        DrawImageByIndex(g, b_credwnd, p, idx - 1);
    }

    void
    KhmDraw::DrawCredWindowBackground(Graphics & g, const Rect & extents, const Rect & clip)
    {
        SolidBrush br(c_background);
        Rect watermark(0, 0, b_watermark->GetWidth(), b_watermark->GetHeight());

        // watermark is at the bottom right corner
        watermark.Offset(extents.GetRight() - watermark.Width,
                         extents.GetBottom() - watermark.Height);

        if (clip.IntersectsWith(watermark)) {
            Region bkg(clip);

            bkg.Exclude(watermark);
            g.FillRegion(&br, &bkg);

            Rect rmark(watermark);
            rmark.Intersect(clip);
            g.DrawImage(b_watermark, rmark.X, rmark.Y,
                        rmark.X - watermark.X, rmark.Y - watermark.Y,
                        rmark.Width, rmark.Height, UnitPixel);
        } else {
            g.FillRectangle(&br, clip);
        }
    }



    std::wstring LoadStringResource(UINT res_id, HINSTANCE inst)
    {
        wchar_t str[2048] = L"";
        if (LoadString(inst, res_id, str, ARRAYLENGTH(str)) == 0)
            str[0] = L'\0';
        return std::wstring(str);
    }

    HICON LoadIconResource(UINT res_id, bool small_icon,
                           bool shared, HINSTANCE inst)
    {
        return (HICON) LoadImage(inst, MAKEINTRESOURCE(res_id), IMAGE_ICON,
                                 GetSystemMetrics((small_icon)? SM_CXSMICON : SM_CXICON),
                                 GetSystemMetrics((small_icon)? SM_CYSMICON : SM_CYICON),
                                 LR_DEFAULTCOLOR | ((shared)? LR_SHARED : 0));
    }

    HBITMAP LoadImageResource(UINT res_id, bool shared,
                              HINSTANCE inst)
    {
        return (HBITMAP) LoadImage(inst, MAKEINTRESOURCE(res_id), IMAGE_BITMAP,
                                   0, 0,
                                   LR_DEFAULTCOLOR | LR_DEFAULTSIZE | ((shared)? LR_SHARED : 0));
    }

    void
    khm_measure_identity_menu_item(HWND hwnd, LPMEASUREITEMSTRUCT lpm, khui_action * act)
    {
    }

    void
    khm_draw_identity_menu_item(HWND hwnd, LPDRAWITEMSTRUCT lpd, khui_action * act)
    {
    }

#if 0
    void
    khm_measure_identity_menu_item(HWND hwnd, LPMEASUREITEMSTRUCT lpm, khui_action * act)
    {
        wchar_t * cap;
        HDC hdc;
        SIZE sz;
        size_t len;
        HFONT hf_old;

        sz.cx = MENU_SIZE_ICON_X;
        sz.cy = MENU_SIZE_ICON_Y;

        cap = act->caption;
        assert(cap);
        hdc = GetDC(khm_hwnd_main);
        assert(hdc);

        StringCchLength(cap, KHUI_MAXCCH_NAME, &len);

        hf_old = SelectFont(hdc, (HFONT) GetStockObject(DEFAULT_GUI_FONT));

        GetTextExtentPoint32(hdc, cap, (int) len, &sz);

        SelectFont(hdc, hf_old);

        ReleaseDC(khm_hwnd_main, hdc);

        lpm->itemWidth = sz.cx + sz.cy * 3 / 2 + GetSystemMetrics(SM_CXSMICON);
        lpm->itemHeight = sz.cy * 3 / 2;
    }

    void
    khm_draw_identity_menu_item(HWND hwnd, LPDRAWITEMSTRUCT lpd, khui_action * act)
    {
        khui_credwnd_tbl * tbl;
        khm_handle ident;
        size_t count = 0;
        COLORREF old_clr;
        wchar_t * cap;
        size_t len;
        int margin;
        SIZE sz;
        HBRUSH hbr;
        COLORREF text_clr;
        khm_int32 idflags;
        khm_int32 expflags;

        tbl = (khui_credwnd_tbl *)(LONG_PTR) GetWindowLongPtr(hwnd, 0);
        if (tbl == NULL)
            return;

        ident = act->data;
        cap = act->caption;
        assert(ident != NULL);
        assert(cap != NULL);

        {
            khui_credwnd_ident * cwi;

            cwi = cw_find_ident(tbl, ident);
            if (cwi) {
                count = cwi->id_credcount;
            } else {
                count = 0;
            }
        }

        expflags = cw_get_buf_exp_flags(tbl, ident);

        text_clr = tbl->cr_hdr_normal;

        if (lpd->itemState & (ODS_HOTLIGHT | ODS_SELECTED)) {
            hbr = GetSysColorBrush(COLOR_HIGHLIGHT);
            text_clr = GetSysColor(COLOR_HIGHLIGHTTEXT);
        } else if (expflags == CW_EXPSTATE_EXPIRED) {
            hbr = tbl->hb_hdr_bg_exp;
        } else if (expflags == CW_EXPSTATE_WARN) {
            hbr = tbl->hb_hdr_bg_warn;
        } else if (expflags == CW_EXPSTATE_CRITICAL) {
            hbr = tbl->hb_hdr_bg_crit;
        } else if (count > 0) {
            hbr = tbl->hb_hdr_bg_cred;
        } else {
            hbr = tbl->hb_hdr_bg;
        }

        FillRect(lpd->hDC, &lpd->rcItem, hbr);

        SetBkMode(lpd->hDC, TRANSPARENT);

        old_clr = SetTextColor(lpd->hDC, text_clr);

        StringCchLength(cap, KHUI_MAXCCH_NAME, &len);

        GetTextExtentPoint32(lpd->hDC, cap, (int) len, &sz);
        margin = sz.cy / 4;

        TextOut(lpd->hDC, lpd->rcItem.left + margin * 2 + GetSystemMetrics(SM_CXSMICON),
                lpd->rcItem.top + margin, cap, (int) len);

        SetTextColor(lpd->hDC, old_clr);

        kcdb_identity_get_flags(ident, &idflags);

        if (idflags & KCDB_IDENT_FLAG_DEFAULT) {
            HICON hic;

            hic = (HICON) LoadImage(khm_hInstance, MAKEINTRESOURCE(IDI_ENABLED),
                                    IMAGE_ICON,
                                    GetSystemMetrics(SM_CXSMICON),
                                    GetSystemMetrics(SM_CYSMICON),
                                    LR_DEFAULTCOLOR);
            if (hic) {
                DrawIconEx(lpd->hDC,
                           lpd->rcItem.left + margin,
                           lpd->rcItem.top + margin,
                           hic,
                           GetSystemMetrics(SM_CXSMICON),
                           GetSystemMetrics(SM_CYSMICON),
                           0,
                           hbr,
                           DI_NORMAL);
                DestroyIcon(hic);
            }
        }
    }

#endif
}
