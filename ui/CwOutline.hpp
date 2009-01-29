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
