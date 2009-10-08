
#pragma once

namespace nim {

    enum NewCredPage {
        NC_PAGE_NONE = 0,
        NC_PAGE_IDSPEC,
        NC_PAGE_CREDOPT_BASIC,
        NC_PAGE_CREDOPT_ADV,
        NC_PAGE_PASSWORD,
        NC_PAGE_PROGRESS,
        NC_PAGE_CREDOPT_WIZ,

        NC_PAGET_NEXT,
        NC_PAGET_PREV,
        NC_PAGET_FINISH,
        NC_PAGET_CANCEL,
        NC_PAGET_END,
        NC_PAGET_DEFAULT
    };

    class NewCredWizard;

}


#include "NewCredIdentitySpecifier.hpp"
#include "NewCredIdentitySelector.hpp"
#include "NewCredNavigation.hpp"
#include "NewCredProgress.hpp"
#include "NewCredPanels.hpp"


namespace nim {

    class NewCredWizard : public DialogWindow {
    public:
        khui_new_creds                  *nc;

        NewCredIdentitySpecifier        m_idspec;

        NewCredIdentitySelector         m_idsel;

        NewCredNavigation               m_nav;

        NewCredProgress                 m_progress;

        NewCredPanels                   m_privint;

        NewCredPage                     page;

        bool                            flashing_enabled;

    public:
        NewCredWizard(khui_new_creds * _nc):
            DialogWindow(MAKEINTRESOURCE(IDD_NC_CONTAINER),
                         khm_hInstance),
            nc(_nc),
            m_idspec(_nc),
            m_idsel(_nc),
            m_nav(_nc),
            m_progress(_nc),
            m_privint(_nc)
        {
            page = NC_PAGE_NONE;
            flashing_enabled = false;
            is_modal = false;
            nc->wizard = this;
        }

    public:

        void OnActivate(UINT state, HWND hwndActDeact, BOOL fMinimized);

        void OnClose();

        void OnDeriveFromPrivCred(khui_collect_privileged_creds_data * pcd);

        void OnCollectPrivCred(khui_collect_privileged_creds_data * pcd);

        void OnProcessComplete(int has_error);

        void OnSetPrompts();

        void OnIdentityStateChange(const nc_identity_state_notification * isn);

        void OnNewIdentity();

        void OnDialogActivate();

        void OnDialogSetup();

        BOOL OnPosChanging(LPWINDOWPOS wpos);

        void OnCommand(int id, HWND hwndCtl, UINT codeNotify);

        void OnDestroy(void);

        BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);

        LRESULT OnHelp(HELPINFO * info);

    public:
        void Navigate(NewCredPage new_page);

        void NotifyNewIdentity(bool notify_ui);

        void NotifyTypes(khui_wm_nc_notifications N, LPARAM lParam, bool sync);

        void EnableControls(bool enable);

        void UpdateLayout();

        void PrepCredTypes();

        void PositionSelf();

    public:
        static NewCredWizard * FromNC(khui_new_creds * _nc) {
            return reinterpret_cast<NewCredWizard *>(_nc->wizard);
        }

    private:

        class NewCredLayout {
            NewCredWizard * w;
            HDWP dwp;
            RECT r_pos;
            HWND hwnd_last;

        public:
            enum Slot {
                Hidden,
                Header,
                ContentNoHeader,
                ContentNormal,
                ContentLarge,
                Footer
            };

            NewCredLayout(NewCredWizard * w, int nWindows);

            void AddPanel(ControlWindow * cwnd, Slot slot);

            void HidePanel(ControlWindow * cwnd) {
                AddPanel(cwnd, Hidden);
            }

            BOOL Commit();
        };
    };
}
