/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
 *
 * Copyright (c) 2007 Secure Endpoints Inc.
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

#include "kmqinternal.h"

CRITICAL_SECTION cs_kmq_msg;
kmq_message * msg_free = NULL;
kmq_message * msg_active = NULL;

#ifdef DEBUG

#include<stdio.h>

void
kmqint_dump_publisher(FILE * f) {

    int n_free = 0;
    int n_active = 0;
    kmq_message * m;

    EnterCriticalSection(&cs_kmq_msg);

    fprintf(f, "qp0\t*** Free Messages ***\n");
    fprintf(f, "qp1\tAddress\n");

    m = msg_free;
    while(m) {
        n_free++;

        fprintf(f, "qp2\t0x%p\n", m);

        m = LNEXT(m);
    }

    fprintf(f, "qp3\tTotal free messages : %d\n", n_free);

    fprintf(f, "qp4\t*** Active Messages ***\n");
    fprintf(f, "qp5\tAddress\tType\tSubtype\tuParam\tvParam\tnSent\tnCompleted\tnFailed\twait_o\trefcount\n");

    m = msg_active;
    while(m) {

        n_active++;

        fprintf(f, "qp6\t0x%p\t%d\t%d\t0x%x\t0x%p\t%d\t%d\t%d\t0x%p\t%d\n",
                m,
                (int) m->type,
                (int) m->subtype,
                (unsigned int) m->uparam,
                m->vparam,
                (int) m->nSent,
                (int) m->nCompleted,
                (int) m->nFailed,
                (void *) m->wait_o,
                (int) m->refcount);

        m = LNEXT(m);
    }

    fprintf(f, "qp7\tTotal number of active messages = %d\n", n_active);

    fprintf(f, "qp8\t--- End ---\n");

    LeaveCriticalSection(&cs_kmq_msg);

}

const wchar_t *
kmqint_msgsubtype_to_str(wchar_t * buf, khm_size cb, khm_int32 t, khm_int32 st)
{
    static const wchar_t *mt[] = {
        L"SYSTEM",
        L"ADHOC",
        L"KCDB",
        L"KMM",
        L"CRED",
        L"ACT",
        L"ALERT",
        L"IDENT",
        L"TASKOBJ",
        L"CREDP"
    };

    static const wchar_t *_sys[] = {
        L"(*)", L"INIT", L"EXIT", L"Completion"
    };

    static const wchar_t *_adhoc[] = {
        L"(*)"
    };

    static const wchar_t *_kcdb[] = {
        L"(*)", L"IDENT", L"CREDTYPE", L"ATTRIB", L"TYPE", L"IDENTPRO"
    };

    static const wchar_t *_kmm[] = {
        L"(*)", L"I_REG", L"I_DONE"
    };

    static const wchar_t *_cred[] = {
        L"(*)", L"ROOTDELTA", L"REFRESH", L"RESOURCE_REQ", NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        L"PASSWORD", L"NEW_CREDS", L"RENEW_CREDS", L"DIALOG_SETUP", L"DIALOG_PRESTART",
        L"DIALOG_START", L"DIALOG_NEW_IDENTITY", L"DIALOG_NEW_OPTIONS", L"PROCESS",
        L"END", L"IMPORT", NULL, NULL, NULL, NULL, NULL,
        L"DESTROY_CREDS"
    };

    static const wchar_t *_act[] = {
        L"(*)", L"ENABLE", L"CHECK", L"REFRESH", L"NEW", L"DELETE", L"ACTIVATE"
    };

    static const wchar_t *_alert[] = {
        L"(*)", L"SHOW", L"QUEUE", L"SHOW_QUEUED", L"CHECK_QUEUE", L"SHOW_MODAL"
    };

    static const wchar_t *_ident[] = {
        L"(*)", L"INIT", L"EXIT", L"VALIDATE_NAME",
        L"VALIDATE_IDENTITY", L"CANON_NAME", L"COMPARE_NAME", L"SET_DEFAULT",
        L"SET_SEARCHABLE", L"GET_INFO", L"ENUM_KNOWN", L"UPDATE",
        L"GET_UI_CALLBACK", L"NOTIFY_CREATE", L"RESOURCE_REQ", L"GET_IDSEL_FACTORY"
    };

    static const wchar_t *_taskobj[] = {
        L"(*)", L"CALLBACK"
    };

    static const wchar_t *_credp[] = {
        L"(*)", L"BEGIN", L"END", L"PROGRESS"
    };

#define MT(s) {ARRAYLENGTH(s),s}

    static const struct {
        int max;
        const wchar_t **s;
    } mst[] = {
        MT(_sys),
        MT(_adhoc),
        MT(_kcdb),
        MT(_kmm),
        MT(_cred),
        MT(_act),
        MT(_alert),
        MT(_ident),
        MT(_taskobj),
        MT(_credp),
    };
#undef MT

    if (t >= 0 && t < ARRAYLENGTH(mst)) {
        if (st >= 0 && st < mst[t].max) {
            StringCbPrintf(buf, cb, L"<%s, %s>", mt[t], mst[t].s[st]);
        } else {
            StringCbPrintf(buf, cb, L"<%s, %d>", mt[t], st);
        }
    } else {
        StringCbPrintf(buf, cb, L"<%d, %d>", t, st);
    }

    return buf;
}

extern khm_boolean
decode_kmsg_cred_message_to_string(wchar_t * buf, khm_size cb,
                                   khm_int32 ty, khm_int32 sty, khm_ui_4 up, void * vp);

extern khm_boolean
decode_kmsg_kcdb_message_to_string(wchar_t * buf, khm_size cb,
                                   khm_int32 ty, khm_int32 sty, khm_ui_4 up, void * vp);

static void
kmqint_report_message(const wchar_t * prefix,
                      khm_int32 ty, khm_int32 sty, khm_ui_4 up, void * vp)
{
    wchar_t buf[128];
    wchar_t tbuf[64];

    switch (ty) {
    case KMSG_CRED:
        if (decode_kmsg_cred_message_to_string(buf, sizeof(buf), ty, sty, up, vp))
            break;

    case KMSG_KCDB:
        if (decode_kmsg_kcdb_message_to_string(buf, sizeof(buf), ty, sty, up, vp))
            break;

    default:
        StringCbPrintf(buf, sizeof(buf), L"%s with uparam=%d, vparam=0x%p",
                       kmqint_msgsubtype_to_str(tbuf, sizeof(tbuf), ty, sty),
                       up, vp);
    }

    _reportf(L"%s %s", prefix, buf);
}

#endif

/*! \internal
    \brief Get a message object
    \note called with ::cs_kmq_msg held */
kmq_message * 
kmqint_get_message(void) {
    kmq_message * m;

    LPOP(&msg_free,&m);
    if(!m) {
        /* allocate one */
        m = PMALLOC(sizeof(kmq_message));
    }
    ZeroMemory((void*)m, sizeof(kmq_message));

    LPUSH(&msg_active, m);

    return m;
}

/*! \internal
    \brief Frees a message object
    \note called with ::cs_kmq_msg held
    */
void 
kmqint_put_message(kmq_message *m) {
    int queued;
    kherr_context * ctx = NULL;

    /* we can only free a message if the refcount is zero.
       Otherwise we have to wait until the call is freed. */
    if(m->refcount == 0) {
        LDELETE(&msg_active, m);
        LeaveCriticalSection(&cs_kmq_msg);
        queued = kmqint_notify_msg_completion(m);
        EnterCriticalSection(&cs_kmq_msg);
        if (!queued) {
            ctx = m->err_ctx;
            m->err_ctx = NULL;

            if(m->wait_o) {
                CloseHandle(m->wait_o);
                m->wait_o = NULL;
            }
            LPUSH(&msg_free,m);
        }

        if (ctx) {
            LeaveCriticalSection(&cs_kmq_msg);
            kherr_release_context(ctx);
            EnterCriticalSection(&cs_kmq_msg);
        }
    } else if(m->wait_o) {
        SetEvent(m->wait_o);
    }
}

/*! \internal
    \note Obtains ::cs_kmq_msg, ::cs_kmq_types, ::cs_kmq_msg_ref, kmq_queue::cs
    */
KHMEXP khm_int32 KHMAPI 
kmq_send_message(khm_int32 type, khm_int32 subtype, 
                 khm_ui_4 uparam, void * blob) {
    kmq_call c;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    rv = kmqint_post_message_ex(type, subtype, uparam, blob, &c, TRUE);
    if(KHM_FAILED(rv))
        return rv;

    rv = kmq_wait(c, INFINITE);
    if(KHM_SUCCEEDED(rv) && c->nFailed > 0)
        rv = KHM_ERROR_PARTIAL;

    kmq_free_call(c);

    return rv;
}

/*! \internal
    \note Obtains ::cs_kmq_msg, ::cs_kmq_types, ::cs_kmq_msg_ref, kmq_queue::cs
    */
KHMEXP khm_int32 KHMAPI 
kmq_post_message(khm_int32 type, khm_int32 subtype, 
                 khm_ui_4 uparam, void * blob) {
    return kmqint_post_message_ex(type, subtype, uparam, blob, NULL, FALSE);
}

/*! \internal
    \brief Frees a call
    \note Obtains ::cs_kmq_msg
    */
KHMEXP khm_int32 KHMAPI 
kmq_free_call(kmq_call call) {
    kmq_message * m;

    m = call;

    EnterCriticalSection(&cs_kmq_msg);
    m->refcount--;
    if(!m->refcount) {
        kmqint_put_message(m);
    }
    LeaveCriticalSection(&cs_kmq_msg);

    return KHM_ERROR_SUCCESS;
}

/*! \internal
    \note Obtains ::cs_kmq_msg, ::cs_kmq_types, ::cs_kmq_msg_ref, kmq_queue::cs 
    */
khm_int32 
kmqint_post_message_ex(khm_int32 type, khm_int32 subtype, khm_ui_4 uparam, 
    void * blob, kmq_call * call, khm_boolean try_send) 
{
    kmq_message * m;
    kherr_context * ctx;

#ifdef DEBUG
    kmqint_report_message(L"Posting message", type, subtype, uparam, blob);
#endif

    EnterCriticalSection(&cs_kmq_msg);
    m = kmqint_get_message();
    LeaveCriticalSection(&cs_kmq_msg);

    m->type = type;
    m->subtype = subtype;
    m->uparam = uparam;
    m->vparam = blob;

    m->timeSent = GetTickCount();
    m->timeExpire = m->timeSent + kmq_call_dead_timeout;

    ctx = kherr_peek_context();
    if (ctx) {
        if (ctx->flags & KHERR_CF_TRANSITIVE) {
            m->err_ctx = ctx;
            /* leave it held */
        } else {
            kherr_release_context(ctx);
        }
    }

    if(call) {
        m->wait_o = CreateEvent(NULL,FALSE,FALSE,NULL);
        *call = m;
        m->refcount++;
    } else
        m->wait_o = NULL;

    kmqint_msg_publish(m, try_send);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI 
kmq_post_message_ex(khm_int32 type, khm_int32 subtype, 
                    khm_ui_4 uparam, void * blob, kmq_call * call)
{
    return kmqint_post_message_ex(type, subtype, uparam, blob, call, FALSE);
}

KHMEXP khm_int32 KHMAPI
kmq_abort_call(kmq_call call)
{
    /* TODO: Implement this */
    return KHM_ERROR_NOT_IMPLEMENTED;
}

/*! \internal
*/
KHMEXP khm_int32 KHMAPI 
kmq_post_sub_msg(khm_handle sub, khm_int32 type, khm_int32 subtype, 
                 khm_ui_4 uparam, void * vparam)
{
    return kmq_post_sub_msg_ex(sub, type, subtype, uparam, vparam, NULL);
}

/*! \internal
*/
khm_int32 
kmqint_post_sub_msg_ex(khm_handle sub, khm_int32 type, khm_int32 subtype, 
                       khm_ui_4 uparam, void * vparam, 
                       kmq_call * call, khm_boolean try_send)
{
    kmq_message * m;
    kherr_context * ctx;

#ifdef DEBUG
    kmqint_report_message(L"Posting message (Subscription)", type, subtype, uparam, vparam);
#endif

    EnterCriticalSection(&cs_kmq_msg);
    m = kmqint_get_message();
    LeaveCriticalSection(&cs_kmq_msg);

    m->type = type;
    m->subtype = subtype;
    m->uparam = uparam;
    m->vparam = vparam;

    m->timeSent = GetTickCount();
    m->timeExpire = m->timeSent + kmq_call_dead_timeout;

    ctx = kherr_peek_context();
    if (ctx) {
        if (ctx->flags & KHERR_CF_TRANSITIVE) {
            m->err_ctx = ctx;
            /* leave it held */
        } else {
            kherr_release_context(ctx);
        }
    }

    if(call) {
        m->wait_o = CreateEvent(NULL,FALSE,FALSE,NULL);
        *call = m;
        m->refcount++;
    } else
        m->wait_o = NULL;

    if (try_send)
        EnterCriticalSection(&cs_kmq_types);
    EnterCriticalSection(&cs_kmq_msg);
    kmqint_post((kmq_msg_subscription *) sub, m, try_send);

    if(m->nCompleted + m->nFailed == m->nSent) {
        kmqint_put_message(m);
    }
    LeaveCriticalSection(&cs_kmq_msg);
    if (try_send)
        LeaveCriticalSection(&cs_kmq_types);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI 
kmq_post_sub_msg_ex(khm_handle sub, khm_int32 type, khm_int32 subtype, 
                    khm_ui_4 uparam, void * vparam, kmq_call * call)
{
    return kmqint_post_sub_msg_ex(sub, type, subtype, 
                                  uparam, vparam, call, FALSE);
}

khm_int32 
kmqint_post_subs_msg_ex(khm_handle * subs, khm_size   n_subs, khm_int32 type, 
                        khm_int32 subtype, khm_ui_4 uparam, void * vparam, 
                        kmq_call * call, khm_boolean try_send)
{
    kmq_message * m;
    kherr_context * ctx;
    khm_size i;

#ifdef DEBUG
    kmqint_report_message(L"Posting message (Subscriptions)", type, subtype,
                          uparam, vparam);
#endif

    if(n_subs == 0)
        return KHM_ERROR_SUCCESS;

    EnterCriticalSection(&cs_kmq_msg);
    m = kmqint_get_message();
    LeaveCriticalSection(&cs_kmq_msg);

    m->type = type;
    m->subtype = subtype;
    m->uparam = uparam;
    m->vparam = vparam;

    m->timeSent = GetTickCount();
    m->timeExpire = m->timeSent + kmq_call_dead_timeout;

    ctx = kherr_peek_context();
    if (ctx) {
        if (ctx->flags & KHERR_CF_TRANSITIVE) {
            m->err_ctx = ctx;
            /* leave it held */
        } else {
            kherr_release_context(ctx);
        }
    }

    if(call) {
        m->wait_o = CreateEvent(NULL,FALSE,FALSE,NULL);
        *call = m;
        m->refcount++;
    } else
        m->wait_o = NULL;

    if (try_send)
        EnterCriticalSection(&cs_kmq_types);
    EnterCriticalSection(&cs_kmq_msg);
    for(i=0;i<n_subs;i++) {
        kmqint_post((kmq_msg_subscription *) subs[i], m, try_send);
    }

    if(m->nCompleted + m->nFailed == m->nSent) {
        kmqint_put_message(m);
    }
    LeaveCriticalSection(&cs_kmq_msg);
    if (try_send)
        EnterCriticalSection(&cs_kmq_types);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI 
kmq_post_subs_msg(khm_handle * subs, 
                  khm_size   n_subs, 
                  khm_int32 type, 
                  khm_int32 subtype, 
                  khm_ui_4 uparam, 
                  void * vparam)
{
    return kmqint_post_subs_msg_ex(subs,
                                   n_subs,
                                   type,
                                   subtype,
                                   uparam,
                                   vparam,
                                   NULL,
                                   FALSE);
}

KHMEXP khm_int32 KHMAPI 
kmq_post_subs_msg_ex(khm_handle * subs, 
                     khm_int32 n_subs, 
                     khm_int32 type, 
                     khm_int32 subtype, 
                     khm_ui_4 uparam, 
                     void * vparam, 
                     kmq_call * call)
{
    return kmqint_post_subs_msg_ex(subs, n_subs, type, subtype, 
                                   uparam, vparam, call, FALSE);
}

KHMEXP khm_int32 KHMAPI 
kmq_send_subs_msg(khm_handle *subs, 
                  khm_int32 n_subs,
                  khm_int32 type, 
                  khm_int32 subtype, 
                  khm_ui_4 uparam, 
                  void * vparam)
{
    kmq_call c;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    rv = kmqint_post_subs_msg_ex(subs, n_subs, type, subtype,
                                 uparam, vparam, &c, TRUE);
    if(KHM_FAILED(rv))
        return rv;

    rv = kmq_wait(c, INFINITE);
    if(KHM_SUCCEEDED(rv) && c->nFailed > 0)
        rv = KHM_ERROR_PARTIAL;

    kmq_free_call(c);

    return rv;
}

/*! \internal
*/
KHMEXP khm_int32 KHMAPI 
kmq_send_sub_msg(khm_handle sub, khm_int32 type, khm_int32 subtype, 
                 khm_ui_4 uparam, void * vparam)
{
    kmq_call c;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    rv = kmqint_post_sub_msg_ex(sub, type, subtype, uparam, vparam, &c, TRUE);
    if(KHM_FAILED(rv))
        return rv;

    rv = kmq_wait(c, INFINITE);
    if(KHM_SUCCEEDED(rv) && c->nFailed > 0)
        rv = KHM_ERROR_PARTIAL;

    kmq_free_call(c);

    return rv;
}

/*! \internal
    \note Obtains ::cs_kmq_global, ::cs_kmq_msg, ::cs_kmq_msg_ref, kmq_queue::cs
    */
KHMEXP khm_int32 KHMAPI 
kmq_send_thread_quit_message(kmq_thread_id thread, khm_ui_4 uparam) {
    kmq_call c;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    rv = kmq_post_thread_quit_message(thread, uparam, &c);
    if(KHM_FAILED(rv))
        return rv;

    rv = kmq_wait(c, INFINITE);

    kmq_free_call(c);

    return rv;
}

/*! \internal
    \note Obtains ::cs_kmq_global, ::cs_kmq_msg, ::cs_kmq_msg_ref, kmq_queue::cs
    */ 
KHMEXP khm_int32 KHMAPI 
kmq_post_thread_quit_message(kmq_thread_id thread, 
                             khm_ui_4 uparam, kmq_call * call) {
    kmq_message * m;
    kmq_queue * q;

    EnterCriticalSection(&cs_kmq_global);
    q = queues;
    while(q) {
        if(q->thread == thread)
            break;
        q = LNEXT(q);
    }
    LeaveCriticalSection(&cs_kmq_global);

    if(!q)
        return KHM_ERROR_NOT_FOUND;

    EnterCriticalSection(&cs_kmq_msg);
    m = kmqint_get_message();
    LeaveCriticalSection(&cs_kmq_msg);

    m->type = KMSG_SYSTEM;
    m->subtype = KMSG_SYSTEM_EXIT;
    m->uparam = uparam;
    m->vparam = NULL;

    m->timeSent = GetTickCount();
    m->timeExpire = m->timeSent + kmq_call_dead_timeout;

    if(call) {
        m->wait_o = CreateEvent(NULL,FALSE,FALSE,NULL);
        *call = m;
        m->refcount++;
    } else
        m->wait_o = NULL;

    kmqint_post_queue(q, m);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI 
kmq_get_next_response(kmq_call call, void ** resp) {
    /* TODO: Implement this */
    return 0;
}

KHMEXP khm_boolean KHMAPI 
kmq_has_completed(kmq_call call) {
    khm_boolean completed;

    EnterCriticalSection(&cs_kmq_msg);
    completed = (call->nCompleted + call->nFailed == call->nSent);
    LeaveCriticalSection(&cs_kmq_msg);

    return completed;
}

KHMEXP khm_int32 KHMAPI 
kmq_wait(kmq_call call, kmq_timer timeout) {
    kmq_message * m = call;
    DWORD rv;
    /*TODO: check for call free */

    if(m && m->wait_o) {
        rv = WaitForSingleObject(m->wait_o, timeout);
        if(rv == WAIT_OBJECT_0)
            return KHM_ERROR_SUCCESS;
        else
            return KHM_ERROR_TIMEOUT;
    } else
        return KHM_ERROR_INVALID_PARAM;
}

/*! \internal
    \note Obtains ::cs_kmq_types
    */
KHMEXP khm_int32 KHMAPI 
kmq_set_completion_handler(khm_int32 type, 
                           kmq_msg_completion_handler handler) {
    return kmqint_msg_type_set_handler(type, handler);
}



