/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
 * Copyright (c) 2007-2010 Secure Endpoints Inc.
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

#ifndef __KHIMAIRA_KCDB_IDENTITY_H
#define __KHIMAIRA_KCDB_IDENTITY_H

/* Identity */

#define KCDB_IDENT_HASHTABLE_SIZE 31

typedef struct tag_kcdb_identity {
    khm_int32 magic;            /*!< Must be KCDB_IDENT_MAGIC (invariant) */
    const wchar_t * name;       /*!< Name of the identity (invariant)*/
    kcdb_identpro_i * id_pro;   /*!< Held pointer to identity provider (invariant) */
    khm_ui_8 serial;            /*!< Serial number for the identity (invariant) */

    khm_int32 flags;            /*!< Identity flags from KCDB_IDENT_FLAG_* */
    khm_int32 refcount;         /*!< Reference count */
    kcdb_buf  buf;              /*!< Buffer for holding attributes */

    khm_ui_4  refresh_cycle;    /*!< Last refresh cycle during which
                                   this identity was looked at. */

    /* Statistics (based on activitiy in the root credentials set) */
    khm_size  n_credentials;    /*!< Total number of credentials for
                                   this identity */
    khm_size  n_id_credentials; /*!< Number of identity credentials
                                   for this identity */
    khm_size  n_init_credentials; /*!< Number of initial credentials */

    FILETIME  ft_lastupdate;    /*!< Time at which credentials were
                                   last updated for this identity. */

    FILETIME  ft_thr_last_update;
                                /*!< The timestamp for configuration
                                   space from which the threshold
                                   values were read from. */

    FILETIME  ft_thr_renew;     /*!< Renewal threshold */

    FILETIME  ft_thr_warn;      /*!< Warning threshold */

    FILETIME  ft_thr_crit;      /*!< Critical threshold */

    struct tag_kcdb_identity * parent;
                                /*!< Parent identity, if there is one. */

    LDCL(struct tag_kcdb_identity);
} kcdb_identity;

#define KCDB_IDENT_MAGIC 0x31938d4f

extern hashtable * kcdb_identities_namemap;
extern khm_int32 kcdb_n_identities;
extern kcdb_identity * kcdb_identities;         /* all identities */
extern kcdb_identity * kcdb_def_identity;       /* default identity */
extern khm_ui_4 kcdb_ident_refresh_cycle;       /* last refresh cycle */
extern CRITICAL_SECTION cs_ident;

void
kcdbint_ident_init(void);

void
kcdbint_ident_exit(void);

void
kcdbint_ident_msg_completion(kmq_message * m);

void
kcdbint_ident_post_message(khm_int32 op, kcdb_identity * id);

khm_int32
kcdbint_ident_attr_cb(khm_handle h, khm_int32 attr,
                      void * buf, khm_size *pcb_buf);

#define kcdb_is_identity(id) ((id) && ((kcdb_identity *)(id))->magic == KCDB_IDENT_MAGIC)

#ifndef  DEBUG

#define kcdb_is_active_identity(id) (kcdb_is_identity(id) && (((kcdb_identity *)(id))->flags & KCDB_IDENT_FLAG_ACTIVE))

#else

#include<assert.h>

__inline int kcdb_is_active_identity(const void * v) {
    if (!kcdb_is_identity(v))
        return 0;

    if (((const kcdb_identity *)v)->flags & KCDB_IDENT_FLAG_ACTIVE)
        return 1;

    return 0;
}

#endif

#endif
