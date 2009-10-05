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

        virtual void OnContextMenu(const Point& p) { }

        virtual void OnSetFocus(HWND hwnd_old) { }

        virtual void OnKillFocus(HWND hwnd_new) { }

        virtual LRESULT OnNotify(int id, NMHDR * pnmh) { return 0; }

        virtual void OnActivate(UINT state, HWND hwndActDeact, BOOL fMinimized) { }

        virtual void OnClose() { DestroyWindow(); }

        virtual LRESULT OnHelp( HELPINFO * hlp ) { return 0; }

	virtual void OnWmTimer(UINT id) { }

        virtual LRESULT OnDrawItem( const DRAWITEMSTRUCT * lpDrawItem) { return 0; }

        virtual LRESULT OnMeasureItem( MEASUREITEMSTRUCT * lpMeasureItem) { return 0; }

	virtual UINT OnGetDlgCode(LPMSG pMsg);

#ifdef KMQ_WM_DISPATCH
        virtual khm_int32 OnWmDispatch(khm_int32 msg_type, khm_int32 msg_subtype, khm_ui_4 uparam,
                                       void * vparam) { return 0; }
#endif

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

	void HandleTimer(HWND hwnd, UINT id) {
	    return OnWmTimer(id);
	}

        void HandleOnPaint(HWND hwnd);

        void HandleOnDestroy(HWND hwnd);

        void HandleHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos);

        void HandleVScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos);

	UINT HandleGetDlgCode(HWND hwnd, LPMSG lpMsg);

        LRESULT HandleDispatch(LPARAM lParam);

        static BOOL HandleOnCreate(HWND hwnd, LPCREATESTRUCT lpc);

        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    public:
        HWND Create(HWND parent, const Rect & extents, int id = 0, LPVOID createParams = NULL);

        BOOL ShowWindow(int nCmdShow = SW_SHOW);

        BOOL DestroyWindow();

        BOOL Invalidate() {
            return ::InvalidateRect(hwnd, NULL, TRUE);
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