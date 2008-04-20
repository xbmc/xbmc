/* 
   Unix SMB/CIFS implementation.

   Winbind authentication mechnism

   Copyright (C) Tim Potter 2000
   Copyright (C) Andrew Bartlett 2001 - 2002
   
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

static NTSTATUS get_info3_from_ndr(TALLOC_CTX *mem_ctx, struct winbindd_response *response, NET_USER_INFO_3 *info3)
{
	uint8 *info3_ndr;
	size_t len = response->length - sizeof(struct winbindd_response);
	prs_struct ps;
	if (len > 0) {
		info3_ndr = response->extra_data.data;
		if (!prs_init(&ps, len, mem_ctx, UNMARSHALL)) {
			return NT_STATUS_NO_MEMORY;
		}
		prs_copy_data_in(&ps, (char *)info3_ndr, len);
		prs_set_offset(&ps,0);
		if (!net_io_user_info3("", info3, &ps, 1, 3, False)) {
			DEBUG(2, ("get_info3_from_ndr: could not parse info3 struct!\n"));
			return NT_STATUS_UNSUCCESSFUL;
		}
		prs_mem_free(&ps);

		return NT_STATUS_OK;
	} else {
		DEBUG(2, ("get_info3_from_ndr: No info3 struct found!\n"));
		return NT_STATUS_UNSUCCESSFUL;
	}
}

/* Authenticate a user with a challenge/response */

static NTSTATUS check_winbind_security(const struct auth_context *auth_context,
				       void *my_private_data, 
				       TALLOC_CTX *mem_ctx,
				       const auth_usersupplied_info *user_info, 
				       auth_serversupplied_info **server_info)
{
	struct winbindd_request request;
	struct winbindd_response response;
        NSS_STATUS result;
	NTSTATUS nt_status;
        NET_USER_INFO_3 info3;

	if (!user_info) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	if (!auth_context) {
		DEBUG(3,("Password for user %s cannot be checked because we have no auth_info to get the challenge from.\n", 
			 user_info->internal_username));
		return NT_STATUS_INVALID_PARAMETER;
	}		

	if (strequal(user_info->domain, get_global_sam_name())) {
		DEBUG(3,("check_winbind_security: Not using winbind, requested domain [%s] was for this SAM.\n",
			user_info->domain));
		return NT_STATUS_NOT_IMPLEMENTED;
	}

	/* Send off request */

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	request.flags = WBFLAG_PAM_INFO3_NDR;

	request.data.auth_crap.logon_parameters = user_info->logon_parameters;

	fstrcpy(request.data.auth_crap.user, user_info->smb_name);
	fstrcpy(request.data.auth_crap.domain, user_info->domain);
	fstrcpy(request.data.auth_crap.workstation, user_info->wksta_name);

	memcpy(request.data.auth_crap.chal, auth_context->challenge.data, sizeof(request.data.auth_crap.chal));
	
	request.data.auth_crap.lm_resp_len = MIN(user_info->lm_resp.length, 
						 sizeof(request.data.auth_crap.lm_resp));
	request.data.auth_crap.nt_resp_len = MIN(user_info->nt_resp.length, 
						 sizeof(request.data.auth_crap.nt_resp));
	
	memcpy(request.data.auth_crap.lm_resp, user_info->lm_resp.data, 
	       request.data.auth_crap.lm_resp_len);
	memcpy(request.data.auth_crap.nt_resp, user_info->nt_resp.data, 
	       request.data.auth_crap.nt_resp_len);

	/* we are contacting the privileged pipe */
	become_root();
	result = winbindd_request_response(WINBINDD_PAM_AUTH_CRAP, &request, &response);
	unbecome_root();

	if ( result == NSS_STATUS_UNAVAIL )  {
		struct auth_methods *auth_method = my_private_data;

		if ( auth_method )
			return auth_method->auth(auth_context, auth_method->private_data, 
				mem_ctx, user_info, server_info);
		else
			/* log an error since this should not happen */
			DEBUG(0,("check_winbind_security: ERROR!  my_private_data == NULL!\n"));
	}

	nt_status = NT_STATUS(response.data.auth.nt_status);

	if (result == NSS_STATUS_SUCCESS && response.extra_data.data) {
		if (NT_STATUS_IS_OK(nt_status)) {
			if (NT_STATUS_IS_OK(nt_status = get_info3_from_ndr(mem_ctx, &response, &info3))) { 
				nt_status = make_server_info_info3(mem_ctx, 
					user_info->smb_name, user_info->domain, 
					server_info, &info3); 
			}
			
			if (NT_STATUS_IS_OK(nt_status)) {
				(*server_info)->was_mapped |= user_info->was_mapped;
			}
		}
	} else if (NT_STATUS_IS_OK(nt_status)) {
		nt_status = NT_STATUS_NO_LOGON_SERVERS;
	}

	SAFE_FREE(response.extra_data.data);
        return nt_status;
}

/* module initialisation */
static NTSTATUS auth_init_winbind(struct auth_context *auth_context, const char *param, auth_methods **auth_method) 
{
	if (!make_auth_methods(auth_context, auth_method)) {
		return NT_STATUS_NO_MEMORY;
	}

	(*auth_method)->name = "winbind";
	(*auth_method)->auth = check_winbind_security;

	if (param && *param) {
		/* we load the 'fallback' module - if winbind isn't here, call this
		   module */
		if (!load_auth_module(auth_context, param, (auth_methods **)&(*auth_method)->private_data)) {
			return NT_STATUS_UNSUCCESSFUL;
		}
		
	}
	return NT_STATUS_OK;
}

NTSTATUS auth_winbind_init(void)
{
	return smb_register_auth(AUTH_INTERFACE_VERSION, "winbind", auth_init_winbind);
}
