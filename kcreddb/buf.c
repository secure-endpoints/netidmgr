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

void kcdb_buf_new(kcdb_buf * buf, khm_size n_fields)
{
    buf->buffer = PMALLOC(KCDB_BUF_CBBUF_INITIAL);
    buf->cb_buffer = KCDB_BUF_CBBUF_INITIAL;
    buf->cb_used = 0;

    if(n_fields == KCDB_BUF_DEFAULT)
        n_fields = KCDB_BUF_FIELDS_INITIAL;

    assert(n_fields < KCDB_BUF_MAX_SLOTS);

    buf->n_fields = 0;
    buf->nc_fields = UBOUNDSS(n_fields, KCDB_BUF_FIELDS_INITIAL, KCDB_BUF_FIELDS_GROWTH);
    buf->fields = PMALLOC(sizeof(buf->fields[0]) * buf->nc_fields);
    ZeroMemory(buf->fields, sizeof(buf->fields[0]) * buf->nc_fields);
}

void kcdb_buf_delete(kcdb_buf * buf)
{
    if(buf->buffer) {
        SecureZeroMemory(buf->buffer, buf->cb_buffer);
        PFREE(buf->buffer);
    }
    buf->cb_buffer = 0;
    buf->cb_used = 0;
    buf->buffer = NULL;

    buf->n_fields = 0;
    buf->nc_fields = 0;
    if(buf->fields)
        PFREE(buf->fields);
    buf->fields = NULL;
}

static void kcdb_buf_assert_size(kcdb_buf * buf, khm_size cbsize)
{
    khm_size new_size;
    void * new_buffer;

    /* should be less than or equal to the max signed 32 bit int */
    assert(cbsize <= KHM_INT32_MAX);
    if(cbsize <= buf->cb_buffer)
        return;

    new_size = UBOUNDSS(cbsize, KCDB_BUF_CBBUF_INITIAL, KCDB_BUF_CBBUF_GROWTH);

    assert(new_size > buf->cb_buffer && new_size > 0);

    new_buffer = PMALLOC(new_size);
    if (new_buffer != NULL) {
        ZeroMemory(new_buffer, new_size);
        memcpy(new_buffer, buf->buffer, buf->cb_buffer);
    }
    SecureZeroMemory(buf->buffer, buf->cb_buffer);
    PFREE(buf->buffer);
    buf->buffer = new_buffer;
    assert(buf->buffer != NULL);
    if (buf->buffer == NULL) {
        buf->cb_buffer = 0;
        buf->cb_used = 0;
        buf->n_fields = 0;
    }
}

void kcdb_buf_alloc(kcdb_buf * buf, khm_size slot, khm_ui_2 id, khm_size cbsize)
{
    khm_size cbnew;
    khm_ssize cbdelta;
    khm_size cbold;
    kcdb_buf_field * f;

    cbnew = UBOUND32(cbsize);

    assert(slot <= KCDB_BUF_APPEND);

    if(slot == KCDB_BUF_APPEND) {
        slot = kcdb_buf_slot_by_id(buf, id);
        if(slot == KCDB_BUF_INVALID_SLOT)
            slot = kcdb_buf_first_free_slot(buf);
    }

    assert(slot < KCDB_BUF_MAX_SLOTS);

    if((slot + 1) > buf->nc_fields) {
        khm_size ns;

        ns = UBOUNDSS((slot + 1), KCDB_BUF_FIELDS_INITIAL, KCDB_BUF_FIELDS_GROWTH);

        buf->fields = PREALLOC(buf->fields, sizeof(buf->fields[0]) * ns);

        if (ns > buf->n_fields)
            memset(&(buf->fields[buf->n_fields]), 0,
                   sizeof(buf->fields[0]) * (ns - buf->n_fields));

        buf->nc_fields = ns;
    }

    if((slot + 1) > buf->n_fields)
        buf->n_fields = slot + 1;

    f = &(buf->fields[slot]);

    if(f->flags & KCDB_CREDF_FLAG_ALLOCD) {
        /* there's already an allocation.  we have to resize it to
           accomodate the new size */
        cbold = UBOUND32(f->cbsize);
        /* demote before substraction */
        cbdelta = ((khm_ssize) cbnew) - (khm_ssize) cbold;

        if(cbnew > cbold) {
            kcdb_buf_assert_size(buf, buf->cb_used + cbdelta);
        }

        if(buf->cb_used > f->offset + cbold) {
            khm_size i;

            memmove(((BYTE *) buf->buffer) + (f->offset + cbnew),
                    ((BYTE *) buf->buffer) + (f->offset + cbold),
                    buf->cb_used - (f->offset + cbold));

            for(i=0; i < (int) buf->n_fields; i++) {
                if(i != slot && 
                   (buf->fields[i].flags & KCDB_CREDF_FLAG_ALLOCD) &&
                   buf->fields[i].offset > f->offset) {
                    buf->fields[i].offset = 
                        (khm_ui_4)(((khm_ssize) buf->fields[i].offset) + cbdelta);
                }
            }
        }

        /* demote integer before adding signed quantity */
        buf->cb_used = (khm_size)(((khm_ssize) buf->cb_used) + cbdelta);

        f->cbsize = (khm_ui_4) cbsize;

    } else {
        kcdb_buf_assert_size(buf, buf->cb_used + cbnew);
        f->offset = (khm_ui_4) buf->cb_used;
        f->cbsize = (khm_ui_4) cbsize;
        buf->cb_used += cbnew;
    }

    if(cbsize == 0) {
        f->flags &= ~KCDB_CREDF_FLAG_ALLOCD;
        f->flags &= ~KCDB_CREDF_FLAG_DATA;
        f->id = KCDB_BUFF_ID_INVALID;
    } else {
        f->flags |= KCDB_CREDF_FLAG_ALLOCD;
        f->id = id;
    }
}

void kcdb_buf_dup(kcdb_buf * dest, const kcdb_buf * src)
{
    khm_size cb_buf;
    khm_size nc_fields;

    cb_buf = UBOUNDSS(src->cb_used, KCDB_BUF_CBBUF_INITIAL, KCDB_BUF_CBBUF_GROWTH);

    kcdb_buf_delete(dest);

    dest->cb_buffer = cb_buf;
    dest->cb_used = src->cb_used;
    dest->buffer = PMALLOC(cb_buf);
    memcpy(dest->buffer, src->buffer, src->cb_used);

    nc_fields = UBOUNDSS(src->n_fields, KCDB_BUF_FIELDS_INITIAL, KCDB_BUF_FIELDS_GROWTH);
    dest->nc_fields = nc_fields;
    dest->n_fields = src->n_fields;
    dest->fields = PMALLOC(nc_fields * sizeof(dest->fields[0]));
    memcpy(dest->fields, src->fields, src->n_fields * sizeof(dest->fields[0]));
    if(dest->n_fields < dest->nc_fields)
        memset(&(dest->fields[dest->n_fields]), 0, (dest->nc_fields - dest->n_fields) * sizeof(dest->fields[0]));
}

void kcdb_buf_set_value(kcdb_buf * buf, khm_size slot, khm_ui_2 id, void * src, khm_size cb_src)
{
    void * dest;
    kcdb_buf_alloc(buf, slot, id, cb_src);
    if(slot == KCDB_BUF_APPEND) {
        slot = kcdb_buf_slot_by_id(buf, id);
        if(slot == KCDB_BUF_INVALID_SLOT) {
#ifdef DEBUG
            assert(FALSE);
#else
            return;
#endif
        }
    }
    if(kcdb_buf_exist(buf, slot)) {
        dest = kcdb_buf_get(buf, slot);
        memcpy(dest, src, cb_src);

        buf->fields[slot].flags |= KCDB_CREDF_FLAG_DATA;
    }
}

int kcdb_buf_exist(kcdb_buf * buf, khm_size slot)
{
    if(slot >= buf->n_fields)
        return 0;
    return (buf->fields[slot].flags & KCDB_CREDF_FLAG_ALLOCD);
}

int kcdb_buf_val_exist(kcdb_buf * buf, khm_size slot)
{
    if(slot >= buf->n_fields)
        return 0;
    return (buf->fields[slot].flags & KCDB_CREDF_FLAG_DATA);
}

void * kcdb_buf_get(kcdb_buf * buf, khm_size slot)
{
    if(slot >= buf->n_fields || 
        !(buf->fields[slot].flags & KCDB_CREDF_FLAG_ALLOCD))
        return NULL;
    return (((BYTE *) buf->buffer) + buf->fields[slot].offset);
}

khm_size kcdb_buf_size(kcdb_buf * buf, khm_size slot)
{
    if(slot >= buf->n_fields || 
        !(buf->fields[slot].flags & KCDB_CREDF_FLAG_ALLOCD))
        return 0;
    return (buf->fields[slot].cbsize);
}

void kcdb_buf_set_value_flag(kcdb_buf * buf, khm_size slot)
{
    if(slot >= buf->n_fields || 
        !(buf->fields[slot].flags & KCDB_CREDF_FLAG_ALLOCD))
        return;

    (buf->fields[slot].flags |= KCDB_CREDF_FLAG_DATA);
}

khm_size kcdb_buf_first_free_slot(kcdb_buf * buf)
{
    khm_size i;

    for (i=0; i < buf->n_fields; i++) {
        if (buf->fields[i].id == KCDB_BUFF_ID_INVALID)
            break;
    }

    return i;
}

khm_size kcdb_buf_slot_by_id(kcdb_buf * buf, khm_ui_2 id)
{
    khm_size i;

    for(i=0; i < buf->n_fields; i++) {
        if(buf->fields[i].id == id)
            break;
    }

    if(i < buf->n_fields)
        return i;
    else
        return KCDB_BUF_INVALID_SLOT;
}

/* API for accessing generic buffers */

KHMEXP khm_int32 KHMAPI
kcdb_buf_get_attr(khm_handle  record, 
                  khm_int32   attr_id, 
                  khm_int32 * attr_type, 
                  void *      buffer, 
                  khm_size *  pcb_buf)
{
    if (record == NULL)
        return KHM_ERROR_INVALID_PARAM;

    switch (*((khm_int32 *) record)) {
    case KCDB_CRED_MAGIC:
        return kcdb_cred_get_attr(record, attr_id, attr_type, buffer, pcb_buf);

    case KCDB_IDENT_MAGIC:
        return kcdb_identity_get_attr(record, attr_id, attr_type, buffer, pcb_buf);

    default:
        return KHM_ERROR_INVALID_PARAM;
    }
}

KHMEXP khm_int32 KHMAPI
kcdb_buf_get_attrib(khm_handle  record,
                    const wchar_t *   attr_name,
                    khm_int32 * attr_type,
                    void *      buffer,
                    khm_size *  pcb_buf)
{
    if (record == NULL)
        return KHM_ERROR_INVALID_PARAM;

    switch (*((khm_int32 *) record)) {
    case KCDB_CRED_MAGIC:
        return kcdb_cred_get_attrib(record, attr_name, attr_type, buffer, pcb_buf);

    case KCDB_IDENT_MAGIC:
        return kcdb_identity_get_attrib(record, attr_name, attr_type, buffer, pcb_buf);

    default:
        return KHM_ERROR_INVALID_PARAM;
    }
}

KHMEXP khm_int32 KHMAPI
kcdb_buf_get_attr_string(khm_handle  record,
                         khm_int32   attr_id,
                         wchar_t *   buffer,
                         khm_size *  pcbbuf,
                         khm_int32  flags)
{
    if (record == NULL)
        return KHM_ERROR_INVALID_PARAM;

    switch (*((khm_int32 *) record)) {
    case KCDB_CRED_MAGIC:
        return kcdb_cred_get_attr_string(record, attr_id, buffer, pcbbuf, flags);

    case KCDB_IDENT_MAGIC:
        return kcdb_identity_get_attr_string(record, attr_id, buffer, pcbbuf, flags);

    default:
        return KHM_ERROR_INVALID_PARAM;
    }
}

KHMEXP khm_int32 KHMAPI
kcdb_buf_get_attrib_string(khm_handle  record,
                           const wchar_t *   attr_name,
                           wchar_t *   buffer,
                           khm_size *  pcbbuf,
                           khm_int32   flags)
{
    if (record == NULL)
        return KHM_ERROR_INVALID_PARAM;

    switch (*((khm_int32 *) record)) {
    case KCDB_CRED_MAGIC:
        return kcdb_cred_get_attrib_string(record, attr_name, buffer, pcbbuf, flags);

    case KCDB_IDENT_MAGIC:
        return kcdb_identity_get_attrib_string(record, attr_name, buffer, pcbbuf, flags);

    default:
        return KHM_ERROR_INVALID_PARAM;
    }
}

KHMEXP khm_int32 KHMAPI
kcdb_buf_set_attr(khm_handle  record,
                  khm_int32   attr_id,
                  const void *buffer,
                  khm_size    cbbuf)
{
    if (record == NULL)
        return KHM_ERROR_INVALID_PARAM;

    switch (*((khm_int32 *) record)) {
    case KCDB_CRED_MAGIC:
        return kcdb_cred_set_attr(record, attr_id, buffer, cbbuf);

    case KCDB_IDENT_MAGIC:
        return kcdb_identity_set_attr(record, attr_id, buffer, cbbuf);

    default:
        return KHM_ERROR_INVALID_PARAM;
    }
}

KHMEXP khm_int32 KHMAPI
kcdb_buf_set_attrib(khm_handle  record,
                    const wchar_t *   attr_name,
                    const void *      buffer,
                    khm_size    cbbuf)
{
    if (record == NULL)
        return KHM_ERROR_INVALID_PARAM;

    switch (*((khm_int32 *) record)) {
    case KCDB_CRED_MAGIC:
        return kcdb_cred_set_attrib(record, attr_name, buffer, cbbuf);

    case KCDB_IDENT_MAGIC:
        return kcdb_identity_set_attrib(record, attr_name, buffer, cbbuf);

    default:
        return KHM_ERROR_INVALID_PARAM;
    }
}

KHMEXP khm_int32 KHMAPI
kcdb_buf_hold(khm_handle  record)
{
    if (record == NULL)
        return KHM_ERROR_INVALID_PARAM;

    switch (*((khm_int32 *) record)) {
    case KCDB_CRED_MAGIC:
        return kcdb_cred_hold(record);

    case KCDB_IDENT_MAGIC:
        return kcdb_identity_hold(record);

    case KCDB_IDENTPRO_MAGIC:
        return kcdb_identpro_hold(record);

    default:
        return KHM_ERROR_INVALID_PARAM;
    }
}

KHMEXP khm_int32 KHMAPI kcdb_buf_release(khm_handle record)
{
    if (record == NULL)
        return KHM_ERROR_INVALID_PARAM;

    switch (*((khm_int32 *) record)) {
    case KCDB_CRED_MAGIC:
        return kcdb_cred_release(record);

    case KCDB_IDENT_MAGIC:
        return kcdb_identity_release(record);

    case KCDB_IDENTPRO_MAGIC:
        return kcdb_identpro_release(record);

    default:
        return KHM_ERROR_INVALID_PARAM;
    }
}

