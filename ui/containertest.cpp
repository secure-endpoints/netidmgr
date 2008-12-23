
#include<windows.h>

#define KHUI_ACTION_BASE 50000
#define KHUI_PACTION_BASE   (KHUI_ACTION_BASE + 500)

#define KHUI_PACTION_MENU   (KHUI_PACTION_BASE + 0)
#define KHUI_PACTION_UP     (KHUI_PACTION_BASE + 1)
#define KHUI_PACTION_DOWN   (KHUI_PACTION_BASE + 2)
#define KHUI_PACTION_LEFT   (KHUI_PACTION_BASE + 3)
#define KHUI_PACTION_RIGHT  (KHUI_PACTION_BASE + 4)
#define KHUI_PACTION_ENTER  (KHUI_PACTION_BASE + 5)
#define KHUI_PACTION_ESC    (KHUI_PACTION_BASE + 6)
#define KHUI_PACTION_OK     (KHUI_PACTION_BASE + 7)
#define KHUI_PACTION_CANCEL (KHUI_PACTION_BASE + 8)
#define KHUI_PACTION_CLOSE  (KHUI_PACTION_BASE + 9)
#define KHUI_PACTION_DELETE (KHUI_PACTION_BASE + 10)
#define KHUI_PACTION_UP_EXTEND (KHUI_PACTION_BASE + 11)
#define KHUI_PACTION_UP_TOGGLE (KHUI_PACTION_BASE + 12)
#define KHUI_PACTION_DOWN_EXTEND (KHUI_PACTION_BASE + 13)
#define KHUI_PACTION_DOWN_TOGGLE (KHUI_PACTION_BASE + 14)
#define KHUI_PACTION_BLANK  (KHUI_PACTION_BASE + 15)
#define KHUI_PACTION_NEXT   (KHUI_PACTION_BASE + 16)
#define KHUI_PACTION_SELALL (KHUI_PACTION_BASE + 17)
#define KHUI_PACTION_YES    (KHUI_PACTION_BASE + 18)
#define KHUI_PACTION_NO     (KHUI_PACTION_BASE + 19)
#define KHUI_PACTION_YESALL (KHUI_PACTION_BASE + 20)
#define KHUI_PACTION_NOALL  (KHUI_PACTION_BASE + 21)
#define KHUI_PACTION_REMOVE (KHUI_PACTION_BASE + 22)
#define KHUI_PACTION_KEEP   (KHUI_PACTION_BASE + 23)
#define KHUI_PACTION_DISCARD (KHUI_PACTION_BASE + 24)
#define KHUI_PACTION_PGDN   (KHUI_PACTION_BASE + 25)
#define KHUI_PACTION_PGUP   (KHUI_PACTION_BASE + 26)
#define KHUI_PACTION_PGUP_EXTEND (KHUI_PACTION_BASE + 27)
#define KHUI_PACTION_PGDN_EXTEND (KHUI_PACTION_BASE + 28)

#include "container.h"

using namespace nim;

ACCEL accel_table[] = {
    {FVIRTKEY,  VK_UP,  KHUI_PACTION_UP},
    {FVIRTKEY|FSHIFT,  VK_UP,  KHUI_PACTION_UP_EXTEND},
    {FVIRTKEY|FCONTROL,  VK_UP,  KHUI_PACTION_UP_TOGGLE},
    {FVIRTKEY,  VK_PRIOR,  KHUI_PACTION_PGUP},
    {FVIRTKEY|FSHIFT,  VK_PRIOR,  KHUI_PACTION_PGUP_EXTEND},
    {FVIRTKEY,  VK_DOWN,  KHUI_PACTION_DOWN},
    {FVIRTKEY|FSHIFT,  VK_DOWN,  KHUI_PACTION_DOWN_EXTEND},
    {FVIRTKEY|FCONTROL,  VK_DOWN,  KHUI_PACTION_DOWN_TOGGLE},
    {FVIRTKEY,  VK_NEXT,  KHUI_PACTION_PGDN},
    {FVIRTKEY|FSHIFT,  VK_NEXT,  KHUI_PACTION_PGDN_EXTEND},
    {FVIRTKEY,  VK_LEFT,  KHUI_PACTION_LEFT},
    {FVIRTKEY,  VK_RIGHT,  KHUI_PACTION_RIGHT},
    {FVIRTKEY,  VK_RETURN,  KHUI_PACTION_ENTER},
    {FVIRTKEY,  VK_ESCAPE,  KHUI_PACTION_ESC},
    {FCONTROL|FVIRTKEY,  'A',  KHUI_PACTION_SELALL}
};

class MyWidget : public DisplayElement {
    bool has_mouse;
    bool toggle;

    virtual void UpdateLayoutPost(Graphics& g, const Rect& layout) {
        extents.Width = 10;
        extents.Height = 10;
        origin.X = 10;
        origin.Y = 10;
    }

    virtual void PaintSelf(Graphics& g, const Rect& bounds) {
        SolidBrush w(toggle ? Color(0, 255, 0): Color(255,255,255));
        SolidBrush r(Color(255,0,0));
        g.FillEllipse((has_mouse)?&r:&w, bounds);
    }

    virtual void OnMouse(const Point& p, UINT keyflags) {
        if (!has_mouse) {
            has_mouse = true;
            Invalidate(Rect(Point(0,0), extents));
        }
    }

    virtual void OnMouseOut() {
        if (has_mouse) {
            has_mouse = false;
            Invalidate(Rect(Point(0,0), extents));
        }
    }

    virtual void OnClick(const Point& p, UINT keyflags, bool doubleClick)
    {
        toggle = !toggle;
    }

public:
    MyWidget() { has_mouse = false; toggle = false; }
};

class AnimWidget : public DisplayElement, public TimerQueueHost, public TimerQueueClient {
    bool do_animation;
    bool timer_set;

    virtual void UpdateLayoutPost(Graphics& g, const Rect& layout) {
        extents.Width = 100;
        extents.Height = 10;
        origin.X = 10;
        origin.Y = 30;
    }

    virtual void PaintSelf(Graphics & g, const Rect & bounds) {
        Rect r;
        int slot = (GetTickCount() / 100) % 100;
        if (!do_animation)
            slot = 0;
        bounds.GetBounds(&r);
        r.Width = slot;
        SolidBrush pos(Color(128,0,255,0));
        SolidBrush neg(Color(128,255,255,255));
        g.FillRectangle(&pos, r);
        r.X = r.GetRight();
        r.Width = bounds.Width - r.Width;
        g.FillRectangle(&neg, r);

        if (!timer_set) {
            SetTimer(this, 100);
        }
    }

    virtual void OnTimer() {
        timer_set = false;
        Invalidate();
    }

public:
    AnimWidget() {
        timer_set = false;
        do_animation = true;
    }
};

class MyElement : public WithTabStop< DisplayElement > {
    virtual void UpdateLayoutPost(Graphics& g, const Rect& layout) {
        extents.Width = layout.Width;
        extents.Height = 100;
    }

    virtual void PaintSelf(Graphics& g, const Rect& bounds) {
        LinearGradientBrush lb(Point(bounds.GetLeft(), bounds.GetTop()),
                               Point(bounds.GetRight(), bounds.GetBottom()),
                               Color((selected)?0:128, 255, 0, 0),
                               Color(0, 0, 255));

        g.FillRectangle(&lb, bounds);

        const wchar_t * s = L"Hello world!";
        Font f(L"Arial", 12.0);
        SolidBrush b(Color(0,0,0));
        RectF bb;
        PointF o(0.0, 0.0);

        g.MeasureString(s, -1, &f, o, &bb);
        o.X = bounds.X + (bounds.Width - bb.Width) / 2;
        o.Y = bounds.Y + (bounds.Height - bb.Height) / 2;
        g.DrawString(s, -1, &f, o, &b);
    }
};

class MyColElement : public WithTextDisplay< WithColumnAlignment<DisplayElement> >{
    Font * f;
    Brush * b;

public:
    MyColElement(std::wstring& _caption, DisplayContainer * _owner, int idx, int span) {
        SetColumnAlignment(idx, span); SetText(_caption);
        f = new Font(L"Arial", 10.0);
        b = new SolidBrush(Color(0,0,0));
    }

    ~MyColElement() { delete f; delete b; }

    virtual Font * GetFont() { return f; }
    virtual Brush * GetForegroundBrush() { return b; }
};

class MyRowContainer : public WithTabStop< DisplayElement > {
    virtual void PaintSelf(Graphics& g, const Rect& bounds) {
        if (selected) {
            LinearGradientBrush lb(bounds, Color(0,0,0,255), Color(255,0,0,255), LinearGradientModeHorizontal);
            g.FillRectangle(&lb, bounds);
        }
    }
};

class MyVBlock : public WithTabStop < WithOutline < WithVerticalLayout < DisplayElement > > > {
};

class MyControlWindow : public WithNavigation < WithTooltips< WithVerticalLayout< DisplayContainer > > >{
public:
    virtual DWORD GetStyle() {
        return WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL;
    }

    virtual void OnDestroy(void) {
        PostQuitMessage(0);
    }

    virtual void PaintSelf(Graphics& g, const Rect& bounds) {
        Rect cr;
        SolidBrush w(Color(255,255,255));
        SolidBrush e(Color(0,255,0));
        g.FillRectangle(&w, bounds);
        GetClientRect(&cr);
        cr.X = cr.Y = 0;
        g.FillEllipse(&e, cr);
    }
};

HINSTANCE khm_hInstance;

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpszCmdLine, int nCmdShow) 
{ 
    MSG msg;
    BOOL bRet; 
    HWND hw;
    MyControlWindow *cw = new MyControlWindow();
    GdiplusStartupInput s_input;
    ULONG_PTR gp_token;
    MyElement * me;

    {
        INITCOMMONCONTROLSEX icc;

        icc.dwSize = sizeof(icc);
        icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_WIN95_CLASSES;

        InitCommonControlsEx(&icc);
    }

    GdiplusStartup(&gp_token, &s_input, NULL);
 
    khm_hInstance = hInstance;  // save instance handle 

    MyControlWindow::RegisterWindowClass();

    cw->columns.push_back(new DisplayColumn(L"Hello", 100));
    cw->columns.push_back(new DisplayColumn(L"World!", 100));
    cw->columns[1]->filler = true;
    cw->columns[1]->sort = true;
    cw->columns[1]->sort_increasing = true;
    cw->columns.push_back(new DisplayColumn(L"Foo", 50));
    cw->columns.push_back(new DisplayColumn(L"Bar", 50));
    cw->columns[3]->fixed_position = true;
    cw->columns.push_back(new DisplayColumn(L"Baz", 50));
    cw->columns[4]->fixed_width = true;

    cw->show_header = true;

    hw = cw->Create(NULL, Rect(0, 0, 300, 300), 0);

    cw->InsertChildAfter(me = new MyElement(), NULL);
    me->InsertChildAfter(new MyWidget(), NULL);
    cw->InsertChildAfter(me = new MyElement(), NULL);
    me->InsertChildAfter(new MyWidget(), NULL);
    me->InsertChildAfter(new AnimWidget(), NULL);
    cw->InsertChildAfter(new MyElement(), NULL);
    DisplayElement * row = new MyRowContainer();
    cw->InsertChildAfter(row, NULL);
    row->InsertChildAfter(new MyColElement(std::wstring(L"KJLK KJLk jlk kj"), cw, 1, 1), NULL);
    row->InsertChildAfter(new MyColElement(std::wstring(L"kjlkdsa flksjdf lksajdlf k"), cw, 2, 0), NULL);
    row = new MyRowContainer();
    cw->InsertChildAfter(row, NULL);
    row->InsertChildAfter(new MyColElement(std::wstring(L"lKl kj lkj lkjlkasd ioj"), cw, 1, 1), NULL);
    row->InsertChildAfter(new MyColElement(std::wstring(L"LOLZ"), cw, 2, 0), NULL);
    cw->InsertChildAfter(new MyElement());
    cw->InsertChildAfter(new MyElement());
    cw->InsertChildAfter(new MyElement());

    MyVBlock * v = new MyVBlock();
    cw->InsertChildAfter(v);
    v->InsertChildAfter(new MyColElement(std::wstring(L"Anchor row"), cw, 1, 1));
    v->InsertOutlineAfter(new MyColElement(std::wstring(L"Hello"), cw, 1, 1));
    v->InsertOutlineAfter(new MyColElement(std::wstring(L"World"), cw, 1, 1));
    v->InsertOutlineAfter(new MyColElement(std::wstring(L"Blah!"), cw, 1, 1));


    if (hw == NULL) {
        MessageBox(NULL, L"FOO", L"Failed to create window", MB_OK);
        GdiplusShutdown(gp_token);
        return 1;
    }

    ShowWindow(hw, nCmdShow); 
    UpdateWindow(hw);

    HACCEL ha = CreateAcceleratorTable(accel_table, sizeof(accel_table)/sizeof(accel_table[0]));
 
    // Start the message loop. 
 
    while( (bRet = GetMessage( &msg, NULL, 0, 0 )) != 0)
    { 
        if (bRet == -1)
        {
            // handle the error and possibly exit
        }
        else
        {
            if (!TranslateAccelerator(hw, ha, &msg)) {
                TranslateMessage(&msg); 
                DispatchMessage(&msg); 
            }
        }
    } 


    delete cw;

    MyControlWindow::UnregisterWindowClass();

    GdiplusShutdown(gp_token);
 
    // Return the exit code to the system. 
 
    return msg.wParam; 
} 
