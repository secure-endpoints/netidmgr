#include "khmapp.h"
#include <assert.h>

namespace nim {
    Point DisplayContainer::MapToScreen(const Point& p)
    {
        POINT wpt = { p.X - scroll.X, p.Y + header_height - scroll.Y };
        ClientToScreen(hwnd, &wpt);
        return Point(wpt.x, wpt.y);
    }

    void DisplayContainer::Invalidate(const Rect & r)
    {
        RECT wr;

        SetRect(&wr, r.GetLeft(), r.GetTop(), r.GetRight(), r.GetBottom());
        OffsetRect(&wr, - scroll.X, header_height - scroll.Y);
        ::InvalidateRect(hwnd, &wr, FALSE);
    }

    bool DisplayContainer::ValidateScrollPos(void)
    {
        Rect client;
        Rect oldScroll = scroll;

        GetClientRectNoScroll(&client);

        bool need_vscroll = (extents.Height > client.Height);
        bool need_hscroll = (extents.Width > client.Width);

        if (need_vscroll || need_hscroll) {
            int cxscroll = GetSystemMetrics(SM_CXVSCROLL);
            int cyscroll = GetSystemMetrics(SM_CYHSCROLL);

            if (!need_vscroll &&
                extents.Height + cyscroll > client.Height) {
                need_vscroll = true;
            }

            if (!need_hscroll &&
                extents.Width + cxscroll > client.Width) {
                need_hscroll = true;
            }

            if (need_vscroll)
                client.Width -= cxscroll;
            if (need_hscroll)
                client.Height -= cyscroll;
        }

        scroll.Width = client.Width;
        scroll.Height = client.Height;

        if (scroll.Y + scroll.Height > extents.Height)
            scroll.Y = extents.Height - scroll.Height;
        if (scroll.X + scroll.Width  > extents.Width )
            scroll.X = extents.Width - scroll.Width;

        if (scroll.X < 0)
            scroll.X = 0;
        if (scroll.Y < 0)
            scroll.Y = 0;

        return need_hscroll || need_vscroll;
    }

    bool DisplayContainer::UpdateScrollBars(bool redraw)
    {
        ValidateScrollPos();

        SCROLLINFO horiz = {
            sizeof(SCROLLINFO), SIF_PAGE|SIF_POS|SIF_RANGE,
            0, extents.Width,
            scroll.Width + 1, scroll.X, 0
        };

        SetScrollInfo(hwnd, SB_HORZ, &horiz, redraw);

        SCROLLINFO vert = {
            sizeof(SCROLLINFO), SIF_PAGE|SIF_POS|SIF_RANGE,
            0, extents.Height,
            scroll.Height + 1, scroll.Y, 0
        };

        SetScrollInfo(hwnd, SB_VERT, &vert, redraw);

        return true;
    }

    bool DisplayContainer::UpdateScrollInfo(void)
    {
        return true;
    }

    void DisplayContainer::ScrollBy(const Point& delta)
    {
        InvalidateRect(hwnd, NULL, FALSE);
    }

    void DisplayContainer::OnHScroll(UINT code, int pos)
    {
        Rect oldScroll = scroll;
        SCROLLINFO si = { sizeof(si), SIF_POS | SIF_TRACKPOS, 0, 0, 0, 0, 0 };
        Rect r;

        switch (code) {
        case SB_BOTTOM:
            scroll.X = extents.Width; break;

        case SB_TOP:
            scroll.X = 0; break;

        case SB_ENDSCROLL:
            GetScrollInfo(hwnd, SB_HORZ, &si);
            scroll.X = si.nPos;
            break;

        case SB_LINEDOWN:
            scroll.X += GetSystemMetrics(SM_CXICON);
            break;

        case SB_LINEUP:
            scroll.X -= GetSystemMetrics(SM_CXICON);
            break;

        case SB_PAGEDOWN:
            scroll.X += GetClientRect(&r).Width;
            break;

        case SB_PAGEUP:
            scroll.X -= GetClientRect(&r).Width;
            break;

        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            GetScrollInfo(hwnd, SB_VERT, &si);
            scroll.X = si.nTrackPos;
            break;

        }

        ValidateScrollPos();

        if (!oldScroll.Equals(scroll)) {
            UpdateScrollBars(true);
            ScrollBy(Point(scroll.X - oldScroll.X, scroll.Y - oldScroll.Y));
        }
    }

    void DisplayContainer::OnVScroll(UINT code, int pos)
    {
        Rect oldScroll = scroll;
        SCROLLINFO si = { sizeof(si), SIF_POS | SIF_TRACKPOS, 0, 0, 0, 0, 0 };
        Rect r;

        switch (code) {
        case SB_BOTTOM:
            scroll.Y = extents.Height; break;

        case SB_TOP:
            scroll.Y = 0; break;

        case SB_ENDSCROLL:
            GetScrollInfo(hwnd, SB_VERT, &si);
            scroll.Y = si.nPos;
            break;

        case SB_LINEDOWN:
            scroll.Y += GetSystemMetrics(SM_CYICON);
            break;

        case SB_LINEUP:
            scroll.Y -= GetSystemMetrics(SM_CYICON);
            break;

        case SB_PAGEDOWN:
            scroll.Y += GetClientRect(&r).Height;
            break;

        case SB_PAGEUP:
            scroll.Y -= GetClientRect(&r).Height;
            break;

        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            GetScrollInfo(hwnd, SB_VERT, &si);
            scroll.Y = si.nTrackPos;
            break;

        }

        ValidateScrollPos();

        if (!oldScroll.Equals(scroll)) {
            UpdateScrollBars(true);
            ScrollBy(Point(scroll.X - oldScroll.X, scroll.Y - oldScroll.Y));
        }
    }

    void DisplayContainer::UpdateExtents(Graphics &g)
    {
        Rect client;
        GetClientRectNoScroll(&client);

        if (!recalc_extents)
            MarkForExtentUpdate();

        UpdateLayout(g, client);

        if (ValidateScrollPos()) {

            // we needed a scroll bar

            if (client.Width > scroll.Width) {
                MarkForExtentUpdate();
                client.Width = scroll.Width;
                client.Height = scroll.Height;
                UpdateLayout(g, client);
            }
        }

        UpdateScrollBars(true);
    }

    void DisplayContainer::OnPaint(Graphics& g, const Rect& clip)
    {
#ifdef DEBUG
        kherr_debug_printf(L"OnPaint for %dx%d region at (%d,%d)\n", clip.Width, clip.Height, clip.X, clip.Y);
#endif
        Rect b;

        if (recalc_extents)
            UpdateExtents(g);

        GetClientRect(&b);

        if (dbuffer == NULL ||
            dbuffer->GetWidth() < (unsigned) b.Width ||
            dbuffer->GetHeight() < (unsigned) b.Height) {
            if (dbuffer)
                delete dbuffer;

            dbuffer = new Bitmap(b.Width, b.Height, &g);
        }

        {
            Rect client_b(-scroll.X, -scroll.Y,
                          __max(extents.Width, b.Width + scroll.X),
                          __max(extents.Height, b.Height + scroll.Y));
            Rect client_clip = clip;
            Graphics ig(dbuffer);
            client_clip.Y -= header_height;
            DisplayElement::OnPaint(ig, client_b, client_clip);
        }

        g.DrawImage(dbuffer, b.X, b.Y, 0, 0, b.Width, b.Height, UnitPixel);
    }

    BOOL DisplayContainer::OnCreate(LPVOID createParams)
    {
        return ControlWindow::OnCreate(createParams);
    }

    Rect& DisplayContainer::GetClientRectNoScroll(Rect * cr)
    {
        SCROLLBARINFO sbi;

        GetClientRect(cr);

        sbi.cbSize = sizeof(SCROLLBARINFO);
        if (GetScrollBarInfo(hwnd, OBJID_HSCROLL, &sbi) &&
            (sbi.rgstate[0] & (STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_OFFSCREEN)) == 0) {
            cr->Height += sbi.rcScrollBar.bottom - sbi.rcScrollBar.top;
        }

        sbi.cbSize = sizeof(SCROLLBARINFO);
        if (GetScrollBarInfo(hwnd, OBJID_VSCROLL, &sbi) &&
            (sbi.rgstate[0] & (STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_OFFSCREEN)) == 0) {
            cr->Width += sbi.rcScrollBar.right - sbi.rcScrollBar.left;
        }

        return *cr;
    }

    Rect& DisplayContainer::GetClientRect(Rect *cr)
    {
        ControlWindow::GetClientRect(cr);
        if (show_header && hwnd_header != NULL) {
            cr->Y += header_height;
            cr->Height -= header_height;
        }
        return *cr;
    }

    void DisplayContainer::UpdateLayoutPre(Graphics& g, Rect& layout)
    {
        columns.AdjustColumnPositions(layout.Width);

        if (show_header && hwnd_header == NULL) {
            hwnd_header = CreateWindowEx(0, WC_HEADER, NULL,
                                         HDS_BUTTONS | HDS_DRAGDROP | HDS_HORZ | WS_CHILD,
                                         0, 0, 0, 0, hwnd, NULL, khm_hInstance, NULL);

            assert(hwnd_header != NULL);

            HFONT hf = GetHFONT();
            SendMessage(hwnd_header, WM_SETFONT, (WPARAM) hf, 0);
            columns.AddColumnsToHeaderControl(hwnd_header);

            // Since child elements might depend on what
            // GetClientRect() returns, we have to update
            // header_height.

            RECT r;
            WINDOWPOS wp;
            HDLAYOUT  hdl = {&r, &wp};

            ::GetClientRect(hwnd, &r);
            Header_Layout(hwnd_header, &hdl);
            header_height = wp.cy;

        } else if (!show_header && hwnd_header != NULL) {

            DestroyWindow(hwnd_header);
            hwnd_header = NULL;

        }

        if (!show_header)
            header_height = 0;
    }

    void DisplayContainer::UpdateLayoutPost(Graphics& g, const Rect& layout)
    {
        if (show_header && hwnd_header != NULL) {
            RECT rc_client;
            WINDOWPOS wp;
            HDLAYOUT  hdl = {&rc_client, &wp};

            columns.UpdateHeaderControl(hwnd_header);

            ::GetClientRect(hwnd, &rc_client);
            rc_client.left = -scroll.X;
            rc_client.right = max(rc_client.right, extents.Width - scroll.X);
            Header_Layout(hwnd_header, &hdl);
            header_height = wp.cy;

            SetWindowPos(hwnd_header, wp.hwndInsertAfter, wp.x, wp.y, wp.cx, wp.cy, wp.flags | SWP_SHOWWINDOW);
        }
    }

    void DisplayContainer::OnPosChanged(LPWINDOWPOS lp)
    {
        if (!(lp->flags & SWP_NOSIZE)) {
            MarkForExtentUpdate();
        }
    }

    void DisplayContainer::OnMouseMove(const Point& pt_c, UINT keyflags)
    {
        Point p = ClientToVirtual(pt_c);
        DisplayElement * ne = DescendantFromPoint(p);
        if (keyflags & MK_LBUTTON) {
            if (ne == mouse_element) {
                if (ne)
                    ne->OnMouse(MapToDescendant(ne, p), keyflags);
            } else if (mouse_element)
                mouse_element->OnMouseOut();
        } else {
            if (ne == mouse_element) {
                if (ne)
                    ne->OnMouse(MapToDescendant(ne, p), keyflags);
            } else {
                if (mouse_element)
                    mouse_element->OnMouseOut();
                mouse_element = ne;
                if (ne)
                    ne->OnMouse(MapToDescendant(ne, p), keyflags);
            }
        }

        if (!mouse_track) {
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_HOVER | TME_LEAVE, hwnd, HOVER_DEFAULT };
            TrackMouseEvent(&tme);
            mouse_track = true;
        }
    }

    void DisplayContainer::OnLButtonDown(bool doubleClick, const Point& pt_c, UINT keyflags)
    {
        Point p = ClientToVirtual(pt_c);
        DisplayElement * ne = DescendantFromPoint(p);
        mouse_element = ne;
        mouse_dblclk = doubleClick;
    }

    void DisplayContainer::OnLButtonUp(const Point& pt_c, UINT keyflags)
    {
        Point p = ClientToVirtual(pt_c);
        DisplayElement * ne = DescendantFromPoint(p);
        if (ne == mouse_element && ne) {
            ne->OnClick(MapToDescendant(ne, p), keyflags, mouse_dblclk);
            ne->OnMouseOut();
        }
        mouse_element = NULL;
        mouse_dblclk = false;
    }

    void DisplayContainer::OnMouseHover(const Point& p, UINT keyflags)
    {
        mouse_track = false;
    }

    void DisplayContainer::OnMouseLeave()
    {
        if (mouse_element) {
            mouse_element->OnMouseOut();
            mouse_element = NULL;
        }

        mouse_track = false;
    }

    void DisplayContainer::OnContextMenu(const Point& p)
    {
        POINT pt = { p.X, p.Y };
        ScreenToClient(hwnd, &pt);
        Point pv = ClientToVirtual(Point(pt.x, pt.y));
        DisplayElement * ne = DescendantFromPoint(pv);
        if (ne)
            ne->OnContextMenu(MapToDescendant(ne, pv));
    }

    LRESULT DisplayContainer::OnHeaderBeginDrag(NMHEADER * pnmh)
    {
        HDITEM hdi = { HDI_LPARAM, 0 };
        Header_GetItem(hwnd_header, pnmh->iItem, &hdi);
        DisplayColumn * c = reinterpret_cast<DisplayColumn *>(hdi.lParam);
        if (c && c->fixed_position)
            return TRUE;
        return FALSE;
    }

    LRESULT DisplayContainer::OnHeaderEndDrag(NMHEADER * pnmh)
    {
        HDITEM hdi = { HDI_ORDER, 0 };

        if (pnmh->pitem == NULL)
            return TRUE;

        unsigned int dragged_from, dragged_to;
        Header_GetItem(hwnd_header, pnmh->iItem, &hdi);
        dragged_from = hdi.iOrder;
        dragged_to = pnmh->pitem->iOrder;

        // The user dragged the column at index dragged_from to
        // index dragged_to.

        if ( dragged_from == dragged_to)
            return TRUE;

        DisplayColumn * column = columns[dragged_from];
        columns.erase(columns.begin() + dragged_from);
        columns.insert(columns.begin() +
                       ((dragged_from < dragged_to)? dragged_to - 1 : dragged_to),
                       column);

        if (dragged_to < columns.size() - 1 &&
            columns[dragged_to + 1]->group &&
            !column->group) {
            column->group = true;
        }

        columns.ValidateColumns();
        MarkForExtentUpdate();
        OnColumnPosChanged(dragged_from, dragged_to);
        DisplayElement::Invalidate();

        return 0;
    }

    LRESULT DisplayContainer::OnHeaderBeginTrack(NMHEADER * pnmh)
    {
        HDITEM hdi = { HDI_LPARAM, 0 };
        Header_GetItem(hwnd_header, pnmh->iItem, &hdi);
        DisplayColumn * c = reinterpret_cast<DisplayColumn *>(hdi.lParam);
        if (c && (c->fixed_width || c->filler))
            return TRUE;
        return FALSE;
    }

    LRESULT DisplayContainer::OnHeaderEndTrack(NMHEADER * pnmh)
    {
        HDITEM hdi;

        ZeroMemory(&hdi, sizeof(hdi));
        hdi.mask = HDI_LPARAM | HDI_WIDTH | HDI_ORDER;
        if (Header_GetItem(pnmh->hdr.hwndFrom, pnmh->iItem, &hdi) &&
            hdi.lParam != 0) {
            DisplayColumn * col = reinterpret_cast<DisplayColumn *>(
                reinterpret_cast<void *>(hdi.lParam));
            assert(col != NULL);
            col->width = pnmh->pitem->cxy;

            columns.UpdateHeaderControl(hwnd_header, HDI_WIDTH);
            MarkForExtentUpdate();
            OnColumnSizeChanged(hdi.iOrder);
            Invalidate();
        }
        return 0;
    }

    LRESULT DisplayContainer::OnHeaderTrack(NMHEADER * pnmh)
    {
        return 0;
    }
    
    LRESULT DisplayContainer::OnHeaderItemChanging(NMHEADER * pnmh)
    {
        return 0;
    }

    LRESULT DisplayContainer::OnHeaderItemChanged(NMHEADER * pnmh)
    {
        return 0;
    }

    LRESULT DisplayContainer::OnHeaderItemClick(NMHEADER * pnmh)
    {
        HDITEM hdi = { HDI_LPARAM | HDI_ORDER, 0 };

        // iButton == 0 if left mouse button was used
        // iButton == 1 if right mouse button was used

        if ((pnmh->iButton != 0 && pnmh->iButton != 1) ||
            !Header_GetItem(hwnd_header, pnmh->iItem, &hdi))
            return 0;

        if (pnmh->iButton == 1) { // Right mouse button
            DWORD dp = GetMessagePos();
            POINTS pts = MAKEPOINTS(dp);
            OnColumnContextMenu(hdi.iOrder, Point(pts.x, pts.y));
            return 0;
        }

        DisplayColumn * col = reinterpret_cast<DisplayColumn *>(hdi.lParam);

        if (col == NULL)
            return 0;

        if (col->sort) {
            if (col->sort_increasing)
                col->sort_increasing = false;
            else
                col->sort = false;
        } else {
            col->sort = true;
            col->sort_increasing = true;
        }

        columns.ValidateColumns();
        columns.UpdateHeaderControl(hwnd_header, HDI_FORMAT);

        MarkForExtentUpdate();
        OnColumnSortChanged(hdi.iOrder);
        DisplayElement::Invalidate();

        return 0;
    }

    LRESULT DisplayContainer::OnHeaderItemDblClick(NMHEADER * pnmh)
    {
        HDITEM hdi = { HDI_LPARAM | HDI_ORDER, 0 };

        if (pnmh->iButton != 0 ||
            !Header_GetItem(hwnd_header, pnmh->iItem, &hdi))
            return 0;

        DisplayColumn * col = reinterpret_cast<DisplayColumn *>(hdi.lParam);

        if (col == NULL)
            return 0;

        if (col->group) {
            col->group = false;
        } else {
            for (unsigned int i=0; i < columns.size() && i <= (unsigned) hdi.iOrder; i++)
                if (!columns[i]->group) {
                    columns[i]->group = true;
                }
        }

        columns.ValidateColumns();
        columns.UpdateHeaderControl(hwnd_header, HDI_FORMAT);

        MarkForExtentUpdate();
        OnColumnSortChanged(hdi.iOrder);
        DisplayElement::Invalidate();

        return 0;
    }

    LRESULT DisplayContainer::OnHeaderRightClick(NMHDR * pnmh)
    {
        DWORD dp = GetMessagePos();
        POINTS pts = MAKEPOINTS(dp);
        OnColumnContextMenu(-1, Point(pts.x, pts.y));
        return 1;
    }

    LRESULT DisplayContainer::OnHeaderNotify(NMHDR * pnmh)
    {
        switch(pnmh->code) {
        case HDN_BEGINDRAG:
            return OnHeaderBeginDrag(reinterpret_cast<NMHEADER *>(pnmh));

        case HDN_BEGINTRACK:
            return OnHeaderBeginTrack(reinterpret_cast<NMHEADER *>(pnmh));

        case HDN_ENDDRAG:
            return OnHeaderEndDrag(reinterpret_cast<NMHEADER *>(pnmh));

        case HDN_ENDTRACK:
            return OnHeaderEndTrack(reinterpret_cast<NMHEADER *>(pnmh));

        case HDN_ITEMCHANGING:
            return OnHeaderItemChanging(reinterpret_cast<NMHEADER *>(pnmh));

        case HDN_ITEMCHANGED:
            return OnHeaderItemChanged(reinterpret_cast<NMHEADER *>(pnmh));

        case HDN_ITEMCLICK:
            return OnHeaderItemClick(reinterpret_cast<NMHEADER *>(pnmh));

        case HDN_ITEMDBLCLICK:
            return OnHeaderItemDblClick(reinterpret_cast<NMHEADER *>(pnmh));

        case HDN_TRACK:
            return OnHeaderTrack(reinterpret_cast<NMHEADER *>(pnmh));

        case NM_RCLICK:
            return OnHeaderRightClick(pnmh);
        }
        return 0;
    }

    LRESULT DisplayContainer::OnNotify(int id, NMHDR * pnmh)
    {
        if (pnmh->hwndFrom == hwnd_header && hwnd_header != NULL)
            return OnHeaderNotify(pnmh);
        return ControlWindow::OnNotify(id, pnmh);
    }
}
