/*
 * Copyright (c) 2009 Secure Endpoints Inc.
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

#include "kmminternal.h"
#include <wintrust.h>

khm_boolean verify_module_signatures = FALSE;

static const wchar_t * last_component_of(const wchar_t * path)
{
    const wchar_t * slash = wcsrchr(path, L'\\');
    const wchar_t * slash2;

    if (slash == NULL)
        slash = path;
    else
        slash++;

    slash2 = wcsrchr(slash, L'/');
    if (slash2 == NULL)
        return slash;
    return ++slash2;
}

static khm_boolean
get_full_path_for_module(const wchar_t * modname, wchar_t *path, size_t cch)
{
    HMODULE hm = NULL;
    DWORD d;
    khm_boolean rv = FALSE;

    if (!GetModuleHandleEx(0, modname, &hm)) {
        _report_mr1(KHERR_WARNING, MSG_MOD_VERIFY_FAILED_NP, _cstr((modname)? modname: L"executable"));
        _suggest_mr(MSG_MOD_VF_NOT_FOUND, KHERR_SUGGEST_ABORT);
        _resolve();
        return FALSE;
    }

    d = GetModuleFileName(hm, path, cch);
    if (d == 0 || d == cch) {
        _report_mr2(KHERR_WARNING, MSG_MOD_VERIFY_FAILED, _cstr(last_component_of(path)),
                    _cstr(path));
        _suggest_mr(MSG_MOD_VF_NAME_TOO_LONG, KHERR_SUGGEST_ABORT);
        rv = FALSE;
    } else {
        rv = TRUE;
    }

    FreeLibrary(hm);
    return rv;
}

khm_boolean
kmmint_verify_trust_by_module_name(const wchar_t * modname)
{
    wchar_t path[MAX_PATH];

    return
        get_full_path_for_module(modname, path, ARRAYLENGTH(path)) &&
        kmmint_verify_trust_by_full_path(path);
}

khm_boolean
kmmint_verify_trust_by_full_path(const wchar_t * filename)
{
    WIN_TRUST_ACTDATA_CONTEXT_WITH_SUBJECT fContextWSubject;
    WIN_TRUST_SUBJECT_FILE fSubjectFile;
    GUID trustAction = WIN_SPUB_ACTION_PUBLISHED_SOFTWARE;
    GUID subject = WIN_TRUST_SUBJTYPE_PE_IMAGE;
    LONG ret;
    khm_boolean success = FALSE;

    LONG (WINAPI *pWinVerifyTrust)(HWND hWnd, GUID* pgActionID, WINTRUST_DATA* pWinTrustData) = NULL;
    HINSTANCE hWinTrust;

    if (filename == NULL) 
        return FALSE;

    hWinTrust = LoadLibrary(L"Wintrust");
    if ( !hWinTrust )
        return FALSE;

    if ((pWinVerifyTrust = (LONG (WINAPI *)(HWND, GUID*, WINTRUST_DATA*))
         GetProcAddress( hWinTrust, "WinVerifyTrust" )) == NULL ) {
        FreeLibrary(hWinTrust);
        return FALSE;
    }

    fSubjectFile.hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                    0, NULL);
    fSubjectFile.lpPath = filename;
    fContextWSubject.hClientToken = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                                FALSE, GetCurrentProcessId());
    fContextWSubject.SubjectType = &subject;
    fContextWSubject.Subject = &fSubjectFile;

    ret = (*pWinVerifyTrust)(INVALID_HANDLE_VALUE, &trustAction, (WINTRUST_DATA *)&fContextWSubject);

    if ( fSubjectFile.hFile != INVALID_HANDLE_VALUE )
        CloseHandle( fSubjectFile.hFile );
    if ( fContextWSubject.hClientToken != INVALID_HANDLE_VALUE )
        CloseHandle( fContextWSubject.hClientToken );

    if (ret == ERROR_SUCCESS) {
        success = TRUE;
    } else {
        DWORD gle = GetLastError();

        _report_mr3(KHERR_WARNING, MSG_MOD_VERIFY_FAILED,
                    _cstr(last_component_of(filename)),
                    _cstr(filename), _int32(gle));

        switch (gle) {
        case TRUST_E_PROVIDER_UNKNOWN:
            _suggest_mr(MSG_MOD_VF_PROVIDER_UNKNOWN, KHERR_SUGGEST_ABORT);
            break;
        case TRUST_E_NOSIGNATURE:
            _suggest_mr(MSG_MOD_VF_NO_SIGNATURE, KHERR_SUGGEST_ABORT);
            break;
        case TRUST_E_EXPLICIT_DISTRUST:
            _suggest_mr(MSG_MOD_VF_EXPLICIT_DISTRUST, KHERR_SUGGEST_ABORT);
            break;
        case TRUST_E_SUBJECT_NOT_TRUSTED:
            _suggest_mr(MSG_MOD_VF_SUBJECT_NOT_TRUSTED, KHERR_SUGGEST_ABORT);
            break;
        case TRUST_E_BAD_DIGEST:
            _suggest_mr(MSG_MOD_VF_BAD_DIGEST, KHERR_SUGGEST_ABORT);
            break;
        case CRYPT_E_SECURITY_SETTINGS:
            _suggest_mr(MSG_MOD_VF_SECURITY_SETTINGS, KHERR_SUGGEST_ABORT);
            break;
        default:
            _suggest_mr(MSG_MOD_VF_GENERIC, KHERR_SUGGEST_ABORT);
        }
        _resolve();
        success = FALSE;
    }
    FreeLibrary(hWinTrust);
    return success;
}

#define NIMEXE_NAME L"netidmgr.exe"
#ifdef _WIN32
#define NIMDLL_NAME L"nidmgr32.dll"
#else
#define NIMDLL_NAME L"nidmgr64.dll"
#endif

static khm_boolean
verify_core_module_paths(void)
{
    wchar_t p_exe[MAX_PATH];
    wchar_t p_dll[MAX_PATH];
    const wchar_t * p_last;

    if (!get_full_path_for_module(NULL, p_exe, ARRAYLENGTH(p_exe)) ||
        !get_full_path_for_module(NIMDLL_NAME, p_dll, ARRAYLENGTH(p_dll)))
        return FALSE;

    p_last = last_component_of(p_exe);
    if (p_last - p_exe != last_component_of(p_dll) - p_dll ||
        _wcsnicmp(p_exe, p_dll, p_last - p_exe) != 0) {

        kherr_report(KHERR_ERROR,
                     (const wchar_t *) MSG_MOD_VF_CORE_PATH,
                     (const wchar_t *) KHERR_FACILITY,
                     (const wchar_t *) NULL,
                     (const wchar_t *) MSG_MOD_VF_CORE_PATH_LONG,
                     (const wchar_t *) MSG_MOD_VF_CORE_PATH_SUG,
                     KHERR_FACILITY_ID,
                     KHERR_SUGGEST_ABORT,
                     _vnull(), _vnull(), _vnull(), _vnull(),
                     KHERR_RF_MSG_SHORT_DESC|
                     KHERR_RF_MSG_LONG_DESC|
                     KHERR_RF_MSG_SUGGEST,
                     KHERR_HMODULE);
    }
    return TRUE;
}

static khm_boolean
get_module_fixed_version(const wchar_t * module, VS_FIXEDFILEINFO * vinfo)
{
    wchar_t path[MAX_PATH];
    DWORD cb_verinfo;
    DWORD dummy = 1;
    BYTE * verinfo = NULL;
    khm_boolean rv = FALSE;
    VOID * lp_vinfo = NULL;
    UINT cb_vinfo = 0;

    if (!get_full_path_for_module(module, path, ARRAYLENGTH(path)))
        return FALSE;

    cb_verinfo = GetFileVersionInfoSize(path, &dummy);
    if (cb_verinfo == 0 || dummy != 0) {
        _report_mr1(KHERR_ERROR, MSG_RMI_NOT_FOUND, _dupstr(path));
        return FALSE;
    }

    verinfo = PMALLOC(cb_verinfo);
    if (!GetFileVersionInfo(path, 0, cb_verinfo, verinfo)) {
        _report_mr1(KHERR_ERROR, MSG_RMI_NOT_FOUND, _dupstr(path));
        return FALSE;
    }

    if (VerQueryValue(verinfo, L"\\", &lp_vinfo, &cb_vinfo) &&
        cb_vinfo == sizeof(*vinfo) && lp_vinfo != NULL) {
        wchar_t filever[24];
        wchar_t prodver[24];

        *vinfo = *((VS_FIXEDFILEINFO *) lp_vinfo);
        rv = TRUE;

        StringCbPrintf(filever, sizeof(filever), L"%d.%d.%d.%d",
                       HIWORD(vinfo->dwFileVersionMS),
                       LOWORD(vinfo->dwFileVersionMS),
                       HIWORD(vinfo->dwFileVersionLS),
                       LOWORD(vinfo->dwFileVersionLS));

        StringCbPrintf(prodver, sizeof(prodver), L"%d.%d.%d.%d",
                       HIWORD(vinfo->dwProductVersionMS),
                       LOWORD(vinfo->dwProductVersionMS),
                       HIWORD(vinfo->dwProductVersionLS),
                       LOWORD(vinfo->dwProductVersionLS));

        _report_mr4(KHERR_INFO, MSG_MOD_VERINFO,
                    _dupstr((module)? module : NIMEXE_NAME), _dupstr(filever), _dupstr(prodver),
                    _dupstr(path));
    } else {
        _report_mr1(KHERR_ERROR, MSG_RMI_NOT_FOUND, _dupstr(path));
    }

    PFREE(verinfo);
    return rv;
}

static khm_boolean
verify_core_module_versions(void)
{
    VS_FIXEDFILEINFO ffi_exe, ffi_dll;

    if (!get_module_fixed_version(NULL, &ffi_exe) ||
        !get_module_fixed_version(NIMDLL_NAME, &ffi_dll))
        return FALSE;

    if (ffi_exe.dwProductVersionMS != ffi_dll.dwProductVersionMS ||
        ffi_exe.dwProductVersionLS != ffi_dll.dwProductVersionLS ||
        ffi_exe.dwFileVersionMS != ffi_dll.dwFileVersionMS ||
        ffi_exe.dwFileVersionLS != ffi_dll.dwFileVersionLS) {

        kherr_report(KHERR_ERROR,
                     (const wchar_t *) MSG_MOD_VF_CORE_VER,
                     (const wchar_t *) KHERR_FACILITY,
                     (const wchar_t *) NULL,
                     (const wchar_t *) MSG_MOD_VF_CORE_VER_LONG,
                     (const wchar_t *) MSG_MOD_VF_CORE_VER_SUG,
                     KHERR_FACILITY_ID,
                     KHERR_SUGGEST_ABORT,
                     _vnull(), _vnull(), _vnull(), _vnull(),
                     KHERR_RF_MSG_SHORT_DESC|
                     KHERR_RF_MSG_LONG_DESC|
                     KHERR_RF_MSG_SUGGEST,
                     KHERR_HMODULE);
        return FALSE;
    }

    return TRUE;
}

static khm_boolean
verify_core_module_signatures(void)
{
    khm_boolean b_nim_module;
    khm_boolean b_nim_dll;

    b_nim_module = kmmint_verify_trust_by_module_name(NULL);

    b_nim_dll = kmmint_verify_trust_by_module_name(NIMDLL_NAME);

    verify_module_signatures = (verify_module_signatures && b_nim_module && b_nim_dll);

    if ((b_nim_module && !b_nim_dll) ||
        (!b_nim_module && b_nim_dll)) {

        kherr_report(KHERR_ERROR,
                     (const wchar_t *) MSG_MOD_VF_CORE_SIG,
                     (const wchar_t *) KHERR_FACILITY,
                     (const wchar_t *) NULL,
                     (const wchar_t *) MSG_MOD_VF_CORE_SIG_LONG,
                     (const wchar_t *) MSG_MOD_VF_CORE_SIG_SUG,
                     KHERR_FACILITY_ID,
                     KHERR_SUGGEST_ABORT,
                     _vnull(), _vnull(), _vnull(), _vnull(),
                     KHERR_RF_MSG_SHORT_DESC|
                     KHERR_RF_MSG_LONG_DESC|
                     KHERR_RF_MSG_SUGGEST,
                     KHERR_HMODULE);

        return FALSE;

    } else {

        /* We don't consider the case where none of the core binaries
           are signed as being an error condition. */
        return TRUE;
    }
}

khm_boolean
kmmint_verify_core_modules(void)
{
    return
        verify_core_module_paths() &&
        verify_core_module_versions() &&
        verify_core_module_signatures();
}
