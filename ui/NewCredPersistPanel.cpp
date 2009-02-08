
#include "khmapp.h"

namespace nim {

    void NewCredPersistPanel::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
    {
        khui_new_creds * nc = parent->nc;

        if (id == IDC_PERSIST && codeNotify == BN_CLICKED) {
            khui_new_creds_by_type * t = NULL;
            khm_int32 ks_type = KCDB_CREDTYPE_INVALID;
            khm_size cb;

            nc->persist_privcred = (!!IsDlgButtonChecked(hwnd, IDC_PERSIST) == BST_CHECKED);
            cb = sizeof(ks_type);
            kcdb_identity_get_attr(nc->persist_identity, KCDB_ATTR_TYPE, NULL, &ks_type, &cb);
            khui_cw_find_type(nc, ks_type, &t);
            if (t != NULL && t->hwnd_panel != NULL) {
                SendMessage(t->hwnd_panel, KHUI_WM_NC_NOTIFY,
                            MAKEWPARAM(0, WMNC_COLLECT_PRIVCRED), 0);
            }
        }
    }
}
