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
#include<prsht.h>
#include<assert.h>

namespace nim
{

#define CW_CANAME_FLAGS L"_CWFlags"

    /* The expiration states */
#define CW_EXPSTATE_NONE        0x00000000
#define CW_EXPSTATE_WARN        0x00000400
#define CW_EXPSTATE_CRITICAL    0x00000800
#define CW_EXPSTATE_EXPIRED     0x00000c00

#define CW_EXPSTATE_MASK        0x00000c00

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


    template<void (KhmDraw::*DF)(Graphics&, const Rect&, DrawState)>
    class CwButtonT : public WithFixedSizePos< DisplayElement > {
    public:
        CwButtonT(const Point& _p) {
            SetPosition(_p);
            SetSize(g_theme->sz_icon_sm);
        }

        virtual bool IsChecked() { return false; }

        virtual void PaintSelf(Graphics &g, const Rect& bounds) {
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

    class CwOutlineWidget : public CwButtonT< &KhmDraw::DrawCredWindowOutlineWidget > {
    public:
        CwOutlineWidget(const Point& _p) : CwButtonT(_p) {}

        void OnClick(const Point& p, UINT keyflags, bool doubleClick) {
            TQPARENT(this)->Expand(!TQPARENT(this)->expanded);
        }

        bool IsChecked() {
            return TQPARENT(this)->expanded;
        }
    };


    class CwIconWidget : public WithFixedSizePos< DisplayElement > {
        Bitmap i;
        bool large;
    public:
        CwIconWidget(const Point& _p, HICON _icon, bool _large) : i(_icon), large(_large) {
            SetPosition(_p);
            SetSize((large)? g_theme->sz_icon : g_theme->sz_icon_sm);
        }

        virtual void PaintSelf(Graphics &g, const Rect& bounds) {
            g.DrawImage(&i, bounds);
        }
    };

    class CwDefaultIdentityWidget : public CwButton< &KhmDraw::DrawStarWidget > {
        Identity * pidentity;

    public:
        CwDefaultIdentityWidget(Identity * _pidentity, const Point& _p) :
            CwButtonT(_p), pidentity(_pidentity) {};

        bool IsChecked() {
            return (pidentity->GetFlags() & KCDB_IDENT_FLAG_DEFAULT);
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


    class CwOutlineBase : virtual public DisplayElement, public TimerQueueClient {
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
    };

    class CwOutline :
        public WithOutline< WithTabStop< WithColumnAlignment< CwOutlineBase > > > {
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

        virtual bool Represents(Credential& credential) { return false; }
        virtual bool Represents(Identity& identity) { return false; }

        virtual void UpdateLayoutPre(Graphics& g, Rect& layout);
        virtual void UpdateLayoutPost(Graphics& g, const Rect& layout);

        virtual void PaintSelf(Graphics& g, const Rect& bounds);

        virtual DrawState GetDrawState() {
            return (DrawState)(((expanded) ? DrawStateChecked : DrawStateNone) |
                               ((highlight) ? DrawStateHotTrack : DrawStateNone) |
                               ((selected) ? DrawStateSelected : DrawStateNone) |
                               ((focus) ? DrawStateFocusRect : DrawStateNone));
        }
    };


    class CwIdentityOutline : public CwOutline
    {
    public:
        Identity identity;
        DisplayElement * icon_widget;
        DisplayElement * default_widget;

    public:
        CwIdentityOutline(Identity& _identity, int _column)
            : identity(_identity), CwOutline(_column) {
            icon_widget = NULL;
            default_widget = NULL;
        }

        ~CwIdentityOutline() {
            icon_widget = NULL; // Will be deleted automatically
            default_widget = NULL;
        }

        virtual bool Represents(Credential& credential) {
            return credential.GetIdentity() == identity;
        }

        virtual bool Represents(Identity& _identity) {
            return identity == _identity;
        }

        virtual void UpdateLayoutPre(Graphics& g, Rect& layout) {
            __super::UpdateLayoutPre(g, layout);

            if (icon_widget == NULL) {
                InsertChildAfter(icon_widget =
                                 new CwIconWidget(Point(0,0),
                                                  identity.GetIcon(KCDB_RES_ICON_NORMAL),
                                                  true));

                InsertChildAfter(default_widget =
                                 new CwDefaultIdentityWidget(&identity, Point(0,16)));
            }

            layout.Y = g_theme->sz_icon.Height + g_theme->sz_margin.Height * 2;
        }

        virtual void UpdateLayoutPost(Graphics& g, const Rect& layout) {
            icon_widget->origin =
                g_theme->pt_margin_cx +
                g_theme->pt_margin_cy +
                ((expandable)? g_theme->pt_icon_sm_cx + g_theme->pt_margin_cx : Point(0,0));

            default_widget->origin =
                icon_widget->origin + g_theme->pt_icon_cx + g_theme->pt_margin_cx;

            __super::UpdateLayoutPost(g, layout);
        }

        virtual void PaintSelf(Graphics& g, const Rect& bounds) {
            __super::PaintSelf(g, bounds);

            KhmTextLayout t(g, bounds, g_theme);
            DrawState s = GetDrawState();

            t.SetLeftMargin(g_theme->sz_margin.Width +
                            ((expandable)? g_theme->sz_icon_sm.Width + g_theme->sz_margin.Width : 0) +
                            g_theme->sz_icon.Width);
            t.DrawText(identity.GetString(KCDB_RES_DISPLAYNAME), DrawTextCredWndIdentity, s);
            t.DrawText(identity.GetType().GetString(KCDB_RES_DISPLAYNAME), DrawTextCredWndType, s);
        }

        virtual DrawState GetDrawState() {
            DrawState ds = DrawStateNone;

            do {
                if (identity.GetAttribInt32(KCDB_ATTR_N_IDCREDS) == 0) {

                    ds = DrawStateDisabled;

                } else if (identity.Exists(KCDB_ATTR_EXPIRE)) {
                    khm_int64 expire = identity.GetAttribFileTimeAsInt(KCDB_ATTR_EXPIRE);
                    khm_int64 now;

                    FILETIME ft;

                    GetSystemTimeAsFileTime(&ft);
                    now = FtToInt(&ft) + SECONDS_TO_FT(TT_TIMEEQ_ERROR_SMALL);

                    if (now > expire) {
                        ds = DrawStateExpired;
                        break;
                    }

                    khm_int64 thr_crit = identity.GetAttribFileTimeAsInt(KCDB_ATTR_THR_CRIT);

                    if (thr_crit != 0 && now > expire - thr_crit) {
                        ds = DrawStateCritial;
                        break;
                    }

                    khm_int64 thr_warn = identity.GetAttribFileTimeAsInt(KCDB_ATTR_THR_WARN);

                    if (thr_warn != 0 && now > expire - thr_warn) {
                        ds = DrawStateWarning;
                        break;
                    }
                }
            } while (false);

            return (DrawState)(ds | __super::GetDrawState());
        }
    };


    class CwCredTypeOutline : public CwOutline
    {
    public:
        CredentialType credtype;

    public:
        CwCredTypeOutline(CredentialType& _credtype, int _column) :
            credtype(_credtype),
            CwOutline(_column) {
        }

        virtual bool Represents(Credential& credential) {
            return credtype == credential.GetType();
        }

        virtual void UpdateLayoutPre(Graphics& g, Rect& layout);

        virtual void UpdateLayoutPost(Graphics& g, const Rect& layout);

        virtual void PaintSelf(Graphics& g, const Rect& bounds);
    };


    class CwGenericOutline : public CwOutline
    {
    public:
        Credential credential;
        khm_int32  attr_id;

    public:
        CwGenericOutline(Credential& _credential, khm_int32 _attr_id, int _column) :
            credential(_credential),
            attr_id(_attr_id),
            CwOutline(_column) {
        }

        ~CwGenericOutline() {}

        virtual bool Represents(Credential& _credential) {
            return credential.CompareAttrib(_credential, attr_id) == 0;
        }

        virtual void UpdateLayoutPre(Graphics& g, Rect& layout);

        virtual void UpdateLayoutPost(Graphics& g, const Rect& layout);

        virtual void PaintSelf(Graphics& g, const Rect& bounds);
    };


    class CwCredentialRow : public CwOutline
    {
    public:
        Credential  credential;

    public:
        CwCredentialRow(Credential& _credential, int _column) :
            credential(_credential),
            CwOutline(_column) {
        }

        ~CwCredentialRow() {}

        virtual bool Represents(Credential& _credential) {
            return credential == _credential;
        }

        virtual void UpdateLayoutPre(Graphics& g, Rect& layout);

        virtual void UpdateLayoutPost(Graphics& g, const Rect& layout);

        virtual void PaintSelf(Graphics& g, const Rect& bounds);
    };

#define DCL_KMSG(T,ST) \
    khm_int32 OnKmsg ## T ## _ ## ST (khm_ui_4 uparam, void * vparam)
#define DEFINE_KMSG(T,ST) \
    khm_int32 CwTable::OnKmsg ## T ## _ ## ST (khm_ui_4 uparam, void * vparam)

#define HANDLE_KMSG(T,ST) \
    if (msg_type == KMSG_ ## T && msg_subtype == KMSG_ ## T ## _ ## ST) \
        return OnKmsg ## T ## _ ## ST (uparam, vparam)

#pragma warning(push)
    // C4250 is 'class A' : inherits 'A::method()' via dominance

    // It gets triggered a lot because of virtual overrides in
    // DisplayContainer and CwOutlineBase.  This is expected.
#pragma warning(disable: 4250)

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
        bool   has_no_header   : 1; // No header control?
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

    public:                     // Overrides
        virtual BOOL OnCreate(LPVOID createParams);
        virtual void OnDestroy();
        virtual void OnColumnPosChanged(int from, int to);
        virtual void OnColumnSizeChanged(int idx);
        virtual void OnColumnSortChanged(int idx);
        virtual void OnColumnContextMenu(int idx);
        virtual khm_int32 OnWmDispatch(khm_int32 msg_type, khm_int32 msg_subtype,
                                       khm_ui_4 uparam, void * vparam);
        virtual void OnCommand(int id, HWND hwndCtl, UINT codeNotify);
        virtual void OnContextMenu(const Point& p);

        virtual DWORD GetStyle();
        virtual DWORD GetStyleEx();

        virtual void PaintSelf(Graphics& g, const Rect& bounds);

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

    public:
        static khm_int32 attrib_to_action[KCDB_ATTR_MAX_ID + 1];
        static void RefreshGlobalColumnMenu();
    };

#pragma warning(pop)


    // Implementation section

    khm_int32 CwTable::attrib_to_action[KCDB_ATTR_MAX_ID + 1] = {0};

    inline CwOutline * NextOutline(DisplayElement * e) {
        for (DisplayElement * i = e; i != NULL; i = TQNEXTSIBLING(i)) {
            CwOutline * t = dynamic_cast< CwOutline * >(i);
            if (t)
                return t;
        }
        return static_cast< CwOutline * >(NULL);
    }

    template <class T>
    bool AllowRecursiveInsert(DisplayColumnList& columns, int column)
    {
        return false;
    }

    template <>
    bool AllowRecursiveInsert<Credential&>(DisplayColumnList& columns, int column)
    {
        return (column > 0 && (unsigned) column < columns.size() && columns[column]->group);
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
            InsertChildBefore(target, insertion_point);
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

    void CwOutline::UpdateLayoutPre(Graphics& g, Rect& layout)
    {
        __super::UpdateLayoutPre(g, layout);

        if (expandable && outline_widget == NULL) {
            outline_widget = new CwOutlineWidget(g_theme->pt_margin_cx + g_theme->pt_margin_cy);
            InsertChildAfter(outline_widget);
        }

        if (expandable)
            outline_widget->visible = true;
        else if (outline_widget != NULL)
            outline_widget->visible = false;

        extents.Width = layout.Width;
        layout.Y = 0;
    }

    void CwOutline::UpdateLayoutPost(Graphics& g, const Rect& layout)
    {
        Point p;

        layout.GetLocation(&p);
        extents.Height = p.Y;

        for_each_child(c) {
            c->origin = p;
            p.Y += c->extents.Height;
            extents.Height = p.Y;
        }
    }

    void CwOutline::PaintSelf(Graphics& g, const Rect& bounds)
    {
        g_theme->DrawCredWindowOutline(g, bounds, GetDrawState());
    }

    void CwCredTypeOutline::UpdateLayoutPre(Graphics& g, Rect& layout)
    {
        __super::UpdateLayoutPre(g, layout);
    }

    void CwCredTypeOutline::UpdateLayoutPost(Graphics& g, const Rect& layout)
    {
        __super::UpdateLayoutPost(g, layout);
    }

    void CwCredTypeOutline::PaintSelf(Graphics& g, const Rect& bounds)
    {
        __super::PaintSelf(g, bounds);
    }

    void CwGenericOutline::UpdateLayoutPre(Graphics& g, Rect& layout)
    {
        __super::UpdateLayoutPre(g, layout);
    }

    void CwGenericOutline::UpdateLayoutPost(Graphics& g, const Rect& layout)
    {
        __super::UpdateLayoutPost(g, layout);
    }

    void CwGenericOutline::PaintSelf(Graphics& g, const Rect& bounds)
    {
        __super::PaintSelf(g, bounds);
    }

    void CwCredentialRow::UpdateLayoutPre(Graphics& g, Rect& layout)
    {
        __super::UpdateLayoutPre(g, layout);
    }

    void CwCredentialRow::UpdateLayoutPost(Graphics& g, const Rect& layout)
    {
        __super::UpdateLayoutPost(g, layout);
    }

    void CwCredentialRow::PaintSelf(Graphics& g, const Rect& bounds)
    {
        __super::PaintSelf(g, bounds);
    }

    CwOutline * CreateOutlineNode(Identity & identity, DisplayColumnList& columns, int column)
    {
        assert((unsigned) column >= columns.size() ||
               (static_cast<CwColumn *>(columns[column]))->attr_id == KCDB_ATTR_ID_DISPLAY_NAME);
        return new CwIdentityOutline(identity, column);
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

    void CwTable::PaintSelf(Graphics& g, const Rect& bounds)
    {
        Rect clip;
        Region rgn;

        if (g.GetClip(&rgn) == Ok) {
            rgn.GetBounds(&clip, &g);
        } else {
            bounds.GetBounds(&clip);
        }

        g_theme->DrawCredWindowBackground(g, bounds, clip);
    }

    void
    CwTable::RefreshGlobalColumnMenu()
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

                    //kmq_create_hwnd_subscription(hwnd, &sub);

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
        cs_view.Set(L"NoHeader", static_cast<khm_int32>(!!has_no_header));

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

        if (cview == NULL && KHM_SUCCEEDED(cs_view.GetLastError()))
            goto have_view;

        if (cview == NULL) {
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

        is_custom_view  = false;
        has_no_header   = !!cs_view.GetInt32(L"NoHeader");
        view_all_idents = !!cs_cw.GetInt32(L"ViewAllIdents");
        is_identity_view= !!cs_view.GetInt32(L"ExpandedIdentity");
        skipped_columns = false;

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

        if (!is_identity_view)
            view_all_idents = false;

        columns_dirty = TRUE;

        if (is_primary_view)
            khui_refresh_actions();

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
        std::wstring dn1(i1.GetString(KCDB_RES_DISPLAYNAME));
        std::wstring dn2(i2.GetString(KCDB_RES_DISPLAYNAME));

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
            InsertCredentialProcData d;
            d.table = this;
            d.target = this;
            d.column = 0;
            d.filter_by = NULL;

            credset.Apply(InsertCredentialProc, &d);
        }

        EndUpdate();

        // TODO: We should also update the notification icon here.
    }

    BOOL CwTable::OnCreate(LPVOID createParams)
    {
        // TODO: Check lpc and set initial create options such as
        // is_primary_view and is_identity_view.

        if (is_primary_view)
            RefreshGlobalColumnMenu();

        LoadView(NULL);

        UpdateCredentials();
        UpdateOutline();

        // TODO: Select the cursor row
        // TODD: Update selection state

        kmq_subscribe_hwnd(KMSG_CRED, hwnd);
        kmq_subscribe_hwnd(KMSG_KCDB, hwnd);
        kmq_subscribe_hwnd(KMSG_KMM, hwnd);

        return __super::OnCreate(createParams);
    }

    void CwTable::OnDestroy()
    {
        kmq_unsubscribe_hwnd(KMSG_CRED, hwnd);
        kmq_unsubscribe_hwnd(KMSG_KCDB, hwnd);
        kmq_unsubscribe_hwnd(KMSG_KMM, hwnd);
    }

    void CwTable::OnColumnPosChanged(int from, int to)
    {
        is_custom_view = true;
        columns_dirty = true;
    }

    void CwTable::OnColumnSizeChanged(int order)
    {
        is_custom_view = true;
    }

    void CwTable::OnColumnSortChanged(int order)
    {
        is_custom_view = true;
        columns_dirty = true;
    }

    void CwTable::OnColumnContextMenu(int order)
    {
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
        return KHM_ERROR_SUCCESS;
    }

    DEFINE_KMSG(CRED, PP_PRECREATE)
    {
        return KHM_ERROR_SUCCESS;
    }

    DEFINE_KMSG(CRED, PP_END)
    {
        return KHM_ERROR_SUCCESS;
    }

    DEFINE_KMSG(CRED, PP_DESTROY)
    {
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
            if (is_primary_view) {
                IdentityProvider provider = IdentityProvider::GetDefault();
                Identity identity = provider.GetDefaultIdentity();

                if (static_cast<khm_handle>(identity) == NULL) {
                    khm_notify_icon_tooltip(LoadStringResource(IDS_NOTIFY_READY).c_str());
                } else {
                    khm_notify_icon_tooltip(identity.GetString(KCDB_RES_DISPLAYNAME).c_str());
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
            RefreshGlobalColumnMenu();
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
                columns_dirty = true;
                is_custom_view = true;

                UpdateCredentials();
                UpdateOutline();
                Invalidate();

                if (is_primary_view) {
                    khui_check_action(attrib_to_action[attr_id], FALSE);
                    kmq_post_message(KMSG_ACT, KMSG_ACT_REFRESH, 0, 0);
                }

                return;
            }
        }

        // We didn't find the attribute in the current columns list.
        // We should add a new column.

        CwColumn * col = new CwColumn();
        AttributeInfo info(attr_id);

        if (KHM_FAILED(info.GetLastError()))
            return;

        col->attr_id = info->id;
        col->caption = info.GetDescription(KCDB_TS_SHORT);
        col->width = -1;
        col->SetFlags(0);

        columns.push_back(col);

        columns_dirty = true;
        is_custom_view = true;

        UpdateCredentials();
        UpdateOutline();
        Invalidate();

        if (is_primary_view) {
            khui_check_action(attrib_to_action[attr_id], TRUE);
            kmq_post_message(KMSG_ACT, KMSG_ACT_REFRESH, 0, 0);
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
            LoadView(L"ByIdentity");
            UpdateCredentials();
            UpdateOutline();
            Invalidate();
            return;

        case KHUI_ACTION_LAYOUT_LOC:
            SaveView();
            LoadView(L"ByLocation");
            UpdateCredentials();
            UpdateOutline();
            Invalidate();
            return;

        case KHUI_ACTION_LAYOUT_TYPE:
            SaveView();
            LoadView(L"ByType");
            UpdateCredentials();
            UpdateOutline();
            Invalidate();
            return;

        case KHUI_ACTION_LAYOUT_CUST:
            SaveView();
            LoadView(L"Custom_0");
            UpdateCredentials();
            UpdateOutline();
            Invalidate();
            return;

        case KHUI_ACTION_LAYOUT_MINI:
            SaveView();
            LoadView(NULL);
            UpdateCredentials();
            UpdateOutline();
            Invalidate();
            return;

        case KHUI_ACTION_VIEW_ALL_IDS:
            {
                ConfigSpace cs_cw(L"CredWindow", KHM_PERM_READ|KHM_PERM_WRITE);

                view_all_idents = !view_all_idents;

                UpdateOutline();
                Invalidate();

                cs_cw.Set(L"ViewAllIdents", view_all_idents);
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

    void CwTable::OnContextMenu(const Point& p)
    {
        // TODO: show context menu.  note that x,y = (-1,-1) if the
        // context menu was invoked via keyboard.
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

        assert(main_table == NULL);

        main_table = new CwTable;

        ZeroMemory(CwTable::attrib_to_action, sizeof(CwTable::attrib_to_action));

        GetClientRect(parent, &r);

        hwnd = main_table->Create(parent, RectFromRECT(&r), 0, NULL);

        main_table->ShowWindow();

        return hwnd;
    }
}
