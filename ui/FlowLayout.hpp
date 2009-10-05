#pragma once

#include <gdiplus.h>
#include <stack>

using namespace Gdiplus;

namespace nim {

    class FlowLayout {
        Rect bounds;
        INT  top;
        INT  baseline;
        INT  left;
        Size margin;

        FlowLayout * parent;

        std::stack<INT> indent;

        void ExtendBaseline(INT nb) {
            if (nb + top > baseline) {
                baseline = nb + top;
                if (bounds.Height < baseline)
                    bounds.Height = baseline;
            }

            if (parent) {
                parent->ExtendBaseline(bounds.Height);
            }
        }

        FlowLayout(const Rect& _bounds, const Size& _margin, FlowLayout& _parent) {
            parent = &_parent;
            bounds = _bounds;
            bounds.Height = 0;
            top = 0;
            baseline = top;
            left = 0;
            margin = _margin;
            indent.push(0);
        }

    public:
        FlowLayout(const Rect& _bounds, const Size& _margin) {
            bounds = _bounds;
            bounds.Height = 0;
            top = 0;
            baseline = top;
            left = 0;
            margin = _margin;
            indent.push(0);
            parent = NULL;
        }

        FlowLayout(const FlowLayout& that) :indent(that.indent) {
            bounds = that.bounds;
            bounds.Height = 0;
            top = that.top;
            baseline = that.baseline;
            left = that.left;
            margin = that.margin;
            parent = that.parent;
        }

        Rect GetBounds() {
            return Rect(bounds.X, bounds.Y, bounds.Width, bounds.Height + margin.Height);
        }

        Size GetSize() {
            return Size(bounds.Width, bounds.Height + margin.Height);
        }

        Rect GetInsertRect() {
            LineBreak();

            return Rect(left + bounds.X, top + bounds.Y, bounds.Width - left, 0);
        }

        FlowLayout& InsertRect(const Rect& r) {
            ExtendBaseline(r.Height);
            LineBreak();
            return *this;
        }

        FlowLayout& PadLeftByMargin() {
            left += margin.Width;
            return *this;
        }

        FlowLayout& LineBreak() {
            left = indent.top();
            top = baseline + margin.Height;
            return *this;
        }

        FlowLayout& PushIndent(INT v) {
            indent.push(indent.top() + v);
            return *this;
        }

        FlowLayout& PushThisIndent() {
            indent.push(left);
            return *this;
        }

        FlowLayout& PopIndent() {
            indent.pop();
            return *this;
        }

        FlowLayout InsertFillerColumn() {
            if (left != 0)
                left += margin.Width;
            Rect nb(left + bounds.X, top + bounds.Y,
                    bounds.Width - left, 0);
            left = bounds.Width;
            return FlowLayout(nb, margin, *this);
        }

        FlowLayout InsertFixedColumn(INT width) {
            if (left != 0)
                left += margin.Width;
            Rect nb(left + bounds.X, top + bounds.Y,
                    width, 0);
            left = width;
            return FlowLayout(nb, margin, *this);
        }

        enum Alignment {
            Left = 1,
            Right,
            Center
        };

        enum Type {
            Fixed = 0,
            Squish
        };

        FlowLayout& Add(DisplayElement * e, Alignment opt = Left,
                        Type etype = Fixed, bool condition = true) {

            if (!condition) {
                e->visible = false;
                return *this;
            }

            e->visible = true;

            switch (opt) {
            case Left:
                if (left != 0)
                    left += margin.Width;
                e->origin.X = left + bounds.X;
                e->origin.Y = top + bounds.Y;
                if (etype == Squish)
                    e->extents.Width = __min(bounds.Width - left, e->extents.Width);
                left += e->extents.Width;
                break;

            case Right:
                if (left != 0)
                    left += margin.Width;
                e->origin.Y = top + bounds.Y;
                if (etype == Squish)
                    e->extents.Width = __min(bounds.Width - left, e->extents.Width);
                e->origin.X = (bounds.Width - e->extents.Width) + bounds.X;
                left = bounds.Width;
                break;

            case Center:
                if (etype == Squish)
                    e->extents.Width = __min(bounds.Width, e->extents.Width);
                left = (bounds.Width - e->extents.Width) / 2;
                if (left < 0)
                    left = 0;
                e->origin.X = left + bounds.X;
                e->origin.Y = top + bounds.Y;
                left += e->extents.Width;
                break;
            }

            ExtendBaseline(e->extents.Height);
            return *this;
        }
    };
}