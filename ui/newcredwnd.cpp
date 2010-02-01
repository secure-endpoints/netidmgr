/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
 * Copyright (c) 2008-2009 Secure Endpoints Inc.
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

/* Include the OEMRESOURCE constants for locating standard icon
   resources. */
#define OEMRESOURCE



#include "khmapp.h"

#include "NewCredWizard.hpp"
#include "AlertContainer.hpp"

#include<assert.h>

using namespace nim;


extern "C"
void
khm_nc_track_progress_of_this_task(khui_new_creds * tnc)
{
    khui_new_creds * nc = tnc;

    if (tnc->parent != NULL) {
        AutoRef<NewCredWizard> pw(NewCredWizard::FromNC(tnc));

        if (pw.IsNull() || pw->m_progress.hwnd == NULL)
            nc = tnc->parent;
    }

    AutoRef<NewCredWizard> w (NewCredWizard::FromNC(nc));

    if (!w.IsNull() && w->m_progress.hwnd != NULL) {

	AlertContainer * c = NULL;

        if (tnc == nc) {
	    RECT r_pos;

            if (!w->m_progress.cw_container.IsNull()) {
		w->m_progress.cw_container->DestroyWindow();
                w->m_progress.cw_container = (ControlWindow *) NULL;
            }

            {
                HWND hw_rect;

                hw_rect = w->m_progress.GetItem(IDC_CONTAINER);
                GetWindowRect(hw_rect, &r_pos);
                MapWindowRect(HWND_DESKTOP, w->m_progress.hwnd, &r_pos);
            }

	    c = PNEW AlertContainer();
	    w->m_progress.cw_container = c;

	    c->Create(w->m_progress.hwnd, RectFromRECT(&r_pos));
	    c->ShowWindow();
        } else {
            c = dynamic_cast<AlertContainer *>(&*w->m_progress.cw_container);
        }

	if (c) {
	    khui_alert * _a = NULL;

	    khui_alert_create_empty(&_a);

	    Alert a(_a, true);

	    khui_alert_set_type(a, KHUI_ALERTTYPE_PROGRESSACQ);

	    c->Add(a);

	    khui_alert_monitor_progress(a, NULL,
					KHUI_AMP_ADD_CHILDREN |
					KHUI_AMP_SHOW_EVT_ERR |
					KHUI_AMP_SHOW_EVT_WARN);
            c->BeginMonitoringAlert(a);
	}

        nc->ignore_errors = TRUE;
    }
}



/************************************************************
 *                   Custom prompter                        *
 ************************************************************/

#define rect_coords(r) r.left, r.top, (r.right - r.left), (r.bottom - r.top)

static void
nc_layout_custom_prompter(HWND hwnd, khui_new_creds_privint_panel * p, BOOL create)
{
    SIZE window;
    SIZE margin;
    SIZE row;
    RECT r_sm_lbl;
    RECT r_sm_input;
    RECT r_lg_lbl;
    RECT r_lg_input;
    SIZE banner;

    HFONT hf = NULL;
    HFONT hf_old = NULL;
    HDC  hdc = NULL;
    HDWP hdwp = NULL;
    HWND hw;

    RECT r;
    SIZE s;
    size_t cch;
    khm_size i;

    int x,y;

    assert(p->use_custom);

    {
        RECT r_row;

        GetWindowRect(hwnd, &r);

        window.cx = r.right - r.left;
        window.cy = r.bottom - r.top;

        GetWindowRect(GetDlgItem(hwnd, IDC_NC_TPL_ROW), &r_row);

        margin.cx = r_row.left - r.left;
        margin.cy = r_row.top - r.top;

        row.cy = r_row.bottom - r_row.top;
        row.cx = window.cx - margin.cx * 2;

        GetWindowRect(GetDlgItem(hwnd, IDC_NC_TPL_LABEL), &r_sm_lbl);
        OffsetRect(&r_sm_lbl, -r_row.left, -r_row.top);

        GetWindowRect(GetDlgItem(hwnd, IDC_NC_TPL_INPUT), &r_sm_input);
        OffsetRect(&r_sm_input, -r_row.left, -r_row.top);
        r_sm_input.right = row.cx;

        GetWindowRect(GetDlgItem(hwnd, IDC_NC_TPL_ROW_LG), &r_row);

        GetWindowRect(GetDlgItem(hwnd, IDC_NC_TPL_LABEL_LG), &r_lg_lbl);
        OffsetRect(&r_lg_lbl, -r_row.left, -r_row.top);

        GetWindowRect(GetDlgItem(hwnd, IDC_NC_TPL_INPUT_LG), &r_lg_input);
        OffsetRect(&r_lg_input, -r_row.left, -r_row.top);
        r_lg_input.right = row.cx;

        banner.cx = row.cx;
        banner.cy = r_sm_lbl.bottom - r_sm_lbl.top;
    }

    x = margin.cx;
    y = margin.cy;

    hf = (HFONT) SendMessage(GetDlgItem(hwnd, IDC_NC_TPL_LABEL), WM_GETFONT, 0, 0);
    hdc = GetDC(hwnd);
    hf_old = SelectFont(hdc, hf);

    if (!create) {
        hdwp = BeginDeferWindowPos((int) (2 + p->n_prompts * 2));
    }

    if (p->banner) {
        StringCchLength(p->banner, KHUI_MAXCCH_BANNER, &cch);
        SetRect(&r, 0, 0, banner.cx - margin.cx, 0);
        DrawText(hdc, p->banner, (int) cch, &r, DT_CALCRECT | DT_WORDBREAK);
        assert(r.bottom > 0);

        r.right = banner.cx;
        r.bottom += margin.cy / 2;
        OffsetRect(&r, x, y);

        if (create) {
            hw = CreateWindow(L"STATIC", p->banner,
                              SS_LEFT | WS_CHILD,
                              r.left, r.top, r.right - r.left, r.bottom - r.top,
                              hwnd, (HMENU) IDC_NCC_BANNER,
                              khm_hInstance, NULL);
            assert(hw != NULL);
            SendMessage(hw, WM_SETFONT, (WPARAM) hf, 0);
        } else {
            hw = GetDlgItem(hwnd, IDC_NCC_BANNER);
            assert(hw);
            hdwp = DeferWindowPos(hdwp, hw, NULL, rect_coords(r), SWP_MOVESIZE);
        }

        y = r.bottom;
    }

    for (i=0; i < p->n_prompts; i++) {
        khui_new_creds_prompt * pr;
        RECT rlbl, rinp;

        pr = p->prompts[i];

        assert(pr);
        if (pr == NULL)
            continue;

        s.cx = 0;
        StringCchLength(pr->prompt, KHUI_MAXCCH_PROMPT, &cch);
        GetTextExtentPoint32(hdc, pr->prompt, (int) cch, &s);
        assert(s.cx > 0);

        if (s.cx < r_sm_lbl.right - r_sm_lbl.left) {
            CopyRect(&rlbl, &r_sm_lbl);
            OffsetRect(&rlbl, x, y);
            CopyRect(&rinp, &r_sm_input);
            OffsetRect(&rinp, x, y);
            y += row.cy;
        } else if (s.cx < r_lg_lbl.right - r_lg_lbl.left) {
            CopyRect(&rlbl, &r_lg_lbl);
            OffsetRect(&rlbl, x, y);
            CopyRect(&rinp, &r_lg_input);
            OffsetRect(&rinp, x, y);
            y += row.cy;
        } else {
            SetRect(&rlbl, x, y, x + banner.cx, y + banner.cy);
            y += banner.cy;
            CopyRect(&rinp, &r_sm_input);
            OffsetRect(&rinp, x, y);
            y += row.cy;
        }

        if (create) {
            assert(pr->hwnd_edit == NULL);
            assert(pr->hwnd_static == NULL);

            pr->hwnd_static = CreateWindow(L"STATIC", pr->prompt,
                                           SS_LEFT | WS_CHILD,
                                           rlbl.left, rlbl.top,
                                           rlbl.right - rlbl.left, rlbl.bottom - rlbl.top,
                                           hwnd,
                                           (HMENU) (IDC_NCC_CTL + i*2),
                                           khm_hInstance, NULL);
            assert(pr->hwnd_static != NULL);
            SendMessage(pr->hwnd_static, WM_SETFONT, (WPARAM) hf, 0);

            pr->hwnd_edit = CreateWindow(L"EDIT", ((pr->def)?pr->def:L""),
                                         ((pr->flags & KHUI_NCPROMPT_FLAG_HIDDEN)?
                                          ES_PASSWORD: 0) | WS_CHILD | WS_TABSTOP |
                                         WS_BORDER,
                                         rinp.left, rinp.top,
                                         rinp.right - rinp.left, rinp.bottom - rinp.top,
                                         hwnd, (HMENU) (IDC_NCC_CTL + 1 + i*2),
                                         khm_hInstance, NULL);
            assert(pr->hwnd_edit != NULL);
            SendMessage(pr->hwnd_edit, WM_SETFONT, (WPARAM) hf, 0);
        } else {
            hw = GetDlgItem(hwnd, (int)(IDC_NCC_CTL + i*2));
            assert(hw != NULL);
            hdwp = DeferWindowPos(hdwp, hw, NULL, rect_coords(rlbl), SWP_MOVESIZE);

            hw = GetDlgItem(hwnd, (int)(IDC_NCC_CTL + 1 + i*2));
            assert(hw != NULL);
            hdwp = DeferWindowPos(hdwp, hw, NULL, rect_coords(rinp), SWP_MOVESIZE);
        }
    }

    SelectFont(hdc, hf_old);
    ReleaseDC(hwnd, hdc);
    if (!create) {
        EndDeferWindowPos(hdwp);
    }
}

static INT_PTR CALLBACK
nc_prompter_dlg_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    khui_new_creds_privint_panel * p;

    switch (uMsg) {
    case WM_INITDIALOG:
        {
            p = (khui_new_creds_privint_panel *) lParam;
            assert(p->magic == KHUI_NEW_CREDS_PRIVINT_PANEL_MAGIC);

#pragma warning(push)
#pragma warning(disable: 4244)
            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) p);
#pragma warning(pop)

            assert(p->hwnd == NULL);
            p->hwnd = hwnd;

            nc_layout_custom_prompter(hwnd, p, TRUE);
        }
        return TRUE;

    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS * wp;

            p = (khui_new_creds_privint_panel *)(LONG_PTR) GetWindowLongPtr(hwnd, DWLP_USER);
            assert(p->magic == KHUI_NEW_CREDS_PRIVINT_PANEL_MAGIC);

            wp = (WINDOWPOS *) lParam;

            if (!(wp->flags & SWP_NOMOVE) ||
                !(wp->flags & SWP_NOSIZE))
                nc_layout_custom_prompter(hwnd, p, FALSE);
        }
        return TRUE;

    case WM_CLOSE:
        {
            DestroyWindow(hwnd);
        }
        return TRUE;

    case WM_DESTROY:
        {
            p = (khui_new_creds_privint_panel *)(LONG_PTR) GetWindowLongPtr(hwnd, DWLP_USER);
            assert(p->magic == KHUI_NEW_CREDS_PRIVINT_PANEL_MAGIC);

            

            p->hwnd = NULL;
        }
        return TRUE;

    default:
        return FALSE;
    }
}


extern "C"
HWND
khm_create_custom_prompter_dialog(khui_new_creds * nc,
                                  HWND parent,
                                  khui_new_creds_privint_panel * p)
{
    HWND hw;

    assert(p->magic == KHUI_NEW_CREDS_PRIVINT_PANEL_MAGIC);
    assert(p->hwnd == NULL);

    p->nc = nc;
    hw = CreateDialogParam(khm_hInstance,
                           MAKEINTRESOURCE(IDD_NC_PROMPTS),
                           parent,
                           nc_prompter_dlg_proc,
                           (LPARAM) p);
    assert(hw != NULL);

    return hw;
}


/************************************************************
 *    Creating and activating the New Credentials Wizard    *
 ************************************************************/

extern "C"
INT_PTR khm_do_modal_newcredwnd(HWND parent, khui_new_creds * c)
{
    wchar_t wtitle[256];

    khui_cw_lock_nc(c);

    if (c->window_title == NULL) {
        size_t t = 0;

        wtitle[0] = L'\0';

        switch (c->subtype) {
        case KHUI_NC_SUBTYPE_ACQPRIV_ID:
            LoadStringResource(wtitle, IDS_WT_ACQ_PRIV_ID);
            break;

        case KHUI_NC_SUBTYPE_IDSPEC:
            LoadStringResource(wtitle, IDS_WT_IDSPEC);
            break;

        default:
            assert(FALSE);
        }

        c->window_title = PWCSDUP(wtitle);
    }
    khui_cw_unlock_nc(c);

    AutoRef<NewCredWizard> ncw(PNEW NewCredWizard(c), RefCount::TakeOwnership);
    INT_PTR rv = ncw->DoModal(parent);

    return rv;
}

extern "C"
HWND khm_create_newcredwnd(HWND parent, khui_new_creds * c)
{
    wchar_t wtitle[256];
    HWND hwnd;

    khui_cw_lock_nc(c);

    if (c->window_title == NULL) {
        wtitle[0] = L'\0';

        switch (c->subtype) {
        case KHUI_NC_SUBTYPE_PASSWORD:
            LoadStringResource(wtitle, IDS_WT_PASSWORD);
            break;

        case KHUI_NC_SUBTYPE_NEW_CREDS:
            LoadStringResource(wtitle, IDS_WT_NEW_CREDS);
            break;

        case KHUI_NC_SUBTYPE_IDSPEC:
            LoadStringResource(wtitle, IDS_WT_IDSPEC);
            break;

        default:
            assert(FALSE);
        }

        c->window_title = PWCSDUP(wtitle);
    }

    khui_cw_unlock_nc(c);

    AutoRef<NewCredWizard> ncw(PNEW NewCredWizard(c), RefCount::TakeOwnership);

    hwnd = ncw->Create(parent);

    assert(hwnd != NULL);

    return hwnd;
}

extern "C"
void khm_prep_newcredwnd(HWND hwnd)
{
    SendMessage(hwnd, KHUI_WM_NC_NOTIFY, 
                MAKEWPARAM(0, WMNC_DIALOG_SETUP), 0);
}

extern "C"
void khm_show_newcredwnd(HWND hwnd)
{
    PostMessage(hwnd, KHUI_WM_NC_NOTIFY, 
                MAKEWPARAM(0, WMNC_DIALOG_ACTIVATE), 0);
}


extern "C"
void khm_register_newcredwnd_class(void)
{
    /* Nothing to do */
}


extern "C"
void khm_unregister_newcredwnd_class(void)
{
    /* Nothing to do */
}
