#include "khmapp.h"
#include <assert.h>

namespace nim {

    ATOM ControlWindow::window_class = 0;

    void ControlWindow::HandleHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
    {
        OnHScroll(code, pos);
    }

    void ControlWindow::HandleVScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
    {
        OnVScroll(code, pos);
    }

    HWND ControlWindow::Create(HWND parent, const Rect & extents, int id, LPVOID createParams)
    {
        CreateParams cp;

        assert(hwnd == NULL);
        assert(window_class != 0);

        cp.cw = this;
        cp.createParams = createParams;

        hwnd = CreateWindowEx(GetStyleEx(), MAKEINTATOM(window_class),
                              L"",
                              GetStyle(),
                              extents.X, extents.Y, extents.Width, extents.Height,
                              parent, (HMENU)(INT_PTR) id,
                              khm_hInstance, &cp);
        return hwnd;
    }

    BOOL ControlWindow::ShowWindow(int nCmdShow)
    {
        return ::ShowWindow(hwnd, nCmdShow);
    }

    void ControlWindow::HandleOnPaint(HWND hwnd)
    {
        RECT r = {0,0,0,0};
        HDC hdc;
        PAINTSTRUCT ps;
        bool has_update_rect = !!GetUpdateRect(hwnd, &r, FALSE);

        if (has_update_rect)
            hdc = BeginPaint(hwnd, &ps);
        else {
            hdc = GetDC(hwnd);
            ::GetClientRect(hwnd, &r);
        }

        {
            Graphics g(hdc);

            OnPaint(g, Rect(r.left, r.top, r.right - r.left, r.bottom - r.top));
        }

        if (has_update_rect)
            EndPaint(hwnd, &ps);
        else
            ReleaseDC(hwnd, hdc);
    }

    BOOL ControlWindow::HandleOnCreate(HWND hwnd, LPCREATESTRUCT lpc)
    {
        CreateParams * cp = (CreateParams *) lpc->lpCreateParams;
        ControlWindow *cw = cp->cw;

        assert(cw != NULL);

#pragma warning(push)
#pragma warning(disable: 4244)
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) cw);
#pragma warning(pop)

        cw->hwnd = hwnd;

        return cw->OnCreate(cp->createParams);
    }

    void ControlWindow::HandleOnDestroy(HWND hwnd)
    {
        hwnd = NULL;
        OnDestroy();
    }

#ifdef KMQ_WM_DISPATCH
    LRESULT ControlWindow::HandleDispatch(LPARAM lParam)
    {
        kmq_message * m;
        khm_int32 rv;

        kmq_wm_begin(lParam, &m);
        rv = OnWmDispatch(m->type, m->subtype, m->uparam, m->vparam);
        return kmq_wm_end(m, rv);
    }
#endif

#ifndef HANDLE_WM_MOUSEHOVER
#define HANDLE_WM_MOUSEHOVER(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
#endif
#ifndef HANDLE_WM_MOUSELEAVE
#define HANDLE_WM_MOUSELEAVE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd)), 0L)
#endif
#ifndef HANDLE_WM_HELP
#define HANDLE_WM_HELP(hwnd, wParam, lParam, fn) \
    (LRESULT)(fn)((hwnd), (HELPINFO *)(LPARAM) lParam)
#endif

    LRESULT CALLBACK
    ControlWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        ControlWindow *cw = NULL;

        cw = (ControlWindow *)(LONG_PTR) GetWindowLongPtr(hwnd, GWLP_USERDATA);

        switch(uMsg) {
            HANDLE_MSG(hwnd, WM_CREATE, HandleOnCreate);
            HANDLE_MSG(hwnd, WM_DESTROY, cw->HandleOnDestroy);
            HANDLE_MSG(hwnd, WM_COMMAND, cw->HandleCommand);
            HANDLE_MSG(hwnd, WM_PAINT, cw->HandleOnPaint);
            HANDLE_MSG(hwnd, WM_WINDOWPOSCHANGING, cw->HandlePosChanging);
            HANDLE_MSG(hwnd, WM_WINDOWPOSCHANGED, cw->HandlePosChanged);
            HANDLE_MSG(hwnd, WM_VSCROLL, cw->HandleVScroll);
            HANDLE_MSG(hwnd, WM_HSCROLL, cw->HandleHScroll);
            HANDLE_MSG(hwnd, WM_MOUSEMOVE, cw->HandleMouseMove);
            HANDLE_MSG(hwnd, WM_MOUSEHOVER, cw->HandleMouseHover);
            HANDLE_MSG(hwnd, WM_MOUSELEAVE, cw->HandleMouseLeave);
            HANDLE_MSG(hwnd, WM_LBUTTONDOWN, cw->HandleLButtonDown);
            HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK, cw->HandleLButtonDown);
            HANDLE_MSG(hwnd, WM_LBUTTONUP, cw->HandleLButtonUp);
            HANDLE_MSG(hwnd, WM_CONTEXTMENU, cw->HandleContextMenu);
            HANDLE_MSG(hwnd, WM_NOTIFY, cw->HandleNotify);
            HANDLE_MSG(hwnd, WM_SETFOCUS, cw->HandleSetFocus);
            HANDLE_MSG(hwnd, WM_KILLFOCUS, cw->HandleKillFocus);
            HANDLE_MSG(hwnd, WM_ACTIVATE, cw->HandleActivate);
            HANDLE_MSG(hwnd, WM_HELP, cw->HandleHelp);
#ifdef KMQ_WM_DISPATCH
            HANDLE_MSG(hwnd, KMQ_WM_DISPATCH, cw->HandleDispatch);
#endif
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    void ControlWindow::RegisterWindowClass(void)
    {
        assert(window_class == 0);

        WNDCLASSEX wx = {
            sizeof(WNDCLASSEX), // cbSize
            CS_CLASSDC | CS_HREDRAW | CS_VREDRAW, // style
            WindowProc,         // lpfnWndProc
            0,                  // cbClsExtra
            sizeof(ULONG_PTR),  // cbWndExtra
            khm_hInstance,      // hInstance
            NULL,               // hIcon
            LoadCursor(NULL, IDC_ARROW), // hCursor
            0,                           // hbrBackground
            NULL,                        // lpszMenuName
            L"NimControlWindowClass",    // lpszClassName
            NULL                         // hIconSm
        };

        window_class = RegisterClassEx(&wx);
    }

    void ControlWindow::UnregisterWindowClass(void)
    {
        if (window_class != 0) {
            UnregisterClass(MAKEINTATOM(window_class), khm_hInstance);
        }
    }
}
