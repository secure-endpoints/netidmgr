
namespace nim {

    class NewCredWizard;

    class NewCredIdentitySelector : public DialogWindow {
        class IdentityDisplayData {
        public:
            Identity            identity;
            std::wstring        display_string;
            std::wstring        status;
            std::wstring        type_string;
            HICON               icon;

            IdentityDisplayData(const Identity& _identity):
                identity(_identity),
                display_string( _identity.GetResourceString(KCDB_RES_DISPLAYNAME) ),
                type_string( _identity.GetProvider().GetResourceString(KCDB_RES_INSTANCE) ),
                icon( _identity.GetResourceIcon(KCDB_RES_ICON_NORMAL) )
            {}

            IdentityDisplayData()
            {}
        };

        IdentityDisplayData     *m_current;

        khui_new_creds          *nc;

    public:
        NewCredIdentitySelector(khui_new_creds * _nc):
            nc(_nc),
            m_current(NULL),
            DialogWindow(MAKEINTRESOURCE(IDD_NC_IDSEL), khm_hInstance)
        {}

        void    UpdateLayout();

        void    SetStatus(const wchar_t * status);

        void    ShowIdentitySelector();

        LRESULT OnNotifyIdentitySelector(NMHDR * pnmh);

        LRESULT OnNotify(int id, NMHDR * pnmh);

        void    OnCommand(int id, HWND hwndCtl, UINT codeNotify);

        LRESULT OnDrawItem( DRAWITEMSTRUCT * lpDrawItem );

        LRESULT OnMeasureItem( MEASUREITEMSTRUCT * lpMeasureItem );
    };
}
