#include "khmapp.h"
#include <assert.h>

namespace nim {

    struct DialogWindowCreateData {
        DialogWindow * obj;
        LPARAM user_param;
    };

    HWND DialogWindow::Create(HWND parent, LPARAM param = 0)
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

        return cw->OnInitDialog(hwnd_focus, pd->user_param);
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

    void DialogWindow::HandleNcNotify(HWND hwnd, khui_wm_nc_notifications code,
                                      inst sParam, void * vParam)
    {
#define HANDLE_NCMSG(c, fn)                        \
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
    case (message): SetDlgReturn( HANDLE_##message((hwnd), (wParam), (lParam), (fn)) ); break

    INT_PTR CALLBACK DialogWindow::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        DialogWindow * dw = NULL;

        dw = (DialogWindow *)(LONG_PTR) GetWindowLongPtr(hwnd, DWLP_USER);
        bHandled = TRUE;

        switch (uMsg) {
        case WM_INITDIALOG: return HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, HandleOnInitDialog);
            HANDLE_MSG(hwnd, WM_DESTROY, dw->HandleOnDestroy);
            HANDLE_MSG(hwnd, WM_CLOSE, dw->HandleOnClose);
            HANDLE_MSG(hwnd, WM_COMMAND, dw->HandleOnCommand);
            HANDLE_MSG(hwnd, WM_ACTIVATE, dw->HandleOnActivate);
            HANDLE_MSG(hwnd, WM_WINDOWPOSCHANGING, dw->HandlePosChanging);
            HANDLE_MSG(hwnd, WM_WINDOWPOSCHANGED, dw->HandlePosChanged);
            HANDLE_MSG(hwnd, WM_HELP, dw->HandleOnHelp);
            HANDLE_MSG(hwnd, WM_DRAWITEM, dw->HandleDrawItem);
            HANDLE_MSG(hwnd, WM_MEASUREITEM, dw->HandleMeasureItem);
#ifdef KHUI_WM_NC_NOTIFY
            HANDLE_MSG(hwnd, KHUI_WM_NC_NOTIFY, dw->HandleNcNotify);
#endif
#ifdef KMQ_WM_DISPATCH
            HANDLE_MSG(hwnd, KMQ_WM_DISPATCH, dw->HandleDispatch);
#endif
        default:
            bHandled = FALSE;
        }

        return bHandled;
    }

} // namespace nim
