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

    static const INT FOCUS_RECT_OPACITY         = 64;

    static const INT SELECTION_OPACITY          = 64;

    static const INT SELECTION_OPACITY_END      = 0;

    static const INT OUTLINE_OPACITY            = 64;

    static const INT OUTLINE_OPACITY_END        = 0;

    static const INT OUTLINE_BORDER_OPACITY     = 96;

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
        c_normal       .SetFromCOLORREF(khm_get_element_color(KHM_CLR_HEADER));
        c_warning      .SetFromCOLORREF(khm_get_element_color(KHM_CLR_HEADER_WARN));
        c_critical     .SetFromCOLORREF(khm_get_element_color(KHM_CLR_HEADER_CRIT));
        c_expired      .SetFromCOLORREF(khm_get_element_color(KHM_CLR_HEADER_EXP));
        c_empty = c_background;
        c_text         .SetFromCOLORREF(khm_get_element_color(KHM_CLR_TEXT));
        c_text_selected.SetFromCOLORREF(khm_get_element_color(KHM_CLR_TEXT_SEL));

        b_watermark =   Bitmap::FromResource(khm_hInstance, MAKEINTRESOURCE(IDB_LOGO_SHADE));
        b_credwnd =     LoadImageResourceAsStream(MAKEINTRESOURCE(IDB_CREDWND_IMAGELIST), L"PNG");
        b_meter_state = LoadImageResourceAsStream(MAKEINTRESOURCE(IDB_BATT_STATE), L"PNG");
        b_meter_life  = LoadImageResourceAsStream(MAKEINTRESOURCE(IDB_BATT_LIFE),  L"PNG");
        b_meter_renew = LoadImageResourceAsStream(MAKEINTRESOURCE(IDB_BATT_RENEW), L"PNG");
        b_progress    = dynamic_cast<Bitmap*>
            (LoadImageResourceAsStream(MAKEINTRESOURCE(IDB_PROGRESS),   L"PNG"));

        sz_icon.Width     = GetSystemMetrics(SM_CXICON);
        sz_icon.Height    = GetSystemMetrics(SM_CYICON);
        sz_icon_sm.Width  = GetSystemMetrics(SM_CXSMICON);
        sz_icon_sm.Height = GetSystemMetrics(SM_CYSMICON);
        sz_margin.Width   = sz_icon_sm.Width / 4;
        sz_margin.Height  = sz_icon_sm.Height / 4;

        sz_meter.Width = 32;
        sz_meter.Height = 16;

        line_thickness = 1;

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
#define DEL_BITMAP(b) if (b) { delete b; b = NULL; }

        DEL_BITMAP(b_credwnd);
        DEL_BITMAP(b_watermark);
        DEL_BITMAP(b_meter_state);
        DEL_BITMAP(b_meter_life);
        DEL_BITMAP(b_meter_renew);
        DEL_BITMAP(b_progress);

#undef  DEL_BITMAP

        isThemeLoaded = FALSE;
    }

    void KhmDraw::DrawImageByIndex(Graphics & g, Image * img, const Point& p, INT idx, const Size& sz)
    {
        if (idx < 0 || idx * img->GetHeight() > img->GetWidth())
            return;

        g.DrawImage(img, p.X, p.Y,
                    sz.Width * idx, 0,
                    sz.Width, sz.Height,
                    UnitPixel);
    }

    void
    KhmDraw::DrawCredWindowImage(Graphics & g, CredWndImages img, const Point& p)
    {
        int idx = static_cast<int>(img);

        DrawImageByIndex(g, b_credwnd, p, idx - 1, sz_icon_sm);
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

    void
    KhmDraw::DrawFocusRect(Graphics& g, const Rect& extents)
    {
        Rect fr = extents;

        // Fix off by one problem.  Windows GDI calls including Window
        // management functions rectangles differently from GDI+.  In
        // GDI+ a rectangle drawn at (0,0) that's 100x100 pixels in
        // dimension will have it's right edge at the 100th column,
        // while WINGDI and WINUSER will expect the rightmost edge to
        // be at the 99th column.

        fr.Width -= 1;
        fr.Height -= 1;

        fr.Inflate(-sz_margin.Width / 2, -sz_margin.Height / 2);

        Pen p(c_text * FOCUS_RECT_OPACITY);
        p.SetDashStyle(DashStyleDot);

        g.DrawRectangle(&p, fr);
    }

    void 
    KhmDraw::DrawCredWindowOutline(Graphics& g, const Rect& extents, DrawState state)
    {
        Color c1;
        Rect r = extents;

        if (state & DrawStateSelected)
            c1 = c_selection;
        else if (state & DrawStateExpired)
            c1 = c_expired;
        else if (state & DrawStateCritial)
            c1 = c_critical;
        else if (state & DrawStateWarning)
            c1 = c_warning;
        else
            c1 = c_normal;

        r.Width -= 1;
        r.Height -= 1;

        {
            Pen p_frame(c1 * OUTLINE_BORDER_OPACITY, 1.0);
            g.DrawRectangle(&p_frame, r);
        }

        r.Inflate(-sz_margin.Width / 2, -sz_margin.Height / 2);

        {
            LinearGradientBrush br(extents,
                                   c1 * OUTLINE_OPACITY,
                                   c1 * OUTLINE_OPACITY_END, 0, FALSE);
            g.FillRectangle(&br, r);
        }

        if (state & DrawStateFocusRect)
            DrawFocusRect(g, extents);
    }

    void
    KhmDraw::DrawProgressBar(Graphics& g, const Rect& extents, int progress)
    {
        INT partition;
        const INT width = extents.Width - 1;
        const INT cel_cx = sz_icon_sm.Width;
        const INT cel_cy = sz_icon_sm.Height;
        const INT X = extents.X;
        const INT Y = extents.Y;

        static const INT
            BEGIN_FILLED = 0,
            MID_FILLED = 1,
            END_FILLED = 2,
            BEGIN_EMPTY = 3,
            MID_EMPTY = 4,
            END_EMPTY = 5;

        if (progress < 0)
            progress = 0;
        if (progress > 256)
            progress = 256;

        partition = width * progress / 256;

        if (partition > 0) {
            g.DrawImage(b_progress, X, Y,
                        BEGIN_FILLED * cel_cx, 0,
                        __min(cel_cx, partition),
                        cel_cy, UnitPixel);
        }

        if (partition > cel_cx) {
            for (INT x = cel_cx;
                 x < partition && x < width - cel_cx;
                 x += cel_cx) {
                g.DrawImage(b_progress,
                            X + x, Y,
                            MID_FILLED * cel_cx, 0,
                            __min(cel_cx, partition - x),
                            cel_cy,
                            UnitPixel);
            }
        }

        if (partition > width - cel_cx) {
            g.DrawImage(b_progress,
                        X + width - cel_cx, Y,
                        END_FILLED * cel_cx, 0,
                        partition - (width - cel_cx),
                        cel_cy, UnitPixel);
        }

        if (partition < cel_cx) {
            g.DrawImage(b_progress,
                        X + partition, Y,
                        BEGIN_EMPTY * cel_cx + partition, 0,
                        cel_cx - partition, cel_cy, UnitPixel);
            partition = cel_cx;
        }

        if (partition < width - cel_cx) {
            for (INT x = width - cel_cx * 2;
                 x > partition - cel_cx;
                 x -= cel_cx) {
                g.DrawImage(b_progress,
                            __max(partition, x) + X, Y,
                            MID_EMPTY * cel_cx +
                            __max(0, partition - x), 0,
                            __min(x + cel_cx - partition, cel_cx),
                            cel_cy,
                            UnitPixel);
            }
            partition = width - cel_cx;
        }

        if (partition < width) {
            g.DrawImage(b_progress,
                        X + partition, Y,
                        END_EMPTY * cel_cx + partition
                        - (width - cel_cx), 0,
                        width - partition, cel_cy, UnitPixel);
        }
    }

    void 
    KhmDraw::DrawCredWindowOutlineWidget(Graphics& g, const Rect& extents, DrawState state)
    {
        CredWndImages image;

        image = ((state & DrawStateChecked)?
            ((state & DrawStateHotTrack)? ImgCollapseHi : ImgCollapse) :
            ((state & DrawStateHotTrack)? ImgExpandHi : ImgExpand));

        DrawCredWindowImage(g, image, Point(extents.X, extents.Y));
    }
    
    void
    KhmDraw::DrawStarWidget(Graphics& g, const Rect& extents, DrawState state)
    {
        CredWndImages image;

        image = ((state & DrawStateChecked)?
                 ((state & DrawStateHotTrack)? ImgStarHi : ImgStar) :
                 ((state & DrawStateHotTrack)? ImgStarEmptyHi : ImgStarEmpty));

        DrawCredWindowImage(g, image, Point(extents.X, extents.Y));
    }

    void 
    KhmDraw::DrawCredWindowNormalBackground(Graphics& g, const Rect& extents, DrawState state)
    {
        Color c1;
        Color c2;
        Rect r = extents;

        r.Width -= 1;
        r.Height -= 1;

        if (state & DrawStateSelected) {
            c1 = c_selection * SELECTION_OPACITY;
            c2 = c_background + c_selection * SELECTION_OPACITY_END;
        } else {
            c1 = c_background;
            c2 = c_background;
        }

        LinearGradientBrush br(r, c1, c2, 0, FALSE);

        g.FillRectangle(&br, r);

        if (state & DrawStateFocusRect)
            DrawFocusRect(g, extents);
    }

    inline void Next_Frame_And_Refresh(int _Delay, unsigned int Next_Threshold,
                                       int _Frames,
                                       int& frame, DWORD& refresh)
    {
        DWORD ticks = GetTickCount();
        int   fidx = (ticks / _Delay);
        DWORD nextin = _Delay - (ticks % _Delay);

        if (nextin < Next_Threshold) {
            nextin += _Delay;
            fidx++;
        }

        fidx %= _Frames;

        frame += fidx;
        refresh = _Delay;
    }

    void
    KhmDraw::DrawCredMeterState(Graphics& g, const Rect& extents, DrawState state, DWORD *ms_to_next)
    {
        static const int ANIMATION_DELAY  = 500;
        static const int NEXT_THRESHOLD   = 100;
        static const int ANIMATION_FRAMES = 2;

        static const int FRAME_DISABLED   = 0;
        static const int FRAME_CRIT_BEGIN = 3;
        static const int FRAME_WARN_BEGIN = 1;

        int frame;

        if (state & DrawStateWarning) {
            frame = FRAME_WARN_BEGIN;
            Next_Frame_And_Refresh(ANIMATION_DELAY, NEXT_THRESHOLD, ANIMATION_FRAMES,
                                   frame, *ms_to_next);
        } else if (state & DrawStateCritial) {
            frame = FRAME_CRIT_BEGIN;
            Next_Frame_And_Refresh(ANIMATION_DELAY, NEXT_THRESHOLD, ANIMATION_FRAMES,
                                   frame, *ms_to_next);
        } else {
            frame = FRAME_DISABLED;
            *ms_to_next = 0;
        }

        DrawImageByIndex(g, b_meter_state, Point(extents.X, extents.Y), frame, sz_meter);
    }

    void
    KhmDraw::DrawCredMeterLife(Graphics& g, const Rect& extents, unsigned int index)
    {
        index /= 32;

        if (index < 0)
            index = 0;

        if (index > 7)
            index = 7;

        DrawImageByIndex(g, b_meter_life, Point(extents.X, extents.Y), index, sz_meter);
    }

    void
    KhmDraw::DrawCredMeterBusy(Graphics& g, const Rect& extents, DWORD *ms_to_next)
    {
        static const int ANIMATION_DELAY = 200;
        static const int NEXT_THRESHOLD  = 100;
        static const int ANIMATION_FRAMES = 8;

        int frame = 0;

        Next_Frame_And_Refresh(ANIMATION_DELAY, NEXT_THRESHOLD, ANIMATION_FRAMES,
                               frame, *ms_to_next);

        DrawImageByIndex(g, b_meter_renew, Point(extents.X, extents.Y), frame, sz_meter);
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

    Image* LoadImageResourceAsStream(LPCTSTR name, LPCTSTR type, HINSTANCE inst)
    {
        Image * rimage = NULL;
        HRSRC   hres = 0;
        HGLOBAL hgres = NULL;
        HGLOBAL hgmem = NULL;
        IStream * istr = NULL;
        DWORD cb;

        hres = FindResource(inst, name, type);
        if (hres == NULL)
            goto done;

        cb = SizeofResource(inst, hres);
        if (cb == 0)
            goto done;

        hgres = LoadResource(inst, hres);
        if (hgres == NULL)
            goto done;

        void *  pv_img;
        pv_img = LockResource(hgres);
        if (pv_img == NULL)
            goto done;

        hgmem = GlobalAlloc(GMEM_MOVEABLE, cb);
        if (hgmem == NULL)
            goto done;

        void * pv_mem = GlobalLock(hgmem);
        if (pv_mem == NULL)
            goto done;

        memcpy(pv_mem, pv_img, cb);

        GlobalUnlock(hgmem);

        if (S_OK != CreateStreamOnHGlobal(hgmem, TRUE, &istr))
            goto done;

        rimage = Bitmap::FromStream(istr);

    done:
        if (istr != NULL)
            istr->Release();

        if (rimage == NULL && hgmem != NULL)
            GlobalFree(hgmem);

        return rimage;
    }

    KhmDraw * g_theme = NULL;
    ULONG_PTR gdiplus_token;

    extern "C" void khm_init_drawfuncs(void)
    {
        if (g_theme == NULL) {
            GdiplusStartupInput gdip_inp;

            GdiplusStartup(&gdiplus_token, &gdip_inp, NULL);

            g_theme = new KhmDraw;

            g_theme->LoadTheme();
        }
    }

    extern "C" void khm_exit_drawfuncs(void)
    {
        if (g_theme != NULL) {
            delete g_theme;
            g_theme = NULL;

            GdiplusShutdown(gdiplus_token);
        }
    }

    DrawState GetIdentityDrawState(Identity& identity)
    {
        DrawState ds = DrawStateNone;

        do {
            if (identity.GetAttribInt32(KCDB_ATTR_N_IDCREDS) == 0) {

                ds = DrawStateDisabled;

            } else if (identity.Exists(KCDB_ATTR_EXPIRE)) {
                khm_int64 expire = identity.GetAttribFileTimeAsInt(KCDB_ATTR_EXPIRE);
                khm_int64 now;

                FILETIME ft;

                GetSystemTimeAsFileTime(&ft);
                now = FtToInt(&ft) + SECONDS_TO_FT(TT_TIMEEQ_ERROR_SMALL);

                if (now > expire) {
                    ds = DrawStateExpired;
                    break;
                }

                khm_int64 thr_crit = identity.GetAttribFileTimeAsInt(KCDB_ATTR_THR_CRIT);

                if (thr_crit != 0 && now > expire - thr_crit) {
                    ds = DrawStateCritial;
                    break;
                }

                khm_int64 thr_warn = identity.GetAttribFileTimeAsInt(KCDB_ATTR_THR_WARN);

                if (thr_warn != 0 && now > expire - thr_warn) {
                    ds = DrawStateWarning;
                    break;
                }
            }
        } while (false);

        return ds;
    }

    extern "C"
    void
    khm_measure_identity_menu_item(HWND hwnd, LPMEASUREITEMSTRUCT lpm, khui_action * act)
    {
        Graphics g(hwnd);

        HDC hdc = g.GetHDC();
        Font font(hdc, (HFONT) GetStockObject(DEFAULT_GUI_FONT));
        g.ReleaseHDC(hdc);

        StringFormat sf(StringFormatFlagsNoWrap);

        sf.SetAlignment(StringAlignmentNear);
        sf.SetLineAlignment(StringAlignmentCenter);
        sf.SetTrimming(StringTrimmingEllipsisCharacter);
        
        RectF rf;
        g.MeasureString(act->caption, -1, &font, PointF(0.0, 0.0), &sf, &rf);
        
        lpm->itemWidth = (UINT)(rf.Width + g_theme->sz_margin.Width * 2 + g_theme->sz_icon_sm.Width);
        lpm->itemHeight =(UINT)(rf.Height + g_theme->sz_margin.Height * 2);
    }

    extern "C"
    void
    khm_draw_identity_menu_item(HWND hwnd, LPDRAWITEMSTRUCT lpd, khui_action * act)
    {
        Identity identity(act->data, false);

        wchar_t * cap = act->caption;

        DrawState draw_state = GetIdentityDrawState(identity);

        Color color_bgl;
        Color color_bgr;

        Color color_fg = g_theme->c_text;

        {
            Color c_key;

            if (lpd->itemState & (ODS_HOTLIGHT | ODS_SELECTED)) {
                color_fg  = g_theme->c_text_selected;
                c_key = g_theme->c_selection;
            } else if (draw_state & DrawStateExpired) {
                c_key = g_theme->c_expired;
            } else if (draw_state & DrawStateWarning) {
                c_key = g_theme->c_warning;
            } else if (draw_state & DrawStateCritial) {
                c_key = g_theme->c_critical;
            } else if (identity.GetAttribInt32(KCDB_ATTR_N_IDCREDS) > 0) {
                c_key = g_theme->c_normal;
            } else {
                c_key = g_theme->c_empty;
            }

            color_bgl = g_theme->c_background + c_key * OUTLINE_OPACITY;
            color_bgr = g_theme->c_background + c_key * OUTLINE_OPACITY_END;
        }

        Graphics g(lpd->hDC);
        Rect     r_item = RectFromRECT(&lpd->rcItem);

        {
            LinearGradientBrush lgr(r_item, color_bgl, color_bgr, LinearGradientModeHorizontal);
            g.FillRectangle(&lgr, r_item);
        }

        {
            Font font(lpd->hDC, (HFONT) GetStockObject(DEFAULT_GUI_FONT));
            StringFormat sf(StringFormatFlagsNoWrap);
            SolidBrush br(color_fg);

            sf.SetAlignment(StringAlignmentNear);
            sf.SetLineAlignment(StringAlignmentCenter);
            sf.SetTrimming(StringTrimmingEllipsisCharacter);

            int margin = g_theme->sz_icon_sm.Width + g_theme->sz_margin.Width * 2;

            RectF rf((REAL)(r_item.X + margin),
                     (REAL) r_item.Y,
                     (REAL) r_item.Width - margin,
                     (REAL) r_item.Height);

            g.DrawString(cap, -1, &font, rf, &sf, &br);
        }

        if (identity.GetFlags() & KCDB_IDENT_FLAG_DEFAULT) {
            g_theme->DrawStarWidget(g, Rect(r_item.X + g_theme->sz_margin.Width,
                                            r_item.Y + (r_item.Height - g_theme->sz_icon_sm.Height) / 2,
                                            g_theme->sz_icon_sm.Width,
                                            g_theme->sz_icon_sm.Height),
                                    (DrawState)
                                    (((lpd->itemState & ODS_HOTLIGHT) ?
                                      DrawStateHotTrack : DrawStateNone) |
                                     ((lpd->itemState & ODS_SELECTED) ?
                                      DrawStateSelected : DrawStateNone) |
                                     DrawStateChecked)
                                    );
        }
    }
}