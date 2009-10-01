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

	extents.Height = 0;
	extents.Width = layout.Width;

	{
	    Size sz_icon = g_theme->sz_margin + g_theme->sz_icon + g_theme->sz_margin;

	    layout.X += sz_icon.Width;
	    layout.Width -= sz_icon.Width;
	}
    }

    void AlertElement::UpdateLayoutPost(Graphics & g, const Rect & layout)
    {
	Point pos = g_theme->pt_margin_cx + g_theme->pt_margin_cx + g_theme->pt_margin_cy +
	    g_theme->pt_icon_cx;

	if (el_title) {
	    el_title->origin = pos;
	    pos.Y += el_title->extents.Height + g_theme->sz_margin.Height;
	}

	if (el_message) {
	    el_message->origin = pos;
	    pos.Y += el_message->extents.Height + g_theme->sz_margin.Height;
	}

	if (el_suggestion) {
	    el_suggestion->origin = pos;
	    pos.Y += el_suggestion->extents.Height + g_theme->sz_margin.Height;
	}

	if (el_progress) {
	    el_progress->origin = pos;
	    pos.Y += el_progress->extents.Height + g_theme->sz_margin.Height;
	}

	if (el_expose) {
	    el_expose->origin = pos - (g_theme->pt_margin_cx +
				       g_theme->pt_icon_sm_cx +
				       g_theme->pt_margin_cy +
				       g_theme->pt_icon_sm_cy);
	}

	for (DisplayElement * e = TQFIRSTCHILD(this); e;
	     e = TQNEXTSIBLING(e)) {
	    AlertElement * ca = dynamic_cast<AlertElement *>(e);

	    if (!ca || !ca->visible)
		continue;

	    ca->origin = pos;
	    pos.Y += ca->extents.Height;
	}

	extents.Height = pos.Y;
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
}
