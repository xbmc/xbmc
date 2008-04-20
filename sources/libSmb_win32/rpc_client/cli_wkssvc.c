/* 
   Unix SMB/CIFS implementation.
   NT Domain Authentication SMB / MSRPC client
   Copyright (C) Andrew Tridgell 1994-2000
   Copyright (C) Tim Potter 2001
   Copyright (C) Rafal Szczesniak 2002
   Copyright (C) Jeremy Allison 2005.

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

/**
 * WksQueryInfo rpc call (like query for server's capabilities)
 *
 * @param initialised client structure with \PIPE\wkssvc opened
 * @param mem_ctx memory context assigned to this rpc binding
 * @param wks100 WksQueryInfo structure
 *
 * @return NTSTATUS of rpc call
 */
 
NTSTATUS rpccli_wks_query_info(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
			    WKS_INFO_100 *wks100)
{
	prs_struct qbuf;
	prs_struct rbuf;
	WKS_Q_QUERY_INFO q;
	WKS_R_QUERY_INFO r;

	if (cli == NULL || wks100 == NULL)
		return NT_STATUS_UNSUCCESSFUL;

	DEBUG(4, ("WksQueryInfo\n"));
	
	/* init query structure with rpc call arguments */
	init_wks_q_query_info(&q, cli->cli->desthost, 100);
	r.wks100 = wks100;

	CLI_DO_RPC(cli, mem_ctx, PI_WKSSVC, WKS_QUERY_INFO,
		q, r,
		qbuf, rbuf,
		wks_io_q_query_info,
		wks_io_r_query_info,
		NT_STATUS_UNSUCCESSFUL);
		
	/* check returnet status code */
	if (NT_STATUS_IS_ERR(r.status)) {
		/* report the error */
		DEBUG(0,("WKS_R_QUERY_INFO: %s\n", nt_errstr(r.status)));
		return r.status;
	}
	
	return NT_STATUS_OK;
}
