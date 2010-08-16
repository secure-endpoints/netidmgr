/*
 * Copyright (c) 2009-2010 Secure Endpoints Inc.
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

#define OEMRESOURCE

#include "khmapp.h"
#include "AlertContainer.hpp"

namespace nim {

AlertElement::AlertElement(Alert& a, int index) :
    m_alert(a),
    m_index(index),
    m_monitor(NULL),
    m_err_completed(false),
    m_err_context(NULL),

    el_title(NULL),
    el_message(NULL),
    el_suggestion(NULL),
    el_icon(NULL),
    el_expose(NULL),
    el_progress(NULL)
{
    AutoLock<Alert> a_lock(&m_alert);

    m_alert->displayed = TRUE;
}

AlertElement::~AlertElement()
{
    {
        AutoLock<Alert> a_lock(&m_alert);
        m_alert->displayed = FALSE;
    }

    m_monitor.Attach(NULL);
}

// alert must be locked
HICON AlertElement::IconForAlert()
{
    if ((m_alert->alert_type == KHUI_ALERTTYPE_PROGRESS ||
         m_alert->alert_type == KHUI_ALERTTYPE_PROGRESSACQ) &&
        m_alert->severity >= KHERR_INFO &&
        (m_alert->flags & KHUI_ALERT_FLAG_VALID_ERROR)) {

        return LoadIconResource(m_err_completed ? IDI_CHECK : IDI_CLOCK);
    } else {
        return LoadIconResource(
                                (m_alert->severity == KHERR_ERROR)? OIC_HAND :
                                (m_alert->severity == KHERR_WARNING)? OIC_BANG :
                                OIC_NOTE,
                                false, true, NULL
                                );
    }
}

DrawState AlertElement::GetDrawState()
{
    return (DrawState)(0 |
                       (selected ? DrawStateSelected : DrawStateNone) |
                       (highlight ? DrawStateHotTrack : DrawStateNone) |
                       (focus ? DrawStateFocusRect : DrawStateNone));
}

void AlertElement::UpdateLayoutPre(Graphics & g, Rect & layout)
{
    AutoLock<Alert> a_lock(&m_alert);
    bool was_modified = !!(m_alert->flags & KHUI_ALERT_FLAG_MODIFIED);

    if (!el_title) {
        el_title = PNEW AlertTitleElement();
        InsertChildAfter(el_title);
    }

    if (m_alert->title && was_modified)
        el_title->SetCaption(std::wstring(m_alert->title));

    if (!el_message) {
        el_message = PNEW AlertMessageElement();
        InsertChildAfter(el_message);
    }

    if (m_alert->message && was_modified)
        el_message->SetCaption(std::wstring(m_alert->message));

    if (!el_suggestion) {
        el_suggestion = PNEW AlertSuggestionElement();
        InsertChildAfter(el_suggestion);
    }

    if (m_alert->suggestion && was_modified)
        el_suggestion->SetCaption(std::wstring(m_alert->suggestion));

    if (m_alert->flags & KHUI_ALERT_FLAG_VALID_ERROR) {

        if (!el_progress) {
	    khm_ui_4 p = 0;
	    if (m_err_context) {
		khm_ui_4 num, denom;
		kherr_get_progress_i(m_err_context, &num, &denom);
		p = (denom != 0)? num * 256 / denom : 0;
	    }
            el_progress = PNEW ProgressBarElement();
            InsertChildAfter(el_progress);
            UpdateProgress(p);
        }

        if (!el_expose) {
            el_expose = PNEW ExposeControlElement();
            InsertChildAfter(el_expose);
        }
    }

    if (!el_icon) {
        el_icon = PNEW IconDisplayElement(g_theme->pt_margin_cx + g_theme->pt_margin_cy,
                                          IconForAlert(),
                                          true /* Large */);
        InsertChildAfter(el_icon);
    }

    if (el_buttons.size() == 0 && HasCommands()) {
        AutoLock<Alert> a_lock(&m_alert);

        for (int i = 0; i < m_alert->n_alert_commands; i++) {
            if (m_alert->alert_commands[i] != KHUI_PACTION_CLOSE) {
                AlertCommandButtonElement * e = PNEW AlertCommandButtonElement();

                InsertChildAfter(e);
                el_buttons.push_back(e);

                e->CreateFromCommand(m_alert->alert_commands[i],
                                     AlertContainer::ControlIdFromIndex(m_index, i));
            }
        }
    }

    extents.Height = 0;
    extents.Width = layout.Width;

    {
        Size sz_icon = g_theme->sz_margin + g_theme->sz_icon + g_theme->sz_margin;

        layout.X += sz_icon.Width;
        layout.Width -= sz_icon.Width;
    }
}

void AlertElement::UpdateLayoutPost(Graphics & g, const Rect & bounds)
{
    int left_indent = g_theme->sz_margin.Width * 2 + g_theme->sz_icon.Width;
    bool has_children = false;

    if (el_expose) {
        for (DisplayElement * e = TQFIRSTCHILD(this); e; e = TQNEXTSIBLING(e)) {
            AlertElement * ae = dynamic_cast<AlertElement *>(e);

            if (ae) {
                has_children = true;
                break;
            }
        }
    }

    FlowLayout layout(Rect(Point(0,0),
                           Size(bounds.Width + left_indent, 0)),
                      g_theme->sz_margin);

    layout
        .PushIndent(left_indent - g_theme->sz_margin.Width)
        .LineBreak()
        .Add(el_title)
        .LineBreak()
        .Add(el_message)
        .LineBreak()
        .Add(el_suggestion)
        .LineBreak()
        .Add(el_progress, !m_err_completed)
        .LineBreak()
        .PopIndent()
        .PushIndent(g_theme->sz_margin.Width + g_theme->sz_icon.Width - g_theme->sz_icon_sm.Width)
        .LineBreak()
        .Add(el_expose, FlowLayout::Left, FlowLayout::Fixed, has_children)
        .PopIndent()
        .PushIndent(g_theme->sz_margin.Width + g_theme->sz_icon.Width)
        ;

    for (std::vector<AlertCommandButtonElement *>::iterator i = el_buttons.begin();
         i != el_buttons.end(); ++i) {
        layout.Add(*i);
    }
    layout.LineBreak();

    for (DisplayElement * e = TQFIRSTCHILD(this); e;
         e = TQNEXTSIBLING(e)) {
        AlertElement * ca = dynamic_cast<AlertElement *>(e);

        if (!ca || !ca->visible)
            continue;

        layout.Add(e).LineBreak();
    }

    extents = layout.GetSize();
    extents.Height = max(extents.Height, g_theme->sz_margin.Height * 2 + g_theme->sz_icon.Height);
}

void AlertElement::UpdateIcon()
{
    if (el_icon)
        el_icon->SetIcon(IconForAlert());
}

void AlertElement::UpdateProgress(khm_ui_4 norm_progress)
{
    if (el_progress)
        el_progress->SetProgress(norm_progress);
    else {
        MarkForExtentUpdate();
        Invalidate();
    }

    if (norm_progress >= 256) {
        m_err_completed = true;
        UpdateIcon();
    }
}

void AlertElement::SetMonitor(AlertContextMonitor * m)
{
    assert (m_monitor.IsNull() || m == NULL);
    m_monitor.Attach(m);
    m_err_context = m_alert->err_context;
    assert(m_err_context != NULL || m == NULL);
}

void AlertElement::TryMonitorNewChildContext(kherr_context * c)
{
    for (DisplayElement * de = TQFIRSTCHILD(this); de;
         de = TQNEXTSIBLING(de)) {
        AlertElement * ae = dynamic_cast<AlertElement *>(de);

        if (ae == NULL)
            continue;

        {
            AutoLock<Alert> a_lock(&ae->m_alert);

            if (kherr_context_is_equal(c, ae->m_err_context))
                return;	// Already there
        }
    }

    // Wasn't there.  We need to create a new alert element and
    // add it to this element

    khui_alert * a = NULL;

    khui_alert_create_empty(&a);
    khui_alert_set_type(a, KHUI_ALERTTYPE_PROGRESSACQ);

    Alert alert(a, true);
    AlertElement * ae = PNEW AlertElement(alert, -1);

    InsertOutlineAfter(ae);

    khui_alert_monitor_progress(a, c,
                                KHUI_AMP_SHOW_EVT_ERR |
                                KHUI_AMP_SHOW_EVT_WARN);

    dynamic_cast<AlertContainer *>(owner)->BeginMonitoringAlert(alert);
}

void AlertElement::OnErrCtxEvent()
{
    AlertContextMonitor::ContextEvent * event = NULL;

    while (!m_monitor.IsNull() &&
	   (event = m_monitor->NextEvent()) != NULL) {
#ifdef DEBUG
	kherr_debug_printf(L"AlertElement %p received event %s\n",
			   (void *) this,
			   (event->e == KHERR_CTX_BEGIN)? L"BEGIN":
			   (event->e == KHERR_CTX_DESCRIBE)? L"DESCRIBE":
			   (event->e == KHERR_CTX_END)? L"END":
			   (event->e == KHERR_CTX_ERROR)? L"ERROR":
			   (event->e == KHERR_CTX_EVTCOMMIT)? L"EVTCOMMIT":
			   (event->e == KHERR_CTX_NEWCHILD)? L"NEWCHILD":
			   (event->e == KHERR_CTX_FOLDCHILD)? L"FOLDCHILD":
			   (event->e == KHERR_CTX_PROGRESS)? L"PROGRESS":
			   L"(*UNKNOWN*)");
#endif

	switch (event->e) {
	case KHERR_CTX_DESCRIBE:
        {
            AutoLock<Alert> a_lock(&m_alert);
	    AlertContextMonitor::ContextDescribeEvent * ev =
		static_cast<AlertContextMonitor::ContextDescribeEvent *>(event);

	    khui_alert_set_title(m_alert, ev->description.c_str());
            MarkForExtentUpdate();
            Invalidate();
        }
        break;

	case KHERR_CTX_END:
        {
            m_err_completed = true;
            m_err_context = NULL;
	    m_monitor.Attach(NULL);
	    MarkForExtentUpdate();
	    Invalidate();
            UpdateIcon();
        }
        break;

	case KHERR_CTX_EVTCOMMIT:
	case KHERR_CTX_ERROR:
        {
            AutoLock<Alert> a_lock(&m_alert);
	    AlertContextMonitor::ContextCommitEvent * ev =
		static_cast<AlertContextMonitor::ContextCommitEvent *>(event);

            if (ev->severity > m_alert->severity)
                break;

            if (((m_alert->monitor_flags & KHUI_AMP_SHOW_EVT_ERR) &&
                 ev->severity <= KHERR_ERROR) ||

                ((m_alert->monitor_flags & KHUI_AMP_SHOW_EVT_WARN) &&
                 ev->severity <= KHERR_WARNING) ||

                ((m_alert->monitor_flags & KHUI_AMP_SHOW_EVT_INFO) &&
                 ev->severity <= KHERR_INFO) ||

                ((m_alert->monitor_flags & KHUI_AMP_SHOW_EVT_DEBUG) &&
                 ev->severity <= KHERR_DEBUG_3)) {

                khui_alert_set_severity(m_alert, ev->severity);

		khui_alert_set_message(m_alert, ev->description.c_str());

		khui_alert_set_suggestion(m_alert, ev->suggestion.c_str());

                UpdateIcon();
                MarkForExtentUpdate();
                Invalidate();
            }
        }
        break;

	case KHERR_CTX_NEWCHILD:
        {
	    AutoLock<Alert> a_lock(&m_alert);
	    kherr_context * c;

	    c = kherr_get_first_context(m_err_context);

            for (; c ; c = kherr_get_next_context(c)) {
                TryMonitorNewChildContext(c);
            }
        }
        break;

	case KHERR_CTX_PROGRESS:
        {
	    AlertContextMonitor::ContextProgressEvent * ev =
		static_cast<AlertContextMonitor::ContextProgressEvent *>(event);

#ifdef DEBUG
	    kherr_debug_printf(L"  Progress value: %d\n", (int) ev->progress);
#endif
            UpdateProgress(ev->progress);
        }
        break;
	}

	delete event;
    }
}

void AlertElement::PaintSelf(Graphics &g, const Rect& bounds, const Rect& clip)
{
    g_theme->DrawAlertBackground(g, bounds, GetDrawState());
}

void AlertCommandButtonElement::CreateFromCommand(khm_int32 cmd, UINT ctl_id)
{
    assert (owner);
    assert (owner->hwnd);

    HWND hwnd_button;
    wchar_t caption[KHUI_MAXCCH_SHORT_DESC] = L"";

    khm_get_action_caption(cmd, caption, sizeof(caption));

    hwnd_button = CreateWindow(L"BUTTON",
                               caption,
                               WS_TABSTOP | WS_CHILD | BS_PUSHBUTTON | BS_NOTIFY,
                               0, 0, 100, 100, // Dummy x,y,width,height
                               owner->hwnd,
                               (HMENU)(UINT_PTR) ctl_id,
                               khm_hInstance,
                               NULL);
    SendMessage(hwnd_button, WM_SETFONT, (WPARAM) g_theme->hf_normal, FALSE);
    SetWindow(hwnd_button);
}

void AlertCommandButtonElement::UpdateLayoutPre(Graphics& g, Rect& layout)
{
    HWND hwnd_button;

    hwnd_button = GetWindow();

    if (hwnd_button) {
        wchar_t caption[KHUI_MAXCCH_SHORT_DESC] = L"";
        HFONT hf;
        Font * font;

        GetWindowText(hwnd_button, caption, ARRAYLENGTH(caption));
        hf = (HFONT) SendMessage(hwnd_button, WM_GETFONT, 0, 0);
        if (hf == NULL)
            hf = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
        {
            HDC hdc = g.GetHDC();
            font = new Font(hdc, hf);
            g.ReleaseHDC(hdc);
        }

        StringFormat fmt(StringFormatFlagsNoWrap);
        RectF brect;

        g.MeasureString(caption, -1, font, PointF(0.0, 0.0), &fmt, &brect);

        int bwidth = (int) brect.Width + g_theme->sz_margin.Width * 2;
        int bheight = (int) brect.Height + g_theme->sz_margin.Height * 2;

        SetWindowPos(hwnd_button, NULL, 0, 0,
                     bwidth, bheight, SWP_SIZEONLY);

        delete font;
    }
}
}
