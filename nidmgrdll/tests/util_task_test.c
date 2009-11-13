/*
 * Copyright (c) 2008-2009 Secure Endpoints Inc.
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

#define NIMPRIVATE
#include "tests.h"

khm_int32 __stdcall test_task_1(void * p) {
    return (khm_int32)(INT_PTR) p;
}

int create_test(void)
{
    khm_task t;
    HANDLE   h;
    int      r;

    r = rand() & 0x0fffffff;
    log(KHERR_DEBUG_1, "Picked random number r=%d\n", r);
    t = task_create(NULL, 0, test_task_1, (void *)(ssize_t) r, NULL, 0);
    CHECK(t != NULL);
    h = task_get_thread_handle(t);
    CHECK(h != NULL);
    WaitForSingleObject(h, INFINITE);
    CHECK(task_get_thread_rv(t) == r);

    task_release(t);

    return 0;
}

__declspec(align(8)) LONG volatile _task_2_counter = 0;

khm_int32 __stdcall test_task_2(void * p) {
    int count = 0;

    while (!task_is_aborted()) {
        Sleep(100);
        InterlockedIncrement(&_task_2_counter);
    }

    return (khm_int32) count;
}

int abort_test(void)
{
    khm_task t;
    HANDLE   h;
    int      r;
    unsigned local_cycles = 0;
    LONG previous = 0;

    r = (rand() & 0x0f) + 8;
    log(KHERR_DEBUG_1, "Picked random number r=%d\n", r);
    t = task_create(NULL, 0, test_task_2, NULL, NULL, 0);
    CHECK(t != NULL);
    h = task_get_thread_handle(t);
    CHECK(h != NULL);

    while(_task_2_counter < r) {
        local_cycles++;
        if (previous != _task_2_counter) {
            previous = _task_2_counter;
            log(KHERR_DEBUG_1, "  Thread cycles: %d, Local cycles: %d\n",
                (int) previous, (int) local_cycles);
        }
        Sleep(0);
    }

    log(KHERR_DEBUG_1, "  Local cycles: %d\n", local_cycles);

    task_abort(t);

    log(KHERR_DEBUG_1, "Waiting for thread to abort\n");

    local_cycles = 0;
    while(WaitForSingleObject(h, 0) == WAIT_TIMEOUT) {
        local_cycles++;
        Sleep(0);
    }

    log(KHERR_DEBUG_1, "Took %d local cycles\n", local_cycles);

    task_release(t);

    return 0;
}

khm_int32 KHMAPI test_task_3(void * p)
{
    khm_int32 c = 1;

    task_call_parent(0, NULL);

    Sleep(100);

    task_call_parent(1, test_task_2);

    Sleep(100);

    task_call_parent(1, test_task_2);

    Sleep(100);

    while (c) {
        task_call_parent(2, &c);
    }

    return 0;
}

int call_test(void);

khm_int32 KHMAPI call_test_msg(khm_int32 msg_type, khm_int32 msg_subtype,
                               khm_ui_4 uparam, void * vparam)
{
    if (msg_type == KMSG_TASKOBJ && msg_subtype == KMSG_TASKOBJ_CALLBACK) {
        khm_task_callback_params *p;

        log (KHERR_DEBUG_1, "Received message from task : uparam=%ud\n", uparam);

        p = (khm_task_callback_params *) vparam;

        CHECK(p->magic == KHM_TASK_CALLBACK_PARAMS_MAGIC);
        CHECK(p->uparam == uparam);
        CHECK(p->proc == test_task_3);
        CHECK(p->proc_param == call_test);

        if (uparam == 1) {
            CHECK(p->vparam == test_task_2);
        } else if (uparam == 2) {
            khm_int32 * pi = (khm_int32 *) p->vparam;

            log (KHERR_DEBUG_1, "Received iteration %d ... ", *pi);
            *pi = (*pi + 1) % 20;
            log (KHERR_DEBUG_1, "Sending iteration %d\n", *pi);
        }

    } else {
        log (KHERR_DEBUG_1, "Unknown message type : %d and subtype %d\n", msg_type, msg_subtype);
    }

    return KHM_ERROR_SUCCESS;
}

int call_test(void)
{
    khm_task   t;
    HANDLE h;
    khm_handle sub;

    CHECK(KHM_SUCCEEDED(kmq_create_subscription(call_test_msg, &sub)));

    t = task_create(NULL, 0, test_task_3, call_test, sub, 0);
    CHECK(t != NULL);
    h = task_get_thread_handle(t);
    CHECK(h != NULL);

    while (WaitForSingleObject(h, 10) == WAIT_TIMEOUT) {
        if (KHM_SUCCEEDED(kmq_dispatch(10))) {
            log(KHERR_DEBUG_1, "Message successfully dispatched...\n");
        }
    }

    task_release(t);

    return 0;
}

static nim_test tests[] = {
    {"TaskCreate", "task_create() test", create_test},
    {"TaskAbort", "task_abort() tesT", abort_test},
    {"TaskCall", "task_call_parent() test", call_test},
};

nim_test_suite util_task_suite = {
    "UtilTask", "[util] Task Object tests", NULL, NULL, ARRAYLENGTH(tests), tests
};
