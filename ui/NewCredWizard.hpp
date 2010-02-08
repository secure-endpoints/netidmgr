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

    // If auto_configure is true, then if the identity status
    // transitions from unknown->valid, then the wizard will
    // automatically navigate to the 'wizard' mode from the
    // 'basic' mode.
    bool                            auto_configure;

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
        auto_configure = false;
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

    bool HaveValidIdentity();

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
