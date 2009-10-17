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

namespace nim
{

    /*! \brief Basic outline object
     */
    class NOINITVTABLE CwOutline :
        public WithTabStop< WithColumnAlignment< CwOutlineBase > > {
    public:
        DisplayElement *outline_widget;

    public:
        CwOutline() {
            outline_widget = NULL;
        }

        CwOutline(int column) {
            outline_widget = NULL;
            SetColumnAlignment(column, -1);
        }

        ~CwOutline() {
            outline_widget = NULL; // will be automatically deleted
        }

        virtual void PaintSelf(Graphics& g, const Rect& bounds, const Rect& clip);

        virtual DrawState GetDrawState() {
            return (DrawState)(((expanded) ? DrawStateChecked : DrawStateNone) |
                               ((highlight) ? DrawStateHotTrack : DrawStateNone) |
                               ((selected) ? DrawStateSelected : DrawStateNone) |
                               ((focus) ? DrawStateFocusRect : DrawStateNone));
        }

        void LayoutOutlineSetup(Graphics& g, Rect& layout);
        void LayoutOutlineChildren(Graphics& g, Rect& layout);

        void UpdateLayoutPre(Graphics& g, Rect& layout) {
            LayoutOutlineSetup(g, layout);
        }

        void UpdateLayoutPost(Graphics& g, const Rect& layout) {
            Rect r = layout;
            LayoutOutlineChildren(g, r);
        }

        void Select(bool _select);

        void Focus(bool _focus) {
            if (focus == _focus)
                return;

            __super::Focus(_focus);

            SelectionContext sctx;

            SetContext(sctx);
        }

        void Activate();
    };




    /*! \brief Get the next outline object in an element
     */
    inline CwOutline * NextOutline(DisplayElement * e) {
        for (DisplayElement * i = e; i != NULL; i = TQNEXTSIBLING(i)) {
            CwOutline * t = dynamic_cast< CwOutline * >(i);
            if (t)
                return t;
        }
        return static_cast< CwOutline * >(NULL);
    }

#define for_each_child(c)                                               \
    for (CwOutline * c = NextOutline(TQFIRSTCHILD(this)),               \
             *nc = ((c != NULL)?NextOutline(TQNEXTSIBLING(c)):NULL);    \
         c != NULL;                                                     \
         c = nc, nc = ((c != NULL)?NextOutline(TQNEXTSIBLING(c)):NULL))


}
