/* 
   Unix SMB/CIFS implementation.
   RPC pipe client

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
#include "rpcclient.h"

static NTSTATUS cmd_testme(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
			   int argc, const char **argv)
{
	struct rpc_pipe_client *lsa_pipe = NULL, *samr_pipe = NULL;
	NTSTATUS status = NT_STATUS_UNSUCCESSFUL;
	POLICY_HND pol;

	d_printf("testme\n");

	lsa_pipe = cli_rpc_pipe_open_noauth(cli->cli, PI_LSARPC, &status);
	if (lsa_pipe == NULL) goto done;

	samr_pipe = cli_rpc_pipe_open_noauth(cli->cli, PI_SAMR, &status);
	if (samr_pipe == NULL) goto done;

	status = rpccli_lsa_open_policy(lsa_pipe, mem_ctx, False,
					SEC_RIGHTS_QUERY_VALUE, &pol);

	if (!NT_STATUS_IS_OK(status))
		goto done;

	status = rpccli_lsa_close(lsa_pipe, mem_ctx, &pol);

	if (!NT_STATUS_IS_OK(status))
		goto done;

 done:
	if (lsa_pipe != NULL) cli_rpc_pipe_close(lsa_pipe);
	if (samr_pipe != NULL) cli_rpc_pipe_close(samr_pipe);

	return status;
}

/* List of commands exported by this module */

struct cmd_set test_commands[] = {

	{ "TESTING" },

	{ "testme", RPC_RTYPE_NTSTATUS, cmd_testme, NULL,
	  -1, NULL, "Sample test", "testme" },

	{ NULL }
};
