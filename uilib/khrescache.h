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

#ifndef __KHIMAIRA_RESCACHE_H
#define __KHIMAIRA_RESCACHE_H

#include "khdefs.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NIMPRIVATE

KHMEXP void KHMAPI 
khui_init_rescache(void);

KHMEXP void KHMAPI 
khui_exit_rescache(void);

#if 0

KHMEXP void KHMAPI 
khui_cache_bitmap(UINT id, HBITMAP hbm);

KHMEXP HBITMAP KHMAPI 
khui_get_cached_bitmap(UINT id);

#endif

KHMEXP khm_int32 KHMAPI
khui_cache_add_resource(khm_handle owner, khm_int32 id,
                        khm_restype type,
                        const void * buf, khm_size cb_buf);

KHMEXP khm_int32 KHMAPI
khui_cache_get_resource(khm_handle owner, khm_int32 id, khm_restype type,
                        void * buf, khm_size *pcb_buf);

KHMEXP khm_int32 KHMAPI
khui_cache_del_resource(khm_handle owner, khm_int32 id, khm_restype type);

KHMEXP khm_int32 KHMAPI
khui_cache_del_by_owner(khm_handle owner);

#endif  /* NIMPRIVATE */

#define KHUI_PREFIX_ICO    L"ICO:"
#define KHUI_PREFIX_ICODLL L"ICODLL:"
#define KHUI_PREFIX_IMG    L"IMG:"
#define KHUI_PREFIX_IMGDLL L"IMGDLL:"

#define KHUI_LIFR_SMALL    0x00000001
#define KHUI_LIFR_TOOLBAR  0x00000002
#define KHUI_LIFR_FROMLIB  0x00010000

/*! \brief Load an icon from a resource path

  Resource paths are of the form:

  ICO:c:\path\to\iconfile.ico
  ICODLL:c:\path\to\dllfile.dll,resid
  IMG:c:\path\to\imagefile.bmp
  IMGDLL:c:\path\to\dllfile.dll,resid

  \param[in] respath Path to file containing resource

  \param[in] flags   Flags.  This is a combination of :
     - ::KHUI_LIFR_SMALL
     - ::KHUI_LIFR_TOOLBAR

  \param[out] picon Receives a handle to the icon if the call is
     successful.

  \note For IMG and IMGDLL resources, only BMP files are supported.
 */
KHMEXP khm_int32 KHMAPI
khui_load_icon_from_resource_path(const wchar_t * respath, khm_int32 flags,
                                  HICON * picon);


KHMEXP khm_int32 KHMAPI
khui_load_icons_from_path(const wchar_t * path, khm_restype restype,
                          khm_int32 index, khm_int32 flags,
                          HICON * picon, khm_size * pn_icons);

typedef struct khui_ilist_t {
    int cx;
    int cy;
    int n;
    int ng;
    int nused;
    HBITMAP img;
    HBITMAP mask;
    int *idlist;
} khui_ilist;

typedef struct khui_bitmap_t {
    HBITMAP hbmp;
    int cx;
    int cy;
} khui_bitmap;

KHMEXP void KHMAPI  
khui_bitmap_from_hbmp(khui_bitmap * kbm, HBITMAP hbm);

KHMEXP void KHMAPI
khui_delete_bitmap(khui_bitmap * kbm);

KHMEXP void KHMAPI
khui_draw_bitmap(HDC hdc, int x, int y, khui_bitmap * kbm);

/* image lists */
KHMEXP khui_ilist * KHMAPI  
khui_create_ilist(int cx, int cy, int n, int ng, int opt);

KHMEXP BOOL KHMAPI          
khui_delete_ilist(khui_ilist * il);

KHMEXP int KHMAPI           
khui_ilist_add_masked(khui_ilist * il, HBITMAP hbm, COLORREF cbkg);

KHMEXP int KHMAPI           
khui_ilist_add_masked_id(khui_ilist *il, HBITMAP hbm, 
                         COLORREF cbkg, int id);

KHMEXP int KHMAPI           
khui_ilist_lookup_id(khui_ilist *il, int id);

KHMEXP void KHMAPI          
khui_ilist_draw(khui_ilist * il, int idx, HDC dc, int x, int y, int opt);

KHMEXP void KHMAPI          
khui_ilist_draw_bg(khui_ilist * il, int idx, HDC dc, int x, int y, 
                   int opt, COLORREF bgcolor);

#define khui_ilist_draw_id(il, id, dc, x, y, opt) \
    khui_ilist_draw((il),khui_ilist_lookup_id((il),(id)),(dc),(x),(y),(opt))

#define KHUI_SMICON_CX 16
#define KHUI_SMICON_CY 16

#ifdef __cplusplus
}
#endif

#endif
