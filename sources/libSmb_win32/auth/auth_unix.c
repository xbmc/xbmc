/* 
   Unix SMB/CIFS implementation.
   Password and authentication handling
   Copyright (C) Andrew Bartlett              2001
   
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

/**
 * update the encrypted smbpasswd file from the plaintext username and password
 *  
 *  this ugly hack needs to die, but not quite yet, I think people still use it...
 **/
static BOOL update_smbpassword_file(const char *user, const char *password)
{
	struct samu 	*sampass;
	BOOL            ret;
	
	if ( !(sampass = samu_new( NULL )) ) {
		return False;
	}
	
	become_root();
	ret = pdb_getsampwnam(sampass, user);
	unbecome_root();

	if(ret == False) {
		DEBUG(0,("pdb_getsampwnam returned NULL\n"));
		TALLOC_FREE(sampass);
		return False;
	}

	/*
	 * Remove the account disabled flag - we are updating the
	 * users password from a login.
	 */
	if (!pdb_set_acct_ctrl(sampass, pdb_get_acct_ctrl(sampass) & ~ACB_DISABLED, PDB_CHANGED)) {
		TALLOC_FREE(sampass);
		return False;
	}

	if (!pdb_set_plaintext_passwd (sampass, password)) {
		TALLOC_FREE(sampass);
		return False;
	}

	/* Now write it into the file. */
	become_root();

	ret = NT_STATUS_IS_OK(pdb_update_sam_account (sampass));

	unbecome_root();

	if (ret) {
		DEBUG(3,("pdb_update_sam_account returned %d\n",ret));
	}

	TALLOC_FREE(sampass);
	return ret;
}


/** Check a plaintext username/password
 *
 * Cannot deal with an encrupted password in any manner whatsoever,
 * unless the account has a null password.
 **/

static NTSTATUS check_unix_security(const struct auth_context *auth_context,
			     void *my_private_data, 
			     TALLOC_CTX *mem_ctx,
			     const auth_usersupplied_info *user_info, 
			     auth_serversupplied_info **server_info)
{
	NTSTATUS nt_status;
	struct passwd *pass = NULL;

	become_root();
	pass = Get_Pwnam(user_info->internal_username);

	
	/** @todo This call assumes a ASCII password, no charset transformation is 
	    done.  We may need to revisit this **/
	nt_status = pass_check(pass,
				pass ? pass->pw_name : user_info->internal_username, 
				(char *)user_info->plaintext_password.data,
				user_info->plaintext_password.length-1,
				lp_update_encrypted() ? 
				update_smbpassword_file : NULL,
				True);
	
	unbecome_root();

	if (NT_STATUS_IS_OK(nt_status)) {
		if (pass) {
			make_server_info_pw(server_info, pass->pw_name, pass);
		} else {
			/* we need to do somthing more useful here */
			nt_status = NT_STATUS_NO_SUCH_USER;
		}
	}

	return nt_status;
}

/* module initialisation */
static NTSTATUS auth_init_unix(struct auth_context *auth_context, const char* param, auth_methods **auth_method) 
{
	if (!make_auth_methods(auth_context, auth_method)) {
		return NT_STATUS_NO_MEMORY;
	}

	(*auth_method)->name = "unix";
	(*auth_method)->auth = check_unix_security;
	return NT_STATUS_OK;
}

NTSTATUS auth_unix_init(void)
{
	return smb_register_auth(AUTH_INTERFACE_VERSION, "unix", auth_init_unix);
}
