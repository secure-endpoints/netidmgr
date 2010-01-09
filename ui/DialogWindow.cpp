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
#include <assert.h>

namespace nim {

    struct DialogWindowCreateData {
        DialogWindow * obj;
        LPARAM user_param;
    };

    HWND DialogWindow::Create(HWND parent, LPARAM param)
    {
        DialogWindowCreateData d;

        d.obj = this;
        d.user_param = param;

        return CreateDialogParam(hInstance, templateName, parent,
                                 DialogProc, (LPARAM) &d);
    }

    INT_PTR DialogWindow::DoModal(HWND parent, LPARAM param)
    {
        DialogWindowCreateData d;

        d.obj = this;
        d.user_param = param;

        is_modal = true;

        return DialogBoxParam(hInstance, templateName, parent, DialogProc,
                              (LPARAM) &d);
    }

    BOOL DialogWindow::EnableItem(int nID, BOOL bEnable) {
        HWND hwnd_item = GetItem(nID);

        if (hwnd_item == NULL)
            return FALSE;

        if (!bEnable && GetFocus() == hwnd_item) {
            PostMessage(WM_NEXTDLGCTL, 0, 0);
        }

        return EnableWindow(hwnd_item, bEnable);
    }


    INT_PTR CALLBACK DialogWindow::HandleOnInitDialog(HWND hwnd, HWND hwnd_focus, LPARAM lParam)
    {
        DialogWindowCreateData *pd = (DialogWindowCreateData *) lParam;
        DialogWindow * dw = pd->obj;

        assert(dw != NULL);
        assert(dw->hwnd == NULL);

#pragma warning(push)
#pragma warning(disable: 4244)
        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) dw);
#pragma warning(pop)

        dw->hwnd = hwnd;

        dw->Hold();

	khm_add_dialog(hwnd);

#ifdef DEBUG
	dw->SetOkToDispose(false);
#endif

        return dw->OnInitDialog(hwnd_focus, pd->user_param);
    }

#ifdef KHUI_WM_NC_NOTIFY

#define HANDLE_WMNC_DERIVE_FROM_PRIVCRED(s, v, fn)      ((fn)((khui_collect_privileged_creds_data *) v))
#define HANDLE_WMNC_COLLECT_PRIVCRED(s, v, fn)          ((fn)((khui_collect_privileged_creds_data *) v))
#define HANDLE_WMNC_DIALOG_PROCESS_COMPLETE(s, v, fn)   ((fn)((int) s))
#define HANDLE_WMNC_SET_PROMPTS(s, v, fn)               ((fn)())
#define HANDLE_WMNC_IDENTITY_STATE(s, v, fn)            ((fn)((nc_identity_state_notification *) v))
#define HANDLE_WMNC_IDENTITY_CHANGE(s, v, fn)           ((fn)())
#define HANDLE_WMNC_DIALOG_ACTIVATE(s, v, fn)           ((fn)())
#define HANDLE_WMNC_DIALOG_SETUP(s, v, fn)              ((fn)())

#define HANDLE_WM_HELP(hwnd, wParam, lParam, fn)    \
    ((fn)((hwnd), (LPHELPINFO)(lParam)),0L)
#define FORWARD_WM_HELP(hwnd, lphi, fn)                             \
    (void)(fn)((hwnd),WM_HELP,(WPARAM)0,(LPARAM)(LPHELPINFO)(lphi))

    void DialogWindow::HandleNcNotify(HWND hwnd, khui_wm_nc_notifications code,
                                      int sParam, void * vParam)
    {
#define HANDLE_NCMSG(c, fn)				\
        case c: HANDLE_##c(sParam, vParam, fn); break

        switch (code) {
            HANDLE_NCMSG(WMNC_DIALOG_SETUP, OnDialogSetup);
            HANDLE_NCMSG(WMNC_DIALOG_ACTIVATE, OnDialogActivate);
            HANDLE_NCMSG(WMNC_IDENTITY_CHANGE, OnNewIdentity);
            HANDLE_NCMSG(WMNC_IDENTITY_STATE, OnIdentityStateChange);
            HANDLE_NCMSG(WMNC_SET_PROMPTS, OnSetPrompts);
            HANDLE_NCMSG(WMNC_DIALOG_PROCESS_COMPLETE, OnProcessComplete);
            HANDLE_NCMSG(WMNC_COLLECT_PRIVCRED, OnCollectPrivCred);
            HANDLE_NCMSG(WMNC_DERIVE_FROM_PRIVCRED, OnDeriveFromPrivCred);
        }
#undef HANDLE_NCMSG
    }

#endif

#undef HANDLE_MSG
#define HANDLE_MSG(hwnd, message, fn)                                   \
    case (message): lr = HANDLE_##message((hwnd), (wParam), (lParam), (fn)); break

    INT_PTR CALLBACK DialogWindow::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        AutoRef<DialogWindow> dw (static_cast<DialogWindow *>((void *)(LONG_PTR) GetWindowLongPtr(hwnd, DWLP_USER)));
        INT_PTR old_bHandled;
        LRESULT lr = 0;

        if (dw.IsNull()) {
            if (uMsg == WM_INITDIALOG)
                return HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, HandleOnInitDialog);
            return FALSE;
        }

        if (uMsg == WM_DESTROY) {
	    khm_del_dialog(hwnd);
            HANDLE_WM_DESTROY(hwnd, wParam, lParam, dw->HandleOnDestroy);
            return TRUE;
        }

        old_bHandled = dw->bHandled;
        dw->bHandled = TRUE;

        switch (uMsg) {
#pragma warning(push)
#pragma warning(disable: 4244)

            HANDLE_MSG(hwnd, WM_CLOSE, dw->HandleOnClose);
            HANDLE_MSG(hwnd, WM_COMMAND, dw->HandleCommand);
            HANDLE_MSG(hwnd, WM_ACTIVATE, dw->HandleActivate);
            HANDLE_MSG(hwnd, WM_WINDOWPOSCHANGING, dw->HandlePosChanging);
            HANDLE_MSG(hwnd, WM_WINDOWPOSCHANGED, dw->HandlePosChanged);
            HANDLE_MSG(hwnd, WM_HELP, dw->HandleHelp);
            HANDLE_MSG(hwnd, WM_DRAWITEM, dw->HandleDrawItem);
            HANDLE_MSG(hwnd, WM_MEASUREITEM, dw->HandleMeasureItem);
            HANDLE_MSG(hwnd, WM_NOTIFY, dw->HandleNotify);
	    HANDLE_MSG(hwnd, WM_GETDLGCODE, dw->HandleGetDlgCode);
            HANDLE_MSG(hwnd, WM_TIMER, dw->HandleTimer);
#ifdef KHUI_WM_NC_NOTIFY
            HANDLE_MSG(hwnd, KHUI_WM_NC_NOTIFY, dw->HandleNcNotify);
#endif
#ifdef KMQ_WM_DISPATCH
            HANDLE_MSG(hwnd, KMQ_WM_DISPATCH, dw->HandleDispatch);
#endif
        default:
            dw->bHandled = FALSE;

#pragma warning(pop)
        }

        if (!dw->bHandled)
            dw->bHandled = dw->HandleMessage(uMsg, wParam, lParam, &lr);

        {
            INT_PTR rv = ((dw->bHandled) ? SetDlgMsgResult(hwnd, uMsg, lr) : 0);
            dw->bHandled = old_bHandled;
            return rv;
        }
    }

} // namespace nim
