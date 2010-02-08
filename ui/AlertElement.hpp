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

#pragma once

namespace nim {

class AlertCommandButtonElement :
        public WithHostedControl< > {
public:
    void CreateFromCommand(khm_int32 cmd, UINT ctl_id);
    void UpdateLayoutPre(Graphics& g, Rect& layout);
};

class AlertTitleElement :
	public GenericTextT< WithStaticCaption < > >
{};

class AlertMessageElement :
	public GenericTextT< WithStaticCaption < > >
{};

class AlertSuggestionElement :
	public BackgroundT< GenericTextT< WithStaticCaption < > >,
			    &KhmDraw::DrawAlertSuggestBackground >
{};

// forward dcl.
class AlertContextMonitor;

class AlertElement : public WithOutline< WithTabStop< DisplayElement > > {
    AlertTitleElement	*el_title;
    AlertMessageElement	*el_message;
    AlertSuggestionElement	*el_suggestion;
    IconDisplayElement	*el_icon;
    ExposeControlElement	*el_expose;
    ProgressBarElement	*el_progress;

    AlertContextMonitor     *m_monitor;
    bool                    m_err_completed;
    kherr_context           *m_err_context;

    std::vector<AlertCommandButtonElement *> el_buttons;

public:
    AlertElement(Alert& a, int index);

    ~AlertElement();

    bool HasCommands() const {
        return
            (m_alert->n_alert_commands > 1) ||
            (m_alert->n_alert_commands == 1 &&
             m_alert->alert_commands[0] != KHUI_PACTION_CLOSE);
    }

    void SetMonitor(AlertContextMonitor * m);

    void UpdateIcon();

    void UpdateProgress();

    void TryMonitorNewChildContext(kherr_context * c);

    void OnErrCtxEvent(enum kherr_ctx_event e);

    DrawState GetDrawState();

    bool IsMonitored() {
        return m_monitor != NULL;
    }

    HICON IconForAlert();

public:
    Alert                    m_alert;
    int			 m_index;

    virtual void UpdateLayoutPre(Graphics & g, Rect & layout);

    virtual void UpdateLayoutPost(Graphics & g, const Rect & layout);

    virtual void PaintSelf(Graphics &g, const Rect& bounds, const Rect& clip);
};
}
