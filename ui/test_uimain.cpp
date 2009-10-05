#include "khmapp.h"
#include <assert.h>

namespace nim {

    extern "C" int test_Magic = 0;

    extern "C" void KHMCALLBACK test_alerts(void);

    struct test_data {
        khm_int32 test_action;
        const wchar_t * test_name;
        const wchar_t * test_desc;
        void (KHMCALLBACK *test_func)(void);
    };

    static test_data tests[] = {
        { 0, L"KhmTestAlerts", L"Test Alerts", test_alerts },
    };

    static const int n_tests = ARRAYLENGTH(tests);

    static khm_int32 act_menu = 0;

    extern "C" void
    test_action_trigger(int action)
    {
        for (int i = 0; i < n_tests; i++) {
            if (tests[i].test_action == action) {
                (*tests[i].test_func)();
                break;
            }
        }
    }

    khm_int32
    register_action(const wchar_t * name,
                    const wchar_t * display,
                    const wchar_t * tooltip,
                    void * userdata)
    {
        khm_handle h_sub;

        kmq_create_hwnd_subscription(khm_hwnd_main, &h_sub);
        return khui_action_create(name, display, tooltip, userdata,
                                  KHUI_ACTIONTYPE_TRIGGER,
                                  h_sub);
    }

    extern "C" void
    test_init(void) {
        khui_menu_def * mm;
        khui_menu_def * mt;

        mm = khui_find_menu(KHUI_MENU_MAIN);
        assert(mm);

        act_menu = register_action(L"KhmTestMenu", L"Tests", L"Run tests", NULL);

        mt = khui_menu_create(act_menu);

        for (int i=0; i < n_tests; i++) {
            tests[i].test_action = register_action(tests[i].test_name,
                                                   tests[i].test_desc,
                                                   tests[i].test_desc,
                                                   TESTACTION_MAGIC);
            khui_menu_insert_action(mt, -1, tests[i].test_action, 0);
        }

        khui_action_lock();

        khui_menu_insert_action(mm, -1, act_menu, KHUI_ACTIONREF_SUBMENU);

        khui_action_unlock();

        khui_refresh_actions();
    }

    extern "C" void
    test_exit(void) {
    }

}
