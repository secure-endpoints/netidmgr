/*
 * Copyright (c) 2005-2008 Massachusetts Institute of Technology
 * Copyright (c) 2008-2009 Secure Endpoints Inc.
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

/* $Id$ */

#include "kcreddbinternal.h"

KHMEXP void KHMAPI TimetToFileTime( time_t t, LPFILETIME pft )
{
    LONGLONG ll;

    if ( sizeof(time_t) == 4 )
	ll = Int32x32To64(t, 10000000) + 116444736000000000i64;
    else {
	ll = t * 10000000i64 + 116444736000000000i64;
    }
    pft->dwLowDateTime = (DWORD) ll;
    pft->dwHighDateTime = (DWORD) (ll >> 32);
}

KHMEXP void KHMAPI TimetToFileTimeInterval(time_t t, LPFILETIME pft)
{
    LONGLONG ll;

    if ( sizeof(time_t) == 4 )
	ll = Int32x32To64(t, 10000000);
    else {
	ll = t * 10000000i64;
    }
    pft->dwLowDateTime = (DWORD) ll;
    pft->dwHighDateTime = (DWORD) (ll >> 32);
}

KHMEXP long KHMAPI FtIntervalToSeconds(const FILETIME * pft)
{
    __int64 i = FtToInt(pft);
    return (long) (i / 10000000i64);
}

KHMEXP long KHMAPI FtIntervalToMilliseconds(const FILETIME * pft)
{
    __int64 i = FtToInt(pft);
    return (long) (i / 10000i64);
}

KHMEXP khm_int64 KHMAPI FtToInt(const FILETIME * pft) {
    LARGE_INTEGER ll;
    ll.LowPart = pft->dwLowDateTime;
    ll.HighPart = pft->dwHighDateTime;
    return ll.QuadPart;
}

KHMEXP FILETIME KHMAPI IntToFt(khm_int64 i) {
    LARGE_INTEGER ll;
    FILETIME ft;

    ll.QuadPart = i;
    ft.dwLowDateTime = ll.LowPart;
    ft.dwHighDateTime = ll.HighPart;

    return ft;
}

KHMEXP FILETIME KHMAPI FtSub(const FILETIME * ft1, const FILETIME * ft2) {
    FILETIME d;
    LARGE_INTEGER l1, l2;

    l1.LowPart = ft1->dwLowDateTime;
    l1.HighPart = ft1->dwHighDateTime;
    l2.LowPart = ft2->dwLowDateTime;
    l2.HighPart = ft2->dwHighDateTime;

    l1.QuadPart -= l2.QuadPart;

    d.dwLowDateTime = l1.LowPart;
    d.dwHighDateTime = l1.HighPart;

    return d;
}

KHMEXP FILETIME KHMAPI FtAdd(const FILETIME * ft1, const FILETIME * ft2) {
    FILETIME d;
    LARGE_INTEGER l1, l2;

    l1.LowPart = ft1->dwLowDateTime;
    l1.HighPart = ft1->dwHighDateTime;
    l2.LowPart = ft2->dwLowDateTime;
    l2.HighPart = ft2->dwHighDateTime;

    l1.QuadPart += l2.QuadPart;

    d.dwLowDateTime = l1.LowPart;
    d.dwHighDateTime = l1.HighPart;

    return d;
}

#define MAX_IVL_SPECLIST_LEN 256
#define MAX_IVL_UNITS 5

enum _ivl_indices {
    IVL_SECONDS = 0,
    IVL_MINUTES,
    IVL_HOURS,
    IVL_DAYS,
    IVL_WEEKS
};

typedef struct ivspec_t {
    wchar_t str[MAX_IVL_SPECLIST_LEN];
    __int64 mul;
} ivspec;

static ivspec ivspecs[MAX_IVL_UNITS];
static BOOL ivspecs_loaded = FALSE;

int _iv_is_in_spec(const wchar_t *s, int n, const wchar_t * spec)
{
    /* spec strings are comma separated */
    const wchar_t *b, *e;

    b = spec;
    while(*b) {
        e = wcschr(b, L',');
        if(!e)
            e = b + wcslen(b);

        if((e - b) == n  && !_wcsnicmp(b, s, n)) {
            return TRUE;
        }

        if(*e)
            b = e+1;
        else
            break;
    }

    return FALSE;
}

KHMEXP khm_int32 KHMAPI IntervalStringToFt(FILETIME * pft, const wchar_t * str)
{
    size_t cb;
    const wchar_t * b;
    __int64 t;

    *pft = IntToFt(0);

    /* ideally we should synchronize this, but it doesn't hurt if two
       threads do this at the same time, because we only set the ivspecs_loaded
       flag when we are done */
    if(!ivspecs_loaded) {
        LoadString(hinst_kcreddb, IDS_IVL_S_SPEC, ivspecs[IVL_SECONDS].str, MAX_IVL_SPECLIST_LEN);
        ivspecs[IVL_SECONDS].mul = 10000000i64;
        LoadString(hinst_kcreddb, IDS_IVL_M_SPEC, ivspecs[IVL_MINUTES].str, MAX_IVL_SPECLIST_LEN);
        ivspecs[IVL_MINUTES].mul = ivspecs[IVL_SECONDS].mul * 60;
        LoadString(hinst_kcreddb, IDS_IVL_H_SPEC, ivspecs[2].str, MAX_IVL_SPECLIST_LEN);
        ivspecs[IVL_HOURS].mul = ivspecs[IVL_MINUTES].mul * 60;
        LoadString(hinst_kcreddb, IDS_IVL_D_SPEC, ivspecs[3].str, MAX_IVL_SPECLIST_LEN);
        ivspecs[IVL_DAYS].mul = ivspecs[IVL_HOURS].mul * 24;
        LoadString(hinst_kcreddb, IDS_IVL_W_SPEC, ivspecs[4].str, MAX_IVL_SPECLIST_LEN);
        ivspecs[IVL_WEEKS].mul = ivspecs[IVL_DAYS].mul * 7;

        ivspecs_loaded = TRUE;
    }

    if(!str || FAILED(StringCbLength(str, MAX_IVL_SPECLIST_LEN, &cb)))
        return KHM_ERROR_INVALID_PARAM;

    b = str;
    t = 0;
    while(*b) {
        __int64 f = 1;
        const wchar_t *e;
        int i;

        while(*b && iswspace(*b))
            b++;

        if(*b && iswdigit(*b)) {
            f = _wtoi64(b);

            while(*b && iswdigit(*b))
                b++;
        }

        while(*b && iswspace(*b))
            b++;

        if(!*b) /* no unit specified */
            return KHM_ERROR_INVALID_PARAM;

        e = b;

        while(*e && !iswspace(*e))
            e++;

        for(i=0; i < MAX_IVL_UNITS; i++) {
            if(_iv_is_in_spec(b, (int)(e-b), ivspecs[i].str))
                break;
        }

        if(i==MAX_IVL_UNITS)
            return KHM_ERROR_INVALID_PARAM;

        t += f * ivspecs[i].mul;

        b = e;
    }

    *pft = IntToFt(t);

    return KHM_ERROR_SUCCESS;
}


/* Returns the number of milliseconds after which the representation
   of *pft would change.  The representation in question is what's
   reported by FtIntervalToString(). */
KHMEXP long KHMAPI
FtIntervalMsToRepChange(const FILETIME * pft)
{
    __int64 ms,s,m,h,d;
    __int64 ift;
    long l;

    ift = FtToInt(pft);
    ms = ift / 10000i64 - 1;

    if(ms < 0 || ift == _I64_MAX)
        return -1;

    s = ms / 1000i64;
    m = s / 60;
    h = s / 3600;
    d = s / (3600*24);

    if (d > 0) {
        /* rep change at next hour change */
        l = (long) (ms % (3600*1000i64)) + 1;
    } else if (h > 0) {
        /* rep change at next minute change */
        l = (long) (ms % (60*1000i64)) + 1;
    } else if (m > 5) {
        /* rep change at next minute change */
        l = (long) (ms % (60*1000i64)) + 1;
    } else {
        /* rep change at next second change */
        l = (long) (ms % 1000) + 1;
    }

    return l;
}

KHMEXP khm_int32 KHMAPI
FtIntervalToString(const FILETIME * data, wchar_t * buffer, khm_size * cb_buf)
{
    size_t cbsize;
    __int64 s,m,h,d;
    __int64 ift;
    wchar_t ibuf[256];
    wchar_t fbuf[256];
    wchar_t * t;

    if(!cb_buf)
        return KHM_ERROR_INVALID_PARAM;

    ift = FtToInt(data);
    s = ift / 10000000i64;

    m = s / 60;
    h = s / 3600;
    d = s / (3600*24);

    if(ift == _I64_MAX) {
#ifdef INDICATE_UNKNOWN_EXPIRY_TIMES
        LoadString(hinst_kcreddb, IDS_IVL_UNKNOWN, ibuf, sizeof(ibuf)/sizeof(wchar_t));
#else
        StringCbCopy(ibuf, sizeof(ibuf), L"");
#endif
    } else if(s < 0) {
        LoadString(hinst_kcreddb, IDS_IVL_EXPIRED, ibuf, sizeof(ibuf)/sizeof(wchar_t));
    } else if(d > 0) {
        h = (s - (d * 3600 * 24)) / 3600;
        if(d == 1) {
            LoadString(hinst_kcreddb, IDS_IVL_1D, ibuf, ARRAYLENGTH(ibuf));
        } else {
            LoadString(hinst_kcreddb, IDS_IVL_D, fbuf, ARRAYLENGTH(fbuf));
            StringCbPrintf(ibuf, sizeof(ibuf), fbuf, d);
        }
        if(h > 0) {
            StringCbCat(ibuf, sizeof(ibuf), L" ");
            t = ibuf + wcslen(ibuf);
            if(h == 1)
            {
                LoadString(hinst_kcreddb, IDS_IVL_1H, t,
                           (int) (ARRAYLENGTH(ibuf) - wcslen(ibuf)));
            } else {
                LoadString(hinst_kcreddb, IDS_IVL_H, fbuf,
                           (int) ARRAYLENGTH(fbuf));
                StringCbPrintf(t, sizeof(ibuf) - wcslen(ibuf)*sizeof(wchar_t), fbuf, h);
            }
        }
    } else if(h > 0 || m > 5) {
        m = (s - (h * 3600)) / 60;
        if(h == 1) {
            LoadString(hinst_kcreddb, IDS_IVL_1H, ibuf, ARRAYLENGTH(ibuf));
        } else if (h > 1) {
            LoadString(hinst_kcreddb, IDS_IVL_H, fbuf, ARRAYLENGTH(fbuf));
            StringCbPrintf(ibuf, sizeof(ibuf), fbuf, h);
        } else {
            *ibuf = L'\0';
        }

        if(m > 0 || h == 0) {
            if (h >= 1)
                StringCbCat(ibuf, sizeof(ibuf), L" ");

            t = ibuf + wcslen(ibuf);
            if(m == 1)
            {
                LoadString(hinst_kcreddb, IDS_IVL_1M, t,
                           (int) (ARRAYLENGTH(ibuf) - wcslen(ibuf)));
            } else {
                LoadString(hinst_kcreddb, IDS_IVL_M, fbuf,
                           (int) ARRAYLENGTH(fbuf));
                StringCbPrintf(t, sizeof(ibuf) - wcslen(ibuf)*sizeof(wchar_t), fbuf, m);
            }
        }
    } else if(m > 0) {
        s -= m * 60;
        if(m == 1) {
            LoadString(hinst_kcreddb, IDS_IVL_1M, ibuf, ARRAYLENGTH(ibuf));
        } else {
            LoadString(hinst_kcreddb, IDS_IVL_M, fbuf, ARRAYLENGTH(fbuf));
            StringCbPrintf(ibuf, sizeof(ibuf), fbuf, m);
        }
        if(s > 0) {
            StringCbCat(ibuf, sizeof(ibuf), L" ");
            t = ibuf + wcslen(ibuf);
            if(s == 1)
            {
                LoadString(hinst_kcreddb, IDS_IVL_1S, t,
                           (int) (ARRAYLENGTH(ibuf) - wcslen(ibuf)));
            } else {
                LoadString(hinst_kcreddb, IDS_IVL_S, fbuf,
                           (int) ARRAYLENGTH(fbuf));
                StringCbPrintf(t, sizeof(ibuf) - wcslen(ibuf)*sizeof(wchar_t), fbuf, s);
            }
        }
    } else {
        if(s == 1) {
            LoadString(hinst_kcreddb, IDS_IVL_1S, ibuf, ARRAYLENGTH(ibuf));
        } else {
            LoadString(hinst_kcreddb, IDS_IVL_S, fbuf, sizeof(fbuf)/sizeof(wchar_t));
            StringCbPrintf(ibuf, sizeof(ibuf), fbuf, s);
        }
    }

    StringCbLength(ibuf, sizeof(ibuf), &cbsize);
    cbsize += sizeof(wchar_t);

    if(!buffer || *cb_buf < cbsize) {
        *cb_buf = cbsize;
        return KHM_ERROR_TOO_LONG;
    }

    StringCbCopy(buffer, *cb_buf, ibuf);
    *cb_buf = cbsize;

    return KHM_ERROR_SUCCESS;
}

khm_int32
FtToString(const FILETIME * pft, khm_int32 flags, wchar_t * buf, khm_size * pcb_buf)
{
    size_t cbsize;
    size_t cchsize;
    wchar_t * bufend;
    SYSTEMTIME st_now;
    SYSTEMTIME st_d;
    SYSTEMTIME st_dl;
    int today = 0;

    if(!pcb_buf)
        return KHM_ERROR_INVALID_PARAM;

    GetLocalTime(&st_now);
    FileTimeToSystemTime(pft, &st_d);
    SystemTimeToTzSpecificLocalTime(NULL, &st_d, &st_dl);
    if (st_now.wYear == st_dl.wYear &&
        st_now.wMonth == st_dl.wMonth &&
        st_now.wDay == st_dl.wDay)
        today = 1;

    if(today && (flags & KCDB_TS_SHORT)) {
        cbsize = 0;
    } else {
        cbsize = GetDateFormat(LOCALE_USER_DEFAULT,
                               DATE_SHORTDATE, &st_dl,
                               NULL, NULL, 0) * sizeof(wchar_t);
    }

    cbsize += GetTimeFormat(LOCALE_USER_DEFAULT,
                            0, &st_dl, NULL, NULL,
                            0) * sizeof(wchar_t);

    if(!buf || *pcb_buf < cbsize) {
        *pcb_buf = cbsize;
        return KHM_ERROR_TOO_LONG;
    }

    cchsize = cbsize / sizeof(wchar_t);

    if(!today || !(flags & KCDB_TS_SHORT)) {
        size_t cch_buf_len;

        GetDateFormat(LOCALE_USER_DEFAULT,
                      DATE_SHORTDATE,
                      &st_dl, NULL, buf,
                      (int) cchsize);

        StringCchCat(buf, cchsize, L" ");
        StringCchLength(buf, cchsize, &cch_buf_len);
        bufend = buf + cch_buf_len;
        cchsize -= cch_buf_len;
    } else {
        bufend = buf;
    }

    GetTimeFormat(LOCALE_USER_DEFAULT,
                  0, &st_dl, NULL, bufend,
                  (int) cchsize);

    *pcb_buf = cbsize;

    return KHM_ERROR_SUCCESS;
}


/* The returned string would be one of the following:

   If *pft is a date/time:
     If FTSE_RELATIVE_DAY is set
       If *pft is within a day of today (local time)
         "at <time>"
       Else if *pft is within a day of tomorrow (local time)
         "tomorrow at <time>"
       Else if *pft is within a day of yesterday (local time)
         "yesterday at <time>"
       Else
         failover to FTSE_RELATIVE
     If FTSE_RELATIVE is set
       If *pft is in the future
         "in [<days> days] [<hours> hours] [<mins> minutes] [<secs> seconds]"
       Else
         "[<days> days] [<hours> hours] [<mins> minutes] [<secs> seconds] ago"
     Else
       "<date> <time>"
   Else (an interval)
     "[<days> days][<hours> hours][<mins> minutes][<secs> seconds]"

 */
KHMEXP khm_int32 KHMAPI
FtToStringEx(const FILETIME * pft, khm_int32 flags,
             long * ms_to_change,
             wchar_t * buffer, khm_size * pcb_buf)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;
    FILETIME ft_now;
    FILETIME ft_diff;
    wchar_t str[128];
    khm_size cb;
    int i;

    if (ms_to_change)
        *ms_to_change = 0;

    GetSystemTimeAsFileTime(&ft_now);

    if (!(flags & FTSE_INTERVAL)) {
        if (flags & FTSE_RELATIVE_DAY) {
            FILETIME ftl_target; /* Target local time */
            SYSTEMTIME stl_target; /* Target local time */
            FILETIME ftl_now;    /* Current local time */
            SYSTEMTIME stl_now;  /* Current local time */
            FILETIME fti_day;    /* 24-hour interval */
            FILETIME t;
            SYSTEMTIME st;
            wchar_t str_time[64];
            wchar_t fmt[64];

            FileTimeToSystemTime(pft, &st);
            SystemTimeToTzSpecificLocalTime(NULL, &st, &stl_target);
            SystemTimeToFileTime(&stl_target, &ftl_target);

            FileTimeToSystemTime(&ft_now, &st);
            SystemTimeToTzSpecificLocalTime(NULL, &st, &stl_now);
            SystemTimeToFileTime(&stl_now, &ftl_now);

            stl_now.wHour = stl_now.wMinute = stl_now.wSecond = stl_now.wMilliseconds = 0;
            SystemTimeToFileTime(&stl_now, &t);

            TimetToFileTimeInterval(3600 * 24L, &fti_day);

            if(GetTimeFormat(LOCALE_USER_DEFAULT,
                             TIME_NOSECONDS,
                             &stl_target, NULL,
                             str_time, ARRAYLENGTH(str_time)) == 0) {
                rv = KHM_ERROR_UNKNOWN;
                goto _done;
            }

            if (CompareFileTime(&ftl_target, &t) >= 0) {
                t = FtAdd(&t, &fti_day);
                if (CompareFileTime(&ftl_target, &t) < 0) {
                    /* ftl_target happens sometime today */
                    LoadString(hinst_kcreddb, IDS_FTS_REL_TOD, fmt, ARRAYLENGTH(fmt));
                    StringCbPrintf(str, sizeof(str), fmt, str_time);
                    goto _done;
                }

                t = FtAdd(&t, &fti_day);
                if (CompareFileTime(&ftl_target, &t) < 0) {
                    /* ftl_target happens sometime tomorrow */
                    LoadString(hinst_kcreddb, IDS_FTS_REL_TOM, fmt, ARRAYLENGTH(fmt));
                    StringCbPrintf(str, sizeof(str), fmt, str_time);
                    goto _done;
                }
            } else {
                t = FtSub(&t, &fti_day);
                if (CompareFileTime(&ftl_target, &t) >= 0) {
                    /* ftl_target happened yesterday */
                    LoadString(hinst_kcreddb, IDS_FTS_REL_YEST, fmt, ARRAYLENGTH(fmt));
                    StringCbPrintf(str, sizeof(str), fmt, str_time);
                    goto _done;
                }
            }

            /* failover to FTSE_RELATIVE */
            flags |= FTSE_RELATIVE;
        }

        if (flags & FTSE_RELATIVE) {
            wchar_t str_interval[128];
            wchar_t str_fmt[32];

            if (CompareFileTime(&ft_now, pft) < 0) {
                /* "in [<days> days] [<hours> hours] [<mins> minutes] [<secs> seconds]" */
                ft_diff = FtSub(pft, &ft_now);
                LoadString(hinst_kcreddb, IDS_FTS_REL_POS, str_fmt, ARRAYLENGTH(str_fmt));
            } else {
                /* "[<days> days] [<hours> hours] [<mins> minutes] [<secs> seconds] ago" */
                ft_diff = FtSub(&ft_now, pft);
                LoadString(hinst_kcreddb, IDS_FTS_REL_NEG, str_fmt, ARRAYLENGTH(str_fmt));
            }

            cb = sizeof(str_interval);
            FtIntervalToString(&ft_diff, str_interval, &cb);
            if (ms_to_change)
                *ms_to_change = FtIntervalMsToRepChange(&ft_diff);

            StringCbPrintf(str, sizeof(str), str_fmt, str_interval);

        } else {
            /* "<date> <time>" */
            wchar_t str_time[64];
            wchar_t str_date[64];
            wchar_t str_fmt[32];
            SYSTEMTIME stl_target;
            SYSTEMTIME st;

            FileTimeToSystemTime(pft, &st);
            SystemTimeToTzSpecificLocalTime(NULL, &st, &stl_target);

            i = GetDateFormat(LOCALE_USER_DEFAULT,
                              ((flags & KCDB_TS_SHORT)?DATE_SHORTDATE:DATE_LONGDATE),
                              &stl_target, NULL, str_date, ARRAYLENGTH(str_date));
            if (i == 0) {
                rv = KHM_ERROR_UNKNOWN;
                goto _done;
            }

            i = GetTimeFormat(LOCALE_USER_DEFAULT,
                              ((flags & KCDB_TS_SHORT)?TIME_NOSECONDS:0),
                              &stl_target, NULL, str_time, ARRAYLENGTH(str_time));

            if (i == 0) {
                rv = KHM_ERROR_UNKNOWN;
                goto _done;
                /* TODO: in this block and the above, we should check
                   GetLastError() and come up with a better error
                   value. */
            }

            LoadString(hinst_kcreddb, IDS_FTS_TS, str_fmt, ARRAYLENGTH(str_fmt));
            FormatString(str, sizeof(str), str_fmt, str_date, str_time);
        }
    } else {
        /* "[<days> days][<hours> hours][<mins> minutes][<secs> seconds]" */

        cb = sizeof(str);
        FtIntervalToString(pft, str, &cb);
        if (ms_to_change)
            *ms_to_change = FtIntervalMsToRepChange(pft);
    }

 _done:

    if (KHM_FAILED(rv))
        return rv;

    StringCbLength(str, sizeof(str), &cb);
    cb += sizeof(wchar_t);

    if (buffer == NULL || *pcb_buf < cb) {
        *pcb_buf = cb;
        rv = KHM_ERROR_TOO_LONG;
    } else {
        StringCbCopy(buffer, *pcb_buf, str);
        *pcb_buf = cb;
    }

    return rv;
}
