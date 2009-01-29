#pragma once

namespace nim
{

    /*! \brief Generic Outline Node
     */
    class CwGenericOutline : public CwOutline
    {
    public:
        Credential credential;
        khm_int32  attr_id;
        CwStaticTextElement * el_text;

    public:
        CwGenericOutline(const Credential& _credential, khm_int32 _attr_id, int _column) :
            credential(_credential),
            attr_id(_attr_id),
            CwOutline(_column) {

            InsertChildAfter(el_text =
                             new CwStaticTextElement(credential.GetAttribStringObj(attr_id)));
        }

        ~CwGenericOutline() {}

        virtual bool Represents(const Credential& _credential) {
            return credential.CompareAttrib(_credential, attr_id) == 0;
        }

        virtual void UpdateLayoutPost(Graphics& g, const Rect& layout) {
            FlowLayout l(layout, g_theme->sz_margin);

            l.Add(el_text, FlowLayout::Left)
                .LineBreak();

            Rect r = l.GetInsertRect();
            LayoutOutlineChildren(g, r);
            l.InsertRect(r);

            r = l.GetBounds();
            extents.Width = r.GetRight();
            extents.Height = r.GetBottom();
        }

        void SetContext(SelectionContext& sctx) {
            sctx.Add(credential, attr_id);
            dynamic_cast<CwOutlineBase*>(TQPARENT(this))->SetContext(sctx);
        }
    };

}
