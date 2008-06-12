/* 
   Unix SMB/CIFS implementation.

   Call out to a shell script for an authentication check.

   Copyright (C) Jeremy Allison 2005.
   
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

#undef malloc

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_AUTH

/* Create a string containing the supplied :
 * domain\n
 * user\n
 * ascii hex challenge\n
 * ascii hex LM response\n
 * ascii hex NT response\n\0
 * and execute a shell script to check this.
 * Allows external programs to create users on demand.
 * Script returns zero on success, non-zero on fail.
 */

static NTSTATUS script_check_user_credentials(const struct auth_context *auth_context,
					void *my_private_data, 
					TALLOC_CTX *mem_ctx,
					const auth_usersupplied_info *user_info, 
					auth_serversupplied_info **server_info)
{
	const char *script = lp_parm_const_string( GLOBAL_SECTION_SNUM, "auth_script", "script", NULL);
	char *secret_str;
	size_t secret_str_len;
	char hex_str[49];
	int ret, i;

	if (!script) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	if (!user_info) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	if (!auth_context) {
		DEBUG(3,("script_check_user_credentials: no auth_info !\n"));
		return NT_STATUS_INVALID_PARAMETER;
	}		

	secret_str_len = strlen(user_info->domain) + 1 +
			strlen(user_info->smb_name) + 1 +
			16 + 1 + /* 8 bytes of challenge going to 16 */
			48 + 1 + /* 24 bytes of challenge going to 48 */
			48 + 1;

	secret_str = malloc(secret_str_len);
	if (!secret_str) {
		return NT_STATUS_NO_MEMORY;
	}

	safe_strcpy( secret_str, user_info->domain, secret_str_len - 1);
	safe_strcat( secret_str, "\n", secret_str_len - 1);
	safe_strcat( secret_str, user_info->smb_name, secret_str_len - 1);
	safe_strcat( secret_str, "\n", secret_str_len - 1);

	for (i = 0; i < 8; i++) {
		slprintf(&hex_str[i*2], 3, "%02X", auth_context->challenge.data[i]);
	}
	safe_strcat( secret_str, hex_str, secret_str_len - 1);
	safe_strcat( secret_str, "\n", secret_str_len - 1);

	if (user_info->lm_resp.data) {
		for (i = 0; i < 24; i++) {
			slprintf(&hex_str[i*2], 3, "%02X", user_info->lm_resp.data[i]);
		}
		safe_strcat( secret_str, hex_str, secret_str_len - 1);
	}
	safe_strcat( secret_str, "\n", secret_str_len - 1);

	if (user_info->nt_resp.data) {
		for (i = 0; i < 24; i++) {
			slprintf(&hex_str[i*2], 3, "%02X", user_info->nt_resp.data[i]);
		}
		safe_strcat( secret_str, hex_str, secret_str_len - 1);
	}
	safe_strcat( secret_str, "\n", secret_str_len - 1);

	DEBUG(10,("script_check_user_credentials: running %s with parameters:\n%s\n",
		script, secret_str ));

	ret = smbrunsecret( script, secret_str);

	SAFE_FREE(secret_str);

	if (ret) {
		DEBUG(1,("script_check_user_credentials: failed to authenticate %s\\%s\n",
			user_info->domain, user_info->smb_name ));
		/* auth failed. */
		return NT_STATUS_NO_SUCH_USER;
	}

	/* Cause the auth system to keep going.... */
	return NT_STATUS_NOT_IMPLEMENTED;
}

/* module initialisation */
static NTSTATUS auth_init_script(struct auth_context *auth_context, const char *param, auth_methods **auth_method) 
{
	if (!make_auth_methods(auth_context, auth_method)) {
		return NT_STATUS_NO_MEMORY;
	}

	(*auth_method)->name = "script";
	(*auth_method)->auth = script_check_user_credentials;

	if (param && *param) {
		/* we load the 'fallback' module - if script isn't here, call this
		   module */
		if (!load_auth_module(auth_context, param, (auth_methods **)&(*auth_method)->private_data)) {
			return NT_STATUS_UNSUCCESSFUL;
		}
		
	}
	return NT_STATUS_OK;
}

NTSTATUS auth_script_init(void)
{
	return smb_register_auth(AUTH_INTERFACE_VERSION, "script", auth_init_script);
}
