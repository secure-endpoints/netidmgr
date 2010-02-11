/*
 * Copyright (c) 2008-2010 Secure-Endpoints Inc.
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

#ifndef __NETIDMGR_APPEARANCE_H__
#define __NETIDMGR_APPEARANCE_H__

#ifdef __cplusplus
extern "C" {
#endif

enum tag_khm_ui_element {

    /* Fonts */

    KHM_FONT_BASE = 1,

    KHM_FONT_HEADER,
    KHM_FONT_HEADERSEL,
    KHM_FONT_NORMAL,
    KHM_FONT_SELECT,
    KHM_FONT_TITLE,
    KHM_FONT_AUX,

    KHM_FONT_T_MAX,             /* Marker */

    /* Colors */

    KHM_CLR_BASE = 1001,
    KHM_CLR_SELECTION,
    KHM_CLR_BACKGROUND,
    KHM_CLR_ALERTBKG,
    KHM_CLR_SUGGESTBKG,
    KHM_CLR_ACCENT,

    KHM_CLR_HEADER,
    KHM_CLR_HEADER_ACCENT,

    KHM_CLR_HEADER_CRED,
    KHM_CLR_HEADER_WARN,
    KHM_CLR_HEADER_CRIT,
    KHM_CLR_HEADER_EXP,
    KHM_CLR_HEADER_SEL,

    KHM_CLR_HEADER_CRED_SEL,
    KHM_CLR_HEADER_WARN_SEL,
    KHM_CLR_HEADER_CRIT_SEL,
    KHM_CLR_HEADER_EXP_SEL,

    KHM_CLR_TEXT,
    KHM_CLR_TEXT_SEL,
    KHM_CLR_TEXT_ERR,

    KHM_CLR_TEXT_HEADER,
    KHM_CLR_TEXT_HEADER_SEL,
    KHM_CLR_TEXT_HEADER_DIS,
    KHM_CLR_TEXT_HEADER_DIS_SEL,

    KHM_CLR_T_MAX               /* Marker */
};

typedef enum tag_khm_ui_element khm_ui_element;

HFONT
khm_get_element_font(khm_ui_element element);

khm_int32
khm_get_element_lfont(HDC hdc, khm_ui_element element, khm_boolean use_default,
                     LOGFONT * pfont);

khm_int32
khm_set_element_lfont(khm_ui_element element, const LOGFONT * pfont);

COLORREF
khm_mix_colors(COLORREF c1, COLORREF c2, int alpha);

COLORREF
khm_get_element_color(khm_ui_element element);

khm_int32
khm_set_element_color(khm_ui_element element, COLORREF cr);

khm_int32
khm_load_theme(const wchar_t * theme);

void
khm_refresh_theme(void);

void
khm_init_themes(void);

void
khm_exit_themes(void);

#ifdef __cplusplus
}
#endif

#endif
