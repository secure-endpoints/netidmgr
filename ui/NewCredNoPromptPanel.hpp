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

namespace nim {

    class NewCredNoPromptPanel : public DialogWindow {
        khui_new_creds * nc;

        int             noprompt_flags;
                                /*!< Flags used by hwnd_noprompts.
                                  Combination of NC_NPF_* values. */

        /* The marquee is running */
#define NC_NPF_MARQUEE       0x0001

    public:
        NewCredNoPromptPanel(khui_new_creds * _nc):
            nc(_nc),
            DialogWindow(MAKEINTRESOURCE(IDD_NC_NOPROMPTS), khm_hInstance),
            noprompt_flags(0)
        {}

        void SetProgress(int progress, bool show);

        void SetText(kherr_severity severity,
                     const wchar_t * text,
                     const wchar_t * status = NULL);
    };
}
