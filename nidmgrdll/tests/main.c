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

#include "tests.h"

int main(int argc, char ** argv)
{
    int rv = 0;

#if FULL_INIT
    PDESCTHREAD(L"UI",L"App");

    khm_version_init();
    kmq_init();
    khui_init_actions();
    kmm_init();
#endif

    if (argc == 1)
        rv = run_tests();
    else {
        int i;
        int trv;

        for (i=1; i < argc; i++) {
            if (!strcmp(argv[i], "-v")) {
                max_severity =
                    (max_severity < KHERR_INFO)? KHERR_INFO:
                    (max_severity < KHERR_DEBUG_1)? KHERR_DEBUG_1:
                    (max_severity < KHERR_DEBUG_3)? KHERR_DEBUG_3:
                    max_severity;

                continue;
            }

            trv = run_test_by_name(argv[i]);
            if (trv)
                rv = trv;
        }
    }

#if FULL_INIT
 _exit:
    kmm_exit();
    khui_exit_actions();
    kmq_exit();
#endif

    return rv;
}
