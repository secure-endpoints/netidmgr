/*
 * Copyright (c) 2008-2010 Secure Endpoints Inc.
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

#include<windows.h>
#include<khdefs.h>
#include<kherror.h>
#include<khmsgtypes.h>
#include<utils.h>
#include<process.h>
#include<assert.h>

enum task_state {
    TASK_PREP,
    TASK_RUNNING,
    TASK_ABORTED,
    TASK_EXITED
};

/*! \brief A task object
 */
typedef struct tag_khm_task {
    khm_int32   magic;

    HANDLE      hThread;
    DWORD       dwThread;

    task_proc_t proc;
    khm_int32   proc_rv;
    void *      proc_param;

    khm_handle  parent_sub;
    enum task_state state;

    CRITICAL_SECTION cs;
    khm_int32   refcount;
} * khm_task;

#define KHM_TASK_MAGIC 0x07bed850

#define is_task(t) ((t) && ((khm_task) t)->magic == KHM_TASK_MAGIC)

DECLARE_ONCE(task_once);
DWORD tls_task = 0;

unsigned __stdcall task_runner(void * vparam)
{
    khm_task ptask = (khm_task) vparam;
    khm_int32 rv;

#ifdef DEBUG
    assert(is_task(ptask));
    assert(tls_task != 0);
#endif

    PDESCTHREAD(L"Discardable Task Object", L"Util");

    if (!is_task(ptask))
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&ptask->cs);

    TlsSetValue(tls_task, (LPVOID) ptask);
    ptask->state = TASK_RUNNING;

    LeaveCriticalSection(&ptask->cs);

    rv = (*ptask->proc)(ptask->proc_param);

    EnterCriticalSection(&ptask->cs);

    ptask->proc_rv = rv;
    ptask->state = TASK_EXITED;
    TlsSetValue(tls_task, NULL);

    LeaveCriticalSection(&ptask->cs);

    /* Undo the hold done in task_create() */

    task_release(ptask);

    /* Note that it is not safe to access ptask beyond this point,
       since the preceding release can cause *ptask to be freed. */

    return 0;
}

KHMEXP khm_task KHMAPI
task_create(LPSECURITY_ATTRIBUTES thread_sec_attrs,
            unsigned stack_size,
            task_proc_t task_proc,
            void * vparam,
            khm_handle parent_sub,
            unsigned init_flags)
{
    khm_task ptask;
    unsigned u;

    if (InitializeOnce(&task_once)) {
        tls_task = TlsAlloc();
        InitializeOnceDone(&task_once);
    }

#ifdef DEBUG
    assert(tls_task != 0);
#endif
    if (tls_task == 0)
        return NULL;

    ptask = PMALLOC(sizeof(*ptask));
    ZeroMemory(ptask, sizeof(*ptask));

    ptask->magic = KHM_TASK_MAGIC;
    InitializeCriticalSection(&ptask->cs);
    ptask->parent_sub = parent_sub;
    ptask->proc = task_proc;
    ptask->proc_param = vparam;
    ptask->state = TASK_PREP;
    ptask->refcount = 2;        /* one hold that is released when
                                   task_runner exits, and one hold for
                                   the caller. */

    ptask->hThread = (HANDLE) _beginthreadex(thread_sec_attrs, stack_size,
                                             task_runner, (void *) ptask,
                                             init_flags, &u);
    ptask->dwThread = u;

    return ptask;
}

KHMEXP HANDLE KHMAPI
task_get_thread_handle(khm_task ptask)
{
#ifdef DEBUG
    assert(is_task(ptask));
#endif

    return ptask->hThread;
}

KHMEXP DWORD KHMAPI
task_get_thread_id(khm_task ptask)
{
#ifdef DEBUG
    assert(is_task(ptask));
#endif

    return ptask->dwThread;
}

KHMEXP khm_int32 KHMAPI
task_get_thread_rv(khm_task ptask)
{
    khm_int32 rv;
#ifdef DEBUG
    assert(is_task(ptask));
#endif

    EnterCriticalSection(&ptask->cs);
    rv = ptask->proc_rv;
    LeaveCriticalSection(&ptask->cs);

    return rv;
}

KHMEXP void KHMAPI
task_hold(khm_task ptask)
{
#ifdef DEBUG
    assert(is_task(ptask));
#endif

    EnterCriticalSection(&ptask->cs);
    ptask->refcount++;
    LeaveCriticalSection(&ptask->cs);
}

/* forward DCLs.  Since util is built before kmq, we won't have kmq.h
   in the build include directory at the time we build this file. */
KHMEXP khm_int32 KHMAPI kmq_delete_subscription(khm_handle);
KHMEXP khm_int32 KHMAPI kmq_send_sub_msg(khm_handle, khm_int32, khm_int32, khm_ui_4, void *);

/* Called with ptask->cs held.  Returns with ptask->cs released and
   ptask freed. */
static void
free_task(khm_task ptask)
{
#ifdef DEBUG
    assert(ptask->state == TASK_EXITED);
    assert(ptask->refcount == 0);
#endif

    if (ptask->parent_sub) {
        kmq_delete_subscription(ptask->parent_sub);
        ptask->parent_sub = NULL;
    }
    if (ptask->hThread) {
        CloseHandle(ptask->hThread);
        ptask->hThread = NULL;
    }
    LeaveCriticalSection(&ptask->cs);
    DeleteCriticalSection(&ptask->cs);

    ZeroMemory(ptask, sizeof(*ptask));
    PFREE(ptask);
}

KHMEXP void KHMAPI
task_release(khm_task ptask)
{
#ifdef DEBUG
    assert(is_task(ptask));
#endif

    EnterCriticalSection(&ptask->cs);
    if (--ptask->refcount == 0) {
        /* ptask->cs is released in free_task */
        free_task(ptask);
        return;
    }
    LeaveCriticalSection(&ptask->cs);
}

KHMEXP void KHMAPI
task_abort(khm_task ptask)
{
#ifdef DEBUG
    assert(is_task(ptask));
#endif

    EnterCriticalSection(&ptask->cs);
    ptask->state = TASK_ABORTED;
    LeaveCriticalSection(&ptask->cs);
}

KHMEXP khm_boolean KHMAPI
task_is_aborted(void)
{
    khm_task ptask;
    khm_boolean is_aborted = FALSE;

    if (tls_task == 0)
        return FALSE;

    ptask = (khm_task) TlsGetValue(tls_task);
    if (!is_task(ptask)) {
#ifdef DEBUG
        assert(FALSE);
#endif
        return FALSE;
    }

    EnterCriticalSection(&ptask->cs);
    is_aborted = (ptask->state == TASK_ABORTED);
    LeaveCriticalSection(&ptask->cs);

    return is_aborted;
}

KHMEXP khm_int32 KHMAPI
task_call_parent(khm_ui_4 uparam, void * vparam)
{
    khm_task ptask;
    khm_int32 rv = KHM_ERROR_INVALID_OPERATION;

    if (tls_task == 0)
        return rv;

    ptask = (khm_task) TlsGetValue(tls_task);
    if (!is_task(ptask)) {
#ifdef DEBUG
        assert(FALSE);
#endif
        return rv;
    }

    {
        khm_handle parent_sub;
        khm_task_callback_params p;

        ZeroMemory(&p, sizeof(p));
        p.magic = KHM_TASK_CALLBACK_PARAMS_MAGIC;
        p.uparam = uparam;
        p.vparam = vparam;

        EnterCriticalSection(&ptask->cs);
        parent_sub = ptask->parent_sub;

        p.ptask = ptask;
        task_hold(ptask);
        p.proc = ptask->proc;
        p.proc_param = ptask->proc_param;
        LeaveCriticalSection(&ptask->cs);

        if (parent_sub) {
            rv = kmq_send_sub_msg(parent_sub, KMSG_TASKOBJ, KMSG_TASKOBJ_CALLBACK,
                                  uparam, &p);
        }

        task_release(ptask);
    }

    return rv;
}

