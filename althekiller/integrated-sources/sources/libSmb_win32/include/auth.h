#ifndef _SMBAUTH_H_
#define _SMBAUTH_H_
/* 
   Unix SMB/CIFS implementation.
   Standardised Authentication types
   Copyright (C) Andrew Bartlett 2001
   
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

typedef struct auth_usersupplied_info {
 	DATA_BLOB lm_resp;
	DATA_BLOB nt_resp;
 	DATA_BLOB lm_interactive_pwd;
	DATA_BLOB nt_interactive_pwd;
 	DATA_BLOB plaintext_password;
	
	BOOL encrypted;
	
	BOOL was_mapped;	      /* Did the username map actually match? */
	char *client_domain;          /* domain name string */
	char *domain;                 /* domain name after mapping */
	char *internal_username;      /* username after mapping */
	char *smb_name;               /* username before mapping */
	char *wksta_name;             /* workstation name (netbios calling
				       * name) unicode string */
	
	uint32 logon_parameters;

} auth_usersupplied_info;

typedef struct auth_serversupplied_info {
	BOOL guest;

	DOM_SID *sids; 	/* These SIDs are preliminary between
			   check_ntlm_password and the token creation. */
	size_t num_sids;

	uid_t uid;
	gid_t gid;
	
	/* This groups info is needed for when we become_user() for this uid */
	size_t n_groups;
	gid_t *groups;
	
	/* NT group information taken from the info3 structure */
	
	NT_USER_TOKEN *ptok;
	
	DATA_BLOB user_session_key;
	DATA_BLOB lm_session_key;

        char *login_server; /* which server authorized the login? */
	
	struct samu *sam_account;
	
	void *pam_handle;

	BOOL was_mapped;	/* Did the username map match? */
	char *unix_name;
	
} auth_serversupplied_info;

struct auth_context {
	DATA_BLOB challenge; 

	/* Who set this up in the first place? */ 
	const char *challenge_set_by; 

	BOOL challenge_may_be_modified;

	struct auth_methods *challenge_set_method; 
	/* What order are the various methods in?   Try to stop it changing under us */ 
	struct auth_methods *auth_method_list;	

	TALLOC_CTX *mem_ctx;
	const uint8 *(*get_ntlm_challenge)(struct auth_context *auth_context);
	NTSTATUS (*check_ntlm_password)(const struct auth_context *auth_context,
					const struct auth_usersupplied_info *user_info, 
					struct auth_serversupplied_info **server_info);
	NTSTATUS (*nt_status_squash)(NTSTATUS nt_status);
	void (*free)(struct auth_context **auth_context);
};

typedef struct auth_methods
{
	struct auth_methods *prev, *next;
	const char *name; /* What name got this module */

	NTSTATUS (*auth)(const struct auth_context *auth_context,
			 void *my_private_data, 
			 TALLOC_CTX *mem_ctx,
			 const struct auth_usersupplied_info *user_info, 
			 auth_serversupplied_info **server_info);

	/* If you are using this interface, then you are probably
	 * getting something wrong.  This interface is only for
	 * security=server, and makes a number of compromises to allow
	 * that.  It is not compatible with being a PDC.  */
	DATA_BLOB (*get_chal)(const struct auth_context *auth_context,
			      void **my_private_data, 
			      TALLOC_CTX *mem_ctx);
	
	/* Used to keep tabs on things like the cli for SMB server authentication */
	void *private_data;
	
	/* Function to clean up the above arbitary structure */
	void (*free_private_data)(void **private_data);

	/* Function to send a keepalive message on the above structure */
	void (*send_keepalive)(void **private_data);

} auth_methods;

typedef NTSTATUS (*auth_init_function)(struct auth_context *, const char *, struct auth_methods **);

struct auth_init_function_entry {
	const char *name;
	/* Function to create a member of the authmethods list */

	auth_init_function init;

	struct auth_init_function_entry *prev, *next;
};

typedef struct auth_ntlmssp_state {
	TALLOC_CTX *mem_ctx;
	struct auth_context *auth_context;
	struct auth_serversupplied_info *server_info;
	struct ntlmssp_state *ntlmssp_state;
} AUTH_NTLMSSP_STATE;

/* Changed from 1 -> 2 to add the logon_parameters field. */
#define AUTH_INTERFACE_VERSION 2

#endif /* _SMBAUTH_H_ */
