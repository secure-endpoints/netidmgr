/*
 * Copyright (c) 2009-2010 Secure Endpoints Inc.
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
        if (left != indent.top()) {
            left = indent.top();
            top = baseline + margin.Height;
        }
        return *this;
    }

    FlowLayout& PushIndent(INT v) {
        indent.push(indent.top() + v);
        if (left < indent.top())
            left = indent.top();
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
        left += width;
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

    FlowLayout& Add(DisplayElement * e, bool condition) {
        return Add(e, Left, Fixed, condition);
    }

    FlowLayout& Add(DisplayElement * e, Alignment opt = Left,
                    Type etype = Fixed, bool condition = true) {

        if (!condition || !e) {
            if (e)
                e->Show(false);
            return *this;
        }

        e->Show();

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
