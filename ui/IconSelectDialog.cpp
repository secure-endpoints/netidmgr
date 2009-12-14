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

#define NO_STRSAFE
#include <shlwapi.h>
#include <shlobj.h>
#include "khmapp.h"
#include "IconSelectDialog.hpp"
#include "httpfetch.h"
#include <strsafe.h>
#include <assert.h>

namespace nim {

    IconSelectDialog::IconSelectDialog(Identity& _identity) :
        DialogWindow(MAKEINTRESOURCE(IDD_ICONSELECT),
                     khm_hInstance),
        m_identity(_identity),
        m_cropper(new PictureCropWindow(), RefCount::TakeOwnership)
    {
    }

    IconSelectDialog::~IconSelectDialog()
    {
    }

    BOOL IconSelectDialog::OnInitDialog(HWND hwndFocus, LPARAM lParam)
    {
        assert(!m_identity.IsNull());

        Edit_SetCueBannerText(GetItem(IDC_PATH), LoadStringResource(IDS_IS_CUE_FILE).c_str());
        Edit_LimitText(GetItem(IDC_PATH), MAX_PATH);

        Edit_SetCueBannerText(GetItem(IDC_URL), LoadStringResource(IDS_IS_CUE_URL).c_str());
        Edit_LimitText(GetItem(IDC_URL), MAXCCH_URL);

        Edit_SetCueBannerText(GetItem(IDC_DOMAIN), LoadStringResource(IDS_IS_CUE_DOM).c_str());
        Edit_LimitText(GetItem(IDC_DOMAIN), MAX_PATH);

        Edit_SetCueBannerText(GetItem(IDC_EMAIL), LoadStringResource(IDS_IS_CUE_EMAIL).c_str());
        Edit_LimitText(GetItem(IDC_EMAIL), MAXCCH_URL);

        if (m_identity.Exists(KCDB_ATTR_NAME_EMAIL))
            SetItemText(IDC_EMAIL, m_identity.GetAttribStringObj(KCDB_ATTR_NAME_EMAIL).c_str());
        else
            EnableItem(IDC_GRAVGO, FALSE);

        if (m_identity.Exists(KCDB_ATTR_NAME_DOMAIN))
            SetItemText(IDC_DOMAIN, m_identity.GetAttribStringObj(KCDB_ATTR_NAME_DOMAIN).c_str());
        else
            EnableItem(IDC_FAVGO, FALSE);

        {
            wchar_t title[64 + KCDB_IDENT_MAXCCH_NAME + KCDB_MAXCCH_NAME];

            StringCchPrintf(title, ARRAYLENGTH(title),
                            LoadStringResource(IDS_IS_TITLE_FMT).c_str(),
                            m_identity.GetResourceString(KCDB_RES_DISPLAYNAME).c_str(),
                            m_identity.GetResourceString(KCDB_RES_INSTANCE).c_str());
            SetWindowText(hwnd, title);
        }

        {
            RECT r_cropper;

            GetWindowRect(GetItem(IDC_IMGEDITOR), &r_cropper);
            MapWindowRect(NULL, hwnd, &r_cropper);

            m_cropper->Create(hwnd, RectFromRECT(&r_cropper));
        }

        {
            ConfigSpace csp = m_identity.GetConfig(0);
            if (!m_cropper->LoadImage(csp))
                m_cropper->SetImage(m_identity.GetResourceIcon(KCDB_RES_ICON_NORMAL));
        }

        return FALSE;
    }

    void IconSelectDialog::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
    {
        if (codeNotify == EN_CHANGE && id == IDC_EMAIL) {
            EnableItem(IDC_GRAVGO, (GetItemTextLength(IDC_EMAIL) > 0));
            return;
        }

        if (codeNotify == EN_CHANGE && id == IDC_DOMAIN) {
            EnableItem(IDC_FAVGO, (GetItemTextLength(IDC_EMAIL) > 0));
            return;
        }

        if (codeNotify == BN_CLICKED && id == IDC_BROWSE) {
            DoBrowse();
            return;
        }

        if (codeNotify == BN_CLICKED && id == IDC_URLGO) {
            DoFetchURL();
            return;
        }

        if (codeNotify == BN_CLICKED && id == IDC_FAVGO) {
            DoFetchFavicon();
            return;
        }

        if (codeNotify == BN_CLICKED && id == IDC_GRAVGO) {
            DoFetchGravatar();
            return;
        }

        if (codeNotify == BN_CLICKED && id == IDOK) {
            DoOk();
            return;
        }

        if (codeNotify == BN_CLICKED && id == IDCANCEL) {
            OnClose();
            return;
        }
    }

    void IconSelectDialog::OnDestroy(void)
    {
    }

    void IconSelectDialog::DoBrowse()
    {
        wchar_t path[MAX_PATH];

        if (khm_show_select_icon_dialog(hwnd, path, sizeof(path))) {

            if (!wcsncmp(path, KHUI_PREFIX_IMG, ARRAYLENGTH(KHUI_PREFIX_IMG) - 1)) {
                m_cropper->SetImage(path + (ARRAYLENGTH(KHUI_PREFIX_IMG) - 1));
            } else {
                HICON h = NULL;

                khui_load_icon_from_resource_path(path, 0, &h);
                if (h != NULL) {
                    m_cropper->SetImage(h);
                    DestroyIcon(h);
                }
            }
        }
    }

    void IconSelectDialog::DoFetchURL()
    {
        wchar_t url[MAXCCH_URL];
        wchar_t path_o[MAX_PATH] = L"";
        wchar_t path[MAX_PATH] = L"";
        HANDLE hFile = INVALID_HANDLE_VALUE;

        __try {
            if (!khm_get_temp_path(path_o, ARRAYLENGTH(path_o)))
                return;
            StringCchCopy(path, ARRAYLENGTH(path), path_o);

            if (GetItemText(IDC_URL, url, ARRAYLENGTH(url)) == 0)
                return;

            hFile = khm_get_image_from_url(url, path, ARRAYLENGTH(path));
            if (hFile == INVALID_HANDLE_VALUE) {
                // Show some sort of error message
                return;
            }

            CloseHandle(hFile);

            m_cropper->SetImage(path);
        }
        __finally {
            if (path_o[0] != L'\0')
                DeleteFile(path_o);
            if (path[0] != L'\0')
                DeleteFile(path);
        };
    }

    void IconSelectDialog::DoFetchFavicon()
    {
        wchar_t url[MAXCCH_URL];
        wchar_t path[MAX_PATH] = L"";
        HANDLE hFile = INVALID_HANDLE_VALUE;

        __try {
            if (!khm_get_temp_path(path, ARRAYLENGTH(path)))
                return;

            if (GetItemText(IDC_DOMAIN, url, ARRAYLENGTH(url)) == 0)
                return;

            hFile = khm_get_favicon_for_domain(url, path);
            if (hFile == INVALID_HANDLE_VALUE) {
                // Show some sort of error message
                return;
            }

            CloseHandle(hFile);

            m_cropper->SetImage(path);
        }
        __finally {
            if (path[0] != L'\0')
                DeleteFile(path);
        };
    }

    void IconSelectDialog::DoFetchGravatar()
    {
        wchar_t email[MAXCCH_URL];
        wchar_t path[MAX_PATH] = L"";
        HANDLE hFile = INVALID_HANDLE_VALUE;

        __try {
            if (!khm_get_temp_path(path, ARRAYLENGTH(path)))
                return;

            if (GetItemText(IDC_EMAIL, email, ARRAYLENGTH(email)) == 0)
                return;

            hFile = khm_get_gravatar_for_email(email, path);
            if (hFile == INVALID_HANDLE_VALUE) {
                // Show some sort of error message
                return;
            }

            CloseHandle(hFile);

            m_cropper->SetImage(path);
        }
        __finally {
            if (path[0] != L'\0')
                DeleteFile(path);
        };
    }

    bool IconSelectDialog::PrepareIdentityIconDirectory(wchar_t path[MAX_PATH])
    {
        HRESULT hr;

        hr = SHGetFolderPathAndSubDir(NULL, CSIDL_APPDATA|CSIDL_FLAG_CREATE, NULL,
                                      SHGFP_TYPE_CURRENT, NIDM_APPDATA_DIR, path);
        if (hr != S_OK) {
            assert(hr == E_FAIL);
            _reportf_ex(KHERR_ERROR, KHERR_FACILITY, KHERR_FACILITY_ID, KHERR_HMODULE,
                        L"Can't create the application data directory " NIDM_APPDATA_EXP);
            return false;
        }

        if (!PathAppend(path, NIDM_IDIMAGES_DIR)) {
            _reportf_ex(KHERR_ERROR, KHERR_FACILITY, KHERR_FACILITY_ID, KHERR_HMODULE,
                        L"Can't create identity icon cache directory");
            return false;
        }

        if (!PathFileExists(path)) {
            if (!CreateDirectory(path, NULL)) {
                _reportf_ex(KHERR_ERROR, KHERR_FACILITY, KHERR_FACILITY_ID, KHERR_HMODULE,
                            L"Can't create identity icon cache directory.  Last error is %d",
                            GetLastError());
                return false;
            }
        }

        if (!PathIsDirectory(path)) {
            _reportf_ex(KHERR_ERROR, KHERR_FACILITY, KHERR_FACILITY_ID, khm_hInstance,
                        L"Icon cache directory is invalid");
            return false;
        }

        return true;
    }

    void IconSelectDialog::DoOk()
    {
        wchar_t path[MAX_PATH];

        if (!PrepareIdentityIconDirectory(path))
            return;

        ConfigSpace cfg = m_identity.GetConfig(KHM_FLAG_CREATE);

        {
            wchar_t fname[MAX_PATH];

            StringCchPrintf(fname, ARRAYLENGTH(fname), L"IdImage%I64u", m_identity.GetSerial());
            PathAppend(path, fname);
        }

        m_cropper->SaveImage(cfg, path);

        m_identity.GetResourceIcon(KCDB_RES_ICON_DISABLED, KCDB_RF_SKIPCACHE | KCDB_RFI_SMALL);
        m_identity.GetResourceIcon(KCDB_RES_ICON_NORMAL, KCDB_RF_SKIPCACHE | KCDB_RFI_SMALL);
        m_identity.GetResourceIcon(KCDB_RES_ICON_DISABLED, KCDB_RF_SKIPCACHE);
        m_identity.GetResourceIcon(KCDB_RES_ICON_NORMAL, KCDB_RF_SKIPCACHE);

        kmq_post_message(KMSG_KCDB, KMSG_KCDB_IDENT, KCDB_OP_RESUPDATE, m_identity.GetHandle());

        EndDialog(0);
    }

    extern "C" khm_int32
    khm_select_icon_for_identity(HWND parent, khm_handle _identity)
    {
        Identity identity(_identity, FALSE);
        AutoRef<IconSelectDialog> d(new IconSelectDialog(identity),
                                    RefCount::TakeOwnership);

        return (d->DoModal(parent) == IDOK)? KHM_ERROR_SUCCESS : KHM_ERROR_CANCELLED;
    }
}
