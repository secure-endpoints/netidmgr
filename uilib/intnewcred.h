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

#ifndef NIMPRIVATE
#error  NIMPRIVATE must be defined for intnewcred.h to be used
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct tag_khui_new_creds_type_int;

/*! \internal
    \brief Privileged interaction panels

    These start life either on a ::khui_new_creds_type_int structure
    or on the nc_privint::legacy_panels structure depending on how it
    was created.  When the wizard is ready to show the panel, it will
    dequeue the panel object and enqueue it at the end of the
    nc_privint::shown structure.
*/
typedef struct tag_khui_new_creds_privint_panel {
    khm_int32  magic;

    HWND       hwnd;                            /*!< (Invariant) */

    wchar_t    caption[KCDB_MAXCCH_SHORT_DESC]; /*!< (Invariant) */

    khui_new_creds * nc;        /*!< New Credentials dialog that owns
                                  this panel.  */

    khm_int32  ctype;           /*!< Credentials type that provided
                                  this panel, if known. */

    khm_int32  identity_state;  /*!< Last reported identity state
                                   (reported through
                                   khui_cw_notify_identity_state()) */

    /* For basic custom prompting */
    khm_boolean use_custom;     /*!< Use custom prompting
                                   instead. (hwnd must be NULL).  This
                                   should only be set to TRUE if all
                                   the prompts have been specified. */

    khm_boolean processed;            /*!< This panel has already been
                                         processed. */

    wchar_t    *banner;         /*!< Banner text */
    wchar_t    *pname;          /*!< Heading */
    khui_new_creds_prompt ** prompts; /*!< Individual prompts */
    khm_size   n_prompts;            /*!< Number of prompts */
    khm_size   nc_prompts;           /*!< Total number of prompts allocated */

    LDCL(struct tag_khui_new_creds_privint_panel);
} khui_new_creds_privint_panel;

#define KHUI_NEW_CREDS_PRIVINT_PANEL_MAGIC 0x31e80e7b

#define IDC_NCC_PNAME  1001
#define IDC_NCC_BANNER 1002
#define IDC_NCC_CTL    1010

/*! \internal
    \brief A credentials type for a new credentials operation
 */
typedef struct tag_khui_new_creds_type_int {
    khui_new_creds_by_type * nct;    /*!< Credential type */
    const wchar_t * display_name;    /*!< Display name */
    khm_boolean is_id_credtype;      /*!< Is this the identity
                                        credentials type? */
    khm_int32  identity_state;  /*!< Identity state.  The state returned by a call to khui_cw_notify_identity_state() */

    QDCL(khui_new_creds_privint_panel); /*!< Queue of privileged
                                          interaction panels */
} khui_new_creds_type_int;



typedef struct tag_nc_privint {
    khui_new_creds_privint_panel * legacy_panel;
                                /*!< Legacy privileged interaction
                                  panel.  Only created if
                                  necessary. */

    struct {
        khui_new_creds_privint_panel * current_panel;
        khm_boolean show_blank; /*!< Show a blank panel instead of the
                                  tail of this queue. */
        QDCL(khui_new_creds_privint_panel);
    } shown;                    /*!< Queue of privileged interaction
                                  panels that have been shown.  The
                                  panels follow in chronological order
                                  from QTOP() to QBOTTOM().
                                  current_panel should always be
                                  pointing at one of these panels in
                                  the queue. */

    khm_boolean initialized;    /*!< Has the tab control been
                                  initialized? */
} nc_privint;



/*! \brief Notification data for the WMNC_IDENTITY_STATE notification

  \see khui_cw_notify_identity_state()
*/
typedef struct tag_nc_identity_state_notification {
    khm_int32      magic; /*!< == KHUI_NC_IDENTITY_STATE_NOTIF_MAGIC */

    HWND           owner;       /*!< Owner of the state notification.
                                   For credentials type panels, this
                                   can be the window handle of the
                                   credentials type panel.  For
                                   privileged interaction panels, this
                                   can be the window handle of the
                                   privileged interaction panel. */

    const wchar_t *state_string;
                                /*!< Localized string describing the
                                   current state or reason why the
                                   identity is not ready for
                                   credentials acquisition. */

    khm_int32      flags;      /*!< Combination of KHUI_CWNIS_* flags
                                   defined in khnewcred.h */

    khm_int32      progress;   /*!< 0..100 inclusive.  The progress
                                   that has been made so far on this
                                   operation. */

    khui_new_creds_privint_panel *privint_panel;
                                /*!< If the \a owner is a privileged
                                   interaction panel, then this will
                                   receive a pointer to the panel. */

    khui_new_creds_type_int      *credtype_panel;
                                /*!< If the \a owner is a credtype
                                   panel, then this will receive a
                                   pointer t othe credtype panel. */

} nc_identity_state_notification;

#define KHUI_NC_IDENTITY_STATE_NOTIF_MAGIC 0xcA1c2013

/*! \internal
    \brief New Credentials Operation Data
 */
typedef struct tag_khui_new_creds {

    /* --- Fields below this should be kept as is for backwards
           compatibility --- */

    khm_int32           magic;
                                /*!< Magic number.  Always KHUI_NC_MAGIC */

    khui_nc_subtype     subtype;
                                /*!< Subtype of the request that is
                                  being handled through this object.
                                  One of ::khui_nc_subtypes */

    CRITICAL_SECTION    cs;     /*!< Synchronization */

    khm_boolean         set_default;
                                /*!< After a successfull credentials
                                  acquisition, set the primary
                                  identity as the default. */

    khm_handle         *identities;
                                /*!< The list of identities associated
                                  with this request.  The first
                                  identity in this list (\a
                                  identities[0]) is the primary
                                  identity. */
    khm_size            n_identities;
                                /*!< Number of identities in the list
                                  \a identities */
    khm_size            nc_identities;
                                /*!< Number of handles allocated in \a
                                   identities */

    khui_action_context ctx;    /*!< An action context specifying the
                                  context in which the credentials
                                  acquisition operation was
                                  launced. */

    khm_int32           mode;   /*!< The mode of the user interface.
                                  One of ::KHUI_NC_MODE_MINI or
                                  ::KHUI_NC_MODE_EXPANDED. */

    HWND                hwnd;   /*!< Handle to the new credentials
                                  window. */

    khui_new_creds_type_int *types;
                                /*!< Credential types */
    khm_handle         *type_subs;
                                /*!< Type subscriptions.  We keep this
                                   list separate so we can use it with
                                   kmq_{send,post}_subs_msg(). */
    khm_size            n_types;
                                /*!< Number of types */
    khm_size            nc_types;
                                /*!< Number allocated */

    khm_int32           result;
                                /*!< One of ::KHUI_NC_RESULT_CANCEL or
                                  ::KHUI_NC_RESULT_PROCESS indicating
                                  the result of the dialog with the
                                  user */

    khm_int32           response;
                                /*!< Response.  See individual message
                                  documentation for info on what to do
                                  with this field when handling
                                  different subtypes. */

    void        *reserved1;         /*!< Not used. */
    void        *reserved2;         /*!< Not used */
    void        *reserved3;         /*!< Not used */
    khm_size    reserved4;          /*!< Not used */
    khm_size    reserved5;          /*!< Not used */
    void        *reserved6;         /*!< Not used */

    khui_ident_new_creds_cb reserved7; /*!< Not used */

    wchar_t            *window_title;  /*!< Internal use */

    void               *res_ident_aux;
                                /*!< Auxilliary field which is
                                  reserved for use by the identity
                                  provider. */

    /* --- Fields above this should be kept as is for backwards
           compatibility --- */

    khui_identity_selector *selectors;
                                /*!< Identity Selectors */
    khm_size            n_selectors;
                                /*!< Number of identity providers */
    khm_size            nc_selectors;
                                /*!< Internal */

    khm_handle          cs_privcred;
                                /*!< Credential set for transferring
                                   privileged credentials */
    khm_boolean         persist_privcred;
                                /*!< Should the privileged crdentials
                                   be saved? */
    khm_handle          persist_identity;
                                /*!< identity which will be used to
                                  store privileged credentials. */

    nc_privint          privint; /*!< Privileged interaction */

    khm_boolean         ignore_errors;
                                /*!< Don't report errors in separate
                                   alert boxes.  Usually set when
                                   there's already a UI for reporting
                                   errors to the user. */

    struct tag_khui_new_creds * parent;
                                /*!< If this new credentials operation
                                   was intiated by another new
                                   credentials operaiton (i.e. if this
                                   is a derived identity acquisition
                                   based on privileged credentials
                                   from another new credentials
                                   operation), this field is used to
                                   point to the parent. */

    khm_int32           n_children;
                                /*!< Number of child operations that
                                  we are waiting for. */

    void                *wizard;
                                /*!< Used to hold a pointer to the
                                  dialog data structure. */

} khui_new_creds;

#define KHUI_NC_MAGIC 0x84270427

KHMEXP khm_int32 KHMAPI
khui_cw_peek_next_privint(khui_new_creds * c,
                          khui_new_creds_privint_panel ** ppp);

KHMEXP khm_int32 KHMAPI
khui_cw_get_next_privint(khui_new_creds * c,
                         khui_new_creds_privint_panel ** ppp);

KHMEXP khm_int32 KHMAPI
khui_cw_free_privint(khui_new_creds_privint_panel * pp);

KHMEXP khm_int32 KHMAPI
khui_cw_clear_all_privints(khui_new_creds * c);

KHMEXP khm_int32 KHMAPI
khui_cw_purge_deleted_shown_panels(khui_new_creds * c);

KHMEXP khui_new_creds_privint_panel * KHMAPI
khui_cw_get_current_privint_panel(khui_new_creds * c);

KHMEXP void KHMAPI
khui_cw_set_current_privint_panel(khui_new_creds * c,
                                  khui_new_creds_privint_panel * p);

KHMEXP khm_boolean KHMAPI
khui_cw_is_ready(khui_new_creds * c);

typedef struct khui_collect_privileged_cred_data {
    khui_new_creds * nc;
    khm_handle       target_identity;
    khm_handle       dest_credset;
} khui_collect_privileged_creds_data;

#ifdef __cplusplus
}
#endif

#endif
