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

/*! \brief Generic Outline Node
 */
class CwGenericOutline : public CwOutline
{
public:
    Credential credential;
    khm_int32  attr_id;
    StaticTextElement * el_text;

public:
    CwGenericOutline(const Credential& _credential, khm_int32 _attr_id, int _column) :
        credential(_credential),
        attr_id(_attr_id),
        CwOutline(_column) {

        InsertChildAfter(el_text =
                         PNEW StaticTextElement(credential.GetAttribStringObj(attr_id)));
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
