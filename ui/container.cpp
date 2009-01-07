
#include "khmapp.h"
#include <windowsx.h>
#include <commctrl.h>
#include <assert.h>


namespace nim {

    void DisplayElement::MarkForExtentUpdate(void)
    {
        MarkChildrenForExtentUpdate();
        for (DisplayElement * p = this; p; p = TQPARENT(p)) {
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
        e->owner = owner;
        MarkForExtentUpdate();
    }

    void DisplayElement::InsertChildBefore(DisplayElement * e, DisplayElement * next)
    {
        TQINSERTP(this, next, e);
        e->recalc_extents = true;
        e->visible = visible;
        e->owner = owner;
        MarkForExtentUpdate();
    }

    void DisplayElement::DeleteChild(DisplayElement * e)
    {
        NotifyDeleteChild(e);
        e->DeleteAllChildren();
        TQDELCHILD(this, e);
        delete e;
        MarkForExtentUpdate();
    }

    void DisplayElement::DeleteAllChildren()
    {
        NotifyDeleteAllChildren();
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
        r.GetLocation(&pto);
        pto = owner->MapFromDescendant(this, pto);
        owner->Invalidate(Rect(pto, Size(r.Width, r.Height)));
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

    HDITEM& DisplayColumn::GetHDITEM(HDITEM& hdi)
    {
        ZeroMemory(&hdi, sizeof(hdi));

        hdi.mask = HDI_FORMAT | HDI_LPARAM | HDI_WIDTH;
        hdi.fmt = 0;
#if (_WIN32_WINNT >= 0x501)
        if (sort)
            hdi.fmt |= ((sort_increasing)? HDF_SORTUP : HDF_SORTDOWN);
#endif
        hdi.lParam = (LPARAM) this; 
        hdi.cxy = width;

        hdi.pszText = (LPWSTR) caption.c_str();
        if (hdi.pszText != NULL) {
            hdi.mask |= HDI_TEXT;
            hdi.fmt |= HDF_CENTER | HDF_STRING;
        }
        return hdi;
    }

    void DisplayColumnList::ValidateColumns()
    {
        bool do_grouping = true;

        for (DisplayColumnList::iterator column = begin();
             column != end(); column++) {

            (*column)->CheckFlags();

            if (!(*column)->group)
                do_grouping = false;

            if (!do_grouping && (*column)->group)
                (*column)->group = false;
        }
    }

    void DisplayColumnList::AdjustColumnPositions(int max_width)
    {
        DisplayColumn * filler_col = NULL;
        int width = 0;

        for (iterator i = begin(); i != end(); ++i) {
            if ((*i)->filler && filler_col == NULL) {
                filler_col = *i;
                continue;
            }
            width += (*i)->width;
        }

        if (filler_col != NULL)
            filler_col->width = max(max_width - width, 0);

        int x = 0;

        for (iterator i = begin(); i != end(); ++i) {
            (*i)->x = x;
            x += (*i)->width;
        }
    }

    void DisplayColumnList::AddColumnsToHeaderControl(HWND hwnd_header)
    {
        int h_count = Header_GetItemCount(hwnd_header);
        int ord = 0;

        for (iterator i = begin(); i != end(); ++i) {
            HDITEM hdi;

            (*i)->GetHDITEM(hdi);
            if (ord < h_count) {
                int hidx = Header_OrderToIndex(hwnd_header, ord);
                Header_SetItem(hwnd_header, hidx, &hdi);
            } else {
                Header_InsertItem(hwnd_header, ord, &hdi);
            }
            ord++;
        }

        for (; ord < h_count; h_count--) {
            int hidx = Header_OrderToIndex(hwnd_header, ord);
            Header_DeleteItem(hwnd_header, hidx);
        }
    }

    void DisplayColumnList::UpdateHeaderControl(HWND hwnd_header, UINT mask)
    {
        int h_count = Header_GetItemCount(hwnd_header);
        int ord = 0;
        assert(h_count == size());

        for (iterator i = begin(); i != end(); ++i) {
            int hidx = Header_OrderToIndex(hwnd_header, ord);
            HDITEM hdi;

            (*i)->GetHDITEM(hdi);
            hdi.mask &= mask;
            Header_SetItem(hwnd_header, hidx, &hdi);
            ord++;
        }
    }

    void DisplayColumnList::Clear()
    {
        while (!empty()) {
            DisplayColumn * c = back();
            pop_back();
            delete c;
        }
    }

    DisplayColumnList::~DisplayColumnList()
    {
        Clear();
    }

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

    void ControlWindow::HandleHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
    {
        OnHScroll(code, pos);
    }

    void ControlWindow::HandleVScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
    {
        OnVScroll(code, pos);
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

            // TODO: set the header font here
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
            col->width = hdi.cxy;

            columns.UpdateHeaderControl(hwnd_header, HDI_WIDTH);
            MarkForExtentUpdate();
            OnColumnSizeChanged(hdi.iOrder);
            DisplayElement::Invalidate();
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
            OnColumnContextMenu(hdi.iOrder);
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

    LRESULT DisplayContainer::OnHeaderNotify(NMHEADER * pnmh)
    {
        switch(pnmh->hdr.code) {
            case HDN_BEGINDRAG:
                return OnHeaderBeginDrag(pnmh);

            case HDN_BEGINTRACK:
                return OnHeaderBeginTrack(pnmh);

            case HDN_ENDDRAG:
                return OnHeaderEndDrag(pnmh);

            case HDN_ENDTRACK:
                return OnHeaderEndTrack(pnmh);

            case HDN_ITEMCHANGING:
                return OnHeaderItemChanging(pnmh);

            case HDN_ITEMCHANGED:
                return OnHeaderItemChanged(pnmh);

            case HDN_ITEMCLICK:
                return OnHeaderItemClick(pnmh);

            case HDN_ITEMDBLCLICK:
                return OnHeaderItemDblClick(pnmh);

            case HDN_TRACK:
                return OnHeaderTrack(pnmh);
        }
        return 0;
    }

    LRESULT DisplayContainer::OnNotify(int id, NMHDR * pnmh)
    {
        if (pnmh->hwndFrom == hwnd_header && hwnd_header != NULL)
            return OnHeaderNotify(reinterpret_cast<NMHEADER *>(pnmh));
        return ControlWindow::OnNotify(id, pnmh);
    }

    ATOM ControlWindow::window_class = 0;

    HWND ControlWindow::Create(HWND parent, Rect & extents, int id, LPVOID createParams)
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
        CreateParams * cp = (UNALIGNED CreateParams *) lpc->lpCreateParams;
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
