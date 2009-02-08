
namespace nim {

    class NewCredBasicPanel : public DialogWindow {
        khui_new_creds * nc;

    public:
        NewCredBasicPanel(khui_new_creds * _nc):
            nc(_nc),
            DialogWindow(MAKEINTRESOURCE(IDD_NC_PRIVINT_BASIC), khm_hInstance)
        {}

        void OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    };
}
