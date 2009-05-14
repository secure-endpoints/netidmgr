
namespace nim {

    class NewCredIdentitySpecifier : public DialogWindow {
        khui_new_creds *nc;

        khm_boolean     initialized; 
                                /*< Has the identity provider list
                                  been initialized? */

        khm_boolean     in_layout;
                                /*< Are we in nc_layout_idspec()?
                                  This field is used to suppress event
                                  handling when we are modifying some
                                  of the controls. */

        khm_ssize       idx_current;
                                /*< Index of last provider */

        NewCredPage     prev_page;
                                /*< Previous page, if we allow back
                                  navigation */

        friend class NewCredWizard;

    public:
        NewCredIdentitySpecifier(khui_new_creds * _nc):
            nc(_nc),
            DialogWindow(MAKEINTRESOURCE(IDD_NC_IDSPEC), khm_hInstance)
        {}

        void Initialize(HWND hwnd);

        HWND UpdateLayout();

        bool ProcessNewIdentity();

        LRESULT OnNotify(int id, NMHDR * pnmh);
    };

}
