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

#define NOEXPORT
#define _NIMLIB_

#include<khuidefs.h>
#include<utils.h>
#include<assert.h>
#include<strsafe.h>

CRITICAL_SECTION cs_res;

typedef struct tag_cached_resource {
    khm_handle       owner;
    khm_restype      type;
    khm_int32        id;

    union {
        const wchar_t * str;
        HANDLE       h_gdiobj;
    };
    khm_size         cb_buf;

    LDCL(struct tag_cached_resource);
} cached_resource;

cached_resource * cached_resources;
hashtable * ht_resources;

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

    switch (r->type) {
    case KHM_RESTYPE_STRING:
        break;

    case KHM_RESTYPE_ICON:
        DestroyIcon(r->h_gdiobj);
        break;

    case KHM_RESTYPE_BITMAP:
        DeleteObject(r->h_gdiobj);
        break;

    default:
#ifdef DEBUG
        assert(FALSE);
#endif
    }

    PFREE(r);
    LDELETE(&cached_resources, r);
}

KHMEXP void KHMAPI 
khui_init_rescache(void) {
    InitializeCriticalSection(&cs_res);
    ht_resources = hash_new_hashtable(127, hash_resource, comp_resource,
                                      NULL, del_ref_resource);
}

KHMEXP void KHMAPI 
khui_exit_rescache(void) {
    EnterCriticalSection(&cs_res);

    hash_del_hashtable(ht_resources);

    LeaveCriticalSection(&cs_res);
    DeleteCriticalSection(&cs_res);
}

/* Owner is one of the following:

   - Credential type identifier
   - Identity handler
   - Identity provider handle
   - HMODULE
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
