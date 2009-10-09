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

#include <gdiplus.h>
#include <assert.h>

using namespace Gdiplus;

namespace nim {

    class DisplayContainer;

    class DisplayElement {
    public:
        Point origin;
        Size  extents;
        DisplayContainer * owner;

        bool expandable      : 1;
        bool is_outline      : 1;

        bool visible         : 1;
        bool expanded        : 1;

        bool focus           : 1;
        bool selected        : 1;
        bool highlight       : 1;

        bool recalc_extents  : 1;

        TQDCL(DisplayElement);

    public:
        DisplayElement() {
            TQINIT(this); 
            owner          = NULL;

            expandable     = false; 
            is_outline     = false;

            visible        = true; 
            expanded       = false; 

            focus          = false;
            selected       = false;
            highlight      = false;

            recalc_extents = true; 
        }

        virtual ~DisplayElement() {
            DeleteAllChildren();

            assert(TQPARENT(this) == NULL);
            assert(TQNEXTSIBLING(this) == NULL);
            assert(TQPREVSIBLING(this) == NULL);
        }

        void   InsertChildAfter(DisplayElement * e, DisplayElement * previous = NULL);

        void   InsertChildBefore(DisplayElement * e, DisplayElement * after = NULL);

        void   DeleteChild(DisplayElement * e);

        void   DeleteAllChildren();

        virtual void NotifyDeleteChild(DisplayElement * _parent, DisplayElement * e);

        virtual void NotifyDeleteAllChildren(DisplayElement * _parent);

        void   MoveChildAfter(DisplayElement * e, DisplayElement * previous);

        void   MoveChildBefore(DisplayElement * e, DisplayElement * next);

        void   SetOwner(DisplayContainer * _owner);

        void   MarkForExtentUpdate(void);

        void   MarkChildrenForExtentUpdate(void);

        Point  MapToParent(const Point & p);

        virtual Point MapToScreen(const Point & p);

        Point  MapToDescendant(const DisplayElement * e, const Point & p);

        Point  MapFromDescendant(const DisplayElement * c, const Point & p);

        DisplayElement * ChildFromPoint(const Point & p);

        DisplayElement * DescendantFromPoint(const Point & p);

        void UpdateLayout(Graphics & g, const Rect & layout);

        void NotifyLayoutInternal();

        void Invalidate(const Rect & r);

        void Invalidate() {
            Invalidate(Rect(Point(0,0), extents));
        }

        virtual void UpdateLayoutPre(Graphics & g, Rect & layout);

        virtual void UpdateLayoutPost(Graphics & g, const Rect & layout);

        virtual void NotifyLayout() { };

        virtual void Expand(bool _expand = true);

        virtual void Show(bool _show = true);

        virtual void Select(bool _select = true);

        virtual void Focus(bool _focus = true);

        bool IsVisible();

        void OnPaint(Graphics& g, const Rect& bounds, const Rect& clip);

        virtual void PaintSelf(Graphics &g, const Rect& bounds, const Rect& clip) { }

        virtual void OnClick(const Point& p, UINT keyflags, bool doubleClick) {
            if (TQPARENT(this))
                TQPARENT(this)->OnClick(MapToParent(p), keyflags, doubleClick);
        }

        virtual void OnChildClick(DisplayElement * c, const Point& p, UINT keyflags, bool doubleClick) { }

        virtual void OnContextMenu(const Point& p) { }

        virtual void OnMouse(const Point& p, UINT keyflags) {
            highlight = true;
        }

        virtual void OnMouseOut(void) {
            highlight = false;
        }

        virtual bool OnShowToolTip(std::wstring& caption, Rect& align_rect) { return false; }

        virtual bool IsTabStop() { return false; }

        virtual HFONT GetHFONT() { return NULL; }

        virtual Font *GetFont(Graphics& g) { return NULL; }

        virtual Color GetForegroundColor() { return Color((ARGB) Color::Black); }
    };

}
