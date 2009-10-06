#include "khmapp.h"

namespace nim {

    extern "C" void KHMCALLBACK test_alert_group()
    {
        wchar_t title[80];
        wchar_t message[200];
        wchar_t suggestion[200];

        for (int i=0; i < 10; i++) {
            khui_alert * a;

            StringCbPrintf(title, sizeof(title), L"Alert number %d", i + 1);
            StringCbPrintf(message, sizeof(message),
                           L"This is the alert message.  The text of the message may be split "
                           L"across multiple lines.  This is the text of alert number %d.", i + 1);
            StringCbPrintf(suggestion, sizeof(suggestion),
                           L"Suggestion goes here.  This may also be used to specify some "
                           L"additional information, like additional debug information "
                           L"or support information");

            khui_alert_create_empty(&a);
            khui_alert_set_title(a, title);
            khui_alert_set_message(a, message);
            khui_alert_set_suggestion(a, suggestion);
            khui_alert_set_severity(a,
                                    (i % 3 == 0)? KHERR_ERROR :
                                    (i % 3 == 1)? KHERR_WARNING :
                                    (i % 3 == 2)? KHERR_INFO : 0);

            khui_alert_add_command(a, KHUI_ACTION_NEW_CRED);
            khui_alert_add_command(a, KHUI_PACTION_CLOSE);
            khui_alert_add_command(a, KHUI_ACTION_RENEW_CRED);
            khui_alert_add_command(a, KHUI_ACTION_DESTROY_CRED);
            khui_alert_add_command(a, KHUI_ACTION_OPEN_APP);
            khui_alert_set_type(a, KHUI_ALERTTYPE_TEST);
            khui_alert_show(a);
        }
    }

    extern "C" void KHMCALLBACK test_alert_dispatch()
    {
        wchar_t title[80];
        wchar_t message[200];
        wchar_t suggestion[200];

        for (int i=0; i < 5; i++) {
            khui_alert * a;

            StringCbPrintf(title, sizeof(title), L"Alert number %d", i + 1);
            StringCbPrintf(message, sizeof(message),
                           L"This is the alert message.  The text of the message may be split "
                           L"across multiple lines.  This is the text of alert number %d.", i + 1);
            StringCbPrintf(suggestion, sizeof(suggestion),
                           L"Suggestion goes here.  This may also be used to specify some "
                           L"additional information, like additional debug information "
                           L"or support information");

            khui_alert_create_empty(&a);
            khui_alert_set_title(a, title);
            khui_alert_set_message(a, message);
            khui_alert_set_suggestion(a, suggestion);
            khui_alert_set_severity(a,
                                    (i % 3 == 0)? KHERR_ERROR :
                                    (i % 3 == 1)? KHERR_WARNING :
                                    (i % 3 == 2)? KHERR_INFO : 0);

            khui_alert_add_command(a, KHUI_ACTION_NEW_CRED);
            khui_alert_add_command(a, KHUI_PACTION_CLOSE);
            khui_alert_add_command(a, KHUI_ACTION_RENEW_CRED);
            khui_alert_add_command(a, KHUI_ACTION_DESTROY_CRED);
            khui_alert_add_command(a, KHUI_ACTION_OPEN_APP);
            khui_alert_set_flags(a, KHUI_ALERT_FLAG_DISPATCH_CMD, KHUI_ALERT_FLAG_DISPATCH_CMD);
            khui_alert_set_type(a, KHUI_ALERTTYPE_TEST);
            khui_alert_show(a);
        }
    }
}
