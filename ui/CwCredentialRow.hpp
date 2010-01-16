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
    /*! \brief Credential Row Outline Node
     */
    class CwCredentialRow : public CwOutline
    {
    public:
        Credential  credential;
        std::vector<DisplayElement *> el_list;

    public:
        CwCredentialRow(const Credential& _credential, int _column) :
            credential(_credential),
            CwOutline(_column) {
        }

        ~CwCredentialRow() {}

        bool Represents(const Credential& _credential) {
            return credential == _credential;
        }

        void UpdateLayoutPre(Graphics& g, Rect& layout) {

            WithColumnAlignment::UpdateLayoutPre(g, layout);

            if (el_list.size() == 0) {
                for (int i = col_idx;
                     (col_span > 0 && i < col_idx + col_span) ||
                         (col_span <= 0 && (unsigned int) i < owner->columns.size()); i++) {
                    CwColumn * cwc = dynamic_cast<CwColumn *>(owner->columns[i]);
                    DisplayElement * e;

                    switch (cwc->attr_id) {
                    case KCDB_ATTR_RENEW_TIMELEFT:
                    case KCDB_ATTR_TIMELEFT:
                        e = PNEW CwTimeTextCellElement(credential, cwc->attr_id,
                                                       FTSE_INTERVAL, i);
                        break;

                    case KCDB_ATTR_NAME:
                        {
                            std::wstring wc = credential.GetAttribStringObj(KCDB_ATTR_DISPLAY_NAME);

                            if (wc.length() > 0)
                                e = PNEW CwStaticCellElement(wc, i);
                            else
                                e = PNEW CwStaticCellElement(credential.GetAttribStringObj(cwc->attr_id), i);
                        }
                        break;

                    case KCDB_ATTR_ID_NAME:
                    case KCDB_ATTR_ID:
                    case KCDB_ATTR_ID_DISPLAY_NAME:
                        e = PNEW CwStaticCellElement(credential.GetAttribStringObj(KCDB_ATTR_ID_DISPLAY_NAME), i);
                        break;

                    default:
                        e = PNEW CwStaticCellElement(credential.GetAttribStringObj(cwc->attr_id), i);
                    }

                    InsertChildAfter(e);
                    el_list.push_back(e);
                }
            }
        }

        void UpdateLayoutPost(Graphics& g, const Rect& layout) {
            DisplayElement::UpdateLayoutPost(g, layout);
        }

        void PaintSelf(Graphics& g, const Rect& bounds, const Rect& clip) {
            g_theme->DrawCredWindowNormalBackground(g, bounds, GetDrawState());
        }

        void SetContext(SelectionContext& sctx) {
            sctx.Add(credential);
            dynamic_cast<CwOutlineBase*>(TQPARENT(this))->SetContext(sctx);
        }

        void Select(bool _select) {
            if (_select == selected)
                return;

            __super::Select(_select);

            credential.SetFlags((selected)? KCDB_CRED_FLAG_SELECTED : 0,
                                KCDB_CRED_FLAG_SELECTED);
        }
    };
}
