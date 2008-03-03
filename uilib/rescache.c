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

#define NIMPRIVATE
#define _NIMLIB_

#include<khuidefs.h>
#include<utils.h>
#include<assert.h>
#include<strsafe.h>

CRITICAL_SECTION cs_res;

typedef struct tag_cached_resource {
    /* Keys { */
    khm_handle       owner;
    khm_restype      type;
    khm_int32        id;
    /* } Keys */

    union {
        const wchar_t * str;
        HANDLE       h_gdiobj;
    };
    khm_size         cb_buf;

    LDCL(struct tag_cached_resource);
} cached_resource;

static cached_resource * cached_resources;
static cached_resource * discarded_resources;
static hashtable * ht_resources;

static khm_int32
hash_resource(const void * k)
{
    const cached_resource * r = (const cached_resource *) k;
    size_t s;

    s = (size_t) r->owner;
    return (khm_int32) (((s >> 3) + r->id) << 2) + r->type;
}

static khm_int32
comp_resource(const void * k1, const void * k2)
{
    const cached_resource * r1 = (const cached_resource *) k1;
    const cached_resource * r2 = (const cached_resource *) k2;

    return
        (r1->owner != r2->owner)? ((r1->owner < r2->owner)? -1: 1):
        ((r1->type != r2->type)? r1->type - r2->type: r1->id - r2->id);
}

static void
del_ref_resource(const void * k, void * d)
{
    cached_resource * r = (cached_resource *) d;

    LDELETE(&cached_resources, r);

    switch (r->type) {
    case KHM_RESTYPE_STRING:
        PFREE(r);
        break;

        /* For icons and bitmaps, we can't immediately delete the GDI
           object because the UI might still be using the handle.
           Therefore, we move the resource to the discarded_resources
           list where it will be disposed of later. */

    case KHM_RESTYPE_ICON:
        LPUSH(&discarded_resources, r);
        break;

    case KHM_RESTYPE_FONT:
    case KHM_RESTYPE_BITMAP:
        LPUSH(&discarded_resources, r);
        break;

    default:
#ifdef DEBUG
        assert(FALSE);
#endif
    }
}

KHMEXP void KHMAPI 
khui_init_rescache(void) {
    InitializeCriticalSection(&cs_res);
    cached_resources = NULL;
    discarded_resources = NULL;
    ht_resources = hash_new_hashtable(127, hash_resource, comp_resource,
                                      NULL, del_ref_resource);
}

KHMEXP void KHMAPI 
khui_exit_rescache(void) {
    EnterCriticalSection(&cs_res);

    hash_del_hashtable(ht_resources);

#ifdef DEBUG
    assert(cached_resources == NULL);
#endif

    while (discarded_resources) {
        cached_resource *r;

        LPOP(&discarded_resources, &r);

        switch (r->type) {
        case KHM_RESTYPE_ICON:
            DestroyIcon(r->h_gdiobj);
            break;

        case KHM_RESTYPE_FONT:
        case KHM_RESTYPE_BITMAP:
            DeleteObject(r->h_gdiobj);
            break;

        default:
#ifdef DEBUG
            assert(FALSE);
#endif
        }

        PFREE(r);
    }

    LeaveCriticalSection(&cs_res);
    DeleteCriticalSection(&cs_res);
}

/* Owner is one of the following:

   - Credential type identifier
   - Identity handle
   - Identity provider handle
   - HMODULE
   - Pointer to an object in the process address space
 */
KHMEXP khm_int32 KHMAPI
khui_cache_add_resource(khm_handle owner, khm_int32 id,
                        khm_restype type,
                        const void * buf, khm_size cb_buf)
{
    cached_resource * r;
    khm_int32 rv = KHM_ERROR_SUCCESS;

    EnterCriticalSection(&cs_res);
    switch (type) {
    case KHM_RESTYPE_STRING:
        {
            const wchar_t * s;
            size_t cb_s, cb_req;
            wchar_t * d;

            s = (const wchar_t *) buf;

            if (FAILED(StringCbLength(s, KCDB_MAXCB_LONG_DESC, &cb_s))) {
                rv = KHM_ERROR_INVALID_PARAM;
                break;
            }

            cb_s += sizeof(wchar_t);
            cb_req = cb_s + sizeof(cached_resource);

            r = PMALLOC(cb_req);

            r->owner = owner;
            r->id = id;
            r->type = type;

            d = (wchar_t *) &r[1];
            r->str = d;
            r->cb_buf = cb_s;
            LINIT(r);

            StringCbCopy(d, cb_s, s);

            LPUSH(&cached_resources, r);

            hash_add(ht_resources, r, r);
        }
        break;

    case KHM_RESTYPE_FONT:
    case KHM_RESTYPE_ICON:
    case KHM_RESTYPE_BITMAP:
        {
            HANDLE hres;

            if (buf == NULL || cb_buf != sizeof(HANDLE)) {
                rv = KHM_ERROR_INVALID_PARAM;
                break;
            }

            hres = *(HANDLE *)buf;

            r = PMALLOC(sizeof(*r));

            r->owner = owner;
            r->id = id;
            r->type = type;

            r->h_gdiobj = hres;
            r->cb_buf = sizeof(hres);
            LINIT(r);

            LPUSH(&cached_resources, r);

            hash_add(ht_resources, r, r);
        }
        break;

    default:
#ifdef DEBUG
        assert(FALSE);
#endif
        rv = KHM_ERROR_INVALID_PARAM;
    }
    LeaveCriticalSection(&cs_res);

    return rv;
}

KHMEXP khm_int32 KHMAPI
khui_cache_get_resource(khm_handle owner, khm_int32 id, khm_restype type,
                        void * buf, khm_size *pcb_buf)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;
    cached_resource k;
    cached_resource * r;

    EnterCriticalSection(&cs_res);

    ZeroMemory(&k, sizeof(k));
    k.owner = owner;
    k.id = id;
    k.type = type;

    r = (cached_resource *) hash_lookup(ht_resources, &k);

    if (r == NULL) {
        rv = KHM_ERROR_NOT_FOUND;
        goto _exit;
    }

    switch (type) {
    case KHM_RESTYPE_STRING:
        if (buf != NULL && *pcb_buf >= r->cb_buf) {
            StringCbCopy((wchar_t *) buf, *pcb_buf, r->str);
            *pcb_buf = r->cb_buf;
        } else {
            rv = KHM_ERROR_TOO_LONG;
            *pcb_buf = r->cb_buf;
        }
        break;

    case KHM_RESTYPE_FONT:
    case KHM_RESTYPE_ICON:
    case KHM_RESTYPE_BITMAP:
        if (buf != NULL && *pcb_buf >= sizeof(HANDLE)) {
            HANDLE * ph = (HANDLE *) buf;

            *ph = r->h_gdiobj;
            *pcb_buf = sizeof(HANDLE);
        } else {
            rv = KHM_ERROR_TOO_LONG;
            *pcb_buf = sizeof(HANDLE);
        }
        break;

    default:
        rv = KHM_ERROR_UNKNOWN;
#ifdef DEBUG
        assert(FALSE);
#endif
    }

 _exit:
    LeaveCriticalSection(&cs_res);

    return rv;
}

KHMEXP khm_int32 KHMAPI
khui_cache_del_resource(khm_handle owner, khm_int32 id, khm_restype type)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;
    cached_resource k;
    cached_resource * r;

    EnterCriticalSection(&cs_res);

    ZeroMemory(&k, sizeof(k));
    k.owner = owner;
    k.id = id;
    k.type = type;

    r = hash_lookup(ht_resources, &k);

    if (r == NULL) {
        rv = KHM_ERROR_NOT_FOUND;
        goto _exit;
    }

    LDELETE(&cached_resources, r);
    hash_del(ht_resources, r);

 _exit:
    LeaveCriticalSection(&cs_res);

    return rv;
}

KHMEXP khm_int32 KHMAPI
khui_cache_del_by_owner(khm_handle owner)
{
    cached_resource * r;
    cached_resource * rn;
    int n_removed = 0;

    EnterCriticalSection(&cs_res);
    for (r = cached_resources; r; r = rn) {
        rn = LNEXT(r);

        if (r->owner == owner) {
            hash_del(ht_resources, r);
            n_removed++;
        }
    }
    LeaveCriticalSection(&cs_res);

    return (n_removed > 0)? KHM_ERROR_SUCCESS : KHM_ERROR_NOT_FOUND;
}

KHMEXP khui_ilist * KHMAPI 
khui_create_ilist(int cx, int cy, int n, int ng, int opt) {
    BITMAPV5HEADER head;
    HDC hdc;

    khui_ilist * il = PMALLOC(sizeof(khui_ilist));
    il->cx = cx;
    il->cy = cy;
    il->n = n;
    il->ng = ng;
    il->nused = 0;
    hdc = GetDC(NULL);
    head.bV5Size = sizeof(head);
    head.bV5Width = cx * n;
    head.bV5Height = cy;
    head.bV5Planes = 1;
    head.bV5BitCount = 24;
    head.bV5Compression = BI_RGB;
    head.bV5SizeImage = 0;
    head.bV5XPelsPerMeter = 2835;
    head.bV5YPelsPerMeter = 2835;
    head.bV5ClrUsed = 0;
    head.bV5ClrImportant = 0;
    head.bV5AlphaMask = 0;
    head.bV5CSType = LCS_WINDOWS_COLOR_SPACE;
    head.bV5Intent = LCS_GM_GRAPHICS;
    head.bV5ProfileData = 0;
    head.bV5ProfileSize = 0;
    head.bV5Reserved = 0;
    il->img = CreateDIBitmap(hdc, (BITMAPINFOHEADER *) &head, 0, NULL, NULL, DIB_RGB_COLORS);
    il->mask = CreateBitmap(cx * n, cy, 1, 1, NULL);
    il->idlist = PMALLOC(sizeof(int) * n);

    return il;
}

KHMEXP BOOL KHMAPI 
khui_delete_ilist(khui_ilist * il) {
    DeleteObject(il->img);
    DeleteObject(il->mask);
    PFREE(il->idlist);
    PFREE(il);

    return TRUE;
}

KHMEXP int KHMAPI 
khui_ilist_add_masked_id(khui_ilist *il, 
                         HBITMAP hbm, 
                         COLORREF cbkg, 
                         int id) {
    int idx;

    idx = khui_ilist_add_masked(il,hbm,cbkg);
    if(idx >= 0) {
        il->idlist[idx] = id;
    }

    return idx;
}

KHMEXP int KHMAPI 
khui_ilist_lookup_id(khui_ilist *il, int id) {
    int i;

    for(i=0;i<il->nused;i++) {
        if(il->idlist[i] == id)
            return i;
    }

    return -1;
}

KHMEXP int KHMAPI 
khui_ilist_add_masked(khui_ilist * il, HBITMAP hbm, COLORREF cbkg) {
    HDC dcr,dci,dct,dcb;
    HBITMAP hb_oldb, hb_oldi, hb_oldt;
    int sx, i;
    int x,y;

    dcr = GetDC(NULL);
    dci = CreateCompatibleDC(dcr);
    dct = CreateCompatibleDC(dcr);
    dcb = CreateCompatibleDC(dcr);
    ReleaseDC(NULL,dcr);

    i = il->nused++;
    il->idlist[i] = -1;
    sx = i * il->cx;

    hb_oldb = SelectObject(dcb, hbm);
    hb_oldi = SelectObject(dci, il->img);
    hb_oldt = SelectObject(dct, il->mask);

    SetBkColor(dct, RGB(0,0,0));
    SetTextColor(dct, RGB(255,255,255));

    BitBlt(dci, sx, 0, il->cx, il->cy, dcb, 0, 0, SRCCOPY);
    for(y=0;y < il->cy; y++)
        for(x=0; x<il->cx; x++) {
            COLORREF c = GetPixel(dcb, x, y);
            if(c==cbkg) {
                SetPixel(dct, sx + x, y, RGB(255,255,255));
                SetPixel(dci, sx + x, y, RGB(0,0,0));
            } else {
                SetPixel(dct, sx + x, y, RGB(0,0,0));
            }
        }

    SelectObject(dct, hb_oldt);
    SelectObject(dci, hb_oldi);
    SelectObject(dcb, hb_oldb);

    DeleteDC(dcb);
    DeleteDC(dct);
    DeleteDC(dci);

    return i;
}

KHMEXP void KHMAPI 
khui_ilist_draw(khui_ilist * il, 
                int idx, 
                HDC dc, 
                int x, 
                int y, 
                int opt) {
    HDC dci;
    HBITMAP hb_oldi;

    if(idx < 0)
        return;

    dci = CreateCompatibleDC(dc);

    hb_oldi = SelectObject(dci, il->img);

    /*BitBlt(dc, x, y, il->cx, il->cy, dci, idx*il->cx, 0, SRCCOPY); */
    MaskBlt(dc, x, y, il->cx, il->cy, dci, idx * il->cx, 0, il->mask, idx * il->cx, 0, MAKEROP4(SRCPAINT, SRCCOPY));
/*    MaskBlt(dc, x, y, il->cx, il->cy, dci, idx * il->cx, 0, il->mask, idx * il->cx, 0, MAKEROP4(SRCINVERT, SRCCOPY)); */

    SelectObject(dci, hb_oldi);

    DeleteDC(dci);
}

KHMEXP void KHMAPI 
khui_ilist_draw_bg(khui_ilist * il, 
                   int idx, 
                   HDC dc, 
                   int x, 
                   int y, 
                   int opt, 
                   COLORREF bgcolor) {
    HDC dcm;
    HBITMAP hb_oldm, hb_mem;
    HBRUSH hbr;
    RECT r;

    dcm = CreateCompatibleDC(dc);

    hb_mem = CreateCompatibleBitmap(dc, il->cx, il->cy);

    hb_oldm = SelectObject(dcm, hb_mem);

    hbr = CreateSolidBrush(bgcolor);

    r.left = 0;
    r.top = 0;
    r.right = il->cx;
    r.bottom = il->cy;

    FillRect(dcm, &r, hbr);

    khui_ilist_draw(il,idx,dcm,0,0,opt);

    BitBlt(dc,x,y,il->cx,il->cy,dcm,0,0,SRCCOPY);

    SelectObject(dcm, hb_oldm);
    
    DeleteObject(hb_mem);
    DeleteObject(hbr);

    DeleteDC(dcm);
}


KHMEXP void KHMAPI 
khui_bitmap_from_hbmp(khui_bitmap * kbm, HBITMAP hbm)
{
    HDC hdc;
    BITMAPINFO bmi;

    hdc = CreateCompatibleDC(NULL);

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

    kbm->hbmp = hbm;

    if(GetDIBits(hdc, hbm, 0, 0, NULL, &bmi, DIB_RGB_COLORS)) {
        kbm->cx = bmi.bmiHeader.biWidth;
        kbm->cy = bmi.bmiHeader.biHeight;
    } else {
        kbm->cx = -1;
        kbm->cy = -1;
    }

    DeleteDC(hdc);
}

KHMEXP void KHMAPI
khui_delete_bitmap(khui_bitmap * kbm) {
    if (kbm->hbmp)
        DeleteObject(kbm->hbmp);
    kbm->hbmp = NULL;
}

KHMEXP void KHMAPI
khui_draw_bitmap(HDC hdc, int x, int y, khui_bitmap * kbm) {
    HDC hdcb = CreateCompatibleDC(hdc);
    HBITMAP hbmold = SelectObject(hdcb, kbm->hbmp);

    BitBlt(hdc, x, y, kbm->cx, kbm->cy,
           hdcb, 0, 0, SRCCOPY);

    SelectObject(hdcb, hbmold);
    DeleteDC(hdcb);
}

static const struct s_respath_prefixes {
    const wchar_t * prefix;
    khm_size        cch;
} respath_prefixes[] = {
    { KHUI_PREFIX_ICO, ARRAYLENGTH(KHUI_PREFIX_ICO) - 1 },
    { KHUI_PREFIX_IMG, ARRAYLENGTH(KHUI_PREFIX_IMG) - 1 },
    { KHUI_PREFIX_ICODLL, ARRAYLENGTH(KHUI_PREFIX_ICODLL) - 1 },
    { KHUI_PREFIX_IMGDLL, ARRAYLENGTH(KHUI_PREFIX_IMGDLL) - 1}
};

static HICON
HICON_from_HBITMAP(HBITMAP hbm, SIZE * ps)
{
    HICON hicon = NULL;
    HBITMAP bmask = NULL;
    WORD   *pdata = NULL;
    int    n_per_line;
    size_t cb;
    ICONINFO iinfo;

    /* number of WORDs per scanline */
    n_per_line = (ps->cx - 1) / (sizeof(WORD)*8) + 1;

    /* number of bytes for bitmap */
    cb = n_per_line * ps->cy * sizeof(WORD);

    pdata = (WORD *) PMALLOC(cb);
#ifdef DEBUG
    assert(pdata);
#endif
    if (pdata == NULL)
        goto _img_cleanup;

    memset(pdata, 0, cb);

    bmask = CreateBitmap(ps->cx, ps->cy,
                         1, /* number of color planes */
                         1, /* number of bits per pixel */
                         pdata /* pixel data */);
    if (bmask == NULL)
        goto _img_cleanup;

    memset(&iinfo, 0, sizeof(iinfo));

    iinfo.fIcon = TRUE;
    iinfo.hbmMask = bmask;
    iinfo.hbmColor = hbm;

    hicon = CreateIconIndirect(&iinfo);

 _img_cleanup:
    if (bmask)
        DeleteObject(bmask);
    if (pdata)
        PFREE(pdata);

    return hicon;
}

struct image_enum_data {
    int       n_skip;           /* number of images to skip */
    int       n_images;         /* number of matching images found so
                                   far (not counting ones we
                                   skipped). */

    int       n_copied;         /* number of images extracted as icons */

    SIZE      s;                /* dimensions of icon */

    khm_restype restype;        /* type of resource we are looking for */
    khm_size  nc_picon;         /* number of handles we can return */
    HICON *   picon;            /* return handles */

    khm_boolean need_count;     /* do we need an icon count? */
};

/* EnumResNameProc for use with EnumResourceNames(). */
static BOOL CALLBACK
image_enum_proc(HMODULE hm,
               LPCTSTR lptype,
               LPTSTR lpname,
               LONG_PTR param)
{
    struct image_enum_data * d;
    HICON icon;

    d = (struct image_enum_data *) param;

    if (d->n_skip == 0) {

        if (d->nc_picon > 0) {
            HANDLE hobj;

            hobj = LoadImage(hm, lpname,
                             (d->restype == KHM_RESTYPE_ICON)?IMAGE_ICON:IMAGE_BITMAP,
                             d->s.cx, d->s.cy,
                             LR_DEFAULTCOLOR);

            if (hobj == NULL) {
                icon = NULL;
            } else if (d->restype == KHM_RESTYPE_ICON) {
                icon = CopyIcon((HICON) hobj);
                DestroyIcon((HICON) hobj);
            } else {
                icon = HICON_from_HBITMAP((HBITMAP) hobj, &d->s);
                DeleteObject(hobj);
            }

            if (icon) {
                *d->picon++ = icon;
                d->nc_picon --;
                d->n_copied++;
            }
        }

        d->n_images++;

        if (d->nc_picon == 0 && !d->need_count)
            return FALSE;

    } else
        d->n_skip--;

    return TRUE;
}

KHMEXP khm_int32 KHMAPI
khui_load_icons_from_path(const wchar_t * spath, khm_restype restype,
                          khm_int32 index, khm_int32 flags,
                          HICON * picon, khm_size * pn_icons)
{
    wchar_t path[MAX_PATH];
    HICON hicon = NULL;
    HANDLE hobj = NULL;
    SIZE s;

    if (restype != KHM_RESTYPE_BITMAP &&
        restype != KHM_RESTYPE_ICON)
        return KHM_ERROR_INVALID_PARAM;

    if (flags & KHUI_LIFR_SMALL) {
        s.cx = GetSystemMetrics(SM_CXSMICON);
        s.cy = GetSystemMetrics(SM_CYSMICON);
    } else if (flags & KHUI_LIFR_TOOLBAR) {
        s.cx = s.cy = 24;
    } else {
        s.cx = GetSystemMetrics(SM_CXICON);
        s.cy = GetSystemMetrics(SM_CYICON);
    }

    /* spath can contain unexpanded environment strings */
    if (wcschr(spath, L'%')) {
        DWORD dwrv;

        dwrv = ExpandEnvironmentStrings(spath, path, ARRAYLENGTH(path));
        if (dwrv == 0 || dwrv > ARRAYLENGTH(path)) {
            return KHM_ERROR_TOO_LONG;
        }
    } else
        StringCchCopy(path, ARRAYLENGTH(path), spath);

    if (flags & KHUI_LIFR_FROMLIB) {
        HMODULE hm;
        struct image_enum_data d;
        khm_int32 rv = KHM_ERROR_UNKNOWN;

        hm = LoadLibraryEx(path, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (hm == NULL) {
            rv = KHM_ERROR_NOT_FOUND;
            goto _dll_cleanup;
        }

        memset(&d, 0, sizeof(d));
        d.n_skip = index;
        d.nc_picon = (picon != NULL)?*pn_icons:0;
        d.s = s;
        d.restype = restype;
        d.picon = picon;
        d.need_count = (picon == NULL);

        EnumResourceNames(hm, (restype == KHM_RESTYPE_ICON)?RT_GROUP_ICON:RT_BITMAP,
                          image_enum_proc, (LONG_PTR) &d);

        if (picon != NULL)
            *pn_icons = d.n_copied;
        else
            *pn_icons = d.n_images;

        if (d.n_copied < d.n_images)
            rv = KHM_ERROR_TOO_LONG;
        else if (d.n_copied > 0)
            rv = KHM_ERROR_SUCCESS;
        else
            rv = KHM_ERROR_NOT_FOUND;

    _dll_cleanup:
        if (hm)
            FreeLibrary(hm);

        return rv;

    } else {
        /* We are loading a .ico or a .bmp file.  In this case we
           already know we are only going to find at most one icon. */

        if (picon == NULL || *pn_icons < 1) {
            *pn_icons = 1;
            return KHM_ERROR_TOO_LONG;
        }

        hobj = LoadImage(NULL, path, ((restype == KHM_RESTYPE_ICON)?IMAGE_ICON:IMAGE_BITMAP),
                         s.cx, s.cy, LR_DEFAULTCOLOR|LR_LOADFROMFILE);
        if (hobj == NULL) {
            DWORD gle = GetLastError();
            if (gle == ERROR_FILE_NOT_FOUND ||
                gle == ERROR_PATH_NOT_FOUND) {
                return KHM_ERROR_NOT_FOUND;
            }
            return KHM_ERROR_UNKNOWN;
        }

        if (restype == KHM_RESTYPE_ICON) {
            *picon = (HICON) hobj;
            *pn_icons = 1;
            return KHM_ERROR_SUCCESS;
        } else {
            *picon = HICON_from_HBITMAP((HBITMAP) hobj, &s);
            DeleteObject(hobj);
            if (*picon) {
                *pn_icons = 1;
                return KHM_ERROR_SUCCESS;
            } else
                return KHM_ERROR_UNKNOWN;
        }
    }
}

KHMEXP khm_int32 KHMAPI
khui_load_icon_from_resource_path(const wchar_t * respath, khm_int32 flags,
                                  HICON * picon)
{
    wchar_t path[MAX_PATH];
    int index;
    size_t len;
    int method;
    khm_restype restype;
    khm_size n = 1;

    if (respath == NULL || picon == NULL ||
        FAILED(StringCchLength(respath, MAX_PATH, &len)))
        return KHM_ERROR_INVALID_PARAM;

    for (method = 0; method < ARRAYLENGTH(respath_prefixes); method ++) {
        if (wcsncmp(respath, respath_prefixes[method].prefix,
                    respath_prefixes[method].cch) == 0)
            break;
    }

    {
        /* The respath is expected to be in the format :
           "<prefix>:<path>[,<index>]"
         */
        const wchar_t * colon;
        const wchar_t * ppath;
        const wchar_t * comma;

        colon = wcschr(respath, L':');
        if (colon == NULL)
            return KHM_ERROR_INVALID_PARAM;

        ppath = colon + 1;

        comma = wcsrchr(ppath, L',');

        if (comma)
            len = comma - ppath;
        else
            len -= ppath - respath;

        StringCchCopyN(path, ARRAYLENGTH(path), ppath, len);

        if (comma)
            index = _wtoi(comma + 1);
        else
            index = 0;
    }

    switch (method) {
    case 0:                     /* ICO: */
        restype = KHM_RESTYPE_ICON;
        flags &= ~KHUI_LIFR_FROMLIB;
        break;

    case 1:                     /* IMG: */
        restype = KHM_RESTYPE_BITMAP;
        flags &= ~KHUI_LIFR_FROMLIB;
        break;

    case 2:                     /* ICODLL: */
        restype = KHM_RESTYPE_ICON;
        flags |= KHUI_LIFR_FROMLIB;
        break;

    case 3:                     /* IMGDLL: */
        restype = KHM_RESTYPE_BITMAP;
        flags |= KHUI_LIFR_FROMLIB;
        break;

    default:
        return KHM_ERROR_INVALID_PARAM;
    }

    return khui_load_icons_from_path(path, restype, index, flags, picon, &n);
}
