/*
 * Copyright (c) 2006, 2007, 2008, 2009 Secure Endpoints Inc.
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

#include "krbcred.h"
#include<strsafe.h>
#include<assert.h>

static khm_int32 __stdcall
kinit_task_proc(void * vparam);

k5_kinit_task *
k5_kinit_task_create(khui_new_creds * nc)
{
    k5_kinit_task * kt;
    k5_dlg_data   * d;

    wchar_t idname[KCDB_IDENT_MAXCCH_NAME];
    khm_size cbbuf;

    LPNETID_DLGINFO pdlginfo;
    khui_action_context * pctx = NULL;

    khm_handle hsub = NULL;

    khui_cw_find_type(nc, credtype_id_krb5, (khui_new_creds_by_type **) &d);
    if (!d)
	return NULL;

    kt = (k5_kinit_task *) PMALLOC(sizeof(*kt));
    memset(kt, 0, sizeof(*kt));
    kt->magic = K5_KINIT_TASK_MAGIC;
    kt->nc = nc;
    kt->dlg_data = d;
    kt->nct = &d->nct;

    khui_cw_get_primary_id(nc, &kt->identity);
    cbbuf = sizeof(idname);
    kcdb_identity_get_name(kt->identity, idname, &cbbuf);
    cbbuf = (cbbuf * sizeof(char)) / sizeof(wchar_t);

    kt->principal = PMALLOC(cbbuf);
    UnicodeStrToAnsi(kt->principal, cbbuf, idname);
    kt->password = NULL;
    kt->params = d->params;
    kt->params.lifetime = (krb5_deltat) d->tc_lifetime.current;
    kt->params.renew_life = (krb5_deltat) d->tc_renew.current;

    /* if we have external parameters, we should use them as well */
    pctx = khui_cw_get_ctx(nc);
    if (pctx && pctx->cb_vparam == sizeof(NETID_DLGINFO) &&
        (pdlginfo = pctx->vparam) != NULL &&
        pdlginfo->size == NETID_DLGINFO_V1_SZ) {

        wchar_t * t;
        size_t size;

        if (pdlginfo->in.ccache[0] &&
            SUCCEEDED(StringCchLength(pdlginfo->in.ccache,
                                      NETID_CCACHE_NAME_SZ,
                                      &size))) {
            kt->ccache = PMALLOC(sizeof(char) * (size + 1));
            UnicodeStrToAnsi(kt->ccache, size + 1,
                             pdlginfo->in.ccache);

            /* this is the same as the output cache */

            StringCbCopy(pdlginfo->out.ccache, sizeof(pdlginfo->out.ccache),
                         pdlginfo->in.ccache);
        } else {
            wchar_t ccache[KRB5_MAXCCH_CCNAME];

            size = sizeof(ccache);

            khm_krb5_get_identity_default_ccache(kt->identity, ccache, &size);

            StringCbCopy(pdlginfo->out.ccache, sizeof(pdlginfo->out.ccache),
                         ccache);
        }

        t = khm_get_realm_from_princ(idname);

        if (t) {
            StringCbCopy(pdlginfo->out.realm,
                         sizeof(pdlginfo->out.realm),
                         t);

            if ((t - idname) > 1) {
                StringCchCopyN(pdlginfo->out.username,
                               ARRAYLENGTH(pdlginfo->out.username),
                               idname,
                               (t - idname) - 1);
            } else {
                StringCbCopy(pdlginfo->out.username,
                             sizeof(pdlginfo->out.username),
                             L"");
            }
        } else {
            StringCbCopy(pdlginfo->out.username,
                         sizeof(pdlginfo->out.username),
                         idname);
            StringCbCopy(pdlginfo->out.realm,
                         sizeof(pdlginfo->out.realm),
                         L"");
        }
    }

    kt->state = K5_KINIT_STATE_PREP;
    InitializeCriticalSection(&kt->cs);
    kt->h_task_wait = CreateEvent(NULL, FALSE, FALSE, NULL);
    kt->h_parent_wait = CreateEvent(NULL, FALSE, FALSE, NULL);

    kt->refcount = 2;           /* One hold for the caller, one hold
                                   for the thread */

    kmq_create_subscription(k5_msg_callback, &hsub);

    kt->task = task_create(NULL, 32 * 4096, kinit_task_proc, kt, hsub, 0);

    return kt;
}


static void
kinit_task_free(k5_kinit_task * kt)
{
    EnterCriticalSection(&kt->cs);

    if (kt->refcount != 0 ||
        (kt->state != K5_KINIT_STATE_NONE &&
         kt->state != K5_KINIT_STATE_DONE)) {
        LeaveCriticalSection(&kt->cs);
        return;
    }

    kt->magic = 0;

    if (kt->principal)
        PFREE(kt->principal);

    if (kt->password) {
        SecureZeroMemory(kt->password, strlen(kt->password));
        PFREE(kt->password);
    }

    if (kt->identity)
        kcdb_identity_release(kt->identity);

    if (kt->ccache)
        PFREE(kt->ccache);

    LeaveCriticalSection(&kt->cs);

    DeleteCriticalSection(&kt->cs);
    CloseHandle(kt->h_task_wait);
    CloseHandle(kt->h_parent_wait);

    if (kt->task)
        task_release(kt->task);

    ZeroMemory(kt, sizeof(*kt));
}

void
k5_kinit_task_hold(k5_kinit_task * kt)
{
    EnterCriticalSection(&kt->cs);
    kt->refcount++;
    LeaveCriticalSection(&kt->cs);
}

/* Should not be called with kt->cs held */
void
k5_kinit_task_release(k5_kinit_task * kt)
{
    khm_boolean free_task = FALSE;

    EnterCriticalSection(&kt->cs);
    if (-- kt->refcount == 0)
        free_task = TRUE;
    LeaveCriticalSection(&kt->cs);

    if (free_task) {
        kinit_task_free(kt);
    }
}

void
k5_kinit_task_abort_and_release(k5_kinit_task * kt)
{
    EnterCriticalSection(&kt->cs);
    if (kt->state < K5_KINIT_STATE_ABORTED) {
        _reportf(L"Aborting k5_kinit_task [%p] for principal [%S]", kt, kt->principal);
        kt->state = K5_KINIT_STATE_ABORTED;
        SetEvent(kt->h_task_wait);
    } /* else, the task is in state DONE */
    LeaveCriticalSection(&kt->cs);
    k5_kinit_task_release(kt);
}

void
k5_kinit_task_confirm_and_wait(k5_kinit_task * kt)
{
 retry:
    EnterCriticalSection(&kt->cs);
    switch (kt->state) {
    case K5_KINIT_STATE_ABORTED:
    case K5_KINIT_STATE_DONE:
        /* The task is not running. */
        LeaveCriticalSection(&kt->cs);
        return;

    case K5_KINIT_STATE_PREP:
    case K5_KINIT_STATE_INCALL:
    case K5_KINIT_STATE_RETRY:
        /* The task hasn't reached a wait state yet.  We should wait
           for one and retry. */
        ResetEvent(kt->h_parent_wait);
        LeaveCriticalSection(&kt->cs);
        WaitForSingleObject(kt->h_parent_wait, INFINITE);
        goto retry;

    case K5_KINIT_STATE_WAIT:
        _reportf(L"Confirming k5_kinit_task [%p] for principal [%S]", kt, kt->principal);
        kt->state = K5_KINIT_STATE_CONFIRM;
        ResetEvent(kt->h_parent_wait);
        SetEvent(kt->h_task_wait);
        break;

    case K5_KINIT_STATE_CONFIRM:
        assert(FALSE);
        break;

    default:
        assert(FALSE);
    }
    LeaveCriticalSection(&kt->cs);

    WaitForSingleObject(kt->h_parent_wait, INFINITE);
}

/* return TRUE if we should go ahead with creds acquisition */
static khm_boolean
cp_check_continue(k5_kinit_task * kt) {
    khm_size i;
    khm_size n_p;
    khui_new_creds_prompt * p;
    size_t cch;

    if (KHM_FAILED(khui_cw_get_prompt_count(kt->nc, &n_p))) {
        assert(FALSE);
        return TRUE;
    }

    khui_cw_sync_prompt_values(kt->nc);

    kt->is_null_password = FALSE;

    /* we are just checking whether there was a password field that
       was left empty, in which case we can't continue with the
       credentials acquisition. */
    for (i=0; i < n_p; i++) {
        if(KHM_FAILED(khui_cw_get_prompt(kt->nc, (int) i, &p)))
            continue;
        if(p->type == KHUI_NCPROMPT_TYPE_PASSWORD) {
            if (p->value == NULL ||
                FAILED(StringCchLength(p->value, KHUI_MAXCCH_PROMPT_VALUE,
                                       &cch)) ||
                cch == 0) {
                kt->is_null_password = TRUE;
                return FALSE;
            } else
                break;
        }
    }

    return TRUE;
}

/* Returns true if we find cached prompts.
   Called with kt->cs held.
 */
static khm_boolean
cached_kinit_prompter(k5_kinit_task * kt) {
    khm_boolean rv = FALSE;
    khm_handle csp_idconfig = NULL;
    khm_handle csp_k5config = NULL;
    khm_handle csp_prcache = NULL;
    khm_size cb;
    khm_size n_cur_prompts;
    khm_int32 n_prompts;
    khm_int32 i;
    khm_int64 iexpiry;
    FILETIME expiry;

    assert(kt->nc);
    
    if (KHM_FAILED(kcdb_identity_get_config(kt->identity, 0, &csp_idconfig)) ||

        KHM_FAILED(khc_open_space(csp_idconfig, CSNAME_KRB5CRED,
                                  0, &csp_k5config)) ||

        KHM_FAILED(khc_open_space(csp_k5config, CSNAME_PROMPTCACHE,
                                  0, &csp_prcache)) ||

        KHM_FAILED(khc_read_int32(csp_prcache, L"PromptCount",
                                  &n_prompts)) ||
        n_prompts == 0)

        goto _cleanup;

    if (KHM_SUCCEEDED(khc_read_int64(csp_prcache, L"ExpiresOn", &iexpiry))) {
        FILETIME current;

        /* has the cache expired? */
        expiry = IntToFt(iexpiry);
        GetSystemTimeAsFileTime(&current);

        if (CompareFileTime(&expiry, &current) < 0)
            /* already expired */
            goto _cleanup;
    } else {
        /* if there is no value for ExpiresOn, we assume the prompts
           have already expired. */
        goto _cleanup;
    }

    /* we found a prompt cache.  We take this to imply that the
       principal is valid. */
    kt->is_valid_principal = TRUE;

    /* check if there are any prompts currently showing.  If there are
       we check if they are the same as the ones we are going to show.
       In which case we just reuse the exisitng prompts */
    if (KHM_FAILED(khui_cw_get_prompt_count(kt->nc, &n_cur_prompts)) ||
        n_prompts != (khm_int32) n_cur_prompts)
        goto _show_new_prompts;

    for(i = 0; i < n_prompts; i++) {
        wchar_t wsname[8];
        wchar_t wprompt[KHUI_MAXCCH_PROMPT];
        khm_handle csp_p = NULL;
        khm_int32 p_type;
        khm_int32 p_flags;
        khui_new_creds_prompt * p;

        if (KHM_FAILED(khui_cw_get_prompt(kt->nc, i, &p)))
            break;

        StringCbPrintf(wsname, sizeof(wsname), L"%d", i);

        if (KHM_FAILED(khc_open_space(csp_prcache, wsname, 0, &csp_p)))
            break;

        cb = sizeof(wprompt);
        if (KHM_FAILED(khc_read_string(csp_p, L"Prompt", 
                                       wprompt, &cb))) {
            khc_close_space(csp_p);
            break;
        }

        if (KHM_FAILED(khc_read_int32(csp_p, L"Type", &p_type)))
            p_type = 0;

        if (KHM_FAILED(khc_read_int32(csp_p, L"Flags", &p_flags)))
            p_flags = 0;

        if (                    /* if we received a prompt string,
                                   then it should be the same as the
                                   one that is displayed */
            (wprompt[0] &&
             (p->prompt == NULL ||
              wcscmp(wprompt, p->prompt))) ||

                                /* if we didn't receive one, then
                                   there shouldn't be one displayed.
                                   This case really shouldn't happen
                                   in reality, but we check anyway. */
            (!wprompt[0] &&
             p->prompt != NULL) ||

                                /* the type should match */
            (p_type != p->type) ||

                                /* if this prompt should be hidden,
                                   then it must also be so */
            (p_flags != p->flags)
            ) {

            khc_close_space(csp_p);
            break;

        }
        

        khc_close_space(csp_p);
    }

    if (i == n_prompts) {
        /* We are already showing the right set of prompts. */
        rv = TRUE;
        goto _cleanup;
    }

 _show_new_prompts:

    khui_cw_clear_prompts(kt->nc);

    {
        wchar_t wbanner[KHUI_MAXCCH_BANNER];
        wchar_t wpname[KHUI_MAXCCH_PNAME];

        cb = sizeof(wbanner);
        if (KHM_FAILED(khc_read_string(csp_prcache, L"Banner", 
                                      wbanner, &cb)))
            wbanner[0] = 0;

        cb = sizeof(wpname);
        if (KHM_FAILED(khc_read_string(csp_prcache, L"Name",
                                       wpname, &cb)))
            wpname[0] = 0;

        khui_cw_begin_custom_prompts(kt->nc,
                                     n_prompts,
                                     (wbanner[0]? wbanner: NULL),
                                     (wpname[0]? wpname: NULL));
    }

    for(i = 0; i < n_prompts; i++) {
        wchar_t wsname[8];
        wchar_t wprompt[KHUI_MAXCCH_PROMPT];
        khm_handle csp_p = NULL;
        khm_int32 p_type;
        khm_int32 p_flags;

        StringCbPrintf(wsname, sizeof(wsname), L"%d", i);

        if (KHM_FAILED(khc_open_space(csp_prcache, wsname, 0, &csp_p)))
            break;

        cb = sizeof(wprompt);
        if (KHM_FAILED(khc_read_string(csp_p, L"Prompt", 
                                       wprompt, &cb))) {
            khc_close_space(csp_p);
            break;
        }

        if (KHM_FAILED(khc_read_int32(csp_p, L"Type", &p_type)))
            p_type = 0;

        if (KHM_FAILED(khc_read_int32(csp_p, L"Flags", &p_flags)))
            p_flags = 0;

        khui_cw_add_prompt(kt->nc, p_type, wprompt, NULL, p_flags);

        khc_close_space(csp_p);
    }

    if (i < n_prompts) {
        khui_cw_clear_prompts(kt->nc);
    } else {
        rv = TRUE;
    }
     
 _cleanup:

    if (csp_prcache)
        khc_close_space(csp_prcache);

    if (csp_k5config)
        khc_close_space(csp_k5config);

    if (csp_idconfig)
        khc_close_space(csp_idconfig);

    return rv;
}

static krb5_error_code KRB5_CALLCONV 
kinit_prompter(krb5_context context,
               void *data,
               const char *name,
               const char *banner,
               int num_prompts,
               krb5_prompt prompts[])
{
    int i;
    k5_kinit_task * kt;
    krb5_prompt_type * ptypes = NULL;
    khm_size ncp;
    krb5_error_code code = 0;
    BOOL new_prompts = TRUE;
    khm_handle csp_prcache = NULL;

    kt = (k5_kinit_task *) data;
    assert(kt && kt->magic == K5_KINIT_TASK_MAGIC);

    EnterCriticalSection(&kt->cs);

    if (kt->state == K5_KINIT_STATE_ABORTED) {
        LeaveCriticalSection(&kt->cs);
        return KRB5_LIBOS_PWDINTR;
    }

#ifdef DEBUG
    assert(kt->state == K5_KINIT_STATE_INCALL ||
           kt->state == K5_KINIT_STATE_CONFIRM);

    _reportf(L"k5_kinit_prompter() received %d prompts with name=[%S] banner=[%S]",
             num_prompts,
             name, banner);
    for (i=0; i < num_prompts; i++) {
        _reportf(L"Prompt[%d]: string[%S]", i, prompts[i].prompt);
    }
#endif

    /* we got prompts?  Then we assume that the principal is valid */

    if (!kt->is_valid_principal) {
        kt->is_valid_principal = TRUE;

        /* if the flags that were used to call kinit were restricted
           because we didn't know the validity of the principal, then
           we need to go back and retry the call with the correct
           flags. */
        if (kt->params.forwardable ||
            kt->params.proxiable ||
            kt->params.renewable) {

            _reportf(L"Retrying kinit call due to restricted flags on first call.");
            kt->state = K5_KINIT_STATE_RETRY;
            LeaveCriticalSection(&kt->cs);

            return KRB5_LIBOS_PWDINTR;
        }
    }

    if(pkrb5_get_prompt_types)
        ptypes = pkrb5_get_prompt_types(context);

    /* check if we are already showing the right prompts */
    khui_cw_get_prompt_count(kt->nc, &ncp);

    if (num_prompts != (int) ncp && num_prompts != 0)
        goto _show_new_prompts;

    for (i=0; i < num_prompts; i++) {
        wchar_t wprompt[KHUI_MAXCCH_PROMPT];
        khui_new_creds_prompt * p;

        if(prompts[i].prompt) {
            AnsiStrToUnicode(wprompt, sizeof(wprompt), 
                             prompts[i].prompt);
        } else {
            wprompt[0] = L'\0';
        }

        if (KHM_FAILED(khui_cw_get_prompt(kt->nc, i, &p)))
            break;

        if (                    /* if we received a prompt string,
                                   then it should be the same as the
                                   one that is displayed */
            (wprompt[0] != L'\0' &&
             (p->prompt == NULL ||
              wcscmp(wprompt, p->prompt))) ||

                                /* if we didn't receive one, then
                                   there shouldn't be one displayed.
                                   This case really shouldn't happen
                                   in reality, but we check anyway. */
            (wprompt[0] == L'\0' &&
             p->prompt != NULL) ||

                                /* the type should match */
            (ptypes &&
             ptypes[i] != p->type) ||
            (!ptypes &&
             p->type != 0) ||

                                /* if this prompt should be hidden,
                                   then it must also be so */
            (prompts[i].hidden &&
             !(p->flags & KHUI_NCPROMPT_FLAG_HIDDEN)) ||
            (!prompts[i].hidden &&
             (p->flags & KHUI_NCPROMPT_FLAG_HIDDEN))
            )

            break;
    }

    if (i >= num_prompts) {

        new_prompts = FALSE;

        /* ok. looks like we are already showing the same set of
           prompts that we were supposed to show.  Sync up the values
           and go ahead. */
        goto _process_prompts;
    }

 _show_new_prompts:
    if (num_prompts == 0) {

        assert(FALSE);

        khui_cw_notify_identity_state(kt->nc,
                                      kt->nct->hwnd_panel,
                                      NULL,
                                      KHUI_CWNIS_READY |
                                      KHUI_CWNIS_NOPROGRESS |
                                      KHUI_CWNIS_VALIDATED, 0);

        code = 0;
        kt->is_null_password = TRUE;
        goto _process_prompts;
    }

    /* in addition to showing new prompts, we also cache the first set
       of prompts. */
    if (kt->prompt_set_index == 0) {
        khm_handle csp_idconfig = NULL;
        khm_handle csp_idk5 = NULL;

        kcdb_identity_get_config(kt->identity,
                                 KHM_FLAG_CREATE,
                                 &csp_idconfig);

        if (csp_idconfig != NULL)
            khc_open_space(csp_idconfig,
                           CSNAME_KRB5CRED,
                           KHM_FLAG_CREATE,
                           &csp_idk5);

        if (csp_idk5 != NULL)
            khc_open_space(csp_idk5,
                           CSNAME_PROMPTCACHE,
                           KHM_FLAG_CREATE,
                           &csp_prcache);

        khc_close_space(csp_idconfig);
        khc_close_space(csp_idk5);
    }

    {
        wchar_t wbanner[KHUI_MAXCCH_BANNER];
        wchar_t wname[KHUI_MAXCCH_PNAME];

        if(banner)
            AnsiStrToUnicode(wbanner, sizeof(wbanner), banner);
        else
            wbanner[0] = L'\0';

        if(name)
            AnsiStrToUnicode(wname, sizeof(wname), name);
        else
            wname[0] = L'\0';

        khui_cw_clear_prompts(kt->nc);

        khui_cw_begin_custom_prompts(kt->nc, 
                                     num_prompts,
                                     (banner)? wbanner : NULL,
                                     (name)? wname : NULL);

        if (csp_prcache) {
            FILETIME current;
            FILETIME lifetime;
            FILETIME expiry;
            khm_int64 iexpiry;
            khm_int32 t = 0;

            khc_write_string(csp_prcache, L"Banner", wbanner);
            khc_write_string(csp_prcache, L"Name", wname);
            khc_write_int32(csp_prcache, L"PromptCount", (khm_int32) num_prompts);

            GetSystemTimeAsFileTime(&current);
#ifdef USE_PROMPT_CACHE_LIFETIME
            khc_read_int32(csp_params, L"PromptCacheLifetime", &t);
            if (t == 0)
                t = 172800;         /* 48 hours */
#else
            khc_read_int32(csp_params, L"MaxRenewLifetime", &t);
            if (t == 0)
                t = 2592000;    /* 30 days */
            t += 604800;        /* + 7 days */
#endif
            TimetToFileTimeInterval(t, &lifetime);
            expiry = FtAdd(&current, &lifetime);
            iexpiry = FtToInt(&expiry);

            khc_write_int64(csp_prcache, L"ExpiresOn", iexpiry);
        }
    }

    for(i=0; i < num_prompts; i++) {
        wchar_t wprompt[KHUI_MAXCCH_PROMPT];

        if(prompts[i].prompt) {
            AnsiStrToUnicode(wprompt, sizeof(wprompt), 
                             prompts[i].prompt);
        } else {
            wprompt[0] = 0;
        }

        khui_cw_add_prompt(kt->nc, (ptypes?ptypes[i]:0),
                           wprompt, NULL,
                           (prompts[i].hidden?KHUI_NCPROMPT_FLAG_HIDDEN:0));

        if (csp_prcache) {
            khm_handle csp_p = NULL;
            wchar_t wnum[8];    /* should be enough for 10
                                   million prompts */

            wnum[0] = 0;
            StringCbPrintf(wnum, sizeof(wnum), L"%d", i);

            khc_open_space(csp_prcache, wnum, KHM_FLAG_CREATE, &csp_p);

            if (csp_p) {
                khc_write_string(csp_p, L"Prompt", wprompt);
                khc_write_int32(csp_p, L"Type", (ptypes?ptypes[i]:0));
                khc_write_int32(csp_p, L"Flags",
                                (prompts[i].hidden?
                                 KHUI_NCPROMPT_FLAG_HIDDEN:0));

                khc_close_space(csp_p);
            }
        }
    }

    if (csp_prcache) {
        khc_close_space(csp_prcache);
        csp_prcache = NULL;
    }

 _process_prompts:
    if (new_prompts) {
        kt->state = K5_KINIT_STATE_WAIT;

        kcdb_identity_set_flags(kt->identity,
                                KCDB_IDENT_FLAG_VALID | KCDB_IDENT_FLAG_KEY_EXPORT,
                                KCDB_IDENT_FLAG_VALID | KCDB_IDENT_FLAG_KEY_EXPORT);
        khui_cw_notify_identity_state(kt->nc, kt->nct->hwnd_panel, L"",
                                      KHUI_CWNIS_VALIDATED |
                                      KHUI_CWNIS_READY, 0);

        SetEvent(kt->h_parent_wait);
        LeaveCriticalSection(&kt->cs);
        WaitForSingleObject(kt->h_task_wait, INFINITE);
        EnterCriticalSection(&kt->cs);
    }

    /* we get here after the user selects an action that either
       cancels the credentials acquisition operation or triggers the
       actual acquisition of credentials. */
    if (kt->state != K5_KINIT_STATE_INCALL &&
        kt->state != K5_KINIT_STATE_CONFIRM) {
        code = KRB5_LIBOS_PWDINTR;
        goto _exit;
    }

    kt->is_null_password = FALSE;

    /* otherwise, we need to get the data back from the UI and return
       0 */

    khui_cw_sync_prompt_values(kt->nc);

    for(i=0; i<num_prompts; i++) {
        krb5_data * d;
        wchar_t wbuf[512];
        khm_size cbbuf;
        size_t cch;

        d = prompts[i].reply;

        cbbuf = sizeof(wbuf);
        if(KHM_SUCCEEDED(khui_cw_get_prompt_value(kt->nc, i, wbuf, &cbbuf))) {
            UnicodeStrToAnsi(d->data, d->length, wbuf);
            if(SUCCEEDED(StringCchLengthA(d->data, d->length, &cch)))
                d->length = (unsigned int) cch;
            else
                d->length = 0;
        } else {
            assert(FALSE);
            d->length = 0;
        }

        if (ptypes && 
            ptypes[i] == KRB5_PROMPT_TYPE_PASSWORD &&
            d->length == 0)

            kt->is_null_password = TRUE;
    }

    if (khui_cw_get_persist_private_data(kt->nc) &&
        num_prompts == 1 &&
        ptypes &&
        ptypes[0] == KRB5_PROMPT_TYPE_PASSWORD &&
        prompts[0].reply->length != 0) {
        k5_reply_to_acqpriv_id_request(kt->nc, prompts[0].reply);
    }

 _exit:

    kt->prompt_set_index++;

    LeaveCriticalSection(&kt->cs);

    /* entering a NULL password is equivalent to cancelling out */
    if (kt->is_null_password)
        return KRB5_LIBOS_PWDINTR;
    else
        return code;
}



/* Executed inside a task object */
static khm_int32 __stdcall
kinit_task_proc(void * vparam)
{
    k5_kinit_task * kt;
    long l;

    kt = (k5_kinit_task *) vparam;
    if (kt == NULL || kt->magic != K5_KINIT_TASK_MAGIC) {
        assert(FALSE);
        return KHM_ERROR_INVALID_PARAM;
    }

    EnterCriticalSection(&kt->cs);

    if (kt->state == K5_KINIT_STATE_ABORTED) {
        LeaveCriticalSection(&kt->cs);
        return KHM_ERROR_SUCCESS;
    }

    if (kt->state != K5_KINIT_STATE_PREP) {
        LeaveCriticalSection(&kt->cs);
        assert(FALSE);
        return KHM_ERROR_INVALID_OPERATION;
    }

    kt->prompt_set_index = 0;

    if (cached_kinit_prompter(kt)) {

        kt->state = K5_KINIT_STATE_WAIT;

        kcdb_identity_set_flags(kt->identity,
                                KCDB_IDENT_FLAG_VALID | KCDB_IDENT_FLAG_KEY_EXPORT,
                                KCDB_IDENT_FLAG_VALID | KCDB_IDENT_FLAG_KEY_EXPORT);
        khui_cw_notify_identity_state(kt->nc, kt->nct->hwnd_panel, L"",
                                      KHUI_CWNIS_VALIDATED |
                                      KHUI_CWNIS_READY, 0);

        SetEvent(kt->h_parent_wait);
        LeaveCriticalSection(&kt->cs);
        WaitForSingleObject(kt->h_task_wait, INFINITE);
        EnterCriticalSection(&kt->cs);

        assert(kt->state == K5_KINIT_STATE_CONFIRM ||
               kt->state == K5_KINIT_STATE_ABORTED);

        if (kt->state != K5_KINIT_STATE_CONFIRM)
            goto done;

        if (!cp_check_continue(kt)) {
            kt->kinit_code = KRB5KRB_AP_ERR_BAD_INTEGRITY;
            goto done;
        }
    } else {
        kt->state = K5_KINIT_STATE_INCALL;
    }

 call_kinit:

#ifdef DEBUG
    _reportf(L"kinit task state prior to calling khm_krb5_kinit() :");
    _reportf(L"  kt->principal        = [%S]", kt->principal);
    _reportf(L"  kt->kinit_code       = %d", kt->kinit_code);
    _reportf(L"  kt->state            = %d", kt->state);
    _reportf(L"  kt->prompt_set_index = %d", kt->prompt_set_index);
    _reportf(L"  kt->is_valid_principal = %d", (int) kt->is_valid_principal);
    _reportf(L"  kt->ccache           = [%s]", kt->ccache);
#endif

    LeaveCriticalSection(&kt->cs);

    l = khm_krb5_kinit
        (0,
         kt->principal,
         kt->password,
         kt->ccache,
         kt->params.lifetime,
         (kt->is_valid_principal) ? kt->params.forwardable : 0,
         (kt->is_valid_principal) ? kt->params.proxiable : 0,
         (kt->is_valid_principal && kt->params.renewable) ? kt->params.renew_life : 0,
         kt->params.addressless,
         kt->params.publicIP,
         kinit_prompter,
         kt);

    EnterCriticalSection(&kt->cs);

    kt->kinit_code = l;

    _reportf(L"  kinit return code = %d", (int) l);

    if (kt->state == K5_KINIT_STATE_RETRY) {
        /* If the principal was found to be valid, and if we
           restricted the options that were being passed to kinit,
           then we need to retry the kinit call.  This time we use the
           real options. */

        assert(kt->is_valid_principal);
        kt->state = K5_KINIT_STATE_INCALL;
        goto call_kinit;
    }

    assert(kt->state == K5_KINIT_STATE_INCALL ||
           kt->state == K5_KINIT_STATE_CONFIRM ||
           kt->state == K5_KINIT_STATE_ABORTED);

    if (kt->state == K5_KINIT_STATE_ABORTED) {
        goto done;
    }

    if (kt->state == K5_KINIT_STATE_CONFIRM) {
        goto done;
    }

    /* The call completed without any prompting.  We haven't had a
       chance to update the identity state. */

    if (kt->state == K5_KINIT_STATE_INCALL) {
        wchar_t msg[KHUI_MAXCCH_BANNER];

        switch (kt->kinit_code) {
        case 0:
            kcdb_identity_set_flags(kt->identity,
                                    KCDB_IDENT_FLAG_VALID | KCDB_IDENT_FLAG_KEY_EXPORT,
                                    KCDB_IDENT_FLAG_VALID | KCDB_IDENT_FLAG_KEY_EXPORT);
            khui_cw_notify_identity_state(kt->nc, kt->nct->hwnd_panel, L"",
                                          KHUI_CWNIS_READY |
                                          KHUI_CWNIS_VALIDATED, 0);
            khui_cw_clear_prompts(kt->nc);
            break;

        case KRB5KDC_ERR_KEY_EXP:
            kcdb_identity_set_flags(kt->identity,
                                    KCDB_IDENT_FLAG_VALID | KCDB_IDENT_FLAG_KEY_EXPORT,
                                    KCDB_IDENT_FLAG_VALID | KCDB_IDENT_FLAG_KEY_EXPORT);
            k5_force_password_change(kt->dlg_data);
            LoadString(hResModule, IDS_K5ERR_KEY_EXPIRED, msg, ARRAYLENGTH(msg));
            khui_cw_notify_identity_state(kt->nc, kt->nct->hwnd_panel, msg,
                                          KHUI_CWNIS_READY |
                                          KHUI_CWNIS_VALIDATED, 0);
            break;

        case KRB5KDC_ERR_C_PRINCIPAL_UNKNOWN:
            kcdb_identity_set_flags(kt->identity,
                                    KCDB_IDENT_FLAG_INVALID,
                                    KCDB_IDENT_FLAG_INVALID);
            khui_cw_clear_prompts(kt->nc);
            khm_err_describe(kt->kinit_code, msg, sizeof(msg), NULL, NULL);
            khui_cw_notify_identity_state(kt->nc, kt->nct->hwnd_panel,
                                          msg, KHUI_CWNIS_VALIDATED, 0);
            break;

        default:
            kcdb_identity_set_flags(kt->identity,
                                    KCDB_IDENT_FLAG_UNKNOWN,
                                    KCDB_IDENT_FLAG_UNKNOWN);
            khui_cw_clear_prompts(kt->nc);
            khm_err_describe(kt->kinit_code, msg, sizeof(msg), NULL, NULL);
            khui_cw_notify_identity_state(kt->nc, kt->nct->hwnd_panel,
                                          msg, KHUI_CWNIS_VALIDATED, 0);
        }
    }

 done:

    kt->state = K5_KINIT_STATE_DONE;

    LeaveCriticalSection(&kt->cs);

    SetEvent(kt->h_parent_wait);

    k5_kinit_task_release(kt);

    return KHM_ERROR_SUCCESS;
}
