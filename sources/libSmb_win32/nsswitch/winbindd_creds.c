/*
   Unix SMB/CIFS implementation.

   Winbind daemon - cached credentials funcions

   Copyright (C) Guenther Deschner 2005
   
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
#include "winbindd.h"
#undef DBGC_CLASS
#define DBGC_CLASS DBGC_WINBIND

#define MAX_CACHED_LOGINS 10

NTSTATUS winbindd_get_creds(struct winbindd_domain *domain,
			    TALLOC_CTX *mem_ctx,
			    const DOM_SID *sid,
			    NET_USER_INFO_3 **info3,
			    const uint8 *cached_nt_pass[NT_HASH_LEN])
{
	NET_USER_INFO_3 *info;
	NTSTATUS status;

	status = wcache_get_creds(domain, mem_ctx, sid, cached_nt_pass);
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}

	info = netsamlogon_cache_get(mem_ctx, sid);
	if (info == NULL) {
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;
	}

	*info3 = info;

	return NT_STATUS_OK;
}


NTSTATUS winbindd_store_creds(struct winbindd_domain *domain,
			      TALLOC_CTX *mem_ctx, 
			      const char *user, 
			      const char *pass, 
			      NET_USER_INFO_3 *info3,
			      const DOM_SID *user_sid)
{
	NTSTATUS status;
	uchar nt_pass[NT_HASH_LEN];
	DOM_SID cred_sid;

	if (info3 != NULL) {
	
		DOM_SID sid;
		sid_copy(&sid, &(info3->dom_sid.sid));
		sid_append_rid(&sid, info3->user_rid);
		sid_copy(&cred_sid, &sid);
		info3->user_flgs |= LOGON_CACHED_ACCOUNT;
		
	} else if (user_sid != NULL) {
	
		sid_copy(&cred_sid, user_sid);
		
	} else if (user != NULL) {
	
		/* do lookup ourself */

		enum SID_NAME_USE type;
		
		if (!lookup_cached_name(mem_ctx,
	        	                domain->name,
					user,
					&cred_sid,
					&type)) {
			return NT_STATUS_NO_SUCH_USER;
		}
	} else {
		return NT_STATUS_INVALID_PARAMETER;
	}
		
	if (pass) {

		int count = 0;

		status = wcache_count_cached_creds(domain, &count);
		if (!NT_STATUS_IS_OK(status)) {
			return status;
		}

		DEBUG(11,("we have %d cached creds\n", count));

		if (count + 1 > MAX_CACHED_LOGINS) {

			DEBUG(10,("need to delete the oldest cached login\n"));

			status = wcache_remove_oldest_cached_creds(domain, &cred_sid);
			if (!NT_STATUS_IS_OK(status)) {
				DEBUG(10,("failed to remove oldest cached cred: %s\n", 
					nt_errstr(status)));
				return status;
			}
		}

		E_md4hash(pass, nt_pass);

#if DEBUG_PASSWORD
		dump_data(100, (const char *)nt_pass, NT_HASH_LEN);
#endif

		status = wcache_save_creds(domain, mem_ctx, &cred_sid, nt_pass);
		if (!NT_STATUS_IS_OK(status)) {
			return status;
		}
	}

	if (info3 != NULL && user != NULL) {
		if (!netsamlogon_cache_store(user, info3)) {
			return NT_STATUS_ACCESS_DENIED;
		}
	}

	return NT_STATUS_OK;
}

NTSTATUS winbindd_update_creds_by_info3(struct winbindd_domain *domain,
				        TALLOC_CTX *mem_ctx,
				        const char *user,
				        const char *pass,
				        NET_USER_INFO_3 *info3)
{
	return winbindd_store_creds(domain, mem_ctx, user, pass, info3, NULL);
}

NTSTATUS winbindd_update_creds_by_sid(struct winbindd_domain *domain,
				      TALLOC_CTX *mem_ctx,
				      const DOM_SID *sid,
				      const char *pass)
{
	return winbindd_store_creds(domain, mem_ctx, NULL, pass, NULL, sid);
}

NTSTATUS winbindd_update_creds_by_name(struct winbindd_domain *domain,
				       TALLOC_CTX *mem_ctx,
				       const char *user,
				       const char *pass)
{
	return winbindd_store_creds(domain, mem_ctx, user, pass, NULL, NULL);
}


