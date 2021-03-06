/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
 * Copyright (c) 2006-2010 Secure Endpoints Inc.
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

/* Adapted from multiple Leash header files */

#ifndef __KHIMAIRA_KRB5COMMON_H
#define __KHIMAIRA_KRB5COMMON_H

#include<krb5.h>
#include<com_err.h>

krb5_error_code
khm_krb5_error(krb5_error_code rc, LPCSTR FailedFunctionName,
               int FreeContextFlag, krb5_context *ctx,
               krb5_ccache *cache);

int khm_krb5_initialize(khm_handle ident, krb5_context *, krb5_ccache *);

khm_int32 KHMAPI
khm_krb5_find_ccache_for_identity(khm_handle ident, krb5_context *pctx,
                                  void * buffer, khm_size * pcbbuf);

khm_int32 KHMAPI
khm_get_identity_expiration_time(krb5_context ctx, krb5_ccache cc,
                                 khm_handle ident,
                                 krb5_timestamp * pexpiration);

#ifdef HEIMDAL

#define IS_IMPORTED(sym) (sym != NULL)

#else

#define IS_IMPORTED(sym) (p ## sym != NULL)

#define error_message		(*perror_message)
#define krb5_cc_close		(*pkrb5_cc_close)
#define krb5_cc_default		(*pkrb5_cc_default)
#define krb5_cc_end_seq_get	(*pkrb5_cc_end_seq_get)
#define krb5_cc_get_principal	(*pkrb5_cc_get_principal)
#define krb5_cc_next_cred	(*pkrb5_cc_next_cred)
#define krb5_cc_resolve		(*pkrb5_cc_resolve)
#define krb5_cc_set_flags	(*pkrb5_cc_set_flags)
#define krb5_cc_start_seq_get	(*pkrb5_cc_start_seq_get)
#define krb5_free_context 	(*pkrb5_free_context)
#define krb5_free_principal 	(*pkrb5_free_principal)
#define krb5_free_unparsed_name	(*pkrb5_free_unparsed_name)
#define krb5_init_context	(*pkrb5_init_context)
#define krb5_timeofday		(*pkrb5_timeofday)
#define krb5_unparse_name	(*pkrb5_unparse_name)

#endif

#endif
