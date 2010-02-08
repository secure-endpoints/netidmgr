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

#include "DisplayElement.hpp"
#include "ControlWindow.hpp"
#include "TimerQueue.hpp"

#include <list>

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
        dbuffer = NULL; show_header = false; hwnd_header = NULL;
        header_height = 0;
        mouse_element = NULL; mouse_dblclk = false;
        owner = this;
    }

    virtual ~DisplayContainer() {
        DeleteAllChildren();

        if (dbuffer) {
            delete dbuffer;
            dbuffer = NULL;
        }
    }

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

    bool UpdateScrollBars(bool redraw);

    bool UpdateScrollInfo(void);

    bool ValidateScrollPos(void);

    void ScrollBy(const Point & delta);

    void UpdateExtents(Graphics& g);

    Rect&   GetClientRectNoScroll(Rect * cr);

    virtual BOOL OnCreate(LPVOID createParams);

    virtual LRESULT OnNotify(int id, NMHDR * pnmh);

    virtual Point MapToScreen(const Point & p);

    virtual Point MapFromScreen(const Point & p);

    virtual Rect& GetClientRect(Rect * cr);

    virtual void Invalidate(const Rect & r);

    virtual void OnContextMenu(const Point & p);

    virtual void OnHScroll(UINT code, int pos);

    virtual void OnLButtonDown(bool doubleClick, const Point& p, UINT keyflags);

    virtual void OnLButtonUp(const Point & p, UINT keyflags);

    virtual void OnMouseHover(const Point& p, UINT keyflags);

    virtual void OnMouseLeave();

    virtual void OnMouseMove(const Point& p, UINT keyflags);

    virtual void OnMouseWheel(const Point& p, UINT keyflags, int zDelta);

    virtual void OnPaint(Graphics& g, const Rect& clip);

    virtual void OnPosChanged(LPWINDOWPOS lp);

    virtual void OnVScroll(UINT code, int pos);

    virtual void OnWmTimer(UINT_PTR id);

    virtual void UpdateLayoutPost(Graphics& g, const Rect& layout);

    virtual void UpdateLayoutPre(Graphics & g, Rect & layout);

    void Invalidate() {
        ::InvalidateRect(hwnd, NULL, TRUE);
    }


    LRESULT OnHeaderBeginDrag(NMHEADER * pnmh);

    LRESULT OnHeaderBeginTrack(NMHEADER * pnmh);

    LRESULT OnHeaderEndDrag(NMHEADER * pnmh);

    LRESULT OnHeaderEndTrack(NMHEADER * pnmh);

    LRESULT OnHeaderItemChanged(NMHEADER * pnmh);

    LRESULT OnHeaderItemChanging(NMHEADER * pnmh);

    LRESULT OnHeaderItemClick(NMHEADER * pnmh);

    LRESULT OnHeaderItemDblClick(NMHEADER * pnmh);

    LRESULT OnHeaderNotify(NMHDR * pnmh);

    LRESULT OnHeaderRightClick(NMHDR * pnmh);

    LRESULT OnHeaderTrack(NMHEADER * pnmh);

    void SetHeaderPosition(void);

    void SetHeaderFont(void);

public:                     // Overridables
    virtual void OnColumnContextMenu(int idx, const Point& p) {}

    virtual void OnColumnPosChanged(int from, int to) {}

    virtual void OnColumnSizeChanged(int idx) {}

    virtual void OnColumnSortChanged(int idx) {}

private:
    friend class IsKilledTimerEqualTo;

    struct KilledTimer {
        TimerQueueClient   *cb;
        DWORD               time_of_death;
    };

    typedef std::list<KilledTimer> KilledTimers;

    KilledTimers m_killed_timers;

    enum {
        TMR_DISCARD_THRESHOLD = 2000
    };

    void PurgeKilledTimers();

    bool TimerWasKilled(TimerQueueClient * cb);

public:
    void SetTimer(TimerQueueClient * cb, DWORD milliseconds);

    void KillTimer(TimerQueueClient * cb);
};


}
