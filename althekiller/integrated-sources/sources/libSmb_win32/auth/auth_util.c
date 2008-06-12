/*
   Unix SMB/CIFS implementation.
   Authentication utility functions
   Copyright (C) Andrew Tridgell 1992-1998
   Copyright (C) Andrew Bartlett 2001
   Copyright (C) Jeremy Allison 2000-2001
   Copyright (C) Rafal Szczesniak 2002
   Copyright (C) Volker Lendecke 2006

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

static struct nt_user_token *create_local_nt_token(TALLOC_CTX *mem_ctx,
						   const DOM_SID *user_sid,
						   BOOL is_guest,
						   int num_groupsids,
						   const DOM_SID *groupsids);

/****************************************************************************
 Create a UNIX user on demand.
****************************************************************************/

static int smb_create_user(const char *domain, const char *unix_username, const char *homedir)
{
	pstring add_script;
	int ret;

	pstrcpy(add_script, lp_adduser_script());
	if (! *add_script)
		return -1;
	all_string_sub(add_script, "%u", unix_username, sizeof(pstring));
	if (domain)
		all_string_sub(add_script, "%D", domain, sizeof(pstring));
	if (homedir)
		all_string_sub(add_script, "%H", homedir, sizeof(pstring));
	ret = smbrun(add_script,NULL);
	flush_pwnam_cache();
	DEBUG(ret ? 0 : 3,("smb_create_user: Running the command `%s' gave %d\n",add_script,ret));
	return ret;
}

/****************************************************************************
 Create an auth_usersupplied_data structure
****************************************************************************/

static NTSTATUS make_user_info(auth_usersupplied_info **user_info, 
                               const char *smb_name, 
                               const char *internal_username,
                               const char *client_domain, 
                               const char *domain,
                               const char *wksta_name, 
                               DATA_BLOB *lm_pwd, DATA_BLOB *nt_pwd,
                               DATA_BLOB *lm_interactive_pwd, DATA_BLOB *nt_interactive_pwd,
                               DATA_BLOB *plaintext, 
                               BOOL encrypted)
{

	DEBUG(5,("attempting to make a user_info for %s (%s)\n", internal_username, smb_name));

	*user_info = SMB_MALLOC_P(auth_usersupplied_info);
	if (*user_info == NULL) {
		DEBUG(0,("malloc failed for user_info (size %lu)\n", (unsigned long)sizeof(*user_info)));
		return NT_STATUS_NO_MEMORY;
	}

	ZERO_STRUCTP(*user_info);

	DEBUG(5,("making strings for %s's user_info struct\n", internal_username));

	(*user_info)->smb_name = SMB_STRDUP(smb_name);
	if ((*user_info)->smb_name == NULL) { 
		free_user_info(user_info);
		return NT_STATUS_NO_MEMORY;
	}
	
	(*user_info)->internal_username = SMB_STRDUP(internal_username);
	if ((*user_info)->internal_username == NULL) { 
		free_user_info(user_info);
		return NT_STATUS_NO_MEMORY;
	}

	(*user_info)->domain = SMB_STRDUP(domain);
	if ((*user_info)->domain == NULL) { 
		free_user_info(user_info);
		return NT_STATUS_NO_MEMORY;
	}

	(*user_info)->client_domain = SMB_STRDUP(client_domain);
	if ((*user_info)->client_domain == NULL) { 
		free_user_info(user_info);
		return NT_STATUS_NO_MEMORY;
	}

	(*user_info)->wksta_name = SMB_STRDUP(wksta_name);
	if ((*user_info)->wksta_name == NULL) { 
		free_user_info(user_info);
		return NT_STATUS_NO_MEMORY;
	}

	DEBUG(5,("making blobs for %s's user_info struct\n", internal_username));

	if (lm_pwd)
		(*user_info)->lm_resp = data_blob(lm_pwd->data, lm_pwd->length);
	if (nt_pwd)
		(*user_info)->nt_resp = data_blob(nt_pwd->data, nt_pwd->length);
	if (lm_interactive_pwd)
		(*user_info)->lm_interactive_pwd = data_blob(lm_interactive_pwd->data, lm_interactive_pwd->length);
	if (nt_interactive_pwd)
		(*user_info)->nt_interactive_pwd = data_blob(nt_interactive_pwd->data, nt_interactive_pwd->length);

	if (plaintext)
		(*user_info)->plaintext_password = data_blob(plaintext->data, plaintext->length);

	(*user_info)->encrypted = encrypted;

	(*user_info)->logon_parameters = 0;

	DEBUG(10,("made an %sencrypted user_info for %s (%s)\n", encrypted ? "":"un" , internal_username, smb_name));

	return NT_STATUS_OK;
}

/****************************************************************************
 Create an auth_usersupplied_data structure after appropriate mapping.
****************************************************************************/

NTSTATUS make_user_info_map(auth_usersupplied_info **user_info, 
			    const char *smb_name, 
			    const char *client_domain, 
			    const char *wksta_name, 
 			    DATA_BLOB *lm_pwd, DATA_BLOB *nt_pwd,
 			    DATA_BLOB *lm_interactive_pwd, DATA_BLOB *nt_interactive_pwd,
			    DATA_BLOB *plaintext, 
			    BOOL encrypted)
{
	const char *domain;
	NTSTATUS result;
	BOOL was_mapped;
	fstring internal_username;
	fstrcpy(internal_username, smb_name);
	was_mapped = map_username(internal_username); 
	
	DEBUG(5, ("make_user_info_map: Mapping user [%s]\\[%s] from workstation [%s]\n",
	      client_domain, smb_name, wksta_name));
	
	/* don't allow "" as a domain, fixes a Win9X bug 
	   where it doens't supply a domain for logon script
	   'net use' commands.                                 */

	if ( *client_domain )
		domain = client_domain;
	else
		domain = lp_workgroup();

	/* do what win2k does.  Always map unknown domains to our own
	   and let the "passdb backend" handle unknown users. */

	if ( !is_trusted_domain(domain) && !strequal(domain, get_global_sam_name()) ) 
		domain = my_sam_name();
	
	/* we know that it is a trusted domain (and we are allowing them) or it is our domain */
	
	result = make_user_info(user_info, smb_name, internal_username, 
			      client_domain, domain, wksta_name, 
			      lm_pwd, nt_pwd,
			      lm_interactive_pwd, nt_interactive_pwd,
			      plaintext, encrypted);
	if (NT_STATUS_IS_OK(result)) {
		(*user_info)->was_mapped = was_mapped;
	}
	return result;
}

/****************************************************************************
 Create an auth_usersupplied_data, making the DATA_BLOBs here. 
 Decrypt and encrypt the passwords.
****************************************************************************/

BOOL make_user_info_netlogon_network(auth_usersupplied_info **user_info, 
				     const char *smb_name, 
				     const char *client_domain, 
				     const char *wksta_name, 
				     uint32 logon_parameters,
				     const uchar *lm_network_pwd,
				     int lm_pwd_len,
				     const uchar *nt_network_pwd,
				     int nt_pwd_len)
{
	BOOL ret;
	NTSTATUS status;
	DATA_BLOB lm_blob = data_blob(lm_network_pwd, lm_pwd_len);
	DATA_BLOB nt_blob = data_blob(nt_network_pwd, nt_pwd_len);

	status = make_user_info_map(user_info,
				    smb_name, client_domain, 
				    wksta_name, 
				    lm_pwd_len ? &lm_blob : NULL, 
				    nt_pwd_len ? &nt_blob : NULL,
				    NULL, NULL, NULL,
				    True);

	if (NT_STATUS_IS_OK(status)) {
		(*user_info)->logon_parameters = logon_parameters;
	}
	ret = NT_STATUS_IS_OK(status) ? True : False;

	data_blob_free(&lm_blob);
	data_blob_free(&nt_blob);
	return ret;
}

/****************************************************************************
 Create an auth_usersupplied_data, making the DATA_BLOBs here. 
 Decrypt and encrypt the passwords.
****************************************************************************/

BOOL make_user_info_netlogon_interactive(auth_usersupplied_info **user_info, 
					 const char *smb_name, 
					 const char *client_domain, 
					 const char *wksta_name, 
					 uint32 logon_parameters,
					 const uchar chal[8], 
					 const uchar lm_interactive_pwd[16], 
					 const uchar nt_interactive_pwd[16], 
					 const uchar *dc_sess_key)
{
	char lm_pwd[16];
	char nt_pwd[16];
	unsigned char local_lm_response[24];
	unsigned char local_nt_response[24];
	unsigned char key[16];
	
	ZERO_STRUCT(key);
	memcpy(key, dc_sess_key, 8);
	
	if (lm_interactive_pwd)
		memcpy(lm_pwd, lm_interactive_pwd, sizeof(lm_pwd));

	if (nt_interactive_pwd)
		memcpy(nt_pwd, nt_interactive_pwd, sizeof(nt_pwd));
	
#ifdef DEBUG_PASSWORD
	DEBUG(100,("key:"));
	dump_data(100, (char *)key, sizeof(key));
	
	DEBUG(100,("lm owf password:"));
	dump_data(100, lm_pwd, sizeof(lm_pwd));
	
	DEBUG(100,("nt owf password:"));
	dump_data(100, nt_pwd, sizeof(nt_pwd));
#endif
	
	if (lm_interactive_pwd)
		SamOEMhash((uchar *)lm_pwd, key, sizeof(lm_pwd));
	
	if (nt_interactive_pwd)
		SamOEMhash((uchar *)nt_pwd, key, sizeof(nt_pwd));
	
#ifdef DEBUG_PASSWORD
	DEBUG(100,("decrypt of lm owf password:"));
	dump_data(100, lm_pwd, sizeof(lm_pwd));
	
	DEBUG(100,("decrypt of nt owf password:"));
	dump_data(100, nt_pwd, sizeof(nt_pwd));
#endif
	
	if (lm_interactive_pwd)
		SMBOWFencrypt((const unsigned char *)lm_pwd, chal,
			      local_lm_response);

	if (nt_interactive_pwd)
		SMBOWFencrypt((const unsigned char *)nt_pwd, chal,
			      local_nt_response);
	
	/* Password info paranoia */
	ZERO_STRUCT(key);

	{
		BOOL ret;
		NTSTATUS nt_status;
		DATA_BLOB local_lm_blob;
		DATA_BLOB local_nt_blob;

		DATA_BLOB lm_interactive_blob;
		DATA_BLOB nt_interactive_blob;
		
		if (lm_interactive_pwd) {
			local_lm_blob = data_blob(local_lm_response,
						  sizeof(local_lm_response));
			lm_interactive_blob = data_blob(lm_pwd,
							sizeof(lm_pwd));
			ZERO_STRUCT(lm_pwd);
		}
		
		if (nt_interactive_pwd) {
			local_nt_blob = data_blob(local_nt_response,
						  sizeof(local_nt_response));
			nt_interactive_blob = data_blob(nt_pwd,
							sizeof(nt_pwd));
			ZERO_STRUCT(nt_pwd);
		}

		nt_status = make_user_info_map(
			user_info, 
			smb_name, client_domain, wksta_name, 
			lm_interactive_pwd ? &local_lm_blob : NULL,
			nt_interactive_pwd ? &local_nt_blob : NULL,
			lm_interactive_pwd ? &lm_interactive_blob : NULL,
			nt_interactive_pwd ? &nt_interactive_blob : NULL,
			NULL, True);

		if (NT_STATUS_IS_OK(nt_status)) {
			(*user_info)->logon_parameters = logon_parameters;
		}

		ret = NT_STATUS_IS_OK(nt_status) ? True : False;
		data_blob_free(&local_lm_blob);
		data_blob_free(&local_nt_blob);
		data_blob_free(&lm_interactive_blob);
		data_blob_free(&nt_interactive_blob);
		return ret;
	}
}


/****************************************************************************
 Create an auth_usersupplied_data structure
****************************************************************************/

BOOL make_user_info_for_reply(auth_usersupplied_info **user_info, 
			      const char *smb_name, 
			      const char *client_domain,
			      const uint8 chal[8],
			      DATA_BLOB plaintext_password)
{

	DATA_BLOB local_lm_blob;
	DATA_BLOB local_nt_blob;
	NTSTATUS ret = NT_STATUS_UNSUCCESSFUL;
			
	/*
	 * Not encrypted - do so.
	 */
	
	DEBUG(5,("make_user_info_for_reply: User passwords not in encrypted "
		 "format.\n"));
	
	if (plaintext_password.data) {
		unsigned char local_lm_response[24];
		
#ifdef DEBUG_PASSWORD
		DEBUG(10,("Unencrypted password (len %d):\n",
			  (int)plaintext_password.length));
		dump_data(100, (const char *)plaintext_password.data,
			  plaintext_password.length);
#endif

		SMBencrypt( (const char *)plaintext_password.data,
			    (const uchar*)chal, local_lm_response);
		local_lm_blob = data_blob(local_lm_response, 24);
		
		/* We can't do an NT hash here, as the password needs to be
		   case insensitive */
		local_nt_blob = data_blob(NULL, 0); 
		
	} else {
		local_lm_blob = data_blob(NULL, 0); 
		local_nt_blob = data_blob(NULL, 0); 
	}
	
	ret = make_user_info_map(
		user_info, smb_name, client_domain, 
		get_remote_machine_name(),
		local_lm_blob.data ? &local_lm_blob : NULL,
		local_nt_blob.data ? &local_nt_blob : NULL,
		NULL, NULL,
		plaintext_password.data ? &plaintext_password : NULL, 
		False);
	
	data_blob_free(&local_lm_blob);
	return NT_STATUS_IS_OK(ret) ? True : False;
}

/****************************************************************************
 Create an auth_usersupplied_data structure
****************************************************************************/

NTSTATUS make_user_info_for_reply_enc(auth_usersupplied_info **user_info, 
                                      const char *smb_name,
                                      const char *client_domain, 
                                      DATA_BLOB lm_resp, DATA_BLOB nt_resp)
{
	return make_user_info_map(user_info, smb_name, 
				  client_domain, 
				  get_remote_machine_name(), 
				  lm_resp.data ? &lm_resp : NULL, 
				  nt_resp.data ? &nt_resp : NULL, 
				  NULL, NULL, NULL,
				  True);
}

/****************************************************************************
 Create a guest user_info blob, for anonymous authenticaion.
****************************************************************************/

BOOL make_user_info_guest(auth_usersupplied_info **user_info) 
{
	NTSTATUS nt_status;

	nt_status = make_user_info(user_info, 
				   "","", 
				   "","", 
				   "", 
				   NULL, NULL, 
				   NULL, NULL, 
				   NULL,
				   True);
			      
	return NT_STATUS_IS_OK(nt_status) ? True : False;
}

/****************************************************************************
 prints a NT_USER_TOKEN to debug output.
****************************************************************************/

void debug_nt_user_token(int dbg_class, int dbg_lev, NT_USER_TOKEN *token)
{
	size_t     i;
	
	if (!token) {
		DEBUGC(dbg_class, dbg_lev, ("NT user token: (NULL)\n"));
		return;
	}
	
	DEBUGC(dbg_class, dbg_lev,
	       ("NT user token of user %s\n",
		sid_string_static(&token->user_sids[0]) ));
	DEBUGADDC(dbg_class, dbg_lev,
		  ("contains %lu SIDs\n", (unsigned long)token->num_sids));
	for (i = 0; i < token->num_sids; i++)
		DEBUGADDC(dbg_class, dbg_lev,
			  ("SID[%3lu]: %s\n", (unsigned long)i, 
			   sid_string_static(&token->user_sids[i])));

	dump_se_priv( dbg_class, dbg_lev, &token->privileges );
}

/****************************************************************************
 prints a UNIX 'token' to debug output.
****************************************************************************/

void debug_unix_user_token(int dbg_class, int dbg_lev, uid_t uid, gid_t gid,
			   int n_groups, gid_t *groups)
{
	int     i;
	DEBUGC(dbg_class, dbg_lev,
	       ("UNIX token of user %ld\n", (long int)uid));

	DEBUGADDC(dbg_class, dbg_lev,
		  ("Primary group is %ld and contains %i supplementary "
		   "groups\n", (long int)gid, n_groups));
	for (i = 0; i < n_groups; i++)
		DEBUGADDC(dbg_class, dbg_lev, ("Group[%3i]: %ld\n", i, 
			(long int)groups[i]));
}

/******************************************************************************
 Create a token for the root user to be used internally by smbd.
 This is similar to running under the context of the LOCAL_SYSTEM account
 in Windows.  This is a read-only token.  Do not modify it or free() it.
 Create a copy if your need to change it.
******************************************************************************/

NT_USER_TOKEN *get_root_nt_token( void )
{
	static NT_USER_TOKEN *token = NULL;
	DOM_SID u_sid, g_sid;
	struct passwd *pw;
	
	if ( token )
		return token;
		
	if ( !(pw = sys_getpwnam( "root" )) ) {
		DEBUG(0,("get_root_nt_token: getpwnam\"root\") failed!\n"));
		return NULL;
	}
	
	/* get the user and primary group SIDs; although the 
	   BUILTIN\Administrators SId is really the one that matters here */
	   
	uid_to_sid(&u_sid, pw->pw_uid);
	gid_to_sid(&g_sid, pw->pw_gid);

	token = create_local_nt_token(NULL, &u_sid, False,
				      1, &global_sid_Builtin_Administrators);
	return token;
}

static int server_info_dtor(void *p)
{
	auth_serversupplied_info *server_info =
		talloc_get_type_abort(p, auth_serversupplied_info);

	if (server_info->sam_account != NULL) {
		TALLOC_FREE(server_info->sam_account);
	}

	ZERO_STRUCTP(server_info);
	return 0;
}

/***************************************************************************
 Make a server_info struct. Free with TALLOC_FREE().
***************************************************************************/

static auth_serversupplied_info *make_server_info(TALLOC_CTX *mem_ctx)
{
	struct auth_serversupplied_info *result;

	result = TALLOC_ZERO_P(mem_ctx, auth_serversupplied_info);
	if (result == NULL) {
		DEBUG(0, ("talloc failed\n"));
		return NULL;
	}

	talloc_set_destructor(result, server_info_dtor);

	/* Initialise the uid and gid values to something non-zero
	   which may save us from giving away root access if there
	   is a bug in allocating these fields. */

	result->uid = -1;
	result->gid = -1;
	return result;
}

/***************************************************************************
 Make (and fill) a user_info struct from a struct samu
***************************************************************************/

NTSTATUS make_server_info_sam(auth_serversupplied_info **server_info, 
			      struct samu *sampass)
{
	NTSTATUS status;
	struct passwd *pwd;
	gid_t *gids;
	auth_serversupplied_info *result;
	int i;
	size_t num_gids;
	DOM_SID unix_group_sid;
	

	if ( !(pwd = getpwnam_alloc(NULL, pdb_get_username(sampass))) ) {
		DEBUG(1, ("User %s in passdb, but getpwnam() fails!\n",
			  pdb_get_username(sampass)));
		return NT_STATUS_NO_SUCH_USER;
	}

	if ( !(result = make_server_info(NULL)) ) {
		TALLOC_FREE(pwd);
		return NT_STATUS_NO_MEMORY;
	}

	result->sam_account = sampass;
	result->unix_name = talloc_strdup(result, pwd->pw_name);
	result->gid = pwd->pw_gid;
	result->uid = pwd->pw_uid;
	
	TALLOC_FREE(pwd);

	status = pdb_enum_group_memberships(result, sampass,
					    &result->sids, &gids,
					    &result->num_sids);

	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(10, ("pdb_enum_group_memberships failed: %s\n",
			   nt_errstr(status)));
		result->sam_account = NULL; /* Don't free on error exit. */
		TALLOC_FREE(result);
		return status;
	}
	
	/* Add the "Unix Group" SID for each gid to catch mapped groups
	   and their Unix equivalent.  This is to solve the backwards 
	   compatibility problem of 'valid users = +ntadmin' where 
	   ntadmin has been paired with "Domain Admins" in the group 
	   mapping table.  Otherwise smb.conf would need to be changed
	   to 'valid user = "Domain Admins"'.  --jerry */
	
	num_gids = result->num_sids;
	for ( i=0; i<num_gids; i++ ) {
		if ( !gid_to_unix_groups_sid( gids[i], &unix_group_sid ) ) {
			DEBUG(1,("make_server_info_sam: Failed to create SID "
				"for gid %d!\n", gids[i]));
			continue;
		}
		add_sid_to_array_unique( result, &unix_group_sid,
			&result->sids, &result->num_sids );
	}

	/* For now we throw away the gids and convert via sid_to_gid
	 * later. This needs fixing, but I'd like to get the code straight and
	 * simple first. */
	TALLOC_FREE(gids);

	DEBUG(5,("make_server_info_sam: made server info for user %s -> %s\n",
		 pdb_get_username(sampass), result->unix_name));

	*server_info = result;

	return NT_STATUS_OK;
}

/*
 * Add alias SIDs from memberships within the partially created token SID list
 */

static NTSTATUS add_aliases(TALLOC_CTX *tmp_ctx, const DOM_SID *domain_sid,
			    struct nt_user_token *token)
{
	uint32 *aliases;
	size_t i, num_aliases;
	NTSTATUS status;

	aliases = NULL;
	num_aliases = 0;

	status = pdb_enum_alias_memberships(tmp_ctx, domain_sid,
					    token->user_sids,
					    token->num_sids,
					    &aliases, &num_aliases);

	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(10, ("pdb_enum_alias_memberships failed: %s\n",
			   nt_errstr(status)));
		return status;
	}

	for (i=0; i<num_aliases; i++) {
		DOM_SID alias_sid;
		sid_compose(&alias_sid, domain_sid, aliases[i]);
		add_sid_to_array_unique(token, &alias_sid,
					&token->user_sids,
					&token->num_sids);
		if (token->user_sids == NULL) {
			DEBUG(0, ("add_sid_to_array failed\n"));
			return NT_STATUS_NO_MEMORY;
		}
	}

	return NT_STATUS_OK;
}

static NTSTATUS log_nt_token(TALLOC_CTX *tmp_ctx, NT_USER_TOKEN *token)
{
	char *command;
	char *group_sidstr;
	size_t i;

	if ((lp_log_nt_token_command() == NULL) ||
	    (strlen(lp_log_nt_token_command()) == 0)) {
		return NT_STATUS_OK;
	}

	group_sidstr = talloc_strdup(tmp_ctx, "");
	for (i=1; i<token->num_sids; i++) {
		group_sidstr = talloc_asprintf(
			tmp_ctx, "%s %s", group_sidstr,
			sid_string_static(&token->user_sids[i]));
	}

	command = talloc_string_sub(
		tmp_ctx, lp_log_nt_token_command(),
		"%s", sid_string_static(&token->user_sids[0]));
	command = talloc_string_sub(tmp_ctx, command, "%t", group_sidstr);

	if (command == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	DEBUG(8, ("running command: [%s]\n", command));
	if (smbrun(command, NULL) != 0) {
		DEBUG(0, ("Could not log NT token\n"));
		return NT_STATUS_ACCESS_DENIED;
	}

	return NT_STATUS_OK;
}

/*******************************************************************
*******************************************************************/

static NTSTATUS add_builtin_administrators( TALLOC_CTX *ctx, struct nt_user_token *token )
{
	DOM_SID domadm;

	/* nothing to do if we aren't in a domain */
	
	if ( !(IS_DC || lp_server_role()==ROLE_DOMAIN_MEMBER) ) {
		return NT_STATUS_OK;
	}
	
	/* Find the Domain Admins SID */
	
	if ( IS_DC ) {
		sid_copy( &domadm, get_global_sam_sid() );
	} else {
		if ( !secrets_fetch_domain_sid( lp_workgroup(), &domadm ) )
			return NT_STATUS_CANT_ACCESS_DOMAIN_INFO;
	}
	sid_append_rid( &domadm, DOMAIN_GROUP_RID_ADMINS );
	
	/* Add Administrators if the user beloongs to Domain Admins */
	
	if ( nt_token_check_sid( &domadm, token ) ) {
		add_sid_to_array(token, &global_sid_Builtin_Administrators,
				 &token->user_sids, &token->num_sids);
	}
	
	return NT_STATUS_OK;
}

/*******************************************************************
*******************************************************************/

static NTSTATUS create_builtin_users( void )
{
	NTSTATUS status;
	DOM_SID dom_users;

	status = pdb_create_builtin_alias( BUILTIN_ALIAS_RID_USERS );
	if ( !NT_STATUS_IS_OK(status) ) {
		DEBUG(0,("create_builtin_users: Failed to create Users\n"));
		return status;
	}
	
	/* add domain users */
	if ((IS_DC || (lp_server_role() == ROLE_DOMAIN_MEMBER)) 
		&& secrets_fetch_domain_sid(lp_workgroup(), &dom_users))
	{
		sid_append_rid(&dom_users, DOMAIN_GROUP_RID_USERS );
		status = pdb_add_aliasmem( &global_sid_Builtin_Users, &dom_users);
		if ( !NT_STATUS_IS_OK(status) ) {
			DEBUG(0,("create_builtin_administrators: Failed to add Domain Users to"
				" Users\n"));
			return status;
		}
	}
			
	return NT_STATUS_OK;
}		

/*******************************************************************
*******************************************************************/

static NTSTATUS create_builtin_administrators( void )
{
	NTSTATUS status;
	DOM_SID dom_admins, root_sid;
	fstring root_name;
	enum SID_NAME_USE type;		
	TALLOC_CTX *ctx;
	BOOL ret;

	status = pdb_create_builtin_alias( BUILTIN_ALIAS_RID_ADMINS );
	if ( !NT_STATUS_IS_OK(status) ) {
		DEBUG(0,("create_builtin_administrators: Failed to create Administrators\n"));
		return status;
	}
	
	/* add domain admins */
	if ((IS_DC || (lp_server_role() == ROLE_DOMAIN_MEMBER)) 
		&& secrets_fetch_domain_sid(lp_workgroup(), &dom_admins))
	{
		sid_append_rid(&dom_admins, DOMAIN_GROUP_RID_ADMINS);
		status = pdb_add_aliasmem( &global_sid_Builtin_Administrators, &dom_admins );
		if ( !NT_STATUS_IS_OK(status) ) {
			DEBUG(0,("create_builtin_administrators: Failed to add Domain Admins"
				" Administrators\n"));
			return status;
		}
	}
			
	/* add root */
	if ( (ctx = talloc_init("create_builtin_administrators")) == NULL ) {
		return NT_STATUS_NO_MEMORY;
	}
	fstr_sprintf( root_name, "%s\\root", get_global_sam_name() );
	ret = lookup_name( ctx, root_name, 0, NULL, NULL, &root_sid, &type );
	TALLOC_FREE( ctx );

	if ( ret ) {
		status = pdb_add_aliasmem( &global_sid_Builtin_Administrators, &root_sid );
		if ( !NT_STATUS_IS_OK(status) ) {
			DEBUG(0,("create_builtin_administrators: Failed to add root"
				" Administrators\n"));
			return status;
		}
	}
	
	return NT_STATUS_OK;
}		

/*******************************************************************
 Create a NT token for the user, expanding local aliases
*******************************************************************/

static struct nt_user_token *create_local_nt_token(TALLOC_CTX *mem_ctx,
						   const DOM_SID *user_sid,
						   BOOL is_guest,
						   int num_groupsids,
						   const DOM_SID *groupsids)
{
	TALLOC_CTX *tmp_ctx;
	struct nt_user_token *result = NULL;
	int i;
	NTSTATUS status;
	gid_t gid;

	tmp_ctx = talloc_new(mem_ctx);
	if (tmp_ctx == NULL) {
		DEBUG(0, ("talloc_new failed\n"));
		return NULL;
	}

	result = TALLOC_ZERO_P(tmp_ctx, NT_USER_TOKEN);
	if (result == NULL) {
		DEBUG(0, ("talloc failed\n"));
		goto done;
	}

	/* Add the user and primary group sid */

	add_sid_to_array(result, user_sid,
			 &result->user_sids, &result->num_sids);

	/* For guest, num_groupsids may be zero. */
	if (num_groupsids) {
		add_sid_to_array(result, &groupsids[0],
				 &result->user_sids, &result->num_sids);
	}
			 
	/* Add in BUILTIN sids */
	
	add_sid_to_array(result, &global_sid_World,
			 &result->user_sids, &result->num_sids);
	add_sid_to_array(result, &global_sid_Network,
			 &result->user_sids, &result->num_sids);

	if (is_guest) {
		add_sid_to_array(result, &global_sid_Builtin_Guests,
				 &result->user_sids, &result->num_sids);
	} else {
		add_sid_to_array(result, &global_sid_Authenticated_Users,
				 &result->user_sids, &result->num_sids);
	}
	
	/* Now the SIDs we got from authentication. These are the ones from
	 * the info3 struct or from the pdb_enum_group_memberships, depending
	 * on who authenticated the user.
	 * Note that we start the for loop at "1" here, we already added the
	 * first group sid as primary above. */

	for (i=1; i<num_groupsids; i++) {
		add_sid_to_array_unique(result, &groupsids[i],
					&result->user_sids, &result->num_sids);
	}
	
	/* Deal with the BUILTIN\Administrators group.  If the SID can
	   be resolved then assume that the add_aliasmem( S-1-5-32 ) 
	   handled it. */

	if ( !sid_to_gid( &global_sid_Builtin_Administrators, &gid ) ) {
		/* We can only create a mapping if winbind is running 
		   and the nested group functionality has been enabled */
		   
		if ( lp_winbind_nested_groups() && winbind_ping() ) {
			become_root();
			status = create_builtin_administrators( );
			if ( !NT_STATUS_IS_OK(status) ) {
				DEBUG(2,("create_local_nt_token: Failed to create BUILTIN\\Administrators group!\n"));
				/* don't fail, just log the message */
			}
			unbecome_root();
		}
		else {
			status = add_builtin_administrators( tmp_ctx, result );	
			if ( !NT_STATUS_IS_OK(status) ) {
				/* just log a complaint but do not fail */
				DEBUG(3,("create_local_nt_token: failed to check for local Administrators"
					" membership (%s)\n", nt_errstr(status)));
			}			
		}		
	}

	/* Deal with the BUILTIN\Users group.  If the SID can
	   be resolved then assume that the add_aliasmem( S-1-5-32 ) 
	   handled it. */

	if ( !sid_to_gid( &global_sid_Builtin_Users, &gid ) ) {
		/* We can only create a mapping if winbind is running 
		   and the nested group functionality has been enabled */
		   
		if ( lp_winbind_nested_groups() && winbind_ping() ) {
			become_root();
			status = create_builtin_users( );
			if ( !NT_STATUS_IS_OK(status) ) {
				DEBUG(2,("create_local_nt_token: Failed to create BUILTIN\\Users group!\n"));
				/* don't fail, just log the message */
			}
			unbecome_root();
		}
	}

	/* Deal with local groups */
	
	if (lp_winbind_nested_groups()) {

		/* Now add the aliases. First the one from our local SAM */

		status = add_aliases(tmp_ctx, get_global_sam_sid(), result);

		if (!NT_STATUS_IS_OK(status)) {
			result = NULL;
			goto done;
		}

		/* Finally the builtin ones */

		status = add_aliases(tmp_ctx, &global_sid_Builtin, result);

		if (!NT_STATUS_IS_OK(status)) {
			result = NULL;
			goto done;
		}
	} 


	get_privileges_for_sids(&result->privileges, result->user_sids,
				result->num_sids);

	talloc_steal(mem_ctx, result);

 done:
	TALLOC_FREE(tmp_ctx);
	return result;
}

/*
 * Create the token to use from server_info->sam_account and
 * server_info->sids (the info3/sam groups). Find the unix gids.
 */

NTSTATUS create_local_token(auth_serversupplied_info *server_info)
{
	TALLOC_CTX *mem_ctx;
	NTSTATUS status;
	size_t i;
	

	mem_ctx = talloc_new(NULL);
	if (mem_ctx == NULL) {
		DEBUG(0, ("talloc_new failed\n"));
		return NT_STATUS_NO_MEMORY;
	}

	if (((lp_server_role() == ROLE_DOMAIN_MEMBER) && !winbind_ping()) ||
	    (server_info->was_mapped)) {
		status = create_token_from_username(server_info,
						    server_info->unix_name,
						    server_info->guest,
						    &server_info->uid,
						    &server_info->gid,
						    &server_info->unix_name,
						    &server_info->ptok);
		
	} else {
		server_info->ptok = create_local_nt_token(
			server_info,
			pdb_get_user_sid(server_info->sam_account),
			server_info->guest,
			server_info->num_sids, server_info->sids);
		status = server_info->ptok ?
			NT_STATUS_OK : NT_STATUS_NO_SUCH_USER;
	}

	if (!NT_STATUS_IS_OK(status)) {
		TALLOC_FREE(mem_ctx);
		return status;
	}
	
	/* Convert the SIDs to gids. */

	server_info->n_groups = 0;
	server_info->groups = NULL;

	/* Start at index 1, where the groups start. */

	for (i=1; i<server_info->ptok->num_sids; i++) {
		gid_t gid;
		DOM_SID *sid = &server_info->ptok->user_sids[i];

		if (!sid_to_gid(sid, &gid)) {
			DEBUG(10, ("Could not convert SID %s to gid, "
				   "ignoring it\n", sid_string_static(sid)));
			continue;
		}
		add_gid_to_array_unique(server_info, gid, &server_info->groups,
					&server_info->n_groups);
	}
	
	debug_nt_user_token(DBGC_AUTH, 10, server_info->ptok);

	status = log_nt_token(mem_ctx, server_info->ptok);

	TALLOC_FREE(mem_ctx);
	return status;
}

/*
 * Create an artificial NT token given just a username. (Initially indended
 * for force user)
 *
 * We go through lookup_name() to avoid problems we had with 'winbind use
 * default domain'.
 *
 * We have 3 cases:
 *
 * unmapped unix users: Go directly to nss to find the user's group.
 *
 * A passdb user: The list of groups is provided by pdb_enum_group_memberships.
 *
 * If the user is provided by winbind, the primary gid is set to "domain
 * users" of the user's domain. For an explanation why this is necessary, see
 * the thread starting at
 * http://lists.samba.org/archive/samba-technical/2006-January/044803.html.
 */

NTSTATUS create_token_from_username(TALLOC_CTX *mem_ctx, const char *username,
				    BOOL is_guest,
				    uid_t *uid, gid_t *gid,
				    char **found_username,
				    struct nt_user_token **token)
{
	NTSTATUS result = NT_STATUS_NO_SUCH_USER;
	TALLOC_CTX *tmp_ctx;
	DOM_SID user_sid;
	enum SID_NAME_USE type;
	gid_t *gids;
	DOM_SID primary_group_sid;
	DOM_SID *group_sids;
	DOM_SID unix_group_sid;
	size_t num_group_sids;
	size_t num_gids;
	size_t i;

	tmp_ctx = talloc_new(NULL);
	if (tmp_ctx == NULL) {
		DEBUG(0, ("talloc_new failed\n"));
		return NT_STATUS_NO_MEMORY;
	}

	if (!lookup_name_smbconf(tmp_ctx, username, LOOKUP_NAME_ALL,
			 NULL, NULL, &user_sid, &type)) {
		DEBUG(1, ("lookup_name_smbconf for %s failed\n", username));
		goto done;
	}

	if (type != SID_NAME_USER) {
		DEBUG(1, ("%s is a %s, not a user\n", username,
			  sid_type_lookup(type)));
		goto done;
	}

	if (!sid_to_uid(&user_sid, uid)) {
		DEBUG(1, ("sid_to_uid for %s (%s) failed\n",
			  username, sid_string_static(&user_sid)));
		goto done;
	}

	if (sid_check_is_in_our_domain(&user_sid)) {

		/* This is a passdb user, so ask passdb */

		struct samu *sam_acct = NULL;

		if ( !(sam_acct = samu_new( tmp_ctx )) ) {
			result = NT_STATUS_NO_MEMORY;
			goto done;
		}

		if (!pdb_getsampwsid(sam_acct, &user_sid)) {
			DEBUG(1, ("pdb_getsampwsid(%s) for user %s failed\n",
				  sid_string_static(&user_sid), username));
			DEBUGADD(1, ("Fall back to unix user %s\n", username));
			goto unix_user;
		}

		result = pdb_enum_group_memberships(tmp_ctx, sam_acct,
						    &group_sids, &gids,
						    &num_group_sids);
		if (!NT_STATUS_IS_OK(result)) {
			DEBUG(10, ("enum_group_memberships failed for %s\n",
				   username));
			DEBUGADD(1, ("Fall back to unix user %s\n", username));
			goto unix_user;
		}

		/* see the smb_panic() in pdb_default_enum_group_memberships */
		SMB_ASSERT(num_group_sids > 0); 

		*gid = gids[0];
		*found_username = talloc_strdup(mem_ctx,
						pdb_get_username(sam_acct));

	} else 	if (sid_check_is_in_unix_users(&user_sid)) {

		/* This is a unix user not in passdb. We need to ask nss
		 * directly, without consulting passdb */

		struct passwd *pass;

		/*
		 * This goto target is used as a fallback for the passdb
		 * case. The concrete bug report is when passdb gave us an
		 * unmapped gid.
		 */

	unix_user:

		uid_to_unix_users_sid(*uid, &user_sid);

		pass = getpwuid_alloc(tmp_ctx, *uid);
		if (pass == NULL) {
			DEBUG(1, ("getpwuid(%d) for user %s failed\n",
				  *uid, username));
			goto done;
		}

		if (!getgroups_unix_user(tmp_ctx, username, pass->pw_gid,
					 &gids, &num_group_sids)) {
			DEBUG(1, ("getgroups_unix_user for user %s failed\n",
				  username));
			goto done;
		}

		group_sids = talloc_array(tmp_ctx, DOM_SID, num_group_sids);
		if (group_sids == NULL) {
			DEBUG(1, ("talloc_array failed\n"));
			result = NT_STATUS_NO_MEMORY;
			goto done;
		}

		for (i=0; i<num_group_sids; i++) {
			gid_to_sid(&group_sids[i], gids[i]);
		}

		/* In getgroups_unix_user we always set the primary gid */
		SMB_ASSERT(num_group_sids > 0); 

		*gid = gids[0];
		*found_username = talloc_strdup(mem_ctx, pass->pw_name);

	} else {

		/* This user is from winbind, force the primary gid to the
		 * user's "domain users" group. Under certain circumstances
		 * (user comes from NT4), this might be a loss of
		 * information. But we can not rely on winbind getting the
		 * correct info. AD might prohibit winbind looking up that
		 * information. */

		uint32 dummy;

		sid_copy(&primary_group_sid, &user_sid);
		sid_split_rid(&primary_group_sid, &dummy);
		sid_append_rid(&primary_group_sid, DOMAIN_GROUP_RID_USERS);

		if (!sid_to_gid(&primary_group_sid, gid)) {
			DEBUG(1, ("sid_to_gid(%s) failed\n",
				  sid_string_static(&primary_group_sid)));
			goto done;
		}

		num_group_sids = 1;
		group_sids = &primary_group_sid;
		gids = gid;

		*found_username = talloc_strdup(mem_ctx, username);
	}

	/* Add the "Unix Group" SID for each gid to catch mapped groups
	   and their Unix equivalent.  This is to solve the backwards
	   compatibility problem of 'valid users = +ntadmin' where
	   ntadmin has been paired with "Domain Admins" in the group
	   mapping table.  Otherwise smb.conf would need to be changed
	   to 'valid user = "Domain Admins"'.  --jerry */

	num_gids = num_group_sids;
	for ( i=0; i<num_gids; i++ ) {
		gid_t high, low;

		/* don't pickup anything managed by Winbind */

		if ( lp_idmap_gid(&low, &high) && (gids[i] >= low) && (gids[i] <= high) )
			continue;

		if ( !gid_to_unix_groups_sid( gids[i], &unix_group_sid ) ) {
			DEBUG(1,("create_token_from_username: Failed to create SID "
				"for gid %d!\n", gids[i]));
			continue;
		}
		add_sid_to_array_unique( mem_ctx, &unix_group_sid,
			&group_sids, &num_group_sids );
	}

	*token = create_local_nt_token(mem_ctx, &user_sid,
				       is_guest, num_group_sids, group_sids);

	if ((*token == NULL) || (*found_username == NULL)) {
		result = NT_STATUS_NO_MEMORY;
		goto done;
	}

	result = NT_STATUS_OK;
 done:
	TALLOC_FREE(tmp_ctx);
	return result;
}

/***************************************************************************
 Build upon create_token_from_username:

 Expensive helper function to figure out whether a user given its name is
 member of a particular group.
***************************************************************************/
BOOL user_in_group_sid(const char *username, const DOM_SID *group_sid)
{
	NTSTATUS status;
	uid_t uid;
	gid_t gid;
	char *found_username;
	struct nt_user_token *token;
	BOOL result;

	TALLOC_CTX *mem_ctx;

	mem_ctx = talloc_new(NULL);
	if (mem_ctx == NULL) {
		DEBUG(0, ("talloc_new failed\n"));
		return False;
	}

	status = create_token_from_username(mem_ctx, username, False,
					    &uid, &gid, &found_username,
					    &token);

	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(10, ("could not create token for %s\n", username));
		return False;
	}

	result = nt_token_check_sid(group_sid, token);

	TALLOC_FREE(mem_ctx);
	return result;
	
}

BOOL user_in_group(const char *username, const char *groupname)
{
	TALLOC_CTX *mem_ctx;
	DOM_SID group_sid;
	BOOL ret;

	mem_ctx = talloc_new(NULL);
	if (mem_ctx == NULL) {
		DEBUG(0, ("talloc_new failed\n"));
		return False;
	}

	ret = lookup_name(mem_ctx, groupname, LOOKUP_NAME_ALL,
			  NULL, NULL, &group_sid, NULL);
	TALLOC_FREE(mem_ctx);

	if (!ret) {
		DEBUG(10, ("lookup_name for (%s) failed.\n", groupname));
		return False;
	}

	return user_in_group_sid(username, &group_sid);
}


/***************************************************************************
 Make (and fill) a user_info struct from a 'struct passwd' by conversion 
 to a struct samu
***************************************************************************/

NTSTATUS make_server_info_pw(auth_serversupplied_info **server_info, 
                             char *unix_username,
			     struct passwd *pwd)
{
	NTSTATUS status;
	struct samu *sampass = NULL;
	gid_t *gids;
	auth_serversupplied_info *result;
	
	if ( !(sampass = samu_new( NULL )) ) {
		return NT_STATUS_NO_MEMORY;
	}
	
	status = samu_set_unix( sampass, pwd );
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}

	result = make_server_info(NULL);
	if (result == NULL) {
		TALLOC_FREE(sampass);
		return NT_STATUS_NO_MEMORY;
	}

	result->sam_account = sampass;
	result->unix_name = talloc_strdup(result, unix_username);
	result->uid = pwd->pw_uid;
	result->gid = pwd->pw_gid;

	status = pdb_enum_group_memberships(result, sampass,
					    &result->sids, &gids,
					    &result->num_sids);

	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(10, ("pdb_enum_group_memberships failed: %s\n",
			   nt_errstr(status)));
		TALLOC_FREE(result);
		return status;
	}

	/* For now we throw away the gids and convert via sid_to_gid
	 * later. This needs fixing, but I'd like to get the code straight and
	 * simple first. */
	TALLOC_FREE(gids);

	*server_info = result;

	return NT_STATUS_OK;
}

/***************************************************************************
 Make (and fill) a user_info struct for a guest login.
 This *must* succeed for smbd to start. If there is no mapping entry for
 the guest gid, then create one.
***************************************************************************/

static NTSTATUS make_new_server_info_guest(auth_serversupplied_info **server_info)
{
	NTSTATUS status;
	struct samu *sampass = NULL;
	DOM_SID guest_sid;
	BOOL ret;
	static const char zeros[16];

	if ( !(sampass = samu_new( NULL )) ) {
		return NT_STATUS_NO_MEMORY;
	}

	sid_copy(&guest_sid, get_global_sam_sid());
	sid_append_rid(&guest_sid, DOMAIN_USER_RID_GUEST);

	become_root();
	ret = pdb_getsampwsid(sampass, &guest_sid);
	unbecome_root();

	if (!ret) {
		TALLOC_FREE(sampass);
		return NT_STATUS_NO_SUCH_USER;
	}

	status = make_server_info_sam(server_info, sampass);
	if (!NT_STATUS_IS_OK(status)) {
		TALLOC_FREE(sampass);
		return status;
	}
	
	(*server_info)->guest = True;

	status = create_local_token(*server_info);
	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(10, ("create_local_token failed: %s\n",
			   nt_errstr(status)));
		return status;
	}

	/* annoying, but the Guest really does have a session key, and it is
	   all zeros! */
	(*server_info)->user_session_key = data_blob(zeros, sizeof(zeros));
	(*server_info)->lm_session_key = data_blob(zeros, sizeof(zeros));

	return NT_STATUS_OK;
}

static auth_serversupplied_info *copy_serverinfo(auth_serversupplied_info *src)
{
	auth_serversupplied_info *dst;

	dst = make_server_info(NULL);
	if (dst == NULL) {
		return NULL;
	}

	dst->guest = src->guest;
	dst->uid = src->uid;
	dst->gid = src->gid;
	dst->n_groups = src->n_groups;
	if (src->n_groups != 0) {
		dst->groups = talloc_memdup(dst, src->groups,
					    sizeof(gid_t)*dst->n_groups);
	} else {
		dst->groups = NULL;
	}

	if (src->ptok) {
		dst->ptok = dup_nt_token(dst, src->ptok);
		if (!dst->ptok) {
			TALLOC_FREE(dst);
			return NULL;
		}
	}
	
	dst->user_session_key = data_blob_talloc( dst, src->user_session_key.data,
						src->user_session_key.length);

	dst->lm_session_key = data_blob_talloc(dst, src->lm_session_key.data,
						src->lm_session_key.length);

	dst->sam_account = samu_new(NULL);
	if (!dst->sam_account) {
		TALLOC_FREE(dst);
		return NULL;
	}

	if (!pdb_copy_sam_account(dst->sam_account, src->sam_account)) {
		TALLOC_FREE(dst);
		return NULL;
	}
	
	dst->pam_handle = NULL;
	dst->unix_name = talloc_strdup(dst, src->unix_name);
	if (!dst->unix_name) {
		TALLOC_FREE(dst);
		return NULL;
	}

	return dst;
}

static auth_serversupplied_info *guest_info = NULL;

BOOL init_guest_info(void)
{
	if (guest_info != NULL)
		return True;

	return NT_STATUS_IS_OK(make_new_server_info_guest(&guest_info));
}

NTSTATUS make_server_info_guest(auth_serversupplied_info **server_info)
{
	*server_info = copy_serverinfo(guest_info);
	return (*server_info != NULL) ? NT_STATUS_OK : NT_STATUS_NO_MEMORY;
}

/***************************************************************************
 Purely internal function for make_server_info_info3
 Fill the sam account from getpwnam
***************************************************************************/
static NTSTATUS fill_sam_account(TALLOC_CTX *mem_ctx, 
				 const char *domain,
				 const char *username,
				 char **found_username,
				 uid_t *uid, gid_t *gid,
				 struct samu *account,
				 BOOL *username_was_mapped)
{
	NTSTATUS nt_status;
	fstring dom_user, lower_username;
	fstring real_username;
	struct passwd *passwd;

	fstrcpy( lower_username, username );
	strlower_m( lower_username );

	fstr_sprintf(dom_user, "%s%c%s", domain, *lp_winbind_separator(), 
		lower_username);

	/* Get the passwd struct.  Try to create the account is necessary. */

	*username_was_mapped = map_username( dom_user );

	if ( !(passwd = smb_getpwnam( NULL, dom_user, real_username, True )) )
		return NT_STATUS_NO_SUCH_USER;

	*uid = passwd->pw_uid;
	*gid = passwd->pw_gid;

	/* This is pointless -- there is no suport for differing 
	   unix and windows names.  Make sure to always store the 
	   one we actually looked up and succeeded. Have I mentioned
	   why I hate the 'winbind use default domain' parameter?   
	                                 --jerry              */
	   
	*found_username = talloc_strdup( mem_ctx, real_username );
	
	DEBUG(5,("fill_sam_account: located username was [%s]\n", *found_username));

	nt_status = samu_set_unix( account, passwd );
	
	TALLOC_FREE(passwd);
	
	return nt_status;
}

/****************************************************************************
 Wrapper to allow the getpwnam() call to strip the domain name and 
 try again in case a local UNIX user is already there.  Also run through 
 the username if we fallback to the username only.
 ****************************************************************************/
 
struct passwd *smb_getpwnam( TALLOC_CTX *mem_ctx, char *domuser,
			     fstring save_username, BOOL create )
{
	struct passwd *pw = NULL;
	char *p;
	fstring username;
	
	/* we only save a copy of the username it has been mangled 
	   by winbindd use default domain */
	   
	save_username[0] = '\0';
	   
	/* don't call map_username() here since it has to be done higher 
	   up the stack so we don't call it mutliple times */

	fstrcpy( username, domuser );
	
	p = strchr_m( username, *lp_winbind_separator() );
	
	/* code for a DOMAIN\user string */
	
	if ( p ) {
		fstring strip_username;

		pw = Get_Pwnam_alloc( mem_ctx, domuser );
		if ( pw ) {	
			/* make sure we get the case of the username correct */
			/* work around 'winbind use default domain = yes' */

			if ( !strchr_m( pw->pw_name, *lp_winbind_separator() ) ) {
				char *domain;
				
				/* split the domain and username into 2 strings */
				*p = '\0';
				domain = username;

				fstr_sprintf(save_username, "%s%c%s", domain, *lp_winbind_separator(), pw->pw_name);
			}
			else
				fstrcpy( save_username, pw->pw_name );

			/* whew -- done! */		
			return pw;
		}

		/* setup for lookup of just the username */
		/* remember that p and username are overlapping memory */

		p++;
		fstrcpy( strip_username, p );
		fstrcpy( username, strip_username );
	}
	
	/* just lookup a plain username */
	
	pw = Get_Pwnam_alloc(mem_ctx, username);
		
	/* Create local user if requested but only if winbindd
	   is not running.  We need to protect against cases
	   where winbindd is failing and then prematurely
	   creating users in /etc/passwd */
	
	if ( !pw && create && !winbind_ping() ) {
		/* Don't add a machine account. */
		if (username[strlen(username)-1] == '$')
			return NULL;

		smb_create_user(NULL, username, NULL);
		pw = Get_Pwnam_alloc(mem_ctx, username);
	}
	
	/* one last check for a valid passwd struct */
	
	if ( pw )
		fstrcpy( save_username, pw->pw_name );

	return pw;
}

/***************************************************************************
 Make a server_info struct from the info3 returned by a domain logon 
***************************************************************************/

NTSTATUS make_server_info_info3(TALLOC_CTX *mem_ctx, 
				const char *sent_nt_username,
				const char *domain,
				auth_serversupplied_info **server_info, 
				NET_USER_INFO_3 *info3) 
{
	static const char zeros[16];

	NTSTATUS nt_status = NT_STATUS_OK;
	char *found_username;
	const char *nt_domain;
	const char *nt_username;
	struct samu *sam_account = NULL;
	DOM_SID user_sid;
	DOM_SID group_sid;
	BOOL username_was_mapped;

	uid_t uid;
	gid_t gid;

	size_t i;

	auth_serversupplied_info *result;

	/* 
	   Here is where we should check the list of
	   trusted domains, and verify that the SID 
	   matches.
	*/

	sid_copy(&user_sid, &info3->dom_sid.sid);
	if (!sid_append_rid(&user_sid, info3->user_rid)) {
		return NT_STATUS_INVALID_PARAMETER;
	}
	
	sid_copy(&group_sid, &info3->dom_sid.sid);
	if (!sid_append_rid(&group_sid, info3->group_rid)) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	if (!(nt_username = unistr2_tdup(mem_ctx, &(info3->uni_user_name)))) {
		/* If the server didn't give us one, just use the one we sent
		 * them */
		nt_username = sent_nt_username;
	}

	if (!(nt_domain = unistr2_tdup(mem_ctx, &(info3->uni_logon_dom)))) {
		/* If the server didn't give us one, just use the one we sent
		 * them */
		nt_domain = domain;
	}
	
	/* try to fill the SAM account..  If getpwnam() fails, then try the 
	   add user script (2.2.x behavior).

	   We use the _unmapped_ username here in an attempt to provide
	   consistent username mapping behavior between kerberos and NTLM[SSP]
	   authentication in domain mode security.  I.E. Username mapping
	   should be applied to the fully qualified username
	   (e.g. DOMAIN\user) and not just the login name.  Yes this means we
	   called map_username() unnecessarily in make_user_info_map() but
	   that is how the current code is designed.  Making the change here
	   is the least disruptive place.  -- jerry */
	   
	if ( !(sam_account = samu_new( NULL )) ) {
		return NT_STATUS_NO_MEMORY;
	}

	/* this call will try to create the user if necessary */

	nt_status = fill_sam_account(mem_ctx, nt_domain, sent_nt_username,
				     &found_username, &uid, &gid, sam_account,
				     &username_was_mapped);

	
	/* if we still don't have a valid unix account check for 
	  'map to guest = bad uid' */
	  
	if (!NT_STATUS_IS_OK(nt_status)) {
		TALLOC_FREE( sam_account );
		if ( lp_map_to_guest() == MAP_TO_GUEST_ON_BAD_UID ) {
		 	make_server_info_guest(server_info); 
			return NT_STATUS_OK;
		}
		return nt_status;
	}
		
	if (!pdb_set_nt_username(sam_account, nt_username, PDB_CHANGED)) {
		TALLOC_FREE(sam_account);
		return NT_STATUS_NO_MEMORY;
	}

	if (!pdb_set_username(sam_account, nt_username, PDB_CHANGED)) {
		TALLOC_FREE(sam_account);
		return NT_STATUS_NO_MEMORY;
	}

	if (!pdb_set_domain(sam_account, nt_domain, PDB_CHANGED)) {
		TALLOC_FREE(sam_account);
		return NT_STATUS_NO_MEMORY;
	}

	if (!pdb_set_user_sid(sam_account, &user_sid, PDB_CHANGED)) {
		TALLOC_FREE(sam_account);
		return NT_STATUS_UNSUCCESSFUL;
	}

	if (!pdb_set_group_sid(sam_account, &group_sid, PDB_CHANGED)) {
		TALLOC_FREE(sam_account);
		return NT_STATUS_UNSUCCESSFUL;
	}
		
	if (!pdb_set_fullname(sam_account,
			      unistr2_static(&(info3->uni_full_name)), 
			      PDB_CHANGED)) {
		TALLOC_FREE(sam_account);
		return NT_STATUS_NO_MEMORY;
	}

	if (!pdb_set_logon_script(sam_account,
				  unistr2_static(&(info3->uni_logon_script)),
				  PDB_CHANGED)) {
		TALLOC_FREE(sam_account);
		return NT_STATUS_NO_MEMORY;
	}

	if (!pdb_set_profile_path(sam_account,
				  unistr2_static(&(info3->uni_profile_path)),
				  PDB_CHANGED)) {
		TALLOC_FREE(sam_account);
		return NT_STATUS_NO_MEMORY;
	}

	if (!pdb_set_homedir(sam_account,
			     unistr2_static(&(info3->uni_home_dir)),
			     PDB_CHANGED)) {
		TALLOC_FREE(sam_account);
		return NT_STATUS_NO_MEMORY;
	}

	if (!pdb_set_dir_drive(sam_account,
			       unistr2_static(&(info3->uni_dir_drive)),
			       PDB_CHANGED)) {
		TALLOC_FREE(sam_account);
		return NT_STATUS_NO_MEMORY;
	}

	if (!pdb_set_acct_ctrl(sam_account, info3->acct_flags, PDB_CHANGED)) {
		TALLOC_FREE(sam_account);
		return NT_STATUS_NO_MEMORY;
	}

	result = make_server_info(NULL);
	if (result == NULL) {
		DEBUG(4, ("make_server_info failed!\n"));
		TALLOC_FREE(sam_account);
		return NT_STATUS_NO_MEMORY;
	}

	/* save this here to _net_sam_logon() doesn't fail (it assumes a 
	   valid struct samu) */
		   
	result->sam_account = sam_account;
	result->unix_name = talloc_strdup(result, found_username);

	/* Fill in the unix info we found on the way */

	result->uid = uid;
	result->gid = gid;

	/* Create a 'combined' list of all SIDs we might want in the SD */

	result->num_sids = 0;
	result->sids = NULL;

	/* and create (by appending rids) the 'domain' sids */
	
	for (i = 0; i < info3->num_groups2; i++) {
		DOM_SID sid;
		if (!sid_compose(&sid, &info3->dom_sid.sid,
				 info3->gids[i].g_rid)) {
			DEBUG(3,("could not append additional group rid "
				 "0x%x\n", info3->gids[i].g_rid));
			TALLOC_FREE(result);
			return NT_STATUS_INVALID_PARAMETER;
		}
		add_sid_to_array(result, &sid, &result->sids,
				 &result->num_sids);
	}

	/* Copy 'other' sids.  We need to do sid filtering here to
 	   prevent possible elevation of privileges.  See:

           http://www.microsoft.com/windows2000/techinfo/administration/security/sidfilter.asp
         */

	for (i = 0; i < info3->num_other_sids; i++) {
		add_sid_to_array(result, &info3->other_sids[i].sid,
				 &result->sids,
				 &result->num_sids);
	}

	result->login_server = unistr2_tdup(result, 
					    &(info3->uni_logon_srv));

	/* ensure we are never given NULL session keys */
	
	if (memcmp(info3->user_sess_key, zeros, sizeof(zeros)) == 0) {
		result->user_session_key = data_blob(NULL, 0);
	} else {
		result->user_session_key = data_blob_talloc(
			result, info3->user_sess_key,
			sizeof(info3->user_sess_key));
	}

	if (memcmp(info3->lm_sess_key, zeros, 8) == 0) {
		result->lm_session_key = data_blob(NULL, 0);
	} else {
		result->lm_session_key = data_blob_talloc(
			result, info3->lm_sess_key,
			sizeof(info3->lm_sess_key));
	}

	result->was_mapped = username_was_mapped;

	*server_info = result;

	return NT_STATUS_OK;
}

/***************************************************************************
 Free a user_info struct
***************************************************************************/

void free_user_info(auth_usersupplied_info **user_info)
{
	DEBUG(5,("attempting to free (and zero) a user_info structure\n"));
	if (*user_info != NULL) {
		if ((*user_info)->smb_name) {
			DEBUG(10,("structure was created for %s\n",
				  (*user_info)->smb_name));
		}
		SAFE_FREE((*user_info)->smb_name);
		SAFE_FREE((*user_info)->internal_username);
		SAFE_FREE((*user_info)->client_domain);
		SAFE_FREE((*user_info)->domain);
		SAFE_FREE((*user_info)->wksta_name);
		data_blob_free(&(*user_info)->lm_resp);
		data_blob_free(&(*user_info)->nt_resp);
		data_blob_clear_free(&(*user_info)->lm_interactive_pwd);
		data_blob_clear_free(&(*user_info)->nt_interactive_pwd);
		data_blob_clear_free(&(*user_info)->plaintext_password);
		ZERO_STRUCT(**user_info);
	}
	SAFE_FREE(*user_info);
}

/***************************************************************************
 Make an auth_methods struct
***************************************************************************/

BOOL make_auth_methods(struct auth_context *auth_context, auth_methods **auth_method) 
{
	if (!auth_context) {
		smb_panic("no auth_context supplied to "
			  "make_auth_methods()!\n");
	}

	if (!auth_method) {
		smb_panic("make_auth_methods: pointer to auth_method pointer "
			  "is NULL!\n");
	}

	*auth_method = TALLOC_P(auth_context->mem_ctx, auth_methods);
	if (!*auth_method) {
		DEBUG(0,("make_auth_method: malloc failed!\n"));
		return False;
	}
	ZERO_STRUCTP(*auth_method);
	
	return True;
}

/****************************************************************************
 Duplicate a SID token.
****************************************************************************/

NT_USER_TOKEN *dup_nt_token(TALLOC_CTX *mem_ctx, NT_USER_TOKEN *ptoken)
{
	NT_USER_TOKEN *token;

	if (!ptoken)
		return NULL;

	token = TALLOC_P(mem_ctx, NT_USER_TOKEN);
	if (token == NULL) {
		DEBUG(0, ("talloc failed\n"));
		return NULL;
	}

	ZERO_STRUCTP(token);

	if (ptoken->user_sids && ptoken->num_sids) {
		token->user_sids = talloc_memdup(token, ptoken->user_sids,
					 sizeof(DOM_SID) * ptoken->num_sids );

		if (token->user_sids == NULL) {
			DEBUG(0, ("talloc_memdup failed\n"));
			TALLOC_FREE(token);
			return NULL;
		}
		token->num_sids = ptoken->num_sids;
	}

	/* copy the privileges; don't consider failure to be critical here */
	
	if ( !se_priv_copy( &token->privileges, &ptoken->privileges ) ) {
		DEBUG(0,("dup_nt_token: Failure to copy SE_PRIV!.  "
			 "Continuing with 0 privileges assigned.\n"));
	}

	return token;
}

/****************************************************************************
 Check for a SID in an NT_USER_TOKEN
****************************************************************************/

BOOL nt_token_check_sid ( const DOM_SID *sid, const NT_USER_TOKEN *token )
{
	int i;
	
	if ( !sid || !token )
		return False;
	
	for ( i=0; i<token->num_sids; i++ ) {
		if ( sid_equal( sid, &token->user_sids[i] ) )
			return True;
	}

	return False;
}

BOOL nt_token_check_domain_rid( NT_USER_TOKEN *token, uint32 rid ) 
{
	DOM_SID domain_sid;

	/* if we are a domain member, the get the domain SID, else for 
	   a DC or standalone server, use our own SID */

	if ( lp_server_role() == ROLE_DOMAIN_MEMBER ) {
		if ( !secrets_fetch_domain_sid( lp_workgroup(),
						&domain_sid ) ) {
			DEBUG(1,("nt_token_check_domain_rid: Cannot lookup "
				 "SID for domain [%s]\n", lp_workgroup()));
			return False;
		}
	} 
	else
		sid_copy( &domain_sid, get_global_sam_sid() );

	sid_append_rid( &domain_sid, rid );
	
	return nt_token_check_sid( &domain_sid, token );\
}

/**
 * Verify whether or not given domain is trusted.
 *
 * @param domain_name name of the domain to be verified
 * @return true if domain is one of the trusted once or
 *         false if otherwise
 **/

BOOL is_trusted_domain(const char* dom_name)
{
	DOM_SID trustdom_sid;
	BOOL ret;

	/* no trusted domains for a standalone server */

	if ( lp_server_role() == ROLE_STANDALONE )
		return False;

	/* if we are a DC, then check for a direct trust relationships */

	if ( IS_DC ) {
		become_root();
		DEBUG (5,("is_trusted_domain: Checking for domain trust with "
			  "[%s]\n", dom_name ));
		ret = secrets_fetch_trusted_domain_password(dom_name, NULL,
							    NULL, NULL);
		unbecome_root();
		if (ret)
			return True;
	}
	else {
		NSS_STATUS result;

		/* If winbind is around, ask it */

		result = wb_is_trusted_domain(dom_name);

		if (result == NSS_STATUS_SUCCESS) {
			return True;
		}

		if (result == NSS_STATUS_NOTFOUND) {
			/* winbind could not find the domain */
			return False;
		}

		/* The only other possible result is that winbind is not up
		   and running. We need to update the trustdom_cache
		   ourselves */
		
		update_trustdom_cache();
	}

	/* now the trustdom cache should be available a DC could still
	 * have a transitive trust so fall back to the cache of trusted
	 * domains (like a domain member would use  */

	if ( trustdom_cache_fetch(dom_name, &trustdom_sid) ) {
		return True;
	}

	return False;
}

