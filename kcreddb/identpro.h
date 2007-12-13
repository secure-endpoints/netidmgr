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

#ifndef __KHIMAIRA_KCDB_IDENTPRO_H
#define __KHIMAIRA_KCDB_IDENTPRO_H

typedef khm_handle kmm_plugin;

/* Identity Providers */

typedef struct tag_kcdb_identpro_i {
    khm_int32  magic;           /*!< Must be KCDB_IDENTPRO_MAGIC (invariant) */
    wchar_t *  name;            /*!< Name of identity provider (invariant) */

    khm_handle sub;             /*!< Message subscription (invariant) */
    kmm_plugin plugin;          /*!< Plug-in for identity provider (invariant) */

    khm_handle default_id;      /*!< Default identity for this provider */
    khm_int32  cred_type;       /*!< Primary credentials type */

    khm_int32  flags;
#define KCDB_IDENTPRO_FLAG_DELETED 0x08000000L

    khm_int32  refcount;

    LDCL(struct tag_kcdb_identpro_i);
} kcdb_identpro_i;

#define KCDB_IDENTPRO_MAGIC 0xdf03386f

#define kcdb_is_identpro(idp) ((idp) && ((kcdb_identpro_i *)(idp))->magic == KCDB_IDENTPRO_MAGIC)

#define kcdb_is_active_identpro(idp) (kcdb_is_identpro(idp) && !((((kcdb_identpro_i *)(idp))->flags) & KCDB_IDENTPRO_FLAG_DELETED))

#define kcdb_identpro_from_handle(vidp) ((kcdb_identpro_i *) vidp)

#define kcdb_handle_from_identpro(idp) ((khm_handle) idp)

typedef struct tag_kcdb_identpro_enumeration {
    khm_int32 magic;            /*!< Must be KCDB_IDENTPRO_ENUM_MAGIC */
    khm_size  n_providers;      /*!< Number of providers in enumeration */
    khm_size  next;             /*!< Next provider */
    kcdb_identpro_i ** providers; /*!< Providers */
} kcdb_identpro_enumeration;

#define KCDB_IDENTPRO_ENUM_MAGIC 0x596f241f

#define kcdb_is_identpro_enum(ie) ((ie) && ((kcdb_identpro_enumeration *) ie)->magic == KCDB_IDENTPRO_ENUM_MAGIC)

extern CRITICAL_SECTION cs_identpro;

void
kcdbint_identpro_init(void);

void
kcdbint_identpro_exit(void);

void
kcdbint_identpro_post_message(khm_int32 op, kcdb_identpro_i * p);

void
kcdbint_identpro_msg_completion(kmq_message * m);

#endif
