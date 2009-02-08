namespace nim {

    class NewCredNavigation : public DialogWindow {
        khui_new_creds * nc;

        khm_int32       transitions; /*!< Combination of NC_TRANS_* */
#define NC_TRANS_NEXT           0x0001L
#define NC_TRANS_PREV           0x0002L
#define NC_TRANS_FINISH         0x0004L
#define NC_TRANS_ABORT          0x0008L
#define NC_TRANS_SHOWCLOSEIF    0x0010L
#define NC_TRANS_CLOSE          0x0020L
#define NC_TRANS_RETRY          0x0040L

        khm_int32       state;  /*!< State flags */
#define NC_NAVSTATE_PREEND      0x0001L
#define NC_NAVSTATE_NOCLOSE     0x0002L
#define NC_NAVSTATE_OKTOFINISH  0x0004L
#define NC_NAVSTATE_CANCELLED   0x0008L

    public:
        NewCredNavigation( khui_new_creds * _nc ) :
            DialogWindow(MAKEINTRESOURCE(IDD_NC_NAV), khm_hInstance),
            nc(_nc) {}

        HWND UpdateLayout();

        void OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    };
}
