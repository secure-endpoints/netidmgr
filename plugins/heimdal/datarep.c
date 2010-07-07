/*
 * Copyright (c) 2010 Secure Endpoints Inc.
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

/* Data representation and related functions */

#include<winsock2.h>
#include "krbcred.h"
#include<krb5.h>
#include<strsafe.h>
#include<assert.h>

khm_int32 KHMAPI
enctype_toString(const void * data, khm_size cbdata,
		 wchar_t *destbuf, khm_size *pcbdestbuf,
		 khm_int32 flags)
{
    int resid = 0;
    krb5_enctype etype;
    wchar_t buf[256];
    size_t cblength;

    if(cbdata != sizeof(khm_int32))
        return KHM_ERROR_INVALID_PARAM;

    etype = (krb5_enctype) *((khm_int32 *) data);

    switch(etype) {
    case ENCTYPE_NULL:
        resid = IDS_ETYPE_NULL;
        break;

    case ENCTYPE_DES_CBC_CRC:
        resid = IDS_ETYPE_DES_CBC_CRC;
        break;

    case ENCTYPE_DES_CBC_MD4:
        resid = IDS_ETYPE_DES_CBC_MD4;
        break;

    case ENCTYPE_DES_CBC_MD5:
        resid = IDS_ETYPE_DES_CBC_MD5;
        break;

    case ENCTYPE_DES3_CBC_SHA1:
        resid = IDS_ETYPE_DES3_CBC_SHA1;
        break;

    case ENCTYPE_AES128_CTS_HMAC_SHA1_96:
        resid = IDS_ETYPE_AES128_CTS_HMAC_SHA1_96;
        break;

    case ENCTYPE_AES256_CTS_HMAC_SHA1_96:
        resid = IDS_ETYPE_AES256_CTS_HMAC_SHA1_96;
        break;

    case ENCTYPE_ARCFOUR_HMAC:
        resid = IDS_ETYPE_ARCFOUR_HMAC;
        break;

#if 0
    case ENCTYPE_LOCAL_DES3_HMAC_SHA1:
        resid = IDS_ETYPE_LOCAL_DES3_HMAC_SHA1;
        break;

    case ENCTYPE_LOCAL_RC4_MD4:
        resid = IDS_ETYPE_LOCAL_RC4_MD4;
        break;
#endif
    }

    if(resid != 0) {
        LoadString(hResModule, (UINT) resid, buf, ARRAYLENGTH(buf));
    } else {
        StringCbPrintf(buf, sizeof(buf), L"#%d", etype);
    }

    StringCbLength(buf, ARRAYLENGTH(buf), &cblength);
    cblength += sizeof(wchar_t);

    if(!destbuf || *pcbdestbuf < cblength) {
        *pcbdestbuf = cblength;
        return KHM_ERROR_TOO_LONG;
    } else {
        StringCbCopy(destbuf, *pcbdestbuf, buf);
        *pcbdestbuf = cblength;
        return KHM_ERROR_SUCCESS;
    }
}

khm_int32 KHMAPI
addr_list_comp(const void *d1, khm_size cb_d1,
	       const void *d2, khm_size cb_d2)
{
    if (cb_d1 < cb_d2)
	return -1;
    if (cb_d1 > cb_d2)
	return 1;
    return memcmp(d1, d2, cb_d1);
}

khm_int32 KHMAPI
addr_list_toString(const void *d, khm_size cb_d,
		   wchar_t *buf, khm_size *pcb_buf,
		   khm_int32 flags)
{
    HostAddresses as;
    size_t len;
    size_t i;

    wchar_t wstr[2048] = L"";
    wchar_t *wstr_d = &wstr[0];
    khm_size cch_wstr = ARRAYLENGTH(wstr);

    if ( decode_HostAddresses((const unsigned char *) d, cb_d, &as, &len) ) {
	assert(FALSE);
	return KHM_ERROR_INVALID_PARAM;
    }

    for (i=0; i < as.len; i++) {
	char buf[1024];
	wchar_t wbuf[1024];

	len = sizeof(buf);
	if (krb5_print_address(&as.val[i], buf, sizeof(buf), &len)) {
	    assert(FALSE);
	    continue;
	}

	AnsiStrToUnicode(wbuf, sizeof(wbuf), buf);
	if (FAILED(StringCchCatEx(wstr_d, cch_wstr, wbuf, &wstr_d, &cch_wstr,
				  STRSAFE_NO_TRUNCATION))) {
	    assert(FALSE);
	    continue;
	}

	if (i + 1 < as.len) {
	    if (FAILED(StringCchCatEx(wstr_d, cch_wstr, L",", &wstr_d, &cch_wstr,
				      STRSAFE_NO_TRUNCATION))) {
		assert(FALSE);
		continue;
	    }
	}
    }

    len = (ARRAYLENGTH(wstr) - cch_wstr) * sizeof(wchar_t);

    if (buf == NULL || *pcb_buf < len) {
	*pcb_buf = len;
	return KHM_ERROR_TOO_LONG;
    }

    StringCbCopy(buf, *pcb_buf, wstr);
    *pcb_buf = len;
    free_HostAddresses(&as);

    return KHM_ERROR_SUCCESS;
}

khm_int32 KHMAPI
krb5flags_toString(const void *d,
		   khm_size cb_d,
		   wchar_t *buf,
		   khm_size *pcb_buf,
		   khm_int32 f)
{
    krb5_ticket_flags tf;

    wchar_t sbuf[32];
    int i = 0;
    khm_size cb;

    tf.i = *((khm_int32 *) d);

    if (tf.b.forwardable)
        sbuf[i++] = L'F';

    if (tf.b.forwarded)
        sbuf[i++] = L'f';

    if (tf.b.proxiable)
        sbuf[i++] = L'P';

    if (tf.b.proxy)
        sbuf[i++] = L'p';

    if (tf.b.may_postdate)
        sbuf[i++] = L'D';

    if (tf.b.postdated)
        sbuf[i++] = L'd';

    if (tf.b.invalid)
        sbuf[i++] = L'i';

    if (tf.b.renewable)
        sbuf[i++] = L'R';

    if (tf.b.initial)
        sbuf[i++] = L'I';

    if (tf.b.hw_authent)
        sbuf[i++] = L'H';

    if (tf.b.pre_authent)
        sbuf[i++] = L'A';

    sbuf[i++] = L'\0';

    cb = i * sizeof(wchar_t);

    if (!buf || *pcb_buf < cb) {
        *pcb_buf = cb;
        return KHM_ERROR_TOO_LONG;
    } else {
        StringCbCopy(buf, *pcb_buf, sbuf);
        *pcb_buf = cb;
        return KHM_ERROR_SUCCESS;
    }
}

khm_int32 KHMAPI
kvno_toString(const void * data, khm_size cbdata,
              wchar_t *destbuf, khm_size *pcbdestbuf,
              khm_int32 flags)
{
    int resid = 0;
    int kvno;
    wchar_t buf[256];
    size_t cblength;

    if (cbdata != sizeof(khm_int32))
        return KHM_ERROR_INVALID_PARAM;

    kvno = *((khm_int32 *) data);

    StringCbPrintf(buf, sizeof(buf), L"#%d", kvno);

    StringCbLength(buf, ARRAYLENGTH(buf), &cblength);
    cblength += sizeof(wchar_t);

    if (!destbuf || *pcbdestbuf < cblength) {
        *pcbdestbuf = cblength;
        return KHM_ERROR_TOO_LONG;
    } else {
        StringCbCopy(destbuf, *pcbdestbuf, buf);
        *pcbdestbuf = cblength;
        return KHM_ERROR_SUCCESS;
    }
}

khm_int32
serialize_krb5_addresses(const krb5_addresses * as, void * buf, size_t * pcbbuf)
{
    size_t len = 0;

    len = length_HostAddresses(as);
    if (buf == 0 || *pcbbuf < len) {
	*pcbbuf = len;
	return KHM_ERROR_TOO_LONG;
    }

    encode_HostAddresses(BYTEOFFSET(buf, len - 1), *pcbbuf, as, &len);

    return KHM_ERROR_SUCCESS;
}
