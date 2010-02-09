/*
 * Copyright (c) 2008-2009 Secure Endpoints Inc.
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

/*  */

khm_handle h_idprov = NULL;
khm_handle idprov_sub = NULL;

extern HANDLE h_idprov_event;

khm_int32
handle_kmsg_ident_init(void) {

    assert(h_idprov_event);
    if (h_idprov_event)
	SetEvent(h_idprov_event);

    /* First get a handle to self.  Something is very wrong if we
       can't do that. */
    if (KHM_FAILED(kcdb_identpro_find(IDPROV_NAMEW, &h_idprov))) {
        return KHM_ERROR_UNKNOWN;
    }

    /* TODO: Additional initialization code goes here */
    assert (credtype_id > 0);
    kcdb_identity_set_type(credtype_id);

    return KHM_ERROR_SUCCESS;
}

khm_int32
handle_kmsg_ident_exit(void)
{

    /* TODO: Additional uninitialization code goes here */

    if (idprov_sub) {
        kmq_delete_subscription(idprov_sub);
        idprov_sub = NULL;
    }

    return KHM_ERROR_SUCCESS;
}

khm_int32
handle_kmsg_ident_validate_name(kcdb_ident_name_xfer * nx) {

    /* TODO: Validate the provided name */

    /* The name is provided through the name_src field of nx.  The
       result of the validation should be set in nx->result.  One
       possible implementation would be as follows: */

    /* nx->result = my_name_validation_func(nx->name_src) */

    nx->result = KHM_ERROR_SUCCESS;

    return KHM_ERROR_SUCCESS;
}

khm_int32
handle_kmsg_ident_set_default(khm_handle def_ident) {

    /* TODO: Handle this message */

    return KHM_ERROR_SUCCESS;
}

khm_int32
handle_kmsg_ident_notify_config(khm_handle identity)
{
    khm_int32 ibuf[1024];       /* 4k buffer */
    void * pBuffer;
    khm_size cbBuffer;
    khm_int32 rv;
    khm_handle h_idconfig;

    rv = kcdb_identity_get_config(identity, 0, &h_idconfig);

#ifdef DEBUG
    assert(KHM_SUCCEEDED(rv));
#endif
    if (KHM_FAILED(rv))
        return KHM_ERROR_SUCCESS;

    /* If we are creating a configuration space, we should set up the
       issuer and the serial nuber binary properties. */

    if (KHM_FAILED(kcdb_identity_get_attr(identity, attr_id_issuer, NULL, NULL, NULL)) ||
        KHM_FAILED(kcdb_identity_get_attr(identity, attr_id_serial, NULL, NULL, NULL))) {

        /* We can't do anything in this case.  It is probably
           unintentional that we created an X509 identity without
           knowing what the corresponding certificate is. */
#ifdef DEBUG
        assert(FALSE);
#endif

        return KHM_ERROR_SUCCESS;
    }

    pBuffer = ibuf;
    cbBuffer = sizeof(ibuf);

    rv = kcdb_identity_get_attr(identity, attr_id_issuer, NULL, pBuffer, &cbBuffer);
    if (rv == KHM_ERROR_TOO_LONG) {
        pBuffer = PMALLOC(cbBuffer);
        rv = kcdb_identity_get_attr(identity, attr_id_issuer, NULL, pBuffer, &cbBuffer);
    }

    if (KHM_SUCCEEDED(rv)) {
        khc_write_binary(h_idconfig, L"GX509Issuer", pBuffer, cbBuffer);
    }

    if (pBuffer != ibuf)
        PFREE(pBuffer);

    pBuffer = ibuf;
    cbBuffer = sizeof(ibuf);

    rv = kcdb_identity_get_attr(identity, attr_id_serial, NULL, pBuffer, &cbBuffer);
    if (rv == KHM_ERROR_TOO_LONG) {
        pBuffer = PMALLOC(cbBuffer);
        rv = kcdb_identity_get_attr(identity, attr_id_serial, NULL, pBuffer, &cbBuffer);
    }

    if (KHM_SUCCEEDED(rv)) {
        khc_write_binary(h_idconfig, L"GX509Serial", pBuffer, cbBuffer);
    }

    if (pBuffer != ibuf)
        PFREE(pBuffer);

    khc_close_space(h_idconfig);

    return KHM_ERROR_SUCCESS;
}

khm_int32
handle_kmsg_ident_notify_create(khm_handle identity)
{
    PCERT_CONTEXT pCtx;
    khm_size cb;

    cb = sizeof(pCtx);
    if (KHM_SUCCEEDED(kcdb_identity_get_attr(identity, KCDB_ATTR_PARAM, NULL,
                                             &pCtx, &cb))) {
        wchar_t subject[KCDB_MAXCCH_NAME];

        subject[0] = L'\0';
        CertGetNameString(pCtx, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL, subject,
                          ARRAYLENGTH(subject));

        kcdb_identity_set_attr(identity, KCDB_ATTR_DISPLAY_NAME, subject, KCDB_CBSIZE_AUTO);

        kcdb_identity_set_attr(identity, attr_id_issuer,
                               pCtx->pCertInfo->Issuer.pbData,
                               pCtx->pCertInfo->Issuer.cbData);

        kcdb_identity_set_attr(identity, attr_id_serial,
                               pCtx->pCertInfo->SerialNumber.pbData,
                               pCtx->pCertInfo->SerialNumber.cbData);

        kcdb_identity_set_flags(identity, KCDB_IDENT_FLAG_VALID, KCDB_IDENT_FLAG_VALID);
    } else {

        /* TODO: going here.  We want to query the issuer and serial
           number values from the configuration space and populate the
           identity here. */

        /* TODO: the library should deal with the case where we don't
           have valid values anywhere. One possibility is to mark the
           identity as uninitialized or maybe have a flag that says
           that the identity provider has been notified. That way, if
           we recevie a create_identity_ex call that specifies the
           extra parameters, then we can go ahead and finish
           initializing the identity. */

        assert(FALSE);
    }

    return KHM_ERROR_SUCCESS;
}

khm_int32
handle_kmsg_ident_update(khm_handle ident) {

    /* TODO: Handle this message */

    return KHM_ERROR_SUCCESS;
}

khm_int32
handle_kmsg_ident_compare_name(kcdb_ident_name_xfer *px)
{

    /* The names that are to be compared are passed in px->name_src
       and px->name_alt, respectively.  They are guaranteed to be
       non-NULL and point to valid NULL terminated strings not
       exceeding KCDB_IDENT_MAXCCH_NAME characters (including the
       termintating NULL). */

    /* TODO: Add a proper implementation here */
    px->result = wcscmp(px->name_src, px->name_alt);

    return KHM_ERROR_SUCCESS;
}

khm_int32
handle_kmsg_ident_resource_req(kcdb_resource_request * preq)
{
    wchar_t buf[KCDB_MAXCCH_SHORT_DESC];
    HICON   hicon;
    BOOL found = TRUE;
    size_t cb = 0;

    buf[0] = L'\0';
    hicon = NULL;

    if (preq->h_obj != NULL) {

        /* This is a resource request for a specific identity
           specified by h_obj */
        switch(preq->res_id) {

            /* These are resources that we don't specify here.
               Returning without setting preq->code will result in
               NetIDMgr providing default resources. */

        case KCDB_RES_DESCRIPTION:
            /* Not providing a description results in NetIDMgr using
               the name of the identity as the description. */
            return KHM_ERROR_SUCCESS;

        case KCDB_RES_ICON_DISABLED:
        case KCDB_RES_ICON_NORMAL:
            /* Not providing icons allows NetIDMgr to manage identity
               icons. */

            if (preq->flags & KCDB_RFI_SMALL) {
                hicon = LoadImage(hResModule, MAKEINTRESOURCE(IDI_IDENTITY),
                                  IMAGE_ICON,
                                  GetSystemMetrics(SM_CXSMICON),
                                  GetSystemMetrics(SM_CYSMICON),
                                  LR_DEFAULTCOLOR);
            } else {
                hicon = LoadImage(hResModule, MAKEINTRESOURCE(IDI_IDENTITY),
                                  IMAGE_ICON, 0, 0,
                                  LR_DEFAULTSIZE | LR_DEFAULTCOLOR);
            }
            break;

        default:
            /* Do default processing */
            return KHM_ERROR_SUCCESS;
        }

    } else {

        /* This is a resource request for the identity provider */
        switch(preq->res_id) {
        case KCDB_RES_DISPLAYNAME:
            LoadString(hResModule, IDS_ID_DISPLAYNAME, buf, ARRAYLENGTH(buf));
            break;

        case KCDB_RES_DESCRIPTION:
            LoadString(hResModule, IDS_ID_DESCRIPTION, buf, ARRAYLENGTH(buf));
            break;

        case KCDB_RES_TOOLTIP:
            LoadString(hResModule, IDS_ID_TOOLTIP, buf, ARRAYLENGTH(buf));
            break;

        case KCDB_RES_INSTANCE:
            LoadString(hResModule, IDS_ID_INSTANCE, buf, ARRAYLENGTH(buf));
            break;

        case KCDB_RES_ICON_NORMAL:
            if (preq->flags & KCDB_RFI_SMALL) {
                hicon = LoadImage(hResModule, MAKEINTRESOURCE(IDI_IDENTITY),
                                  IMAGE_ICON,
                                  GetSystemMetrics(SM_CXSMICON),
                                  GetSystemMetrics(SM_CYSMICON),
                                  LR_DEFAULTCOLOR);
            } else {
                hicon = LoadImage(hResModule, MAKEINTRESOURCE(IDI_IDENTITY),
                                  IMAGE_ICON, 0, 0,
                                  LR_DEFAULTSIZE | LR_DEFAULTCOLOR);
            }
            break;

        default:
            found = FALSE;
        }
    }

    if (found && buf[0] != L'\0' &&
        SUCCEEDED(StringCbLength(buf, sizeof(buf), &cb))) {

        /* We are returning a string */

        cb += sizeof(wchar_t);
        if (preq->buf == NULL || preq->cb_buf < cb) {
            preq->cb_buf = cb;
            preq->code = KHM_ERROR_TOO_LONG;
        } else {
            StringCbCopy(preq->buf, preq->cb_buf, buf);
            preq->cb_buf = cb;
            preq->code = KHM_ERROR_SUCCESS;
        }
    } else if (found && hicon != NULL) {

        /* We are returngin an icon */

        if (preq->buf == NULL || preq->cb_buf < sizeof(HICON)) {
            preq->cb_buf = sizeof(HICON);
            preq->code = KHM_ERROR_TOO_LONG;

            DestroyIcon(hicon);
            hicon = NULL;

        } else {
            *((HICON *) preq->buf) = hicon;
            preq->cb_buf = sizeof(HICON);
            preq->code = KHM_ERROR_SUCCESS;
        }
    }

    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMAPI
handle_kmsg_ident(khm_int32 msg_type,
                  khm_int32 msg_subtype,
                  khm_ui_4 uparam,
                  void * vparam)
{
    switch(msg_subtype) {
    case KMSG_IDENT_INIT:
        return handle_kmsg_ident_init();

    case KMSG_IDENT_EXIT:
        return handle_kmsg_ident_exit();

    case KMSG_IDENT_VALIDATE_NAME:
        return handle_kmsg_ident_validate_name((kcdb_ident_name_xfer *) vparam);

    case KMSG_IDENT_COMPARE_NAME:
        return handle_kmsg_ident_compare_name((kcdb_ident_name_xfer *) vparam);

    case KMSG_IDENT_SET_DEFAULT:
        return handle_kmsg_ident_set_default((khm_handle) vparam);

    case KMSG_IDENT_UPDATE:
        return handle_kmsg_ident_update((khm_handle) vparam);

    case KMSG_IDENT_NOTIFY_CREATE:
        return handle_kmsg_ident_notify_create((khm_handle) vparam);

    case KMSG_IDENT_RESOURCE_REQ:
        return handle_kmsg_ident_resource_req((kcdb_resource_request *) vparam);
    }

    return KHM_ERROR_SUCCESS;
}

khm_int32
handle_kmsg_system_idprov(khm_int32 msg_type, khm_int32 msg_subtype,
                          khm_ui_4 uparam, void * vparam)
{

    switch(msg_subtype) {
    case KMSG_SYSTEM_INIT:
        {
            kmq_create_subscription(idprov_msg_proc, &idprov_sub);
        }
        break;

    case KMSG_SYSTEM_EXIT:
        {
        }
        break;
    }

    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMAPI
idprov_msg_proc(khm_int32 msg_type, khm_int32 msg_subtype,
                khm_ui_4 uparam, void * vparam)
{
    switch(msg_type) {
    case KMSG_SYSTEM:
        return handle_kmsg_system_idprov(msg_type, msg_subtype, uparam, vparam);

    case KMSG_IDENT:
        return handle_kmsg_ident(msg_type, msg_subtype, uparam, vparam);
    }

    return KHM_ERROR_SUCCESS;
}
