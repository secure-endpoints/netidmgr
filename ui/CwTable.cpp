#include "khmapp.h"
#include<assert.h>

#include "credwnd_base.hpp"
#include "Notifier.hpp"

namespace nim
{
    khm_int32 CwTable::attrib_to_action[KCDB_ATTR_MAX_ID + 1] = {0};

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

        ticks_UpdateCredentials = GetTickCount();

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
        CwTable        *table;
        CwOutlineBase  *target;
        Identity       *filter_by;
        int             column;
        int             n_added;
    };

    static khm_int32 KHMCALLBACK InsertCredentialProc(Credential& credential, void * p)
    {
        InsertCredentialProcData *d = static_cast<InsertCredentialProcData *>(p);

        if (d->filter_by == NULL || credential.GetIdentity() == *d->filter_by) {
            d->target->Insert(credential, d->table->columns, d->column);
            d->n_added++;
        }
        return KHM_ERROR_SUCCESS;
    }

    static khm_int32 KHMAPI IdentityNameComparator(const Identity& i1, const Identity& i2,
                                                   void * param)
    {
        std::wstring dn1 = i1.GetResourceString(KCDB_RES_DISPLAYNAME);
        std::wstring dn2 = i2.GetResourceString(KCDB_RES_DISPLAYNAME);

        return CompareString(LOCALE_USER_DEFAULT,
                             NORM_IGNORECASE | NORM_IGNORENONSPACE |
                             NORM_IGNORESYMBOLS | NORM_IGNOREWIDTH |
                             SORT_STRINGSORT,
                             dn1.c_str(), (int) dn1.size(),
                             dn2.c_str(), (int) dn2.size());
    }

    struct FilterByParentData {
        Identity * parent;
    };

    khm_boolean KHMCALLBACK FilterByParent(const Identity& identity, void * param)
    {
        FilterByParentData * d = reinterpret_cast<FilterByParentData *>(param);
        return
            (d->parent == NULL && static_cast<khm_handle>(identity.GetParent()) == NULL) ||
            (d->parent != NULL && identity.GetParent() == *(d->parent));
    }

    khm_boolean KHMCALLBACK FilterByAlwaysVisible(const Identity& identity, void * param)
    {
        return (identity.GetFlags() & (KCDB_IDENT_FLAG_STICKY |
                                       KCDB_IDENT_FLAG_DEFAULT)) ||
            identity.GetAttribInt32(KCDB_ATTR_N_CREDS) > 0;
    }

    int
    CwTable::InsertDerivedIdentities(CwOutlineBase * o, Identity * id)
    {
        int n_added = 0;

        Identity::Enumeration e = Identity::Enum(0,0);
        FilterByParentData d;
        d.parent = id;
        e.Filter(FilterByParent, &d);
        e.Sort(IdentityNameComparator);

        for (; !e.AtEnd(); ++e) {
            int n_child_ids = 0;
            CwOutlineBase * co = o->Insert(*e, columns, 0);

            InsertCredentialProcData d;
            d.table = this;
            d.target = co;
            d.column = 1;
            d.filter_by = &(*e);
            d.n_added = 0;
            credset.Apply(InsertCredentialProc, &d);

            n_child_ids = InsertDerivedIdentities(co, &(*e));
            if (!view_all_idents && n_child_ids + d.n_added == 0 &&
                !(e->GetFlags() & (KCDB_IDENT_FLAG_STICKY |
                                   KCDB_IDENT_FLAG_DEFAULT))) {
                o->DeleteChild(co);
            } else {
                n_added ++;
            }
        }

        return n_added;
    }

    void
    CwTable::UpdateOutline()
    {
        /*  this is called after calling UpdateCredentials(), so we
            assume that the credentials are all loaded and sorted
            according to grouping rules  */

        ticks_UpdateOutline = GetTickCount();

#ifdef DEBUG
        OutputDebugString(L"In UpdateOutline()\n");
#endif
        DCL_TIMER(uo);
        START_TIMER(uo);

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

                Identity::Enumeration e = Identity::Enum(0,0);
                if (!view_all_idents)
                    e.Filter(FilterByAlwaysVisible, NULL);
                e.Sort(IdentityNameComparator);

                for (; !e.AtEnd(); ++e) {
                    this->Insert(*e, columns, 0);
                }
            }

            InsertCredentialProcData d;
            d.table = this;
            d.target = this;
            d.column = 0;
            d.filter_by = NULL;
            d.n_added = 0;

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

		g_notifier->m_icon->SetExpiryState(expstate);
            } else {
		g_notifier->m_icon->SetExpiryState(KHM_NOTIF_EMPTY);
            }
        }

        END_TIMER(uo, L"UpdateOutline done");
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
        {
            kmq_message * m;
            kmq_get_current_message(&m);
            if (m && m->timeSent < ticks_UpdateCredentials) {
#ifdef DEBUG
                OutputDebugString(L"Skipping CRED_ROOTDELTA\n");
#endif
                return KHM_ERROR_SUCCESS;
            }
        }
#ifdef DEBUG
        OutputDebugString(L"CRED_ROOTDELTA\n");
#endif
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
        case KCDB_OP_RESUPDATE:
            {
                khm_handle identity = (khm_handle) vparam;
                std::vector<CwIdentityOutline *> elts;

                FindElements(Identity(identity, false), elts);
                for (std::vector<CwIdentityOutline *>::iterator i = elts.begin();
                     i != elts.end(); ++i) {
                    (*i)->NotifyIdentityResUpdate();
                }
            }
            break;

        case KCDB_OP_MODIFY:
            {
                khm_handle identity = (khm_handle) vparam;
                std::vector<CwIdentityOutline *> elts;

                FindElements(Identity(identity, false), elts);
                for (std::vector<CwIdentityOutline *>::iterator i = elts.begin();
                     i != elts.end(); ++i) {
                    (*i)->MarkForExtentUpdate();
                }
                Invalidate();
            }
            break;

        case KCDB_OP_INSERT:
            {
                kmq_message * m;
                kmq_get_current_message(&m);
                if (m && m->timeSent < ticks_UpdateOutline) {
#ifdef DEBUG
                    OutputDebugString(L"Skipping KCDB_IDENT <KCDB_OP_INSERT>\n");
#endif
                    break;
                }
            }
#ifdef DEBUG
            if (uparam == KCDB_OP_INSERT)
                OutputDebugString(L"KCDB_IDENT <KCDB_OP_INSERT>\n");
            else
                OutputDebugString(L"KCDB_IDENT <KCDB_OP_MODIFY>\n");
#endif
            UpdateOutline();
            Invalidate();
            break;

        case KCDB_OP_NEW_DEFAULT:
#ifdef DEBUG
            OutputDebugString(L"KCDB_IDENT <KCDB_OP_NEW_DEFAULT>\n");
#endif
            UpdateOutline();
            Invalidate();
            if (is_primary_view) {
                IdentityProvider provider = IdentityProvider::GetDefault();
                Identity identity = provider.GetDefaultIdentity();

                if (static_cast<khm_handle>(identity) == NULL) {
		    g_notifier->m_icon->SetTooltip(LoadStringResource(IDS_NOTIFY_READY).c_str());
                } else {
		    g_notifier->m_icon->SetTooltip(identity.GetResourceString(KCDB_RES_DISPLAYNAME).c_str());
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
            UpdateWindow(hwnd);
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

}
