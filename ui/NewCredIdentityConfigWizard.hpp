
namespace nim {

    class NewCredIdentityConfigWizard : public DialogWindow {
        khui_new_creds * nc;

    public:
        NewCredIdentityConfigWizard(khui_new_creds * _nc):
            nc(_nc),
            DialogWindow(MAKEINTRESOURCE(IDD_NC_PRIVINT_WIZARD), khm_hInstance)
        {}
    };

}
