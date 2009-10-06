#define OEMRESOURCE

#include "khmapp.h"
#include "AlertContainer.hpp"

namespace nim {

    AlertElement::AlertElement(Alert& a, int index) :
	m_alert(a),
	m_index(index),
	m_monitor(NULL),

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

	 if (m_monitor)
	     delete m_monitor;
     }

    // alert must be locked
    static HICON IconForAlert(Alert & alert)
    {
	if ((alert->alert_type == KHUI_ALERTTYPE_PROGRESS ||
	     alert->alert_type == KHUI_ALERTTYPE_PROGRESSACQ) &&
	    alert->severity >= KHERR_INFO &&
	    alert->err_context) {

	    khm_ui_4 num, denom;

	    kherr_get_progress_i(alert->err_context, &num, &denom);

	    return LoadIconResource((denom != 0 && num >= denom) ? IDI_CHECK : IDI_CLOCK);
	} else {
	    return LoadIconResource(
				    (alert->severity == KHERR_ERROR)? OIC_HAND :
				    (alert->severity == KHERR_WARNING)? OIC_BANG :
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

	if (!el_title) {
	    el_title = new AlertTitleElement();
	    if (m_alert->title)
		el_title->SetCaption(std::wstring(m_alert->title));
	    InsertChildAfter(el_title);
	}

	if (!el_message) {
	    el_message = new AlertMessageElement();
	    if (m_alert->message)
		el_message->SetCaption(std::wstring(m_alert->message));
	    InsertChildAfter(el_message);
	}

	if (!el_suggestion) {
	    el_suggestion = new AlertSuggestionElement();
	    if (m_alert->suggestion)
		el_suggestion->SetCaption(std::wstring(m_alert->suggestion));
	    InsertChildAfter(el_suggestion);
	}

	if (m_alert->err_context) {

	    if (!el_progress) {
		el_progress = new ProgressBarElement();
		InsertChildAfter(el_progress);
	    }

	    if (!el_expose) {
		el_expose = new ExposeControlElement();
		InsertChildAfter(el_suggestion);
	    }
	}

	if (!el_icon) {
	    el_icon = new IconDisplayElement(g_theme->pt_margin_cx + g_theme->pt_margin_cy,
					     IconForAlert(m_alert),
					     true /* Large */);
	    InsertChildAfter(el_icon);
	}

        if (el_buttons.size() == 0 && HasCommands()) {
            AutoLock<Alert> a_lock(&m_alert);

            for (int i = 0; i < m_alert->n_alert_commands; i++) {
                if (m_alert->alert_commands[i] != KHUI_PACTION_CLOSE) {
                    AlertCommandButtonElement * e = new AlertCommandButtonElement();

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

        FlowLayout layout(Rect(Point(0,0),
                               Size(bounds.Width + left_indent, 0)),
                          g_theme->sz_margin);

        layout
            .PushIndent(left_indent - g_theme->sz_margin.Width)
            .LineBreak()
            .Add(el_title, FlowLayout::Left, FlowLayout::Fixed, el_title != NULL)
            .LineBreak()
            .Add(el_message, FlowLayout::Left, FlowLayout::Fixed, el_message != NULL)
            .LineBreak()
            .Add(el_suggestion, FlowLayout::Left, FlowLayout::Fixed, el_suggestion != NULL)
            .LineBreak()
            .Add(el_progress, FlowLayout::Left, FlowLayout::Fixed, el_progress != NULL)
            .LineBreak()
            .PopIndent()
            .PushIndent(g_theme->sz_margin.Width + g_theme->sz_icon.Width - g_theme->sz_icon_sm.Width)
            .Add(el_expose, FlowLayout::Left, FlowLayout::Fixed, el_expose != NULL)
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
    }

    void AlertElement::UpdateIcon()
    {
    }

    void AlertElement::SetMonitor(AlertContextMonitor * m)
    {
	m_monitor = m;
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

		if (kherr_context_is_equal(c, ae->m_alert->err_context))
		    return;	// Already there
	    }
	}

	// Wasn't there.  We need to create a new alert element and
	// add it to this element

	khui_alert * a = NULL;

	khui_alert_create_empty(&a);
	khui_alert_set_type(a, KHUI_ALERTTYPE_PROGRESSACQ);

	Alert alert(a, true);
	AlertElement * ae = new AlertElement(alert, -1);

	InsertChildAfter(ae);

	khui_alert_monitor_progress(a, c,
				    KHUI_AMP_SHOW_EVT_ERR |
				    KHUI_AMP_SHOW_EVT_WARN);
    }

    void AlertElement::OnErrCtxEvent(enum kherr_ctx_event e)
    {
	switch (e) {
	case KHERR_CTX_DESCRIBE:
	    {
		AutoLock<Alert> a_lock(&m_alert);
		kherr_event * ev;

		ev = kherr_get_desc_event(m_alert->err_context);
		if (ev) {
		    if (ev->short_desc && ev->long_desc) {
			el_title->SetCaption(ev->long_desc);
			el_message->SetCaption(ev->short_desc);
		    } else if (ev->long_desc) {
			el_title->SetCaption(ev->long_desc);
		    } else if (ev->short_desc) {
			el_title->SetCaption(ev->short_desc);
		    }
		}
	    }
	    break;

	case KHERR_CTX_END:
	    {
		UpdateIcon();
	    }
	    break;

	case KHERR_CTX_EVTCOMMIT:
	case KHERR_CTX_ERROR:
	    {
		AutoLock<Alert> a_lock(&m_alert);
		kherr_event * ev;

		ev = kherr_get_last_event(m_alert->err_context);

		if (((m_alert->monitor_flags & KHUI_AMP_SHOW_EVT_ERR) &&
		     ev->severity <= KHERR_ERROR) ||

		    ((m_alert->monitor_flags & KHUI_AMP_SHOW_EVT_WARN) &&
		     ev->severity <= KHERR_WARNING) ||

		    ((m_alert->monitor_flags & KHUI_AMP_SHOW_EVT_INFO) &&
		     ev->severity <= KHERR_INFO) ||

		    ((m_alert->monitor_flags & KHUI_AMP_SHOW_EVT_DEBUG) &&
		     ev->severity <= KHERR_DEBUG_3)) {

		    khui_alert_set_severity(m_alert, ev->severity);

		    if (!(ev->flags & KHERR_RF_STR_RESOLVED))
			kherr_evaluate_event(ev);

		    if (ev->long_desc) {
			el_message->SetCaption(ev->long_desc);
		    } else if (ev->short_desc) {
			el_message->SetCaption(ev->short_desc);
		    }

		    UpdateIcon();
		}
	    }
	    break;

	case KHERR_CTX_NEWCHILD:
	    {
		kherr_context * c;

		{
		    AutoLock<Alert> a_lock(&m_alert);
		    c = kherr_get_first_context(m_alert->err_context);
		}

		for (; c ; c = kherr_get_next_context(c)) {
		    TryMonitorNewChildContext(c);
		}
	    }
	    break;

	case KHERR_CTX_PROGRESS:
	    {
		khm_ui_4 num, denom;
		int norm_progress;

		{
		    AutoLock<Alert> a_lock(&m_alert);
		    kherr_get_progress_i(m_alert->err_context, &num, &denom);
		}

		if (denom == 0)
		    norm_progress = 0;
		else if (num > denom)
		    norm_progress = 256;
		else
		    norm_progress = (num * 256) / denom;

		el_progress->SetProgress(norm_progress);
	    }
	    break;
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
        }
    }
}
