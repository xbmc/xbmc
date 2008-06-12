/* 
   Unix SMB/CIFS implementation.
   RPC pipe client

   Copyright (C) Tim Potter 2003

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
#include "rpcclient.h"

static NTSTATUS cmd_echo_add_one(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				 int argc, const char **argv)
{
	uint32 request = 1, response;
	NTSTATUS result;

	if (argc > 2) {
		printf("Usage: %s [num]\n", argv[0]);
		return NT_STATUS_OK;
	}

	if (argc == 2)
		request = atoi(argv[1]);

	result = rpccli_echo_add_one(cli, mem_ctx, request, &response);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	printf("%d + 1 = %d\n", request, response);

done:
	return result;
}

static NTSTATUS cmd_echo_data(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
			      int argc, const char **argv)
{
	uint32 size, i;
	NTSTATUS result;
	char *in_data = NULL, *out_data = NULL;

	if (argc != 2) {
		printf("Usage: %s num\n", argv[0]);
		return NT_STATUS_OK;
	}

	size = atoi(argv[1]);
	in_data = SMB_MALLOC(size);

	for (i = 0; i < size; i++)
		in_data[i] = i & 0xff;

	result = rpccli_echo_data(cli, mem_ctx, size, in_data, &out_data);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	for (i = 0; i < size; i++) {
		if (in_data[i] != out_data[i]) {
			printf("mismatch at offset %d, %d != %d\n",
			       i, in_data[i], out_data[i]);
			result = NT_STATUS_UNSUCCESSFUL;
		}
	}

done:
	SAFE_FREE(in_data);

	return result;
}

static NTSTATUS cmd_echo_source_data(struct rpc_pipe_client *cli, 
				     TALLOC_CTX *mem_ctx, int argc, 
				     const char **argv)
{
	uint32 size, i;
	NTSTATUS result;
	char *out_data = NULL;

	if (argc != 2) {
		printf("Usage: %s num\n", argv[0]);
		return NT_STATUS_OK;
	}

	size = atoi(argv[1]);

	result = rpccli_echo_source_data(cli, mem_ctx, size, &out_data);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	for (i = 0; i < size; i++) {
		if (out_data && out_data[i] != (i & 0xff)) {
			printf("mismatch at offset %d, %d != %d\n",
			       i, out_data[i], i & 0xff);
			result = NT_STATUS_UNSUCCESSFUL;
		}
	}

done:
	return result;
}

static NTSTATUS cmd_echo_sink_data(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				   int argc, const char **argv)
{
	uint32 size, i;
	NTSTATUS result;
	char *in_data = NULL;

	if (argc != 2) {
		printf("Usage: %s num\n", argv[0]);
		return NT_STATUS_OK;
	}

	size = atoi(argv[1]);
	in_data = SMB_MALLOC(size);

	for (i = 0; i < size; i++)
		in_data[i] = i & 0xff;

	result = rpccli_echo_sink_data(cli, mem_ctx, size, in_data);

	if (!NT_STATUS_IS_OK(result))
		goto done;

done:
	SAFE_FREE(in_data);

	return result;
}

/* List of commands exported by this module */

struct cmd_set echo_commands[] = {

	{ "ECHO" },

	{ "echoaddone", RPC_RTYPE_NTSTATUS, cmd_echo_add_one,     NULL, PI_ECHO, NULL, "Add one to a number", "" },
	{ "echodata",   RPC_RTYPE_NTSTATUS, cmd_echo_data,        NULL, PI_ECHO, NULL, "Echo data",           "" },
	{ "sinkdata",   RPC_RTYPE_NTSTATUS, cmd_echo_sink_data,   NULL, PI_ECHO, NULL, "Sink data",           "" },
	{ "sourcedata", RPC_RTYPE_NTSTATUS, cmd_echo_source_data, NULL, PI_ECHO, NULL, "Source data",         "" },
	{ NULL }
};
