#pragma once


namespace nim
{

    /*! \brief Credential Type (Text) element
     */
    class CwCredentialTypeElement :
        public SubheaderTextBoxT< WithStaticCaption < WithTextDisplay <> > > {
    public:
        CwCredentialTypeElement(const CredentialType& credtype) {
            SetCaption(credtype.GetResourceString(KCDB_RES_DISPLAYNAME));
        }
    };


    /*! \brief Credential Type Outline Node
     */
    class CwCredTypeOutline : public CwOutline
    {
    public:
        CredentialType credtype;
        CwCredentialTypeElement * el_typename;

    public:
        CwCredTypeOutline(const CredentialType& _credtype, int _column) :
            credtype(_credtype),
            CwOutline(_column) {

            InsertChildAfter(el_typename =
                             new CwCredentialTypeElement(_credtype));
        }

        virtual bool Represents(const Credential& credential) {
            return credtype == credential.GetType();
        }

        virtual void UpdateLayoutPost(Graphics& g, const Rect& layout) {
            FlowLayout l(layout, g_theme->sz_margin);

            l.Add(el_typename, FlowLayout::Left)
                .LineBreak();

            Rect r = l.GetInsertRect();
            LayoutOutlineChildren(g, r);
            l.InsertRect(r);

            r = l.GetBounds();
            extents.Width = r.GetRight();
            extents.Height = r.GetBottom();
        }

        void SetContext(SelectionContext& sctx) {
            sctx.Add(credtype);
            dynamic_cast<CwOutlineBase*>(TQPARENT(this))->SetContext(sctx);
        }
    };


}
