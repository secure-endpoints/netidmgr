#pragma once

#include <vector>
#include "DisplayColumn.hpp"

namespace nim {

    class DisplayColumnList : public std::vector<DisplayColumn *> {
    public:
        virtual ~DisplayColumnList();

    public:
        void ValidateColumns();

        void AdjustColumnPositions(int max_width);

        void AddColumnsToHeaderControl(HWND hwnd_header);

        void UpdateHeaderControl(HWND hwnd_header, UINT mask = (HDI_FORMAT | HDI_WIDTH));

        void Clear();
    };


}
