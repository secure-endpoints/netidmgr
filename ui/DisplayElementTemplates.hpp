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

#pragma once

namespace nim {

    // Applies to DisplayElement
    template<class T = DisplayElement>
    class NOINITVTABLE WithColumnAlignment : public T {
    protected:
        int col_idx;
        int col_span;

    public:
        WithColumnAlignment() { col_idx = -1; col_span = 0; }

        WithColumnAlignment(int idx, int span = 1) {
            col_idx = idx;
            col_span = span;
        }

        ~WithColumnAlignment() { }

        virtual void UpdateLayoutPre(Graphics & g, Rect & layout) {
            if (owner == NULL || col_idx < 0 || (unsigned int) col_idx >= owner->columns.size()) {
                visible = false;
                return;
            }

            origin = owner->MapToDescendant(TQPARENT(this), Point(owner->columns[col_idx]->x, 0));
            origin.Y = 0;

            extents.Width = 0;
            extents.Height = 0;

            for (int i = col_idx;
                (col_span > 0 && i < col_idx + col_span) ||
                (col_span <= 0 && (unsigned int) i < owner->columns.size()); i++) {
                    extents.Width += owner->columns[i]->width;
            }

            layout.Width = extents.Width;
            layout.X = origin.X;
        }

        virtual void UpdateLayoutPost(Graphics& g, const Rect& layout) {}

        void SetColumnAlignment(int idx, int span = 1) {
            col_idx = idx;
            col_span = span;
            MarkForExtentUpdate();
        }
    };

    // Applies to DisplayElement
    template<class T = DisplayElement>
    class NOINITVTABLE WithTextDisplay : public T {
    protected:
        Point caption_pos;
        bool  truncated;

    protected:
        virtual void UpdateLayoutPost(Graphics& g, const Rect& bounds) {
            std::wstring caption = GetCaption();

            StringFormat sf;
            GetStringFormat(sf);

            SizeF sz((REAL) bounds.Width, (REAL) bounds.Height);
            SizeF szr;

            if (caption.length() == 0) {
                extents.Width = 0;
                extents.Height = 0;
                return;
            }

            g.MeasureString(caption.c_str(), (int) caption.length(), GetFont(g), sz, &sf, &szr);

            extents.Width = (INT) szr.Width + 1;
            extents.Height = (INT) szr.Height + 1;
        }

        virtual void PaintSelf(Graphics& g, const Rect& bounds, const Rect& clip) {
            std::wstring caption = GetCaption();

            if (caption.length() == 0)
                return;

            StringFormat sf;
            GetStringFormat(sf);

            RectF rf((REAL) bounds.X, (REAL) bounds.Y, (REAL) bounds.Width, (REAL) bounds.Height);
            Color fgc = GetForegroundColor();
            SolidBrush br(fgc);
            g.DrawString(caption.c_str(), (int) caption.length(), GetFont(g), rf, &sf, &br);

            RectF bb(0,0,0,0);
            INT   chars;
            g.MeasureString(caption.c_str(), (int) caption.length(), GetFont(g), rf, &sf, &bb, &chars);
            caption_pos.X = (INT) bb.X - bounds.X;
            caption_pos.Y = (INT) bb.Y - bounds.Y;
            truncated = ((unsigned int) chars != caption.length());

            if (bb.Width > extents.Width) {
                MarkForExtentUpdate();
                Invalidate();
            }
        }

        virtual bool OnShowToolTip(std::wstring& _caption, Rect& align_rect) {
            if (!truncated)
                return false;

            Point pto = MapToScreen(caption_pos);
            align_rect.X = pto.X; align_rect.Y = pto.Y;
            align_rect.Width = extents.Width; align_rect.Height = extents.Height;
            _caption = GetCaption();
            return true;
        }

    public:
        WithTextDisplay() {
            truncated = false;
        }

        virtual std::wstring GetCaption() {
            return std::wstring(L"");
        }

        virtual void GetStringFormat(StringFormat& sf) {
            sf.SetFormatFlags(StringFormatFlagsNoWrap);
            sf.SetTrimming(StringTrimmingEllipsisCharacter);
            sf.SetLineAlignment(StringAlignmentCenter);
        }
    };

    // Applies to WithTextDisplay<>
    template<class T = WithTextDisplay<> >
    class NOINITVTABLE WithStaticCaption : public T {
        std::wstring static_caption;

    public:
        virtual std::wstring GetCaption() {
            return static_caption;
        }

        void SetCaption(const std::wstring& _caption) {
            static_caption = _caption;
            MarkForExtentUpdate();
            Invalidate();
        }
    };

    // Applies to WithTextDisplay
    template <class T = WithTextDisplay< > >
    class NOINITVTABLE WithCachedFont : public T {
        Font *cached_font;

    public:
        WithCachedFont() {
            cached_font = NULL;
        }

        ~WithCachedFont() {
            if (cached_font)
                delete cached_font;
            cached_font = NULL;
        }

    public:
        virtual Font* GetFont(Graphics& g) {
            if (cached_font == NULL) {
                HDC hdc = g.GetHDC();
                cached_font = GetFontCreate(hdc);
                g.ReleaseHDC(hdc);
            }

            return cached_font;
        }

        virtual Font* GetFontCreate(HDC hdc) = 0;
    };

    // Applies to DisplayElement
    template<class T = DisplayElement>
    class NOINITVTABLE WithTabStop : public T {

        virtual bool IsTabStop() {
            return true;
        }

        virtual void OnClick(const Point& pt, UINT keyflags, bool doubleClick) {
            if (owner)
                owner->OnChildClick(this, pt, keyflags, doubleClick);
            else
                T::OnClick(pt, keyflags, doubleClick);
        }
    };

    // Applies to DisplayElement
    template<class T = DisplayElement>
    class NOINITVTABLE WithOutline : public T {
    public:
        WithOutline() { expandable = true; expanded = true; }

        void InsertOutlineAfter(DisplayElement * e, DisplayElement * previous = NULL) {
            T::InsertChildAfter(e, previous);
            e->is_outline = true;
            e->visible = expanded;
        }

        void InsertOutlineBefore(DisplayElement * e, DisplayElement * next = NULL) {
            T::InsertChildBefore(e, next);
            e->is_outline = true;
            e->visible = expanded;
        }
    };

    // Applies to DisplayElement
    template <class T = DisplayElement>
    class NOINITVTABLE WithVerticalLayout : public T {
        virtual void UpdateLayoutPost(Graphics & g, const Rect & layout) {
            DisplayElement * c;
            Point p(0,0);
            Size ext(0,0);

            T::UpdateLayoutPost(g, layout);

            for (c = TQFIRSTCHILD(this); c; c = TQNEXTSIBLING(c)) 
                if (c->visible) {
                    c->origin.Y = p.Y;
                    ext.Height += c->extents.Height;
                    p.Y = ext.Height;
                    if (ext.Width < c->extents.Width)
                        ext.Width = c->extents.Width;
                }

            extents = ext;
        }
    };

    // Applies to DisplayElement
    template <class T = DisplayElement>
    class NOINITVTABLE WithFixedSizePos : public T {
        Point fixed_origin;
        Size  fixed_extents;
    public:
        WithFixedSizePos(const Point& p, const Size& s) : fixed_origin(p), fixed_extents(s) {}
        WithFixedSizePos() {}

        void SetSize(const Size& s) {
            fixed_extents = s;
            MarkForExtentUpdate();
        }

        void SetPosition(const Point& p) {
            fixed_origin = p;
            MarkForExtentUpdate();
        }

        virtual void UpdateLayoutPost(Graphics & g, const Rect & layout) {
            origin = fixed_origin;
            extents = fixed_extents;
        }
    };
}
