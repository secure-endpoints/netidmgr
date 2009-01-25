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

#include "kcreddbinternal.h"
#include<assert.h>

CRITICAL_SECTION cs_enum;

void
kcdbint_enum_init(void)
{
    InitializeCriticalSection(&cs_enum);
}

void
kcdbint_enum_exit(void)
{
    DeleteCriticalSection(&cs_enum);
}


void
kcdbint_enum_create(kcdb_enumeration * pe)
{
    kcdb_enumeration e;

    e = PMALLOC(sizeof(*e));
    ZeroMemory(e, sizeof(*e));

    e->magic = KCDB_ENUM_MAGIC;

    *pe = e;
}

void
kcdbint_enum_free_objs(kcdb_enumeration e)
{
    khm_size i;

#ifdef DEBUG
    assert(e->objs != NULL || e->n == 0);
#endif

    for (i=0; i < e->n; i++)
        kcdb_buf_release(e->objs[i]);

    if (e->objs)
        PFREE(e->objs);

    e->n = 0;
    e->objs = NULL;
}

void
kcdbint_enum_alloc(kcdb_enumeration e, khm_size n)
{
    if (e->objs)
        kcdbint_enum_free_objs(e);

    e->n = n;
    e->objs = PCALLOC(n, sizeof(e->objs[0]));
}

KHMEXP khm_int32 KHMAPI
kcdb_enum_next(kcdb_enumeration e, khm_handle * ph)
{

    if (!kcdb_is_enum(e))
        return KHM_ERROR_INVALID_PARAM;

    if (*ph) {
        kcdb_buf_release(*ph);
        *ph = NULL;
    }

    if (e->next < e->n) {
        *ph = e->objs[e->next++];
        kcdb_buf_hold(*ph);
    } else {
        *ph = NULL;
    }

    if (*ph)
        return KHM_ERROR_SUCCESS;
    else
        return KHM_ERROR_NOT_FOUND;
}

KHMEXP khm_int32 KHMAPI
kcdb_enum_reset(kcdb_enumeration e)
{
    if (!kcdb_is_enum(e))
        return KHM_ERROR_INVALID_PARAM;

    e->next = 0;

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
kcdb_enum_end(kcdb_enumeration e)
{
    if (!kcdb_is_enum(e))
        return KHM_ERROR_INVALID_PARAM;

    kcdbint_enum_free_objs(e);
    PFREE(e);

    return KHM_ERROR_SUCCESS;
}

static kcdb_comp_func compare_func;
static void *         compare_param;

static int compare_pobj(const void * e1, const void * e2)
{
    khm_handle *p1 = (khm_handle *) e1;
    khm_handle *p2 = (khm_handle *) e2;

    if (*p1 == *p2)
        return 0;

    return (*compare_func)(*p1, *p2, compare_param);
}

KHMEXP khm_int32 KHMAPI
kcdb_enum_sort(kcdb_enumeration e,
               kcdb_comp_func   f,
               void * vparam)
{
    if (!kcdb_is_enum(e))
        return KHM_ERROR_INVALID_PARAM;

    EnterCriticalSection(&cs_enum);
    if (e->n > 1) {
        compare_func = f;
        compare_param = vparam;
        qsort(e->objs, e->n, sizeof(e->objs[0]), compare_pobj);
    }
    LeaveCriticalSection(&cs_enum);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
kcdb_enum_filter(kcdb_enumeration e,
                 kcdb_filter_func f,
                 void * vparam)
{
    khm_size i;

    if (!kcdb_is_enum(e))
        return KHM_ERROR_INVALID_PARAM;

    for (i=0; i < e->n; i++) {
        if (!(*f)(e->objs[i], vparam)) {
            kcdb_buf_release(e->objs[i]);
            if (i+1 < e->n)
                memmove(&e->objs[i], &e->objs[i+1], (e->n - (i+1)) * sizeof(e->objs[0]));
            e->n--;
            --i;
        }
    }

    kcdb_enum_reset(e);
    return KHM_ERROR_SUCCESS;
}
