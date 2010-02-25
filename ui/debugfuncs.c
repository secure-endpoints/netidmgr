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

#include <tchar.h>

#include <shlwapi.h>
#include "khmapp.h"
#include "netidmgr_intver.h"
#include <ntsecapi.h>

#include<stdio.h>
#include<share.h>
#include<process.h>

#include<assert.h>

#define LOGFILENAME "nidmdbg.log"

/* Note: Due to the OutputDebugString() capture code using cs_dbg and
   the fact that any code what-so-ever could be generating debug
   output, any code protected using cs_dbg MUST NOT obtain any other
   critical section. */
static CRITICAL_SECTION cs_dbg;

FILE * logfile = NULL;
BOOL log_started = FALSE;

static wchar_t *
severity_string(kherr_severity severity) {
    switch(severity) {
    case KHERR_FATAL:
	return L"FATAL:";

    case KHERR_ERROR:
	return L"ERROR:";

    case KHERR_WARNING:
	return L"Warning:";

    case KHERR_INFO:
	return L"Info:";

    case KHERR_DEBUG_3:
	return L"Debug(3):";

    case KHERR_DEBUG_2:
	return L"Debug(2):";

    case KHERR_DEBUG_1:
	return L"Debug(1):";

    case KHERR_NONE:
	return L"";

    default:
	return L"(Unknown severity):";
    }
}

static void
fwprint_systime(FILE * f, SYSTEMTIME *psystime, BOOL full) {
    if (full)
        fwprintf(logfile,
                 L"%d-%d-%d ",
                 (int) psystime->wYear,
                 (int) psystime->wMonth,
                 (int) psystime->wDay);

    fwprintf(logfile,
             L"%02d:%02d:%02d.%03d ",

             (int) psystime->wHour,
             (int) psystime->wMinute,
             (int) psystime->wSecond,
             (int) psystime->wMilliseconds);
}

static void KHMAPI
debug_event_handler(enum kherr_ctx_event e,
		    kherr_context * c) {
    kherr_event * evt;

    EnterCriticalSection(&cs_dbg);

    if (!logfile)
	goto _done;

    if (e == KHERR_CTX_BEGIN) {
        ;                       /* Nothing to do here.  We don't
                                   report context begin events until
                                   we have a description. */
    } else if (e == KHERR_CTX_DESCRIBE) {
        LeaveCriticalSection(&cs_dbg);
	evt = kherr_get_desc_event(c);
	if (evt) {
            FILETIME ltime;
            SYSTEMTIME systime;

	    kherr_evaluate_event(evt);
            FileTimeToLocalFileTime(&evt->time_ft, &ltime);
            FileTimeToSystemTime(&ltime, &systime);

            EnterCriticalSection(&cs_dbg);
            fwprint_systime(logfile, &systime, FALSE);
	    fwprintf(logfile,
                     L"[%d] Begin: %s\n",
                     c->serial,
                     (evt->long_desc)? evt->long_desc: evt->short_desc);
	} else {
            EnterCriticalSection(&cs_dbg);
        }
    } else if (e == KHERR_CTX_END) {
        SYSTEMTIME systime;

        GetLocalTime(&systime);
        fwprint_systime(logfile, &systime, FALSE);
        fwprintf(logfile, L"[%d] End\n", c->serial);

    } else if (e == KHERR_CTX_EVTCOMMIT) {
        LeaveCriticalSection(&cs_dbg);
	evt = kherr_get_last_event(c);
	if (evt && (evt->short_desc || evt->long_desc)) {
	    SYSTEMTIME systime;
            FILETIME ltime;

	    kherr_evaluate_event(evt);
            FileTimeToLocalFileTime(&evt->time_ft, &ltime);
	    FileTimeToSystemTime(&ltime, &systime);

            EnterCriticalSection(&cs_dbg);

            fwprint_systime(logfile, &systime, FALSE);
	    fwprintf(logfile,
                     L"%d[%d] %s",
                     evt->thread_id,
                     c->serial,
                     severity_string(evt->severity));

            fwprintf(logfile,
                     L"%s%s%s%s%s%s%s%s%s%s\n",

                     (evt->facility ? L"(": L""),
                     (evt->facility ? evt->facility : L""),
                     (evt->facility ? L") ": L" "),

                     (evt->short_desc ? evt->short_desc: L""),

                     (evt->short_desc ? L" (":L""),
                     (evt->long_desc ? evt->long_desc: L""),
                     (evt->short_desc ? L")":L""),

                     (evt->suggestion ? L" [":L""),
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
    assert(cb_buf > sizeof(wchar_t));
    *buf = L'\0';

    GetTempPath((DWORD) cb_buf / sizeof(wchar_t), buf);

    StringCbCat(buf, cb_buf, _T(LOGFILENAME));
}


static BOOL
IsUserAdmin(VOID)
{
    BOOL b;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;

    b = AllocateAndInitializeSid(&NtAuthority,
                                 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0,
                                 &AdministratorsGroup);
    if(b) {
        if (!CheckTokenMembership( NULL, AdministratorsGroup, &b)) {
            b = FALSE;
        }
        FreeSid(AdministratorsGroup);
    }

    return(b);
}


static void
log_logon_session_data(FILE * f)
{
    PSECURITY_LOGON_SESSION_DATA pSessionData = NULL;
    NTSTATUS Status = 0;
    HANDLE  TokenHandle = NULL;
    TOKEN_STATISTICS Stats;
    PTOKEN_PRIVILEGES pPrivs = NULL;
    DWORD   ReqLen = 0;
    BOOL    Success;
    DWORD i;

    Success = OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &TokenHandle );
    if ( !Success ) {
        fwprintf(f, L"        GetCurrentProcess() failed.  GLE=%d\n", GetLastError());
        goto cleanup;
    }

    Success = GetTokenInformation( TokenHandle, TokenPrivileges, NULL, 0, &ReqLen );
    if ( Success || GetLastError() != ERROR_INSUFFICIENT_BUFFER || ReqLen == 0 ) {
        fwprintf(f, L"        GetTokenInformation() failed.  GLE=%d\n", GetLastError());
        goto cleanup;
    }

    pPrivs = PMALLOC(ReqLen);
    Success = GetTokenInformation( TokenHandle, TokenPrivileges, pPrivs, ReqLen, &ReqLen );
    if ( !Success ) {
        fwprintf(f, L"        GetTokenInformation() failed.  GLE=%d\n", GetLastError());
        goto cleanup;
    }

    Success = GetTokenInformation( TokenHandle, TokenStatistics,
                                   &Stats, sizeof(TOKEN_STATISTICS), &ReqLen );
    if ( !Success ) {
        fwprintf(f, L"        GetTokenInformation() failed(2).  GLE=%d\n", GetLastError());
        goto cleanup;;
    }

    Status = LsaGetLogonSessionData( &Stats.AuthenticationId, &pSessionData );
    if ( FAILED(Status) || !pSessionData ) {
        fwprintf(f, L"        LsaGetLogonSessionData() failed.  GLE=%d\n", GetLastError());
        goto cleanup;
    }

    fwprintf(f, L"        Logon session: [%.*s]@[%.*s]\n",
             pSessionData->UserName.Length / sizeof(wchar_t),
             pSessionData->UserName.Buffer,
             pSessionData->LogonDomain.Length / sizeof(wchar_t),
             pSessionData->LogonDomain.Buffer);

    fwprintf(f, L"        UPN: [%.*s]\n",
             pSessionData->Upn.Length / sizeof(wchar_t),
             pSessionData->Upn.Buffer);

    fwprintf(f, L"        Logon server: [%.*s] @[%.*s]\n",
             pSessionData->LogonServer.Length / sizeof(wchar_t),
             pSessionData->LogonServer.Buffer,
             pSessionData->DnsDomainName.Length / sizeof(wchar_t),
             pSessionData->DnsDomainName.Buffer);

    fwprintf(f, L"        Authentication package: [%.*s]\n",
             pSessionData->AuthenticationPackage.Length,
             pSessionData->AuthenticationPackage.Buffer);

    fwprintf(f, L"        Authentication package: [%.*s], Logon Type: %s\n",
             pSessionData->AuthenticationPackage.Length,
             pSessionData->AuthenticationPackage.Buffer,
             (pSessionData->LogonType == Interactive)? L"Interactive":
             (pSessionData->LogonType == Network)? L"Network":
             (pSessionData->LogonType == Batch)? L"Batch":
             (pSessionData->LogonType == Service)? L"Service":
             (pSessionData->LogonType == Proxy)? L"Proxy":
             (pSessionData->LogonType == Unlock)? L"Unlock":
             (pSessionData->LogonType == NetworkCleartext)? L"NetworkCleartext":
             (pSessionData->LogonType == NewCredentials)? L"NewCredentials":
#if (_WIN32_WINNT >= 0x0501)
             (pSessionData->LogonType == RemoteInteractive)? L"RemoteInteractive":
             (pSessionData->LogonType == CachedInteractive)? L"CachedInteractive":
#endif
#if (_WIN32_WINNT >= 0x0502)
             (pSessionData->LogonType == CachedRemoteInteractive)? L"CachedRemoteInteractive":
             (pSessionData->LogonType == CachedUnlock)? L"CachedUnlock":
#endif
             L"Other/Unknown");

    fwprintf(f, L"        Token privileges:\n");
    for (i = 0; i < pPrivs->PrivilegeCount; i++) {
        wchar_t pName[128];
        DWORD cch;

        cch = ARRAYLENGTH(pName);
        if (LookupPrivilegeName(NULL, &pPrivs->Privileges[i].Luid, pName, &cch)) {
            fwprintf(f, L"         : %s (%s)\n", pName,
                     (pPrivs->Privileges[i].Attributes & (SE_PRIVILEGE_ENABLED_BY_DEFAULT |
                                                          SE_PRIVILEGE_ENABLED)) ? L"Enabled" : L"Disabled");
        }
    }

    fwprintf(f, L"        Process tokens %s admin privileges.\n",
             (IsUserAdmin())? L"HAVE": L"have no");

 cleanup:
    if ( TokenHandle )
        CloseHandle( TokenHandle );

    if ( pPrivs )
        PFREE( pPrivs );

    if ( pSessionData )
        LsaFreeReturnBuffer(pSessionData);
}


static void
fwprint_prologue(FILE * f)
{
    SYSTEMTIME systime;

    GetLocalTime(&systime);

    fwprintf(f, L"Logging started for Network Identity Manager at ");
    fwprint_systime(f, &systime, TRUE);
    fwprintf(f, L"\n");
    fwprintf(f, L"Product: %s\n", _T(KH_VERSTR_PRODUCT_1033));
    fwprintf(f, L"         %s\n", _T(KH_VERSTR_COPYRIGHT_1033));
#ifdef KH_VERSTR_BUILDINFO_1033
    fwprintf(f, L"         %s\n", _T(KH_VERSTR_BUILDINFO_1033));
#endif

    log_logon_session_data(f);

    fwprint_systime(f, &systime, FALSE);
    fwprintf(f, L"Begin logging\n");
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

    fwprint_prologue(logfile);

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

    PDESCTHREAD(L"OutputDebugString() collector", L"NIM");

    if (!log_started)
        return 0;

    h_ods_buffer_ready = CreateEvent(NULL, FALSE, FALSE, L"DBWIN_BUFFER_READY");
    h_ods_data_ready = CreateEvent(NULL, FALSE, FALSE, L"DBWIN_DATA_READY");

    if (h_ods_buffer_ready == NULL || h_ods_data_ready == NULL) {
        EnterCriticalSection(&cs_dbg);
        fwprintf(logfile, L"Can't create DBWIN_BUFFER_READY event or DBWIN_DATA_READY event.\n");
        LeaveCriticalSection(&cs_dbg);
        goto cleanup;
    }

    h_ods_mmap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, L"DBWIN_BUFFER");
    if (h_ods_mmap == NULL) {
        EnterCriticalSection(&cs_dbg);
        fwprintf(logfile, L"Can't create memory mapped file DBWIN_BUFFER.  GLE=%d\n", GetLastError());
        LeaveCriticalSection(&cs_dbg);
        goto cleanup;
    }

    dbg = MapViewOfFile(h_ods_mmap, FILE_MAP_READ, 0, 0, 4096);
    if (dbg == NULL) {
        EnterCriticalSection(&cs_dbg);
        fwprintf(logfile, L"Can't map view of debug shared memory mapping GLE=%d\n", GetLastError());
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
            DWORD proc_id = *((DWORD *) dbg);
            char *text = (char *)(&((DWORD *) dbg)[1]);
            size_t len;
            SYSTEMTIME systime;

            if (proc_id != dw_proc_id)
                continue;       /* We are only listening for our own
                                   debug messages for now. */

            GetLocalTime(&systime);

            EnterCriticalSection(&cs_dbg);
            fwprint_systime(logfile, &systime, FALSE);
            fwprintf(logfile, L"[DBG] %.4091hs", text);
            if (FAILED(StringCchLengthA(text, 4096 - sizeof(DWORD), &len)) ||
                len == 0 || text[len - 1] != '\n')
                fwprintf(logfile, L"\n");
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
