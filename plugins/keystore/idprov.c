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

    assert(credtype_id > 0);

    kcdb_identity_set_type(credtype_id);

    return KHM_ERROR_SUCCESS;
}

khm_int32
handle_kmsg_ident_exit(void)
{

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

    wchar_t idname[KCDB_IDENT_MAXCCH_NAME];
    khm_size cb = sizeof(idname);
    khm_handle csp_p = NULL;
    khm_int32 rv;

    rv = kmm_get_plugin_config(IDPROV_NAMEW, KHM_FLAG_CREATE, &csp_p);
    if (KHM_FAILED(rv))
        return rv;

    rv = kcdb_identity_get_name(def_ident, idname, &cb);
    if (KHM_FAILED(rv))
        goto done;

    rv = khc_write_string(csp_p, L"DefaultIdentity", idname);

 done:
    if (csp_p)
        khc_close_space(csp_p);

    return rv;
}

khm_int32
handle_kmsg_ident_notify_create(khm_handle ident)
{
    keystore_t * ks = NULL;
    khm_size cb;

    assert(credtype_id > 0);

    /* when we get a notification, if there is a parameter specified,
       then it is assumed to be a keystore. */
    cb = sizeof(ks);
    kcdb_identity_get_attr(ident, KCDB_ATTR_PARAM, NULL, &ks, &cb);

    if (!ks) {
        /* if we didn't get a parameter, then we try to create the
           keystore based on the identity. */
        ks = create_keystore_for_identity(ident);
    } else {
        ks_keystore_hold(ks);
    }

    if (ks) {
        assert(is_keystore_t(ks));

        if (!is_keystore_t(ks))
            ks = NULL;
    }

    if (ks) {
        khm_boolean can_init = TRUE;

        KSLOCK(ks);
        if (ks->display_name)
            kcdb_identity_set_attr(ident, KCDB_ATTR_DISPLAY_NAME, ks->display_name,
                                   KCDB_CBSIZE_AUTO);
        KSUNLOCK(ks);

        can_init = (ks_keystore_has_key(ks) || ks_is_keystore_locked(ks));

        if (KHM_FAILED(associate_keystore_and_identity(ks, ident))) {
            ks_keystore_release(ks);
            ks = NULL;          /* invokes the ks==NULL code below */
        } else {
            ks_keystore_release(ks);

            kcdb_identity_set_flags(ident,
                                    KCDB_IDENT_FLAG_VALID | KCDB_IDENT_FLAG_KEY_STORE |
                                    ((can_init)? KCDB_IDENT_FLAG_CRED_INIT : 0),
                                    KCDB_IDENT_FLAG_VALID | KCDB_IDENT_FLAG_KEY_STORE |
                                    KCDB_IDENT_FLAG_CRED_INIT);
            kcdb_identity_set_attr(ident, KCDB_ATTR_STATUS, NULL, 0);

            {
                khm_handle csp_p = NULL;
                wchar_t didname[KCDB_IDENT_MAXCCH_NAME];
                wchar_t idname[KCDB_IDENT_MAXCCH_NAME];

                cb = sizeof(idname);

                if (KHM_SUCCEEDED(kmm_get_plugin_config(IDPROV_NAMEW, 0, &csp_p)) &&
                    KHM_SUCCEEDED(khc_read_string(csp_p, L"DefaultIdentity", didname, &cb)) &&
                    (cb = sizeof(idname)) != 0 &&
                    KHM_SUCCEEDED(kcdb_identity_get_name(ident, idname, &cb)) &&
                    !wcscmp(idname, didname)) {

                    kcdb_identity_set_default_int(ident);

                }

                if (csp_p)
                    khc_close_space(csp_p);
            }
        }
    }

    if (ks == NULL) {
        /* A keystore could not be created for the identity.  This may
           be because the keystore blob couldn't be read or was
           corrupted etc. */

        wchar_t buf[128] = L"";

        LoadString(hResModule, IDS_INVALIDID, buf, ARRAYLENGTH(buf));
        kcdb_identity_set_attr(ident, KCDB_ATTR_DISPLAY_NAME, buf,
                               KCDB_CBSIZE_AUTO);
        kcdb_identity_set_flags(ident, KCDB_IDENT_FLAG_INVALID, KCDB_IDENT_FLAG_INVALID);
        LoadString(hResModule, IDS_INVALIDREASON, buf, ARRAYLENGTH(buf));
        kcdb_identity_set_attr(ident, KCDB_ATTR_STATUS, buf, KCDB_CBSIZE_AUTO);
    }

    return KHM_ERROR_SUCCESS;
}

khm_int32
handle_kmsg_ident_update(khm_handle ident) {

    keystore_t * ks;

    ks = find_keystore_for_identity(ident);
    if (ks != NULL) {
        khm_boolean can_init;

        if (ks_keystore_has_key(ks)) {
            KSLOCK(ks);
            kcdb_identity_set_attr(ident, KCDB_ATTR_ISSUE, &ks->ft_key_ctime, sizeof(FILETIME));
            if (FtToInt(&ks->ft_key_lifetime) < KS_INF_KEY_LIFETIME)
                kcdb_identity_set_attr(ident, KCDB_ATTR_EXPIRE, &ks->ft_key_expire, sizeof(FILETIME));
            else
                kcdb_identity_set_attr(ident, KCDB_ATTR_EXPIRE, NULL, 0);
            KSUNLOCK(ks);
            can_init = TRUE;
        } else {
            kcdb_identity_set_attr(ident, KCDB_ATTR_ISSUE, NULL, 0);
            kcdb_identity_set_attr(ident, KCDB_ATTR_EXPIRE, NULL, 0);
            can_init = ks_is_keystore_locked(ks);
        }
        kcdb_identity_set_flags(ident,
                                KCDB_IDENT_FLAG_VALID | KCDB_IDENT_FLAG_KEY_STORE |
                                ((can_init)? KCDB_IDENT_FLAG_CRED_INIT : 0),
                                KCDB_IDENT_FLAG_VALID | KCDB_IDENT_FLAG_KEY_STORE |
                                KCDB_IDENT_FLAG_CRED_INIT);
    } else {
        kcdb_identity_set_attr(ident, KCDB_ATTR_ISSUE, NULL, 0);
        kcdb_identity_set_attr(ident, KCDB_ATTR_EXPIRE, NULL, 0);
        kcdb_identity_set_flags(ident, KCDB_IDENT_FLAG_INVALID, KCDB_IDENT_FLAG_INVALID);
    }

    if (ks)
        ks_keystore_release(ks);

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
    size_t cb = 0;

    buf[0] = L'\0';
    hicon = NULL;

    if (preq->h_obj != NULL) {
        keystore_t * ks;

        ks = find_keystore_for_identity(preq->h_obj);
        if (ks == NULL)
            return KHM_ERROR_SUCCESS;

        /* This is a resource request for a specific identity
           specified by h_obj */
        switch(preq->res_id) {

        case KCDB_RES_DISPLAYNAME:
        case KCDB_RES_DESCRIPTION:
            /* Not providing a description results in NetIDMgr using
               the name of the identity as the description. */
            StringCbCopy(buf, sizeof(buf), ks->display_name);
            break;

        case KCDB_RES_ICON_DISABLED:
        case KCDB_RES_ICON_NORMAL:
            hicon = LoadImage(hResModule, MAKEINTRESOURCE(IDI_IDENTITY),
                              IMAGE_ICON, 0, 0,
                              LR_DEFAULTSIZE | LR_DEFAULTCOLOR);
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
            hicon = LoadImage(hResModule, MAKEINTRESOURCE(IDI_IDENTITY),
                              IMAGE_ICON, 0, 0,
                              LR_DEFAULTSIZE | LR_DEFAULTCOLOR);
            break;

        default:
            return KHM_ERROR_SUCCESS;
        }
    }

    if (buf[0] != L'\0' &&
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
    } else if (hicon != NULL) {

        /* We are returngin an icon */

        if (preq->buf == NULL || preq->cb_buf < sizeof(HICON)) {
            preq->cb_buf = sizeof(HICON);
            preq->code = KHM_ERROR_TOO_LONG;
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
