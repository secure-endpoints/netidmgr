/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
 * Copyright (c) 2006-2009 Secure Endpoints Inc.
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

khui_timer_event * khui_timers = NULL;
khm_size khui_n_timers = 0;
khm_size khui_nc_timers = 0;

CRITICAL_SECTION cs_timers;

/*********************************************************************
  Timers
 *********************************************************************/


static const wchar_t *
tmr_type_to_string(khui_timer_type t)
{
    switch (t) {
    case KHUI_TTYPE_ID_MARK:
        return L"identity marker";

    case KHUI_TTYPE_CRED_WARN:
        return L"credential warning";

    case KHUI_TTYPE_ID_WARN:
        return L"identity warning";

    case KHUI_TTYPE_CRED_CRIT:
        return L"credential critical";

    case KHUI_TTYPE_ID_CRIT:
        return L"identity critical";

    case KHUI_TTYPE_CRED_EXP:
        return L"credential expiration";

    case KHUI_TTYPE_ID_EXP:
        return L"identity expiration";

    case KHUI_TTYPE_CRED_RENEW:
        return L"credential renewal";

    case KHUI_TTYPE_ID_RENEW:
        return L"identity renewal";

    default:
        return L"(unknown)";
    }
}

#define KHUI_TIMER_ALLOC_INCR 16

void 
khm_timer_init(void) {
    assert(khui_timers == NULL);

    khui_nc_timers = KHUI_TIMER_ALLOC_INCR;
    khui_n_timers = 0;
    khui_timers = PMALLOC(sizeof(*khui_timers) * khui_nc_timers);

    assert(khui_timers != NULL);

    InitializeCriticalSection(&cs_timers);
}

void
khm_timer_exit(void) {
    khm_size i;

    EnterCriticalSection(&cs_timers);

    for (i=0; i < khui_n_timers; i++) {
        kcdb_buf_release(khui_timers[i].key);
    }

    if (khui_timers)
        PFREE(khui_timers);
    khui_timers = NULL;
    khui_n_timers = 0;
    khui_nc_timers = 0;

    LeaveCriticalSection(&cs_timers);
    DeleteCriticalSection(&cs_timers);
}

static khm_int64
tmr_get_current_time(void)
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    return FtToInt(&ft);
}

/* called with cs_timers held */
static khm_boolean
tmr_fire_timer(khm_int64 * pnext_event) {
    int i;
    khm_int64 curtime = tmr_get_current_time();
    khm_int64 next_event = 0;
    khm_boolean found = FALSE;

    _begin_task(0);
    _report_cs0(KHERR_DEBUG_1, L"Checking for expired timers");
    _describe();

    for (i=0; i < (int) khui_n_timers; i++) {
        khm_boolean is_ident = FALSE;

        if ((khui_timers[i].flags & 
             (KHUI_TE_FLAG_STALE | KHUI_TE_FLAG_EXPIRED)) ||
            khui_timers[i].type == KHUI_TTYPE_ID_MARK)
            continue;

        if (khui_timers[i].event < curtime + SECONDS_TO_FT(TT_TIMEEQ_ERROR_SMALL)) {

            found = TRUE;

            _report_cs3(KHERR_DEBUG_1, L"Expiring timer index=%1!d!, type=%2!s!, key=%3!p!",
                        _int32(i), _cstr(tmr_type_to_string(khui_timers[i].type)),
                        _cptr(khui_timers[i].key));

            switch (khui_timers[i].type) {
            case KHUI_TTYPE_ID_RENEW:
                _report_cs1(KHERR_DEBUG_1, L"Renewing identity %1!p!", _cptr(khui_timers[i].key));
                khm_cred_renew_identity(khui_timers[i].key);
                khui_timers[i].flags |= KHUI_TE_FLAG_EXPIRED;
                break;

            case KHUI_TTYPE_CRED_RENEW:
                /* the equivalence threshold for setting the timer is
                   a lot larger than what we are testing for here
                   (TT_TIMEEQ_ERROR vs TT_TIMEEQ_ERROR_SMALL) so
                   we assume that it is safe to trigger a renew_cred
                   call here without checking if there's an imminent
                   renew_identity call. */
                _report_cs1(KHERR_DEBUG_1, L"Renewing credential %1!p!", _cptr(khui_timers[i].key));
                khm_cred_renew_cred(khui_timers[i].key);
                khui_timers[i].flags |= KHUI_TE_FLAG_EXPIRED;
                break;

            case KHUI_TTYPE_ID_WARN:
            case KHUI_TTYPE_ID_CRIT:
            case KHUI_TTYPE_ID_EXP:
                is_ident = TRUE;
                /* fallthrough */

            case KHUI_TTYPE_CRED_WARN:
            case KHUI_TTYPE_CRED_CRIT:
            case KHUI_TTYPE_CRED_EXP:

                {
                    wchar_t buffer[256];
                    wchar_t bufname[KCDB_MAXCCH_SHORT_DESC];
                    wchar_t format[64];
                    wchar_t timestamp[128];
                    khm_size cb;
                    FILETIME ft;
                    khm_int32 cmd;
                    khm_boolean expired;

                    khui_alert * alert = NULL;

                    expired = (khui_timers[i].type == KHUI_TTYPE_ID_EXP ||
                               khui_timers[i].type == KHUI_TTYPE_CRED_EXP ||
                               curtime + FT_EXPIRED_THRESHOLD > khui_timers[i].key_expire);

                    khui_alert_create_empty(&alert);

                    LoadString(khm_hInstance,
                               (expired ? IDS_WARN_EXP_TITLE : IDS_WARN_TITLE),
                               buffer, ARRAYLENGTH(buffer));
                    khui_alert_set_title(alert, buffer);

                    LoadString(khm_hInstance,
                               (expired ? IDS_WARN_EXPD_CRED : IDS_WARN_EXP_CRED),
                               format, ARRAYLENGTH(format));
                    cb = sizeof(bufname);
                    kcdb_get_resource(khui_timers[i].key, KCDB_RES_DISPLAYNAME,
                                      0, NULL, NULL, bufname, &cb);
                    ft = IntToFt(khui_timers[i].key_expire);
                    cb = sizeof(timestamp);
                    FtToStringEx(&ft, FTSE_RELATIVE, NULL, timestamp, &cb);
                    FormatString(buffer, sizeof(buffer), format, bufname, timestamp);
                    khui_alert_set_message(alert, buffer);

                    if (is_ident &&
                        (cmd = khm_get_identity_new_creds_action(khui_timers[i].key)) > 0) {
                        khui_alert_add_command(alert, cmd);
                        khui_alert_add_command(alert, KHUI_PACTION_CLOSE);
                    }

                    khui_alert_set_severity(alert, KHERR_WARNING);
                    khui_alert_set_flags(alert,
                                         KHUI_ALERT_FLAG_REQUEST_BALLOON | KHUI_ALERT_FLAG_DISPATCH_CMD,
                                         KHUI_ALERT_FLAG_REQUEST_BALLOON | KHUI_ALERT_FLAG_DISPATCH_CMD);
                    khui_alert_set_type(alert, KHUI_ALERTTYPE_EXPIRE);
                    khui_alert_show(alert);
                    khui_alert_release(alert);

                    khui_timers[i].flags |= KHUI_TE_FLAG_EXPIRED;
                }
                break;

            default:
                assert(FALSE);
                khui_timers[i].flags |= KHUI_TE_FLAG_EXPIRED;
            }
        } else {
            if (next_event == 0 || next_event > khui_timers[i].event)
                next_event = khui_timers[i].event;
        }
    }

    _end_task();

    if (pnext_event)
        *pnext_event = next_event;

    return found;
}

void
khm_timer_fire(HWND hwnd) {
    EnterCriticalSection(&cs_timers);
    tmr_fire_timer(NULL);
    LeaveCriticalSection(&cs_timers);

    khm_timer_refresh(hwnd);
}

/* called with cs_timers held */
static khui_timer_event *
tmr_get_event(khm_handle key, khui_timer_type type) {
    int i;

    for (i=0; i < (int) khui_n_timers; i++) {
        if (khui_timers[i].key == key &&
            khui_timers[i].type == type)
            return &khui_timers[i];
    }

    return NULL;
}

static khui_timer_event *
tmr_update_event(khm_handle key, khui_timer_type type, khm_int64 event,
                 khm_int64 key_expire) {
    khui_timer_event * tmr;
    wchar_t name[KCDB_MAXCCH_NAME];
    wchar_t tstamp[128] = L"(unspecified)";
    FILETIME ft;
    khm_size cb;

    if (event) {
        ft = IntToFt(event);
        cb = sizeof(tstamp);
        FtToStringEx(&ft, 0, NULL, tstamp, &cb);
    }

    cb = sizeof(name); name[0] = L'\0';
    kcdb_buf_get_attr(key, KCDB_ATTR_NAME, NULL, name, &cb);

    _reportf(L"Updating %s timer for [%s].  Expires at %s", tmr_type_to_string(type),
             name, tstamp);

    tmr = tmr_get_event(key, type);

    if (!tmr) {
        if (khui_n_timers >= (int) khui_nc_timers) {
            khui_nc_timers = UBOUNDSS(khui_n_timers+1, KHUI_TIMER_ALLOC_INCR,
                                      KHUI_TIMER_ALLOC_INCR);
            khui_timers = PREALLOC(khui_timers, sizeof(khui_timers[0]) * khui_nc_timers);
            assert(khui_timers);
        }

        tmr = &khui_timers[khui_n_timers++];

        ZeroMemory(tmr, sizeof(*tmr));
        tmr->key = key;
        kcdb_buf_hold(tmr->key);
        tmr->type = type;
        tmr->flags = ((type == KHUI_TTYPE_ID_MARK)? KHUI_TE_FLAG_STALE: 0);
    }

    if (type == KHUI_TTYPE_ID_MARK) {
        /* we don't update the STALE flag here.  Instead we leave it
           in so that the caller can detect whether this is the first
           time it is seeing this identity. */
    } else {
        if (tmr->event != event &&
            event + SECONDS_TO_FT(TT_TIMEEQ_ERROR) > tmr_get_current_time()) {
            tmr->flags &= ~KHUI_TE_FLAG_EXPIRED;
        }

        tmr->key_expire = key_expire;
        tmr->event = event;
        tmr->flags &= ~KHUI_TE_FLAG_STALE;
    }

    return tmr;
}

/* called with cs_timers held. */
static khm_int64
tmr_next_halflife_timeout(khm_int64 last_halftime, khm_int64 issue, khm_int64 expire)
{
    khm_int64 ret = 0;
    khm_int64 life;
    khm_int64 current = tmr_get_current_time();

    if (issue == 0 || expire == 0 ||
        issue >= expire || current >= expire)
        return 0;

    life = expire - issue;

    while(life / 2 > FT_MIN_HALFLIFE_INTERVAL) {
        life /= 2;
        ret = expire - life;

        if (ret > current) {
            if (last_halftime == ret) {
                continue;
            } else {
                return ret;
            }
        }
    }

    return 0;
}

khm_int32
khm_get_identity_timer_info(khm_handle identity, khui_timer_info * pinfo, khm_int64 last_halftime)
{
    khm_int32 monitor     = TRUE;
    khm_int32 do_warning  = TRUE;
    khm_int32 do_critical = TRUE;
    khm_int32 do_renew    = TRUE;
    khm_int32 do_halflife = TRUE;
    khm_int32 to_warning  = KHUI_DEF_TIMEOUT_WARN;
    khm_int32 to_critical = KHUI_DEF_TIMEOUT_CRIT;
    khm_int32 to_renew    = KHUI_DEF_TIMEOUT_RENEW;

    FILETIME  id_expire   = IntToFt(0);
    FILETIME  id_issue    = IntToFt(0);

    khm_int32 rv = KHM_ERROR_SUCCESS;

    khm_handle csp_id = NULL;
    khm_size cb;

    memset(pinfo, 0, sizeof(*pinfo));

    cb = sizeof(id_expire);
    kcdb_identity_get_attr(identity, KCDB_ATTR_EXPIRE, NULL, &id_expire, &cb);
    if (FtIsZero(&id_expire))
        goto done;
    cb = sizeof(id_issue);
    kcdb_identity_get_attr(identity, KCDB_ATTR_ISSUE, NULL, &id_issue, &cb);

    pinfo->expire = FtToInt(&id_expire);
    pinfo->issue = FtToInt(&id_issue);

    kcdb_identity_get_config(identity, KHM_PERM_READ | KCONF_FLAG_SHADOW, &csp_id);

    khc_read_int32(csp_id, L"Monitor", &monitor);
    khc_read_int32(csp_id, L"AllowWarn", &do_warning);
    khc_read_int32(csp_id, L"AllowCritical", &do_critical);
    khc_read_int32(csp_id, L"AllowAutoRenew", &do_renew);
    khc_read_int32(csp_id, L"RenewAtHalfLife", &do_halflife);
    khc_read_int32(csp_id, L"WarnThreshold", &to_warning);
    khc_read_int32(csp_id, L"CriticalThreshold", &to_critical);
    khc_read_int32(csp_id, L"AutoRenewThreshold", &to_renew);

    if (!monitor) goto done;

    if (do_renew) {
        if (do_halflife && !FtIsZero(&id_issue)) {
            pinfo->renewal = tmr_next_halflife_timeout(last_halftime,
                                                       pinfo->issue, pinfo->expire);
            pinfo->use_halflife = TRUE;
        }
        if (pinfo->renewal == 0)
            pinfo->renewal = pinfo->expire - SECONDS_TO_FT(to_renew);
    }

    if (do_warning)
        pinfo->warning = pinfo->expire - SECONDS_TO_FT(to_warning);

    if (do_critical)
        pinfo->critical = pinfo->expire - SECONDS_TO_FT(to_critical);

 done:
    if (csp_id) khc_close_space(csp_id);

    return rv;
}

static khui_timer_event *
tmr_update_identity_timers(khui_timer_event * e_marker)
{
    khui_timer_info ti = e_marker->marker;
    khm_handle identity = e_marker->key;
    khm_int64 current = tmr_get_current_time();

    khui_timer_event * e;

    _reportf(L"Updating identity timers ...");

    if (ti.renewal != 0 && ti.use_halflife && ti.issue != 0 && ti.expire != 0) {
        e = tmr_get_event(identity, KHUI_TTYPE_ID_RENEW);
        ti.renewal = tmr_next_halflife_timeout
            (((e && (e->flags & KHUI_TE_FLAG_EXPIRED))? e->event: 0),
             ti.issue, ti.expire);
    }

    if (ti.renewal != 0) {
        tmr_update_event(identity, KHUI_TTYPE_ID_RENEW,
                         ti.renewal, ti.expire);
    } else {
        if (ti.warning + SECONDS_TO_FT(TT_TIMEEQ_ERROR_SMALL) > current)
            tmr_update_event(identity, KHUI_TTYPE_ID_WARN,
                             ti.warning, ti.expire);

        if (ti.critical + SECONDS_TO_FT(TT_TIMEEQ_ERROR_SMALL) > current)
            tmr_update_event(identity, KHUI_TTYPE_ID_CRIT,
                             ti.critical, ti.expire);

        if (ti.expire + SECONDS_TO_FT(TT_TIMEEQ_ERROR_SMALL) > current)
            tmr_update_event(identity, KHUI_TTYPE_ID_CRIT,
                             ti.expire, ti.expire);
    }

    e_marker = tmr_get_event(identity, KHUI_TTYPE_ID_MARK);
    e_marker->marker = ti;
    return e_marker;
}

/* called with cs_timers held.  Called once for each credential in the
   root credentials set. */
static khm_int32 KHMAPI
tmr_cred_apply_proc(khm_handle credential, void * rock) {
    khm_handle identity = NULL;
    khui_timer_event * e_mark;
    khui_timer_event * e;
    khui_timer_info ti;
    khm_int32 cred_flags;
    khm_int64 cred_expire;
    khm_int64 cred_issue = 0;

    {
        khm_size cb;
        wchar_t wname[KCDB_MAXCCH_NAME] = L"";

        cb = sizeof(wname);
        kcdb_cred_get_name(credential, wname, &cb);
        _reportf(L"Looking at cred [%s]", wname);
    }

    kcdb_cred_get_identity(credential, &identity);

    e_mark = tmr_update_event(identity, KHUI_TTYPE_ID_MARK, 0, 0);
    if (e_mark->flags & KHUI_TE_FLAG_STALE) {
        e_mark->flags &= ~KHUI_TE_FLAG_STALE;
        khm_get_identity_timer_info(identity, &e_mark->marker, 0);
        e_mark = tmr_update_identity_timers(e_mark);
    }
    ti = e_mark->marker;

    kcdb_cred_get_flags(credential, &cred_flags);
    if (!(cred_flags & KCDB_CRED_FLAG_RENEWABLE)) {
        _reportf(L"Credential is not renewable. Skipping");
        goto done;
    }

    {
        FILETIME ft; khm_size cb;
        cb = sizeof(ft);
        if (KHM_FAILED(kcdb_cred_get_attr(credential, KCDB_ATTR_EXPIRE,
                                          NULL, &ft, &cb)) || FtIsZero(&ft)) {
            _reportf(L"Can't look up KCDB_ATTR_EXPIRE. Skipping");
            goto done;
        }
        cred_expire = FtToInt(&ft);
        if (KHM_SUCCEEDED(kcdb_cred_get_attr(credential, KCDB_ATTR_ISSUE, NULL, &ft, &cb)))
            cred_issue = FtToInt(&ft);
    }

    if (cred_expire + SECONDS_TO_FT(TT_TIMEEQ_ERROR) > ti.expire) {
        _reportf(L"Skipping credential.  Credential expiration is too close to the identity expiration");
        goto done;
    }

    if (ti.renewal != 0) {
        khm_int64 current = tmr_get_current_time();

        if (ti.use_halflife && cred_issue != 0 && cred_expire != 0) {

            e = tmr_get_event(credential, KHUI_TTYPE_CRED_RENEW);
            ti.renewal = tmr_next_halflife_timeout
                (((e && (e->flags & KHUI_TE_FLAG_EXPIRED))? e->event: 0),
                 cred_issue, cred_expire);

        } else if (!ti.use_halflife &&
                   ((cred_expire - (ti.expire - ti.renewal))
                    + SECONDS_TO_FT(TT_TIMEEQ_ERROR_SMALL)) > current) {
            ti.renewal = cred_expire - (ti.expire - ti.renewal);
        } else if (ti.use_halflife &&
                   (cred_expire - SECONDS_TO_FT(KHUI_DEF_TIMEOUT_RENEW)) > current) {
            ti.renewal = cred_expire - SECONDS_TO_FT(KHUI_DEF_TIMEOUT_RENEW);
        } else {
            ti.renewal = 0;
        }
    }

    if (ti.renewal != 0) {
        tmr_update_event(credential, KHUI_TTYPE_CRED_RENEW,
                         ti.renewal, cred_expire);
    } else {
        khm_int64 current = tmr_get_current_time();

        if ((cred_expire - (ti.expire - ti.warning)) + SECONDS_TO_FT(TT_TIMEEQ_ERROR_SMALL) > current)
            tmr_update_event(identity, KHUI_TTYPE_ID_WARN,
                             ti.warning, ti.expire);

        if (ti.critical + SECONDS_TO_FT(TT_TIMEEQ_ERROR_SMALL) > current)
            tmr_update_event(identity, KHUI_TTYPE_ID_CRIT,
                             ti.critical, ti.expire);

        if (ti.expire + SECONDS_TO_FT(TT_TIMEEQ_ERROR_SMALL) > current)
            tmr_update_event(identity, KHUI_TTYPE_ID_CRIT,
                             ti.expire, ti.expire);
    }

 done:
    if (identity)
        kcdb_identity_release(identity);

    return KHM_ERROR_SUCCESS;
}

/* called with cs_timers held */
static void
tmr_purge(void) {
    int i, j;

    for (i=0,j=0; i < (int) khui_n_timers; i++) {
        if (khui_timers[i].flags & KHUI_TE_FLAG_STALE) {
            kcdb_buf_release(khui_timers[i].key);
        } else {
            if (i != j)
                khui_timers[j] = khui_timers[i];
            j++;
        }
    }

    khui_n_timers = j;
}

/* go through all the credentials and set timers as appropriate.  hwnd
   is the window that will receive the timer events.*/
void 
khm_timer_refresh(HWND hwnd) {
    int i;
    khm_int64 next_event = 0;

    _begin_task(0);
    _report_cs0(KHERR_DEBUG_1, L"Refreshing timers");
    _describe();

    kcdb_identity_refresh_all();

    EnterCriticalSection(&cs_timers);

    KillTimer(hwnd, KHUI_TRIGGER_TIMER_ID);

    /* When refreshing timers, we go through all of them and mark them
       as stale.  Then we go through the credentials in the root
       credential set and add or refresh the timers associated with
       each identity and credential.  Once this is done, we remove the
       timers that are still stale, since they are no longer in
       use. */

    for (i=0; i < (int) khui_n_timers; i++) {
        khui_timers[i].flags |= KHUI_TE_FLAG_STALE;
    }

    do {
        _report_cs1(KHERR_DEBUG_1, L"Starting with %1!d! timers",
                    _int32(khui_n_timers));

        kcdb_credset_apply(NULL, tmr_cred_apply_proc, NULL);
        tmr_purge();

        _report_cs1(KHERR_DEBUG_1, L"Leaving with %1!d! timers",
                    _int32(khui_n_timers));

    } while (tmr_fire_timer(&next_event));

    if (next_event != 0) {
        khm_int64 diff;

        diff = next_event - tmr_get_current_time();
        if (diff > 0) {
            SetTimer(hwnd,
                     KHUI_TRIGGER_TIMER_ID,
                     (int) FT_TO_MS(diff),
                     NULL);
        }
    }

    LeaveCriticalSection(&cs_timers);

    _end_task();
}
