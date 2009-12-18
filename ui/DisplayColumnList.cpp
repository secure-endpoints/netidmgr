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
#include <assert.h>

namespace nim {
    void DisplayColumnList::ValidateColumns()
    {
        bool do_grouping = true;

        for (DisplayColumnList::iterator column = begin();
             column != end(); column++) {

            if (!(*column)->group)
                do_grouping = false;

            if (!do_grouping && (*column)->group)
                (*column)->group = false;

            (*column)->CheckFlags();
        }
    }

    void DisplayColumnList::AdjustColumnPositions(int max_width)
    {
        DisplayColumn * filler_col = NULL;
        int width = 0;

        for (iterator i = begin(); i != end(); ++i) {
            if ((*i)->filler && filler_col == NULL) {
                filler_col = *i;
                continue;
            }
            width += (*i)->width;
        }

        if (filler_col != NULL)
            filler_col->width = max(max_width - width, 0);

        int x = 0;

        for (iterator i = begin(); i != end(); ++i) {
            (*i)->x = x;
            x += (*i)->width;
        }
    }

    void DisplayColumnList::AddColumnsToHeaderControl(HWND hwnd_header)
    {
        int h_count = Header_GetItemCount(hwnd_header);
        int ord = 0;

        for (iterator i = begin(); i != end(); ++i) {
            HDITEM hdi;

            (*i)->GetHDITEM(hdi);
            if (ord < h_count) {
                int hidx = Header_OrderToIndex(hwnd_header, ord);
                Header_SetItem(hwnd_header, hidx, &hdi);
            } else {
                Header_InsertItem(hwnd_header, ord, &hdi);
            }
            ord++;
        }

        for (; ord < h_count; h_count--) {
            int hidx = Header_OrderToIndex(hwnd_header, ord);
            Header_DeleteItem(hwnd_header, hidx);
        }
    }

    void DisplayColumnList::UpdateHeaderControl(HWND hwnd_header, UINT mask)
    {
        int h_count = Header_GetItemCount(hwnd_header);
        int ord = 0;

        if (h_count != size()) {
            assert(false);
            AddColumnsToHeaderControl(hwnd_header);
            return;
        }

        for (iterator i = begin(); i != end(); ++i) {
            int hidx = Header_OrderToIndex(hwnd_header, ord);
            HDITEM hdi;

            (*i)->GetHDITEM(hdi);
            hdi.mask &= mask;
            if (hdi.mask != 0)
                Header_SetItem(hwnd_header, hidx, &hdi);
            ord++;
        }
    }

    void DisplayColumnList::Clear()
    {
        while (!empty()) {
            DisplayColumn * c = back();
            pop_back();
            delete c;
        }
    }

    DisplayColumnList::~DisplayColumnList()
    {
        Clear();
    }

}
