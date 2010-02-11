/*
 * Copyright (c) 2009-2010 Secure Endpoints Inc.
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

#pragma once

#include "container.h"

#ifdef DEBUG
#define DCL_TIMER(t) LARGE_INTEGER pcounter_ ## t
#define START_TIMER(t) QueryPerformanceCounter(&pcounter_ ## t)
#define END_TIMER(t, prefix)                                            \
    do {                                                                \
        LARGE_INTEGER tend;                                             \
        LARGE_INTEGER freq;                                             \
        QueryPerformanceCounter(&tend);                                 \
        QueryPerformanceFrequency(&freq);                               \
        wchar_t buf[100];                                               \
        StringCbPrintf(buf, sizeof(buf), prefix L" Time elapsed: %f seconds\n", \
                       (tend.QuadPart - pcounter_ ## t.QuadPart * 1.0) / freq.QuadPart);                  \
        OutputDebugString(buf);                                         \
    } while(0)
#else
#define DCL_TIMER(t)
#define START_TIMER(t)
#define END_TIMER(t, prefix)
#endif

#define CW_CANAME_FLAGS L"_CWFlags"

#include "CwColumn.hpp"
#include "credwnd_widgets.hpp"
#include "CwOutlineBase.hpp"
#include "CwOutline.hpp"
#include "CwIdentityOutline.hpp"
#include "CwCredTypeOutline.hpp"
#include "CwGenericOutline.hpp"
#include "CwCredentialRow.hpp"
#include "CwTable.hpp"

