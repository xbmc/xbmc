/* 
   Unix SMB/CIFS implementation.

   RPC pipe client

   Copyright (C) Tim Potter 2003
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

NTSTATUS rpccli_echo_add_one(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
			  uint32 request, uint32 *response)
{
	prs_struct qbuf, rbuf;
	ECHO_Q_ADD_ONE q;
	ECHO_R_ADD_ONE r;
	BOOL result = False;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

        init_echo_q_add_one(&q, request);

	CLI_DO_RPC( cli, mem_ctx, PI_ECHO, ECHO_ADD_ONE,
			q, r,
			qbuf, rbuf,
			echo_io_q_add_one,
			echo_io_r_add_one,
			NT_STATUS_UNSUCCESSFUL);

	if (response)
		*response = r.response;

	result = True;

	return result ? NT_STATUS_OK : NT_STATUS_UNSUCCESSFUL;
}

NTSTATUS rpccli_echo_data(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
		       uint32 size, char *in_data, char **out_data)
{
	prs_struct qbuf, rbuf;
	ECHO_Q_ECHO_DATA q;
	ECHO_R_ECHO_DATA r;
	BOOL result = False;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

        init_echo_q_echo_data(&q, size, in_data);

	CLI_DO_RPC( cli, mem_ctx, PI_ECHO, ECHO_DATA,
			q, r,
			qbuf, rbuf,
			echo_io_q_echo_data,
			echo_io_r_echo_data,
			NT_STATUS_UNSUCCESSFUL);

	result = True;

	if (out_data) {
		*out_data = TALLOC(mem_ctx, size);
		if (!*out_data) {
			return NT_STATUS_NO_MEMORY;
		}
		memcpy(*out_data, r.data, size);
	}

	return result ? NT_STATUS_OK : NT_STATUS_UNSUCCESSFUL;
}

NTSTATUS rpccli_echo_sink_data(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
			    uint32 size, char *in_data)
{
	prs_struct qbuf, rbuf;
	ECHO_Q_SINK_DATA q;
	ECHO_R_SINK_DATA r;
	BOOL result = False;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

        init_echo_q_sink_data(&q, size, in_data);

	CLI_DO_RPC( cli, mem_ctx, PI_ECHO, ECHO_SINK_DATA,
			q, r,
			qbuf, rbuf,
			echo_io_q_sink_data,
			echo_io_r_sink_data,
			NT_STATUS_UNSUCCESSFUL);

	result = True;

	return result ? NT_STATUS_OK : NT_STATUS_UNSUCCESSFUL;
}

NTSTATUS rpccli_echo_source_data(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
			      uint32 size, char **out_data)
{
	prs_struct qbuf, rbuf;
	ECHO_Q_SOURCE_DATA q;
	ECHO_R_SOURCE_DATA r;
	BOOL result = False;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Marshall data and send request */

        init_echo_q_source_data(&q, size);

	CLI_DO_RPC( cli, mem_ctx, PI_ECHO, ECHO_SOURCE_DATA,
			q, r,
			qbuf, rbuf,
			echo_io_q_source_data,
			echo_io_r_source_data,
			NT_STATUS_UNSUCCESSFUL);

	result = True;

	return result ? NT_STATUS_OK : NT_STATUS_UNSUCCESSFUL;
}
