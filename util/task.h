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

#ifndef __NETIDMGR_TASK_H
#define __NETIDMGR_TASK_H

#include "khdefs.h"

/*! \defgroup tasko Task Objects

    Task objects can be used to encapsulate a thread that performs
    some task to completion and then terminates.

 */
/*@{*/

#ifdef __cplusplus
extern "C" {
#endif

struct tag_khm_task;
typedef struct tag_khm_task * khm_task;

typedef khm_int32 (__stdcall *task_proc_t)(void *);

KHMEXP khm_task KHMAPI
task_create(LPSECURITY_ATTRIBUTES thread_sec_attrs,
            unsigned stack_size,
            task_proc_t task_proc,
            void * vparam,
            khm_handle parent_sub,
            unsigned init_flags);

KHMEXP HANDLE KHMAPI
task_get_thread_handle(khm_task ptask);

KHMEXP DWORD KHMAPI
task_get_thread_id(khm_task ptask);

KHMEXP khm_int32 KHMAPI
task_get_thread_rv(khm_task ptask);

KHMEXP void KHMAPI
task_hold(khm_task ptask);

KHMEXP void KHMAPI
task_release(khm_task ptask);

KHMEXP void KHMAPI
task_abort(khm_task ptask);

KHMEXP khm_boolean KHMAPI
task_is_aborted(void);

typedef struct tag_khm_task_callback_params {
    khm_int32   magic;
    khm_task    ptask;
    task_proc_t proc;
    void *      proc_param;

    khm_ui_4  uparam;
    void *    vparam;
} khm_task_callback_params;

#define KHM_TASK_CALLBACK_PARAMS_MAGIC 0x75293c5c

KHMEXP khm_int32 KHMAPI
task_call_parent(khm_ui_4 uparam, void * vparam);

#ifdef __cplusplus
}
#endif

/*@}*/

#endif  /* __NETIDMGR_TASK_H */
