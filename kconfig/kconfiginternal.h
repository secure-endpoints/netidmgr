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

#ifndef __KHIMAIRA_KCONFIGINTERNAL_H
#define __KHIMAIRA_KCONFIGINTERNAL_H

#define _NIMLIB_

#include<windows.h>
#include<kconfig.h>
#include<khlist.h>
#include<kherror.h>
#include<utils.h>
#include<strsafe.h>

typedef struct kconf_conf_space {
    khm_int32           magic;
    wchar_t *           name;

    struct {
        const khc_provider_interface * provider;
        void * nodeHandle;
    } user, machine, schema;

    volatile LONG       refcount;
    khm_int32           flags;

    TDCL(struct kconf_conf_space);
} kconf_conf_space;

#define KCONF_SPACE_MAGIC 0x7E9488EE

#define khc_is_space(h)         ((h) && ((kconf_conf_space *)h)->magic == KCONF_SPACE_MAGIC)
#define khc_provider(h, t)      ((h)->t.provider)
#define khc_provider_data(h, t) ((h)->t.nodeHandle)

#define CCallIf(h,t,f)          (khc_provider(h,t)? (f) : KHM_ERROR_NO_PROVIDER)

#define CInit(h,t,pa,fl,ct)     CCallIf(h,t,(h)->t.provider->init((h),(pa),(fl),(ct),&(h)->t.nodeHandle))
#define CExit(h,t)              CCallIf(h,t,(h)->t.provider->exit((h)->t.nodeHandle))

#define COpen(h,t)              CCallIf(h,t,(h)->t.provider->open((h)->t.nodeHandle))
#define CClose(h,t)             CCallIf(h,t,(h)->t.provider->close((h)->t.nodeHandle))
#define CRemove(h,t)            CCallIf(h,t,(h)->t.provider->remove((h)->t.nodeHandle))
#define CCreate(h,t,na,f)       CCallIf(h,t,(h)->t.provider->create((h)->t.nodeHandle, (na), (f)))
#define CBeginEnum(h,t)         CCallIf(h,t,(h)->t.provider->begin_enum((h)->t.nodeHandle))
#define CGetMTime(h,t,pft)      CCallIf(h,t,(h)->t.provider->get_mtime((h)->t.nodeHandle, pft))

#define CReadValue(h,t,vn,pvt,pb,pcb) CCallIf(h,t,(h)->t.provider->read_value((h)->t.nodeHandle,(vn),(pvt),(pb),(pcb)))
#define CWriteValue(h,t,vn,vt,pb,cb) CCallIf(h,t,(h)->t.provider->write_value((h)->t.nodeHandle,(vn),(vt),(pb),(cb)))
#define CRemoveValue(h,t,na)    CCallIf(h,t,(h)->t.provider->remove_value((h)->t.nodeHandle, (na)))

#define KCONF_SPACE_FLAG_DELETE_U 0x00000040
#define KCONF_SPACE_FLAG_DELETE_M 0x00000080

typedef struct kconf_conf_handle {
    khm_int32   magic;
    khm_int32   flags;
    kconf_conf_space * space;

    struct kconf_conf_handle * lower;

    LDCL(struct kconf_conf_handle);
} kconf_handle;

#define KCONF_HANDLE_MAGIC 0x38eb49d2
#define khc_is_handle(h)    ((h) && ((kconf_handle *)h)->magic == KCONF_HANDLE_MAGIC)
#define khc_shadow(h)       (((kconf_handle *)h)->lower)
#define khc_is_shadowed(h)  (khc_is_handle(h) && khc_shadow(h) != NULL)

extern kconf_conf_space * conf_root;
extern kconf_handle *     conf_handles;
extern kconf_handle *     conf_free_handles;
extern CRITICAL_SECTION   cs_conf_global;
extern LONG volatile      conf_status;

#define khc_is_config_running() (conf_status != 0)

void init_kconf(void);
void exit_kconf(void);

/* handle operations */
#define khc_handle_flags(h)         (((kconf_handle *) h)->flags)
#define khc_is_machine_handle(h)    (((kconf_handle *) h)->flags & KCONF_FLAG_MACHINE)
#define khc_is_readonly(h)          (((kconf_handle *) h)->flags & KCONF_FLAG_READONLY)
#define khc_is_schema_handle(h)     (((kconf_handle *) h)->flags & KCONF_FLAG_SCHEMA)
#define khc_is_user_handle(h)       (((kconf_handle *) h)->flags & KCONF_FLAG_USER)
#define khc_space_from_handle(h)    (((kconf_handle *) h)->space)
#define khc_is_any(h)               ((h)==NULL || khc_is_handle(h) || khc_is_space(h))
#define khc_space_from_any(h)       (khc_is_handle(h)? khc_space_from_handle(h) : (khc_is_space(h)? (kconf_conf_space *) (h) : conf_root))

extern const khc_provider_interface khc_reg_provider;

extern const khc_provider_interface khc_schema_provider;

kconf_conf_space *
khcint_create_empty_space(void);

kconf_handle *
khcint_handle_from_space(kconf_conf_space * s, khm_int32 flags);

khm_boolean 
khcint_is_valid_name(wchar_t * name);

void
khcint_free_space(kconf_conf_space * r);

void
khcint_handle_free(kconf_handle * h);

void
khcint_space_hold(kconf_conf_space * s);

void
khcint_space_release(kconf_conf_space * s);

/* Schema */

khm_int32 
khcint_unload_schema(khm_handle parent, const kconf_schema * schema);

khm_int32
khcint_load_schema(khm_handle parent, const kconf_schema * schema, int * p_end);

#endif
