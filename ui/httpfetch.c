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

#include "khmapp.h"
#include<winhttp.h>
#include<wchar.h>
#include<strsafe.h>

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

static khm_int32
get_http_resource(const wchar_t * domain,
                  const wchar_t * resource,
                  const wchar_t ** mimetypes,
                  wchar_t * path,
                  size_t cch_path,
                  HANDLE * phFile)
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

    if (hSession == NULL) goto done;

    hConnect = WinHttpConnect(hSession, domain, INTERNET_DEFAULT_HTTP_PORT, 0);

    if (hConnect == NULL) goto done;

    hRequest = WinHttpOpenRequest(hConnect, L"GET", resource, NULL, WINHTTP_NO_REFERER,
                                  mimetypes, WINHTTP_FLAG_ESCAPE_PERCENT);

    if (hRequest == NULL) goto done;

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
                            0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
        goto done;

    if (!WinHttpReceiveResponse(hRequest, NULL))
        goto done;

    rv = KHM_ERROR_GENERAL;

    {
        DWORD status = 0;
        DWORD nb = sizeof(status);

        if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,
                                 WINHTTP_HEADER_NAME_BY_INDEX, &status, &nb, WINHTTP_NO_HEADER_INDEX))
            goto done;

        if (status == HTTP_STATUS_NOT_FOUND) {
            rv = KHM_ERROR_NOT_FOUND;
            goto done;
        }

        if (status != HTTP_STATUS_OK) {
            rv = KHM_ERROR_GENERAL;
            goto done;
        }
    }

    if (cch_path > 0) {
        wchar_t contenttype[128];
        DWORD nb = sizeof(contenttype);
        wchar_t * extension;
        int i;

        if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_TYPE,
                                WINHTTP_HEADER_NAME_BY_INDEX, contenttype, &nb,
                                WINHTTP_NO_HEADER_INDEX)) {
            extension = wcsrchr(path, L'.');
            if (extension == NULL)
                extension = path + wcslen(path);

            for (i=0; i < ARRAYLENGTH(content_type_map); i++) {
                if (!_wcsicmp(contenttype, content_type_map[i].content_type))
                    break;
            }

            if (i < ARRAYLENGTH(content_type_map)) {
                StringCchCopy(extension, cch_path - (extension - path),
                              content_type_map[i].extension);
            }
        }
    }

    /* The request went through.  Create the file now */
    hFile = CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                       NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_TEMPORARY,
                       NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        goto done;

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

        buffer = malloc(nBytes);
        if (buffer == NULL)
            break;

        if (!WinHttpReadData(hRequest, buffer, nBytes, &nRead) || nRead == 0) {
            /* Fail */
        } else {
            /* Data found */
            if (WriteFile(hFile, buffer, nRead, &nWritten, NULL))
                bContinue = TRUE;
        }

        free(buffer);
        nTotalBytes += nBytes;
    }

    if (bContinue) {
        /* Done with file */
        rv = KHM_ERROR_SUCCESS;
        *phFile = hFile;
    } else {
        /* File is incomplete */
        DeleteFile(path);
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

 done:

    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    return rv;
}

static khm_int32
fetch_icon_resource_from_domain(const wchar_t * domain, wchar_t * path, HANDLE * phFile)
{
    return get_http_resource(domain, L"/favicon.ico", icon_mimetypes, path, 0, phFile);
}

BOOL
khm_get_temp_path(wchar_t * path, size_t cch)
{
    wchar_t temp_path[MAX_PATH];
    wchar_t temp_file[MAX_PATH];
    DWORD d;

    if ((d = GetTempPath(ARRAYLENGTH(temp_path), temp_path)) == 0 || d > ARRAYLENGTH(temp_path))
        return FALSE;

    d = GetTempFileName(temp_path, L"NIM", 0, temp_file);

    if (d == 0)
        return FALSE;

    return SUCCEEDED(StringCchCopy(path, cch, temp_file));
}

HANDLE
khm_get_image_from_url(const wchar_t * url, wchar_t * path, size_t cch_path)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;

    do {
        khm_int32 rv;
        URL_COMPONENTS uc;
        wchar_t domain[MAX_PATH];

        memset(&uc, 0, sizeof(uc));
        uc.dwStructSize = sizeof(uc);
        uc.lpszHostName = domain;
        uc.dwHostNameLength = ARRAYLENGTH(domain);
        uc.lpszUrlPath = NULL;
        uc.dwUrlPathLength = 1;

        if (!WinHttpCrackUrl(url, 0, 0, &uc))
            break;

        rv = get_http_resource(domain, uc.lpszUrlPath, image_mimetypes, path, cch_path, &hFile);

    } while(FALSE);

    return hFile;
}

HANDLE
khm_get_favicon_for_domain(const wchar_t * url, wchar_t *path)
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

        if (!WinHttpCrackUrl(url, 0, 0, &uc))
            StringCchCopy(domain, ARRAYLENGTH(domain), url);
    }

    do {
        khm_int32 rv;

        rv = fetch_icon_resource_from_domain(domain_ptr, path, &hFile);
        if (KHM_SUCCEEDED(rv) || rv != KHM_ERROR_NOT_FOUND)
            break;

        {
            wchar_t wwwdomain[MAX_PATH];

            if (SUCCEEDED(StringCbPrintf(wwwdomain, sizeof(wwwdomain), L"www.%s", domain_ptr)) &&
                (KHM_SUCCEEDED(fetch_icon_resource_from_domain(wwwdomain, path, &hFile))))
                break;
        }

        domain_ptr = wcschr(domain_ptr, L'.');
        if (domain_ptr)
            domain_ptr++;
    } while(domain_ptr != NULL && *domain_ptr != L'\0');

    return hFile;
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

HANDLE
khm_get_gravatar_for_email(const wchar_t * wemail, wchar_t * path)
{
    unsigned char ehash[MD5LEN];

    {
        char email[1024];
        DWORD d;
        size_t len;

        if (WideCharToMultiByte(CP_UTF8, 0, wemail, -1, email, sizeof(email), NULL, NULL) == 0)
            goto done;

        _strlwr_s(email, sizeof(email));

        if (FAILED(StringCbLengthA(email, sizeof(email), &len)))
            goto done;

        d = sizeof(ehash);
        if (KHM_FAILED(hash_data((BYTE *) email, len, CALG_MD5, (BYTE *) ehash, &d)))
            goto done;
    }

    {
        wchar_t resource[60];
        wchar_t * tail;
        size_t len;
        int i;
        static const wchar_t hexdigits[] = L"0123456789abcdef";
        HANDLE hFile = INVALID_HANDLE_VALUE;

        StringCbCopyEx(resource, sizeof(resource), L"/avatar/", &tail, &len, STRSAFE_NO_TRUNCATION);

        for (i = 0; i < sizeof(ehash); i++) {
            *tail++ = hexdigits[ehash[i] >> 4];
            *tail++ = hexdigits[ehash[i] & 0xf];
            len -= sizeof(wchar_t) * 2;
        }
        *tail++ = L'\0';

        StringCbCat(resource, sizeof(resource), L".jpg?d=404&s=128");

        get_http_resource(L"www.gravatar.com", resource, jpg_mimetypes, path, 0, &hFile);

        return hFile;
    }

 done:
    return INVALID_HANDLE_VALUE;
}
