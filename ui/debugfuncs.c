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

#include<tchar.h>

#include<shlwapi.h>
#include "khmapp.h"

#include<stdio.h>
#include<share.h>
#include<process.h>

#if DEBUG
#include<assert.h>
#endif

#define LOGFILENAME "nidmdbg.log"

static CRITICAL_SECTION cs_dbg;
FILE * logfile = NULL;
BOOL log_started = FALSE;

wchar_t *
severity_string(kherr_severity severity) {
    switch(severity) {
    case KHERR_FATAL:
	return L"FATAL";

    case KHERR_ERROR:
	return L"ERROR";

    case KHERR_WARNING:
	return L"Warning";

    case KHERR_INFO:
	return L"Info";

    case KHERR_DEBUG_3:
	return L"Debug(3)";

    case KHERR_DEBUG_2:
	return L"Debug(2)";

    case KHERR_DEBUG_1:
	return L"Debug(1)";

    case KHERR_NONE:
	return L"(None)";

    default:
	return L"(Unknown severity)";
    }
}

void
fprint_systime(FILE * f, SYSTEMTIME *psystime) {
    fprintf(logfile,
            "%d-%d-%d %02d:%02d:%02d.%03d",

            (int) psystime->wYear,
            (int) psystime->wMonth,
            (int) psystime->wDay,

            (int) psystime->wHour,
            (int) psystime->wMinute,
            (int) psystime->wSecond,
            (int) psystime->wMilliseconds);
}

void KHMAPI
debug_event_handler(enum kherr_ctx_event e,
		    kherr_context * c) {
    kherr_event * evt;

    EnterCriticalSection(&cs_dbg);

    if (!logfile)
	goto _done;

    if (e == KHERR_CTX_BEGIN) {
        SYSTEMTIME systime;

        GetSystemTime(&systime);
	fprintf(logfile,
		"%d\t",
		c->serial);

        fprint_systime(logfile, &systime);

        fprintf(logfile,
                "\t<< Context begin --\n");

    } else if (e == KHERR_CTX_DESCRIBE) {

        LeaveCriticalSection(&cs_dbg);
	evt = kherr_get_desc_event(c);
	if (evt) {
	    kherr_evaluate_event(evt);
            EnterCriticalSection(&cs_dbg);
	    fprintf(logfile,
		    "%d\t  Description: %S\n",
		    c->serial,
		    (evt->long_desc)? evt->long_desc: evt->short_desc);
	} else {
            EnterCriticalSection(&cs_dbg);
        }
    } else if (e == KHERR_CTX_END) {
        SYSTEMTIME systime;

	fprintf(logfile,
		"%d\t",
		c->serial);

        GetSystemTime(&systime);
        fprint_systime(logfile, &systime);

        fprintf(logfile,
                "\t>> Context end --\n");

    } else if (e == KHERR_CTX_EVTCOMMIT) {
        LeaveCriticalSection(&cs_dbg);
	evt = kherr_get_last_event(c);
	if (evt && (evt->short_desc || evt->long_desc)) {
	    SYSTEMTIME systime;

	    kherr_evaluate_event(evt);
	    FileTimeToSystemTime(&evt->time_ft, &systime);

            EnterCriticalSection(&cs_dbg);
	    fprintf(logfile,
		    "%d[%d](%S)\t",
		    c->serial,
		    evt->thread_id,
		    (evt->facility ? evt->facility: L""));

            fprint_systime(logfile, &systime);

            fprintf(logfile,
                    "\t%S: %S %S%S%S %S%S%S\n",

		    severity_string(evt->severity),

		    (evt->short_desc ? evt->short_desc: L""),

		    (evt->short_desc ? L"(":L""),
		    (evt->long_desc ? evt->long_desc: L""),
		    (evt->short_desc ? L")":L""),

		    (evt->suggestion ? L"[":L""),
		    (evt->suggestion ? evt->suggestion: L""),
		    (evt->suggestion ? L"]":L"")
		    );
	} else {
            EnterCriticalSection(&cs_dbg);
        }
    }

 _done:
    fflush(logfile);

    LeaveCriticalSection(&cs_dbg);
}

void khm_get_file_log_path(khm_size cb_buf, wchar_t * buf) {
#ifdef DEBUG
    assert(cb_buf > sizeof(wchar_t));
#endif
    *buf = L'\0';

    GetTempPath((DWORD) cb_buf / sizeof(wchar_t), buf);

    StringCbCat(buf, cb_buf, _T(LOGFILENAME));
}

void khm_start_file_log(void) {
    wchar_t temppath[MAX_PATH];
    khm_handle cs_cw = NULL;
    khm_int32 t = 0;

    if (log_started)
	goto _done;

    if (KHM_FAILED(khc_open_space(NULL, L"CredWindow", 0, &cs_cw)))
	goto _done;

    if (KHM_FAILED(khc_read_int32(cs_cw, L"LogToFile", &t)) ||
	!t)
	goto _done;

    EnterCriticalSection(&cs_dbg);

    khm_get_file_log_path(sizeof(temppath), temppath);

    logfile = _wfsopen(temppath, L"w", _SH_DENYWR);
    kherr_add_ctx_handler(debug_event_handler,
			  KHERR_CTX_BEGIN |
			  KHERR_CTX_END |
			  KHERR_CTX_DESCRIBE |
			  KHERR_CTX_EVTCOMMIT,
			  0);

    log_started = TRUE;

    LeaveCriticalSection(&cs_dbg);

 _done:
    if (cs_cw)
	khc_close_space(cs_cw);
}

void khm_stop_file_log(void) {

    if (!log_started)
	goto _done;

    kherr_remove_ctx_handler(debug_event_handler, 0);

    EnterCriticalSection(&cs_dbg);

    if (logfile)
	fclose (logfile);
    logfile = NULL;

    log_started = FALSE;

    LeaveCriticalSection(&cs_dbg);

 _done:
    ;
}

static HANDLE h_ods_kill = NULL;
static HANDLE h_ods_mmap = NULL;
static HANDLE h_ods_buffer_ready = NULL;
static HANDLE h_ods_data_ready = NULL;
static DWORD  dw_proc_id = 0;

static unsigned __stdcall ods_collector(void * param)
{
    LPVOID * dbg = NULL;
    HANDLE handles[2];

    if (!log_started)
        return 0;

    h_ods_buffer_ready = CreateEvent(NULL, FALSE, FALSE, L"DBWIN_BUFFER_READY");
    h_ods_data_ready = CreateEvent(NULL, FALSE, FALSE, L"DBWIN_DATA_READY");

    if (h_ods_buffer_ready == NULL || h_ods_data_ready == NULL) {
        EnterCriticalSection(&cs_dbg);
        fprintf(logfile, "Can't create DBWIN_BUFFER_READY event or DBWIN_DATA_READY event.\n");
        LeaveCriticalSection(&cs_dbg);
        goto cleanup;
    }

    h_ods_mmap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, L"DBWIN_BUFFER");
    if (h_ods_mmap == NULL) {
        EnterCriticalSection(&cs_dbg);
        fprintf(logfile, "Can't create memory mapped file DBWIN_BUFFER.  GLE=%d\n", GetLastError());
        LeaveCriticalSection(&cs_dbg);
        goto cleanup;
    }

    dbg = MapViewOfFile(h_ods_mmap, FILE_MAP_READ, 0, 0, 4096);
    if (dbg == NULL) {
        EnterCriticalSection(&cs_dbg);
        fprintf(logfile, "Can't map view of debug shared memory mapping GLE=%d\n", GetLastError());
        LeaveCriticalSection(&cs_dbg);
        goto cleanup;
    }

    handles[0] = h_ods_data_ready;
    handles[1] = h_ods_kill;

    do {
        DWORD o;

        SetEvent(h_ods_buffer_ready);
        o = WaitForMultipleObjects(2, handles, FALSE, INFINITE);

        if (o == WAIT_OBJECT_0) {
            DWORD thread_id = *((DWORD *) dbg);
            char *text = (char *)(&((DWORD *) dbg)[1]);
            size_t len;

            if (thread_id != dw_proc_id)
                continue;       /* We are only listening for our own
                                   debug messages for now. */

            EnterCriticalSection(&cs_dbg);
            fprintf(logfile, "[%d]DBG %.4091s", thread_id, text);
            if (FAILED(StringCchLengthA(text, 4096 - sizeof(DWORD), &len)) ||
                len == 0 || text[len - 1] != '\n')
                fprintf(logfile, "\n");
            fflush(logfile);
            LeaveCriticalSection(&cs_dbg);
        } else if (o == WAIT_OBJECT_0 + 1) {
            break;
        } else if (o == WAIT_FAILED) {
            break;
        } else {
            continue;
        }
    } while(TRUE);

 cleanup:
    if (dbg != NULL)
        UnmapViewOfFile(dbg);
    if (h_ods_buffer_ready)
        CloseHandle(h_ods_buffer_ready);
    if (h_ods_data_ready)
        CloseHandle(h_ods_data_ready);
    if (h_ods_mmap)
        CloseHandle(h_ods_mmap);

    return 0;
}

static HANDLE thrd_ods = NULL;

static void start_ods_collector(void)
{
    h_ods_kill = CreateEvent(NULL, FALSE, FALSE, NULL);
    dw_proc_id = GetCurrentProcessId();
    thrd_ods = (HANDLE) _beginthreadex(NULL, 8192, ods_collector, NULL, 0, NULL);
}

static void end_ods_collector(void)
{
    SetEvent(h_ods_kill);
    WaitForSingleObject(thrd_ods, INFINITE);
    CloseHandle(thrd_ods);
    CloseHandle(h_ods_kill);
    thrd_ods = NULL;
    h_ods_kill = NULL;
}

void khm_init_debug(void) {
    InitializeCriticalSection(&cs_dbg);

    khm_start_file_log();
    start_ods_collector();
}

void khm_exit_debug(void) {
    end_ods_collector();
    khm_stop_file_log();

    DeleteCriticalSection(&cs_dbg);
}
