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

#ifndef __KHIMAIRA_KRBAFSCRED_H
#define __KHIMAIRA_KRBAFSCRED_H

#define WIN32_LEAN_AND_MEAN
#include<windows.h>

/* While we generally pull resources out of hResModule, the message
   strings for all the languages are kept in the main DLL. */
#define KHERR_HMODULE hInstance
#define KHERR_FACILITY k5_facility
#define KHERR_FACILITY_ID 64

#include<netidmgr.h>

#include "krb5funcs.h"
#include "errorfuncs.h"

#include "langres.h"
#include "datarep.h"
#include "krb5_msgs.h"

#define TYPENAME_ENCTYPE        L"EncType"
#define TYPENAME_ADDR_LIST      L"AddrList"
#define TYPENAME_KRB5_FLAGS     L"Krb5Flags"
#define TYPENAME_KRB5_PRINC     L"Krb5Principal"
#define TYPENAME_KVNO           L"Kvno"

#define ATTRNAME_KEY_ENCTYPE    L"KeyEncType"
#define ATTRNAME_TKT_ENCTYPE    L"TktEncType"
#define ATTRNAME_ADDR_LIST      L"AddrList"
#define ATTRNAME_KRB5_FLAGS     L"Krb5Flags"
#define ATTRNAME_KRB5_CCNAME    L"Krb5CCName"
#define ATTRNAME_KVNO           L"Kvno"
#define ATTRNAME_KRB5_IDFLAGS   L"Krb5IDFlags"
#define ATTRNAME_KRB5_PKEYV1    L"Krb5PrivateKey1"

/* Flag bits for Krb5IDFlags property */

/* identity was imported from MSLSA: */
#define K5IDFLAG_IMPORTED       0x00000001

/* Configuration spaces */
#define CSNAME_KRB5CRED      L"Krb5Cred"
#define CSNAME_PARAMS        L"Parameters"
#define CSNAME_PROMPTCACHE   L"PromptCache"
#define CSNAME_REALMS        L"Realms"

/* plugin constants */
#define KRB5_PLUGIN_NAME    L"Krb5Cred"
#define KRB5_IDENTPRO_NAME  L"Krb5Ident"

#define KRB5_CREDTYPE_NAME  L"Krb5Cred"

/* limits */
/* Maximum number of characters in a realm name */
#define K5_MAXCCH_REALM       256

/* Maximum number of characters in a host name */
#define K5_MAXCCH_HOST        128

/* Maximum number of KDC's per realm */
#define K5_MAX_KDC             64

/* Maximum number of domains that map to a realm */
#define K5_MAX_DOMAIN_MAPPINGS 32

/* Maximum number of characters in a credentials cache name */
#define KRB5_MAXCCH_CCNAME   1024

typedef enum tag_k5_lsa_import {
    K5_LSAIMPORT_NEVER = 0,
    K5_LSAIMPORT_ALWAYS = 1,
    K5_LSAIMPORT_MATCH = 2,     /* only when the principal name matches */
} k5_lsa_import;

/* globals */
extern kmm_module     h_khModule;
extern HMODULE        hResModule;
extern HINSTANCE      hInstance;
extern const wchar_t *k5_facility;

extern khm_int32  type_id_enctype;
extern khm_int32  type_id_addr_list;
extern khm_int32  type_id_krb5_flags;
extern khm_int32  type_id_krb5_princ;
extern khm_int32  type_id_kvno;

extern khm_int32  attr_id_key_enctype;
extern khm_int32  attr_id_tkt_enctype;
extern khm_int32  attr_id_addr_list;
extern khm_int32  attr_id_krb5_flags;
extern khm_int32  attr_id_krb5_ccname;
extern khm_int32  attr_id_kvno;
extern khm_int32  attr_id_krb5_idflags;
extern khm_int32  attr_id_krb5_pkeyv1;

extern khm_ui_4   k5_commctl_version;
#define IS_COMMCTL6() (k5_commctl_version >= 0x60000)

extern khm_handle csp_plugins;
extern khm_handle csp_krbcred;
extern khm_handle csp_params;

extern kconf_schema schema_krbconfig[];

extern khm_int32  credtype_id_krb5;

extern khm_boolean krb5_initialized;

extern khm_handle krb5_credset;

extern khm_handle k5_sub;

extern krb5_context k5_identpro_ctx;

extern khm_handle k5_identpro;

/* plugin callbacks */
khm_int32 KHMAPI
k5_msg_callback(khm_int32 msg_type, khm_int32 msg_subtype, khm_ui_4 uparam, void * vparam);

khm_int32 KHMAPI
k5_ident_callback(khm_int32 msg_type, khm_int32 msg_subtype, khm_ui_4 uparam, void * vparam);

enum k5_kinit_task_state {
    K5_KINIT_STATE_NONE,        /*!< Unknown state */
    K5_KINIT_STATE_PREP,        /*!< Prepared for kinit */

    K5_KINIT_STATE_INCALL,      /*!< Called krb5_get_init_creds_password() */
    K5_KINIT_STATE_RETRY,       /*!< Should retry calling gic_pwd() */
    K5_KINIT_STATE_WAIT,        /*!< Waiting for user confirmation */
    K5_KINIT_STATE_CONFIRM,     /*!< Received user confirmation */

    K5_KINIT_STATE_ABORTED,     /*!< The kinit task has been aborted. */
    K5_KINIT_STATE_DONE         /*!< Task completed */
};

typedef struct tag_k5_dlg_data k5_dlg_data;

/* kinit task data */
typedef struct tag_kinit_task {
    khm_int32               magic; /* == K5_KINIT_TASK_MAGIC */
    khui_new_creds         *nc;
    khui_new_creds_by_type *nct;
    k5_dlg_data            *dlg_data;

    khm_handle              identity;
    char                   *principal;
    char                   *password;
    char                   *ccache;
    k5_params               params;
    krb5_context            context;

    krb5_error_code         kinit_code;
    int                     prompt_set_index;

    khm_boolean             is_null_password;
    khm_boolean             is_valid_principal;

    khm_int32               refcount;

    enum k5_kinit_task_state state;
    CRITICAL_SECTION        cs;
    HANDLE                  h_task_wait;
    HANDLE                  h_parent_wait;

    khm_task                task;
} k5_kinit_task;

#define K5_KINIT_TASK_MAGIC 0xc623477e

k5_kinit_task *
k5_kinit_task_create(khui_new_creds * nc);

void
k5_kinit_task_hold(k5_kinit_task * kt);

void
k5_kinit_task_release(k5_kinit_task * kt);

void
k5_kinit_task_abort_and_release(k5_kinit_task * kt);

khm_int32
k5_kinit_task_confirm_and_wait(k5_kinit_task * kt);

/* kinit dialog data */
typedef struct tag_k5_dlg_data {
    khui_new_creds_by_type  nct;

    khui_tracker            tc_lifetime;
    khui_tracker            tc_renew;

    khm_boolean             dirty; /* is the data in sync with the
                                      configuration store? */
    khm_boolean             sync; /* is the data in sync with the kinit
                                     request? */
    k5_params               params;

    BOOL                    pwd_change; /* force a password change */

    k5_kinit_task          *kinit_task; /* currently executing kinit task */
} k5_dlg_data;

void
k5_reply_to_acqpriv_id_request(khui_new_creds * nc,
                               krb5_data * privdata);

LRESULT
k5_force_password_change(k5_dlg_data * d);

void
k5_pp_begin(khui_property_sheet * s);

void
k5_pp_end(khui_property_sheet * s);

khm_int32 KHMAPI
k5_msg_cred_dialog(khm_int32 msg_type,
                   khm_int32 msg_subtype,
                   khm_ui_4 uparam,
                   void * vparam);

khm_int32 KHMAPI
k5_msg_ident(khm_int32 msg_type,
               khm_int32 msg_subtype,
               khm_ui_4 uparam,
               void * vparam);

khm_int32
k5_remove_from_LRU(khm_handle identity);

int
k5_get_realm_from_nc(khui_new_creds * nc,
                     wchar_t * buf,
                     khm_size cch_buf);

void
k5_register_config_panels(void);

void
k5_unregister_config_panels(void);

INT_PTR CALLBACK
k5_ccconfig_dlgproc(HWND hwnd,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam);

INT_PTR CALLBACK
k5_id_tab_dlgproc(HWND hwndDlg,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam);

INT_PTR CALLBACK
k5_ids_tab_dlgproc(HWND hwnd,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam);

khm_int32 KHMAPI
k5_idselector_factory(HWND hwnd_parent, khui_identity_selector * u);

#endif
