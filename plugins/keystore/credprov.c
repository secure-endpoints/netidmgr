/*
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

#include "module.h"
#include<assert.h>

/* This file provides the message processing function and the support
   routines for implementing our plugin.  Note that some of the
   message processing routines have been moved to other source files
   based on their use.
*/

khm_int32 credtype_id = KCDB_CREDTYPE_INVALID;
khm_int32 idk_credtype_id = KCDB_CREDTYPE_INVALID;
khm_handle g_credset = NULL;
CRITICAL_SECTION cs_ks;

HANDLE h_idprov_event = NULL;

khm_int32
handle_kmsg_system_init(void)
{
    kcdb_credtype ct;
    wchar_t short_desc[KCDB_MAXCCH_SHORT_DESC];
    wchar_t long_desc[KCDB_MAXCCH_LONG_DESC];
    khui_config_node cnode;
    khui_config_node_reg creg;
    khm_int32 rv;

    assert (h_idprov_event == NULL);

    h_idprov_event = CreateEvent(NULL, TRUE, FALSE, L"Local\\SEIKeystoreIdProWaiter");

    InitializeCriticalSection(&cs_ks);

    /* First and foremost, we need to register a credential type. */
    ZeroMemory(&ct, sizeof(ct));
    ct.id = KCDB_CREDTYPE_AUTO;
    ct.name = CREDTYPE_NAMEW;
    ct.short_desc = short_desc;
    ct.long_desc = long_desc;

    short_desc[0] = L'\0';
    LoadString(hResModule, IDS_CT_SHORT_DESC,
               short_desc, ARRAYLENGTH(short_desc));

    long_desc[0] = L'\0';
    LoadString(hResModule, IDS_CT_LONG_DESC,
               long_desc, ARRAYLENGTH(long_desc));

    ct.icon = NULL;     /* We skip the icon for now, but you can
                           assign a handle to an icon here.  The icon
                           will be used to represent the credentials
                           type.*/
    
    kmq_create_subscription(credprov_msg_proc, &ct.sub);

    ct.is_equal = cred_is_equal;

    rv = kcdb_credtype_register(&ct, &credtype_id);
    if (KHM_FAILED(rv))
        return rv;

    /* We need an auxiliary credentials type for representing
       identity private keys */

    ZeroMemory(&ct, sizeof(ct));
    ct.id = KCDB_CREDTYPE_AUTO;
    ct.name = IDK_CREDTYPE_NAMEW;
    ct.short_desc = short_desc;
    ct.long_desc = long_desc;

    short_desc[0] = L'\0';
    LoadString(hResModule, IDS_IDK_CT_SHORT_DESC,
               short_desc, ARRAYLENGTH(short_desc));

    long_desc[0] = L'\0';
    LoadString(hResModule, IDS_IDK_CT_LONG_DESC,
               long_desc, ARRAYLENGTH(long_desc));

    ct.icon = NULL;     /* We skip the icon for now, but you can
                           assign a handle to an icon here.  The icon
                           will be used to represent the credentials
                           type.*/

    kmq_create_subscription(idk_credprov_msg_proc, &ct.sub);

    ct.is_equal = idk_cred_is_equal;

    rv = kcdb_credtype_register(&ct, &idk_credtype_id);
    if (KHM_FAILED(rv))
        return rv;

    /* We create a global credential set that we use in the plug-in
       thread.  This alleviates the need to create one everytime we
       need one. Keep in mind that this should only be used in the
       plug-in thread and should not be touched from the UI thread or
       any other thread. */
    kcdb_credset_create(&g_credset);

    /* Now we register our configuration panels. */

    /* This configuration panel is the one that controls general
       options.  We leave the identity specific and identity defaults
       for other configuration panels. */

#ifdef NOT_IMPLEMENTED
    /* General options aren't implemented yet for Keystores */
    ZeroMemory(&creg, sizeof(creg));

    short_desc[0] = L'\0';

    LoadString(hResModule, IDS_CFG_SHORT_DESC,
               short_desc, ARRAYLENGTH(short_desc));

    long_desc[0] = L'\0';

    LoadString(hResModule, IDS_CFG_LONG_DESC,
               long_desc, ARRAYLENGTH(long_desc));

    creg.name = CONFIGNODE_MAIN;
    creg.short_desc = short_desc;
    creg.long_desc = long_desc;
    creg.h_module = hResModule;
    creg.dlg_template = MAKEINTRESOURCE(IDD_CONFIG);
    creg.dlg_proc = config_dlgproc;
    creg.flags = 0;

    khui_cfg_register(NULL, &creg);
#endif

    /* Now we do the identity specific and identity default
       configuration panels. "KhmIdentities" is a predefined
       configuration node under which all the identity spcific
       configuration is managed. */

    if (KHM_FAILED(khui_cfg_open(NULL, L"KhmIdentities", &cnode))) {
        /* this should always work */
        assert(FALSE);
        return KHM_ERROR_NOT_FOUND;
    }

    /* First the tab panel for defaults for all identities */

#ifdef NOT_IMPLEMENTED
    ZeroMemory(&creg, sizeof(creg));

    short_desc[0] = L'\0';
    LoadString(hResModule, IDS_CFG_IDS_SHORT_DESC,
               short_desc, ARRAYLENGTH(short_desc));
    long_desc[0] = L'\0';
    LoadString(hResModule, IDS_CFG_IDS_LONG_DESC,
               long_desc, ARRAYLENGTH(long_desc));

    creg.name = CONFIGNODE_ALL_ID;
    creg.short_desc = short_desc;
    creg.long_desc = long_desc;
    creg.h_module = hResModule;
    creg.dlg_template = MAKEINTRESOURCE(IDD_CONFIG_IDS);
    creg.dlg_proc = config_ids_dlgproc;
    creg.flags = KHUI_CNFLAG_SUBPANEL;

    khui_cfg_register(cnode, &creg);
#endif

    /* Now the panel for per identity configuration */

    ZeroMemory(&creg, sizeof(creg));

    short_desc[0] = L'\0';
    LoadString(hResModule, IDS_CFG_ID_SHORT_DESC,
               short_desc, ARRAYLENGTH(short_desc));
    long_desc[0] = L'\0';
    LoadString(hResModule, IDS_CFG_ID_LONG_DESC,
               long_desc, ARRAYLENGTH(long_desc));

    creg.name = CONFIGNODE_PER_ID;
    creg.short_desc = short_desc;
    creg.long_desc = long_desc;
    creg.h_module = hResModule;
    creg.dlg_template = MAKEINTRESOURCE(IDD_CONFIG_ID);
    creg.dlg_proc = config_id_dlgproc;
    creg.flags = KHUI_CNFLAG_SUBPANEL | KHUI_CNFLAG_INSTANCE;

    khui_cfg_register(cnode, &creg);

    khui_cfg_release(cnode);

    init_credtype();

    kmq_subscribe(KMSG_KCDB, credprov_msg_proc);

    return KHM_ERROR_SUCCESS;
}

khm_int32
handle_kmsg_system_exit(void)
{
    khui_config_node cnode;
    khui_config_node cn_idents;
    khm_size i;

    kmq_unsubscribe(KMSG_KCDB, credprov_msg_proc);

    /* It should not be assumed that initialization of the plugin went
       well at this point since we receive a KMSG_SYSTEM_EXIT even if
       the initialization failed. */

    if (credtype_id != KCDB_CREDTYPE_INVALID) {
        kcdb_credtype_unregister(credtype_id);
        credtype_id = KCDB_CREDTYPE_INVALID;
    }

    if (g_credset) {
        kcdb_credset_delete(g_credset);
        g_credset = NULL;
    }

    EnterCriticalSection(&cs_ks);
    for (i=0; i < n_key_type_map; i++) {
        kcdb_credtype_unregister(key_type_map[i].ctype);
        PFREE(key_type_map[i].provider_name);
        ZeroMemory(&key_type_map[i], sizeof(key_type_map[i]));
    }
    if (key_type_map)
        PFREE(key_type_map);
    key_type_map = NULL;
    n_key_type_map = 0;
    nc_key_type_map = 0;
    LeaveCriticalSection(&cs_ks);

    /* Now unregister any configuration nodes we registered. */

    if (KHM_SUCCEEDED(khui_cfg_open(NULL, CONFIGNODE_MAIN, &cnode))) {
        khui_cfg_remove(cnode);
        khui_cfg_release(cnode);
    }

    if (KHM_SUCCEEDED(khui_cfg_open(NULL, L"KhmIdentities", &cn_idents))) {
        if (KHM_SUCCEEDED(khui_cfg_open(cn_idents,
                                        CONFIGNODE_ALL_ID,
                                        &cnode))) {
            khui_cfg_remove(cnode);
            khui_cfg_release(cnode);
        }

        if (KHM_SUCCEEDED(khui_cfg_open(cn_idents,
                                        CONFIGNODE_PER_ID,
                                        &cnode))) {
            khui_cfg_remove(cnode);
            khui_cfg_release(cnode);
        }

        khui_cfg_release(cn_idents);
    }

    DeleteCriticalSection(&cs_ks);

    if (h_idprov_event != NULL) {
        CloseHandle(h_idprov_event);
        h_idprov_event = NULL;
    }

    return KHM_ERROR_SUCCESS;
}

/* Handler for system messages.  The only two we handle are
   KMSG_SYSTEM_INIT and KMSG_SYSTEM_EXIT. */
khm_int32 KHMAPI
handle_kmsg_system_credprov(khm_int32 msg_type,
                            khm_int32 msg_subtype,
                            khm_ui_4  uparam,
                            void *    vparam)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;

    switch (msg_subtype) {

        /* This is the first message that will be received by a
           plugin.  We use it to perform initialization operations
           such as registering any credential types, data types and
           attributes. */
    case KMSG_SYSTEM_INIT:
        return handle_kmsg_system_init();

        /* This is the last message that will be received by the
           plugin. */
    case KMSG_SYSTEM_EXIT:
        return handle_kmsg_system_exit();
    }

    return rv;
}

void
list_credentials(void)
{
    khm_size i;

    update_keystore_list();

    kcdb_credset_flush(g_credset);
    EnterCriticalSection(&cs_ks);
    for (i=0; i < n_keystores; i++) {
        khm_handle credential;
        khm_size j;
        int add_idkeys = 0;

        credential = get_keystore_credential(keystores[i]);
        if (credential != NULL) {
            kcdb_credset_add_cred(g_credset, credential, -1);
            kcdb_cred_release(credential);
            add_idkeys = 1;
        }

        KSLOCK(keystores[i]);
        for (j=0; j < keystores[i]->n_keys; j++) {
            if (add_idkeys) {
                credential = get_identkey_credential(keystores[i], keystores[i]->keys[j]);
                if (credential == NULL)
                    continue;

                kcdb_credset_add_cred(g_credset, credential, -1);
                kcdb_cred_release(credential);
            }

            if (keystores[i]->identity) {
                khm_handle prov = NULL;
                khm_handle ident = NULL;

                if (KHM_SUCCEEDED(kcdb_identpro_find(keystores[i]->keys[j]->provider_name, &prov)) &&
                    KHM_SUCCEEDED(kcdb_identity_create_ex(prov, keystores[i]->keys[j]->identity_name,
                                                          KCDB_IDENT_FLAG_CREATE, NULL, &ident))) {
                    kcdb_identity_set_parent(ident, keystores[i]->identity);
                }

                if (prov)
                    kcdb_identpro_release(prov);
                if (ident)
                    kcdb_identity_release(ident);
            }
        }
        KSUNLOCK(keystores[i]);
    }

    for (i=0; i < n_key_type_map; i++) {
        LeaveCriticalSection(&cs_ks);
        kcdb_credset_collect(NULL, g_credset, NULL, key_type_map[i].ctype, NULL);
        EnterCriticalSection(&cs_ks);
    }

    LeaveCriticalSection(&cs_ks);

    kcdb_credset_collect(NULL, g_credset, NULL, credtype_id, NULL);
    kcdb_credset_collect(NULL, g_credset, NULL, idk_credtype_id, NULL);
}

/* Handler for credentials the refresh message. */
khm_int32
handle_kmsg_cred_refresh(void)
{
    list_credentials();
    return KHM_ERROR_SUCCESS;
}

static khm_int32 KHMAPI
destroy_creds_proc(khm_handle cred, void * rock)
{
    khm_int32 ctype;
    khm_handle identity = NULL;
    keystore_t * ks = NULL;

    if (KHM_FAILED(kcdb_cred_get_type(cred, &ctype)) ||
        ctype != credtype_id ||
        KHM_FAILED(kcdb_cred_get_identity(cred, &identity)))
        return KHM_ERROR_SUCCESS;

    ks = find_keystore_for_identity(identity);
    if (ks) {
        ks_keystore_reset_key(ks);
        ks_keystore_release(ks);
    }

    kcdb_identity_release(identity);
    return KHM_ERROR_SUCCESS;
}

/* Handler for destroying credentials */
khm_int32
handle_kmsg_cred_destroy_creds(khui_action_context * ctx)
{
    khm_int32 rv;

    rv = kcdb_credset_apply(ctx->credset, destroy_creds_proc, NULL);
    list_credentials();

    return rv;

}

/* Begin a property sheet */
khm_int32
handle_kmsg_cred_pp_begin(khui_property_sheet * ps)
{

    /* TODO: Provide the information necessary to show a property
       page for a credentials belonging to our credential type. */

    PROPSHEETPAGE *p;

    if (ps->credtype == credtype_id &&
        ps->cred) {
        /* We have been requested to show a property sheet for one of
           our credentials. */
        p = PMALLOC(sizeof(*p));
        ZeroMemory(p, sizeof(*p));

        p->dwSize = sizeof(*p);
        p->dwFlags = 0;
        p->hInstance = hResModule;
        p->pszTemplate = MAKEINTRESOURCE(IDD_PP_CRED);
        p->pfnDlgProc = pp_cred_dlg_proc;
        p->lParam = (LPARAM) ps;
        khui_ps_add_page(ps, credtype_id, 0, p, NULL);
    }

    return KHM_ERROR_SUCCESS;
}

/* End a property sheet */
khm_int32
handle_kmsg_cred_pp_end(khui_property_sheet * ps)
{
    /* TODO: Handle the end of a property sheet. */

    khui_property_page * p = NULL;

    khui_ps_find_page(ps, credtype_id, &p);
    if (p) {
        if (p->p_page)
            PFREE(p->p_page);
        p->p_page = NULL;
    }

    return KHM_ERROR_SUCCESS;
}

/* Handler for destroying credentials */
khm_int32
handle_idk_kmsg_cred_destroy_creds(khui_action_context * ctx)
{
    /* TODO: Destroy credentials of our type as specified by the
       action context passed in through vparam. */

    /* The credential set in ctx->credset contains the credentials
       that are to be destroyed. */

    return KHM_ERROR_SUCCESS;
}

/* Begin a property sheet */
khm_int32
handle_idk_kmsg_cred_pp_begin(khui_property_sheet * ps)
{

    /* TODO: Provide the information necessary to show a property
       page for a credentials belonging to our credential type. */

    PROPSHEETPAGE *p;

    if (ps->credtype == credtype_id &&
        ps->cred) {
        /* We have been requested to show a property sheet for one of
           our credentials. */
        p = PMALLOC(sizeof(*p));
        ZeroMemory(p, sizeof(*p));

        p->dwSize = sizeof(*p);
        p->dwFlags = 0;
        p->hInstance = hResModule;
        p->pszTemplate = MAKEINTRESOURCE(IDD_PP_CRED);
        p->pfnDlgProc = pp_cred_dlg_proc;
        p->lParam = (LPARAM) ps;
        khui_ps_add_page(ps, credtype_id, 0, p, NULL);
    }

    return KHM_ERROR_SUCCESS;
}

/* End a property sheet */
khm_int32
handle_idk_kmsg_cred_pp_end(khui_property_sheet * ps)
{
    /* TODO: Handle the end of a property sheet. */

    khui_property_page * p = NULL;

    khui_ps_find_page(ps, credtype_id, &p);
    if (p) {
        if (p->p_page)
            PFREE(p->p_page);
        p->p_page = NULL;
    }

    return KHM_ERROR_SUCCESS;
}

/* IP address change notification */
khm_int32
handle_kmsg_cred_addr_change(void)
{
    /* TODO: Handle this message. */

    return KHM_ERROR_SUCCESS;
}

/* Message dispatcher for credentials messages. */
khm_int32 KHMAPI
handle_kmsg_cred (khm_int32 msg_type,
                  khm_int32 msg_subtype,
                  khm_ui_4  uparam,
                  void *    vparam)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;

    assert(h_idprov_event != NULL);

    if (h_idprov_event)
        WaitForSingleObject(h_idprov_event, INFINITE);

    switch(msg_subtype) {
    case KMSG_CRED_REFRESH:
        return handle_kmsg_cred_refresh();

    case KMSG_CRED_DESTROY_CREDS:
        return handle_kmsg_cred_destroy_creds((khui_action_context *) vparam);

    case KMSG_CRED_PP_BEGIN:
        return handle_kmsg_cred_pp_begin((khui_property_sheet *) vparam);

    case KMSG_CRED_PP_END:
        return handle_kmsg_cred_pp_end((khui_property_sheet *) vparam);

    case KMSG_CRED_ADDR_CHANGE:
        return handle_kmsg_cred_addr_change();

    default:
        /* Credentials acquisition messages are all handled in a
           different source file. */
        if (IS_CRED_ACQ_MSG(msg_subtype))
            return handle_cred_acq_msg(msg_type, msg_subtype,
                                       uparam, vparam);
    }

    return rv;
}

khm_int32
handle_kmsg_kcdb(khm_int32 msg_type,
                 khm_int32 msg_subtype,
                 khm_ui_4 uparam,
                 void * vparam)
{
    /* if the configuration for an identity was deleted, we should
       remove any passwords we have for it from all the keystores we
       keep track of. */

    if (msg_type == KMSG_KCDB &&
        msg_subtype == KMSG_KCDB_IDENT &&
        uparam == KCDB_OP_DELCONFIG) {

        if (KHM_SUCCEEDED(delete_identkey_from_all_keystores((khm_handle) vparam))) {
            kcdb_identity_set_parent((khm_handle) vparam, NULL);
            kmq_post_message(KMSG_CRED, KMSG_CRED_REFRESH, 0, NULL);
        }

    }

    return KHM_ERROR_SUCCESS;
}


khm_int32 KHMAPI
handle_idk_kmsg_cred (khm_int32 msg_type,
                      khm_int32 msg_subtype,
                      khm_ui_4  uparam,
                      void *    vparam)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;

    switch(msg_subtype) {
    case KMSG_CRED_DESTROY_CREDS:
        return handle_idk_kmsg_cred_destroy_creds((khui_action_context *) vparam);

    case KMSG_CRED_PP_BEGIN:
        return handle_idk_kmsg_cred_pp_begin((khui_property_sheet *) vparam);

    case KMSG_CRED_PP_END:
        return handle_idk_kmsg_cred_pp_end((khui_property_sheet *) vparam);
    }

    return rv;
}

khm_int32 KHMAPI
idk_credprov_msg_proc (khm_int32 msg_type, khm_int32 msg_subtype,
                   khm_ui_4  uparam, void *vparam)
{

    switch(msg_type) {
    case KMSG_CRED:
        return handle_idk_kmsg_cred(msg_type, msg_subtype, uparam, vparam);
    }

    return KHM_ERROR_SUCCESS;
}

/* This is the main message handler for our plugin.  All the plugin
   messages end up here where we either handle it directly or dispatch
   it to other handlers. */
khm_int32 KHMAPI
credprov_msg_proc (khm_int32 msg_type, khm_int32 msg_subtype,
                   khm_ui_4  uparam, void *vparam)
{

    switch(msg_type) {
    case KMSG_SYSTEM:
        return handle_kmsg_system_credprov(msg_type, msg_subtype, uparam, vparam);

    case KMSG_CRED:
        return handle_kmsg_cred(msg_type, msg_subtype, uparam, vparam);

    case KMSG_KCDB:
        return handle_kmsg_kcdb(msg_type, msg_subtype, uparam, vparam);
    }

    return KHM_ERROR_SUCCESS;
}
