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

#pragma once

namespace nim {

class AlertContextMonitor : public RefCount {

public:
    class ContextEvent {
    public:
	kherr_ctx_event e;
	LDCL(ContextEvent);

	ContextEvent(kherr_ctx_event_data * d) {
	    e = d->event;
	}
    };

    class ContextDescribeEvent : public ContextEvent {
    public:
	std::wstring description;

	ContextDescribeEvent(kherr_ctx_event_data * d) :
	    ContextEvent(d),
	    description((d->data.event->long_desc)?
			d->data.event->long_desc:
			d->data.event->short_desc)
	    {}
    };

    class ContextNewChildEvent : public ContextEvent {
    public:
	kherr_serial new_child_serial;

	ContextNewChildEvent(kherr_ctx_event_data * d) :
	    ContextEvent(d),
	    new_child_serial(d->data.child_ctx->serial)
	    {}
    };

    class ContextProgressEvent : public ContextEvent {
    public:
	khm_ui_4 progress;

	ContextProgressEvent(kherr_ctx_event_data * d) :
	    ContextEvent(d), progress(d->data.progress)
	    {}
    };

    class ContextCommitEvent: public ContextEvent {
    public:
	kherr_severity severity;
	DWORD thread_id;

	std::wstring description;
	std::wstring short_description;
	std::wstring suggestion;

	const wchar_t * facility;
	const wchar_t * location;

	kherr_suggestion suggestion_id;
	khm_int32 facility_id;
	int		 flags;

	ContextCommitEvent(kherr_ctx_event_data * d) :
	    ContextEvent(d),
	    severity(d->data.event->severity),
	    thread_id(d->data.event->thread_id),
	    description((d->data.event->long_desc)?
			d->data.event->long_desc:
			(d->data.event->short_desc)?
			d->data.event->short_desc: L""),
	    short_description((d->data.event->long_desc)?
			      ((d->data.event->short_desc)?
			       d->data.event->short_desc:L""):
			      L""),
	    suggestion((d->data.event->suggestion)?
		       d->data.event->suggestion: L""),
	    facility((d->data.event->facility)?
		     d->data.event->facility: L""),
	    location((d->data.event->location)?
		     d->data.event->location: L""),
	    suggestion_id(d->data.event->suggestion_id),
	    facility_id(d->data.event->facility_id),
	    flags(d->data.event->flags)
	    {}
    };

private:
    AutoRef<ControlWindow>	listener;
    AlertElement		*element;
    UINT			controlID;
    kherr_serial                serial;
    CRITICAL_SECTION            cs;

    QDCL(ContextEvent);

    static void KHMAPI
    ErrorContextCallback(enum kherr_ctx_event e,
                         kherr_ctx_event_data * d,
                         void * vparam);

    bool ContextEventFromData(kherr_ctx_event_data *);

public:
    AlertContextMonitor(AlertElement * _e, ControlWindow *_cw, UINT controlID);

    ~AlertContextMonitor();

    ContextEvent * NextEvent();

};

}
