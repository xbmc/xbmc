/* 
   Unix SMB/CIFS implementation.
   RPC pipe client
   Copyright (C) Tim Potter                        2000-2001,
   Copyright (C) Andrew Tridgell              1992-1997,2000,
   Copyright (C) Rafal Szczesniak                       2002
   Copyright (C) Jeremy Allison				2005.
   
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

/** @defgroup lsa LSA - Local Security Architecture
 *  @ingroup rpc_client
 *
 * @{
 **/

/**
 * @file cli_lsarpc.c
 *
 * RPC client routines for the LSA RPC pipe.  LSA means "local
 * security authority", which is half of a password database.
 **/

/** Open a LSA policy handle
 *
 * @param cli Handle on an initialised SMB connection */

NTSTATUS rpccli_lsa_open_policy(struct rpc_pipe_client *cli,
				TALLOC_CTX *mem_ctx,
				BOOL sec_qos, uint32 des_access,
				POLICY_HND *pol)
{
	prs_struct qbuf, rbuf;
	LSA_Q_OPEN_POL q;
	LSA_R_OPEN_POL r;
	LSA_SEC_QOS qos;
	NTSTATUS result;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Initialise input parameters */

	if (sec_qos) {
		init_lsa_sec_qos(&qos, 2, 1, 0);
		init_q_open_pol(&q, '\\', 0, des_access, &qos);
	} else {
		init_q_open_pol(&q, '\\', 0, des_access, NULL);
	}

	/* Marshall data and send request */

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_OPENPOLICY,
			q, r,
			qbuf, rbuf,
			lsa_io_q_open_pol,
			lsa_io_r_open_pol,
			NT_STATUS_UNSUCCESSFUL );

	/* Return output parameters */

	result = r.status;

	if (NT_STATUS_IS_OK(result)) {
		*pol = r.pol;
#ifdef __INSURE__
		pol->marker = MALLOC(1);
#endif
	}

	return result;
}

/** Open a LSA policy handle
  *
  * @param cli Handle on an initialised SMB connection 
  */

NTSTATUS rpccli_lsa_open_policy2(struct rpc_pipe_client *cli,
				 TALLOC_CTX *mem_ctx, BOOL sec_qos,
				 uint32 des_access, POLICY_HND *pol)
{
	prs_struct qbuf, rbuf;
	LSA_Q_OPEN_POL2 q;
	LSA_R_OPEN_POL2 r;
	LSA_SEC_QOS qos;
	NTSTATUS result;
	char *srv_name_slash = talloc_asprintf(mem_ctx, "\\\\%s", cli->cli->desthost);

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	if (sec_qos) {
		init_lsa_sec_qos(&qos, 2, 1, 0);
		init_q_open_pol2(&q, srv_name_slash, 0, des_access, &qos);
	} else {
		init_q_open_pol2(&q, srv_name_slash, 0, des_access, NULL);
	}

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_OPENPOLICY2,
			q, r,
			qbuf, rbuf,
			lsa_io_q_open_pol2,
			lsa_io_r_open_pol2,
			NT_STATUS_UNSUCCESSFUL );

	/* Return output parameters */

	result = r.status;

	if (NT_STATUS_IS_OK(result)) {
		*pol = r.pol;
#ifdef __INSURE__
		pol->marker = (char *)malloc(1);
#endif
	}

	return result;
}

/** Close a LSA policy handle */

NTSTATUS rpccli_lsa_close(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, 
			  POLICY_HND *pol)
{
	prs_struct qbuf, rbuf;
	LSA_Q_CLOSE q;
	LSA_R_CLOSE r;
	NTSTATUS result;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	init_lsa_q_close(&q, pol);

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_CLOSE,
			q, r,
			qbuf, rbuf,
			lsa_io_q_close,
			lsa_io_r_close,
			NT_STATUS_UNSUCCESSFUL );

	/* Return output parameters */

	result = r.status;

	if (NT_STATUS_IS_OK(result)) {
#ifdef __INSURE__
		SAFE_FREE(pol->marker);
#endif
		*pol = r.pol;
	}

	return result;
}

/** Lookup a list of sids */

NTSTATUS rpccli_lsa_lookup_sids(struct rpc_pipe_client *cli,
				TALLOC_CTX *mem_ctx,
				POLICY_HND *pol, int num_sids,
				const DOM_SID *sids, 
				char ***domains, char ***names, uint32 **types)
{
	prs_struct qbuf, rbuf;
	LSA_Q_LOOKUP_SIDS q;
	LSA_R_LOOKUP_SIDS r;
	DOM_R_REF ref;
	LSA_TRANS_NAME_ENUM t_names;
	NTSTATUS result = NT_STATUS_OK;
	int i;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	init_q_lookup_sids(mem_ctx, &q, pol, num_sids, sids, 1);

	ZERO_STRUCT(ref);
	ZERO_STRUCT(t_names);

	r.dom_ref = &ref;
	r.names = &t_names;

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_LOOKUPSIDS,
			q, r,
			qbuf, rbuf,
			lsa_io_q_lookup_sids,
			lsa_io_r_lookup_sids,
			NT_STATUS_UNSUCCESSFUL );

	if (!NT_STATUS_IS_OK(r.status) &&
	    NT_STATUS_V(r.status) != NT_STATUS_V(STATUS_SOME_UNMAPPED)) {
	  
		/* An actual error occured */
		result = r.status;

		goto done;
	}

	/* Return output parameters */

	if (r.mapped_count == 0) {
		result = NT_STATUS_NONE_MAPPED;
		goto done;
	}

	if (!((*domains) = TALLOC_ARRAY(mem_ctx, char *, num_sids))) {
		DEBUG(0, ("cli_lsa_lookup_sids(): out of memory\n"));
		result = NT_STATUS_NO_MEMORY;
		goto done;
	}

	if (!((*names) = TALLOC_ARRAY(mem_ctx, char *, num_sids))) {
		DEBUG(0, ("cli_lsa_lookup_sids(): out of memory\n"));
		result = NT_STATUS_NO_MEMORY;
		goto done;
	}

	if (!((*types) = TALLOC_ARRAY(mem_ctx, uint32, num_sids))) {
		DEBUG(0, ("cli_lsa_lookup_sids(): out of memory\n"));
		result = NT_STATUS_NO_MEMORY;
		goto done;
	}
		
	for (i = 0; i < num_sids; i++) {
		fstring name, dom_name;
		uint32 dom_idx = t_names.name[i].domain_idx;

		/* Translate optimised name through domain index array */

		if (dom_idx != 0xffffffff) {

			rpcstr_pull_unistr2_fstring(
                                dom_name, &ref.ref_dom[dom_idx].uni_dom_name);
			rpcstr_pull_unistr2_fstring(
                                name, &t_names.uni_name[i]);

			(*names)[i] = talloc_strdup(mem_ctx, name);
			(*domains)[i] = talloc_strdup(mem_ctx, dom_name);
			(*types)[i] = t_names.name[i].sid_name_use;
			
			if (((*names)[i] == NULL) || ((*domains)[i] == NULL)) {
				DEBUG(0, ("cli_lsa_lookup_sids(): out of memory\n"));
				result = NT_STATUS_UNSUCCESSFUL;
				goto done;
			}

		} else {
			(*names)[i] = NULL;
			(*domains)[i] = NULL;
			(*types)[i] = SID_NAME_UNKNOWN;
		}
	}

 done:

	return result;
}

/** Lookup a list of names */

NTSTATUS rpccli_lsa_lookup_names(struct rpc_pipe_client *cli,
				 TALLOC_CTX *mem_ctx,
				 POLICY_HND *pol, int num_names, 
				 const char **names,
				 const char ***dom_names,
				 DOM_SID **sids,
				 uint32 **types)
{
	prs_struct qbuf, rbuf;
	LSA_Q_LOOKUP_NAMES q;
	LSA_R_LOOKUP_NAMES r;
	DOM_R_REF ref;
	NTSTATUS result;
	int i;
	
	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	ZERO_STRUCT(ref);
	r.dom_ref = &ref;

	init_q_lookup_names(mem_ctx, &q, pol, num_names, names);

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_LOOKUPNAMES,
			q, r,
			qbuf, rbuf,
			lsa_io_q_lookup_names,
			lsa_io_r_lookup_names,
			NT_STATUS_UNSUCCESSFUL);

	result = r.status;

	if (!NT_STATUS_IS_OK(result) && NT_STATUS_V(result) !=
	    NT_STATUS_V(STATUS_SOME_UNMAPPED)) {

		/* An actual error occured */

		goto done;
	}

	/* Return output parameters */

	if (r.mapped_count == 0) {
		result = NT_STATUS_NONE_MAPPED;
		goto done;
	}

	if (!((*sids = TALLOC_ARRAY(mem_ctx, DOM_SID, num_names)))) {
		DEBUG(0, ("cli_lsa_lookup_sids(): out of memory\n"));
		result = NT_STATUS_NO_MEMORY;
		goto done;
	}

	if (!((*types = TALLOC_ARRAY(mem_ctx, uint32, num_names)))) {
		DEBUG(0, ("cli_lsa_lookup_sids(): out of memory\n"));
		result = NT_STATUS_NO_MEMORY;
		goto done;
	}

	if (dom_names != NULL) {
		*dom_names = TALLOC_ARRAY(mem_ctx, const char *, num_names);
		if (*dom_names == NULL) {
			DEBUG(0, ("cli_lsa_lookup_sids(): out of memory\n"));
			result = NT_STATUS_NO_MEMORY;
			goto done;
		}
	}

	for (i = 0; i < num_names; i++) {
		DOM_RID *t_rids = r.dom_rid;
		uint32 dom_idx = t_rids[i].rid_idx;
		uint32 dom_rid = t_rids[i].rid;
		DOM_SID *sid = &(*sids)[i];

		/* Translate optimised sid through domain index array */

		if (dom_idx == 0xffffffff) {
			/* Nothing to do, this is unknown */
			ZERO_STRUCTP(sid);
			(*types)[i] = SID_NAME_UNKNOWN;
			continue;
		}

		sid_copy(sid, &ref.ref_dom[dom_idx].ref_dom.sid);

		if (dom_rid != 0xffffffff) {
			sid_append_rid(sid, dom_rid);
		}

		(*types)[i] = t_rids[i].type;

		if (dom_names == NULL) {
			continue;
		}

		(*dom_names)[i] = rpcstr_pull_unistr2_talloc(
			*dom_names, &ref.ref_dom[dom_idx].uni_dom_name);
	}

 done:

	return result;
}

NTSTATUS rpccli_lsa_query_info_policy_new(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
					  POLICY_HND *pol, uint16 info_class,
					  LSA_INFO_CTR *ctr) 
{
	prs_struct qbuf, rbuf;
	LSA_Q_QUERY_INFO q;
	LSA_R_QUERY_INFO r;
	NTSTATUS result;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	init_q_query(&q, pol, info_class);

	CLI_DO_RPC(cli, mem_ctx, PI_LSARPC, LSA_QUERYINFOPOLICY,
		q, r,
		qbuf, rbuf,
		lsa_io_q_query,
		lsa_io_r_query,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

 done:

	*ctr = r.ctr;
	
	return result;
}

NTSTATUS rpccli_lsa_query_info_policy2_new(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
					  POLICY_HND *pol, uint16 info_class,
					  LSA_INFO_CTR2 *ctr) 
{
	prs_struct qbuf, rbuf;
	LSA_Q_QUERY_INFO2 q;
	LSA_R_QUERY_INFO2 r;
	NTSTATUS result;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	init_q_query2(&q, pol, info_class);

	CLI_DO_RPC(cli, mem_ctx, PI_LSARPC, LSA_QUERYINFO2,
		q, r,
		qbuf, rbuf,
		lsa_io_q_query_info2,
		lsa_io_r_query_info2,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

 done:

	*ctr = r.ctr;
	
	return result;
}



/** Query info policy
 *
 *  @param domain_sid - returned remote server's domain sid */

NTSTATUS rpccli_lsa_query_info_policy(struct rpc_pipe_client *cli,
				      TALLOC_CTX *mem_ctx,
				      POLICY_HND *pol, uint16 info_class, 
				      char **domain_name, DOM_SID **domain_sid)
{
	prs_struct qbuf, rbuf;
	LSA_Q_QUERY_INFO q;
	LSA_R_QUERY_INFO r;
	NTSTATUS result;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	init_q_query(&q, pol, info_class);

	CLI_DO_RPC(cli, mem_ctx, PI_LSARPC, LSA_QUERYINFOPOLICY,
		q, r,
		qbuf, rbuf,
		lsa_io_q_query,
		lsa_io_r_query,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	/* Return output parameters */

	switch (info_class) {

	case 3:
		if (domain_name && (r.ctr.info.id3.buffer_dom_name != 0)) {
			*domain_name = unistr2_tdup(mem_ctx, 
						   &r.ctr.info.id3.
						   uni_domain_name);
			if (!*domain_name) {
				return NT_STATUS_NO_MEMORY;
			}
		}

		if (domain_sid && (r.ctr.info.id3.buffer_dom_sid != 0)) {
			*domain_sid = TALLOC_P(mem_ctx, DOM_SID);
			if (!*domain_sid) {
				return NT_STATUS_NO_MEMORY;
			}
			sid_copy(*domain_sid, &r.ctr.info.id3.dom_sid.sid);
		}

		break;

	case 5:
		
		if (domain_name && (r.ctr.info.id5.buffer_dom_name != 0)) {
			*domain_name = unistr2_tdup(mem_ctx, 
						   &r.ctr.info.id5.
						   uni_domain_name);
			if (!*domain_name) {
				return NT_STATUS_NO_MEMORY;
			}
		}
			
		if (domain_sid && (r.ctr.info.id5.buffer_dom_sid != 0)) {
			*domain_sid = TALLOC_P(mem_ctx, DOM_SID);
			if (!*domain_sid) {
				return NT_STATUS_NO_MEMORY;
			}
			sid_copy(*domain_sid, &r.ctr.info.id5.dom_sid.sid);
		}
		break;
			
	default:
		DEBUG(3, ("unknown info class %d\n", info_class));
		break;		      
	}
	
 done:

	return result;
}

/** Query info policy2
 *
 *  @param domain_name - returned remote server's domain name
 *  @param dns_name - returned remote server's dns domain name
 *  @param forest_name - returned remote server's forest name
 *  @param domain_guid - returned remote server's domain guid
 *  @param domain_sid - returned remote server's domain sid */

NTSTATUS rpccli_lsa_query_info_policy2(struct rpc_pipe_client *cli,
				       TALLOC_CTX *mem_ctx,
				       POLICY_HND *pol, uint16 info_class, 
				       char **domain_name, char **dns_name,
				       char **forest_name,
				       struct uuid **domain_guid,
				       DOM_SID **domain_sid)
{
	prs_struct qbuf, rbuf;
	LSA_Q_QUERY_INFO2 q;
	LSA_R_QUERY_INFO2 r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	if (info_class != 12)
		goto done;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	init_q_query2(&q, pol, info_class);

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_QUERYINFO2,
		q, r,
		qbuf, rbuf,
		lsa_io_q_query_info2,
		lsa_io_r_query_info2,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	/* Return output parameters */

	ZERO_STRUCTP(domain_guid);

	if (domain_name && r.ctr.info.id12.hdr_nb_dom_name.buffer) {
		*domain_name = unistr2_tdup(mem_ctx, 
					    &r.ctr.info.id12
					    .uni_nb_dom_name);
		if (!*domain_name) {
			return NT_STATUS_NO_MEMORY;
		}
	}
	if (dns_name && r.ctr.info.id12.hdr_dns_dom_name.buffer) {
		*dns_name = unistr2_tdup(mem_ctx, 
					 &r.ctr.info.id12
					 .uni_dns_dom_name);
		if (!*dns_name) {
			return NT_STATUS_NO_MEMORY;
		}
	}
	if (forest_name && r.ctr.info.id12.hdr_forest_name.buffer) {
		*forest_name = unistr2_tdup(mem_ctx, 
					    &r.ctr.info.id12
					    .uni_forest_name);
		if (!*forest_name) {
			return NT_STATUS_NO_MEMORY;
		}
	}
	
	if (domain_guid) {
		*domain_guid = TALLOC_P(mem_ctx, struct uuid);
		if (!*domain_guid) {
			return NT_STATUS_NO_MEMORY;
		}
		memcpy(*domain_guid, 
		       &r.ctr.info.id12.dom_guid, 
		       sizeof(struct uuid));
	}

	if (domain_sid && r.ctr.info.id12.ptr_dom_sid != 0) {
		*domain_sid = TALLOC_P(mem_ctx, DOM_SID);
		if (!*domain_sid) {
			return NT_STATUS_NO_MEMORY;
		}
		sid_copy(*domain_sid, 
			 &r.ctr.info.id12.dom_sid.sid);
	}
	
 done:

	return result;
}

NTSTATUS rpccli_lsa_set_info_policy(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				    POLICY_HND *pol, uint16 info_class,
				    LSA_INFO_CTR ctr) 
{
	prs_struct qbuf, rbuf;
	LSA_Q_SET_INFO q;
	LSA_R_SET_INFO r;
	NTSTATUS result;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	init_q_set(&q, pol, info_class, ctr);

	CLI_DO_RPC(cli, mem_ctx, PI_LSARPC, LSA_SETINFOPOLICY,
		q, r,
		qbuf, rbuf,
		lsa_io_q_set,
		lsa_io_r_set,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	/* Return output parameters */

 done:

	return result;
}


/**
 * Enumerate list of trusted domains
 *
 * @param cli client state (cli_state) structure of the connection
 * @param mem_ctx memory context
 * @param pol opened lsa policy handle
 * @param enum_ctx enumeration context ie. index of first returned domain entry
 * @param pref_num_domains preferred max number of entries returned in one response
 * @param num_domains total number of trusted domains returned by response
 * @param domain_names returned trusted domain names
 * @param domain_sids returned trusted domain sids
 *
 * @return nt status code of response
 **/

NTSTATUS rpccli_lsa_enum_trust_dom(struct rpc_pipe_client *cli,
				   TALLOC_CTX *mem_ctx,
				   POLICY_HND *pol, uint32 *enum_ctx, 
				   uint32 *num_domains,
				   char ***domain_names, DOM_SID **domain_sids)
{
	prs_struct qbuf, rbuf;
	LSA_Q_ENUM_TRUST_DOM in;
	LSA_R_ENUM_TRUST_DOM out;
	int i;
	fstring tmp;

	ZERO_STRUCT(in);
	ZERO_STRUCT(out);

	/* 64k is enough for about 2000 trusted domains */
	
        init_q_enum_trust_dom(&in, pol, *enum_ctx, 0x10000);

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_ENUMTRUSTDOM, 
	            in, out, 
	            qbuf, rbuf,
	            lsa_io_q_enum_trust_dom,
	            lsa_io_r_enum_trust_dom, 
	            NT_STATUS_UNSUCCESSFUL );


	/* check for an actual error */

	if ( !NT_STATUS_IS_OK(out.status) 
		&& !NT_STATUS_EQUAL(out.status, NT_STATUS_NO_MORE_ENTRIES) 
		&& !NT_STATUS_EQUAL(out.status, STATUS_MORE_ENTRIES) )
	{
		return out.status;
	}
		
	/* Return output parameters */

	*num_domains  = out.count;
	*enum_ctx     = out.enum_context;
	
	if ( out.count ) {

		/* Allocate memory for trusted domain names and sids */

		if ( !(*domain_names = TALLOC_ARRAY(mem_ctx, char *, out.count)) ) {
			DEBUG(0, ("cli_lsa_enum_trust_dom(): out of memory\n"));
			return NT_STATUS_NO_MEMORY;
		}

		if ( !(*domain_sids = TALLOC_ARRAY(mem_ctx, DOM_SID, out.count)) ) {
			DEBUG(0, ("cli_lsa_enum_trust_dom(): out of memory\n"));
			return NT_STATUS_NO_MEMORY;
		}

		/* Copy across names and sids */

		for (i = 0; i < out.count; i++) {

			rpcstr_pull( tmp, out.domlist->domains[i].name.string->buffer, 
				sizeof(tmp), out.domlist->domains[i].name.length, 0);
			(*domain_names)[i] = talloc_strdup(mem_ctx, tmp);

			sid_copy(&(*domain_sids)[i], &out.domlist->domains[i].sid->sid );
		}
	}

	return out.status;
}

/** Enumerate privileges*/

NTSTATUS rpccli_lsa_enum_privilege(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                                POLICY_HND *pol, uint32 *enum_context, uint32 pref_max_length,
				uint32 *count, char ***privs_name, uint32 **privs_high, uint32 **privs_low)
{
	prs_struct qbuf, rbuf;
	LSA_Q_ENUM_PRIVS q;
	LSA_R_ENUM_PRIVS r;
	NTSTATUS result;
	int i;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	init_q_enum_privs(&q, pol, *enum_context, pref_max_length);

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_ENUM_PRIVS,
		q, r,
		qbuf, rbuf,
		lsa_io_q_enum_privs,
		lsa_io_r_enum_privs,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	/* Return output parameters */

	*enum_context = r.enum_context;
	*count = r.count;

	if (!((*privs_name = TALLOC_ARRAY(mem_ctx, char *, r.count)))) {
		DEBUG(0, ("(cli_lsa_enum_privilege): out of memory\n"));
		result = NT_STATUS_UNSUCCESSFUL;
		goto done;
	}

	if (!((*privs_high = TALLOC_ARRAY(mem_ctx, uint32, r.count)))) {
		DEBUG(0, ("(cli_lsa_enum_privilege): out of memory\n"));
		result = NT_STATUS_UNSUCCESSFUL;
		goto done;
	}

	if (!((*privs_low = TALLOC_ARRAY(mem_ctx, uint32, r.count)))) {
		DEBUG(0, ("(cli_lsa_enum_privilege): out of memory\n"));
		result = NT_STATUS_UNSUCCESSFUL;
		goto done;
	}

	for (i = 0; i < r.count; i++) {
		fstring name;

		rpcstr_pull_unistr2_fstring( name, &r.privs[i].name);

		(*privs_name)[i] = talloc_strdup(mem_ctx, name);

		(*privs_high)[i] = r.privs[i].luid_high;
		(*privs_low)[i] = r.privs[i].luid_low;
	}

 done:

	return result;
}

/** Get privilege name */

NTSTATUS rpccli_lsa_get_dispname(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
			      POLICY_HND *pol, const char *name, 
			      uint16 lang_id, uint16 lang_id_sys,
			      fstring description, uint16 *lang_id_desc)
{
	prs_struct qbuf, rbuf;
	LSA_Q_PRIV_GET_DISPNAME q;
	LSA_R_PRIV_GET_DISPNAME r;
	NTSTATUS result;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	init_lsa_priv_get_dispname(&q, pol, name, lang_id, lang_id_sys);

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_PRIV_GET_DISPNAME,
		q, r,
		qbuf, rbuf,
		lsa_io_q_priv_get_dispname,
		lsa_io_r_priv_get_dispname,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	/* Return output parameters */
	
	rpcstr_pull_unistr2_fstring(description , &r.desc);
	*lang_id_desc = r.lang_id;

 done:

	return result;
}

/** Enumerate list of SIDs  */

NTSTATUS rpccli_lsa_enum_sids(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                                POLICY_HND *pol, uint32 *enum_ctx, uint32 pref_max_length, 
                                uint32 *num_sids, DOM_SID **sids)
{
	prs_struct qbuf, rbuf;
	LSA_Q_ENUM_ACCOUNTS q;
	LSA_R_ENUM_ACCOUNTS r;
	NTSTATUS result;
	int i;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

        init_lsa_q_enum_accounts(&q, pol, *enum_ctx, pref_max_length);

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_ENUM_ACCOUNTS,
		q, r,
		qbuf, rbuf,
		lsa_io_q_enum_accounts,
		lsa_io_r_enum_accounts,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	if (r.sids.num_entries==0)
		goto done;

	/* Return output parameters */

	*sids = TALLOC_ARRAY(mem_ctx, DOM_SID, r.sids.num_entries);
	if (!*sids) {
		DEBUG(0, ("(cli_lsa_enum_sids): out of memory\n"));
		result = NT_STATUS_UNSUCCESSFUL;
		goto done;
	}

	/* Copy across names and sids */

	for (i = 0; i < r.sids.num_entries; i++) {
		sid_copy(&(*sids)[i], &r.sids.sid[i].sid);
	}

	*num_sids= r.sids.num_entries;
	*enum_ctx = r.enum_context;

 done:

	return result;
}

/** Create a LSA user handle
 *
 * @param cli Handle on an initialised SMB connection
 *
 * FIXME: The code is actually identical to open account
 * TODO: Check and code what the function should exactly do
 *
 * */

NTSTATUS rpccli_lsa_create_account(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                             POLICY_HND *dom_pol, DOM_SID *sid, uint32 desired_access, 
			     POLICY_HND *user_pol)
{
	prs_struct qbuf, rbuf;
	LSA_Q_CREATEACCOUNT q;
	LSA_R_CREATEACCOUNT r;
	NTSTATUS result;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Initialise input parameters */

	init_lsa_q_create_account(&q, dom_pol, sid, desired_access);

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_CREATEACCOUNT,
		q, r,
		qbuf, rbuf,
		lsa_io_q_create_account,
		lsa_io_r_create_account,
		NT_STATUS_UNSUCCESSFUL);

	/* Return output parameters */

	result = r.status;

	if (NT_STATUS_IS_OK(result)) {
		*user_pol = r.pol;
	}

	return result;
}

/** Open a LSA user handle
 *
 * @param cli Handle on an initialised SMB connection */

NTSTATUS rpccli_lsa_open_account(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                             POLICY_HND *dom_pol, DOM_SID *sid, uint32 des_access, 
			     POLICY_HND *user_pol)
{
	prs_struct qbuf, rbuf;
	LSA_Q_OPENACCOUNT q;
	LSA_R_OPENACCOUNT r;
	NTSTATUS result;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Initialise input parameters */

	init_lsa_q_open_account(&q, dom_pol, sid, des_access);

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_OPENACCOUNT,
		q, r,
		qbuf, rbuf,
		lsa_io_q_open_account,
		lsa_io_r_open_account,
		NT_STATUS_UNSUCCESSFUL);

	/* Return output parameters */

	result = r.status;

	if (NT_STATUS_IS_OK(result)) {
		*user_pol = r.pol;
	}

	return result;
}

/** Enumerate user privileges
 *
 * @param cli Handle on an initialised SMB connection */

NTSTATUS rpccli_lsa_enum_privsaccount(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                             POLICY_HND *pol, uint32 *count, LUID_ATTR **set)
{
	prs_struct qbuf, rbuf;
	LSA_Q_ENUMPRIVSACCOUNT q;
	LSA_R_ENUMPRIVSACCOUNT r;
	NTSTATUS result;
	int i;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Initialise input parameters */

	init_lsa_q_enum_privsaccount(&q, pol);

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_ENUMPRIVSACCOUNT,
		q, r,
		qbuf, rbuf,
		lsa_io_q_enum_privsaccount,
		lsa_io_r_enum_privsaccount,
		NT_STATUS_UNSUCCESSFUL);

	/* Return output parameters */

	result = r.status;

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	if (r.count == 0)
		goto done;

	if (!((*set = TALLOC_ARRAY(mem_ctx, LUID_ATTR, r.count)))) {
		DEBUG(0, ("(cli_lsa_enum_privsaccount): out of memory\n"));
		result = NT_STATUS_UNSUCCESSFUL;
		goto done;
	}

	for (i=0; i<r.count; i++) {
		(*set)[i].luid.low = r.set.set[i].luid.low;
		(*set)[i].luid.high = r.set.set[i].luid.high;
		(*set)[i].attr = r.set.set[i].attr;
	}

	*count=r.count;
 done:

	return result;
}

/** Get a privilege value given its name */

NTSTATUS rpccli_lsa_lookup_priv_value(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				 POLICY_HND *pol, const char *name, LUID *luid)
{
	prs_struct qbuf, rbuf;
	LSA_Q_LOOKUP_PRIV_VALUE q;
	LSA_R_LOOKUP_PRIV_VALUE r;
	NTSTATUS result;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_lsa_q_lookup_priv_value(&q, pol, name);

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_LOOKUPPRIVVALUE,
		q, r,
		qbuf, rbuf,
		lsa_io_q_lookup_priv_value,
		lsa_io_r_lookup_priv_value,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	/* Return output parameters */

	(*luid).low=r.luid.low;
	(*luid).high=r.luid.high;

 done:

	return result;
}

/** Query LSA security object */

NTSTATUS rpccli_lsa_query_secobj(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
			      POLICY_HND *pol, uint32 sec_info, 
			      SEC_DESC_BUF **psdb)
{
	prs_struct qbuf, rbuf;
	LSA_Q_QUERY_SEC_OBJ q;
	LSA_R_QUERY_SEC_OBJ r;
	NTSTATUS result;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_q_query_sec_obj(&q, pol, sec_info);

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_QUERYSECOBJ,
		q, r,
		qbuf, rbuf,
		lsa_io_q_query_sec_obj,
		lsa_io_r_query_sec_obj,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	/* Return output parameters */

	if (psdb)
		*psdb = r.buf;

 done:

	return result;
}


/* Enumerate account rights This is similar to enum_privileges but
   takes a SID directly, avoiding the open_account call.
*/

NTSTATUS rpccli_lsa_enum_account_rights(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				     POLICY_HND *pol, DOM_SID *sid,
				     uint32 *count, char ***priv_names)
{
	prs_struct qbuf, rbuf;
	LSA_Q_ENUM_ACCT_RIGHTS q;
	LSA_R_ENUM_ACCT_RIGHTS r;
	NTSTATUS result;
	int i;
	fstring *privileges;
	char **names;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */
	init_q_enum_acct_rights(&q, pol, 2, sid);

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_ENUMACCTRIGHTS,
		q, r,
		qbuf, rbuf,
		lsa_io_q_enum_acct_rights,
		lsa_io_r_enum_acct_rights,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	*count = r.count;
	if (! *count) {
		goto done;
	}

	
	privileges = TALLOC_ARRAY( mem_ctx, fstring, *count );
	names      = TALLOC_ARRAY( mem_ctx, char *, *count );

	if ((privileges == NULL) || (names == NULL)) {
		TALLOC_FREE(privileges);
		TALLOC_FREE(names);
		return NT_STATUS_NO_MEMORY;
	}

	for ( i=0; i<*count; i++ ) {
		UNISTR4 *uni_string = &r.rights->strings[i];

		if ( !uni_string->string )
			continue;

		rpcstr_pull( privileges[i], uni_string->string->buffer, sizeof(privileges[i]), -1, STR_TERMINATE );
			
		/* now copy to the return array */
		names[i] = talloc_strdup( mem_ctx, privileges[i] );
	}
	
	*priv_names = names;

done:

	return result;
}



/* add account rights to an account. */

NTSTATUS rpccli_lsa_add_account_rights(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				    POLICY_HND *pol, DOM_SID sid,
					uint32 count, const char **privs_name)
{
	prs_struct qbuf, rbuf;
	LSA_Q_ADD_ACCT_RIGHTS q;
	LSA_R_ADD_ACCT_RIGHTS r;
	NTSTATUS result;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */
	init_q_add_acct_rights(&q, pol, &sid, count, privs_name);

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_ADDACCTRIGHTS,
		q, r,
		qbuf, rbuf,
		lsa_io_q_add_acct_rights,
		lsa_io_r_add_acct_rights,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}
done:

	return result;
}


/* remove account rights for an account. */

NTSTATUS rpccli_lsa_remove_account_rights(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				       POLICY_HND *pol, DOM_SID sid, BOOL removeall,
				       uint32 count, const char **privs_name)
{
	prs_struct qbuf, rbuf;
	LSA_Q_REMOVE_ACCT_RIGHTS q;
	LSA_R_REMOVE_ACCT_RIGHTS r;
	NTSTATUS result;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */
	init_q_remove_acct_rights(&q, pol, &sid, removeall?1:0, count, privs_name);

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_REMOVEACCTRIGHTS,
		q, r,
		qbuf, rbuf,
		lsa_io_q_remove_acct_rights,
		lsa_io_r_remove_acct_rights,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}
done:

	return result;
}


#if 0

/** An example of how to use the routines in this file.  Fetch a DOMAIN
    sid. Does complete cli setup / teardown anonymously. */

BOOL fetch_domain_sid( char *domain, char *remote_machine, DOM_SID *psid)
{
	extern pstring global_myname;
	struct cli_state cli;
	NTSTATUS result;
	POLICY_HND lsa_pol;
	BOOL ret = False;
 
	ZERO_STRUCT(cli);
	if(cli_initialise(&cli) == False) {
		DEBUG(0,("fetch_domain_sid: unable to initialize client connection.\n"));
		return False;
	}
 
	if(!resolve_name( remote_machine, &cli.dest_ip, 0x20)) {
		DEBUG(0,("fetch_domain_sid: Can't resolve address for %s\n", remote_machine));
		goto done;
	}
 
	if (!cli_connect(&cli, remote_machine, &cli.dest_ip)) {
		DEBUG(0,("fetch_domain_sid: unable to connect to SMB server on \
machine %s. Error was : %s.\n", remote_machine, cli_errstr(&cli) ));
		goto done;
	}

	if (!attempt_netbios_session_request(&cli, global_myname, remote_machine, &cli.dest_ip)) {
		DEBUG(0,("fetch_domain_sid: machine %s rejected the NetBIOS session request.\n", 
			remote_machine));
		goto done;
	}
 
	cli.protocol = PROTOCOL_NT1;
 
	if (!cli_negprot(&cli)) {
		DEBUG(0,("fetch_domain_sid: machine %s rejected the negotiate protocol. \
Error was : %s.\n", remote_machine, cli_errstr(&cli) ));
		goto done;
	}
 
	if (cli.protocol != PROTOCOL_NT1) {
		DEBUG(0,("fetch_domain_sid: machine %s didn't negotiate NT protocol.\n",
			remote_machine));
		goto done;
	}
 
	/*
	 * Do an anonymous session setup.
	 */
 
	if (!cli_session_setup(&cli, "", "", 0, "", 0, "")) {
		DEBUG(0,("fetch_domain_sid: machine %s rejected the session setup. \
Error was : %s.\n", remote_machine, cli_errstr(&cli) ));
		goto done;
	}
 
	if (!(cli.sec_mode & NEGOTIATE_SECURITY_USER_LEVEL)) {
		DEBUG(0,("fetch_domain_sid: machine %s isn't in user level security mode\n",
			remote_machine));
		goto done;
	}

	if (!cli_send_tconX(&cli, "IPC$", "IPC", "", 1)) {
		DEBUG(0,("fetch_domain_sid: machine %s rejected the tconX on the IPC$ share. \
Error was : %s.\n", remote_machine, cli_errstr(&cli) ));
		goto done;
	}

	/* Fetch domain sid */
 
	if (!cli_nt_session_open(&cli, PI_LSARPC)) {
		DEBUG(0, ("fetch_domain_sid: Error connecting to SAM pipe\n"));
		goto done;
	}
 
	result = cli_lsa_open_policy(&cli, cli.mem_ctx, True, SEC_RIGHTS_QUERY_VALUE, &lsa_pol);
	if (!NT_STATUS_IS_OK(result)) {
		DEBUG(0, ("fetch_domain_sid: Error opening lsa policy handle. %s\n",
			nt_errstr(result) ));
		goto done;
	}
 
	result = cli_lsa_query_info_policy(&cli, cli.mem_ctx, &lsa_pol, 5, domain, psid);
	if (!NT_STATUS_IS_OK(result)) {
		DEBUG(0, ("fetch_domain_sid: Error querying lsa policy handle. %s\n",
			nt_errstr(result) ));
		goto done;
	}
 
	ret = True;

  done:

	cli_shutdown(&cli);
	return ret;
}

#endif

NTSTATUS rpccli_lsa_open_trusted_domain(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				     POLICY_HND *pol, DOM_SID *dom_sid, uint32 access_mask,
				     POLICY_HND *trustdom_pol)
{
	prs_struct qbuf, rbuf;
	LSA_Q_OPEN_TRUSTED_DOMAIN q;
	LSA_R_OPEN_TRUSTED_DOMAIN r;
	NTSTATUS result;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Initialise input parameters */

	init_lsa_q_open_trusted_domain(&q, pol, dom_sid, access_mask);

	/* Marshall data and send request */

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_OPENTRUSTDOM,
		q, r,
		qbuf, rbuf,
		lsa_io_q_open_trusted_domain,
		lsa_io_r_open_trusted_domain,
		NT_STATUS_UNSUCCESSFUL);

	/* Return output parameters */
	
	result = r.status;

	if (NT_STATUS_IS_OK(result)) {
		*trustdom_pol = r.handle;
	}

	return result;
}

NTSTATUS rpccli_lsa_query_trusted_domain_info(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
					   POLICY_HND *pol, 
					   uint16 info_class,  
					   LSA_TRUSTED_DOMAIN_INFO **info)
{
	prs_struct qbuf, rbuf;
	LSA_Q_QUERY_TRUSTED_DOMAIN_INFO q;
	LSA_R_QUERY_TRUSTED_DOMAIN_INFO r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_q_query_trusted_domain_info(&q, pol, info_class); 

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_QUERYTRUSTDOMINFO,
		q, r,
		qbuf, rbuf,
		lsa_io_q_query_trusted_domain_info,
		lsa_io_r_query_trusted_domain_info,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	*info = r.info;
		
done:
	return result;
}

NTSTATUS rpccli_lsa_open_trusted_domain_by_name(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
						POLICY_HND *pol, const char *name, uint32 access_mask,
						POLICY_HND *trustdom_pol)
{
	prs_struct qbuf, rbuf;
	LSA_Q_OPEN_TRUSTED_DOMAIN_BY_NAME q;
	LSA_R_OPEN_TRUSTED_DOMAIN_BY_NAME r;
	NTSTATUS result;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Initialise input parameters */

	init_lsa_q_open_trusted_domain_by_name(&q, pol, name, access_mask);

	/* Marshall data and send request */

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_OPENTRUSTDOMBYNAME,
		q, r,
		qbuf, rbuf,
		lsa_io_q_open_trusted_domain_by_name,
		lsa_io_r_open_trusted_domain_by_name,
		NT_STATUS_UNSUCCESSFUL);

	/* Return output parameters */
	
	result = r.status;

	if (NT_STATUS_IS_OK(result)) {
		*trustdom_pol = r.handle;
	}

	return result;
}


NTSTATUS rpccli_lsa_query_trusted_domain_info_by_sid(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
						  POLICY_HND *pol, 
						  uint16 info_class, DOM_SID *dom_sid, 
						  LSA_TRUSTED_DOMAIN_INFO **info)
{
	prs_struct qbuf, rbuf;
	LSA_Q_QUERY_TRUSTED_DOMAIN_INFO_BY_SID q;
	LSA_R_QUERY_TRUSTED_DOMAIN_INFO r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_q_query_trusted_domain_info_by_sid(&q, pol, info_class, dom_sid); 

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_QUERYTRUSTDOMINFOBYSID,
		q, r,
		qbuf, rbuf,
		lsa_io_q_query_trusted_domain_info_by_sid,
		lsa_io_r_query_trusted_domain_info,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	*info = r.info;

done:

	return result;
}

NTSTATUS rpccli_lsa_query_trusted_domain_info_by_name(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
						   POLICY_HND *pol, 
						   uint16 info_class, const char *domain_name, 
						   LSA_TRUSTED_DOMAIN_INFO **info)
{
	prs_struct qbuf, rbuf;
	LSA_Q_QUERY_TRUSTED_DOMAIN_INFO_BY_NAME q;
	LSA_R_QUERY_TRUSTED_DOMAIN_INFO r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_q_query_trusted_domain_info_by_name(&q, pol, info_class, domain_name); 

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_QUERYTRUSTDOMINFOBYNAME,
		q, r,
		qbuf, rbuf,
		lsa_io_q_query_trusted_domain_info_by_name,
		lsa_io_r_query_trusted_domain_info,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	*info = r.info;

done:
	
	return result;
}

NTSTATUS cli_lsa_query_domain_info_policy(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
					  POLICY_HND *pol, 
					  uint16 info_class, LSA_DOM_INFO_UNION **info)
{
	prs_struct qbuf, rbuf;
	LSA_Q_QUERY_DOM_INFO_POLICY q;
	LSA_R_QUERY_DOM_INFO_POLICY r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

	init_q_query_dom_info(&q, pol, info_class); 

	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC, LSA_QUERYDOMINFOPOL, 
		q, r,
		qbuf, rbuf,
		lsa_io_q_query_dom_info,
		lsa_io_r_query_dom_info,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	*info = r.info;

done:
	return result;
}

