/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
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

#define _NIMLIB_
#define NIMPRIVATE

#include<khuidefs.h>
#include<intnewcred.h>
#include<utils.h>
#include<assert.h>
#include<strsafe.h>

#define CW_ALLOC_INCR 8

#define IS_NC(c) ((c) && (c)->magic == KHUI_NC_MAGIC)
#define ASSERT_NC(c) assert(IS_NC(c))
#define IS_PP(p) ((p) && (p)->magic == KHUI_NEW_CREDS_PRIVINT_PANEL_MAGIC)
#define ASSERT_PP(p) assert(IS_PP(p))

static void
cw_free_prompts(khui_new_creds_privint_panel * pp);

static void
cw_free_prompt(khui_new_creds_prompt * p);

static khui_new_creds_prompt * 
cw_create_prompt(khm_size idx, khm_int32 type, wchar_t * prompt,
                 wchar_t * def, khm_int32 flags);

KHMEXP khm_int32 KHMAPI 
khui_cw_create_cred_blob(khui_new_creds ** ppnc)
{
    khui_new_creds * c;

    c = PMALLOC(sizeof(*c));
    ZeroMemory(c, sizeof(*c));

    c->magic = KHUI_NC_MAGIC;
    InitializeCriticalSection(&c->cs);
    c->result = KHUI_NC_RESULT_CANCEL;
    c->mode = KHUI_NC_MODE_MINI;

    khui_context_create(&c->ctx, KHUI_SCOPE_NONE, NULL, KCDB_CREDTYPE_INVALID, NULL);

    *ppnc = c;

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI 
khui_cw_destroy_cred_blob(khui_new_creds *c)
{
    khm_size i;

    ASSERT_NC(c);

    khui_cw_clear_all_privints(c);

    EnterCriticalSection(&c->cs);

    assert(c->n_types == 0);

    for (i=0; i < c->n_identities; i++) {
        kcdb_identity_release(c->identities[i]);
    }

    for (i=0; i < c->n_selectors; i++) {
        if (c->selectors[i].factory_cb != NULL)
            c->selectors[i].factory_cb(NULL, &c->selectors[i]);
    }

    khui_context_release(&c->ctx);
    LeaveCriticalSection(&c->cs);
    DeleteCriticalSection(&c->cs);

    if (c->identities)
        PFREE(c->identities);

    if (c->selectors)
        PFREE(c->selectors);

    if (c->types)
        PFREE(c->types);

    if (c->window_title)
        PFREE(c->window_title);

    if (c->cs_privcred)
        kcdb_credset_delete(c->cs_privcred);

    if (c->persist_identity)
        kcdb_identity_release(c->persist_identity);

    ZeroMemory(c, sizeof(*c));
    PFREE(c);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI 
khui_cw_lock_nc(khui_new_creds * c)
{
    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);
    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI 
khui_cw_unlock_nc(khui_new_creds * c)
{
    LeaveCriticalSection(&c->cs);
    return KHM_ERROR_SUCCESS;
}

#define NC_N_IDENTITIES 4

#pragma warning(push)
#pragma warning(disable: 4995)

KHMEXP khm_int32 KHMAPI 
khui_cw_add_identity(khui_new_creds * c, 
                     khm_handle id)
{
    ASSERT_NC(c);

    if(id == NULL)
        return KHM_ERROR_SUCCESS; /* we return success because adding
                                  a NULL id is equivalent to adding
                                  nothing. */
    EnterCriticalSection(&(c->cs));

    if(c->identities == NULL) {
        c->nc_identities = NC_N_IDENTITIES;
        c->identities = PMALLOC(sizeof(*(c->identities)) * 
                               c->nc_identities);
        c->n_identities = 0;
    } else if(c->n_identities + 1 > c->nc_identities) {
        khm_handle * ni;

        c->nc_identities = UBOUNDSS(c->n_identities + 1, 
                                    NC_N_IDENTITIES, 
                                    NC_N_IDENTITIES);
        ni = PMALLOC(sizeof(*(c->identities)) * c->nc_identities);
        memcpy(ni, c->identities, 
               sizeof(*(c->identities)) * c->n_identities);
        PFREE(c->identities);
        c->identities = ni;
    }

    kcdb_identity_hold(id);
    c->identities[c->n_identities++] = id;
    LeaveCriticalSection(&(c->cs));

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI 
khui_cw_set_primary_id(khui_new_creds * c, 
                       khm_handle id)
{
    khm_size  i;
    khm_int32 rv;

    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);

    /* no change */
    if((c->n_identities > 0 && c->identities[0] == id) ||
       (c->n_identities == 0 && id == NULL)) {
        LeaveCriticalSection(&c->cs);
        return KHM_ERROR_DUPLICATE;
    }

    for(i=0; i<c->n_identities; i++) {
        kcdb_identity_release(c->identities[i]);
    }
    c->n_identities = 0;

    rv = khui_cw_add_identity(c,id);
    LeaveCriticalSection(&(c->cs));

    if(c->hwnd != NULL) {
        PostMessage(c->hwnd, KHUI_WM_NC_NOTIFY, 
                    MAKEWPARAM(0, WMNC_IDENTITY_CHANGE), 0);
    }

    return rv;
}

#pragma warning(pop)

KHMEXP khm_int32 KHMAPI
khui_cw_get_primary_id(khui_new_creds * c,
                       khm_handle * ph)
{
    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);
    *ph = NULL;
    if (c->n_identities > 0) {
        *ph = c->identities[0];
    }
    LeaveCriticalSection(&c->cs);

    if (*ph) {
        kcdb_identity_hold(*ph);
        return KHM_ERROR_SUCCESS;
    } else {
        return KHM_ERROR_NOT_FOUND;
    }
}

KHMEXP khui_action_context * KHMAPI
khui_cw_get_ctx(khui_new_creds * c)
{
    ASSERT_NC(c);
    return &c->ctx;
}

KHMEXP khm_int32 KHMAPI
khui_cw_get_subtype(khui_new_creds * c)
{
    ASSERT_NC(c);
    return c->subtype;
}

KHMEXP khm_int32 KHMAPI
khui_cw_get_result(khui_new_creds * c)
{
    ASSERT_NC(c);
    return c->result;
}

KHMEXP khm_boolean KHMAPI
khui_cw_get_use_as_default(khui_new_creds * c)
{
    ASSERT_NC(c);
    return c->set_default;
}

KHMEXP khm_boolean KHMAPI
khui_cw_get_persist_private_data(khui_new_creds * c)
{
    ASSERT_NC(c);
    return c->persist_privcred;
}

KHMEXP khm_int32 KHMAPI 
khui_cw_add_type(khui_new_creds * c, 
                 khui_new_creds_by_type * t)
{
    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);

    if(c->n_types >= KHUI_MAX_NCTYPES) {
        LeaveCriticalSection(&c->cs);
        return KHM_ERROR_OUT_OF_BOUNDS;
    }

    if(c->nc_types < c->n_types + 1) {
        c->nc_types = UBOUNDSS(c->n_types + 1, CW_ALLOC_INCR, CW_ALLOC_INCR);
#ifdef DEBUG
        assert (c->nc_types >= c->n_types + 1);
#endif
        c->types = PREALLOC(c->types, sizeof(c->types[0]) * c->nc_types);
        c->type_subs = PREALLOC(c->type_subs, sizeof(c->type_subs[0]) * c->nc_types);
    }

#ifdef DEBUG
    assert(c->types != NULL);
    assert(c->type_subs != NULL);
#endif

    ZeroMemory(&c->types[c->n_types], sizeof(c->types[0]));
    c->types[c->n_types].nct = t;
    if (t->name)
        c->types[c->n_types].display_name = PWCSDUP(t->name);
    else {
        khm_size cb = 0;
        wchar_t * s = NULL;

        kcdb_get_resource(KCDB_HANDLE_FROM_CREDTYPE(t->type),
                          KCDB_RES_DISPLAYNAME, KCDB_RFS_SHORT, NULL, NULL,
                          NULL, &cb);

        if (cb > sizeof(wchar_t)) {
            s = PMALLOC(cb);

            kcdb_get_resource(KCDB_HANDLE_FROM_CREDTYPE(t->type),
                              KCDB_RES_DISPLAYNAME, KCDB_RFS_SHORT, NULL, NULL,
                              s, &cb);
        }
        c->types[c->n_types].display_name = s;
    }
    c->types[c->n_types].identity_state = KHUI_CWNIS_READY;
    c->type_subs[c->n_types] = kcdb_credtype_get_sub(t->type);
    t->nc = c;
    c->n_types ++;
    LeaveCriticalSection(&c->cs);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI 
khui_cw_del_type(khui_new_creds * c, 
                 khm_int32 type_id)
{
    khm_size  i;

    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);

    for(i=0; i < c->n_types; i++) {
        if(c->types[i].nct->type == type_id)
            break;
    }

    if(i >= c->n_types) {
        LeaveCriticalSection(&c->cs);
        return KHM_ERROR_NOT_FOUND;
    }

    if (c->types[i].display_name != NULL) {
        PFREE((void *) c->types[i].display_name);
        c->types[i].display_name = NULL;
    }

    c->n_types--;

    for(;i < c->n_types; i++) {
        c->types[i] = c->types[i+1];
    }

    LeaveCriticalSection(&c->cs);
    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI 
khui_cw_find_type(khui_new_creds * c, 
                  khm_int32 type, 
                  khui_new_creds_by_type **t)
{
    khm_size i;

    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);
    *t = NULL;
    for(i=0;i<c->n_types;i++) {
        if(c->types[i].nct->type == type) {
            *t = c->types[i].nct;
            break;
        }
    }
    LeaveCriticalSection(&c->cs);

    if(*t)
        return KHM_ERROR_SUCCESS;
    return KHM_ERROR_NOT_FOUND;
}


KHMEXP khm_int32 KHMAPI 
khui_cw_enable_type(khui_new_creds * c,
                    khm_int32 type,
                    khm_boolean enable)
{
    khui_new_creds_by_type * t = NULL;

    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);
    if(KHM_SUCCEEDED(khui_cw_find_type(c, type, &t))) {
        if(enable && (t->flags & KHUI_NCT_FLAG_DISABLED)) {
            t->flags &= ~KHUI_NCT_FLAG_DISABLED;
            c->privint.initialized = FALSE;
        }
        else if (!enable && !(t->flags & KHUI_NCT_FLAG_DISABLED)) {
            t->flags |= KHUI_NCT_FLAG_DISABLED;
            c->privint.initialized = FALSE;
        }
    }
    LeaveCriticalSection(&c->cs);

    return (t)?KHM_ERROR_SUCCESS:KHM_ERROR_NOT_FOUND;
}

KHMEXP khm_boolean KHMAPI 
khui_cw_type_succeeded(khui_new_creds * c,
                       khm_int32 type)
{
    khui_new_creds_by_type * t;
    khm_boolean s;

    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);
    if(KHM_SUCCEEDED(khui_cw_find_type(c, type, &t))) {
        s = (t->flags & KHUI_NC_RESPONSE_COMPLETED) &&
            !(t->flags & KHUI_NC_RESPONSE_FAILED);
    } else {
        s = FALSE;
    }
    LeaveCriticalSection(&c->cs);

    return s;
}

KHMEXP khm_int32 KHMAPI 
khui_cw_set_response(khui_new_creds * c, 
                     khm_int32 type, 
                     khm_int32 response)
{
    khui_new_creds_by_type * t = NULL;

    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);
    khui_cw_find_type(c, type, &t);
    if (response & KHUI_NC_RESPONSE_PENDING)
        response |= KHUI_NC_RESPONSE_NOEXIT;
    c->response |= response & KHUI_NCMASK_RESPONSE;
    if(t) {
        t->flags &= ~KHUI_NCMASK_RESULT;
        t->flags |= (response & KHUI_NCMASK_RESULT);

        if (response & (KHUI_NC_RESPONSE_NOEXIT |
                        KHUI_NC_RESPONSE_PENDING))
            t->flags &= ~KHUI_NC_RESPONSE_COMPLETED;
        else
            t->flags |= KHUI_NC_RESPONSE_COMPLETED;
    }
    LeaveCriticalSection(&c->cs);
    return KHM_ERROR_SUCCESS;
}


/*******************************************************
  New identity creation
 *******************************************************/

/* called with c->cs held */
static khui_identity_selector *
cw_find_selector(khui_new_creds * c,
                 khui_idsel_factory cb,
                 void * cb_data)
{
    khm_size i;

    for (i=0; i < c->n_selectors; i++) {
        if (cb == c->selectors[i].factory_cb &&
            cb_data == c->selectors[i].factory_cb_data)
            return &c->selectors[i];
    }

    return NULL;
}

#define NC_N_SELECTORS 4

KHMEXP khm_int32 KHMAPI
khui_cw_add_selector(khui_new_creds * c,
                     khui_idsel_factory factory_cb,
                     void * factory_cb_data)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;

    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);

    if (cw_find_selector(c, factory_cb, factory_cb_data)) {
        rv = KHM_ERROR_EXISTS;
        goto _exit;
    }

    if (c->nc_selectors < c->n_selectors + 1) {
        c->nc_selectors = UBOUNDSS(c->n_selectors + 1, NC_N_SELECTORS, NC_N_SELECTORS);
        c->selectors = PREALLOC(c->selectors, sizeof(c->selectors[0]) * c->nc_selectors);
    }

    assert(c->nc_selectors >= c->n_selectors + 1);
    assert(c->selectors != NULL);

    memset(&c->selectors[c->n_selectors], 0, sizeof(c->selectors[0]));

    c->selectors[c->n_selectors].hwnd_selector = NULL;
    c->selectors[c->n_selectors].display_name = NULL;
    c->selectors[c->n_selectors].icon = NULL;
    c->selectors[c->n_selectors].factory_cb_data = factory_cb_data;
    c->selectors[c->n_selectors].factory_cb = factory_cb;

    c->n_selectors++;

 _exit:
    LeaveCriticalSection(&c->cs);

    return rv;
}

/**********************************************************/
/*     Communication with the New Credentials Wizard      */
/**********************************************************/

KHMEXP khm_int32 KHMAPI
khui_cw_notify_dialog(khui_new_creds * c, enum khui_wm_nc_notifications notification,
                      khm_boolean sync_required, void * param)
{
    ASSERT_NC(c);

    if (c->hwnd == NULL)
        return KHM_ERROR_INVALID_OPERATION;

    if (sync_required) {
        SendMessage(c->hwnd, KHUI_WM_NC_NOTIFY,
                    MAKEWPARAM(0, notification),
                    (LPARAM) param);
    } else {
        PostMessage(c->hwnd, KHUI_WM_NC_NOTIFY,
                    MAKEWPARAM(0, notification),
                    (LPARAM) param);
    }

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
khui_cw_notify_identity_state(khui_new_creds * c,
                              HWND owner,
                              const wchar_t * state_string,
                              khm_int32 flags,
                              khm_int32 progress)
{
    nc_identity_state_notification notif;

    ASSERT_NC(c);

    if (owner == NULL)
        return KHM_ERROR_INVALID_PARAM;

    notif.credtype_panel = NULL;
    notif.privint_panel = NULL;

    EnterCriticalSection(&c->cs);
    do {
        khui_new_creds_privint_panel * p = NULL;
        khm_size i;

        for (i=0; i < c->n_types; i++) {
            if (c->types[i].nct->hwnd_panel == owner) {
                if (c->types[i].nct->flags & KHUI_NCT_FLAG_DISABLED)
                    continue;
                notif.credtype_panel = &c->types[i];
                break;
            }
        }

        if (notif.credtype_panel != NULL)
            break;

        for (p = QTOP(&c->privint.shown); p; p = QNEXT(p)) {
            ASSERT_PP(p);
            if (p->hwnd == owner) {
                notif.privint_panel = p;
                break;
            }
        }

        if (notif.privint_panel != NULL)
            break;

        for (i=0; i < c->n_types; i++) {

            /* A disabled credentials type panel can still ask for a
               privileged interaction panel to be shown.  Therefore we
               don't test for the DISABLED flag here. */

            for (p = QTOP(&c->types[i]); p; p = QNEXT(p)) {
                ASSERT_PP(p);
                if (p->hwnd == owner) {
                    notif.privint_panel = p;
                    break;
                }
            }
        }

    } while (FALSE);

    if (notif.credtype_panel)
        notif.credtype_panel->identity_state = flags;
    if (notif.privint_panel)
        notif.privint_panel->identity_state = flags;

    LeaveCriticalSection(&c->cs);

    if (notif.privint_panel == NULL && notif.credtype_panel == NULL)
        return KHM_ERROR_INVALID_PARAM;

    notif.magic = KHUI_NC_IDENTITY_STATE_NOTIF_MAGIC;
    notif.owner = owner;
    notif.state_string = state_string;
    notif.flags = flags;
    notif.progress = progress;

    SendMessage(c->hwnd, KHUI_WM_NC_NOTIFY,
                MAKEWPARAM(0, WMNC_IDENTITY_STATE),
                (LPARAM) &notif);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_boolean KHMAPI
khui_cw_is_ready(khui_new_creds * c)
{
    khm_boolean ready = TRUE;

    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);
    {
        khui_new_creds_privint_panel * p;
        khm_size i;

        for (i=0; i < c->n_types && ready; i++) {
            if (c->types[i].nct->flags & KHUI_NCT_FLAG_DISABLED)
                continue;

            if ((c->types[i].identity_state & KHUI_CWNIS_READY) == 0)
                ready = FALSE;

            for (p = QTOP(&c->types[i]); p && ready; p = QNEXT(p)) {
                ASSERT_PP(p);

                if ((p->identity_state & KHUI_CWNIS_READY) == 0)
                    ready = FALSE;
            }
        }

        for (p = QTOP(&c->privint.shown); p && ready; p = QNEXT(p)) {
            if ((p->identity_state & KHUI_CWNIS_READY) == 0)
                ready = FALSE;
        }
    }
    LeaveCriticalSection(&c->cs);

    return ready;
}

/************************************************************/
/*                 Privileged Interaction                   */
/************************************************************/

static khui_new_creds_privint_panel *
cw_create_privint_panel(HWND hwnd,
                        const wchar_t * caption)
{
    khui_new_creds_privint_panel * p;

    p = PMALLOC(sizeof(*p));
    ZeroMemory(p, sizeof(*p));

    p->magic = KHUI_NEW_CREDS_PRIVINT_PANEL_MAGIC;
    p->hwnd = hwnd;
    p->identity_state = KHUI_CWNIS_READY;
    if (caption)
        StringCbCopy(p->caption, sizeof(p->caption), caption);

    return p;
}

KHMEXP khm_int32 KHMAPI
khui_cw_free_privint(khui_new_creds_privint_panel * pp)
{
    ASSERT_PP(pp);

    if (pp->hwnd) {
        DestroyWindow(pp->hwnd);
        pp->hwnd = NULL;
    }

    cw_free_prompts(pp);

    pp->magic = 0;

    PFREE(pp);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
khui_cw_show_privileged_dialog(khui_new_creds * nc, khm_int32 ctype,
                               HWND hwnd, const wchar_t * caption)
{
    khm_size i;
    khui_new_creds_privint_panel * pp;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    ASSERT_NC(nc);

    EnterCriticalSection(&nc->cs);
    for (i=0; i < nc->n_types; i++) {
        if (nc->types[i].nct->type == ctype)
            break;
    }
    if (i == nc->n_types) {
        rv = KHM_ERROR_NOT_FOUND;
        goto done;
    }

    pp = cw_create_privint_panel(hwnd, caption);

    pp->nc = nc;
    pp->ctype = ctype;
    pp->use_custom = FALSE;

    QPUT(&nc->types[i], pp);
 done:
    LeaveCriticalSection(&nc->cs);

    if (KHM_SUCCEEDED(rv)) {
        PostMessage(nc->hwnd, KHUI_WM_NC_NOTIFY, 
                    MAKEWPARAM(0, WMNC_SET_PROMPTS), (LPARAM) nc);
    }

    return rv;
}

KHMEXP khm_int32 KHMAPI
khui_cw_peek_next_privint(khui_new_creds * c,
                          khui_new_creds_privint_panel ** ppp)
{
    khm_int32 rv = KHM_ERROR_NOT_FOUND;
    khm_size i;
    ASSERT_NC(c);

    if (ppp)
        *ppp = NULL;

    EnterCriticalSection(&c->cs);
    if (c->privint.legacy_panel &&
        c->privint.legacy_panel->use_custom &&
        !QPREV(c->privint.legacy_panel) &&
        !QNEXT(c->privint.legacy_panel) &&
        c->privint.shown.current_panel != c->privint.legacy_panel &&
        !c->privint.legacy_panel->processed) {
        if (ppp)
            *ppp = c->privint.legacy_panel;
        rv = KHM_ERROR_SUCCESS;
        goto done;
    }

    for (i=0; i < c->n_types; i++) {
        if (c->types[i].nct->flags & KHUI_NCT_FLAG_DISABLED)
            continue;

        if (QTOP(&c->types[i])) {
            if (ppp)
                *ppp = QTOP(&c->types[i]);
            rv = KHM_ERROR_SUCCESS;
            break;
        }
    }

 done:
    LeaveCriticalSection(&c->cs);

    return rv;
}


KHMEXP khm_int32 KHMAPI
khui_cw_get_next_privint(khui_new_creds * c,
                         khui_new_creds_privint_panel ** ppp)
{
    khui_new_creds_privint_panel * pp = NULL;
    khm_size i;

    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);

    /* First see if there is a usable legacy privileged interaction panel. */
    pp = c->privint.legacy_panel;

    if (pp == NULL || !pp->use_custom || pp->processed) {

        pp = NULL;

        /* This assumes that the credentials types are in a suitable
           order since we pick the first one we find a privint panel
           for. */
        for (i=0; i < c->n_types; i++) {
            if (c->types[i].nct->flags & KHUI_NCT_FLAG_DISABLED)
                continue;

            if (QTOP(&c->types[i])) {
                QGET(&c->types[i], &pp);
                break;
            }
        }
    }

    if (pp) {
        khui_new_creds_privint_panel * q;

        ASSERT_PP(pp);
        for (q = QTOP(&c->privint.shown); q; q = QNEXT(q)) {
            if (pp == q)
                break;
        }

        assert(q == NULL);

        if (q == NULL)
            QPUT(&c->privint.shown, pp);
        c->privint.shown.show_blank = FALSE;
    } else {
        c->privint.shown.show_blank = TRUE;
    }

    LeaveCriticalSection(&c->cs);

    if (ppp)
        *ppp = pp;

    return (pp)?KHM_ERROR_SUCCESS:KHM_ERROR_NOT_FOUND;
}

KHMEXP khm_int32 KHMAPI
khui_cw_revoke_privileged_dialogs(khui_new_creds * c, khm_int32 ctype)
{
    khui_new_creds_privint_panel *p, *np;
    khm_size i;
    khm_boolean do_set_prompts = FALSE;

    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);

    for (p = QTOP(&c->privint.shown); p; p = np) {
        np = QNEXT(p);
        ASSERT_PP(p);
        if (p->ctype == ctype) {
            QDEL(&c->privint.shown, p);
            if (c->privint.shown.current_panel == p)
                c->privint.shown.current_panel = NULL;
            khui_cw_free_privint(p);
            do_set_prompts = TRUE;
        }
    }

    for (i=0; i < c->n_types; i++) {
        if (c->types[i].nct->type != ctype)
            continue;
        while (QTOP(&c->types[i])) {
            QGET(&c->types[i], &p);
            ASSERT_PP(p);
            if (c->privint.shown.current_panel == p)
                c->privint.shown.current_panel = NULL;
            khui_cw_free_privint(p);
            do_set_prompts = TRUE;
        }
    }
    LeaveCriticalSection(&c->cs);

    if (do_set_prompts)
        PostMessage(c->hwnd, KHUI_WM_NC_NOTIFY, 
                    MAKEWPARAM(0, WMNC_SET_PROMPTS), (LPARAM) c);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
khui_cw_clear_all_privints(khui_new_creds * c)
{
    khui_new_creds_privint_panel * p;
    khm_size i;

    ASSERT_NC(c);

    khui_cw_clear_prompts(c);

    EnterCriticalSection(&c->cs);

    QGET(&c->privint.shown, &p);
    while (p) {
        khui_cw_free_privint(p);
        QGET(&c->privint.shown, &p);
    }

    for (i=0; i < c->n_types; i++) {
        while (QTOP(&c->types[i])) {
            QGET(&c->types[i], &p);
            khui_cw_free_privint(p);
        }
    }
    c->privint.shown.current_panel = NULL;
    LeaveCriticalSection(&c->cs);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
khui_cw_purge_deleted_shown_panels(khui_new_creds * c)
{
    khui_new_creds_privint_panel * p;
    khui_new_creds_privint_panel * np;

    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);
    for (p = QTOP(&c->privint.shown); p; p = np) {
        np = QNEXT(p);
        ASSERT_PP(p);

        if (!IsWindow(p->hwnd)) {
            QDEL(&c->privint.shown, p);

            if (c->privint.shown.current_panel == p)
                c->privint.shown.current_panel = NULL;

            p->hwnd = NULL;
            khui_cw_free_privint(p);
        }
    }
    LeaveCriticalSection(&c->cs);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khui_new_creds_privint_panel * KHMAPI
khui_cw_get_current_privint_panel(khui_new_creds * c)
{
    khui_new_creds_privint_panel * p;

    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);
    p = c->privint.shown.current_panel;
    LeaveCriticalSection(&c->cs);

    return p;
}

KHMEXP void KHMAPI
khui_cw_set_current_privint_panel(khui_new_creds * c,
                                  khui_new_creds_privint_panel * p)
{

    ASSERT_NC(c);
    assert(p == NULL || IS_PP(p));

    EnterCriticalSection(&c->cs);
    c->privint.shown.current_panel = p;
    LeaveCriticalSection(&c->cs);

}

/************************************************************/
/*                     Custom prompts                       */
/************************************************************/

static khui_new_creds_prompt * 
cw_create_prompt(khm_size idx,
                 khm_int32 type,
                 wchar_t * prompt,
                 wchar_t * def,
                 khm_int32 flags)
{
    khui_new_creds_prompt * p;
    size_t cb_prompt = 0;
    size_t cb_def = 0;

    if(prompt && FAILED(StringCbLength(prompt, KHUI_MAXCB_PROMPT, &cb_prompt)))
        return NULL;
    if(def && FAILED(StringCbLength(def, KHUI_MAXCB_PROMPT_VALUE, &cb_def)))
        return NULL;

    p = PMALLOC(sizeof(*p));
    ZeroMemory(p, sizeof(*p));

    if(prompt) {
        cb_prompt += sizeof(wchar_t);
        p->prompt = PMALLOC(cb_prompt);
        StringCbCopy(p->prompt, cb_prompt, prompt);
    }

    if(def && cb_def > 0) {
        cb_def += sizeof(wchar_t);
        p->def = PMALLOC(cb_def);
        StringCbCopy(p->def, cb_def, def);
    }

    p->value = PMALLOC(KHUI_MAXCB_PROMPT_VALUE);
    ZeroMemory(p->value, KHUI_MAXCB_PROMPT_VALUE);

    p->type = type;
    p->flags = flags;
    p->index = idx;

    return p;
}

static void 
cw_free_prompt(khui_new_creds_prompt * p) {
    size_t cb;

    if(p->prompt) {
        if(SUCCEEDED(StringCbLength(p->prompt, KHUI_MAXCB_PROMPT, &cb)))
            SecureZeroMemory(p->prompt, cb);
        PFREE(p->prompt);
    }

    if(p->def) {
        if(SUCCEEDED(StringCbLength(p->def, KHUI_MAXCB_PROMPT, &cb)))
            SecureZeroMemory(p->def, cb);
        PFREE(p->def);
    }

    if(p->value) {
        if(SUCCEEDED(StringCbLength(p->value, KHUI_MAXCB_PROMPT_VALUE, &cb)))
            SecureZeroMemory(p->value, cb);
        PFREE(p->value);
    }

#ifdef DEBUG
    assert(p->hwnd_static == NULL);
    assert(p->hwnd_edit == NULL);
#endif

    PFREE(p);
}

static void 
cw_free_prompts(khui_new_creds_privint_panel * p)
{
    khm_size i;

    ASSERT_PP(p);

    if(p->banner != NULL) {
        PFREE(p->banner);
        p->banner = NULL;
    }

    if(p->pname != NULL) {
        PFREE(p->pname);
        p->pname = NULL;
    }

    for (i=0; i < p->n_prompts; i++) {
        if (p->prompts[i]) {
            cw_free_prompt(p->prompts[i]);
            p->prompts[i] = NULL;
        }
    }

    if(p->prompts != NULL) {
        PFREE(p->prompts);
        p->prompts = NULL;
    }

    p->nc_prompts = 0;
    p->n_prompts = 0;
    p->use_custom = FALSE;

#ifdef DEBUG
    assert(p->hwnd == NULL);
#endif
}

KHMEXP khm_int32 KHMAPI 
khui_cw_clear_prompts(khui_new_creds * c)
{
    khui_new_creds_privint_panel * p;
    khui_new_creds_privint_panel * q;

    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);
    p = c->privint.legacy_panel;
    if (p) {

        /* Is this the current privileged interaction panel? */
        if (p == c->privint.shown.current_panel &&
            !c->privint.shown.show_blank) {
#ifdef DEBUG
            assert(p->use_custom);
#endif
            p->use_custom = FALSE;

            LeaveCriticalSection(&c->cs);
            SendMessage(c->hwnd, KHUI_WM_NC_NOTIFY,
                        MAKEWPARAM(0, WMNC_SET_PROMPTS),
                        (LPARAM) c);
            EnterCriticalSection(&c->cs);

            QDEL(&c->privint.shown, p);
        }

        /* If p is in the shown queue, we should dequeue it since we
           are preparing it to be shown again.  Otherwise the queue
           will break later. */
        for (q = QTOP(&c->privint.shown); q; q = QNEXT(q))
            if (p == q)
                break;

        if (q)
            QDEL(&c->privint.shown, p);

        {
            khm_size i;

            if (p->hwnd != NULL) {
                LeaveCriticalSection(&c->cs);
                SendMessage(p->hwnd, WM_CLOSE, 0, 0);
                EnterCriticalSection(&c->cs);
            }

            /* The controls should already have been destroyed.  We
               just dis-associate the controls from the data
               structure */

            for (i=0; i < p->n_prompts; i++) {
                if (p->prompts[i]) {
                    p->prompts[i]->hwnd_edit = NULL;
                    p->prompts[i]->hwnd_static = NULL;
                }
            }

            p->hwnd = NULL;
        }

        cw_free_prompts(p);
    }
    LeaveCriticalSection(&c->cs);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI 
khui_cw_begin_custom_prompts(khui_new_creds * c, 
                             khm_size n_prompts, 
                             wchar_t * banner, 
                             wchar_t * pname)
{
    size_t cb;
    khui_new_creds_privint_panel * p;

    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);

    p = c->privint.legacy_panel;

    assert(p == NULL || (p->hwnd == NULL && p->n_prompts == 0));

    if (p == NULL) {
        p = cw_create_privint_panel(NULL, NULL);
        c->privint.legacy_panel = p;
    }

    assert(p != NULL);

    if(SUCCEEDED(StringCbLength(banner, KHUI_MAXCB_BANNER, &cb)) && 
       cb > 0) {

        cb += sizeof(wchar_t);
        p->banner = PMALLOC(cb);
        StringCbCopy(p->banner, cb, banner);

    } else {
        p->banner = NULL;
    }

    if(SUCCEEDED(StringCbLength(pname, KHUI_MAXCB_PNAME, &cb)) && 
       cb > 0) {

        cb += sizeof(wchar_t);
        p->pname = PMALLOC(cb);
        StringCbCopy(p->pname, cb, pname);

    } else {
        p->pname = NULL;
    }

    if(n_prompts > 0) {
        p->prompts = PMALLOC(sizeof(*(p->prompts)) * n_prompts);
        ZeroMemory(p->prompts, sizeof(*(p->prompts)) * n_prompts);
        p->nc_prompts = n_prompts;
        p->n_prompts = 0;
        p->use_custom = FALSE;
    } else {
        p->prompts = NULL;
        p->n_prompts = 0;
        p->nc_prompts = 0;
        p->use_custom = TRUE;
    }

    LeaveCriticalSection(&c->cs);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI 
khui_cw_add_prompt(khui_new_creds * c, 
                   khm_int32 type, 
                   wchar_t * prompt, 
                   wchar_t * def, 
                   khm_int32 flags)
{
    khui_new_creds_privint_panel * p;
    khui_new_creds_prompt * pr;
    khm_boolean done_with_panel = FALSE;

    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);

    p = c->privint.legacy_panel;

    if (p == NULL || p->n_prompts >= p->nc_prompts || p->hwnd || p->use_custom) {
        LeaveCriticalSection(&c->cs);
#ifdef DEBUG
        assert(FALSE);
#endif

        return KHM_ERROR_INVALID_OPERATION;
    }

#ifdef DEBUG
    assert(p->prompts != NULL);
#endif

    pr = cw_create_prompt(p->n_prompts, type, prompt, def, flags);
    if(pr == NULL) {
        LeaveCriticalSection(&c->cs);
        return KHM_ERROR_INVALID_PARAM;
    }
    p->prompts[p->n_prompts++] = pr;

    if (p->n_prompts == p->nc_prompts) {
        p->use_custom = TRUE;
        done_with_panel = TRUE;
    }
    LeaveCriticalSection(&c->cs);

    if(done_with_panel) {
        PostMessage(c->hwnd, KHUI_WM_NC_NOTIFY, 
                    MAKEWPARAM(0, WMNC_SET_PROMPTS), (LPARAM) c);
    }

    return KHM_ERROR_SUCCESS;
}


KHMEXP khm_int32 KHMAPI 
khui_cw_get_prompt_count(khui_new_creds * c,
                         khm_size * np) {

    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);
    if (c->privint.legacy_panel) {
        *np = c->privint.legacy_panel->n_prompts;
    } else {
        *np = 0;
    }
    LeaveCriticalSection(&c->cs);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI 
khui_cw_get_prompt(khui_new_creds * c, 
                   khm_size idx, 
                   khui_new_creds_prompt ** prompt)
{
    khm_int32 rv;
    khui_new_creds_privint_panel * p;

    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);
    p = c->privint.legacy_panel;

    if (p == NULL) {
        rv = KHM_ERROR_INVALID_PARAM;
        *prompt = NULL;
    } else if (p->n_prompts <= idx ||
               p->prompts == NULL) {
        rv = KHM_ERROR_OUT_OF_BOUNDS;
        *prompt = NULL;
    } else {
        *prompt = p->prompts[idx];
        rv = KHM_ERROR_SUCCESS;
    }
    LeaveCriticalSection(&c->cs);

    return rv;
}

void
khuiint_trim_str(wchar_t * s, khm_size cch) {
    wchar_t * c, * last_ws;

    for (c = s; *c && iswspace(*c) && ((khm_size)(c - s)) < cch; c++);

    if (((khm_size)(c - s)) >= cch)
        return;

    if (c != s && ((khm_size)(c - s)) < cch) {
#if _MSC_VER >= 1400 && __STDC_WANT_SECURE_LIB__
        wmemmove_s(s, cch, c, cch - ((khm_size)(c - s)));
#else
        memmove(s, c, (cch - ((khm_size)(c - s)))* sizeof(wchar_t));
#endif
    }

    last_ws = NULL;
    for (c = s; *c && ((khm_size)(c - s)) < cch; c++) {
        if (!iswspace(*c))
            last_ws = NULL;
        else if (last_ws == NULL)
            last_ws = c;
    }

    if (last_ws)
        *last_ws = L'\0';
}

KHMEXP khm_int32 KHMAPI 
khui_cw_sync_prompt_values(khui_new_creds * c)
{
    khm_size i;
    khm_size n;
    HWND hw;
    wchar_t tmpbuf[KHUI_MAXCCH_PROMPT_VALUE];
    khui_new_creds_privint_panel * p;

    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);

    p = c->privint.legacy_panel;
    if (p == NULL) {
        LeaveCriticalSection(&c->cs);
        return KHM_ERROR_INVALID_OPERATION;
    }

 redo_loop:
    n = p->n_prompts;
    for(i=0; i<n; i++) {
        khui_new_creds_prompt * pr;

        pr = p->prompts[i];
        if (pr->hwnd_edit) {
            hw = pr->hwnd_edit;
            LeaveCriticalSection(&c->cs);

            GetWindowText(hw, tmpbuf, ARRAYLENGTH(tmpbuf));
            khuiint_trim_str(tmpbuf, ARRAYLENGTH(tmpbuf));

            EnterCriticalSection(&c->cs);
            if (n != p->n_prompts)
                goto redo_loop;
            SecureZeroMemory(pr->value, KHUI_MAXCB_PROMPT_VALUE);
            StringCchCopy(pr->value, KHUI_MAXCCH_PROMPT_VALUE,
                          tmpbuf);
        }
    }
    LeaveCriticalSection(&c->cs);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI 
khui_cw_get_prompt_value(khui_new_creds * c, 
                         khm_size idx, 
                         wchar_t * buf, 
                         khm_size *cbbuf)
{
    khui_new_creds_prompt * pr;
    khm_int32 rv;
    size_t cb;

    ASSERT_NC(c);

    rv = khui_cw_get_prompt(c, idx, &pr);
    if(KHM_FAILED(rv))
        return rv;

    EnterCriticalSection(&c->cs);

    if(FAILED(StringCbLength(pr->value, KHUI_MAXCB_PROMPT_VALUE, &cb))) {
        *cbbuf = 0;
        if(buf != NULL)
            *buf = 0;
        LeaveCriticalSection(&c->cs);
        return KHM_ERROR_UNKNOWN;
    }
    cb += sizeof(wchar_t);

    if(buf == NULL || *cbbuf < cb) {
        *cbbuf = cb;
        LeaveCriticalSection(&c->cs);
        return KHM_ERROR_TOO_LONG;
    }

    StringCbCopy(buf, *cbbuf, pr->value);
    *cbbuf = cb;
    LeaveCriticalSection(&c->cs);

    return KHM_ERROR_SUCCESS;
}

/* only called from a identity provider callback */
KHMEXP khm_int32 KHMAPI
khui_cw_add_control_row(khui_new_creds * c,
                        HWND label,
                        HWND input,
                        khui_control_size size)
{

    ASSERT_NC(c);

    if (c && c->hwnd) {
        khui_control_row row;

        row.label = label;
        row.input = input;
        row.size = size;

        SendMessage(c->hwnd,
                    KHUI_WM_NC_NOTIFY,
                    MAKEWPARAM(0, WMNC_ADD_CONTROL_ROW),
                    (LPARAM) &row);

        return KHM_ERROR_SUCCESS;
    } else {
        return KHM_ERROR_INVALID_PARAM;
    }
}

KHMEXP khm_int32 KHMAPI
khui_cw_collect_privileged_credentials(khui_new_creds * c,
                                       khm_handle identity,
                                       khm_handle dest_credset)
{
    khui_collect_privileged_creds_data cpcd;

    ASSERT_NC(c);

    cpcd.nc = c;
    cpcd.target_identity = identity;
    cpcd.dest_credset = dest_credset;

    return (khm_int32)
        SendMessage(c->hwnd, KHUI_WM_NC_NOTIFY,
                    MAKEWPARAM(0, WMNC_COLLECT_PRIVCRED),
                    (LPARAM) &cpcd);
}

KHMEXP khm_int32 KHMAPI
khui_cw_get_privileged_credential_collector(khui_new_creds * c,
                                            khm_handle * dest_credset)
{
    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);
    *dest_credset = c->cs_privcred;
    LeaveCriticalSection(&c->cs);

    if (*dest_credset)
        return KHM_ERROR_SUCCESS;
    else
        return KHM_ERROR_NOT_FOUND;
}

KHMEXP khm_int32 KHMAPI
khui_cw_get_privileged_credential_store(khui_new_creds * c,
                                        khm_handle * p_id)
{
    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);
    *p_id = c->persist_identity;
    if (*p_id)
        kcdb_identity_hold(*p_id);
    LeaveCriticalSection(&c->cs);

    if (*p_id)
        return KHM_ERROR_SUCCESS;
    else
        return KHM_ERROR_NOT_FOUND;
}

KHMEXP khm_int32 KHMAPI
khui_cw_set_privileged_credential_collector(khui_new_creds * c,
                                            khm_handle dest_credset)
{
    ASSERT_NC(c);

    EnterCriticalSection(&c->cs);
    c->cs_privcred = dest_credset;
    LeaveCriticalSection(&c->cs);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
khui_cw_derive_credentials(khui_new_creds * c,
                           khm_handle identity,
                           khm_handle dest_credset)
{
    khui_collect_privileged_creds_data cpcd;

    ASSERT_NC(c);

    cpcd.nc = c;
    cpcd.target_identity = identity;
    cpcd.dest_credset = dest_credset;

    return (khm_int32)
        SendMessage(c->hwnd, KHUI_WM_NC_NOTIFY,
                    MAKEWPARAM(0, WMNC_DERIVE_FROM_PRIVCRED),
                    (LPARAM) &cpcd);
}
