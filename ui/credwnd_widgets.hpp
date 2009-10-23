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

#include "generic_widgets.hpp"

namespace nim
{


    /*! \brief Time Value (Text) element
     */
    class CwTimeTextCellElement :
        public WithColumnAlignment< ColumnCellTextT < WithStaticCaption < WithTextDisplay <> > > >,
        public TimerQueueClient {

    protected:
        Credential credential;
        khm_int32  attr_id;
        khm_int32  ftse_flags;
        volatile bool need_update;

    public:
        CwTimeTextCellElement(Credential& _credential,
                              khm_int32 _attr_id, khm_int32 _ftse_flags, int _column):
            WithColumnAlignment(_column, 1),
            credential(_credential),
            attr_id(_attr_id),
            ftse_flags(_ftse_flags),
            need_update(true)
        {}

    public:
        void UpdateCaption() {
            FILETIME ft = credential.GetAttribFileTime(attr_id);
            wchar_t buf[128] = L"";
            khm_size cb = sizeof(buf);
            long ms = 0;

            CancelTimer();

            FtToStringEx(&ft, ftse_flags, &ms, buf, &cb);

            SetCaption(std::wstring(buf));
            if (ms != 0 && owner) {
                owner->SetTimer(this, ms);
            }

            need_update = false;
        }

        void OnTimer() {
            need_update = true;
            MarkForExtentUpdate();
            Invalidate();
        }

        void UpdateLayoutPre(Graphics& g, Rect& layout) {
            if (need_update)
                UpdateCaption();
            __super::UpdateLayoutPre(g, layout);
        }

        void UpdateLayoutPost(Graphics& g, const Rect& bounds) {
            INT width = extents.Width;
            WithTextDisplay::UpdateLayoutPost(g, bounds);
            extents.Width = width;
            extents.Height += g_theme->sz_margin.Height;
        }
    };


    /*! \brief Static Table Cell (Text) Element
     */
    class CwStaticCellElement :
        public WithColumnAlignment< ColumnCellTextT < WithStaticCaption < WithTextDisplay <> > > > {
    public:
        CwStaticCellElement(const std::wstring& _caption, int _column, int _span = 1) :
          WithColumnAlignment(_column, _span) {
            SetCaption(_caption);
        }

        void UpdateLayoutPost(Graphics& g, const Rect& bounds) {
            INT width = extents.Width;
            WithTextDisplay::UpdateLayoutPost(g, bounds);
            extents.Width = width;
            extents.Height += g_theme->sz_margin.Height;
        }
    };



}
