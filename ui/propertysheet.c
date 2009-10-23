/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
 * Copyright (c) 2007-2009 Secure Endpoints Inc.
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

static INT_PTR CALLBACK 
identpp_dialog_proc
(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    khui_property_sheet * s;

    switch(uMsg) {
    case WM_INITDIALOG:
        {
            PROPSHEETPAGE * p;
            khm_handle ident;
            wchar_t idname[KCDB_IDENT_MAXCCH_NAME];
            khm_size t;
            khm_int32 i;

            p = (PROPSHEETPAGE *) lParam;
            s = (khui_property_sheet *) p->lParam;

#pragma warning(push)
#pragma warning(disable: 4244)
            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) s);
#pragma warning(pop)

            ident = s->identity;

            t = sizeof(idname);
            kcdb_get_resource(ident, KCDB_RES_DISPLAYNAME, 0, NULL, NULL, idname, &t);
            SetDlgItemText(hwnd, IDC_PP_IDNAME, idname);

            kcdb_identity_get_flags(ident, &i);

            CheckDlgButton(hwnd, IDC_PP_IDDEF,
                           ((i & KCDB_IDENT_FLAG_DEFAULT)?BST_CHECKED:
                            BST_UNCHECKED));

            /* if it's default, you can't change it further */
            if (i & KCDB_IDENT_FLAG_DEFAULT) {
                EnableWindow(GetDlgItem(hwnd, IDC_PP_IDDEF), FALSE);
            }

            CheckDlgButton(hwnd, IDC_PP_IDSEARCH,
                           ((i & KCDB_IDENT_FLAG_SEARCHABLE)?BST_CHECKED:
                            BST_UNCHECKED));

            CheckDlgButton(hwnd, IDC_PP_STICKY,
                           ((i & KCDB_IDENT_FLAG_STICKY)?BST_CHECKED:
                            BST_UNCHECKED));

            khui_property_wnd_set_record(GetDlgItem(hwnd, IDC_PP_PROPLIST),
                                         ident);
        }
        return TRUE;

    case WM_COMMAND:
        s = (khui_property_sheet *) (LONG_PTR) 
            GetWindowLongPtr(hwnd, DWLP_USER);
        if (s == NULL)
            return 0;

        switch(wParam) {
        case MAKEWPARAM(IDC_PP_IDDEF, BN_CLICKED):
            /* fallthrough */
        case MAKEWPARAM(IDC_PP_STICKY, BN_CLICKED):

            if (s->status != KHUI_PS_STATUS_NONE)
                PropSheet_Changed(s->hwnd, hwnd);
            return TRUE;

        case MAKEWPARAM(IDC_PP_CONFIG, BN_CLICKED):
            khm_show_identity_config_pane(s->identity);
            return TRUE;
        }
        return FALSE;

    case WM_NOTIFY:
        {
            LPPSHNOTIFY lpp;
            khm_int32 flags;

            lpp = (LPPSHNOTIFY) lParam;
            s = (khui_property_sheet *) (LONG_PTR) 
                GetWindowLongPtr(hwnd, DWLP_USER);
            if (s == NULL)
                return 0;

            switch(lpp->hdr.code) {
            case PSN_APPLY:
                flags = 0;
                if (IsDlgButtonChecked(hwnd, IDC_PP_STICKY) == BST_CHECKED)
                    flags |= KCDB_IDENT_FLAG_STICKY;
                if (IsDlgButtonChecked(hwnd, IDC_PP_IDDEF) == BST_CHECKED)
                    flags |= KCDB_IDENT_FLAG_DEFAULT;

                kcdb_identity_set_flags(s->identity, flags,
                                        KCDB_IDENT_FLAG_STICKY |
                                        KCDB_IDENT_FLAG_DEFAULT);
                khm_refresh_identity_menus();
                return TRUE;

            case PSN_RESET:
                kcdb_identity_get_flags(s->identity, &flags);

                CheckDlgButton(hwnd, 
                               IDC_PP_IDDEF, 
                               ((flags & KCDB_IDENT_FLAG_DEFAULT)?BST_CHECKED:
                                BST_UNCHECKED));

                /* if it's default, you can't change it further */
                if (flags & KCDB_IDENT_FLAG_DEFAULT) {
                    EnableWindow(GetDlgItem(hwnd, IDC_PP_IDDEF), FALSE);
                }

                CheckDlgButton(hwnd, IDC_PP_IDSEARCH,
                               ((flags & KCDB_IDENT_FLAG_SEARCHABLE)?BST_CHECKED:BST_UNCHECKED));

                CheckDlgButton(hwnd, IDC_PP_STICKY,
                               ((flags & KCDB_IDENT_FLAG_STICKY)?BST_CHECKED:BST_UNCHECKED));
                return TRUE;
            }
        }
        break;
    }
    return FALSE;
}

static INT_PTR CALLBACK 
credpp_dialog_proc
(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg) {
        case WM_INITDIALOG:
            {
                khui_property_sheet * s;
                PROPSHEETPAGE * p;
                khm_handle cred;

                p = (PROPSHEETPAGE *) lParam;
                s = (khui_property_sheet *) p->lParam;

#pragma warning(push)
#pragma warning(disable: 4244)
                SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) s);
#pragma warning(pop)

                cred = s->cred;

                khui_property_wnd_set_record(
                    GetDlgItem(hwnd, IDC_PP_CPROPLIST),
                    cred);
            }
            return TRUE;
    }
    return FALSE;
}

void 
khm_pp_begin(khui_property_sheet * s)
{
    PROPSHEETPAGE *p;

    if(s->identity) {
        p = PMALLOC(sizeof(*p));
        ZeroMemory(p, sizeof(*p));

        p->dwSize = sizeof(*p);
        p->dwFlags = 0;
        p->hInstance = khm_hInstance;
        p->pszTemplate = MAKEINTRESOURCE(IDD_PP_IDENT);
        p->pfnDlgProc = identpp_dialog_proc;
        p->lParam = (LPARAM) s;

        khui_ps_add_page(s, KHUI_PPCT_IDENTITY, 129, p, NULL);
    }

    if(s->cred) {
        p = PMALLOC(sizeof(*p));
        ZeroMemory(p, sizeof(*p));

        p->dwSize = sizeof(*p);
        p->dwFlags = 0;
        p->hInstance = khm_hInstance;
        p->pszTemplate = MAKEINTRESOURCE(IDD_PP_CRED);
        p->pfnDlgProc = credpp_dialog_proc;
        p->lParam = (LPARAM) s;

        khui_ps_add_page(s, KHUI_PPCT_CREDENTIAL, 128, p, NULL);
    }
}

void 
khm_pp_precreate(khui_property_sheet * s)
{
    khui_ps_show_sheet(khm_hwnd_main, s);

    khm_add_property_sheet(s);
}

void 
khm_pp_end(khui_property_sheet * s)
{
    khui_property_page * p = NULL;

    khui_ps_find_page(s, KHUI_PPCT_IDENTITY, &p);
    if(p) {
        PFREE(p->p_page);
        p->p_page = NULL;
    }

    p = NULL;

    khui_ps_find_page(s, KHUI_PPCT_CREDENTIAL, &p);
    if(p) {
        PFREE(p->p_page);
        p->p_page = NULL;
    }
}

void 
khm_pp_destroy(khui_property_sheet *ps)
{
    if(ps->ctx.scope == KHUI_SCOPE_CRED) {
        if(ps->header.pszCaption)
            PFREE((LPWSTR) ps->header.pszCaption);
    }

    khui_context_release(&ps->ctx);

    khui_ps_destroy_sheet(ps);

    /* this is pretty weird because ps gets freed when
       khui_ps_destroy_sheet() is called.  However, since destroying
       ps involves sending a WM_DESTROY message to the property sheet,
       we still need to keep it on the property sheet chain (or else
       the messages will not be delivered).  This is only safe because
       we are not relinquishing the thread in-between destroying ps
       and removing it from the chain. */

    /* TODO: fix this */
    khm_del_property_sheet(ps);
}

LRESULT
khm_show_properties(void)
{
    /* show a property sheet of some sort */
    khui_action_context ctx;
    khui_property_sheet * ps;

    khui_context_get(&ctx);

    if((ctx.scope != KHUI_SCOPE_IDENT &&
        ctx.scope != KHUI_SCOPE_CRED) ||
       ctx.identity == NULL ||
       khm_find_and_activate_property_sheet(&ctx)) {
        khui_context_release(&ctx);
        return FALSE;
    }

    khui_ps_create_sheet(&ps);

    ps->header.hInstance = khm_hInstance;
    ps->header.pszIcon = MAKEINTRESOURCE(IDI_MAIN_APP);
    ps->ctx = ctx;

    if(ctx.scope == KHUI_SCOPE_IDENT) {
        khm_handle ident;
        khm_size t = 0;

        ident = ctx.identity;

        kcdb_get_resource(ident, KCDB_RES_DISPLAYNAME, 0, NULL, NULL, NULL, &t);

        if(t > 0) {
            ps->header.pszCaption = PMALLOC(t);
            kcdb_get_resource(ident, KCDB_RES_DISPLAYNAME, 0, NULL, NULL,
                              (wchar_t *) ps->header.pszCaption, &t);
        } else {
            ps->header.pszCaption = NULL;
        }

        ps->identity = ident;
        kcdb_identity_hold(ps->identity);
        ps->credtype = KCDB_CREDTYPE_INVALID;
        ps->cred = NULL;

    } else if(ctx.scope == KHUI_SCOPE_CRED) {
        khm_handle cred;
        khm_size t = 0;

        cred = ctx.cred;

        kcdb_get_resource(cred, KCDB_RES_DISPLAYNAME, 0, NULL, NULL, NULL, &t);
        ps->header.pszCaption = PMALLOC(t);
        kcdb_get_resource(cred, KCDB_RES_DISPLAYNAME, 0, NULL, NULL, (wchar_t *) ps->header.pszCaption, &t);

        kcdb_cred_get_identity(cred, &ps->identity);
        kcdb_cred_get_type(cred, &ps->credtype);
        ps->cred = cred;
        kcdb_cred_hold(cred);
    }

    kmq_post_message(KMSG_CRED, KMSG_CRED_PP_BEGIN, 0, (void *) ps);

    /* Leaving ctx held, since it is now copied to ps->ctx */

    return TRUE;
}

