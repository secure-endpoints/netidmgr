/*
 * Copyright (c) 2009 Secure Endpoints Inc.
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

#include "kconfiginternal.h"
#include <assert.h>

typedef struct schema_node {
    khm_int32           magic;
    const kconf_schema *schema;
    int                 nSchema;
    khm_handle          h;
    FILETIME            ft_mount;
} schema_node;

#define SCHEMA_MAGIC 0x44f62b14
#define is_schema_node(n) ((n) && ((schema_node *) (n))->magic == SCHEMA_MAGIC)

typedef struct schema_mount_parameters {
    const kconf_schema *schema;
    int                *end;
} schema_mount_parameters;

/* no locks */
khm_int32 
validate_schema(const kconf_schema *schema,
                int                 begin,
                int                *end)
{
    int i;
    enum { INITIAL, VALUES, SUBSPACES } state = INITIAL;
    int end_found = 0;

    i = begin;
    while (!end_found) {
        switch (state) {
        case INITIAL: /* initial.  this record should start a config space */
            if(!khcint_is_valid_name(schema[i].name) ||
               schema[i].type != KC_SPACE)
                return KHM_ERROR_INVALID_PARAM;
            state = VALUES;
            break;

        case VALUES: /* we are inside a config space, in the values area */
            if (!khcint_is_valid_name(schema[i].name))
                return KHM_ERROR_INVALID_PARAM;
            if (schema[i].type == KC_SPACE) {
                if(KHM_FAILED(validate_schema(schema, i, &i)))
                    return KHM_ERROR_INVALID_PARAM;
                state = SUBSPACES;
            } else if (schema[i].type == KC_ENDSPACE) {
                end_found = 1;
                if (end)
                    *end = i;
            } else {
                if (schema[i].type != KC_STRING &&
                    schema[i].type != KC_INT32 &&
                    schema[i].type != KC_INT64 &&
                    schema[i].type != KC_BINARY)
                    return KHM_ERROR_INVALID_PARAM;
            }
            break;

        case SUBSPACES: /* we are inside a config space, in the subspace area */
            if (schema[i].type == KC_SPACE) {
                if (KHM_FAILED(validate_schema(schema, i, &i)))
                    return KHM_ERROR_INVALID_PARAM;
            } else if (schema[i].type == KC_ENDSPACE) {
                end_found = 1;
                if (end)
                    *end = i;
            } else {
                return KHM_ERROR_INVALID_PARAM;
            }
            break;
            
        default:
            /* unreachable */
            return KHM_ERROR_INVALID_PARAM;
        }
        i++;
    }

    return KHM_ERROR_SUCCESS;
}

const kconf_schema *
unload_schema(khm_handle parent, const kconf_schema * schema)
{
    enum { INITIAL, VALUES, SPACES } state = INITIAL;
    int end_found = 0;
    khm_handle h = NULL;

    while(schema) {
        switch(state) {
        case INITIAL: /* initial.  this record should start a config space */
            assert(h == NULL);
            assert(schema->type == KC_SPACE);
            if(KHM_FAILED(khc_open_space(parent, schema->name, KCONF_FLAG_SCHEMA, &h))) {
                return NULL;
            }
            khc_unmount_provider(h, &khc_schema_provider, KCONF_FLAG_SCHEMA);
            state = VALUES;
            break;

        case VALUES: /* we are inside a config space, in the values area */
            if(schema->type == KC_SPACE) {

                schema = unload_schema(h, schema);
                state = SPACES;

            } else if(schema->type == KC_ENDSPACE) {

                khc_close_space(h);
                return schema;

            } else {
                assert(schema->type == KC_INT32 ||
                       schema->type == KC_INT64 ||
                       schema->type == KC_STRING ||
                       schema->type == KC_BINARY);
            }
            break;

        case SPACES: /* we are inside a config space, in the subspace area */
            if(schema->type == KC_SPACE) {

                schema = unload_schema(h, schema);

            } else if(schema->type == KC_ENDSPACE) {

                khc_close_space(h);
                return schema;

            } else {

                khc_close_space(h);
                return NULL;

            }
            break;

        default:
            /* unreachable */
            return NULL;
        }
        schema++;
    }

    khc_close_space(h);
    return NULL;
}

khm_int32
load_schema(schema_node * node, const kconf_schema * schema, int * end)
{
    int i;
    enum {INITIAL, VALUES, SUBSPACES} state = INITIAL;
    int end_found = 0;
    kconf_conf_space * thisconf = NULL;
    khm_handle h = NULL;

    assert(is_schema_node(node));

    i=0;
    while(!end_found) {
        switch(state) {
        case INITIAL: /* initial.  this record should start a config space */
            node->schema = schema + 1;
            node->nSchema = 0;
            state = VALUES;
            break;

        case VALUES: /* we are inside a config space, in the values area */
            if (schema[i].type == KC_SPACE) {
                int n = 0;

                node->nSchema = i - 1;

                if (KHM_FAILED(khcint_load_schema(node->h, schema + i, &n)))
                    return KHM_ERROR_INVALID_PARAM;
                i += n;
                state = SUBSPACES;

            } else if(schema[i].type == KC_ENDSPACE) {

                node->nSchema = i - 1;
                end_found = 1;
                if(end)
                    *end = i;
            }
            break;

        case SUBSPACES: /* we are inside a config space, in the subspace area */
            if(schema[i].type == KC_SPACE) {
                int n = 0;

                if(KHM_FAILED(khcint_load_schema(node->h, schema + i, &n)))
                    return KHM_ERROR_INVALID_PARAM;
                i += n;
            } else if(schema[i].type == KC_ENDSPACE) {
                end_found = 1;
                if(end)
                    *end = i;
                khc_close_space(h);
            } else {
                return KHM_ERROR_INVALID_PARAM;
            }
            break;

        default:
            /* unreachable */
            return KHM_ERROR_INVALID_PARAM;
        }
        i++;
    }

    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK
schema_init(khm_handle sp_handle,
            const wchar_t * path, khm_int32 flags,
            void * context, void ** r_nodeHandle)
{
    khm_int32 rv;
    schema_node * node;
    schema_mount_parameters * p = (schema_mount_parameters *) context;

    if ((flags & (KCONF_FLAG_SCHEMA|KHM_FLAG_CREATE)) != (KCONF_FLAG_SCHEMA|KHM_FLAG_CREATE))
        return KHM_ERROR_NOT_FOUND;

    node = PMALLOC(sizeof(*node));
    memset(node, 0, sizeof(*node));

    node->magic = SCHEMA_MAGIC;
    node->h = sp_handle;
    GetSystemTimeAsFileTime(&node->ft_mount);

    rv = load_schema(node, p->schema, p->end);

    if (KHM_FAILED(rv)) {
        unload_schema(node, p->schema);
        PFREE(node);
    } else {
        *r_nodeHandle = node;
    }
    return rv;
}

khm_int32 KHMCALLBACK
schema_exit(void * nodeHandle)
{
    schema_node * node = (schema_node *) nodeHandle;

    assert(is_schema_node(node));
    PFREE(node);

    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK
schema_open(void * nodeHandle)
{
    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK
schema_close(void * nodeHandle)
{
    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK
schema_remove(void * nodeHandle)
{
    return KHM_ERROR_INVALID_OPERATION;
}

khm_int32 KHMCALLBACK
schema_create(void * nodeHandle, const wchar_t * name, khm_int32 flags)
{
    return KHM_ERROR_INVALID_OPERATION;
}

khm_int32 KHMCALLBACK
schema_begin_enum(void * nodeHandle)
{
    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK
schema_get_mtime(void * nodeHandle, FILETIME * mtime)
{
    schema_node * node = (schema_node *) nodeHandle;
    assert(is_schema_node(node));
    *mtime = node->ft_mount;
    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK
schema_read_value(void * nodeHandle, const wchar_t * valuename,
                  khm_int32 * pvalue_type, void * buffer, khm_size * pcb_buffer)
{
    schema_node * node = (schema_node *) nodeHandle;
    int i;

    assert(is_schema_node(node));

    for (i = 0; i < node->nSchema; i++) {

        if (!wcscmp(valuename, node->schema[i].name)) {

            khm_int32 i32 = 0;
            khm_int64 i64 = 0;
            size_t cbsize = 0;
            void * src = NULL;
            const kconf_schema * s = &node->schema[i];

            if (node->schema[i].type != *pvalue_type &&
                *pvalue_type != KC_NONE)
                return KHM_ERROR_TYPE_MISMATCH;

            *pvalue_type = s->type;

            switch (s->type) {
            case KC_INT32:
                cbsize = sizeof(khm_int32);
                i32 = (khm_int32) s->value;
                src = &i32;
                break;

            case KC_INT64:
                cbsize = sizeof(khm_int64);
                i64 = (khm_int64) s->value;
                src = &i64;
                break;
                
            case KC_STRING:
                if(FAILED(StringCbLength((wchar_t *) s->value, KCONF_MAXCB_STRING, &cbsize))) {
                    break;
                }
                cbsize += sizeof(wchar_t);
                src = (wchar_t *) s->value;
                break;
            }

            if (src == NULL)
                return KHM_ERROR_NOT_FOUND;

            if (buffer == NULL || *pcb_buffer < cbsize) {
                *pcb_buffer = cbsize;
                return KHM_ERROR_TOO_LONG;
            }

            memcpy(buffer, src, cbsize);
            *pcb_buffer = cbsize;
            return KHM_ERROR_SUCCESS;
        }
    }

    return KHM_ERROR_NOT_FOUND;
}

khm_int32 KHMCALLBACK
schema_write_value(void * nodeHandle, const wchar_t * valuename,
                   khm_int32 vtype, const void * buffer, khm_size cb)
{
    return KHM_ERROR_READONLY;
}

khm_int32 KHMCALLBACK
schema_remove_value(void * nodeHandle, const wchar_t * valuename)
{
    return KHM_ERROR_READONLY;
}

khm_int32 
khcint_unload_schema(khm_handle parent, const kconf_schema * schema)
{
    khm_int32 rv;
    int end = 0;

    rv = validate_schema(schema, 0, &end);
    if (KHM_FAILED(rv))
        return rv;

    schema = unload_schema(parent, schema);

    return (schema)? KHM_ERROR_SUCCESS : KHM_ERROR_INVALID_PARAM;
}

khm_int32
khcint_load_schema(khm_handle parent, const kconf_schema * schema, int * p_end)
{
    khm_int32 rv;
    int end;
    schema_mount_parameters p;

    rv = validate_schema(schema, 0, &end);
    if (KHM_FAILED(rv))
        return rv;

    p.schema = schema;
    p.end = &end;

    rv = khc_mount_provider(parent, schema->name, KCONF_FLAG_SCHEMA|KHM_FLAG_CREATE,
                            &khc_schema_provider, &p, NULL);

    if (p_end)
        *p_end = end;
    return rv;
}

const khc_provider_interface khc_schema_provider = {
    KHC_PROVIDER_V1,
    schema_init,
    schema_exit,

    schema_open,
    schema_close,
    schema_remove,
    schema_create,
    schema_begin_enum,
    schema_get_mtime,

    schema_read_value,
    schema_write_value,
    schema_remove_value
};
