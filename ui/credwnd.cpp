/*
 * Copyright (c) 2008 Secure Endpoints Inc.
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

/* $Id$ */

#include "khmapp.h"
#include<assert.h>

namespace nim
{

#define CW_CANAME_FLAGS L"_CWFlags"

    /*! \brief Credential window column
     */
    class CwColumn : public DisplayColumn
    {
        // Legacy flags
#define KHUI_CW_COL_AUTOSIZE    0x00000001
#define KHUI_CW_COL_SORT_INC    0x00000002
#define KHUI_CW_COL_SORT_DEC    0x00000004
#define KHUI_CW_COL_GROUP       0x00000008
#define KHUI_CW_COL_FIXED_WIDTH 0x00000010
#define KHUI_CW_COL_FIXED_POS   0x00000020
#define KHUI_CW_COL_META        0x00000040
#define KHUI_CW_COL_FILLER      0x00000080

    public:

        khm_int32  attr_id;

    public:
        CwColumn() : DisplayColumn() {
            attr_id = KCDB_ATTR_INVALID;
        }

        khm_int32 GetFlags(void) {
            CheckFlags();
            return
                ((sort && sort_increasing)? KHUI_CW_COL_SORT_INC : 0) |
                ((sort && !sort_increasing)? KHUI_CW_COL_SORT_DEC : 0) |
                ((group)? KHUI_CW_COL_GROUP : 0) |
                ((fixed_width)? KHUI_CW_COL_FIXED_WIDTH : 0) |
                ((fixed_position)? KHUI_CW_COL_FIXED_POS : 0) |
                ((filler)? KHUI_CW_COL_FILLER : 0);
        }

        void SetFlags(khm_int32 f) {
            sort = !!(f & (KHUI_CW_COL_SORT_INC | KHUI_CW_COL_SORT_DEC | KHUI_CW_COL_GROUP));
            sort_increasing = ((f & KHUI_CW_COL_SORT_INC) ||
                               ((f & KHUI_CW_COL_GROUP) && !(f & KHUI_CW_COL_SORT_DEC)));
            group = !!(f & KHUI_CW_COL_GROUP);
            fixed_width = !!(f & KHUI_CW_COL_FIXED_WIDTH);
            fixed_position = !!(f & KHUI_CW_COL_FIXED_POS);
            filler = !!(f & KHUI_CW_COL_FILLER);
            CheckFlags();
        }
    };


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
        Bitmap i;
        bool large;
    public:
        CwIconDisplayElement(const Point& _p, HICON _icon, bool _large) : i(_icon), large(_large) {
            SetPosition(_p);
            SetSize((large)? g_theme->sz_icon : g_theme->sz_icon_sm);
        }

        virtual void PaintSelf(Graphics &g, const Rect& bounds, const Rect& clip) {
            g.DrawImage(&i, bounds);
        }
    };

    /*! \brief Default identity control

      This is the control which allows the user to set an identity as
      the default.  It also indicates whether the current identity is
      default.
     */
    class CwDefaultIdentityElement : public CwButtonT< &KhmDraw::DrawStarWidget > {
        Identity * pidentity;

    public:
        CwDefaultIdentityElement(Identity * _pidentity, const Point& _p) :
            CwButtonT(_p), pidentity(_pidentity) {};

        bool IsChecked() {
            return (pidentity->GetFlags() & KCDB_IDENT_FLAG_DEFAULT);
        }

        bool OnShowToolTip(std::wstring& caption, Rect& align_rect) {
            if (pidentity->GetFlags() & KCDB_IDENT_FLAG_DEFAULT) {
                caption = LoadStringResource(IDS_CWTT_DEFAULT_ID1);
            } else {
                caption = LoadStringResource(IDS_CWTT_DEFAULT_ID0);
            }
            Point pt = MapToScreen(Point(extents.Width, extents.Height));
            align_rect.X = pt.X;
            align_rect.Y = pt.Y;
            align_rect.Width = extents.Width;
            align_rect.Height = extents.Height;

            return true;
        }

        void OnClick(const Point& pt, UINT keyflags, bool doubleClick) {
            if (!(pidentity->GetFlags() & KCDB_IDENT_FLAG_DEFAULT)) {
                pidentity->SetDefault();
            }
        }
    };

    class CwOutline;
    class CwIdentityOutline;
    class CwCredTypeOutline;
    class CwGenericOutline;
    class CwCredentialRow;
    class CwTable;

    CwOutline * CreateOutlineNode(Identity& identity, DisplayColumnList& columns, int column);
    CwOutline * CreateOutlineNode(Credential& cred, DisplayColumnList& columns, int column);

    template < class T >
    bool AllowRecursiveInsert(DisplayColumnList& columns, int column);

    class CwOutlineElement :
        virtual public DisplayElement {};



    /*! \brief Base for all outline objects in the credentials window
     */
    class CwOutlineBase :
        public WithOutline< CwOutlineElement >,
        public TimerQueueClient {

    public:
        CwOutlineBase *insertion_point;
        int indent;

    public:
        CwOutlineBase() {
            insertion_point = NULL;
            indent = 0;
        }

        void BeginUpdate();

        void EndUpdate();

        template <class T>
        CwOutlineBase * Insert(T& obj, DisplayColumnList& columns, unsigned int column);

        virtual bool Represents(Credential& credential) { return false; }
        virtual bool Represents(Identity& identity) { return false; }
        virtual void SetContext(SelectionContext& sctx) = 0;

        template <class U, class V>
        bool FindElements(V& criterion, std::vector<U *>& results);
    };



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


    /*! \brief Identity Title (Text) element

        Used for displaying identity display name in an identity
        outline node.
     */
    class CwIdentityTitleElement :
        public HeaderTextBoxT< WithStaticCaption < WithTextDisplay <> > > {
    public:
        CwIdentityTitleElement(Identity& identity) {
            SetCaption(identity.GetResourceString(KCDB_RES_DISPLAYNAME));
        }
    };

    /*! \brief Identity Type (Text) element

        Displays the type of the identity (KCDB_RES_INSTANCE resource
        of the identity provider).
     */
    class CwIdentityTypeElement :
        public SubheaderTextBoxT< WithStaticCaption < WithTextDisplay <> > > {
    public:
        CwIdentityTypeElement(Identity& identity) {
            SetCaption(identity.GetProvider().GetResourceString(KCDB_RES_INSTANCE));
        }
    };

    /*! \brief Identit Lifetime Meter

        Lifetime meter and also busy/renew indicator.
     */
    class CwIdentityMeterElement :
        public WithFixedSizePos<>, public TimerQueueClient {

        Identity     identity;
        DrawState    state;
        std::wstring caption;

    public:
        CwIdentityMeterElement(Identity& _identity) :
            WithFixedSizePos(Point(0,0), g_theme->sz_meter),
            identity(_identity) {

            state = DrawStateNone;

        }

        void OnClick(const Point& pt, UINT keyflags, bool doubleClick) {
            khm_cred_renew_identity(identity);
        }

        void OnTimer() {
            Invalidate();
        }

        void SetStatus(DrawState _state, const wchar_t * _caption = NULL) {
            if (_caption)
                caption = _caption;
            else
                caption = L"";

            state = _state;

            Invalidate();
        }

        void PaintSelf(Graphics& g, const Rect& bounds, const Rect& clip) {
            CancelTimer();

            switch (state) {
            case DrawStateCritial:
            case DrawStateWarning:
            case DrawStateBusy:
                {
                    DWORD ms = 0;

                    if (state == DrawStateBusy)
                        g_theme->DrawCredMeterBusy(g, bounds, &ms);
                    else
                        g_theme->DrawCredMeterState(g, bounds, state, &ms);

                    if (ms != 0) {
                        TimerQueueHost * host = dynamic_cast<TimerQueueHost *>(owner);
                        host->SetTimer(this, ms);
                    }
                }
                break;

            default:
                {
                    FILETIME ftnow;
                    khm_int64 now;
                    khm_int64 expire;
                    DWORD ms = 0;

                    GetSystemTimeAsFileTime(&ftnow);

                    now = FtToInt(&ftnow) + SECONDS_TO_FT(TT_TIMEEQ_ERROR_SMALL);

                    expire = identity.GetAttribFileTimeAsInt(KCDB_ATTR_EXPIRE);

                    if (expire < now) {
                        g_theme->DrawCredMeterState(g, bounds, DrawStateExpired, &ms);
                    } else {
                        khm_int64 lifetime;

                        lifetime = identity.GetAttribFileTimeAsInt(KCDB_ATTR_LIFETIME);

                        if (lifetime == 0) {
                            g_theme->DrawCredMeterState(g, bounds, DrawStateExpired, &ms);
                        } else if (expire - lifetime > now) {
                            g_theme->DrawCredMeterState(g, bounds, DrawStatePostDated, &ms);
                        } else {
                            khm_int64 remainder;

                            remainder = expire - now;

                            g_theme->DrawCredMeterLife(g, bounds,
                                                       (unsigned int) ((remainder * 256) / lifetime));
                            remainder = FT_TO_MS(remainder);
                            lifetime = FT_TO_MS(lifetime);

                            khm_int64 q = lifetime / 256;

                            if (q != 0) {
                                ms = (DWORD) (remainder % q);

                                if (ms < 100) {
                                    ms += (DWORD) q;
                                }
                            }

                            if (ms > 0) {
                                TimerQueueHost * host = dynamic_cast<TimerQueueHost *>(owner);
                                host->SetTimer(this, ms);
                            }
                        }
                    }
                }
            }
        }
    };


    /*! \brief Identity Status (Text) element
     */
    class CwIdentityStatusElement :
        public IdentityStatusTextT< WithStaticCaption< WithTextDisplay <> > >,
        public TimerQueueClient {

        Identity        identity;
        long            ms_to_next_change;
        bool            use_static_caption;

    public:
        CwIdentityStatusElement(Identity& _identity) : identity(_identity) {
            ms_to_next_change   = 0;
            use_static_caption  = false;
        }

        void SetCaption(const std::wstring& _caption) {
            use_static_caption = (_caption.length()  != 0);
            __super::SetCaption(_caption);
        }

        std::wstring GetCaption() {

            if (use_static_caption) {

                return __super::GetCaption();

            } else if (identity.GetAttribInt32(KCDB_ATTR_N_IDCREDS) == 0 ||
                       !identity.Exists(KCDB_ATTR_EXPIRE)) {

                return std::wstring(L"(Unknown expiration)");

            } else {
                FILETIME ft_exp;
                FILETIME ft_now;

                identity.GetObject(KCDB_ATTR_EXPIRE, ft_exp);
                GetSystemTimeAsFileTime(&ft_now);

                if (CompareFileTime(&ft_now, &ft_exp) >= 0) {
                    return LoadStringResource(IDS_CW_EXPIRED); // "(Expired)"
                } else {
                    wchar_t fmt[32];

                    // "Valid for %s"
                    LoadStringResource(fmt, IDS_CW_EXPIREF);

                    FILETIME ft_timeleft;
                    wchar_t timeref[128];
                    khm_size cb;

                    ft_timeleft = FtSub(&ft_exp, &ft_now);
                    cb = sizeof(timeref);
                    FtToStringEx(&ft_timeleft, FTSE_INTERVAL, &ms_to_next_change,
                                 timeref, &cb);

                    wchar_t status_msg[160];
                    StringCbPrintf(status_msg, sizeof(status_msg), fmt, timeref);

                    return std::wstring(status_msg);
                }
            }
        }

        void PaintSelf(Graphics& g, const Rect& bounds, const Rect& clip) {

            CancelTimer();
            ms_to_next_change = 0;

            __super::PaintSelf(g, bounds, clip);

            if (ms_to_next_change != 0) {
                TimerQueueHost * host = dynamic_cast<TimerQueueHost *>(owner);
                host->SetTimer(this, ms_to_next_change);
            }
        }

        void OnTimer() {
            Invalidate();
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



    /*! \brief  Identity Outline Node
     */
    class CwIdentityOutline : public CwOutline
    {
    public:
        Identity                  identity;
        CwIconDisplayElement     *el_icon;
        CwDefaultIdentityElement *el_default_id;
        CwIdentityTitleElement   *el_display_name;
        CwIdentityTypeElement    *el_type_name;
        CwIdentityStatusElement  *el_status;
        CwIdentityMeterElement   *el_meter;
        CwProgressBarElement     *el_progress;

        bool                     monitor_progress;

    public:
        CwIdentityOutline(Identity& _identity, int _column)
            : identity(_identity), CwOutline(_column) {
            monitor_progress    = false;

            InsertChildAfter(el_icon =
                             new CwIconDisplayElement(Point(0,0),
                                                      identity.GetResourceIcon(KCDB_RES_ICON_NORMAL),
                                                      true));

            InsertChildAfter(el_default_id =
                             new CwDefaultIdentityElement(&identity, Point(0,16)));

            InsertChildAfter(el_display_name =
                             new CwIdentityTitleElement(identity));

            InsertChildAfter(el_type_name =
                             new CwIdentityTypeElement(identity));

            InsertChildAfter(el_meter =
                             new CwIdentityMeterElement(identity));

            InsertChildAfter(el_status =
                             new CwIdentityStatusElement(identity));

            InsertChildAfter(el_progress =
                             new CwProgressBarElement());
        }

        ~CwIdentityOutline() {
            el_icon             = NULL;
            el_default_id       = NULL;
            el_display_name     = NULL;
            el_type_name        = NULL;
            el_status           = NULL;
            el_meter            = NULL;
            el_progress         = NULL;
        }

        virtual bool Represents(Credential& credential) {
            return credential.GetIdentity() == identity;
        }

        virtual bool Represents(Identity& _identity) {
            return identity == _identity;
        }

        virtual void UpdateLayoutPre(Graphics& g, Rect& layout) {
            LayoutOutlineSetup(g, layout);
        }

        virtual void UpdateLayoutPost(Graphics& g, const Rect& layout) {
            bool has_creds = false;

            FlowLayout l(layout, g_theme->sz_margin);

            FlowLayout ll = l.InsertFixedColumn(g_theme->sz_icon.Width);

            ll
                .Add(el_icon, FlowLayout::Center)
                .LineBreak()
                .Add(el_meter, FlowLayout::Center, FlowLayout::Fixed,
                     (has_creds = (identity.GetAttribInt32(KCDB_ATTR_N_IDCREDS) > 0)))
                ;

            FlowLayout lr = l.InsertFillerColumn();

            lr
                .Add(el_default_id, FlowLayout::Left)
                .Add(el_display_name, FlowLayout::Left, FlowLayout::Squish)
                .Add(el_type_name, FlowLayout::Right, FlowLayout::Squish)
                .LineBreak()
                .Add(el_status, FlowLayout::Left, FlowLayout::Squish, has_creds)
                .Add(el_progress, FlowLayout::Right, FlowLayout::Squish, monitor_progress)
                .LineBreak()
                ;

            Rect r;

            r = lr.GetInsertRect();
            LayoutOutlineChildren(g, r);
            lr.InsertRect(r);

            r = l.GetBounds();
            extents.Width = r.GetRight();
            extents.Height = r.GetBottom();
        }

        virtual DrawState GetDrawState() {
            return (DrawState)(GetIdentityDrawState(identity) | __super::GetDrawState());
        }

        void SetContext(SelectionContext& sctx) {
            sctx.Add(identity);
            dynamic_cast<CwOutlineBase*>(TQPARENT(this))->SetContext(sctx);
        }

        void SetIdentityOp(khui_nc_subtype stype) {
            switch (stype) {
            case KHUI_NC_SUBTYPE_NONE:
                monitor_progress = false;
                el_status->SetCaption(L"");
                el_meter->SetStatus(DrawStateNone);
                MarkForExtentUpdate();
                Invalidate();
                break;

            case KHUI_NC_SUBTYPE_NEW_CREDS:
                monitor_progress = true;
                el_progress->SetProgress(0);
                el_status->SetCaption(L"Obtaining new credentials");
                el_meter->SetStatus(DrawStateBusy);
                MarkForExtentUpdate();
                Invalidate();
                break;

            case KHUI_NC_SUBTYPE_RENEW_CREDS:
                monitor_progress = true;
                el_progress->SetProgress(0);
                el_status->SetCaption(L"Renewing credentials");
                el_meter->SetStatus(DrawStateBusy);
                MarkForExtentUpdate();
                Invalidate();
                break;

            case KHUI_NC_SUBTYPE_PASSWORD:
                monitor_progress = true;
                el_progress->SetProgress(0);
                el_status->SetCaption(L"Changing password");
                el_meter->SetStatus(DrawStateBusy);
                MarkForExtentUpdate();
                Invalidate();
                break;

            case KHUI_NC_SUBTYPE_ACQPRIV_ID:
                monitor_progress = true;
                el_progress->SetProgress(0);
                el_status->SetCaption(L"Acquiring privileged credentials");
                el_meter->SetStatus(DrawStateBusy);
                MarkForExtentUpdate();
                Invalidate();
                break;


            default:
                // Includes KHUI_NC_SUBTYPE_OTHER

                monitor_progress = true;
                el_progress->SetProgress(0);
                el_status->SetCaption(L"Busy");
                el_meter->SetStatus(DrawStateBusy);
                MarkForExtentUpdate();
                Invalidate();
                break;
            }
        }

        void SetIdentityOpProgress(int progress) {
            if (!monitor_progress) {
                SetIdentityOp(KHUI_NC_SUBTYPE_OTHER);
            }
            el_progress->SetProgress(progress);
        }
    };


    /*! \brief Credential Type (Text) element
     */
    class CwCredentialTypeElement :
        public SubheaderTextBoxT< WithStaticCaption < WithTextDisplay <> > > {
    public:
        CwCredentialTypeElement(CredentialType& credtype) {
            SetCaption(credtype.GetResourceString(KCDB_RES_DISPLAYNAME));
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


    /*! \brief Credential Type Outline Node
     */
    class CwCredTypeOutline : public CwOutline
    {
    public:
        CredentialType credtype;
        CwCredentialTypeElement * el_typename;

    public:
        CwCredTypeOutline(CredentialType& _credtype, int _column) :
            credtype(_credtype),
            CwOutline(_column) {

            InsertChildAfter(el_typename =
                             new CwCredentialTypeElement(_credtype));
        }

        virtual bool Represents(Credential& credential) {
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


    /*! \brief Generic Outline Node
     */
    class CwGenericOutline : public CwOutline
    {
    public:
        Credential credential;
        khm_int32  attr_id;
        CwStaticTextElement * el_text;

    public:
        CwGenericOutline(Credential& _credential, khm_int32 _attr_id, int _column) :
            credential(_credential),
            attr_id(_attr_id),
            CwOutline(_column) {

            InsertChildAfter(el_text =
                             new CwStaticTextElement(credential.GetAttribStringObj(attr_id)));
        }

        ~CwGenericOutline() {}

        virtual bool Represents(Credential& _credential) {
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


    /*! \brief Credential Row Outline Node
     */
    class CwCredentialRow : public CwOutline
    {
    public:
        Credential  credential;
        std::vector<DisplayElement *> el_list;

    public:
        CwCredentialRow(Credential& _credential, int _column) :
            credential(_credential),
            CwOutline(_column) {
        }

        ~CwCredentialRow() {}

        virtual bool Represents(Credential& _credential) {
            return credential == _credential;
        }

        virtual void UpdateLayoutPre(Graphics& g, Rect& layout) {

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
                        e = new CwTimeTextCellElement(credential, cwc->attr_id,
                                                      FTSE_INTERVAL, i);
                        break;

                    default:
                        e = new CwStaticCellElement(credential.GetAttribStringObj(cwc->attr_id), i);
                    }

                    InsertChildAfter(e);
                    el_list.push_back(e);
                }
            }
        }

        virtual void UpdateLayoutPost(Graphics& g, const Rect& layout) {
            DisplayElement::UpdateLayoutPost(g, layout);
        }

        virtual void PaintSelf(Graphics& g, const Rect& bounds, const Rect& clip) {
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

#define DCL_KMSG(T,ST)                                                  \
    khm_int32 OnKmsg ## T ## _ ## ST (khm_ui_4 uparam, void * vparam)
#define DEFINE_KMSG(T,ST)                                               \
    khm_int32 CwTable::OnKmsg ## T ## _ ## ST (khm_ui_4 uparam, void * vparam)

#define HANDLE_KMSG(T,ST)                                               \
    if (msg_type == KMSG_ ## T && msg_subtype == KMSG_ ## T ## _ ## ST) \
        return OnKmsg ## T ## _ ## ST (uparam, vparam)

#pragma warning(push)
    // C4250 is 'class A' : inherits 'A::method()' via dominance

    // It gets triggered a lot because of virtual overrides in
    // DisplayContainer and CwOutlineBase.  This is expected.
#pragma warning(disable: 4250)




    /*! \brief Credential Window
     */
    class CwTable : public WithVerticalLayout< WithNavigation< WithTooltips< DisplayContainer > > >,
                    public CwOutlineBase,
                    public TimerQueueHost
    {
    public:
        // Credentials selection
        Identity   filter_by_identity; // Only if we are filtering by identity

        ConfigSpace cs_view;

        CredentialSet credset;

        // Flags
        bool   is_identity_view: 1; // Is this a compact identity view?
        bool   is_primary_view : 1; // Is this the primary credentials view?
        bool   is_custom_view  : 1; // Has this view been customized?
        bool   view_all_idents : 1; // View all identities? (only valid if is_identity_view)
        bool   columns_dirty   : 1; // Have the columns changed?
        bool   skipped_columns : 1; // Have any columns been skipped?

    public:
        CwTable() {}

        ~CwTable() {}

        void SaveView();
        void LoadView(const wchar_t * view_name = NULL);
        void UpdateCredentials();
        void UpdateOutline();

        void InsertDerivedIdentities(CwOutlineBase * o, Identity * id);
        void ToggleColumnByAttributeId(khm_int32 attr_id);

    public:
        typedef struct CwCreateParams {
            bool is_primary_view;
        };

        // createParams is a pointer to CwCreateParams
        virtual BOOL OnCreate(LPVOID createParams);

    public:                     // Overrides
        virtual void OnDestroy();
        virtual void OnColumnPosChanged(int from, int to);
        virtual void OnColumnSizeChanged(int idx);
        virtual void OnColumnSortChanged(int idx);
        virtual void OnColumnContextMenu(int idx, const Point& p);
        virtual khm_int32 OnWmDispatch(khm_int32 msg_type, khm_int32 msg_subtype,
                                       khm_ui_4 uparam, void * vparam);
        virtual void OnCommand(int id, HWND hwndCtl, UINT codeNotify);
        virtual void OnContextMenu(const Point& p);

        virtual DWORD GetStyle();
        virtual DWORD GetStyleEx();

        virtual HFONT GetHFONT();

        virtual void PaintSelf(Graphics& g, const Rect& bounds, const Rect& clip);

        virtual void SetContext(SelectionContext& sctx);

    public:                     // Handlers for KMSG_*
        DCL_KMSG(CRED, ROOTDELTA);
        DCL_KMSG(CRED, PP_BEGIN);
        DCL_KMSG(CRED, PP_PRECREATE);
        DCL_KMSG(CRED, PP_END);
        DCL_KMSG(CRED, PP_DESTROY);
        DCL_KMSG(KCDB, IDENT);
        DCL_KMSG(KCDB, ATTRIB);
        DCL_KMSG(KMM, I_DONE);
        DCL_KMSG(ACT, ACTIVATE);
        DCL_KMSG(CREDP, BEGIN_NEWCRED);
        DCL_KMSG(CREDP, END_NEWCRED);
        DCL_KMSG(CREDP, PROG_NEWCRED);

    public:
        static khm_int32 attrib_to_action[KCDB_ATTR_MAX_ID + 1];
        static void RefreshGlobalColumnMenu(HWND hwnd);
    };

#pragma warning(pop)


    // Implementation section

    khm_int32 CwTable::attrib_to_action[KCDB_ATTR_MAX_ID + 1] = {0};

    template <class T>
    bool AllowRecursiveInsert(DisplayColumnList& columns, int column)
    {
        return false;
    }

    template <>
    bool AllowRecursiveInsert<Credential>(DisplayColumnList& columns, int column)
    {
        return (column >= 0 && (unsigned) column < columns.size() && columns[column]->group);
    }

#define for_each_child(c)                                               \
    for (CwOutline * c = NextOutline(TQFIRSTCHILD(this)),               \
             *nc = ((c != NULL)?NextOutline(TQNEXTSIBLING(c)):NULL);    \
         c != NULL;                                                     \
         c = nc, nc = ((c != NULL)?NextOutline(TQNEXTSIBLING(c)):NULL))

    template <class T>
    CwOutlineBase * CwOutlineBase::Insert(T& obj, DisplayColumnList& columns, unsigned int column) 
    {
        CwOutlineBase * target = NULL;
        bool insert_new = false;

        for_each_child(c) {
            if (c == insertion_point)
                insert_new = true;

            if (c->Represents(obj)) {
                if (insert_new) {
                    if (c == insertion_point) {
                        insertion_point = dynamic_cast<CwOutlineBase*>(TQNEXTSIBLING(insertion_point));
                    } else {
                        MoveChildBefore(c, insertion_point);
                    }
                }
                target = c;
                break;
            }
        }

        if (target == NULL) {
            target = CreateOutlineNode(obj, columns, column);
            InsertOutlineBefore(target, insertion_point);
            // TODO: set the indent
        }

        if (AllowRecursiveInsert<T>(columns, column)) {
            target->Insert(obj, columns, ++column);
        }

        return target;
    }

    void CwOutlineBase::BeginUpdate() 
    {
        insertion_point = NextOutline(TQFIRSTCHILD(this));
        CancelTimer();

        for_each_child(c)
            c->BeginUpdate();
    }

    void CwOutlineBase::EndUpdate()
    {
        CwOutlineBase * c;

        for (insertion_point = NextOutline(insertion_point);
             insertion_point;) {
            c = insertion_point;
            insertion_point = NextOutline(TQNEXTSIBLING(c));
            DeleteChild(c);
        }

        for_each_child(c)
            c->EndUpdate();
    }

    void CwOutline::LayoutOutlineSetup(Graphics& g, Rect& layout)
    {
        __super::UpdateLayoutPre(g, layout);

        expandable = (owner != NULL &&
                      (unsigned)col_idx + 1 < owner->columns.size() &&
                      !!(NextOutline(TQFIRSTCHILD(this))));

        if (expandable && outline_widget == NULL) {
            outline_widget = new CwExposeControlElement(g_theme->pt_margin_cx + g_theme->pt_margin_cy);
            InsertChildAfter(outline_widget);
        }

        layout.Width = extents.Width;

        if (expandable) {
            outline_widget->visible = true;
            layout.X = g_theme->sz_margin.Width * 2 + g_theme->sz_icon_sm.Width;
            layout.Width -= layout.X;
        } else {
            if (outline_widget != NULL)
                outline_widget->visible = false;
            layout.X = g_theme->sz_margin.Width;
            layout.Width -= layout.X;
            expanded = false;

            for_each_child(c) {
                if (c->visible)
                    c->Show(false);
            }
        }

        layout.Y = g_theme->sz_margin.Height;
    }

    void CwOutline::LayoutOutlineChildren(Graphics& g, Rect& layout)
    {
        Point p;

        layout.GetLocation(&p);
        layout.Height = 0;

        for_each_child(c) {
            c->visible = expanded;
            if (!c->visible)
                continue;

            c->origin.Y = p.Y;
            p.Y += c->extents.Height;
            layout.Height += c->extents.Height;
            if (c->extents.Width > layout.Width)
                layout.Width = c->extents.Width;
        }
    }

    void CwOutline::PaintSelf(Graphics& g, const Rect& bounds, const Rect& clip)
    {
        g_theme->DrawCredWindowOutline(g, bounds, GetDrawState());
    }

    CwOutline * CreateOutlineNode(Identity & identity, DisplayColumnList& columns, int column)
    {
        assert((unsigned) column >= columns.size() ||
               (static_cast<CwColumn *>(columns[column]))->attr_id == KCDB_ATTR_ID_DISPLAY_NAME);
        return new CwIdentityOutline(identity, column);
    }

    void CwOutline::Select(bool _select) {
        if (selected == _select)
            return;

        __super::Select(_select);
        for_each_child(c) {
            c->Select(_select);
        }
    }

    CwOutline * CreateOutlineNode(Credential & cred, DisplayColumnList& columns, int column)
    {
        CwColumn * cwcol = NULL;

        if ((unsigned) column < columns.size())
            cwcol = static_cast<CwColumn *>(columns[column]);

        if (cwcol == NULL || !cwcol->group)
            return new CwCredentialRow(cred, column);

        switch (cwcol->attr_id) {
        case KCDB_ATTR_ID_DISPLAY_NAME:
            return new CwIdentityOutline(cred.GetIdentity(), column);

        case KCDB_ATTR_TYPE_NAME:
            return new CwCredTypeOutline(cred.GetType(), column);

        default:
            return new CwGenericOutline(cred, cwcol->attr_id, column);
        }
    }

    template <class elementT, class queryT>
    bool CwOutlineBase::FindElements(queryT& criterion, std::vector<elementT *>& results) {
        elementT * addend;
        if (Represents(criterion) &&
            (addend = dynamic_cast<elementT *>(this)) != NULL) {
            results.push_back(addend);
        }

        for_each_child(c) {
            c->FindElements(criterion, results);
        }

        return results.size() > 0;
    }

    void CwTable::PaintSelf(Graphics& g, const Rect& bounds, const Rect& clip)
    {
        g_theme->DrawCredWindowBackground(g, bounds, clip);
    }

    void
    CwTable::RefreshGlobalColumnMenu(HWND hwnd)
    {
        khui_menu_def * column_menu = khui_find_menu(KHUI_MENU_COLUMNS);

        for (khm_int32 i=0; i <= KCDB_ATTR_MAX_ID; i++) {

            kcdb_attrib * attr_info = NULL;

            if (KHM_FAILED(kcdb_attrib_get_info(i, &attr_info)) ||

                (attr_info->flags & KCDB_ATTR_FLAG_HIDDEN) != 0 ||

                (attr_info->short_desc == NULL && attr_info->long_desc == NULL)) {
                
                if (attrib_to_action[i] != 0) {

                    // This attribute should be removed from the action table

                    khui_menu_remove_action(column_menu, attrib_to_action[i]);

                    khui_action_delete(attrib_to_action[i]);

                    attrib_to_action[i] = 0;
                }

            } else {

                if (attrib_to_action[i] == 0) {

                    // This attribute should be added to the action table

                    khm_handle sub = NULL;
                    khm_int32  act;

                    kmq_create_hwnd_subscription(hwnd, &sub);

                    act = khui_action_create(attr_info->name,
                                             (attr_info->short_desc?
                                              attr_info->short_desc: attr_info->long_desc),
                                             NULL,
                                             (void *)(UINT_PTR) i,
                                             KHUI_ACTIONTYPE_TOGGLE,
                                             sub);
                    
                    attrib_to_action[i] = act;

                    khui_menu_insert_action(column_menu, 5000, act, 0);
                }

            }

            if (attr_info)
                kcdb_attrib_release_info(attr_info);
        }
    }

    void
    CwTable::SaveView()
    {
        multi_string column_list;

        /* if we aren't saving to a specific view, and the view has
           been customized, then we save it to "Custom_0", unless we
           are in the mini mode, in which case we save it to
           "Custom_1" */

        if (is_custom_view) {

            ConfigSpace cs_cw(L"CredWindow", KHM_PERM_READ | KHM_PERM_WRITE);
            ConfigSpace cs_views(cs_cw, L"Views", KHM_PERM_READ);

            cs_view.Open(cs_views, (is_identity_view)? L"Custom_1" : L"Custom_0",
                         KHM_PERM_WRITE | KHM_FLAG_CREATE | KCONF_FLAG_WRITEIFMOD);
            cs_cw.Set((is_identity_view)? L"DefaultViewMini": L"DefaultView",
                      (is_identity_view)? L"Custom_1" : L"Custom_0");

        }

        ConfigSpace cs_columns;
        cs_columns.Open(cs_view, L"Columns", KHM_PERM_WRITE | KHM_FLAG_CREATE);

        cs_view.Set(L"ExpandedIdentity", static_cast<khm_int32>(!!is_identity_view));
        cs_view.Set(L"NoHeader", static_cast<khm_int32>(!show_header));

        for (DisplayColumnList::iterator column = columns.begin();
             column != columns.end();
             ++column) {

            AttributeInfo attr_info((static_cast<CwColumn*>(*column))->attr_id);

            if (attr_info.InError())
                continue;

            column_list.push_back(new std::wstring(attr_info->name));

            ConfigSpace cs_col(cs_columns, attr_info->name,
                               KHM_PERM_WRITE | KHM_FLAG_CREATE | KCONF_FLAG_WRITEIFMOD);

            cs_col.Set(L"Width", (*column)->width);
            cs_col.Set(L"Flags", (static_cast<CwColumn *>(*column))->GetFlags());
        }

        cs_view.Set(L"ColumnList", column_list);

        {
            khm_version v = app_version;

            cs_view.Set(L"_AppVersion", &v, sizeof(v));
        }
    }

    void 
    CwTable::LoadView(const wchar_t * cview)
    {
        std::wstring view_name;
        khm_int32 open_flags = KHM_PERM_READ;

        ConfigSpace cs_cw(L"CredWindow", KHM_PERM_READ | KHM_PERM_WRITE);

        is_identity_view = (khm_main_wnd_mode == KHM_MAIN_WND_MINI);

        if (cview == NULL) {
            if (KHM_SUCCEEDED(cs_view.GetLastError()))
                goto have_view;
            view_name = cs_cw.GetString((is_identity_view)? L"DefaultViewMini" : L"DefaultView");
        } else {
            view_name = cview;
            cs_cw.Set((is_identity_view)? L"DefaultViewMini" : L"DefaultView", view_name);
        }

        {
            ConfigSpace cs_views(cs_cw, L"Views", KHM_PERM_READ);

            cs_view.Open(cs_views, view_name.c_str(), KHM_PERM_READ);

            /* view data is very sensitive to version changes.  We
               need to check if this configuration data was created 
               with this version of NetIDMgr.  If not, we switch to
               using a schema handle. */
            {
                khm_version cfg_v;

                cs_view.GetObject(L"_AppVersion", cfg_v);

                if (khm_compare_version(&cfg_v, &app_version) != 0) {

                    std::wstring base = ((is_identity_view)? L"CompactIdentity" : L"ByIdentity");

                    cs_view.Close();
                    
                    if (KHM_FAILED(cs_view.Open(cs_views, view_name.c_str(), KCONF_FLAG_SCHEMA))) {
                        cs_view.Open(cs_views, base.c_str(), KCONF_FLAG_SCHEMA);
                        view_name = base;
                    }

                    open_flags = KCONF_FLAG_SCHEMA;
                }
            }

            if (KHM_FAILED(cs_view.GetLastError()))
                return;
        }

    have_view:

        // Begin loading the view

        if (!is_primary_view)
            ;                   // do nothing
        else if (view_name.compare(L"ByIdentity") == 0)
            khui_check_radio_action(khui_find_menu(KHUI_MENU_LAYOUT),
                                    KHUI_ACTION_LAYOUT_ID);
        else if (view_name.compare(L"ByLocation") == 0)
            khui_check_radio_action(khui_find_menu(KHUI_MENU_LAYOUT),
                                    KHUI_ACTION_LAYOUT_LOC);
        else if (view_name.compare(L"ByType") == 0)
            khui_check_radio_action(khui_find_menu(KHUI_MENU_LAYOUT),
                                    KHUI_ACTION_LAYOUT_TYPE);
        else if (view_name.compare(L"Custom_0") == 0 || view_name.compare(L"Custom_1") == 0)
            khui_check_radio_action(khui_find_menu(KHUI_MENU_LAYOUT),
                                    KHUI_ACTION_LAYOUT_CUST);
        else {
            /* do nothing */
        }

        ConfigSpace cs_columns(cs_view, L"Columns", open_flags);
        multi_string column_list = cs_view.GetMultiString(L"ColumnList");

        columns.Clear();
        columns.reserve(column_list.size());

        is_custom_view   = false;
        show_header      = !cs_view.GetInt32(L"NoHeader");
        view_all_idents  = !!cs_cw.GetInt32(L"ViewAllIdents");
        is_identity_view = !!cs_view.GetInt32(L"ExpandedIdentity");
        skipped_columns  = false;

        if (is_primary_view) {
            khui_check_action(KHUI_ACTION_VIEW_ALL_IDS, view_all_idents);
            khui_refresh_actions();
        }

        for (multi_string::iterator colname = column_list.begin();
             colname != column_list.end();
             ++colname) {

            AttributeInfo info((*colname)->c_str());
            ConfigSpace cs_col(cs_columns, (*colname)->c_str(), open_flags);
            CwColumn * cw_col = NULL;

            if (KHM_FAILED(cs_col.GetLastError()))
                continue;

            cw_col = new CwColumn;

            if ((*colname)->compare(CW_CANAME_FLAGS) == 0) {
                // Ignore
                delete cw_col;
                continue;
            } else if (KHM_FAILED(info.GetLastError())) {
                skipped_columns = true;
                delete cw_col;
                continue;
            } else {
                cw_col->attr_id = info->id;
                cw_col->caption = info.GetDescription(KCDB_TS_SHORT);
                if (is_primary_view &&
                    cw_col->attr_id >= 0 && cw_col->attr_id <= KCDB_ATTR_MAX_ID &&
                    attrib_to_action[cw_col->attr_id] != 0)
                    khui_check_action(attrib_to_action[cw_col->attr_id], TRUE);
            }

            cw_col->width = cs_col.GetInt32(L"Width", -1);
            cw_col->SetFlags(cs_col.GetInt32(L"Flags", 0));

            columns.push_back(cw_col);
        }

        columns.ValidateColumns();

        if (is_identity_view &&
            (columns.size() != 1 ||
             static_cast<CwColumn*>(columns[0])->attr_id != KCDB_ATTR_ID_DISPLAY_NAME ||
             !columns[0]->group)) {
            assert(FALSE);
            is_identity_view = false;
        }

        columns_dirty = TRUE;

        if (is_primary_view)
            khui_refresh_actions();

        if (hwnd_header)
            columns.AddColumnsToHeaderControl(hwnd_header);

        MarkForExtentUpdate();
    }

    void
    CwTable::UpdateCredentials()
    {
        kcdb_cred_comp_field * fields;
        kcdb_cred_comp_order comp_order;
        khm_int32 n;

        credset.Purge();

        kcdb_identity_refresh_all();

        credset.Collect(NULL, static_cast<khm_handle>(filter_by_identity),
                        KCDB_CREDTYPE_ALL, NULL);

        /* now we need to figure out how to sort the credentials */
        fields = static_cast<kcdb_cred_comp_field *>
            (PCALLOC(columns.size(), sizeof(kcdb_cred_comp_field)));

        n = 0;
        for (DisplayColumnList::iterator col = columns.begin();
             col != columns.end(); ++col) {

            if (!(*col)->sort)
                continue;

            fields[n].attrib = (static_cast<CwColumn *>(*col))->attr_id;
            fields[n].order = ((*col)->sort_increasing)?
                KCDB_CRED_COMP_INCREASING : KCDB_CRED_COMP_DECREASING;
            if (static_cast<CwColumn*>(*col)->attr_id == KCDB_ATTR_NAME ||
                static_cast<CwColumn*>(*col)->attr_id == KCDB_ATTR_TYPE_NAME)
                fields[n].order |= KCDB_CRED_COMP_INITIAL_FIRST;
            n++;
        }

        if (n > 0) {
            comp_order.nFields = n;
            comp_order.fields = fields;

            credset.Sort(kcdb_cred_comp_generic, &comp_order);
        }

        if (fields) {
            PFREE(fields);
            fields = NULL;
        }
    }

    struct InsertCredentialProcData {
        CwTable       *table;
        CwOutlineBase *target;
        Identity      *filter_by;
        int column;
    };

    static khm_int32 KHMCALLBACK InsertCredentialProc(Credential& credential, void * p)
    {
        InsertCredentialProcData *d = static_cast<InsertCredentialProcData *>(p);

        if (d->filter_by == NULL || credential.GetIdentity() == *d->filter_by)
            d->target->Insert(credential, d->table->columns, d->column);
        return KHM_ERROR_SUCCESS;
    }

    static khm_int32 KHMAPI IdentityNameComparator(const Identity& i1, const Identity& i2,
                                                   void * param)
    {
        std::wstring dn1 = i1.GetResourceString(KCDB_RES_DISPLAYNAME);
        std::wstring dn2 = i2.GetResourceString(KCDB_RES_DISPLAYNAME);

        if (dn1 < dn2)
            return -1;
        else if (dn1 == dn2)
            return 0;
        return 1;
    }

    void
    CwTable::InsertDerivedIdentities(CwOutlineBase * o, Identity * id)
    {
        Identity::Enumeration e = Identity::Enum(0,0);
        e.Sort(IdentityNameComparator);

        for (; !e.AtEnd(); ++e) {
            if ((id == NULL && static_cast<khm_handle>(e->GetParent()) == NULL) ||
                (id != NULL && e->GetParent() == *id)) {

                // Match
                CwOutlineBase * co = o->Insert(*e, columns, 0);

                InsertCredentialProcData d;
                d.table = this;
                d.target = co;
                d.column = 1;
                d.filter_by = &(*e);
                credset.Apply(InsertCredentialProc, &d);

                InsertDerivedIdentities(co, &(*e));
            }
        }
    }

    void
    CwTable::UpdateOutline()
    {
        int ordinal = 0;

        /*  this is called after calling UpdateCredentials(), so we
            assume that the credentials are all loaded and sorted
            according to grouping rules  */

        if (columns_dirty) {
            DeleteAllChildren();
        }

        BeginUpdate();

        if (is_identity_view) {

            InsertDerivedIdentities(this, NULL);

        } else {
            if (columns.size() > 0 &&
                dynamic_cast<CwColumn *>(columns[0])->attr_id == KCDB_ATTR_ID_DISPLAY_NAME &&
                columns[0]->group) {

                if (!view_all_idents) {
                    Identity::Enumeration e = Identity::Enum(KCDB_IDENT_FLAG_DEFAULT,
                                                             KCDB_IDENT_FLAG_DEFAULT);
                    e.Sort(IdentityNameComparator);

                    for (; !e.AtEnd(); ++e) {
                        this->Insert(*e, columns, 0);
                    }
                } else {
                    Identity::Enumeration e = Identity::Enum(0,0);
                    e.Sort(IdentityNameComparator);

                    for (; !e.AtEnd(); ++e) {
                        this->Insert(*e, columns, 0);
                    }
                }
            }

            InsertCredentialProcData d;
            d.table = this;
            d.target = this;
            d.column = 0;
            d.filter_by = NULL;

            credset.Apply(InsertCredentialProc, &d);
        }

        EndUpdate();

        columns_dirty = false;

        // Update the notification icon
        {
            IdentityProvider p = IdentityProvider::GetDefault();
            Identity i = p.GetDefaultIdentity();

            if (p.IsValid() && i.IsValid()) {
                DrawState ds = GetIdentityDrawState(i);
                khm_notif_expstate expstate = KHM_NOTIF_EMPTY;

                if (ds & DrawStateDisabled)
                    expstate = KHM_NOTIF_EMPTY;
                else if (ds & DrawStateWarning)
                    expstate = KHM_NOTIF_WARN;
                else if (ds & DrawStateExpired)
                    expstate = KHM_NOTIF_EXP;
                else
                    expstate = KHM_NOTIF_OK;

                khm_notify_icon_expstate(expstate);
            } else {
                khm_notify_icon_expstate(KHM_NOTIF_EMPTY);
            }
        }
    }

    BOOL CwTable::OnCreate(LPVOID createParams)
    {
        CwCreateParams * cparams;

        cparams = reinterpret_cast<CwCreateParams *>(createParams);

        if (cparams && cparams->is_primary_view)
            is_primary_view = true;

        if (is_primary_view)
            RefreshGlobalColumnMenu(hwnd);

        LoadView(NULL);

        UpdateCredentials();
        UpdateOutline();

        // TODO: Select the cursor row
        // TODD: Update selection state

        kmq_subscribe_hwnd(KMSG_CRED, hwnd);
        kmq_subscribe_hwnd(KMSG_KCDB, hwnd);
        kmq_subscribe_hwnd(KMSG_KMM, hwnd);
        kmq_subscribe_hwnd(KMSG_CREDP, hwnd);

        return __super::OnCreate(createParams);
    }

    void CwTable::OnDestroy()
    {
        kmq_unsubscribe_hwnd(KMSG_CREDP, hwnd);
        kmq_unsubscribe_hwnd(KMSG_CRED, hwnd);
        kmq_unsubscribe_hwnd(KMSG_KCDB, hwnd);
        kmq_unsubscribe_hwnd(KMSG_KMM, hwnd);
    }

    void CwTable::OnColumnPosChanged(int from, int to)
    {
        is_custom_view = true;
        columns_dirty = true;
        UpdateOutline();
        SaveView();
    }

    void CwTable::OnColumnSizeChanged(int order)
    {
        is_custom_view = true;
        SaveView();
    }

    void CwTable::OnColumnSortChanged(int order)
    {
        is_custom_view = true;
        columns_dirty = true;
        UpdateOutline();
        SaveView();
    }

    void CwTable::SetContext(SelectionContext& sctx)
    {
        sctx.SetContext(credset);
    }

    void CwTable::OnColumnContextMenu(int order, const Point& p)
    {
        if (is_primary_view) {
            khm_menu_show_panel(KHUI_MENU_COLUMNS, p.X, p.Y);
        } else {
            assert(false);
        }
    }

    DEFINE_KMSG(CRED, ROOTDELTA)
    {
        UpdateCredentials();
        UpdateOutline();
        Invalidate();
        return KHM_ERROR_SUCCESS;
    }

    DEFINE_KMSG(CRED, PP_BEGIN)
    {
        khm_pp_begin((khui_property_sheet *) vparam);
        return KHM_ERROR_SUCCESS;
    }

    DEFINE_KMSG(CRED, PP_PRECREATE)
    {
        khm_pp_precreate((khui_property_sheet *) vparam);
        return KHM_ERROR_SUCCESS;
    }

    DEFINE_KMSG(CRED, PP_END)
    {
        khm_pp_end((khui_property_sheet *) vparam);
        return KHM_ERROR_SUCCESS;
    }

    DEFINE_KMSG(CRED, PP_DESTROY)
    {
        khm_pp_destroy((khui_property_sheet *) vparam);
        return KHM_ERROR_SUCCESS;
    }

    DEFINE_KMSG(KCDB, IDENT)
    {
        switch (uparam) {
        case KCDB_OP_MODIFY:
            UpdateOutline();
            Invalidate();
            break;

        case KCDB_OP_NEW_DEFAULT:
            UpdateOutline();
            Invalidate();
            if (is_primary_view) {
                IdentityProvider provider = IdentityProvider::GetDefault();
                Identity identity = provider.GetDefaultIdentity();

                if (static_cast<khm_handle>(identity) == NULL) {
                    khm_notify_icon_tooltip(LoadStringResource(IDS_NOTIFY_READY).c_str());
                } else {
                    khm_notify_icon_tooltip(identity.GetResourceString(KCDB_RES_DISPLAYNAME).c_str());
                }
            }
            break;
        }
        return KHM_ERROR_SUCCESS;
    }

    DEFINE_KMSG(KCDB, ATTRIB)
    {
        if ((uparam == KCDB_OP_INSERT || uparam == KCDB_OP_DELETE) &&
            is_primary_view) {
            RefreshGlobalColumnMenu(hwnd);
        }
        return KHM_ERROR_SUCCESS;
    }

    DEFINE_KMSG(KMM, I_DONE)
    {
        if (skipped_columns) {
            LoadView();
            UpdateCredentials();
            UpdateOutline();
            Invalidate();
        }
        return KHM_ERROR_SUCCESS;
    }

    DEFINE_KMSG(CREDP, BEGIN_NEWCRED)
    {
        khui_nc_subtype subtype = (khui_nc_subtype) uparam;
        khm_handle identity = (khm_handle) vparam;
        std::vector<CwIdentityOutline *> elts;

        FindElements(Identity(identity, false), elts);

        for (std::vector<CwIdentityOutline *>::iterator i = elts.begin();
             i != elts.end(); ++i) {
            (*i)->SetIdentityOp(subtype);
        }
        return KHM_ERROR_SUCCESS;
    }

    DEFINE_KMSG(CREDP, PROG_NEWCRED)
    {
        int progress = (int) uparam;
        khm_handle identity = (khm_handle) vparam;
        std::vector<CwIdentityOutline *> elts;

        FindElements(Identity(identity, false), elts);

        for (std::vector<CwIdentityOutline *>::iterator i = elts.begin();
             i != elts.end(); ++i) {
            (*i)->SetIdentityOpProgress(progress);
        }

        return KHM_ERROR_SUCCESS;
    }

    DEFINE_KMSG(CREDP, END_NEWCRED)
    {
        khm_handle identity = (khm_handle) vparam;
        std::vector<CwIdentityOutline *> elts;

        FindElements(Identity(identity, false), elts);

        for (std::vector<CwIdentityOutline *>::iterator i = elts.begin();
             i != elts.end(); ++i) {
            (*i)->SetIdentityOp((khui_nc_subtype) 0);
        }

        return KHM_ERROR_SUCCESS;
    }

    void CwTable::ToggleColumnByAttributeId(khm_int32 attr_id)
    {
        if (attr_id < 0 || attr_id > KCDB_ATTR_MAX_ID)
            return;

        for (DisplayColumnList::iterator i = columns.begin(); i != columns.end(); ++i) {
            CwColumn * col = dynamic_cast<CwColumn *>(*i);
            assert(col);
            if (col == NULL) continue;

            if (col->attr_id == attr_id) {
                if (i == columns.begin())
                    return; // We don't allow deleting the first column

                // We found the column.  It has to be removed.
                columns.erase(i);
                delete(col);

                columns.ValidateColumns();
                columns.AddColumnsToHeaderControl(hwnd_header);
                columns_dirty = true;
                is_custom_view = true;

                UpdateCredentials();
                UpdateOutline();
                Invalidate();

                if (is_primary_view) {
                    khui_check_action(attrib_to_action[attr_id], FALSE);
                    kmq_post_message(KMSG_ACT, KMSG_ACT_REFRESH, 0, 0);
                    SaveView();
                }

                return;
            }
        }

        // We didn't find the attribute in the current columns list.
        // We should add a new column.

        AttributeInfo info(attr_id);

        if (KHM_FAILED(info.GetLastError()))
            return;

        CwColumn * col = new CwColumn();

        col->attr_id = info->id;
        col->caption = info.GetDescription(KCDB_TS_SHORT);
        col->width = 100;
        col->SetFlags(0);

        columns.push_back(col);
        columns.ValidateColumns();
        columns.AddColumnsToHeaderControl(hwnd_header);

        columns_dirty = true;
        is_custom_view = true;

        UpdateCredentials();
        UpdateOutline();
        Invalidate();

        if (is_primary_view) {
            khui_check_action(attrib_to_action[attr_id], TRUE);
            kmq_post_message(KMSG_ACT, KMSG_ACT_REFRESH, 0, 0);
            SaveView();
        }
    }

    DEFINE_KMSG(ACT, ACTIVATE)
    {
        khm_int32 attr_id;
        khm_int32 action;
        khui_action * paction;

        action = uparam;
        paction = khui_find_action(action);
        if (paction == NULL)
            return KHM_ERROR_SUCCESS;

        attr_id = (khm_int32)(INT_PTR) paction->data;

        ToggleColumnByAttributeId(attr_id);

        return KHM_ERROR_SUCCESS;
    }

    khm_int32 CwTable::OnWmDispatch(khm_int32 msg_type, khm_int32 msg_subtype, khm_ui_4 uparam,
                               void * vparam)
    {
        HANDLE_KMSG(CRED, ROOTDELTA);
        HANDLE_KMSG(CRED, PP_BEGIN);
        HANDLE_KMSG(CRED, PP_PRECREATE);
        HANDLE_KMSG(CRED, PP_END);
        HANDLE_KMSG(CRED, PP_DESTROY);
        HANDLE_KMSG(KCDB, IDENT);
        HANDLE_KMSG(KCDB, ATTRIB);
        HANDLE_KMSG(KMM, I_DONE);
        HANDLE_KMSG(ACT, ACTIVATE);
        HANDLE_KMSG(CREDP, BEGIN_NEWCRED);
        HANDLE_KMSG(CREDP, PROG_NEWCRED);
        HANDLE_KMSG(CREDP, END_NEWCRED);

        return KHM_ERROR_SUCCESS;
    }

    void CwTable::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id) {
        case KHUI_PACTION_ENTER:
        case KHUI_ACTION_PROPERTIES:
            khm_show_properties();
            return;

        case KHUI_ACTION_LAYOUT_RELOAD:
            LoadView();
            UpdateCredentials();
            UpdateOutline();
            Invalidate();
            return;

        case KHUI_ACTION_LAYOUT_ID:
            SaveView();
            cs_view.Close();
            LoadView(L"ByIdentity");
            UpdateCredentials();
            UpdateOutline();
            Invalidate();
            return;

        case KHUI_ACTION_LAYOUT_LOC:
            SaveView();
            cs_view.Close();
            LoadView(L"ByLocation");
            UpdateCredentials();
            UpdateOutline();
            Invalidate();
            return;

        case KHUI_ACTION_LAYOUT_TYPE:
            SaveView();
            cs_view.Close();
            LoadView(L"ByType");
            UpdateCredentials();
            UpdateOutline();
            Invalidate();
            return;

        case KHUI_ACTION_LAYOUT_CUST:
            SaveView();
            cs_view.Close();
            LoadView(L"Custom_0");
            UpdateCredentials();
            UpdateOutline();
            Invalidate();
            return;

        case KHUI_ACTION_LAYOUT_MINI:
            SaveView();
            cs_view.Close();
            LoadView(NULL);
            UpdateCredentials();
            UpdateOutline();
            Invalidate();
            return;

        case KHUI_ACTION_VIEW_ALL_IDS:
            {
                view_all_idents = !view_all_idents;

                UpdateOutline();
                Invalidate();

                if (is_primary_view) {
                    ConfigSpace cs_cw(L"CredWindow", KHM_PERM_READ|KHM_PERM_WRITE);
                    cs_cw.Set(L"ViewAllIdents", view_all_idents);
                    khui_check_action(KHUI_ACTION_VIEW_ALL_IDS, view_all_idents);
                    khui_refresh_actions();
                }
            }
            return;
        }
        __super::OnCommand(id, hwndCtl, codeNotify);
    }

    DWORD CwTable::GetStyle() {
        return WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
            WS_HSCROLL | WS_VISIBLE | WS_VSCROLL;
    }

    DWORD CwTable::GetStyleEx() {
        return 0;
    }

    HFONT CwTable::GetHFONT() {
        return g_theme->hf_normal;
    }

    void CwTable::OnContextMenu(const Point& p)
    {
        Point pm = p;

        if (p.X == -1 && p.Y == -1) {
            if (el_focus)
                pm = el_focus->MapToScreen(Point(0,0));
            else
                pm = MapToScreen(Point(scroll.X, scroll.Y));
        }

        khm_menu_show_panel(KHUI_MENU_IDENT_CTX, pm.X, pm.Y);
    }

    extern "C" void 
    khm_register_credwnd_class(void) {
        ControlWindow::RegisterWindowClass();
    }

    extern "C" void 
    khm_unregister_credwnd_class(void) {
        ControlWindow::UnregisterWindowClass();
    }

    static CwTable * main_table = NULL;

    extern "C" HWND 
    khm_create_credwnd(HWND parent) {
        RECT r;
        HWND hwnd;

        CwTable::CwCreateParams cparams;

        cparams.is_primary_view = true;

        assert(main_table == NULL);

        main_table = new CwTable;

        ZeroMemory(CwTable::attrib_to_action, sizeof(CwTable::attrib_to_action));

        GetClientRect(parent, &r);

        hwnd = main_table->Create(parent, RectFromRECT(&r), 0, &cparams);

        main_table->ShowWindow();

        return hwnd;
    }
}
