/*
 * Copyright (c) 2009-2010 Secure Endpoints Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
