#pragma once

#include "ControlWindow.hpp"

namespace nim {

    class DialogWindow : public ControlWindow {
    protected:
        const HINSTANCE hInstance;
        const LPCTSTR   templateName;
        INT_PTR         bHandled;
        bool            is_modal;

    public:
        DialogWindow(LPCTSTR _templateName, HINSTANCE _hInstance) :
            hInstance(_hInstance), templateName(_templateName) {
            bHandled = 0;
            is_modal = false;
        }

        HWND Create(HWND parent, LPARAM param = 0);

        INT_PTR DoModal(HWND parent, LPARAM param = 0);

        BOOL EndDialog(INT_PTR result) {
	    if (is_modal)
		return ::EndDialog(hwnd, result);
	    else {
		DestroyWindow();
		return TRUE;
	    }
        }

        LONG_PTR SetDlgResult(LONG_PTR rv) {
#pragma warning(push)
#pragma warning(disable: 4244)
            // VC++ 2005 on 32-bit archs reports C4244 when converting
            // rv from LONG_PTR to LONG and then back to LONG_PTR
            // because clearly we are losing precision that way.
            return SetWindowLongPtr(hwnd, DWLP_MSGRESULT, rv);
#pragma warning(pop)
        }

        void DoDefault() {
            bHandled = FALSE;
        }

        HWND GetItem(int nID) {
            return ::GetDlgItem(hwnd, nID);
        }

        BOOL SetItemText(int nID, LPCTSTR text) {
            return ::SetDlgItemText(hwnd, nID, text);
        }

        UINT GetItemText(int nIDDlgItem, LPTSTR lpString, int nMaxCount) {
            return ::GetDlgItemText(hwnd, nIDDlgItem, lpString, nMaxCount);
        }

        LRESULT SendItemMessage(int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam) {
            return SendDlgItemMessage(hwnd, nIDDlgItem, Msg, wParam, lParam);
        }

        BOOL CheckButton(int nIDButton, UINT uCheck) {
            return CheckDlgButton(hwnd, nIDButton, uCheck);
        }

        UINT IsButtonChecked(int nIDButton) {
            return IsDlgButtonChecked(hwnd, nIDButton);
        }

        BOOL EnableItem(int nID, BOOL bEnable = TRUE) {
            return EnableWindow(GetItem(nID), bEnable);
        }

    public:
        virtual BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam) { return FALSE; }

        virtual void OnClose() { DoDefault(); }

        virtual LRESULT OnHelp(HELPINFO * info) { DoDefault(); return 0; }

#ifdef KHUI_WM_NC_NOTIFY
    public:
        virtual void OnDeriveFromPrivCred(khui_collect_privileged_creds_data * pcd) { }

        virtual void OnCollectPrivCred(khui_collect_privileged_creds_data * pcd) { }

        virtual void OnProcessComplete(int has_error) { }

        virtual void OnSetPrompts() { }

        virtual void OnIdentityStateChange(const nc_identity_state_notification * isn) { }

        virtual void OnNewIdentity() { }

        virtual void OnDialogActivate() { }

        virtual void OnDialogSetup() { }
#endif

    private:
        static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

        static INT_PTR CALLBACK HandleOnInitDialog(HWND hwnd, HWND hwnd_focus, LPARAM lParam);

#ifdef KHUI_WM_NC_NOTIFY
        void HandleNcNotify(HWND hwnd, khui_wm_nc_notifications code,
                            int sParam, void * vParam);
#endif
    };
}
