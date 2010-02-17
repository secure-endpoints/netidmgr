/*
 * Copyright (c) 2008-2010 Secure Endpoints Inc.
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

#ifdef __cplusplus

#include<string>
#include<wchar.h>
#include "Refs.hpp"

namespace nim {

__interface HttpRequestStatusListener {

    void HttpRequestStatus(kherr_severity severity,
                           const wchar_t * status,
                           const wchar_t * long_desc);

    void HttpRequestCompleted(const wchar_t * path);
};

class HttpRequest : public RefCount {
public:
    enum Method {
        ByURL,
        ByGravatar,
        ByFavIcon
    };

private:
    khm_task        m_task;

    std::wstring    m_target;

    Method          m_method;

    std::wstring    m_path;

    volatile HttpRequestStatusListener * m_listener;

    bool            m_muted;

private:
    HttpRequest(const wchar_t * target, Method method,
                HttpRequestStatusListener * listener);

    void ReportStatus(kherr_severity severity,
                      const wchar_t * title,
                      const wchar_t * message,
                      ...);

    void ReportComplete(bool succeeded);

    khm_int32 FetchResource(const wchar_t * domain,
                            const wchar_t * resource,
                            const wchar_t ** mimetypes);

    void MuteReports(bool mute = true);

    bool SetTempPath();

    void FetchImageFromURL();

    void FetchFaviconForDomain();

    void FetchGravatarForEmail();

    khm_int32 ExecuteRequest();

    static khm_int32 __stdcall TaskRunner(void * param);

public:
    ~HttpRequest();

    void Abort();

    static HttpRequest * CreateRequest(const wchar_t * target,
                                       Method method,
                                       HttpRequestStatusListener * listener);
};

}

#endif
