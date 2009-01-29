#include "khmapp.h"
#include <assert.h>

namespace nim {
    void DisplayColumnList::ValidateColumns()
    {
        bool do_grouping = true;

        for (DisplayColumnList::iterator column = begin();
             column != end(); column++) {

            (*column)->CheckFlags();

            if (!(*column)->group)
                do_grouping = false;

            if (!do_grouping && (*column)->group)
                (*column)->group = false;
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
