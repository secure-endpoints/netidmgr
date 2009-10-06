#pragma once

#include "DisplayElement.hpp"
#include "ControlWindow.hpp"

namespace nim {

    class DisplayContainer : virtual public DisplayElement, public ControlWindow {
    protected:
        Rect    scroll;
        Image * dbuffer;

        HWND    hwnd_header;
        int     header_height;

    public:
        bool    show_header  : 1;
        DisplayColumnList columns;

    public:
        DisplayElement * mouse_element;
        bool    mouse_dblclk : 1;
        bool    mouse_track  : 1;

    public:
        DisplayContainer(): ControlWindow() {
            dbuffer = 0; show_header = false; hwnd_header = NULL;
            header_height = 0;
            mouse_element = NULL; mouse_dblclk = false;
            owner = this;
        }

        virtual ~DisplayContainer() { if (dbuffer) delete dbuffer; }

        virtual void NotifyDeleteChild(DisplayElement * _parent, DisplayElement * e) {
            if (mouse_element == e)
                mouse_element = NULL;
            __super::NotifyDeleteChild(_parent, e);
        }

        virtual void NotifyDeleteAllChildren(DisplayElement * _parent) {
            mouse_element = NULL;
            __super::NotifyDeleteAllChildren(_parent);
        }

        Point ClientToVirtual(const Point& p) { 
            return Point(p.X + scroll.X, p.Y + scroll.Y - header_height);
        }

        Point VirtualToClient(const Point& p) {
            return Point(p.X - scroll.X, (p.Y - scroll.Y) + header_height);
        }

        bool ValidateScrollPos(void);

        bool UpdateScrollBars(bool redraw);

        bool UpdateScrollInfo(void);

        void UpdateExtents(Graphics& g);

        void ScrollBy(const Point & delta);

        virtual BOOL OnCreate(LPVOID createParams);

        virtual void UpdateLayoutPre(Graphics & g, Rect & layout);

        virtual void UpdateLayoutPost(Graphics& g, const Rect& layout);

        virtual void OnPosChanged(LPWINDOWPOS lp);

        virtual void Invalidate(const Rect & r);

        void Invalidate() {
            ::InvalidateRect(hwnd, NULL, TRUE);
        }

        virtual Point MapToScreen(const Point & p);

        virtual void OnPaint(Graphics& g, const Rect& clip);

        virtual void OnHScroll(UINT code, int pos);

        virtual void OnVScroll(UINT code, int pos);

        virtual Rect& GetClientRect(Rect * cr);

        Rect&   GetClientRectNoScroll(Rect * cr);

        virtual void OnMouseMove(const Point& p, UINT keyflags);

        virtual void OnMouseHover(const Point& p, UINT keyflags);

        virtual void OnMouseLeave();

        virtual void OnLButtonDown(bool doubleClick, const Point& p, UINT keyflags);

        virtual void OnLButtonUp(const Point & p, UINT keyflags);

        virtual void OnContextMenu(const Point & p);

        virtual LRESULT OnNotify(int id, NMHDR * pnmh);


        LRESULT OnHeaderNotify(NMHDR * pnmh);

        LRESULT OnHeaderBeginTrack(NMHEADER * pnmh);

        LRESULT OnHeaderEndTrack(NMHEADER * pnmh);

        LRESULT OnHeaderTrack(NMHEADER * pnmh);

        LRESULT OnHeaderBeginDrag(NMHEADER * pnmh);

        LRESULT OnHeaderEndDrag(NMHEADER * pnmh);

        LRESULT OnHeaderItemClick(NMHEADER * pnmh);

        LRESULT OnHeaderItemDblClick(NMHEADER * pnmh);

        LRESULT OnHeaderItemChanging(NMHEADER * pnmh);

        LRESULT OnHeaderItemChanged(NMHEADER * pnmh);

        LRESULT OnHeaderRightClick(NMHDR * pnmh);

    public:                     // Overridables
        virtual void OnColumnSizeChanged(int idx) {}

        virtual void OnColumnPosChanged(int from, int to) {}

        virtual void OnColumnSortChanged(int idx) {}

        virtual void OnColumnContextMenu(int idx, const Point& p) {}
    };


}
