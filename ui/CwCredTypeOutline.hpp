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
                         PNEW CwCredentialTypeElement(_credtype));
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
