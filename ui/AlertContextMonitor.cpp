/*
 * Copyright (c) 2009 Secure Endpoints Inc.
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

#include "khmapp.h"
#include "AlertContainer.hpp"
#include "AlertContextMonitor.hpp"

namespace nim {

    AlertContextMonitor::AlertContextMonitor(AlertElement * _e, ControlWindow *_cw, UINT _controlID):
	element(_e), listener(_cw), controlID(_controlID) {

	kherr_event * ev;
	khm_int32 evt_flags;

	Alert& a = element->m_alert;
	AutoLock<Alert> a_lock(&a);

	if (a->err_context == NULL) {
	    delete this;
	    return;
	}

	evt_flags =
	    KHERR_CTX_END |
	    KHERR_CTX_DESCRIBE |
	    KHERR_CTX_NEWCHILD |
	    KHERR_CTX_PROGRESS;

	if (a->monitor_flags & KHUI_AMP_SHOW_EVT_ERR)
	    evt_flags |= KHERR_CTX_ERROR;

	if (a->monitor_flags & (KHUI_AMP_SHOW_EVT_WARN |
				KHUI_AMP_SHOW_EVT_INFO |
				KHUI_AMP_SHOW_EVT_DEBUG))
	    evt_flags |= KHERR_CTX_EVTCOMMIT;

	kherr_add_ctx_handler_param(ErrorContextCallback,
				    evt_flags,
				    a->err_context->serial, (void *) this);

	ev = kherr_get_desc_event(a->err_context);
	if (ev) {
	    if (ev->short_desc && ev->long_desc) {
		khui_alert_set_title(a, ev->long_desc);
		khui_alert_set_message(a, ev->short_desc);
	    } else if (ev->long_desc) {
		khui_alert_set_title(a, ev->long_desc);
	    } else if (ev->short_desc) {
		khui_alert_set_title(a, ev->short_desc);
	    }
	}

	element->SetMonitor(this);

        context = a->err_context;
        kherr_release_context(a->err_context);
        a->err_context = NULL;
    }

    AlertContextMonitor::~AlertContextMonitor()
    {
	Alert& a = element->m_alert;
	AutoLock<Alert> a_lock(&a);

	if (context) {
	    kherr_remove_ctx_handler_param(ErrorContextCallback, context->serial,
					   (void *) this);
	}

	element->SetMonitor(NULL);
    }

    void KHMAPI
    AlertContextMonitor::ErrorContextCallback(enum kherr_ctx_event e,
					      kherr_context * c,
					      void * vparam)
    {
	AlertContextMonitor * m;

	m = static_cast<AlertContextMonitor *>(vparam);
	assert(m);

	m->listener->PostMessage(WM_COMMAND,
				 MAKEWPARAM(m->controlID, e),
				 (LPARAM) m->element);
	if (e == KHERR_CTX_END) {
	    delete m;
	}
    }
}
