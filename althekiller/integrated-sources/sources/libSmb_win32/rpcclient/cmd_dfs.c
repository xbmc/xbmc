/* 
   Unix SMB/CIFS implementation.
   RPC pipe client

   Copyright (C) Tim Potter 2000
   Copyright (C) Jelmer Vernooij       2005.

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

/* Check DFS is supported by the remote server */

static NTSTATUS cmd_dfs_exist(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                              int argc, const char **argv)
{
	uint32 dfs_exists;
	NTSTATUS result;

	if (argc != 1) {
		printf("Usage: %s\n", argv[0]);
		return NT_STATUS_OK;
	}

	result = rpccli_dfs_GetManagerVersion(cli, mem_ctx, &dfs_exists);

	if (NT_STATUS_IS_OK(result))
		printf("dfs is %spresent\n", dfs_exists ? "" : "not ");

	return result;
}

static NTSTATUS cmd_dfs_add(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                            int argc, const char **argv)
{
	NTSTATUS result;
	const char *path, *servername, *sharename, *comment;
	uint32 flags = 0;

	if (argc != 5) {
		printf("Usage: %s path servername sharename comment\n", 
		       argv[0]);
		return NT_STATUS_OK;
	}

	path = argv[1];
	servername = argv[2];
	sharename = argv[3];
	comment = argv[4];

	result = rpccli_dfs_Add(cli, mem_ctx, path, servername, 
			     sharename, comment, flags);

	return result;
}

static NTSTATUS cmd_dfs_remove(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                               int argc, const char **argv)
{
	NTSTATUS result;
	const char *path, *servername, *sharename;

	if (argc != 4) {
		printf("Usage: %s path servername sharename\n", argv[0]);
		return NT_STATUS_OK;
	}

	path = argv[1];
	servername = argv[2];
	sharename = argv[3];

	result = rpccli_dfs_Remove(cli, mem_ctx, path, servername, 
				sharename);

	return result;
}

/* Display a DFS_INFO_1 structure */

static void display_dfs_info_1(NETDFS_DFS_INFO1 *info1)
{
	fstring temp;

	unistr2_to_ascii(temp, &info1->path, sizeof(temp) - 1);
	printf("path: %s\n", temp);
}

/* Display a DFS_INFO_2 structure */

static void display_dfs_info_2(NETDFS_DFS_INFO2 *info2)
{
	fstring temp;

	unistr2_to_ascii(temp, &info2->path, sizeof(temp) - 1);
	printf("path: %s\n", temp);

	unistr2_to_ascii(temp, &info2->comment, sizeof(temp) - 1);
	printf("\tcomment: %s\n", temp);

	printf("\tstate: %d\n", info2->state);
	printf("\tnum_stores: %d\n", info2->num_stores);
}

/* Display a DFS_INFO_3 structure */

static void display_dfs_info_3(NETDFS_DFS_INFO3 *info3)
{
	fstring temp;
	int i;

	unistr2_to_ascii(temp, &info3->path, sizeof(temp) - 1);
	printf("path: %s\n", temp);

	unistr2_to_ascii(temp, &info3->comment, sizeof(temp) - 1);
	printf("\tcomment: %s\n", temp);

	printf("\tstate: %d\n", info3->state);
	printf("\tnum_stores: %d\n", info3->num_stores);

	for (i = 0; i < info3->num_stores; i++) {
		NETDFS_DFS_STORAGEINFO *dsi = &info3->stores[i];

		unistr2_to_ascii(temp, &dsi->server, sizeof(temp) - 1);
		printf("\t\tstorage[%d] server: %s\n", i, temp);

		unistr2_to_ascii(temp, &dsi->share, sizeof(temp) - 1);
		printf("\t\tstorage[%d] share: %s\n", i, temp);
	}
}


/* Display a DFS_INFO_CTR structure */
static void display_dfs_info(NETDFS_DFS_INFO_CTR *ctr)
{
	switch (ctr->switch_value) {
		case 0x01:
			display_dfs_info_1(&ctr->u.info1);
			break;
		case 0x02:
			display_dfs_info_2(&ctr->u.info2);
			break;
		case 0x03:
			display_dfs_info_3(&ctr->u.info3);
			break;
		default:
			printf("unsupported info level %d\n", 
			       ctr->switch_value);
			break;
	}
}

static void display_dfs_enumstruct(NETDFS_DFS_ENUMSTRUCT *ctr)
{
	int i;
	
	/* count is always the first element, so we can just use info1 here */
	for (i = 0; i < ctr->e.u.info1.count; i++) {
		switch (ctr->level) {
		case 1: display_dfs_info_1(&ctr->e.u.info1.s[i]); break;
		case 2: display_dfs_info_2(&ctr->e.u.info2.s[i]); break;
		case 3: display_dfs_info_3(&ctr->e.u.info3.s[i]); break;
		default:
				printf("unsupported info level %d\n", 
			       ctr->level);
				return;
		}
	}
}

/* Enumerate dfs shares */

static NTSTATUS cmd_dfs_enum(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                             int argc, const char **argv)
{
	NETDFS_DFS_ENUMSTRUCT str;
	NETDFS_DFS_ENUMINFO_CTR ctr;
	NTSTATUS result;
	uint32 info_level = 1;
	uint32 total = 0;

	if (argc > 2) {
		printf("Usage: %s [info_level]\n", argv[0]);
		return NT_STATUS_OK;
	}

	if (argc == 2)
		info_level = atoi(argv[1]);

	ZERO_STRUCT(ctr);
	init_netdfs_dfs_EnumStruct(&str, info_level, ctr);
	str.e.ptr0 = 1;

	result = rpccli_dfs_Enum(cli, mem_ctx, info_level, 0xFFFFFFFF, &str, &total);

	if (NT_STATUS_IS_OK(result))
		display_dfs_enumstruct(&str);

	return result;
}

static NTSTATUS cmd_dfs_getinfo(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                                int argc, const char **argv)
{
	NTSTATUS result;
	const char *path, *servername, *sharename;
	uint32 info_level = 1;
	NETDFS_DFS_INFO_CTR ctr;

	if (argc < 4 || argc > 5) {
		printf("Usage: %s path servername sharename "
                       "[info_level]\n", argv[0]);
		return NT_STATUS_OK;
	}

	path = argv[1];
	servername = argv[2];
	sharename = argv[3];

	if (argc == 5)
		info_level = atoi(argv[4]);

	result = rpccli_dfs_GetInfo(cli, mem_ctx, path, servername, 
				  sharename, info_level, &ctr);

	if (NT_STATUS_IS_OK(result))
		display_dfs_info(&ctr);

	return result;
}

/* List of commands exported by this module */

struct cmd_set dfs_commands[] = {

	{ "DFS" },

	{ "dfsexist",  RPC_RTYPE_NTSTATUS, cmd_dfs_exist,   NULL, PI_NETDFS, NULL, "Query DFS support",    "" },
	{ "dfsadd",    RPC_RTYPE_NTSTATUS, cmd_dfs_add,     NULL, PI_NETDFS, NULL, "Add a DFS share",      "" },
	{ "dfsremove", RPC_RTYPE_NTSTATUS, cmd_dfs_remove,  NULL, PI_NETDFS, NULL, "Remove a DFS share",   "" },
	{ "dfsgetinfo",RPC_RTYPE_NTSTATUS, cmd_dfs_getinfo, NULL, PI_NETDFS, NULL, "Query DFS share info", "" },
	{ "dfsenum",   RPC_RTYPE_NTSTATUS, cmd_dfs_enum,    NULL, PI_NETDFS, NULL, "Enumerate dfs shares", "" },

	{ NULL }
};
