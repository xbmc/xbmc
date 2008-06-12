/* 
   Unix SMB/CIFS implementation.
   Password and authentication handling
   Copyright (C) Andrew Bartlett         2001-2002
   
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

extern struct auth_context *negprot_global_auth_context;
extern BOOL global_encrypted_passwords_negotiated;

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_AUTH

/****************************************************************************
 COMPATIBILITY INTERFACES:
 ***************************************************************************/

/****************************************************************************
check if a username/password is OK assuming the password is a 24 byte
SMB hash
return True if the password is correct, False otherwise
****************************************************************************/

NTSTATUS check_plaintext_password(const char *smb_name, DATA_BLOB plaintext_password, auth_serversupplied_info **server_info)
{
	struct auth_context *plaintext_auth_context = NULL;
	auth_usersupplied_info *user_info = NULL;
	const uint8 *chal;
	NTSTATUS nt_status;
	if (!NT_STATUS_IS_OK(nt_status = make_auth_context_subsystem(&plaintext_auth_context))) {
		return nt_status;
	}
	
	chal = plaintext_auth_context->get_ntlm_challenge(plaintext_auth_context);
	
	if (!make_user_info_for_reply(&user_info, 
				      smb_name, lp_workgroup(), chal,
				      plaintext_password)) {
		return NT_STATUS_NO_MEMORY;
	}
	
	nt_status = plaintext_auth_context->check_ntlm_password(plaintext_auth_context, 
								user_info, server_info); 
	
	(plaintext_auth_context->free)(&plaintext_auth_context);
	free_user_info(&user_info);
	return nt_status;
}

static NTSTATUS pass_check_smb(const char *smb_name,
			       const char *domain, 
			       DATA_BLOB lm_pwd,
			       DATA_BLOB nt_pwd,
			       DATA_BLOB plaintext_password,
			       BOOL encrypted)

{
	NTSTATUS nt_status;
	auth_serversupplied_info *server_info = NULL;
	if (encrypted) {		
		auth_usersupplied_info *user_info = NULL;
		make_user_info_for_reply_enc(&user_info, smb_name, 
					     domain,
					     lm_pwd, 
					     nt_pwd);
		nt_status = negprot_global_auth_context->check_ntlm_password(negprot_global_auth_context, 
									     user_info, &server_info);
		free_user_info(&user_info);
	} else {
		nt_status = check_plaintext_password(smb_name, plaintext_password, &server_info);
	}		
	TALLOC_FREE(server_info);
	return nt_status;
}

/****************************************************************************
check if a username/password pair is ok via the auth subsystem.
return True if the password is correct, False otherwise
****************************************************************************/
BOOL password_ok(char *smb_name, DATA_BLOB password_blob)
{

	DATA_BLOB null_password = data_blob(NULL, 0);
	BOOL encrypted = (global_encrypted_passwords_negotiated && password_blob.length == 24);
	
	if (encrypted) {
		/* 
		 * The password could be either NTLM or plain LM.  Try NTLM first, 
		 * but fall-through as required.
		 * NTLMv2 makes no sense here.
		 */
		if (NT_STATUS_IS_OK(pass_check_smb(smb_name, lp_workgroup(), null_password, password_blob, null_password, encrypted))) {
			return True;
		}
		
		if (NT_STATUS_IS_OK(pass_check_smb(smb_name, lp_workgroup(), password_blob, null_password, null_password, encrypted))) {
			return True;
		}
	} else {
		if (NT_STATUS_IS_OK(pass_check_smb(smb_name, lp_workgroup(), null_password, null_password, password_blob, encrypted))) {
			return True;
		}
	}

	return False;
}


