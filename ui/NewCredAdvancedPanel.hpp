
namespace nim {

    class NewCredAdvancedPanel : public DialogWindow {
        khui_new_creds * nc;

    public:
        NewCredAdvancedPanel(khui_new_creds * _nc):
            nc(_nc),
            DialogWindow(MAKEINTRESOURCE(IDD_NC_PRIVINT_ADVANCED), khm_hInstance)
        {}

        void OnCommand(int id, HWND hwndCtl, UINT codeNotify);

        LRESULT OnNotify(int id, NMHDR * pnmh);
    };

}
