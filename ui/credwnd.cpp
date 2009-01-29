/*
 * Copyright (c) 2008 Secure Endpoints Inc.
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

/* $Id$ */

#include "khmapp.h"
#include<assert.h>

#include "credwnd_base.hpp"

namespace nim
{
    extern "C" void 
    khm_register_credwnd_class(void) {
        ControlWindow::RegisterWindowClass();
    }

    extern "C" void 
    khm_unregister_credwnd_class(void) {
        ControlWindow::UnregisterWindowClass();
    }

    static CwTable * main_table = NULL;

    extern "C" HWND 
    khm_create_credwnd(HWND parent) {
        RECT r;
        HWND hwnd;

        CwTable::CwCreateParams cparams;

        cparams.is_primary_view = true;

        assert(main_table == NULL);

        main_table = new CwTable;

        ZeroMemory(CwTable::attrib_to_action, sizeof(CwTable::attrib_to_action));

        GetClientRect(parent, &r);

        hwnd = main_table->Create(parent, RectFromRECT(&r), 0, &cparams);

        main_table->ShowWindow();

        return hwnd;
    }
}
