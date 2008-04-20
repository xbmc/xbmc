/* 
   Unix SMB/CIFS implementation.
   RPC Pipe client
 
   Copyright (C) Andrew Tridgell              1992-1998,
   Largely rewritten by Jeremy Allison (C)	   2005.
   Copyright (C) Jim McDonough (jmcd@us.ibm.com)   2003.
   
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

/* Shutdown a server */

NTSTATUS rpccli_shutdown_init(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
			   const char *msg, uint32 timeout, BOOL do_reboot,
			   BOOL force)
{
	prs_struct qbuf;
	prs_struct rbuf; 
	SHUTDOWN_Q_INIT q;
	SHUTDOWN_R_INIT r;
	WERROR result = WERR_GENERAL_FAILURE;

	if (msg == NULL) 
		return NT_STATUS_INVALID_PARAMETER;

	ZERO_STRUCT (q);
	ZERO_STRUCT (r);

	/* Marshall data and send request */

	init_shutdown_q_init(&q, msg, timeout, do_reboot, force);

	CLI_DO_RPC(cli, mem_ctx, PI_SHUTDOWN, SHUTDOWN_INIT,
		q, r,
		qbuf, rbuf,
		shutdown_io_q_init,
		shutdown_io_r_init,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;
	return werror_to_ntstatus(result);
}

/* Shutdown a server */

NTSTATUS rpccli_shutdown_init_ex(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
			   const char *msg, uint32 timeout, BOOL do_reboot,
			   BOOL force, uint32 reason)
{
	prs_struct qbuf;
	prs_struct rbuf; 
	SHUTDOWN_Q_INIT_EX q;
	SHUTDOWN_R_INIT_EX r;
	WERROR result = WERR_GENERAL_FAILURE;

	if (msg == NULL) 
		return NT_STATUS_INVALID_PARAMETER;

	ZERO_STRUCT (q);
	ZERO_STRUCT (r);

	/* Marshall data and send request */

	init_shutdown_q_init_ex(&q, msg, timeout, do_reboot, force, reason);

	CLI_DO_RPC(cli, mem_ctx, PI_SHUTDOWN, SHUTDOWN_INIT_EX,
		q, r,
		qbuf, rbuf,
		shutdown_io_q_init_ex,
		shutdown_io_r_init_ex,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;
	return werror_to_ntstatus(result);
}


/* Abort a server shutdown */

NTSTATUS rpccli_shutdown_abort(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx)
{
	prs_struct rbuf;
	prs_struct qbuf; 
	SHUTDOWN_Q_ABORT q;
	SHUTDOWN_R_ABORT r;
	WERROR result = WERR_GENERAL_FAILURE;

	ZERO_STRUCT (q);
	ZERO_STRUCT (r);

	/* Marshall data and send request */

	init_shutdown_q_abort(&q);

	CLI_DO_RPC(cli, mem_ctx, PI_SHUTDOWN, SHUTDOWN_ABORT,
		q, r,
		qbuf, rbuf,
		shutdown_io_q_abort,
		shutdown_io_r_abort,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;
	return werror_to_ntstatus(result);
}
