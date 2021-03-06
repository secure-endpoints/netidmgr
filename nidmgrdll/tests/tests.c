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
extern nim_test_suite kconf_prov_suite;
extern nim_test_suite kconf_mem_suite;

const nim_test_suite *suites[] = {
    &util_str_suite,
    &uilib_res_suite,
    &uilib_cfg_suite,
    &util_sync_suite,
    &util_task_suite,
    &kconf_basic_suite,
    &kconf_prov_suite,
    &kconf_mem_suite,
};

int  current_test;
int  task_status;

kherr_severity max_severity = KHERR_ERROR;

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

    log (KHERR_INFO, "\n\n\nMAIN: Starting tests at %s\n", ctime(&now));

    for (i=0; i < ARRAYLENGTH(suites) && keep_running; i++) {

        if (name && _stricmp(name, suites[i]->name))
            continue;

        current_test = i;

        log(KHERR_INFO, "\n\n\nMAIN: Starting test suite [%s]\n", suites[i]->desc);

        if (suites[i]->initialize)
            suites[i]->initialize();

        task_status = 0;

        for (j=0; j < suites[i]->n_tests; j++) {
            int trv;

            log(KHERR_INFO, "\n\n\n%8s: Starting test %s\n", suites[i]->name,
                suites[i]->tests[j].desc);

            trv = suites[i]->tests[j].func();

            log(KHERR_INFO, "%8s: Ending test %s with %s\n", suites[i]->name,
                suites[i]->tests[j].name,
                ((task_status == 0)?"SUCCESS":"FAILURE"));

            if (trv)
                rv = trv;
        }

        if (suites[i]->finalize)
            suites[i]->finalize();

        log(KHERR_INFO, "MAIN: Ending test suite %s\n", suites[i]->name);

        if (task_status)
            rv = task_status;
    }

    log (KHERR_INFO, "MAIN: Ending tests\n");

    return rv;
}

void log(kherr_severity severity, const char *fmt, ...)
{
    va_list args;

    if (severity <= max_severity || severity == KHERR_INFO) {
        va_start( args, fmt );

        vprintf( fmt, args );

        va_end( args );
    }
}

void begin_task(const char * desc)
{
    log(KHERR_INFO, "\n%8s: Starting task %s\n", suites[current_test]->name, desc);
}

void end_task(int failed)
{
    failed = (failed || task_status);

    log(KHERR_INFO, "%8s: Ending task with %s\n", suites[current_test]->name,
        ((failed? "FAILURE":"SUCCESS")));

    task_status = failed;
}

void check_ifx(khm_boolean b, const char * msg, const char * file, int line)
{
    if (!b) {
        log(KHERR_ERROR, "%s(%d) : FAILED %8s: %.50s\n", file, line, suites[current_test]->name, msg);
        task_status = 1;
    } else {
        log(KHERR_DEBUG_1, "%s(%d) : Pass   %8s: %.50s\n", file, line, suites[current_test]->name, msg);
    }
}
