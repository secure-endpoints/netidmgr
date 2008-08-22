/*
 * Copyright (c) 2008 Secure Endpoints Inc.
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

#include "khmapp.h"

#ifdef DEBUG
#include<assert.h>
#endif

static khm_handle csp_theme = NULL;

static const wchar_t *
font_element_names[] = {
    L"FontBase",
    L"FontHeader",
    L"FontHeaderSel",
    L"FontNormal",
    L"FontSel",
    L"FontTitle",
    L"FontAux"
};

#define is_font_id(id) ((id) >= KHM_FONT_BASE && (id) < KHM_FONT_T_MAX)
#define font_id_to_name(id) (font_element_names[(id) - KHM_FONT_BASE])

static const wchar_t *
clr_element_names[] = {
    L"ClrBase",
    L"ClrSelection",
    L"ClrBackground",
    L"ClrGray",
    L"ClrHeader",
    L"ClrHeaderOutline",
    L"ClrHeaderCred",
    L"ClrHeaderWarn",
    L"ClrHeaderCrit",
    L"ClrHeaderExp",
    L"ClrHeaderSel",
    L"ClrHeaderCredSel",
    L"ClrHeaderWarnSel",
    L"ClrHeaderCritSel",
    L"ClrHeaderExpSel",
    L"ClrText",
    L"ClrTextSel",
    L"ClrTextErr",
    L"ClrTextHeader",
    L"ClrTextHeaderSel",
    L"ClrTextHeaderGray",
    L"ClrTextHeaderGraySel"
};

#define is_clr_id(id) ((id) >= KHM_CLR_BASE && (id) < KHM_CLR_T_MAX)
#define clr_id_to_name(id) (clr_element_names[(id) - KHM_CLR_BASE])

/* The address of this object is used as the owner of all theme
   objects when caching them in the resource cache. */
static khm_int32  g_owner = 0;

static khm_int32
open_theme_handle(const wchar_t * name)
{
    khm_handle csp_cw = NULL;
    khm_handle csp_themes = NULL;
    khm_handle csp_t = NULL;

    wchar_t buf[KCDB_MAXCCH_NAME];
    khm_size cb;

    khm_int32 rv = KHM_ERROR_SUCCESS;

    if (KHM_FAILED(rv = khc_open_space(NULL, L"CredWindow",
                                       KHM_PERM_READ|KHM_PERM_WRITE, &csp_cw)) ||
        KHM_FAILED(rv = khc_open_space(csp_cw, L"Themes",
                                       KHM_PERM_READ|KHM_PERM_WRITE, &csp_themes))) {
        goto _exit;
    }

    if (name == NULL || name[0] == L'\0') {
        cb = sizeof(buf);
        if (KHM_SUCCEEDED(rv = khc_read_string(csp_cw, L"DefaultTheme", buf, &cb)))
            name = buf;
        else
            goto _exit;
    }

    if (KHM_SUCCEEDED(rv = khc_open_space(csp_themes, name, KHM_PERM_READ|KHM_PERM_WRITE,
                                          &csp_t))) {
        if (csp_theme)
            khc_close_space(csp_theme);
        csp_theme = csp_t;
        csp_t = NULL;
    }

 _exit:
    if (csp_cw)
        khc_close_space(csp_cw);
    if (csp_themes)
        khc_close_space(csp_themes);
    if (csp_t)
        khc_close_space(csp_t);

#ifdef DEBUG
    assert(csp_theme != NULL);
#endif

    return rv;
}

HFONT
khm_get_element_font(khm_ui_element element)
{
    HFONT hf = NULL;
    khm_size cb;

    cb = sizeof(hf);
    if (KHM_FAILED(khui_cache_get_resource(&g_owner, element,
                                           KHM_RESTYPE_FONT, &hf, &cb))) {

        LOGFONT lf;

        if (KHM_SUCCEEDED(khm_get_element_lfont(NULL, element, FALSE, &lf))) {
            hf = CreateFontIndirect(&lf);
            if (hf) {
                khui_cache_add_resource(&g_owner, element, KHM_RESTYPE_FONT,
                                        &hf, sizeof(hf));
            }
        }
    }

    return hf;
}

khm_int32
khm_get_element_lfont(HDC hdc, khm_ui_element element, khm_boolean use_default,
                      LOGFONT * pfont)
{
    khm_size cb;
    const wchar_t * element_name = NULL;
    LOGFONT lf;
    khm_int32 rv = KHM_ERROR_SUCCESS;
    khm_boolean adjust_size = TRUE;

#ifdef DEBUG
    assert(is_font_id(element));
    assert(csp_theme != NULL);
#endif

    if (csp_theme == NULL)
        return KHM_ERROR_INVALID_OPERATION;

    if (!is_font_id(element))
        return KHM_ERROR_INVALID_PARAM;

    element_name = font_id_to_name(element);

    ZeroMemory(&lf, sizeof(lf));

    if (use_default)
        goto _use_default;

    cb = sizeof(lf);
    if (KHM_FAILED(rv = khc_read_binary(csp_theme, element_name, &lf, &cb)) ||
        cb != sizeof(LOGFONT)) {

        /* If the specific entry is not found, we have to try and
           derive it. */

        if (element == KHM_FONT_BASE)
            goto _use_default;

        cb = sizeof(lf);
        if (KHM_FAILED(rv = khc_read_binary(csp_theme, font_id_to_name(KHM_FONT_BASE),
                                            &lf, &cb)) ||
            cb != sizeof(LOGFONT)) {
            /* For backwards compatibility, we also look at the
               FontBase value which might be in the CredWindow config
               space. */
            khm_handle csp_cw = NULL;

            cb = sizeof(lf);
            if (KHM_SUCCEEDED(khc_open_space(NULL, L"CredWindow", 0, &csp_cw)) &&
                KHM_SUCCEEDED(khc_read_binary(csp_cw, L"FontBase", &lf, &cb)) &&
                cb == sizeof(lf)) {

                rv = KHM_ERROR_SUCCESS;
                adjust_size = FALSE;

            }

            if (csp_cw)
                khc_close_space(csp_cw);
        }

        if (KHM_FAILED(rv))
            goto _use_default;

        switch (element) {
        case KHM_FONT_HEADERSEL:
        case KHM_FONT_SELECT:
            lf.lfWeight = FW_BOLD;
            break;

        case KHM_FONT_AUX:
            lf.lfItalic = TRUE;
            break;

        case KHM_FONT_TITLE:
            lf.lfHeight = 10;
            adjust_size = TRUE;
            break;
        }
    }

    goto _prepare_lf;

 _use_default:

    {
        static const LOGFONT lfs[] = {
            /* Base */
            { 8, 0, 0, 0, FW_THIN,
              FALSE, FALSE, FALSE, DEFAULT_CHARSET,
              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
              FF_SWISS, L""},

            /* Header */
            { 8, 0, 0, 0, FW_THIN,
              FALSE, FALSE, FALSE, DEFAULT_CHARSET,
              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
              FF_SWISS, L""},

            /* Header Selected */
            { 8, 0, 0, 0, FW_BOLD,
              FALSE, FALSE, FALSE, DEFAULT_CHARSET,
              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
              FF_SWISS, L""},

            /* Normal */
            { 8, 0, 0, 0, FW_THIN,
              FALSE, FALSE, FALSE, DEFAULT_CHARSET,
              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
              FF_SWISS, L""},

            /* Selected */
            { 8, 0, 0, 0, FW_BOLD,
              FALSE, FALSE, FALSE, DEFAULT_CHARSET,
              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
              FF_SWISS, L""},

            /* Title */
            { 10, 0, 0, 0, FW_BOLD,
              FALSE, FALSE, FALSE, DEFAULT_CHARSET,
              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
              FF_SWISS, L""},

            /* Aux */
            { 8, 0, 0, 0, FW_THIN,
              TRUE, FALSE, FALSE, DEFAULT_CHARSET,
              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
              FF_SWISS, L""}
        };

#ifdef DEBUG
        assert(ARRAYLENGTH(lfs) == KHM_FONT_T_MAX - KHM_FONT_BASE);
#endif

        lf = lfs[element - KHM_FONT_BASE];
        LoadString(khm_hInstance, IDS_DEFAULT_FONT,
                   lf.lfFaceName, ARRAYLENGTH(lf.lfFaceName));
        adjust_size = TRUE;
        rv = KHM_ERROR_SUCCESS;
    }

 _prepare_lf:
    /* The lfHeight field contains the point size of the font.  We
       have to convert that to logical units in use by the DC. */
    if (adjust_size && KHM_SUCCEEDED(rv)) {
        int logpx;

        if (hdc) {
            logpx = GetDeviceCaps(hdc, LOGPIXELSY);
        } else {
            hdc = GetWindowDC(khm_hwnd_main);
            logpx = GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(khm_hwnd_main, hdc);
            hdc = NULL;
        }

        lf.lfHeight = -MulDiv(lf.lfHeight, logpx, 72);
    }

    if (KHM_SUCCEEDED(rv))
        *pfont = lf;

    return rv;
}

khm_int32
khm_set_element_lfont(khm_ui_element element, const LOGFONT * pfont)
{
    LOGFONT lf;
    size_t cch;

#ifdef DEBUG
    assert(is_font_id(element));
    assert(csp_theme != NULL);
#endif

    if (csp_theme == NULL)
        return KHM_ERROR_INVALID_OPERATION;

    if (!is_font_id(element) || pfont == NULL) {
        return KHM_ERROR_INVALID_PARAM;
    }

    lf = *pfont;

    if (FAILED(StringCchLength(lf.lfFaceName, ARRAYLENGTH(lf.lfFaceName), &cch)))
        return KHM_ERROR_INVALID_PARAM;

    if (ARRAYLENGTH(lf.lfFaceName) - cch > 1) {
        ZeroMemory(&lf.lfFaceName[cch],
                   (ARRAYLENGTH(lf.lfFaceName) - cch) * sizeof(TCHAR));
    }

    return khc_write_binary(csp_theme, font_id_to_name(element), &lf, sizeof(lf));
}


/*! \brief Overlay color

  For each color constant value (KHM_CLR_ACCENT - KHM_CLR_BASE)
  defines the default overlay color for color composition.  For now
  it's used to combine base colors with the selection color to come up
  with the selection color for various elements.
 */
static const int overlay_color[] = {
    0, 0, 0, 0,  0, 0,  0, 0, 0, 0, KHM_CLR_SELECTION,

    KHM_CLR_SELECTION,
    KHM_CLR_SELECTION,
    KHM_CLR_SELECTION,
    KHM_CLR_SELECTION,

    0, KHM_CLR_SELECTION, 0,

    0, KHM_CLR_SELECTION, 0, KHM_CLR_SELECTION
};

COLORREF
khm_mix_colors(COLORREF c1, COLORREF c2, int alpha) {
    int r = (GetRValue(c1) * alpha + GetRValue(c2) * (255 - alpha)) / 255;
    int g = (GetGValue(c1) * alpha + GetGValue(c2) * (255 - alpha)) / 255;
    int b = (GetBValue(c1) * alpha + GetBValue(c2) * (255 - alpha)) / 255;

#ifdef DEBUG
    assert(alpha >= 0 && alpha < 256);
#endif

    return RGB(r,g,b);
}

COLORREF
khm_get_element_color(khm_ui_element element)
{
    COLORREF c, cref;
    khm_int32 t;
    int alpha;
    khm_ui_element refclr;

#ifdef DEBUG
    assert(is_clr_id(element));
    assert(csp_theme);
#endif

    if (!is_clr_id(element) || csp_theme == NULL)
        return RGB(0, 0, 0);

#ifdef DEBUG
    assert(ARRAYLENGTH(overlay_color) == KHM_CLR_T_MAX - KHM_CLR_BASE);
#endif

    refclr = overlay_color[element - KHM_CLR_BASE];
    if (refclr == 0)
        refclr = KHM_CLR_BASE;

#ifdef DEBUG
    assert(is_clr_id(refclr));
#endif

    t = 0;
    khc_read_int32(csp_theme, clr_id_to_name(refclr), &t);
    cref = (COLORREF) t;

    if (KHM_FAILED(khc_read_int32(csp_theme, clr_id_to_name(element), &t))) {
        c = cref;
    } else {
        alpha = ((t >> 24) & 0xff);
        c = (COLORREF) (t & 0xffffff);
        c = khm_mix_colors(cref, c, alpha);
    }

    return c;
}

khm_int32
khm_set_element_color(khm_ui_element element, COLORREF cr)
{
#ifdef DEBUG
    assert(is_clr_id(element));
    assert(csp_theme);
#endif

    if (!is_clr_id(element))
        return KHM_ERROR_INVALID_PARAM;

    if (csp_theme == NULL)
        return KHM_ERROR_INVALID_OPERATION;

    return khc_write_int32(csp_theme, clr_id_to_name(element), cr);
}

khm_int32
khm_load_theme(const wchar_t * theme)
{
    return open_theme_handle(theme);
}

void
khm_init_themes(void)
{
    open_theme_handle(NULL);
}

void
khm_exit_themes(void)
{
    if (csp_theme) {
        khc_close_space(csp_theme);
        csp_theme = NULL;
    }

    khui_cache_del_by_owner(&g_owner);
}

khm_int32
khm_draw_text(HDC hdc, const wchar_t * text, khm_ui_element font,
              unsigned int dt_flags, RECT * r)
{
    HFONT hf_old;
    HFONT hf_new;
    size_t cch;
    int right;

    if (FAILED(StringCchLength(text, KHUI_MAXCCH_MESSAGE, &cch)))
        return KHM_ERROR_TOO_LONG;

    hf_new = khm_get_element_font(font);

    if (hf_new == NULL)
        return KHM_ERROR_INVALID_PARAM;

    hf_old = SelectFont(hdc, hf_new);

    right = r->right;

    DrawText(hdc, text, (int) cch, r, dt_flags);

    if ((dt_flags & DT_CALCRECT) == DT_CALCRECT &&
        (dt_flags & DT_SINGLELINE) != DT_SINGLELINE) {
        /* We preserve the right margin if we are calculating the
           extents and it's not a single line.  Otherwise DrawText()
           will adjust the right side to tightly fit the text. */
        r->right = right;
    }

    SelectFont(hdc, hf_old);

    return KHM_ERROR_SUCCESS;
}

