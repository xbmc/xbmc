/* 
   Unix SMB/CIFS implementation.

   Winbind daemon for ntdom nss module

   Copyright (C) Tim Potter 2000
   Copyright (C) Jeremy Allison 2001.
   Copyright (C) Gerald (Jerry) Carter 2003.
   Copyright (C) Volker Lendecke 2005
   
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

extern BOOL opt_nocache;

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_WINBIND

/***************************************************************
 Empty static struct for negative caching.
****************************************************************/

/* Fill a grent structure from various other information */

static BOOL fill_grent(struct winbindd_gr *gr, const char *dom_name, 
		       const char *gr_name, gid_t unix_gid)
{
	fstring full_group_name;

	fill_domain_username( full_group_name, dom_name, gr_name, True );

	gr->gr_gid = unix_gid;
    
	/* Group name and password */
    
	safe_strcpy(gr->gr_name, full_group_name, sizeof(gr->gr_name) - 1);
	safe_strcpy(gr->gr_passwd, "x", sizeof(gr->gr_passwd) - 1);

	return True;
}

/* Fill in the group membership field of a NT group given by group_sid */

static BOOL fill_grent_mem(struct winbindd_domain *domain,
			   DOM_SID *group_sid, 
			   enum SID_NAME_USE group_name_type, 
			   size_t *num_gr_mem, char **gr_mem, size_t *gr_mem_len)
{
	DOM_SID *sid_mem = NULL;
	uint32 num_names = 0;
	uint32 *name_types = NULL;
	unsigned int buf_len, buf_ndx, i;
	char **names = NULL, *buf;
	BOOL result = False;
	TALLOC_CTX *mem_ctx;
	NTSTATUS status;
	fstring sid_string;

	if (!(mem_ctx = talloc_init("fill_grent_mem(%s)", domain->name)))
		return False;

	/* Initialise group membership information */
	
	DEBUG(10, ("group SID %s\n", sid_to_string(sid_string, group_sid)));

	*num_gr_mem = 0;

	/* HACK ALERT!! This whole routine does not cope with group members
	 * from more than one domain, ie aliases. Thus we have to work it out
	 * ourselves in a special routine. */

	if (domain->internal)
		return fill_passdb_alias_grmem(domain, group_sid,
					       num_gr_mem,
					       gr_mem, gr_mem_len);
	
	if ( !((group_name_type==SID_NAME_DOM_GRP) ||
		((group_name_type==SID_NAME_ALIAS) && domain->primary)) )
	{
		DEBUG(1, ("SID %s in domain %s isn't a domain group (%d)\n", 
			  sid_to_string(sid_string, group_sid), domain->name, 
			  group_name_type));
                goto done;
	}

	/* Lookup group members */
	status = domain->methods->lookup_groupmem(domain, mem_ctx, group_sid, &num_names, 
						  &sid_mem, &names, &name_types);
	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(1, ("could not lookup membership for group rid %s in domain %s (error: %s)\n", 
			  sid_to_string(sid_string, group_sid), domain->name, nt_errstr(status)));

		goto done;
	}

	DEBUG(10, ("looked up %d names\n", num_names));

	if (DEBUGLEVEL >= 10) {
		for (i = 0; i < num_names; i++)
			DEBUG(10, ("\t%20s %s %d\n", names[i],
				   sid_string_static(&sid_mem[i]),
				   name_types[i]));
	}

	/* Add members to list */

	buf = NULL;
	buf_len = buf_ndx = 0;

 again:

	for (i = 0; i < num_names; i++) {
		char *the_name;
		fstring name;
		int len;
			
		the_name = names[i];

		DEBUG(10, ("processing name %s\n", the_name));

		/* FIXME: need to cope with groups within groups.  These
                   occur in Universal groups on a Windows 2000 native mode
                   server. */

		/* make sure to allow machine accounts */

		if (name_types[i] != SID_NAME_USER && name_types[i] != SID_NAME_COMPUTER) {
			DEBUG(3, ("name %s isn't a domain user (%s)\n", the_name, sid_type_lookup(name_types[i])));
			continue;
		}

		/* Append domain name */

		fill_domain_username(name, domain->name, the_name, True);

		len = strlen(name);
		
		/* Add to list or calculate buffer length */

		if (!buf) {
			buf_len += len + 1; /* List is comma separated */
			(*num_gr_mem)++;
			DEBUG(10, ("buf_len + %d = %d\n", len + 1, buf_len));
		} else {
			DEBUG(10, ("appending %s at ndx %d\n", name, len));
			safe_strcpy(&buf[buf_ndx], name, len);
			buf_ndx += len;
			buf[buf_ndx] = ',';
			buf_ndx++;
		}
	}

	/* Allocate buffer */

	if (!buf && buf_len != 0) {
		if (!(buf = SMB_MALLOC(buf_len))) {
			DEBUG(1, ("out of memory\n"));
			result = False;
			goto done;
		}
		memset(buf, 0, buf_len);
		goto again;
	}

	if (buf && buf_ndx > 0) {
		buf[buf_ndx - 1] = '\0';
	}

	*gr_mem = buf;
	*gr_mem_len = buf_len;

	DEBUG(10, ("num_mem = %u, len = %u, mem = %s\n", (unsigned int)*num_gr_mem, 
		   (unsigned int)buf_len, *num_gr_mem ? buf : "NULL")); 
	result = True;

done:

	talloc_destroy(mem_ctx);
	
	DEBUG(10, ("fill_grent_mem returning %d\n", result));

	return result;
}

/* Return a group structure from a group name */

void winbindd_getgrnam(struct winbindd_cli_state *state)
{
	DOM_SID group_sid, tmp_sid;
	uint32 grp_rid;
	struct winbindd_domain *domain;
	enum SID_NAME_USE name_type;
	fstring name_domain, name_group;
	char *tmp, *gr_mem;
	size_t gr_mem_len;
	size_t num_gr_mem;
	gid_t gid;
	union unid_t id;
	NTSTATUS status;
	
	/* Ensure null termination */
	state->request.data.groupname[sizeof(state->request.data.groupname)-1]='\0';

	DEBUG(3, ("[%5lu]: getgrnam %s\n", (unsigned long)state->pid,
		  state->request.data.groupname));

	/* Parse domain and groupname */
	
	memset(name_group, 0, sizeof(fstring));

	tmp = state->request.data.groupname;
	
	parse_domain_user(tmp, name_domain, name_group);

	/* if no domain or our local domain and no local tdb group, default to
	 * our local domain for aliases */

	if ( !*name_domain || strequal(name_domain, get_global_sam_name()) ) {
		fstrcpy(name_domain, get_global_sam_name());
	}

	/* Get info for the domain */

	if ((domain = find_domain_from_name(name_domain)) == NULL) {
		DEBUG(3, ("could not get domain sid for domain %s\n",
			  name_domain));
		request_error(state);
		return;
	}
	/* should we deal with users for our domain? */
	
	if ( lp_winbind_trusted_domains_only() && domain->primary) {
		DEBUG(7,("winbindd_getgrnam: My domain -- rejecting "
			 "getgrnam() for %s\\%s.\n", name_domain, name_group));
		request_error(state);
		return;
	}

	/* Get rid and name type from name */
        
	if (!winbindd_lookup_sid_by_name(state->mem_ctx, domain, domain->name,
					 name_group, &group_sid, &name_type)) {
		DEBUG(1, ("group %s in domain %s does not exist\n", 
			  name_group, name_domain));
		request_error(state);
		return;
	}

	if ( !((name_type==SID_NAME_DOM_GRP) ||
	       ((name_type==SID_NAME_ALIAS) && domain->primary) ||
	       ((name_type==SID_NAME_ALIAS) && domain->internal) ||
	       ((name_type==SID_NAME_WKN_GRP) && domain->internal)) )
	{
		DEBUG(1, ("name '%s' is not a local, domain or builtin "
			  "group: %d\n", name_group, name_type));
		request_error(state);
		return;
	}

	/* Make sure that the group SID is within the domain of the
	   original domain */

	sid_copy( &tmp_sid, &group_sid );
	sid_split_rid( &tmp_sid, &grp_rid );
	if ( !sid_equal( &tmp_sid, &domain->sid ) ) {
		DEBUG(3,("winbindd_getgrnam: group %s resolves to a SID in the wrong domain [%s]\n", 
			state->request.data.groupname, sid_string_static(&group_sid)));
		request_error(state);
		return;
	}

	

	/* Try to get the GID */

	status = idmap_sid_to_gid(&group_sid, &gid, 0);

	if (NT_STATUS_IS_OK(status)) {
		goto got_gid;
	}

	/* Maybe it's one of our aliases in passdb */

	if (pdb_sid_to_id(&group_sid, &id, &name_type) &&
	    ((name_type == SID_NAME_ALIAS) ||
	     (name_type == SID_NAME_WKN_GRP))) {
		gid = id.gid;
		goto got_gid;
	}

	DEBUG(1, ("error converting unix gid to sid\n"));
	request_error(state);
	return;

 got_gid:

	if (!fill_grent(&state->response.data.gr, name_domain,
			name_group, gid) ||
	    !fill_grent_mem(domain, &group_sid, name_type,
			    &num_gr_mem,
			    &gr_mem, &gr_mem_len)) {
		request_error(state);
		return;
	}

	state->response.data.gr.num_gr_mem = (uint32)num_gr_mem;

	/* Group membership lives at start of extra data */

	state->response.data.gr.gr_mem_ofs = 0;

	state->response.length += gr_mem_len;
	state->response.extra_data.data = gr_mem;
	request_ok(state);
}

static void getgrgid_got_sid(struct winbindd_cli_state *state, DOM_SID group_sid)
{
	struct winbindd_domain *domain;
	enum SID_NAME_USE name_type;
	fstring dom_name;
	fstring group_name;
	size_t gr_mem_len;
	size_t num_gr_mem;
	char *gr_mem;

	/* Get name from sid */

	if (!winbindd_lookup_name_by_sid(state->mem_ctx, &group_sid, dom_name,
					 group_name, &name_type)) {
		DEBUG(1, ("could not lookup sid\n"));
		request_error(state);
		return;
	}

	/* Fill in group structure */

	domain = find_domain_from_sid_noinit(&group_sid);

	if (!domain) {
		DEBUG(1,("Can't find domain from sid\n"));
		request_error(state);
		return;
	}

	if ( !((name_type==SID_NAME_DOM_GRP) ||
	       ((name_type==SID_NAME_ALIAS) && domain->primary) ||
	       ((name_type==SID_NAME_ALIAS) && domain->internal)) )
	{
		DEBUG(1, ("name '%s' is not a local or domain group: %d\n", 
			  group_name, name_type));
		request_error(state);
		return;
	}

	if (!fill_grent(&state->response.data.gr, dom_name, group_name, 
			state->request.data.gid) ||
	    !fill_grent_mem(domain, &group_sid, name_type,
			    &num_gr_mem,
			    &gr_mem, &gr_mem_len)) {
		request_error(state);
		return;
	}

	state->response.data.gr.num_gr_mem = (uint32)num_gr_mem;

	/* Group membership lives at start of extra data */

	state->response.data.gr.gr_mem_ofs = 0;

	state->response.length += gr_mem_len;
	state->response.extra_data.data = gr_mem;
	request_ok(state);
}

static void getgrgid_recv(void *private_data, BOOL success, const char *sid)
{
	struct winbindd_cli_state *state = talloc_get_type_abort(private_data, struct winbindd_cli_state);
	enum SID_NAME_USE name_type;
	DOM_SID group_sid;

	if (success) {
		DEBUG(10,("getgrgid_recv: gid %lu has sid %s\n",
			  (unsigned long)(state->request.data.gid), sid));

		string_to_sid(&group_sid, sid);
		getgrgid_got_sid(state, group_sid);
		return;
	}

	/* Ok, this might be "ours", i.e. an alias */
	if (pdb_gid_to_sid(state->request.data.gid, &group_sid) &&
	    lookup_sid(state->mem_ctx, &group_sid, NULL, NULL, &name_type) &&
	    (name_type == SID_NAME_ALIAS)) {
		/* Hey, got an alias */
		DEBUG(10,("getgrgid_recv: we have an alias with gid %lu and sid %s\n",
			  (unsigned long)(state->request.data.gid), sid));
		getgrgid_got_sid(state, group_sid);
		return;
	}

	DEBUG(1, ("could not convert gid %lu to sid\n", 
		  (unsigned long)state->request.data.gid));
	request_error(state);
}

/* Return a group structure from a gid number */
void winbindd_getgrgid(struct winbindd_cli_state *state)
{
	DOM_SID group_sid;
	NTSTATUS status;

	DEBUG(3, ("[%5lu]: getgrgid %lu\n", (unsigned long)state->pid, 
		  (unsigned long)state->request.data.gid));

	/* Bug out if the gid isn't in the winbind range */

	if ((state->request.data.gid < server_state.gid_low) ||
	    (state->request.data.gid > server_state.gid_high)) {
		request_error(state);
		return;
	}

	/* Get sid from gid */

	status = idmap_gid_to_sid(&group_sid, state->request.data.gid, ID_EMPTY);
	if (NT_STATUS_IS_OK(status)) {
		/* This is a remote one */
		getgrgid_got_sid(state, group_sid);
		return;
	}

	DEBUG(10,("winbindd_getgrgid: gid %lu not found in cache, try with the async interface\n",
		  (unsigned long)state->request.data.gid));

	winbindd_gid2sid_async(state->mem_ctx, state->request.data.gid, getgrgid_recv, state);
}

/*
 * set/get/endgrent functions
 */

/* "Rewind" file pointer for group database enumeration */

static BOOL winbindd_setgrent_internal(struct winbindd_cli_state *state)
{
	struct winbindd_domain *domain;

	DEBUG(3, ("[%5lu]: setgrent\n", (unsigned long)state->pid));

	/* Check user has enabled this */

	if (!lp_winbind_enum_groups()) {
		return False;
	}		

	/* Free old static data if it exists */
	
	if (state->getgrent_state != NULL) {
		free_getent_state(state->getgrent_state);
		state->getgrent_state = NULL;
	}
	
	/* Create sam pipes for each domain we know about */
	
	for (domain = domain_list(); domain != NULL; domain = domain->next) {
		struct getent_state *domain_state;
		
		/* Create a state record for this domain */

		/* don't add our domaina if we are a PDC or if we 
		   are a member of a Samba domain */
		
		if ( lp_winbind_trusted_domains_only() && domain->primary )
		{
			continue;
		}
						
		
		if ((domain_state = SMB_MALLOC_P(struct getent_state)) == NULL) {
			DEBUG(1, ("winbindd_setgrent: malloc failed for domain_state!\n"));
			return False;
		}
		
		ZERO_STRUCTP(domain_state);
		
		fstrcpy(domain_state->domain_name, domain->name);

		/* Add to list of open domains */
		
		DLIST_ADD(state->getgrent_state, domain_state);
	}
	
	state->getgrent_initialized = True;
	return True;
}

void winbindd_setgrent(struct winbindd_cli_state *state)
{
	if (winbindd_setgrent_internal(state)) {
		request_ok(state);
	} else {
		request_error(state);
	}
}

/* Close file pointer to ntdom group database */

void winbindd_endgrent(struct winbindd_cli_state *state)
{
	DEBUG(3, ("[%5lu]: endgrent\n", (unsigned long)state->pid));

	free_getent_state(state->getgrent_state);
	state->getgrent_initialized = False;
	state->getgrent_state = NULL;
	request_ok(state);
}

/* Get the list of domain groups and domain aliases for a domain.  We fill in
   the sam_entries and num_sam_entries fields with domain group information.  
   The dispinfo_ndx field is incremented to the index of the next group to 
   fetch. Return True if some groups were returned, False otherwise. */

static BOOL get_sam_group_entries(struct getent_state *ent)
{
	NTSTATUS status;
	uint32 num_entries;
	struct acct_info *name_list = NULL;
	TALLOC_CTX *mem_ctx;
	BOOL result = False;
	struct acct_info *sam_grp_entries = NULL;
	struct winbindd_domain *domain;
        
	if (ent->got_sam_entries)
		return False;

	if (!(mem_ctx = talloc_init("get_sam_group_entries(%s)",
					  ent->domain_name))) {
		DEBUG(1, ("get_sam_group_entries: could not create talloc context!\n")); 
		return False;
	}
		
	/* Free any existing group info */

	SAFE_FREE(ent->sam_entries);
	ent->num_sam_entries = 0;
	ent->got_sam_entries = True;

	/* Enumerate domain groups */

	num_entries = 0;

	if (!(domain = find_domain_from_name(ent->domain_name))) {
		DEBUG(3, ("no such domain %s in get_sam_group_entries\n", ent->domain_name));
		goto done;
	}

	/* always get the domain global groups */

	status = domain->methods->enum_dom_groups(domain, mem_ctx, &num_entries, &sam_grp_entries);
	
	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(3, ("get_sam_group_entries: could not enumerate domain groups! Error: %s\n", nt_errstr(status)));
		result = False;
		goto done;
	}

	/* Copy entries into return buffer */

	if (num_entries) {
		if ( !(name_list = SMB_MALLOC_ARRAY(struct acct_info, num_entries)) ) {
			DEBUG(0,("get_sam_group_entries: Failed to malloc memory for %d domain groups!\n", 
				num_entries));
			result = False;
			goto done;
		}
		memcpy( name_list, sam_grp_entries, num_entries * sizeof(struct acct_info) );
	}
	
	ent->num_sam_entries = num_entries;
	
	/* get the domain local groups if we are a member of a native win2k domain
	   and are not using LDAP to get the groups */
	   
	if ( ( lp_security() != SEC_ADS && domain->native_mode 
		&& domain->primary) || domain->internal )
	{
		DEBUG(4,("get_sam_group_entries: %s domain; enumerating local groups as well\n", 
			domain->native_mode ? "Native Mode 2k":"BUILTIN or local"));
		
		status = domain->methods->enum_local_groups(domain, mem_ctx, &num_entries, &sam_grp_entries);
		
		if ( !NT_STATUS_IS_OK(status) ) { 
			DEBUG(3,("get_sam_group_entries: Failed to enumerate domain local groups!\n"));
			num_entries = 0;
		}
		else
			DEBUG(4,("get_sam_group_entries: Returned %d local groups\n", num_entries));
		
		/* Copy entries into return buffer */

		if ( num_entries ) {
			if ( !(name_list = SMB_REALLOC_ARRAY( name_list, struct acct_info, ent->num_sam_entries+num_entries)) )
			{
				DEBUG(0,("get_sam_group_entries: Failed to realloc more memory for %d local groups!\n", 
					num_entries));
				result = False;
				goto done;
			}
			
			memcpy( &name_list[ent->num_sam_entries], sam_grp_entries, 
				num_entries * sizeof(struct acct_info) );
		}
	
		ent->num_sam_entries += num_entries;
	}
	
		
	/* Fill in remaining fields */

	ent->sam_entries = name_list;
	ent->sam_entry_index = 0;

	result = (ent->num_sam_entries > 0);

 done:
	talloc_destroy(mem_ctx);

	return result;
}

/* Fetch next group entry from ntdom database */

#define MAX_GETGRENT_GROUPS 500

void winbindd_getgrent(struct winbindd_cli_state *state)
{
	struct getent_state *ent;
	struct winbindd_gr *group_list = NULL;
	int num_groups, group_list_ndx = 0, i, gr_mem_list_len = 0;
	char *gr_mem_list = NULL;

	DEBUG(3, ("[%5lu]: getgrent\n", (unsigned long)state->pid));

	/* Check user has enabled this */

	if (!lp_winbind_enum_groups()) {
		request_error(state);
		return;
	}

	num_groups = MIN(MAX_GETGRENT_GROUPS, state->request.data.num_entries);

	if ((state->response.extra_data.data = SMB_MALLOC_ARRAY(struct winbindd_gr, num_groups)) == NULL) {
		request_error(state);
		return;
	}

	memset(state->response.extra_data.data, '\0',
		num_groups * sizeof(struct winbindd_gr) );

	state->response.data.num_entries = 0;

	group_list = (struct winbindd_gr *)state->response.extra_data.data;

	if (!state->getgrent_initialized)
		winbindd_setgrent_internal(state);

	if (!(ent = state->getgrent_state)) {
		request_error(state);
		return;
	}

	/* Start sending back groups */

	for (i = 0; i < num_groups; i++) {
		struct acct_info *name_list = NULL;
		fstring domain_group_name;
		uint32 result;
		gid_t group_gid;
		size_t gr_mem_len;
		char *gr_mem;
		DOM_SID group_sid;
		struct winbindd_domain *domain;
				
		/* Do we need to fetch another chunk of groups? */

	tryagain:

		DEBUG(10, ("entry_index = %d, num_entries = %d\n",
			   ent->sam_entry_index, ent->num_sam_entries));

		if (ent->num_sam_entries == ent->sam_entry_index) {

			while(ent && !get_sam_group_entries(ent)) {
				struct getent_state *next_ent;

				DEBUG(10, ("freeing state info for domain %s\n", ent->domain_name)); 

				/* Free state information for this domain */

				SAFE_FREE(ent->sam_entries);

				next_ent = ent->next;
				DLIST_REMOVE(state->getgrent_state, ent);
				
				SAFE_FREE(ent);
				ent = next_ent;
			}

			/* No more domains */

			if (!ent) 
                                break;
		}
		
		name_list = ent->sam_entries;
		
		if (!(domain = 
		      find_domain_from_name(ent->domain_name))) {
			DEBUG(3, ("No such domain %s in winbindd_getgrent\n", ent->domain_name));
			result = False;
			goto done;
		}

		/* Lookup group info */
		
		sid_copy(&group_sid, &domain->sid);
		sid_append_rid(&group_sid, name_list[ent->sam_entry_index].rid);

		if (!NT_STATUS_IS_OK(idmap_sid_to_gid(&group_sid,
                                                    &group_gid, 0))) {
			union unid_t id;
			enum SID_NAME_USE type;

			DEBUG(10, ("SID %s not in idmap\n",
				   sid_string_static(&group_sid)));

			if (!pdb_sid_to_id(&group_sid, &id, &type)) {
				DEBUG(1, ("could not look up gid for group "
					  "%s\n", 
					  name_list[ent->sam_entry_index].acct_name));
				ent->sam_entry_index++;
				goto tryagain;
			}

			if ((type != SID_NAME_DOM_GRP) &&
			    (type != SID_NAME_ALIAS) &&
			    (type != SID_NAME_WKN_GRP)) {
				DEBUG(1, ("Group %s is a %s, not a group\n",
					  sid_type_lookup(type),
					  name_list[ent->sam_entry_index].acct_name));
				ent->sam_entry_index++;
				goto tryagain;
			}
			group_gid = id.gid;
		}

		DEBUG(10, ("got gid %lu for group %lu\n", (unsigned long)group_gid,
			   (unsigned long)name_list[ent->sam_entry_index].rid));
		
		/* Fill in group entry */

		fill_domain_username(domain_group_name, ent->domain_name, 
			 name_list[ent->sam_entry_index].acct_name, True);

		result = fill_grent(&group_list[group_list_ndx], 
				    ent->domain_name,
				    name_list[ent->sam_entry_index].acct_name,
				    group_gid);

		/* Fill in group membership entry */

		if (result) {
			size_t num_gr_mem = 0;
			DOM_SID member_sid;
			group_list[group_list_ndx].num_gr_mem = 0;
			gr_mem = NULL;
			gr_mem_len = 0;
			
			/* Get group membership */			
			if (state->request.cmd == WINBINDD_GETGRLST) {
				result = True;
			} else {
				sid_copy(&member_sid, &domain->sid);
				sid_append_rid(&member_sid, name_list[ent->sam_entry_index].rid);
				result = fill_grent_mem(
					domain,
					&member_sid,
					SID_NAME_DOM_GRP,
					&num_gr_mem,
					&gr_mem, &gr_mem_len);

				group_list[group_list_ndx].num_gr_mem = (uint32)num_gr_mem;
			}
		}

		if (result) {
			/* Append to group membership list */
			gr_mem_list = SMB_REALLOC( gr_mem_list, gr_mem_list_len + gr_mem_len);

			if (!gr_mem_list && (group_list[group_list_ndx].num_gr_mem != 0)) {
				DEBUG(0, ("out of memory\n"));
				gr_mem_list_len = 0;
				break;
			}

			DEBUG(10, ("list_len = %d, mem_len = %u\n",
				   gr_mem_list_len, (unsigned int)gr_mem_len));

			memcpy(&gr_mem_list[gr_mem_list_len], gr_mem,
			       gr_mem_len);

			SAFE_FREE(gr_mem);

			group_list[group_list_ndx].gr_mem_ofs = 
				gr_mem_list_len;

			gr_mem_list_len += gr_mem_len;
		}

		ent->sam_entry_index++;
		
		/* Add group to return list */
		
		if (result) {

			DEBUG(10, ("adding group num_entries = %d\n",
				   state->response.data.num_entries));

			group_list_ndx++;
			state->response.data.num_entries++;
			
			state->response.length +=
				sizeof(struct winbindd_gr);
			
		} else {
			DEBUG(0, ("could not lookup domain group %s\n", 
				  domain_group_name));
		}
	}

	/* Copy the list of group memberships to the end of the extra data */

	if (group_list_ndx == 0)
		goto done;

	state->response.extra_data.data = SMB_REALLOC(
		state->response.extra_data.data,
		group_list_ndx * sizeof(struct winbindd_gr) + gr_mem_list_len);

	if (!state->response.extra_data.data) {
		DEBUG(0, ("out of memory\n"));
		group_list_ndx = 0;
		SAFE_FREE(gr_mem_list);
		request_error(state);
		return;
	}

	memcpy(&((char *)state->response.extra_data.data)
	       [group_list_ndx * sizeof(struct winbindd_gr)], 
	       gr_mem_list, gr_mem_list_len);

	state->response.length += gr_mem_list_len;

	DEBUG(10, ("returning %d groups, length = %d\n",
		   group_list_ndx, gr_mem_list_len));

	/* Out of domains */

 done:

       	SAFE_FREE(gr_mem_list);

	if (group_list_ndx > 0)
		request_ok(state);
	else
		request_error(state);
}

/* List domain groups without mapping to unix ids */

void winbindd_list_groups(struct winbindd_cli_state *state)
{
	uint32 total_entries = 0;
	struct winbindd_domain *domain;
	const char *which_domain;
	char *extra_data = NULL;
	unsigned int extra_data_len = 0, i;

	DEBUG(3, ("[%5lu]: list groups\n", (unsigned long)state->pid));

	/* Ensure null termination */
	state->request.domain_name[sizeof(state->request.domain_name)-1]='\0';	
	which_domain = state->request.domain_name;
	
	/* Enumerate over trusted domains */

	for (domain = domain_list(); domain; domain = domain->next) {
		struct getent_state groups;

		/* if we have a domain name restricting the request and this
		   one in the list doesn't match, then just bypass the remainder
		   of the loop */
		   
		if ( *which_domain && !strequal(which_domain, domain->name) )
			continue;
			
		ZERO_STRUCT(groups);

		/* Get list of sam groups */
		
		fstrcpy(groups.domain_name, domain->name);

		get_sam_group_entries(&groups);
			
		if (groups.num_sam_entries == 0) {
			/* this domain is empty or in an error state */
			continue;
		}

		/* keep track the of the total number of groups seen so 
		   far over all domains */
		total_entries += groups.num_sam_entries;
		
		/* Allocate some memory for extra data.  Note that we limit
		   account names to sizeof(fstring) = 128 characters.  */		
		extra_data = SMB_REALLOC(extra_data, sizeof(fstring) * total_entries);
 
		if (!extra_data) {
			DEBUG(0,("failed to enlarge buffer!\n"));
			request_error(state);
			return;
		}

		/* Pack group list into extra data fields */
		for (i = 0; i < groups.num_sam_entries; i++) {
			char *group_name = ((struct acct_info *)
					    groups.sam_entries)[i].acct_name; 
			fstring name;

			fill_domain_username(name, domain->name, group_name, True);
			/* Append to extra data */			
			memcpy(&extra_data[extra_data_len], name, 
                               strlen(name));
			extra_data_len += strlen(name);
			extra_data[extra_data_len++] = ',';
		}

		SAFE_FREE(groups.sam_entries);
	}

	/* Assign extra_data fields in response structure */
	if (extra_data) {
		extra_data[extra_data_len - 1] = '\0';
		state->response.extra_data.data = extra_data;
		state->response.length += extra_data_len;
	}

	/* No domains may have responded but that's still OK so don't
	   return an error. */

	request_ok(state);
}

/* Get user supplementary groups.  This is much quicker than trying to
   invert the groups database.  We merge the groups from the gids and
   other_sids info3 fields as trusted domain, universal group
   memberships, and nested groups (win2k native mode only) are not
   returned by the getgroups RPC call but are present in the info3. */

struct getgroups_state {
	struct winbindd_cli_state *state;
	struct winbindd_domain *domain;
	char *domname;
	char *username;
	DOM_SID user_sid;

	const DOM_SID *token_sids;
	size_t i, num_token_sids;

	gid_t *token_gids;
	size_t num_token_gids;
};

static void getgroups_usersid_recv(void *private_data, BOOL success,
				   const DOM_SID *sid, enum SID_NAME_USE type);
static void getgroups_tokensids_recv(void *private_data, BOOL success,
				     DOM_SID *token_sids, size_t num_token_sids);
static void getgroups_sid2gid_recv(void *private_data, BOOL success, gid_t gid);

void winbindd_getgroups(struct winbindd_cli_state *state)
{
	struct getgroups_state *s;

	/* Ensure null termination */
	state->request.data.username
		[sizeof(state->request.data.username)-1]='\0';

	DEBUG(3, ("[%5lu]: getgroups %s\n", (unsigned long)state->pid,
		  state->request.data.username));

	/* Parse domain and username */

	s = TALLOC_P(state->mem_ctx, struct getgroups_state);
	if (s == NULL) {
		DEBUG(0, ("talloc failed\n"));
		request_error(state);
		return;
	}

	s->state = state;

	if (!parse_domain_user_talloc(state->mem_ctx,
				      state->request.data.username,
				      &s->domname, &s->username)) {
		DEBUG(5, ("Could not parse domain user: %s\n",
			  state->request.data.username));

		/* error out if we do not have nested group support */

		if ( !lp_winbind_nested_groups() ) {
			request_error(state);
			return;
		}

		s->domname = talloc_strdup( state->mem_ctx, get_global_sam_name() );
		s->username = talloc_strdup( state->mem_ctx, state->request.data.username );
	}
	
	/* Get info for the domain */

	s->domain = find_domain_from_name_noinit(s->domname);

	if (s->domain == NULL) {
		DEBUG(7, ("could not find domain entry for domain %s\n", 
			  s->domname));
		request_error(state);
		return;
	}

	if ( s->domain->primary && lp_winbind_trusted_domains_only()) {
		DEBUG(7,("winbindd_getpwnam: My domain -- rejecting "
			 "getgroups() for %s\\%s.\n", s->domname,
			 s->username));
		request_error(state);
		return;
	}	

	/* Get rid and name type from name.  The following costs 1 packet */

	winbindd_lookupname_async(state->mem_ctx, s->domname, s->username,
				  getgroups_usersid_recv, s);
}

static void getgroups_usersid_recv(void *private_data, BOOL success,
				   const DOM_SID *sid, enum SID_NAME_USE type)
{
	struct getgroups_state *s = private_data;

	if ((!success) ||
	    ((type != SID_NAME_USER) && (type != SID_NAME_COMPUTER))) {
		request_error(s->state);
		return;
	}

	sid_copy(&s->user_sid, sid);

	winbindd_gettoken_async(s->state->mem_ctx, &s->user_sid,
				getgroups_tokensids_recv, s);
}

static void getgroups_tokensids_recv(void *private_data, BOOL success,
				     DOM_SID *token_sids, size_t num_token_sids)
{
	struct getgroups_state *s = private_data;

	/* We need at least the user sid and the primary group in the token,
	 * otherwise it's an error */

	if ((!success) || (num_token_sids < 2)) {
		request_error(s->state);
		return;
	}

	s->token_sids = token_sids;
	s->num_token_sids = num_token_sids;
	s->i = 0;

	s->token_gids = NULL;
	s->num_token_gids = 0;

	getgroups_sid2gid_recv(s, False, 0);
}

static void getgroups_sid2gid_recv(void *private_data, BOOL success, gid_t gid)
{
	struct getgroups_state *s = private_data;

	if (success)
		add_gid_to_array_unique(NULL, gid,
					&s->token_gids,
					&s->num_token_gids);

	if (s->i < s->num_token_sids) {
		const DOM_SID *sid = &s->token_sids[s->i];
		s->i += 1;

		if (sid_equal(sid, &s->user_sid)) {
			getgroups_sid2gid_recv(s, False, 0);
			return;
		}

		winbindd_sid2gid_async(s->state->mem_ctx, sid,
				       getgroups_sid2gid_recv, s);
		return;
	}

	s->state->response.data.num_entries = s->num_token_gids;
	s->state->response.extra_data.data = s->token_gids;
	s->state->response.length += s->num_token_gids * sizeof(gid_t);
	request_ok(s->state);
}

/* Get user supplementary sids. This is equivalent to the
   winbindd_getgroups() function but it involves a SID->SIDs mapping
   rather than a NAME->SID->SIDS->GIDS mapping, which means we avoid
   idmap. This call is designed to be used with applications that need
   to do ACL evaluation themselves. Note that the cached info3 data is
   not used 

   this function assumes that the SID that comes in is a user SID. If
   you pass in another type of SID then you may get unpredictable
   results.
*/

static void getusersids_recv(void *private_data, BOOL success, DOM_SID *sids,
			     size_t num_sids);

void winbindd_getusersids(struct winbindd_cli_state *state)
{
	DOM_SID *user_sid;

	/* Ensure null termination */
	state->request.data.sid[sizeof(state->request.data.sid)-1]='\0';

	user_sid = TALLOC_P(state->mem_ctx, DOM_SID);
	if (user_sid == NULL) {
		DEBUG(1, ("talloc failed\n"));
		request_error(state);
		return;
	}

	if (!string_to_sid(user_sid, state->request.data.sid)) {
		DEBUG(1, ("Could not get convert sid %s from string\n",
			  state->request.data.sid));
		request_error(state);
		return;
	}

	winbindd_gettoken_async(state->mem_ctx, user_sid, getusersids_recv,
				state);
}

static void getusersids_recv(void *private_data, BOOL success, DOM_SID *sids,
			     size_t num_sids)
{
	struct winbindd_cli_state *state = private_data;
	char *ret = NULL;
	unsigned ofs, ret_size = 0;
	size_t i;

	if (!success) {
		request_error(state);
		return;
	}

	/* work out the response size */
	for (i = 0; i < num_sids; i++) {
		const char *s = sid_string_static(&sids[i]);
		ret_size += strlen(s) + 1;
	}

	/* build the reply */
	ret = SMB_MALLOC(ret_size);
	if (!ret) {
		DEBUG(0, ("malloc failed\n"));
		request_error(state);
		return;
	}
	ofs = 0;
	for (i = 0; i < num_sids; i++) {
		const char *s = sid_string_static(&sids[i]);
		safe_strcpy(ret + ofs, s, ret_size - ofs - 1);
		ofs += strlen(ret+ofs) + 1;
	}

	/* Send data back to client */
	state->response.data.num_entries = num_sids;
	state->response.extra_data.data = ret;
	state->response.length += ret_size;
	request_ok(state);
}

void winbindd_getuserdomgroups(struct winbindd_cli_state *state)
{
	DOM_SID user_sid;
	struct winbindd_domain *domain;

	/* Ensure null termination */
	state->request.data.sid[sizeof(state->request.data.sid)-1]='\0';

	if (!string_to_sid(&user_sid, state->request.data.sid)) {
		DEBUG(1, ("Could not get convert sid %s from string\n",
			  state->request.data.sid));
		request_error(state);
		return;
	}

	/* Get info for the domain */	
	if ((domain = find_domain_from_sid_noinit(&user_sid)) == NULL) {
		DEBUG(0,("could not find domain entry for sid %s\n", 
			 sid_string_static(&user_sid)));
		request_error(state);
		return;
	}

	sendto_domain(state, domain);
}

enum winbindd_result winbindd_dual_getuserdomgroups(struct winbindd_domain *domain,
						    struct winbindd_cli_state *state)
{
	DOM_SID user_sid;
	NTSTATUS status;

	char *sidstring;
	ssize_t len;
	DOM_SID *groups;
	uint32 num_groups;

	/* Ensure null termination */
	state->request.data.sid[sizeof(state->request.data.sid)-1]='\0';

	if (!string_to_sid(&user_sid, state->request.data.sid)) {
		DEBUG(1, ("Could not get convert sid %s from string\n",
			  state->request.data.sid));
		return WINBINDD_ERROR;
	}

	status = domain->methods->lookup_usergroups(domain, state->mem_ctx,
						    &user_sid, &num_groups,
						    &groups);
	if (!NT_STATUS_IS_OK(status))
		return WINBINDD_ERROR;

	if (num_groups == 0) {
		state->response.data.num_entries = 0;
		state->response.extra_data.data = NULL;
		return WINBINDD_OK;
	}

	if (!print_sidlist(NULL, groups, num_groups, &sidstring, &len)) {
		DEBUG(0, ("malloc failed\n"));
		return WINBINDD_ERROR;
	}

	state->response.extra_data.data = sidstring;
	state->response.length += len+1;
	state->response.data.num_entries = num_groups;

	return WINBINDD_OK;
}
