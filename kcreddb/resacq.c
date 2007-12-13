/*
 * Copyright (c) 2007 Massachusetts Institute of Technology
 * Copyright (c) 2007 Secure Endpoints Inc.
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

#include<kcreddbinternal.h>
#include<khrescache.h>
#include<assert.h>

#define MAKERESID(r_id, flags) (((r_id) << 16) | (flags))

#define BUFSIZE 8192

KHMEXP khm_int32 KHMAPI
kcdb_cleanup_resources_by_handle(khm_handle h)
{
    return khui_cache_del_by_owner(h);
}

static khm_int32
get_resource_from_cred_type(kcdb_resource_request * preq)
{
    khm_handle sub;
    khm_int32  rv;

    sub = kcdb_credtype_get_sub((khm_int32)(INT_PTR) preq->h_obj);
    if (sub == NULL)
        return KHM_ERROR_INVALID_PARAM;

    rv = kmq_send_sub_msg(sub, KMSG_CRED, KMSG_CRED_RESOURCE_REQ,
                          0, preq);

    if (KHM_FAILED(rv))
        return rv;

    if (preq->code == KHM_ERROR_NOT_IMPLEMENTED) {
        rv = KHM_ERROR_NOT_FOUND;

#ifdef DEBUG
        assert(FALSE);
#endif

        switch (preq->res_id) {
        }
    }

    return rv;
}

static khm_int32
get_resource_from_identity(kcdb_resource_request * preq)
{
    kcdb_identity * id = (kcdb_identity *) preq->h_obj;
    khm_int32 rv;
    khm_size cb;

#ifdef DEBUG
    assert(kcdb_is_identity(id));
#endif

    /* There are some resources that we don't call the identity
       provider for */
    switch (preq->res_id) {
    case KCDB_RES_DISPLAYNAME:
        cb = preq->cb_buf;
        if (KHM_FAILED(kcdb_identity_get_attr(preq->h_obj,
                                              KCDB_ATTR_DISPLAY_NAME, NULL,
                                              preq->buf, &cb)))
            return kcdb_identity_get_name(preq->h_obj, preq->buf, &preq->cb_buf);

        preq->cb_buf = cb;
        return KHM_ERROR_SUCCESS;
    }

    if (!kcdb_is_identity(id) || !kcdb_is_identpro(id->id_pro) ||
        id->id_pro->sub == NULL)
        return KHM_ERROR_INVALID_PARAM;

    rv = kmq_send_sub_msg(id->id_pro->sub, KMSG_IDENT, KMSG_IDENT_RESOURCE_REQ,
                          0, preq);

    if (KHM_FAILED(rv))
        return rv;

    if (preq->code == KHM_ERROR_NOT_IMPLEMENTED) {
        rv = KHM_ERROR_NOT_FOUND;

#ifdef DEBUG
        assert(FALSE);
#endif

        switch (preq->res_id) {
        }
    }

    return rv;
}

static khm_int32
get_resource_from_identpro(kcdb_resource_request * preq)
{
    kcdb_identpro_i * idp = kcdb_identpro_from_handle(preq->h_obj);
    khm_int32 rv;

#ifdef DEBUG
    assert(kcdb_is_identpro(idp));
#endif

    if (!kcdb_is_identpro(idp) || idp->sub == NULL)
        return KHM_ERROR_INVALID_PARAM;

    preq->h_obj = NULL;

    rv = kmq_send_sub_msg(idp->sub, KMSG_IDENT, KMSG_IDENT_RESOURCE_REQ,
                          0, preq);

    if (KHM_FAILED(rv))
        return rv;

    if (preq->code == KHM_ERROR_NOT_IMPLEMENTED) {
        rv = KHM_ERROR_NOT_FOUND;

#ifdef DEBUG
        assert(FALSE);
#endif

        switch (preq->res_id) {
        }
    }

    return rv;
}

static khm_int32
get_resource_from_credential(kcdb_resource_request * preq)
{
    kcdb_cred * c = (kcdb_cred *) preq->h_obj;
    khm_handle sub;
    khm_int32 rv;

#ifdef DEBUG
    assert(kcdb_cred_is_cred(c));
#endif

    if (!kcdb_cred_is_cred(c) ||
        (sub = kcdb_credtype_get_sub(c->type)) == NULL)
        return KHM_ERROR_INVALID_PARAM;

    rv = kmq_send_sub_msg(sub, KMSG_CRED, KMSG_CRED_RESOURCE_REQ,
                          0, preq);

    if (KHM_FAILED(rv))
        return rv;

    if (preq->code == KHM_ERROR_NOT_IMPLEMENTED) {
        rv = KHM_ERROR_NOT_FOUND;

#ifdef DEBUG
        assert(FALSE);
#endif

        switch (preq->res_id) {

        }
    }

    return rv;
}

static khm_int32
get_resource_from_owner(kcdb_resource_request * preq)
{

    preq->code = KHM_ERROR_NOT_IMPLEMENTED;

    if ((((UINT_PTR) preq->h_obj) >> 16) == 0) {
        return get_resource_from_cred_type(preq);
    } else {
        khm_int32 * pmagic = (khm_int32 *) preq->h_obj;
        switch (*pmagic) {
        case KCDB_IDENT_MAGIC:
            return get_resource_from_identity(preq);

        case KCDB_IDENTPRO_MAGIC:
            return get_resource_from_identpro(preq);

        case KCDB_CRED_MAGIC:
            return get_resource_from_credential(preq);

        default:
#ifdef DEBUG
            assert(FALSE);
#endif
        }
    }

    return KHM_ERROR_INVALID_PARAM;
}

/* retrieve a specific resource from the given handle using the cache
   if possible */
static khm_int32
get_resource_with_cache(kcdb_resource_request * preq)
{
    khm_int32 rv;

    rv = khui_cache_get_resource(preq->h_obj,
                                 MAKERESID(preq->res_id, preq->flags),
                                 preq->res_type, preq->buf, &preq->cb_buf);

    if (rv == KHM_ERROR_NOT_FOUND) {
        if (preq->buf && preq->cb_buf >= BUFSIZE) {
            rv = get_resource_from_owner(preq);

            if (rv == KHM_ERROR_SUCCESS) {
                khui_cache_add_resource(preq->h_obj,
                                        MAKERESID(preq->res_id, preq->flags),
                                        preq->res_type, preq->buf, preq->cb_buf);
            }
        } else {
            BYTE buf[BUFSIZE];
            void *   caller_buf;
            khm_size caller_cb;

            caller_buf = preq->buf;
            preq->buf = buf;
            caller_cb = preq->cb_buf;
            preq->cb_buf = sizeof(buf);

            rv = get_resource_from_owner(preq);

            if (rv == KHM_ERROR_SUCCESS) {
                khui_cache_add_resource(preq->h_obj,
                                        MAKERESID(preq->res_id, preq->flags),
                                        preq->res_type, preq->buf, preq->cb_buf);

                if (caller_buf && caller_cb >= preq->cb_buf) {
                    memcpy(caller_buf, preq->buf, preq->cb_buf);
                } else {
                    rv = KHM_ERROR_TOO_LONG;
                }
            }

            preq->buf = caller_buf;
        }

        if (rv == KHM_ERROR_NOT_FOUND) {
            /* cache the negative result so we won't have to dispatch
               another expensive call. */

            HANDLE h = NULL;

#ifdef DEBUG
        assert(FALSE);
#endif

            switch (preq->res_type) {
            case KHM_RESTYPE_STRING:
                khui_cache_add_resource(preq->h_obj,
                                        MAKERESID(preq->res_id, preq->flags),
                                        preq->res_type, L"", sizeof(wchar_t));
                break;

            case KHM_RESTYPE_BITMAP:
            case KHM_RESTYPE_ICON:
                khui_cache_add_resource(preq->h_obj,
                                        MAKERESID(preq->res_id, preq->flags),
                                        preq->res_type, &h, sizeof(h));
                break;

            default:
#ifdef DEBUG
                assert(FALSE);
#endif
            }
        }
    } else {
        preq->code = rv;
    }

    return rv;
}

KHMEXP khm_int32 KHMAPI
kcdb_get_resource(khm_handle h,
                  kcdb_resource_id r_id,
                  khm_int32 flags,
                  khm_int32 *prflags,
                  void * vparam,
                  void * buf,
                  khm_size * pcb_buf)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;
    kcdb_resource_request req;

    ZeroMemory(&req, sizeof(req));

    if (h == NULL || pcb_buf == NULL)
        return KHM_ERROR_INVALID_PARAM;

    if (r_id > KCDB_RES_T_BEGINSTRING && r_id < KCDB_RES_T_ENDSTRING)
        req.res_type = KHM_RESTYPE_STRING;
    else if (r_id > KCDB_RES_T_BEGINICON && r_id < KCDB_RES_T_ENDICON)
        req.res_type = KHM_RESTYPE_ICON;
    else
        return KHM_ERROR_INVALID_PARAM;

    req.magic = KCDB_RESOURCE_REQ_MAGIC;
    req.h_obj = h;
    req.res_id = r_id;
    req.flags = flags;
    req.vparam = vparam;
    req.buf = buf;
    req.cb_buf = *pcb_buf;

    rv = get_resource_with_cache(&req);

    if (rv == KHM_ERROR_NOT_FOUND) {
    }

    *pcb_buf = req.cb_buf;
    if (prflags)
        *prflags = req.rflags;

    return rv;
}
