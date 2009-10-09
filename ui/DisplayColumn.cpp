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

#include "khmapp.h"

namespace nim {

    HDITEM& DisplayColumn::GetHDITEM(HDITEM& hdi)
    {
        ZeroMemory(&hdi, sizeof(hdi));

        hdi.mask = HDI_FORMAT | HDI_LPARAM | HDI_WIDTH;
        hdi.fmt = 0;
#if (_WIN32_WINNT >= 0x501)
        if (sort)
            hdi.fmt |= ((sort_increasing)? HDF_SORTUP : HDF_SORTDOWN);
#endif
        hdi.lParam = (LPARAM) this; 
        hdi.cxy = width;

        hdi.pszText = (LPWSTR) caption.c_str();
        if (hdi.pszText != NULL) {
            hdi.mask |= HDI_TEXT;
            hdi.fmt |= HDF_CENTER | HDF_STRING;
        }
        return hdi;
    }

}
