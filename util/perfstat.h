/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
 * Copyright (c) 2006-2009 Secure Endpoints Inc.
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

#ifndef __KHIMAIRA_PERFSTAT_H
#define __KHIMAIRA_PERFSTAT_H

#define _CRTDBG_MAP_ALLOC
#include<stdlib.h>
#include<crtdbg.h>

#include<khdefs.h>

#ifdef _DEBUG
#define PMALLOC(s) _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define PCALLOC(n,s) _calloc_dbg(n,s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define PREALLOC(d,s) _realloc_dbg(d,s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define PFREE(p)   _free_dbg(p, _NORMAL_BLOCK)
#define PWCSDUP(s) _wcsdup_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define PSTRDUP(s) _strdup_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)

#define PINIT()    perf_init()
#define PEXIT()    perf_exit()
#define PDUMP(f)   perf_dump(f)
#define PDESCTHREAD(n,c) perf_set_thread_desc(__FILE__,__LINE__,n,c);
#else
#define PMALLOC(s) malloc(s)
#define PCALLOC(n,s) calloc(n,s)
#define PREALLOC(d,s) realloc(d,s)
#define PFREE(p)   free(p)
#define PWCSDUP(s) _wcsdup(s)
#define PSTRDUP(s) strdup(s)

#define PINIT()    ((void) 0)
#define PEXIT()    ((void) 0)
#define PDESCTHREAD(n,c) ((void) 0)
#define PDUMP(f)   ((void) 0)
#endif

BEGIN_C

#ifdef _DEBUG
KHMEXP void KHMAPI
perf_init(void);

KHMEXP void KHMAPI
perf_exit(void);
#endif

KHMEXP void
perf_set_thread_desc(const char * file, int line,
                     const wchar_t * name, const wchar_t * creator);

END_C

#ifdef __cplusplus

#ifdef _DEBUG
#define PNEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
#else
#define PNEW new
#endif

#endif

#endif
