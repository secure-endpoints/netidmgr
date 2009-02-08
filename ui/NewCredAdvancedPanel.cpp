
#include "khmapp.h"
#include "NewCredWizard.hpp"

namespace nim {

    void NewCredAdvancedPanel::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
    {
        if (id == IDC_NC_ADVANCED && codeNotify == BN_CLICKED)
            reinterpret_cast<NewCredWizard *>(nc->wizard)->Navigate( NC_PAGE_CREDOPT_BASIC );
    }

    LRESULT NewCredAdvancedPanel::OnNotify(int id, NMHDR * pnmh)
    {
        if (pnmh->code == TCN_SELCHANGE) {
            reinterpret_cast<NewCredWizard *>(nc->wizard)->m_privint.UpdateLayout();
        }
        return 0;
    }
}
