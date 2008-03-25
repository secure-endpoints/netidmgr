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

#include "krbcred.h"
#include<commctrl.h>
#include<strsafe.h>
#include<krb5.h>

#ifdef DEBUG
#include<assert.h>
#endif

khm_int32 type_id_enctype       = -1;
khm_int32 type_id_addr_list     = -1;
khm_int32 type_id_krb5_flags    = -1;
khm_int32 type_id_krb5_princ    = -1;
khm_int32 type_id_kvno          = -1;

BOOL type_regd_enctype      = FALSE;
BOOL type_regd_addr_list    = FALSE;
BOOL type_regd_krb5_flags   = FALSE;
BOOL type_regd_krb5_princ   = FALSE;
BOOL type_regd_kvno         = FALSE;

khm_int32 attr_id_key_enctype   = -1;
khm_int32 attr_id_tkt_enctype   = -1;
khm_int32 attr_id_addr_list     = -1;
khm_int32 attr_id_krb5_flags    = -1;
khm_int32 attr_id_krb5_ccname   = -1;
khm_int32 attr_id_kvno          = -1;
khm_int32 attr_id_krb5_idflags  = -1;

BOOL attr_regd_key_enctype  = FALSE;
BOOL attr_regd_tkt_enctype  = FALSE;
BOOL attr_regd_addr_list    = FALSE;
BOOL attr_regd_krb5_flags   = FALSE;
BOOL attr_regd_krb5_ccname  = FALSE;
BOOL attr_regd_kvno         = FALSE;
BOOL attr_regd_krb5_idflags = FALSE;

khm_int32 credtype_id_krb5 = KCDB_CREDTYPE_INVALID;
khm_boolean krb5_initialized = FALSE;
khm_handle krb5_credset = NULL;

khm_handle k5_sub = NULL;

krb5_context k5_identpro_ctx = NULL;

extern khm_int32 KHMAPI
k5_ident_name_comp_func(const void * dl, khm_size cb_dl,
                        const void * dr, khm_size cb_dr);

khm_int32
k5_register_data_types(void)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if(KHM_FAILED(kcdb_type_get_id(TYPENAME_ENCTYPE, &type_id_enctype))) {
        kcdb_type type;
        kcdb_type *t32;

        kcdb_type_get_info(KCDB_TYPE_INT32, &t32);

        type.id = KCDB_TYPE_INVALID;
        type.name = TYPENAME_ENCTYPE;
        type.flags = KCDB_TYPE_FLAG_CB_FIXED;
        type.cb_max = t32->cb_max;
        type.cb_min = t32->cb_min;
        type.isValid = t32->isValid;
        type.comp = t32->comp;
        type.dup = t32->dup;
        type.toString = enctype_toString;

        rv = kcdb_type_register(&type, &type_id_enctype);
        kcdb_type_release_info(t32);

        if(KHM_FAILED(rv))
            goto _exit;
        type_regd_enctype = TRUE;
    }

    if(KHM_FAILED(kcdb_type_get_id(TYPENAME_ADDR_LIST, &type_id_addr_list))) {
        kcdb_type type;
        kcdb_type *tdata;

        kcdb_type_get_info(KCDB_TYPE_DATA, &tdata);

        type.id = KCDB_TYPE_INVALID;
        type.name = TYPENAME_ADDR_LIST;
        type.flags = KCDB_TYPE_FLAG_CB_MIN;
        type.cb_min = 0;
        type.cb_max = 0;
        type.isValid = tdata->isValid;
        type.comp = addr_list_comp;
        type.dup = tdata->dup;
        type.toString = addr_list_toString;

        rv = kcdb_type_register(&type, &type_id_addr_list);
        kcdb_type_release_info(tdata);

        if(KHM_FAILED(rv))
            goto _exit;
        type_regd_addr_list = TRUE;
    }

    if(KHM_FAILED(kcdb_type_get_id(TYPENAME_KRB5_FLAGS, &type_id_krb5_flags))) {
        kcdb_type type;
        kcdb_type *t32;

        kcdb_type_get_info(KCDB_TYPE_INT32, &t32);

        type.id = KCDB_TYPE_INVALID;
        type.name = TYPENAME_KRB5_FLAGS;
        type.flags = KCDB_TYPE_FLAG_CB_FIXED;
        type.cb_max = t32->cb_max;
        type.cb_min = t32->cb_min;
        type.isValid = t32->isValid;
        type.comp = t32->comp;
        type.dup = t32->dup;
        type.toString = krb5flags_toString;

        rv = kcdb_type_register(&type, &type_id_krb5_flags);
        kcdb_type_release_info(t32);

        if(KHM_FAILED(rv))
            goto _exit;
        type_regd_krb5_flags = TRUE;
    }

    if (KHM_FAILED(kcdb_type_get_id(TYPENAME_KVNO, &type_id_kvno))) {
        kcdb_type type;
        kcdb_type *t32;

        kcdb_type_get_info(KCDB_TYPE_INT32, &t32);

        type.id = KCDB_TYPE_INVALID;
        type.name = TYPENAME_KVNO;
        type.flags = KCDB_TYPE_FLAG_CB_FIXED;
        type.cb_max = t32->cb_max;
        type.cb_min = t32->cb_min;
        type.isValid = t32->isValid;
        type.comp = t32->comp;
        type.dup = t32->dup;
        type.toString = kvno_toString;

        rv = kcdb_type_register(&type, &type_id_kvno);
        kcdb_type_release_info(t32);

        if (KHM_FAILED(rv))
            goto _exit;

        type_regd_kvno = TRUE;
    }

    if (KHM_FAILED(kcdb_type_get_id(TYPENAME_KRB5_PRINC, 
                                    &type_id_krb5_princ))) {
        kcdb_type type;
        kcdb_type *pstr;

        kcdb_type_get_info(KCDB_TYPE_STRING, &pstr);

        ZeroMemory(&type, sizeof(type));
        type.name = TYPENAME_KRB5_PRINC;
        type.id = KCDB_TYPE_INVALID;
        type.flags = KCDB_TYPE_FLAG_CB_AUTO;
        type.cb_min = pstr->cb_min;
        type.cb_max = pstr->cb_max;
        type.toString = pstr->toString;
        type.isValid = pstr->isValid;
        type.comp = k5_ident_name_comp_func;
        type.dup = pstr->dup;

        kcdb_type_register(&type, &type_id_krb5_princ);

        kcdb_type_release_info(pstr);

        type_regd_krb5_princ = TRUE;
    }

 _exit:
    return rv;
}

khm_int32
k5_register_attributes(void)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if(KHM_FAILED(kcdb_attrib_get_id(ATTRNAME_KEY_ENCTYPE, &attr_id_key_enctype))) {
        kcdb_attrib attrib;
        wchar_t sbuf[KCDB_MAXCCH_SHORT_DESC];
        wchar_t lbuf[KCDB_MAXCCH_SHORT_DESC];

        /* although we are loading a long descriptoin, it still fits
           in the short descriptoin buffer */

        ZeroMemory(&attrib, sizeof(attrib));

        attrib.name = ATTRNAME_KEY_ENCTYPE;
        attrib.id = KCDB_ATTR_INVALID;
        attrib.type = type_id_enctype;
        attrib.flags = KCDB_ATTR_FLAG_TRANSIENT;
        LoadString(hResModule, IDS_KEY_ENCTYPE_SHORT_DESC, sbuf, ARRAYLENGTH(sbuf));
        LoadString(hResModule, IDS_KEY_ENCTYPE_LONG_DESC, lbuf, ARRAYLENGTH(lbuf));
        attrib.short_desc = sbuf;
        attrib.long_desc = lbuf;
        
        rv = kcdb_attrib_register(&attrib, &attr_id_key_enctype);

        if(KHM_FAILED(rv))
            goto _exit;

        attr_regd_key_enctype = TRUE;
    }

    if(KHM_FAILED(kcdb_attrib_get_id(ATTRNAME_TKT_ENCTYPE, &attr_id_tkt_enctype))) {
        kcdb_attrib attrib;
        wchar_t sbuf[KCDB_MAXCCH_SHORT_DESC];
        wchar_t lbuf[KCDB_MAXCCH_SHORT_DESC];
        /* although we are loading a long descriptoin, it still fits
        in the short descriptoin buffer */

        ZeroMemory(&attrib, sizeof(attrib));

        attrib.name = ATTRNAME_TKT_ENCTYPE;
        attrib.id = KCDB_ATTR_INVALID;
        attrib.type = type_id_enctype;
        attrib.flags = KCDB_ATTR_FLAG_TRANSIENT;
        LoadString(hResModule, IDS_TKT_ENCTYPE_SHORT_DESC, sbuf, ARRAYLENGTH(sbuf));
        LoadString(hResModule, IDS_TKT_ENCTYPE_LONG_DESC, lbuf, ARRAYLENGTH(lbuf));
        attrib.short_desc = sbuf;
        attrib.long_desc = lbuf;
        
        rv = kcdb_attrib_register(&attrib, &attr_id_tkt_enctype);

        if(KHM_FAILED(rv))
            goto _exit;

        attr_regd_tkt_enctype = TRUE;
    }

    if(KHM_FAILED(kcdb_attrib_get_id(ATTRNAME_ADDR_LIST, &attr_id_addr_list))) {
        kcdb_attrib attrib;
        wchar_t sbuf[KCDB_MAXCCH_SHORT_DESC];
        wchar_t lbuf[KCDB_MAXCCH_SHORT_DESC];
        /* although we are loading a long descriptoin, it still fits
        in the short descriptoin buffer */

        ZeroMemory(&attrib, sizeof(attrib));

        attrib.name = ATTRNAME_ADDR_LIST;
        attrib.id = KCDB_ATTR_INVALID;
        attrib.type = type_id_addr_list;
        attrib.flags = KCDB_ATTR_FLAG_TRANSIENT;
        LoadString(hResModule, IDS_ADDR_LIST_SHORT_DESC, sbuf, ARRAYLENGTH(sbuf));
        LoadString(hResModule, IDS_ADDR_LIST_LONG_DESC, lbuf, ARRAYLENGTH(lbuf));
        attrib.short_desc = sbuf;
        attrib.long_desc = lbuf;
        
        rv = kcdb_attrib_register(&attrib, &attr_id_addr_list);

        if(KHM_FAILED(rv))
            goto _exit;

        attr_regd_addr_list = TRUE;
    }

    if(KHM_FAILED(kcdb_attrib_get_id(ATTRNAME_KRB5_FLAGS, &attr_id_krb5_flags))) {
        kcdb_attrib attrib;
        wchar_t sbuf[KCDB_MAXCCH_SHORT_DESC];

        /* although we are loading a long descriptoin, it still fits
        in the short descriptoin buffer */

        ZeroMemory(&attrib, sizeof(attrib));

        attrib.name = ATTRNAME_KRB5_FLAGS;
        attrib.id = KCDB_ATTR_INVALID;
        attrib.type = type_id_krb5_flags;
        attrib.flags = KCDB_ATTR_FLAG_TRANSIENT;
        LoadString(hResModule, IDS_KRB5_FLAGS_SHORT_DESC, sbuf, ARRAYLENGTH(sbuf));
        attrib.short_desc = sbuf;
        attrib.long_desc = NULL;
        
        rv = kcdb_attrib_register(&attrib, &attr_id_krb5_flags);

        if(KHM_FAILED(rv))
            goto _exit;

        attr_regd_krb5_flags = TRUE;
    }

    if(KHM_FAILED(kcdb_attrib_get_id(ATTRNAME_KRB5_CCNAME, &attr_id_krb5_ccname))) {
        kcdb_attrib attrib;
        wchar_t sbuf[KCDB_MAXCCH_SHORT_DESC];
        wchar_t lbuf[KCDB_MAXCCH_SHORT_DESC];
        /* although we are loading a long descriptoin, it still fits
        in the short descriptoin buffer */

        ZeroMemory(&attrib, sizeof(attrib));

        attrib.name = ATTRNAME_KRB5_CCNAME;
        attrib.id = KCDB_ATTR_INVALID;
        attrib.type = KCDB_TYPE_STRING;
        attrib.flags =
	  KCDB_ATTR_FLAG_PROPERTY |
	  KCDB_ATTR_FLAG_TRANSIENT;
        LoadString(hResModule, IDS_KRB5_CCNAME_SHORT_DESC, sbuf, ARRAYLENGTH(sbuf));
        LoadString(hResModule, IDS_KRB5_CCNAME_LONG_DESC, lbuf, ARRAYLENGTH(lbuf));
        attrib.short_desc = sbuf;
        attrib.long_desc = lbuf;
        
        rv = kcdb_attrib_register(&attrib, &attr_id_krb5_ccname);

        if(KHM_FAILED(rv))
            goto _exit;

        attr_regd_krb5_ccname = TRUE;
    }

    if (KHM_FAILED(kcdb_attrib_get_id(ATTRNAME_KVNO, &attr_id_kvno))) {
        kcdb_attrib attrib;
        wchar_t sbuf[KCDB_MAXCCH_SHORT_DESC];
        wchar_t lbuf[KCDB_MAXCCH_LONG_DESC];
        /* although we are loading a long description, it still fits
           in the short description buffer */

        ZeroMemory(&attrib, sizeof(attrib));

        attrib.name = ATTRNAME_KVNO;
        attrib.id = KCDB_ATTR_INVALID;
        attrib.type = type_id_kvno;
        attrib.flags = KCDB_ATTR_FLAG_TRANSIENT;
        LoadString(hResModule, IDS_KVNO_SHORT_DESC, sbuf, ARRAYLENGTH(sbuf));
        LoadString(hResModule, IDS_KVNO_LONG_DESC, lbuf, ARRAYLENGTH(lbuf));
        attrib.short_desc = sbuf;
        attrib.long_desc = lbuf;

        rv = kcdb_attrib_register(&attrib, &attr_id_kvno);

        if (KHM_FAILED(rv))
            goto _exit;

        attr_regd_kvno = TRUE;
    }

    if (KHM_FAILED(kcdb_attrib_get_id(ATTRNAME_KRB5_IDFLAGS, &attr_id_krb5_idflags))) {
        kcdb_attrib attrib;

        ZeroMemory(&attrib, sizeof(attrib));

        attrib.name = ATTRNAME_KRB5_IDFLAGS;
        attrib.id = KCDB_ATTR_INVALID;
        attrib.type = KCDB_TYPE_INT32;
        attrib.flags = KCDB_ATTR_FLAG_PROPERTY |
            KCDB_ATTR_FLAG_HIDDEN;
        /* we don't bother localizing these strings since the
           attribute is hidden.  The user will not see these
           descriptions anyway. */
        attrib.short_desc = L"Krb5 ID flags";
        attrib.long_desc = L"Kerberos 5 Identity Flags";

        rv = kcdb_attrib_register(&attrib, &attr_id_krb5_idflags);

        if (KHM_FAILED(rv))
            goto _exit;

        attr_regd_krb5_idflags = TRUE;
    }

 _exit:
    return rv;
}

khm_int32
k5_register_credtype(void)
{
    kcdb_credtype ct;
    wchar_t short_desc[KCDB_MAXCCH_SHORT_DESC];
    wchar_t long_desc[KCDB_MAXCCH_LONG_DESC];

    ZeroMemory(&ct, sizeof(ct));
    ct.id = KCDB_CREDTYPE_AUTO;
    ct.name = KRB5_CREDTYPE_NAME;
    ct.short_desc = short_desc;
    ct.long_desc = long_desc;

    LoadString(hResModule, IDS_KRB5_SHORT_DESC, 
               short_desc, ARRAYLENGTH(short_desc));

    LoadString(hResModule, IDS_KRB5_LONG_DESC, 
               long_desc, ARRAYLENGTH(long_desc));

    ct.icon = NULL; /* TODO: set a proper icon */

    kmq_create_subscription(k5_msg_callback, &ct.sub);

    ct.is_equal = khm_krb5_creds_is_equal;

    return kcdb_credtype_register(&ct, &credtype_id_krb5);
}

void
k5_unregister_credtype(void)
{
    if(credtype_id_krb5 >= 0) {
        kcdb_credtype_unregister(credtype_id_krb5);
    }
}

void
k5_unregister_data_types(void)
{
    if(type_regd_enctype)
        kcdb_type_unregister(type_id_enctype);
    if(type_regd_addr_list)
        kcdb_type_unregister(type_id_addr_list);
    if(type_regd_krb5_flags)
        kcdb_type_unregister(type_id_krb5_flags);
    if(type_regd_kvno)
        kcdb_type_unregister(type_id_kvno);
    if(type_regd_krb5_princ)
        kcdb_type_unregister(type_id_krb5_princ);
}

void
k5_unregister_attributes(void)
{
    if(attr_regd_key_enctype)
        kcdb_attrib_unregister(attr_id_key_enctype);
    if(attr_regd_tkt_enctype)
        kcdb_attrib_unregister(attr_id_tkt_enctype);
    if(attr_regd_addr_list)
        kcdb_attrib_unregister(attr_id_addr_list);
    if(attr_regd_krb5_flags)
        kcdb_attrib_unregister(attr_id_krb5_flags);
    if(attr_regd_krb5_ccname)
        kcdb_attrib_unregister(attr_id_krb5_ccname);
    if(attr_regd_kvno)
        kcdb_attrib_unregister(attr_id_kvno);
    if(attr_regd_krb5_idflags)
        kcdb_attrib_unregister(attr_id_krb5_idflags);
}

/*  The system message handler.

    Runs in the context of the plugin thread */
khm_int32 KHMAPI 
k5_msg_system(khm_int32 msg_type, khm_int32 msg_subtype, 
              khm_ui_4 uparam, void * vparam)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;

    switch(msg_subtype) {
    case KMSG_SYSTEM_INIT:

        rv = k5_register_data_types();
        if (KHM_FAILED(rv)) break;

        rv = k5_register_attributes();
        if (KHM_FAILED(rv)) break;

        rv = k5_register_credtype();
        if (KHM_FAILED(rv)) break;

        rv = kcdb_credset_create(&krb5_credset);
        if (KHM_FAILED(rv)) break;

        krb5_initialized = TRUE;

        kmq_create_subscription(k5_msg_callback, &k5_sub);

        k5_register_config_panels();

        {
            krb5_context ctx = NULL;

            khm_krb5_list_tickets(&ctx);

            if(ctx != NULL)
                pkrb5_free_context(ctx);
        }

        break;

    case KMSG_SYSTEM_EXIT:

        k5_unregister_config_panels();
        k5_unregister_credtype();

        kcdb_credset_delete(krb5_credset);
        krb5_credset = NULL;

        if(k5_sub != NULL) {
            kmq_delete_subscription(k5_sub);
            k5_sub = NULL;
        }

        k5_unregister_attributes();
        k5_unregister_data_types();

        break;
    }

    return rv;
}

khm_int32 KHMAPI
k5_msg_kcdb(khm_int32 msg_type, khm_int32 msg_subtype,
            khm_ui_4 uparam, void * vparam)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;

    switch(msg_subtype) {
    case KMSG_KCDB_IDENT:
        if (uparam == KCDB_OP_DELCONFIG) {
            k5_remove_from_LRU((khm_handle) vparam);
        }
        break;
    }

    return rv;
}


/* Handler for CRED type messages

    Runs in the context of the Krb5 plugin
*/
khm_int32 KHMAPI 
k5_msg_cred(khm_int32 msg_type, khm_int32 msg_subtype, 
            khm_ui_4 uparam, void * vparam)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;

    switch(msg_subtype) {
    case KMSG_CRED_REFRESH:
        {
            krb5_context ctx = NULL;

            khm_krb5_list_tickets(&ctx);

            if(ctx != NULL)
                pkrb5_free_context(ctx);
        }
        break;

    case KMSG_CRED_DESTROY_CREDS:
        {
            khui_action_context * ctx;

            ctx = (khui_action_context *) vparam;

            if (ctx->credset) {
                _begin_task(0);
                _report_mr0(KHERR_INFO, MSG_ERR_CTX_DESTROY_CREDS);
                _describe();

                khm_krb5_destroy_by_credset(ctx->credset);

                _end_task();
            }
        }
        break;

    case KMSG_CRED_PP_BEGIN:
        k5_pp_begin((khui_property_sheet *) vparam);
        break;

    case KMSG_CRED_PP_END:
        k5_pp_end((khui_property_sheet *) vparam);
        break;

    default:
        if(IS_CRED_ACQ_MSG(msg_subtype))
            return k5_msg_cred_dialog(msg_type, msg_subtype, 
                                      uparam, vparam);
    }

    return rv;
}

/*  The main message handler.  We don't do much here, except delegate
    to other message handlers

    Runs in the context of the Krb5 plugin
*/
khm_int32 KHMAPI 
k5_msg_callback(khm_int32 msg_type, khm_int32 msg_subtype, 
                khm_ui_4 uparam, void * vparam)
{
    switch(msg_type) {
    case KMSG_SYSTEM:
        return k5_msg_system(msg_type, msg_subtype, uparam, vparam);
    case KMSG_CRED:
        return k5_msg_cred(msg_type, msg_subtype, uparam, vparam);
    case KMSG_KCDB:
        return k5_msg_kcdb(msg_type, msg_subtype, uparam, vparam);
    }
    return KHM_ERROR_SUCCESS;
}
