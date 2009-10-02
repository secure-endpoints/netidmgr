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

        void Invalidate(const Rect & r);

        void Invalidate() {
            Invalidate(Rect(Point(0,0), extents));
        }

        virtual void UpdateLayoutPre(Graphics & g, Rect & layout);

        virtual void UpdateLayoutPost(Graphics & g, const Rect & layout);

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
