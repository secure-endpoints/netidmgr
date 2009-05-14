
namespace nim {

    class NewCredPersistPanel : public DialogWindow {
        khui_new_creds * nc;

    public:
        NewCredPersistPanel(khui_new_creds * _nc) :
            nc(_nc),
            DialogWindow(MAKEINTRESOURCE(IDD_NC_PERSIST), khm_hInstance)
        {}

        BOOL OnCreate(LPVOID createParams);

        void OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    };
}
