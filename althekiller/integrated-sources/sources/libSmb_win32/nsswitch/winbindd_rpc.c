/* 
   Unix SMB/CIFS implementation.

   Winbind rpc backend functions

   Copyright (C) Tim Potter 2000-2001,2003
   Copyright (C) Andrew Tridgell 2001
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


/* Query display info for a domain.  This returns enough information plus a
   bit extra to give an overview of domain users for the User Manager
   application. */
static NTSTATUS query_user_list(struct winbindd_domain *domain,
			       TALLOC_CTX *mem_ctx,
			       uint32 *num_entries, 
			       WINBIND_USERINFO **info)
{
	NTSTATUS result;
	POLICY_HND dom_pol;
	unsigned int i, start_idx;
	uint32 loop_count;
	struct rpc_pipe_client *cli;

	DEBUG(3,("rpc: query_user_list\n"));

	*num_entries = 0;
	*info = NULL;

	result = cm_connect_sam(domain, mem_ctx, &cli, &dom_pol);
	if (!NT_STATUS_IS_OK(result))
		return result;

	i = start_idx = 0;
	loop_count = 0;

	do {
		TALLOC_CTX *ctx2;
		uint32 num_dom_users, j;
		uint32 max_entries, max_size;
		SAM_DISPINFO_CTR ctr;
		SAM_DISPINFO_1 info1;

		ZERO_STRUCT( ctr );
		ZERO_STRUCT( info1 );
		ctr.sam.info1 = &info1;
	
		if (!(ctx2 = talloc_init("winbindd enum_users")))
			return NT_STATUS_NO_MEMORY;

		/* this next bit is copied from net_user_list_internal() */

		get_query_dispinfo_params(loop_count, &max_entries,
					  &max_size);

		result = rpccli_samr_query_dispinfo(cli, mem_ctx, &dom_pol,
						    &start_idx, 1,
						    &num_dom_users,
						    max_entries, max_size,
						    &ctr);

		loop_count++;

		*num_entries += num_dom_users;

		*info = TALLOC_REALLOC_ARRAY(mem_ctx, *info, WINBIND_USERINFO,
					     *num_entries);

		if (!(*info)) {
			talloc_destroy(ctx2);
			return NT_STATUS_NO_MEMORY;
		}

		for (j = 0; j < num_dom_users; i++, j++) {
			fstring username, fullname;
			uint32 rid = ctr.sam.info1->sam[j].rid_user;
			
			unistr2_to_ascii( username, &(&ctr.sam.info1->str[j])->uni_acct_name, sizeof(username)-1);
			unistr2_to_ascii( fullname, &(&ctr.sam.info1->str[j])->uni_full_name, sizeof(fullname)-1);
			
			(*info)[i].acct_name = talloc_strdup(mem_ctx, username );
			(*info)[i].full_name = talloc_strdup(mem_ctx, fullname );
			(*info)[i].homedir = NULL;
			(*info)[i].shell = NULL;
			sid_compose(&(*info)[i].user_sid, &domain->sid, rid);
			
			/* For the moment we set the primary group for
			   every user to be the Domain Users group.
			   There are serious problems with determining
			   the actual primary group for large domains.
			   This should really be made into a 'winbind
			   force group' smb.conf parameter or
			   something like that. */
			   
			sid_compose(&(*info)[i].group_sid, &domain->sid, 
				    DOMAIN_GROUP_RID_USERS);
		}

		talloc_destroy(ctx2);

	} while (NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES));

	return result;
}

/* list all domain groups */
static NTSTATUS enum_dom_groups(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				uint32 *num_entries, 
				struct acct_info **info)
{
	POLICY_HND dom_pol;
	NTSTATUS status;
	uint32 start = 0;
	struct rpc_pipe_client *cli;

	*num_entries = 0;
	*info = NULL;

	DEBUG(3,("rpc: enum_dom_groups\n"));

	status = cm_connect_sam(domain, mem_ctx, &cli, &dom_pol);
	if (!NT_STATUS_IS_OK(status))
		return status;

	do {
		struct acct_info *info2 = NULL;
		uint32 count = 0;
		TALLOC_CTX *mem_ctx2;

		mem_ctx2 = talloc_init("enum_dom_groups[rpc]");

		/* start is updated by this call. */
		status = rpccli_samr_enum_dom_groups(cli, mem_ctx2, &dom_pol,
						     &start,
						     0xFFFF, /* buffer size? */
						     &info2, &count);

		if (!NT_STATUS_IS_OK(status) && 
		    !NT_STATUS_EQUAL(status, STATUS_MORE_ENTRIES)) {
			talloc_destroy(mem_ctx2);
			break;
		}

		(*info) = TALLOC_REALLOC_ARRAY(mem_ctx, *info,
					       struct acct_info,
					       (*num_entries) + count);
		if (! *info) {
			talloc_destroy(mem_ctx2);
			status = NT_STATUS_NO_MEMORY;
			break;
		}

		memcpy(&(*info)[*num_entries], info2, count*sizeof(*info2));
		(*num_entries) += count;
		talloc_destroy(mem_ctx2);
	} while (NT_STATUS_EQUAL(status, STATUS_MORE_ENTRIES));

	return NT_STATUS_OK;
}

/* List all domain groups */

static NTSTATUS enum_local_groups(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				uint32 *num_entries, 
				struct acct_info **info)
{
	POLICY_HND dom_pol;
	NTSTATUS result;
	struct rpc_pipe_client *cli;

	*num_entries = 0;
	*info = NULL;

	DEBUG(3,("rpc: enum_local_groups\n"));

	result = cm_connect_sam(domain, mem_ctx, &cli, &dom_pol);
	if (!NT_STATUS_IS_OK(result))
		return result;

	do {
		struct acct_info *info2 = NULL;
		uint32 count = 0, start = *num_entries;
		TALLOC_CTX *mem_ctx2;

		mem_ctx2 = talloc_init("enum_dom_local_groups[rpc]");

		result = rpccli_samr_enum_als_groups( cli, mem_ctx2, &dom_pol,
						      &start, 0xFFFF, &info2,
						      &count);
					  
		if (!NT_STATUS_IS_OK(result) &&
		    !NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES) ) 
		{
			talloc_destroy(mem_ctx2);
			return result;
		}

		(*info) = TALLOC_REALLOC_ARRAY(mem_ctx, *info,
					       struct acct_info,
					       (*num_entries) + count);
		if (! *info) {
			talloc_destroy(mem_ctx2);
			return NT_STATUS_NO_MEMORY;
		}

		memcpy(&(*info)[*num_entries], info2, count*sizeof(*info2));
		(*num_entries) += count;
		talloc_destroy(mem_ctx2);

	} while (NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES));

	return NT_STATUS_OK;
}

/* convert a single name to a sid in a domain */
NTSTATUS msrpc_name_to_sid(struct winbindd_domain *domain,
			    TALLOC_CTX *mem_ctx,
			    const char *domain_name,
			    const char *name,
			    DOM_SID *sid,
			    enum SID_NAME_USE *type)
{
	NTSTATUS result;
	DOM_SID *sids = NULL;
	uint32 *types = NULL;
	const char *full_name;
	struct rpc_pipe_client *cli;
	POLICY_HND lsa_policy;

        if(name == NULL || *name=='\0') {
                DEBUG(3,("rpc: name_to_sid name=%s\n", domain_name));
                full_name = talloc_asprintf(mem_ctx, "%s", domain_name);
        } else {
                DEBUG(3,("rpc: name_to_sid name=%s\\%s\n", domain_name, name));
                full_name = talloc_asprintf(mem_ctx, "%s\\%s", domain_name, name);
        }
	if (!full_name) {
		DEBUG(0, ("talloc_asprintf failed!\n"));
		return NT_STATUS_NO_MEMORY;
	}

	DEBUG(3,("name_to_sid [rpc] %s for domain %s\n", full_name?full_name:"", domain_name ));

	result = cm_connect_lsa(domain, mem_ctx, &cli, &lsa_policy);
	if (!NT_STATUS_IS_OK(result))
		return result;

	result = rpccli_lsa_lookup_names(cli, mem_ctx, &lsa_policy, 1, 
					 &full_name, NULL, &sids, &types);
        
	if (!NT_STATUS_IS_OK(result))
		return result;

	/* Return rid and type if lookup successful */

	sid_copy(sid, &sids[0]);
	*type = (enum SID_NAME_USE)types[0];

	return NT_STATUS_OK;
}

/*
  convert a domain SID to a user or group name
*/
NTSTATUS msrpc_sid_to_name(struct winbindd_domain *domain,
			    TALLOC_CTX *mem_ctx,
			    const DOM_SID *sid,
			    char **domain_name,
			    char **name,
			    enum SID_NAME_USE *type)
{
	char **domains;
	char **names;
	uint32 *types;
	NTSTATUS result;
	struct rpc_pipe_client *cli;
	POLICY_HND lsa_policy;

	DEBUG(3,("sid_to_name [rpc] %s for domain %s\n", sid_string_static(sid),
			domain->name ));

	result = cm_connect_lsa(domain, mem_ctx, &cli, &lsa_policy);
	if (!NT_STATUS_IS_OK(result))
		return result;

	result = rpccli_lsa_lookup_sids(cli, mem_ctx, &lsa_policy,
					1, sid, &domains, &names, &types);
	if (!NT_STATUS_IS_OK(result))
		return result;

	*type = (enum SID_NAME_USE)types[0];
	*domain_name = domains[0];
	*name = names[0];
	DEBUG(5,("Mapped sid to [%s]\\[%s]\n", domains[0], *name));
	return NT_STATUS_OK;
}

/* Lookup user information from a rid or username. */
static NTSTATUS query_user(struct winbindd_domain *domain, 
			   TALLOC_CTX *mem_ctx, 
			   const DOM_SID *user_sid, 
			   WINBIND_USERINFO *user_info)
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	POLICY_HND dom_pol, user_pol;
	SAM_USERINFO_CTR *ctr;
	fstring sid_string;
	uint32 user_rid;
	NET_USER_INFO_3 *user;
	struct rpc_pipe_client *cli;

	DEBUG(3,("rpc: query_user rid=%s\n",
		 sid_to_string(sid_string, user_sid)));

	if (!sid_peek_check_rid(&domain->sid, user_sid, &user_rid))
		return NT_STATUS_UNSUCCESSFUL;
	
	/* try netsamlogon cache first */
			
	if ( (user = netsamlogon_cache_get( mem_ctx, user_sid )) != NULL ) 
	{
				
		DEBUG(5,("query_user: Cache lookup succeeded for %s\n", 
			sid_string_static(user_sid)));

		sid_compose(&user_info->user_sid, &domain->sid, user_rid);
		sid_compose(&user_info->group_sid, &domain->sid,
			    user->group_rid);
				
		user_info->acct_name = unistr2_tdup(mem_ctx,
						    &user->uni_user_name);
		user_info->full_name = unistr2_tdup(mem_ctx,
						    &user->uni_full_name);
		
		user_info->homedir = NULL;
		user_info->shell = NULL;
						
		SAFE_FREE(user);
				
		return NT_STATUS_OK;
	}
	
	/* no cache; hit the wire */
		
	result = cm_connect_sam(domain, mem_ctx, &cli, &dom_pol);
	if (!NT_STATUS_IS_OK(result))
		return result;

	/* Get user handle */
	result = rpccli_samr_open_user(cli, mem_ctx, &dom_pol,
				       SEC_RIGHTS_MAXIMUM_ALLOWED, user_rid,
				       &user_pol);

	if (!NT_STATUS_IS_OK(result))
		return result;

	/* Get user info */
	result = rpccli_samr_query_userinfo(cli, mem_ctx, &user_pol,
					    0x15, &ctr);

	rpccli_samr_close(cli, mem_ctx, &user_pol);

	if (!NT_STATUS_IS_OK(result))
		return result;

	sid_compose(&user_info->user_sid, &domain->sid, user_rid);
	sid_compose(&user_info->group_sid, &domain->sid,
		    ctr->info.id21->group_rid);
	user_info->acct_name = unistr2_tdup(mem_ctx, 
					    &ctr->info.id21->uni_user_name);
	user_info->full_name = unistr2_tdup(mem_ctx, 
					    &ctr->info.id21->uni_full_name);
	user_info->homedir = NULL;
	user_info->shell = NULL;

	return NT_STATUS_OK;
}                                   

/* Lookup groups a user is a member of.  I wish Unix had a call like this! */
static NTSTATUS lookup_usergroups(struct winbindd_domain *domain,
				  TALLOC_CTX *mem_ctx,
				  const DOM_SID *user_sid,
				  uint32 *num_groups, DOM_SID **user_grpsids)
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	POLICY_HND dom_pol, user_pol;
	uint32 des_access = SEC_RIGHTS_MAXIMUM_ALLOWED;
	DOM_GID *user_groups;
	unsigned int i;
	fstring sid_string;
	uint32 user_rid;
	struct rpc_pipe_client *cli;

	DEBUG(3,("rpc: lookup_usergroups sid=%s\n",
		 sid_to_string(sid_string, user_sid)));

	if (!sid_peek_check_rid(&domain->sid, user_sid, &user_rid))
		return NT_STATUS_UNSUCCESSFUL;

	*num_groups = 0;
	*user_grpsids = NULL;

	/* so lets see if we have a cached user_info_3 */
	result = lookup_usergroups_cached(domain, mem_ctx, user_sid, 
					  num_groups, user_grpsids);

	if (NT_STATUS_IS_OK(result)) {
		return NT_STATUS_OK;
	}

	/* no cache; hit the wire */
	
	result = cm_connect_sam(domain, mem_ctx, &cli, &dom_pol);
	if (!NT_STATUS_IS_OK(result))
		return result;

	/* Get user handle */
	result = rpccli_samr_open_user(cli, mem_ctx, &dom_pol,
					des_access, user_rid, &user_pol);

	if (!NT_STATUS_IS_OK(result))
		return result;

	/* Query user rids */
	result = rpccli_samr_query_usergroups(cli, mem_ctx, &user_pol, 
					   num_groups, &user_groups);

	rpccli_samr_close(cli, mem_ctx, &user_pol);

	if (!NT_STATUS_IS_OK(result) || (*num_groups) == 0)
		return result;

	(*user_grpsids) = TALLOC_ARRAY(mem_ctx, DOM_SID, *num_groups);
	if (!(*user_grpsids))
		return NT_STATUS_NO_MEMORY;

	for (i=0;i<(*num_groups);i++) {
		sid_copy(&((*user_grpsids)[i]), &domain->sid);
		sid_append_rid(&((*user_grpsids)[i]),
				user_groups[i].g_rid);
	}
	
	return NT_STATUS_OK;
}

NTSTATUS msrpc_lookup_useraliases(struct winbindd_domain *domain,
				  TALLOC_CTX *mem_ctx,
				  uint32 num_sids, const DOM_SID *sids,
				  uint32 *num_aliases, uint32 **alias_rids)
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	POLICY_HND dom_pol;
	DOM_SID2 *query_sids;
	uint32 num_query_sids = 0;
	int i;
	struct rpc_pipe_client *cli;
	uint32 *alias_rids_query, num_aliases_query;
	int rangesize = MAX_SAM_ENTRIES_W2K;
	uint32 total_sids = 0;
	int num_queries = 1;

	*num_aliases = 0;
	*alias_rids = NULL;

	DEBUG(3,("rpc: lookup_useraliases\n"));

	result = cm_connect_sam(domain, mem_ctx, &cli, &dom_pol);
	if (!NT_STATUS_IS_OK(result))
		return result;

	do {
		/* prepare query */

		num_query_sids = MIN(num_sids - total_sids, rangesize);

		DEBUG(10,("rpc: lookup_useraliases: entering query %d for %d sids\n", 
			num_queries, num_query_sids));	


		query_sids = TALLOC_ARRAY(mem_ctx, DOM_SID2, num_query_sids);
		if (query_sids == NULL) {
			return NT_STATUS_NO_MEMORY;
		}

		for (i=0; i<num_query_sids; i++) {
			sid_copy(&query_sids[i].sid, &sids[total_sids++]);
			query_sids[i].num_auths = query_sids[i].sid.num_auths;
		}

		/* do request */

		result = rpccli_samr_query_useraliases(cli, mem_ctx, &dom_pol,
						       num_query_sids, query_sids,
						       &num_aliases_query, 
						       &alias_rids_query);

		if (!NT_STATUS_IS_OK(result)) {
			*num_aliases = 0;
			*alias_rids = NULL;
			TALLOC_FREE(query_sids);
			goto done;
		}

		/* process output */

		for (i=0; i<num_aliases_query; i++) {
			size_t na = *num_aliases;
			add_rid_to_array_unique(mem_ctx, alias_rids_query[i], 
						alias_rids, &na);
			*num_aliases = na;
		}

		TALLOC_FREE(query_sids);

		num_queries++;

	} while (total_sids < num_sids);

 done:
	DEBUG(10,("rpc: lookup_useraliases: got %d aliases in %d queries "
		"(rangesize: %d)\n", *num_aliases, num_queries, rangesize));

	return result;
}


/* Lookup group membership given a rid.   */
static NTSTATUS lookup_groupmem(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				const DOM_SID *group_sid, uint32 *num_names, 
				DOM_SID **sid_mem, char ***names, 
				uint32 **name_types)
{
        NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
        uint32 i, total_names = 0;
        POLICY_HND dom_pol, group_pol;
        uint32 des_access = SEC_RIGHTS_MAXIMUM_ALLOWED;
	uint32 *rid_mem = NULL;
	uint32 group_rid;
	unsigned int j;
	fstring sid_string;
	struct rpc_pipe_client *cli;
	unsigned int orig_timeout;

	DEBUG(10,("rpc: lookup_groupmem %s sid=%s\n", domain->name,
		  sid_to_string(sid_string, group_sid)));

	if (!sid_peek_check_rid(&domain->sid, group_sid, &group_rid))
		return NT_STATUS_UNSUCCESSFUL;

	*num_names = 0;

	result = cm_connect_sam(domain, mem_ctx, &cli, &dom_pol);
	if (!NT_STATUS_IS_OK(result))
		return result;

        result = rpccli_samr_open_group(cli, mem_ctx, &dom_pol,
					des_access, group_rid, &group_pol);

        if (!NT_STATUS_IS_OK(result))
		return result;

        /* Step #1: Get a list of user rids that are the members of the
           group. */

	/* This call can take a long time - allow the server to time out.
	   35 seconds should do it. */

	orig_timeout = cli_set_timeout(cli->cli, 35000);

        result = rpccli_samr_query_groupmem(cli, mem_ctx,
					    &group_pol, num_names, &rid_mem,
					    name_types);

	/* And restore our original timeout. */
	cli_set_timeout(cli->cli, orig_timeout);

	rpccli_samr_close(cli, mem_ctx, &group_pol);

        if (!NT_STATUS_IS_OK(result))
		return result;

	if (!*num_names) {
		names = NULL;
		name_types = NULL;
		sid_mem = NULL;
		return NT_STATUS_OK;
	}

        /* Step #2: Convert list of rids into list of usernames.  Do this
           in bunches of ~1000 to avoid crashing NT4.  It looks like there
           is a buffer overflow or something like that lurking around
           somewhere. */

#define MAX_LOOKUP_RIDS 900

        *names = TALLOC_ZERO_ARRAY(mem_ctx, char *, *num_names);
        *name_types = TALLOC_ZERO_ARRAY(mem_ctx, uint32, *num_names);
        *sid_mem = TALLOC_ZERO_ARRAY(mem_ctx, DOM_SID, *num_names);

	for (j=0;j<(*num_names);j++)
		sid_compose(&(*sid_mem)[j], &domain->sid, rid_mem[j]);
	
	if (*num_names>0 && (!*names || !*name_types))
		return NT_STATUS_NO_MEMORY;

        for (i = 0; i < *num_names; i += MAX_LOOKUP_RIDS) {
                int num_lookup_rids = MIN(*num_names - i, MAX_LOOKUP_RIDS);
                uint32 tmp_num_names = 0;
                char **tmp_names = NULL;
                uint32 *tmp_types = NULL;

                /* Lookup a chunk of rids */

                result = rpccli_samr_lookup_rids(cli, mem_ctx,
						 &dom_pol,
						 num_lookup_rids,
						 &rid_mem[i],
						 &tmp_num_names,
						 &tmp_names, &tmp_types);

		/* see if we have a real error (and yes the
		   STATUS_SOME_UNMAPPED is the one returned from 2k) */
		
                if (!NT_STATUS_IS_OK(result) &&
		    !NT_STATUS_EQUAL(result, STATUS_SOME_UNMAPPED))
			return result;
			
                /* Copy result into array.  The talloc system will take
                   care of freeing the temporary arrays later on. */

                memcpy(&(*names)[i], tmp_names, sizeof(char *) * 
                       tmp_num_names);

                memcpy(&(*name_types)[i], tmp_types, sizeof(uint32) *
                       tmp_num_names);
		
                total_names += tmp_num_names;
        }

        *num_names = total_names;

	return NT_STATUS_OK;
}

#ifdef HAVE_LDAP

#include <ldap.h>

static int get_ldap_seq(const char *server, int port, uint32 *seq)
{
	int ret = -1;
	struct timeval to;
	const char *attrs[] = {"highestCommittedUSN", NULL};
	LDAPMessage *res = NULL;
	char **values = NULL;
	LDAP *ldp = NULL;

	*seq = DOM_SEQUENCE_NONE;

	/*
	 * Parameterised (5) second timeout on open. This is needed as the
	 * search timeout doesn't seem to apply to doing an open as well. JRA.
	 */

	ldp = ldap_open_with_timeout(server, port, lp_ldap_timeout());
	if (ldp == NULL)
		return -1;

	/* Timeout if no response within 20 seconds. */
	to.tv_sec = 10;
	to.tv_usec = 0;

	if (ldap_search_st(ldp, "", LDAP_SCOPE_BASE, "(objectclass=*)",
			   CONST_DISCARD(char **, attrs), 0, &to, &res))
		goto done;

	if (ldap_count_entries(ldp, res) != 1)
		goto done;

	values = ldap_get_values(ldp, res, "highestCommittedUSN");
	if (!values || !values[0])
		goto done;

	*seq = atoi(values[0]);
	ret = 0;

  done:

	if (values)
		ldap_value_free(values);
	if (res)
		ldap_msgfree(res);
	if (ldp)
		ldap_unbind(ldp);
	return ret;
}

/**********************************************************************
 Get the sequence number for a Windows AD native mode domain using
 LDAP queries
**********************************************************************/

static int get_ldap_sequence_number( const char* domain, uint32 *seq)
{
	int ret = -1;
	int i, port = LDAP_PORT;
	struct ip_service *ip_list = NULL;
	int count;
	
	if ( !get_sorted_dc_list(domain, &ip_list, &count, False) ) {
		DEBUG(3, ("Could not look up dc's for domain %s\n", domain));
		return False;
	}

	/* Finally return first DC that we can contact */

	for (i = 0; i < count; i++) {
		fstring ipstr;

		/* since the is an LDAP lookup, default to the LDAP_PORT is
		 * not set */
		port = (ip_list[i].port!= PORT_NONE) ?
			ip_list[i].port : LDAP_PORT;

		fstrcpy( ipstr, inet_ntoa(ip_list[i].ip) );
		
		if (is_zero_ip(ip_list[i].ip))
			continue;

		if ( (ret = get_ldap_seq( ipstr, port,  seq)) == 0 )
			goto done;

		/* add to failed connection cache */
		add_failed_connection_entry( domain, ipstr,
					     NT_STATUS_UNSUCCESSFUL );
	}

done:
	if ( ret == 0 ) {
		DEBUG(3, ("get_ldap_sequence_number: Retrieved sequence "
			  "number for Domain (%s) from DC (%s:%d)\n", 
			domain, inet_ntoa(ip_list[i].ip), port));
	}

	SAFE_FREE(ip_list);

	return ret;
}

#endif /* HAVE_LDAP */

/* find the sequence number for a domain */
static NTSTATUS sequence_number(struct winbindd_domain *domain, uint32 *seq)
{
	TALLOC_CTX *mem_ctx;
	SAM_UNK_CTR ctr;
	NTSTATUS result;
	POLICY_HND dom_pol;
	BOOL got_seq_num = False;
	int retry;
	struct rpc_pipe_client *cli;

	DEBUG(10,("rpc: fetch sequence_number for %s\n", domain->name));

	*seq = DOM_SEQUENCE_NONE;

	if (!(mem_ctx = talloc_init("sequence_number[rpc]")))
		return NT_STATUS_NO_MEMORY;

	retry = 0;

#ifdef HAVE_LDAP
	if ( domain->native_mode ) 
	{
		int res;

		DEBUG(8,("using get_ldap_seq() to retrieve the "
			 "sequence number\n"));

		res =  get_ldap_sequence_number( domain->name, seq );
		if (res == 0)
		{			
			result = NT_STATUS_OK;
			DEBUG(10,("domain_sequence_number: LDAP for "
				  "domain %s is %u\n",
				  domain->name, *seq));
			goto done;
		}

		DEBUG(10,("domain_sequence_number: failed to get LDAP "
			  "sequence number for domain %s\n",
			  domain->name ));
	}
#endif /* HAVE_LDAP */

	result = cm_connect_sam(domain, mem_ctx, &cli, &dom_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	/* Query domain info */

	result = rpccli_samr_query_dom_info(cli, mem_ctx, &dom_pol, 8, &ctr);

	if (NT_STATUS_IS_OK(result)) {
		*seq = ctr.info.inf8.seq_num.low;
		got_seq_num = True;
		goto seq_num;
	}

	/* retry with info-level 2 in case the dc does not support info-level 8
	 * (like all older samba2 and samba3 dc's - Guenther */

	result = rpccli_samr_query_dom_info(cli, mem_ctx, &dom_pol, 2, &ctr);
	
	if (NT_STATUS_IS_OK(result)) {
		*seq = ctr.info.inf2.seq_num.low;
		got_seq_num = True;
	}

 seq_num:
	if (got_seq_num) {
		DEBUG(10,("domain_sequence_number: for domain %s is %u\n",
			  domain->name, (unsigned)*seq));
	} else {
		DEBUG(10,("domain_sequence_number: failed to get sequence "
			  "number (%u) for domain %s\n",
			  (unsigned)*seq, domain->name ));
	}

  done:

	talloc_destroy(mem_ctx);

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
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 enum_ctx = 0;
	struct rpc_pipe_client *cli;
	POLICY_HND lsa_policy;

	DEBUG(3,("rpc: trusted_domains\n"));

	*num_domains = 0;
	*names = NULL;
	*alt_names = NULL;
	*dom_sids = NULL;

	result = cm_connect_lsa(domain, mem_ctx, &cli, &lsa_policy);
	if (!NT_STATUS_IS_OK(result))
		return result;

	result = STATUS_MORE_ENTRIES;

	while (NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES)) {
		uint32 start_idx, num;
		char **tmp_names;
		DOM_SID *tmp_sids;
		int i;

		result = rpccli_lsa_enum_trust_dom(cli, mem_ctx,
						   &lsa_policy, &enum_ctx,
						   &num, &tmp_names,
						   &tmp_sids);

		if (!NT_STATUS_IS_OK(result) &&
		    !NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES))
			break;

		start_idx = *num_domains;
		*num_domains += num;
		*names = TALLOC_REALLOC_ARRAY(mem_ctx, *names,
					      char *, *num_domains);
		*dom_sids = TALLOC_REALLOC_ARRAY(mem_ctx, *dom_sids,
						 DOM_SID, *num_domains);
		*alt_names = TALLOC_REALLOC_ARRAY(mem_ctx, *alt_names,
						 char *, *num_domains);
		if ((*names == NULL) || (*dom_sids == NULL) ||
		    (*alt_names == NULL))
			return NT_STATUS_NO_MEMORY;

		for (i=0; i<num; i++) {
			(*names)[start_idx+i] = tmp_names[i];
			(*dom_sids)[start_idx+i] = tmp_sids[i];
			(*alt_names)[start_idx+i] = talloc_strdup(mem_ctx, "");
		}
	}
	return result;
}

/* find the lockout policy for a domain */
NTSTATUS msrpc_lockout_policy(struct winbindd_domain *domain, 
			      TALLOC_CTX *mem_ctx,
			      SAM_UNK_INFO_12 *lockout_policy)
{
	NTSTATUS result;
	struct rpc_pipe_client *cli;
	POLICY_HND dom_pol;
	SAM_UNK_CTR ctr;

	DEBUG(10,("rpc: fetch lockout policy for %s\n", domain->name));

	result = cm_connect_sam(domain, mem_ctx, &cli, &dom_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_samr_query_dom_info(cli, mem_ctx, &dom_pol, 12, &ctr);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	*lockout_policy = ctr.info.inf12;

	DEBUG(10,("msrpc_lockout_policy: bad_attempt_lockout %d\n", 
		ctr.info.inf12.bad_attempt_lockout));

  done:

	return result;
}

/* find the password policy for a domain */
NTSTATUS msrpc_password_policy(struct winbindd_domain *domain, 
			       TALLOC_CTX *mem_ctx,
			       SAM_UNK_INFO_1 *password_policy)
{
	NTSTATUS result;
	struct rpc_pipe_client *cli;
	POLICY_HND dom_pol;
	SAM_UNK_CTR ctr;

	DEBUG(10,("rpc: fetch password policy for %s\n", domain->name));

	result = cm_connect_sam(domain, mem_ctx, &cli, &dom_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_samr_query_dom_info(cli, mem_ctx, &dom_pol, 1, &ctr);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	*password_policy = ctr.info.inf1;

	DEBUG(10,("msrpc_password_policy: min_length_password %d\n", 
		ctr.info.inf1.min_length_password));

  done:

	return result;
}


/* the rpc backend methods are exposed via this structure */
struct winbindd_methods msrpc_methods = {
	False,
	query_user_list,
	enum_dom_groups,
	enum_local_groups,
	msrpc_name_to_sid,
	msrpc_sid_to_name,
	query_user,
	lookup_usergroups,
	msrpc_lookup_useraliases,
	lookup_groupmem,
	sequence_number,
	msrpc_lockout_policy,
	msrpc_password_policy,
	trusted_domains,
};
