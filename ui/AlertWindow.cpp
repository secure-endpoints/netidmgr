#include "khmapp.h"
#include "AlertWindow.hpp"

namespace nim {

    BOOL AlertWindow::OnInitDialog(HWND hwndFocus, LPARAM lParam)
    {
	HWND hw_container = GetItem(IDC_CONTAINER);
	RECT r_container;

	GetWindowRect(hw_container, &r_container);
	MapWindowRect(NULL, hwnd, &r_container);
	m_alerts->Create(hwnd, RectFromRECT(&r_container));

	return FALSE;
    }

    void AlertWindow::SetOwnerList(AlertWindowList * _owner)
    {
    }

    void AlertWindow::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
    {
	if (codeNotify == BN_CLICKED) {
	    if (id == KHUI_PACTION_NEXT ||
		id == IDCANCEL ||
		id == IDOK) {

		EndDialog(id);
	    } else {

		m_alerts->ProcessCommand(id);
	    }
	}
    }

    LRESULT AlertWindow::OnHelp(HELPINFO * info)
    {
	DoDefault();
	return 0;
    }
}
