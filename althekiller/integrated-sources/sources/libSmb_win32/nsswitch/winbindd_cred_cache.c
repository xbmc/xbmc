/*
   Unix SMB/CIFS implementation.

   Winbind daemon - krb5 credential cache funcions

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

#define MAX_CCACHES 100 

static struct WINBINDD_CCACHE_ENTRY *ccache_list;

static TALLOC_CTX *mem_ctx;

const char *get_ccache_name_by_username(const char *username) 
{
	struct WINBINDD_CCACHE_ENTRY *entry;

	for (entry = ccache_list; entry; entry = entry->next) {
		if (strequal(entry->username, username)) {
			return entry->ccname;
		}
	}
	return NULL;
}

struct WINBINDD_CCACHE_ENTRY *get_ccache_by_username(const char *username)
{
	struct WINBINDD_CCACHE_ENTRY *entry;

	for (entry = ccache_list; entry; entry = entry->next) {
		if (strequal(entry->username, username)) {
			return entry;
		}
	}
	return NULL;
}

static int ccache_entry_count(void)
{
	struct WINBINDD_CCACHE_ENTRY *entry;
	int i = 0;

	for (entry = ccache_list; entry; entry = entry->next) {
		i++;
	}
	return i;
}

NTSTATUS remove_ccache_by_ccname(const char *ccname)
{
	struct WINBINDD_CCACHE_ENTRY *entry;

	for (entry = ccache_list; entry; entry = entry->next) {
		if (strequal(entry->ccname, ccname)) {
			DLIST_REMOVE(ccache_list, entry);
			TALLOC_FREE(entry->event); /* unregisters events */
#ifdef HAVE_MUNLOCK
			if (entry->pass) {	
				size_t len = strlen(entry->pass)+1;
#ifdef DEBUG_PASSWORD
				DEBUG(10,("unlocking memory: %p\n", entry->pass));
#endif
				memset(entry->pass, 0, len);
				if ((munlock(entry->pass, len)) == -1) {
					DEBUG(0,("failed to munlock memory: %s (%d)\n", 
						strerror(errno), errno));
					return map_nt_error_from_unix(errno);
				}
#ifdef DEBUG_PASSWORD
				DEBUG(10,("munlocked memory: %p\n", entry->pass));
#endif
			}
#endif /* HAVE_MUNLOCK */
			TALLOC_FREE(entry);
			DEBUG(10,("remove_ccache_by_ccname: removed ccache %s\n", ccname));
			return NT_STATUS_OK;
		}
	}
	return NT_STATUS_OBJECT_NAME_NOT_FOUND;
}

static void krb5_ticket_refresh_handler(struct timed_event *te,
					const struct timeval *now,
					void *private_data)
{
	struct WINBINDD_CCACHE_ENTRY *entry =
		talloc_get_type_abort(private_data, struct WINBINDD_CCACHE_ENTRY);
	int ret;
	time_t new_start;
	struct timeval t;


	DEBUG(10,("krb5_ticket_refresh_handler called\n"));
	DEBUGADD(10,("event called for: %s, %s\n", entry->ccname, entry->username));

	TALLOC_FREE(entry->event);

#ifdef HAVE_KRB5

	/* Kinit again if we have the user password and we can't renew the old
	 * tgt anymore */

	if ((entry->renew_until < time(NULL)) && (entry->pass != NULL)) {
	     
		set_effective_uid(entry->uid);

		ret = kerberos_kinit_password_ext(entry->principal_name,
						  entry->pass,
						  0, /* hm, can we do time correction here ? */
						  &entry->refresh_time,
						  &entry->renew_until,
						  entry->ccname,
						  False, /* no PAC required anymore */
						  True,
						  WINBINDD_PAM_AUTH_KRB5_RENEW_TIME);
		gain_root_privilege();

		if (ret) {
			DEBUG(3,("could not re-kinit: %s\n", error_message(ret)));
			TALLOC_FREE(entry->event);
			return;
		}

		DEBUG(10,("successful re-kinit for: %s in ccache: %s\n", 
			entry->principal_name, entry->ccname));

		new_start = entry->refresh_time;

		goto done;
	}

	set_effective_uid(entry->uid);

	ret = smb_krb5_renew_ticket(entry->ccname, 
				    entry->principal_name,
				    entry->service,
				    &new_start);
	gain_root_privilege();

	if (ret) {
		DEBUG(3,("could not renew tickets: %s\n", error_message(ret)));
		/* maybe we are beyond the renewing window */
		return;
	}

done:

	t = timeval_set(new_start, 0);

	entry->event = add_timed_event(mem_ctx, 
				       t,
				       "krb5_ticket_refresh_handler",
				       krb5_ticket_refresh_handler,
				       entry);

#endif
}

NTSTATUS add_ccache_to_list(const char *princ_name,
			    const char *ccname,
			    const char *service,
			    const char *username, 
			    const char *sid_string,
			    const char *pass,
			    uid_t uid,
			    time_t create_time, 
			    time_t ticket_end, 
			    time_t renew_until, 
			    BOOL schedule_refresh_event)
{
	struct WINBINDD_CCACHE_ENTRY *new_entry = NULL;
	struct WINBINDD_CCACHE_ENTRY *old_entry = NULL;
	NTSTATUS status;

	if ((username == NULL && sid_string == NULL && princ_name == NULL) || 
	    ccname == NULL) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	status = init_ccache_list();
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}

	if (mem_ctx == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	if (ccache_entry_count() + 1 > MAX_CCACHES) {
		DEBUG(10,("add_ccache_to_list: max number of ccaches reached\n"));
		return NT_STATUS_NO_MORE_ENTRIES;
	}

	/* get rid of old entries */
	old_entry = get_ccache_by_username(username);
	if (old_entry) {
		status = remove_ccache_by_ccname(old_entry->ccname);
		if (!NT_STATUS_IS_OK(status)) {
			DEBUG(10,("add_ccache_to_list: failed to delete old ccache entry\n"));
			return status;
		}
	}
	
	new_entry = TALLOC_P(mem_ctx, struct WINBINDD_CCACHE_ENTRY);
	if (new_entry == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	ZERO_STRUCTP(new_entry);

	if (username) {
		new_entry->username = talloc_strdup(mem_ctx, username);
		NT_STATUS_HAVE_NO_MEMORY(new_entry->username);
	}
	if (sid_string) {
		new_entry->sid_string = talloc_strdup(mem_ctx, sid_string);
		NT_STATUS_HAVE_NO_MEMORY(new_entry->sid_string);
	}
	if (princ_name) {
		new_entry->principal_name = talloc_strdup(mem_ctx, princ_name);
		NT_STATUS_HAVE_NO_MEMORY(new_entry->principal_name);
	}
	if (service) {
		new_entry->service = talloc_strdup(mem_ctx, service);
		NT_STATUS_HAVE_NO_MEMORY(new_entry->service);
	}

	if (schedule_refresh_event && pass) {
#ifdef HAVE_MLOCK
		size_t len = strlen(pass)+1;
		
		new_entry->pass = TALLOC_ZERO(mem_ctx, len);
		NT_STATUS_HAVE_NO_MEMORY(new_entry->pass);
		
#ifdef DEBUG_PASSWORD
		DEBUG(10,("mlocking memory: %p\n", new_entry->pass));
#endif		
		if ((mlock(new_entry->pass, len)) == -1) {
			DEBUG(0,("failed to mlock memory: %s (%d)\n", 
				strerror(errno), errno));
			return map_nt_error_from_unix(errno);
		} 
		
#ifdef DEBUG_PASSWORD
		DEBUG(10,("mlocked memory: %p\n", new_entry->pass));
#endif		
		memcpy(new_entry->pass, pass, len);
#else
		new_entry->pass = talloc_strdup(mem_ctx, pass);
		NT_STATUS_HAVE_NO_MEMORY(new_entry->pass);
#endif /* HAVE_MLOCK */
	}

	new_entry->create_time = create_time;
	new_entry->renew_until = renew_until;
	new_entry->ccname = talloc_strdup(mem_ctx, ccname);
	if (new_entry->ccname == NULL) {
		return NT_STATUS_NO_MEMORY;
	}
	new_entry->uid = uid;


	if (schedule_refresh_event && renew_until > 0) {

		struct timeval t = timeval_set((ticket_end -1 ), 0);

		new_entry->event = add_timed_event(mem_ctx, 
						   t,
						   "krb5_ticket_refresh_handler",
						   krb5_ticket_refresh_handler,
						   new_entry);
	}

	DLIST_ADD(ccache_list, new_entry);

	DEBUG(10,("add_ccache_to_list: added ccache [%s] for user [%s] to the list\n", ccname, username));

	return NT_STATUS_OK;
}

NTSTATUS destroy_ccache_list(void)
{
#ifdef HAVE_MUNLOCKALL
	if ((munlockall()) == -1) {
		DEBUG(0,("failed to unlock memory: %s (%d)\n", 
			strerror(errno), errno));
		return map_nt_error_from_unix(errno);
	}
#endif /* HAVE_MUNLOCKALL */
	return talloc_destroy(mem_ctx) ? NT_STATUS_OK : NT_STATUS_UNSUCCESSFUL;
}

NTSTATUS init_ccache_list(void)
{
	if (ccache_list) {
		return NT_STATUS_OK;
	}

	mem_ctx = talloc_init("winbindd_ccache_krb5_handling");
	if (mem_ctx == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	ZERO_STRUCTP(ccache_list);

	return NT_STATUS_OK;
}
