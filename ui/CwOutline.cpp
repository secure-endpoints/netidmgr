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
            outline_widget = new ExposeControlElement(g_theme->pt_margin_cx + g_theme->pt_margin_cy);
            InsertChildAfter(outline_widget);
        }

        layout.Width = extents.Width;

        if (expandable) {
            outline_widget->visible = true;
            layout.X = g_theme->sz_margin.Width * 2 + g_theme->sz_icon_sm.Width;
            layout.Width -= layout.X;
        } else {
            if (outline_widget != NULL)
                outline_widget->visible = false;
            layout.X = g_theme->sz_margin.Width;
            layout.Width -= layout.X;
            expanded = false;

            for_each_child(c) {
                if (c->visible)
                    c->Show(false);
            }
        }

        layout.Y = g_theme->sz_margin.Height;
    }

    void CwOutline::LayoutOutlineChildren(Graphics& g, Rect& layout)
    {
        Point p;

        layout.GetLocation(&p);
        layout.Height = 0;

        for_each_child(c) {
            c->visible = expanded;
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
        return new CwIdentityOutline(identity, column);
    }

    void CwOutline::Select(bool _select) {
        if (selected == _select)
            return;

        __super::Select(_select);
        for_each_child(c) {
            c->Select(_select);
        }
    }

    CwOutline * CreateOutlineNode(Credential & cred, DisplayColumnList& columns, int column)
    {
        CwColumn * cwcol = NULL;

        if ((unsigned) column < columns.size())
            cwcol = static_cast<CwColumn *>(columns[column]);

        if (cwcol == NULL || !cwcol->group)
            return new CwCredentialRow(cred, column);

        switch (cwcol->attr_id) {
        case KCDB_ATTR_ID_DISPLAY_NAME:
            return new CwIdentityOutline(cred.GetIdentity(), column);

        case KCDB_ATTR_TYPE_NAME:
            return new CwCredTypeOutline(cred.GetType(), column);

        default:
            return new CwGenericOutline(cred, cwcol->attr_id, column);
        }
    }

}
