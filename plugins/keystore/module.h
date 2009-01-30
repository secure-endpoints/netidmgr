/*
 * Copyright (c) 2006 Secure Endpoints Inc.
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

/* only include this header file once */
#pragma once

#ifndef _UNICODE
#ifndef RC_INVOKED
/* This template relies on _UNICODE being defined to call the correct
   APIs. */
#error  This template needs to be compiled with _UNICODE
#endif
#endif

/* Pull in configuration macros from the Makefile */
#include "module_config.h"

/* declare a few macros about our plugin */

/* The following macro will be used throughout the template to refer
   to the name of the credentials provider plugin.  The macro is
   actually defined the Makefile generated configuration header file.
   Modify the CREDPROVNAME Makefile macro.*/
#ifndef CREDPROV_NAME
#error  CREDPROV_NAME not defined
#endif

/* Also define the unicde equivalent of the name.  In general strings
   in NetIDMgr are unicode. */
#define CREDPROV_NAMEW _T(CREDPROV_NAME)

/* The following macro will be used throughout the template to refer
   to the name of the identity provider plugin.  The macro is
   actually defined the Makefile generated configuration header file.
   Modify the IDPROVNAME Makefile macro.*/
#ifndef IDPROV_NAME
#error  IDPROV_NAME not defined
#endif

/* Also define the unicde equivalent of the name.  In general strings
   in NetIDMgr are unicode. */
#define IDPROV_NAMEW _T(IDPROV_NAME)

/* The name of the module.  This is distinct from the name of the
   plugin for several reasons.  One of which is that a single module
   can provide multiple plugins.  Also, having a module name distinct
   from a plugin name allows multiple vendors to provide the same
   plugin.  For example, the module name for the MIT Kerberos 5 plugin
   is MITKrb5 while the plugin name is Krb5Cred.  The macro is
   actually defined in the Makefile generated configuration header
   file.  Modify the MODULENAME Makefile macro.*/
#ifndef MODULE_NAME
#error  MODULE_NAME not defined
#endif

#define MODULE_NAMEW _T(MODULE_NAME)

/* When logging events from our plugin, the event logging API can
   optionally take a facility name to provide a friendly label to
   identify where each event came from.  We will default to the plugin
   name, although it can be anything. */
#define PLUGIN_FACILITYW CREDPROV_NAMEW

/* Base name of the DLL that will be providing the plugin.  We use it
   to construct names of the DLLs that will contain localized
   resources.  This is defined in the Makefile and fed in to the build
   through there.  The macro to change in the Makefile is
   DLLBASENAME. */
#ifndef PLUGIN_DLLBASE
#error  PLUGIN_DLLBASE Not defined!
#endif

#define PLUGIN_DLLBASEW _T(PLUGIN_DLLBASE)

/* Name of the credentials type that will be registered by the plugin.
   This macro is actually defined in the Makefile generated
   configuration header file.  Change the CREDTYPENAME macro in the
   Makefile. */
#ifndef CREDTYPE_NAME
#error  CREDTYPE_NAME not defined
#endif

#define CREDTYPE_NAMEW _T(CREDTYPE_NAME)

/* Configuration node names.  We just concatenate a few strings
   together, although you should feel free to completely define your
   own. */

#define CONFIGNODE_MAIN   CREDTYPE_NAMEW L"Config"
#define CONFIGNODE_ALL_ID CREDTYPE_NAMEW L"AllIdents"
#define CONFIGNODE_PER_ID CREDTYPE_NAMEW L"PerIdent"

/* For the keystore provider, we use an auxilliary credentials type to
   designate private keys, since those are distinct from the keystore
   credentials which represent keystores. */

#define IDK_CREDTYPE_NAME "KeystoreIdentityPrivateKeyInstance"
#define IDK_CREDTYPE_NAMEW _T(IDK_CREDTYPE_NAME)

#define KHERR_FACILITY PLUGIN_FACILITYW
#define KHERR_HMODULE  hResModule

#include<windows.h>
/* include the standard NetIDMgr header files */
#include<netidmgr.h>
#include<tchar.h>

/* declarations for language resources */
#include "langres.h"

#ifndef NOSTRSAFE
#include<strsafe.h>
#endif

#include "keystore.h"

#define KMSG_MYMSG            KMSGBASE_USER
#define KMSG_MYMSG_SET_CTYPE  1

/***************************************************
 Externals
***************************************************/

extern kmm_module h_khModule;
extern HINSTANCE  hInstance;
extern HMODULE    hResModule;

extern const wchar_t * my_facility;

extern khm_int32  credtype_id;
extern khm_int32  idk_credtype_id;
extern khm_handle h_idprov;
extern khm_handle idprov_sub;

extern CRITICAL_SECTION cs_ks;

/* Function declarations */

/* in credprov.c */
khm_int32 KHMAPI
credprov_msg_proc(khm_int32 msg_type,
                  khm_int32 msg_subtype,
                  khm_ui_4  uparam,
                  void * vparam);

khm_int32 KHMAPI
idk_credprov_msg_proc (khm_int32 msg_type, khm_int32 msg_subtype,
                       khm_ui_4  uparam, void *vparam);

void
list_credentials(void);

/* in idprov.c */
khm_int32 KHMAPI
idprov_msg_proc(khm_int32 msg_type,
                khm_int32 msg_subtype,
                khm_ui_4  uparam,
                void * vparam);

/* in credtype.c */
khm_int32 KHMAPI
cred_is_equal(khm_handle cred1,
              khm_handle cred2,
              void * rock);

khm_int32 KHMAPI
idk_cred_is_equal(khm_handle cred1,
                  khm_handle cred2,
                  void * rock);

/* in credacq.c */
khm_int32 KHMAPI
handle_cred_acq_msg(khm_int32 msg_type,
                    khm_int32 msg_subtype,
                    khm_ui_4  uparam,
                    void *    vparam);

void
creddlg_setup_idlist(HWND hwlist);

void
creddlg_refresh_idlist(HWND hwlist, keystore_t * ks);

/* in proppage.c */
INT_PTR CALLBACK
pp_cred_dlg_proc(HWND hwnd,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam);

/* in config_id.c */
INT_PTR CALLBACK
config_id_dlgproc(HWND hwndDlg,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam);

/* in config_ids.c */
INT_PTR CALLBACK
config_ids_dlgproc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam);

/* in config_main.c */
INT_PTR CALLBACK
config_dlgproc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam);

/* in idselect.c */
khm_int32
handle_kmsg_ident_get_idsel_factory(kcdb_idsel_factory * pcb);

/* credype.c : */

/* Protected by cs_ks */
extern khm_size n_keystores;
extern khm_size nc_keystores;
extern keystore_t ** keystores;

typedef struct {
    khm_int32  ctype;
    wchar_t *  provider_name;
} key_type_map_;

extern key_type_map_ * key_type_map;
extern khm_size n_key_type_map;
extern khm_size nc_key_type_map;

void
init_credtype(void);

keystore_t *
find_keystore_for_identity(khm_handle identity);

void
associate_keystore_and_identity(keystore_t * ks, khm_handle identity);

khm_handle
create_identity_from_keystore(keystore_t * ks);

keystore_t *
create_keystore_from_location(const wchar_t * path, khm_handle csp);

keystore_t *
create_keystore_for_identity(khm_handle identity);

void
update_keystore_list(void);

khm_handle
get_keystore_credential(keystore_t * ks);

khm_handle
get_identkey_credential(keystore_t * ks, identkey_t * idk);

khm_handle
get_keystore_credential_for_identity(khm_handle identity);

khm_size
get_keystores_with_identkey(khm_handle s_identity, keystore_t *** pks);

void
free_keystores_list(keystore_t ** aks, khm_size n_ks);

khm_int32
save_keystore_with_identity(keystore_t * ks);

khm_int32
destroy_keystore_identity(keystore_t * ks);

khm_handle
create_default_keystore(void);
