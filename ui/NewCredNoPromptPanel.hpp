
namespace nim {

    class NewCredNoPromptPanel : public DialogWindow {
        khui_new_creds * nc;

        int             noprompt_flags;
                                /*!< Flags used by hwnd_noprompts.
                                  Combination of NC_NPF_* values. */

        /* The marquee is running */
#define NC_NPF_MARQUEE       0x0001

    public:
        NewCredNoPromptPanel(khui_new_creds * _nc):
            nc(_nc),
            DialogWindow(MAKEINTRESOURCE(IDD_NC_NOPROMPTS), khm_hInstance),
            noprompt_flags(0)
        {}

        void SetProgress(int progress, bool show);
    };
}
