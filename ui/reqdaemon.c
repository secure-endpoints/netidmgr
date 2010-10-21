/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
 * Copyright (c) 2007-2010 Secure Endpoints Inc.
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

#include "khmapp.h"
#include<process.h>
#include<assert.h>

#define KRB5_MAXCCH_CCNAME 1024

ATOM reqdaemon_atom = 0;
HANDLE reqdaemon_thread = NULL;
HWND reqdaemon_hwnd = NULL;

struct cache_data {
    khm_int32 result;
    wchar_t idname[KCDB_IDENT_MAXCCH_NAME];
    wchar_t ccname[KRB5_MAXCCH_CCNAME];
};

static void
pick_cache(khui_new_creds * nc, void * data)
{
    struct cache_data * d = (struct cache_data *) data;
    khm_size i;

    khui_cw_lock_nc(nc);

    d->result = nc->result;
    d->idname[0] = L'\0';
    d->ccname[0] = L'\0';

    if (d->result == KHUI_NC_RESULT_PROCESS) {
        for (i = 0; i < nc->n_identities; i++) {
            khm_size cb;
            khm_handle ident;

            if (!kcdb_identity_by_provider(nc->identities[i], L"Krb5Ident"))
                continue;

            ident = nc->identities[i];

            cb = sizeof(d->idname);
            kcdb_identity_get_attr(ident, KCDB_ATTR_DISPLAY_NAME, NULL, d->idname, &cb);

            cb = sizeof(d->ccname);
            kcdb_identity_get_attrib(ident, L"Krb5CCName", NULL, d->ccname, &cb);

            break;
        }
    }

    khui_cw_unlock_nc(nc);
}

LRESULT CALLBACK
reqdaemonwnd_proc(HWND hwnd,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam) {

    switch(uMsg) {
    case WM_CREATE:
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        reqdaemon_hwnd = NULL;
        PostQuitMessage(0);
        break;

        /* Leash compatibility */
    case ID_OBTAIN_TGT_WITH_LPARAM:
        {
            wchar_t wmapping[ARRAYLENGTH(KHUI_REQD_MAPPING_FORMAT) + 10];
            khm_handle identity = NULL;
            LPNETID_DLGINFO pdlginfo;
            LRESULT lr = 1;
            HANDLE hmap = NULL;
            HRESULT hr;
            struct cache_data cdata;

            hr = StringCbPrintf(wmapping, sizeof(wmapping),
                                KHUI_REQD_MAPPING_FORMAT, (DWORD) lParam);
            assert(SUCCEEDED(hr));

            memset(&cdata, 0, sizeof(cdata));

            hmap = CreateFileMapping(INVALID_HANDLE_VALUE,
                                     NULL,
                                     PAGE_READWRITE,
                                     0, 4096,
                                     wmapping);

            if (hmap == NULL) {
                return -1;
            } else if (hmap != NULL && GetLastError() != ERROR_ALREADY_EXISTS) {
                CloseHandle(hmap);
                return -1;
            }

            pdlginfo = MapViewOfFile(hmap,
                                     FILE_MAP_WRITE,
                                     0, 0,
                                     sizeof(*pdlginfo));

            if (pdlginfo == NULL) {
                CloseHandle(hmap);
                return 1;
            }

            if (pdlginfo->in.username[0] &&
                pdlginfo->in.realm[0] &&
                SUCCEEDED(StringCbPrintf(cdata.idname,
                                         sizeof(cdata.idname),
                                         L"%s@%s",
                                         pdlginfo->in.username,
                                         pdlginfo->in.realm))) {

                kcdb_identity_create(cdata.idname,
                                     KCDB_IDENT_FLAG_CREATE,
                                     &identity);
            }

            do {
                if (khm_cred_is_in_dialog()) {
                    khm_cred_wait_for_dialog(INFINITE, NULL, NULL);
                }

                khui_context_set_ex(KHUI_SCOPE_IDENT,
                                    identity,
                                    KCDB_CREDTYPE_INVALID,
                                    NULL, NULL, 0, NULL,
                                    pdlginfo, sizeof(*pdlginfo));

                if (pdlginfo->dlgtype == NETID_DLGTYPE_TGT)
                    SendMessage(khm_hwnd_main, WM_COMMAND,
                                MAKEWPARAM(KHUI_ACTION_NEW_CRED, 0), 0);
                else if (pdlginfo->dlgtype == NETID_DLGTYPE_CHPASSWD)
                    SendMessage(khm_hwnd_main, WM_COMMAND,
                                MAKEWPARAM(KHUI_ACTION_PASSWD_ID, 0), 0);
                else
                    break;

                memset(&cdata, 0, sizeof(cdata));

                if (KHM_FAILED(khm_cred_wait_for_dialog(INFINITE, pick_cache, &cdata)))
                    continue;
                else {
                    lr = (cdata.result != KHUI_NC_RESULT_PROCESS);
                    break;
                }
            } while(TRUE);

            assert(lr || pdlginfo->dlgtype != NETID_DLGTYPE_TGT ||
                   cdata.idname[0]);

            if (!lr && pdlginfo->dlgtype == NETID_DLGTYPE_TGT &&
                cdata.idname[0]) {
                wchar_t * atsign;

                atsign = wcsrchr(cdata.idname, L'@');

                if (atsign == NULL)
                    goto _exit;

                StringCbCopy(pdlginfo->out.ccache, sizeof(pdlginfo->out.ccache),
                             cdata.ccname);

                *atsign++ = 0;

                StringCbCopy(pdlginfo->out.username,
                             sizeof(pdlginfo->out.username),
                             cdata.idname);

                StringCbCopy(pdlginfo->out.realm,
                             sizeof(pdlginfo->out.realm),
                             atsign);
            }

        _exit:

            if (pdlginfo)
                UnmapViewOfFile(pdlginfo);
            if (hmap)
                CloseHandle(hmap);
            if (identity)
                kcdb_identity_release(identity);

            return lr;
        }

#ifdef DEPRECATED_REMOTE_CALL
        /* deprecated */
    case ID_OBTAIN_TGT_WITH_LPARAM:
        {
            char * param = (char *) GlobalLock((HGLOBAL) lParam);
            char * username = NULL;
            char * realm = NULL;
            char * title = NULL;
            char * ccache = NULL;
            wchar_t widname[KCDB_IDENT_MAXCCH_NAME];
            wchar_t wtitle[KHUI_MAXCCH_TITLE];
            size_t cch;
            khm_int32 rv = KHM_ERROR_SUCCESS;
            khm_handle identity = NULL;
            NETID_DLGINFO dlginfo;

            if (param) {
                if (*param)
                    title = param;

                if (FAILED(StringCchLengthA(param, KHUI_MAXCCH_TITLE, &cch))) {
#ifdef DEBUG
                    assert(FALSE);
#endif
                    rv = KHM_ERROR_INVALID_PARAM;
                    goto _exit_tgt_with_lparam;
                }

                param += cch + 1;

                if (*param)
                    username = param;

                if (FAILED(StringCchLengthA(param, KCDB_IDENT_MAXCCH_NAME, &cch))) {
#ifdef DEBUG
                    assert(FALSE);
#endif
                    rv = KHM_ERROR_INVALID_PARAM;
                    goto _exit_tgt_with_lparam;
                }

                param += cch + 1;

                if (*param)
                    realm = param;

                if (FAILED(StringCchLengthA(param, KCDB_IDENT_MAXCCH_NAME, &cch))) {
#ifdef DEBUG
                    assert(FALSE);
#endif
                    rv = KHM_ERROR_INVALID_PARAM;
                    goto _exit_tgt_with_lparam;
                }

                param += cch + 1;

                if (*param)
                    ccache = param;
            }

            if (username && realm) {

                if (FAILED(StringCbPrintf(widname, sizeof(widname),
                                          L"%hs@%hs", username, realm))) {
                    rv = KHM_ERROR_INVALID_PARAM;
                    goto _exit_tgt_with_lparam;
                }

                rv = kcdb_identity_create(widname,
                                          KCDB_IDENT_FLAG_CREATE,
                                          &identity);
                if (KHM_FAILED(rv)) {
                    goto _exit_tgt_with_lparam;
                }
            }

            ZeroMemory(&dlginfo, sizeof(dlginfo));

            dlginfo.size = NETID_DLGINFO_V1_SZ;
            dlginfo.dlgtype = NETID_DLGTYPE_TGT;
            
            if (title)
                StringCbCopy(dlginfo.in.title, sizeof(dlginfo.in.title),
                             wtitle);
            if (username)
                AnsiStrToUnicode(dlginfo.in.username, sizeof(dlginfo.in.username),
                                 username);
            if (realm)
                AnsiStrToUnicode(dlginfo.in.realm, sizeof(dlginfo.in.realm),
                                 realm);

            if (ccache)
                AnsiStrToUnicode(dlginfo.in.ccache, sizeof(dlginfo.in.ccache),
                                 ccache);

            dlginfo.in.use_defaults = TRUE;

            do {
                if (khm_cred_is_in_dialog()) {
                    khm_cred_wait_for_dialog(INFINITE, NULL, NULL, 0);
                }

                khui_context_set_ex(KHUI_SCOPE_IDENT,
                                    identity,
                                    KCDB_CREDTYPE_INVALID,
                                    NULL,
                                    NULL,
                                    0,
                                    NULL,
                                    &dlginfo,
                                    sizeof(dlginfo));

                if (title) {
                    AnsiStrToUnicode(wtitle, sizeof(wtitle),
                                     title);

                    khm_cred_obtain_new_creds(wtitle);
                } else {
                    khm_cred_obtain_new_creds(NULL);
                }

                if (KHM_FAILED(khm_cred_wait_for_dialog(INFINITE, NULL, NULL, 0)))
                    continue;
                else
                    break;
            } while(TRUE);

        _exit_tgt_with_lparam:
            if (identity)
                kcdb_identity_release(identity);

            GlobalUnlock((HGLOBAL) lParam);
        }
        return 0;
#endif

    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

unsigned __stdcall
khm_reqdaemon_thread_proc(void * vparam) {
    BOOL rv;
    MSG msg;
#ifdef DEBUG
    DWORD dw;
#endif

    PDESCTHREAD(L"Remote Request Daemon", L"App");

    khm_register_reqdaemonwnd_class();

#ifdef DEBUG
    assert(reqdaemon_atom != 0);
#endif

    reqdaemon_hwnd = CreateWindowEx(0,
                                    MAKEINTATOM(reqdaemon_atom),
                                    KHUI_REQDAEMONWND_NAME,
                                    0,
                                    0,0,0,0,
                                    HWND_MESSAGE,
                                    NULL,
                                    khm_hInstance,
                                    NULL);

#ifdef DEBUG
    dw = GetLastError();
    assert(reqdaemon_hwnd != NULL);
#endif

    while(rv = GetMessage(&msg, NULL, 0, 0)) {
        if (rv == -1) {
#ifdef DEBUG
            assert(FALSE);
#endif
            break;
        } else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    reqdaemon_thread = NULL;

    khm_unregister_reqdaemonwnd_class();

    return 0;
}

void
khm_register_reqdaemonwnd_class(void) {
    WNDCLASSEX wcx;

    ZeroMemory(&wcx, sizeof(wcx));

    wcx.cbSize = sizeof(wcx);
    wcx.style = 0;
    wcx.lpfnWndProc = reqdaemonwnd_proc;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = 0;
    wcx.hInstance = khm_hInstance;
    wcx.hIcon = NULL;
    wcx.hCursor = NULL;
    wcx.hbrBackground = NULL;
    wcx.lpszMenuName = NULL;
    wcx.lpszClassName = KHUI_REQDAEMONWND_CLASS;
    wcx.hIconSm = NULL;

    reqdaemon_atom = RegisterClassEx(&wcx);

#ifdef DEBUG
    assert(reqdaemon_atom != 0);
#endif    
}

void
khm_unregister_reqdaemonwnd_class(void) {
    if (reqdaemon_atom != 0) {
        UnregisterClass(MAKEINTATOM(reqdaemon_atom), khm_hInstance);
        reqdaemon_atom = 0;
    }
}

void
khm_init_request_daemon(void) {
#ifdef DEBUG
    assert(reqdaemon_thread == NULL);
#endif

    reqdaemon_thread = (HANDLE) _beginthreadex(NULL, 16 * 4096,
                                               khm_reqdaemon_thread_proc,
                                               NULL, 0, NULL);

#ifdef DEBUG
    assert(reqdaemon_thread != NULL);
#endif    
}

void
khm_exit_request_daemon(void) {
    if (reqdaemon_hwnd == NULL)
        return;

    SendMessage(reqdaemon_hwnd, WM_CLOSE, 0, 0);
}
