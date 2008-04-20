/* 
   Unix SMB/CIFS implementation.

   Wrapper around winbindd_rpc.c to centralize retry logic.

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

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_WINBIND

extern struct winbindd_methods msrpc_methods;

/* List all users */
static NTSTATUS query_user_list(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				uint32 *num_entries, 
				WINBIND_USERINFO **info)
{
	NTSTATUS result;

	result = msrpc_methods.query_user_list(domain, mem_ctx,
					       num_entries, info);

	if (NT_STATUS_EQUAL(result, NT_STATUS_UNSUCCESSFUL))
		result = msrpc_methods.query_user_list(domain, mem_ctx,
						       num_entries, info);
	return result;
}

/* list all domain groups */
static NTSTATUS enum_dom_groups(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				uint32 *num_entries, 
				struct acct_info **info)
{
	NTSTATUS result;

	result = msrpc_methods.enum_dom_groups(domain, mem_ctx,
					       num_entries, info);

	if (NT_STATUS_EQUAL(result, NT_STATUS_UNSUCCESSFUL))
		result = msrpc_methods.enum_dom_groups(domain, mem_ctx,
						       num_entries, info);
	return result;
}

/* List all domain groups */

static NTSTATUS enum_local_groups(struct winbindd_domain *domain,
				  TALLOC_CTX *mem_ctx,
				  uint32 *num_entries, 
				  struct acct_info **info)
{
	NTSTATUS result;

	result = msrpc_methods.enum_local_groups(domain, mem_ctx,
						 num_entries, info);

	if (NT_STATUS_EQUAL(result, NT_STATUS_UNSUCCESSFUL))
		result = msrpc_methods.enum_local_groups(domain, mem_ctx,
							 num_entries, info);

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
	NTSTATUS result;

	result = msrpc_methods.name_to_sid(domain, mem_ctx,
					   domain_name, name,
					   sid, type);

	if (NT_STATUS_EQUAL(result, NT_STATUS_UNSUCCESSFUL))
		result = msrpc_methods.name_to_sid(domain, mem_ctx,
						   domain_name, name,
						   sid, type);

	return result;
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
	NTSTATUS result;

	result = msrpc_methods.sid_to_name(domain, mem_ctx, sid,
					   domain_name, name, type);

	if (NT_STATUS_EQUAL(result, NT_STATUS_UNSUCCESSFUL))
		result = msrpc_methods.sid_to_name(domain, mem_ctx, sid,
						   domain_name, name, type);

	return result;
}

/* Lookup user information from a rid or username. */
static NTSTATUS query_user(struct winbindd_domain *domain, 
			   TALLOC_CTX *mem_ctx, 
			   const DOM_SID *user_sid,
			   WINBIND_USERINFO *user_info)
{
	NTSTATUS result;

	result = msrpc_methods.query_user(domain, mem_ctx, user_sid,
					  user_info);

	if (NT_STATUS_EQUAL(result, NT_STATUS_UNSUCCESSFUL))
		result = msrpc_methods.query_user(domain, mem_ctx, user_sid,
						  user_info);

	return result;
}

/* Lookup groups a user is a member of.  I wish Unix had a call like this! */
static NTSTATUS lookup_usergroups(struct winbindd_domain *domain,
				  TALLOC_CTX *mem_ctx,
				  const DOM_SID *user_sid,
				  uint32 *num_groups, DOM_SID **user_gids)
{
	NTSTATUS result;

	result = msrpc_methods.lookup_usergroups(domain, mem_ctx,
						 user_sid, num_groups,
						 user_gids);

	if (NT_STATUS_EQUAL(result, NT_STATUS_UNSUCCESSFUL))
		result = msrpc_methods.lookup_usergroups(domain, mem_ctx,
							 user_sid, num_groups,
							 user_gids);

	return result;
}

static NTSTATUS lookup_useraliases(struct winbindd_domain *domain,
				   TALLOC_CTX *mem_ctx,
				   uint32 num_sids, const DOM_SID *sids,
				   uint32 *num_aliases, uint32 **alias_rids)
{
	NTSTATUS result;

	result = msrpc_methods.lookup_useraliases(domain, mem_ctx,
						  num_sids, sids,
						  num_aliases,
						  alias_rids);

	if (NT_STATUS_EQUAL(result, NT_STATUS_UNSUCCESSFUL))
		result = msrpc_methods.lookup_useraliases(domain, mem_ctx,
							  num_sids, sids,
							  num_aliases,
							  alias_rids);

	return result;
}

/* Lookup group membership given a rid.   */
static NTSTATUS lookup_groupmem(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				const DOM_SID *group_sid, uint32 *num_names, 
				DOM_SID **sid_mem, char ***names, 
				uint32 **name_types)
{
	NTSTATUS result;

	result = msrpc_methods.lookup_groupmem(domain, mem_ctx,
					       group_sid, num_names,
					       sid_mem, names,
					       name_types);

	if (NT_STATUS_EQUAL(result, NT_STATUS_UNSUCCESSFUL))
		result = msrpc_methods.lookup_groupmem(domain, mem_ctx,
						       group_sid, num_names,
						       sid_mem, names,
						       name_types);

	return result;
}

/* find the sequence number for a domain */
static NTSTATUS sequence_number(struct winbindd_domain *domain, uint32 *seq)
{
	NTSTATUS result;

	result = msrpc_methods.sequence_number(domain, seq);

	if (NT_STATUS_EQUAL(result, NT_STATUS_UNSUCCESSFUL))
		result = msrpc_methods.sequence_number(domain, seq);

	return result;
}

/* find the lockout policy of a domain */
static NTSTATUS lockout_policy(struct winbindd_domain *domain, 
			       TALLOC_CTX *mem_ctx,
			       SAM_UNK_INFO_12 *policy)
{
	NTSTATUS result;

	result = msrpc_methods.lockout_policy(domain, mem_ctx, policy);

	if (NT_STATUS_EQUAL(result, NT_STATUS_UNSUCCESSFUL))
		result = msrpc_methods.lockout_policy(domain, mem_ctx, policy);

	return result;
}

/* find the password policy of a domain */
static NTSTATUS password_policy(struct winbindd_domain *domain, 
				TALLOC_CTX *mem_ctx,
				SAM_UNK_INFO_1 *policy)
{
 	NTSTATUS result;
 
	result = msrpc_methods.password_policy(domain, mem_ctx, policy);

	if (NT_STATUS_EQUAL(result, NT_STATUS_UNSUCCESSFUL))
		result = msrpc_methods.password_policy(domain, mem_ctx, policy);
	
	return result;
}

/* get a list of trusted domains */
static NTSTATUS trusted_domains(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				uint32 *num_domains,
				char ***names,
				char ***alt_names,
				DOM_SID **dom_sids)
{
	NTSTATUS result;

	result = msrpc_methods.trusted_domains(domain, mem_ctx,
					       num_domains, names,
					       alt_names, dom_sids);

	if (NT_STATUS_EQUAL(result, NT_STATUS_UNSUCCESSFUL))
		result = msrpc_methods.trusted_domains(domain, mem_ctx,
						       num_domains, names,
						       alt_names, dom_sids);

	return result;
}

/* the rpc backend methods are exposed via this structure */
struct winbindd_methods reconnect_methods = {
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
