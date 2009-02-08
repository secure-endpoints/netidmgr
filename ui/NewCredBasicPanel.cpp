
#include "khmapp.h"
#include "NewCredWizard.hpp"

namespace nim {

    void NewCredBasicPanel::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
    {
        if (id == IDC_NC_ADVANCED && codeNotify == BN_CLICKED) {
            reinterpret_cast<NewCredWizard *>(nc->wizard)->
                Navigate( NC_PAGE_CREDOPT_ADV );
        } else if (id == IDC_PERSIST && codeNotify == BN_CLICKED) {
            reinterpret_cast<NewCredWizard *>(nc->wizard)->
                m_privint.m_persist.OnCommand(id, hwndCtl, codeNotify);
        }
    }
}
