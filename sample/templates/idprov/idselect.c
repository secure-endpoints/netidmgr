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

#include "module.h"
#include<assert.h>

/*  */

/************************************************************/
/*                Identity Selector Control                 */
/************************************************************/

struct idsel_dlg_data {
    khm_int32 magic;            /* Always IDSEL_DLG_DATA_MAGIC */
    khm_handle ident;           /* Current selection */
};

#define IDSEL_DLG_DATA_MAGIC 0x5967d973

/* Dialog procedure for IDD_IDSEL

   runs in UI thread */
static INT_PTR CALLBACK
idspec_dlg_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg) {
    case WM_INITDIALOG:
        {
            struct idsel_dlg_data * d;

            d = PMALLOC(sizeof(*d));
            ZeroMemory(d, sizeof(*d));
            d->magic = IDSEL_DLG_DATA_MAGIC;
            d->identity = NULL;

#pragma warning(push)
#pragma warning(disable: 4244)
            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) d);
#pragma warning(pop)

            /* TODO: Initialize controls etc. */

        }
        /* We return FALSE here because this is an embedded modeless
           dialog and we don't want to steal focus from the
           container. */
        return FALSE;

    case WM_DESTROY:
        {
            struct idsel_dlg_data * d;

            d = (struct idsel_dlg_data *)(LONG_PTR)
                GetWindowLongPtr(hwnd, DWLP_USER);

#ifdef DEBUG
            assert(d != NULL);
            assert(d->magic == IDSEL_DLG_DATA_MAGIC);
#endif
            if (d && d->magic == IDSEL_DLG_DATA_MAGIC) {
                if (d->ident) {
                    kcdb_identity_release(d->ident);
                    d->ident = NULL;
                }

                d->magic = 0;
                PFREE(d);
            }
#pragma warning(push)
#pragma warning(disable: 4244)
            SetWindowLongPtr(hwnd, DWLP_USER, 0);
#pragma warning(pop)
        }
        return TRUE;

    case WM_COMMAND:
        break;

    case KHUI_WM_NC_NOTIFY:
        {
            struct idsel_dlg_data * d;
            khm_handle * ph;

            d = (struct idsel_dlg_data *)(LONG_PTR)
                GetWindowLongPtr(hwnd, DWLP_USER);

            switch (HIWORD(wParam)) {
            case WMNC_IDSEL_GET_IDENT:
                ph = (khm_handle *) lParam;

                /* TODO: Make sure that d->ident is up-to-date */
                if (ph) {
                    *ph = d->ident;
                    if (d->ident)
                        kcdb_identity_hold(d->ident);
                }
                break;
            }
        }
        return TRUE;
    }
    return FALSE;
}

/* Identity Selector control factory

   Runs in UI thread */
khm_int32 KHMAPI
idsel_factory(HWND hwnd_parent, khui_identity_selector * u_data) {
    wchar_t dn[KHUI_MAXCCH_NAME];

    if (hwnd_parent) {

        u_data->hwnd_selector = CreateDialog(hResModule, MAKEINTRESOURCE(IDD_IDSPEC),
                                             hwnd_parent, idspec_dlg_proc);

        assert(u_data->hwnd_selector);

        LoadString(hResModule, IDS_ID_INSTANCE, dn, ARRAYLENGTH(dn));
        u_data->display_name = PWCSDUP(dn);

        u_data->icon = LoadIcon(hResModule, MAKEINTRESOURCE(IDI_IDENTITY));

        return (u_data->hwnd_selector ? KHM_ERROR_SUCCESS : KHM_ERROR_UNKNOWN);

    } else {
        if (u_data->display_name) {
            PFREE(u_data->display_name);
            u_data->display_name = NULL;
        }

        if (u_data->icon) {
            DestroyIcon(u_data->icon);
            u_data->icon = NULL;
        }

        return KHM_ERROR_SUCCESS;
    }
}

