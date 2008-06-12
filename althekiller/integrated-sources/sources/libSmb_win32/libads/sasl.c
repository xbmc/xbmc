/* 
   Unix SMB/CIFS implementation.
   ads sasl code
   Copyright (C) Andrew Tridgell 2001
   
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

#ifdef HAVE_LDAP

/* 
   perform a LDAP/SASL/SPNEGO/NTLMSSP bind (just how many layers can
   we fit on one socket??)
*/
static ADS_STATUS ads_sasl_spnego_ntlmssp_bind(ADS_STRUCT *ads)
{
	DATA_BLOB msg1 = data_blob(NULL, 0);
	DATA_BLOB blob = data_blob(NULL, 0);
	DATA_BLOB blob_in = data_blob(NULL, 0);
	DATA_BLOB blob_out = data_blob(NULL, 0);
	struct berval cred, *scred = NULL;
	int rc;
	NTSTATUS nt_status;
	int turn = 1;

	struct ntlmssp_state *ntlmssp_state;

	if (!NT_STATUS_IS_OK(nt_status = ntlmssp_client_start(&ntlmssp_state))) {
		return ADS_ERROR_NT(nt_status);
	}
	ntlmssp_state->neg_flags &= ~NTLMSSP_NEGOTIATE_SIGN;

	if (!NT_STATUS_IS_OK(nt_status = ntlmssp_set_username(ntlmssp_state, ads->auth.user_name))) {
		return ADS_ERROR_NT(nt_status);
	}
	if (!NT_STATUS_IS_OK(nt_status = ntlmssp_set_domain(ntlmssp_state, ads->auth.realm))) {
		return ADS_ERROR_NT(nt_status);
	}
	if (!NT_STATUS_IS_OK(nt_status = ntlmssp_set_password(ntlmssp_state, ads->auth.password))) {
		return ADS_ERROR_NT(nt_status);
	}

	blob_in = data_blob(NULL, 0);

	do {
		nt_status = ntlmssp_update(ntlmssp_state, 
					   blob_in, &blob_out);
		data_blob_free(&blob_in);
		if ((NT_STATUS_EQUAL(nt_status, NT_STATUS_MORE_PROCESSING_REQUIRED) 
		     || NT_STATUS_IS_OK(nt_status))
		    && blob_out.length) {
			if (turn == 1) {
				/* and wrap it in a SPNEGO wrapper */
				msg1 = gen_negTokenInit(OID_NTLMSSP, blob_out);
			} else {
				/* wrap it in SPNEGO */
				msg1 = spnego_gen_auth(blob_out);
			}

			data_blob_free(&blob_out);

			cred.bv_val = (char *)msg1.data;
			cred.bv_len = msg1.length;
			scred = NULL;
			rc = ldap_sasl_bind_s(ads->ld, NULL, "GSS-SPNEGO", &cred, NULL, NULL, &scred);
			data_blob_free(&msg1);
			if ((rc != LDAP_SASL_BIND_IN_PROGRESS) && (rc != 0)) {
				if (scred) {
					ber_bvfree(scred);
				}

				ntlmssp_end(&ntlmssp_state);
				return ADS_ERROR(rc);
			}
			if (scred) {
				blob = data_blob(scred->bv_val, scred->bv_len);
				ber_bvfree(scred);
			} else {
				blob = data_blob(NULL, 0);
			}

		} else {

			ntlmssp_end(&ntlmssp_state);
			data_blob_free(&blob_out);
			return ADS_ERROR_NT(nt_status);
		}
		
		if ((turn == 1) && 
		    (rc == LDAP_SASL_BIND_IN_PROGRESS)) {
			DATA_BLOB tmp_blob = data_blob(NULL, 0);
			/* the server might give us back two challenges */
			if (!spnego_parse_challenge(blob, &blob_in, 
						    &tmp_blob)) {

				ntlmssp_end(&ntlmssp_state);
				data_blob_free(&blob);
				DEBUG(3,("Failed to parse challenges\n"));
				return ADS_ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}
			data_blob_free(&tmp_blob);
		} else if (rc == LDAP_SASL_BIND_IN_PROGRESS) {
			if (!spnego_parse_auth_response(blob, nt_status, 
							&blob_in)) {

				ntlmssp_end(&ntlmssp_state);
				data_blob_free(&blob);
				DEBUG(3,("Failed to parse auth response\n"));
				return ADS_ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}
		}
		data_blob_free(&blob);
		data_blob_free(&blob_out);
		turn++;
	} while (rc == LDAP_SASL_BIND_IN_PROGRESS && !NT_STATUS_IS_OK(nt_status));
	
	/* we have a reference conter on ntlmssp_state, if we are signing
	   then the state will be kept by the signing engine */

	ntlmssp_end(&ntlmssp_state);

	return ADS_ERROR(rc);
}

/* 
   perform a LDAP/SASL/SPNEGO/KRB5 bind
*/
static ADS_STATUS ads_sasl_spnego_krb5_bind(ADS_STRUCT *ads, const char *principal)
{
	DATA_BLOB blob = data_blob(NULL, 0);
	struct berval cred, *scred = NULL;
	DATA_BLOB session_key = data_blob(NULL, 0);
	int rc;

	rc = spnego_gen_negTokenTarg(principal, ads->auth.time_offset, &blob, &session_key, 0);

	if (rc) {
		return ADS_ERROR_KRB5(rc);
	}

	/* now send the auth packet and we should be done */
	cred.bv_val = (char *)blob.data;
	cred.bv_len = blob.length;

	rc = ldap_sasl_bind_s(ads->ld, NULL, "GSS-SPNEGO", &cred, NULL, NULL, &scred);

	data_blob_free(&blob);
	data_blob_free(&session_key);
	if(scred)
		ber_bvfree(scred);

	return ADS_ERROR(rc);
}

/* 
   this performs a SASL/SPNEGO bind
*/
static ADS_STATUS ads_sasl_spnego_bind(ADS_STRUCT *ads)
{
	struct berval *scred=NULL;
	int rc, i;
	ADS_STATUS status;
	DATA_BLOB blob;
	char *principal = NULL;
	char *OIDs[ASN1_MAX_OIDS];
#ifdef HAVE_KRB5
	BOOL got_kerberos_mechanism = False;
#endif

	rc = ldap_sasl_bind_s(ads->ld, NULL, "GSS-SPNEGO", NULL, NULL, NULL, &scred);

	if (rc != LDAP_SASL_BIND_IN_PROGRESS) {
		status = ADS_ERROR(rc);
		goto failed;
	}

	blob = data_blob(scred->bv_val, scred->bv_len);

	ber_bvfree(scred);

#if 0
	file_save("sasl_spnego.dat", blob.data, blob.length);
#endif

	/* the server sent us the first part of the SPNEGO exchange in the negprot 
	   reply */
	if (!spnego_parse_negTokenInit(blob, OIDs, &principal)) {
		data_blob_free(&blob);
		status = ADS_ERROR(LDAP_OPERATIONS_ERROR);
		goto failed;
	}
	data_blob_free(&blob);

	/* make sure the server understands kerberos */
	for (i=0;OIDs[i];i++) {
		DEBUG(3,("ads_sasl_spnego_bind: got OID=%s\n", OIDs[i]));
#ifdef HAVE_KRB5
		if (strcmp(OIDs[i], OID_KERBEROS5_OLD) == 0 ||
		    strcmp(OIDs[i], OID_KERBEROS5) == 0) {
			got_kerberos_mechanism = True;
		}
#endif
		free(OIDs[i]);
	}
	DEBUG(3,("ads_sasl_spnego_bind: got server principal name =%s\n", principal));

#ifdef HAVE_KRB5
	if (!(ads->auth.flags & ADS_AUTH_DISABLE_KERBEROS) &&
	    got_kerberos_mechanism) {
		status = ads_sasl_spnego_krb5_bind(ads, principal);
		if (ADS_ERR_OK(status)) {
			SAFE_FREE(principal);
			return status;
		}

		status = ADS_ERROR_KRB5(ads_kinit_password(ads)); 

		if (ADS_ERR_OK(status)) {
			status = ads_sasl_spnego_krb5_bind(ads, principal);
		}

		/* only fallback to NTLMSSP if allowed */
		if (ADS_ERR_OK(status) || 
		    !(ads->auth.flags & ADS_AUTH_ALLOW_NTLMSSP)) {
			SAFE_FREE(principal);
			return status;
		}
	}
#endif

	SAFE_FREE(principal);

	/* lets do NTLMSSP ... this has the big advantage that we don't need
	   to sync clocks, and we don't rely on special versions of the krb5 
	   library for HMAC_MD4 encryption */
	return ads_sasl_spnego_ntlmssp_bind(ads);

failed:
	return status;
}

#ifdef HAVE_GSSAPI
#define MAX_GSS_PASSES 3

/* this performs a SASL/gssapi bind
   we avoid using cyrus-sasl to make Samba more robust. cyrus-sasl
   is very dependent on correctly configured DNS whereas
   this routine is much less fragile
   see RFC2078 and RFC2222 for details
*/
static ADS_STATUS ads_sasl_gssapi_bind(ADS_STRUCT *ads)
{
	uint32 minor_status;
	gss_name_t serv_name;
	gss_buffer_desc input_name;
	gss_ctx_id_t context_handle;
	gss_OID mech_type = GSS_C_NULL_OID;
	gss_buffer_desc output_token, input_token;
	uint32 ret_flags, conf_state;
	struct berval cred;
	struct berval *scred = NULL;
	int i=0;
	int gss_rc, rc;
	uint8 *p;
	uint32 max_msg_size = 0;
	char *sname;
	ADS_STATUS status;
	krb5_principal principal;
	krb5_context ctx = NULL;
	krb5_enctype enc_types[] = {
#ifdef ENCTYPE_ARCFOUR_HMAC
			ENCTYPE_ARCFOUR_HMAC,
#endif
			ENCTYPE_DES_CBC_MD5,
			ENCTYPE_NULL};
	gss_OID_desc nt_principal = 
	{10, CONST_DISCARD(char *, "\052\206\110\206\367\022\001\002\002\002")};

	/* we need to fetch a service ticket as the ldap user in the
	   servers realm, regardless of our realm */
	asprintf(&sname, "ldap/%s@%s", ads->config.ldap_server_name, ads->config.realm);

	initialize_krb5_error_table();
	status = ADS_ERROR_KRB5(krb5_init_context(&ctx));
	if (!ADS_ERR_OK(status)) {
		return status;
	}
	status = ADS_ERROR_KRB5(krb5_set_default_tgs_ktypes(ctx, enc_types));
	if (!ADS_ERR_OK(status)) {
		return status;
	}
	status = ADS_ERROR_KRB5(smb_krb5_parse_name(ctx, sname, &principal));
	if (!ADS_ERR_OK(status)) {
		return status;
	}

	free(sname);
	krb5_free_context(ctx);	

	input_name.value = &principal;
	input_name.length = sizeof(principal);

	gss_rc = gss_import_name(&minor_status, &input_name, &nt_principal, &serv_name);
	if (gss_rc) {
		return ADS_ERROR_GSS(gss_rc, minor_status);
	}

	context_handle = GSS_C_NO_CONTEXT;

	input_token.value = NULL;
	input_token.length = 0;

	for (i=0; i < MAX_GSS_PASSES; i++) {
		gss_rc = gss_init_sec_context(&minor_status,
					  GSS_C_NO_CREDENTIAL,
					  &context_handle,
					  serv_name,
					  mech_type,
					  GSS_C_MUTUAL_FLAG | GSS_C_REPLAY_FLAG,
					  0,
					  NULL,
					  &input_token,
					  NULL,
					  &output_token,
					  &ret_flags,
					  NULL);

		if (input_token.value) {
			gss_release_buffer(&minor_status, &input_token);
		}

		if (gss_rc && gss_rc != GSS_S_CONTINUE_NEEDED) {
			status = ADS_ERROR_GSS(gss_rc, minor_status);
			goto failed;
		}

		cred.bv_val = output_token.value;
		cred.bv_len = output_token.length;

		rc = ldap_sasl_bind_s(ads->ld, NULL, "GSSAPI", &cred, NULL, NULL, 
				      &scred);
		if (rc != LDAP_SASL_BIND_IN_PROGRESS) {
			status = ADS_ERROR(rc);
			goto failed;
		}

		if (output_token.value) {
			gss_release_buffer(&minor_status, &output_token);
		}

		if (scred) {
			input_token.value = scred->bv_val;
			input_token.length = scred->bv_len;
		} else {
			input_token.value = NULL;
			input_token.length = 0;
		}

		if (gss_rc == 0) break;
	}

	gss_release_name(&minor_status, &serv_name);

	gss_rc = gss_unwrap(&minor_status,context_handle,&input_token,&output_token,
			    (int *)&conf_state,NULL);
	if (gss_rc) {
		status = ADS_ERROR_GSS(gss_rc, minor_status);
		goto failed;
	}

	gss_release_buffer(&minor_status, &input_token);

	p = (uint8 *)output_token.value;

#if 0
	file_save("sasl_gssapi.dat", output_token.value, output_token.length);
#endif
	if (p) {
		max_msg_size = (p[1]<<16) | (p[2]<<8) | p[3];
	}

	gss_release_buffer(&minor_status, &output_token);

	output_token.value = SMB_MALLOC(strlen(ads->config.bind_path) + 8);
	p = output_token.value;

	*p++ = 1; /* no sign & seal selection */
	/* choose the same size as the server gave us */
	*p++ = max_msg_size>>16;
	*p++ = max_msg_size>>8;
	*p++ = max_msg_size;
	snprintf((char *)p, strlen(ads->config.bind_path)+4, "dn:%s", ads->config.bind_path);
	p += strlen((const char *)p);

	output_token.length = PTR_DIFF(p, output_token.value);

	gss_rc = gss_wrap(&minor_status, context_handle,0,GSS_C_QOP_DEFAULT,
			  &output_token, (int *)&conf_state,
			  &input_token);
	if (gss_rc) {
		status = ADS_ERROR_GSS(gss_rc, minor_status);
		goto failed;
	}

	free(output_token.value);

	cred.bv_val = input_token.value;
	cred.bv_len = input_token.length;

	rc = ldap_sasl_bind_s(ads->ld, NULL, "GSSAPI", &cred, NULL, NULL, 
			      &scred);
	status = ADS_ERROR(rc);

	gss_release_buffer(&minor_status, &input_token);

failed:
	if(scred)
		ber_bvfree(scred);
	return status;
}
#endif /* HAVE_GGSAPI */

/* mapping between SASL mechanisms and functions */
static struct {
	const char *name;
	ADS_STATUS (*fn)(ADS_STRUCT *);
} sasl_mechanisms[] = {
	{"GSS-SPNEGO", ads_sasl_spnego_bind},
#ifdef HAVE_GSSAPI
	{"GSSAPI", ads_sasl_gssapi_bind}, /* doesn't work with .NET RC1. No idea why */
#endif
	{NULL, NULL}
};

ADS_STATUS ads_sasl_bind(ADS_STRUCT *ads)
{
	const char *attrs[] = {"supportedSASLMechanisms", NULL};
	char **values;
	ADS_STATUS status;
	int i, j;
	void *res;

	/* get a list of supported SASL mechanisms */
	status = ads_do_search(ads, "", LDAP_SCOPE_BASE, "(objectclass=*)", attrs, &res);
	if (!ADS_ERR_OK(status)) return status;

	values = ldap_get_values(ads->ld, res, "supportedSASLMechanisms");

	/* try our supported mechanisms in order */
	for (i=0;sasl_mechanisms[i].name;i++) {
		/* see if the server supports it */
		for (j=0;values && values[j];j++) {
			if (strcmp(values[j], sasl_mechanisms[i].name) == 0) {
				DEBUG(4,("Found SASL mechanism %s\n", values[j]));
				status = sasl_mechanisms[i].fn(ads);
				ldap_value_free(values);
				ldap_msgfree(res);
				return status;
			}
		}
	}

	ldap_value_free(values);
	ldap_msgfree(res);
	return ADS_ERROR(LDAP_AUTH_METHOD_NOT_SUPPORTED);
}

#endif /* HAVE_LDAP */

