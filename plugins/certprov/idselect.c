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
#include <cryptuiapi.h>
#include<assert.h>

/*  */

/************************************************************/
/*                Identity Selector Control                 */
/************************************************************/

struct idsel_dlg_data {
    khm_int32 magic;            /* Always IDSEL_DLG_DATA_MAGIC */
    khm_handle identity;           /* Current selection */
};

#define IDSEL_DLG_DATA_MAGIC 0x5967d973

static
update_display(HWND hwnd, struct idsel_dlg_data * d) {
    HCERTSTORE hCertStore = 0;
    PCCERT_CONTEXT pCtx = 0;
    wchar_t subject[KCDB_MAXCCH_NAME] = L"";
    wchar_t issuer[KCDB_MAXCCH_NAME] = L"";
    wchar_t altname[KCDB_MAXCCH_NAME] = L"";

    if (d->identity == NULL)
        goto clear_fields;

    find_matching_cert(d->identity, &pCtx, &hCertStore);

    if (pCtx == NULL)
        goto clear_fields;

    CertGetNameString(pCtx, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, subject, ARRAYLENGTH(subject));
    CertGetNameString(pCtx, CERT_NAME_EMAIL_TYPE, 0, NULL, altname, ARRAYLENGTH(altname));
    CertGetNameString(pCtx, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, NULL, issuer, ARRAYLENGTH(issuer));

    SetDlgItemText(hwnd, IDC_SUBJECT, subject);
    SetDlgItemText(hwnd, IDC_ALTERNATE, altname);
    SetDlgItemText(hwnd, IDC_ISSUED, issuer);

    EnableWindow(GetDlgItem(hwnd, IDC_VIEW), TRUE);

    goto cleanup;

 clear_fields:

    SetDlgItemText(hwnd, IDC_SUBJECT, L"");
    SetDlgItemText(hwnd, IDC_ALTERNATE, L"");
    SetDlgItemText(hwnd, IDC_ISSUED, L"");

    EnableWindow(GetDlgItem(hwnd, IDC_VIEW), FALSE);

 cleanup:

    if (pCtx)
        CertFreeCertificateContext(pCtx);

    if (hCertStore)
        CertCloseStore(hCertStore, 0);
}

static
handle_cert_choose(HWND hwnd, struct idsel_dlg_data * d) {
    HCERTSTORE hCertStore = CertOpenSystemStore(0, L"MY");
    PCCERT_CONTEXT pCtx;

    pCtx = CryptUIDlgSelectCertificateFromStore(hCertStore, hwnd, NULL, NULL, 0,
                                                0, NULL);

    if (pCtx) {
        khm_handle identity;

        identity = get_identity_for_cert_ctx(pCtx, KCDB_IDENT_FLAG_CREATE);

        if (d->identity)
            kcdb_identity_release(d->identity);

        d->identity = identity;

        update_display(hwnd, d);
    }

    if (pCtx)
        CertFreeCertificateContext(pCtx);
    if (hCertStore)
        CertCloseStore(hCertStore, 0);
}

static
handle_cert_view(HWND hwnd, struct idsel_dlg_data * d) {
    if (d->identity != NULL) {
        PCCERT_CONTEXT pCtx = 0;
        HCERTSTORE hCertStore = 0;

        find_matching_cert(d->identity, &pCtx, &hCertStore);

        if (pCtx) {
            CryptUIDlgViewContext(CERT_STORE_CERTIFICATE_CONTEXT,
                                  pCtx, hwnd, NULL, 0, NULL);
            CertFreeCertificateContext(pCtx);
        }

        if (hCertStore)
            CertCloseStore(hCertStore, 0);
    }
}

static
handle_cert_import(HWND hwnd, struct idsel_dlg_data * d) {
    HCERTSTORE hCertStore = CertOpenSystemStore(0, L"MY");

    if (CryptUIWizImport(CRYPTUI_WIZ_IMPORT_ALLOW_CERT,
                         hwnd, L"Select a certificate to import ...",
                         NULL,
                         hCertStore
                         )) {

        PCCERT_CONTEXT pCtx = NULL;

        while ((pCtx = CertEnumCertificatesInStore(hCertStore, pCtx)) != NULL) {
            wchar_t cname[KCDB_MAXCCH_NAME];
            khm_handle identity;
            khm_handle cred = NULL;
            khm_handle cred2 = NULL;
            khm_boolean this_is_the_cert = FALSE;

            identity = get_identity_for_cert_ctx(pCtx, 0);
            if (identity == NULL) {
                this_is_the_cert = TRUE;
                goto done;
            }

            cname_from_cert_ctx(pCtx, cname, sizeof(cname));

            kcdb_cred_create(cname, identity, credtype_id, &cred);

            if (KHM_FAILED(kcdb_credset_find_cred(NULL, cred, &cred2))) {
                this_is_the_cert = TRUE;
                goto done;
            }

        done:

            if (this_is_the_cert) {
                if (identity  == NULL) {
                    identity = get_identity_for_cert_ctx(pCtx, KCDB_IDENT_FLAG_CREATE);
                }

                if (d->identity)
                    kcdb_identity_release(d->identity);

                d->identity = identity;
                identity = NULL;
            }

            if (identity)
                kcdb_identity_release(identity);
            if (cred)
                kcdb_cred_release(cred);
            if (cred2)
                kcdb_cred_release(cred2);
            
            if (this_is_the_cert)
                break;
        }
    }

    if (hCertStore)
        CertCloseStore(hCertStore, 0);

    update_display(hwnd, d);
}

/* Dialog procedure for IDD_IDSPEC

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
                if (d->identity) {
                    kcdb_identity_release(d->identity);
                    d->identity = NULL;
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
        {
            struct idsel_dlg_data * d;

            d = (struct idsel_dlg_data *)(LONG_PTR)
                GetWindowLongPtr(hwnd, DWLP_USER);

            switch (wParam) {
            case MAKEWPARAM(IDC_CHOOSE, BN_CLICKED):
                handle_cert_choose(hwnd, d);
                break;

            case MAKEWPARAM(IDC_IMPORT, BN_CLICKED):
                handle_cert_import(hwnd, d);
                break;

            case MAKEWPARAM(IDC_VIEW, BN_CLICKED):
                handle_cert_view(hwnd, d);
                break;
            }
        }
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
                    *ph = d->identity;
                    if (d->identity)
                        kcdb_identity_hold(d->identity);
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
idsel_factory(HWND hwnd_parent, HWND * phwnd_return) {

    HWND hw_dlg;

    hw_dlg = CreateDialog(hResModule, MAKEINTRESOURCE(IDD_IDSPEC),
                          hwnd_parent, idspec_dlg_proc);

#ifdef DEBUG
    assert(hw_dlg);
#endif
    *phwnd_return = hw_dlg;

    return (hw_dlg ? KHM_ERROR_SUCCESS : KHM_ERROR_UNKNOWN);
}

khm_int32
handle_kmsg_ident_get_idsel_factory(kcdb_idsel_factory * pcb)
{
    *pcb = idsel_factory;

    return KHM_ERROR_SUCCESS;
}

