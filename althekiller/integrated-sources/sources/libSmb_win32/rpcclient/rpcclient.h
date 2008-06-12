/* 
   Unix SMB/CIFS implementation.
   RPC pipe client

   Copyright (C) Tim Potter 2000

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

#ifndef RPCCLIENT_H
#define RPCCLIENT_H

typedef enum {
        RPC_RTYPE_NTSTATUS = 0,
        RPC_RTYPE_WERROR,
        MAX_RPC_RETURN_TYPE
} RPC_RETURN_TYPE;

struct cmd_set {
	const char *name;
	RPC_RETURN_TYPE returntype;
	NTSTATUS (*ntfn)(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, int argc, 
			const char **argv);
	WERROR (*wfn)(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, int argc, const char **argv);
	int pipe_idx;
	struct rpc_pipe_client *rpc_pipe;
	const char *description;
	const char *usage;
};

#endif /* RPCCLIENT_H */
