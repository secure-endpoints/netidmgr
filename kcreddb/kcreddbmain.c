/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
 * Copyright (c) 2006-2009 Secure Endpoints Inc.
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

#include "kcreddbinternal.h"

HINSTANCE hinst_kcreddb;

void
kcdb_process_attach(HINSTANCE hinstDLL) {
    hinst_kcreddb = hinstDLL;
    kcdb_init();
}

void
kcdb_process_detach(void) {
    kcdb_exit();
}


#ifdef DEBUG
const wchar_t *
kcdb_resource_id_to_string(kcdb_resource_id i)
{
    switch (i) {
    case KCDB_RES_DISPLAYNAME:
        return L"DISPLAYNAME";

    case KCDB_RES_DESCRIPTION:
        return L"DESCRIPTION";

    case KCDB_RES_TOOLTIP:
        return L"TOOLTIP";

    case KCDB_RES_INSTANCE:
        return L"INSTANCE";

    case KCDB_RES_ICON_NORMAL:
        return L"ICON_NORMAL";

    case KCDB_RES_ICON_DISABLED:
        return L"ICON_DISABLED";

    default:
        return L"(Unknown)";
    }
}

khm_boolean
decode_kmsg_cred_message_to_string(wchar_t * buf, khm_size cb,
                                   khm_int32 ty, khm_int32 sty, khm_ui_4 up, void * vp)
{
    switch (sty) {
    case KMSG_CRED_ROOTDELTA:
        StringCbPrintf(buf, cb, L"<CRED, ROOTDELTA> for %s %s %s (%d)",
                       (up & KCDB_DELTA_ADD)? L"ADD":L"",
                       (up & KCDB_DELTA_DEL)? L"DEL":L"",
                       (up & KCDB_DELTA_MODIFY)? L"MODIFY":L"",
                       up);
        return TRUE;

    case KMSG_CRED_RESOURCE_REQ:
        {
            kcdb_resource_request * req = (kcdb_resource_request *) vp;

            StringCbPrintf(buf, cb, L"<CRED, RESOURCE_REQ> %s for 0x%p ",
                           kcdb_resource_id_to_string(req->res_id), req->h_obj);
        }
        return TRUE;
    };

    return FALSE;
}

const wchar_t *
kcdb_op_to_string(khm_int32 op)
{
    const wchar_t * ops[] = {
        L"(None)",
        L"OP_INSERT",
        L"OP_DELETE",
        L"OP_MODIFY",
        L"OP_ACTIVATE",
        L"OP_DEACTIVATE",
        L"OP_HIDE",
        L"OP_UNHIDE",
        L"OP_SETSEARCH",
        L"OP_UNSETSEARCH",
        L"OP_NEW_DEFAULT",
        L"OP_DELCONFIG",
        L"OP_RESUPDATE"
    };

    if (op >= 0 && op < ARRAYLENGTH(ops))
        return ops[op];
    else
        return L"(Unknown)";
}

khm_boolean
decode_kmsg_kcdb_message_to_string(wchar_t * buf, khm_size cb,
                                   khm_int32 ty, khm_int32 sty, khm_ui_4 up, void * vp)
{
    switch (sty) {
    case KMSG_KCDB_IDENT:
        StringCbPrintf(buf, cb,
                       L"<KCDB, IDENT> %s for %s", kcdb_op_to_string(up),
                       (vp)?((kcdb_identity *) vp)->name : L"(Null)");
        return TRUE;

    case KMSG_KCDB_CREDTYPE:
        StringCbPrintf(buf, cb,
                       L"<KCDB, CREDTYPE> %s for %s", kcdb_op_to_string(up),
                       (vp)?((kcdb_credtype *) vp)->name : L"(Null)");
        return TRUE;

    case KMSG_KCDB_ATTRIB:
        StringCbPrintf(buf, cb,
                       L"<KCDB, ATTRIB> %s for %s", kcdb_op_to_string(up),
                       (vp)?((kcdb_attrib_i*) vp)->attr.name : L"(Null)");
        return TRUE;

    case KMSG_KCDB_TYPE:
        StringCbPrintf(buf, cb,
                       L"<KCDB, TYPE> %s for %s", kcdb_op_to_string(up),
                       (vp)?((kcdb_type_i*) vp)->type.name : L"(Null)");
        return TRUE;

    case KMSG_KCDB_IDENTPRO:
        StringCbPrintf(buf, cb,
                       L"<KCDB, IDENTPRO> %s for %s", kcdb_op_to_string(up),
                       (vp)?((kcdb_identpro_i*) vp)->name : L"(Null)");
        return TRUE;

    }
    return FALSE;
}
#endif
