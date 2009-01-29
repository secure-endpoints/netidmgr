#pragma once

namespace nim
{

    /*! \brief Credential window column
     */
    class CwColumn : public DisplayColumn
    {
        // Legacy flags
#define KHUI_CW_COL_AUTOSIZE    0x00000001
#define KHUI_CW_COL_SORT_INC    0x00000002
#define KHUI_CW_COL_SORT_DEC    0x00000004
#define KHUI_CW_COL_GROUP       0x00000008
#define KHUI_CW_COL_FIXED_WIDTH 0x00000010
#define KHUI_CW_COL_FIXED_POS   0x00000020
#define KHUI_CW_COL_META        0x00000040
#define KHUI_CW_COL_FILLER      0x00000080

    public:

        khm_int32  attr_id;

    public:
        CwColumn() : DisplayColumn() {
            attr_id = KCDB_ATTR_INVALID;
        }

        khm_int32 GetFlags(void) {
            CheckFlags();
            return
                ((sort && sort_increasing)? KHUI_CW_COL_SORT_INC : 0) |
                ((sort && !sort_increasing)? KHUI_CW_COL_SORT_DEC : 0) |
                ((group)? KHUI_CW_COL_GROUP : 0) |
                ((fixed_width)? KHUI_CW_COL_FIXED_WIDTH : 0) |
                ((fixed_position)? KHUI_CW_COL_FIXED_POS : 0) |
                ((filler)? KHUI_CW_COL_FILLER : 0);
        }

        void SetFlags(khm_int32 f) {
            sort = !!(f & (KHUI_CW_COL_SORT_INC | KHUI_CW_COL_SORT_DEC | KHUI_CW_COL_GROUP));
            sort_increasing = ((f & KHUI_CW_COL_SORT_INC) ||
                               ((f & KHUI_CW_COL_GROUP) && !(f & KHUI_CW_COL_SORT_DEC)));
            group = !!(f & KHUI_CW_COL_GROUP);
            fixed_width = !!(f & KHUI_CW_COL_FIXED_WIDTH);
            fixed_position = !!(f & KHUI_CW_COL_FIXED_POS);
            filler = !!(f & KHUI_CW_COL_FILLER);
            CheckFlags();
        }
    };

}
