/* 
   Unix SMB/CIFS implementation.

   Winbind daemon - user related functions

   Copyright (C) Tim Potter 2000
   Copyright (C) Jeremy Allison 2001.
   Copyright (C) Gerald (Jerry) Carter 2003.
   
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

extern userdom_struct current_user_info;

static BOOL fillup_pw_field(const char *lp_template, 
			    const char *username, 
			    const char *domname,
			    uid_t uid,
			    gid_t gid,
			    const char *in, 
			    fstring out)
{
	char *templ;

	if (out == NULL)
		return False;

	if (in && !strequal(in,"") && lp_security() == SEC_ADS && (get_nss_info(domname))) {
		safe_strcpy(out, in, sizeof(fstring) - 1);
		return True;
	}

	/* Home directory and shell - use template config parameters.  The
	   defaults are /tmp for the home directory and /bin/false for
	   shell. */
	
	/* The substitution of %U and %D in the 'template homedir' is done
	   by alloc_sub_specified() below. */

	templ = alloc_sub_specified(lp_template, username, domname, uid, gid);
		
	if (!templ)
		return False;

	safe_strcpy(out, templ, sizeof(fstring) - 1);
	SAFE_FREE(templ);
		
	return True;
	
}
/* Fill a pwent structure with information we have obtained */

static BOOL winbindd_fill_pwent(char *dom_name, char *user_name, 
				DOM_SID *user_sid, DOM_SID *group_sid,
				char *full_name, char *homedir, char *shell,
				struct winbindd_pw *pw)
{
	fstring output_username;
	fstring sid_string;
	
	if (!pw || !dom_name || !user_name)
		return False;
	
	/* Resolve the uid number */

	if (!NT_STATUS_IS_OK(idmap_sid_to_uid(user_sid, &pw->pw_uid, 0))) {
		DEBUG(1, ("error getting user id for sid %s\n", sid_to_string(sid_string, user_sid)));
		return False;
	}
	
	/* Resolve the gid number */   

	if (!NT_STATUS_IS_OK(idmap_sid_to_gid(group_sid, &pw->pw_gid, 0))) {
		DEBUG(1, ("error getting group id for sid %s\n", sid_to_string(sid_string, group_sid)));
		return False;
	}

	strlower_m(user_name);

	/* Username */

	fill_domain_username(output_username, dom_name, user_name, True); 

	safe_strcpy(pw->pw_name, output_username, sizeof(pw->pw_name) - 1);
	
	/* Full name (gecos) */
	
	safe_strcpy(pw->pw_gecos, full_name, sizeof(pw->pw_gecos) - 1);

	/* Home directory and shell - use template config parameters.  The
	   defaults are /tmp for the home directory and /bin/false for
	   shell. */
	
	/* The substitution of %U and %D in the 'template homedir' is done
	   by alloc_sub_specified() below. */

	fstrcpy(current_user_info.domain, dom_name);

	if (!fillup_pw_field(lp_template_homedir(), user_name, dom_name, 
			     pw->pw_uid, pw->pw_gid, homedir, pw->pw_dir))
		return False;

	if (!fillup_pw_field(lp_template_shell(), user_name, dom_name, 
			     pw->pw_uid, pw->pw_gid, shell, pw->pw_shell))
		return False;

	/* Password - set to "*" as we can't generate anything useful here.
	   Authentication can be done using the pam_winbind module. */

	safe_strcpy(pw->pw_passwd, "*", sizeof(pw->pw_passwd) - 1);

	return True;
}

/* Wrapper for domain->methods->query_user, only on the parent->child pipe */

enum winbindd_result winbindd_dual_userinfo(struct winbindd_domain *domain,
					    struct winbindd_cli_state *state)
{
	DOM_SID sid;
	WINBIND_USERINFO user_info;
	NTSTATUS status;

	/* Ensure null termination */
	state->request.data.sid[sizeof(state->request.data.sid)-1]='\0';

	DEBUG(3, ("[%5lu]: lookupsid %s\n", (unsigned long)state->pid, 
		  state->request.data.sid));

	if (!string_to_sid(&sid, state->request.data.sid)) {
		DEBUG(5, ("%s not a SID\n", state->request.data.sid));
		return WINBINDD_ERROR;
	}

	status = domain->methods->query_user(domain, state->mem_ctx,
					     &sid, &user_info);
	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(1, ("error getting user info for sid %s\n",
			  sid_string_static(&sid)));
		return WINBINDD_ERROR;
	}

	fstrcpy(state->response.data.user_info.acct_name, user_info.acct_name);
	fstrcpy(state->response.data.user_info.full_name, user_info.full_name);
	fstrcpy(state->response.data.user_info.homedir, user_info.homedir);
	fstrcpy(state->response.data.user_info.shell, user_info.shell);
	if (!sid_peek_check_rid(&domain->sid, &user_info.group_sid,
				&state->response.data.user_info.group_rid)) {
		DEBUG(1, ("Could not extract group rid out of %s\n",
			  sid_string_static(&sid)));
		return WINBINDD_ERROR;
	}

	return WINBINDD_OK;
}

struct getpwsid_state {
	struct winbindd_cli_state *state;
	struct winbindd_domain *domain;
	char *username;
	char *fullname;
	char *homedir;
	char *shell;
	DOM_SID user_sid;
	uid_t uid;
	DOM_SID group_sid;
	gid_t gid;
};

static void getpwsid_queryuser_recv(void *private_data, BOOL success,
				    const char *acct_name,
				    const char *full_name, 
				    const char *homedir,
				    const char *shell,
				    uint32 group_rid);
static void getpwsid_sid2uid_recv(void *private_data, BOOL success, uid_t uid);
static void getpwsid_sid2gid_recv(void *private_data, BOOL success, gid_t gid);

static void winbindd_getpwsid(struct winbindd_cli_state *state,
			      const DOM_SID *sid)
{
	struct getpwsid_state *s;

	s = TALLOC_ZERO_P(state->mem_ctx, struct getpwsid_state);
	if (s == NULL) {
		DEBUG(0, ("talloc failed\n"));
		goto error;
	}

	s->state = state;
	s->domain = find_domain_from_sid_noinit(sid);
	if (s->domain == NULL) {
		DEBUG(3, ("Could not find domain for sid %s\n",
			  sid_string_static(sid)));
		goto error;
	}

	sid_copy(&s->user_sid, sid);

	query_user_async(s->state->mem_ctx, s->domain, sid,
			 getpwsid_queryuser_recv, s);
	return;

 error:
	request_error(state);
}
	
static void getpwsid_queryuser_recv(void *private_data, BOOL success,
				    const char *acct_name,
				    const char *full_name, 
				    const char *homedir,
				    const char *shell,
				    uint32 group_rid)
{
	fstring username;
	struct getpwsid_state *s =
		talloc_get_type_abort(private_data, struct getpwsid_state);

	if (!success) {
		DEBUG(5, ("Could not query domain %s SID %s\n", s->domain->name,
			  sid_string_static(&s->user_sid)));
		request_error(s->state);
		return;
	}

	fstrcpy( username, acct_name );
	strlower_m( username );
	s->username = talloc_strdup(s->state->mem_ctx, username);
	s->fullname = talloc_strdup(s->state->mem_ctx, full_name);
	s->homedir = talloc_strdup(s->state->mem_ctx, homedir);
	s->shell = talloc_strdup(s->state->mem_ctx, shell);
	sid_copy(&s->group_sid, &s->domain->sid);
	sid_append_rid(&s->group_sid, group_rid);

	winbindd_sid2uid_async(s->state->mem_ctx, &s->user_sid,
			       getpwsid_sid2uid_recv, s);
}

static void getpwsid_sid2uid_recv(void *private_data, BOOL success, uid_t uid)
{
	struct getpwsid_state *s =
		talloc_get_type_abort(private_data, struct getpwsid_state);

	if (!success) {
		DEBUG(5, ("Could not query user's %s\\%s uid\n",
			  s->domain->name, s->username));
		request_error(s->state);
		return;
	}

	s->uid = uid;
	winbindd_sid2gid_async(s->state->mem_ctx, &s->group_sid,
			       getpwsid_sid2gid_recv, s);
}

static void getpwsid_sid2gid_recv(void *private_data, BOOL success, gid_t gid)
{
	struct getpwsid_state *s =
		talloc_get_type_abort(private_data, struct getpwsid_state);
	struct winbindd_pw *pw;
	fstring output_username;

	if (!success) {
		DEBUG(5, ("Could not query user's %s\\%s\n gid",
			  s->domain->name, s->username));
		goto failed;
	}

	s->gid = gid;

	pw = &s->state->response.data.pw;
	pw->pw_uid = s->uid;
	pw->pw_gid = s->gid;
	fill_domain_username(output_username, s->domain->name, s->username, True); 
	safe_strcpy(pw->pw_name, output_username, sizeof(pw->pw_name) - 1);
	safe_strcpy(pw->pw_gecos, s->fullname, sizeof(pw->pw_gecos) - 1);

	fstrcpy(current_user_info.domain, s->domain->name);

	if (!fillup_pw_field(lp_template_homedir(), s->username, s->domain->name, 
			     pw->pw_uid, pw->pw_gid, s->homedir, pw->pw_dir)) {
		DEBUG(5, ("Could not compose homedir\n"));
		goto failed;
	}

	if (!fillup_pw_field(lp_template_shell(), s->username, s->domain->name, 
			     pw->pw_uid, pw->pw_gid, s->shell, pw->pw_shell)) {
		DEBUG(5, ("Could not compose shell\n"));
		goto failed;
	}

	/* Password - set to "*" as we can't generate anything useful here.
	   Authentication can be done using the pam_winbind module. */

	safe_strcpy(pw->pw_passwd, "*", sizeof(pw->pw_passwd) - 1);

	request_ok(s->state);
	return;

 failed:
	request_error(s->state);
}

/* Return a password structure from a username.  */

static void getpwnam_name2sid_recv(void *private_data, BOOL success,
				   const DOM_SID *sid, enum SID_NAME_USE type);

void winbindd_getpwnam(struct winbindd_cli_state *state)
{
	struct winbindd_domain *domain;
	fstring domname, username;

	/* Ensure null termination */
	state->request.data.username[sizeof(state->request.data.username)-1]='\0';

	DEBUG(3, ("[%5lu]: getpwnam %s\n", (unsigned long)state->pid,
		  state->request.data.username));

	if (!parse_domain_user(state->request.data.username, domname,
			       username)) {
		DEBUG(5, ("Could not parse domain user: %s\n",
			  state->request.data.username));
		request_error(state);
		return;
	}
	
	/* Get info for the domain */

	domain = find_domain_from_name(domname);

	if (domain == NULL) {
		DEBUG(7, ("could not find domain entry for domain %s\n",
			  domname));
		request_error(state);
		return;
	}

	if ( strequal(domname, lp_workgroup()) && lp_winbind_trusted_domains_only() ) {
		DEBUG(7,("winbindd_getpwnam: My domain -- rejecting getpwnam() for %s\\%s.\n", 
			domname, username));
		request_error(state);
		return;
	}	

	/* Get rid and name type from name.  The following costs 1 packet */

	winbindd_lookupname_async(state->mem_ctx, domname, username,
				  getpwnam_name2sid_recv, state);
}

static void getpwnam_name2sid_recv(void *private_data, BOOL success,
				   const DOM_SID *sid, enum SID_NAME_USE type)
{
	struct winbindd_cli_state *state = private_data;

	if (!success) {
		DEBUG(5, ("Could not lookup name for user %s\n",
			  state->request.data.username));
		request_error(state);
		return;
	}

	if ((type != SID_NAME_USER) && (type != SID_NAME_COMPUTER)) {
		DEBUG(5, ("%s is not a user\n", state->request.data.username));
		request_error(state);
		return;
	}

	winbindd_getpwsid(state, sid);
}

static void getpwuid_recv(void *private_data, BOOL success, const char *sid)
{
	struct winbindd_cli_state *state = private_data;
	DOM_SID user_sid;

	if (!success) {
		DEBUG(10,("uid2sid_recv: uid [%lu] to sid mapping failed\n.",
			  (unsigned long)(state->request.data.uid)));
		request_error(state);
		return;
	}
	
	DEBUG(10,("uid2sid_recv: uid %lu has sid %s\n",
		  (unsigned long)(state->request.data.uid), sid));

	string_to_sid(&user_sid, sid);
	winbindd_getpwsid(state, &user_sid);
}

/* Return a password structure given a uid number */

void winbindd_getpwuid(struct winbindd_cli_state *state)
{
	DOM_SID user_sid;
	NTSTATUS status;
	
	/* Bug out if the uid isn't in the winbind range */

	if ((state->request.data.uid < server_state.uid_low ) ||
	    (state->request.data.uid > server_state.uid_high)) {
		request_error(state);
		return;
	}

	DEBUG(3, ("[%5lu]: getpwuid %lu\n", (unsigned long)state->pid, 
		  (unsigned long)state->request.data.uid));

	status = idmap_uid_to_sid(&user_sid, state->request.data.uid,
				  ID_QUERY_ONLY | ID_CACHE_ONLY);

	if (NT_STATUS_IS_OK(status)) {
		winbindd_getpwsid(state, &user_sid);
		return;
	}

	DEBUG(10,("Could not find SID for uid %lu in the cache. Querying idmap backend\n",
		  (unsigned long)state->request.data.uid));

	winbindd_uid2sid_async(state->mem_ctx, state->request.data.uid, getpwuid_recv, state);
}

/*
 * set/get/endpwent functions
 */

/* Rewind file pointer for ntdom passwd database */

static BOOL winbindd_setpwent_internal(struct winbindd_cli_state *state)
{
	struct winbindd_domain *domain;
        
	DEBUG(3, ("[%5lu]: setpwent\n", (unsigned long)state->pid));
        
	/* Check user has enabled this */
        
	if (!lp_winbind_enum_users()) {
		return False;
	}

	/* Free old static data if it exists */
        
	if (state->getpwent_state != NULL) {
		free_getent_state(state->getpwent_state);
		state->getpwent_state = NULL;
	}

#if 0	/* JERRY */
	/* add any local users we have */
	        
	if ( (domain_state = (struct getent_state *)malloc(sizeof(struct getent_state))) == NULL )
		return False;
                
	ZERO_STRUCTP(domain_state);

	/* Add to list of open domains */
                
	DLIST_ADD(state->getpwent_state, domain_state);
#endif
        
	/* Create sam pipes for each domain we know about */
        
	for(domain = domain_list(); domain != NULL; domain = domain->next) {
		struct getent_state *domain_state;
                
		
		/* don't add our domaina if we are a PDC or if we 
		   are a member of a Samba domain */
		
		if ( (IS_DC || lp_winbind_trusted_domains_only())
			&& strequal(domain->name, lp_workgroup()) )
		{
			continue;
		}
						
		/* Create a state record for this domain */
                
		if ((domain_state = SMB_MALLOC_P(struct getent_state)) == NULL) {
			DEBUG(0, ("malloc failed\n"));
			return False;
		}
                
		ZERO_STRUCTP(domain_state);

		fstrcpy(domain_state->domain_name, domain->name);

		/* Add to list of open domains */
                
		DLIST_ADD(state->getpwent_state, domain_state);
	}
        
	state->getpwent_initialized = True;
	return True;
}

void winbindd_setpwent(struct winbindd_cli_state *state)
{
	if (winbindd_setpwent_internal(state)) {
		request_ok(state);
	} else {
		request_error(state);
	}
}

/* Close file pointer to ntdom passwd database */

void winbindd_endpwent(struct winbindd_cli_state *state)
{
	DEBUG(3, ("[%5lu]: endpwent\n", (unsigned long)state->pid));

	free_getent_state(state->getpwent_state);    
	state->getpwent_initialized = False;
	state->getpwent_state = NULL;
	request_ok(state);
}

/* Get partial list of domain users for a domain.  We fill in the sam_entries,
   and num_sam_entries fields with domain user information.  The dispinfo_ndx
   field is incremented to the index of the next user to fetch.  Return True if
   some users were returned, False otherwise. */

static BOOL get_sam_user_entries(struct getent_state *ent, TALLOC_CTX *mem_ctx)
{
	NTSTATUS status;
	uint32 num_entries;
	WINBIND_USERINFO *info;
	struct getpwent_user *name_list = NULL;
	BOOL result = False;
	struct winbindd_domain *domain;
	struct winbindd_methods *methods;
	unsigned int i;

	if (ent->num_sam_entries)
		return False;

	if (!(domain = find_domain_from_name(ent->domain_name))) {
		DEBUG(3, ("no such domain %s in get_sam_user_entries\n",
			  ent->domain_name));
		return False;
	}

	methods = domain->methods;

	/* Free any existing user info */

	SAFE_FREE(ent->sam_entries);
	ent->num_sam_entries = 0;
	
	/* Call query_user_list to get a list of usernames and user rids */

	num_entries = 0;

	status = methods->query_user_list(domain, mem_ctx, &num_entries, 
					  &info);
		
	if (num_entries) {
		name_list = SMB_REALLOC_ARRAY(name_list, struct getpwent_user, ent->num_sam_entries + num_entries);
		
		if (!name_list) {
			DEBUG(0,("get_sam_user_entries realloc failed.\n"));
			goto done;
		}
	}

	for (i = 0; i < num_entries; i++) {
		/* Store account name and gecos */
		if (!info[i].acct_name) {
			fstrcpy(name_list[ent->num_sam_entries + i].name, "");
		} else {
			fstrcpy(name_list[ent->num_sam_entries + i].name, 
				info[i].acct_name); 
		}
		if (!info[i].full_name) {
			fstrcpy(name_list[ent->num_sam_entries + i].gecos, "");
		} else {
			fstrcpy(name_list[ent->num_sam_entries + i].gecos, 
				info[i].full_name); 
		}
		if (!info[i].homedir) {
			fstrcpy(name_list[ent->num_sam_entries + i].homedir, "");
		} else {
			fstrcpy(name_list[ent->num_sam_entries + i].homedir, 
				info[i].homedir); 
		}
		if (!info[i].shell) {
			fstrcpy(name_list[ent->num_sam_entries + i].shell, "");
		} else {
			fstrcpy(name_list[ent->num_sam_entries + i].shell, 
				info[i].shell); 
		}
	
	
		/* User and group ids */
		sid_copy(&name_list[ent->num_sam_entries+i].user_sid,
			 &info[i].user_sid);
		sid_copy(&name_list[ent->num_sam_entries+i].group_sid,
			 &info[i].group_sid);
	}
		
	ent->num_sam_entries += num_entries;
	
	/* Fill in remaining fields */
	
	ent->sam_entries = name_list;
	ent->sam_entry_index = 0;
	result = ent->num_sam_entries > 0;

 done:

	return result;
}

/* Fetch next passwd entry from ntdom database */

#define MAX_GETPWENT_USERS 500

void winbindd_getpwent(struct winbindd_cli_state *state)
{
	struct getent_state *ent;
	struct winbindd_pw *user_list;
	int num_users, user_list_ndx = 0, i;

	DEBUG(3, ("[%5lu]: getpwent\n", (unsigned long)state->pid));

	/* Check user has enabled this */

	if (!lp_winbind_enum_users()) {
		request_error(state);
		return;
	}

	/* Allocate space for returning a chunk of users */

	num_users = MIN(MAX_GETPWENT_USERS, state->request.data.num_entries);
	
	if ((state->response.extra_data.data = SMB_MALLOC_ARRAY(struct winbindd_pw, num_users)) == NULL) {
		request_error(state);
		return;
	}

	memset(state->response.extra_data.data, 0, num_users * 
	       sizeof(struct winbindd_pw));

	user_list = (struct winbindd_pw *)state->response.extra_data.data;

	if (!state->getpwent_initialized)
		winbindd_setpwent_internal(state);
	
	if (!(ent = state->getpwent_state)) {
		request_error(state);
		return;
	}

	/* Start sending back users */

	for (i = 0; i < num_users; i++) {
		struct getpwent_user *name_list = NULL;
		uint32 result;

		/* Do we need to fetch another chunk of users? */

		if (ent->num_sam_entries == ent->sam_entry_index) {

			while(ent &&
			      !get_sam_user_entries(ent, state->mem_ctx)) {
				struct getent_state *next_ent;

				/* Free state information for this domain */

				SAFE_FREE(ent->sam_entries);

				next_ent = ent->next;
				DLIST_REMOVE(state->getpwent_state, ent);

				SAFE_FREE(ent);
				ent = next_ent;
			}
 
			/* No more domains */

			if (!ent) 
				break;
		}

		name_list = ent->sam_entries;

		/* Lookup user info */
		
		result = winbindd_fill_pwent(
			ent->domain_name, 
			name_list[ent->sam_entry_index].name,
			&name_list[ent->sam_entry_index].user_sid,
			&name_list[ent->sam_entry_index].group_sid,
			name_list[ent->sam_entry_index].gecos,
			name_list[ent->sam_entry_index].homedir,
			name_list[ent->sam_entry_index].shell,
			&user_list[user_list_ndx]);
		
		ent->sam_entry_index++;
		
		/* Add user to return list */
		
		if (result) {
				
			user_list_ndx++;
			state->response.data.num_entries++;
			state->response.length += 
				sizeof(struct winbindd_pw);

		} else
			DEBUG(1, ("could not lookup domain user %s\n",
				  name_list[ent->sam_entry_index].name));
	}

	/* Out of domains */

	if (user_list_ndx > 0)
		request_ok(state);
	else
		request_error(state);
}

/* List domain users without mapping to unix ids */

void winbindd_list_users(struct winbindd_cli_state *state)
{
	struct winbindd_domain *domain;
	WINBIND_USERINFO *info;
	const char *which_domain;
	uint32 num_entries = 0, total_entries = 0;
	char *extra_data = NULL;
	int extra_data_len = 0;
	enum winbindd_result rv = WINBINDD_ERROR;

	DEBUG(3, ("[%5lu]: list users\n", (unsigned long)state->pid));

	/* Ensure null termination */
	state->request.domain_name[sizeof(state->request.domain_name)-1]='\0';	
	which_domain = state->request.domain_name;
	
	/* Enumerate over trusted domains */

	for (domain = domain_list(); domain; domain = domain->next) {
		NTSTATUS status;
		struct winbindd_methods *methods;
		unsigned int i;
		
		/* if we have a domain name restricting the request and this
		   one in the list doesn't match, then just bypass the remainder
		   of the loop */
		   
		if ( *which_domain && !strequal(which_domain, domain->name) )
			continue;
			
		methods = domain->methods;

		/* Query display info */
		status = methods->query_user_list(domain, state->mem_ctx, 
						  &num_entries, &info);

		if (num_entries == 0)
			continue;

		/* Allocate some memory for extra data */
		total_entries += num_entries;
			
		extra_data = SMB_REALLOC(extra_data, sizeof(fstring) * total_entries);
			
		if (!extra_data) {
			DEBUG(0,("failed to enlarge buffer!\n"));
			goto done;
		}

		/* Pack user list into extra data fields */
			
		for (i = 0; i < num_entries; i++) {
			fstring acct_name, name;
			
			if (!info[i].acct_name) {
				fstrcpy(acct_name, "");
			} else {
				fstrcpy(acct_name, info[i].acct_name);
			}
			
			fill_domain_username(name, domain->name, acct_name, True);
			
				/* Append to extra data */
			memcpy(&extra_data[extra_data_len], name, 
			       strlen(name));
			extra_data_len += strlen(name);
			extra_data[extra_data_len++] = ',';
		}   
        }

	/* Assign extra_data fields in response structure */

	if (extra_data) {
		extra_data[extra_data_len - 1] = '\0';
		state->response.extra_data.data = extra_data;
		state->response.length += extra_data_len;
	}

	/* No domains responded but that's still OK so don't return an
	   error. */

	rv = WINBINDD_OK;

 done:

	if (rv == WINBINDD_OK)
		request_ok(state);
	else
		request_error(state);
}
