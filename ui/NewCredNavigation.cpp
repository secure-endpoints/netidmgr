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

#include "khmapp.h"
#include "NewCredWizard.hpp"
#include <assert.h>

namespace nim {

#define CFG_CLOSE_AFTER_PROCESS_END L"CredWindow\\Windows\\NewCred\\CloseAfterProcessEnd"

HWND NewCredNavigation::UpdateLayout()
{
    HDWP dwp;

    CheckControls();

    dwp = BeginDeferWindowPos(8);

    dwp = DeferWindowPos(dwp, GetItem(IDC_BACK), NULL, 0, 0, 0, 0,
                         ((IsControlEnabled(Prev)) ?
                          SWP_SHOWONLY : SWP_HIDEONLY));

    dwp = DeferWindowPos(dwp, GetItem(IDC_NEXT), NULL, 0, 0, 0, 0,
                         ((IsControlEnabled(Next)) ?
                          SWP_SHOWONLY : SWP_HIDEONLY));

    dwp = DeferWindowPos(dwp, GetItem(IDC_FINISH), NULL, 0, 0, 0, 0,
                         ((IsControlEnabled(Finish)) ?
                          SWP_SHOWONLY : SWP_HIDEONLY));

    dwp = DeferWindowPos(dwp, GetItem(IDC_RETRY), NULL, 0, 0, 0, 0,
                         ((IsControlEnabled(Retry)) ?
                          SWP_SHOWONLY : SWP_HIDEONLY));

    dwp = DeferWindowPos(dwp, GetItem(IDC_NC_ABORT), NULL, 0, 0, 0, 0,
                         ((IsControlEnabled(Abort)) ?
                          SWP_SHOWONLY : SWP_HIDEONLY));

    dwp = DeferWindowPos(dwp, GetItem(IDCANCEL), NULL, 0, 0, 0, 0,
                         (!(IsControlEnabled(Abort)) &&
                          !(IsControlEnabled(Close))) ?
                         SWP_SHOWONLY : SWP_HIDEONLY);

    dwp = DeferWindowPos(dwp, GetItem(IDC_NC_CLOSE), NULL, 0, 0, 0, 0,
                         (IsControlEnabled(Close)) ?
                         SWP_SHOWONLY : SWP_HIDEONLY);

    dwp = DeferWindowPos(dwp, GetItem(IDC_CLOSEIF), NULL, 0, 0, 0, 0,
                         ((IsControlEnabled(ShowCloseIf)) ?
                          SWP_SHOWONLY : SWP_HIDEONLY));

    if (IsControlEnabled(ShowCloseIf)) {
        khm_int32 t = 1;

        khc_read_int32(NULL, CFG_CLOSE_AFTER_PROCESS_END, &t);

        CheckDlgButton(hwnd, IDC_CLOSEIF, ((t)? BST_CHECKED: BST_UNCHECKED));
        if (t)
            m_state &= ~NoClose;
        else
            m_state |= NoClose;
    }

    EndDeferWindowPos(dwp);

    return GetItem((IsControlEnabled(Next))? IDC_NEXT :
                   (IsControlEnabled(Finish))? IDC_FINISH :
                   (IsControlEnabled(Close))? IDC_NC_CLOSE :
                   (IsControlEnabled(Abort))? IDC_NC_ABORT :
                   IDCANCEL);
}

void NewCredNavigation::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
    AutoRef<NewCredWizard> w (NewCredWizard::FromNC(nc));

    if (codeNotify == BN_CLICKED) {
        switch (id) {
        case IDC_BACK:
            w->Navigate( NC_PAGET_PREV );
            return;

        case IDC_NEXT:
            w->Navigate( NC_PAGET_NEXT );
            return;

        case IDC_RETRY:
        case IDC_FINISH:
            w->Navigate( NC_PAGET_FINISH);
            return;

        case IDCANCEL:
        case IDC_NC_CLOSE:
        case IDC_NC_ABORT:
            // These messages can result in the New Credentials
            // Wizard window being destroyed.  Since we don't have
            // a great reference counting mechanism, we can't
            // initiate an EndDialog() from here.
            ::PostMessage(w->hwnd, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED),
                          (LPARAM) hwndCtl);
            return;

        case IDC_CLOSEIF:
            {
                khm_boolean should_close;

                should_close = (IsDlgButtonChecked(hwnd, IDC_CLOSEIF) == BST_CHECKED);

                if (should_close)
                    DisableState(NoClose);
                else
                    EnableState(NoClose);
                khc_write_int32(NULL, CFG_CLOSE_AFTER_PROCESS_END, should_close);
            }
            return;
        }
    }
}

void NewCredNavigation::CheckControls()
{
    khui_new_creds_privint_panel * p = NULL;
    AutoRef<NewCredWizard> cw (NewCredWizard::FromNC(nc));

    khui_cw_lock_nc(nc);

    switch (cw->page) {
    case NC_PAGE_CREDOPT_WIZ:

        DisableControl(Next | Prev | Finish);

        p = khui_cw_get_current_privint_panel(nc);

        if (cw->m_privint.idx_current == NC_PRIVINT_PANEL) {

            if ((p && QNEXT(p)) || KHM_SUCCEEDED(khui_cw_peek_next_privint(nc, NULL)))
                EnableControl(Next);

            if (p && QPREV(p) ||
                (nc->n_types > 0 && !(nc->types[0].nct->flags & KHUI_NCT_FLAG_DISABLED)))
                EnableControl(Prev);

        } else {

            int idx = cw->m_privint.idx_current;

            if ((idx + 1 < (int) nc->n_types &&
                 !(nc->types[idx + 1].nct->flags & KHUI_NCT_FLAG_DISABLED)) ||
                p != NULL ||
                KHM_SUCCEEDED(khui_cw_peek_next_privint(nc, NULL)))
                EnableControl(Next);

            if (idx > 0)
                EnableControl(Prev);

        }
        break;

    case NC_PAGE_PROGRESS:

        DisableControl(Next | Prev | Finish);

        p = khui_cw_get_current_privint_panel(nc);

        if (!khm_cred_is_new_creds_pending(nc))
            DisableControl(Retry);

        if (p && !(nc->response & KHUI_NC_RESPONSE_PROCESSING) && IsControlEnabled(Retry))
            EnableControl(Prev);

        break;

    case NC_PAGE_CREDOPT_BASIC:
    case NC_PAGE_CREDOPT_ADV:
    case NC_PAGE_PASSWORD:

        DisableControl(Next | Prev | Finish);

        p = khui_cw_get_current_privint_panel(nc);

        if ((p && QNEXT(p)) || KHM_SUCCEEDED(khui_cw_peek_next_privint(nc, NULL)))
            EnableControl(Next);

        if (p && QPREV(p))
            EnableControl(Prev);
    }

    khui_cw_unlock_nc(nc);

    if (khui_cw_is_ready(nc) &&
        !IsControlEnabled(Retry) &&
        !IsControlEnabled(Close) &&
        !IsControlEnabled(Abort))
        EnableControl(Finish);
}
}
