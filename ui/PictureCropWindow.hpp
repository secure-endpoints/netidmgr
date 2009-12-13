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

#include "khmapp.h"
#include "envvar.h"
#include <assert.h>

namespace nim {

    class PictureCropWindow :
        virtual public ControlWindow {

        Image   *m_image;
        Bitmap  *m_buffer;
        RectF    m_crop;        // In image coordinates
        RectF    m_source;      // In image coordinates
        RectF    m_dest;        // In client coordinates
        RectF    m_p_small;     // In client coordinates
        RectF    m_p_large;     // In client coordinates
        SizeF    m_min;

        bool     m_dragging;
        Point    m_dragfrom;

        enum Corner {
            None,
            TopLeft,
            TopRight,
            BottomLeft,
            BottomRight,
            Interior
        } m_highlight;

    public:
        PictureCropWindow() :
            ControlWindow(),
            m_image(NULL),
            m_crop(0,0,0,0),
            m_dest(0,0,0,0),
            m_source(0,0,0,0),
            m_highlight(None),
            m_dragging(false),
            m_buffer(NULL)
        {
        }

        ~PictureCropWindow() {
            if (m_image)
                delete m_image;
            if (m_buffer)
                delete m_buffer;
        }

        void ResetCropBounds() {
            assert(m_image);

            INT w = m_image->GetWidth();
            INT h = m_image->GetHeight();
            INT d = min(w,h);
            INT x = ((w > d)? (w - d) / 2: 0);
            INT y = ((h > d)? (h - d) / 2: 0);
            Rect client;

            assert(w > 0 && h > 0);

            GetClientRect(&client);
            client.Height -= g_theme->sz_icon.Height + g_theme->sz_margin.Height * 2;

            m_crop = RectF((REAL) x, (REAL) y, (REAL) d, (REAL) d);
            m_source = RectF(0, 0, (REAL) w, (REAL) h);
            if ((h * client.Width) / w > client.Height) {
                m_dest.Width = (w * (REAL) client.Height) / h;
                m_dest.Height = (REAL) client.Height;
                m_dest.Y = 0;
                m_dest.X = (client.Width - (REAL) m_dest.Width) / 2;
            } else {
                m_dest.Height = (h * (REAL) client.Width) / w;
                m_dest.Width = (REAL) client.Width;
                m_dest.X = 0;
                m_dest.Y = (client.Height - (REAL) m_dest.Height) / 2;
            }

            m_p_small.X = (REAL) (client.GetRight() - (g_theme->sz_margin.Width * 3 +
                                                       g_theme->sz_icon.Width +
                                                       g_theme->sz_icon_sm.Width));
            m_p_small.Y = (REAL) (client.GetBottom() + (g_theme->sz_margin.Height +
                                                        g_theme->sz_icon.Height -
                                                        g_theme->sz_icon_sm.Height));
            m_p_small.Width = (REAL) g_theme->sz_icon_sm.Width;
            m_p_small.Height = (REAL) g_theme->sz_icon_sm.Height;

            m_p_large.X = (REAL) (client.GetRight() - (g_theme->sz_margin.Width +
                                                       g_theme->sz_icon.Width));
            m_p_large.Y = (REAL) (client.GetBottom() + g_theme->sz_margin.Height);
            m_p_large.Width = (REAL) g_theme->sz_icon.Width;
            m_p_large.Height = (REAL) g_theme->sz_icon.Height;

            m_min.Width = (REAL) g_theme->sz_icon_sm.Width;
            m_min.Height = (REAL) g_theme->sz_icon_sm.Height;
        }

        RectF ImageToClient(RectF r) {
            REAL scale = m_dest.Width / m_source.Width;

            r.X = m_dest.X + r.X * scale;
            r.Y = m_dest.Y + r.Y * scale;
            r.Width *= scale;
            r.Height *= scale;

            return r;
        }

        RectF ClientToImage(RectF r) {
            REAL scale = m_source.Width / m_dest.Width;

            r.X = (r.X - m_dest.X) * scale;
            r.Y = (r.Y - m_dest.Y) * scale;
            r.Width *= scale;
            r.Height *= scale;

            return r;
        }

        void SetImage(Image * _image, bool own = false) {
            if (m_image)
                delete m_image;
            if (own)
                m_image = _image;
            else
                m_image = _image->Clone();
            ResetCropBounds();
            if (hwnd) {
                Invalidate();
            }
        }

        void SetImage(HICON icon) {
            Bitmap * i = new Bitmap(icon);

            SetImage(i);
            delete i;
        }

        void SetImage(const wchar_t * path) {
            Image i(path);

            if (i.GetLastStatus() == Ok) {
                SetImage(&i);
            }
        }

        bool LoadImage(ConfigSpace& cfg) {
            std::wstring original = cfg.GetString(L"IconOriginal");

            if (KHM_FAILED(cfg.GetLastError()) || original.size() == 0)
                return false;

            {
                wchar_t exp_original[MAX_PATH];
                DWORD d;

                d = ExpandEnvironmentStrings(original.c_str(), exp_original, ARRAYLENGTH(exp_original));
                if (d == 0 || d >= ARRAYLENGTH(exp_original))
                    return false;

                SetImage(exp_original);
                if (m_image == NULL || m_image->GetLastStatus() != Ok)
                    return false;
            }

            RectF r_crop(0,0,0,0);

            cfg.GetObject(L"IconCrop", r_crop);

            if (KHM_SUCCEEDED(cfg.GetLastError()) && m_source.Contains(r_crop))
                m_crop = r_crop;

            return true;
        }

        bool SaveImage(ConfigSpace& cfg, const wchar_t * opath) {
            wchar_t path[MAX_PATH];
            CLSID bmp_clsid;

            if (!GetEncoderClsid(L"image/bmp", &bmp_clsid)) {
                _reportf_ex(KHERR_ERROR, KHERR_FACILITY, KHERR_FACILITY_ID, khm_hInstance,
                            L"Internal error : Bitmap encoder not found.");
                return false;
            }

            {
                if (FAILED(StringCchPrintf(path, ARRAYLENGTH(path), L"%s-full.bmp", opath)))
                    return false;
                m_image->Save(path, &bmp_clsid, NULL);
                unexpand_env_var_prefix(path, sizeof(path));
                cfg.Set(L"IconOriginal", path);
                cfg.SetObject(L"IconCrop", m_crop);
            }

            {
                Bitmap imgx64(64, 64, PixelFormat32bppARGB);

                {
                    Graphics g(&imgx64);
                    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
                    g.DrawImage(m_image, RectF(0, 0, 64, 64), m_crop, UnitPixel, NULL);
                }

                if (FAILED(StringCchPrintf(path, ARRAYLENGTH(path), L"%s-64.bmp", opath)))
                    return false;
                imgx64.Save(path, &bmp_clsid, NULL);
                unexpand_env_var_prefix(path, sizeof(path));

                {
                    wchar_t respath[MAX_PATH];

                    StringCchPrintf(respath, ARRAYLENGTH(respath), KHUI_PREFIX_IMG L"%s", path);
                    cfg.Set(L"IconNormal", respath);
                }
            }

            return KHM_SUCCEEDED(cfg.GetLastError());
        }

        void OnPaint(Graphics& _g, const Rect& clip) {
            Rect r;

            GetClientRect(&r);

            if (m_buffer == NULL)
                m_buffer = new Bitmap(r.Width, r.Height, &_g);

            Graphics g(m_buffer);

            g_theme->DrawImageNeutralBackground(g, r);

            g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

            g.DrawImage(m_image, m_dest, m_source, UnitPixel, (ImageAttributes *) NULL);

            RectF rcrop = ImageToClient(m_crop);

            {
                SolidBrush tintbrush(g_theme->c_tint);
                Region totint(r);

                totint.Exclude(rcrop);
                g.FillRegion(&tintbrush, &totint);
            }

            {
                Pen p(Color::White);

                g.DrawRectangle(&p, rcrop);
            }

            g.DrawImage(m_image, m_p_small, m_crop, UnitPixel, (ImageAttributes *) NULL);
            g.DrawImage(m_image, m_p_large, m_crop, UnitPixel, (ImageAttributes *) NULL);

            _g.DrawImage(m_buffer, 0, 0);
        }

        void OnLButtonDown(bool doubleClick, const Point& p, UINT keyflags) {
            if (!m_dragging) {
                m_dragging = true;
                SetCapture(hwnd);

                RECT r;
                ::GetClientRect(hwnd, &r);
                r.bottom -= g_theme->sz_icon.Height + g_theme->sz_margin.Height * 2;
                MapWindowRect(hwnd, NULL, &r);
                ClipCursor(&r);

                m_dragfrom = p;
            }
        }

        void OnLButtonUp(const Point& p, UINT keyflags) {
            if (m_dragging) {
                ReleaseCapture();
                ClipCursor(NULL);
                m_dragging = false;
            }
        }

        void OnMouseDrag(const Point& ip, UINT keyflags) {
            RectF rcrop = ImageToClient(m_crop);
            PointF p((REAL) ip.X, (REAL) ip.Y);
            REAL l = rcrop.GetLeft(), t = rcrop.GetTop(), r = rcrop.GetRight(), b = rcrop.GetBottom();

            switch (m_highlight) {
            case TopLeft:
                l = p.X; t = p.Y;

                if (l < m_dest.GetLeft()) l = m_dest.GetLeft();
                if (l < r - m_dest.Width) l = r - m_dest.Width;
                if (l > r - m_min.Width) l = r - m_min.Width;

                if (t < m_dest.GetTop()) t = m_dest.GetTop();
                if (t < b - m_dest.Height) t = b - m_dest.Height;
                if (t > b - m_min.Height) t = b - m_dest.Height;

                if (r - l < b - t)
                    t = b - (r - l);
                else
                    l = r - (b - t);
                break;

            case TopRight:
                r = p.X; t = p.Y;

                if (r > m_dest.GetRight()) r = m_dest.GetRight();
                if (r > l + m_dest.Width) r = l + m_dest.Width;
                if (r < l + m_min.Width) r = l + m_min.Width;

                if (t < m_dest.GetTop()) t = m_dest.GetTop();
                if (t < b - m_dest.Height) t = b - m_dest.Height;
                if (t > b - m_min.Height) t = b - m_dest.Height;

                if (r - l < b - t)
                    t = b - (r - l);
                else
                    r = l + (b - t);
                break;

            case BottomLeft:
                l = p.X; b = p.Y;

                if (l < m_dest.GetLeft()) l = m_dest.GetLeft();
                if (l < r - m_dest.Width) l = r - m_dest.Width;
                if (l > r - m_min.Width) l = r - m_min.Width;

                if (b > m_dest.GetBottom()) b = m_dest.GetBottom();
                if (b > t + m_dest.Height) b = t + m_dest.Height;
                if (b < t + m_min.Height)  b = t + m_min.Height;

                if (r - l < b - t)
                    b = t + (r - l);
                else
                    l = r - (b - t);
                break;

            case BottomRight:
                r = p.X; b = p.Y;

                if (r > m_dest.GetRight()) r = m_dest.GetRight();
                if (r > l + m_dest.Width) r = l + m_dest.Width;
                if (r < l + m_min.Width) r = l + m_min.Width;

                if (b > m_dest.GetBottom()) b = m_dest.GetBottom();
                if (b > t + m_dest.Height) b = t + m_dest.Height;
                if (b < t + m_min.Height)  b = t + m_min.Height;

                if (r - l < b - t)
                    b = t + (r - l);
                else
                    r = l + (b - t);
                break;

            case Interior:
                {
                    Point dt = ip - m_dragfrom;
                    PointF d((REAL) dt.X, (REAL) dt.Y);

                    if (r + d.X > m_dest.GetRight()) d.X = m_dest.GetRight() - r;
                    if (l + d.X < m_dest.GetLeft()) d.X = m_dest.GetLeft() - l;
                    if (t + d.Y < m_dest.GetTop()) d.Y = m_dest.GetTop() - t;
                    if (b + d.Y > m_dest.GetBottom()) d.Y = m_dest.GetBottom() - b;

                    l += d.X;
                    r += d.X;
                    t += d.Y;
                    b += d.Y;

                    m_dragfrom = ip;
                }
                break;

            default:
                return;
            }

            m_crop = ClientToImage(RectF(l, t, r - l, b - t));
            Invalidate();
        }

        void OnMouseMoveNoDrag(const Point& ip, UINT keyflags) {
            RectF rcrop = ImageToClient(m_crop);
            PointF p((REAL) ip.X, (REAL) ip.Y);

            bool left = (p.X <= rcrop.X + g_theme->sz_margin.Width &&
                         rcrop.X - g_theme->sz_margin.Width <= p.X);

            bool top = (p.Y <= rcrop.Y + g_theme->sz_margin.Height &&
                        rcrop.Y - g_theme->sz_margin.Height <= p.Y);

            bool right = (p.X <= rcrop.X + rcrop.Width + g_theme->sz_margin.Width &&
                          (rcrop.X + rcrop.Width) - g_theme->sz_margin.Width <= p.X);

            bool bottom = (p.Y <= rcrop.Y + rcrop.Height + g_theme->sz_margin.Height &&
                           (rcrop.Y + rcrop.Height) - g_theme->sz_margin.Height <= p.Y);

            enum Corner hnew;

            if (left && top)
                hnew = TopLeft;
            else if (top && right)
                hnew = TopRight;
            else if (bottom && left)
                hnew = BottomLeft;
            else if (bottom && right)
                hnew = BottomRight;
            else if (rcrop.Contains((REAL) p.X, (REAL) p.Y))
                hnew = Interior;
            else
                hnew = None;

            if (hnew != m_highlight)
                m_highlight = hnew;
        }

        void OnMouseMove(const Point& ip, UINT keyflags) {
            if (m_dragging) {
                OnMouseDrag(ip, keyflags);
            } else {
                OnMouseMoveNoDrag(ip, keyflags);
            }
        }

        BOOL OnSetCursor(HWND hwndCursor, UINT codeHitTest, UINT msg)
        {
            LPTSTR id = NULL;

            switch (m_highlight) {
            case BottomRight:
            case TopLeft:
                id = IDC_SIZENWSE;
                break;

            case TopRight:
            case BottomLeft:
                id = IDC_SIZENESW;
                break;

            case Interior:
                id = IDC_SIZEALL;
                break;

            default:
                id = IDC_ARROW;
            }

            if (id != NULL) {
                HCURSOR hc;
                hc = (HCURSOR) ::LoadImage(NULL, id, IMAGE_CURSOR, 0, 0,
                                           LR_DEFAULTSIZE|LR_DEFAULTCOLOR|LR_SHARED);
                if (hc != NULL)
                    SetCursor(hc);

                return TRUE;
            }

            return FALSE;
        }

        DWORD GetStyle() {
            return WS_CHILD | WS_VISIBLE;
        }
    };
}
