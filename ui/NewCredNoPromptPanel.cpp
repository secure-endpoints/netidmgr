
#include "khmapp.h"
#include "NewCredWizard.hpp"
#include <assert.h>

namespace nim {
#define MARQUEE_TIMEOUT 100

    void NewCredNoPromptPanel::SetProgress(int progress, bool show)
    {

#if _WIN32_WINNT >= 0x0501
        /* The marquee mode of the progress bar control is only
           supported on Windows XP or later.  If you are running an
           older OS, you get to stare at a progress bar that isn't
           moving.  */

        if (show && progress == KHUI_CWNIS_MARQUEE) {
            /* enabling marquee */
            if (!(noprompt_flags & NC_NPF_MARQUEE)) {
                HWND hw;

                hw = GetDlgItem(hwnd, IDC_PROGRESS);
                SetWindowLong(hw, GWL_STYLE, WS_CHILD|PBS_MARQUEE);
                ::SendMessage(hw, PBM_SETMARQUEE, TRUE, MARQUEE_TIMEOUT);
                noprompt_flags |= NC_NPF_MARQUEE;
            }
        } else {
            /* disabling marquee */
            if (noprompt_flags & NC_NPF_MARQUEE) {
                SendDlgItemMessage(hwnd, IDC_PROGRESS,
                                   PBM_SETMARQUEE, FALSE, MARQUEE_TIMEOUT);
                noprompt_flags &= ~NC_NPF_MARQUEE;
            }
        }
#endif

        if (show && progress >= 0 && progress <= 100) {
            SendDlgItemMessage(hwnd, IDC_PROGRESS,
                               PBM_SETPOS, progress, 0);
        }

        ::ShowWindow(GetDlgItem(hwnd, IDC_PROGRESS), (show?SW_SHOW:SW_HIDE));
    }
}
