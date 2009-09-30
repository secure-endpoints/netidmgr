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
            if (ms != 0) {
                TimerQueueHost * host = dynamic_cast<TimerQueueHost *>(owner);
                if (host)
                    host->SetTimer(this, ms);
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
