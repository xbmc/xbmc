/* 
   Unix SMB/CIFS implementation.
   client connect/disconnect routines
   Copyright (C) Andrew Tridgell 1994-1998
   Copyright (C) Andrew Bartlett 2001-2003
   
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

extern pstring user_socket_options;

static const struct {
	int prot;
	const char *name;
} prots[] = {
	{PROTOCOL_CORE,"PC NETWORK PROGRAM 1.0"},
	{PROTOCOL_COREPLUS,"MICROSOFT NETWORKS 1.03"},
	{PROTOCOL_LANMAN1,"MICROSOFT NETWORKS 3.0"},
	{PROTOCOL_LANMAN1,"LANMAN1.0"},
	{PROTOCOL_LANMAN2,"LM1.2X002"},
	{PROTOCOL_LANMAN2,"DOS LANMAN2.1"},
	{PROTOCOL_LANMAN2,"Samba"},
	{PROTOCOL_NT1,"NT LANMAN 1.0"},
	{PROTOCOL_NT1,"NT LM 0.12"},
	{-1,NULL}
};

/**
 * Set the user session key for a connection
 * @param cli The cli structure to add it too
 * @param session_key The session key used.  (A copy of this is taken for the cli struct)
 *
 */

static void cli_set_session_key (struct cli_state *cli, const DATA_BLOB session_key) 
{
	cli->user_session_key = data_blob(session_key.data, session_key.length);
}

/****************************************************************************
 Do an old lanman2 style session setup.
****************************************************************************/

static BOOL cli_session_setup_lanman2(struct cli_state *cli, const char *user, 
				      const char *pass, size_t passlen, const char *workgroup)
{
	DATA_BLOB session_key = data_blob(NULL, 0);
	DATA_BLOB lm_response = data_blob(NULL, 0);
	fstring pword;
	char *p;

	if (passlen > sizeof(pword)-1)
		return False;

	/* LANMAN servers predate NT status codes and Unicode and ignore those 
	   smb flags so we must disable the corresponding default capabilities  
	   that would otherwise cause the Unicode and NT Status flags to be
	   set (and even returned by the server) */

	cli->capabilities &= ~(CAP_UNICODE | CAP_STATUS32);

	/* if in share level security then don't send a password now */
	if (!(cli->sec_mode & NEGOTIATE_SECURITY_USER_LEVEL))
		passlen = 0;

	if (passlen > 0 && (cli->sec_mode & NEGOTIATE_SECURITY_CHALLENGE_RESPONSE) && passlen != 24) {
		/* Encrypted mode needed, and non encrypted password supplied. */
		lm_response = data_blob(NULL, 24);
		if (!SMBencrypt(pass, cli->secblob.data,(uchar *)lm_response.data)) {
			DEBUG(1, ("Password is > 14 chars in length, and is therefore incompatible with Lanman authentication\n"));
			return False;
		}
	} else if ((cli->sec_mode & NEGOTIATE_SECURITY_CHALLENGE_RESPONSE) && passlen == 24) {
		/* Encrypted mode needed, and encrypted password supplied. */
		lm_response = data_blob(pass, passlen);
	} else if (passlen > 0) {
		/* Plaintext mode needed, assume plaintext supplied. */
		passlen = clistr_push(cli, pword, pass, sizeof(pword), STR_TERMINATE);
		lm_response = data_blob(pass, passlen);
	}

	/* send a session setup command */
	memset(cli->outbuf,'\0',smb_size);
	set_message(cli->outbuf,10, 0, True);
	SCVAL(cli->outbuf,smb_com,SMBsesssetupX);
	cli_setup_packet(cli);
	
	SCVAL(cli->outbuf,smb_vwv0,0xFF);
	SSVAL(cli->outbuf,smb_vwv2,cli->max_xmit);
	SSVAL(cli->outbuf,smb_vwv3,2);
	SSVAL(cli->outbuf,smb_vwv4,1);
	SIVAL(cli->outbuf,smb_vwv5,cli->sesskey);
	SSVAL(cli->outbuf,smb_vwv7,lm_response.length);

	p = smb_buf(cli->outbuf);
	memcpy(p,lm_response.data,lm_response.length);
	p += lm_response.length;
	p += clistr_push(cli, p, user, -1, STR_TERMINATE|STR_UPPER);
	p += clistr_push(cli, p, workgroup, -1, STR_TERMINATE|STR_UPPER);
	p += clistr_push(cli, p, "Unix", -1, STR_TERMINATE);
	p += clistr_push(cli, p, "Samba", -1, STR_TERMINATE);
	cli_setup_bcc(cli, p);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli))
		return False;

	show_msg(cli->inbuf);

	if (cli_is_error(cli))
		return False;
	
	/* use the returned vuid from now on */
	cli->vuid = SVAL(cli->inbuf,smb_uid);	
	fstrcpy(cli->user_name, user);

	if (session_key.data) {
		/* Have plaintext orginal */
		cli_set_session_key(cli, session_key);
	}

	return True;
}

/****************************************************************************
 Work out suitable capabilities to offer the server.
****************************************************************************/

static uint32 cli_session_setup_capabilities(struct cli_state *cli)
{
	uint32 capabilities = CAP_NT_SMBS;

	if (!cli->force_dos_errors)
		capabilities |= CAP_STATUS32;

	if (cli->use_level_II_oplocks)
		capabilities |= CAP_LEVEL_II_OPLOCKS;

	capabilities |= (cli->capabilities & (CAP_UNICODE|CAP_LARGE_FILES|CAP_LARGE_READX|CAP_LARGE_WRITEX|CAP_DFS));
	return capabilities;
}

/****************************************************************************
 Do a NT1 guest session setup.
****************************************************************************/

static BOOL cli_session_setup_guest(struct cli_state *cli)
{
	char *p;
	uint32 capabilities = cli_session_setup_capabilities(cli);

	memset(cli->outbuf, '\0', smb_size);
	set_message(cli->outbuf,13,0,True);
	SCVAL(cli->outbuf,smb_com,SMBsesssetupX);
	cli_setup_packet(cli);
			
	SCVAL(cli->outbuf,smb_vwv0,0xFF);
	SSVAL(cli->outbuf,smb_vwv2,CLI_BUFFER_SIZE);
	SSVAL(cli->outbuf,smb_vwv3,2);
	SSVAL(cli->outbuf,smb_vwv4,cli->pid);
	SIVAL(cli->outbuf,smb_vwv5,cli->sesskey);
	SSVAL(cli->outbuf,smb_vwv7,0);
	SSVAL(cli->outbuf,smb_vwv8,0);
	SIVAL(cli->outbuf,smb_vwv11,capabilities); 
	p = smb_buf(cli->outbuf);
	p += clistr_push(cli, p, "", -1, STR_TERMINATE); /* username */
	p += clistr_push(cli, p, "", -1, STR_TERMINATE); /* workgroup */
	p += clistr_push(cli, p, "Unix", -1, STR_TERMINATE);
	p += clistr_push(cli, p, "Samba", -1, STR_TERMINATE);
	cli_setup_bcc(cli, p);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli))
	      return False;
	
	show_msg(cli->inbuf);
	
	if (cli_is_error(cli))
		return False;

	cli->vuid = SVAL(cli->inbuf,smb_uid);

	p = smb_buf(cli->inbuf);
	p += clistr_pull(cli, cli->server_os, p, sizeof(fstring), -1, STR_TERMINATE);
	p += clistr_pull(cli, cli->server_type, p, sizeof(fstring), -1, STR_TERMINATE);
	p += clistr_pull(cli, cli->server_domain, p, sizeof(fstring), -1, STR_TERMINATE);

	if (strstr(cli->server_type, "Samba")) {
		cli->is_samba = True;
	}

	fstrcpy(cli->user_name, "");

	return True;
}

/****************************************************************************
 Do a NT1 plaintext session setup.
****************************************************************************/

static BOOL cli_session_setup_plaintext(struct cli_state *cli, const char *user, 
					const char *pass, const char *workgroup)
{
	uint32 capabilities = cli_session_setup_capabilities(cli);
	char *p;
	fstring lanman;
	
	fstr_sprintf( lanman, "Samba %s", SAMBA_VERSION_STRING);

	memset(cli->outbuf, '\0', smb_size);
	set_message(cli->outbuf,13,0,True);
	SCVAL(cli->outbuf,smb_com,SMBsesssetupX);
	cli_setup_packet(cli);
			
	SCVAL(cli->outbuf,smb_vwv0,0xFF);
	SSVAL(cli->outbuf,smb_vwv2,CLI_BUFFER_SIZE);
	SSVAL(cli->outbuf,smb_vwv3,2);
	SSVAL(cli->outbuf,smb_vwv4,cli->pid);
	SIVAL(cli->outbuf,smb_vwv5,cli->sesskey);
	SSVAL(cli->outbuf,smb_vwv8,0);
	SIVAL(cli->outbuf,smb_vwv11,capabilities); 
	p = smb_buf(cli->outbuf);
	
	/* check wether to send the ASCII or UNICODE version of the password */
	
	if ( (capabilities & CAP_UNICODE) == 0 ) {
		p += clistr_push(cli, p, pass, -1, STR_TERMINATE); /* password */
		SSVAL(cli->outbuf,smb_vwv7,PTR_DIFF(p, smb_buf(cli->outbuf)));
	}
	else { 
		p += clistr_push(cli, p, pass, -1, STR_UNICODE|STR_TERMINATE); /* unicode password */
		SSVAL(cli->outbuf,smb_vwv8,PTR_DIFF(p, smb_buf(cli->outbuf)));	
	}
	
	p += clistr_push(cli, p, user, -1, STR_TERMINATE); /* username */
	p += clistr_push(cli, p, workgroup, -1, STR_TERMINATE); /* workgroup */
	p += clistr_push(cli, p, "Unix", -1, STR_TERMINATE);
	p += clistr_push(cli, p, lanman, -1, STR_TERMINATE);
	cli_setup_bcc(cli, p);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli))
	      return False;
	
	show_msg(cli->inbuf);
	
	if (cli_is_error(cli))
		return False;

	cli->vuid = SVAL(cli->inbuf,smb_uid);
	p = smb_buf(cli->inbuf);
	p += clistr_pull(cli, cli->server_os, p, sizeof(fstring), -1, STR_TERMINATE);
	p += clistr_pull(cli, cli->server_type, p, sizeof(fstring), -1, STR_TERMINATE);
	p += clistr_pull(cli, cli->server_domain, p, sizeof(fstring), -1, STR_TERMINATE);
	fstrcpy(cli->user_name, user);

	if (strstr(cli->server_type, "Samba")) {
		cli->is_samba = True;
	}

	return True;
}

/****************************************************************************
   do a NT1 NTLM/LM encrypted session setup - for when extended security
   is not negotiated.
   @param cli client state to create do session setup on
   @param user username
   @param pass *either* cleartext password (passlen !=24) or LM response.
   @param ntpass NT response, implies ntpasslen >=24, implies pass is not clear
   @param workgroup The user's domain.
****************************************************************************/

static BOOL cli_session_setup_nt1(struct cli_state *cli, const char *user, 
				  const char *pass, size_t passlen,
				  const char *ntpass, size_t ntpasslen,
				  const char *workgroup)
{
	uint32 capabilities = cli_session_setup_capabilities(cli);
	DATA_BLOB lm_response = data_blob(NULL, 0);
	DATA_BLOB nt_response = data_blob(NULL, 0);
	DATA_BLOB session_key = data_blob(NULL, 0);
	BOOL ret = False;
	char *p;

	if (passlen == 0) {
		/* do nothing - guest login */
	} else if (passlen != 24) {
		if (lp_client_ntlmv2_auth()) {
			DATA_BLOB server_chal;
			DATA_BLOB names_blob;
			server_chal = data_blob(cli->secblob.data, MIN(cli->secblob.length, 8)); 

			/* note that the 'workgroup' here is a best guess - we don't know
			   the server's domain at this point.  The 'server name' is also
			   dodgy... 
			*/
			names_blob = NTLMv2_generate_names_blob(cli->called.name, workgroup);

			if (!SMBNTLMv2encrypt(user, workgroup, pass, &server_chal, 
					      &names_blob,
					      &lm_response, &nt_response, &session_key)) {
				data_blob_free(&names_blob);
				data_blob_free(&server_chal);
				return False;
			}
			data_blob_free(&names_blob);
			data_blob_free(&server_chal);

		} else {
			uchar nt_hash[16];
			E_md4hash(pass, nt_hash);

#ifdef LANMAN_ONLY
			nt_response = data_blob(NULL, 0);
#else
			nt_response = data_blob(NULL, 24);
			SMBNTencrypt(pass,cli->secblob.data,nt_response.data);
#endif
			/* non encrypted password supplied. Ignore ntpass. */
			if (lp_client_lanman_auth()) {
				lm_response = data_blob(NULL, 24);
				if (!SMBencrypt(pass,cli->secblob.data, lm_response.data)) {
					/* Oops, the LM response is invalid, just put 
					   the NT response there instead */
					data_blob_free(&lm_response);
					lm_response = data_blob(nt_response.data, nt_response.length);
				}
			} else {
				/* LM disabled, place NT# in LM field instead */
				lm_response = data_blob(nt_response.data, nt_response.length);
			}

			session_key = data_blob(NULL, 16);
#ifdef LANMAN_ONLY
			E_deshash(pass, session_key.data);
			memset(&session_key.data[8], '\0', 8);
#else
			SMBsesskeygen_ntv1(nt_hash, NULL, session_key.data);
#endif
		}
#ifdef LANMAN_ONLY
		cli_simple_set_signing(cli, session_key, lm_response); 
#else
		cli_simple_set_signing(cli, session_key, nt_response); 
#endif
	} else {
		/* pre-encrypted password supplied.  Only used for 
		   security=server, can't do
		   signing because we don't have original key */

		lm_response = data_blob(pass, passlen);
		nt_response = data_blob(ntpass, ntpasslen);
	}

	/* send a session setup command */
	memset(cli->outbuf,'\0',smb_size);

	set_message(cli->outbuf,13,0,True);
	SCVAL(cli->outbuf,smb_com,SMBsesssetupX);
	cli_setup_packet(cli);
			
	SCVAL(cli->outbuf,smb_vwv0,0xFF);
	SSVAL(cli->outbuf,smb_vwv2,CLI_BUFFER_SIZE);
	SSVAL(cli->outbuf,smb_vwv3,2);
	SSVAL(cli->outbuf,smb_vwv4,cli->pid);
	SIVAL(cli->outbuf,smb_vwv5,cli->sesskey);
	SSVAL(cli->outbuf,smb_vwv7,lm_response.length);
	SSVAL(cli->outbuf,smb_vwv8,nt_response.length);
	SIVAL(cli->outbuf,smb_vwv11,capabilities); 
	p = smb_buf(cli->outbuf);
	if (lm_response.length) {
		memcpy(p,lm_response.data, lm_response.length); p += lm_response.length;
	}
	if (nt_response.length) {
		memcpy(p,nt_response.data, nt_response.length); p += nt_response.length;
	}
	p += clistr_push(cli, p, user, -1, STR_TERMINATE);

	/* Upper case here might help some NTLMv2 implementations */
	p += clistr_push(cli, p, workgroup, -1, STR_TERMINATE|STR_UPPER);
	p += clistr_push(cli, p, "Unix", -1, STR_TERMINATE);
	p += clistr_push(cli, p, "Samba", -1, STR_TERMINATE);
	cli_setup_bcc(cli, p);

	if (!cli_send_smb(cli) || !cli_receive_smb(cli)) {
		ret = False;
		goto end;
	}

	/* show_msg(cli->inbuf); */

	if (cli_is_error(cli)) {
		ret = False;
		goto end;
	}

	/* use the returned vuid from now on */
	cli->vuid = SVAL(cli->inbuf,smb_uid);
	
	p = smb_buf(cli->inbuf);
	p += clistr_pull(cli, cli->server_os, p, sizeof(fstring), -1, STR_TERMINATE);
	p += clistr_pull(cli, cli->server_type, p, sizeof(fstring), -1, STR_TERMINATE);
	p += clistr_pull(cli, cli->server_domain, p, sizeof(fstring), -1, STR_TERMINATE);

	if (strstr(cli->server_type, "Samba")) {
		cli->is_samba = True;
	}

	fstrcpy(cli->user_name, user);

	if (session_key.data) {
		/* Have plaintext orginal */
		cli_set_session_key(cli, session_key);
	}

	ret = True;
end:	
	data_blob_free(&lm_response);
	data_blob_free(&nt_response);
	data_blob_free(&session_key);
	return ret;
}

/****************************************************************************
 Send a extended security session setup blob
****************************************************************************/

static BOOL cli_session_setup_blob_send(struct cli_state *cli, DATA_BLOB blob)
{
	uint32 capabilities = cli_session_setup_capabilities(cli);
	char *p;

	capabilities |= CAP_EXTENDED_SECURITY;

	/* send a session setup command */
	memset(cli->outbuf,'\0',smb_size);

	set_message(cli->outbuf,12,0,True);
	SCVAL(cli->outbuf,smb_com,SMBsesssetupX);

	cli_setup_packet(cli);
			
	SCVAL(cli->outbuf,smb_vwv0,0xFF);
	SSVAL(cli->outbuf,smb_vwv2,CLI_BUFFER_SIZE);
	SSVAL(cli->outbuf,smb_vwv3,2);
	SSVAL(cli->outbuf,smb_vwv4,1);
	SIVAL(cli->outbuf,smb_vwv5,0);
	SSVAL(cli->outbuf,smb_vwv7,blob.length);
	SIVAL(cli->outbuf,smb_vwv10,capabilities); 
	p = smb_buf(cli->outbuf);
	memcpy(p, blob.data, blob.length);
	p += blob.length;
	p += clistr_push(cli, p, "Unix", -1, STR_TERMINATE);
	p += clistr_push(cli, p, "Samba", -1, STR_TERMINATE);
	cli_setup_bcc(cli, p);
	return cli_send_smb(cli);
}

/****************************************************************************
 Send a extended security session setup blob, returning a reply blob.
****************************************************************************/

static DATA_BLOB cli_session_setup_blob_receive(struct cli_state *cli)
{
	DATA_BLOB blob2 = data_blob(NULL, 0);
	char *p;
	size_t len;

	if (!cli_receive_smb(cli))
		return blob2;

	show_msg(cli->inbuf);

	if (cli_is_error(cli) && !NT_STATUS_EQUAL(cli_nt_error(cli),
						  NT_STATUS_MORE_PROCESSING_REQUIRED)) {
		return blob2;
	}
	
	/* use the returned vuid from now on */
	cli->vuid = SVAL(cli->inbuf,smb_uid);
	
	p = smb_buf(cli->inbuf);

	blob2 = data_blob(p, SVAL(cli->inbuf, smb_vwv3));

	p += blob2.length;
	p += clistr_pull(cli, cli->server_os, p, sizeof(fstring), -1, STR_TERMINATE);

	/* w2k with kerberos doesn't properly null terminate this field */
	len = smb_buflen(cli->inbuf) - PTR_DIFF(p, smb_buf(cli->inbuf));
	p += clistr_pull(cli, cli->server_type, p, sizeof(fstring), len, 0);

	return blob2;
}

#ifdef HAVE_KRB5

/****************************************************************************
 Send a extended security session setup blob, returning a reply blob.
****************************************************************************/

static DATA_BLOB cli_session_setup_blob(struct cli_state *cli, DATA_BLOB blob)
{
	DATA_BLOB blob2 = data_blob(NULL, 0);
	if (!cli_session_setup_blob_send(cli, blob)) {
		return blob2;
	}
		
	return cli_session_setup_blob_receive(cli);
}

/****************************************************************************
 Use in-memory credentials cache
****************************************************************************/

static void use_in_memory_ccache(void) {
	setenv(KRB5_ENV_CCNAME, "MEMORY:cliconnect", 1);
}

/****************************************************************************
 Do a spnego/kerberos encrypted session setup.
****************************************************************************/

static ADS_STATUS cli_session_setup_kerberos(struct cli_state *cli, const char *principal, const char *workgroup)
{
	DATA_BLOB blob2, negTokenTarg;
	DATA_BLOB session_key_krb5;
	DATA_BLOB null_blob = data_blob(NULL, 0);
	int rc;

	DEBUG(2,("Doing kerberos session setup\n"));

	/* generate the encapsulated kerberos5 ticket */
	rc = spnego_gen_negTokenTarg(principal, 0, &negTokenTarg, &session_key_krb5, 0);

	if (rc) {
		DEBUG(1, ("spnego_gen_negTokenTarg failed: %s\n", error_message(rc)));
		return ADS_ERROR_KRB5(rc);
	}

#if 0
	file_save("negTokenTarg.dat", negTokenTarg.data, negTokenTarg.length);
#endif

	cli_simple_set_signing(cli, session_key_krb5, null_blob); 
			
	blob2 = cli_session_setup_blob(cli, negTokenTarg);

	/* we don't need this blob for kerberos */
	data_blob_free(&blob2);

	cli_set_session_key(cli, session_key_krb5);

	data_blob_free(&negTokenTarg);
	data_blob_free(&session_key_krb5);

	if (cli_is_error(cli)) {
		if (NT_STATUS_IS_OK(cli_nt_error(cli))) {
			return ADS_ERROR_NT(NT_STATUS_UNSUCCESSFUL);
		}
	} 
	return ADS_ERROR_NT(cli_nt_error(cli));
}
#endif	/* HAVE_KRB5 */


/****************************************************************************
 Do a spnego/NTLMSSP encrypted session setup.
****************************************************************************/

static NTSTATUS cli_session_setup_ntlmssp(struct cli_state *cli, const char *user, 
					  const char *pass, const char *domain)
{
	struct ntlmssp_state *ntlmssp_state;
	NTSTATUS nt_status;
	int turn = 1;
	DATA_BLOB msg1;
	DATA_BLOB blob = data_blob(NULL, 0);
	DATA_BLOB blob_in = data_blob(NULL, 0);
	DATA_BLOB blob_out = data_blob(NULL, 0);

	cli_temp_set_signing(cli);

	if (!NT_STATUS_IS_OK(nt_status = ntlmssp_client_start(&ntlmssp_state))) {
		return nt_status;
	}

	if (!NT_STATUS_IS_OK(nt_status = ntlmssp_set_username(ntlmssp_state, user))) {
		return nt_status;
	}
	if (!NT_STATUS_IS_OK(nt_status = ntlmssp_set_domain(ntlmssp_state, domain))) {
		return nt_status;
	}
	if (!NT_STATUS_IS_OK(nt_status = ntlmssp_set_password(ntlmssp_state, pass))) {
		return nt_status;
	}

	do {
		nt_status = ntlmssp_update(ntlmssp_state, 
						  blob_in, &blob_out);
		data_blob_free(&blob_in);
		if (NT_STATUS_EQUAL(nt_status, NT_STATUS_MORE_PROCESSING_REQUIRED) || NT_STATUS_IS_OK(nt_status)) {
			if (turn == 1) {
				/* and wrap it in a SPNEGO wrapper */
				msg1 = gen_negTokenInit(OID_NTLMSSP, blob_out);
			} else {
				/* wrap it in SPNEGO */
				msg1 = spnego_gen_auth(blob_out);
			}
		
			/* now send that blob on its way */
			if (!cli_session_setup_blob_send(cli, msg1)) {
				DEBUG(3, ("Failed to send NTLMSSP/SPNEGO blob to server!\n"));
				nt_status = NT_STATUS_UNSUCCESSFUL;
			} else {
				data_blob_free(&msg1);
				
				blob = cli_session_setup_blob_receive(cli);
				
				nt_status = cli_nt_error(cli);
				if (cli_is_error(cli) && NT_STATUS_IS_OK(nt_status)) {
					if (cli->smb_rw_error == READ_BAD_SIG) {
						nt_status = NT_STATUS_ACCESS_DENIED;
					} else {
						nt_status = NT_STATUS_UNSUCCESSFUL;
					}
				}
			}
		}
		
		if (!blob.length) {
			if (NT_STATUS_IS_OK(nt_status)) {
				nt_status = NT_STATUS_UNSUCCESSFUL;
			}
		} else if ((turn == 1) && 
			   NT_STATUS_EQUAL(nt_status, NT_STATUS_MORE_PROCESSING_REQUIRED)) {
			DATA_BLOB tmp_blob = data_blob(NULL, 0);
			/* the server might give us back two challenges */
			if (!spnego_parse_challenge(blob, &blob_in, 
						    &tmp_blob)) {
				DEBUG(3,("Failed to parse challenges\n"));
				nt_status = NT_STATUS_INVALID_PARAMETER;
			}
			data_blob_free(&tmp_blob);
		} else {
			if (!spnego_parse_auth_response(blob, nt_status, 
							&blob_in)) {
				DEBUG(3,("Failed to parse auth response\n"));
				if (NT_STATUS_IS_OK(nt_status) 
				    || NT_STATUS_EQUAL(nt_status, NT_STATUS_MORE_PROCESSING_REQUIRED)) 
					nt_status = NT_STATUS_INVALID_PARAMETER;
			}
		}
		data_blob_free(&blob);
		data_blob_free(&blob_out);
		turn++;
	} while (NT_STATUS_EQUAL(nt_status, NT_STATUS_MORE_PROCESSING_REQUIRED));

	if (NT_STATUS_IS_OK(nt_status)) {

		DATA_BLOB key = data_blob(ntlmssp_state->session_key.data,
					  ntlmssp_state->session_key.length);
		DATA_BLOB null_blob = data_blob(NULL, 0);
		BOOL res;

		fstrcpy(cli->server_domain, ntlmssp_state->server_domain);
		cli_set_session_key(cli, ntlmssp_state->session_key);

		res = cli_simple_set_signing(cli, key, null_blob);

		data_blob_free(&key);

		if (res) {
			
			/* 'resign' the last message, so we get the right sequence numbers
			   for checking the first reply from the server */
			cli_calculate_sign_mac(cli);
			
			if (!cli_check_sign_mac(cli)) {
				nt_status = NT_STATUS_ACCESS_DENIED;
			}
		}
	}

	/* we have a reference conter on ntlmssp_state, if we are signing
	   then the state will be kept by the signing engine */

	ntlmssp_end(&ntlmssp_state);

	return nt_status;
}

/****************************************************************************
 Do a spnego encrypted session setup.
****************************************************************************/

ADS_STATUS cli_session_setup_spnego(struct cli_state *cli, const char *user, 
			      const char *pass, const char *domain)
{
	char *principal;
	char *OIDs[ASN1_MAX_OIDS];
	int i;
	BOOL got_kerberos_mechanism = False;
	DATA_BLOB blob;

	DEBUG(3,("Doing spnego session setup (blob length=%lu)\n", (unsigned long)cli->secblob.length));

	/* the server might not even do spnego */
	if (cli->secblob.length <= 16) {
		DEBUG(3,("server didn't supply a full spnego negprot\n"));
		goto ntlmssp;
	}

#if 0
	file_save("negprot.dat", cli->secblob.data, cli->secblob.length);
#endif

	/* there is 16 bytes of GUID before the real spnego packet starts */
	blob = data_blob(cli->secblob.data+16, cli->secblob.length-16);

	/* the server sent us the first part of the SPNEGO exchange in the negprot 
	   reply */
	if (!spnego_parse_negTokenInit(blob, OIDs, &principal)) {
		data_blob_free(&blob);
		return ADS_ERROR_NT(NT_STATUS_INVALID_PARAMETER);
	}
	data_blob_free(&blob);

	/* make sure the server understands kerberos */
	for (i=0;OIDs[i];i++) {
		DEBUG(3,("got OID=%s\n", OIDs[i]));
		if (strcmp(OIDs[i], OID_KERBEROS5_OLD) == 0 ||
		    strcmp(OIDs[i], OID_KERBEROS5) == 0) {
			got_kerberos_mechanism = True;
		}
		free(OIDs[i]);
	}

	DEBUG(3,("got principal=%s\n", principal ? principal : "<null>"));

	if (got_kerberos_mechanism && (principal == NULL)) {
		/*
		 * It is WRONG to depend on the principal sent in the negprot
		 * reply, but right now we do it. So for safety (don't
		 * segfault later) disable Kerberos when no principal was
		 * sent. -- VL
		 */
		DEBUG(1, ("Kerberos mech was offered, but no principal was "
			"sent, disabling Kerberos\n"));
		cli->use_kerberos = False;
	}

	fstrcpy(cli->user_name, user);

#ifdef HAVE_KRB5
	/* If password is set we reauthenticate to kerberos server
	 * and do not store results */

	if (got_kerberos_mechanism && cli->use_kerberos) {
		ADS_STATUS rc;

		if (pass && *pass) {
			int ret;
			
			use_in_memory_ccache();
			ret = kerberos_kinit_password(user, pass, 0 /* no time correction for now */, NULL);
			
			if (ret){
				SAFE_FREE(principal);
				DEBUG(0, ("Kinit failed: %s\n", error_message(ret)));
				if (cli->fallback_after_kerberos)
					goto ntlmssp;
				return ADS_ERROR_KRB5(ret);
			}
		}
		
		rc = cli_session_setup_kerberos(cli, principal, domain);
		if (ADS_ERR_OK(rc) || !cli->fallback_after_kerberos) {
			SAFE_FREE(principal);
			return rc;
		}
	}
#endif

	SAFE_FREE(principal);

ntlmssp:

	return ADS_ERROR_NT(cli_session_setup_ntlmssp(cli, user, pass, domain));
}

/****************************************************************************
 Send a session setup. The username and workgroup is in UNIX character
 format and must be converted to DOS codepage format before sending. If the
 password is in plaintext, the same should be done.
****************************************************************************/

BOOL cli_session_setup(struct cli_state *cli, 
		       const char *user, 
		       const char *pass, int passlen,
		       const char *ntpass, int ntpasslen,
		       const char *workgroup)
{
	char *p;
	fstring user2;

	/* allow for workgroups as part of the username */
	fstrcpy(user2, user);
	if ((p=strchr_m(user2,'\\')) || (p=strchr_m(user2,'/')) ||
	    (p=strchr_m(user2,*lp_winbind_separator()))) {
		*p = 0;
		user = p+1;
		workgroup = user2;
	}

	if (cli->protocol < PROTOCOL_LANMAN1)
		return True;

	/* now work out what sort of session setup we are going to
           do. I have split this into separate functions to make the
           flow a bit easier to understand (tridge) */

	/* if its an older server then we have to use the older request format */

	if (cli->protocol < PROTOCOL_NT1) {
		if (!lp_client_lanman_auth() && passlen != 24 && (*pass)) {
			DEBUG(1, ("Server requested LM password but 'client lanman auth'"
				  " is disabled\n"));
			return False;
		}

		if ((cli->sec_mode & NEGOTIATE_SECURITY_CHALLENGE_RESPONSE) == 0 &&
		    !lp_client_plaintext_auth() && (*pass)) {
			DEBUG(1, ("Server requested plaintext password but 'client use plaintext auth'"
				  " is disabled\n"));
			return False;
		}

		return cli_session_setup_lanman2(cli, user, pass, passlen, workgroup);
	}

	/* if no user is supplied then we have to do an anonymous connection.
	   passwords are ignored */

	if (!user || !*user)
		return cli_session_setup_guest(cli);

	/* if the server is share level then send a plaintext null
           password at this point. The password is sent in the tree
           connect */

	if ((cli->sec_mode & NEGOTIATE_SECURITY_USER_LEVEL) == 0) 
		return cli_session_setup_plaintext(cli, user, "", workgroup);

	/* if the server doesn't support encryption then we have to use 
	   plaintext. The second password is ignored */

	if ((cli->sec_mode & NEGOTIATE_SECURITY_CHALLENGE_RESPONSE) == 0) {
		if (!lp_client_plaintext_auth() && (*pass)) {
			DEBUG(1, ("Server requested plaintext password but 'client use plaintext auth'"
				  " is disabled\n"));
			return False;
		}
		return cli_session_setup_plaintext(cli, user, pass, workgroup);
	}

	/* if the server supports extended security then use SPNEGO */

	if (cli->capabilities & CAP_EXTENDED_SECURITY) {
		ADS_STATUS status = cli_session_setup_spnego(cli, user, pass, workgroup);
		if (!ADS_ERR_OK(status)) {
			DEBUG(3, ("SPNEGO login failed: %s\n", ads_errstr(status)));
			return False;
		}
	} else {
		/* otherwise do a NT1 style session setup */
		if ( !cli_session_setup_nt1(cli, user, pass, passlen, ntpass, ntpasslen, workgroup) ) {
			DEBUG(3,("cli_session_setup: NT1 session setup failed!\n"));
			return False;
		}
	}

	if (strstr(cli->server_type, "Samba")) {
		cli->is_samba = True;
	}

	return True;

}

/****************************************************************************
 Send a uloggoff.
*****************************************************************************/

BOOL cli_ulogoff(struct cli_state *cli)
{
	memset(cli->outbuf,'\0',smb_size);
	set_message(cli->outbuf,2,0,True);
	SCVAL(cli->outbuf,smb_com,SMBulogoffX);
	cli_setup_packet(cli);
	SSVAL(cli->outbuf,smb_vwv0,0xFF);
	SSVAL(cli->outbuf,smb_vwv2,0);  /* no additional info */

	cli_send_smb(cli);
	if (!cli_receive_smb(cli))
		return False;

	if (cli_is_error(cli)) {
		return False;
	}

        cli->cnum = -1;
        return True;
}

/****************************************************************************
 Send a tconX.
****************************************************************************/

BOOL cli_send_tconX(struct cli_state *cli, 
		    const char *share, const char *dev, const char *pass, int passlen)
{
	fstring fullshare, pword;
	char *p;
	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	fstrcpy(cli->share, share);

	/* in user level security don't send a password now */
	if (cli->sec_mode & NEGOTIATE_SECURITY_USER_LEVEL) {
		passlen = 1;
		pass = "";
	} else if (!pass) {
		DEBUG(1, ("Server not using user level security and no password supplied.\n"));
		return False;
	}

	if ((cli->sec_mode & NEGOTIATE_SECURITY_CHALLENGE_RESPONSE) &&
	    *pass && passlen != 24) {
		if (!lp_client_lanman_auth()) {
			DEBUG(1, ("Server requested LANMAN password (share-level security) but 'client use lanman auth'"
				  " is disabled\n"));
			return False;
		}

		/*
		 * Non-encrypted passwords - convert to DOS codepage before encryption.
		 */
		passlen = 24;
		SMBencrypt(pass,cli->secblob.data,(uchar *)pword);
	} else {
		if((cli->sec_mode & (NEGOTIATE_SECURITY_USER_LEVEL|NEGOTIATE_SECURITY_CHALLENGE_RESPONSE)) == 0) {
			if (!lp_client_plaintext_auth() && (*pass)) {
				DEBUG(1, ("Server requested plaintext password but 'client use plaintext auth'"
					  " is disabled\n"));
				return False;
			}

			/*
			 * Non-encrypted passwords - convert to DOS codepage before using.
			 */
			passlen = clistr_push(cli, pword, pass, sizeof(pword), STR_TERMINATE);
			
		} else {
			if (passlen) {
				memcpy(pword, pass, passlen);
			}
		}
	}

	slprintf(fullshare, sizeof(fullshare)-1,
		 "\\\\%s\\%s", cli->desthost, share);

	set_message(cli->outbuf,4, 0, True);
	SCVAL(cli->outbuf,smb_com,SMBtconX);
	cli_setup_packet(cli);

	SSVAL(cli->outbuf,smb_vwv0,0xFF);
	SSVAL(cli->outbuf,smb_vwv3,passlen);

	p = smb_buf(cli->outbuf);
	if (passlen) {
		memcpy(p,pword,passlen);
	}
	p += passlen;
	p += clistr_push(cli, p, fullshare, -1, STR_TERMINATE |STR_UPPER);
	p += clistr_push(cli, p, dev, -1, STR_TERMINATE |STR_UPPER | STR_ASCII);

	cli_setup_bcc(cli, p);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli))
		return False;

	if (cli_is_error(cli))
		return False;

	clistr_pull(cli, cli->dev, smb_buf(cli->inbuf), sizeof(fstring), -1, STR_TERMINATE|STR_ASCII);

	if (cli->protocol >= PROTOCOL_NT1 &&
	    smb_buflen(cli->inbuf) == 3) {
		/* almost certainly win95 - enable bug fixes */
		cli->win95 = True;
	}
	
	/* Make sure that we have the optional support 16-bit field.  WCT > 2 */
	/* Avoids issues when connecting to Win9x boxes sharing files */

	cli->dfsroot = False;
	if ( (CVAL(cli->inbuf, smb_wct))>2 && cli->protocol >= PROTOCOL_LANMAN2 )
		cli->dfsroot = (SVAL( cli->inbuf, smb_vwv2 ) & SMB_SHARE_IN_DFS) ? True : False;

	cli->cnum = SVAL(cli->inbuf,smb_tid);
	return True;
}

/****************************************************************************
 Send a tree disconnect.
****************************************************************************/

BOOL cli_tdis(struct cli_state *cli)
{
	memset(cli->outbuf,'\0',smb_size);
	set_message(cli->outbuf,0,0,True);
	SCVAL(cli->outbuf,smb_com,SMBtdis);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);
	
	cli_send_smb(cli);
	if (!cli_receive_smb(cli))
		return False;
	
	if (cli_is_error(cli)) {
		return False;
	}

	cli->cnum = -1;
	return True;
}

/****************************************************************************
 Send a negprot command.
****************************************************************************/

void cli_negprot_send(struct cli_state *cli)
{
	char *p;
	int numprots;

	if (cli->protocol < PROTOCOL_NT1)
		cli->use_spnego = False;

	memset(cli->outbuf,'\0',smb_size);

	/* setup the protocol strings */
	set_message(cli->outbuf,0,0,True);

	p = smb_buf(cli->outbuf);
	for (numprots=0;
	     prots[numprots].name && prots[numprots].prot<=cli->protocol;
	     numprots++) {
		*p++ = 2;
		p += clistr_push(cli, p, prots[numprots].name, -1, STR_TERMINATE);
	}

	SCVAL(cli->outbuf,smb_com,SMBnegprot);
	cli_setup_bcc(cli, p);
	cli_setup_packet(cli);

	SCVAL(smb_buf(cli->outbuf),0,2);

	cli_send_smb(cli);
}

/****************************************************************************
 Send a negprot command.
****************************************************************************/

BOOL cli_negprot(struct cli_state *cli)
{
	char *p;
	int numprots;
	int plength;

	if (cli->protocol < PROTOCOL_NT1)
		cli->use_spnego = False;

	memset(cli->outbuf,'\0',smb_size);

	/* setup the protocol strings */
	for (plength=0,numprots=0;
	     prots[numprots].name && prots[numprots].prot<=cli->protocol;
	     numprots++)
		plength += strlen(prots[numprots].name)+2;
    
	set_message(cli->outbuf,0,plength,True);

	p = smb_buf(cli->outbuf);
	for (numprots=0;
	     prots[numprots].name && prots[numprots].prot<=cli->protocol;
	     numprots++) {
		*p++ = 2;
		p += clistr_push(cli, p, prots[numprots].name, -1, STR_TERMINATE);
	}

	SCVAL(cli->outbuf,smb_com,SMBnegprot);
	cli_setup_packet(cli);

	SCVAL(smb_buf(cli->outbuf),0,2);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli))
		return False;

	show_msg(cli->inbuf);

	if (cli_is_error(cli) ||
	    ((int)SVAL(cli->inbuf,smb_vwv0) >= numprots)) {
		return(False);
	}

	cli->protocol = prots[SVAL(cli->inbuf,smb_vwv0)].prot;	

	if ((cli->protocol < PROTOCOL_NT1) && cli->sign_info.mandatory_signing) {
		DEBUG(0,("cli_negprot: SMB signing is mandatory and the selected protocol level doesn't support it.\n"));
		return False;
	}

	if (cli->protocol >= PROTOCOL_NT1) {    
		/* NT protocol */
		cli->sec_mode = CVAL(cli->inbuf,smb_vwv1);
		cli->max_mux = SVAL(cli->inbuf, smb_vwv1+1);
		cli->max_xmit = IVAL(cli->inbuf,smb_vwv3+1);
		cli->sesskey = IVAL(cli->inbuf,smb_vwv7+1);
		cli->serverzone = SVALS(cli->inbuf,smb_vwv15+1);
		cli->serverzone *= 60;
		/* this time arrives in real GMT */
		cli->servertime = interpret_long_date(cli->inbuf+smb_vwv11+1);
		cli->secblob = data_blob(smb_buf(cli->inbuf),smb_buflen(cli->inbuf));
		cli->capabilities = IVAL(cli->inbuf,smb_vwv9+1);
		if (cli->capabilities & CAP_RAW_MODE) {
			cli->readbraw_supported = True;
			cli->writebraw_supported = True;      
		}
		/* work out if they sent us a workgroup */
		if (!(cli->capabilities & CAP_EXTENDED_SECURITY) &&
		    smb_buflen(cli->inbuf) > 8) {
			clistr_pull(cli, cli->server_domain, 
				    smb_buf(cli->inbuf)+8, sizeof(cli->server_domain),
				    smb_buflen(cli->inbuf)-8, STR_UNICODE|STR_NOALIGN);
		}

		/*
		 * As signing is slow we only turn it on if either the client or
		 * the server require it. JRA.
		 */

		if (cli->sec_mode & NEGOTIATE_SECURITY_SIGNATURES_REQUIRED) {
			/* Fail if server says signing is mandatory and we don't want to support it. */
			if (!cli->sign_info.allow_smb_signing) {
				DEBUG(0,("cli_negprot: SMB signing is mandatory and we have disabled it.\n"));
				return False;
			}
			cli->sign_info.negotiated_smb_signing = True;
			cli->sign_info.mandatory_signing = True;
		} else if (cli->sign_info.mandatory_signing && cli->sign_info.allow_smb_signing) {
			/* Fail if client says signing is mandatory and the server doesn't support it. */
			if (!(cli->sec_mode & NEGOTIATE_SECURITY_SIGNATURES_ENABLED)) {
				DEBUG(1,("cli_negprot: SMB signing is mandatory and the server doesn't support it.\n"));
				return False;
			}
			cli->sign_info.negotiated_smb_signing = True;
			cli->sign_info.mandatory_signing = True;
		} else if (cli->sec_mode & NEGOTIATE_SECURITY_SIGNATURES_ENABLED) {
			cli->sign_info.negotiated_smb_signing = True;
		}

		if (cli->capabilities & (CAP_LARGE_READX|CAP_LARGE_WRITEX)) {
			SAFE_FREE(cli->outbuf);
			SAFE_FREE(cli->inbuf);
			cli->outbuf = (char *)SMB_MALLOC(CLI_SAMBA_MAX_LARGE_READX_SIZE+SAFETY_MARGIN);
			cli->inbuf = (char *)SMB_MALLOC(CLI_SAMBA_MAX_LARGE_READX_SIZE+SAFETY_MARGIN);
			cli->bufsize = CLI_SAMBA_MAX_LARGE_READX_SIZE;
		}

	} else if (cli->protocol >= PROTOCOL_LANMAN1) {
		cli->use_spnego = False;
		cli->sec_mode = SVAL(cli->inbuf,smb_vwv1);
		cli->max_xmit = SVAL(cli->inbuf,smb_vwv2);
		cli->max_mux = SVAL(cli->inbuf, smb_vwv3); 
		cli->sesskey = IVAL(cli->inbuf,smb_vwv6);
		cli->serverzone = SVALS(cli->inbuf,smb_vwv10);
		cli->serverzone *= 60;
		/* this time is converted to GMT by make_unix_date */
		cli->servertime = cli_make_unix_date(cli,cli->inbuf+smb_vwv8);
		cli->readbraw_supported = ((SVAL(cli->inbuf,smb_vwv5) & 0x1) != 0);
		cli->writebraw_supported = ((SVAL(cli->inbuf,smb_vwv5) & 0x2) != 0);
		cli->secblob = data_blob(smb_buf(cli->inbuf),smb_buflen(cli->inbuf));
	} else {
		/* the old core protocol */
		cli->use_spnego = False;
		cli->sec_mode = 0;
		cli->serverzone = get_time_zone(time(NULL));
	}

	cli->max_xmit = MIN(cli->max_xmit, CLI_BUFFER_SIZE);

	/* a way to force ascii SMB */
#ifndef _XBOX
	if (getenv("CLI_FORCE_ASCII"))
		cli->capabilities &= ~CAP_UNICODE;
#endif //_XBOX

	return True;
}

/****************************************************************************
 Send a session request. See rfc1002.txt 4.3 and 4.3.2.
****************************************************************************/

BOOL cli_session_request(struct cli_state *cli,
			 struct nmb_name *calling, struct nmb_name *called)
{
	char *p;
	int len = 4;

	memcpy(&(cli->calling), calling, sizeof(*calling));
	memcpy(&(cli->called ), called , sizeof(*called ));
  
	/* put in the destination name */
	p = cli->outbuf+len;
	name_mangle(cli->called .name, p, cli->called .name_type);
	len += name_len(p);

	/* and my name */
	p = cli->outbuf+len;
	name_mangle(cli->calling.name, p, cli->calling.name_type);
	len += name_len(p);

	/* 445 doesn't have session request */
	if (cli->port == 445)
		return True;

	/* send a session request (RFC 1002) */
	/* setup the packet length
         * Remove four bytes from the length count, since the length
         * field in the NBT Session Service header counts the number
         * of bytes which follow.  The cli_send_smb() function knows
         * about this and accounts for those four bytes.
         * CRH.
         */
        len -= 4;
	_smb_setlen(cli->outbuf,len);
	SCVAL(cli->outbuf,0,0x81);

	cli_send_smb(cli);
	DEBUG(5,("Sent session request\n"));

	if (!cli_receive_smb(cli))
		return False;

	if (CVAL(cli->inbuf,0) == 0x84) {
		/* C. Hoch  9/14/95 Start */
		/* For information, here is the response structure.
		 * We do the byte-twiddling to for portability.
		struct RetargetResponse{
		unsigned char type;
		unsigned char flags;
		int16 length;
		int32 ip_addr;
		int16 port;
		};
		*/
		int port = (CVAL(cli->inbuf,8)<<8)+CVAL(cli->inbuf,9);
		/* SESSION RETARGET */
		putip((char *)&cli->dest_ip,cli->inbuf+4);

		cli->fd = open_socket_out(SOCK_STREAM, &cli->dest_ip, port, LONG_CONNECT_TIMEOUT);
		if (cli->fd == -1)
			return False;

		DEBUG(3,("Retargeted\n"));

		set_socket_options(cli->fd,user_socket_options);

		/* Try again */
		{
			static int depth;
			BOOL ret;
			if (depth > 4) {
				DEBUG(0,("Retarget recursion - failing\n"));
				return False;
			}
			depth++;
			ret = cli_session_request(cli, calling, called);
			depth--;
			return ret;
		}
	} /* C. Hoch 9/14/95 End */

	if (CVAL(cli->inbuf,0) != 0x82) {
                /* This is the wrong place to put the error... JRA. */
		cli->rap_error = CVAL(cli->inbuf,4);
		return False;
	}
	return(True);
}

/****************************************************************************
 Open the client sockets.
****************************************************************************/

BOOL cli_connect(struct cli_state *cli, const char *host, struct in_addr *ip)
{
	int name_type = 0x20;
	char *p;

	/* reasonable default hostname */
	if (!host) host = "*SMBSERVER";

	fstrcpy(cli->desthost, host);

	/* allow hostnames of the form NAME#xx and do a netbios lookup */
	if ((p = strchr(cli->desthost, '#'))) {
		name_type = strtol(p+1, NULL, 16);		
		*p = 0;
	}
	
	if (!ip || is_zero_ip(*ip)) {
                if (!resolve_name(cli->desthost, &cli->dest_ip, name_type)) {
                        return False;
                }
		if (ip) *ip = cli->dest_ip;
	} else {
		cli->dest_ip = *ip;
	}

#ifdef _XBOX
	{
#elif
	if (getenv("LIBSMB_PROG")) {
		cli->fd = sock_exec(getenv("LIBSMB_PROG"));
	} else {
#endif //_XBOX
		/* try 445 first, then 139 */
		int port = cli->port?cli->port:445;
		cli->fd = open_socket_out(SOCK_STREAM, &cli->dest_ip, 
					  port, cli->timeout);
		if (cli->fd == -1 && cli->port == 0) {
			port = 139;
			cli->fd = open_socket_out(SOCK_STREAM, &cli->dest_ip, 
						  port, cli->timeout);
		}
		if (cli->fd != -1)
			cli->port = port;
	}
	if (cli->fd == -1) {
		DEBUG(1,("Error connecting to %s (%s)\n",
			 ip?inet_ntoa(*ip):host,strerror(errno)));
		return False;
	}

	set_socket_options(cli->fd,user_socket_options);

	return True;
}

/**
   establishes a connection to after the negprot. 
   @param output_cli A fully initialised cli structure, non-null only on success
   @param dest_host The netbios name of the remote host
   @param dest_ip (optional) The the destination IP, NULL for name based lookup
   @param port (optional) The destination port (0 for default)
   @param retry BOOL. Did this connection fail with a retryable error ?

*/
NTSTATUS cli_start_connection(struct cli_state **output_cli, 
			      const char *my_name, 
			      const char *dest_host, 
			      struct in_addr *dest_ip, int port,
			      int signing_state, int flags,
			      BOOL *retry) 
{
	NTSTATUS nt_status;
	struct nmb_name calling;
	struct nmb_name called;
	struct cli_state *cli;
	struct in_addr ip;

	if (retry)
		*retry = False;

	if (!my_name) 
		my_name = global_myname();
	
	if (!(cli = cli_initialise(NULL)))
		return NT_STATUS_NO_MEMORY;
	
	make_nmb_name(&calling, my_name, 0x0);
	make_nmb_name(&called , dest_host, 0x20);

	if (cli_set_port(cli, port) != port) {
		cli_shutdown(cli);
		return NT_STATUS_UNSUCCESSFUL;
	}

	cli_set_timeout(cli, 10000); /* 10 seconds. */

	if (dest_ip)
		ip = *dest_ip;
	else
		ZERO_STRUCT(ip);

again:

	DEBUG(3,("Connecting to host=%s\n", dest_host));
	
	if (!cli_connect(cli, dest_host, &ip)) {
		DEBUG(1,("cli_start_connection: failed to connect to %s (%s)\n",
			 nmb_namestr(&called), inet_ntoa(ip)));
		cli_shutdown(cli);
		if (is_zero_ip(ip)) {
			return NT_STATUS_BAD_NETWORK_NAME;
		} else {
			return NT_STATUS_CONNECTION_REFUSED;
		}
	}

	if (retry)
		*retry = True;

	if (!cli_session_request(cli, &calling, &called)) {
		char *p;
		DEBUG(1,("session request to %s failed (%s)\n", 
			 called.name, cli_errstr(cli)));
		if ((p=strchr(called.name, '.')) && !is_ipaddress(called.name)) {
			*p = 0;
			goto again;
		}
		if (strcmp(called.name, "*SMBSERVER")) {
			make_nmb_name(&called , "*SMBSERVER", 0x20);
			goto again;
		}
		return NT_STATUS_BAD_NETWORK_NAME;
	}

	cli_setup_signing_state(cli, signing_state);

	if (flags & CLI_FULL_CONNECTION_DONT_SPNEGO)
		cli->use_spnego = False;
	else if (flags & CLI_FULL_CONNECTION_USE_KERBEROS)
		cli->use_kerberos = True;

	if (!cli_negprot(cli)) {
		DEBUG(1,("failed negprot\n"));
		nt_status = cli_nt_error(cli);
		if (NT_STATUS_IS_OK(nt_status)) {
			nt_status = NT_STATUS_UNSUCCESSFUL;
		}
		cli_shutdown(cli);
		return nt_status;
	}

	*output_cli = cli;
	return NT_STATUS_OK;
}


/**
   establishes a connection right up to doing tconX, password specified.
   @param output_cli A fully initialised cli structure, non-null only on success
   @param dest_host The netbios name of the remote host
   @param dest_ip (optional) The the destination IP, NULL for name based lookup
   @param port (optional) The destination port (0 for default)
   @param service (optional) The share to make the connection to.  Should be 'unqualified' in any way.
   @param service_type The 'type' of serivice. 
   @param user Username, unix string
   @param domain User's domain
   @param password User's password, unencrypted unix string.
   @param retry BOOL. Did this connection fail with a retryable error ?
*/

NTSTATUS cli_full_connection(struct cli_state **output_cli, 
			     const char *my_name, 
			     const char *dest_host, 
			     struct in_addr *dest_ip, int port,
			     const char *service, const char *service_type,
			     const char *user, const char *domain, 
			     const char *password, int flags,
			     int signing_state,
			     BOOL *retry) 
{
	NTSTATUS nt_status;
	struct cli_state *cli = NULL;
	int pw_len = password ? strlen(password)+1 : 0;

	if (password == NULL) {
		password = "";
	}

	nt_status = cli_start_connection(&cli, my_name, dest_host, 
					 dest_ip, port, signing_state, flags, retry);
	
	if (!NT_STATUS_IS_OK(nt_status)) {
		return nt_status;
	}

	if (!cli_session_setup(cli, user, password, pw_len, password, pw_len, domain)) {
		if ((flags & CLI_FULL_CONNECTION_ANNONYMOUS_FALLBACK)
		    && cli_session_setup(cli, "", "", 0, "", 0, domain)) {
		} else {
			nt_status = cli_nt_error(cli);
			DEBUG(1,("failed session setup with %s\n", nt_errstr(nt_status)));
			cli_shutdown(cli);
			if (NT_STATUS_IS_OK(nt_status)) 
				nt_status = NT_STATUS_UNSUCCESSFUL;
			return nt_status;
		}
	} 

	if (service) {
		if (!cli_send_tconX(cli, service, service_type, password, pw_len)) {
			nt_status = cli_nt_error(cli);
			DEBUG(1,("failed tcon_X with %s\n", nt_errstr(nt_status)));
			cli_shutdown(cli);
			if (NT_STATUS_IS_OK(nt_status)) {
				nt_status = NT_STATUS_UNSUCCESSFUL;
			}
			return nt_status;
		}
	}

	cli_init_creds(cli, user, domain, password);

	*output_cli = cli;
	return NT_STATUS_OK;
}

/****************************************************************************
 Attempt a NetBIOS session request, falling back to *SMBSERVER if needed.
****************************************************************************/

BOOL attempt_netbios_session_request(struct cli_state *cli, const char *srchost, const char *desthost,
                                     struct in_addr *pdest_ip)
{
	struct nmb_name calling, called;

	make_nmb_name(&calling, srchost, 0x0);

	/*
	 * If the called name is an IP address
	 * then use *SMBSERVER immediately.
	 */

	if(is_ipaddress(desthost))
		make_nmb_name(&called, "*SMBSERVER", 0x20);
	else
		make_nmb_name(&called, desthost, 0x20);

	if (!cli_session_request(cli, &calling, &called)) {
		struct nmb_name smbservername;

		make_nmb_name(&smbservername , "*SMBSERVER", 0x20);

		/*
		 * If the name wasn't *SMBSERVER then
		 * try with *SMBSERVER if the first name fails.
		 */

		if (nmb_name_equal(&called, &smbservername)) {

			/*
			 * The name used was *SMBSERVER, don't bother with another name.
			 */

			DEBUG(0,("attempt_netbios_session_request: %s rejected the session for name *SMBSERVER \
with error %s.\n", desthost, cli_errstr(cli) ));
			return False;
		}

		/*
		 * We need to close the connection here but can't call cli_shutdown as
		 * will free an allocated cli struct. cli_close_connection was invented
		 * for this purpose. JRA. Based on work by "Kim R. Pedersen" <krp@filanet.dk>.
		 */

		cli_close_connection(cli);

		if (!cli_initialise(cli) ||
				!cli_connect(cli, desthost, pdest_ip) ||
				!cli_session_request(cli, &calling, &smbservername)) {
			DEBUG(0,("attempt_netbios_session_request: %s rejected the session for \
name *SMBSERVER with error %s\n", desthost, cli_errstr(cli) ));
			return False;
		}
	}

	return True;
}





/****************************************************************************
 Send an old style tcon.
****************************************************************************/
NTSTATUS cli_raw_tcon(struct cli_state *cli, 
		      const char *service, const char *pass, const char *dev,
		      uint16 *max_xmit, uint16 *tid)
{
	char *p;

	if (!lp_client_plaintext_auth() && (*pass)) {
		DEBUG(1, ("Server requested plaintext password but 'client use plaintext auth'"
			  " is disabled\n"));
		return NT_STATUS_ACCESS_DENIED;
	}

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf, 0, 0, True);
	SCVAL(cli->outbuf,smb_com,SMBtcon);
	cli_setup_packet(cli);

	p = smb_buf(cli->outbuf);
	*p++ = 4; p += clistr_push(cli, p, service, -1, STR_TERMINATE | STR_NOALIGN);
	*p++ = 4; p += clistr_push(cli, p, pass, -1, STR_TERMINATE | STR_NOALIGN);
	*p++ = 4; p += clistr_push(cli, p, dev, -1, STR_TERMINATE | STR_NOALIGN);

	cli_setup_bcc(cli, p);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli)) {
		return NT_STATUS_UNEXPECTED_NETWORK_ERROR;
	}

	if (cli_is_error(cli)) {
		return cli_nt_error(cli);
	}

	*max_xmit = SVAL(cli->inbuf, smb_vwv0);
	*tid = SVAL(cli->inbuf, smb_vwv1);

	return NT_STATUS_OK;
}

/* Return a cli_state pointing at the IPC$ share for the given server */

struct cli_state *get_ipc_connect(char *server, struct in_addr *server_ip,
                                         struct user_auth_info *user_info)
{
        struct cli_state *cli;
        pstring myname;
	NTSTATUS nt_status;

        get_myname(myname);
	
	nt_status = cli_full_connection(&cli, myname, server, server_ip, 0, "IPC$", "IPC", 
					user_info->username, lp_workgroup(), user_info->password, 
					CLI_FULL_CONNECTION_ANNONYMOUS_FALLBACK, Undefined, NULL);

	if (NT_STATUS_IS_OK(nt_status)) {
		return cli;
	} else if (is_ipaddress(server)) {
	    /* windows 9* needs a correct NMB name for connections */
	    fstring remote_name;

	    if (name_status_find("*", 0, 0, *server_ip, remote_name)) {
		cli = get_ipc_connect(remote_name, server_ip, user_info);
		if (cli)
		    return cli;
	    }
	}
	return NULL;
}

/*
 * Given the IP address of a master browser on the network, return its
 * workgroup and connect to it.
 *
 * This function is provided to allow additional processing beyond what
 * get_ipc_connect_master_ip_bcast() does, e.g. to retrieve the list of master
 * browsers and obtain each master browsers' list of domains (in case the
 * first master browser is recently on the network and has not yet
 * synchronized with other master browsers and therefore does not yet have the
 * entire network browse list)
 */

struct cli_state *get_ipc_connect_master_ip(struct ip_service * mb_ip, pstring workgroup, struct user_auth_info *user_info)
{
        static fstring name;
	struct cli_state *cli;
	struct in_addr server_ip; 

        DEBUG(99, ("Looking up name of master browser %s\n",
                   inet_ntoa(mb_ip->ip)));

        /*
         * Do a name status query to find out the name of the master browser.
         * We use <01><02>__MSBROWSE__<02>#01 if *#00 fails because a domain
         * master browser will not respond to a wildcard query (or, at least,
         * an NT4 server acting as the domain master browser will not).
         *
         * We might be able to use ONLY the query on MSBROWSE, but that's not
         * yet been tested with all Windows versions, so until it is, leave
         * the original wildcard query as the first choice and fall back to
         * MSBROWSE if the wildcard query fails.
         */
        if (!name_status_find("*", 0, 0x1d, mb_ip->ip, name) &&
            !name_status_find(MSBROWSE, 1, 0x1d, mb_ip->ip, name)) {

                DEBUG(99, ("Could not retrieve name status for %s\n",
                           inet_ntoa(mb_ip->ip)));
                return NULL;
        }

        if (!find_master_ip(name, &server_ip)) {
                DEBUG(99, ("Could not find master ip for %s\n", name));
                return NULL;
        }

                pstrcpy(workgroup, name);

                DEBUG(4, ("found master browser %s, %s\n", 
                  name, inet_ntoa(mb_ip->ip)));

		cli = get_ipc_connect(inet_ntoa(server_ip), &server_ip, user_info);

		return cli;
    
}

/*
 * Return the IP address and workgroup of a master browser on the network, and
 * connect to it.
 */

struct cli_state *get_ipc_connect_master_ip_bcast(pstring workgroup, struct user_auth_info *user_info)
{
	struct ip_service *ip_list;
	struct cli_state *cli;
	int i, count;

        DEBUG(99, ("Do broadcast lookup for workgroups on local network\n"));

        /* Go looking for workgroups by broadcasting on the local network */ 

        if (!name_resolve_bcast(MSBROWSE, 1, &ip_list, &count)) {
                DEBUG(99, ("No master browsers responded\n"));
                return False;
        }

	for (i = 0; i < count; i++) {
            DEBUG(99, ("Found master browser %s\n", inet_ntoa(ip_list[i].ip)));

            cli = get_ipc_connect_master_ip(&ip_list[i], workgroup, user_info);
            if (cli)
                    return(cli);
	}

	return NULL;
}
