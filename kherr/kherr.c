/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
 * Copyright (c) 2006-2011 Secure Endpoints Inc.
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

#include "kherrinternal.h"
#include<assert.h>
#include<stdarg.h>

CRITICAL_SECTION cs_error;
DWORD tls_error = 0;
kherr_context * ctx_free_list = NULL;
kherr_context * ctx_root_list = NULL;
kherr_context * ctx_error_list = NULL;
kherr_event * evt_free_list = NULL;

kherr_handler_node * ctx_handlers = NULL;
khm_size n_ctx_handlers = 0;
khm_size nc_ctx_handlers = 0;

kherr_serial ctx_serial = 0;

static kherr_context *
peek_context(void);

static void
get_progress(kherr_context * c, khm_ui_4 * pnum, khm_ui_4 * pdenom);

#ifdef DEBUG
#define DEBUG_CONTEXT
#endif

KHMEXP void
kherr_debug_printf(wchar_t * fmt, ...)
{
    va_list vl;
    wchar_t buf[1024];

    va_start(vl, fmt);
    StringCbVPrintf(buf, sizeof(buf), fmt, vl);
    OutputDebugString(buf);
    va_end(vl);
}

/* Called with cs_error held */
static void
remove_ctx_handler_by_index(khm_size i)
{
#ifdef DEBUG
    assert(i < n_ctx_handlers);
#endif

    if (i < n_ctx_handlers - 1)
        memmove(&ctx_handlers[i], &ctx_handlers[i+1],
                (n_ctx_handlers - (i + 1)) * sizeof(ctx_handlers[0]));
    else
        memset(&ctx_handlers[i], 0, sizeof(ctx_handlers[0]));

    n_ctx_handlers --;
}

static void
add_ctx_handler_node(const kherr_handler_node * n)
{
    khm_size idx;

    EnterCriticalSection(&cs_error);

    /* Make sure we have enough space */
    if (n_ctx_handlers + 1 > nc_ctx_handlers) {
        nc_ctx_handlers = UBOUNDSS(n_ctx_handlers + 1, CTX_ALLOC_INCR, CTX_ALLOC_INCR);
        ctx_handlers = PREALLOC(ctx_handlers, sizeof(*ctx_handlers) * nc_ctx_handlers);
    }

    /* Since commit events are the most frequent, we put those
       handlers at the top of the list.  When dispatching a commit
       event, we stop looking at the list when we find a filter that
       doesn't filter for commit events. */
    if (n->filter & KHERR_CTX_EVTCOMMIT) {
	idx = 0;
        if (n_ctx_handlers > 0)
            memmove(&ctx_handlers[1], &ctx_handlers[0],
                    n_ctx_handlers * sizeof(ctx_handlers[0]));
    } else {
	idx = n_ctx_handlers;
    }

    n_ctx_handlers++;

    ctx_handlers[idx] = *n;

    if (ctx_handlers[idx].filter == 0)
        ctx_handlers[idx].filter =
            KHERR_CTX_BEGIN |
            KHERR_CTX_DESCRIBE |
            KHERR_CTX_END |
            KHERR_CTX_ERROR |
            KHERR_CTX_NEWCHILD |
            KHERR_CTX_FOLDCHILD;

    if (ctx_handlers[idx].serial == KHERR_SERIAL_CURRENT) {
        kherr_context * c;

        c = peek_context();

        if (IS_KHERR_CTX(c)) {
            ctx_handlers[idx].serial = c->serial;
        } else {

            /* If we were going to refer to the current context and
               there is no current context, then we don't want to keep
               this handler node.  If we do, it will never get called
               and will not be automatically removed from the list. */

            remove_ctx_handler_by_index(idx);
        }
    }

    LeaveCriticalSection(&cs_error);
}

#pragma warning(push)
#pragma warning(disable: 4995)

KHMEXP void KHMAPI
kherr_add_ctx_handler(kherr_ctx_handler h,
                      khm_int32 filter,
                      kherr_serial serial)
{
    kherr_handler_node n;

    assert(h);

    n.filter = filter;
    n.serial = serial;
    n.use_param = FALSE;
    n.vparam = NULL;
    n.h.p_handler = h;

    add_ctx_handler_node(&n);
}

#pragma warning(pop)

KHMEXP void KHMAPI
kherr_add_ctx_handler_param(kherr_ctx_handler_param h,
                            khm_int32 filter,
                            kherr_serial serial,
                            void * vparam)
{
    kherr_handler_node n;

    assert(h);

    n.filter = filter;
    n.serial = serial;
    n.use_param = TRUE;
    n.vparam = vparam;
    n.h.p_handler_param = h;

    add_ctx_handler_node(&n);
}

#pragma warning(push)
#pragma warning(disable: 4995)

KHMEXP void KHMAPI
kherr_remove_ctx_handler(kherr_ctx_handler h,
                         kherr_serial serial)
{
    khm_size i;
    EnterCriticalSection(&cs_error);

    for (i=0 ; i < n_ctx_handlers; i++) {
        if (!ctx_handlers[i].use_param &&
            ctx_handlers[i].h.p_handler == h &&
            ctx_handlers[i].serial == serial) {
            break;
        }
    }

    if ( i < n_ctx_handlers ) {
        remove_ctx_handler_by_index(i);
    }

    LeaveCriticalSection(&cs_error);
}

#pragma warning(pop)

KHMEXP void KHMAPI
kherr_remove_ctx_handler_param(kherr_ctx_handler_param h,
                               kherr_serial serial,
                               void * vparam)
{
    khm_size i;
    EnterCriticalSection(&cs_error);

    for (i=0 ; i < n_ctx_handlers; i++) {
        if (ctx_handlers[i].use_param &&
            ctx_handlers[i].h.p_handler_param == h &&
            ctx_handlers[i].vparam == vparam &&
            ctx_handlers[i].serial == serial) {
            break;
        }
    }

    if ( i < n_ctx_handlers ) {
        remove_ctx_handler_by_index(i);
    }

    LeaveCriticalSection(&cs_error);
}



/* Called with cs_error held. Lets go of cs_error while processing */
static void
notify_ctx_event(enum kherr_ctx_event e, kherr_context * c,
		 kherr_context * d_c, kherr_event * evt, int p)
{
    unsigned i, nh = 0;
    kherr_ctx_event_data d;
#define MAX_NOTIFICATIONS 8
    kherr_handler_node h[MAX_NOTIFICATIONS];

    ZeroMemory(&d, sizeof(d));
    d.event = e;
    d.ctx = c;
    switch (e) {
    case KHERR_CTX_END:
    case KHERR_CTX_BEGIN:
	break;

    case KHERR_CTX_DESCRIBE:
    case KHERR_CTX_ERROR:
    case KHERR_CTX_EVTCOMMIT:
    case KHERR_CTX_FOLDCHILD:
	d.data.event = evt;
	break;

    case KHERR_CTX_NEWCHILD:
	d.data.child_ctx = d_c;
	break;

    case KHERR_CTX_PROGRESS:
	d.data.progress = p;
	break;

    default:
	assert(FALSE);
    }

    for (i=0; i < n_ctx_handlers && nh < MAX_NOTIFICATIONS; i++) {
        if ((ctx_handlers[i].filter & e) != 0 &&
            (ctx_handlers[i].serial == 0 ||
             ctx_handlers[i].serial == c->serial)) {

	    h[nh++] = ctx_handlers[i];

            /* If this was a notification that the context is done, we
               should remove the handler. */
            if (e == KHERR_CTX_END && ctx_handlers[i].serial == c->serial) {
                remove_ctx_handler_by_index(i);
                i--;
            }
        } else if (e == KHERR_CTX_EVTCOMMIT &&
		   !(ctx_handlers[i].filter & KHERR_CTX_EVTCOMMIT)) {
	    /* All handlers that filter for commit events are at the
	       top of the list.  If this handler wasn't filtering for
	       it, then there's no point in going further down the
	       list. */
	    break;
	}
    }

    assert(cs_error.RecursionCount == 1);
    LeaveCriticalSection(&cs_error);

    for (i = 0; i < nh; i++) {
	if (h[i].use_param)
	    (*h[i].h.p_handler_param)(e, &d, h[i].vparam);
	else
	    (*h[i].h.p_handler)(e, c);
    }

    EnterCriticalSection(&cs_error);
}

void
attach_this_thread(void)
{
    kherr_thread * t;

    t = (kherr_thread *) TlsGetValue(tls_error);
    if (t)
        return;

    t = PMALLOC(sizeof(kherr_thread) +
                sizeof(kherr_context *) * THREAD_STACK_SIZE);
    t->nc_ctx = THREAD_STACK_SIZE;
    t->n_ctx = 0;
    t->ctx = (kherr_context **) &t[1];

    TlsSetValue(tls_error, t);
}

/* Should NOT be called with cs_error held */
void
detach_this_thread(void)
{
    kherr_thread * t;
    khm_size i;

    t = (kherr_thread *) TlsGetValue(tls_error);
    if (t) {
        for(i=0; i < t->n_ctx; i++) {
            kherr_release_context(t->ctx[i]);
        }
        PFREE(t);
        TlsSetValue(tls_error, 0);
    }
}

static kherr_context *
peek_context(void)
{
    kherr_thread * t;

    t = (kherr_thread *) TlsGetValue(tls_error);
    if (t) {
        if (t->n_ctx > 0) {
            kherr_context * c;

            c = t->ctx[t->n_ctx - 1];

            assert(c == NULL || IS_KHERR_CTX(c));

            return c;
        } else {
            return NULL;
        }
    } else
        return NULL;
}

static void
push_context(kherr_context * c)
{
    kherr_thread * t;

    t = (kherr_thread *) TlsGetValue(tls_error);
    if (!t) {
        attach_this_thread();
        t = (kherr_thread *) TlsGetValue(tls_error);
        assert(t);
    }

    if (t->n_ctx == t->nc_ctx) {
        khm_size nc_new;
        khm_size cb_new;
        kherr_thread * nt;

        nc_new = t->nc_ctx + THREAD_STACK_SIZE;
        cb_new = sizeof(kherr_thread) +
            sizeof(kherr_context *) * nc_new;

        nt = PMALLOC(cb_new);
        memcpy(nt, t, sizeof(kherr_thread) +
               sizeof(kherr_context *) * t->n_ctx);
        nt->ctx = (kherr_context **) &nt[1];
        nt->nc_ctx = nc_new;

        PFREE(t);
        t = nt;
        TlsSetValue(tls_error, t);
    }

    assert(t->n_ctx < t->nc_ctx);
    t->ctx[t->n_ctx++] = c;

    kherr_hold_context(c);
}

/* returned pointer is still held */
static kherr_context *
pop_context(void)
{
    kherr_thread * t;
    kherr_context * c;

    t = (kherr_thread *) TlsGetValue(tls_error);
    if (t) {
        if (t->n_ctx > 0) {
            c = t->ctx[--(t->n_ctx)];
            assert(IS_KHERR_CTX(c));
            return c;
        } else
            return NULL;
    } else {
        return NULL;
    }
}

static kherr_event *
get_empty_event(void)
{
    kherr_event * e;

    EnterCriticalSection(&cs_error);
    if(evt_free_list) {
        LPOP(&evt_free_list, &e);
    } else {
        e = PMALLOC(sizeof(*e));
    }
    LeaveCriticalSection(&cs_error);
    ZeroMemory(e, sizeof(*e));
    e->severity = KHERR_NONE;
    e->magic = KHERR_EVENT_MAGIC;

    return e;
}

static void
free_event_params(kherr_event * e)
{
    assert(IS_KHERR_EVENT(e));

    if(parm_type(e->p1) == KEPT_STRINGT) {
        assert((void *) parm_data(e->p1));
        PFREE((void*) parm_data(e->p1));
    }
    ZeroMemory(&e->p1, sizeof(e->p1));

    if(parm_type(e->p2) == KEPT_STRINGT) {
        assert((void *) parm_data(e->p2));
        PFREE((void*) parm_data(e->p2));
    }
    ZeroMemory(&e->p2, sizeof(e->p2));

    if(parm_type(e->p3) == KEPT_STRINGT) {
        assert((void *) parm_data(e->p3));
        PFREE((void*) parm_data(e->p3));
    }
    ZeroMemory(&e->p3, sizeof(e->p3));

    if(parm_type(e->p4) == KEPT_STRINGT) {
        assert((void *) parm_data(e->p4));
        PFREE((void*) parm_data(e->p4));
    }
    ZeroMemory(&e->p4, sizeof(e->p4));
}

static void
free_event(kherr_event * e)
{
    EnterCriticalSection(&cs_error);

    assert(IS_KHERR_EVENT(e));
#ifdef DEBUG
    assert(LNEXT(e) == NULL);
    assert(LPREV(e) == NULL);
#endif

    if(e->flags & KHERR_RF_FREE_SHORT_DESC) {
        assert(e->short_desc);
        PFREE((void *) e->short_desc);
    }
    if(e->flags & KHERR_RF_FREE_LONG_DESC) {
        assert(e->long_desc);
        PFREE((void *) e->long_desc);
    }
    if(e->flags & KHERR_RF_FREE_SUGGEST) {
        assert(e->suggestion);
        PFREE((void *) e->suggestion);
    }

    free_event_params(e);

    ZeroMemory(e, sizeof(*e));

    LPUSH(&evt_free_list, e);
    LeaveCriticalSection(&cs_error);
}

static kherr_context *
get_empty_context(void)
{
    kherr_context * c;

    EnterCriticalSection(&cs_error);
    if(ctx_free_list) {
        LPOP(&ctx_free_list, &c);
    } else {
        c = PMALLOC(sizeof(kherr_context));
    }

    ZeroMemory(c,sizeof(*c));
    c->severity = KHERR_NONE;
    c->flags = KHERR_CF_UNBOUND;
    c->magic = KHERR_CONTEXT_MAGIC;
    c->serial = ++ctx_serial;

    LPUSH(&ctx_root_list, c);

    LeaveCriticalSection(&cs_error);

    return c;
}


/* Assumes that the context has been deleted from all relevant
   lists */
static void
free_context(kherr_context * c)
{
    kherr_context * ch;
    kherr_event * e;

    assert(IS_KHERR_CTX(c));

#ifdef DEBUG_CONTEXT
    if (IsDebuggerPresent())
        kherr_debug_printf(L"Freeing context 0x%x\n", c);
#endif

    EnterCriticalSection(&cs_error);

    if (c->desc_event)
        free_event(c->desc_event);
    c->desc_event = NULL;

    TPOPCHILD(c, &ch);
    while(ch) {
        free_context(ch);
        TPOPCHILD(c, &ch);
    }
    QGET(c, &e);
    while(e) {
        free_event(e);
        QGET(c, &e);
    }

    c->serial = 0;

    LPUSH(&ctx_free_list,c);
    LeaveCriticalSection(&cs_error);

#ifdef DEBUG_CONTEXT
    if (IsDebuggerPresent())
        kherr_debug_printf(L"Done with context 0x%x\n", c);
#endif
}

/* MUST be called with cs_error held */
static void
commit_event(kherr_context * c, kherr_event * e)
{
    if (e->flags & KHERR_RF_COMMIT)
        return;

#ifdef DEBUG_CONTEXT
    if (IsDebuggerPresent()) {
        if (!(e->flags & KHERR_RF_STR_RESOLVED))
            resolve_event_strings(e);

        if (e->short_desc && e->long_desc) {
            kherr_debug_printf(L"E:%s (%s)\n", e->short_desc, e->long_desc);
        } else if (e->short_desc) {
            kherr_debug_printf(L"E:%s\n", e->short_desc);
        } else if (e->long_desc) {
            kherr_debug_printf(L"E:%s\n", e->long_desc);
        } else {
            kherr_debug_printf(L"E:[No description for event 0x%p]\n", e);
        }

        if (e->suggestion)
            kherr_debug_printf(L"  Suggest:[%s]\n", e->suggestion);
        if (e->facility)
            kherr_debug_printf(L"  Facility:[%s]\n", e->facility);
    }
#endif

    notify_ctx_event(KHERR_CTX_EVTCOMMIT, c, NULL, e, 0);
    e->flags |= KHERR_RF_COMMIT;

    if(c->severity >= e->severity) {
        c->severity = e->severity;
        c->err_event = e;
        c->flags &= ~KHERR_CF_DIRTY;

        if (e->severity <= KHERR_ERROR)
            notify_ctx_event(KHERR_CTX_ERROR, c, NULL, e, 0);
    }
}

/* MUST be called with cs_error held */
static void
add_event(kherr_context * c, kherr_event * e)
{
    kherr_event * te;

    assert(IS_KHERR_CTX(c));
    assert(IS_KHERR_EVENT(e));
#ifdef DEBUG
    assert(LPREV(e) == NULL && LNEXT(e) == NULL);
#endif

    te = QBOTTOM(c);
    if (te && !(te->flags & KHERR_RF_COMMIT)) {
        commit_event(c, te);
    }

    QPUT(c,e);
}

static void
pick_err_event(kherr_context * c)
{
    kherr_event * e;
    kherr_event * ce = NULL;
    enum kherr_severity s;

    s = KHERR_RESERVED_BANK;

    EnterCriticalSection(&cs_error);
    e = QTOP(c);
    while(e) {
        if(!(e->flags & KHERR_RF_INERT) &&
           s >= e->severity) {
            ce = e;
            s = e->severity;
        }
        e = QNEXT(e);
    }

    if(ce) {
        c->err_event = ce;
        c->severity = ce->severity;
    } else {
        c->err_event = NULL;
        c->severity = KHERR_NONE;
    }

    c->flags &= ~KHERR_CF_DIRTY;
    LeaveCriticalSection(&cs_error);
}

static void
va_arg_from_param(va_list * parm, kherr_param p)
{
    int t = parm_type(p);

    khm_int32 * pi;
    wchar_t ** ps;
    void ** pptr;
    khm_int64 * pli;

    if (t != KEPT_NONE) {
        switch (t) {
        case KEPT_INT32:
        case KEPT_UINT32:
            pi = (khm_int32 *)(*parm);
            va_arg(*parm, khm_int32);
            *pi = (khm_int32) parm_data(p);
            break;

        case KEPT_STRINGC:
        case KEPT_STRINGT:
            ps = (wchar_t **) (*parm);
            va_arg(*parm, wchar_t *);
            *ps = (wchar_t *) parm_data(p);
            break;

        case KEPT_PTR:
            pptr = (void **) (*parm);
            va_arg(*parm, void *);
            *pptr = (void *) parm_data(p);
            break;

        case KEPT_INT64:
        case KEPT_UINT64:
            pli = (khm_int64 *) (*parm);
            va_arg(*parm, khm_int64);
            *pli = (khm_int64) parm_data(p);
            break;

#ifdef DEBUG
        default:
            assert(FALSE);
#endif
        }
    }
}

static void
va_args_from_event(va_list args, kherr_event * e, khm_size cb)
{
    ZeroMemory(args, cb);

    va_arg_from_param(&args, e->p1);
    va_arg_from_param(&args, e->p2);
    va_arg_from_param(&args, e->p3);
    va_arg_from_param(&args, e->p4);
}

static void
resolve_string_resource(kherr_event * e,
                        const wchar_t ** str,
                        va_list vl,
                        khm_int32 if_flag,
                        khm_int32 or_flag)
{
    wchar_t tfmt[KHERR_MAXCCH_STRING];
    wchar_t tbuf[KHERR_MAXCCH_STRING];
    size_t chars = 0;
    size_t bytes = 0;

    if(e->flags & if_flag) {
        if(e->h_module != NULL)
            chars = LoadString(e->h_module, (UINT)(INT_PTR) *str,
                               tfmt, ARRAYLENGTH(tbuf));
        if(e->h_module == NULL || chars == 0)
            *str = NULL;
        else {
            wchar_t * s;

            chars = FormatMessage(FORMAT_MESSAGE_FROM_STRING, tfmt,
                                  0, 0, tbuf, ARRAYLENGTH(tbuf), &vl);

            if (chars == 0) {
                *str = NULL;
            } else {
                bytes = (chars + 1) * sizeof(wchar_t);
                s = PMALLOC(bytes);
                assert(s);
                StringCbCopy(s, bytes, tbuf);
                *str = s;
                e->flags |= or_flag;
            }
        }
        e->flags &= ~if_flag;
    }
}

static void
resolve_msg_resource(kherr_event * e,
                     const wchar_t ** str,
                     va_list vl,
                     khm_int32 if_flag,
                     khm_int32 or_flag)
{
    wchar_t tbuf[KHERR_MAXCCH_STRING];
    size_t chars = 0;
    size_t bytes = 0;

    if(e->flags & if_flag) {
        if(e->h_module != NULL) {

            chars = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
                                  (LPCVOID) e->h_module,
                                  (DWORD)(DWORD_PTR) *str,
                                  0,
                                  tbuf,
                                  ARRAYLENGTH(tbuf),
                                  &vl);
        }

        if(e->h_module == NULL || chars == 0) {
            *str = NULL;
        } else {
            wchar_t * s;

            /* MC inserts trailing \r\n to each message unless the
               message is terminated with a %0.  We remove the last
               line break since it is irrelevant to our handling of
               the string in the UI. */
            if (tbuf[chars-1] == L'\n')
                tbuf[--chars] = L'\0';
            if (tbuf[chars-1] == L'\r')
                tbuf[--chars] = L'\0';

            bytes = (chars + 1) * sizeof(wchar_t);
            s = PMALLOC(bytes);
            assert(s);
            StringCbCopy(s, bytes, tbuf);
            *str = s;
            e->flags |= or_flag;
        }
        e->flags &= ~if_flag;
    }
}

static void
resolve_string(kherr_event * e,
               const wchar_t ** str,
               va_list vl,
               khm_int32 mask,
               khm_int32 free_if,
               khm_int32 or_flag)
{
    wchar_t tbuf[KHERR_MAXCCH_STRING];
    size_t chars;
    size_t bytes;

    if (((e->flags & mask) == 0 ||
         (e->flags & mask) == free_if) &&
        *str != NULL) {

        chars = FormatMessage(FORMAT_MESSAGE_FROM_STRING,
                              (LPCVOID) *str,
                              0,
                              0,
                              tbuf,
                              ARRAYLENGTH(tbuf),
                              &vl);

        if ((e->flags & mask) == free_if) {
            assert(FALSE);
            /* We can't safely free *str here since other threads may
               have references to it.  Threads access event objects
               without locks. */
            PFREE((void *) *str);
        }

        e->flags &= ~mask;

        if (chars == 0) {
            *str = 0;
        } else {
            wchar_t * s;

            bytes = (chars + 1) * sizeof(wchar_t);
            s = PMALLOC(bytes);
            assert(s);
            StringCbCopy(s, bytes, tbuf);
            *str = s;
            e->flags |= or_flag;
        }
    }

}

/* MUST be called with cs_error held. */
void
resolve_event_strings(kherr_event * e)
{
    DWORD_PTR args[8];
    va_list vl = (va_list) args;

    if ((e->flags & KHERR_RF_STR_RESOLVED) != 0)
        return;

    va_args_from_event(vl, e, sizeof(args));

    resolve_string(e, &e->short_desc, vl,
                   KHERR_RFMASK_SHORT_DESC,
                   KHERR_RF_FREE_SHORT_DESC,
                   KHERR_RF_FREE_SHORT_DESC);

    resolve_string(e, &e->long_desc, vl,
                   KHERR_RFMASK_LONG_DESC,
                   KHERR_RF_FREE_LONG_DESC,
                   KHERR_RF_FREE_LONG_DESC);

    resolve_string(e, &e->suggestion, vl,
                   KHERR_RFMASK_SUGGEST,
                   KHERR_RF_FREE_SUGGEST,
                   KHERR_RF_FREE_SUGGEST);

    resolve_string_resource(e, &e->short_desc, vl,
                            KHERR_RF_RES_SHORT_DESC,
                            KHERR_RF_FREE_SHORT_DESC);

    resolve_string_resource(e, &e->long_desc, vl,
                            KHERR_RF_RES_LONG_DESC,
                            KHERR_RF_FREE_LONG_DESC);

    resolve_string_resource(e, &e->suggestion, vl,
                            KHERR_RF_RES_SUGGEST,
                            KHERR_RF_FREE_SUGGEST);

    resolve_msg_resource(e, &e->short_desc, vl,
                         KHERR_RF_MSG_SHORT_DESC,
                         KHERR_RF_FREE_SHORT_DESC);
    resolve_msg_resource(e, &e->long_desc, vl,
                         KHERR_RF_MSG_LONG_DESC,
                         KHERR_RF_FREE_LONG_DESC);
    resolve_msg_resource(e, &e->suggestion, vl,
                         KHERR_RF_MSG_SUGGEST,
                         KHERR_RF_FREE_SUGGEST);

    /* get rid of dangling reference now that we have done everything
       we can with it.  Since we have already dealt with all the
       parameter inserts, we don't need the parameters anymore
       either. */
    free_event_params(e);

    e->h_module = NULL;
    e->flags |= KHERR_RF_STR_RESOLVED;
}


KHMEXP void KHMAPI
kherr_evaluate_event(kherr_event * e)
{
    if (!IS_KHERR_EVENT(e))
        return;

    EnterCriticalSection(&cs_error);
    resolve_event_strings(e);
    LeaveCriticalSection(&cs_error);
}

KHMEXP void KHMAPI
kherr_evaluate_last_event(void)
{
    kherr_context * c;
    kherr_event * e;
    DWORD tid;

    c = peek_context();
    if(!IS_KHERR_CTX(c))
        return;
    tid = GetCurrentThreadId();

    EnterCriticalSection(&cs_error);
    e = QBOTTOM(c);
    while (e != NULL && e->thread_id != tid)
        e = QPREV(e);

    if(!IS_KHERR_EVENT(e))
        goto _exit;

    resolve_event_strings(e);

 _exit:
    LeaveCriticalSection(&cs_error);
}

KHMEXP kherr_event * __cdecl
kherr_reportf(const wchar_t * long_desc_fmt, ...)
{
    va_list vl;
    wchar_t buf[1024];
    kherr_event * e;

    va_start(vl, long_desc_fmt);
    StringCbVPrintf(buf, sizeof(buf), long_desc_fmt, vl);
    va_end(vl);

    e = kherr_report(KHERR_DEBUG_1,
                     NULL, NULL, NULL, buf, NULL, 0,
                     KHERR_SUGGEST_NONE, _vnull(), _vnull(), _vnull(), _vnull(),
                     KHERR_RF_CSTR_LONG_DESC
#ifdef _WIN32
                     ,NULL
#endif
                     );
    if (IS_KHERR_EVENT(e)) {
        kherr_evaluate_event(e);
    }

    return e;
}

KHMEXP kherr_event * __cdecl
kherr_reportf_ex(enum kherr_severity severity,
                 const wchar_t * facility,
                 khm_int32 facility_id,
#ifdef _WIN32
                 HMODULE hModule,
#endif
                 const wchar_t * long_desc_fmt, ...)
{
    va_list vl;
    wchar_t buf[1024];
    kherr_event * e;

    va_start(vl, long_desc_fmt);
    StringCbVPrintf(buf, sizeof(buf), long_desc_fmt, vl);
#ifdef DEBUG
    if (IsDebuggerPresent())
        OutputDebugString(buf);
#endif
    va_end(vl);

    e = kherr_report(severity, NULL, facility, NULL, buf, NULL, facility_id,
                     KHERR_SUGGEST_NONE,
                     _vnull(),
                     _vnull(),
                     _vnull(),
                     _vnull(), KHERR_RF_CSTR_LONG_DESC
#ifdef _WIN32
                     ,hModule
#endif
                     );
    if (IS_KHERR_EVENT(e)) {
        kherr_evaluate_event(e);
    }

    return e;
}

/* Should NOT be called with cs_error held */
KHMEXP kherr_event * KHMAPI
kherr_report(enum kherr_severity severity,
             const wchar_t * short_desc,
             const wchar_t * facility,
             const wchar_t * location,
             const wchar_t * long_desc,
             const wchar_t * suggestion,
             khm_int32 facility_id,
             enum kherr_suggestion suggestion_id,
             kherr_param p1,
             kherr_param p2,
             kherr_param p3,
             kherr_param p4,
             khm_int32 flags
#ifdef _WIN32
             ,HMODULE  h_module
#endif
             )
{
    kherr_context * c;
    kherr_event * e;

    e = get_empty_event();

    e->thread_id = GetCurrentThreadId();
    e->time_ticks = GetTickCount();
    GetSystemTimeAsFileTime(&e->time_ft);

    e->severity = severity;
    e->short_desc = short_desc;
    e->facility = facility;
    e->location = location;
    e->long_desc = long_desc;
    e->suggestion = suggestion;
    e->facility_id = facility_id;
    e->suggestion_id = suggestion_id;
    e->p1 = p1;
    e->p2 = p2;
    e->p3 = p3;
    e->p4 = p4;
    e->flags = flags;
#ifdef _WIN32
    e->h_module = h_module;
#endif

    /* sanity check */
    if (!IS_POW2(flags & KHERR_RFMASK_SHORT_DESC) ||
	!IS_POW2(flags & KHERR_RFMASK_LONG_DESC) ||
	!IS_POW2(flags & KHERR_RFMASK_SUGGEST))
    {
        /* the reason why we are doing it this way is because p1..p4,
           the descriptions and the suggestion may contain allocations
           that has to be freed. */
#ifdef DEBUG
	assert(FALSE);
#else
	if (IsDebuggerPresent())
	    DebugBreak();
#endif
        free_event(e);
        e = NULL;
    } else {
	EnterCriticalSection(&cs_error);
	c = peek_context();
	if(c) {
	    add_event(c,e);
	} else {
	    free_event(e);
	    e = NULL;
	}
	LeaveCriticalSection(&cs_error);
    }

    return e;
}

KHMEXP void KHMAPI
kherr_suggest(wchar_t * suggestion,
              enum kherr_suggestion suggestion_id,
              khm_int32 flags)
{
    kherr_context * c;
    kherr_event * e;
    DWORD tid;

    if (flags != KHERR_RF_CSTR_SUGGEST &&
        flags != KHERR_RF_RES_SUGGEST &&
        flags != KHERR_RF_MSG_SUGGEST &&
        flags != KHERR_RF_FREE_SUGGEST)
        return;

    c = peek_context();
    if(!IS_KHERR_CTX(c))
        return;

    tid = GetCurrentThreadId();

    EnterCriticalSection(&cs_error);
    e = QBOTTOM(c);
    while (e != NULL && e->thread_id != tid)
        e = QPREV(e);

    if(!IS_KHERR_EVENT(e))
        goto _exit;

    /* if strings have already been resolved in this event, we cant
       add any more unresolved strings. */
    if ((flags == KHERR_RF_RES_SUGGEST ||
         flags == KHERR_RF_MSG_SUGGEST) &&
        (e->flags & KHERR_RF_STR_RESOLVED))
        goto _exit;

    e->suggestion = suggestion;
    e->suggestion_id = suggestion_id;
    e->flags |= flags;
_exit:
    LeaveCriticalSection(&cs_error);
}

KHMEXP void KHMAPI
kherr_location(wchar_t * location)
{
    kherr_context * c;
    kherr_event * e;
    DWORD tid;

    c = peek_context();
    if(!IS_KHERR_CTX(c))
        return;
    tid = GetCurrentThreadId();

    EnterCriticalSection(&cs_error);
    e = QBOTTOM(c);
    while (e != NULL && e->thread_id != tid)
        e = QPREV(e);

    if(!IS_KHERR_EVENT(e))
        goto _exit;
    e->location = location;
_exit:
    LeaveCriticalSection(&cs_error);
}

KHMEXP void KHMAPI
kherr_facility(wchar_t * facility,
               khm_int32 facility_id)
{
    kherr_context * c;
    kherr_event * e;
    DWORD tid;

    c = peek_context();
    if(!IS_KHERR_CTX(c))
        return;
    tid = GetCurrentThreadId();
    EnterCriticalSection(&cs_error);
    e = QBOTTOM(c);
    while (e != NULL && e->thread_id != tid)
        e = QPREV(e);

    if(!IS_KHERR_EVENT(e))
        goto _exit;
    e->facility = facility;
    e->facility_id = facility_id;
_exit:
    LeaveCriticalSection(&cs_error);
}

/* Should NOT be called with cs_error held */
KHMEXP void KHMAPI
kherr_set_desc_event(void)
{
    kherr_context * c;
    kherr_event * e;
    DWORD tid;

    c = peek_context();
    if(!IS_KHERR_CTX(c))
        return;
    tid = GetCurrentThreadId();

    EnterCriticalSection(&cs_error);
    e = QBOTTOM(c);
    while (e != NULL && e->thread_id != tid)
        e = QPREV(e);

    if(!IS_KHERR_EVENT(e) || c->desc_event)
        goto _exit;

    QDEL(c,e);
    c->desc_event = e;
    e->severity = KHERR_NONE;
    resolve_event_strings(e);

    notify_ctx_event(KHERR_CTX_DESCRIBE, c, NULL, e, 0);

_exit:
    LeaveCriticalSection(&cs_error);
}

KHMEXP void KHMAPI
kherr_del_last_event(void)
{
    kherr_context * c;
    kherr_event * e;
    DWORD tid;

    c = peek_context();

    if(!IS_KHERR_CTX(c))
        return;

    tid = GetCurrentThreadId();

    EnterCriticalSection(&cs_error);
    e = QBOTTOM(c);
    while (e != NULL && e->thread_id != tid)
        e = QPREV(e);

    if(IS_KHERR_EVENT(e)) {
        QDEL(c, e);
        if(c->err_event == e) {
            pick_err_event(c);
        }
        free_event(e);
    }
    LeaveCriticalSection(&cs_error);
}

KHMEXP void KHMAPI
kherr_push_context(kherr_context * c)
{

    if (!IS_KHERR_CTX(c))
        return;

    EnterCriticalSection(&cs_error);

    push_context(c);

    LeaveCriticalSection(&cs_error);
}

/* Should NOT be called with cs_error held */
KHMEXP void KHMAPI
kherr_push_new_context(khm_int32 flags)
{
    kherr_context * p = NULL;
    kherr_context * c;

    flags &= KHERR_CFMASK_INITIAL;

    EnterCriticalSection(&cs_error);
    p = peek_context();
    c = get_empty_context();
    if(IS_KHERR_CTX(p)) {
        LDELETE(&ctx_root_list, c);
#ifdef DEBUG
        assert(TPARENT(c) == NULL);
#endif
        TADDCHILD(p,c);
        c->flags &= ~KHERR_CF_UNBOUND;
        kherr_hold_context(p);
    }
    c->flags |= flags;
    push_context(c);

    notify_ctx_event(KHERR_CTX_BEGIN, c, NULL, NULL, 0);
    if (IS_KHERR_CTX(p)) {
        notify_ctx_event(KHERR_CTX_NEWCHILD, p, c, NULL, 0);
    }

    LeaveCriticalSection(&cs_error);
}

/* does the context 'c' use it's own progress marker? If this is
   false, the progress marker for the context is derived from the
   progress markers of its children. */
#define CTX_USES_OWN_PROGRESS(c)		\
    ((c)->progress_num != 0 ||			\
     (c)->progress_denom != 0 ||		\
     ((c)->flags & KHERR_CF_OWN_PROGRESS))

/* MUST be called with cs_error held */
static void
set_and_notify_progress_change(kherr_context * c, khm_ui_4 num, khm_ui_4 denom)
{
    kherr_context * p;

    c->progress_denom = denom;
    c->progress_num = num;

    notify_ctx_event(KHERR_CTX_PROGRESS, c, NULL, NULL, (denom != 0)? num * 256 / denom : 0);

    for (p = TPARENT(c);
	 IS_KHERR_CTX(p) && !CTX_USES_OWN_PROGRESS(p);
	 p = TPARENT(p)) {

	get_progress(p, &num, &denom);
	notify_ctx_event(KHERR_CTX_PROGRESS, p, NULL, NULL,
			 (denom != 0)? num * 256 / denom : 0);
    }
}

/* Should NOT be called with cs_error held */
KHMEXP void KHMAPI
kherr_set_progress(khm_ui_4 num, khm_ui_4 denom)
{
    kherr_context * c = peek_context();
    if(IS_KHERR_CTX(c)) {
        EnterCriticalSection(&cs_error);

        if (num > denom)
            num = denom;

        if (c->progress_denom != denom ||
            c->progress_num != denom) {

	    set_and_notify_progress_change(c, num, denom);
        }
        LeaveCriticalSection(&cs_error);
    }
}

KHMEXP void KHMAPI
kherr_get_progress(khm_ui_4 * num, khm_ui_4 * denom)
{
    kherr_context * c = peek_context();
    kherr_get_progress_i(c,num,denom);
}

/* MUST be called with cs_error held */
static void
get_progress(kherr_context * c, khm_ui_4 * pnum, khm_ui_4 * pdenom)
{
    if (CTX_USES_OWN_PROGRESS(c)) {
        *pnum = c->progress_num;
        *pdenom = c->progress_denom;
    } else {
        khm_ui_4 num = 0;
        khm_ui_4 denom = 0;

        kherr_context * cc;

        for (cc = TFIRSTCHILD(c);
             cc;
             cc = TNEXTSIBLING(cc)) {

            khm_ui_4 cnum, cdenom;

            assert(IS_KHERR_CTX(cc));

            get_progress(cc, &cnum, &cdenom);

            if (cdenom == 0) {
                continue;
            } else {
                if (cnum > cdenom)
                    cnum = cdenom;

                if (cdenom != 256) {
                    cnum = ((long)cnum * 256) / cdenom;
                    cdenom = 256;
                }

                /*
                 * The following works around a bug that I have
                 * been unable to find the cause of.  This function
                 * iterates forever because cc == TNEXTSIBLING(cc)
                 */
                if (cc == TNEXTSIBLING(cc)) {
#ifdef DEBUG
                    assert(cc != TNEXTSIBLING(cc));
#endif
                    cc->next = NULL;
                }
                if (cc == TPREVSIBLING(cc)) {
#ifdef DEBUG
                    assert(cc != TPREVSIBLING(cc));
#endif
                    cc->prev = NULL;
                }
            }

            num += cnum;
            denom += cdenom;
        }

        *pnum = num;
        *pdenom = denom;
    }
}

KHMEXP void KHMAPI
kherr_get_progress_i(kherr_context * c,
                     khm_ui_4 * num,
                     khm_ui_4 * denom)
{
    if (num == NULL || denom == NULL)
        return;

    if(IS_KHERR_CTX(c)) {
        EnterCriticalSection(&cs_error);
        get_progress(c, num, denom);
        LeaveCriticalSection(&cs_error);
    } else {
        *num = 0;
        *denom = 0;
    }
}

static kherr_param
dup_parm(kherr_param p)
{
    if(parm_type(p) == KEPT_STRINGT) {
        wchar_t * d = PWCSDUP((wchar_t *)parm_data(p));
        return kherr_val(KEPT_STRINGT, (khm_ui_8) d);
    } else
        return p;
}

static kherr_event *
fold_context(kherr_context * c)
{
    kherr_event * e;
    kherr_event * g;

    if (!IS_KHERR_CTX(c))
        return NULL;

    EnterCriticalSection(&cs_error);
    if(!c->err_event || (c->flags & KHERR_CF_DIRTY)) {
        pick_err_event(c);
    }
    if(c->err_event) {
        g = c->err_event;
        e = get_empty_event();
        *e = *g;
        g->short_desc = NULL;
        g->long_desc = NULL;
        g->suggestion = NULL;
        g->flags &=
            ~(KHERR_RF_FREE_SHORT_DESC |
              KHERR_RF_FREE_LONG_DESC |
              KHERR_RF_FREE_SUGGEST);
        LINIT(e);
        e->p1 = dup_parm(g->p1);
        e->p2 = dup_parm(g->p2);
        e->p3 = dup_parm(g->p3);
        e->p4 = dup_parm(g->p4);
    } else {
        e = c->desc_event;
        c->desc_event = NULL;
    }

    if (IS_KHERR_EVENT(e)) {
        e->flags |= KHERR_RF_CONTEXT_FOLD;
	e->flags &= ~KHERR_RF_COMMIT;
    }

    LeaveCriticalSection(&cs_error);

    return e;
}

KHMEXP void KHMAPI
kherr_hold_context(kherr_context * c)
{
    if(!IS_KHERR_CTX(c))
        return;

    EnterCriticalSection(&cs_error);
    c->refcount++;
    LeaveCriticalSection(&cs_error);
}

/* MUST be called with cs_error held */
static void
release_context(kherr_context * c)
{
    if (IS_KHERR_CTX(c)) {
        c->refcount--;

        {
            kherr_event * e;

            e = QBOTTOM(c);
            if (IS_KHERR_EVENT(e) && !(e->flags & KHERR_RF_COMMIT)) {
                commit_event(c, e);
            }
        }

        if (c->refcount == 0) {
            kherr_context * p;

            if (CTX_USES_OWN_PROGRESS(c)) {
                set_and_notify_progress_change(c, 256, 256);
            }

            p = TPARENT(c);

#ifdef DEBUG
	    kherr_debug_printf(L"Posting KHERR_CTX_END for %p\n", (void *) c);
#endif
            notify_ctx_event(KHERR_CTX_END, c, NULL, NULL, 0);

            if (IS_KHERR_CTX(p)) {
                kherr_event * e;

                e = fold_context(c);
                TDELCHILD(p, c);
                
		if (e) {
                    add_event(p, e);
		    notify_ctx_event(KHERR_CTX_FOLDCHILD, p, NULL, e, 0);
		}

                release_context(p);
            } else {
                LDELETE(&ctx_root_list, c);
            }
            free_context(c);
        }
    }
}

/* Should NOT be called with cs_error held */
KHMEXP void KHMAPI
kherr_release_context(kherr_context * c)
{
    if (!IS_KHERR_CTX(c))
        return;

    EnterCriticalSection(&cs_error);
    release_context(c);
    LeaveCriticalSection(&cs_error);
}

/* Should NOT be called with cs_error held */
KHMEXP void KHMAPI
kherr_pop_context(void)
{
    kherr_context * c;

    EnterCriticalSection(&cs_error);
    c = pop_context();
    if(IS_KHERR_CTX(c)) {
        release_context(c);
    }
    LeaveCriticalSection(&cs_error);
}

KHMEXP kherr_context * KHMAPI
kherr_peek_context(void)
{
    kherr_context * c;

    c = peek_context();
    if (IS_KHERR_CTX(c))
        kherr_hold_context(c);

    return c;
}

KHMEXP khm_boolean KHMAPI
kherr_is_error(void)
{
    kherr_context * c = peek_context();
    return kherr_is_error_i(c);
}

KHMEXP khm_boolean KHMAPI
kherr_is_error_i(kherr_context * c)
{
    khm_boolean is_error = FALSE;

    if(IS_KHERR_CTX(c)) {
        kherr_context * cc;

        EnterCriticalSection(&cs_error);
        if (c->severity <= KHERR_ERROR)
            is_error = TRUE;
        for (cc = TFIRSTCHILD(c); cc && !is_error; cc = TNEXTSIBLING(cc)) {
            is_error = kherr_is_error_i(cc);
        }
        LeaveCriticalSection(&cs_error);

        return is_error;
    } else {
        return FALSE;
    }
}

KHMEXP void KHMAPI
kherr_clear_error(void)
{
    kherr_context * c = peek_context();
    if (IS_KHERR_CTX(c))
        kherr_clear_error_i(c);
}

KHMEXP void KHMAPI
kherr_clear_error_i(kherr_context * c)
{
    kherr_event * e;
    if (IS_KHERR_CTX(c)) {
        EnterCriticalSection(&cs_error);
        e = QTOP(c);
        while(e) {
            assert(IS_KHERR_EVENT(e));

            e->flags |= KHERR_RF_INERT;
            e = QNEXT(e);
        }
        c->severity = KHERR_NONE;
        c->err_event = NULL;
        c->flags &= ~KHERR_CF_DIRTY;
        LeaveCriticalSection(&cs_error);
    }
}

KHMEXP kherr_event * KHMAPI
kherr_get_first_event(kherr_context * c)
{
    kherr_event * e;

    if (!IS_KHERR_CTX(c))
        return NULL;

    EnterCriticalSection(&cs_error);
    e = QTOP(c);
    LeaveCriticalSection(&cs_error);
    assert(e == NULL || IS_KHERR_EVENT(e));
    return e;
}

KHMEXP kherr_event * KHMAPI
kherr_get_next_event(kherr_event * e)
{
    kherr_event * ee;

    if (!IS_KHERR_EVENT(e))
        return NULL;

    EnterCriticalSection(&cs_error);
    ee = QNEXT(e);
    LeaveCriticalSection(&cs_error);
    assert(ee == NULL || IS_KHERR_EVENT(ee));
    return ee;
}

KHMEXP kherr_event * KHMAPI
kherr_get_prev_event(kherr_event * e)
{
    kherr_event * ee;

    if (!IS_KHERR_EVENT(e))
        return NULL;

    EnterCriticalSection(&cs_error);
    ee = QPREV(e);
    LeaveCriticalSection(&cs_error);
    assert(ee == NULL || IS_KHERR_EVENT(ee));
    return ee;
}

KHMEXP kherr_event * KHMAPI
kherr_get_last_event(kherr_context * c)
{
    kherr_event * e;

    if (!IS_KHERR_CTX(c))
        return NULL;

    EnterCriticalSection(&cs_error);
    e = QBOTTOM(c);
    LeaveCriticalSection(&cs_error);
    assert(e == NULL || IS_KHERR_EVENT(e));
    return e;
}

KHMEXP kherr_context * KHMAPI
kherr_get_first_context(kherr_context * c)
{
    kherr_context * cc;

    if (c != NULL && !IS_KHERR_CTX(c))
        return NULL;

    EnterCriticalSection(&cs_error);
    if (IS_KHERR_CTX(c)) {
        cc = TFIRSTCHILD(c);
        if (cc)
            kherr_hold_context(cc);
    } else {
        cc = ctx_root_list;
        if (cc)
            kherr_hold_context(cc);
    }
    LeaveCriticalSection(&cs_error);
    assert(cc == NULL || IS_KHERR_CTX(cc));
    return cc;
}

KHMEXP kherr_context * KHMAPI
kherr_get_next_context(kherr_context * c)
{
    kherr_context * cc;

    if (!IS_KHERR_CTX(c))
        return NULL;

    EnterCriticalSection(&cs_error);
    cc = LNEXT(c);
    if (cc)
        kherr_hold_context(cc);
    LeaveCriticalSection(&cs_error);
    assert(cc == NULL || IS_KHERR_CTX(cc));
    kherr_release_context(c);
    return cc;
}

KHMEXP kherr_event * KHMAPI
kherr_get_err_event(kherr_context * c)
{
    kherr_event * e;

    if (!IS_KHERR_CTX(c))
        return NULL;

    EnterCriticalSection(&cs_error);
    if(!c->err_event) {
        pick_err_event(c);
    }
    e = c->err_event;
    LeaveCriticalSection(&cs_error);
    assert(e == NULL || IS_KHERR_EVENT(e));
    return e;
}

KHMEXP kherr_event * KHMAPI
kherr_get_desc_event(kherr_context * c)
{
    kherr_event * e;

    if (!IS_KHERR_CTX(c))
        return NULL;

    EnterCriticalSection(&cs_error);
    e = c->desc_event;
    LeaveCriticalSection(&cs_error);
    assert(e == NULL || IS_KHERR_EVENT(e));
    return e;
}

KHMEXP kherr_param
kherr_dup_string(const wchar_t * s)
{
    wchar_t * dest;
    size_t cb_s;

    if (s == NULL)
        return _vnull();

    if (FAILED(StringCbLength(s, KHERR_MAXCB_STRING, &cb_s)))
        cb_s = KHERR_MAXCB_STRING;
    else
        cb_s += sizeof(wchar_t);

    dest = PMALLOC(cb_s);
    assert(dest != NULL);
    dest[0] = L'\0';

    StringCbCopy(dest, cb_s, s);

    return _tstr(dest);
}

KHMEXP khm_boolean KHMAPI
kherr_context_is_equal(kherr_context *c1, kherr_context *c2)
{
    return c1 == c2;
}

KHMEXP khm_int32 KHMAPI
kherr_context_get_flags(kherr_context * c)
{
    khm_int32 flags;

    EnterCriticalSection(&cs_error);
    flags = c->flags;
    LeaveCriticalSection(&cs_error);

    return flags;
}

KHMEXP khm_int32 KHMAPI
kherr_context_set_flags(kherr_context * c,
                        khm_int32 mask,
                        khm_int32 flags)
{
    EnterCriticalSection(&cs_error);
    c->flags = ((c->flags & ~mask) | (flags & mask));
    flags = c->flags;
    LeaveCriticalSection(&cs_error);

    return flags;
}
