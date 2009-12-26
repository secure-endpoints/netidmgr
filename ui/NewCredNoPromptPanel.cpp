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

#define OEMRESOURCE

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

    void NewCredNoPromptPanel::SetText(kherr_severity severity,
                                       const wchar_t * text,
                                       const wchar_t * status)
    {
        UINT res = 0;

        SetItemText(IDC_TEXT, (text)? text : L"");
        SetItemText(IDC_TEXT2, (status)? status : L"");

        switch (severity) {
        case KHERR_INFO:
            res = OIC_INFORMATION;
            break;

        case KHERR_WARNING:
            res = OIC_WARNING;
            break;

        case KHERR_ERROR:
            res = OIC_ERROR;
            break;

        default:
            SendItemMessage(IDC_ICONCTL, STM_SETICON, NULL, 0);
            return;
        }

        SendItemMessage(IDC_ICONCTL, STM_SETICON,
                        (WPARAM) LoadIconResource(res, false, true, NULL), 0);
    }
}
