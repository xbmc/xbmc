/* 
   Unix SMB/CIFS implementation.

   Winbind rpc backend functions

   Copyright (C) Tim Potter 2000-2001,2003
   Copyright (C) Simo Sorce 2003
   Copyright (C) Volker Lendecke 2004
   
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

static void add_member(const char *domain, const char *user,
	   char **pp_members, size_t *p_num_members)
{
	fstring name;

	fill_domain_username(name, domain, user, True);
	safe_strcat(name, ",", sizeof(name)-1);
	string_append(pp_members, name);
	*p_num_members += 1;
}

/**********************************************************************
 Add member users resulting from sid. Expand if it is a domain group.
**********************************************************************/

static void add_expanded_sid(const DOM_SID *sid, char **pp_members, size_t *p_num_members)
{
	DOM_SID dom_sid;
	uint32 rid;
	struct winbindd_domain *domain;
	size_t i;

	char *domain_name = NULL;
	char *name = NULL;
	enum SID_NAME_USE type;

	uint32 num_names;
	DOM_SID *sid_mem;
	char **names;
	uint32 *types;

	NTSTATUS result;

	TALLOC_CTX *mem_ctx = talloc_init("add_expanded_sid");

	if (mem_ctx == NULL) {
		DEBUG(1, ("talloc_init failed\n"));
		return;
	}

	sid_copy(&dom_sid, sid);
	sid_split_rid(&dom_sid, &rid);

	domain = find_lookup_domain_from_sid(sid);

	if (domain == NULL) {
		DEBUG(3, ("Could not find domain for sid %s\n",
			  sid_string_static(sid)));
		goto done;
	}

	result = domain->methods->sid_to_name(domain, mem_ctx, sid,
					      &domain_name, &name, &type);

	if (!NT_STATUS_IS_OK(result)) {
		DEBUG(3, ("sid_to_name failed for sid %s\n",
			  sid_string_static(sid)));
		goto done;
	}

	DEBUG(10, ("Found name %s, type %d\n", name, type));

	if (type == SID_NAME_USER) {
		add_member(domain_name, name, pp_members, p_num_members);
		goto done;
	}

	if (type != SID_NAME_DOM_GRP) {
		DEBUG(10, ("Alias member %s neither user nor group, ignore\n",
			   name));
		goto done;
	}

	/* Expand the domain group, this must be done via the target domain */

	domain = find_domain_from_sid(sid);

	if (domain == NULL) {
		DEBUG(3, ("Could not find domain from SID %s\n",
			  sid_string_static(sid)));
		goto done;
	}

	result = domain->methods->lookup_groupmem(domain, mem_ctx,
						  sid, &num_names,
						  &sid_mem, &names,
						  &types);

	if (!NT_STATUS_IS_OK(result)) {
		DEBUG(10, ("Could not lookup group members for %s: %s\n",
			   name, nt_errstr(result)));
		goto done;
	}

	for (i=0; i<num_names; i++) {
		DEBUG(10, ("Adding group member SID %s\n",
			   sid_string_static(&sid_mem[i])));

		if (types[i] != SID_NAME_USER) {
			DEBUG(1, ("Hmmm. Member %s of group %s is no user. "
				  "Ignoring.\n", names[i], name));
			continue;
		}

		add_member(domain->name, names[i], pp_members, p_num_members);
	}

 done:
	talloc_destroy(mem_ctx);
	return;
}

BOOL fill_passdb_alias_grmem(struct winbindd_domain *domain,
			     DOM_SID *group_sid, 
			     size_t *num_gr_mem, char **gr_mem, size_t *gr_mem_len)
{
	DOM_SID *members;
	size_t i, num_members;

	*num_gr_mem = 0;
	*gr_mem = NULL;
	*gr_mem_len = 0;

	if (!NT_STATUS_IS_OK(pdb_enum_aliasmem(group_sid, &members,
					       &num_members)))
		return True;

	for (i=0; i<num_members; i++) {
		add_expanded_sid(&members[i], gr_mem, num_gr_mem);
	}

	SAFE_FREE(members);

	if (*gr_mem != NULL) {
		size_t len;

		/* We have at least one member, strip off the last "," */
		len = strlen(*gr_mem);
		(*gr_mem)[len-1] = '\0';
		*gr_mem_len = len;
	}

	return True;
}

/* Query display info for a domain.  This returns enough information plus a
   bit extra to give an overview of domain users for the User Manager
   application. */
static NTSTATUS query_user_list(struct winbindd_domain *domain,
			       TALLOC_CTX *mem_ctx,
			       uint32 *num_entries, 
			       WINBIND_USERINFO **info)
{
	/* We don't have users */
	*num_entries = 0;
	*info = NULL;
	return NT_STATUS_OK;
}

/* list all domain groups */
static NTSTATUS enum_dom_groups(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				uint32 *num_entries, 
				struct acct_info **info)
{
	/* We don't have domain groups */
	*num_entries = 0;
	*info = NULL;
	return NT_STATUS_OK;
}

/* List all domain groups */

static NTSTATUS enum_local_groups(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				uint32 *num_entries, 
				struct acct_info **info)
{
	struct pdb_search *search;
	struct samr_displayentry *aliases;
	int i;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	search = pdb_search_aliases(&domain->sid);
	if (search == NULL) goto done;

	*num_entries = pdb_search_entries(search, 0, 0xffffffff, &aliases);
	if (*num_entries == 0) goto done;

	*info = TALLOC_ARRAY(mem_ctx, struct acct_info, *num_entries);
	if (*info == NULL) {
		result = NT_STATUS_NO_MEMORY;
		goto done;
	}

	for (i=0; i<*num_entries; i++) {
		fstrcpy((*info)[i].acct_name, aliases[i].account_name);
		fstrcpy((*info)[i].acct_desc, aliases[i].description);
		(*info)[i].rid = aliases[i].rid;
	}

	result = NT_STATUS_OK;
 done:
	pdb_search_destroy(search);
	return result;
}

/* convert a single name to a sid in a domain */
static NTSTATUS name_to_sid(struct winbindd_domain *domain,
			    TALLOC_CTX *mem_ctx,
			    const char *domain_name,
			    const char *name,
			    DOM_SID *sid,
			    enum SID_NAME_USE *type)
{
	DEBUG(10, ("Finding name %s\n", name));

	if ( !lookup_name( mem_ctx, name, LOOKUP_NAME_ALL, 
		NULL, NULL, sid, type ) )
	{
		return NT_STATUS_NONE_MAPPED;
	}

	return NT_STATUS_OK;
}

/*
  convert a domain SID to a user or group name
*/
static NTSTATUS sid_to_name(struct winbindd_domain *domain,
			    TALLOC_CTX *mem_ctx,
			    const DOM_SID *sid,
			    char **domain_name,
			    char **name,
			    enum SID_NAME_USE *type)
{
	const char *dom, *nam;

	DEBUG(10, ("Converting SID %s\n", sid_string_static(sid)));

	/* Paranoia check */
	if (!sid_check_is_in_builtin(sid) &&
	    !sid_check_is_in_our_domain(sid)) {
		DEBUG(0, ("Possible deadlock: Trying to lookup SID %s with "
			  "passdb backend\n", sid_string_static(sid)));
		return NT_STATUS_NONE_MAPPED;
	}

	if (!lookup_sid(mem_ctx, sid, &dom, &nam, type)) {
		return NT_STATUS_NONE_MAPPED;
	}

	*domain_name = talloc_strdup(mem_ctx, dom);
	*name = talloc_strdup(mem_ctx, nam);

	return NT_STATUS_OK;
}

/* Lookup user information from a rid or username. */
static NTSTATUS query_user(struct winbindd_domain *domain, 
			   TALLOC_CTX *mem_ctx, 
			   const DOM_SID *user_sid,
			   WINBIND_USERINFO *user_info)
{
	return NT_STATUS_NO_SUCH_USER;
}

/* Lookup groups a user is a member of.  I wish Unix had a call like this! */
static NTSTATUS lookup_usergroups(struct winbindd_domain *domain,
				  TALLOC_CTX *mem_ctx,
				  const DOM_SID *user_sid,
				  uint32 *num_groups, DOM_SID **user_gids)
{
	NTSTATUS result;
	DOM_SID *groups = NULL;
	gid_t *gids = NULL;
	size_t ngroups = 0;
	struct samu *user;

	if ( (user = samu_new(mem_ctx)) == NULL ) {
		return NT_STATUS_NO_MEMORY;
	}

	if ( !pdb_getsampwsid( user, user_sid ) ) {
		return NT_STATUS_NO_SUCH_USER;
	}

	result = pdb_enum_group_memberships( mem_ctx, user, &groups, &gids, &ngroups );

	TALLOC_FREE( user );

	*num_groups = (uint32)ngroups;
	*user_gids = groups;

	return result;
}

static NTSTATUS lookup_useraliases(struct winbindd_domain *domain,
				   TALLOC_CTX *mem_ctx,
				   uint32 num_sids, const DOM_SID *sids,
				   uint32 *p_num_aliases, uint32 **rids)
{
	NTSTATUS result;
	size_t num_aliases = 0;

	result = pdb_enum_alias_memberships(mem_ctx, &domain->sid,
					    sids, num_sids, rids, &num_aliases);

	*p_num_aliases = num_aliases;
	return result;
}

/* Lookup group membership given a rid.   */
static NTSTATUS lookup_groupmem(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				const DOM_SID *group_sid, uint32 *num_names, 
				DOM_SID **sid_mem, char ***names, 
				uint32 **name_types)
{
	size_t i, num_members, num_mapped;
	uint32 *rids;
	NTSTATUS result;
	const DOM_SID **sids;
	struct lsa_dom_info *lsa_domains;
	struct lsa_name_info *lsa_names;

	if (!sid_check_is_in_our_domain(group_sid)) {
		/* There's no groups, only aliases in BUILTIN */
		return NT_STATUS_NO_SUCH_GROUP;
	}

	result = pdb_enum_group_members(mem_ctx, group_sid, &rids,
					&num_members);
	if (!NT_STATUS_IS_OK(result)) {
		return result;
	}

	if (num_members == 0) {
		*num_names = 0;
		*sid_mem = NULL;
		*names = NULL;
		*name_types = NULL;
		return NT_STATUS_OK;
	}

	*sid_mem = TALLOC_ARRAY(mem_ctx, DOM_SID, num_members);
	*names = TALLOC_ARRAY(mem_ctx, char *, num_members);
	*name_types = TALLOC_ARRAY(mem_ctx, uint32, num_members);
	sids = TALLOC_ARRAY(mem_ctx, const DOM_SID *, num_members);

	if (((*sid_mem) == NULL) || ((*names) == NULL) ||
	    ((*name_types) == NULL) || (sids == NULL)) {
		return NT_STATUS_NO_MEMORY;
	}

	for (i=0; i<num_members; i++) {
		DOM_SID *sid = &((*sid_mem)[i]);
		sid_copy(sid, &domain->sid);
		sid_append_rid(sid, rids[i]);
		sids[i] = sid;
	}

	result = lookup_sids(mem_ctx, num_members, sids, 1,
			     &lsa_domains, &lsa_names);
	if (!NT_STATUS_IS_OK(result)) {
		return result;
	}

	num_mapped = 0;
	for (i=0; i<num_members; i++) {
		if (lsa_names[i].type != SID_NAME_USER) {
			DEBUG(2, ("Got %s as group member -- ignoring\n",
				  sid_type_lookup(lsa_names[i].type)));
			continue;
		}
		(*names)[i] = talloc_steal((*names),
					   lsa_names[i].name);
		(*name_types)[i] = lsa_names[i].type;

		num_mapped += 1;
	}

	*num_names = num_mapped;

	return NT_STATUS_OK;
}

/* find the sequence number for a domain */
static NTSTATUS sequence_number(struct winbindd_domain *domain, uint32 *seq)
{
	BOOL result;
	time_t seq_num;

	result = pdb_get_seq_num(&seq_num);
	if (!result) {
		*seq = 1;
	}

	*seq = (int) seq_num;
	/* *seq = 1; */
	return NT_STATUS_OK;
}

static NTSTATUS lockout_policy(struct winbindd_domain *domain,
			       TALLOC_CTX *mem_ctx,
			       SAM_UNK_INFO_12 *policy)
{
	/* actually we have that */
	return NT_STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS password_policy(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				SAM_UNK_INFO_1 *policy)
{
	uint32 min_pass_len,pass_hist,password_properties;
	time_t u_expire, u_min_age;
	NTTIME nt_expire, nt_min_age;
	uint32 account_policy_temp;

	if ((policy = TALLOC_ZERO_P(mem_ctx, SAM_UNK_INFO_1)) == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	if (!pdb_get_account_policy(AP_MIN_PASSWORD_LEN, &account_policy_temp)) {
		return NT_STATUS_ACCESS_DENIED;
	}
	min_pass_len = account_policy_temp;

	if (!pdb_get_account_policy(AP_PASSWORD_HISTORY, &account_policy_temp)) {
		return NT_STATUS_ACCESS_DENIED;
	}
	pass_hist = account_policy_temp;

	if (!pdb_get_account_policy(AP_USER_MUST_LOGON_TO_CHG_PASS, &account_policy_temp)) {
		return NT_STATUS_ACCESS_DENIED;
	}
	password_properties = account_policy_temp;
	
	if (!pdb_get_account_policy(AP_MAX_PASSWORD_AGE, &account_policy_temp)) {
		return NT_STATUS_ACCESS_DENIED;
	}
	u_expire = account_policy_temp;

	if (!pdb_get_account_policy(AP_MIN_PASSWORD_AGE, &account_policy_temp)) {
		return NT_STATUS_ACCESS_DENIED;
	}
	u_min_age = account_policy_temp;

	unix_to_nt_time_abs(&nt_expire, u_expire);
	unix_to_nt_time_abs(&nt_min_age, u_min_age);

	init_unk_info1(policy, (uint16)min_pass_len, (uint16)pass_hist, 
	               password_properties, nt_expire, nt_min_age);

	return NT_STATUS_OK;
}

/* get a list of trusted domains */
static NTSTATUS trusted_domains(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				uint32 *num_domains,
				char ***names,
				char ***alt_names,
				DOM_SID **dom_sids)
{
	NTSTATUS nt_status;
	struct trustdom_info **domains;
	int i;

	*num_domains = 0;
	*names = NULL;
	*alt_names = NULL;
	*dom_sids = NULL;

	nt_status = secrets_trusted_domains(mem_ctx, num_domains,
					    &domains);
	if (!NT_STATUS_IS_OK(nt_status)) {
		return nt_status;
	}

	*names = TALLOC_ARRAY(mem_ctx, char *, *num_domains);
	*alt_names = TALLOC_ARRAY(mem_ctx, char *, *num_domains);
	*dom_sids = TALLOC_ARRAY(mem_ctx, DOM_SID, *num_domains);

	if ((*alt_names == NULL) || (*names == NULL) || (*dom_sids == NULL)) {
		return NT_STATUS_NO_MEMORY;
	}

	for (i=0; i<*num_domains; i++) {
		(*alt_names)[i] = NULL;
		(*names)[i] = talloc_steal((*names), domains[i]->name);
		sid_copy(&(*dom_sids)[i], &domains[i]->sid);
	}

	return NT_STATUS_OK;
}

/* the rpc backend methods are exposed via this structure */
struct winbindd_methods passdb_methods = {
	False,
	query_user_list,
	enum_dom_groups,
	enum_local_groups,
	name_to_sid,
	sid_to_name,
	query_user,
	lookup_usergroups,
	lookup_useraliases,
	lookup_groupmem,
	sequence_number,
	lockout_policy,
	password_policy,
	trusted_domains,
};
