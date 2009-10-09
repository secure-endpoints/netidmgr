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

#include "khmapp.h"
#include "NewCredWizard.hpp"
#if _WIN32_WINNT >= 0x0501
#include <uxtheme.h>
#endif

namespace nim {

    void NewCredPersistPanel::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
    {
        if (id == IDC_PERSIST && codeNotify == BN_CLICKED) {
            khui_new_creds_by_type * t = NULL;
            khm_int32 ks_type = KCDB_CREDTYPE_INVALID;
            khm_size cb;

            nc->persist_privcred = (::SendMessage(hwndCtl, BM_GETCHECK, 0, 0) == BST_CHECKED);
            cb = sizeof(ks_type);
            kcdb_identity_get_attr(nc->persist_identity, KCDB_ATTR_TYPE, NULL, &ks_type, &cb);
            khui_cw_find_type(nc, ks_type, &t);
            if (t != NULL && t->hwnd_panel != NULL) {
                ::SendMessage(t->hwnd_panel, KHUI_WM_NC_NOTIFY,
                            MAKEWPARAM(0, WMNC_COLLECT_PRIVCRED), 0);
            }
        }
    }

    BOOL NewCredPersistPanel::OnCreate(LPVOID createParams)
    {
#if _WIN32_WINNT >= 0x0501
        EnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);
#endif
        return __super::OnCreate(createParams);
    }
}
