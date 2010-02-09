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

#pragma once

#include<windows.h>
#include<netidmgr.h>

typedef struct tag_nim_test {
    const char * name;
    const char * desc;

    int             (*func)(void);
} nim_test;

typedef struct tag_nim_test_suite {
    const char * name;
    const char * desc;

    int             (*initialize)(void);
    int             (*finalize)(void);

    int             n_tests;
    nim_test *      tests;
} nim_test_suite;

int run_tests(void);

int run_test_by_name(const char * name);

void log(kherr_severity, const char *, ...);

void begin_task(const char *);

void end_task(int failed);

void check_ifx(khm_boolean b, const char * msg, const char * file, int line);

extern kherr_severity max_severity;

#define TOSTR_1(e) #e
#define TOSTR(l) TOSTR_1(l)
#define CHECK(e) check_ifx((e), #e, __FILE__, __LINE__)
#define CHECKX(e,m) check_ifx((e), (m), __FILE__, __LINE__)
#define IS(f) CHECK(KHM_SUCCEEDED(f))
#define ISNT(f) CHECK(KHM_FAILED(f))
