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

#define NO_STRSAFE
#include <shlwapi.h>
#include <shlobj.h>
#include "khmapp.h"
#include "IconSelectDialog.hpp"
#include <strsafe.h>
#include <assert.h>

namespace nim {

IconSelectDialog::IconSelectDialog(Identity& _identity) :
    DialogWindow(MAKEINTRESOURCE(IDD_ICONSELECT),
                 khm_hInstance),
    m_identity(_identity),
    m_cropper(new PictureCropWindow(), RefCount::TakeOwnership),
    m_request(NULL)
{
}

IconSelectDialog::~IconSelectDialog()
{
    SetRequest(NULL, 0);
}

void IconSelectDialog::SetRequest(HttpRequest * req, UINT msg_target)
{
    if (!m_request.IsNull())
        m_request->Abort();
    m_request.Assign(req);

    ShowItem(IDC_ABORT, (req != NULL));
    ShowItem(IDC_PROGRESS, (req != NULL));
    SendItemMessage(IDC_PROGRESS, PBM_SETMARQUEE, (req != NULL), 100);
    EnableItem(IDC_BROWSE, (req == NULL));
    EnableItem(IDC_URLGO, (req == NULL));
    EnableItem(IDC_FAVGO, (req == NULL));
    EnableItem(IDC_GRAVGO, (req == NULL));
    EnableItem(IDC_PATH, (req == NULL));
    EnableItem(IDC_URL, (req == NULL));
    EnableItem(IDC_DOMAIN, (req == NULL));
    EnableItem(IDC_EMAIL, (req == NULL));

    m_msg_edit = msg_target;
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

LRESULT IconSelectDialog::OnNotify(int id, NMHDR * pnmh)
{
    if (pnmh->idFrom == IDC_PRIVACY_ISSUES && pnmh->code == NM_CLICK) {
        khm_html_help(hwnd, L"::/html/wnd_icons_privacy.htm", HH_DISPLAY_TOPIC, 0);
    }
    return 0;
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

    if (codeNotify == BN_CLICKED && id == IDC_ABORT) {
        SetRequest(NULL, 0);
    }

    if (codeNotify == BN_CLICKED && id == IDC_IMGEDITOR) {
        if (hwndCtl != NULL)
            m_cropper->SetImage((const wchar_t *) hwndCtl);
        SetRequest(NULL, 0);
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

void IconSelectDialog::HttpRequestStatus(kherr_severity severity,
                                         const wchar_t * status,
                                         const wchar_t * long_desc)
{
    if (m_msg_edit != 0 &&
        (severity == KHERR_ERROR ||
         severity == KHERR_WARNING)) {
        EDITBALLOONTIP tip;

        tip.cbStruct = sizeof(tip);
        tip.pszTitle = (status)? status : L"";
        tip.pszText = (long_desc)? long_desc : L"";
        tip.ttiIcon = ((severity == KHERR_ERROR)? TTI_ERROR : TTI_WARNING);

        Edit_ShowBalloonTip(GetItem(m_msg_edit), &tip);
    }
}

void IconSelectDialog::HttpRequestCompleted(const wchar_t * path)
{
    FORWARD_WM_COMMAND(hwnd, IDC_IMGEDITOR, path, BN_CLICKED, ::SendMessage);
}

void IconSelectDialog::DoFetchURL()
{
    wchar_t url[MAXCCH_URL];

    if (GetItemText(IDC_URL, url, ARRAYLENGTH(url)) == 0)
        return;

    SetRequest(HttpRequest::CreateRequest(url, HttpRequest::ByURL, this), IDC_URL);
}

void IconSelectDialog::DoFetchFavicon()
{
    wchar_t url[MAXCCH_URL];

    if (GetItemText(IDC_DOMAIN, url, ARRAYLENGTH(url)) == 0)
        return;

    SetRequest(HttpRequest::CreateRequest(url, HttpRequest::ByFavIcon, this), IDC_DOMAIN);
}

void IconSelectDialog::DoFetchGravatar()
{
    wchar_t email[MAXCCH_URL];

    if (GetItemText(IDC_EMAIL, email, ARRAYLENGTH(email)) == 0)
        return;

    SetRequest(HttpRequest::CreateRequest(email, HttpRequest::ByGravatar, this), IDC_EMAIL);
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

LRESULT IconSelectDialog::OnHelp(HELPINFO * info)
{
    khm_html_help(khm_hwnd_main, NULL, HH_HELP_CONTEXT, IDH_CFG_IDICON);
    return 0;
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
