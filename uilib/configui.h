/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
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

#ifndef __KHIMAIRA_CONFIGUI_H
#define __KHIMAIRA_CONFIGUI_H

struct tag_khui_config_node_i;

typedef struct tag_cfg_node_data {
    struct tag_khui_config_node_i * ctx_node;
    HWND        hwnd;
    LPARAM      param;
    khm_int32   flags;
} cfg_node_data;

typedef struct tag_khui_config_node_i {
    khm_int32   magic;          /* == KHUI_CONFIG_NODE_MAGIC */

    khui_config_node_reg reg;   /* registration information */
    khm_int32   id;             /* serial number */

    HWND        hwnd;           /* window handle to the dialog for this node */
    LPARAM      param;          /* parameter, usually a handle to a
                                   tree view control node. */

    cfg_node_data * data;       /* for subpanels, the window handles,
                                   parameter and flags need to be maintained for each  */
    khm_size    n_data;
    khm_size    nc_data;

    void *      private_data;   /* private data set with
                                   khui_cfg_set_data() */

    khm_int32   refcount;
    khm_int32   flags;          /* reg.flags is copied here during
                                   configuration node registration.
                                   From then on, this field will be
                                   updated with dynamic flags to
                                   reflect the state of the node.
                                   Note that reg.flags will not be
                                   modified with dynamic flags. */
    TDCL(struct tag_khui_config_node_i);
} khui_config_node_i;

#define KHUI_CONFIG_NODE_MAGIC 0x38f4cb52

#define KHUI_NODEDATA_ALLOC_INCR 8

#define KHUI_CN_FLAG_DELETED 0x0008

#define cfgui_is_valid_node_handle(v) \
((v) && ((khui_config_node_i *) (v))->magic == KHUI_CONFIG_NODE_MAGIC)

#define cfgui_is_valid_node(n) \
((n)->magic == KHUI_CONFIG_NODE_MAGIC)

#define cfgui_node_i_from_handle(v) \
((khui_config_node_i *) v)

#define cfgui_handle_from_node_i(n) \
((khui_config_node) n)

#endif
