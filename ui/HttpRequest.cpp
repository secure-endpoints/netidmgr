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

#include "khmapp.h"
#include <winhttp.h>
#include "HttpRequest.hpp"
#include <stdarg.h>
#include <strsafe.h>

namespace nim {

static const wchar_t * icon_mimetypes[] = {
    L"image/vnd.microsoft.icon",
    L"image/ico",
    L"image/icon",
    L"image/x-icon",
    L"text/ico",
    NULL
};

static const wchar_t * jpg_mimetypes[] = {
    L"image/jpg",
    L"image/jpeg",
    NULL
};

static const wchar_t * image_mimetypes[] = {
    L"image/jpeg",
    L"image/png",
    L"image/gif",
    L"image/tiff",
    L"image/vnd.microsoft.icon",
    L"image/ico",
    L"image/icon",
    L"image/x-icon",
    L"text/ico",
    NULL
};

struct content_type_to_extension {
    const wchar_t * content_type;
    const wchar_t * extension;
} content_type_map[] = {
    { L"image/jpeg", L".JPG" },
    { L"image/png", L".PNG" },
    { L"image/gif", L".GIF" },
    { L"image/tiff", L".TIFF" },
    { L"image/vnd.microsoft.icon", L".ICO" },
    { L"image/ico", L".ICO" },
    { L"image/icon", L".ICO" },
    { L"image/x-icon", L".ICO" },
    { L"text/ico", L".ICO" }
};

#define MAX_ICON_SIZE (128*1024)

#define USER_AGENT L"Network Identity Manager"

static std::wstring
GetLastErrorString()
{
    DWORD gle = GetLastError();
    wchar_t es[1024];

    if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, gle, 0,
                      es, ARRAYLENGTH(es),
                      NULL) != 0) {
        return std::wstring(es);
    } else {
        return std::wstring(L"");
    }
}

void
HttpRequest::Abort()
{
    InterlockedExchangePointer((volatile PVOID *) &m_listener, NULL);
}

void
HttpRequest::ReportStatus(kherr_severity severity,
                          const wchar_t * message,
                          const wchar_t * long_desc, ...)
{
    HttpRequestStatusListener * listener;
    va_list args;
    DWORD gle;

    va_start(args, long_desc);
    gle = GetLastError();

    listener = (HttpRequestStatusListener *) m_listener;

    if (listener && !m_muted) {
        wchar_t ld[2048];

        if (long_desc)
            StringCchVPrintf(ld, ARRAYLENGTH(ld), long_desc, args);
        listener->HttpRequestStatus(severity, message, ((long_desc)? ld: NULL));
    }
}

void
HttpRequest::ReportComplete(bool succeeded)
{
    HttpRequestStatusListener * listener;

    listener = (HttpRequestStatusListener *) m_listener;

    if (listener && (!m_muted || succeeded)) {
        listener->HttpRequestCompleted((succeeded)? m_path.c_str() : NULL);
        Abort();
    }
}

khm_int32
HttpRequest::FetchResource(const wchar_t * domain,
                           const wchar_t * resource,
                           const wchar_t ** mimetypes)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    DWORD nTotalBytes = 0;
    BOOL bContinue = TRUE;
    khm_int32 rv = KHM_ERROR_GENERAL;

    hSession = WinHttpOpen(USER_AGENT, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS,
                           0);

    if (hSession == NULL) {
        ReportStatus(KHERR_ERROR,
                     L"Can't create HTTP session",
                     L"%s", GetLastErrorString().c_str());
        goto done;
    }

    hConnect = WinHttpConnect(hSession, domain, INTERNET_DEFAULT_HTTP_PORT, 0);

    if (hConnect == NULL) {
        ReportStatus(KHERR_ERROR, L"Can't open HTTP connection",
                     L"%s", GetLastErrorString().c_str());
        goto done;
    }

    hRequest = WinHttpOpenRequest(hConnect, L"GET", resource, NULL, WINHTTP_NO_REFERER,
                                  mimetypes, WINHTTP_FLAG_ESCAPE_PERCENT);

    if (hRequest == NULL) {
        ReportStatus(KHERR_ERROR, L"Can't open request",
                     L"%s", GetLastErrorString().c_str());
        goto done;
    }

    {
        DWORD opt;

        opt = WINHTTP_DISABLE_AUTHENTICATION;
        if (!WinHttpSetOption(hRequest, WINHTTP_OPTION_DISABLE_FEATURE, &opt, sizeof(opt)))
            goto done;

        opt = WINHTTP_DISABLE_COOKIES;
        if (!WinHttpSetOption(hRequest, WINHTTP_OPTION_DISABLE_FEATURE, &opt, sizeof(opt)))
            goto done;

        opt = WINHTTP_DISABLE_KEEP_ALIVE;
        if (!WinHttpSetOption(hRequest, WINHTTP_OPTION_DISABLE_FEATURE, &opt, sizeof(opt)))
            goto done;
    }

    rv = KHM_ERROR_NOT_FOUND;

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS,
                            0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        ReportStatus(KHERR_ERROR, L"Can't send request to server",
                     L"Unable to send HTTP request to server at %s.\n"
                     L"%s", domain, GetLastErrorString().c_str());
        goto done;
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        ReportStatus(KHERR_ERROR, L"Error while receiving response",
                     L"%s", GetLastErrorString().c_str());
        goto done;
    }

    rv = KHM_ERROR_GENERAL;

    {
        DWORD status = 0;
        DWORD nb = sizeof(status);

        if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,
                                 WINHTTP_HEADER_NAME_BY_INDEX, &status, &nb, WINHTTP_NO_HEADER_INDEX)) {
            ReportStatus(KHERR_ERROR, L"Error while querying response status",
                         L"%s", GetLastErrorString().c_str());
            goto done;
        }

        if (status == HTTP_STATUS_NOT_FOUND) {
            switch (m_method) {
            case ByFavIcon:
                // Status reports are ignored for Favicon searches
                // anyway.
                break;

            case ByGravatar:
                ReportStatus(KHERR_ERROR, L"Could not find Gravatar",
                             L"An icon could not be found for %s on %s.\n",
                             m_target.c_str(),
                             domain);
                break;

            default:
                ReportStatus(KHERR_ERROR, L"The requested resource was not found",
                             L"The requested resource was not found on %s.", domain);
                break;
            }
            rv = KHM_ERROR_NOT_FOUND;
            goto done;
        }

        if (status != HTTP_STATUS_OK) {
            ReportStatus(KHERR_ERROR, L"The request failed",
                         L"The server at %s returned an unexpected status (%d)", domain, status);
            rv = KHM_ERROR_GENERAL;
            goto done;
        }
    }

    {
        wchar_t contenttype[128];
        DWORD nb = sizeof(contenttype);
        int i;

        if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_TYPE,
                                WINHTTP_HEADER_NAME_BY_INDEX, contenttype, &nb,
                                WINHTTP_NO_HEADER_INDEX)) {
            std::wstring::size_type epos;

            epos = m_path.rfind(L'.');

            if (epos != std::wstring::npos)
                m_path.erase(epos);

            for (i=0; i < ARRAYLENGTH(content_type_map); i++) {
                if (!_wcsicmp(contenttype, content_type_map[i].content_type))
                    break;
            }

            if (i < ARRAYLENGTH(content_type_map)) {
                m_path.append(content_type_map[i].extension);
            } else {
                ReportStatus(KHERR_WARNING, L"Unknown content type",
                             L"The content type %s was not expected for this request.",
                             contenttype);
            }
        } else {
            ReportStatus(KHERR_WARNING, L"Could not query response content type",
                         L"%s", GetLastErrorString().c_str());
        }
    }

    /* The request went through.  Create the file now */
    hFile = CreateFile(m_path.c_str(), GENERIC_WRITE,
                       FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                       NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_TEMPORARY,
                       NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        ReportStatus(KHERR_ERROR, L"Can't create file",
                     L"%s", GetLastErrorString().c_str());
        goto done;
    }

    while (nTotalBytes < MAX_ICON_SIZE && bContinue) {
        DWORD nBytes = 0;
        BYTE * buffer = NULL;
        DWORD nRead = 0;
        DWORD nWritten = 0;

        bContinue = FALSE;

        if (!WinHttpQueryDataAvailable(hRequest, &nBytes))
            break;

        if (nBytes == 0) {
            bContinue = TRUE;
            break;
        }

        if (nBytes + nTotalBytes > MAX_ICON_SIZE)
            break;

        buffer = PNEW BYTE[nBytes];
        if (buffer == NULL)
            break;

        if (!WinHttpReadData(hRequest, buffer, nBytes, &nRead) || nRead == 0) {
            /* Fail */
        } else {
            /* Data found */
            if (WriteFile(hFile, buffer, nRead, &nWritten, NULL))
                bContinue = TRUE;
        }

        delete [] buffer;
        nTotalBytes += nBytes;
    }

    if (bContinue) {
        /* Done with file */
        rv = KHM_ERROR_SUCCESS;

    } else {
        /* File is incomplete */
        ReportStatus(KHERR_ERROR, L"Download incomplete",
                     L"The download was terminated unexpectedly.");
        DeleteFile(m_path.c_str());
    }

 done:

    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    ReportComplete(KHM_SUCCEEDED(rv));

    return rv;
}

bool
HttpRequest::SetTempPath()
{
    wchar_t temp_path[MAX_PATH];
    wchar_t temp_file[MAX_PATH];
    DWORD d;

    if ((d = GetTempPath(ARRAYLENGTH(temp_path), temp_path)) == 0 || d > ARRAYLENGTH(temp_path))
        return false;

    d = GetTempFileName(temp_path, L"NIM", 0, temp_file);

    if (d == 0)
        return false;

    m_path = temp_file;
    return true;
}

void
HttpRequest::FetchImageFromURL()
{
    HANDLE hFile = INVALID_HANDLE_VALUE;

    do {
        URL_COMPONENTS uc;
        wchar_t domain[MAX_PATH];

        memset(&uc, 0, sizeof(uc));
        uc.dwStructSize = sizeof(uc);
        uc.lpszHostName = domain;
        uc.dwHostNameLength = ARRAYLENGTH(domain);
        uc.lpszUrlPath = NULL;
        uc.dwUrlPathLength = 1;

        if (!WinHttpCrackUrl(m_target.c_str(), 0, 0, &uc))
            break;

        FetchResource(domain, uc.lpszUrlPath, image_mimetypes);

    } while(FALSE);
}

void
HttpRequest::FetchFaviconForDomain()
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    wchar_t domain[MAX_PATH];
    const wchar_t * domain_ptr = domain;

    {
        URL_COMPONENTS uc;

        memset(&uc, 0, sizeof(uc));
        uc.dwStructSize = sizeof(uc);
        uc.lpszHostName = domain;
        uc.dwHostNameLength = ARRAYLENGTH(domain);

        if (!WinHttpCrackUrl(m_target.c_str(), 0, 0, &uc))
            StringCchCopy(domain, ARRAYLENGTH(domain), m_target.c_str());
    }

    MuteReports();

    do {
        khm_int32 rv;

        rv = FetchResource(domain_ptr, L"/favicon.ico", icon_mimetypes);
        if (KHM_SUCCEEDED(rv) || rv != KHM_ERROR_NOT_FOUND)
            break;

        {
            wchar_t wwwdomain[MAX_PATH];

            if (SUCCEEDED(StringCbPrintf(wwwdomain, sizeof(wwwdomain), L"www.%s", domain_ptr)) &&
                (KHM_SUCCEEDED(FetchResource(wwwdomain, L"/favicon.ico", icon_mimetypes))))
                break;
        }

        domain_ptr = wcschr(domain_ptr, L'.');
        if (domain_ptr)
            domain_ptr++;
    } while(domain_ptr != NULL && *domain_ptr != L'\0');

    MuteReports(false);
    ReportComplete(false);
}

static khm_int32
hash_data(BYTE * data, DWORD cb_data, ALG_ID alg, BYTE * hash, DWORD *pcb_hash)
{
    khm_int32 rv = KHM_ERROR_GENERAL;
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        goto done;

    if (!CryptCreateHash(hProv, alg, 0, 0, &hHash))
        goto done;

    if (!CryptHashData(hHash, data, cb_data, 0))
        goto done;

    if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, pcb_hash, 0))
        goto done;

    rv = KHM_ERROR_SUCCESS;

 done:
    if (hHash != 0) CryptDestroyHash(hHash);
    if (hProv != 0) CryptReleaseContext(hProv, 0);

    return rv;
}

#define MD5LEN 16

void
HttpRequest::FetchGravatarForEmail()
{
    unsigned char ehash[MD5LEN];

    {
        char email[1024];
        DWORD d;
        size_t len;

        if (WideCharToMultiByte(CP_UTF8, 0, m_target.c_str(), -1,
                                email, sizeof(email), NULL, NULL) == 0) {
            ReportStatus(KHERR_ERROR,
                         L"Can't convert email address to UTF-8",
                         L"%s", GetLastErrorString().c_str());
            return;
        }

        _strlwr_s(email, sizeof(email));

        if (FAILED(StringCbLengthA(email, sizeof(email), &len))) {
            ReportStatus(KHERR_ERROR,
                         L"UTF-8 email address too long",
                         L"The email address can't be longer than 1024 characters");
            return;
        }

        d = sizeof(ehash);
        if (KHM_FAILED(hash_data((BYTE *) email, len, CALG_MD5, (BYTE *) ehash, &d))) {
            ReportStatus(KHERR_ERROR, L"Failed to hash email address", NULL);
            return;
        }
    }

    {
        wchar_t resource[60];
        wchar_t * tail;
        size_t len;
        int i;
        static const wchar_t hexdigits[] = L"0123456789abcdef";

        StringCbCopyEx(resource, sizeof(resource), L"/avatar/", &tail, &len, STRSAFE_NO_TRUNCATION);

        for (i = 0; i < sizeof(ehash); i++) {
            *tail++ = hexdigits[ehash[i] >> 4];
            *tail++ = hexdigits[ehash[i] & 0xf];
            len -= sizeof(wchar_t) * 2;
        }
        *tail++ = L'\0';

        StringCbCat(resource, sizeof(resource), L".jpg?d=404&s=128");

        FetchResource(L"www.gravatar.com", resource, jpg_mimetypes);
    }
}

HttpRequest::HttpRequest(const wchar_t * target, Method method,
                         HttpRequestStatusListener * listener) :
    m_task(NULL), m_target(target), m_method(method), m_listener(listener),
    m_muted(false)
{
    SetOkToDispose(true);       // These can be disposed at any time
}

HttpRequest::~HttpRequest()
{
    if (m_task) {
        task_release(m_task);
        m_task = NULL;
    }
}

void
HttpRequest::MuteReports(bool mute)
{
    m_muted = mute;
}

khm_int32
HttpRequest::ExecuteRequest()
{
    AutoRef<HttpRequest> ref(this, RefCount::TakeOwnership);

    if (!SetTempPath()) {
        ReportStatus(KHERR_ERROR, L"Can't get temporary path",
                     L"The location for temporary files could not be determined.");
        ReportComplete(false);
        return KHM_ERROR_GENERAL;
    }

    std::wstring old_temp_path(m_path);

    switch (m_method) {
    case ByURL:
        FetchImageFromURL();
        break;

    case ByFavIcon:
        FetchFaviconForDomain();
        break;

    case ByGravatar:
        FetchGravatarForEmail();
        break;

    default:
        ReportStatus(KHERR_ERROR, L"Unknown method",
                     L"Internal error.  Method ID=%d", m_method);
    }

    if (old_temp_path != m_path && !old_temp_path.empty())
        DeleteFile(old_temp_path.c_str());

    if (!m_path.empty())
        DeleteFile(m_path.c_str());

    ReportComplete(false);      // Only one completion report goes
                                // through.  If the request succeeded,
                                // then that notification would have
                                // already been sent.

    return KHM_ERROR_SUCCESS;
}

khm_int32 __stdcall
HttpRequest::TaskRunner(void * param)
{
    HttpRequest * req = static_cast<HttpRequest *>(param);
    return req->ExecuteRequest();
}

HttpRequest *
HttpRequest::CreateRequest(const wchar_t * target,
                           Method method,
                           HttpRequestStatusListener * listener)
{
    HttpRequest * req = PNEW HttpRequest(target, method, listener);

    req->Hold();                // One hold for TaskRunner()
    req->m_task = task_create(NULL, 0, TaskRunner, req, NULL, 0);

    return req;                 // Leave held.  Caller should release
                                // the request.
}

}
