
namespace nim {

    class NewCredAdvancedPanel : public DialogWindow {
        khui_new_creds * nc;

    private:
        HWND GetTabControl() {
            return GetItem(IDC_NC_TAB);
        }

    public:
        NewCredAdvancedPanel(khui_new_creds * _nc):
            nc(_nc),
            DialogWindow(MAKEINTRESOURCE(IDD_NC_PRIVINT_ADVANCED), khm_hInstance)
        {}

        void InitializeTabs();

        int  GetCurrentTabId();

        void GetTabPlacement(RECT& r);

        void OnCommand(int id, HWND hwndCtl, UINT codeNotify);

        LRESULT OnNotify(int id, NMHDR * pnmh);
    };

}
