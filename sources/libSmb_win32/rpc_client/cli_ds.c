/* 
   Unix SMB/CIFS implementation.
   RPC pipe client
   Copyright (C) Gerald Carter                        2002,
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

/* implementations of client side DsXXX() functions */

/********************************************************************
 Get information about the server and directory services
********************************************************************/

NTSTATUS rpccli_ds_getprimarydominfo(struct rpc_pipe_client *cli,
				     TALLOC_CTX *mem_ctx, 
				     uint16 level, DS_DOMINFO_CTR *ctr)
{
	prs_struct qbuf, rbuf;
	DS_Q_GETPRIMDOMINFO q;
	DS_R_GETPRIMDOMINFO r;
	NTSTATUS result;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	q.level = level;
	
	CLI_DO_RPC( cli, mem_ctx, PI_LSARPC_DS, DS_GETPRIMDOMINFO,
		q, r,
		qbuf, rbuf,
		ds_io_q_getprimdominfo,
		ds_io_r_getprimdominfo,
		NT_STATUS_UNSUCCESSFUL);
	
	/* Return basic info - if we are requesting at info != 1 then
	   there could be trouble. */ 

	result = r.status;

	if ( r.ptr && ctr ) {
		ctr->basic = TALLOC_P(mem_ctx, DSROLE_PRIMARY_DOMAIN_INFO_BASIC);
		if (!ctr->basic)
			goto done;
		memcpy(ctr->basic, r.info.basic, sizeof(DSROLE_PRIMARY_DOMAIN_INFO_BASIC));
	}
	
done:

	return result;
}

/********************************************************************
 Enumerate trusted domains in an AD forest
********************************************************************/

NTSTATUS rpccli_ds_enum_domain_trusts(struct rpc_pipe_client *cli,
				      TALLOC_CTX *mem_ctx, 
				      const char *server, uint32 flags, 
				      struct ds_domain_trust **trusts,
				      uint32 *num_domains)
{
	prs_struct qbuf, rbuf;
	DS_Q_ENUM_DOM_TRUSTS q;
	DS_R_ENUM_DOM_TRUSTS r;
	NTSTATUS result;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	init_q_ds_enum_domain_trusts( &q, server, flags );
		
	CLI_DO_RPC( cli, mem_ctx, PI_NETLOGON, DS_ENUM_DOM_TRUSTS,
		q, r,
		qbuf, rbuf,
		ds_io_q_enum_domain_trusts,
		ds_io_r_enum_domain_trusts,
		NT_STATUS_UNSUCCESSFUL);
	
	result = r.status;
	
	if ( NT_STATUS_IS_OK(result) ) {
		int i;
	
		*num_domains = r.num_domains;
		*trusts = TALLOC_ARRAY(mem_ctx, struct ds_domain_trust, r.num_domains);

		if (*trusts == NULL) {
			return NT_STATUS_NO_MEMORY;
		}

		for ( i=0; i< *num_domains; i++ ) {
			(*trusts)[i].flags = r.domains.trusts[i].flags;
			(*trusts)[i].parent_index = r.domains.trusts[i].parent_index;
			(*trusts)[i].trust_type = r.domains.trusts[i].trust_type;
			(*trusts)[i].trust_attributes = r.domains.trusts[i].trust_attributes;
			(*trusts)[i].guid = r.domains.trusts[i].guid;

			if (r.domains.trusts[i].sid_ptr) {
				sid_copy(&(*trusts)[i].sid, &r.domains.trusts[i].sid.sid);
			} else {
				ZERO_STRUCT((*trusts)[i].sid);
			}

			if (r.domains.trusts[i].netbios_ptr) {
				(*trusts)[i].netbios_domain = unistr2_tdup( mem_ctx, &r.domains.trusts[i].netbios_domain );
			} else {
				(*trusts)[i].netbios_domain = NULL;
			}

			if (r.domains.trusts[i].dns_ptr) {
				(*trusts)[i].dns_domain = unistr2_tdup( mem_ctx, &r.domains.trusts[i].dns_domain );
			} else {
				(*trusts)[i].dns_domain = NULL;
			}
		}
	}
	
	return result;
}
