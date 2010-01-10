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

#include "NewCredAdvancedPanel.hpp"
#include "NewCredBasicPanel.hpp"
#include "NewCredIdentityConfigWizard.hpp"
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

        NewCredIdentityConfigWizard m_cfgwiz;

        HWND                    hwnd_current;
                                /*!< Handle to currently selected
                                  panel window */

        int                     idx_current;
                                /*!< Index of currently selected panel
                                  in nc or NC_PRIVINT_PANEL if it's
                                  the privileged interaction panel. */

        // The privileged interaction panel
#define NC_PRIVINT_PANEL    -1

        NewCredPanels(khui_new_creds *_nc) :
            nc(_nc),
            m_advanced(_nc),
            m_basic(_nc),
            m_noprompts(_nc),
            m_persist(_nc),
            m_cfgwiz(_nc),
            hwnd_current(NULL),
            idx_current(NC_PRIVINT_PANEL)
        {}

        void Create(HWND parent);

        HWND UpdateLayout();

        bool IsSavePasswordAllowed();

        khui_new_creds_privint_panel * GetPrivintPanel(HWND);
    };
}
