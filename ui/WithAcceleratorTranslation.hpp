#pragma once

namespace nim {

    // Applies to DisplayContainer
    template<class T = DisplayContainer>
    class NOINITVTABLE WithAcceleratorTranslation : public T {
	virtual UINT OnGetDlgCode(LPMSG pMsg) {
	    if (pMsg != NULL) {
		if (TranslateAccelerator(pMsg))
		    return DLGC_WANTMESSAGE;
		else
		    return 0;
	    } else {
		return DLGC_WANTMESSAGE;
	    }
	}

	virtual bool TranslateAccelerator(LPMSG pMsg) {
	    return false;
	}
    };

}
