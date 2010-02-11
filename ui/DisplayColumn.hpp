/*
 * Copyright (c) 2009-2010 Secure Endpoints Inc.
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

#include <string>

namespace nim {

class DisplayColumn {
public:
    bool sort            : 1;
    bool sort_increasing : 1;
    bool filler          : 1;
    bool group           : 1;
    bool fixed_width     : 1;
    bool fixed_position  : 1;

    int  width;
    int  x;
    std::wstring caption;

    DisplayColumn() {
        width = 0;
        x = 0;
        sort = false; sort_increasing = false; filler = false; group = false;
        fixed_width = false; fixed_position = false;
    }

    DisplayColumn(std::wstring _caption, int _width = 0) : caption(_caption) {
        width = _width;
        x = 0;
        sort = false; sort_increasing = false; filler = false; group = false;
        fixed_width = false; fixed_position = false;
    }

    virtual ~DisplayColumn() {}

    void CheckFlags(void) {
        if (group) sort = true;
        if (fixed_width) filler = false;
    }

    HDITEM& GetHDITEM(HDITEM& hdi);
};


}
