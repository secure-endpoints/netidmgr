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
