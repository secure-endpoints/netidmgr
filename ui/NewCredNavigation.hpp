namespace nim {

    class NewCredNavigation : public DialogWindow {
        khui_new_creds * nc;

        int             m_controls; /*!< Combination of enum Controls */

        int             m_state; /*!< State flags */

    public:

        enum Controls {
            Next        = (1L<<0),
            Prev        = (1L<<1),
            Finish      = (1L<<2),
            Abort       = (1L<<3),
            ShowCloseIf = (1L<<4),
            Close       = (1L<<5),
            Retry       = (1L<<6)
        };

        enum States {
            PreEnd      = (1L<<0),
            NoClose     = (1L<<1),
            OkToFinish  = (1L<<2),
            Cancelled   = (1L<<3)
        };

    public:
        NewCredNavigation( khui_new_creds * _nc ) :
            DialogWindow(MAKEINTRESOURCE(IDD_NC_NAV), khm_hInstance),
            nc(_nc),
            m_controls(0),
            m_state(0)
        {}

        void CheckControls();

        khm_int32 EnableControl(int t) {
            m_controls |= t;
            return m_controls;
        }

        khm_int32 DisableControl(int t) {
            m_controls &= ~t;
            return m_controls;
        }

        khm_int32 SetAllControls(int t) {
            m_controls = t;
            return m_controls;
        }

        bool IsControlEnabled(int t) {
            return (m_controls & t) == t;
        }

        khm_int32 EnableState(int s) {
            m_state |= s;
            return m_state;
        }

        khm_int32 DisableState(int s) {
            m_state &= ~s;
            return m_state;
        }

        khm_int32 SetAllState(int s) {
            return (m_state = s);
        }

        bool IsState(int t) {
            return (m_state & t) == t;
        }

        HWND UpdateLayout();

        void OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    };
}
