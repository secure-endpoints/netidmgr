/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
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

#ifndef __KHIMAIRA_NEWCREDWND_H
#define __KHIMAIRA_NEWCREDWND_H

#define NIMPRIVATE
#include<khuidefs.h>
#include<intnewcred.h>

void khm_register_newcredwnd_class(void);
void khm_unregister_newcredwnd_class(void);
HWND khm_create_newcredwnd(HWND parent, khui_new_creds * c);
void khm_prep_newcredwnd(HWND hwnd);
void khm_show_newcredwnd(HWND hwnd);

/* the first control ID that may be used by an identity provider */
#define NC_IS_CTRL_ID_MIN 8016

/* the maximum number of controls that may be created by an identity
   provider*/
#define NC_IS_CTRL_MAX_CTRLS 8

/* the maximum control ID that may be used by an identity provider */
#define NC_IS_CTRL_ID_MAX (NC_IS_CTRL_ID_MIN + NC_IS_MAX_CTRLS - 1)

#endif
