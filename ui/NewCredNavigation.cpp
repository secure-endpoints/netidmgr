
#include "khmapp.h"
#include "NewCredWizard.hpp"
#include <assert.h>

namespace nim {

#define CFG_CLOSE_AFTER_PROCESS_END L"CredWindow\\Windows\\NewCred\\CloseAfterProcessEnd"

    HWND NewCredNavigation::UpdateLayout()
    {
        HDWP dwp;

        dwp = BeginDeferWindowPos(8);

        dwp = DeferWindowPos(dwp, GetDlgItem(hwnd, IDC_BACK), NULL, 0, 0, 0, 0,
                             ((transitions & NC_TRANS_PREV) ?
                              SWP_SHOWONLY : SWP_HIDEONLY));

        dwp = DeferWindowPos(dwp, GetDlgItem(hwnd, IDC_NEXT), NULL, 0, 0, 0, 0,
                             ((transitions & NC_TRANS_NEXT) ?
                              SWP_SHOWONLY : SWP_HIDEONLY));

        dwp = DeferWindowPos(dwp, GetDlgItem(hwnd, IDC_FINISH), NULL, 0, 0, 0, 0,
                             ((transitions & NC_TRANS_FINISH) ?
                              SWP_SHOWONLY : SWP_HIDEONLY));

        dwp = DeferWindowPos(dwp, GetDlgItem(hwnd, IDC_RETRY), NULL, 0, 0, 0, 0,
                             ((transitions & NC_TRANS_RETRY) ?
                              SWP_SHOWONLY : SWP_HIDEONLY));

        dwp = DeferWindowPos(dwp, GetDlgItem(hwnd, IDC_NC_ABORT), NULL, 0, 0, 0, 0,
                             ((transitions & NC_TRANS_ABORT) ?
                              SWP_SHOWONLY : SWP_HIDEONLY));

        dwp = DeferWindowPos(dwp, GetDlgItem(hwnd, IDCANCEL), NULL, 0, 0, 0, 0,
                             (!(transitions & NC_TRANS_ABORT) &&
                              !(transitions & NC_TRANS_CLOSE)) ?
                             SWP_SHOWONLY : SWP_HIDEONLY);

        dwp = DeferWindowPos(dwp, GetDlgItem(hwnd, IDC_NC_CLOSE), NULL, 0, 0, 0, 0,
                             (transitions & NC_TRANS_CLOSE) ?
                             SWP_SHOWONLY : SWP_HIDEONLY);

        dwp = DeferWindowPos(dwp, GetDlgItem(hwnd, IDC_CLOSEIF), NULL, 0, 0, 0, 0,
                             ((transitions & NC_TRANS_SHOWCLOSEIF) ?
                              SWP_SHOWONLY : SWP_HIDEONLY));

        if (transitions & NC_TRANS_SHOWCLOSEIF) {
            khm_int32 t = 1;

            khc_read_int32(NULL, CFG_CLOSE_AFTER_PROCESS_END, &t);

            CheckDlgButton(hwnd, IDC_CLOSEIF, ((t)? BST_CHECKED: BST_UNCHECKED));
            if (t)
                state &= ~NC_NAVSTATE_NOCLOSE;
            else
                state |= NC_NAVSTATE_NOCLOSE;
        }

        EndDeferWindowPos(dwp);

        return GetDlgItem(hwnd,
                          (transitions & NC_TRANS_NEXT)? IDC_NEXT :
                          (transitions & NC_TRANS_FINISH)? IDC_FINISH :
                          (transitions & NC_TRANS_CLOSE)? IDC_NC_CLOSE :
                          (transitions & NC_TRANS_ABORT)? IDC_NC_ABORT :
                          IDCANCEL);
    }

    void NewCredNavigation::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
    {
        NewCredWizard * w = reinterpret_cast<NewCredWizard *>(nc->wizard);

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
                w->Navigate( NC_PAGET_CANCEL);
                return;

            case IDC_CLOSEIF:
                {
                    khm_boolean should_close;

                    should_close = (IsDlgButtonChecked(hwnd, IDC_CLOSEIF) == BST_CHECKED);

                    if (should_close)
                        state &= ~NC_NAVSTATE_NOCLOSE;
                    else
                        state |= NC_NAVSTATE_NOCLOSE;
                    khc_write_int32(NULL, CFG_CLOSE_AFTER_PROCESS_END, should_close);
                }
                return;
            }
        }
    }
}
