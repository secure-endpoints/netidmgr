/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
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

#ifndef __KHIMAIRA_NEWCREDWND_H_INTERNAL
#define __KHIMAIRA_NEWCREDWND_H_INTERNAL

#include "khnewcred.h"

#ifndef NOEXPORT
#error  NOEXPORT must be defined for intnewcred.h to be used
#endif

/*! \internal
    \brief Privileged interaction queue */
typedef struct tag_khui_new_creds_privint {
    HWND       hwnd;            /* Invariant */
    wchar_t    caption[KCDB_MAXCCH_SHORT_DESC]; /* Invariant */

    LDCL(struct tag_khui_new_creds_privint);
} khui_new_creds_privint;

/*! \internal
    \brief Internal representation for a credentials type for a new credentials operation
 */
typedef struct tag_khui_new_creds_type_int {
    khui_new_creds_by_type * nct; /*!< Credential type */
    khm_handle sub;               /*!< Subscription */

    QDCL(khui_new_creds_privint);
} khui_new_creds_type_int;

/*! \internal
    \brief Internal structure for keeping track of identity providers during new credentials operation */
typedef struct tag_khui_new_creds_idpro {
    khm_handle h;               /*!< Handle to identity provider */
    khui_ident_new_creds_cb cb; /*!< UI Callback */

    HICON      icon_lg;
    void *     data;            /*!< Provider data */
} khui_new_creds_idpro;

/*! \internal
    \brief New Credentials Operation Data
 */
typedef struct tag_khui_new_creds {

    /* --- Fields below this should be kept as is for backwards
           compatibility --- */

    khm_int32   magic;          /*!< Magic number.  Always KHUI_NC_MAGIC */

    khm_int32   subtype;        /*!< Subtype of the request that is
                                  being handled through this object.
                                  One of ::KMSG_CRED_NEW_CREDS,
                                  ::KMSG_CRED_RENEW_CREDS or
                                  ::KMSG_CRED_PASSWORD */

    CRITICAL_SECTION cs;        /*!< Synchronization */

    khm_boolean set_default;    /*!< After a successfull credentials
                                  acquisition, set the primary
                                  identity as the default. */

    khm_handle  *identities;    /*!< The list of identities associated
                                  with this request.  The first
                                  identity in this list (\a
                                  identities[0]) is the primary
                                  identity. */

    khm_size    n_identities;   /*!< Number of identities in the list
                                  \a identities */

    khm_size    nc_identities;  /*!< Number of handles allocated in \a
                                   identities */

    khui_action_context ctx;    /*!< An action context specifying the
                                  context in which the credentials
                                  acquisition operation was
                                  launced. */

    khm_int32   mode;           /*!< The mode of the user interface.
                                  One of ::KHUI_NC_MODE_MINI or
                                  ::KHUI_NC_MODE_EXPANDED. */

    HWND        hwnd;           /*!< Handle to the new credentials
                                  window. */

    khui_new_creds_type_int *types;
                                /*!< Credential types */
    void        *reserved0;     /*!< Not used */
    khm_size    n_types;        /*!< Number of types */
    khm_size    nc_types;       /*!< Number allocated */

    khm_int32   result;     /*!< One of ::KHUI_NC_RESULT_CANCEL or
                                ::KHUI_NC_RESULT_PROCESS indicating
                                the result of the dialog with the
                                user */

    khm_int32   response;   /*!< Response.  See individual message
                                documentation for info on what to do
                                with this field when handling
                                different subtypes. */

    void        *reserved1;  /*!< Not used. */

    /* UI stuff */

    void        *reserved2;         /*!< Not used */
    void        *reserved3;         /*!< Not used */
    khm_size    reserved4;          /*!< Not used */
    khm_size    reserved5;          /*!< Not used */
    void        *reserved6;         /*!< Not used */

    khui_ident_new_creds_cb reserved7; /*!< Not used */

    wchar_t     *window_title;  /*!< Internal use */

    void        *res_ident_aux; /*!< Auxilliary field which is
                                  reserved for use by the identity
                                  provider during the course of
                                  conducting this dialog. */

    /* --- Fields above this should be kept as is for backwards
           compatibility --- */

    khui_new_creds_idpro * providers; /*!< Identity providers */
    khm_size    n_providers;
    khm_size    nc_providers;

} khui_new_creds;

#define KHUI_NC_MAGIC 0x84270427

KHMEXP khm_int32 KHMAPI
khui_cw_find_provider(khui_new_creds * c,
                      khm_handle h_idpro,
                      khui_new_creds_idpro ** p);

KHMEXP khm_int32 KHMAPI
khui_cw_add_provider(khui_new_creds * c,
                     khm_handle       h_idpro);

KHMEXP khm_int32 KHMAPI
khui_cw_del_provider(khui_new_creds * c,
                     khm_handle       h_idpro);

KHMEXP khm_int32 KHMAPI
khui_cw_get_next_privint(khui_new_creds * c,
                         khui_new_creds_privint ** ppp);

KHMEXP khm_int32 KHMAPI
khui_cw_free_privint(khui_new_creds_privint * pp);

#endif
