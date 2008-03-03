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

#define _NIMLIB_

#include<khuidefs.h>
#include<kmm.h>
#include<configui.h>
#include<utils.h>
#include<assert.h>

#include<strsafe.h>

khm_int32 cfgui_node_serial;
CRITICAL_SECTION cs_cfgui;
khui_config_node_i * cfgui_root_config;
HWND hwnd_cfgui = NULL;

static khui_config_node_i *
cfgui_create_new_node(void) {
    khui_config_node_i * node;

    node = PMALLOC(sizeof(*node));
#ifdef DEBUG
    assert(node);
#endif
    ZeroMemory(node, sizeof(*node));
    node->magic = KHUI_CONFIG_NODE_MAGIC;

    EnterCriticalSection(&cs_cfgui);
    node->id = ++cfgui_node_serial;
    LeaveCriticalSection(&cs_cfgui);

    return node;
}

/* called with cs_cfgui held */
static void 
cfgui_free_node(khui_config_node_i * node) {
    if (!cfgui_is_valid_node(node))
        return;

    if (node->reg.name)
        PFREE((void *) node->reg.name);

    if (node->reg.short_desc)
        PFREE((void *) node->reg.short_desc);

    if (node->reg.long_desc)
        PFREE((void *) node->reg.long_desc);

#ifdef DEBUG
    /* This node shouldn't have a parent, children nor siblings */
    assert(TFIRSTCHILD(node) == NULL);
    assert(TPARENT(node) == NULL);
    assert(LNEXT(node) == NULL);
    assert(LPREV(node) == NULL);
#endif

    node->magic = 0;

    ZeroMemory(node, sizeof(*node));

    PFREE(node);
}


static void
cfgui_hold_node(khui_config_node_i * node) {
    EnterCriticalSection(&cs_cfgui);
    node->refcount++;
    LeaveCriticalSection(&cs_cfgui);
}


static void
cfgui_release_node(khui_config_node_i * node) {
    EnterCriticalSection(&cs_cfgui);
    node->refcount--;
    if (node->refcount == 0 &&
        (node->flags & KHUI_CN_FLAG_DELETED)) {
        khui_config_node_i * parent;

        parent = TPARENT(node);
#ifdef DEBUG
        assert(TFIRSTCHILD(node) == NULL);
        assert(parent != NULL);
#endif
        TDELCHILD(parent, node);
        cfgui_free_node(node);
        cfgui_release_node(parent);
    }
    LeaveCriticalSection(&cs_cfgui);
}

KHMEXP khm_int32 KHMAPI
khui_cfg_register(khui_config_node vparent,
                  const khui_config_node_reg * reg) {

    size_t cb_name;
    size_t cb_short_desc;
    size_t cb_long_desc;
    khui_config_node_i * node;
    khui_config_node_i * parent;
    khui_config_node t;
    wchar_t * name;
    wchar_t * short_desc;
    wchar_t * long_desc;

    if (!reg ||
        FAILED(StringCbLength(reg->name,
                              KHUI_MAXCB_NAME,
                              &cb_name)) ||
        FAILED(StringCbLength(reg->short_desc,
                              KHUI_MAXCB_SHORT_DESC,
                              &cb_short_desc)) ||
        FAILED(StringCbLength(reg->long_desc,
                              KHUI_MAXCB_LONG_DESC,
                              &cb_long_desc)) ||
        (vparent &&
         !cfgui_is_valid_node_handle(vparent)))
        return KHM_ERROR_INVALID_PARAM;

    if (KHM_SUCCEEDED(khui_cfg_open(vparent,
                                  reg->name,
                                  &t))) {
        khui_cfg_release(t);
        return KHM_ERROR_DUPLICATE;
    }

    cb_name += sizeof(wchar_t);
    cb_short_desc += sizeof(wchar_t);
    cb_long_desc += sizeof(wchar_t);

    node = cfgui_create_new_node();

    node->reg = *reg;
    node->reg.flags &= KHUI_CNFLAGMASK_STATIC;
    if (node->reg.flags & KHUI_CNFLAG_PLURAL)
        node->reg.flags |= KHUI_CNFLAG_SUBPANEL;

    name = PMALLOC(cb_name);
    StringCbCopy(name, cb_name, reg->name);
    short_desc = PMALLOC(cb_short_desc);
    StringCbCopy(short_desc, cb_short_desc, reg->short_desc);
    long_desc = PMALLOC(cb_long_desc);
    StringCbCopy(long_desc, cb_long_desc, reg->long_desc);

    node->reg.name = name;
    node->reg.short_desc = short_desc;
    node->reg.long_desc = long_desc;
    node->flags = node->reg.flags;

    if (vparent == NULL) {
        parent = cfgui_root_config;
    } else {
        parent = cfgui_node_i_from_handle(vparent);
    }

    EnterCriticalSection(&cs_cfgui);
    TADDCHILD(parent, node);
    cfgui_hold_node(parent);

    if (hwnd_cfgui) {
        SendMessage(hwnd_cfgui, KHUI_WM_CFG_NOTIFY,
                    MAKEWPARAM(0, WMCFG_SYNC_NODE_LIST), 0);
    }

    LeaveCriticalSection(&cs_cfgui);

    /* When the root config list changes, we need to notify the UI.
       this way, the Options menu can be kept in sync. */
    if (parent == cfgui_root_config) {
        kmq_post_message(KMSG_ACT, KMSG_ACT_SYNC_CFG, 0, 0);
    }

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
khui_cfg_open(khui_config_node vparent,
              const wchar_t * name,
              khui_config_node * result) {
    khui_config_node_i * parent;
    khui_config_node_i * c;
    size_t sz;

    if ((vparent &&
         !cfgui_is_valid_node_handle(vparent)) ||
        FAILED(StringCbLength(name, KHUI_MAXCB_NAME, &sz)) ||
        !result)
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_cfgui);
    if (vparent)
        parent = cfgui_node_i_from_handle(vparent);
    else
        parent = cfgui_root_config;

    c = TFIRSTCHILD(parent);
    while(c) {
        if (!(c->flags & KHUI_CN_FLAG_DELETED) &&
            !wcscmp(c->reg.name, name))
            break;
        c = LNEXT(c);
    }

    if (c) {
        *result = cfgui_handle_from_node_i(c);
        cfgui_hold_node(c);
    } else {
        *result = NULL;
    }
    LeaveCriticalSection(&cs_cfgui);

    if (*result)
        return KHM_ERROR_SUCCESS;
    else
        return KHM_ERROR_NOT_FOUND;
}

KHMEXP khm_int32 KHMAPI
khui_cfg_remove(khui_config_node vnode) {
    khui_config_node_i * node;
    if (!cfgui_is_valid_node_handle(vnode))
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_cfgui);
    node = cfgui_node_i_from_handle(vnode);
    node->flags |= KHUI_CN_FLAG_DELETED;

    if (hwnd_cfgui) {
        SendMessage(hwnd_cfgui, KHUI_WM_CFG_NOTIFY,
                    MAKEWPARAM(0, WMCFG_SYNC_NODE_LIST), 0);
    }

    /* when the root config list changes, we need to notify the UI.
       this way, the Options menu can be kept in sync. */
    if (TPARENT(node) == cfgui_root_config) {
        kmq_post_message(KMSG_ACT, KMSG_ACT_SYNC_CFG, 0, 0);
    }
    LeaveCriticalSection(&cs_cfgui);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
khui_cfg_hold(khui_config_node vnode) {
    if (!cfgui_is_valid_node_handle(vnode))
        return KHM_ERROR_INVALID_PARAM;

    cfgui_hold_node(cfgui_node_i_from_handle(vnode));

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
khui_cfg_release(khui_config_node vnode) {
    if (!cfgui_is_valid_node_handle(vnode))
        return KHM_ERROR_INVALID_PARAM;

    cfgui_release_node(cfgui_node_i_from_handle(vnode));

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
khui_cfg_get_parent(khui_config_node vnode,
                    khui_config_node * result) {

    khui_config_node_i * node;
    khui_config_node_i * parent;

    if(!cfgui_is_valid_node_handle(vnode) ||
       !result)
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_cfgui);
    if (cfgui_is_valid_node_handle(vnode)) {
        node = cfgui_node_i_from_handle(vnode);
        parent = TPARENT(node);
        if (parent == cfgui_root_config)
            parent = NULL;
    } else {
        parent = NULL;
    }
    if (parent) {
        cfgui_hold_node(parent);
    }
    LeaveCriticalSection(&cs_cfgui);

    *result = parent;

    if (parent)
        return KHM_ERROR_SUCCESS;
    else
        return KHM_ERROR_NOT_FOUND;
}

KHMEXP khm_int32 KHMAPI
khui_cfg_get_first_child(khui_config_node vparent,
                         khui_config_node * result) {
    khui_config_node_i * parent;
    khui_config_node_i * c;

    if((vparent && !cfgui_is_valid_node_handle(vparent)) ||
       !result)
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_cfgui);
    if (cfgui_is_valid_node_handle(vparent)) {
        parent = cfgui_node_i_from_handle(vparent);
    } else if (!vparent) {
        parent = cfgui_root_config;
    } else {
        parent = NULL;
    }

    if (parent) {
        for(c = TFIRSTCHILD(parent);
            c &&
                ((c->reg.flags & KHUI_CNFLAG_SUBPANEL) ||
                 (c->flags & KHUI_CN_FLAG_DELETED));
            c = LNEXT(c));
    } else {
        c = NULL;
    }

    if (c)
        cfgui_hold_node(c);
    LeaveCriticalSection(&cs_cfgui);

    *result = c;

    if (c)
        return KHM_ERROR_SUCCESS;
    else
        return KHM_ERROR_NOT_FOUND;
}

KHMEXP khm_int32 KHMAPI
khui_cfg_get_first_subpanel(khui_config_node vparent,
                            khui_config_node * result) {
    khui_config_node_i * parent;
    khui_config_node_i * c;

    if((vparent && !cfgui_is_valid_node_handle(vparent)) ||
       !result)
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_cfgui);
    if (cfgui_is_valid_node_handle(vparent)) {
        parent = cfgui_node_i_from_handle(vparent);
    } else if (!vparent) {
        parent = cfgui_root_config;
    } else {
        parent = NULL;
    }

    if (parent) {
        for(c = TFIRSTCHILD(parent);
            c &&
                (!(c->reg.flags & KHUI_CNFLAG_SUBPANEL) ||
                 (c->flags & KHUI_CN_FLAG_DELETED));
            c = LNEXT(c));
    } else {
        c = NULL;
    }

    if (c)
        cfgui_hold_node(c);
    LeaveCriticalSection(&cs_cfgui);

    *result = c;

    if (c)
        return KHM_ERROR_SUCCESS;
    else
        return KHM_ERROR_NOT_FOUND;
}

KHMEXP khm_int32 KHMAPI
khui_cfg_get_next(khui_config_node vnode,
                  khui_config_node * result) {

    khui_config_node_i * node;
    khui_config_node_i * nxt_node;

    if (!cfgui_is_valid_node_handle(vnode) ||
        !result)
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_cfgui);
    if (cfgui_is_valid_node_handle(vnode)) {
        node = cfgui_node_i_from_handle(vnode);
        for(nxt_node = LNEXT(node);
            nxt_node &&
                ((nxt_node->flags & KHUI_CN_FLAG_DELETED) ||
                 ((node->reg.flags ^ nxt_node->reg.flags) & 
                  KHUI_CNFLAG_SUBPANEL));
            nxt_node = LNEXT(nxt_node));
        if (nxt_node)
            cfgui_hold_node(nxt_node);
    } else {
        nxt_node = NULL;
    }
    LeaveCriticalSection(&cs_cfgui);

    *result = cfgui_handle_from_node_i(nxt_node);

    if (nxt_node)
        return KHM_ERROR_SUCCESS;
    else
        return KHM_ERROR_NOT_FOUND;
}

KHMEXP khm_int32 KHMAPI
khui_cfg_get_next_release(khui_config_node * pvnode) {

    khui_config_node_i * node;
    khui_config_node_i * nxt_node;

    if (!pvnode || 
        !cfgui_is_valid_node_handle(*pvnode))
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_cfgui);
    if (cfgui_is_valid_node_handle(*pvnode)) {
        node = cfgui_node_i_from_handle(*pvnode);
        for(nxt_node = LNEXT(node);
            nxt_node &&
                (((node->reg.flags ^ nxt_node->reg.flags) & 
                  KHUI_CNFLAG_SUBPANEL) ||
                 (nxt_node->flags & KHUI_CN_FLAG_DELETED));
            nxt_node = LNEXT(nxt_node));
        if (nxt_node)
            cfgui_hold_node(nxt_node);
        cfgui_release_node(node);
    } else {
        nxt_node = NULL;
    }
    LeaveCriticalSection(&cs_cfgui);

    *pvnode = cfgui_handle_from_node_i(nxt_node);

    if (nxt_node)
        return KHM_ERROR_SUCCESS;
    else
        return KHM_ERROR_NOT_FOUND;
}

KHMEXP khm_int32 KHMAPI
khui_cfg_get_reg(khui_config_node vnode,
                 khui_config_node_reg * reg) {

    khui_config_node_i * node;

    

    if ((vnode && !cfgui_is_valid_node_handle(vnode)) ||
        !reg)
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_cfgui);
    if (cfgui_is_valid_node_handle(vnode)) {
        node = cfgui_node_i_from_handle(vnode);
        *reg = node->reg;
    } else if (!vnode) {
        node = cfgui_root_config;
        *reg = node->reg;
    } else {
        node = NULL;
        ZeroMemory(reg, sizeof(*reg));
    }
    LeaveCriticalSection(&cs_cfgui);

    if (node)
        return KHM_ERROR_SUCCESS;
    else
        return KHM_ERROR_INVALID_PARAM;
}

static void
clear_node_data(khui_config_node_i * node) {
    node->n_data = 0;
}

static cfg_node_data *
get_node_data(khui_config_node_i * node,
              khui_config_node_i * ctx_node,
              khm_boolean create) {
    khm_size i;

    for (i=0; i<node->n_data; i++) {
        if (node->data[i].ctx_node == ctx_node)
            return &(node->data[i]);
    }

    if (!create)
        return NULL;

    if (node->n_data == node->nc_data) {
        node->nc_data = UBOUNDSS(node->n_data + 1,
                                 KHUI_NODEDATA_ALLOC_INCR,
                                 KHUI_NODEDATA_ALLOC_INCR);
#ifdef DEBUG
        assert(node->nc_data >= node->n_data + 1);
#endif
        node->data = PREALLOC(node->data, sizeof(node->data[0]) * node->nc_data);
#ifdef DEBUG
        assert(node->data);
#endif
    }

    i = node->n_data++;
    node->data[i].ctx_node = ctx_node;
    node->data[i].hwnd = NULL;
    node->data[i].param = 0;
    node->data[i].flags = 0;

    return &(node->data[i]);
}

/* Macros for basic getters and setters */

#define FBODY_OP(Type, Member, DDCL1, DDCL2, COND, LVALUE, RVALUE, RV ) \
    {                                                                   \
        khui_config_node_i * node;                                      \
        DDCL1; DDCL2;                                                   \
        EnterCriticalSection(&cs_cfgui);                                \
        if (cfgui_is_valid_node_handle(vnode))                          \
            node = cfgui_node_i_from_handle(vnode);                     \
        else if (!vnode)                                                \
            node = cfgui_root_config;                                   \
        else                                                            \
            node = NULL;                                                \
        if (COND) {                                                     \
            LVALUE = RVALUE;                                            \
        }                                                               \
        LeaveCriticalSection(&cs_cfgui);                                \
        RV;                                                             \
    }

#define DEFUN_GET_OP(Type, Member, ExtName)                             \
    KHMEXP Type KHMAPI khui_cfg_get_ ## ExtName (khui_config_node vnode) \
         FBODY_OP(Type, Member, Type Member = (Type) 0, (void) 0, node, Member, node->Member, return Member)

#define DEFUN_SET_OP(Type, Member, ExtName)                             \
    KHMEXP void KHMAPI khui_cfg_set_ ## ExtName                         \
    (khui_config_node vnode,                                            \
     Type Member)                                                       \
         FBODY_OP(Type, Member, (void) 0, (void) 0, node, node->Member, Member, return)

#define DEFUN_GET_OP_INST(Type, Member, ExtName)                        \
    KHMEXP Type KHMAPI khui_cfg_get_ ## ExtName ## _inst                \
    (khui_config_node vnode, khui_config_node ctx)                      \
         FBODY_OP(Type, Member, cfg_node_data * pdata, Type Member = (Type) 0, node && (pdata = get_node_data(node, ctx, FALSE)), Member, pdata->Member, return Member)

#define DEFUN_SET_OP_INST(Type, Member, ExtName)                        \
    KHMEXP void KHMAPI khui_cfg_set_ ## ExtName ## _inst                \
    (khui_config_node vnode, khui_config_node ctx,                      \
     Type Member)                                                       \
         FBODY_OP(Type, Member, cfg_node_data * pdata, (void) 0, node && (pdata = get_node_data(node, ctx, TRUE)), pdata->Member, Member, return)

DEFUN_GET_OP(HWND,hwnd,hwnd);
DEFUN_SET_OP(HWND,hwnd,hwnd);
DEFUN_GET_OP(LPARAM,param,param);
DEFUN_SET_OP(LPARAM,param,param);
DEFUN_GET_OP(void *,private_data, data);
DEFUN_SET_OP(void *,private_data, data);
DEFUN_GET_OP_INST(HWND,hwnd,hwnd);
DEFUN_SET_OP_INST(HWND,hwnd,hwnd);
DEFUN_GET_OP_INST(LPARAM,param,param);
DEFUN_SET_OP_INST(LPARAM,param,param);
DEFUN_GET_OP(khm_int32, flags, flags);

#undef DEFUN_SET_OP
#undef DEFUN_GET_OP
#undef DEFUN_SET_OP_INST
#undef DEFUN_GET_OP_INST
#undef FBODY_OP

/* called with cs_cfgui held  */
static void 
cfgui_clear_params(khui_config_node_i * node) {
    khui_config_node_i * c;

    node->hwnd = NULL;
    node->param = 0;
    node->flags &= KHUI_CNFLAGMASK_STATIC;
    clear_node_data(node);

    c = TFIRSTCHILD(node);
    while(c) {
        cfgui_clear_params(c);
        c = LNEXT(c);
    }
}

KHMEXP void KHMAPI
khui_cfg_clear_params(void) {
    EnterCriticalSection(&cs_cfgui);
    cfgui_clear_params(cfgui_root_config);
    LeaveCriticalSection(&cs_cfgui);
}

KHMEXP void KHMAPI
khui_cfg_set_configui_handle(HWND hwnd) {
    EnterCriticalSection(&cs_cfgui);
    hwnd_cfgui = hwnd;
    LeaveCriticalSection(&cs_cfgui);
}

KHMEXP void KHMAPI
khui_cfg_set_flags(khui_config_node vnode, 
                   khm_int32 flags,
                   khm_int32 mask) {
    khui_config_node_i * node;
    khm_int32 newflags;

    if (vnode &&
        !cfgui_is_valid_node_handle(vnode))
        return;

    mask &= KHUI_CNFLAGMASK_DYNAMIC;

    EnterCriticalSection(&cs_cfgui);
    if (cfgui_is_valid_node_handle(vnode)) {

        node = cfgui_node_i_from_handle(vnode);

        newflags = 
            (flags & mask) |
            (node->flags & ~mask);

        if (newflags != node->flags) {
            node->flags = newflags;

            if (hwnd_cfgui)
                PostMessage(hwnd_cfgui, KHUI_WM_CFG_NOTIFY,
                            MAKEWPARAM((WORD)newflags, WMCFG_UPDATE_STATE),
                            (LPARAM) vnode);
        }
    }
    LeaveCriticalSection(&cs_cfgui);
}

/* called with cs_cfgui held */
static void
recalc_node_flags(khui_config_node vnode, khm_boolean plural) {
    khui_config_node_i * node;
    khui_config_node_i * parent;
    khui_config_node_i * subpanel;
    cfg_node_data * data;
    khm_int32 flags;

#ifdef DEBUG
    assert(cfgui_is_valid_node_handle(vnode));
#endif

    node = cfgui_node_i_from_handle(vnode);

    if (plural)
        parent = TPARENT(node);
    else
        parent = node;
#ifdef DEBUG
    assert(parent);
#endif

    flags = 0;

    for(subpanel = TFIRSTCHILD(parent); subpanel;
        subpanel = LNEXT(subpanel)) {
        if (!(subpanel->reg.flags & KHUI_CNFLAG_SUBPANEL) ||
            (plural && !(subpanel->reg.flags & KHUI_CNFLAG_PLURAL)) ||
            (!plural && (subpanel->reg.flags & KHUI_CNFLAG_PLURAL)))
            continue;

        data = get_node_data(subpanel, vnode, FALSE);

        if (data) {
            flags |= data->flags;
        }
    }

    flags &= KHUI_CNFLAGMASK_DYNAMIC;

    if ((node->flags & KHUI_CNFLAGMASK_DYNAMIC) == flags)
        return;

    node->flags = (node->flags & ~KHUI_CNFLAGMASK_DYNAMIC) | flags;

    if (hwnd_cfgui)
        PostMessage(hwnd_cfgui, KHUI_WM_CFG_NOTIFY,
                    MAKEWPARAM((WORD) node->flags, WMCFG_UPDATE_STATE),
                    (LPARAM) vnode);
}

KHMEXP void KHMAPI
khui_cfg_set_flags_inst(khui_config_init_data * d,
                        khm_int32 flags,
                        khm_int32 mask) {
    khui_config_node_i * node;
    cfg_node_data * data;

    if (!cfgui_is_valid_node_handle(d->this_node))
        return;

    mask &= KHUI_CNFLAGMASK_DYNAMIC;

    EnterCriticalSection(&cs_cfgui);
    if (cfgui_is_valid_node_handle(d->this_node))
        node = cfgui_node_i_from_handle(d->this_node);
    else 
        node = NULL;

    if (node) {
        data = get_node_data(node, d->ctx_node, TRUE);
        if (data) {
            khm_int32 new_flags;

            new_flags = (flags & mask) |
                (data->flags & ~mask);

            if (new_flags != data->flags) {
                data->flags = new_flags;

                if (d->ctx_node != d->ref_node)
                    recalc_node_flags(d->ctx_node, TRUE);
                else
                    recalc_node_flags(d->ctx_node, FALSE);
            }
        }
    }
    LeaveCriticalSection(&cs_cfgui);
}

KHMEXP khm_int32 KHMAPI
khui_cfg_get_name(khui_config_node vnode,
                  wchar_t * buf,
                  khm_size * cb_buf) {
    khui_config_node_i * node;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    if (!cb_buf ||
        !cfgui_is_valid_node_handle(vnode))
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_cfgui);
    if (cfgui_is_valid_node_handle(vnode)) {
        khm_size cb;

        node = cfgui_node_i_from_handle(vnode);

        StringCbLength(node->reg.name, KHUI_MAXCCH_NAME, &cb);

        if (buf == NULL || cb > *cb_buf) {
            *cb_buf = cb;
            rv = KHM_ERROR_TOO_LONG;
        } else {
            StringCbCopy(buf, *cb_buf, node->reg.name);
            *cb_buf = cb;
        }
    } else {
        rv = KHM_ERROR_INVALID_PARAM;
    }
    LeaveCriticalSection(&cs_cfgui);

    return rv;
}

KHMEXP khm_int32 KHMAPI
khui_cfg_init_dialog_data(HWND hwnd_dlg,
                          const khui_config_init_data * data,
                          khm_size cb_extra,
                          khui_config_init_data ** new_data,
                          void ** extra) {
    khm_size cb;
    khui_config_init_data * d;

    cb = sizeof(khui_config_init_data) + cb_extra;
    d = PMALLOC(cb);
#ifdef DEBUG
    assert(d);
#endif
    ZeroMemory(d, cb);

    *d = *data;

    if (d->ctx_node)
        khui_cfg_hold(d->ctx_node);
    if (d->this_node)
        khui_cfg_hold(d->this_node);
    if (d->ref_node)
        khui_cfg_hold(d->ref_node);

#pragma warning(push)
#pragma warning(disable: 4244)
    SetWindowLongPtr(hwnd_dlg, DWLP_USER, (LONG_PTR) d);
#pragma warning(pop)

    if (new_data)
        *new_data = d;
    if (extra)
        *extra = (void *) (d + 1);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
khui_cfg_get_dialog_data(HWND hwnd_dlg,
                         khui_config_init_data ** data,
                         void ** extra) {
    khui_config_init_data * d;

    d = (khui_config_init_data *) (LONG_PTR)
        GetWindowLongPtr(hwnd_dlg, DWLP_USER);

#ifdef DEBUG
    assert(d);
#endif

    *data = d;
    if (extra)
        *extra = (void *) (d + 1);

    return (d)?KHM_ERROR_SUCCESS: KHM_ERROR_NOT_FOUND;
}

KHMEXP khm_int32 KHMAPI
khui_cfg_free_dialog_data(HWND hwnd_dlg) {
    khui_config_init_data * d;

    d = (khui_config_init_data *) (LONG_PTR)
        GetWindowLongPtr(hwnd_dlg, DWLP_USER);

#ifdef DEBUG
    assert(d);
#endif

    if (d) {
        if (d->ctx_node)
            khui_cfg_release(d->ctx_node);
        if (d->this_node)
            khui_cfg_release(d->this_node);
        if (d->ref_node)
            khui_cfg_release(d->ref_node);
        ZeroMemory(d, sizeof(*d));
        PFREE(d);
        SetWindowLongPtr(hwnd_dlg, DWLP_USER, 0);
    }

    return (d)?KHM_ERROR_SUCCESS: KHM_ERROR_NOT_FOUND;
}

void cfgui_init(void) {
    InitializeCriticalSection(&cs_cfgui);
    cfgui_root_config = cfgui_create_new_node();
    cfgui_node_serial = 0;
    hwnd_cfgui = NULL;
}

void cfgui_exit(exit) {
    DeleteCriticalSection(&cs_cfgui);
}

