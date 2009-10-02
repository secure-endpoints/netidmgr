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
