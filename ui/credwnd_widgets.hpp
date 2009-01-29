#pragma once

namespace nim
{


    /*! \brief Credential Window Button template
     */
    template<void (KhmDraw::*DF)(Graphics&, const Rect&, DrawState)>
    class CwButtonT : public WithFixedSizePos< DisplayElement > {
    public:
        CwButtonT(const Point& _p) {
            SetPosition(_p);
            SetSize(g_theme->sz_icon_sm);
        }

        virtual bool IsChecked() { return false; }

        virtual void PaintSelf(Graphics &g, const Rect& bounds, const Rect& clip) {
            (g_theme->*DF)(g, bounds,
                           (DrawState)
                           (((IsChecked())?
                             DrawStateChecked : DrawStateNone) |
                            ((highlight)?
                             DrawStateHotTrack : DrawStateNone)));
        }

        virtual void OnMouse(const Point& p, UINT keyflags) {
            bool h = highlight;
            __super::OnMouse(p, keyflags);
            if (!h)
                Invalidate();
        }

        virtual void OnMouseOut(void) {
            __super::OnMouseOut();
            Invalidate();
        }
    };



    /*! \brief Expose control

      Displays the [+] or [-] image buttons depending on whether the
      parent element is in an expanded state or not.
     */
    class CwExposeControlElement : public CwButtonT< &KhmDraw::DrawCredWindowOutlineWidget > {
    public:
        CwExposeControlElement(const Point& _p) : CwButtonT(_p) {}

        void OnClick(const Point& p, UINT keyflags, bool doubleClick) {
            TQPARENT(this)->Expand(!TQPARENT(this)->expanded);
            owner->Invalidate();
        }

        bool IsChecked() {
            return TQPARENT(this)->expanded;
        }
    };



    /*! \brief Icon display control

      Displays a static icon.
     */
    class CwIconDisplayElement : public WithFixedSizePos< DisplayElement > {
        Bitmap *i;
        bool large;
    public:
        CwIconDisplayElement(const Point& _p, HICON _icon, bool _large) :
            i(NULL), large(_large) {
            SetPosition(_p);
            SetSize((large)? g_theme->sz_icon : g_theme->sz_icon_sm);

            i = GetBitmapFromHICON(_icon);
        }

        void SetIcon(HICON _icon, bool redraw = true) {
            if (i)
                delete i;

            i = GetBitmapFromHICON(_icon);

            if (redraw)
                Invalidate();
        }

        virtual void PaintSelf(Graphics &g, const Rect& bounds, const Rect& clip) {
            g.DrawImage(i, bounds.X, bounds.Y, bounds.Width, bounds.Height);
        }
    };



    /*! \brief Progress bar control
     */
    class CwProgressBarElement :
        public WithFixedSizePos< DisplayElement > {

        int progress;

    public:
        CwProgressBarElement() {
            SetSize(Size(g_theme->sz_icon_sm.Width * 8, g_theme->sz_icon_sm.Height));
        }

        void SetProgress(int _progress) {
            progress = _progress;
            Invalidate();
        }

        void PaintSelf(Graphics& g, const Rect& bounds, const Rect& clip) {
            g_theme->DrawProgressBar(g, bounds, progress);
        }
    };


    /*! \brief Generic Static (Text) element
     */
    class CwStaticTextElement :
        public GenericTextT< WithStaticCaption < WithTextDisplay <> > > {
    public:
        CwStaticTextElement(const std::wstring& _caption) {
            SetCaption(_caption);
        }
    };


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
