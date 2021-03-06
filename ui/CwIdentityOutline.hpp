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

/*! \brief Identity Lifetime Meter

  Lifetime meter and also busy/renew indicator.
*/
class CwIdentityMeterElement :
        public WithFixedSizePos<>, public TimerQueueClient {

    Identity     identity;
    DrawState    state;
    bool         is_expired;

public:
    CwIdentityMeterElement(Identity& _identity) :
        WithFixedSizePos(Point(0,0), g_theme->sz_meter),
        identity(_identity) {

        state = DrawStateNone;
        is_expired = false;
    }

    void OnClick(const Point& pt, UINT keyflags, bool doubleClick) {
        if (is_expired)
            khm_cred_obtain_new_creds_for_ident(identity, NULL);
        else
            khm_cred_renew_identity(identity);
    }

    void OnTimer() {
        Invalidate();
    }

    void SetStatus(DrawState _state) {
        if (state != _state) {
            state = _state;
            Invalidate();
        }
    }

    void PaintSelf(Graphics& g, const Rect& bounds, const Rect& clip) {
        CancelTimer();

        switch (state) {
        case DrawStateCritial:
        case DrawStateWarning:
            {
                DWORD ms = 0;

                g_theme->DrawCredMeterState(g, bounds, state, &ms);

                if (ms != 0)
                    owner->SetTimer(this, ms);
            }
            break;

        case DrawStateBusy:
            {
                DWORD ms = 0;

                g_theme->DrawCredMeterBusy(g, bounds, &ms);

                if (ms != 0)
                    owner->SetTimer(this, ms);
                is_expired = false;
            }
            break;

        default:
            {
                FILETIME ftnow;
                khm_int64 now;
                khm_int64 expire;
                DWORD ms = 0;

                GetSystemTimeAsFileTime(&ftnow);

                now = FtToInt(&ftnow);

                expire = identity.GetAttribFileTimeAsInt(KCDB_ATTR_EXPIRE);

                if (expire == 0 && identity.Exists(KCDB_ATTR_ISSUE)) {
                    g_theme->DrawCredMeterState(g, bounds, DrawStatePostDated, &ms);
                    is_expired = false;

                } else if (expire < now + SECONDS_TO_FT(TT_TIMEEQ_ERROR_SMALL)) {
                    g_theme->DrawCredMeterState(g, bounds, DrawStateExpired, &ms);
                    is_expired = true;

                } else {
                    khm_int64 lifetime;

                    is_expired = false;
                    lifetime = identity.GetAttribFileTimeAsInt(KCDB_ATTR_LIFETIME);

                    if (lifetime == 0) {
                        g_theme->DrawCredMeterState(g, bounds, DrawStateExpired, &ms);
                    } else if (expire - lifetime > now + SECONDS_TO_FT(TT_CLOCKSKEW_THRESHOLD)) {
                        g_theme->DrawCredMeterState(g, bounds, DrawStatePostDated, &ms);
                        ms = FT_TO_MS((expire - lifetime) - (now + SECONDS_TO_FT(TT_CLOCKSKEW_THRESHOLD)));
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
                    }
                }

                    
                if (ms > 0) {
                    owner->SetTimer(this, ms);
                }
            } // default:
        }     // switch
    }         // PaintSelf()
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
        if (use_static_caption)
            __super::SetCaption(_caption);
        else
            GetCaption();       // Automatically sets dynamic caption
    }

    std::wstring GetCaption() {

        if (use_static_caption) {

            // Do nothing

        } else if (identity.Exists(KCDB_ATTR_STATUS)) {

            __super::SetCaption(identity.GetAttribStringObj(KCDB_ATTR_STATUS));

        } else if (identity.GetAttribInt32(KCDB_ATTR_N_IDCREDS) == 0 ||
                   !identity.Exists(KCDB_ATTR_EXPIRE)) {

            __super::SetCaption(LoadStringResource(IDS_CW_NOEXP));

        } else {
            FILETIME ft_exp;
            FILETIME ft_now;

            identity.GetObject(KCDB_ATTR_EXPIRE, ft_exp);
            GetSystemTimeAsFileTime(&ft_now);

            if (CompareFileTime(&ft_now, &ft_exp) >= 0) {
                __super::SetCaption(LoadStringResource(IDS_CW_EXPIRED)); // "(Expired)"
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

                __super::SetCaption(status_msg);
            }
        }

        return __super::GetCaption();
    }

    void PaintSelf(Graphics& g, const Rect& bounds, const Rect& clip) {

        CancelTimer();
        ms_to_next_change = 0;

        __super::PaintSelf(g, bounds, clip);

        if (ms_to_next_change != 0) {
            owner->SetTimer(this, ms_to_next_change);
        }
    }

    void OnTimer() {
        Invalidate();
    }
};

class CwIdentityCredentialTypesListElement :
        public IdentityCredentialListTextT< WithStaticCaption < > >,
        public TimerQueueClient {

    Identity identity;

    enum {
        PixelsPerFrame = 2,
        MillisecondsPerPixel = 100
    };

public:
    CwIdentityCredentialTypesListElement(Identity& _identity) :
        identity(_identity) {}

    void UpdateLayoutPost(Graphics& g, const Rect& layout);

    REAL GetMarqueePosition(REAL width, DWORD * wait_time) {
        DWORD ticks = GetTickCount();
        DWORD dwidth = (DWORD) width;

        *wait_time = ((ticks % MillisecondsPerPixel) == 0)? PixelsPerFrame * MillisecondsPerPixel :
            (ticks % MillisecondsPerPixel) + (PixelsPerFrame - 1) * MillisecondsPerPixel;

        return (REAL)((ticks / MillisecondsPerPixel) % dwidth);
    }

    void PaintSelf(Graphics& g, const Rect& bounds, const Rect& clip) {
        std::wstring caption = GetCaption();

        if (caption.length() == 0)
            return;

        StringFormat sf;
        GetStringFormat(sf);

        RectF rf((REAL) bounds.X, (REAL) bounds.Y, (REAL) bounds.Width, (REAL) bounds.Height);

        RectF bb(0,0,0,0);
        g.MeasureString(caption.c_str(), (int) caption.length(), GetFont(g), PointF(0,0), &bb);

        bool saved = false;
        bool repeat = false;
        DWORD wait = 0;
        GraphicsState gs = 0;

        if (bb.Width > rf.Width) {
            REAL overflow = bb.Width - rf.Width;

            gs = g.Save();
            g.SetClip(rf, CombineModeReplace);
            saved = true;
            rf.Width = bb.Width;
            rf.X -= GetMarqueePosition(rf.Width, &wait);
            repeat = (rf.GetRight() < bounds.GetRight());
        }

        Color fgc = GetForegroundColor();
        SolidBrush br(fgc);
        g.DrawString(caption.c_str(), (int) caption.length(), GetFont(g), rf, &sf, &br);
        if (repeat) {
            rf.X += rf.Width + g_theme->sz_margin.Width;
            g.DrawString(caption.c_str(), (int) caption.length(), GetFont(g), rf, &sf, &br);
        }

        if (saved) {
            g.Restore(gs);
            if (wait > 0)
                owner->SetTimer(this, wait);
        }
    }

    void OnTimer() {
        Invalidate();
    }
};


/*! \brief Default identity control

  This is the control which allows the user to set an identity as
  the default.  It also indicates whether the current identity is
  default.
*/
class CwDefaultIdentityElement : public ButtonElement< &KhmDraw::DrawStarWidget > {
    Identity * pidentity;

public:
    CwDefaultIdentityElement(Identity & _identity) :
        ButtonElement(Point(0,0)), pidentity(&_identity) {};

    bool IsChecked() {
        return !!(pidentity->GetFlags() & KCDB_IDENT_FLAG_DEFAULT);
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


class CwSticktyIdentityElement : public ButtonElement< &KhmDraw::DrawStickyWidget > {
    Identity * pidentity;

public:
    CwSticktyIdentityElement(Identity& _identity) :
        ButtonElement(Point(0,0)), pidentity(&_identity) {};

    bool IsChecked() {
        return !!(pidentity->GetFlags() & KCDB_IDENT_FLAG_STICKY);
    }

    bool OnShowToolTip(std::wstring& caption, Rect& align_rect) {
        if (pidentity->GetFlags() & KCDB_IDENT_FLAG_DEFAULT) {
            caption = LoadStringResource(IDS_CWTT_STICKY_ID1);
        } else {
            caption = LoadStringResource(IDS_CWTT_STICKY_ID0);
        }
        Point pt = MapToScreen(Point(extents.Width, extents.Height));
        align_rect.X = pt.X;
        align_rect.Y = pt.Y;
        align_rect.Width = extents.Width;
        align_rect.Height = extents.Height;

        return true;
    }

    void OnClick(const Point& pt, UINT keyflags, bool doubleClick) {
        pidentity->SetFlags(((IsChecked())? 0 : KCDB_IDENT_FLAG_STICKY),
                            KCDB_IDENT_FLAG_STICKY);
    }
};

typedef ProgressBarElement CwProgressBarElement;

/*! \brief  Identity Outline Node
 */
class CwIdentityOutline : public CwOutline
{
public:
    Identity                  identity;
    IconDisplayElement       *el_icon;
    CwDefaultIdentityElement *el_default_id;
    CwIdentityTitleElement   *el_display_name;
    CwIdentityTypeElement    *el_type_name;
    CwIdentityStatusElement  *el_status;
    CwIdentityMeterElement   *el_meter;
    CwProgressBarElement     *el_progress;
    CwSticktyIdentityElement *el_sticky;
    CwIdentityCredentialTypesListElement *el_creds;

    bool                     monitor_progress;

public:
    CwIdentityOutline(const Identity& _identity, int _column)
        : identity(_identity), CwOutline(_column) {
        monitor_progress    = false;

        InsertChildAfter(el_icon =
                         PNEW IconDisplayElement(Point(0,0),
                                                 identity.GetResourceIcon(KCDB_RES_ICON_NORMAL),
                                                 true));

        InsertChildAfter(el_default_id =
                         PNEW CwDefaultIdentityElement(identity));

        InsertChildAfter(el_display_name =
                         PNEW CwIdentityTitleElement(identity));

        InsertChildAfter(el_type_name =
                         PNEW CwIdentityTypeElement(identity));

        InsertChildAfter(el_meter =
                         PNEW CwIdentityMeterElement(identity));

        InsertChildAfter(el_sticky =
                         PNEW CwSticktyIdentityElement(identity));

        InsertChildAfter(el_status =
                         PNEW CwIdentityStatusElement(identity));

        InsertChildAfter(el_progress =
                         PNEW CwProgressBarElement());

        InsertChildAfter(el_creds =
                         PNEW CwIdentityCredentialTypesListElement(identity));
    }

    ~CwIdentityOutline() {
        el_icon             = NULL;
        el_default_id       = NULL;
        el_display_name     = NULL;
        el_type_name        = NULL;
        el_status           = NULL;
        el_meter            = NULL;
        el_progress         = NULL;
        el_creds            = NULL;
    }

    virtual bool Represents(const Credential& credential) {
        return credential.GetIdentity() == identity;
    }

    virtual bool Represents(const Identity& _identity) {
        return identity == _identity;
    }

    virtual void UpdateLayoutPre(Graphics& g, Rect& layout) {
        CwIdentityOutline * p = dynamic_cast<CwIdentityOutline *>(TQPARENT(this));
        if (p && p->col_idx == col_idx) {
            indent = p->indent + g_theme->sz_margin.Width * 4 +
                g_theme->sz_icon.Width + g_theme->sz_icon_sm.Width;
        }

        LayoutOutlineSetup(g, layout);
    }

    virtual void UpdateLayoutPost(Graphics& g, const Rect& layout) {
        bool has_creds = (identity.GetAttribInt32(KCDB_ATTR_N_IDCREDS) > 0);
        bool is_basic = (owner->columns.size() == 1);

        FlowLayout l(layout, g_theme->sz_margin);

        FlowLayout ll = l.InsertFixedColumn(g_theme->sz_icon.Width);

        ll
            .Add(el_icon, FlowLayout::Center)
            .LineBreak()
            .Add(el_meter, FlowLayout::Center, FlowLayout::Fixed)
            // Formerly, el_meter was conditioned on has_creds
            ;

        FlowLayout lr = l.InsertFillerColumn();

        lr
            .Add(el_default_id, FlowLayout::Left)
            .Add(el_display_name, FlowLayout::Left, FlowLayout::Squish)
            .Add(el_type_name, FlowLayout::Right, FlowLayout::Squish)
            .LineBreak()
            .Add(el_sticky, FlowLayout::Left, FlowLayout::Fixed)
            .Add(el_status, FlowLayout::Left, FlowLayout::Squish, has_creds)
            .Add(el_progress, FlowLayout::Right, FlowLayout::Squish, monitor_progress)
            .LineBreak()
            .Add(el_creds, FlowLayout::Left, FlowLayout::Squish, is_basic)
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

    void SetIdentityOpProgress(khui_nc_subtype subtype, int progress) {
        if (!monitor_progress) {
            SetIdentityOp(subtype);
        }
        el_progress->SetProgress(progress);
    }

    void NotifyIdentityResUpdate() {
        el_icon->SetIcon(identity.GetResourceIcon(KCDB_RES_ICON_NORMAL));
    }
};


}
