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

/* Originally this was krb5routines.c in Leash sources.  Subsequently
 * modified and adapted for NetIDMgr */

#include "krbcred.h"

#define SECURITY_WIN32
#include <security.h>

#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <strsafe.h>

/* we use these structures to keep track of identities that we find
   while going through the API, FILE and MSLSA caches and enumerating
   credentials.  The only identities we want to keep track of are the
   ones that have an initial ticket.  We collect information for each
   of the identities we find that we have initial tickets for and
   then set the properties for the identities at once. */

typedef struct tag_ident_data {
    khm_handle  ident;          /* handle to the identity */
    khm_int32 count;            /* number of initial tickets we have
                                   found for this identity. */
    wchar_t   ccname[KRB5_MAXCCH_CCNAME];
    FILETIME  ft_issue;
    FILETIME  ft_expire;
    FILETIME  ft_renewexpire;
    khm_int32 krb5_flags;

    khm_boolean imported;       /* is this an imported identity? */
} ident_data;

typedef struct tag_identlist {
    ident_data * list;
    khm_size     n_list;
    khm_size     nc_list;
} identlist;

#define IDLIST_ALLOC_INCR 8

static void
tc_prep_idlist(identlist * idlist) {
    khm_int32 rv;
    khm_size cb_ids = 0;
    khm_size n_ids = 0;
    khm_size i;
    kcdb_enumeration e = NULL;
    khm_handle ident = NULL;

    idlist->list = NULL;
    idlist->n_list = 0;
    idlist->nc_list = 0;

    rv = kcdb_identity_begin_enum(KCDB_IDENT_FLAG_ACTIVE,
                                  KCDB_IDENT_FLAG_ACTIVE,
                                  &e, &n_ids);

    if (KHM_FAILED(rv) || n_ids == 0) {
        return;
    }

    idlist->nc_list = UBOUNDSS(n_ids, IDLIST_ALLOC_INCR, IDLIST_ALLOC_INCR);

    idlist->list = PCALLOC(idlist->nc_list, sizeof(idlist->list[0]));

    i = 0;
    while (KHM_SUCCEEDED(kcdb_enum_next(e, &ident))) {

        idlist->list[i].ident = ident;
        idlist->list[i].count = 0;
        ident = NULL;

        i++;
    }

    idlist->n_list = i;

    kcdb_enum_end(e);
}

static ident_data *
tc_add_ident_to_list(identlist * idlist, khm_handle ident) {
    khm_size i;
    ident_data * d;

    for (i=0; i < idlist->n_list; i++) {
        if (kcdb_identity_is_equal(ident, idlist->list[i].ident))
            break;
    }

    if (i < idlist->n_list) {
        /* we already have this identity on our list.  Increment the
           count */
        idlist->list[i].count++;
        return &idlist->list[i];
    }

    /* it wasn't in our list.  Add it */

    if (idlist->n_list + 1 > idlist->nc_list) {
        idlist->nc_list = UBOUNDSS(idlist->n_list + 1,
                                   IDLIST_ALLOC_INCR,
                                   IDLIST_ALLOC_INCR);
#ifdef DEBUG
        assert(idlist->n_list + 1 <= idlist->nc_list);
#endif
        idlist->list = PREALLOC(idlist->list,
                                sizeof(idlist->list[0]) * idlist->nc_list);
#ifdef DEBUG
        assert(idlist->list);
#endif
        ZeroMemory(&idlist->list[idlist->n_list],
                   sizeof(idlist->list[0]) *
                   (idlist->nc_list - idlist->n_list));
    }

    d = &idlist->list[idlist->n_list];

    ZeroMemory(d, sizeof(*d));

    d->ident = ident;
    d->count = 1;

    idlist->n_list++;

    kcdb_identity_hold(ident);

    return d;
}

static void
tc_set_ident_data(identlist * idlist) {
    khm_size i;
    wchar_t k5idtype[KCDB_MAXCCH_NAME];

    k5idtype[0] = L'\0';
    LoadString(hResModule, IDS_KRB5_NC_NAME,
               k5idtype, ARRAYLENGTH(k5idtype));

    for (i=0; i < idlist->n_list; i++) {
#ifdef DEBUG
        assert(idlist->list[i].ident);
#endif

        if (idlist->list[i].count > 0) {
            khm_int32 t;

            t = credtype_id_krb5;
            kcdb_identity_set_attr(idlist->list[i].ident,
                                   KCDB_ATTR_TYPE,
                                   &t,
                                   sizeof(t));

            /* We need to manually add the type name if we want the
               name to show up in the property list for the identity.
               The type name is only automatically calculated for
               credentials. */
            kcdb_identity_set_attr(idlist->list[i].ident,
                                   KCDB_ATTR_TYPE_NAME,
                                   k5idtype,
                                   KCDB_CBSIZE_AUTO);

            kcdb_identity_set_attr(idlist->list[i].ident,
                                   attr_id_krb5_ccname,
                                   idlist->list[i].ccname,
                                   KCDB_CBSIZE_AUTO);

            kcdb_identity_set_attr(idlist->list[i].ident,
                                   KCDB_ATTR_EXPIRE,
                                   &idlist->list[i].ft_expire,
                                   sizeof(idlist->list[i].ft_expire));

            kcdb_identity_set_attr(idlist->list[i].ident,
                                   KCDB_ATTR_ISSUE,
                                   &idlist->list[i].ft_issue,
                                   sizeof(idlist->list[i].ft_issue));

            kcdb_identity_set_attr(idlist->list[i].ident,
                                   attr_id_krb5_flags,
                                   &idlist->list[i].krb5_flags,
                                   sizeof(idlist->list[i].krb5_flags));

            if (idlist->list[i].ft_renewexpire.dwLowDateTime == 0 &&
                idlist->list[i].ft_renewexpire.dwHighDateTime == 0) {
                kcdb_identity_set_attr(idlist->list[i].ident,
                                       KCDB_ATTR_RENEW_EXPIRE,
                                       NULL, 0);
            } else {
                kcdb_identity_set_attr(idlist->list[i].ident,
                                       KCDB_ATTR_RENEW_EXPIRE,
                                       &idlist->list[i].ft_renewexpire,
                                       sizeof(idlist->list[i].ft_renewexpire));
            }

            if (idlist->list[i].imported)
                khm_krb5_set_identity_flags(idlist->list[i].ident,
                                            K5IDFLAG_IMPORTED,
                                            K5IDFLAG_IMPORTED);

        } else {
            /* We didn't see any TGTs for this identity.  We have to
               remove all the Krb5 supplied properties. */

            khm_int32 t;
            khm_size cb;

            cb = sizeof(t);
            if (KHM_SUCCEEDED(kcdb_identity_get_attr(idlist->list[i].ident,
                                                     KCDB_ATTR_TYPE, NULL,
                                                     &t,
                                                     &cb)) &&
                t == credtype_id_krb5) {

                /* disown this and remove all our properties. the
                   system will GC this identity if nobody claims it.*/

                kcdb_identity_set_attr(idlist->list[i].ident,
                                       KCDB_ATTR_TYPE, NULL, 0);
                kcdb_identity_set_attr(idlist->list[i].ident,
                                       KCDB_ATTR_TYPE_NAME, NULL, 0);
                kcdb_identity_set_attr(idlist->list[i].ident,
                                       attr_id_krb5_ccname, NULL, 0);
                kcdb_identity_set_attr(idlist->list[i].ident,
                                       KCDB_ATTR_EXPIRE, NULL, 0);
                kcdb_identity_set_attr(idlist->list[i].ident,
                                       KCDB_ATTR_ISSUE, NULL, 0);
                kcdb_identity_set_attr(idlist->list[i].ident,
                                       attr_id_krb5_flags, NULL, 0);
                kcdb_identity_set_attr(idlist->list[i].ident,
                                       KCDB_ATTR_RENEW_EXPIRE, NULL, 0);
            } else {
                /* otherwise, this identity doesn't belong to us.  We
                   should leave it as is. */
            }
        }
    }
}

static void
tc_free_idlist(identlist * idlist) {
    khm_size i;

    for (i=0; i < idlist->n_list; i++) {
        if (idlist->list[i].ident != NULL) {
            kcdb_identity_release(idlist->list[i].ident);
            idlist->list[i].ident = NULL;
        }
    }

    if (idlist->list)
        PFREE(idlist->list);
    idlist->list = NULL;
    idlist->n_list = 0;
    idlist->nc_list = 0;
}

#ifndef ENCTYPE_LOCAL_RC4_MD4
#define ENCTYPE_LOCAL_RC4_MD4    0xFFFFFF80
#endif

#define MAX_ADDRS 256

static long
get_tickets_from_cache(krb5_context context,
                       krb5_ccache cache,
                       identlist * idlist)
{
    krb5_error_code code;
    krb5_principal  principal = 0;
    krb5_flags	    flags = 0;
    krb5_cc_cursor  cursor = 0;
    krb5_creds	    credentials;
    Ticket          ticket;
    size_t          len;

    char	   *client_name = NULL;
    char	   *principal_name = NULL;
    char	   *server_name = NULL;

    wchar_t         wbuf[256];  /* temporary conversion buffer */
    wchar_t         wcc_name[KRB5_MAXCCH_CCNAME]; /* credential cache name */
    khm_handle      ident = NULL;
    khm_handle      cred = NULL;
    time_t          tt;
    FILETIME        ft, eft;
    khm_int32       ti;

#ifdef KRB5_TC_NOTICKET
    flags = KRB5_TC_NOTICKET;
#endif

    {
        const char * cc_name;
        const char * cc_type;

        cc_name = krb5_cc_get_name(context, cache);
        cc_type = krb5_cc_get_type(context, cache);

        StringCbPrintf(wcc_name, sizeof(wcc_name), L"%S%s%S",
                       ((cc_type && cc_type[0] != '\0')? cc_type : ""),
                       ((cc_type && cc_type[0] != '\0')? ":":""),
                       ((cc_name)? cc_name : ""));
        khm_krb5_canon_cc_name(wcc_name, sizeof(wcc_name));
    }

    _reportf(L"Getting tickets from cache [%s]", wcc_name);

    if ((code = krb5_cc_set_flags(context, cache, flags)))
    {
        if (code != KRB5_FCC_NOFILE && code != KRB5_CC_NOTFOUND)
            khm_krb5_error(code, "krb5_cc_set_flags()", 0, &context, &cache);

        goto _exit;
    }

    if ((code = krb5_cc_get_principal(context, cache, &principal)))
    {
        if (code != KRB5_FCC_NOFILE && code != KRB5_CC_NOTFOUND)
            khm_krb5_error(code, "krb5_cc_get_principal()", 0, &context, &cache);

        goto _exit;
    }

    if ((code = krb5_unparse_name(context, principal,
				  (char **)&principal_name))) {

        khm_krb5_error(code, "krb5_unparse_name()", 0, &context, &cache);

        goto _exit;
    }

    if (!strcspn(principal_name, "@" ))
    {
        _reportf(L"principal_name contains no realm [%S]", principal_name);

        goto _exit;
    }

    AnsiStrToUnicode(wbuf, sizeof(wbuf), principal_name);
    if(KHM_FAILED(kcdb_identity_create(wbuf, KCDB_IDENT_FLAG_CREATE,
                                       &ident))) {
        _reportf(L"Can't create identity");
        code = 1;
        goto _exit;
    }

    _reportf(L"Found principal [%s]", wbuf);

    if ((code = krb5_cc_start_seq_get(context, cache, &cursor)))
    {
        goto _exit;
    }

    memset(&credentials, '\0', sizeof(credentials));

    while (!(code = krb5_cc_next_cred(context, cache, &cursor,
				      &credentials)))
    {
        khm_handle tident = NULL;
        khm_int32 cred_flags = 0;
	krb5_ticket_flags tf;

        if(client_name != NULL)
            krb5_free_unparsed_name(context, client_name);
        if(server_name != NULL)
            krb5_free_unparsed_name(context, server_name);
        if(cred)
            kcdb_cred_release(cred);

        client_name = NULL;
        server_name = NULL;
        cred = NULL;

        if (krb5_unparse_name(context, credentials.client, &client_name))
        {
            krb5_free_cred_contents(context, &credentials);
            khm_krb5_error(code, "krb5_unparse_name()", 0, &context, &cache);
            continue;
        }

        if (krb5_unparse_name(context, credentials.server, &server_name))
        {
            krb5_free_cred_contents(context, &credentials);
            khm_krb5_error(code, "krb5_unparse_name()", 0, &context, &cache);
            continue;
        }

        /* if the client_name differs from principal_name for some
           reason, we need to create a new identity */
        if(strcmp(client_name, principal_name)) {
            AnsiStrToUnicode(wbuf, sizeof(wbuf), client_name);
            if(KHM_FAILED(kcdb_identity_create_ex(k5_identpro, wbuf,
                                                  KCDB_IDENT_FLAG_CREATE,
                                                  NULL, &tident))) {
                krb5_free_cred_contents(context, &credentials);
                continue;
            }
        } else {
            tident = ident;
        }

        AnsiStrToUnicode(wbuf, sizeof(wbuf), server_name);
        if(KHM_FAILED(kcdb_cred_create(wbuf, tident, credtype_id_krb5,
                                       &cred))) {
            krb5_free_cred_contents(context, &credentials);
            continue;
        }

        _reportf(L"Ticket [%s]", wbuf);

        kcdb_cred_set_attr(cred, KCDB_ATTR_DISPLAY_NAME, wbuf, KCDB_CBSIZE_AUTO);

        if (!credentials.times.starttime)
            credentials.times.starttime = credentials.times.authtime;

        tt = credentials.times.starttime;
        TimetToFileTime(tt, &ft);
        kcdb_cred_set_attr(cred, KCDB_ATTR_ISSUE, &ft, sizeof(ft));

        tt = credentials.times.endtime;
        TimetToFileTime(tt, &eft);
        kcdb_cred_set_attr(cred, KCDB_ATTR_EXPIRE, &eft, sizeof(eft));

        if (credentials.times.renew_till > 0) {
            FILETIME ftl;

            tt = credentials.times.renew_till;
            TimetToFileTime(tt, &eft);
            kcdb_cred_set_attr(cred, KCDB_ATTR_RENEW_EXPIRE, &eft,
                               sizeof(eft));

            ftl = FtSub(&eft, &ft);
            kcdb_cred_set_attr(cred, KCDB_ATTR_RENEW_LIFETIME, &ftl,
                               sizeof(ftl));
        }

        tf = credentials.flags;
	ti = tf.i;
        kcdb_cred_set_attr(cred, attr_id_krb5_flags, &ti, sizeof(ti));

        /* special flags understood by NetIDMgr */
        {
            khm_int32 nflags = 0;

            if (tf.b.renewable)
                nflags |= KCDB_CRED_FLAG_RENEWABLE;

            if (tf.b.initial)
                nflags |= KCDB_CRED_FLAG_INITIAL;
	    else {
		const char * c0, *c1, *r;

		/* these are macros that do not allocate any memory */
		c0 = krb5_principal_get_comp_string(context,credentials.server,0);
		c1 = krb5_principal_get_comp_string(context,credentials.server,1);
		r  = krb5_principal_get_realm(context,credentials.server);

		if ( c0 && c1 && r &&
		     !strcmp(c1,r) &&
		     !strcmp("krbtgt",c0) )
		    nflags |= KCDB_CRED_FLAG_INITIAL;
	    }

            kcdb_cred_set_flags(cred, nflags, KCDB_CRED_FLAGMASK_EXT);

            cred_flags = nflags;
        }

        if ( !decode_Ticket(credentials.ticket.data, credentials.ticket.length,
			    &ticket, &len) ) {
            ti = ticket.enc_part.etype;
            kcdb_cred_set_attr(cred, attr_id_tkt_enctype, &ti, sizeof(ti));
	    if (ticket.enc_part.kvno) {
		ti = *ticket.enc_part.kvno;
		kcdb_cred_set_attr(cred, attr_id_kvno, &ti, sizeof(ti));
	    }
            free_Ticket(&ticket);
        }

        ti = credentials.session.keytype;
        kcdb_cred_set_attr(cred, attr_id_key_enctype, &ti, sizeof(ti));

        kcdb_cred_set_attr(cred, KCDB_ATTR_LOCATION, wcc_name,
                           KCDB_CBSIZE_AUTO);

        if ( credentials.addresses.len > 0 ) {
	    khm_int32 buffer[1024];

	    void * bufp;
	    khm_size cb;
	    khm_int32 rv;

	    bufp = (void *) buffer;
	    cb = sizeof(buffer);

	    rv = serialize_krb5_addresses(&credentials.addresses,
					  bufp, &cb);
	    if (rv == KHM_ERROR_TOO_LONG) {
		bufp = PMALLOC(cb);
		rv = serialize_krb5_addresses(&credentials.addresses,
					      bufp, &cb);
	    }

	    if (KHM_SUCCEEDED(rv)) {
		kcdb_cred_set_attr(cred, attr_id_addr_list,
				   bufp, cb);
	    }

	    if (bufp != (void *) buffer)
		PFREE(bufp);
        }

        if(cred_flags & KCDB_CRED_FLAG_INITIAL) {
            FILETIME ft_issue_new;
            FILETIME ft_expire_old;
            FILETIME ft_expire_new;
            ident_data * d;

            /* an initial ticket!  Add it to the list of identities we
               have seen so far with initial tickets. */
            d = tc_add_ident_to_list(idlist, ident);
            assert(d);
            assert(d->count > 0);

            tt = credentials.times.endtime;
            TimetToFileTime(tt, &ft_expire_new);

            tt = credentials.times.starttime;
            TimetToFileTime(tt, &ft_issue_new);

            /* so now, we have to set the properties of the identity
               based on the properties of this credential under the
               following circumstances:

               - If this is the first time we are hitting this
	       identity.

               - If this is not the MSLSA: cache and the expiry time
	       for this credential is longer than the time already
	       found for this identity.
            */

            ft_expire_old = d->ft_expire;

	    /* If we find the identity in MSLSA: and any other
	       credentials cache, we assume the identity was
	       imported. */
            if (((!!wcscmp(wcc_name, L"MSLSA:")) ^ (!!wcscmp(d->ccname, L"MSLSA:"))) &&
                CompareFileTime(&ft_expire_new, &ft_expire_old) == 0) {
                _reportf(L"Setting identity as imported");
                d->imported = TRUE;
            }

            if(d->count == 1
               || (CompareFileTime(&ft_expire_new, &ft_expire_old) > 0 &&
                   wcscmp(wcc_name, L"MSLSA:") != 0)) {

                _reportf(L"Setting properties for identity (count=%d)", d->count);

                StringCbCopy(d->ccname, sizeof(d->ccname),
                             wcc_name);
                d->ft_expire = ft_expire_new;
                d->ft_issue = ft_issue_new;

                if (credentials.times.renew_till > 0) {
                    tt = credentials.times.renew_till;
                    TimetToFileTime(tt, &ft);
                    d->ft_renewexpire = ft;
                } else {
                    ZeroMemory(&d->ft_renewexpire, sizeof(d->ft_renewexpire));
                }

                d->krb5_flags = credentials.flags.i;
            }
        }

        kcdb_credset_add_cred(krb5_credset, cred, -1);

        krb5_free_cred_contents(context, &credentials);

        if(tident != ident)
            kcdb_identity_release(tident);
    }

_exit:
    if (principal != 0)
        krb5_free_principal(context, principal);

    if (principal_name != NULL)
        krb5_free_unparsed_name(context, principal_name);

    if (client_name != NULL)
        krb5_free_unparsed_name(context, client_name);

    if (server_name != NULL)
        krb5_free_unparsed_name(context, server_name);

    if (cred)
        kcdb_cred_release(cred);

    if (cursor != 0) {
        krb5_cc_end_seq_get(context, cache, &cursor);
    }

    return code;
}

long
khm_krb5_list_tickets(krb5_context *pcontext)
{
    krb5_context	context = NULL;
    krb5_ccache		cache = NULL;
    krb5_error_code	code = 0;
    khm_int32           t;
    wchar_t *           ms = NULL;
    khm_size            cb;
    identlist           idl;

    krb5_cccol_cursor   cc_cursor = 0;

    kcdb_credset_flush(krb5_credset);
    tc_prep_idlist(&idl);

    if((*pcontext == 0) && (code = krb5_init_context(pcontext))) {
        goto _exit;
    }

    context = (*pcontext);

    if (krb5_cccol_cursor_new(context, &cc_cursor) != 0)
        goto _skip_cc_iter;

    while (krb5_cccol_cursor_next(context, cc_cursor, &cache) == 0 && cache != 0) {
        code = get_tickets_from_cache(context, cache, &idl);

        krb5_cc_close(context, cache);
        cache = 0;
    }

    krb5_cccol_cursor_free(context, &cc_cursor);

_skip_cc_iter:

    if (khc_read_multi_string(csp_params, L"FileCCList", NULL, &cb)
        == KHM_ERROR_TOO_LONG &&
        cb > sizeof(wchar_t) * 2) {
        wchar_t * t;
        char ccname[KRB5_MAXCCH_CCNAME];

        ms = PMALLOC(cb);
        assert(ms);
        khc_read_multi_string(csp_params, L"FileCCList", ms, &cb);

        for(t = ms; t && *t; t = multi_string_next(t)) {
            wchar_t exppath[KRB5_MAXCCH_CCNAME];
            DWORD len;

            if (wcschr(t, L'%')) {
                len = ExpandEnvironmentStrings(t, exppath, ARRAYLENGTH(exppath));
                if (len == 0 || len > ARRAYLENGTH(exppath)) {
                    _reportf(L"Skipping path [%s].  Expansion overflowed MAX_PATH", t);
                    continue;
                }
            } else {
                StringCbCopy(exppath, sizeof(exppath), t);
            }

            StringCchPrintfA(ccname, ARRAYLENGTH(ccname),
                             "FILE:%S", exppath);

            code = krb5_cc_resolve(context, ccname, &cache);

            if (code) {
                khm_krb5_error(code, "krb5_cc_resolve()", 0, &context, &cache);
                continue;
            }

            code = get_tickets_from_cache(context, cache, &idl);

            if (context != NULL && cache != NULL)
                krb5_cc_close(context, cache);
            cache = 0;
        }

        PFREE(ms);
    }

    if (KHM_SUCCEEDED(khc_read_int32(csp_params, L"MsLsaList", &t)) && t) {
        code = krb5_cc_resolve(context, "MSLSA:", &cache);

        if (code == 0 && cache) {
            code = get_tickets_from_cache(context, cache, &idl);
        } else {
            khm_krb5_error(code, "krb5_cc_resolve()", 0, &context, &cache);
        }

        if (context != NULL && cache != NULL)
            krb5_cc_close(context, cache);
        cache = 0;
    }

_exit:

    kcdb_credset_collect(NULL, krb5_credset, NULL, credtype_id_krb5, NULL);
    tc_set_ident_data(&idl);
    tc_free_idlist(&idl);

    return(code);
}

int
khm_krb5_renew_cred(khm_handle cred)
{
    khm_handle          identity = NULL;
    krb5_error_code     code = 0;
    krb5_context        context = NULL;
    krb5_ccache         cc = NULL;
    krb5_principal	me = NULL, server = NULL;
    krb5_creds          in_creds, cc_creds;
    krb5_creds		* out_creds = NULL;

    wchar_t 		wname[512];
    khm_size		cbname;
    char                name[512];
    khm_boolean		brenewIdentity = FALSE;
    khm_boolean		istgt = FALSE;

    khm_int32           flags;
    int                 ccflags = 0;

    cbname = sizeof(wname);
    kcdb_cred_get_name(cred, wname, &cbname);
    _reportf(L"Krb5 renew cred for %s", wname);

    kcdb_cred_get_flags(cred, &flags);

    if (!(flags & KCDB_CRED_FLAG_INITIAL)) {
        _reportf(L"Krb5 skipping renewal because this is not an initial credential");
        return 0;
    }

    memset(&in_creds, 0, sizeof(in_creds));
    memset(&cc_creds, 0, sizeof(cc_creds));

    if (cred == NULL) {
#ifdef DEBUG
	assert(FALSE);
#endif
	goto cleanup;
    }

    if (KHM_FAILED(kcdb_cred_get_identity(cred, &identity))) {
#ifdef DEBUG
	assert(FALSE);
#endif
	goto cleanup;
    }

    code = khm_krb5_initialize(identity, &context, &cc);
    if (code)
	goto cleanup;

    code = krb5_cc_get_principal(context, cc, &me);
    if (code)
        goto cleanup;

    cbname = sizeof(wname);
    if (KHM_FAILED(kcdb_cred_get_name(cred, wname, &cbname)))
	goto cleanup;

    UnicodeStrToAnsi(name, sizeof(name), wname);

    code = krb5_parse_name(context, name, &server);
    if (code)
	goto cleanup;

    in_creds.client = me;
    in_creds.server = server;

    if (strcmp("krbtgt", krb5_principal_get_comp_string(context, server, 0))) {
	code = krb5_get_renewed_creds(context, &cc_creds, me, cc, name);
	if (code) {
	    code = krb5_cc_retrieve_cred(context, cc, 0, &in_creds, &cc_creds);
	    if (code == 0) {
		code = krb5_cc_remove_cred(context, cc, 0, &cc_creds);
		if (code) {
		    brenewIdentity = TRUE;
		    goto cleanup;
		}
	    }
	}

	code = krb5_get_credentials(context, 0, cc, &in_creds, &out_creds);
    } else {
	istgt = TRUE;
	code = krb5_get_renewed_creds(context, &cc_creds, me, cc, NULL);
    }

    if (code) {
	if ( code != KRB5KDC_ERR_ETYPE_NOSUPP ||
	     code != KRB5_KDC_UNREACH)
	    khm_krb5_error(code, "krb5_get_renewed_creds()", 0, &context, &cc);
	goto cleanup;
    }

    if (istgt) {
	code = krb5_cc_initialize(context, cc, me);
	if (code) goto cleanup;
    }

    code = krb5_cc_store_cred(context, cc, istgt ? &cc_creds : out_creds);
    if (code) goto cleanup;


cleanup:
    if (in_creds.client == me)
        in_creds.client = NULL;
    if (in_creds.server == server)
        in_creds.server = NULL;

    if (me)
	krb5_free_principal(context, me);

    if (server)
	krb5_free_principal(context, server);

    krb5_free_cred_contents(context, &in_creds);
    krb5_free_cred_contents(context, &cc_creds);

    if (out_creds)
	krb5_free_creds(context, out_creds);

    if (cc && context)
	krb5_cc_close(context, cc);

    if (context)
	krb5_free_context(context);

    if (identity) {
	if (brenewIdentity)
	    code = khm_krb5_renew_ident(identity);
	kcdb_identity_release(identity);
    }

    return code;
}

int
khm_krb5_renew_ident(khm_handle identity)
{
    krb5_error_code     code = 0;
    krb5_context        context = NULL;
    krb5_ccache         cc = NULL;
    krb5_principal      me = NULL;
    krb5_principal      server = NULL;
    krb5_creds          my_creds;
    const char         *realm = NULL;
    wchar_t             idname[KCDB_IDENT_MAXCCH_NAME];
    khm_size            cb;
    khm_int32           k5_flags;

    memset(&my_creds, 0, sizeof(krb5_creds));

    cb = sizeof(idname);
    kcdb_identity_get_name(identity, idname, &cb);

    if (khm_krb5_get_identity_flags(identity) & K5IDFLAG_IMPORTED) {
#ifndef NO_REIMPORT_MSLSA_CREDS
        /* we are trying to renew the identity that was imported from
           MSLSA: */
        BOOL  imported;
        BOOL retry_import = FALSE;
        char  cidname[KCDB_IDENT_MAXCCH_NAME];
        khm_handle imported_id = NULL;
        khm_size cb;
        FILETIME ft_expire;
        FILETIME ft_now;
        FILETIME ft_threshold;
        krb5_principal princ = NULL;

        /* assume failure */
        code = EINVAL;

        kherr_reportf(L"Renewing identity using ms2mit [%s]", idname);

        UnicodeStrToAnsi(cidname, sizeof(cidname), idname);

        imported = khm_krb5_ms2mit(cidname, FALSE, TRUE, &imported_id);

        if (imported == 0)
            goto import_failed;

        /* if the imported identity has already expired or will soon,
           we clear the cache and try again. */
        khm_krb5_list_tickets(&context);

        cb = sizeof(ft_expire);
        if (KHM_FAILED(kcdb_identity_get_attr(imported_id, KCDB_ATTR_EXPIRE,
                                              NULL, &ft_expire, &cb)))
            goto import_failed;

        GetSystemTimeAsFileTime(&ft_now);
        TimetToFileTimeInterval(5 * 60, &ft_threshold);

        ft_now = FtAdd(&ft_now, &ft_threshold);

        code = 0;

        if (CompareFileTime(&ft_expire, &ft_now) < 0) {
            /* the ticket lifetime is not long enough */

            if (context == NULL)
                code = krb5_init_context(&context);
            if (code)
                goto import_failed;

            code = krb5_cc_resolve(context, "MSLSA:", &cc);
            if (code) {
                khm_krb5_error(code, "krb5_cc_resolve()", 0, &context, &cc);
                goto import_failed;
            }

            code = krb5_cc_get_principal(context, cc, &princ);
            if (code) {
                khm_krb5_error(code, "krb5_cc_get_principal()", 0, &context, &cc);
                goto import_failed;
            }

            krb5_cc_initialize(context, cc, princ);

            retry_import = TRUE;
        }

    import_failed:

        if (imported_id) {
            kcdb_identity_release(imported_id);
            imported_id = NULL;
        }

        if (context) {
            if (cc) {
                krb5_cc_close(context, cc);
                cc = NULL;
            }

            if (princ) {
                krb5_free_principal(context, princ);
                princ = NULL;
            }

            /* leave context so we can use it later */
        }

        if (retry_import)
            imported = khm_krb5_ms2mit(cidname, FALSE, TRUE, NULL);

        if (imported)
            goto cleanup;

        /* if the import failed, then we try to renew the identity via
           the usual procedure. */

#else
        /* if we are suppressing further imports from MSLSA, we just
           skip renewing this identity. */
        goto cleanup;
#endif
    }

    cb = sizeof(k5_flags);
    if (KHM_SUCCEEDED(kcdb_identity_get_attr(identity,
                                             attr_id_krb5_flags,
                                             NULL,
                                             &k5_flags,
                                             &cb))) {
	krb5_ticket_flags tf;

	tf.i = k5_flags;

	if (!tf.b.renewable) {
	    kherr_reportf(L"Identity is not renewable");

	    code = KRB5KDC_ERR_BADOPTION;
	    goto cleanup;
	}
    }

    {
        FILETIME ft_now;
        FILETIME ft_exp;

        cb = sizeof(ft_exp);
        GetSystemTimeAsFileTime(&ft_now);
        if (KHM_SUCCEEDED(kcdb_identity_get_attr(identity,
                                                 KCDB_ATTR_EXPIRE,
                                                 NULL,
                                                 &ft_exp,
                                                 &cb)) &&
            CompareFileTime(&ft_exp, &ft_now) < 0) {

            code = KRB5KRB_AP_ERR_TKT_EXPIRED;
            goto cleanup;

        }
    }

    code = khm_krb5_initialize(identity, &context, &cc);
    if (code)
        goto cleanup;

    code = krb5_cc_get_principal(context, cc, &me);
    if (code) {
        khm_krb5_error(code, "krb5_cc_get_principal()", 0, &context, &cc);
        goto cleanup;
    }

    realm = krb5_principal_get_realm(context, me);

    code = krb5_build_principal(context, &server,
                                strlen(realm), realm, KRB5_TGS_NAME, realm, 0);

    if (code)
        goto cleanup;

    my_creds.client = me;
    my_creds.server = server;

    code = krb5_get_renewed_creds(context, &my_creds, me, cc, NULL);
    if (code) {
        if ( code != KRB5KDC_ERR_ETYPE_NOSUPP ||
	     code != KRB5_KDC_UNREACH)
            khm_krb5_error(code, "krb5_get_renewed_creds()", 0, &context, &cc);
        goto cleanup;
    }

    code = krb5_cc_initialize(context, cc, me);
    if (code) {
        khm_krb5_error(code, "krb5_cc_initialize()", 0, &context, &cc);
        goto cleanup;
    }

    code = krb5_cc_store_cred(context, cc, &my_creds);
    if (code) {
        khm_krb5_error(code, "krb5_cc_store_cred()", 0, &context, &cc);
        goto cleanup;
    }

cleanup:
    if (my_creds.client == me)
        my_creds.client = NULL;
    if (my_creds.server == server)
        my_creds.server = NULL;

    if (context) {
        krb5_free_cred_contents(context, &my_creds);

        if (me)
            krb5_free_principal(context, me);
        if (server)
            krb5_free_principal(context, server);
        if (cc)
            krb5_cc_close(context, cc);
        krb5_free_context(context);
    }

    return(code);
}

int
khm_krb5_kinit(krb5_context       alt_context,
               char *             principal_name,
               char *             password,
               char *             ccache,
               krb5_deltat        lifetime,
               DWORD              forwardable,
               DWORD              proxiable,
               krb5_deltat        renew_life,
               DWORD              addressless,
               DWORD              publicIP,
               krb5_prompter_fct  prompter,
               void *             p_data)
{
    krb5_error_code		        code = 0;
    krb5_context		        context = NULL;
    krb5_ccache			        cc = NULL;
    krb5_principal		        me = NULL;
    char*                       name = NULL;
    krb5_creds			        my_creds;
    krb5_get_init_creds_opt     *options = NULL;
    int                         i = 0, addr_count = 0;

    _reportf(L"In khm_krb5_kinit");

    memset(&my_creds, 0, sizeof(my_creds));

    if (alt_context) {
        context = alt_context;
    } else {
        code = krb5_init_context(&context);
        if (code)
            goto cleanup;
    }

    if ((code = krb5_get_init_creds_opt_alloc(context, &options)) != 0)
	goto cleanup;

    if (ccache) {
        _reportf(L"Using supplied ccache name %S", ccache);
        code = krb5_cc_resolve(context, ccache, &cc);
    } else {
	khm_handle identity = NULL;
	wchar_t idname[KCDB_IDENT_MAXCCH_NAME];
	char ccname[KRB5_MAXCCH_CCNAME];
	char * pccname = principal_name;
	khm_size cb;

	idname[0] = L'\0';
	AnsiStrToUnicode(idname, sizeof(idname), principal_name);

	cb = sizeof(ccname);

	if (KHM_SUCCEEDED(kcdb_identity_create(idname, 0, &identity)) &&
            KHM_SUCCEEDED(khm_krb5_get_identity_default_ccacheA(identity, ccname, &cb))) {

	    pccname = ccname;

	}

	if (identity)
	    kcdb_identity_release(identity);

        code = krb5_cc_resolve(context, pccname, &cc);
    }

    if (code) {
        khm_krb5_error(code, "krb5_cc_resolve()", 0, &context, &cc);
        goto cleanup;
    }

    code = krb5_parse_name(context, principal_name, &me);
    if (code) goto cleanup;

    code = krb5_unparse_name(context, me, &name);
    if (code) goto cleanup;

    if (lifetime == 0) {
	khm_int32 i = 0;
        khc_read_int32(csp_params, L"DefaultLifetime", &i);
	lifetime = i;
    }

    if (lifetime)
        krb5_get_init_creds_opt_set_tkt_life(options, lifetime);

    krb5_get_init_creds_opt_set_forwardable(options,
					    forwardable ? 1 : 0);
    krb5_get_init_creds_opt_set_proxiable(options,
					  proxiable ? 1 : 0);
    krb5_get_init_creds_opt_set_renew_life(options,
					   renew_life);

    if (addressless) {
        krb5_get_init_creds_opt_set_addressless(context, options, TRUE);
    } else {
	krb5_addresses addrs;

	krb5_get_all_client_addrs(context, &addrs);
        if (publicIP) {
            // we are going to add the public IP address specified by the user
            // to the list provided by the operating system

	    struct sockaddr_in ai;
	    krb5_addresses p_addrs;
	    krb5_address p_addr;

	    ai.sin_family = AF_INET;
	    ai.sin_port = 0;
	    ai.sin_addr.S_un.S_addr = htonl(publicIP);

	    krb5_sockaddr2address(context, (struct sockaddr *) &ai, &p_addr);

	    p_addrs.len = 1;
	    p_addrs.val = &p_addr;

	    krb5_append_addresses(context, &addrs, &p_addrs);

	    krb5_free_address(context, &p_addr);
        }

	krb5_get_init_creds_opt_set_address_list(options, &addrs);
	krb5_free_addresses(context, &addrs);
    }

    code =
        krb5_get_init_creds_password(context,
				     &my_creds,
				     me,
				     password, // password
				     prompter, // prompter
				     p_data, // prompter data
				     0, // start time
				     0, // service name
				     options);
    if (code) {
        khm_krb5_error(code, "krb5_get_init_creds_password()", 0, &context, NULL);
        goto cleanup;
    }

    code = krb5_cc_initialize(context, cc, me);
    if (code) {
        khm_krb5_error(code, "krb5_cc_initialize()", 0, &context, NULL);
        goto cleanup;
    }

    code = krb5_cc_store_cred(context, cc, &my_creds);
    if (code) {
        khm_krb5_error(code, "krb5_cc_store_cred()", 0, &context, NULL);
        goto cleanup;
    }

cleanup:
    if (my_creds.client == me)
        my_creds.client = 0;
    krb5_free_cred_contents(context, &my_creds);
    if (name)
        krb5_free_unparsed_name(context, name);
    if (me)
        krb5_free_principal(context, me);
    if (cc)
        krb5_cc_close(context, cc);
    if (options)
	krb5_get_init_creds_opt_free(context, options);
    if (context && (context != alt_context))
        krb5_free_context(context);
    return(code);
}

long
khm_krb5_copy_ccache_by_name(krb5_context in_context,
                             wchar_t * wscc_dest,
                             wchar_t * wscc_src) {
    krb5_context context = NULL;
    krb5_error_code code = 0;
    khm_boolean free_context;
    krb5_ccache cc_src = NULL;
    krb5_ccache cc_dest = NULL;
    krb5_principal princ_src = NULL;
    char scc_dest[KRB5_MAXCCH_CCNAME];
    char scc_src[KRB5_MAXCCH_CCNAME];
    int t;

    _reportf(L"Trying to copy CC %s to %s", wscc_src, wscc_dest);

    t = UnicodeStrToAnsi(scc_dest, sizeof(scc_dest), wscc_dest);
    if (t == 0)
        return KHM_ERROR_TOO_LONG;
    t = UnicodeStrToAnsi(scc_src, sizeof(scc_src), wscc_src);
    if (t == 0)
        return KHM_ERROR_TOO_LONG;

    if (in_context) {
        context = in_context;
        free_context = FALSE;
    } else {
        code = krb5_init_context(&context);
        if (code) {
            if (context)
                krb5_free_context(context);
            return code;
        }
        free_context = TRUE;
    }

    code = krb5_cc_resolve(context, scc_dest, &cc_dest);
    if (code) {
        khm_krb5_error(code, "krb5_cc_resolve() for dest", 0, &context, NULL);
        goto _cleanup;
    }

    code = krb5_cc_resolve(context, scc_src, &cc_src);
    if (code) {
        khm_krb5_error(code, "krb5_cc_resolve() for source", 0, &context, NULL);
        goto _cleanup;
    }

    code = krb5_cc_get_principal(context, cc_src, &princ_src);
    if (code) {
        khm_krb5_error(code, "krb5_cc_get_principal()", 0, &context, &cc_src);
        goto _cleanup;
    }

    code = krb5_cc_initialize(context, cc_dest, princ_src);
    if (code) {
        khm_krb5_error(code, "krb5_cc_initialize()", 0, &context, &cc_dest);
        goto _cleanup;
    }

    code = krb5_cc_copy_creds(context, cc_src, cc_dest);
    if (code) {
        khm_krb5_error(code, "krb5_cc_copy_creds()", 0, &context, NULL);
    }

_cleanup:
    if (princ_src)
        krb5_free_principal(context, princ_src);

    if (cc_dest)
        krb5_cc_close(context, cc_dest);

    if (cc_src)
        krb5_cc_close(context, cc_src);

    if (free_context && context)
        krb5_free_context(context);

    return code;
}

khm_int32
khm_krb5_get_identity_for_ccache(krb5_context context, const wchar_t * ccnameW,
                                 khm_handle *pidentity)
{
    khm_int32 rv = KHM_ERROR_NOT_FOUND;
    krb5_error_code code;
    krb5_ccache cc = NULL;
    krb5_principal principal = NULL;
    char * princ_nameA = NULL;
    wchar_t princ_nameW[KCDB_IDENT_MAXCCH_NAME];
    const char * name = NULL;

    *pidentity = NULL;

    if (ccnameW == NULL) {
        code = krb5_cc_default(context, &cc);
    } else {
        char ccnameA[KRB5_MAXCCH_CCNAME] = "";

        UnicodeStrToAnsi(ccnameA, sizeof(ccnameA), ccnameW);
        code = krb5_cc_resolve(context, ccnameA, &cc);
    }

    if (code) {
        _reportf(L"Can't open default ccache. code=%d", code);
        goto cleanup;
    }

    code = krb5_cc_get_principal(context, cc, &principal);
    if (code) {

        /* Try to determine the identity from the ccache name */
        const char * type;

        type = krb5_cc_get_type(context, cc);
        if (stricmp(type, "API") != 0) {
            _reportf(L"Credentials cache [%S] is not an API cache", name);
            goto cleanup;
        }

        name = krb5_cc_get_name(context, cc);
        _reportf(L"CC name is [%S]", (name ? name : "[Unknown cache name]"));

        AnsiStrToUnicode(princ_nameW, sizeof(princ_nameW), name);
        if (KHM_SUCCEEDED(khm_krb5_validate_name(context, princ_nameW))) {
            rv = kcdb_identity_create_ex(k5_identpro, princ_nameW,
                                         KCDB_IDENT_FLAG_CREATE, NULL, pidentity);
        } else {
            _reportf(L"Credentials cache name [%s] is not a valid principal name",
                     princ_nameW);
        }
    } else {

        code = krb5_unparse_name(context, principal, &princ_nameA);
        if (code)
            goto cleanup;

        AnsiStrToUnicode(princ_nameW, sizeof(princ_nameW), princ_nameA);

        _reportf(L"Found principal [%s]", princ_nameW);

        rv = kcdb_identity_create_ex(k5_identpro, princ_nameW,
                                     KCDB_IDENT_FLAG_CREATE, NULL, pidentity);

    }

cleanup:
    if (cc)
        krb5_cc_close(context, cc);

    if (princ_nameA)
        krb5_free_unparsed_name(context, princ_nameA);

    if (principal)
        krb5_free_principal(context, principal);

    return rv;
}

khm_int32
khm_krb5_validate_name(krb5_context context, const wchar_t * name)
{
    krb5_principal princ = NULL;
    char princ_name[KCDB_IDENT_MAXCCH_NAME];
    krb5_error_code code;
    wchar_t * atsign;

    if(UnicodeStrToAnsi(princ_name, sizeof(princ_name), name) == 0) {
        return KHM_ERROR_INVALID_NAME;
    }

    code = krb5_parse_name(context, princ_name, &princ);

    if (code) {
        return KHM_ERROR_INVALID_NAME;
    }

    if (princ != NULL)
        krb5_free_principal(context, princ);

    /* krb5_parse_name() accepts principal names with no realm or an
       empty realm.  We don't. */
    atsign = wcschr(name, L'@');
    if (atsign == NULL || atsign[1] == L'\0') {
        return KHM_ERROR_INVALID_NAME;
    }

    return KHM_ERROR_SUCCESS;
}

long
khm_krb5_sync_default_id_with_mslsa(void)
{
    long rv = KHM_ERROR_UNKNOWN;
    khm_handle def_id = NULL;
    wchar_t ccname[KRB5_MAXCCH_CCNAME];
    khm_size cb;

    _reportf(L"Attempting to synchronize default identity into MSLSA:");

    if (k5_identpro == NULL ||
        KHM_FAILED(kcdb_identity_get_default_ex(k5_identpro, &def_id)))
        return KHM_ERROR_NOT_FOUND;

    cb = sizeof(ccname);
    if (KHM_FAILED(kcdb_identity_get_attr(def_id, attr_id_krb5_ccname, NULL,
                                          ccname, &cb)))
        goto _cleanup;

    if (!wcscmp(ccname, L"MSLSA:")) {
        rv = 0;
        goto _cleanup;
    }

    rv = khm_krb5_copy_ccache_by_name(NULL, L"MSLSA:", ccname);

_cleanup:
    if (def_id)
        kcdb_identity_release(def_id);
    return rv;
}

long
khm_krb5_canon_cc_name(wchar_t * wcc_name,
                       size_t cb_cc_name) {
    size_t cb_len;
    wchar_t * colon;

    if (FAILED(StringCbLength(wcc_name,
                              cb_cc_name,
                              &cb_len))) {
#ifdef DEBUG
        assert(FALSE);
#else
        return KHM_ERROR_TOO_LONG;
#endif
    }

    cb_len += sizeof(wchar_t);

    colon = wcschr(wcc_name, L':');

    if (colon) {
        /* if the colon is just 1 character away from the beginning,
           it's a FILE: cc */
        if (colon - wcc_name == 1) {
            if (cb_len + 5 * sizeof(wchar_t) > cb_cc_name)
                return KHM_ERROR_TOO_LONG;

            memmove(&wcc_name[5], &wcc_name[0], cb_len);
            memcpy(&wcc_name[0], L"FILE:", sizeof(wchar_t) * 5);
        }

        return 0;
    }

    if (cb_len + 4 * sizeof(wchar_t) > cb_cc_name)
        return KHM_ERROR_TOO_LONG;

    memmove(&wcc_name[4], &wcc_name[0], cb_len);
    memmove(&wcc_name[0], L"API:", sizeof(wchar_t) * 4);

    return 0;
}

int
khm_krb5_cc_name_cmp(const wchar_t * cc_name_1,
                     const wchar_t * cc_name_2) {
    if (!wcsncmp(cc_name_1, L"API:", 4))
        cc_name_1 += 4;

    if (!wcsncmp(cc_name_2, L"API:", 4))
        cc_name_2 += 4;

    return wcscmp(cc_name_1, cc_name_2);
}

static khm_int32 KHMAPI
khmint_location_comp_func(khm_handle cred1,
                          khm_handle cred2,
                          void * rock) {
    return kcdb_creds_comp_attr(cred1, cred2, KCDB_ATTR_LOCATION);
}

struct khmint_location_check {
    khm_handle credset;
    khm_handle cred;
    wchar_t * ccname;
    khm_boolean success;
};

static khm_int32 KHMAPI
khmint_find_matching_cred_func(khm_handle cred,
                               void * rock) {
    struct khmint_location_check * lc;

    lc = (struct khmint_location_check *) rock;

    if (!kcdb_creds_is_equal(cred, lc->cred))
        return KHM_ERROR_SUCCESS;
    if (kcdb_creds_comp_attr(cred, lc->cred, KCDB_ATTR_LOCATION))
        return KHM_ERROR_SUCCESS;

    /* found it */
    lc->success = TRUE;

    /* break the search */
    return !KHM_ERROR_SUCCESS;
}

static khm_int32 KHMAPI
khmint_location_check_func(khm_handle cred,
                           void * rock) {
    khm_int32 t;
    khm_size cb;
    wchar_t ccname[KRB5_MAXCCH_CCNAME];
    struct khmint_location_check * lc;

    lc = (struct khmint_location_check *) rock;

    if (KHM_FAILED(kcdb_cred_get_type(cred, &t)))
        return KHM_ERROR_SUCCESS;

    if (t != credtype_id_krb5)
        return KHM_ERROR_SUCCESS;

    cb = sizeof(ccname);
    if (KHM_FAILED(kcdb_cred_get_attr(cred,
                                      KCDB_ATTR_LOCATION,
                                      NULL,
                                      ccname,
                                      &cb)))
        return KHM_ERROR_SUCCESS;

    if(wcscmp(ccname, lc->ccname))
        return KHM_ERROR_SUCCESS;

    lc->cred = cred;

    lc->success = FALSE;

    kcdb_credset_apply(lc->credset,
                       khmint_find_matching_cred_func,
                       (void *) lc);

    if (!lc->success)
        return KHM_ERROR_NOT_FOUND;
    else
        return KHM_ERROR_SUCCESS;
}

static khm_int32 KHMAPI
khmint_delete_location_func(khm_handle cred,
                            void * rock) {
    wchar_t cc_cred[KRB5_MAXCCH_CCNAME];
    struct khmint_location_check * lc;
    khm_size cb;

    lc = (struct khmint_location_check *) rock;

    cb = sizeof(cc_cred);

    if (KHM_FAILED(kcdb_cred_get_attr(cred,
                                      KCDB_ATTR_LOCATION,
                                      NULL,
                                      cc_cred,
                                      &cb)))
        return KHM_ERROR_SUCCESS;

    if (wcscmp(cc_cred, lc->ccname))
        return KHM_ERROR_SUCCESS;

    kcdb_credset_del_cred_ref(lc->credset,
                              cred);

    return KHM_ERROR_SUCCESS;
}

int
khm_krb5_destroy_by_credset(khm_handle p_cs)
{
    khm_handle d_cs = NULL;
    khm_int32 rv = KHM_ERROR_SUCCESS;
    khm_size s, cb;
    krb5_context context = NULL;
    krb5_error_code code = 0;
    int i;
    wchar_t ccname[KRB5_MAXCCH_CCNAME];
    struct khmint_location_check lc;

    rv = kcdb_credset_create(&d_cs);

    assert(KHM_SUCCEEDED(rv) && d_cs != NULL);

    kcdb_credset_extract(d_cs, p_cs, NULL, credtype_id_krb5);

    kcdb_credset_get_size(d_cs, &s);

    if (s == 0) {
        _reportf(L"No tickets to delete");

        kcdb_credset_delete(d_cs);
        return 0;
    }

    code = krb5_init_context(&context);
    if (code != 0) {
        rv = code;
        goto _cleanup;
    }

    /* we should synchronize the credential lists before we attempt to
       make any assumptions on the state of the root credset */
    khm_krb5_list_tickets(&context);

    /* so, we need to make a decision about whether to destroy entire
       ccaches or just individual credentials.  Therefore we first
       sort them by ccache. */
    kcdb_credset_sort(d_cs,
                      khmint_location_comp_func,
                      NULL);

    /* now, for each ccache we encounter, we check if we have all the
       credentials from that ccache in the to-be-deleted list. */
    for (i=0; i < (int) s; i++) {
        khm_handle cred;

        if (KHM_FAILED(kcdb_credset_get_cred(d_cs,
                                             i,
                                             &cred)))
            continue;

        cb = sizeof(ccname);
        rv = kcdb_cred_get_attr(cred,
                                KCDB_ATTR_LOCATION,
                                NULL,
                                ccname,
                                &cb);

#ifdef DEBUG
        assert(KHM_SUCCEEDED(rv));
#endif
        kcdb_cred_release(cred);

        lc.credset = d_cs;
        lc.cred = NULL;
        lc.ccname = ccname;
        lc.success = FALSE;

        kcdb_credset_apply(NULL,
                           khmint_location_check_func,
                           (void *) &lc);

        if (lc.success) {
            /* ok the destroy the ccache */
            char a_ccname[KRB5_MAXCCH_CCNAME];
            krb5_ccache cc = NULL;

            _reportf(L"Destroying ccache [%s]", ccname);

            UnicodeStrToAnsi(a_ccname,
                             sizeof(a_ccname),
                             ccname);

            code = krb5_cc_resolve(context,
				   a_ccname,
				   &cc);
            if (code)
                goto _delete_this_set;

            code = krb5_cc_destroy(context, cc);

            if (code) {
                khm_krb5_error(code, "krb5_cc_destroy()", 0, &context, &cc);
            }

        _delete_this_set:

            lc.credset = d_cs;
            lc.ccname = ccname;

            /* note that although we are deleting credentials off the
               credential set, the size of the credential set does not
               decrease since we are doing it from inside
               kcdb_credset_apply().  The deleted creds will simply be
               marked as deleted until kcdb_credset_purge() is
               called. */

            kcdb_credset_apply(d_cs,
                               khmint_delete_location_func,
                               (void *) &lc);
        }
    }

    kcdb_credset_purge(d_cs);

    /* the remainder need to be deleted one by one */

    kcdb_credset_get_size(d_cs, &s);

    for (i=0; i < (int) s; ) {
        khm_handle cred;
        char a_ccname[KRB5_MAXCCH_CCNAME];
        char a_srvname[KCDB_CRED_MAXCCH_NAME];
        wchar_t srvname[KCDB_CRED_MAXCCH_NAME];
        krb5_ccache cc;
        krb5_creds in_cred, out_cred;
        krb5_principal princ;
        khm_int32 etype;

        if (KHM_FAILED(kcdb_credset_get_cred(d_cs,
                                             i,
                                             &cred))) {
            i++;
            continue;
        }

        cb = sizeof(ccname);
        if (KHM_FAILED(kcdb_cred_get_attr(cred,
                                          KCDB_ATTR_LOCATION,
                                          NULL,
                                          ccname,
                                          &cb)))
            goto _done_with_this_cred;

        _reportf(L"Looking at ccache [%s]", ccname);

        UnicodeStrToAnsi(a_ccname,
                         sizeof(a_ccname),
                         ccname);

        code = krb5_cc_resolve(context,
			       a_ccname,
			       &cc);

        if (code)
            goto _skip_similar;

        code = krb5_cc_get_principal(context, cc, &princ);

        if (code) {
            krb5_cc_close(context, cc);
            goto _skip_similar;
        }

    _del_this_cred:

        cb = sizeof(etype);

        if (KHM_FAILED(kcdb_cred_get_attr(cred,
                                          attr_id_key_enctype,
                                          NULL,
                                          &etype,
                                          &cb)))
            goto _do_next_cred;

        cb = sizeof(srvname);
        if (KHM_FAILED(kcdb_cred_get_name(cred,
                                          srvname,
                                          &cb)))
            goto _do_next_cred;

        _reportf(L"Attempting to delete ticket %s", srvname);

        UnicodeStrToAnsi(a_srvname, sizeof(a_srvname), srvname);

        ZeroMemory(&in_cred, sizeof(in_cred));

        code = krb5_parse_name(context, a_srvname, &in_cred.server);
        if (code)
            goto _do_next_cred;
        in_cred.client = princ;
        in_cred.session.keytype = etype;

        code = krb5_cc_retrieve_cred(context,
				     cc,
				     KRB5_TC_MATCH_SRV_NAMEONLY |
				     KRB5_TC_MATCH_KEYTYPE,
				     &in_cred,
				     &out_cred);
        if (code) {
            khm_krb5_error(code, "krb5_cc_retrieve_cred()", 0, &context, &cc);
            goto _do_next_cred_0;
        }

        code = krb5_cc_remove_cred(context, cc,
				   KRB5_TC_MATCH_SRV_NAMEONLY |
				   KRB5_TC_MATCH_KEYTYPE |
				   KRB5_TC_MATCH_AUTHDATA,
				   &out_cred);

        if (code) {
            khm_krb5_error(code, "krb5_cc_remove_cred()", 0, &context, &cc);
        }

        krb5_free_cred_contents(context, &out_cred);
    _do_next_cred_0:
        krb5_free_principal(context, in_cred.server);
    _do_next_cred:

        /* check if the next cred is also of the same ccache */
        kcdb_cred_release(cred);

        for (i++; i < (int) s; i++) {
            if (KHM_FAILED(kcdb_credset_get_cred(d_cs,
                                                 i,
                                                 &cred)))
                continue;
        }

        if (i < (int) s) {
            wchar_t newcc[KRB5_MAXCCH_CCNAME];

            cb = sizeof(newcc);
            if (KHM_FAILED(kcdb_cred_get_attr(cred,
                                              KCDB_ATTR_LOCATION,
                                              NULL,
                                              newcc,
                                              &cb)) ||
                wcscmp(newcc, ccname)) {
                i--;            /* we have to look at this again */
                goto _done_with_this_set;
            }
            goto _del_this_cred;
        }


    _done_with_this_set:
        krb5_free_principal(context, princ);

        krb5_cc_close(context, cc);

    _done_with_this_cred:
        kcdb_cred_release(cred);
        i++;
        continue;

    _skip_similar:
        kcdb_cred_release(cred);

        for (++i; i < (int) s; i++) {
            wchar_t newcc[KRB5_MAXCCH_CCNAME];

            if (KHM_FAILED(kcdb_credset_get_cred(d_cs,
                                                 i,
                                                 &cred)))
                continue;

            cb = sizeof(newcc);
            if (KHM_FAILED(kcdb_cred_get_attr(cred,
                                              KCDB_ATTR_LOCATION,
                                              NULL,
                                              &newcc,
                                              &cb))) {
                kcdb_cred_release(cred);
                continue;
            }

            if (wcscmp(newcc, ccname)) {
                kcdb_cred_release(cred);
                break;
            }
        }
    }

_cleanup:

    if (d_cs)
        kcdb_credset_delete(&d_cs);

    if (context != NULL)
        krb5_free_context(context);

    return rv;
}

int
khm_krb5_destroy_identity(khm_handle identity)
{
    krb5_context		context;
    krb5_ccache			cache;
    krb5_error_code		rc;

    context = NULL;
    cache = NULL;

    if (rc = khm_krb5_initialize(identity, &context, &cache))
        return(rc);

    rc = krb5_cc_destroy(context, cache);

    if (context != NULL)
        krb5_free_context(context);

    return(rc);
}

static BOOL
GetSecurityLogonSessionData(PSECURITY_LOGON_SESSION_DATA * ppSessionData)
{
    NTSTATUS Status = 0;
    HANDLE  TokenHandle;
    TOKEN_STATISTICS Stats;
    DWORD   ReqLen;
    BOOL    Success;

    if (!ppSessionData)
        return FALSE;
    *ppSessionData = NULL;

    Success = OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &TokenHandle );
    if ( !Success )
        return FALSE;

    Success = GetTokenInformation( TokenHandle, TokenStatistics, &Stats, sizeof(TOKEN_STATISTICS), &ReqLen );
    CloseHandle( TokenHandle );
    if ( !Success )
        return FALSE;

    Status = LsaGetLogonSessionData( &Stats.AuthenticationId, ppSessionData );
    if ( FAILED(Status) || !*ppSessionData )
        return FALSE;

    return TRUE;
}

// IsKerberosLogon() does not validate whether or not there are valid
// tickets in the cache.  It validates whether or not it is reasonable
// to assume that if we attempted to retrieve valid tickets we could
// do so.  Microsoft does not automatically renew expired tickets.
// Therefore, the cache could contain expired or invalid tickets.
// Microsoft also caches the user's password and will use it to
// retrieve new TGTs if the cache is empty and tickets are requested.

static BOOL
IsKerberosLogon(VOID)
{
    PSECURITY_LOGON_SESSION_DATA pSessionData = NULL;
    BOOL    Success = FALSE;

    if ( GetSecurityLogonSessionData(&pSessionData) ) {
        if ( pSessionData->AuthenticationPackage.Buffer ) {
            WCHAR buffer[256];
            WCHAR *usBuffer;
            int usLength;

            Success = FALSE;
            usBuffer = (pSessionData->AuthenticationPackage).Buffer;
            usLength = (pSessionData->AuthenticationPackage).Length;
            if (usLength < 256)
            {
                lstrcpynW (buffer, usBuffer, usLength);
                StringCbCatW (buffer, sizeof(buffer), L"");
                if ( !lstrcmpW(L"Kerberos",buffer) )
                    Success = TRUE;
            }
        }
        LsaFreeReturnBuffer(pSessionData);
    }
    return Success;
}

BOOL
khm_krb5_ms2mit(char * match_princ, BOOL match_realm, BOOL save_creds,
                khm_handle * ret_ident)
{
    krb5_context context = 0;
    krb5_error_code code = 0;
    krb5_ccache ccache=0;
    krb5_ccache mslsa_ccache=0;
    krb5_creds creds;
    krb5_cc_cursor cursor=0;
    krb5_principal princ = 0;
    khm_handle ident = NULL;
    wchar_t idname[KCDB_IDENT_MAXCCH_NAME];
    char    ccname[KRB5_MAXCCH_CCNAME];
    char *cache_name = NULL;
    char *princ_name = NULL;
    BOOL rc = FALSE;

    kherr_reportf(L"Begin : khm_krb5_ms2mit. save_cred=%d\n", (int) save_creds);

    if (code = krb5_init_context(&context)) {
        khm_krb5_error(code, "krb5_init_context() for MSLSA:", 0, &context, NULL);
        goto cleanup;
    }

    if (code = krb5_cc_resolve(context, "MSLSA:", &mslsa_ccache)) {
        khm_krb5_error(code, "krb5_cc_resolve() for MSLSA:", 0, &context, NULL);
        goto cleanup;
    }

    if ( save_creds ) {
        if (code = krb5_cc_get_principal(context, mslsa_ccache, &princ)) {
            khm_krb5_error(code, "krb5_cc_get_principal() for MSLSA:", 0, &context, &mslsa_ccache);
            goto cleanup;
        }

        if (code = krb5_unparse_name(context, princ, &princ_name)) {
            khm_krb5_error(code, "krb5_unparse_name()", 0, &context, NULL);
            goto cleanup;
        }

        AnsiStrToUnicode(idname, sizeof(idname), princ_name);

        kherr_reportf(L"Unparsed name [%s]", idname);

        /* see if we have to match a specific principal */
        if (match_princ != NULL) {
            if (strcmp(princ_name, match_princ)) {
                kherr_reportf(L"Principal mismatch.  Wanted [%S], found [%S]",
                              match_princ, princ_name);
                goto cleanup;
            }
        } else if (match_realm) {
            wchar_t * wdefrealm;
            char defrealm[256];
            const char * princ_realm;

            wdefrealm = khm_krb5_get_default_realm();
            if (wdefrealm == NULL) {
                kherr_reportf(L"Can't determine default realm");
                goto cleanup;
            }

            princ_realm = krb5_principal_get_realm(context, princ);
            UnicodeStrToAnsi(defrealm, sizeof(defrealm), wdefrealm);

            if (strcmp(defrealm, princ_realm)) {
                kherr_reportf(L"Realm mismatch.  Wanted [%S], found [%S]",
                              defrealm, princ_realm);
                PFREE(wdefrealm);
                goto cleanup;
            }

            PFREE(wdefrealm);
        }

        if (KHM_SUCCEEDED(kcdb_identity_create(idname,
                                               KCDB_IDENT_FLAG_CREATE,
                                               &ident))) {
            khm_size cb;

            cb = sizeof(ccname);

            khm_krb5_get_identity_default_ccacheA(ident, ccname, &cb);

            cache_name = ccname;

        } else {
            /* the identity could not be created.  we just use the
               name of the principal as the ccache name. */
            assert(FALSE);
            kherr_reportf(L"Failed to create identity");
            StringCbPrintfA(ccname, sizeof(ccname), "API:%s", princ_name);
            cache_name = ccname;
        }

        kherr_reportf(L"Resolving target cache [%S]\n", cache_name);

        if (code = krb5_cc_resolve(context, cache_name, &ccache)) {
            kherr_reportf(L"Cannot resolve cache [%S] with code=%d.  Trying default.\n", cache_name, code);

            if (code = krb5_cc_default(context, &ccache)) {
                khm_krb5_error(code, "krb5_cc_default()", 0, &context, NULL);
                goto cleanup;
            }
        }

        if (code = krb5_cc_initialize(context, ccache, princ)) {
            khm_krb5_error(code, "krb5_cc_initialize()", 0, &context, &ccache);
            goto cleanup;
        }

        if (code = krb5_cc_copy_creds(context, mslsa_ccache, ccache)) {
            khm_krb5_error(code, "krb5_cc_copy_creds()", 0, &context, NULL);
            goto cleanup;
        }

        /* and mark the identity as having been imported */
        if (ident) {
            khm_krb5_set_identity_flags(ident, K5IDFLAG_IMPORTED, K5IDFLAG_IMPORTED);

            if (ret_ident) {
                *ret_ident = ident;
                kcdb_identity_hold(*ret_ident);
            }
        }

        rc = TRUE;

    } else {
        /* Enumerate tickets from cache looking for an initial ticket */
        if ((code = krb5_cc_start_seq_get(context, mslsa_ccache, &cursor))) {
            khm_krb5_error(code, "krb5_cc_start_seq_get()", 0, &context, &mslsa_ccache);
            goto cleanup;
        }

        while (!(code = krb5_cc_next_cred(context, mslsa_ccache,
					  &cursor, &creds))) {
            if ( creds.flags.b.initial ) {
                kherr_reportf(L"Found initial ticket");
                rc = TRUE;
                krb5_free_cred_contents(context, &creds);
                break;
            }
            krb5_free_cred_contents(context, &creds);
        }
        krb5_cc_end_seq_get(context, mslsa_ccache, &cursor);
    }

cleanup:
    if (princ_name)
        krb5_free_unparsed_name(context, princ_name);
    if (princ)
        krb5_free_principal(context, princ);
    if (ccache)
        krb5_cc_close(context, ccache);
    if (mslsa_ccache)
        krb5_cc_close(context, mslsa_ccache);
    if (context)
        krb5_free_context(context);
    if (ident)
        kcdb_identity_release(ident);

    return(rc);
}

#define KRB_FILE                "KRB.CON"
#define KRBREALM_FILE           "KRBREALM.CON"
#define KRB5_FILE               "KRB5.INI"
#define KRB5_TMP_FILE           "KRB5.INI.TMP"

BOOL
khm_krb5_get_temp_profile_file(LPSTR confname, UINT szConfname)
{
    GetTempPathA(szConfname, confname);
    confname[szConfname-1] = '\0';
    StringCchCatA(confname, szConfname, KRB5_TMP_FILE);
    confname[szConfname-1] = '\0';
    return FALSE;
}

#ifdef NOT_QUITE_IMPLEMENTED_YET
BOOL
khm_krb5_set_profile_file(krb5_context context, LPSTR confname)
{
    char *conffiles[2];

    if (confname == NULL ||
        krb5_set_config_files == NULL ||
        context == NULL)
        return FALSE;

    conffiles[0] = confname;
    conffiles[1] = NULL;

    if (krb5_set_config_files(context, conffiles))
        return FALSE;
    else
        return TRUE;
}
#endif

int
readstring(FILE * file, char * buf, int len)
{
    int  c,i;
    memset(buf, '\0', sizeof(buf));
    for (i=0, c=fgetc(file); c != EOF ; c=fgetc(file), i++) {
        if (i < sizeof(buf)) {
            if (c == '\n') {
                buf[i] = '\0';
                return i;
            } else {
                buf[i] = (char) c;
            }
        } else {
            if (c == '\n') {
                buf[len-1] = '\0';
                return(i);
            }
        }
    }
    if (c == EOF) {
        if (i > 0 && i < len) {
            buf[i] = '\0';
            return(i);
        } else {
            buf[len-1] = '\0';
            return(-1);
        }
    }
    return(-1);
}

/*! \internal
  \brief Return a list of configured realms

  The string that is returned is a set of null terminated unicode
  strings, each of which denotes one realm.  The set is terminated
  by a zero length null terminated string.

  The caller should free the returned string using free()

  \return The string with the list of realms or NULL if the
  operation fails.
*/
wchar_t *
khm_krb5_get_realm_list(void)
{
    krb5_context context = 0;
    const krb5_config_binding * list;
    const krb5_config_binding * b;
    size_t cbsize = 0, cbleft;
    wchar_t * rs;
    wchar_t * rsp;

    if (krb5_init_context(&context))
	return NULL;

    list = krb5_config_get_list(context, NULL, "realms", NULL);
    if (list == NULL)
	return NULL;

    for (b = list; b ; b = b->next )
	if (b->type == krb5_config_list) {
	    int nc = MultiByteToWideChar(CP_ACP, 0, b->name, -1, NULL, 0);
	    cbsize += nc * sizeof(wchar_t);
	}
    cbsize += sizeof(wchar_t);

    rsp = rs = PMALLOC(cbsize);
    cbleft = cbsize;

    for (b = list; b ; b = b->next )
	if (b->type == krb5_config_list) {
	    int nc = MultiByteToWideChar(CP_ACP, 0, b->name, -1,
					 rsp, cbleft / sizeof(wchar_t));
	    cbleft -= nc * sizeof(wchar_t);
	    rsp += nc;
	}
    *rsp = L'\0';
    cbleft -= sizeof(wchar_t);

    assert(cbleft == 0);

    krb5_free_context(context);

    return rs;
}

/*! \internal
  \brief Get the default realm

  A string will be returned that specifies the default realm.  The
  caller should free the string using PFREE().

  Returns NULL if the operation fails.
*/
wchar_t *
khm_krb5_get_default_realm(void)
{
    wchar_t * realm;
    size_t cch;
    krb5_context context=0;
    krb5_realm def = 0;

    krb5_init_context(&context);

    if (context == 0)
        return NULL;

    krb5_get_default_realm(context, &def);

    if (def) {
        cch = strlen(def) + 1;
        realm = PMALLOC(sizeof(wchar_t) * cch);
        AnsiStrToUnicode(realm, sizeof(wchar_t) * cch, def);
        krb5_free_default_realm(context, def);
    } else
        realm = NULL;

    krb5_free_context(context);

    return realm;
}

long
khm_krb5_set_default_realm(wchar_t * realm) {
    krb5_context context=0;
    char * def = 0;
    long rv = 0;
    char astr[K5_MAXCCH_REALM];

    UnicodeStrToAnsi(astr, sizeof(astr), realm);

    krb5_init_context(&context);
    krb5_get_default_realm(context, &def);

    if ((def && strcmp(def, astr)) ||
        !def) {
        rv = krb5_set_default_realm(context, astr);
    }

    if (def) {
        krb5_free_default_realm(context, def);
    }

    krb5_free_context(context);

    return rv;
}

wchar_t *
khm_get_realm_from_princ(wchar_t * princ) {
    wchar_t * t;

    if(!princ)
        return NULL;

    for (t = princ; *t; t++) {
        if(*t == L'\\') {       /* escape */
            t++;
            if(! *t)            /* malformed */
                break;
        } else if (*t == L'@')
            break;
    }

    if (*t == '@' && *(t+1) != L'\0')
        return (t+1);
    else
        return NULL;
}

long
khm_krb5_changepwd(char * principal,
                   char * password,
                   char * newpassword,
                   char** error_str)
{
    krb5_error_code rc = 0;
    int result_code = 0;
    krb5_data result_code_string, result_string;
    krb5_context context = 0;
    krb5_principal princ = 0;
    krb5_get_init_creds_opt *opts = NULL;
    krb5_creds creds;

    result_string.data = 0;
    result_code_string.data = 0;

    if (rc = krb5_init_context(&context)) {
        goto cleanup;
    }

    if (rc = krb5_parse_name(context, principal, &princ)) {
        goto cleanup;
    }

    krb5_get_init_creds_opt_alloc(context, &opts);
    krb5_get_init_creds_opt_set_tkt_life(opts, 5*60);
    krb5_get_init_creds_opt_set_renew_life(opts, 0);
    krb5_get_init_creds_opt_set_forwardable(opts, 0);
    krb5_get_init_creds_opt_set_proxiable(opts, 0);
    krb5_get_init_creds_opt_set_address_list(opts,NULL);

    if (rc = krb5_get_init_creds_password(context, &creds, princ,
					  password, 0, 0, 0,
					  "kadmin/changepw", opts)) {
      if (rc == KRB5KRB_AP_ERR_BAD_INTEGRITY) {
	*error_str = PSTRDUP("Password incorrect while getting initial ticket");
      } else {
	*error_str = PSTRDUP(krb5_get_error_message(context, rc));
      }
      goto cleanup;
    }

    if (rc = krb5_set_password(context, &creds, newpassword,
			       princ,
			       &result_code, &result_code_string,
			       &result_string)) {
      *error_str = PSTRDUP(krb5_get_error_message(context, rc));
      goto cleanup;
    }

    if (result_code) {
        int len = result_code_string.length +
            (result_string.length ? (sizeof(": ") - 1) : 0) +
            result_string.length;
        if (len && error_str) {
            *error_str = PMALLOC(len + 1);
            if (*error_str)
                StringCchPrintfA(*error_str, len+1,
                                 "%.*s%s%.*s",
                                 result_code_string.length,
                                 result_code_string.data,
                                 result_string.length?": ":"",
                                 result_string.length,
                                 result_string.data);
        }
        rc = result_code;
        goto cleanup;
    }

cleanup:
    if (result_string.data)
        krb5_data_free(&result_string);

    if (result_code_string.data)
        krb5_data_free(&result_code_string);

    if (princ)
        krb5_free_principal(context, princ);

    if (opts)
	krb5_get_init_creds_opt_free(context, opts);

    if (context)
        krb5_free_context(context);

    return rc;
}

khm_int32 KHMAPI
khm_krb5_creds_is_equal(khm_handle vcred1, khm_handle vcred2, void * dummy) {
    if (kcdb_creds_comp_attr(vcred1, vcred2, KCDB_ATTR_LOCATION) ||
        kcdb_creds_comp_attr(vcred1, vcred2, attr_id_key_enctype) ||
        kcdb_creds_comp_attr(vcred1, vcred2, attr_id_tkt_enctype) ||
        kcdb_creds_comp_attr(vcred1, vcred2, attr_id_kvno))
        return 1;
    else
        return 0;
}

void
khm_krb5_set_identity_flags(khm_handle identity,
                            khm_int32  flag_mask,
                            khm_int32  flag_value) {

    khm_int32 t = 0;
    khm_size  cb;

    cb = sizeof(t);
    if (KHM_FAILED(kcdb_identity_get_attr(identity,
                                          attr_id_krb5_idflags,
                                          NULL,
                                          &t, &cb))) {
        t = 0;
    }

    t &= ~flag_mask;
    t |= (flag_value & flag_mask);

    kcdb_identity_set_attr(identity,
                           attr_id_krb5_idflags,
                           &t, sizeof(t));
}

khm_int32
khm_krb5_get_identity_flags(khm_handle identity) {
    khm_int32 t = 0;
    khm_size  cb;

    cb = sizeof(t);
    kcdb_identity_get_attr(identity,
                           attr_id_krb5_idflags,
                           NULL, &t, &cb);

    return t;
}

long
khm_krb5_get_temp_ccache(krb5_context context,
                         krb5_ccache * prcc) {
    int  rnd = rand();
    char ccname[MAX_PATH];
    long code = 0;
    krb5_ccache cc = 0;

    StringCbPrintfA(ccname, sizeof(ccname), "MEMORY:TempCache%8x", rnd);

    code = krb5_cc_resolve(context, ccname, &cc);

    if (code == 0)
        *prcc = cc;

    return code;
}

/*

  The configuration information for each identity comes from a
  multitude of layers organized as follows.  The ordering is
  decreasing in priority.  When looking up a value, the value will be
  looked up in each layer in turn starting at level 0.  The first
  instance of the value found will be the effective value.

  0  : <identity configuration>\Krb5Cred

  0.1: per user

  0.2: per machine

  1  : <plugin configuration>\Parameters\Realms\<realm of identity>

  1.1: per user

  1.2: per machine

  2  : <plugin configuration>\Parameters

  2.1: per user

  2.2: per machine

  2.3: schema

*/
khm_int32
khm_krb5_get_identity_config(khm_handle ident,
			     khm_int32 flags,
			     khm_handle * ret_csp) {

    khm_int32 rv = KHM_ERROR_SUCCESS;
    khm_handle csp_i = NULL;
    khm_handle csp_ik5 = NULL;
    khm_handle csp_realms = NULL;
    khm_handle csp_realm = NULL;
    khm_handle csp_plugins = NULL;
    khm_handle csp_krbcfg = NULL;
    khm_handle csp_rv = NULL;
    wchar_t realm[KCDB_IDENT_MAXCCH_NAME];

    realm[0] = L'\0';

    if (ident) {
        wchar_t idname[KCDB_IDENT_MAXCCH_NAME];
        wchar_t * trealm;
        khm_size cb_idname = sizeof(idname);

        rv = kcdb_identity_get_name(ident, idname, &cb_idname);
        if (KHM_SUCCEEDED(rv) &&
            (trealm = khm_get_realm_from_princ(idname)) != NULL) {
            StringCbCopy(realm, sizeof(realm), trealm);
        }
    }

    if (ident) {
        rv = kcdb_identity_get_config(ident, flags, &csp_i);
        if (KHM_FAILED(rv))
            goto try_realm;

        rv = khc_open_space(csp_i, CSNAME_KRB5CRED, flags, &csp_ik5);
        if (KHM_FAILED(rv))
            goto try_realm;

    try_realm:

        if (realm[0] == L'\0')
            goto done_shadow_realm;

        rv = khc_open_space(csp_params, CSNAME_REALMS, flags, &csp_realms);
        if (KHM_FAILED(rv))
            goto done_shadow_realm;

        rv = khc_open_space(csp_realms, realm, flags, &csp_realm);
        if (KHM_FAILED(rv))
            goto done_shadow_realm;

        rv = khc_shadow_space(csp_realm, csp_params);

    done_shadow_realm:

        if (csp_ik5) {
            if (csp_realm)
                rv = khc_shadow_space(csp_ik5, csp_realm);
            else
                rv = khc_shadow_space(csp_ik5, csp_params);

            csp_rv = csp_ik5;
        } else {
            if (csp_realm)
                csp_rv = csp_realm;
        }
    }

    if (csp_rv == NULL) {

        /* No valid identity specified or the specified identity
           doesn't have any configuration. We default to the
           parameters key. */

        /* we don't just return csp_params since that's a global
           handle that we shouldn't close until the plugin is
           unloaded.  The caller is going to close the returned handle
           when it is done.  So we need to create a new csp_params
           that can safely be closed. */

        rv = kmm_get_plugins_config(0, &csp_plugins);
        if (KHM_FAILED(rv))
            goto done;

        rv = khc_open_space(csp_plugins, CSNAME_KRB5CRED, flags, &csp_krbcfg);
        if (KHM_FAILED(rv))
            goto done;

        rv = khc_open_space(csp_krbcfg, CSNAME_PARAMS, flags, &csp_rv);
    }

done:

    *ret_csp = csp_rv;

    /* leave csp_ik5.  If it's non-NULL, then it's the return value */
    /* leave csp_rv.  It's the return value. */
    if (csp_i)
        khc_close_space(csp_i);
    if (csp_realms)
        khc_close_space(csp_realms);

    /* csp_realm can also be a return value if csp_ik5 was NULL */
    if (csp_realm && csp_realm != csp_rv)
        khc_close_space(csp_realm);

    if (csp_plugins)
        khc_close_space(csp_plugins);
    if (csp_krbcfg)
        khc_close_space(csp_krbcfg);

    return rv;
}

/* from get_in_tkt.c */
static krb5_error_code
get_libdefault_string(krb5_context context, const char * realm,
                      const char * option, char ** ret_val) {

    const char * val;

    val = krb5_config_get_string(context, NULL, "libdefaults", realm, option, NULL);
    if (val == NULL)
	val = krb5_config_get_string(context, NULL, "libdefaults", option, NULL);

    if (val) {
	*ret_val = PSTRDUP(val);
	return 0;
    } else {
	return ENOENT;
    }
}


const struct escape_char_sequences {
    wchar_t character;
    wchar_t escape;
} file_cc_escapes[] = {

    /* in ASCII order */

    {L'\"', L'd'},
    {L'$',  L'$'},
    {L'%',  L'r'},
    {L'\'', L'i'},
    {L'*',  L's'},
    {L'/',  L'f'},
    {L':',  L'c'},
    {L'<',  L'l'},
    {L'>',  L'g'},
    {L'?',  L'q'},
    {L'\\', L'b'},
    {L'|',  L'p'}
};

static void
escape_string_for_filename(const wchar_t * s,
                           wchar_t * buf,
                           khm_size cb_buf)
{
    wchar_t * d;
    int i;

    for (d = buf; *s && cb_buf > sizeof(wchar_t) * 3; s++) {
        if (iswpunct(*s)) {
            for (i=0; i < ARRAYLENGTH(file_cc_escapes); i++) {
                if (*s == file_cc_escapes[i].character)
                    break;
            }

            if (i < ARRAYLENGTH(file_cc_escapes)) {
                *d++ = L'$';
                *d++ = file_cc_escapes[i].escape;
                cb_buf -= sizeof(wchar_t) * 2;
                continue;
            }
        }

        *d++ = *s;
        cb_buf -= sizeof(wchar_t);
    }

#ifdef DEBUG
    assert(cb_buf >= sizeof(wchar_t));
#endif
    *d++ = L'\0';
}

static khm_int32
get_default_file_cache_for_identity(const wchar_t * idname,
                                    wchar_t * ccname,
                                    khm_size * pcb)
{
    wchar_t escf[KRB5_MAXCCH_CCNAME] = L"";
    wchar_t tmppath[KRB5_MAXCCH_CCNAME] = L"";
    wchar_t tccname[KRB5_MAXCCH_CCNAME];
    khm_size cb;

    escape_string_for_filename(idname, escf, sizeof(escf));
    GetTempPath(ARRAYLENGTH(tmppath), tmppath);

    /* The path returned by GetTempPath always ends in a backslash. */
    StringCbPrintf(tccname, sizeof(tccname), L"FILE:%skrb5cc.%s", tmppath, escf);
    StringCbLength(tccname, sizeof(tccname), &cb);
    cb += sizeof(wchar_t);

    if (ccname && *pcb >= cb) {
        StringCbCopy(ccname, *pcb, tccname);
        *pcb = cb;
        return KHM_ERROR_SUCCESS;
    } else {
        *pcb = cb;
        return KHM_ERROR_TOO_LONG;
    }
}

khm_int32
khm_krb5_get_identity_default_ccache_failover(khm_handle ident, wchar_t * buf, khm_size * pcb)
{
    khm_int32 rv = KHM_ERROR_SUCCESS;

    wchar_t ccname[KRB5_MAXCCH_CCNAME] = L"";
    wchar_t idname[KCDB_IDENT_MAXCCH_NAME];
    khm_size cb;
    khm_int32 use_file_cache = 1;
    khm_handle csp_id = NULL;

    rv = khm_krb5_get_identity_config(ident, 0, &csp_id);
    if (KHM_FAILED(rv))
        return rv;

    khc_read_int32(csp_id, L"DefaultToFileCache", &use_file_cache);

    cb = sizeof(idname);
    kcdb_identity_get_name(ident, idname, &cb);

    if (use_file_cache) {
        cb = sizeof(ccname);
        rv = get_default_file_cache_for_identity(idname, ccname, &cb);
        assert(KHM_SUCCEEDED(rv));
    } else {                /* generate an API: cache */
        StringCbPrintf(ccname, sizeof(ccname), L"API:%s", idname);
    }
    khm_krb5_canon_cc_name(ccname, sizeof(ccname));

    _reportf(L"Setting CCache [%s] for identity [%s]", ccname, idname);

    StringCbLength(ccname, sizeof(ccname), &cb);
    cb += sizeof(wchar_t);

    if (buf && *pcb >= cb) {
        StringCbCopy(buf, *pcb, ccname);
        *pcb = cb;
        rv = KHM_ERROR_SUCCESS;
    } else {
        *pcb = cb;
        rv = KHM_ERROR_TOO_LONG;
    }

    if (csp_id)
        khc_close_space(csp_id);

    return rv;
}

khm_int32
khm_krb5_get_identity_default_ccache(khm_handle ident, wchar_t * buf, khm_size * pcb)
{
    wchar_t ccname[KRB5_MAXCCH_CCNAME] = L"";
    khm_handle csp_id = NULL;
    khm_int32 rv = KHM_ERROR_SUCCESS;
    khm_size cbt;

    rv = khm_krb5_get_identity_config(ident, 0, &csp_id);

    cbt = sizeof(ccname);
    if (KHM_SUCCEEDED(rv))
        rv = khc_read_string(csp_id, L"DefaultCCName", ccname, &cbt);

    if (KHM_FAILED(rv) ||
        (KHM_SUCCEEDED(rv) && ccname[0] == L'\0')) {
        rv = khm_krb5_get_identity_default_ccache_failover(ident, buf, pcb);
    } else {
        if (wcschr(ccname, L'%')) {
            wchar_t expccname[MAX_PATH + 5];
            DWORD cch;

            cch = ExpandEnvironmentStrings(ccname, expccname, ARRAYLENGTH(expccname));
            if (cch == 0 || cch > ARRAYLENGTH(expccname)) {
#ifdef DEBUG
                assert(FALSE);
#endif
                rv = KHM_ERROR_UNKNOWN;
            } else {
                StringCbCopy(ccname, sizeof(ccname), expccname);
                StringCbLength(ccname, sizeof(ccname), &cbt);
                cbt += sizeof(wchar_t);
            }
        }

        if (KHM_SUCCEEDED(rv)) {
            wchar_t idname[KCDB_IDENT_MAXCCH_NAME];
            khm_size cb;

            if (buf && *pcb >= cbt) {
                StringCbCopy(buf, *pcb, ccname);
                *pcb = cbt;
            } else {
                *pcb = cbt;
                rv = KHM_ERROR_TOO_LONG;
            }

            cb = sizeof(idname);
            kcdb_identity_get_name(ident, idname, &cb);

            _reportf(L"Found CCache [%s] for identity [%s]", buf, idname);
        }
    }

    if (csp_id != NULL)
        khc_close_space(csp_id);

    return rv;
}

khm_int32
khm_krb5_get_identity_default_ccacheA(khm_handle ident, char * buf, khm_size * pcb) {
    wchar_t wccname[KRB5_MAXCCH_CCNAME];
    khm_size cbcc;
    khm_int32 rv;

    cbcc = sizeof(wccname);
    rv = khm_krb5_get_identity_default_ccache(ident, wccname, &cbcc);

    if (KHM_SUCCEEDED(rv)) {
        cbcc = sizeof(char) * cbcc / sizeof(wchar_t);
        if (buf == NULL || *pcb < cbcc) {
            *pcb = cbcc;
            rv = KHM_ERROR_TOO_LONG;
        } else {
            UnicodeStrToAnsi(buf, *pcb, wccname);
            *pcb = cbcc;
            rv = KHM_ERROR_SUCCESS;
        }
    }

    return rv;
}

khm_int32
khm_krb5_get_identity_params(khm_handle ident, k5_params * p) {

    khm_int32 rv = KHM_ERROR_SUCCESS;
    khm_handle csp_id = NULL;
    khm_int32 regf = 0;
    khm_int32 proff = 0;
    khm_int32 e;
    khm_int32 v;
    CHAR realmname[K5_MAXCCH_REALM];
    krb5_context context = 0;
    krb5_error_code retval = 0;

    ZeroMemory(p, sizeof(*p));

    rv = khm_krb5_get_identity_config(ident, 0, &csp_id);
    if (KHM_FAILED(rv))
        goto done_reg;


#define GETVAL(vname, vfield, flag)				\
    do {							\
        rv = khc_read_int32(csp_id, vname, &v);			\
        if (KHM_SUCCEEDED(rv)) {				\
            p->vfield = v;					\
            e = khc_value_exists(csp_id, vname);		\
            if ((e & ~KCONF_FLAG_SCHEMA) != 0) regf |= flag;	\
        }							\
    } while(FALSE)

    /* Flags */
    GETVAL(L"Renewable", renewable, K5PARAM_F_RENEW);
    GETVAL(L"Forwardable", forwardable, K5PARAM_F_FORW);
    GETVAL(L"Proxiable", proxiable, K5PARAM_F_PROX);
    GETVAL(L"Addressless", addressless, K5PARAM_F_ADDL);
    GETVAL(L"PublicIP", publicIP, K5PARAM_F_PUBIP);

    /* Lifetime */
    GETVAL(L"DefaultLifetime", lifetime, K5PARAM_F_LIFE);
    GETVAL(L"MaxLifetime", lifetime_max, K5PARAM_F_LIFE_H);
    GETVAL(L"MinLifetime", lifetime_min, K5PARAM_F_LIFE_L);

    /* Renewable lifetime */
    GETVAL(L"DefaultRenewLifetime", renew_life, K5PARAM_F_RLIFE);
    GETVAL(L"MaxRenewLifetime", renew_life_max, K5PARAM_F_RLIFE_H);
    GETVAL(L"MinRenewLifetime", renew_life_min, K5PARAM_F_RLIFE_L);

#undef GETVAL

done_reg:

    if (csp_id)
        khc_close_space(csp_id);

    /* if all the parameters were read from the registry, then we have
       no reason to read from the profile file. */
    if (regf == K5PARAM_FM_ALL) {
        p->source_reg = regf;
        return KHM_ERROR_SUCCESS;
    }

    if (rv)
        return rv;

    /* we need to figure out the realm name, since there might be
       per-realm configuration in the profile file. */

    realmname[0] = '\0';

    if (ident) {
        wchar_t idname[KCDB_IDENT_MAXCCH_NAME];
        khm_size cb;

        idname[0] = L'\0';
        cb = sizeof(idname);
        rv = kcdb_identity_get_name(ident, idname, &cb);
        if (KHM_SUCCEEDED(rv)) {
            wchar_t * wrealm;

            wrealm = khm_get_realm_from_princ(idname);
            if (wrealm) {
                UnicodeStrToAnsi(realmname, sizeof(realmname), wrealm);
            }
        }
    }

    /* If we get here, then some of the settings we read from the
       configuration actually came from the schema.  In other words,
       the values weren't really defined for this identity.  So we now
       have to read the values from the krb5 configuration file. */

    if (krb5_init_context(&context))
	goto cleanup;

    /* default ticket lifetime */
    if (!(regf & K5PARAM_F_LIFE)) {
	char * value = NULL;
	retval = get_libdefault_string(context, realmname,
				       "ticket_lifetime", &value);

	if (retval == 0 && value) {
	    krb5_deltat d;

	    retval = krb5_string_to_deltat(value, &d);
	    if (retval == KRB5_DELTAT_BADFORMAT) {
		/* Historically some sites use relations of the form
		   'ticket_lifetime = 24000' where the unit is left
		   out but is assumed to be seconds. Then there are
		   other sites which use the form 'ticket_lifetime =
		   600' where the unit is assumed to be minutes.
		   While these are technically wrong (a unit needs to
		   be specified), we try to accomodate for this using
		   the safe assumption that the unit is seconds and
		   tack an 's' to the end and see if that works. */

		size_t cch;
		char tmpbuf[256];
		char * buf;

		do {
		    if (FAILED(StringCchLengthA(value, 1024 /* unresonably large size */,
						&cch)))
			break;

		    cch += sizeof(char) * 2; /* NULL and new 's' */
		    if (cch > ARRAYLENGTH(tmpbuf))
			buf = PMALLOC(cch * sizeof(char));
		    else
			buf = tmpbuf;

		    StringCchCopyA(buf, cch, value);
		    StringCchCatA(buf, cch, "s");

		    retval = krb5_string_to_deltat(buf, &d);
		    if (retval == 0) {
			p->lifetime = d;
			proff |= K5PARAM_F_LIFE;
		    }

		    if (buf != tmpbuf)
			PFREE(buf);

		} while(0);

	    } else if (retval == 0) {
		p->lifetime = d;
		proff |= K5PARAM_F_LIFE;
	    }

	    PFREE(value);
	}
    }

    if (!(regf & K5PARAM_F_RLIFE)) {
	char * value = NULL;
	retval = get_libdefault_string(context, realmname,
				       "renew_lifetime", &value);
	if (retval == 0 && value) {
	    krb5_deltat d;

	    retval = krb5_string_to_deltat(value, &d);
	    if (retval == 0) {
		p->renew_life = d;
		proff |= K5PARAM_F_RLIFE;
	    }
	    PFREE(value);
	}
    }

    if (!(regf & K5PARAM_F_FORW)) {
	char * value = NULL;
	retval = get_libdefault_string(context, realmname,
				       "forwardable", &value);
	if (retval == 0 && value) {
	    khm_boolean b;

	    if (!khm_krb5_parse_boolean(value, &b))
		p->forwardable = b;
	    else
		p->forwardable = FALSE;
	    PFREE(value);
	    proff |= K5PARAM_F_FORW;
	}
    }

    if (!(regf & K5PARAM_F_RENEW)) {
	char * value = NULL;
	retval = get_libdefault_string(context, realmname,
				       "renewable", &value);
	if (retval == 0 && value) {
	    khm_boolean b;

	    if (!khm_krb5_parse_boolean(value, &b))
		p->renewable = b;
	    else
		p->renewable = TRUE;
	    PFREE(value);
	    proff |= K5PARAM_F_RENEW;
	}
    }

    if (!(regf & K5PARAM_F_ADDL)) {
	char * value = NULL;
	retval = get_libdefault_string(context, realmname,
				       "noaddresses", &value);
	if (retval == 0 && value) {
	    khm_boolean b;

	    if (!khm_krb5_parse_boolean(value, &b))
		p->addressless = b;
	    else
		p->addressless = TRUE;
	    PFREE(value);
	    proff |= K5PARAM_F_ADDL;
	}
    }

    if (!(regf & K5PARAM_F_PROX)) {
	char * value = NULL;
	retval = get_libdefault_string(context, realmname,
				       "proxiable", &value);
	if (retval == 0 && value) {
	    khm_boolean b;

	    if (!khm_krb5_parse_boolean(value, &b))
		p->proxiable = b;
	    else
		p->proxiable = FALSE;
	    PFREE(value);
	    proff |= K5PARAM_F_PROX;
	}
    }

    p->source_reg = regf;
    p->source_prof = proff;

cleanup:

    if (context)
	krb5_free_context(context);

    return rv;
}

/* Note that p->source_reg and p->source_prof is used in special ways
   here.  All fields that are flagged in source_reg will be written to
   the configuration (if they are different from what
   khm_krb5_get_identity_params() reports).  All fields that are
   flagged in source_prof will be removed from the configuration
   (thereby exposing the value defined in the profile file). */
khm_int32
khm_krb5_set_identity_params(khm_handle ident, const k5_params * p) {
    khm_int32 rv = KHM_ERROR_SUCCESS;
    khm_handle csp_id = NULL;
    k5_params p_s;
    khm_int32 source_reg = p->source_reg;
    khm_int32 source_prof = p->source_prof;

    rv = khm_krb5_get_identity_config(ident,
                                      KHM_PERM_WRITE | KHM_FLAG_CREATE |
                                      KCONF_FLAG_WRITEIFMOD,
                                      &csp_id);
    if (KHM_FAILED(rv))
        goto done_reg;

    khm_krb5_get_identity_params(ident, &p_s);

    /* Remove any bits that don't make sense.  Not all values can be
       specified in the profile file. */
    source_prof &= K5PARAM_FM_PROF;

    /* if a flag appears in both source_prof and source_reg, remove
       the flag from source_reg. */
    source_reg &= ~source_prof;

    /* we only write values that have changed, and that are flagged in
       source_reg */

    if ((source_reg & K5PARAM_F_RENEW) &&
        !!p_s.renewable != !!p->renewable)
        khc_write_int32(csp_id, L"Renewable", !!p->renewable);

    if ((source_reg & K5PARAM_F_FORW) &&
        !!p_s.forwardable != !!p->forwardable)
        khc_write_int32(csp_id, L"Forwardable", !!p->forwardable);

    if ((source_reg & K5PARAM_F_PROX) &&
        !!p_s.proxiable != !!p->proxiable)
        khc_write_int32(csp_id, L"Proxiable", !!p->proxiable);

    if ((source_reg & K5PARAM_F_ADDL) &&
        !!p_s.addressless != !!p->addressless)
        khc_write_int32(csp_id, L"Addressless", !!p->addressless);

    if ((source_reg & K5PARAM_F_PUBIP) &&
        p_s.publicIP != p->publicIP)
        khc_write_int32(csp_id, L"PublicIP", p->publicIP);

    if ((source_reg & K5PARAM_F_LIFE) &&
        p_s.lifetime != p->lifetime)
        khc_write_int32(csp_id, L"DefaultLifetime", p->lifetime);

    if ((source_reg & K5PARAM_F_LIFE_H) &&
        p_s.lifetime_max != p->lifetime_max)
        khc_write_int32(csp_id, L"MaxLifetime", p->lifetime_max);

    if ((source_reg & K5PARAM_F_LIFE_L) &&
        p_s.lifetime_min != p->lifetime_min)
        khc_write_int32(csp_id, L"MinLifetime", p->lifetime_min);

    if ((source_reg & K5PARAM_F_RLIFE) &&
        p_s.renew_life != p->renew_life)
        khc_write_int32(csp_id, L"DefaultRenewLifetime", p->renew_life);

    if ((source_reg & K5PARAM_F_RLIFE_H) &&
        p_s.renew_life_max != p->renew_life_max)
        khc_write_int32(csp_id, L"MaxRenewLifetime", p->renew_life_max);

    if ((source_reg & K5PARAM_F_RLIFE_L) &&
        p_s.renew_life_min != p->renew_life_min)
        khc_write_int32(csp_id, L"MinRenewLifetime", p->renew_life_min);

    /* and now, remove the values that are present in source_prof.
       Not all values are removed since not all values can be
       specified in the profile file. */
    if (source_prof & K5PARAM_F_RENEW)
        khc_remove_value(csp_id, L"Renewable", 0);

    if (source_prof & K5PARAM_F_FORW)
        khc_remove_value(csp_id, L"Forwardable", 0);

    if (source_prof & K5PARAM_F_PROX)
        khc_remove_value(csp_id, L"Proxiable", 0);

    if (source_prof & K5PARAM_F_ADDL)
        khc_remove_value(csp_id, L"Addressless", 0);

    if (source_prof & K5PARAM_F_LIFE)
        khc_remove_value(csp_id, L"DefaultLifetime", 0);

    if (source_prof & K5PARAM_F_RLIFE)
        khc_remove_value(csp_id, L"DefaultRenewLifetime", 0);

done_reg:
    if (csp_id != NULL)
        khc_close_space(csp_id);

    return rv;
}

static const char *const conf_yes[] = {
    "y", "yes", "true", "t", "1", "on",
    0,
};

static const char *const conf_no[] = {
    "n", "no", "false", "nil", "0", "off",
    0,
};

int
khm_krb5_parse_boolean(const char *s, khm_boolean * b)
{
    const char *const *p;

    for(p=conf_yes; *p; p++) {
        if (!_stricmp(*p,s)) {
            *b = TRUE;
            return 0;
        }
    }

    for(p=conf_no; *p; p++) {
        if (!_stricmp(*p,s)) {
            *b = FALSE;
            return 0;
        }
    }

    /* Default to "no" */
    return KHM_ERROR_INVALID_PARAM;
}

static const wchar_t * _exp_env_vars[] = {
    L"TMP",
    L"TEMP",
    L"USERPROFILE",
    L"ALLUSERSPROFILE",
    L"SystemRoot",
#if 0
    L"SystemDrive"
#endif
};

const int _n_exp_env_vars = ARRAYLENGTH(_exp_env_vars);

static int
check_and_replace_env_prefix(wchar_t * s, size_t cb_s, const wchar_t * env) {
    wchar_t evalue[MAX_PATH];
    DWORD len;
    size_t sz;

    evalue[0] = L'\0';
    len = GetEnvironmentVariable(env, evalue, ARRAYLENGTH(evalue));
    if (len > ARRAYLENGTH(evalue) || len == 0)
        return 0;

    if (_wcsnicmp(s, evalue, len))
        return 0;

    if (SUCCEEDED(StringCbPrintf(evalue, sizeof(evalue), L"%%%s%%%s",
                                 env, s + len)) &&
        SUCCEEDED(StringCbLength(evalue, sizeof(evalue), &sz)) &&
        sz <= cb_s) {
        StringCbCopy(s, cb_s, evalue);
    }

    return 1;
}

/* We use this instead of PathUnexpandEnvStrings() because that API
   doesn't take process TMP and TEMP.  The string is modified
   in-place.  The usual case is to call it with a buffer big enough to
   hold MAX_PATH characters. */
void
unexpand_env_var_prefix(wchar_t * s, size_t cb_s) {
    int i;

    for (i=0; i < _n_exp_env_vars; i++) {
        if (check_and_replace_env_prefix(s, cb_s, _exp_env_vars[i]))
            return;
    }
}
