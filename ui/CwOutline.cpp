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

#include "khmapp.h"
#include<assert.h>

#include "credwnd_base.hpp"

namespace nim
{
    void CwOutline::LayoutOutlineSetup(Graphics& g, Rect& layout)
    {
        __super::UpdateLayoutPre(g, layout);

        origin.X += indent;
        extents.Width -= indent;

        expandable = (owner != NULL &&
                      (unsigned)col_idx + 1 < owner->columns.size() &&
                      !!(NextOutline(TQFIRSTCHILD(this))));

        if (!expandable) {
            for (CwOutline * c = NextOutline(TQFIRSTCHILD(this));
                 c != NULL; c =  NextOutline(TQNEXTSIBLING(c)))
                if (c && c->col_idx == col_idx) {
                    expandable = true;
                    break;
                }
        }

        if (expandable && outline_widget == NULL) {
            outline_widget = PNEW ExposeControlElement(g_theme->pt_margin_cx + g_theme->pt_margin_cy);
            InsertChildAfter(outline_widget);
        }

        layout.Width = extents.Width;

        if (expandable) {
            outline_widget->Show();
            layout.X = g_theme->sz_margin.Width * 2 + g_theme->sz_icon_sm.Width;
            layout.Width -= layout.X;
        } else {
            if (outline_widget != NULL)
                outline_widget->Show(false);
            layout.X = g_theme->sz_margin.Width;
            layout.Width -= layout.X;

            Expand(false);
        }

        layout.Y = g_theme->sz_margin.Height;
    }

    void CwOutline::LayoutOutlineChildren(Graphics& g, Rect& layout)
    {
        Point p;

        layout.GetLocation(&p);
        layout.Height = 0;

        for_each_child(c) {
            c->Show(expanded);
            if (!c->visible)
                continue;

            c->origin.Y = p.Y;
            p.Y += c->extents.Height;
            layout.Height += c->extents.Height;
            if (c->extents.Width > layout.Width)
                layout.Width = c->extents.Width;
        }
    }

    void CwOutline::PaintSelf(Graphics& g, const Rect& bounds, const Rect& clip)
    {
        g_theme->DrawCredWindowOutline(g, bounds, GetDrawState());
    }

    CwOutline * CreateOutlineNode(Identity & identity, DisplayColumnList& columns, int column)
    {
        assert((unsigned) column >= columns.size() ||
               (static_cast<CwColumn *>(columns[column]))->attr_id == KCDB_ATTR_ID_DISPLAY_NAME);
        return PNEW CwIdentityOutline(identity, column);
    }

    void CwOutline::Select(bool _select)
    {
        if (selected == _select)
            return;

        __super::Select(_select);
        for_each_child(c) {
            c->Select(_select);
        }
    }

    void CwOutline::Activate()
    {
        khm_show_properties();
    }

    CwOutline * CreateOutlineNode(Credential & cred, DisplayColumnList& columns, int column)
    {
        CwColumn * cwcol = NULL;

        if ((unsigned) column < columns.size())
            cwcol = static_cast<CwColumn *>(columns[column]);

        if (cwcol == NULL || !cwcol->group)
            return PNEW CwCredentialRow(cred, column);

        switch (cwcol->attr_id) {
        case KCDB_ATTR_ID_DISPLAY_NAME:
            return PNEW CwIdentityOutline(cred.GetIdentity(), column);

        case KCDB_ATTR_TYPE_NAME:
            return PNEW CwCredTypeOutline(cred.GetType(), column);

        default:
            return PNEW CwGenericOutline(cred, cwcol->attr_id, column);
        }
    }

}
