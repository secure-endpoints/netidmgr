/*
 * Copyright (c) 2010 Secure Endpoints Inc.
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

/* $Id$ */

#include "krbcred.h"

khm_int32 init_error_funcs()
{
    initialize_krb5_error_table();
    return KHM_ERROR_SUCCESS;
}

khm_int32 exit_error_funcs()
{
    return KHM_ERROR_SUCCESS;
}

#ifdef DEPRECATED_REMOVABLE
HWND GetRootParent (HWND Child)
{
    HWND Last;
    while (Child)
    {
        Last = Child;
        Child = GetParent (Child);
    }
    return Last;
}
#endif

void khm_err_describe(long code, wchar_t * buf, khm_size cbbuf,
                      DWORD * suggestion,
                      kherr_suggestion * suggest_code)
{
    const char * com_err_msg;
    long table_num;
    DWORD msg_id = 0;
    DWORD sugg_id = 0;
    kherr_suggestion sugg_code = KHERR_SUGGEST_NONE;

    if (buf == NULL || cbbuf == 0)
        return;

    *buf = L'\0';

    table_num = (code & ~0xff);
    com_err_msg = error_message(code);

    if (suggestion)
        *suggestion = 0;
    if (sugg_code)
        *suggest_code = KHERR_SUGGEST_NONE;

    if (WSABASEERR <= code && code < (WSABASEERR + 1064)) {
        /* winsock error */
        table_num = WSABASEERR;
    }

    if (table_num == ERROR_TABLE_BASE_krb5) {
        switch(code) {
        case KRB5KDC_ERR_NAME_EXP:           /* 001 Principal expired */
        case KRB5KDC_ERR_SERVICE_EXP:        /* 002 Service expired */
        case KRB5KDC_ERR_PRINCIPAL_NOT_UNIQUE: /* 009 Principal not unique */
        case KRB5KDC_ERR_NULL_KEY:           /* 010 Principal has null key */
            msg_id = MSG_ERR_UNKNOWN;
            sugg_code = KHERR_SUGGEST_RETRY;
            break;

        case KRB5KDC_ERR_C_PRINCIPAL_UNKNOWN: /* 008 Principal unknown */
        case KRB5KDC_ERR_S_PRINCIPAL_UNKNOWN: /* 008 Principal unknown */
            msg_id = MSG_ERR_PR_UNKNOWN;
            sugg_code = KHERR_SUGGEST_RETRY;
            break;

        case KRB5KRB_AP_ERR_SKEW:             /* 037 delta_t too big */
        case KRB5_KDCREP_SKEW:	              /* 037 delta_t too big */
            msg_id = MSG_ERR_CLOCKSKEW;
            sugg_id = MSG_ERR_S_CLOCKSKEW;
            sugg_code = KHERR_SUGGEST_RETRY;
            break;

        case KRB5_KDC_UNREACH:
            msg_id = MSG_ERR_KDC_CONTACT;
            break;

	case KRB5KRB_AP_ERR_BAD_INTEGRITY:
	    sugg_code = MSG_ERR_S_INTEGRITY;
	    break;
        }
    } else if (table_num == ERROR_TABLE_BASE_kadm5) {
        switch(code) {
        case KADM5_PASS_Q_TOOSHORT:
	case KADM5_PASS_Q_CLASS:
	case KADM5_PASS_Q_DICT:
	    /* TODO: Retrieve extended error information */
            msg_id = MSG_ERR_INSECURE_PW;
            sugg_code = KHERR_SUGGEST_RETRY;
            break;
        }
    } else if (table_num == WSABASEERR) {
        switch(code) {
        case WSAENETDOWN:
            msg_id = MSG_ERR_NETDOWN;
            sugg_id = MSG_ERR_S_NETRETRY;
            sugg_code = KHERR_SUGGEST_RETRY;
            break;

        case WSATRY_AGAIN:
            msg_id = MSG_ERR_TEMPDOWN;
            sugg_id = MSG_ERR_S_TEMPDOWN;
            sugg_code = KHERR_SUGGEST_RETRY;
            break;

        case WSAENETUNREACH:
        case WSAENETRESET:
        case WSAECONNABORTED:
        case WSAECONNRESET:
        case WSAETIMEDOUT:
        case WSAECONNREFUSED:
        case WSAEHOSTDOWN:
        case WSAEHOSTUNREACH:
            msg_id = MSG_ERR_NOHOST;
            sugg_id = MSG_ERR_S_NOHOST;
            sugg_code = KHERR_SUGGEST_RETRY;
            break;
        }
    } else {
	sugg_code = KHERR_SUGGEST_RETRY;
    }

    if (msg_id != 0) {
        FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      KHERR_HMODULE,
                      msg_id,
                      0,
                      buf,
                      (int) (cbbuf / sizeof(buf[0])),
                      NULL);
    } else if (com_err_msg) {
        AnsiStrToUnicode(buf, cbbuf, com_err_msg);
    }

    if (sugg_id != 0 && suggestion != NULL)
        *suggestion = sugg_id;

    if (sugg_code != KHERR_SUGGEST_NONE && suggest_code != NULL)
        *suggest_code = sugg_code;
}

#ifdef DEPRECATED_REMOVABLE
int lsh_com_err_proc (LPSTR whoami, long code,
                              LPSTR fmt, va_list args)
{
    int retval;
    HWND hOldFocus;
    char buf[1024], *cp;
    WORD mbformat = MB_OK | MB_ICONEXCLAMATION;

    cp = buf;
    memset(buf, '\0', sizeof(buf));
    cp[0] = '\0';

    if (code)
    {
        err_describe(buf, code);
        while (*cp)
            cp++;
    }

    if (fmt)
    {
        if (fmt[0] == '%' && fmt[1] == 'b')
	{
            fmt += 2;
            mbformat = va_arg(args, WORD);
            /* if the first arg is a %b, we use it for the message
               box MB_??? flags. */
	}
        if (code)
	{
            *cp++ = '\n';
            *cp++ = '\n';
	}
        wvsprintfA((LPSTR)cp, fmt, args);
    }
    hOldFocus = GetFocus();
    retval = MessageBoxA(/*GetRootParent(hOldFocus)*/NULL, buf, whoami,
                        mbformat | MB_ICONHAND | MB_TASKMODAL);
    SetFocus(hOldFocus);
    return retval;
}
#endif
