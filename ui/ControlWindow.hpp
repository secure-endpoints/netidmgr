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
#include "Refs.hpp"

using namespace Gdiplus;

namespace nim {

    class ControlWindow : virtual public RefCount {
    public:
        HWND  hwnd;

    protected:
        static ATOM window_class;
	static LONG init_count;

    public:
        ControlWindow(): RefCount(true), hwnd(NULL) { }

        virtual ~ControlWindow() {
            if (hwnd) DestroyWindow();
        }

        /* Message handlers */
    public:
        virtual BOOL OnCreate(LPVOID createParams) { return TRUE; }

        virtual void OnCommand(int id, HWND hwndCtl, UINT codeNotify) { }

        virtual void OnDestroy(void) { }

        virtual void OnPaint(Graphics& g, const Rect& clip) { }

        virtual BOOL OnPosChanging(LPWINDOWPOS lpos) { return FALSE; }

        virtual void OnPosChanged(LPWINDOWPOS lpos) { }

        virtual void OnHScroll(UINT code, int pos) { }

        virtual void OnVScroll(UINT code, int pos) { }

        virtual void OnMouseMove(const Point& p, UINT keyflags) { }

        virtual void OnMouseHover(const Point& p, UINT keyflags) { }

        virtual void OnMouseLeave() { }

        virtual void OnLButtonDown(bool doubleClick, const Point& p, UINT keyflags) { }

        virtual void OnLButtonUp(const Point& p, UINT keyflags) { }

        virtual void OnMouseWheel(const Point& p, UINT keyflags, int zDelta) { }

        virtual void OnContextMenu(const Point& p) { }

        virtual void OnSetFocus(HWND hwnd_old) { }

        virtual void OnKillFocus(HWND hwnd_new) { }

        virtual LRESULT OnNotify(int id, NMHDR * pnmh) { return 0; }

        virtual void OnActivate(UINT state, HWND hwndActDeact, BOOL fMinimized) { }

        virtual void OnClose() { DestroyWindow(); }

        virtual LRESULT OnHelp( HELPINFO * hlp ) { return 0; }

	virtual void OnWmTimer(UINT_PTR id) { }

        virtual LRESULT OnDrawItem( const DRAWITEMSTRUCT * lpDrawItem) { return 0; }

        virtual LRESULT OnMeasureItem( MEASUREITEMSTRUCT * lpMeasureItem) { return 0; }

	virtual UINT OnGetDlgCode(LPMSG pMsg);

        virtual LRESULT OnSync() { return 0; }

        virtual BOOL OnSetCursor(HWND hwndCursor, UINT codeHitTest, UINT msg) { return FALSE; }

#ifdef KMQ_WM_DISPATCH
        virtual khm_int32 OnWmDispatch(khm_int32 msg_type, khm_int32 msg_subtype, khm_ui_4 uparam,
                                       void * vparam) { return 0; }
#endif

        virtual BOOL HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lr) { return FALSE; }

        /* Styles */
    public:
        virtual DWORD GetStyle() {
            return WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                WS_HSCROLL | WS_TABSTOP | WS_VISIBLE | WS_VSCROLL;
        }

        virtual DWORD GetStyleEx() {
            return WS_EX_CONTROLPARENT;
        }

    public:
        virtual Rect& GetClientRect(Rect *cr) {
            RECT r;
            ::GetClientRect(hwnd, &r);
            cr->X = r.left; cr->Y = r.top; cr->Width = r.right - r.left; cr->Height = r.bottom - r.top;
            return *cr;
        }

    protected:
        BOOL HandlePosChanging(HWND hwnd, LPWINDOWPOS lpos) {
            return OnPosChanging(lpos);
        }

        void HandlePosChanged(HWND hwnd, LPWINDOWPOS lpos) {
            OnPosChanged(lpos);
        }

        void HandleMouseMove(HWND hwnd, int x, int y, UINT keyflags) {
            OnMouseMove(Point(x,y), keyflags);
        }

        void HandleMouseHover(HWND hwnd, int x, int y, UINT keyflags) {
            OnMouseHover(Point(x,y), keyflags);
        }

        void HandleMouseLeave(HWND hwnd) {
            OnMouseLeave();
        }

        void HandleLButtonDown(HWND hwnd, BOOL fDblClick, int x, int y, UINT keyflags) {
            OnLButtonDown(!!fDblClick, Point(x,y), keyflags);
        }

        void HandleLButtonUp(HWND hwnd, int x, int y, UINT keyflags) {
            OnLButtonUp(Point(x,y), keyflags);
        }

        void HandleMouseWheel(HWND hwnd, int x, int y, int zDelta, UINT keyflags) {
            OnMouseWheel(Point(x, y), keyflags, zDelta);
        }

        void HandleContextMenu(HWND hwnd, HWND hwnd_ctx, int x, int y) {
            OnContextMenu(Point(x, y));
        }

        void HandleCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
            OnCommand(id, hwndCtl, codeNotify);
        }

        LRESULT HandleNotify(HWND hwnd, int id, NMHDR * pnmh) {
            return OnNotify(id, pnmh);
        }

        void HandleSetFocus(HWND hwnd, HWND hwnd_old) {
            OnSetFocus(hwnd_old);
        }

        void HandleKillFocus(HWND hwnd, HWND hwnd_new) {
            OnKillFocus(hwnd_new);
        }

        void HandleActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized) {
            OnActivate(state, hwndActDeact, fMinimized);
        }

        void HandleOnClose(HWND hwnd) {
            OnClose();
        }

        LRESULT HandleMeasureItem(HWND hwnd, MEASUREITEMSTRUCT * lpMeasureItem) {
            return OnMeasureItem(lpMeasureItem);
        }

        LRESULT HandleDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem) {
            return OnDrawItem(lpDrawItem);
        }

        LRESULT HandleHelp(HWND hwnd, HELPINFO * info) {
            return OnHelp(info);
        }

	void HandleTimer(HWND hwnd, UINT_PTR id) {
	    return OnWmTimer(id);
	}

        void HandleOnPaint(HWND hwnd);

        void HandleOnDestroy(HWND hwnd);

        void HandleHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos);

        void HandleVScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos);

	UINT HandleGetDlgCode(HWND hwnd, LPMSG lpMsg);

        LRESULT HandleDispatch(LPARAM lParam);

        BOOL HandleSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg) {
            return OnSetCursor(hwndCursor, codeHitTest, msg);
        }

        static BOOL HandleOnCreate(HWND hwnd, LPCREATESTRUCT lpc);

        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    public:
        HWND Create(HWND parent, const Rect & extents, int id = 0, LPVOID createParams = NULL);

        BOOL ShowWindow(int nCmdShow = SW_SHOW);

        BOOL DestroyWindow();

        BOOL Invalidate() {
            return ::InvalidateRect(hwnd, NULL, TRUE);
        }

        BOOL Invalidate(const Rect& r) {
            RECT R = { r.GetLeft(), r.GetTop(), r.GetRight(), r.GetBottom() };
            return ::InvalidateRect(hwnd, &R, TRUE);
        }

	BOOL PostMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
	    return ::PostMessage(hwnd, msg, wParam, lParam);
	}

	LRESULT SendMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
	    return ::SendMessage(hwnd, msg, wParam, lParam);
	}

        static void RegisterWindowClass(void);

        static void UnregisterWindowClass(void);

    private:
        struct CreateParams {
            ControlWindow * cw;
            LPVOID          createParams;
        };
    };
}
