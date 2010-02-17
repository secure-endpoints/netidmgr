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

#include "PictureCropWindow.hpp"
#include "HttpRequest.hpp"

namespace nim {

class IconSelectDialog :
        virtual public DialogWindow,
        public HttpRequestStatusListener {

    Identity                   m_identity;
    AutoRef<PictureCropWindow> m_cropper;
    AutoRef<HttpRequest>       m_request;

    UINT                       m_msg_edit;
    // Edit control that receives status messages from the HttpRequest
    // object

    enum {
        MAXCCH_URL = 2048
    };

public:
    IconSelectDialog(Identity&);

    ~IconSelectDialog();

    bool PrepareIdentityIconDirectory(wchar_t path[MAX_PATH]);

    void DoBrowse();

    void DoFetchURL();

    void DoFetchFavicon();

    void DoFetchGravatar();

    void DoOk();

    void SetRequest(HttpRequest * req, UINT msg_target);

    virtual BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);

    virtual void OnClose() {
        EndDialog(1);
    }

    virtual void OnCommand(int id, HWND hwndCtl, UINT codeNotify);

    virtual void OnDestroy(void);

    virtual LRESULT OnHelp(HELPINFO * info);

    virtual LRESULT OnNotify(int id, NMHDR * pnmh);

    virtual void HttpRequestStatus(kherr_severity severity,
                                   const wchar_t * status,
                                   const wchar_t * long_desc);

    virtual void HttpRequestCompleted(const wchar_t * path);
};

}
