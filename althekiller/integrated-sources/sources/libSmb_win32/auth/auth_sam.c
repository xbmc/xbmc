/* 
   Unix SMB/CIFS implementation.
   Password and authentication handling
   Copyright (C) Andrew Tridgell              1992-2000
   Copyright (C) Luke Kenneth Casson Leighton 1996-2000
   Copyright (C) Andrew Bartlett              2001-2003
   Copyright (C) Gerald Carter                2003
   
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

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_AUTH

/****************************************************************************
 Do a specific test for an smb password being correct, given a smb_password and
 the lanman and NT responses.
****************************************************************************/

static NTSTATUS sam_password_ok(const struct auth_context *auth_context,
				TALLOC_CTX *mem_ctx,
				struct samu *sampass, 
				const auth_usersupplied_info *user_info, 
				DATA_BLOB *user_sess_key, 
				DATA_BLOB *lm_sess_key)
{
	uint32 acct_ctrl;
	const uint8 *lm_pw, *nt_pw;
	const char *username = pdb_get_username(sampass);

	acct_ctrl = pdb_get_acct_ctrl(sampass);
	if (acct_ctrl & ACB_PWNOTREQ) {
		if (lp_null_passwords()) {
			DEBUG(3,("Account for user '%s' has no password and null passwords are allowed.\n", username));
			return NT_STATUS_OK;
		} else {
			DEBUG(3,("Account for user '%s' has no password and null passwords are NOT allowed.\n", username));
			return NT_STATUS_LOGON_FAILURE;
		}		
	}

	lm_pw = pdb_get_lanman_passwd(sampass);
	nt_pw = pdb_get_nt_passwd(sampass);

	return ntlm_password_check(mem_ctx, &auth_context->challenge, 
				   &user_info->lm_resp, &user_info->nt_resp, 
				   &user_info->lm_interactive_pwd, &user_info->nt_interactive_pwd,
				   username, 
				   user_info->smb_name,
				   user_info->client_domain,
				   lm_pw, nt_pw, user_sess_key, lm_sess_key);
}

/****************************************************************************
 Check if a user is allowed to logon at this time. Note this is the
 servers local time, as logon hours are just specified as a weekly
 bitmask.
****************************************************************************/
                                                                                                              
static BOOL logon_hours_ok(struct samu *sampass)
{
	/* In logon hours first bit is Sunday from 12AM to 1AM */
	const uint8 *hours;
	struct tm *utctime;
	time_t lasttime;
	const char *asct;
	uint8 bitmask, bitpos;

	hours = pdb_get_hours(sampass);
	if (!hours) {
		DEBUG(5,("logon_hours_ok: No hours restrictions for user %s\n",pdb_get_username(sampass)));
		return True;
	}

	lasttime = time(NULL);
	utctime = gmtime(&lasttime);
	if (!utctime) {
		DEBUG(1, ("logon_hours_ok: failed to get gmtime. Failing logon for user %s\n",
			pdb_get_username(sampass) ));
		return False;
	}

	/* find the corresponding byte and bit */
	bitpos = (utctime->tm_wday * 24 + utctime->tm_hour) % 168;
	bitmask = 1 << (bitpos % 8);

	if (! (hours[bitpos/8] & bitmask)) {
		struct tm *t = localtime(&lasttime);
		if (!t) {
			asct = "INVALID TIME";
		} else {
			asct = asctime(t);
			if (!asct) {
				asct = "INVALID TIME";
			}
		}
		
		DEBUG(1, ("logon_hours_ok: Account for user %s not allowed to "
			  "logon at this time (%s).\n",
			  pdb_get_username(sampass), asct ));
		return False;
	}

	asct = asctime(utctime);
	DEBUG(5,("logon_hours_ok: user %s allowed to logon at this time (%s)\n",
		pdb_get_username(sampass), asct ? asct : "UNKNOWN TIME" ));

	return True;
}

/****************************************************************************
 Do a specific test for a struct samu being vaild for this connection 
 (ie not disabled, expired and the like).
****************************************************************************/

static NTSTATUS sam_account_ok(TALLOC_CTX *mem_ctx,
			       struct samu *sampass, 
			       const auth_usersupplied_info *user_info)
{
	uint32	acct_ctrl = pdb_get_acct_ctrl(sampass);
	char *workstation_list;
	time_t kickoff_time;
	
	DEBUG(4,("sam_account_ok: Checking SMB password for user %s\n",pdb_get_username(sampass)));

	/* Quit if the account was disabled. */
	if (acct_ctrl & ACB_DISABLED) {
		DEBUG(1,("sam_account_ok: Account for user '%s' was disabled.\n", pdb_get_username(sampass)));
		return NT_STATUS_ACCOUNT_DISABLED;
	}

	/* Quit if the account was locked out. */
	if (acct_ctrl & ACB_AUTOLOCK) {
		DEBUG(1,("sam_account_ok: Account for user %s was locked out.\n", pdb_get_username(sampass)));
		return NT_STATUS_ACCOUNT_LOCKED_OUT;
	}

	/* Quit if the account is not allowed to logon at this time. */
	if (! logon_hours_ok(sampass)) {
		return NT_STATUS_INVALID_LOGON_HOURS;
	}

	/* Test account expire time */
	
	kickoff_time = pdb_get_kickoff_time(sampass);
	if (kickoff_time != 0 && time(NULL) > kickoff_time) {
		DEBUG(1,("sam_account_ok: Account for user '%s' has expired.\n", pdb_get_username(sampass)));
		DEBUG(3,("sam_account_ok: Account expired at '%ld' unix time.\n", (long)kickoff_time));
		return NT_STATUS_ACCOUNT_EXPIRED;
	}

	if (!(pdb_get_acct_ctrl(sampass) & ACB_PWNOEXP)) {
		time_t must_change_time = pdb_get_pass_must_change_time(sampass);
		time_t last_set_time = pdb_get_pass_last_set_time(sampass);

		/* check for immediate expiry "must change at next logon" */
		if (must_change_time == 0 && last_set_time != 0) {
			DEBUG(1,("sam_account_ok: Account for user '%s' password must change!.\n", pdb_get_username(sampass)));
			return NT_STATUS_PASSWORD_MUST_CHANGE;
		}

		/* check for expired password */
		if (must_change_time < time(NULL) && must_change_time != 0) {
			DEBUG(1,("sam_account_ok: Account for user '%s' password expired!.\n", pdb_get_username(sampass)));
			DEBUG(1,("sam_account_ok: Password expired at '%s' (%ld) unix time.\n", http_timestring(must_change_time), (long)must_change_time));
			return NT_STATUS_PASSWORD_EXPIRED;
		}
	}

	/* Test workstation. Workstation list is comma separated. */

	workstation_list = talloc_strdup(mem_ctx, pdb_get_workstations(sampass));
	if (!workstation_list)
		return NT_STATUS_NO_MEMORY;

	if (*workstation_list) {
		BOOL invalid_ws = True;
		fstring tok;
		const char *s = workstation_list;

		const char *machine_name = talloc_asprintf(mem_ctx, "%s$", user_info->wksta_name);
		if (machine_name == NULL)
			return NT_STATUS_NO_MEMORY;
			
			
		while (next_token(&s, tok, ",", sizeof(tok))) {
			DEBUG(10,("sam_account_ok: checking for workstation match %s and %s\n",
				  tok, user_info->wksta_name));
			if(strequal(tok, user_info->wksta_name)) {
				invalid_ws = False;
				break;
			}
			if (tok[0] == '+') {
				DEBUG(10,("sam_account_ok: checking for workstation %s in group: %s\n", 
					machine_name, tok + 1));
				if (user_in_group(machine_name, tok + 1)) {
					invalid_ws = False;
					break;
				}
			}
		}
		
		if (invalid_ws) 
			return NT_STATUS_INVALID_WORKSTATION;
	}

	if (acct_ctrl & ACB_DOMTRUST) {
		DEBUG(2,("sam_account_ok: Domain trust account %s denied by server\n", pdb_get_username(sampass)));
		return NT_STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT;
	}
	
	if (acct_ctrl & ACB_SVRTRUST) {
		if (!(user_info->logon_parameters & MSV1_0_ALLOW_SERVER_TRUST_ACCOUNT)) {
			DEBUG(2,("sam_account_ok: Server trust account %s denied by server\n", pdb_get_username(sampass)));
			return NT_STATUS_NOLOGON_SERVER_TRUST_ACCOUNT;
		}
	}

	if (acct_ctrl & ACB_WSTRUST) {
		if (!(user_info->logon_parameters & MSV1_0_ALLOW_WORKSTATION_TRUST_ACCOUNT)) {
			DEBUG(2,("sam_account_ok: Wksta trust account %s denied by server\n", pdb_get_username(sampass)));
			return NT_STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT;
		}
	}
	return NT_STATUS_OK;
}

/****************************************************************************
check if a username/password is OK assuming the password is a 24 byte
SMB hash supplied in the user_info structure
return an NT_STATUS constant.
****************************************************************************/

static NTSTATUS check_sam_security(const struct auth_context *auth_context,
				   void *my_private_data, 
				   TALLOC_CTX *mem_ctx,
				   const auth_usersupplied_info *user_info, 
				   auth_serversupplied_info **server_info)
{
	struct samu *sampass=NULL;
	BOOL ret;
	NTSTATUS nt_status;
	NTSTATUS update_login_attempts_status;
	DATA_BLOB user_sess_key = data_blob(NULL, 0);
	DATA_BLOB lm_sess_key = data_blob(NULL, 0);
	BOOL updated_autolock = False, updated_badpw = False;

	if (!user_info || !auth_context) {
		return NT_STATUS_UNSUCCESSFUL;
	}

	/* Can't use the talloc version here, because the returned struct gets
	   kept on the server_info */

	if ( !(sampass = samu_new( NULL )) ) {
		return NT_STATUS_NO_MEMORY;
	}

	/* get the account information */

	become_root();
	ret = pdb_getsampwnam(sampass, user_info->internal_username);
	unbecome_root();

	if (ret == False) {
		DEBUG(3,("check_sam_security: Couldn't find user '%s' in "
			 "passdb.\n", user_info->internal_username));
		TALLOC_FREE(sampass);
		return NT_STATUS_NO_SUCH_USER;
	}

	/* see if autolock flag needs to be updated */
	if (pdb_get_acct_ctrl(sampass) & ACB_NORMAL)
		pdb_update_autolock_flag(sampass, &updated_autolock);
	/* Quit if the account was locked out. */
	if (pdb_get_acct_ctrl(sampass) & ACB_AUTOLOCK) {
		DEBUG(3,("check_sam_security: Account for user %s was locked out.\n", pdb_get_username(sampass)));
		return NT_STATUS_ACCOUNT_LOCKED_OUT;
	}

	nt_status = sam_password_ok(auth_context, mem_ctx, sampass, 
				    user_info, &user_sess_key, &lm_sess_key);

	/* Notify passdb backend of login success/failure. If not NT_STATUS_OK the backend doesn't like the login */
	update_login_attempts_status = pdb_update_login_attempts(sampass, NT_STATUS_IS_OK(nt_status));
	if (!NT_STATUS_IS_OK(update_login_attempts_status))
		nt_status = update_login_attempts_status;

	if (!NT_STATUS_IS_OK(nt_status)) {
		if (NT_STATUS_EQUAL(nt_status,NT_STATUS_WRONG_PASSWORD) && 
		    pdb_get_acct_ctrl(sampass) &ACB_NORMAL) {  
			pdb_increment_bad_password_count(sampass);
			updated_badpw = True;
		} else {
			pdb_update_bad_password_count(sampass, 
						      &updated_badpw);
		}
		if (updated_autolock || updated_badpw){
			become_root();
			if(!NT_STATUS_IS_OK(pdb_update_sam_account(sampass)))
				DEBUG(1, ("Failed to modify entry.\n"));
			unbecome_root();
		}
		data_blob_free(&user_sess_key);
		data_blob_free(&lm_sess_key);
		TALLOC_FREE(sampass);
		return nt_status;
	}

	if ((pdb_get_acct_ctrl(sampass) & ACB_NORMAL) && 
	    (pdb_get_bad_password_count(sampass) > 0)){
		pdb_set_bad_password_count(sampass, 0, PDB_CHANGED);
		pdb_set_bad_password_time(sampass, 0, PDB_CHANGED);
		updated_badpw = True;
	}

	if (updated_autolock || updated_badpw){
		become_root();
		if(!NT_STATUS_IS_OK(pdb_update_sam_account(sampass)))
			DEBUG(1, ("Failed to modify entry.\n"));
		unbecome_root();
 	}

	nt_status = sam_account_ok(mem_ctx, sampass, user_info);

	if (!NT_STATUS_IS_OK(nt_status)) {
		TALLOC_FREE(sampass);
		data_blob_free(&user_sess_key);
		data_blob_free(&lm_sess_key);
		return nt_status;
	}

	become_root();
	nt_status = make_server_info_sam(server_info, sampass);
	unbecome_root();

	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(0,("check_sam_security: make_server_info_sam() failed with '%s'\n", nt_errstr(nt_status)));
		TALLOC_FREE(sampass);
		data_blob_free(&user_sess_key);
		data_blob_free(&lm_sess_key);
		return nt_status;
	}

	(*server_info)->user_session_key =
		data_blob_talloc(*server_info, user_sess_key.data,
				 user_sess_key.length);
	data_blob_free(&user_sess_key);

	(*server_info)->lm_session_key =
		data_blob_talloc(*server_info, lm_sess_key.data,
				 lm_sess_key.length);
	data_blob_free(&lm_sess_key);

	(*server_info)->was_mapped |= user_info->was_mapped;

	return nt_status;
}

/* module initialisation */
static NTSTATUS auth_init_sam_ignoredomain(struct auth_context *auth_context, const char *param, auth_methods **auth_method) 
{
	if (!make_auth_methods(auth_context, auth_method)) {
		return NT_STATUS_NO_MEMORY;
	}

	(*auth_method)->auth = check_sam_security;	
	(*auth_method)->name = "sam_ignoredomain";
	return NT_STATUS_OK;
}


/****************************************************************************
Check SAM security (above) but with a few extra checks.
****************************************************************************/

static NTSTATUS check_samstrict_security(const struct auth_context *auth_context,
					 void *my_private_data, 
					 TALLOC_CTX *mem_ctx,
					 const auth_usersupplied_info *user_info, 
					 auth_serversupplied_info **server_info)
{
	BOOL is_local_name, is_my_domain;

	if (!user_info || !auth_context) {
		return NT_STATUS_LOGON_FAILURE;
	}

	is_local_name = is_myname(user_info->domain);
	is_my_domain  = strequal(user_info->domain, lp_workgroup());

	/* check whether or not we service this domain/workgroup name */
	
	switch ( lp_server_role() ) {
		case ROLE_STANDALONE:
		case ROLE_DOMAIN_MEMBER:
			if ( !is_local_name ) {
				DEBUG(6,("check_samstrict_security: %s is not one of my local names (%s)\n",
					user_info->domain, (lp_server_role() == ROLE_DOMAIN_MEMBER 
					? "ROLE_DOMAIN_MEMBER" : "ROLE_STANDALONE") ));
				return NT_STATUS_NOT_IMPLEMENTED;
			}
		case ROLE_DOMAIN_PDC:
		case ROLE_DOMAIN_BDC:
			if ( !is_local_name && !is_my_domain ) {
				DEBUG(6,("check_samstrict_security: %s is not one of my local names or domain name (DC)\n",
					user_info->domain));
				return NT_STATUS_NOT_IMPLEMENTED;
			}
		default: /* name is ok */
			break;
	}
	
	return check_sam_security(auth_context, my_private_data, mem_ctx, user_info, server_info);
}

/* module initialisation */
static NTSTATUS auth_init_sam(struct auth_context *auth_context, const char *param, auth_methods **auth_method) 
{
	if (!make_auth_methods(auth_context, auth_method)) {
		return NT_STATUS_NO_MEMORY;
	}

	(*auth_method)->auth = check_samstrict_security;
	(*auth_method)->name = "sam";
	return NT_STATUS_OK;
}

NTSTATUS auth_sam_init(void)
{
	smb_register_auth(AUTH_INTERFACE_VERSION, "sam", auth_init_sam);
	smb_register_auth(AUTH_INTERFACE_VERSION, "sam_ignoredomain", auth_init_sam_ignoredomain);
	return NT_STATUS_OK;
}
