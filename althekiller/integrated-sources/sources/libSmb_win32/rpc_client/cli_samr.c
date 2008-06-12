/* 
   Unix SMB/CIFS implementation.
   RPC pipe client
   Copyright (C) Tim Potter                        2000-2001,
   Copyright (C) Andrew Tridgell              1992-1997,2000,
   Copyright (C) Rafal Szczesniak                       2002.
   Copyright (C) Jeremy Allison                         2005.
   
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

/* Connect to SAMR database */

NTSTATUS rpccli_samr_connect(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, 
			     uint32 access_mask, POLICY_HND *connect_pol)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_CONNECT q;
	SAMR_R_CONNECT r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_connect to %s\n", cli->cli->desthost));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_connect(&q, cli->cli->desthost, access_mask);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_CONNECT,
		q, r,
		qbuf, rbuf,
		samr_io_q_connect,
		samr_io_r_connect,
		NT_STATUS_UNSUCCESSFUL); 
	/* Return output parameters */

	if (NT_STATUS_IS_OK(result = r.status)) {
		*connect_pol = r.connect_pol;
#ifdef __INSURE__
		connect_pol->marker = malloc(1);
#endif
	}

	return result;
}

/* Connect to SAMR database */

NTSTATUS rpccli_samr_connect4(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, 
			   uint32 access_mask, POLICY_HND *connect_pol)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_CONNECT4 q;
	SAMR_R_CONNECT4 r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_connect4(&q, cli->cli->desthost, access_mask);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_CONNECT4,
		q, r,
		qbuf, rbuf,
		samr_io_q_connect4,
		samr_io_r_connect4,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	if (NT_STATUS_IS_OK(result = r.status)) {
		*connect_pol = r.connect_pol;
#ifdef __INSURE__
		connect_pol->marker = malloc(1);
#endif
	}

	return result;
}

/* Close SAMR handle */

NTSTATUS rpccli_samr_close(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
			   POLICY_HND *connect_pol)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_CLOSE_HND q;
	SAMR_R_CLOSE_HND r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_close\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_close_hnd(&q, connect_pol);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_CLOSE_HND,
		q, r,
		qbuf, rbuf,
		samr_io_q_close_hnd,
		samr_io_r_close_hnd,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	if (NT_STATUS_IS_OK(result = r.status)) {
#ifdef __INSURE__
		SAFE_FREE(connect_pol->marker);
#endif
		*connect_pol = r.pol;
	}

	return result;
}

/* Open handle on a domain */

NTSTATUS rpccli_samr_open_domain(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				 POLICY_HND *connect_pol, uint32 access_mask, 
				 const DOM_SID *domain_sid,
				 POLICY_HND *domain_pol)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_OPEN_DOMAIN q;
	SAMR_R_OPEN_DOMAIN r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_open_domain with sid %s\n", sid_string_static(domain_sid) ));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_open_domain(&q, connect_pol, access_mask, domain_sid);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_OPEN_DOMAIN,
		q, r,
		qbuf, rbuf,
		samr_io_q_open_domain,
		samr_io_r_open_domain,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	if (NT_STATUS_IS_OK(result = r.status)) {
		*domain_pol = r.domain_pol;
#ifdef __INSURE__
		domain_pol->marker = malloc(1);
#endif
	}

	return result;
}

NTSTATUS rpccli_samr_open_user(struct rpc_pipe_client *cli,
			       TALLOC_CTX *mem_ctx,
			       POLICY_HND *domain_pol, uint32 access_mask, 
			       uint32 user_rid, POLICY_HND *user_pol)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_OPEN_USER q;
	SAMR_R_OPEN_USER r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_open_user with rid 0x%x\n", user_rid ));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_open_user(&q, domain_pol, access_mask, user_rid);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_OPEN_USER,
		q, r,
		qbuf, rbuf,
		samr_io_q_open_user,
		samr_io_r_open_user,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	if (NT_STATUS_IS_OK(result = r.status)) {
		*user_pol = r.user_pol;
#ifdef __INSURE__
		user_pol->marker = malloc(1);
#endif
	}

	return result;
}

/* Open handle on a group */

NTSTATUS rpccli_samr_open_group(struct rpc_pipe_client *cli,
				TALLOC_CTX *mem_ctx, 
				POLICY_HND *domain_pol, uint32 access_mask, 
				uint32 group_rid, POLICY_HND *group_pol)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_OPEN_GROUP q;
	SAMR_R_OPEN_GROUP r;
	NTSTATUS result =  NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_open_group with rid 0x%x\n", group_rid ));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_open_group(&q, domain_pol, access_mask, group_rid);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_OPEN_GROUP,
		q, r,
		qbuf, rbuf,
		samr_io_q_open_group,
		samr_io_r_open_group,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	if (NT_STATUS_IS_OK(result = r.status)) {
		*group_pol = r.pol;
#ifdef __INSURE__
		group_pol->marker = malloc(1);
#endif
	}

	return result;
}

/* Create domain group */

NTSTATUS rpccli_samr_create_dom_group(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				   POLICY_HND *domain_pol,
				   const char *group_name,
				   uint32 access_mask, POLICY_HND *group_pol)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_CREATE_DOM_GROUP q;
	SAMR_R_CREATE_DOM_GROUP r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_create_dom_group\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_create_dom_group(&q, domain_pol, group_name, access_mask);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_CREATE_DOM_GROUP,
		q, r,
		qbuf, rbuf,
		samr_io_q_create_dom_group,
		samr_io_r_create_dom_group,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	result = r.status;

	if (NT_STATUS_IS_OK(result))
		*group_pol = r.pol;

	return result;
}

/* Add a domain group member */

NTSTATUS rpccli_samr_add_groupmem(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
			       POLICY_HND *group_pol, uint32 rid)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_ADD_GROUPMEM q;
	SAMR_R_ADD_GROUPMEM r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_add_groupmem\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_add_groupmem(&q, group_pol, rid);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_ADD_GROUPMEM,
		q, r,
		qbuf, rbuf,
		samr_io_q_add_groupmem,
		samr_io_r_add_groupmem,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	result = r.status;

	return result;
}

/* Delete a domain group member */

NTSTATUS rpccli_samr_del_groupmem(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
			       POLICY_HND *group_pol, uint32 rid)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_DEL_GROUPMEM q;
	SAMR_R_DEL_GROUPMEM r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_del_groupmem\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_del_groupmem(&q, group_pol, rid);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_DEL_GROUPMEM,
		q, r,
		qbuf, rbuf,
		samr_io_q_del_groupmem,
		samr_io_r_del_groupmem,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	result = r.status;

	return result;
}

/* Query user info */

NTSTATUS rpccli_samr_query_userinfo(struct rpc_pipe_client *cli,
				    TALLOC_CTX *mem_ctx,
				    const POLICY_HND *user_pol,
				    uint16 switch_value, 
				    SAM_USERINFO_CTR **ctr)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_QUERY_USERINFO q;
	SAMR_R_QUERY_USERINFO r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_query_userinfo\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_query_userinfo(&q, user_pol, switch_value);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_QUERY_USERINFO,
		q, r,
		qbuf, rbuf,
		samr_io_q_query_userinfo,
		samr_io_r_query_userinfo,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	result = r.status;
	*ctr = r.ctr;

	return result;
}

/* Set group info */

NTSTATUS rpccli_samr_set_groupinfo(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				POLICY_HND *group_pol, GROUP_INFO_CTR *ctr)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_SET_GROUPINFO q;
	SAMR_R_SET_GROUPINFO r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_set_groupinfo\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_set_groupinfo(&q, group_pol, ctr);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_SET_GROUPINFO,
		q, r,
		qbuf, rbuf,
		samr_io_q_set_groupinfo,
		samr_io_r_set_groupinfo,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	result = r.status;

	return result;
}

/* Query group info */

NTSTATUS rpccli_samr_query_groupinfo(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                                  POLICY_HND *group_pol, uint32 info_level, 
                                  GROUP_INFO_CTR **ctr)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_QUERY_GROUPINFO q;
	SAMR_R_QUERY_GROUPINFO r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_query_groupinfo\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_query_groupinfo(&q, group_pol, info_level);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_QUERY_GROUPINFO,
		q, r,
		qbuf, rbuf,
		samr_io_q_query_groupinfo,
		samr_io_r_query_groupinfo,
		NT_STATUS_UNSUCCESSFUL); 

	*ctr = r.ctr;

	/* Return output parameters */

	result = r.status;

	return result;
}

/* Query user groups */

NTSTATUS rpccli_samr_query_usergroups(struct rpc_pipe_client *cli,
				      TALLOC_CTX *mem_ctx, 
				      POLICY_HND *user_pol,
				      uint32 *num_groups, 
				      DOM_GID **gid)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_QUERY_USERGROUPS q;
	SAMR_R_QUERY_USERGROUPS r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_query_usergroups\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_query_usergroups(&q, user_pol);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_QUERY_USERGROUPS,
		q, r,
		qbuf, rbuf,
		samr_io_q_query_usergroups,
		samr_io_r_query_usergroups,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	if (NT_STATUS_IS_OK(result = r.status)) {
		*num_groups = r.num_entries;
		*gid = r.gid;
	}

	return result;
}

/* Set alias info */

NTSTATUS rpccli_samr_set_aliasinfo(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				POLICY_HND *alias_pol, ALIAS_INFO_CTR *ctr)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_SET_ALIASINFO q;
	SAMR_R_SET_ALIASINFO r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_set_aliasinfo\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_set_aliasinfo(&q, alias_pol, ctr);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_SET_ALIASINFO,
		q, r,
		qbuf, rbuf,
		samr_io_q_set_aliasinfo,
		samr_io_r_set_aliasinfo,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	result = r.status;

	return result;
}

/* Query user aliases */

NTSTATUS rpccli_samr_query_useraliases(struct rpc_pipe_client *cli,
				       TALLOC_CTX *mem_ctx, 
				       POLICY_HND *dom_pol, uint32 num_sids,
				       DOM_SID2 *sid,
				       uint32 *num_aliases, uint32 **als_rids)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_QUERY_USERALIASES q;
	SAMR_R_QUERY_USERALIASES r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	int i;
	uint32 *sid_ptrs;
	
	DEBUG(10,("cli_samr_query_useraliases\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	sid_ptrs = TALLOC_ARRAY(mem_ctx, uint32, num_sids);
	if (sid_ptrs == NULL)
		return NT_STATUS_NO_MEMORY;

	for (i=0; i<num_sids; i++)
		sid_ptrs[i] = 1;

	/* Marshall data and send request */

	init_samr_q_query_useraliases(&q, dom_pol, num_sids, sid_ptrs, sid);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_QUERY_USERALIASES,
		q, r,
		qbuf, rbuf,
		samr_io_q_query_useraliases,
		samr_io_r_query_useraliases,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	if (NT_STATUS_IS_OK(result = r.status)) {
		*num_aliases = r.num_entries;
		*als_rids = r.rid;
	}

	return result;
}

/* Query user groups */

NTSTATUS rpccli_samr_query_groupmem(struct rpc_pipe_client *cli,
				    TALLOC_CTX *mem_ctx,
				    POLICY_HND *group_pol, uint32 *num_mem, 
				    uint32 **rid, uint32 **attr)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_QUERY_GROUPMEM q;
	SAMR_R_QUERY_GROUPMEM r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_query_groupmem\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_query_groupmem(&q, group_pol);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_QUERY_GROUPMEM,
		q, r,
		qbuf, rbuf,
		samr_io_q_query_groupmem,
		samr_io_r_query_groupmem,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	if (NT_STATUS_IS_OK(result = r.status)) {
		*num_mem = r.num_entries;
		*rid = r.rid;
		*attr = r.attr;
	}

	return result;
}

/**
 * Enumerate domain users
 *
 * @param cli client state structure
 * @param mem_ctx talloc context
 * @param pol opened domain policy handle
 * @param start_idx starting index of enumeration, returns context for
                    next enumeration
 * @param acb_mask account control bit mask (to enumerate some particular
 *                 kind of accounts)
 * @param size max acceptable size of response
 * @param dom_users returned array of domain user names
 * @param rids returned array of domain user RIDs
 * @param num_dom_users numer returned entries
 * 
 * @return NTSTATUS returned in rpc response
 **/

NTSTATUS rpccli_samr_enum_dom_users(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                                 POLICY_HND *pol, uint32 *start_idx, uint32 acb_mask,
                                 uint32 size, char ***dom_users, uint32 **rids,
                                 uint32 *num_dom_users)
{
	prs_struct qbuf;
	prs_struct rbuf;
	SAMR_Q_ENUM_DOM_USERS q;
	SAMR_R_ENUM_DOM_USERS r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	int i;
	
	DEBUG(10,("cli_samr_enum_dom_users starting at index %u\n", (unsigned int)*start_idx));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);
	
	/* always init this */
	*num_dom_users = 0;
	
	/* Fill query structure with parameters */

	init_samr_q_enum_dom_users(&q, pol, *start_idx, acb_mask, size);
	
	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_ENUM_DOM_USERS,
		q, r,
		qbuf, rbuf,
		samr_io_q_enum_dom_users,
		samr_io_r_enum_dom_users,
		NT_STATUS_UNSUCCESSFUL); 

	result = r.status;

	if (!NT_STATUS_IS_OK(result) &&
	    NT_STATUS_V(result) != NT_STATUS_V(STATUS_MORE_ENTRIES))
		goto done;
	
	*start_idx = r.next_idx;
	*num_dom_users = r.num_entries2;

	if (r.num_entries2) {
		/* allocate memory needed to return received data */	
		*rids = TALLOC_ARRAY(mem_ctx, uint32, r.num_entries2);
		if (!*rids) {
			DEBUG(0, ("Error in cli_samr_enum_dom_users(): out of memory\n"));
			return NT_STATUS_NO_MEMORY;
		}
		
		*dom_users = TALLOC_ARRAY(mem_ctx, char*, r.num_entries2);
		if (!*dom_users) {
			DEBUG(0, ("Error in cli_samr_enum_dom_users(): out of memory\n"));
			return NT_STATUS_NO_MEMORY;
		}
		
		/* fill output buffers with rpc response */
		for (i = 0; i < r.num_entries2; i++) {
			fstring conv_buf;
			
			(*rids)[i] = r.sam[i].rid;
			unistr2_to_ascii(conv_buf, &(r.uni_acct_name[i]), sizeof(conv_buf) - 1);
			(*dom_users)[i] = talloc_strdup(mem_ctx, conv_buf);
		}
	}
	
done:
	return result;
}

/* Enumerate domain groups */

NTSTATUS rpccli_samr_enum_dom_groups(struct rpc_pipe_client *cli,
				     TALLOC_CTX *mem_ctx, 
				     POLICY_HND *pol, uint32 *start_idx, 
				     uint32 size, struct acct_info **dom_groups,
				     uint32 *num_dom_groups)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_ENUM_DOM_GROUPS q;
	SAMR_R_ENUM_DOM_GROUPS r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 name_idx, i;

	DEBUG(10,("cli_samr_enum_dom_groups starting at index %u\n", (unsigned int)*start_idx));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_enum_dom_groups(&q, pol, *start_idx, size);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_ENUM_DOM_GROUPS,
		q, r,
		qbuf, rbuf,
		samr_io_q_enum_dom_groups,
		samr_io_r_enum_dom_groups,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	result = r.status;

	if (!NT_STATUS_IS_OK(result) &&
	    NT_STATUS_V(result) != NT_STATUS_V(STATUS_MORE_ENTRIES))
		goto done;

	*num_dom_groups = r.num_entries2;

	if (*num_dom_groups == 0)
		goto done;

	if (!((*dom_groups) = TALLOC_ARRAY(mem_ctx, struct acct_info, *num_dom_groups))) {
		result = NT_STATUS_NO_MEMORY;
		goto done;
	}

	memset(*dom_groups, 0, sizeof(struct acct_info) * (*num_dom_groups));

	name_idx = 0;

	for (i = 0; i < *num_dom_groups; i++) {

		(*dom_groups)[i].rid = r.sam[i].rid;

		if (r.sam[i].hdr_name.buffer) {
			unistr2_to_ascii((*dom_groups)[i].acct_name,
					 &r.uni_grp_name[name_idx],
					 sizeof(fstring) - 1);
			name_idx++;
		}

		*start_idx = r.next_idx;
	}

 done:
	return result;
}

/* Enumerate domain groups */

NTSTATUS rpccli_samr_enum_als_groups(struct rpc_pipe_client *cli,
				     TALLOC_CTX *mem_ctx, 
				     POLICY_HND *pol, uint32 *start_idx, 
				     uint32 size, struct acct_info **dom_aliases,
				     uint32 *num_dom_aliases)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_ENUM_DOM_ALIASES q;
	SAMR_R_ENUM_DOM_ALIASES r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 name_idx, i;

	DEBUG(10,("cli_samr_enum_als_groups starting at index %u\n", (unsigned int)*start_idx));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_enum_dom_aliases(&q, pol, *start_idx, size);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_ENUM_DOM_ALIASES,
		q, r,
		qbuf, rbuf,
		samr_io_q_enum_dom_aliases,
		samr_io_r_enum_dom_aliases,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	result = r.status;

	if (!NT_STATUS_IS_OK(result) &&
	    NT_STATUS_V(result) != NT_STATUS_V(STATUS_MORE_ENTRIES)) {
		goto done;
	}

	*num_dom_aliases = r.num_entries2;

	if (*num_dom_aliases == 0)
		goto done;

	if (!((*dom_aliases) = TALLOC_ARRAY(mem_ctx, struct acct_info, *num_dom_aliases))) {
		result = NT_STATUS_NO_MEMORY;
		goto done;
	}

	memset(*dom_aliases, 0, sizeof(struct acct_info) * *num_dom_aliases);

	name_idx = 0;

	for (i = 0; i < *num_dom_aliases; i++) {

		(*dom_aliases)[i].rid = r.sam[i].rid;

		if (r.sam[i].hdr_name.buffer) {
			unistr2_to_ascii((*dom_aliases)[i].acct_name,
					 &r.uni_grp_name[name_idx],
					 sizeof(fstring) - 1);
			name_idx++;
		}

		*start_idx = r.next_idx;
	}

 done:
	return result;
}

/* Query alias members */

NTSTATUS rpccli_samr_query_aliasmem(struct rpc_pipe_client *cli,
				    TALLOC_CTX *mem_ctx,
				    POLICY_HND *alias_pol, uint32 *num_mem, 
				    DOM_SID **sids)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_QUERY_ALIASMEM q;
	SAMR_R_QUERY_ALIASMEM r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 i;

	DEBUG(10,("cli_samr_query_aliasmem\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_query_aliasmem(&q, alias_pol);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_QUERY_ALIASMEM,
		q, r,
		qbuf, rbuf,
		samr_io_q_query_aliasmem,
		samr_io_r_query_aliasmem,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	if (!NT_STATUS_IS_OK(result = r.status)) {
		goto done;
	}

	*num_mem = r.num_sids;

	if (*num_mem == 0) {
		*sids = NULL;
		result = NT_STATUS_OK;
		goto done;
	}

	if (!(*sids = TALLOC_ARRAY(mem_ctx, DOM_SID, *num_mem))) {
		result = NT_STATUS_UNSUCCESSFUL;
		goto done;
	}

	for (i = 0; i < *num_mem; i++) {
		(*sids)[i] = r.sid[i].sid;
	}

 done:
	return result;
}

/* Open handle on an alias */

NTSTATUS rpccli_samr_open_alias(struct rpc_pipe_client *cli,
				TALLOC_CTX *mem_ctx, 
				POLICY_HND *domain_pol, uint32 access_mask, 
				uint32 alias_rid, POLICY_HND *alias_pol)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_OPEN_ALIAS q;
	SAMR_R_OPEN_ALIAS r;
	NTSTATUS result;

	DEBUG(10,("cli_samr_open_alias with rid 0x%x\n", alias_rid));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_open_alias(&q, domain_pol, access_mask, alias_rid);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_OPEN_ALIAS,
		q, r,
		qbuf, rbuf,
		samr_io_q_open_alias,
		samr_io_r_open_alias,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	if (NT_STATUS_IS_OK(result = r.status)) {
		*alias_pol = r.pol;
#ifdef __INSURE__
		alias_pol->marker = malloc(1);
#endif
	}

	return result;
}

/* Create an alias */

NTSTATUS rpccli_samr_create_dom_alias(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, 
				   POLICY_HND *domain_pol, const char *name,
				   POLICY_HND *alias_pol)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_CREATE_DOM_ALIAS q;
	SAMR_R_CREATE_DOM_ALIAS r;
	NTSTATUS result;

	DEBUG(10,("cli_samr_create_dom_alias named %s\n", name));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_create_dom_alias(&q, domain_pol, name);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_CREATE_DOM_ALIAS,
		q, r,
		qbuf, rbuf,
		samr_io_q_create_dom_alias,
		samr_io_r_create_dom_alias,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	if (NT_STATUS_IS_OK(result = r.status)) {
		*alias_pol = r.alias_pol;
	}

	return result;
}

/* Add an alias member */

NTSTATUS rpccli_samr_add_aliasmem(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, 
			       POLICY_HND *alias_pol, DOM_SID *member)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_ADD_ALIASMEM q;
	SAMR_R_ADD_ALIASMEM r;
	NTSTATUS result;

	DEBUG(10,("cli_samr_add_aliasmem"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_add_aliasmem(&q, alias_pol, member);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_ADD_ALIASMEM,
		q, r,
		qbuf, rbuf,
		samr_io_q_add_aliasmem,
		samr_io_r_add_aliasmem,
		NT_STATUS_UNSUCCESSFUL); 

	result = r.status;

	return result;
}

/* Delete an alias member */

NTSTATUS rpccli_samr_del_aliasmem(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, 
			       POLICY_HND *alias_pol, DOM_SID *member)
{
	prs_struct qbuf, rbuf;
 	SAMR_Q_DEL_ALIASMEM q;
 	SAMR_R_DEL_ALIASMEM r;
 	NTSTATUS result;

 	DEBUG(10,("cli_samr_del_aliasmem"));

 	ZERO_STRUCT(q);
 	ZERO_STRUCT(r);

 	/* Marshall data and send request */

 	init_samr_q_del_aliasmem(&q, alias_pol, member);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_DEL_ALIASMEM,
		q, r,
		qbuf, rbuf,
		samr_io_q_del_aliasmem,
		samr_io_r_del_aliasmem,
		NT_STATUS_UNSUCCESSFUL); 

	result = r.status;

	return result;
}

/* Query alias info */

NTSTATUS rpccli_samr_query_alias_info(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				   POLICY_HND *alias_pol, uint16 switch_value,
				   ALIAS_INFO_CTR *ctr)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_QUERY_ALIASINFO q;
	SAMR_R_QUERY_ALIASINFO r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_query_alias_info\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_query_aliasinfo(&q, alias_pol, switch_value);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_QUERY_ALIASINFO,
		q, r,
		qbuf, rbuf,
		samr_io_q_query_aliasinfo,
		samr_io_r_query_aliasinfo,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	if (!NT_STATUS_IS_OK(result = r.status)) {
		goto done;
	}

	*ctr = *r.ctr;

  done:

	return result;
}

/* Query domain info */

NTSTATUS rpccli_samr_query_dom_info(struct rpc_pipe_client *cli,
				    TALLOC_CTX *mem_ctx, 
				    POLICY_HND *domain_pol,
				    uint16 switch_value,
				    SAM_UNK_CTR *ctr)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_QUERY_DOMAIN_INFO q;
	SAMR_R_QUERY_DOMAIN_INFO r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_query_dom_info\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_query_domain_info(&q, domain_pol, switch_value);

	r.ctr = ctr;

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_QUERY_DOMAIN_INFO,
		q, r,
		qbuf, rbuf,
		samr_io_q_query_domain_info,
		samr_io_r_query_domain_info,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	if (!NT_STATUS_IS_OK(result = r.status)) {
		goto done;
	}

 done:

	return result;
}

/* Query domain info2 */

NTSTATUS rpccli_samr_query_dom_info2(struct rpc_pipe_client *cli,
				     TALLOC_CTX *mem_ctx, 
				     POLICY_HND *domain_pol,
				     uint16 switch_value,
				     SAM_UNK_CTR *ctr)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_QUERY_DOMAIN_INFO2 q;
	SAMR_R_QUERY_DOMAIN_INFO2 r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_query_dom_info2\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_query_domain_info2(&q, domain_pol, switch_value);

	r.ctr = ctr;

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_QUERY_DOMAIN_INFO2,
		q, r,
		qbuf, rbuf,
		samr_io_q_query_domain_info2,
		samr_io_r_query_domain_info2,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	if (!NT_STATUS_IS_OK(result = r.status)) {
		goto done;
	}

 done:

	return result;
}

/* Set domain info */

NTSTATUS rpccli_samr_set_domain_info(struct rpc_pipe_client *cli,
				     TALLOC_CTX *mem_ctx, 
				     POLICY_HND *domain_pol,
				     uint16 switch_value,
				     SAM_UNK_CTR *ctr)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_SET_DOMAIN_INFO q;
	SAMR_R_SET_DOMAIN_INFO r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_set_domain_info\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_set_domain_info(&q, domain_pol, switch_value, ctr);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_SET_DOMAIN_INFO,
		q, r,
		qbuf, rbuf,
		samr_io_q_set_domain_info,
		samr_io_r_set_domain_info,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	if (!NT_STATUS_IS_OK(result = r.status)) {
		goto done;
	}

 done:

	return result;
}

/* User change password */

NTSTATUS rpccli_samr_chgpasswd_user(struct rpc_pipe_client *cli,
				    TALLOC_CTX *mem_ctx, 
				    const char *username, 
				    const char *newpassword, 
				    const char *oldpassword )
{
	prs_struct qbuf, rbuf;
	SAMR_Q_CHGPASSWD_USER q;
	SAMR_R_CHGPASSWD_USER r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	uchar new_nt_password[516];
	uchar new_lm_password[516];
	uchar old_nt_hash[16];
	uchar old_lanman_hash[16];
	uchar old_nt_hash_enc[16];
	uchar old_lanman_hash_enc[16];

	uchar new_nt_hash[16];
	uchar new_lanman_hash[16];

	char *srv_name_slash = talloc_asprintf(mem_ctx, "\\\\%s", cli->cli->desthost);

	DEBUG(10,("rpccli_samr_chgpasswd_user\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Calculate the MD4 hash (NT compatible) of the password */
	E_md4hash(oldpassword, old_nt_hash);
	E_md4hash(newpassword, new_nt_hash);

	if (lp_client_lanman_auth() 
	    && E_deshash(newpassword, new_lanman_hash) 
	    && E_deshash(oldpassword, old_lanman_hash)) {
		/* E_deshash returns false for 'long' passwords (> 14
		   DOS chars).  This allows us to match Win2k, which
		   does not store a LM hash for these passwords (which
		   would reduce the effective password length to 14) */

		encode_pw_buffer(new_lm_password, newpassword, STR_UNICODE);

		SamOEMhash( new_lm_password, old_nt_hash, 516);
		E_old_pw_hash( new_nt_hash, old_lanman_hash, old_lanman_hash_enc);
	} else {
		ZERO_STRUCT(new_lm_password);
		ZERO_STRUCT(old_lanman_hash_enc);
	}

	encode_pw_buffer(new_nt_password, newpassword, STR_UNICODE);
	
	SamOEMhash( new_nt_password, old_nt_hash, 516);
	E_old_pw_hash( new_nt_hash, old_nt_hash, old_nt_hash_enc);

	/* Marshall data and send request */

	init_samr_q_chgpasswd_user(&q, srv_name_slash, username, 
				   new_nt_password, 
				   old_nt_hash_enc, 
				   new_lm_password,
				   old_lanman_hash_enc);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_CHGPASSWD_USER,
		q, r,
		qbuf, rbuf,
		samr_io_q_chgpasswd_user,
		samr_io_r_chgpasswd_user,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	if (!NT_STATUS_IS_OK(result = r.status)) {
		goto done;
	}

 done:

	return result;
}

/* change password 3 */

NTSTATUS rpccli_samr_chgpasswd3(struct rpc_pipe_client *cli,
				TALLOC_CTX *mem_ctx, 
				const char *username, 
				const char *newpassword, 
				const char *oldpassword,
				SAM_UNK_INFO_1 *info,
				SAMR_CHANGE_REJECT *reject)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_CHGPASSWD_USER3 q;
	SAMR_R_CHGPASSWD_USER3 r;

	uchar new_nt_password[516];
	uchar new_lm_password[516];
	uchar old_nt_hash[16];
	uchar old_lanman_hash[16];
	uchar old_nt_hash_enc[16];
	uchar old_lanman_hash_enc[16];

	uchar new_nt_hash[16];
	uchar new_lanman_hash[16];

	char *srv_name_slash = talloc_asprintf(mem_ctx, "\\\\%s", cli->cli->desthost);

	DEBUG(10,("rpccli_samr_chgpasswd_user3\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Calculate the MD4 hash (NT compatible) of the password */
	E_md4hash(oldpassword, old_nt_hash);
	E_md4hash(newpassword, new_nt_hash);

	if (lp_client_lanman_auth() 
	    && E_deshash(newpassword, new_lanman_hash) 
	    && E_deshash(oldpassword, old_lanman_hash)) {
		/* E_deshash returns false for 'long' passwords (> 14
		   DOS chars).  This allows us to match Win2k, which
		   does not store a LM hash for these passwords (which
		   would reduce the effective password length to 14) */

		encode_pw_buffer(new_lm_password, newpassword, STR_UNICODE);

		SamOEMhash( new_lm_password, old_nt_hash, 516);
		E_old_pw_hash( new_nt_hash, old_lanman_hash, old_lanman_hash_enc);
	} else {
		ZERO_STRUCT(new_lm_password);
		ZERO_STRUCT(old_lanman_hash_enc);
	}

	encode_pw_buffer(new_nt_password, newpassword, STR_UNICODE);
	
	SamOEMhash( new_nt_password, old_nt_hash, 516);
	E_old_pw_hash( new_nt_hash, old_nt_hash, old_nt_hash_enc);

	/* Marshall data and send request */

	init_samr_q_chgpasswd_user3(&q, srv_name_slash, username, 
				    new_nt_password, 
				    old_nt_hash_enc, 
				    new_lm_password,
				    old_lanman_hash_enc);
	r.info = info;
	r.reject = reject;

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_CHGPASSWD_USER3,
		q, r,
		qbuf, rbuf,
		samr_io_q_chgpasswd_user3,
		samr_io_r_chgpasswd_user3,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	return r.status;
}

/* This function returns the bizzare set of (max_entries, max_size) required
   for the QueryDisplayInfo RPC to actually work against a domain controller
   with large (10k and higher) numbers of users.  These values were 
   obtained by inspection using ethereal and NT4 running User Manager. */

void get_query_dispinfo_params(int loop_count, uint32 *max_entries,
			       uint32 *max_size)
{
	switch(loop_count) {
	case 0:
		*max_entries = 512;
		*max_size = 16383;
		break;
	case 1:
		*max_entries = 1024;
		*max_size = 32766;
		break;
	case 2:
		*max_entries = 2048;
		*max_size = 65532;
		break;
	case 3:
		*max_entries = 4096;
		*max_size = 131064;
		break;
	default:              /* loop_count >= 4 */
		*max_entries = 4096;
		*max_size = 131071;
		break;
	}
}		     

/* Query display info */

NTSTATUS rpccli_samr_query_dispinfo(struct rpc_pipe_client *cli,
				    TALLOC_CTX *mem_ctx, 
				    POLICY_HND *domain_pol, uint32 *start_idx,
				    uint16 switch_value, uint32 *num_entries,
				    uint32 max_entries, uint32 max_size,
				    SAM_DISPINFO_CTR *ctr)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_QUERY_DISPINFO q;
	SAMR_R_QUERY_DISPINFO r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_query_dispinfo for start_idx = %u\n", *start_idx));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	*num_entries = 0;

	/* Marshall data and send request */

	init_samr_q_query_dispinfo(&q, domain_pol, switch_value,
				   *start_idx, max_entries, max_size);

	r.ctr = ctr;

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_QUERY_DISPINFO,
		q, r,
		qbuf, rbuf,
		samr_io_q_query_dispinfo,
		samr_io_r_query_dispinfo,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

        result = r.status;

	if (!NT_STATUS_IS_OK(result) &&
	    NT_STATUS_V(result) != NT_STATUS_V(STATUS_MORE_ENTRIES)) {
		goto done;
	}

	*num_entries = r.num_entries;
	*start_idx += r.num_entries;  /* No next_idx in this structure! */

 done:
	return result;
}

/* Lookup rids.  Note that NT4 seems to crash if more than ~1000 rids are
   looked up in one packet. */

NTSTATUS rpccli_samr_lookup_rids(struct rpc_pipe_client *cli,
				 TALLOC_CTX *mem_ctx, 
				 POLICY_HND *domain_pol,
				 uint32 num_rids, uint32 *rids, 
				 uint32 *num_names, char ***names,
				 uint32 **name_types)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_LOOKUP_RIDS q;
	SAMR_R_LOOKUP_RIDS r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 i;

	DEBUG(10,("cli_samr_lookup_rids\n"));

        if (num_rids > 1000) {
                DEBUG(2, ("cli_samr_lookup_rids: warning: NT4 can crash if "
                          "more than ~1000 rids are looked up at once.\n"));
        }

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_lookup_rids(mem_ctx, &q, domain_pol, 1000, num_rids, rids);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_LOOKUP_RIDS,
		q, r,
		qbuf, rbuf,
		samr_io_q_lookup_rids,
		samr_io_r_lookup_rids,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	result = r.status;

	if (!NT_STATUS_IS_OK(result) &&
	    !NT_STATUS_EQUAL(result, STATUS_SOME_UNMAPPED))
		goto done;

	if (r.num_names1 == 0) {
		*num_names = 0;
		*names = NULL;
		goto done;
	}

	*num_names = r.num_names1;
	*names = TALLOC_ARRAY(mem_ctx, char *, r.num_names1);
	*name_types = TALLOC_ARRAY(mem_ctx, uint32, r.num_names1);

	if ((*names == NULL) || (*name_types == NULL)) {
		TALLOC_FREE(*names);
		TALLOC_FREE(*name_types);
		return NT_STATUS_NO_MEMORY;
	}

	for (i = 0; i < r.num_names1; i++) {
		fstring tmp;

		unistr2_to_ascii(tmp, &r.uni_name[i], sizeof(tmp) - 1);
		(*names)[i] = talloc_strdup(mem_ctx, tmp);
		(*name_types)[i] = r.type[i];
	}

 done:

	return result;
}

/* Lookup names */

NTSTATUS rpccli_samr_lookup_names(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, 
                               POLICY_HND *domain_pol, uint32 flags,
                               uint32 num_names, const char **names,
                               uint32 *num_rids, uint32 **rids,
                               uint32 **rid_types)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_LOOKUP_NAMES q;
	SAMR_R_LOOKUP_NAMES r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 i;

	DEBUG(10,("cli_samr_lookup_names\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_lookup_names(mem_ctx, &q, domain_pol, flags,
				 num_names, names);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_LOOKUP_NAMES,
		q, r,
		qbuf, rbuf,
		samr_io_q_lookup_names,
		samr_io_r_lookup_names,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	if (!NT_STATUS_IS_OK(result = r.status)) {
		goto done;
	}

	if (r.num_rids1 == 0) {
		*num_rids = 0;
		goto done;
	}

	*num_rids = r.num_rids1;
	*rids = TALLOC_ARRAY(mem_ctx, uint32, r.num_rids1);
	*rid_types = TALLOC_ARRAY(mem_ctx, uint32, r.num_rids1);

	if ((*rids == NULL) || (*rid_types == NULL)) {
		TALLOC_FREE(*rids);
		TALLOC_FREE(*rid_types);
		return NT_STATUS_NO_MEMORY;
	}

	for (i = 0; i < r.num_rids1; i++) {
		(*rids)[i] = r.rids[i];
		(*rid_types)[i] = r.types[i];
	}

 done:

	return result;
}

/* Create a domain user */

NTSTATUS rpccli_samr_create_dom_user(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, 
                                  POLICY_HND *domain_pol, const char *acct_name,
                                  uint32 acb_info, uint32 unknown, 
                                  POLICY_HND *user_pol, uint32 *rid)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_CREATE_USER q;
	SAMR_R_CREATE_USER r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_create_dom_user %s\n", acct_name));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_create_user(&q, domain_pol, acct_name, acb_info, unknown);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_CREATE_USER,
		q, r,
		qbuf, rbuf,
		samr_io_q_create_user,
		samr_io_r_create_user,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	if (!NT_STATUS_IS_OK(result = r.status)) {
		goto done;
	}

	if (user_pol)
		*user_pol = r.user_pol;

	if (rid)
		*rid = r.user_rid;

 done:

	return result;
}

/* Set userinfo */

NTSTATUS rpccli_samr_set_userinfo(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, 
                               const POLICY_HND *user_pol, uint16 switch_value,
                               DATA_BLOB *sess_key, SAM_USERINFO_CTR *ctr)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_SET_USERINFO q;
	SAMR_R_SET_USERINFO r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_set_userinfo\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	if (!sess_key->length) {
		DEBUG(1, ("No user session key\n"));
		return NT_STATUS_NO_USER_SESSION_KEY;
	}

	/* Initialise parse structures */

	prs_init(&qbuf, RPC_MAX_PDU_FRAG_LEN, mem_ctx, MARSHALL);
	prs_init(&rbuf, 0, mem_ctx, UNMARSHALL);

	/* Marshall data and send request */

	q.ctr = ctr;

	init_samr_q_set_userinfo(&q, user_pol, sess_key, switch_value, 
				 ctr->info.id);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_SET_USERINFO,
		q, r,
		qbuf, rbuf,
		samr_io_q_set_userinfo,
		samr_io_r_set_userinfo,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	if (!NT_STATUS_IS_OK(result = r.status)) {
		goto done;
	}

 done:

	return result;
}

/* Set userinfo2 */

NTSTATUS rpccli_samr_set_userinfo2(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, 
				   const POLICY_HND *user_pol, uint16 switch_value,
                                DATA_BLOB *sess_key, SAM_USERINFO_CTR *ctr)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_SET_USERINFO2 q;
	SAMR_R_SET_USERINFO2 r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_set_userinfo2\n"));

	if (!sess_key->length) {
		DEBUG(1, ("No user session key\n"));
		return NT_STATUS_NO_USER_SESSION_KEY;
	}

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_set_userinfo2(&q, user_pol, sess_key, switch_value, ctr);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_SET_USERINFO2,
		q, r,
		qbuf, rbuf,
		samr_io_q_set_userinfo2,
		samr_io_r_set_userinfo2,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	if (!NT_STATUS_IS_OK(result = r.status)) {
		goto done;
	}

 done:

	return result;
}

/* Delete domain group */

NTSTATUS rpccli_samr_delete_dom_group(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, 
                                  POLICY_HND *group_pol)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_DELETE_DOM_GROUP q;
	SAMR_R_DELETE_DOM_GROUP r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_delete_dom_group\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_delete_dom_group(&q, group_pol);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_DELETE_DOM_GROUP,
		q, r,
		qbuf, rbuf,
		samr_io_q_delete_dom_group,
		samr_io_r_delete_dom_group,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	result = r.status;

	return result;
}

/* Delete domain alias */

NTSTATUS rpccli_samr_delete_dom_alias(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, 
                                  POLICY_HND *alias_pol)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_DELETE_DOM_ALIAS q;
	SAMR_R_DELETE_DOM_ALIAS r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_delete_dom_alias\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_delete_dom_alias(&q, alias_pol);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_DELETE_DOM_ALIAS,
		q, r,
		qbuf, rbuf,
		samr_io_q_delete_dom_alias,
		samr_io_r_delete_dom_alias,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	result = r.status;

	return result;
}

/* Delete domain user */

NTSTATUS rpccli_samr_delete_dom_user(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, 
                                  POLICY_HND *user_pol)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_DELETE_DOM_USER q;
	SAMR_R_DELETE_DOM_USER r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_delete_dom_user\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_delete_dom_user(&q, user_pol);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_DELETE_DOM_USER,
		q, r,
		qbuf, rbuf,
		samr_io_q_delete_dom_user,
		samr_io_r_delete_dom_user,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	result = r.status;

	return result;
}

/* Remove foreign SID */

NTSTATUS rpccli_samr_remove_sid_foreign_domain(struct rpc_pipe_client *cli, 
					    TALLOC_CTX *mem_ctx, 
					    POLICY_HND *user_pol,
					    DOM_SID *sid)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_REMOVE_SID_FOREIGN_DOMAIN q;
	SAMR_R_REMOVE_SID_FOREIGN_DOMAIN r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_remove_sid_foreign_domain\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_remove_sid_foreign_domain(&q, user_pol, sid);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_REMOVE_SID_FOREIGN_DOMAIN,
		q, r,
		qbuf, rbuf,
		samr_io_q_remove_sid_foreign_domain,
		samr_io_r_remove_sid_foreign_domain,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	result = r.status;

	return result;
}

/* Query user security object */

NTSTATUS rpccli_samr_query_sec_obj(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                                 POLICY_HND *user_pol, uint32 sec_info, 
                                 TALLOC_CTX *ctx, SEC_DESC_BUF **sec_desc_buf)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_QUERY_SEC_OBJ q;
	SAMR_R_QUERY_SEC_OBJ r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_query_sec_obj\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_query_sec_obj(&q, user_pol, sec_info);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_QUERY_SEC_OBJECT,
		q, r,
		qbuf, rbuf,
		samr_io_q_query_sec_obj,
		samr_io_r_query_sec_obj,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	result = r.status;
	*sec_desc_buf=dup_sec_desc_buf(ctx, r.buf);

	return result;
}

/* Set user security object */

NTSTATUS rpccli_samr_set_sec_obj(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                                 POLICY_HND *user_pol, uint32 sec_info, 
                                 SEC_DESC_BUF *sec_desc_buf)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_SET_SEC_OBJ q;
	SAMR_R_SET_SEC_OBJ r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_set_sec_obj\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_set_sec_obj(&q, user_pol, sec_info, sec_desc_buf);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_SET_SEC_OBJECT,
		q, r,
		qbuf, rbuf,
		samr_io_q_set_sec_obj,
		samr_io_r_set_sec_obj,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	result = r.status;

	return result;
}


/* Get domain password info */

NTSTATUS rpccli_samr_get_dom_pwinfo(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				 uint16 *min_pwd_length, uint32 *password_properties)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_GET_DOM_PWINFO q;
	SAMR_R_GET_DOM_PWINFO r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_get_dom_pwinfo\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_get_dom_pwinfo(&q, cli->cli->desthost);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_GET_DOM_PWINFO,
		q, r,
		qbuf, rbuf,
		samr_io_q_get_dom_pwinfo,
		samr_io_r_get_dom_pwinfo,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	result = r.status;

	if (NT_STATUS_IS_OK(result)) {
		if (min_pwd_length)
			*min_pwd_length = r.min_pwd_length;
		if (password_properties)
			*password_properties = r.password_properties;
	}

	return result;
}

/* Get domain password info */

NTSTATUS rpccli_samr_get_usrdom_pwinfo(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				       POLICY_HND *pol, uint16 *min_pwd_length, 
				       uint32 *password_properties, uint32 *unknown1)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_GET_USRDOM_PWINFO q;
	SAMR_R_GET_USRDOM_PWINFO r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_get_usrdom_pwinfo\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_get_usrdom_pwinfo(&q, pol);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_GET_USRDOM_PWINFO,
		   q, r,
		   qbuf, rbuf,
		   samr_io_q_get_usrdom_pwinfo,
		   samr_io_r_get_usrdom_pwinfo,
		   NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	result = r.status;

	if (NT_STATUS_IS_OK(result)) {
		if (min_pwd_length)
			*min_pwd_length = r.min_pwd_length;
		if (password_properties)
			*password_properties = r.password_properties;
		if (unknown1)
			*unknown1 = r.unknown_1;
	}

	return result;
}


/* Lookup Domain Name */

NTSTATUS rpccli_samr_lookup_domain(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				POLICY_HND *user_pol, char *domain_name, 
				DOM_SID *sid)
{
	prs_struct qbuf, rbuf;
	SAMR_Q_LOOKUP_DOMAIN q;
	SAMR_R_LOOKUP_DOMAIN r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DEBUG(10,("cli_samr_lookup_domain\n"));

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_samr_q_lookup_domain(&q, user_pol, domain_name);

	CLI_DO_RPC(cli, mem_ctx, PI_SAMR, SAMR_LOOKUP_DOMAIN,
		q, r,
		qbuf, rbuf,
		samr_io_q_lookup_domain,
		samr_io_r_lookup_domain,
		NT_STATUS_UNSUCCESSFUL); 

	/* Return output parameters */

	result = r.status;

	if (NT_STATUS_IS_OK(result))
		sid_copy(sid, &r.dom_sid.sid);

	return result;
}
