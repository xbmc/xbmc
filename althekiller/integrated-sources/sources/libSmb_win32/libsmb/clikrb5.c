/* 
   Unix SMB/CIFS implementation.
   simple kerberos5 routines for active directory
   Copyright (C) Andrew Tridgell 2001
   Copyright (C) Luke Howard 2002-2003
   Copyright (C) Andrew Bartlett <abartlet@samba.org> 2005
   Copyright (C) Guenther Deschner 2005
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#define KRB5_PRIVATE    1       /* this file uses PRIVATE interfaces! */
#define KRB5_DEPRECATED 1       /* this file uses DEPRECATED interfaces! */

#include "includes.h"

#ifdef HAVE_KRB5

#ifdef HAVE_KRB5_KEYBLOCK_KEYVALUE
#define KRB5_KEY_TYPE(k)	((k)->keytype)
#define KRB5_KEY_LENGTH(k)	((k)->keyvalue.length)
#define KRB5_KEY_DATA(k)	((k)->keyvalue.data)
#else
#define	KRB5_KEY_TYPE(k)	((k)->enctype)
#define KRB5_KEY_LENGTH(k)	((k)->length)
#define KRB5_KEY_DATA(k)	((k)->contents)
#endif /* HAVE_KRB5_KEYBLOCK_KEYVALUE */

/**************************************************************
 Wrappers around kerberos string functions that convert from
 utf8 -> unix charset and vica versa.
**************************************************************/

/**************************************************************
 krb5_parse_name that takes a UNIX charset.
**************************************************************/

 krb5_error_code smb_krb5_parse_name(krb5_context context,
				const char *name, /* in unix charset */
				krb5_principal *principal)
{
	krb5_error_code ret;
	char *utf8_name;

	if (push_utf8_allocate(&utf8_name, name) == (size_t)-1) {
		return ENOMEM;
	}

	ret = krb5_parse_name(context, utf8_name, principal);
	SAFE_FREE(utf8_name);
	return ret;
}

#ifdef HAVE_KRB5_PARSE_NAME_NOREALM
/**************************************************************
 krb5_parse_name_norealm that takes a UNIX charset.
**************************************************************/

static krb5_error_code smb_krb5_parse_name_norealm_conv(krb5_context context,
				const char *name, /* in unix charset */
				krb5_principal *principal)
{
	krb5_error_code ret;
	char *utf8_name;

	if (push_utf8_allocate(&utf8_name, name) == (size_t)-1) {
		return ENOMEM;
	}

	ret = krb5_parse_name_norealm(context, utf8_name, principal);
	SAFE_FREE(utf8_name);
	return ret;
}
#endif

/**************************************************************
 krb5_parse_name that returns a UNIX charset name. Must
 be freed with normal free() call.
**************************************************************/

 krb5_error_code smb_krb5_unparse_name(krb5_context context,
					krb5_const_principal principal,
					char **unix_name)
{
	krb5_error_code ret;
	char *utf8_name;

	ret = krb5_unparse_name(context, principal, &utf8_name);
	if (ret) {
		return ret;
	}

	if (pull_utf8_allocate(unix_name, utf8_name)==-1) {
		krb5_free_unparsed_name(context, utf8_name);
		return ENOMEM;
	}
	krb5_free_unparsed_name(context, utf8_name);
	return 0;
}

#ifndef HAVE_KRB5_SET_REAL_TIME
/*
 * This function is not in the Heimdal mainline.
 */
 krb5_error_code krb5_set_real_time(krb5_context context, int32_t seconds, int32_t microseconds)
{
	krb5_error_code ret;
	int32_t sec, usec;

	ret = krb5_us_timeofday(context, &sec, &usec);
	if (ret)
		return ret;

	context->kdc_sec_offset = seconds - sec;
	context->kdc_usec_offset = microseconds - usec;

	return 0;
}
#endif

#if defined(HAVE_KRB5_SET_DEFAULT_IN_TKT_ETYPES) && !defined(HAVE_KRB5_SET_DEFAULT_TGS_KTYPES)
 krb5_error_code krb5_set_default_tgs_ktypes(krb5_context ctx, const krb5_enctype *enc)
{
	return krb5_set_default_in_tkt_etypes(ctx, enc);
}
#endif

#if defined(HAVE_ADDR_TYPE_IN_KRB5_ADDRESS)
/* HEIMDAL */
 void setup_kaddr( krb5_address *pkaddr, struct sockaddr *paddr)
{
	pkaddr->addr_type = KRB5_ADDRESS_INET;
	pkaddr->address.length = sizeof(((struct sockaddr_in *)paddr)->sin_addr);
	pkaddr->address.data = (char *)&(((struct sockaddr_in *)paddr)->sin_addr);
}
#elif defined(HAVE_ADDRTYPE_IN_KRB5_ADDRESS)
/* MIT */
 void setup_kaddr( krb5_address *pkaddr, struct sockaddr *paddr)
{
	pkaddr->addrtype = ADDRTYPE_INET;
	pkaddr->length = sizeof(((struct sockaddr_in *)paddr)->sin_addr);
	pkaddr->contents = (krb5_octet *)&(((struct sockaddr_in *)paddr)->sin_addr);
}
#else
#error UNKNOWN_ADDRTYPE
#endif

#if defined(HAVE_KRB5_PRINCIPAL2SALT) && defined(HAVE_KRB5_USE_ENCTYPE) && defined(HAVE_KRB5_STRING_TO_KEY) && defined(HAVE_KRB5_ENCRYPT_BLOCK)
 int create_kerberos_key_from_string_direct(krb5_context context,
					krb5_principal host_princ,
					krb5_data *password,
					krb5_keyblock *key,
					krb5_enctype enctype)
{
	int ret;
	krb5_data salt;
	krb5_encrypt_block eblock;

	ret = krb5_principal2salt(context, host_princ, &salt);
	if (ret) {
		DEBUG(1,("krb5_principal2salt failed (%s)\n", error_message(ret)));
		return ret;
	}
	krb5_use_enctype(context, &eblock, enctype);
	ret = krb5_string_to_key(context, &eblock, key, password, &salt);
	SAFE_FREE(salt.data);
	return ret;
}
#elif defined(HAVE_KRB5_GET_PW_SALT) && defined(HAVE_KRB5_STRING_TO_KEY_SALT)
 int create_kerberos_key_from_string_direct(krb5_context context,
					krb5_principal host_princ,
					krb5_data *password,
					krb5_keyblock *key,
					krb5_enctype enctype)
{
	int ret;
	krb5_salt salt;

	ret = krb5_get_pw_salt(context, host_princ, &salt);
	if (ret) {
		DEBUG(1,("krb5_get_pw_salt failed (%s)\n", error_message(ret)));
		return ret;
	}
	
	ret = krb5_string_to_key_salt(context, enctype, password->data, salt, key);
	krb5_free_salt(context, salt);
	return ret;
}
#else
#error UNKNOWN_CREATE_KEY_FUNCTIONS
#endif

 int create_kerberos_key_from_string(krb5_context context,
					krb5_principal host_princ,
					krb5_data *password,
					krb5_keyblock *key,
					krb5_enctype enctype)
{
	krb5_principal salt_princ = NULL;
	int ret;
	/*
	 * Check if we've determined that the KDC is salting keys for this
	 * principal/enctype in a non-obvious way.  If it is, try to match
	 * its behavior.
	 */
	salt_princ = kerberos_fetch_salt_princ_for_host_princ(context, host_princ, enctype);
	ret = create_kerberos_key_from_string_direct(context, salt_princ ? salt_princ : host_princ, password, key, enctype);
	if (salt_princ) {
		krb5_free_principal(context, salt_princ);
	}
	return ret;
}

#if defined(HAVE_KRB5_GET_PERMITTED_ENCTYPES)
 krb5_error_code get_kerberos_allowed_etypes(krb5_context context, 
					    krb5_enctype **enctypes)
{
	return krb5_get_permitted_enctypes(context, enctypes);
}
#elif defined(HAVE_KRB5_GET_DEFAULT_IN_TKT_ETYPES)
 krb5_error_code get_kerberos_allowed_etypes(krb5_context context, 
					    krb5_enctype **enctypes)
{
	return krb5_get_default_in_tkt_etypes(context, enctypes);
}
#else
#error UNKNOWN_GET_ENCTYPES_FUNCTIONS
#endif

 void free_kerberos_etypes(krb5_context context, 
			   krb5_enctype *enctypes)
{
#if defined(HAVE_KRB5_FREE_KTYPES)
	krb5_free_ktypes(context, enctypes);
	return;
#else
	SAFE_FREE(enctypes);
	return;
#endif
}

#if defined(HAVE_KRB5_AUTH_CON_SETKEY) && !defined(HAVE_KRB5_AUTH_CON_SETUSERUSERKEY)
 krb5_error_code krb5_auth_con_setuseruserkey(krb5_context context,
					krb5_auth_context auth_context,
					krb5_keyblock *keyblock)
{
	return krb5_auth_con_setkey(context, auth_context, keyblock);
}
#endif

BOOL unwrap_pac(TALLOC_CTX *mem_ctx, DATA_BLOB *auth_data, DATA_BLOB *unwrapped_pac_data)
{
	DATA_BLOB pac_contents;
	ASN1_DATA data;
	int data_type;

	if (!auth_data->length) {
		return False;
	}

	asn1_load(&data, *auth_data);
	asn1_start_tag(&data, ASN1_SEQUENCE(0));
	asn1_start_tag(&data, ASN1_SEQUENCE(0));
	asn1_start_tag(&data, ASN1_CONTEXT(0));
	asn1_read_Integer(&data, &data_type);
	
	if (data_type != KRB5_AUTHDATA_WIN2K_PAC ) {
		DEBUG(10,("authorization data is not a Windows PAC (type: %d)\n", data_type));
		asn1_free(&data);
		return False;
	}
	
	asn1_end_tag(&data);
	asn1_start_tag(&data, ASN1_CONTEXT(1));
	asn1_read_OctetString(&data, &pac_contents);
	asn1_end_tag(&data);
	asn1_end_tag(&data);
	asn1_end_tag(&data);
	asn1_free(&data);

	*unwrapped_pac_data = data_blob_talloc(mem_ctx, pac_contents.data, pac_contents.length);

	data_blob_free(&pac_contents);

	return True;
}

 BOOL get_auth_data_from_tkt(TALLOC_CTX *mem_ctx, DATA_BLOB *auth_data, krb5_ticket *tkt)
{
	DATA_BLOB auth_data_wrapped;
	BOOL got_auth_data_pac = False;
	int i;
	
#if defined(HAVE_KRB5_TKT_ENC_PART2)
	if (tkt->enc_part2 && tkt->enc_part2->authorization_data && 
	    tkt->enc_part2->authorization_data[0] && 
	    tkt->enc_part2->authorization_data[0]->length)
	{
		for (i = 0; tkt->enc_part2->authorization_data[i] != NULL; i++) {
		
			if (tkt->enc_part2->authorization_data[i]->ad_type != 
			    KRB5_AUTHDATA_IF_RELEVANT) {
				DEBUG(10,("get_auth_data_from_tkt: ad_type is %d\n", 
					tkt->enc_part2->authorization_data[i]->ad_type));
				continue;
			}

			auth_data_wrapped = data_blob(tkt->enc_part2->authorization_data[i]->contents,
						      tkt->enc_part2->authorization_data[i]->length);

			/* check if it is a PAC */
			got_auth_data_pac = unwrap_pac(mem_ctx, &auth_data_wrapped, auth_data);
			data_blob_free(&auth_data_wrapped);
			
			if (!got_auth_data_pac) {
				continue;
			}
		}

		return got_auth_data_pac;
	}
		
#else
	if (tkt->ticket.authorization_data && 
	    tkt->ticket.authorization_data->len)
	{
		for (i = 0; i < tkt->ticket.authorization_data->len; i++) {
			
			if (tkt->ticket.authorization_data->val[i].ad_type != 
			    KRB5_AUTHDATA_IF_RELEVANT) {
				DEBUG(10,("get_auth_data_from_tkt: ad_type is %d\n", 
					tkt->ticket.authorization_data->val[i].ad_type));
				continue;
			}

			auth_data_wrapped = data_blob(tkt->ticket.authorization_data->val[i].ad_data.data,
						      tkt->ticket.authorization_data->val[i].ad_data.length);

			/* check if it is a PAC */
			got_auth_data_pac = unwrap_pac(mem_ctx, &auth_data_wrapped, auth_data);
			data_blob_free(&auth_data_wrapped);
			
			if (!got_auth_data_pac) {
				continue;
			}
		}

		return got_auth_data_pac;
	}
#endif
	return False;
}

 krb5_const_principal get_principal_from_tkt(krb5_ticket *tkt)
{
#if defined(HAVE_KRB5_TKT_ENC_PART2)
	return tkt->enc_part2->client;
#else
	return tkt->client;
#endif
}

#if !defined(HAVE_KRB5_LOCATE_KDC)
 krb5_error_code krb5_locate_kdc(krb5_context ctx, const krb5_data *realm, struct sockaddr **addr_pp, int *naddrs, int get_masters)
{
	krb5_krbhst_handle hnd;
	krb5_krbhst_info *hinfo;
	krb5_error_code rc;
	int num_kdcs, i;
	struct sockaddr *sa;
	struct addrinfo *ai;

	*addr_pp = NULL;
	*naddrs = 0;

	rc = krb5_krbhst_init(ctx, realm->data, KRB5_KRBHST_KDC, &hnd);
	if (rc) {
		DEBUG(0, ("krb5_locate_kdc: krb5_krbhst_init failed (%s)\n", error_message(rc)));
		return rc;
	}

	for ( num_kdcs = 0; (rc = krb5_krbhst_next(ctx, hnd, &hinfo) == 0); num_kdcs++)
		;

	krb5_krbhst_reset(ctx, hnd);

	if (!num_kdcs) {
		DEBUG(0, ("krb5_locate_kdc: zero kdcs found !\n"));
		krb5_krbhst_free(ctx, hnd);
		return -1;
	}

	sa = SMB_MALLOC_ARRAY( struct sockaddr, num_kdcs );
	if (!sa) {
		DEBUG(0, ("krb5_locate_kdc: malloc failed\n"));
		krb5_krbhst_free(ctx, hnd);
		naddrs = 0;
		return -1;
	}

	memset(sa, '\0', sizeof(struct sockaddr) * num_kdcs );

	for (i = 0; i < num_kdcs && (rc = krb5_krbhst_next(ctx, hnd, &hinfo) == 0); i++) {

#if defined(HAVE_KRB5_KRBHST_GET_ADDRINFO)
		rc = krb5_krbhst_get_addrinfo(ctx, hinfo, &ai);
		if (rc) {
			DEBUG(0,("krb5_krbhst_get_addrinfo failed: %s\n", error_message(rc)));
			continue;
		}
#endif
		if (hinfo->ai && hinfo->ai->ai_family == AF_INET) 
			memcpy(&sa[i], hinfo->ai->ai_addr, sizeof(struct sockaddr));
	}

	krb5_krbhst_free(ctx, hnd);

	*naddrs = num_kdcs;
	*addr_pp = sa;
	return 0;
}
#endif

#if !defined(HAVE_KRB5_FREE_UNPARSED_NAME)
 void krb5_free_unparsed_name(krb5_context context, char *val)
{
	SAFE_FREE(val);
}
#endif

 void kerberos_free_data_contents(krb5_context context, krb5_data *pdata)
{
#if defined(HAVE_KRB5_FREE_DATA_CONTENTS)
	if (pdata->data) {
		krb5_free_data_contents(context, pdata);
	}
#else
	SAFE_FREE(pdata->data);
#endif
}

 void kerberos_set_creds_enctype(krb5_creds *pcreds, int enctype)
{
#if defined(HAVE_KRB5_KEYBLOCK_IN_CREDS)
	KRB5_KEY_TYPE((&pcreds->keyblock)) = enctype;
#elif defined(HAVE_KRB5_SESSION_IN_CREDS)
	KRB5_KEY_TYPE((&pcreds->session)) = enctype;
#else
#error UNKNOWN_KEYBLOCK_MEMBER_IN_KRB5_CREDS_STRUCT
#endif
}

 BOOL kerberos_compatible_enctypes(krb5_context context,
				  krb5_enctype enctype1,
				  krb5_enctype enctype2)
{
#if defined(HAVE_KRB5_C_ENCTYPE_COMPARE)
	krb5_boolean similar = 0;

	krb5_c_enctype_compare(context, enctype1, enctype2, &similar);
	return similar ? True : False;
#elif defined(HAVE_KRB5_ENCTYPES_COMPATIBLE_KEYS)
	return krb5_enctypes_compatible_keys(context, enctype1, enctype2) ? True : False;
#endif
}

static BOOL ads_cleanup_expired_creds(krb5_context context, 
				      krb5_ccache  ccache,
				      krb5_creds  *credsp)
{
	krb5_error_code retval;
	const char *cc_type = krb5_cc_get_type(context, ccache);

	DEBUG(3, ("ads_cleanup_expired_creds: Ticket in ccache[%s:%s] expiration %s\n",
		  cc_type, krb5_cc_get_name(context, ccache),
		  http_timestring(credsp->times.endtime)));

	/* we will probably need new tickets if the current ones
	   will expire within 10 seconds.
	*/
	if (credsp->times.endtime >= (time(NULL) + 10))
		return False;

	/* heimdal won't remove creds from a file ccache, and 
	   perhaps we shouldn't anyway, since internally we 
	   use memory ccaches, and a FILE one probably means that
	   we're using creds obtained outside of our exectuable
	*/
	if (strequal(cc_type, "FILE")) {
		DEBUG(5, ("ads_cleanup_expired_creds: We do not remove creds from a %s ccache\n", cc_type));
		return False;
	}

	retval = krb5_cc_remove_cred(context, ccache, 0, credsp);
	if (retval) {
		DEBUG(1, ("ads_cleanup_expired_creds: krb5_cc_remove_cred failed, err %s\n",
			  error_message(retval)));
		/* If we have an error in this, we want to display it,
		   but continue as though we deleted it */
	}
	return True;
}

/*
  we can't use krb5_mk_req because w2k wants the service to be in a particular format
*/
static krb5_error_code ads_krb5_mk_req(krb5_context context, 
				       krb5_auth_context *auth_context, 
				       const krb5_flags ap_req_options,
				       const char *principal,
				       krb5_ccache ccache, 
				       krb5_data *outbuf)
{
	krb5_error_code 	  retval;
	krb5_principal	  server;
	krb5_creds 		* credsp;
	krb5_creds 		  creds;
	krb5_data in_data;
	BOOL creds_ready = False;
	int i = 0, maxtries = 3;
	
	retval = smb_krb5_parse_name(context, principal, &server);
	if (retval) {
		DEBUG(1,("ads_krb5_mk_req: Failed to parse principal %s\n", principal));
		return retval;
	}
	
	/* obtain ticket & session key */
	ZERO_STRUCT(creds);
	if ((retval = krb5_copy_principal(context, server, &creds.server))) {
		DEBUG(1,("ads_krb5_mk_req: krb5_copy_principal failed (%s)\n", 
			 error_message(retval)));
		goto cleanup_princ;
	}
	
	if ((retval = krb5_cc_get_principal(context, ccache, &creds.client))) {
		/* This can commonly fail on smbd startup with no ticket in the cache.
		 * Report at higher level than 1. */
		DEBUG(3,("ads_krb5_mk_req: krb5_cc_get_principal failed (%s)\n", 
			 error_message(retval)));
		goto cleanup_creds;
	}

	while (!creds_ready && (i < maxtries)) {
		if ((retval = krb5_get_credentials(context, 0, ccache, 
						   &creds, &credsp))) {
			DEBUG(1,("ads_krb5_mk_req: krb5_get_credentials failed for %s (%s)\n",
				 principal, error_message(retval)));
			goto cleanup_creds;
		}

		/* cope with ticket being in the future due to clock skew */
		if ((unsigned)credsp->times.starttime > time(NULL)) {
			time_t t = time(NULL);
			int time_offset =(int)((unsigned)credsp->times.starttime-t);
			DEBUG(4,("ads_krb5_mk_req: Advancing clock by %d seconds to cope with clock skew\n", time_offset));
			krb5_set_real_time(context, t + time_offset + 1, 0);
		}

		if (!ads_cleanup_expired_creds(context, ccache, credsp))
			creds_ready = True;

		i++;
	}

	DEBUG(10,("ads_krb5_mk_req: Ticket (%s) in ccache (%s:%s) is valid until: (%s - %u)\n",
		  principal, krb5_cc_get_type(context, ccache), krb5_cc_get_name(context, ccache),
		  http_timestring((unsigned)credsp->times.endtime), 
		  (unsigned)credsp->times.endtime));

	in_data.length = 0;
	retval = krb5_mk_req_extended(context, auth_context, ap_req_options, 
				      &in_data, credsp, outbuf);
	if (retval) {
		DEBUG(1,("ads_krb5_mk_req: krb5_mk_req_extended failed (%s)\n", 
			 error_message(retval)));
	}
	
	krb5_free_creds(context, credsp);

cleanup_creds:
	krb5_free_cred_contents(context, &creds);

cleanup_princ:
	krb5_free_principal(context, server);

	return retval;
}

/*
  get a kerberos5 ticket for the given service 
*/
int cli_krb5_get_ticket(const char *principal, time_t time_offset, 
			DATA_BLOB *ticket, DATA_BLOB *session_key_krb5, 
			uint32 extra_ap_opts, const char *ccname)
{
	krb5_error_code retval;
	krb5_data packet;
	krb5_context context = NULL;
	krb5_ccache ccdef = NULL;
	krb5_auth_context auth_context = NULL;
	krb5_enctype enc_types[] = {
#ifdef ENCTYPE_ARCFOUR_HMAC
		ENCTYPE_ARCFOUR_HMAC,
#endif 
		ENCTYPE_DES_CBC_MD5, 
		ENCTYPE_DES_CBC_CRC, 
		ENCTYPE_NULL};

	initialize_krb5_error_table();
	retval = krb5_init_context(&context);
	if (retval) {
		DEBUG(1,("cli_krb5_get_ticket: krb5_init_context failed (%s)\n", 
			 error_message(retval)));
		goto failed;
	}

	if (time_offset != 0) {
		krb5_set_real_time(context, time(NULL) + time_offset, 0);
	}

	if ((retval = krb5_cc_resolve(context, ccname ?
			ccname : krb5_cc_default_name(context), &ccdef))) {
		DEBUG(1,("cli_krb5_get_ticket: krb5_cc_default failed (%s)\n",
			 error_message(retval)));
		goto failed;
	}

	if ((retval = krb5_set_default_tgs_ktypes(context, enc_types))) {
		DEBUG(1,("cli_krb5_get_ticket: krb5_set_default_tgs_ktypes failed (%s)\n",
			 error_message(retval)));
		goto failed;
	}

	if ((retval = ads_krb5_mk_req(context, 
					&auth_context, 
					AP_OPTS_USE_SUBKEY | (krb5_flags)extra_ap_opts,
					principal,
					ccdef, &packet))) {
		goto failed;
	}

	get_krb5_smb_session_key(context, auth_context, session_key_krb5, False);

	*ticket = data_blob(packet.data, packet.length);

 	kerberos_free_data_contents(context, &packet); 

failed:

	if ( context ) {
		if (ccdef)
			krb5_cc_close(context, ccdef);
		if (auth_context)
			krb5_auth_con_free(context, auth_context);
		krb5_free_context(context);
	}
		
	return retval;
}

 BOOL get_krb5_smb_session_key(krb5_context context, krb5_auth_context auth_context, DATA_BLOB *session_key, BOOL remote)
 {
	krb5_keyblock *skey;
	krb5_error_code err;
	BOOL ret = False;

	if (remote)
		err = krb5_auth_con_getremotesubkey(context, auth_context, &skey);
	else
		err = krb5_auth_con_getlocalsubkey(context, auth_context, &skey);
	if (err == 0 && skey != NULL) {
		DEBUG(10, ("Got KRB5 session key of length %d\n",  (int)KRB5_KEY_LENGTH(skey)));
		*session_key = data_blob(KRB5_KEY_DATA(skey), KRB5_KEY_LENGTH(skey));
		dump_data_pw("KRB5 Session Key:\n", session_key->data, session_key->length);

		ret = True;

		krb5_free_keyblock(context, skey);
	} else {
		DEBUG(10, ("KRB5 error getting session key %d\n", err));
	}

	return ret;
 }


#if defined(HAVE_KRB5_PRINCIPAL_GET_COMP_STRING) && !defined(HAVE_KRB5_PRINC_COMPONENT)
 const krb5_data *krb5_princ_component(krb5_context context, krb5_principal principal, int i );

 const krb5_data *krb5_princ_component(krb5_context context, krb5_principal principal, int i )
{
	static krb5_data kdata;

	kdata.data = (char *)krb5_principal_get_comp_string(context, principal, i);
	kdata.length = strlen(kdata.data);
	return &kdata;
}
#endif

 krb5_error_code smb_krb5_kt_free_entry(krb5_context context, krb5_keytab_entry *kt_entry)
{
#if defined(HAVE_KRB5_KT_FREE_ENTRY)
	return krb5_kt_free_entry(context, kt_entry);
#elif defined(HAVE_KRB5_FREE_KEYTAB_ENTRY_CONTENTS)
	return krb5_free_keytab_entry_contents(context, kt_entry);
#else
#error UNKNOWN_KT_FREE_FUNCTION
#endif
}

 void smb_krb5_checksum_from_pac_sig(krb5_checksum *cksum, 
				    PAC_SIGNATURE_DATA *sig)
{
#ifdef HAVE_CHECKSUM_IN_KRB5_CHECKSUM
	cksum->cksumtype	= (krb5_cksumtype)sig->type;
	cksum->checksum.length	= sig->signature.buf_len;
	cksum->checksum.data	= sig->signature.buffer;
#else
	cksum->checksum_type	= (krb5_cksumtype)sig->type;
	cksum->length		= sig->signature.buf_len;
	cksum->contents		= sig->signature.buffer;
#endif
}

 krb5_error_code smb_krb5_verify_checksum(krb5_context context,
					 krb5_keyblock *keyblock,
					 krb5_keyusage usage,
					 krb5_checksum *cksum,
					 uint8 *data,
					 size_t length)
{
	krb5_error_code ret;

	/* verify the checksum */

	/* welcome to the wonderful world of samba's kerberos abstraction layer:
	 * 
	 * function			heimdal 0.6.1rc3	heimdal 0.7	MIT krb 1.4.2
	 * -----------------------------------------------------------------------------
	 * krb5_c_verify_checksum	-			works		works
	 * krb5_verify_checksum		works (6 args)		works (6 args)	broken (7 args) 
	 */

#if defined(HAVE_KRB5_C_VERIFY_CHECKSUM)
	{
		krb5_boolean checksum_valid = False;
		krb5_data input;

		input.data = (char *)data;
		input.length = length;

		ret = krb5_c_verify_checksum(context, 
					     keyblock, 
					     usage,
					     &input, 
					     cksum,
					     &checksum_valid);
		if (ret) {
			DEBUG(3,("smb_krb5_verify_checksum: krb5_c_verify_checksum() failed: %s\n", 
				error_message(ret)));
			return ret;
		}

		if (!checksum_valid)
			ret = KRB5KRB_AP_ERR_BAD_INTEGRITY;
	}

#elif KRB5_VERIFY_CHECKSUM_ARGS == 6 && defined(HAVE_KRB5_CRYPTO_INIT) && defined(HAVE_KRB5_CRYPTO) && defined(HAVE_KRB5_CRYPTO_DESTROY)

	/* Warning: MIT's krb5_verify_checksum cannot be used as it will use a key
	 * without enctype and it ignores any key_usage types - Guenther */

	{

		krb5_crypto crypto;
		ret = krb5_crypto_init(context,
				       keyblock,
				       0,
				       &crypto);
		if (ret) {
			DEBUG(0,("smb_krb5_verify_checksum: krb5_crypto_init() failed: %s\n", 
				error_message(ret)));
			return ret;
		}

		ret = krb5_verify_checksum(context,
					   crypto,
					   usage,
					   data,
					   length,
					   cksum);

		krb5_crypto_destroy(context, crypto);
	}

#else
#error UNKNOWN_KRB5_VERIFY_CHECKSUM_FUNCTION
#endif

	return ret;
}

 time_t get_authtime_from_tkt(krb5_ticket *tkt)
{
#if defined(HAVE_KRB5_TKT_ENC_PART2)
	return tkt->enc_part2->times.authtime;
#else
	return tkt->ticket.authtime;
#endif
}

static int get_kvno_from_ap_req(krb5_ap_req *ap_req)
{
#ifdef HAVE_TICKET_POINTER_IN_KRB5_AP_REQ /* MIT */
	if (ap_req->ticket->enc_part.kvno)
		return ap_req->ticket->enc_part.kvno;
#else /* Heimdal */
	if (ap_req->ticket.enc_part.kvno) 
		return *ap_req->ticket.enc_part.kvno;
#endif
	return 0;
}

static krb5_enctype get_enctype_from_ap_req(krb5_ap_req *ap_req)
{
#ifdef HAVE_ETYPE_IN_ENCRYPTEDDATA /* Heimdal */
	return ap_req->ticket.enc_part.etype;
#else /* MIT */
	return ap_req->ticket->enc_part.enctype;
#endif
}

static krb5_error_code
get_key_from_keytab(krb5_context context,
		    krb5_const_principal server,
		    krb5_enctype enctype,
		    krb5_kvno kvno,
		    krb5_keyblock **out_key)
{
	krb5_keytab_entry entry;
	krb5_error_code ret;
	krb5_keytab keytab;
	char *name = NULL;

	/* We have to open a new keytab handle here, as MIT does
	   an implicit open/getnext/close on krb5_kt_get_entry. We
	   may be in the middle of a keytab enumeration when this is
	   called. JRA. */

	ret = krb5_kt_default(context, &keytab);
	if (ret) {
		DEBUG(0,("get_key_from_keytab: failed to open keytab: %s\n", error_message(ret)));
		return ret;
	}

	if ( DEBUGLEVEL >= 10 ) {
		if (smb_krb5_unparse_name(context, server, &name) == 0) {
			DEBUG(10,("get_key_from_keytab: will look for kvno %d, enctype %d and name: %s\n", 
				kvno, enctype, name));
			SAFE_FREE(name);
		}
	}

	ret = krb5_kt_get_entry(context,
				keytab,
				server,
				kvno,
				enctype,
				&entry);

	if (ret) {
		DEBUG(0,("get_key_from_keytab: failed to retrieve key: %s\n", error_message(ret)));
		goto out;
	}

#ifdef HAVE_KRB5_KEYTAB_ENTRY_KEYBLOCK /* Heimdal */
	ret = krb5_copy_keyblock(context, &entry.keyblock, out_key);
#elif defined(HAVE_KRB5_KEYTAB_ENTRY_KEY) /* MIT */
	ret = krb5_copy_keyblock(context, &entry.key, out_key);
#else
#error UNKNOWN_KRB5_KEYTAB_ENTRY_FORMAT
#endif

	if (ret) {
		DEBUG(0,("get_key_from_keytab: failed to copy key: %s\n", error_message(ret)));
		goto out;
	}
		
	smb_krb5_kt_free_entry(context, &entry);
	
out:    
	krb5_kt_close(context, keytab);
	return ret;
}

 void smb_krb5_free_ap_req(krb5_context context, 
			  krb5_ap_req *ap_req)
{
#ifdef HAVE_KRB5_FREE_AP_REQ /* MIT */
	krb5_free_ap_req(context, ap_req);
#elif defined(HAVE_FREE_AP_REQ) /* Heimdal */
	free_AP_REQ(ap_req);
#else
#error UNKNOWN_KRB5_AP_REQ_FREE_FUNCTION
#endif
}

/* Prototypes */
#if defined(HAVE_DECODE_KRB5_AP_REQ) /* MIT */
krb5_error_code decode_krb5_ap_req(const krb5_data *code, krb5_ap_req **rep);
#endif

 krb5_error_code smb_krb5_get_keyinfo_from_ap_req(krb5_context context, 
						 const krb5_data *inbuf, 
						 krb5_kvno *kvno, 
						 krb5_enctype *enctype)
{
	krb5_error_code ret;
#ifdef HAVE_KRB5_DECODE_AP_REQ /* Heimdal */
	{
		krb5_ap_req ap_req;
		
		ret = krb5_decode_ap_req(context, inbuf, &ap_req);
		if (ret)
			return ret;

		*kvno = get_kvno_from_ap_req(&ap_req);
		*enctype = get_enctype_from_ap_req(&ap_req);

		smb_krb5_free_ap_req(context, &ap_req);
	}
#elif defined(HAVE_DECODE_KRB5_AP_REQ) /* MIT */
	{
		krb5_ap_req *ap_req = NULL;

		ret = decode_krb5_ap_req(inbuf, &ap_req);
		if (ret)
			return ret;
		
		*kvno = get_kvno_from_ap_req(ap_req);
		*enctype = get_enctype_from_ap_req(ap_req);

		smb_krb5_free_ap_req(context, ap_req);
	}
#else
#error UNKOWN_KRB5_AP_REQ_DECODING_FUNCTION
#endif
	return ret;
}

 krb5_error_code krb5_rd_req_return_keyblock_from_keytab(krb5_context context,
							krb5_auth_context *auth_context,
							const krb5_data *inbuf,
							krb5_const_principal server,
							krb5_keytab keytab,
							krb5_flags *ap_req_options,
							krb5_ticket **ticket, 
							krb5_keyblock **keyblock)
{
	krb5_error_code ret;
	krb5_kvno kvno;
	krb5_enctype enctype;
	krb5_keyblock *local_keyblock;

	ret = krb5_rd_req(context, 
			  auth_context, 
			  inbuf, 
			  server, 
			  keytab, 
			  ap_req_options, 
			  ticket);
	if (ret) {
		return ret;
	}
	
	ret = smb_krb5_get_keyinfo_from_ap_req(context, inbuf, &kvno, &enctype);
	if (ret) {
		return ret;
	}

	ret = get_key_from_keytab(context, 
				  server,
				  enctype,
				  kvno,
				  &local_keyblock);
	if (ret) {
		DEBUG(0,("krb5_rd_req_return_keyblock_from_keytab: failed to call get_key_from_keytab\n"));
		goto out;
	}

out:
	if (ret && local_keyblock != NULL) {
	        krb5_free_keyblock(context, local_keyblock);
	} else {
		*keyblock = local_keyblock;
	}

	return ret;
}

 krb5_error_code smb_krb5_parse_name_norealm(krb5_context context, 
					    const char *name, 
					    krb5_principal *principal)
{
#ifdef HAVE_KRB5_PARSE_NAME_NOREALM
	return smb_krb5_parse_name_norealm_conv(context, name, principal);
#endif

	/* we are cheating here because parse_name will in fact set the realm.
	 * We don't care as the only caller of smb_krb5_parse_name_norealm
	 * ignores the realm anyway when calling
	 * smb_krb5_principal_compare_any_realm later - Guenther */

	return smb_krb5_parse_name(context, name, principal);
}

 BOOL smb_krb5_principal_compare_any_realm(krb5_context context, 
					  krb5_const_principal princ1, 
					  krb5_const_principal princ2)
{
#ifdef HAVE_KRB5_PRINCIPAL_COMPARE_ANY_REALM

	return krb5_principal_compare_any_realm(context, princ1, princ2);

/* krb5_princ_size is a macro in MIT */
#elif defined(HAVE_KRB5_PRINC_SIZE) || defined(krb5_princ_size)

	int i, len1, len2;
	const krb5_data *p1, *p2;

	len1 = krb5_princ_size(context, princ1);
	len2 = krb5_princ_size(context, princ2);

	if (len1 != len2)
		return False;

	for (i = 0; i < len1; i++) {

		p1 = krb5_princ_component(context, CONST_DISCARD(krb5_principal, princ1), i);
		p2 = krb5_princ_component(context, CONST_DISCARD(krb5_principal, princ2), i);

		if (p1->length != p2->length ||	memcmp(p1->data, p2->data, p1->length))
			return False;
	}

	return True;
#else
#error NO_SUITABLE_PRINCIPAL_COMPARE_FUNCTION
#endif
}

 krb5_error_code smb_krb5_renew_ticket(const char *ccache_string,	/* FILE:/tmp/krb5cc_0 */
				       const char *client_string,	/* gd@BER.SUSE.DE */
				       const char *service_string,	/* krbtgt/BER.SUSE.DE@BER.SUSE.DE */
				       time_t *new_start_time)
{
	krb5_error_code ret;
	krb5_context context = NULL;
	krb5_ccache ccache = NULL;
	krb5_principal client = NULL;

	initialize_krb5_error_table();
	ret = krb5_init_context(&context);
	if (ret) {
		goto done;
	}

	if (!ccache_string) {
		ccache_string = krb5_cc_default_name(context);
	}

	DEBUG(10,("smb_krb5_renew_ticket: using %s as ccache\n", ccache_string));

	/* FIXME: we should not fall back to defaults */
	ret = krb5_cc_resolve(context, CONST_DISCARD(char *, ccache_string), &ccache);
	if (ret) {
		goto done;
	}

#ifdef HAVE_KRB5_GET_RENEWED_CREDS	/* MIT */
	{
		krb5_creds creds;
	
		if (client_string) {
			ret = smb_krb5_parse_name(context, client_string, &client);
			if (ret) {
				goto done;
			}
		} else {
			ret = krb5_cc_get_principal(context, ccache, &client);
			if (ret) {
				goto done;
			}
		}
	
		ret = krb5_get_renewed_creds(context, &creds, client, ccache, CONST_DISCARD(char *, service_string));
		if (ret) {
			DEBUG(10,("smb_krb5_renew_ticket: krb5_get_kdc_cred failed: %s\n", error_message(ret)));
			goto done;
		}

		/* hm, doesn't that create a new one if the old one wasn't there? - Guenther */
		ret = krb5_cc_initialize(context, ccache, client);
		if (ret) {
			goto done;
		}
	
		ret = krb5_cc_store_cred(context, ccache, &creds);

		if (new_start_time) {
			*new_start_time = (time_t) creds.times.renew_till;
		}

		krb5_free_cred_contents(context, &creds);
	}
#elif defined(HAVE_KRB5_GET_KDC_CRED)	/* Heimdal */
	{
		krb5_kdc_flags flags;
		krb5_creds creds_in;
		krb5_realm *client_realm;
		krb5_creds *creds;

		memset(&creds_in, 0, sizeof(creds_in));

		if (client_string) {
			ret = smb_krb5_parse_name(context, client_string, &creds_in.client);
			if (ret) {
				goto done;
			}
		} else {
			ret = krb5_cc_get_principal(context, ccache, &creds_in.client);
			if (ret) {
				goto done;
			}
		}

		if (service_string) {
			ret = smb_krb5_parse_name(context, service_string, &creds_in.server);
			if (ret) { 
				goto done;
			}
		} else {
			/* build tgt service by default */
			client_realm = krb5_princ_realm(context, client);
			ret = krb5_make_principal(context, &creds_in.server, *client_realm, KRB5_TGS_NAME, *client_realm, NULL);
			if (ret) {
				goto done;
			}
		}

		flags.i = 0;
		flags.b.renewable = flags.b.renew = True;

		ret = krb5_get_kdc_cred(context, ccache, flags, NULL, NULL, &creds_in, &creds);
		if (ret) {
			DEBUG(10,("smb_krb5_renew_ticket: krb5_get_kdc_cred failed: %s\n", error_message(ret)));
			goto done;
		}
		
		/* hm, doesn't that create a new one if the old one wasn't there? - Guenther */
		ret = krb5_cc_initialize(context, ccache, creds_in.client);
		if (ret) {
			goto done;
		}
	
		ret = krb5_cc_store_cred(context, ccache, creds);

		if (new_start_time) {
			*new_start_time = (time_t) creds->times.renew_till;
		}
						
		krb5_free_cred_contents(context, &creds_in);
		krb5_free_creds(context, creds);
	}
#else
#error No suitable krb5 ticket renew function available
#endif


done:
	if (client) {
		krb5_free_principal(context, client);
	}
	if (context) {
		krb5_free_context(context);
	}
	if (ccache) {
		krb5_cc_close(context, ccache);
	}

	return ret;
    
}

 krb5_error_code smb_krb5_free_addresses(krb5_context context, smb_krb5_addresses *addr)
{
	krb5_error_code ret = 0;
	if (addr == NULL) {
		return ret;
	}
#if defined(HAVE_MAGIC_IN_KRB5_ADDRESS) && defined(HAVE_ADDRTYPE_IN_KRB5_ADDRESS) /* MIT */
	krb5_free_addresses(context, addr->addrs);
#elif defined(HAVE_ADDR_TYPE_IN_KRB5_ADDRESS) /* Heimdal */
	ret = krb5_free_addresses(context, addr->addrs);
	SAFE_FREE(addr->addrs);
#endif
	SAFE_FREE(addr);
	addr = NULL;
	return ret;
}

 krb5_error_code smb_krb5_gen_netbios_krb5_address(smb_krb5_addresses **kerb_addr)
{
	krb5_error_code ret = 0;
	nstring buf;
#if defined(HAVE_MAGIC_IN_KRB5_ADDRESS) && defined(HAVE_ADDRTYPE_IN_KRB5_ADDRESS) /* MIT */
	krb5_address **addrs = NULL;
#elif defined(HAVE_ADDR_TYPE_IN_KRB5_ADDRESS) /* Heimdal */
	krb5_addresses *addrs = NULL;
#endif

	*kerb_addr = (smb_krb5_addresses *)SMB_MALLOC(sizeof(smb_krb5_addresses));
	if (*kerb_addr == NULL) {
		return ENOMEM;
	}

	put_name(buf, global_myname(), ' ', 0x20);

#if defined(HAVE_MAGIC_IN_KRB5_ADDRESS) && defined(HAVE_ADDRTYPE_IN_KRB5_ADDRESS) /* MIT */
	{
		int num_addr = 2;

		addrs = (krb5_address **)SMB_MALLOC(sizeof(krb5_address *) * num_addr);
		if (addrs == NULL) {
			SAFE_FREE(kerb_addr);
			return ENOMEM;
		}

		memset(addrs, 0, sizeof(krb5_address *) * num_addr);

		addrs[0] = (krb5_address *)SMB_MALLOC(sizeof(krb5_address));
		if (addrs[0] == NULL) {
			SAFE_FREE(addrs);
			SAFE_FREE(kerb_addr);
			return ENOMEM;
		}

		addrs[0]->magic = KV5M_ADDRESS;
		addrs[0]->addrtype = KRB5_ADDR_NETBIOS;
		addrs[0]->length = MAX_NETBIOSNAME_LEN;
		addrs[0]->contents = (unsigned char *)SMB_MALLOC(addrs[0]->length);
		if (addrs[0]->contents == NULL) {
			SAFE_FREE(addrs[0]);
			SAFE_FREE(addrs);
			SAFE_FREE(kerb_addr);
			return ENOMEM;
		}

		memcpy(addrs[0]->contents, buf, addrs[0]->length);

		addrs[1] = NULL;
	}
#elif defined(HAVE_ADDR_TYPE_IN_KRB5_ADDRESS) /* Heimdal */
	{
		addrs = (krb5_addresses *)SMB_MALLOC(sizeof(krb5_addresses));
		if (addrs == NULL) {
			SAFE_FREE(kerb_addr);
			return ENOMEM;
		}

		memset(addrs, 0, sizeof(krb5_addresses));

		addrs->len = 1;
		addrs->val = (krb5_address *)SMB_MALLOC(sizeof(krb5_address));
		if (addrs->val == NULL) {
			SAFE_FREE(addrs);
			SAFE_FREE(kerb_addr);
			return ENOMEM;
		}

		addrs->val[0].addr_type = KRB5_ADDR_NETBIOS;
		addrs->val[0].address.length = MAX_NETBIOSNAME_LEN;
		addrs->val[0].address.data = (unsigned char *)SMB_MALLOC(addrs->val[0].address.length);
		if (addrs->val[0].address.data == NULL) {
			SAFE_FREE(addrs->val);
			SAFE_FREE(addrs);
			SAFE_FREE(kerb_addr);
			return ENOMEM;
		}

		memcpy(addrs->val[0].address.data, buf, addrs->val[0].address.length);
	}
#else
#error UNKNOWN_KRB5_ADDRESS_FORMAT
#endif
	(*kerb_addr)->addrs = addrs;

	return ret;
}

 void smb_krb5_free_error(krb5_context context, krb5_error *krberror)
{
#ifdef HAVE_KRB5_FREE_ERROR_CONTENTS /* Heimdal */
	krb5_free_error_contents(context, krberror);
#else /* MIT */
	krb5_free_error(context, krberror);
#endif
}

 krb5_error_code handle_krberror_packet(krb5_context context,
					krb5_data *packet)
{
	krb5_error_code ret;
	BOOL got_error_code = False;

	DEBUG(10,("handle_krberror_packet: got error packet\n"));
	
#ifdef HAVE_E_DATA_POINTER_IN_KRB5_ERROR /* Heimdal */
	{
		krb5_error krberror;

		if ((ret = krb5_rd_error(context, packet, &krberror))) {
			DEBUG(10,("handle_krberror_packet: krb5_rd_error failed with: %s\n", 
				error_message(ret)));
			return ret;
		}

		if (krberror.e_data == NULL || krberror.e_data->data == NULL) {
			ret = (krb5_error_code) krberror.error_code;
			got_error_code = True;
		}

		smb_krb5_free_error(context, &krberror);
	}
#else /* MIT */
	{
		krb5_error *krberror;

		if ((ret = krb5_rd_error(context, packet, &krberror))) {
			DEBUG(10,("handle_krberror_packet: krb5_rd_error failed with: %s\n", 
				error_message(ret)));
			return ret;
		}

		if (krberror->e_data.data == NULL) {
			ret = ERROR_TABLE_BASE_krb5 + (krb5_error_code) krberror->error;
			got_error_code = True;
		}
		smb_krb5_free_error(context, krberror);
	}
#endif
	if (got_error_code) {
		DEBUG(5,("handle_krberror_packet: got KERBERR from kpasswd: %s (%d)\n", 
			error_message(ret), ret));
	}
	return ret;
}

#else /* HAVE_KRB5 */
 /* this saves a few linking headaches */
 int cli_krb5_get_ticket(const char *principal, time_t time_offset, 
			DATA_BLOB *ticket, DATA_BLOB *session_key_krb5, uint32 extra_ap_opts,
			const char *ccname) 
{
	 DEBUG(0,("NO KERBEROS SUPPORT\n"));
	 return 1;
}

#endif
