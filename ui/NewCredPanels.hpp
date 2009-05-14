
#pragma once

#include "NewCredAdvancedPanel.hpp"
#include "NewCredBasicPanel.hpp"
#include "NewCredNoPromptPanel.hpp"
#include "NewCredPersistPanel.hpp"

namespace nim {

    class NewCredPanels {
    public:
        khui_new_creds          *nc;

        NewCredAdvancedPanel    m_advanced;

        NewCredBasicPanel       m_basic;

        NewCredNoPromptPanel    m_noprompts;

        NewCredPersistPanel     m_persist;

        HWND                    hwnd_current;
                                /*!< Handle to currently selected
                                  panel window */

        int                     idx_current;
                                /*!< Index of currently selected panel
                                  in nc or NC_PRIVINT_PANEL if it's
                                  the privileged interaction panel. */
#define NC_PRIVINT_PANEL -1

        NewCredPanels(khui_new_creds *_nc) :
            nc(_nc),
            m_advanced(_nc),
            m_basic(_nc),
            m_noprompts(_nc),
            m_persist(_nc),
            hwnd_current(NULL),
            idx_current(NC_PRIVINT_PANEL)
        {}

        void Create(HWND parent);

        void PurgeDeletedShownPanels();

        HWND UpdateLayout();

        bool IsSavePasswordAllowed();
    };
}
