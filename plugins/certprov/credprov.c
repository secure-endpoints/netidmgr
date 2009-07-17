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

#include "module.h"
#include<assert.h>

/* This file provides the message processing function and the support
   routines for implementing our plugin.  Note that some of the
   message processing routines have been moved to other source files
   based on their use.
*/

khm_int32 credtype_id = KCDB_CREDTYPE_INVALID;
khm_handle g_credset = NULL;

khm_int32 attr_id_issuer = KCDB_ATTR_INVALID;
khm_int32 attr_id_serial = KCDB_ATTR_INVALID;

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

    assert(h_idprov_event == NULL);
    h_idprov_event = CreateEvent(NULL, TRUE, FALSE, L"Local\\" IDPROV_NAMEW L"Waiter");

    /* First and foremost, we need to register a credential type. */
    ZeroMemory(&ct, sizeof(ct));
    ct.id = KCDB_CREDTYPE_AUTO;
    ct.name = CREDTYPE_NAMEW;

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

    /* Register a few attributes that we will be using */
    {
        kcdb_attrib areg;

        ZeroMemory(&areg, sizeof(areg));

        areg.name = L"GX509Issuer";
        areg.id = KCDB_ATTR_INVALID;
        areg.flags = KCDB_ATTR_FLAG_HIDDEN;
        areg.type = KCDB_TYPE_DATA;

        kcdb_attrib_register(&areg, &attr_id_issuer);

        areg.name = L"GX509Serial";
        kcdb_attrib_register(&areg, &attr_id_serial);
    }

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
    creg.flags = KHUI_CNFLAG_SUBPANEL | KHUI_CNFLAG_PLURAL;

    khui_cfg_register(cnode, &creg);

    khui_cfg_release(cnode);

    /* TODO: Perform additional initialization operations. */

    /* TODO: Also list out the credentials of this type that already
       exist. */

    return KHM_ERROR_SUCCESS;
}

khm_int32
handle_kmsg_system_exit(void)
{
    khui_config_node cnode;
    khui_config_node cn_idents;

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

    /* Unregister the attributes we registered */
    kcdb_attrib_unregister(attr_id_serial);
    kcdb_attrib_unregister(attr_id_issuer);

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

    if (h_idprov_event != NULL) {
	CloseHandle(h_idprov_event);
	h_idprov_event = NULL;
    }

    /* TODO: Perform additional uninitialization operations. */

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
encode_blob_to_string_int(const BYTE * pData, int ntotalbits,
                          wchar_t * buf, size_t *pcbbuf, wchar_t ** end)
{
    DWORD bits;
    int nbits;

    static const wchar_t map[] =
        L"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ#$";
    /*    0123456789012345678901234567890123456789012345678901234567890123 */

    bits = 0;
    nbits = 0;
    while (*pcbbuf > 0 && ntotalbits > 0) {
        DWORD inbits;

        inbits = *pData++;
        ntotalbits -= 8;
        if (ntotalbits < 0) {
            inbits &= (1<<(ntotalbits + 8)) - 1;
        }
        bits |= (inbits << nbits);
        nbits += 8;

        while (nbits > 6 && *pcbbuf > 0) {
            int idx;

            idx = (bits & 0x3f);
            bits >>= 6;
            nbits -= 6;
            *buf++ = map[idx];
            *pcbbuf -= sizeof(wchar_t);
        }
    }

    while (nbits > 0 && *pcbbuf > 0) {
        int idx;

        idx = (bits & 0x3f);
        bits >>= 6;
        nbits -= 6;
        *buf++ = map[idx];
        *pcbbuf -= sizeof(wchar_t);
    }

    if (end)
        *end = buf;

    if (*pcbbuf) {
        *buf++ = L'\0';
        *pcbbuf -= sizeof(wchar_t);
    }
}

void
encode_data_blob_to_string(const DATA_BLOB * blob, wchar_t * buf, size_t *pcbbuf,
                            wchar_t ** end)
{
    encode_blob_to_string_int(blob->pbData, blob->cbData * 8,
                              buf, pcbbuf, end);
}

void
encode_bit_blob_to_string(const CRYPT_BIT_BLOB * bblob, wchar_t * buf, size_t * pcbbuf,
                          wchar_t ** end)
{
    encode_blob_to_string_int(bblob->pbData, bblob->cbData * 8 - bblob->cUnusedBits,
                              buf, pcbbuf, end);
}

void
cname_from_cert_ctx(PCCERT_CONTEXT pCtx, wchar_t * cname, size_t cbname_in)
{
    wchar_t * end;
    size_t cbname = cbname_in;

    encode_data_blob_to_string(&pCtx->pCertInfo->SerialNumber, cname, &cbname, &end);
    StringCbCopyEx(end, cbname, L"-", &end, &cbname, STRSAFE_NULL_ON_FAILURE);
    encode_data_blob_to_string(&pCtx->pCertInfo->Issuer, end, &cbname, &end);

    cname[cbname_in / sizeof(wchar_t) - 1] = L'\0';
}

void
idname_from_cert_ctx(PCCERT_CONTEXT pCtx, wchar_t * idname, size_t cbname_in)
{
    wchar_t * end = idname;
    size_t cbname = cbname_in;

    encode_data_blob_to_string(&pCtx->pCertInfo->Issuer, end, &cbname, &end);
    StringCbCopyEx(end, cbname, L"-", &end, &cbname, STRSAFE_NULL_ON_FAILURE);
    encode_data_blob_to_string(&pCtx->pCertInfo->Subject, end, &cbname, &end);

    idname[cbname_in / sizeof(wchar_t) - 1] = L'\0';

#ifdef DEBUG
    {
        wchar_t subject[KCDB_IDENT_MAXCCH_NAME];

        CertGetNameString(pCtx, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL, subject,
                          ARRAYLENGTH(subject));
        kherr_debug_printf(L"Created name for [%s] is [%s]\n",
                           subject, idname);
    }
#endif
}

khm_handle
get_identity_for_cert_ctx(PCCERT_CONTEXT pCtx, khm_int32 flags)
{
    khm_handle identity = NULL;
    wchar_t idname[KCDB_IDENT_MAXCCH_NAME];

    idname_from_cert_ctx(pCtx, idname, sizeof(idname));

    kcdb_identity_create_ex(h_idprov, idname, flags, (void *) pCtx, &identity);

    return identity;
}

void
find_matching_cert(khm_handle buf, PCCERT_CONTEXT *ppCtx, HCERTSTORE *phCertStore)
{
    CERT_ID cid;
    size_t cb;
    HCERTSTORE hCertStore = 0;
    PCCERT_CONTEXT pCtx = 0;

    ZeroMemory(&cid, sizeof(cid));
    *ppCtx = NULL;
    *phCertStore = NULL;

    if (kcdb_buf_get_attr(buf, attr_id_issuer, NULL, NULL, &cb) != KHM_ERROR_TOO_LONG ||
        cb == 0)
        return;

    cid.IssuerSerialNumber.Issuer.cbData = cb;

    if (kcdb_buf_get_attr(buf, attr_id_serial, NULL, NULL, &cb) != KHM_ERROR_TOO_LONG ||
        cb == 0)
        return;

    cid.IssuerSerialNumber.SerialNumber.cbData = cb;

    cid.IssuerSerialNumber.Issuer.pbData = PMALLOC(cid.IssuerSerialNumber.Issuer.cbData);
    cid.IssuerSerialNumber.SerialNumber.pbData = PMALLOC(cid.IssuerSerialNumber.SerialNumber.cbData);

    cb = cid.IssuerSerialNumber.Issuer.cbData;
    kcdb_buf_get_attr(buf, attr_id_issuer, NULL, cid.IssuerSerialNumber.Issuer.pbData,
                      &cb);
    cb = cid.IssuerSerialNumber.SerialNumber.cbData;
    kcdb_buf_get_attr(buf, attr_id_serial, NULL, cid.IssuerSerialNumber.SerialNumber.pbData,
                      &cb);

    cid.dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;

    hCertStore = CertOpenSystemStore(0, L"MY");
    if (hCertStore == 0)
        goto cleanup;

    pCtx = CertFindCertificateInStore(hCertStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                      0, CERT_FIND_CERT_ID, &cid,
                                      NULL);

 cleanup:
    if (cid.IssuerSerialNumber.Issuer.pbData)
        PFREE(cid.IssuerSerialNumber.Issuer.pbData);

    if (cid.IssuerSerialNumber.SerialNumber.pbData)
        PFREE(cid.IssuerSerialNumber.SerialNumber.pbData);

    *phCertStore = hCertStore;
    *ppCtx = pCtx;
}

void
list_certs(void)
{
    HCERTSTORE hCertStore;
    PCCERT_CONTEXT pCtx = NULL;

    kcdb_credset_flush(g_credset);

    hCertStore = CertOpenSystemStore(0, L"MY");

    if (hCertStore == NULL)
        goto cleanup;

    while ((pCtx = CertEnumCertificatesInStore(hCertStore, pCtx)) != NULL) {
        wchar_t cname[KCDB_MAXCCH_NAME];
        wchar_t issuer[KCDB_MAXCCH_NAME];

        khm_handle identity = NULL;
        khm_handle credential = NULL;

        cname_from_cert_ctx(pCtx, cname, sizeof(cname));

        CertGetNameString(pCtx, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG,
                          NULL, issuer, ARRAYLENGTH(issuer));

        identity = get_identity_for_cert_ctx(pCtx, KCDB_IDENT_FLAG_CREATE);

        kcdb_cred_create(cname, identity, credtype_id, &credential);

        kcdb_cred_set_attr(credential, KCDB_ATTR_DISPLAY_NAME,
                           issuer, KCDB_CBSIZE_AUTO);

        kcdb_cred_set_attr(credential, KCDB_ATTR_ISSUE,
                           &pCtx->pCertInfo->NotBefore, KCDB_CBSIZE_AUTO);

        kcdb_identity_set_attr(identity, KCDB_ATTR_ISSUE,
                               &pCtx->pCertInfo->NotBefore, KCDB_CBSIZE_AUTO);

        kcdb_cred_set_attr(credential, KCDB_ATTR_EXPIRE,
                           &pCtx->pCertInfo->NotAfter, KCDB_CBSIZE_AUTO);
        
        kcdb_identity_set_attr(identity, KCDB_ATTR_EXPIRE,
                               &pCtx->pCertInfo->NotAfter, KCDB_CBSIZE_AUTO);
        
        kcdb_cred_set_attr(credential, attr_id_issuer,
                           pCtx->pCertInfo->Issuer.pbData,
                           pCtx->pCertInfo->Issuer.cbData);

        kcdb_cred_set_attr(credential, attr_id_serial,
                           pCtx->pCertInfo->SerialNumber.pbData,
                           pCtx->pCertInfo->SerialNumber.cbData);

        kcdb_cred_set_flags(credential,
                            KCDB_CRED_FLAG_INITIAL,
                            KCDB_CRED_FLAG_INITIAL);

        kcdb_credset_add_cred(g_credset, credential, -1);

        kcdb_cred_release(credential);
        kcdb_identity_release(identity);
    }

 cleanup:

    kcdb_credset_collect(NULL, g_credset, NULL, credtype_id, NULL);

    if (hCertStore)
        CertCloseStore(hCertStore, 0);

}

/* Handler for credentials the refresh message. */
khm_int32
handle_kmsg_cred_refresh(void)
{
    /* TODO: Re-enumerate the credentials of our credentials type */

    /*
      Re-enumerating credentials would look something like this:

      - flush all credentials from g_credset (kcdb_credset_flush())

      - list out the credentials and add them to g_credset

      - collect the credentials from g_credset to the root credentials
        set. (kcdb_credset_collect())

      Note that when listing credentials, each credential must be
      populated with enough information to locate the actual
      credential at a later time.
     */

    list_certs();

    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMAPI delete_cred_func(khm_handle cred, void * rock)
{
    PCCERT_CONTEXT pCtx = 0;
    HCERTSTORE hCertStore = 0;
    khm_int32 t;

    kcdb_cred_get_type(cred, &t);

    if (t != credtype_id)
        return KHM_ERROR_SUCCESS;

    find_matching_cert(cred, &pCtx, &hCertStore);
    if (pCtx == 0)
        goto cleanup;

    CertDeleteCertificateFromStore(pCtx);

 cleanup:
    if (hCertStore)
        CertCloseStore(hCertStore, 0);

    return KHM_ERROR_SUCCESS;
}

/* Handler for destroying credentials */
khm_int32
handle_kmsg_cred_destroy_creds(khui_action_context * ctx)
{
    /* TODO: Destroy credentials of our type as specified by the
       action context passed in through vparam. */

    /* The credential set in ctx->credset contains the credentials
       that are to be destroyed. */

    kcdb_credset_apply(ctx->credset, delete_cred_func, NULL);

    return KHM_ERROR_SUCCESS;
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
        p = malloc(sizeof(*p));
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
            free(p->p_page);
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

    if (h_idprov_event != NULL)
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
    }

    return KHM_ERROR_SUCCESS;
}
