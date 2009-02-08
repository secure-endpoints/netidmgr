namespace nim {

    class NewCredProgress : public DialogWindow {
    public:
        khui_new_creds *nc;

        HWND            hwnd_container;

        NewCredProgress(khui_new_creds * _nc) :
            DialogWindow( MAKEINTRESOURCE(IDD_NC_PROGRESS), khm_hInstance ),
            nc(_nc) { }
    };

}
