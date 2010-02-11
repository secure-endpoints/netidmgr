/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
 * Copyright (c) 2006-2010 Secure Endpoints Inc.
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
#include<utils.h>
#include<netidmgr_intver.h>

#if defined(DEBUG) && (defined(KH_BUILD_PRIVATE) || defined(KH_BUILD_SPECIAL))
#define USE_FILE_LOG 1
#define DUMP_MEMORY_LEAKS 1
#endif

KHMEXP wchar_t *
perf_wcsdup(const char * file, int line, const wchar_t * str)
{
    return _wcsdup_dbg(str, _NORMAL_BLOCK, file, line);
}

KHMEXP char *
perf_strdup(const char * file, int line, const char * str)
{
    return _strdup_dbg(str, _NORMAL_BLOCK, file, line);
}

KHMEXP void *
perf_calloc(const char * file, int line, size_t num, size_t size)
{
    return _calloc_dbg(num, size, _NORMAL_BLOCK, file, line);
}

KHMEXP void * 
perf_malloc(const char * file, int line, size_t s)
{
    return _malloc_dbg(s, _NORMAL_BLOCK, file, line);
}

KHMEXP void *
perf_realloc(const char * file, int line, void * data, size_t s)
{
    return _realloc_dbg(data, s, _NORMAL_BLOCK, file, line);
}

KHMEXP void
perf_free  (void * b)
{
    _free_dbg(b, _NORMAL_BLOCK);
}


#ifdef _DEBUG

static HANDLE h_CrtReport = INVALID_HANDLE_VALUE;

KHMEXP void KHMAPI
perf_init(void) {
    int dbf = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

    dbf = (dbf & 0x0000ffff) | _CRTDBG_CHECK_EVERY_1024_DF;
    _CrtSetDbgFlag(dbf);

    _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG );

#ifdef USE_FILE_LOG
    {
        HANDLE hLog;

        hLog = CreateFile(L"crtlog.txt", GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE,
                          NULL);
        if (hLog != INVALID_HANDLE_VALUE) {
            _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
            _CrtSetReportFile( _CRT_WARN, hLog );
            _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
            _CrtSetReportFile( _CRT_ERROR, hLog );
            h_CrtReport = hLog;
        }
    }
#else

    _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );

#endif
}

KHMEXP void KHMAPI
perf_exit(void) {
#ifdef USE_FILE_LOG
    if (h_CrtReport != INVALID_HANDLE_VALUE) {
        _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG );
        _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG );
        CloseHandle(h_CrtReport);
        h_CrtReport = INVALID_HANDLE_VALUE;
    }
#endif
}

KHMEXP void KHMAPI
perf_dump() {
#ifdef DUMP_MEMORY_LEAKS
    _CrtDumpMemoryLeaks();
#endif
}

#endif

#define MS_VC_EXCEPTION 0x406D1388

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
   DWORD  dwType;
   LPCSTR szName;
   DWORD  dwThreadID;
   DWORD  dwFlags;
} THREADNAME_INFO;
#pragma pack(pop)

static void
set_thread_name(DWORD tid, const wchar_t * name)
{
    THREADNAME_INFO info;
    char cname[128];

    if (WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS,
                            name, -1,
                            cname, sizeof(cname),
                            NULL,
                            NULL) == 0)
        return;

   Sleep(10);
   info.dwType = 0x1000;
   info.szName = cname;
   info.dwThreadID = tid;
   info.dwFlags = 0;

   __try
   {
       RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
   }
}


KHMEXP void
perf_set_thread_desc(const char * file, int line,
                     const wchar_t * name, const wchar_t * creator) {

    _RPTW3(_CRT_WARN, L"Beginning thread: %s (ID: %d, by %s)",
           name, GetCurrentThreadId(), creator);
    _RPT2(_CRT_WARN, "  @%s:%d\n", file, line);
    set_thread_name((DWORD) -1, name);
}

