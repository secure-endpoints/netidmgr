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

#define _CRT_SECURE_NO_WARNINGS

#include "tests.h"
#include<stdio.h>
#include<time.h>
#include<strsafe.h>

extern nim_test_suite util_str_suite;
extern nim_test_suite util_sync_suite;
extern nim_test_suite uilib_res_suite;
extern nim_test_suite uilib_cfg_suite;
extern nim_test_suite util_task_suite;
extern nim_test_suite kconf_basic_suite;

const nim_test_suite *suites[] = {
    &util_str_suite,
    &uilib_res_suite,
    &uilib_cfg_suite,
    &util_sync_suite,
    &util_task_suite,
    &kconf_basic_suite,
};

int  current_test;
int  task_status;

int run_tests(void)
{
    return run_test_by_name(NULL);
}

int run_test_by_name(const char * name)
{
    int i, j;
    time_t now;
    int keep_running = 1;
    int rv = 0;

    time (&now);

    log ("\n\n\nMAIN: Starting tests at %s\n", ctime(&now));

    for (i=0; i < ARRAYLENGTH(suites) && keep_running; i++) {

        if (name && _stricmp(name, suites[i]->name))
            continue;

        current_test = i;

        log("\n\n\nMAIN: Starting test suite [%s]\n", suites[i]->desc);

        if (suites[i]->initialize)
            suites[i]->initialize();

        task_status = 0;

        for (j=0; j < suites[i]->n_tests; j++) {
            int trv;

            log("\n\n\n%8s: Starting test %s\n", suites[i]->name,
                suites[i]->tests[j].desc);

            trv = suites[i]->tests[j].func();

            log("%8s: Ending test %s with %s\n", suites[i]->name,
                suites[i]->tests[j].name,
                ((trv == 0)?"SUCCESS":"FAILURE"));

            if (trv)
                rv = trv;
        }

        if (suites[i]->finalize)
            suites[i]->finalize();

        log("MAIN: Ending test suite %s\n", suites[i]->name);

        if (task_status)
            rv = task_status;
    }

    log ("MAIN: Ending tests\n");

    return rv;
}

void log(const char *fmt, ...)
{
     va_list args;
     va_start( args, fmt );

     vprintf( fmt, args );

     va_end( args );
}

void begin_task(const char * desc)
{
    log("\n%8s: Starting task %s\n", suites[current_test]->name, desc);
}

void end_task(int failed)
{
    failed = (failed || task_status);

    log("%8s: Ending task with %s\n", suites[current_test]->name,
        ((failed? "FAILURE":"SUCCESS")));

    task_status = failed;
}

void check_ifx(khm_boolean b, const char * msg)
{
    if (!b) {
        log("%8s: FAILED %.50s\n", suites[current_test]->name, msg);
        task_status = 1;
    } else {
        log("%8s: Pass   %.50s\n", suites[current_test]->name, msg);
    }
}
