/* 
   Unix SMB/CIFS implementation.
   krb5 set password implementation
   Copyright (C) Andrew Tridgell 2001
   Copyright (C) Remus Koos 2001 (remuskoos@yahoo.com)
   
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

#include "includes.h"

#ifdef _XBOX
#define close(a) closesocket(a)
#endif

#ifdef HAVE_KRB5

#define DEFAULT_KPASSWD_PORT	464

#define KRB5_KPASSWD_VERS_CHANGEPW		1

#define KRB5_KPASSWD_VERS_SETPW			0xff80
#define KRB5_KPASSWD_VERS_SETPW_ALT		2

#define KRB5_KPASSWD_SUCCESS			0
#define KRB5_KPASSWD_MALFORMED			1
#define KRB5_KPASSWD_HARDERROR			2
#define KRB5_KPASSWD_AUTHERROR			3
#define KRB5_KPASSWD_SOFTERROR			4
#define KRB5_KPASSWD_ACCESSDENIED		5
#define KRB5_KPASSWD_BAD_VERSION		6
#define KRB5_KPASSWD_INITIAL_FLAG_NEEDED	7

/* Those are defined by kerberos-set-passwd-02.txt and are probably 
 * not supported by M$ implementation */
#define KRB5_KPASSWD_POLICY_REJECT		8
#define KRB5_KPASSWD_BAD_PRINCIPAL		9
#define KRB5_KPASSWD_ETYPE_NOSUPP		10

/*
 * we've got to be able to distinguish KRB_ERRORs from other
 * requests - valid response for CHPW v2 replies.
 */

#define krb5_is_krb_error(packet) \
	( packet && packet->length && (((char *)packet->data)[0] == 0x7e || ((char *)packet->data)[0] == 0x5e))

/* This implements kerberos password change protocol as specified in 
 * kerb-chg-password-02.txt and kerberos-set-passwd-02.txt
 * as well as microsoft version of the protocol 
 * as specified in kerberos-set-passwd-00.txt
 */
static DATA_BLOB encode_krb5_setpw(const char *principal, const char *password)
{
        char* princ_part1 = NULL;
	char* princ_part2 = NULL;
	char* realm = NULL;
	char* c;
	char* princ;

	ASN1_DATA req;
	DATA_BLOB ret;


	princ = SMB_STRDUP(principal);

	if ((c = strchr_m(princ, '/')) == NULL) {
		c = princ; 
	} else {
		*c = '\0';
		c++;
		princ_part1 = princ;
	}

	princ_part2 = c;

	if ((c = strchr_m(c, '@')) != NULL) {
		*c = '\0';
		c++;
		realm = c;
	} else {
		/* We must have a realm component. */
		return data_blob(NULL, 0);
	}

	memset(&req, 0, sizeof(req));
	
	asn1_push_tag(&req, ASN1_SEQUENCE(0));
	asn1_push_tag(&req, ASN1_CONTEXT(0));
	asn1_write_OctetString(&req, password, strlen(password));
	asn1_pop_tag(&req);

	asn1_push_tag(&req, ASN1_CONTEXT(1));
	asn1_push_tag(&req, ASN1_SEQUENCE(0));

	asn1_push_tag(&req, ASN1_CONTEXT(0));
	asn1_write_Integer(&req, 1);
	asn1_pop_tag(&req);

	asn1_push_tag(&req, ASN1_CONTEXT(1));
	asn1_push_tag(&req, ASN1_SEQUENCE(0));

	if (princ_part1) {
		asn1_write_GeneralString(&req, princ_part1);
	}
	
	asn1_write_GeneralString(&req, princ_part2);
	asn1_pop_tag(&req);
	asn1_pop_tag(&req);
	asn1_pop_tag(&req);
	asn1_pop_tag(&req);

	asn1_push_tag(&req, ASN1_CONTEXT(2));
	asn1_write_GeneralString(&req, realm);
	asn1_pop_tag(&req);
	asn1_pop_tag(&req);

	ret = data_blob(req.data, req.length);
	asn1_free(&req);

	free(princ);

	return ret;
}	

static krb5_error_code build_kpasswd_request(uint16 pversion,
					   krb5_context context,
					   krb5_auth_context auth_context,
					   krb5_data *ap_req,
					   const char *princ,
					   const char *passwd,
					   BOOL use_tcp,
					   krb5_data *packet)
{
	krb5_error_code ret;
	krb5_data cipherpw;
	krb5_data encoded_setpw;
	krb5_replay_data replay;
	char *p, *msg_start;
	DATA_BLOB setpw;
	unsigned int msg_length;

	ret = krb5_auth_con_setflags(context,
				     auth_context,KRB5_AUTH_CONTEXT_DO_SEQUENCE);
	if (ret) {
		DEBUG(1,("krb5_auth_con_setflags failed (%s)\n",
			 error_message(ret)));
		return ret;
	}

	/* handle protocol differences in chpw and setpw */
	if (pversion  == KRB5_KPASSWD_VERS_CHANGEPW)
		setpw = data_blob(passwd, strlen(passwd));
	else if (pversion == KRB5_KPASSWD_VERS_SETPW ||
		 pversion == KRB5_KPASSWD_VERS_SETPW_ALT)
		setpw = encode_krb5_setpw(princ, passwd);
	else
		return EINVAL;

	if (setpw.data == NULL || setpw.length == 0) {
		return EINVAL;
	}

	encoded_setpw.data = (char *)setpw.data;
	encoded_setpw.length = setpw.length;

	ret = krb5_mk_priv(context, auth_context,
			   &encoded_setpw, &cipherpw, &replay);
	
	data_blob_free(&setpw); 	/*from 'encode_krb5_setpw(...)' */
	
	if (ret) {
		DEBUG(1,("krb5_mk_priv failed (%s)\n", error_message(ret)));
		return ret;
	}

	packet->data = (char *)SMB_MALLOC(ap_req->length + cipherpw.length + (use_tcp ? 10 : 6 ));
	if (!packet->data)
		return -1;



	/* see the RFC for details */

	msg_start = p = ((char *)packet->data) + (use_tcp ? 4 : 0);
	p += 2;
	RSSVAL(p, 0, pversion);
	p += 2;
	RSSVAL(p, 0, ap_req->length);
	p += 2;
	memcpy(p, ap_req->data, ap_req->length);
	p += ap_req->length;
	memcpy(p, cipherpw.data, cipherpw.length);
	p += cipherpw.length;
	packet->length = PTR_DIFF(p,packet->data);
	msg_length = PTR_DIFF(p,msg_start);

	if (use_tcp) {
		RSIVAL(packet->data, 0, msg_length);
	}
	RSSVAL(msg_start, 0, msg_length);
	
	free(cipherpw.data);    /* from 'krb5_mk_priv(...)' */

	return 0;
}

static const struct kpasswd_errors {
	int result_code;
	const char *error_string;
} kpasswd_errors[] = {
	{KRB5_KPASSWD_MALFORMED, "Malformed request error"},
	{KRB5_KPASSWD_HARDERROR, "Server error"},
	{KRB5_KPASSWD_AUTHERROR, "Authentication error"},
	{KRB5_KPASSWD_SOFTERROR, "Password change rejected"},
	{KRB5_KPASSWD_ACCESSDENIED, "Client does not have proper authorization"},
	{KRB5_KPASSWD_BAD_VERSION, "Protocol version not supported"},
	{KRB5_KPASSWD_INITIAL_FLAG_NEEDED, "Authorization ticket must have initial flag set"},
	{KRB5_KPASSWD_POLICY_REJECT, "Password rejected due to policy requirements"},
	{KRB5_KPASSWD_BAD_PRINCIPAL, "Target principal does not exist"},
	{KRB5_KPASSWD_ETYPE_NOSUPP, "Unsupported encryption type"},
	{0, NULL}
};

static krb5_error_code setpw_result_code_string(krb5_context context,
						int result_code,
						const char **code_string)
{
        unsigned int idx = 0;

	while (kpasswd_errors[idx].error_string != NULL) {
		if (kpasswd_errors[idx].result_code == 
                    result_code) {
			*code_string = kpasswd_errors[idx].error_string;
			return 0;
		}
		idx++;
	}
	*code_string = "Password change failed";
        return (0);
}

 krb5_error_code kpasswd_err_to_krb5_err(krb5_error_code res_code) 
{
	switch(res_code) {
		case KRB5_KPASSWD_ACCESSDENIED:
			return KRB5KDC_ERR_BADOPTION;
		case KRB5_KPASSWD_INITIAL_FLAG_NEEDED:
			return KRB5KDC_ERR_BADOPTION;
			/* return KV5M_ALT_METHOD; MIT-only define */
		case KRB5_KPASSWD_ETYPE_NOSUPP:
			return KRB5KDC_ERR_ETYPE_NOSUPP;
		case KRB5_KPASSWD_BAD_PRINCIPAL:
			return KRB5KDC_ERR_C_PRINCIPAL_UNKNOWN;
		case KRB5_KPASSWD_POLICY_REJECT:
		case KRB5_KPASSWD_SOFTERROR:
			return KRB5KDC_ERR_POLICY;
		default:
			return KRB5KRB_ERR_GENERIC;
	}
}
static krb5_error_code parse_setpw_reply(krb5_context context,
					 BOOL use_tcp,
					 krb5_auth_context auth_context,
					 krb5_data *packet)
{
	krb5_data ap_rep;
	char *p;
	int vnum, ret, res_code;
	krb5_data cipherresult;
	krb5_data clearresult;
	krb5_ap_rep_enc_part *ap_rep_enc;
	krb5_replay_data replay;
	unsigned int msg_length = packet->length;


	if (packet->length < (use_tcp ? 8 : 4)) {
		return KRB5KRB_AP_ERR_MODIFIED;
	}
	
	p = packet->data;
	/*
	** see if it is an error
	*/
	if (krb5_is_krb_error(packet)) {

		ret = handle_krberror_packet(context, packet);
		if (ret) {
			return ret;
		}
	}

	
	/* tcp... */
	if (use_tcp) {
		msg_length -= 4;
		if (RIVAL(p, 0) != msg_length) {
			DEBUG(1,("Bad TCP packet length (%d/%d) from kpasswd server\n",
			RIVAL(p, 0), msg_length));
			return KRB5KRB_AP_ERR_MODIFIED;
		}

		p += 4;
	}
	
	if (RSVAL(p, 0) != msg_length) {
		DEBUG(1,("Bad packet length (%d/%d) from kpasswd server\n",
			 RSVAL(p, 0), msg_length));
		return KRB5KRB_AP_ERR_MODIFIED;
	}

	p += 2;

	vnum = RSVAL(p, 0); p += 2;

	/* FIXME: According to standard there is only one type of reply */	
	if (vnum != KRB5_KPASSWD_VERS_SETPW && 
	    vnum != KRB5_KPASSWD_VERS_SETPW_ALT && 
	    vnum != KRB5_KPASSWD_VERS_CHANGEPW) {
		DEBUG(1,("Bad vnum (%d) from kpasswd server\n", vnum));
		return KRB5KDC_ERR_BAD_PVNO;
	}
	
	ap_rep.length = RSVAL(p, 0); p += 2;
	
	if (p + ap_rep.length >= (char *)packet->data + packet->length) {
		DEBUG(1,("ptr beyond end of packet from kpasswd server\n"));
		return KRB5KRB_AP_ERR_MODIFIED;
	}
	
	if (ap_rep.length == 0) {
		DEBUG(1,("got unencrypted setpw result?!\n"));
		return KRB5KRB_AP_ERR_MODIFIED;
	}

	/* verify ap_rep */
	ap_rep.data = p;
	p += ap_rep.length;
	
	ret = krb5_rd_rep(context, auth_context, &ap_rep, &ap_rep_enc);
	if (ret) {
		DEBUG(1,("failed to rd setpw reply (%s)\n", error_message(ret)));
		return KRB5KRB_AP_ERR_MODIFIED;
	}
	
	krb5_free_ap_rep_enc_part(context, ap_rep_enc);
	
	cipherresult.data = p;
	cipherresult.length = ((char *)packet->data + packet->length) - p;
		
	ret = krb5_rd_priv(context, auth_context, &cipherresult, &clearresult,
			   &replay);
	if (ret) {
		DEBUG(1,("failed to decrypt setpw reply (%s)\n", error_message(ret)));
		return KRB5KRB_AP_ERR_MODIFIED;
	}

	if (clearresult.length < 2) {
	        free(clearresult.data);
		ret = KRB5KRB_AP_ERR_MODIFIED;
		return KRB5KRB_AP_ERR_MODIFIED;
	}
	
	p = clearresult.data;
	
	res_code = RSVAL(p, 0);
	
	free(clearresult.data);

	if ((res_code < KRB5_KPASSWD_SUCCESS) || 
	    (res_code > KRB5_KPASSWD_ETYPE_NOSUPP)) {
		return KRB5KRB_AP_ERR_MODIFIED;
	}

	if (res_code == KRB5_KPASSWD_SUCCESS) {
		return 0;
	} else {
		const char *errstr;
		setpw_result_code_string(context, res_code, &errstr);
		DEBUG(1, ("Error changing password: %s (%d)\n", errstr, res_code));

		return kpasswd_err_to_krb5_err(res_code);
	}
}

static ADS_STATUS do_krb5_kpasswd_request(krb5_context context,
					  const char *kdc_host,
					  uint16 pversion,
					  krb5_creds *credsp,
					  const char *princ,
					  const char *newpw)
{
	krb5_auth_context auth_context = NULL;
	krb5_data ap_req, chpw_req, chpw_rep;
	int ret, sock;
	socklen_t addr_len;
	struct sockaddr remote_addr, local_addr;
	struct in_addr *addr = interpret_addr2(kdc_host);
	krb5_address local_kaddr, remote_kaddr;
	BOOL	use_tcp = False;


	ret = krb5_mk_req_extended(context, &auth_context, AP_OPTS_USE_SUBKEY,
				   NULL, credsp, &ap_req);
	if (ret) {
		DEBUG(1,("krb5_mk_req_extended failed (%s)\n", error_message(ret)));
		return ADS_ERROR_KRB5(ret);
	}

	do {

		if (!use_tcp) {

			sock = open_udp_socket(kdc_host, DEFAULT_KPASSWD_PORT);

		} else {

			sock = open_socket_out(SOCK_STREAM, addr, DEFAULT_KPASSWD_PORT, 
					       LONG_CONNECT_TIMEOUT);
		}

		if (sock == -1) {
			int rc = errno;
			SAFE_FREE(ap_req.data);
			krb5_auth_con_free(context, auth_context);
			DEBUG(1,("failed to open kpasswd socket to %s (%s)\n", 
				 kdc_host, strerror(errno)));
			return ADS_ERROR_SYSTEM(rc);
		}
		
		addr_len = sizeof(remote_addr);
		getpeername(sock, &remote_addr, &addr_len);
		addr_len = sizeof(local_addr);
		getsockname(sock, &local_addr, &addr_len);
		
		setup_kaddr(&remote_kaddr, &remote_addr);
		setup_kaddr(&local_kaddr, &local_addr);
	
		ret = krb5_auth_con_setaddrs(context, auth_context, &local_kaddr, NULL);
		if (ret) {
			close(sock);
			SAFE_FREE(ap_req.data);
			krb5_auth_con_free(context, auth_context);
			DEBUG(1,("krb5_auth_con_setaddrs failed (%s)\n", error_message(ret)));
			return ADS_ERROR_KRB5(ret);
		}
	
		ret = build_kpasswd_request(pversion, context, auth_context, &ap_req,
					  princ, newpw, use_tcp, &chpw_req);
		if (ret) {
			close(sock);
			SAFE_FREE(ap_req.data);
			krb5_auth_con_free(context, auth_context);
			DEBUG(1,("build_setpw_request failed (%s)\n", error_message(ret)));
			return ADS_ERROR_KRB5(ret);
		}

		ret = write(sock, chpw_req.data, chpw_req.length); 

		if (ret != chpw_req.length) {
			close(sock);
			SAFE_FREE(chpw_req.data);
			SAFE_FREE(ap_req.data);
			krb5_auth_con_free(context, auth_context);
			DEBUG(1,("send of chpw failed (%s)\n", strerror(errno)));
			return ADS_ERROR_SYSTEM(errno);
		}
	
		SAFE_FREE(chpw_req.data);
	
		chpw_rep.length = 1500;
		chpw_rep.data = (char *) SMB_MALLOC(chpw_rep.length);
		if (!chpw_rep.data) {
			close(sock);
			SAFE_FREE(ap_req.data);
			krb5_auth_con_free(context, auth_context);
			DEBUG(1,("send of chpw failed (%s)\n", strerror(errno)));
			errno = ENOMEM;
			return ADS_ERROR_SYSTEM(errno);
		}
	
		ret = read(sock, chpw_rep.data, chpw_rep.length);
		if (ret < 0) {
			close(sock);
			SAFE_FREE(chpw_rep.data);
			SAFE_FREE(ap_req.data);
			krb5_auth_con_free(context, auth_context);
			DEBUG(1,("recv of chpw reply failed (%s)\n", strerror(errno)));
			return ADS_ERROR_SYSTEM(errno);
		}
	
		close(sock);
		chpw_rep.length = ret;
	
		ret = krb5_auth_con_setaddrs(context, auth_context, NULL,&remote_kaddr);
		if (ret) {
			SAFE_FREE(chpw_rep.data);
			SAFE_FREE(ap_req.data);
			krb5_auth_con_free(context, auth_context);
			DEBUG(1,("krb5_auth_con_setaddrs on reply failed (%s)\n", 
				 error_message(ret)));
			return ADS_ERROR_KRB5(ret);
		}
	
		ret = parse_setpw_reply(context, use_tcp, auth_context, &chpw_rep);
		SAFE_FREE(chpw_rep.data);
	
		if (ret) {
			
			if (ret == KRB5KRB_ERR_RESPONSE_TOO_BIG && !use_tcp) {
				DEBUG(5, ("Trying setpw with TCP!!!\n"));
				use_tcp = True;
				continue;
			}

			SAFE_FREE(ap_req.data);
			krb5_auth_con_free(context, auth_context);
			DEBUG(1,("parse_setpw_reply failed (%s)\n", 
				 error_message(ret)));
			return ADS_ERROR_KRB5(ret);
		}
	
		SAFE_FREE(ap_req.data);
		krb5_auth_con_free(context, auth_context);
	} while ( ret );

	return ADS_SUCCESS;
}

ADS_STATUS ads_krb5_set_password(const char *kdc_host, const char *princ, 
				 const char *newpw, int time_offset)
{

	ADS_STATUS aret;
	krb5_error_code ret = 0;
	krb5_context context = NULL;
	krb5_principal principal = NULL;
	char *princ_name = NULL;
	char *realm = NULL;
	krb5_creds creds, *credsp = NULL;
#if KRB5_PRINC_REALM_RETURNS_REALM
	krb5_realm orig_realm;
#else
	krb5_data orig_realm;
#endif
	krb5_ccache ccache = NULL;

	ZERO_STRUCT(creds);
	
	initialize_krb5_error_table();
	ret = krb5_init_context(&context);
	if (ret) {
		DEBUG(1,("Failed to init krb5 context (%s)\n", error_message(ret)));
		return ADS_ERROR_KRB5(ret);
	}
	
	if (time_offset != 0) {
		krb5_set_real_time(context, time(NULL) + time_offset, 0);
	}

	ret = krb5_cc_default(context, &ccache);
	if (ret) {
	        krb5_free_context(context);
		DEBUG(1,("Failed to get default creds (%s)\n", error_message(ret)));
		return ADS_ERROR_KRB5(ret);
	}

	realm = strchr_m(princ, '@');
	if (!realm) {
		krb5_cc_close(context, ccache);
	        krb5_free_context(context);
		DEBUG(1,("Failed to get realm\n"));
		return ADS_ERROR_KRB5(-1);
	}
	realm++;

	asprintf(&princ_name, "kadmin/changepw@%s", realm);
	ret = smb_krb5_parse_name(context, princ_name, &creds.server);
	if (ret) {
		krb5_cc_close(context, ccache);
                krb5_free_context(context);
		DEBUG(1,("Failed to parse kadmin/changepw (%s)\n", error_message(ret)));
		return ADS_ERROR_KRB5(ret);
	}

	/* parse the principal we got as a function argument */
	ret = smb_krb5_parse_name(context, princ, &principal);
	if (ret) {
		krb5_cc_close(context, ccache);
	        krb5_free_principal(context, creds.server);
                krb5_free_context(context);
		DEBUG(1,("Failed to parse %s (%s)\n", princ_name, error_message(ret)));
		free(princ_name);
		return ADS_ERROR_KRB5(ret);
	}

	free(princ_name);

	/* The creds.server principal takes ownership of this memory.
		Remember to set back to original value before freeing. */
	orig_realm = *krb5_princ_realm(context, creds.server);
	krb5_princ_set_realm(context, creds.server, krb5_princ_realm(context, principal));
	
	ret = krb5_cc_get_principal(context, ccache, &creds.client);
	if (ret) {
		krb5_cc_close(context, ccache);
		krb5_princ_set_realm(context, creds.server, &orig_realm);
	        krb5_free_principal(context, creds.server);
	        krb5_free_principal(context, principal);
                krb5_free_context(context);
		DEBUG(1,("Failed to get principal from ccache (%s)\n", 
			 error_message(ret)));
		return ADS_ERROR_KRB5(ret);
	}
	
	ret = krb5_get_credentials(context, 0, ccache, &creds, &credsp); 
	if (ret) {
		krb5_cc_close(context, ccache);
	        krb5_free_principal(context, creds.client);
		krb5_princ_set_realm(context, creds.server, &orig_realm);
	        krb5_free_principal(context, creds.server);
	        krb5_free_principal(context, principal);
	        krb5_free_context(context);
		DEBUG(1,("krb5_get_credentials failed (%s)\n", error_message(ret)));
		return ADS_ERROR_KRB5(ret);
	}
	
	/* we might have to call krb5_free_creds(...) from now on ... */

	aret = do_krb5_kpasswd_request(context, kdc_host,
				       KRB5_KPASSWD_VERS_SETPW,
				       credsp, princ, newpw);

	krb5_free_creds(context, credsp);
	krb5_free_principal(context, creds.client);
	krb5_princ_set_realm(context, creds.server, &orig_realm);
        krb5_free_principal(context, creds.server);
	krb5_free_principal(context, principal);
	krb5_cc_close(context, ccache);
	krb5_free_context(context);

	return aret;
}

/*
  we use a prompter to avoid a crash bug in the kerberos libs when 
  dealing with empty passwords
  this prompter is just a string copy ...
*/
static krb5_error_code 
kerb_prompter(krb5_context ctx, void *data,
	       const char *name,
	       const char *banner,
	       int num_prompts,
	       krb5_prompt prompts[])
{
	if (num_prompts == 0) return 0;

	memset(prompts[0].reply->data, 0, prompts[0].reply->length);
	if (prompts[0].reply->length > 0) {
		if (data) {
			strncpy(prompts[0].reply->data, data, prompts[0].reply->length-1);
			prompts[0].reply->length = strlen(prompts[0].reply->data);
		} else {
			prompts[0].reply->length = 0;
		}
	}
	return 0;
}

static ADS_STATUS ads_krb5_chg_password(const char *kdc_host,
					const char *principal,
					const char *oldpw, 
					const char *newpw, 
					int time_offset)
{
    ADS_STATUS aret;
    krb5_error_code ret;
    krb5_context context = NULL;
    krb5_principal princ;
    krb5_get_init_creds_opt opts;
    krb5_creds creds;
    char *chpw_princ = NULL, *password;

    initialize_krb5_error_table();
    ret = krb5_init_context(&context);
    if (ret) {
	DEBUG(1,("Failed to init krb5 context (%s)\n", error_message(ret)));
	return ADS_ERROR_KRB5(ret);
    }

    if ((ret = smb_krb5_parse_name(context, principal,
                                    &princ))) {
	krb5_free_context(context);
	DEBUG(1,("Failed to parse %s (%s)\n", principal, error_message(ret)));
	return ADS_ERROR_KRB5(ret);
    }

    krb5_get_init_creds_opt_init(&opts);
    krb5_get_init_creds_opt_set_tkt_life(&opts, 5*60);
    krb5_get_init_creds_opt_set_renew_life(&opts, 0);
    krb5_get_init_creds_opt_set_forwardable(&opts, 0);
    krb5_get_init_creds_opt_set_proxiable(&opts, 0);

    /* We have to obtain an INITIAL changepw ticket for changing password */
    asprintf(&chpw_princ, "kadmin/changepw@%s",
				(char *) krb5_princ_realm(context, princ));
    password = SMB_STRDUP(oldpw);
    ret = krb5_get_init_creds_password(context, &creds, princ, password,
					   kerb_prompter, NULL, 
					   0, chpw_princ, &opts);
    SAFE_FREE(chpw_princ);
    SAFE_FREE(password);

    if (ret) {
      if (ret == KRB5KRB_AP_ERR_BAD_INTEGRITY)
	DEBUG(1,("Password incorrect while getting initial ticket"));
      else
	DEBUG(1,("krb5_get_init_creds_password failed (%s)\n", error_message(ret)));

	krb5_free_principal(context, princ);
	krb5_free_context(context);
	return ADS_ERROR_KRB5(ret);
    }

    aret = do_krb5_kpasswd_request(context, kdc_host,
				   KRB5_KPASSWD_VERS_CHANGEPW,
				   &creds, principal, newpw);

    krb5_free_principal(context, princ);
    krb5_free_context(context);

    return aret;
}


ADS_STATUS kerberos_set_password(const char *kpasswd_server, 
				 const char *auth_principal, const char *auth_password,
				 const char *target_principal, const char *new_password,
				 int time_offset)
{
    int ret;

    if ((ret = kerberos_kinit_password(auth_principal, auth_password, time_offset, NULL))) {
	DEBUG(1,("Failed kinit for principal %s (%s)\n", auth_principal, error_message(ret)));
	return ADS_ERROR_KRB5(ret);
    }

    if (!strcmp(auth_principal, target_principal))
	return ads_krb5_chg_password(kpasswd_server, target_principal,
				     auth_password, new_password, time_offset);
    else
    	return ads_krb5_set_password(kpasswd_server, target_principal,
				     new_password, time_offset);
}


/**
 * Set the machine account password
 * @param ads connection to ads server
 * @param hostname machine whose password is being set
 * @param password new password
 * @return status of password change
 **/
ADS_STATUS ads_set_machine_password(ADS_STRUCT *ads,
				    const char *machine_account,
				    const char *password)
{
	ADS_STATUS status;
	char *principal = NULL; 

	/*
	  we need to use the '$' form of the name here (the machine account name), 
	  as otherwise the server might end up setting the password for a user
	  instead
	 */
	asprintf(&principal, "%s@%s", machine_account, ads->config.realm);
	
	status = ads_krb5_set_password(ads->auth.kdc_server, principal, 
				       password, ads->auth.time_offset);
	
	free(principal);

	return status;
}



#endif
