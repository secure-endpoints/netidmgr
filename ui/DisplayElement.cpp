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
#include <assert.h>

namespace nim {

    void DisplayElement::MarkForExtentUpdate(void)
    {
        MarkChildrenForExtentUpdate();
        for (DisplayElement * p = this; p; p = TQPARENT(p)) {
            if (p->recalc_extents)
                break;
            p->recalc_extents = true;
        }
    }

    void DisplayElement::MarkChildrenForExtentUpdate(void)
    {
        for (DisplayElement * c = TQFIRSTCHILD(this); c; c = TQNEXTSIBLING(c)) 
            if (c->visible) {
                c->recalc_extents = true;
                c->MarkChildrenForExtentUpdate();
            }
    }

    void DisplayElement::InsertChildAfter(DisplayElement * e, DisplayElement * previous)
    {
        TQINSERT(this, previous, e);
        e->recalc_extents = true;
        e->visible = visible;
        e->SetOwner(owner);
        MarkForExtentUpdate();
    }

    void DisplayElement::InsertChildBefore(DisplayElement * e, DisplayElement * next)
    {
        TQINSERTP(this, next, e);
        e->recalc_extents = true;
        e->visible = visible;
        e->SetOwner(owner);
        MarkForExtentUpdate();
    }

    void DisplayElement::DeleteChild(DisplayElement * e)
    {
        NotifyDeleteChild(this, e);
        e->DeleteAllChildren();
        TQDELCHILD(this, e);
        delete e;
        MarkForExtentUpdate();
    }

    void DisplayElement::DeleteAllChildren()
    {
        NotifyDeleteAllChildren(this);
        for (DisplayElement * e = TQFIRSTCHILD(this); e; e = TQFIRSTCHILD(this)) {
            e->DeleteAllChildren();
            TQDELCHILD(this, e);
            delete e;
        }
        MarkForExtentUpdate();
    }

    void DisplayElement::MoveChildAfter(DisplayElement * e, DisplayElement * previous)
    {
        TQDELCHILD(this, e);
        TQINSERT(this, previous, e);
        e->recalc_extents = true;
        MarkForExtentUpdate();
    }

    void DisplayElement::MoveChildBefore(DisplayElement * e, DisplayElement * next)
    {
        TQDELCHILD(this, e);
        TQINSERTP(this, next, e);
        e->recalc_extents = true;
        MarkForExtentUpdate();
    }

    void DisplayElement::SetOwner(DisplayContainer * _owner)
    {
        owner = _owner;
        for (DisplayElement * e = TQFIRSTCHILD(this); e; e = TQNEXTSIBLING(e)) {
            e->SetOwner(_owner);
        }
    }

    void DisplayElement::NotifyDeleteChild(DisplayElement * _parent, DisplayElement * e)
    {
        if (owner != this) {
            owner->NotifyDeleteChild(_parent, e);
        }
    }

    void DisplayElement::NotifyDeleteAllChildren(DisplayElement * _parent)
    {
        if (owner != this) {
            owner->NotifyDeleteAllChildren(_parent);
        }
    }

    void DisplayElement::UpdateLayout(Graphics & g, const Rect & layout)
    {
        Rect t_layout;

        layout.GetBounds(&t_layout);
        UpdateLayoutPre(g, t_layout);

        for (DisplayElement * c = TQFIRSTCHILD(this); c; c = TQNEXTSIBLING(c)) {
            if (c->visible && c->recalc_extents)
                c->UpdateLayout(g, t_layout);
        }

        UpdateLayoutPost(g, t_layout);
        recalc_extents = false;
    }

    void DisplayElement::NotifyLayoutInternal(void)
    {
        NotifyLayout();

        for (DisplayElement * c = TQFIRSTCHILD(this); c; c = TQNEXTSIBLING(c)) {
            c->NotifyLayoutInternal();
        }
    }

    void DisplayElement::UpdateLayoutPre(Graphics& g, Rect& layout)
    {
    }

    void DisplayElement::UpdateLayoutPost(Graphics& g, const Rect& layout)
    {
        extents.Width = 0;
        extents.Height = 0;
        for (DisplayElement * c = TQFIRSTCHILD(this); c; c = TQNEXTSIBLING(c)) {
            if (c->origin.X + c->extents.Width > extents.Width)
                extents.Width = c->origin.X + c->extents.Width;
            if (c->origin.Y + c->extents.Height > extents.Height)
                extents.Height = c->origin.Y + c->extents.Height;
        }
    }

    void DisplayElement::Invalidate(const Rect & r)
    {
        Point pto;
        if (owner == NULL) return;

        if (owner->recalc_extents) {
            owner->Invalidate();
        } else {
            r.GetLocation(&pto);
            pto = owner->MapFromDescendant(this, pto);
            owner->Invalidate(Rect(pto, Size(r.Width, r.Height)));
        }
    }

    Point DisplayElement::MapToParent(const Point& pt)
    {
        return pt + origin;
    }

    Point DisplayElement::MapToScreen(const Point & pt)
    {
        return owner->MapToScreen(owner->MapFromDescendant(this, pt));
    }

    Point DisplayElement::MapToDescendant(const DisplayElement * e, const Point& pt)
    {
        Point cpt(pt);
        for (const DisplayElement * p = e; p && p != this; p = TQPARENT(p)) {
            cpt = cpt - p->origin;
        }
        return cpt;
    }

    Point DisplayElement::MapFromDescendant(const DisplayElement * c, const Point& pt)
    {
        Point ppt(pt);
        for (const DisplayElement * p = c; p && p != this; p = TQPARENT(p)) {
            ppt = ppt + p->origin;
        }
        return ppt;
    }

    DisplayElement * DisplayElement::ChildFromPoint(const Point& pt)
    {
        for (DisplayElement * c = TQFIRSTCHILD(this); c; c = TQNEXTSIBLING(c)) {
            if (c->visible && Rect(c->origin, c->extents).Contains(pt))
                return c;
        }
        return NULL;
    }

    DisplayElement * DisplayElement::DescendantFromPoint(const Point& pt)
    {
        Point cpt(pt);

        DisplayElement * d = ChildFromPoint(pt);
        while (d) {
            cpt = cpt - d->origin;
            DisplayElement * nd = d->ChildFromPoint(cpt);
            if (nd == NULL)
                break;
            d = nd;
        }

        return d;
    }

    void DisplayElement::OnPaint(Graphics& g, const Rect& bounds, const Rect& clip) 
    {
        PaintSelf(g, bounds, clip);

        for (DisplayElement * c = TQFIRSTCHILD(this); c; c = TQNEXTSIBLING(c)) 
            if (c->visible) {
                Rect cr(c->origin, c->extents);
                cr.Offset(bounds.X, bounds.Y);

                if (clip.IntersectsWith(cr))
                    c->OnPaint(g, cr, clip);
            }
    }

    void DisplayElement::Expand(bool expand)
    {
        if (expanded == expand) 
            return; 

        expanded = expand;

        for (DisplayElement * c = TQFIRSTCHILD(this); c; c = TQNEXTSIBLING(c))
            if (c->is_outline) {
                c->visible = expand;
            }

        MarkForExtentUpdate();
        Invalidate();
    }

    void DisplayElement::Show(bool show)
    {
        if (visible == show)
            return;

        visible = show;
        MarkForExtentUpdate();
        Invalidate();
    }

    bool DisplayElement::IsVisible()
    {
        for (DisplayElement * p = this; p; p = TQPARENT(p)) {
            if (!p->visible)
                return false;
        }
        return true;
    }

    void DisplayElement::Select(bool _select)
    {
        if (selected == _select)
            return;

        selected = _select;
        Invalidate();
    }

    void DisplayElement::Focus(bool _focus)
    {
        if (focus == _focus)
            return;
        focus = _focus;
        Invalidate();
    }

    void DisplayElement::Activate()
    {
        DisplayElement * p = TQPARENT(this);

        if (p)
            p->Activate();
    }

}
