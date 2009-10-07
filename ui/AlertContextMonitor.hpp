#pragma once

namespace nim {

    class AlertContextMonitor {
	AutoRef<ControlWindow>	listener;
	AlertElement		*element;
	UINT			controlID;
        kherr_context           *context;

    public:
	AlertContextMonitor(AlertElement * _e, ControlWindow *_cw, UINT controlID);

	~AlertContextMonitor();

	static void KHMAPI
	ErrorContextCallback(enum kherr_ctx_event e,
			     kherr_context * c,
			     void * vparam);
    };
}
