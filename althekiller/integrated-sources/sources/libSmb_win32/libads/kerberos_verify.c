/* 
   Unix SMB/CIFS implementation.
   kerberos utility library
   Copyright (C) Andrew Tridgell 2001
   Copyright (C) Remus Koos 2001
   Copyright (C) Luke Howard 2003   
   Copyright (C) Guenther Deschner 2003, 2005
   Copyright (C) Jim McDonough (jmcd@us.ibm.com) 2003
   Copyright (C) Andrew Bartlett <abartlet@samba.org> 2004-2005
   
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

#ifdef HAVE_KRB5

#if !defined(HAVE_KRB5_PRINC_COMPONENT)
const krb5_data *krb5_princ_component(krb5_context, krb5_principal, int );
#endif

/**********************************************************************************
 Try to verify a ticket using the system keytab... the system keytab has kvno -1 entries, so
 it's more like what microsoft does... see comment in utils/net_ads.c in the
 ads_keytab_add_entry function for details.
***********************************************************************************/

static BOOL ads_keytab_verify_ticket(krb5_context context, krb5_auth_context auth_context,
			const DATA_BLOB *ticket, krb5_data *p_packet, krb5_ticket **pp_tkt, 
			krb5_keyblock **keyblock)
{
	krb5_error_code ret = 0;
	BOOL auth_ok = False;
	krb5_keytab keytab = NULL;
	krb5_kt_cursor kt_cursor;
	krb5_keytab_entry kt_entry;
	char *valid_princ_formats[7] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	char *entry_princ_s = NULL;
	fstring my_name, my_fqdn;
	int i;
	int number_matched_principals = 0;

	/* Generate the list of principal names which we expect
	 * clients might want to use for authenticating to the file
	 * service.  We allow name$,{host,cifs}/{name,fqdn,name.REALM}. */

	fstrcpy(my_name, global_myname());

	my_fqdn[0] = '\0';
	name_to_fqdn(my_fqdn, global_myname());

	asprintf(&valid_princ_formats[0], "%s$@%s", my_name, lp_realm());
	asprintf(&valid_princ_formats[1], "host/%s@%s", my_name, lp_realm());
	asprintf(&valid_princ_formats[2], "host/%s@%s", my_fqdn, lp_realm());
	asprintf(&valid_princ_formats[3], "host/%s.%s@%s", my_name, lp_realm(), lp_realm());
	asprintf(&valid_princ_formats[4], "cifs/%s@%s", my_name, lp_realm());
	asprintf(&valid_princ_formats[5], "cifs/%s@%s", my_fqdn, lp_realm());
	asprintf(&valid_princ_formats[6], "cifs/%s.%s@%s", my_name, lp_realm(), lp_realm());

	ZERO_STRUCT(kt_entry);
	ZERO_STRUCT(kt_cursor);

	ret = krb5_kt_default(context, &keytab);
	if (ret) {
		DEBUG(1, ("ads_keytab_verify_ticket: krb5_kt_default failed (%s)\n", error_message(ret)));
		goto out;
	}

	/* Iterate through the keytab.  For each key, if the principal
	 * name case-insensitively matches one of the allowed formats,
	 * try verifying the ticket using that principal. */

	ret = krb5_kt_start_seq_get(context, keytab, &kt_cursor);
	if (ret) {
		DEBUG(1, ("ads_keytab_verify_ticket: krb5_kt_start_seq_get failed (%s)\n", error_message(ret)));
		goto out;
	}
  
	if (ret != KRB5_KT_END && ret != ENOENT ) {
		while (!auth_ok && (krb5_kt_next_entry(context, keytab, &kt_entry, &kt_cursor) == 0)) {
			ret = smb_krb5_unparse_name(context, kt_entry.principal, &entry_princ_s);
			if (ret) {
				DEBUG(1, ("ads_keytab_verify_ticket: smb_krb5_unparse_name failed (%s)\n",
					error_message(ret)));
				goto out;
			}

			for (i = 0; i < sizeof(valid_princ_formats) / sizeof(valid_princ_formats[0]); i++) {
				if (strequal(entry_princ_s, valid_princ_formats[i])) {
					number_matched_principals++;
					p_packet->length = ticket->length;
					p_packet->data = (krb5_pointer)ticket->data;
					*pp_tkt = NULL;

					ret = krb5_rd_req_return_keyblock_from_keytab(context, &auth_context, p_packet,
									  	      kt_entry.principal, keytab,
										      NULL, pp_tkt, keyblock);

					if (ret) {
						DEBUG(10,("ads_keytab_verify_ticket: "
							"krb5_rd_req_return_keyblock_from_keytab(%s) failed: %s\n",
							entry_princ_s, error_message(ret)));

						/* workaround for MIT: 
						 * as krb5_ktfile_get_entry will
						 * explicitly close the
						 * krb5_keytab as soon as
						 * krb5_rd_req has sucessfully
						 * decrypted the ticket but the
						 * ticket is not valid yet (due
						 * to clockskew) there is no
						 * point in querying more
						 * keytab entries - Guenther */
						
						if (ret == KRB5KRB_AP_ERR_TKT_NYV || 
						    ret == KRB5KRB_AP_ERR_TKT_EXPIRED) {
							break;
						}
					} else {
						DEBUG(3,("ads_keytab_verify_ticket: "
							"krb5_rd_req_return_keyblock_from_keytab succeeded for principal %s\n",
							entry_princ_s));
						auth_ok = True;
						break;
					}
				}
			}

			/* Free the name we parsed. */
			SAFE_FREE(entry_princ_s);

			/* Free the entry we just read. */
			smb_krb5_kt_free_entry(context, &kt_entry);
			ZERO_STRUCT(kt_entry);
		}
		krb5_kt_end_seq_get(context, keytab, &kt_cursor);
	}

	ZERO_STRUCT(kt_cursor);

  out:
	
	for (i = 0; i < sizeof(valid_princ_formats) / sizeof(valid_princ_formats[0]); i++) {
		SAFE_FREE(valid_princ_formats[i]);
	}
	
	if (!auth_ok) {
		if (!number_matched_principals) {
			DEBUG(3, ("ads_keytab_verify_ticket: no keytab principals matched expected file service name.\n"));
		} else {
			DEBUG(3, ("ads_keytab_verify_ticket: krb5_rd_req failed for all %d matched keytab principals\n",
				number_matched_principals));
		}
	}

	SAFE_FREE(entry_princ_s);

	{
		krb5_keytab_entry zero_kt_entry;
		ZERO_STRUCT(zero_kt_entry);
		if (memcmp(&zero_kt_entry, &kt_entry, sizeof(krb5_keytab_entry))) {
			smb_krb5_kt_free_entry(context, &kt_entry);
		}
	}

	{
		krb5_kt_cursor zero_csr;
		ZERO_STRUCT(zero_csr);
		if ((memcmp(&kt_cursor, &zero_csr, sizeof(krb5_kt_cursor)) != 0) && keytab) {
			krb5_kt_end_seq_get(context, keytab, &kt_cursor);
		}
	}

	if (keytab) {
		krb5_kt_close(context, keytab);
	}
	return auth_ok;
}

/**********************************************************************************
 Try to verify a ticket using the secrets.tdb.
***********************************************************************************/

static BOOL ads_secrets_verify_ticket(krb5_context context, krb5_auth_context auth_context,
				      krb5_principal host_princ,
				      const DATA_BLOB *ticket, krb5_data *p_packet, krb5_ticket **pp_tkt,
				      krb5_keyblock **keyblock)
{
	krb5_error_code ret = 0;
	BOOL auth_ok = False;
	char *password_s = NULL;
	krb5_data password;
	krb5_enctype enctypes[4] = { ENCTYPE_DES_CBC_CRC, ENCTYPE_DES_CBC_MD5, 0, 0 };
	int i;

#if defined(ENCTYPE_ARCFOUR_HMAC)
	enctypes[2] = ENCTYPE_ARCFOUR_HMAC;
#endif

	ZERO_STRUCTP(keyblock);

	if (!secrets_init()) {
		DEBUG(1,("ads_secrets_verify_ticket: secrets_init failed\n"));
		return False;
	}

	password_s = secrets_fetch_machine_password(lp_workgroup(), NULL, NULL);
	if (!password_s) {
		DEBUG(1,("ads_secrets_verify_ticket: failed to fetch machine password\n"));
		return False;
	}

	password.data = password_s;
	password.length = strlen(password_s);

	/* CIFS doesn't use addresses in tickets. This would break NAT. JRA */

	p_packet->length = ticket->length;
	p_packet->data = (krb5_pointer)ticket->data;

	/* We need to setup a auth context with each possible encoding type in turn. */
	for (i=0;enctypes[i];i++) {
		krb5_keyblock *key = NULL;

		if (!(key = SMB_MALLOC_P(krb5_keyblock))) {
			goto out;
		}
	
		if (create_kerberos_key_from_string(context, host_princ, &password, key, enctypes[i])) {
			SAFE_FREE(key);
			continue;
		}

		krb5_auth_con_setuseruserkey(context, auth_context, key);

		if (!(ret = krb5_rd_req(context, &auth_context, p_packet, 
					NULL,
					NULL, NULL, pp_tkt))) {
			DEBUG(10,("ads_secrets_verify_ticket: enc type [%u] decrypted message !\n",
				(unsigned int)enctypes[i] ));
			auth_ok = True;
			krb5_copy_keyblock(context, key, keyblock);
			krb5_free_keyblock(context, key);
			break;
		}

		DEBUG((ret != KRB5_BAD_ENCTYPE) ? 3 : 10,
				("ads_secrets_verify_ticket: enc type [%u] failed to decrypt with error %s\n",
				(unsigned int)enctypes[i], error_message(ret)));

		/* successfully decrypted but ticket is just not valid at the moment */
		if (ret == KRB5KRB_AP_ERR_TKT_NYV || 
		    ret == KRB5KRB_AP_ERR_TKT_EXPIRED) {
			break;
		}

		krb5_free_keyblock(context, key);

	}

 out:
	SAFE_FREE(password_s);

	return auth_ok;
}

/**********************************************************************************
 Verify an incoming ticket and parse out the principal name and 
 authorization_data if available.
***********************************************************************************/

NTSTATUS ads_verify_ticket(TALLOC_CTX *mem_ctx,
			   const char *realm, time_t time_offset,
			   const DATA_BLOB *ticket, 
			   char **principal, PAC_DATA **pac_data,
			   DATA_BLOB *ap_rep,
			   DATA_BLOB *session_key)
{
	NTSTATUS sret = NT_STATUS_LOGON_FAILURE;
	NTSTATUS pac_ret;
	DATA_BLOB auth_data;
	krb5_context context = NULL;
	krb5_auth_context auth_context = NULL;
	krb5_data packet;
	krb5_ticket *tkt = NULL;
	krb5_rcache rcache = NULL;
	krb5_keyblock *keyblock = NULL;
	time_t authtime;
	int ret;

	krb5_principal host_princ = NULL;
	krb5_const_principal client_principal = NULL;
	char *host_princ_s = NULL;
	BOOL got_replay_mutex = False;

	BOOL auth_ok = False;
	BOOL got_auth_data = False;

	ZERO_STRUCT(packet);
	ZERO_STRUCT(auth_data);
	ZERO_STRUCTP(ap_rep);
	ZERO_STRUCTP(session_key);

	initialize_krb5_error_table();
	ret = krb5_init_context(&context);
	if (ret) {
		DEBUG(1,("ads_verify_ticket: krb5_init_context failed (%s)\n", error_message(ret)));
		return NT_STATUS_LOGON_FAILURE;
	}

	if (time_offset != 0) {
		krb5_set_real_time(context, time(NULL) + time_offset, 0);
	}

	ret = krb5_set_default_realm(context, realm);
	if (ret) {
		DEBUG(1,("ads_verify_ticket: krb5_set_default_realm failed (%s)\n", error_message(ret)));
		goto out;
	}

	/* This whole process is far more complex than I would
           like. We have to go through all this to allow us to store
           the secret internally, instead of using /etc/krb5.keytab */

	ret = krb5_auth_con_init(context, &auth_context);
	if (ret) {
		DEBUG(1,("ads_verify_ticket: krb5_auth_con_init failed (%s)\n", error_message(ret)));
		goto out;
	}

	asprintf(&host_princ_s, "%s$", global_myname());
	strlower_m(host_princ_s);
	ret = smb_krb5_parse_name(context, host_princ_s, &host_princ);
	if (ret) {
		DEBUG(1,("ads_verify_ticket: smb_krb5_parse_name(%s) failed (%s)\n",
					host_princ_s, error_message(ret)));
		goto out;
	}


	/* Lock a mutex surrounding the replay as there is no locking in the MIT krb5
	 * code surrounding the replay cache... */

	if (!grab_server_mutex("replay cache mutex")) {
		DEBUG(1,("ads_verify_ticket: unable to protect replay cache with mutex.\n"));
		goto out;
	}

	got_replay_mutex = True;

	/*
	 * JRA. We must set the rcache here. This will prevent replay attacks.
	 */

	ret = krb5_get_server_rcache(context, krb5_princ_component(context, host_princ, 0), &rcache);
	if (ret) {
		DEBUG(1,("ads_verify_ticket: krb5_get_server_rcache failed (%s)\n", error_message(ret)));
		goto out;
	}

	ret = krb5_auth_con_setrcache(context, auth_context, rcache);
	if (ret) {
		DEBUG(1,("ads_verify_ticket: krb5_auth_con_setrcache failed (%s)\n", error_message(ret)));
		goto out;
	}

	if (lp_use_kerberos_keytab()) {
		auth_ok = ads_keytab_verify_ticket(context, auth_context, ticket, &packet, &tkt, &keyblock);
	}
	if (!auth_ok) {
		auth_ok = ads_secrets_verify_ticket(context, auth_context, host_princ,
						    ticket, &packet, &tkt, &keyblock);
	}

	release_server_mutex();
	got_replay_mutex = False;

#if 0
	/* Heimdal leaks here, if we fix the leak, MIT crashes */
	if (rcache) {
		krb5_rc_close(context, rcache);
	}
#endif

	if (!auth_ok) {
		DEBUG(3,("ads_verify_ticket: krb5_rd_req with auth failed (%s)\n", 
			 error_message(ret)));
		goto out;
	} 
	
	authtime = get_authtime_from_tkt(tkt);
	client_principal = get_principal_from_tkt(tkt);

	ret = krb5_mk_rep(context, auth_context, &packet);
	if (ret) {
		DEBUG(3,("ads_verify_ticket: Failed to generate mutual authentication reply (%s)\n",
			error_message(ret)));
		goto out;
	}

	*ap_rep = data_blob(packet.data, packet.length);
	SAFE_FREE(packet.data);
	packet.length = 0;

	get_krb5_smb_session_key(context, auth_context, session_key, True);
	dump_data_pw("SMB session key (from ticket)\n", session_key->data, session_key->length);

#if 0
	file_save("/tmp/ticket.dat", ticket->data, ticket->length);
#endif

	/* continue when no PAC is retrieved or we couldn't decode the PAC 
	   (like accounts that have the UF_NO_AUTH_DATA_REQUIRED flag set, or
	   Kerberos tickets encrypted using a DES key) - Guenther */

	got_auth_data = get_auth_data_from_tkt(mem_ctx, &auth_data, tkt);
	if (!got_auth_data) {
		DEBUG(3,("ads_verify_ticket: did not retrieve auth data. continuing without PAC\n"));
	}

	if (got_auth_data && pac_data != NULL) {

		pac_ret = decode_pac_data(mem_ctx, &auth_data, context, keyblock, client_principal, authtime, pac_data);
		if (!NT_STATUS_IS_OK(pac_ret)) {
			DEBUG(3,("ads_verify_ticket: failed to decode PAC_DATA: %s\n", nt_errstr(pac_ret)));
			*pac_data = NULL;
		}
		data_blob_free(&auth_data);
	}

#if 0
#if defined(HAVE_KRB5_TKT_ENC_PART2)
	/* MIT */
	if (tkt->enc_part2) {
		file_save("/tmp/authdata.dat",
			  tkt->enc_part2->authorization_data[0]->contents,
			  tkt->enc_part2->authorization_data[0]->length);
	}
#else
	/* Heimdal */
	if (tkt->ticket.authorization_data) {
		file_save("/tmp/authdata.dat",
			  tkt->ticket.authorization_data->val->ad_data.data,
			  tkt->ticket.authorization_data->val->ad_data.length);
	}
#endif
#endif

	if ((ret = smb_krb5_unparse_name(context, client_principal, principal))) {
		DEBUG(3,("ads_verify_ticket: smb_krb5_unparse_name failed (%s)\n", 
			 error_message(ret)));
		sret = NT_STATUS_LOGON_FAILURE;
		goto out;
	}

	sret = NT_STATUS_OK;

 out:

	if (got_replay_mutex) {
		release_server_mutex();
	}

	if (!NT_STATUS_IS_OK(sret)) {
		data_blob_free(&auth_data);
	}

	if (!NT_STATUS_IS_OK(sret)) {
		data_blob_free(ap_rep);
	}

	if (host_princ) {
		krb5_free_principal(context, host_princ);
	}

	if (keyblock) {
		krb5_free_keyblock(context, keyblock);
	}

	if (tkt != NULL) {
		krb5_free_ticket(context, tkt);
	}

	SAFE_FREE(host_princ_s);

	if (auth_context) {
		krb5_auth_con_free(context, auth_context);
	}

	if (context) {
		krb5_free_context(context);
	}

	return sret;
}

#endif /* HAVE_KRB5 */
