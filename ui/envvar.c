/*
 * Copyright (c) 2009 Secure Endpoints Inc.
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

static const wchar_t * _exp_env_vars[] = {
    L"TMP",
    L"TEMP",
    L"APPDATA",
    L"USERPROFILE",
    L"ALLUSERSPROFILE",
    L"SystemRoot"
#if 0
    /* Replacing this is not typically useful */
    L"SystemDrive"
#endif
};

const int _n_exp_env_vars = ARRAYLENGTH(_exp_env_vars);

static int
check_and_replace_env_prefix(wchar_t * s, size_t cb_s, const wchar_t * env) {
    wchar_t evalue[MAX_PATH];
    DWORD len;
    size_t sz;

    evalue[0] = L'\0';
    len = GetEnvironmentVariable(env, evalue, ARRAYLENGTH(evalue));
    if (len > ARRAYLENGTH(evalue) || len == 0)
        return 0;

    if (_wcsnicmp(s, evalue, len))
        return 0;

    if (SUCCEEDED(StringCbPrintf(evalue, sizeof(evalue), L"%%%s%%%s",
                                 env, s + len)) &&
        SUCCEEDED(StringCbLength(evalue, sizeof(evalue), &sz)) &&
        sz <= cb_s) {
        StringCbCopy(s, cb_s, evalue);
    }

    return 1;
}

/* We use this instead of PathUnexpandEnvStrings() because that API
   doesn't take process TMP and TEMP.  The string is modified
   in-place.  The usual case is to call it with a buffer big enough to
   hold MAX_PATH characters. */
void
unexpand_env_var_prefix(wchar_t * s, size_t cb_s) {
    int i;

    for (i=0; i < _n_exp_env_vars; i++) {
        if (check_and_replace_env_prefix(s, cb_s, _exp_env_vars[i]))
            return;
    }
}
