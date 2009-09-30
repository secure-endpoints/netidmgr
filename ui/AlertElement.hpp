#pragma once

namespace nim {

    class AlertTitleElement :
	public GenericTextT< WithStaticCaption < > > {
    };

    class AlertMessageElement :
	public GenericTextT< WithStaticCaption < > > {
    };

    class AlertSuggestionElement :
	public GenericTextT< WithStaticCaption < > > {
    };

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

	void TryMonitorNewChildContext(kherr_context * c);

	void OnErrCtxEvent(enum kherr_ctx_event e);

    public:
	Alert                    m_alert;
	int			 m_index;

	virtual void UpdateLayoutPre(Graphics & g, Rect & layout);

	virtual void UpdateLayoutPost(Graphics & g, const Rect & layout);

        virtual void PaintSelf(Graphics &g, const Rect& bounds, const Rect& clip);
    };
}