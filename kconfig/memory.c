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
#include<assert.h>

/* \addtogroup kconf_st

   \page p_kconf_st Memory configuration stores

   Memory configuration stores are provideed for the convenience of
   implementing configuration stores that use various backends for
   storage.  During its lifetime, a memory configuration store uses
   in-memory buffers for storing values and child configuration
   spaces.  They can be constructed, enumerated and removed and
   implement the full suite of APIs necessary for a fully functional
   configuration provider.

   \page p_kconf_st_use Using a memory configuration store

   To create a memory configuration store:

   \code
   khm_handle h_mem;

   khc_memory_store_create(&h_mem);
   \endcode

   This creates an empty memory configuration handle.  It can't be
   mounted in its present state since it has no actual stores created
   within it.

   To create a new child configuration space:

   \code
   khc_memory_store_add(h_mem, KC_SPACE, L"Space", NULL, 0);
   \endcode

   This creates a new configuration space named "Space" under the
   memory configuration handle \a h_mem.

   To add data to a memory configuration handle:

   \code
   khm_int32 v = 10;

   khc_memory_store_add(h_mem, KC_INT32, L"ValueName", &v, sizeof(v));
   \endcode

   The above code adds a value named "ValueName" to the configuration
   space created in the previous step.  Note that you must always
   create a configuration space before you add data.

   Then you can close the configuration space with:

   \code
   khc_memory_store_add(h_mem, KC_ENDSPACE, NULL, NULL, 0);
   \endcode

   Now, you can mount the newly created configuration space:

   \code
   khc_memory_store_mount(h_space, KCONF_FLAG_USER, h_mem, &h_mounted);
   \endcode

   This mounts the memory store under the configuration space
   corresponding to \a h_space.  Assuming \a h_space was a handle to
   L"Foo\\Bar", mounting \a h_mem under \a h_space will result in the
   creation of L"Foo\\Bar\\Space" containing the configuration data
   added in the above steps.

   At this point, any operation that modifies the user store under
   L"Foo\\Bar\\Space" will modify the in-memory configuration store we
   created.

   Finally, you can unmount the store using:

   \code
   khc_memory_store_unmount(h_mounted, KCONF_FLAG_USER);
   \endcode

   At any time, you can enumerate the contents of the in-memory
   configuration store including any child configuration stores using:

   \code
   khc_memory_store_enum(h_mem, callback_func, &foo);
   \endcode

   This calls \a callback_func() for each entry found in the in-memory
   store.  For example, for the above data, the following series of
   function calls will be made:

   \code
   callback_func(KC_SPACE, L"Space", NULL, 0, &foo);
   callback_func(KC_INT32, L"ValueName", &value, sizeof(khm_int32), &foo);
   callback_func(KC_ENDSPACE, NULL, NULL, 0, &foo);
   \endcode

   Finally, the handle to the in-memory configuration store should be
   released via:

   \code
   khc_memory_store_release(h_mem);
   \endcode


   \page p_kconf_st_life Lifetime of a memory configuration store

 */

typedef struct p_value {
    wchar_t   *name;
    khm_int32  type;

    union {
        khm_int32  i32;
        khm_int64  i64;
        wchar_t *  str;
        struct {
            void * bin;
            int    cb_bin;
        };
    };
} p_value;

typedef struct p_store {
    khm_int32 magic;
#define P_STORE_MAGIC 0x3eb300a1

    wchar_t * name;

    p_value * values;
    int       n_values;
    int       nc_values;

    FILETIME ft_mtime;

    int refcount;
    /* Counts:

       +1 for each p_store_handle
          that points here.

       +1 for each mount
    */

    khm_handle h;
    khm_int32  store;

    khm_boolean removed;

    int open_count;

    const khc_memory_store_notify * to_notify;
    void * notify_ctx;

    struct p_store * root;

    TDCL(struct p_store);
} p_store;

/* forward declarations */
static void
hold_store(p_store * s);

static void
release_store(p_store * s);

#define NOTIFY(s, n) \
    do {                                                                \
        if (s->root && s->root->to_notify && s->root->to_notify->n)     \
            s->root->to_notify->n(s->root->notify_ctx, s);              \
    } while(FALSE)

void
free_value_contents(p_value * v, khm_boolean free_name)
{
    if (v->name && free_name) {
        PFREE(v->name);
        v->name = NULL;
    }

    switch (v->type) {
    case KC_STRING:
        if (v->str) {
            PFREE(v->str);
            v->str = NULL;
        }
        break;

    case KC_BINARY:
        if (v->bin) {
            PFREE(v->bin);
            v->bin = NULL;
        }
        break;
    }

    v->type = KC_NONE;
}

static p_store *
create_store(p_store * parent, const wchar_t * name)
{
    p_store * s;

    s = PMALLOC(sizeof(*s));
    memset(s, 0, sizeof(*s));

    s->magic = P_STORE_MAGIC;
    s->name = PWCSDUP(name);
    s->h = NULL;
    s->values = NULL;

    GetSystemTimeAsFileTime(&s->ft_mtime);

    TINIT(s);

    s->refcount = 0;

    if (parent) {
        TADDCHILD(parent, s);
        s->root = parent->root;
    } else {
        s->root = s;
    }

    return s;
}

static void
free_store(p_store * s)
{
    int i;
    p_store *c, *nc, *p;

    assert(s->h == NULL);
    assert(s->refcount == 0);

    if (s->name)
        PFREE(s->name);

    for (i=0; i < s->n_values; i++)
        free_value_contents(&s->values[i], TRUE);

    if (s->values)
        PFREE(s->values);

    for (c = TFIRSTCHILD(s); c ; c = nc) {
        nc = TNEXTSIBLING(c);

        free_store(c);
    }

    p = TPARENT(s);

    if (p) {
        TDELCHILD(p, s);
    }

    memset(s, 0, sizeof(*s));
    PFREE(s);
}

static void
hold_store(p_store * s)
{
    if (s && s->magic == P_STORE_MAGIC)
        s->refcount++;
}

static void
release_store(p_store * s)
{
    if (s && s->magic == P_STORE_MAGIC) {
        --(s->refcount);
        assert(s->refcount >= 0);
    }
}

static void
mark_remove_store(p_store * s)
{
    if (s && s->magic == P_STORE_MAGIC) {
        p_store * c;

        s->removed = TRUE;

        for (c = TFIRSTCHILD(s); c; c = TNEXTSIBLING(c)) {
            mark_remove_store(c);
        }
    }
}

static void
unmount_store(p_store * s)
{
    if (s && s->magic == P_STORE_MAGIC) {
        p_store * c;

        if (s->h)
            khc_unmount_provider(s->h, &khc_memory_provider, s->store);

        for (c = TFIRSTCHILD(s); c; c = TNEXTSIBLING(c)) {
            unmount_store(c);
        }
    }
}

khm_int32 KHMCALLBACK p_init(khm_handle sp_handle,
                             const wchar_t * path, khm_int32 flags,
                             void * context, void ** r_nodeHandle)
{
    p_store * s = (p_store *) context;

    assert(s && s->magic == P_STORE_MAGIC);

    if (s == NULL || s->magic != P_STORE_MAGIC)
        return KHM_ERROR_NOT_FOUND;

    assert(s->h == NULL);

    if (s->h != NULL)
        return KHM_ERROR_INVALID_PARAM;

    assert(wcsrchr(path, L'\\') && !wcscmp(wcsrchr(path, L'\\') + 1, s->name));

    s->open_count = 0;
    s->h = sp_handle;
    s->store = (flags & (KCONF_FLAG_USER|KCONF_FLAG_MACHINE|KCONF_FLAG_SCHEMA));
    hold_store(s);

    NOTIFY(s, notify_init);

    *r_nodeHandle = s;

    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK p_exit(void * nodeHandle)
{
    p_store * s = (p_store *) nodeHandle;

    assert(s && s->magic == P_STORE_MAGIC);

    NOTIFY(s, notify_exit);

    s->h = NULL;
    release_store(s);

    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK p_open(void * nodeHandle)
{
    p_store * s = (p_store *) nodeHandle;

    assert(s && s->magic == P_STORE_MAGIC);

    if (s->removed)
        return KHM_ERROR_NOT_FOUND;

    s->open_count++;

    NOTIFY(s, notify_open);

    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK p_close(void * nodeHandle)
{
    p_store * s = (p_store *) nodeHandle;

    assert(s && s->magic == P_STORE_MAGIC);

    s->open_count--;

    assert(s >= 0);

    NOTIFY(s, notify_close);

    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK p_remove(void * nodeHandle)
{
    p_store * s = (p_store *) nodeHandle;

    assert(s && s->magic == P_STORE_MAGIC);

    mark_remove_store(s);

    NOTIFY(s, notify_modify);

    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK p_create(void * nodeHandle, const wchar_t * name, khm_int32 flags)
{
    p_store * s = (p_store *) nodeHandle;
    p_store * c;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    assert(s && s->magic == P_STORE_MAGIC);

    if (s->removed)
        return KHM_ERROR_INVALID_OPERATION;

    for (c = TFIRSTCHILD(s); c; c = TNEXTSIBLING(c)) {
        if (!wcscmp(name, c->name)) {
            break;
        }
    }

    if (c == NULL && (flags & KHM_FLAG_CREATE)) {
        c = create_store(s, name);
        NOTIFY(s, notify_modify);
    }

    if (c)
        rv = khc_mount_provider(s->h, name, flags, &khc_memory_provider,
                                c, NULL);

    return rv;
}

khm_int32 KHMCALLBACK p_begin_enum(void * nodeHandle)
{
    p_store * s = (p_store *) nodeHandle;
    p_store * c;

    assert(s && s->magic == P_STORE_MAGIC);

    if (s->removed)
        return KHM_ERROR_INVALID_OPERATION;

    for (c = TFIRSTCHILD(s); c; c = TNEXTSIBLING(c)) {
        khm_handle h = NULL;

        if (c->removed)
            continue;

        khc_open_space(s->h, c->name, s->store, &h);
        if (h)
            khc_close_space(h);
    }

    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK p_get_mtime(void * nodeHandle, FILETIME * ft_mtime)
{
    p_store * s = (p_store *) nodeHandle;

    assert(s && s->magic == P_STORE_MAGIC);

    if (s->removed)
        return KHM_ERROR_INVALID_OPERATION;

    *ft_mtime = s->ft_mtime;

    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMCALLBACK p_read_value(void * nodeHandle, const wchar_t * valuename,
                                   khm_int32 * vtype, void * buffer, khm_size * pcb)
{
    int i;
    khm_int32 rv = KHM_ERROR_SUCCESS;
    p_value * v = NULL;
    p_store * s = (p_store *) nodeHandle;

    assert(s && s->magic == P_STORE_MAGIC);

    if (s->removed)
        return KHM_ERROR_NOT_FOUND;

    for (i=0; i < s->n_values; i++) {
        if (!wcscmp(valuename, s->values[i].name)) {
            v = &s->values[i];
            break;
        }
    }

    if (v == NULL) {
        rv = KHM_ERROR_NOT_FOUND;
    } else if (*vtype != KC_NONE && *vtype != v->type) {
        rv = KHM_ERROR_TYPE_MISMATCH;
    } else if (buffer == NULL && pcb == NULL) {
        rv = KHM_ERROR_SUCCESS;
    } else {
        khm_size cb = 0;
        void * src = NULL;

        switch (v->type) {
        case KC_INT32:
            src = &v->i32;
            cb = sizeof(khm_int32);
            break;

        case KC_INT64:
            src = &v->i64;
            cb = sizeof(khm_int64);
            break;

        case KC_STRING:
            src = v->str;
            StringCbLength(v->str, KCONF_MAXCCH_STRING, &cb);
            cb += sizeof(wchar_t);
            break;

        case KC_BINARY:
            src = v->bin;
            cb = v->cb_bin;
            break;

        default:
            assert(FALSE);
        }

        assert(src != NULL && cb != 0);

        if (buffer == NULL || *pcb < cb) {
            *pcb = cb;
            rv = KHM_ERROR_TOO_LONG;
        } else {
            *pcb = cb;
            memcpy(buffer, src, cb);
            rv = KHM_ERROR_SUCCESS;
        }
    }

    return rv;
}


khm_int32 KHMCALLBACK p_write_value(void * nodeHandle, const wchar_t * valuename,
                                    khm_int32 vtype, const void * buffer, khm_size cb)
{
    int i;
    p_value * v = NULL;
    p_store * s = (p_store *) nodeHandle;

    assert(s && s->magic == P_STORE_MAGIC);

    if (s->removed)
        return KHM_ERROR_INVALID_OPERATION;

    if (vtype == KC_MTIME) {
        assert(buffer != NULL && cb == sizeof(FILETIME));
        s->ft_mtime = *((FILETIME *) buffer);
        return KHM_ERROR_SUCCESS;
    }

    for (i=0; i < s->n_values; i++) {
        if (!wcscmp(valuename, s->values[i].name)) {
            free_value_contents(&s->values[i], FALSE);
            v = &s->values[i];
            break;
        }
    }

    if (v == NULL) {
        if (s->n_values + 1 > s->nc_values) {
            s->nc_values = UBOUNDSS(s->n_values + 1, 8, 8);
            v = PREALLOC(s->values, sizeof(s->values[0]) * s->nc_values);
            if (v == NULL)
                return KHM_ERROR_NO_RESOURCES;
            s->values = v;
        }
        v = &s->values[s->n_values++];
        memset(v, 0, sizeof(*v));
        v->name = NULL;
    }

    if (!v->name)
        v->name = wcsdup(valuename);
    v->type = vtype;

    switch (v->type) {
    case KC_INT32:
        v->i32 = *((khm_int32 *)buffer);
        break;

    case KC_INT64:
        v->i64 = *((khm_int64 *)buffer);
        break;

    case KC_STRING:
        v->str = wcsdup((wchar_t *) buffer);
        break;

    case KC_BINARY:
        v->bin = PMALLOC(cb);
        v->cb_bin = cb;
        memcpy(v->bin, buffer, cb);
        break;

    default:
        assert(FALSE);
    }

    if (s->h)
        GetSystemTimeAsFileTime(&s->ft_mtime);

    NOTIFY(s, notify_modify);

    return KHM_ERROR_SUCCESS;
}


khm_int32 KHMCALLBACK p_remove_value(void * nodeHandle, const wchar_t * valuename)
{
    int i;
    khm_int32 rv = KHM_ERROR_SUCCESS;
    p_value * v = NULL;
    p_store * s = (p_store *) nodeHandle;

    assert(s && s->magic == P_STORE_MAGIC);

    for (i=0; i < s->n_values; i++) {
        if (!wcscmp(valuename, s->values[i].name)) {
            v = &s->values[i];
            break;
        }
    }

    if (v) {
        free_value_contents(v, TRUE);
        if (v - s->values < s->n_values - 1) {
            memmove(v, v + 1, ((s->n_values - 1) - (v - s->values)) * sizeof(*v));
        }
        s->n_values--;

        NOTIFY(s, notify_modify);

        return KHM_ERROR_SUCCESS;
    }
    return KHM_ERROR_NOT_FOUND;
}

const khc_provider_interface khc_memory_provider = {
    KHC_PROVIDER_V1,
    p_init,
    p_exit,

    p_open,
    p_close,
    p_remove,

    p_create,
    p_begin_enum,
    p_get_mtime,

    p_read_value,
    p_write_value,
    p_remove_value
};

/* Store handles and the external API */

typedef struct p_store_handle {
    khm_int32 magic;
#define P_STORE_HANDLE_MAGIC 0x4890a991

    int refcount;

    p_store *root;
    p_store *cursor;

    const khc_provider_interface *provider;
} p_store_handle;

static p_store_handle *
create_store_handle(void)
{
    p_store_handle * h;

    h = PMALLOC(sizeof(*h));
    memset(h, 0, sizeof(h));

    h->magic = P_STORE_HANDLE_MAGIC;
    h->refcount = 1;
    h->root = NULL;
    h->cursor = NULL;
    h->provider = &khc_memory_provider;

    return h;
}

static void
free_store_handle(p_store_handle * h)
{
    assert(h && h->magic == P_STORE_HANDLE_MAGIC);

    if (h->root) {
        unmount_store(h->root);

        release_store(h->root);
        release_store(h->cursor);

        free_store(h->root);
    }

    memset(h, 0, sizeof(*h));
    PFREE(h);
}

static void
hold_store_handle(p_store_handle * h)
{
    assert(h && h->magic == P_STORE_HANDLE_MAGIC);

    if (h && h->magic == P_STORE_HANDLE_MAGIC) {
        h->refcount++;
    }
}

static void
release_store_handle(p_store_handle * h)
{
    assert(h && h->magic == P_STORE_HANDLE_MAGIC);

    if (h && h->magic == P_STORE_HANDLE_MAGIC) {
        h->refcount--;
        assert(h->refcount >= 0);

        if (h->refcount == 0) {
            free_store_handle(h);
        }
    }
}

KHMEXP khm_int32 KHMAPI
khc_memory_store_create(khm_handle * ret_sp)
{
    *ret_sp = create_store_handle();

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
khc_memory_store_add(khm_handle sp, const wchar_t * name, khm_int32 type,
                     const void * data, khm_size cb)
{
    p_store_handle * h = (p_store_handle *) sp;
    size_t cch;

    if (h == NULL || h->magic != P_STORE_HANDLE_MAGIC ||
        (name != NULL && FAILED(StringCchLength(name, KCONF_MAXCCH_NAME, &cch))))
        return KHM_ERROR_INVALID_PARAM;

    if (type == KC_SPACE) {
        p_store * s;

        if (name == NULL)
            return KHM_ERROR_INVALID_PARAM;

        s = create_store(h->cursor, name);
        release_store(h->cursor);
        h->cursor = s;
        hold_store(h->cursor);
        if (h->root == NULL) {
            h->root = s;
            hold_store(h->root);
        }
        return KHM_ERROR_SUCCESS;

    } else if (type == KC_ENDSPACE) {

        if (h->cursor == NULL)
            return KHM_ERROR_INVALID_OPERATION;

        release_store(h->cursor);
        h->cursor = TPARENT(h->cursor);
        hold_store(h->cursor);
        return KHM_ERROR_SUCCESS;

    } else {

        if (h->cursor == NULL)
            return KHM_ERROR_INVALID_OPERATION;

        if (name == NULL && type != KC_MTIME)
            return KHM_ERROR_INVALID_PARAM;

        return p_write_value(h->cursor, name, type, data, cb);
    }
}

static void
enum_this_store(p_store * s, khc_store_enum_cb cb, void * ctx)
{
    int i;
    p_store * c;

    (*cb)(KC_SPACE, s->name, NULL, 0, ctx);

    for (i = 0; i < s->n_values; i++) {
        void * data = NULL;
        khm_size cb_data = 0;

        switch (s->values[i].type) {
        case KC_INT32:
            data = &s->values[i].i32;
            cb_data = sizeof(khm_int32);
            break;

        case KC_INT64:
            data = &s->values[i].i64;
            cb_data = sizeof(khm_int64);
            break;

        case KC_STRING:
            data = s->values[i].str;
            StringCbLength(s->values[i].str, KCONF_MAXCB_STRING, &cb_data);
            cb_data += sizeof(wchar_t);
            break;

        case KC_BINARY:
            data = s->values[i].bin;
            cb_data = s->values[i].cb_bin;
            break;

        default:
            assert(FALSE);
        }

        (*cb)(s->values[i].type,
              s->values[i].name,
              data, cb_data, ctx);
    }

    (*cb)(KC_MTIME, NULL, &s->ft_mtime, sizeof(s->ft_mtime), ctx);

    for (c = TFIRSTCHILD(s); c; c = TNEXTSIBLING(c)) {
        if (c->removed)
            continue;

        enum_this_store(c, cb, ctx);
    }

    (*cb)(KC_ENDSPACE, NULL, NULL, 0, ctx);
}

KHMEXP khm_int32 KHMAPI
khc_memory_store_enum(khm_handle sp, khc_store_enum_cb cb, void * ctx)
{
    p_store_handle * h = (p_store_handle *) sp;

    if (h == NULL || h->magic != P_STORE_HANDLE_MAGIC)
        return KHM_ERROR_INVALID_PARAM;

    if (h->root == NULL)
        return KHM_ERROR_INVALID_OPERATION;

    enum_this_store(h->root, cb, ctx);

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
khc_memory_store_hold(khm_handle sp)
{
    hold_store_handle(sp);
    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
khc_memory_store_release(khm_handle sp)
{
    release_store_handle(sp);
    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
khc_memory_store_set_notify_interface(khm_handle sp,
                                      const khc_memory_store_notify * pnotify,
                                      void * ctx)
{
    p_store_handle * h = (p_store_handle *) sp;

    if (h == NULL || h->magic != P_STORE_HANDLE_MAGIC ||
        pnotify == NULL || h->root == NULL)
        return KHM_ERROR_INVALID_PARAM;

    h->root->to_notify = pnotify;
    h->root->notify_ctx = ctx;

    return KHM_ERROR_SUCCESS;
}

KHMEXP khm_int32 KHMAPI
khc_memory_store_mount(khm_handle csp_parent, khm_int32 store, khm_handle sp, khm_handle *ret)
{
    p_store_handle * h = (p_store_handle *) sp;

    if (h == NULL || h->magic != P_STORE_HANDLE_MAGIC ||
        (store & (KCONF_FLAG_USER|KCONF_FLAG_MACHINE)) == 0 ||
        (store & ~(KCONF_FLAG_USER|KCONF_FLAG_MACHINE)) != 0 ||
        !IS_POW2(store & (KCONF_FLAG_USER|KCONF_FLAG_MACHINE)))
        return KHM_ERROR_INVALID_PARAM;

    if (h->root == NULL ||
        h->root->h != NULL)
        return KHM_ERROR_INVALID_OPERATION;

    return khc_mount_provider(csp_parent, h->root->name, store, &khc_memory_provider,
                              h->root, ret);
}

KHMEXP khm_int32 KHMAPI
khc_memory_store_unmount(khm_handle sp)
{
    p_store_handle * h = (p_store_handle *) sp;

    if (h == NULL || h->magic != P_STORE_HANDLE_MAGIC)
        return KHM_ERROR_INVALID_PARAM;

    if (h->root == NULL ||
        h->root->h == NULL)
        return KHM_ERROR_INVALID_OPERATION;

    unmount_store(h->root);

    return KHM_ERROR_SUCCESS;
}

