/*
 * Copyright (c) 2005-2008 Massachusetts Institute of Technology
 * Copyright (c) 2008 Secure Endpoints Inc.
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

#include<kcreddbinternal.h>

KHMEXP int KHMAPI AnsiStrToUnicode( wchar_t * wstr, size_t cbwstr, const char * astr)
{
    size_t nc;

    if(cbwstr == 0)
        return 0;

    nc = strlen(astr);
    if(nc == MultiByteToWideChar(
        CP_ACP, 
        0, 
        astr, 
        (int) nc, 
        wstr, 
        (int)(cbwstr / sizeof(wchar_t) - 1))) {
        wstr[nc] = L'\0';
    } else {
        wstr[0] = L'\0';
        nc = 0;
    }

    return (int) nc;
}

KHMEXP int KHMAPI UnicodeStrToAnsi( char * dest, size_t cbdest, const wchar_t * src)
{
    size_t nc;

    if(cbdest == 0)
        return 0;

    dest[0] = 0;

    if(FAILED(StringCchLength(src, cbdest, &nc)) || nc*sizeof(char) >= cbdest)
        // note that cbdest counts the terminating NULL, while nc doesn't
        return 0;

    nc = WideCharToMultiByte(
        CP_ACP, 
        WC_NO_BEST_FIT_CHARS, 
        src, 
        (int) nc, 
        dest, 
        (int) cbdest, 
        NULL, 
        NULL);

    dest[nc] = 0;

    return (int) nc;
}

KHMEXP khm_int32
FormatString(wchar_t * buf, khm_size cb_buf, const wchar_t * format, ...)
{
    int r;
    va_list vl;

    va_start(vl, format);
    r = FormatMessage(FORMAT_MESSAGE_FROM_STRING,
                      (LPCVOID) format, 0, 0,
                      buf, (DWORD) (cb_buf / sizeof(wchar_t)),
                      &vl);
    va_end(vl);

    if (r != 0)
        return KHM_ERROR_SUCCESS;
    else {
        /* TODO: Use GetLastError() and figure out a better return
           value. */
        return KHM_ERROR_UNKNOWN;
    }
}
