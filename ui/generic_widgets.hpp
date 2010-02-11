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


/*! \brief Button element
 */
template<void (KhmDraw::*DF)(Graphics&, const Rect&, DrawState)>
class ButtonElement : public WithFixedSizePos< DisplayElement > {
public:
    ButtonElement(const Point& _p, const Size& _s = g_theme->sz_icon_sm) {
        SetPosition(_p);
        SetSize(_s);
    }

    ButtonElement() {}

    virtual bool IsChecked() { return false; }

    virtual void PaintSelf(Graphics &g, const Rect& bounds, const Rect& clip) {
        (g_theme->*DF)(g, bounds,
                       (DrawState)
                       (((IsChecked())?
                         DrawStateChecked : DrawStateNone) |
                        ((highlight)?
                         DrawStateHotTrack : DrawStateNone)));
    }

    virtual void OnMouse(const Point& p, UINT keyflags) {
        bool h = highlight;
        __super::OnMouse(p, keyflags);
        if (!h)
            Invalidate();
    }

    virtual void OnMouseOut(void) {
        __super::OnMouseOut();
        Invalidate();
    }
};


/*! \brief Expose control

  Displays the [+] or [-] image buttons depending on whether the
  parent element is in an expanded state or not.
*/
class ExposeControlElement : public ButtonElement< &KhmDraw::DrawCredWindowOutlineWidget > {
public:

    ExposeControlElement(const Point& _p) : ButtonElement(_p) {}

    ExposeControlElement() : ButtonElement() {}

    void OnClick(const Point& p, UINT keyflags, bool doubleClick) {
        TQPARENT(this)->Expand(!TQPARENT(this)->expanded);
        owner->Invalidate();
    }

    bool IsChecked() {
        return TQPARENT(this)->expanded;
    }
};



/*! \brief Icon display control

  Displays a static icon.
*/
class IconDisplayElement : public WithFixedSizePos< DisplayElement > {
    Bitmap *i;
    bool large;
public:
    IconDisplayElement(const Point& _p, HICON _icon, bool _large) :
        i(NULL), large(_large) {
        SetPosition(_p);
        SetSize((large)? g_theme->sz_icon : g_theme->sz_icon_sm);

        if (_icon)
            i = GetBitmapFromHICON(_icon);
    }

    void SetIcon(HICON _icon, bool redraw = true) {
        if (i)
            delete i;

        i = GetBitmapFromHICON(_icon);

        if (redraw)
            Invalidate();
    }

    virtual void PaintSelf(Graphics &g, const Rect& bounds, const Rect& clip) {
        if (i)
            g.DrawImage(i, bounds.X, bounds.Y, bounds.Width, bounds.Height);
    }
};



/*! \brief Progress bar control
 */
class ProgressBarElement :
        public DisplayElement {

    int progress;

public:
    ProgressBarElement() {}

    void SetProgress(int _progress) {
        progress = _progress;
        Invalidate();
    }

    void PaintSelf(Graphics& g, const Rect& bounds, const Rect& clip) {
        g_theme->DrawProgressBar(g, bounds, progress);
    }

    virtual void UpdateLayoutPost(Graphics& g, const Rect& bounds) {
        extents.Width = bounds.Width;
        extents.Height = g_theme->sz_icon_sm.Height;
    }
};


/*! \brief Generic Static (Text) element
 */
class StaticTextElement :
        public GenericTextT< WithStaticCaption < WithTextDisplay <> > > {
public:
    StaticTextElement(const std::wstring& _caption) {
        SetCaption(_caption);
    }
};

}
