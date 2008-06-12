/*
 *  Unix SMB/CIFS implementation.
 *  Shell around net rpc subcommands
 *  Copyright (C) Volker Lendecke 2006
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include "includes.h"
#include "utils/net.h"

static NTSTATUS rpc_sh_info(TALLOC_CTX *mem_ctx, struct rpc_sh_ctx *ctx,
			    struct rpc_pipe_client *pipe_hnd,
			    int argc, const char **argv)
{
	return rpc_info_internals(ctx->domain_sid, ctx->domain_name,
				  ctx->cli, pipe_hnd, mem_ctx,
				  argc, argv);
}

static struct rpc_sh_ctx *this_ctx;

static char **completion_fn(const char *text, int start, int end)
{
	char **cmds = NULL;
	int n_cmds = 0;
	struct rpc_sh_cmd *c;

	if (start != 0) {
		return NULL;
	}

	ADD_TO_ARRAY(NULL, char *, SMB_STRDUP(text), &cmds, &n_cmds);

	for (c = this_ctx->cmds; c->name != NULL; c++) {
		BOOL match = (strncmp(text, c->name, strlen(text)) == 0);

		if (match) {
			ADD_TO_ARRAY(NULL, char *, SMB_STRDUP(c->name),
				     &cmds, &n_cmds);
		}
	}

	if (n_cmds == 2) {
		SAFE_FREE(cmds[0]);
		cmds[0] = cmds[1];
		n_cmds -= 1;
	}

	ADD_TO_ARRAY(NULL, char *, NULL, &cmds, &n_cmds);
	return cmds;
}

static NTSTATUS net_sh_run(struct rpc_sh_ctx *ctx, struct rpc_sh_cmd *cmd,
			   int argc, const char **argv)
{
	TALLOC_CTX *mem_ctx;
	struct rpc_pipe_client *pipe_hnd;
	NTSTATUS status;

	mem_ctx = talloc_new(ctx);
	if (mem_ctx == NULL) {
		d_fprintf(stderr, "talloc_new failed\n");
		return NT_STATUS_NO_MEMORY;
	}

	pipe_hnd = cli_rpc_pipe_open_noauth(ctx->cli, cmd->pipe_idx, &status);
	if (pipe_hnd == NULL) {
		d_fprintf(stderr, "Could not open pipe: %s\n",
			  nt_errstr(status));
		return status;
	}

	status = cmd->fn(mem_ctx, ctx, pipe_hnd, argc, argv);

	cli_rpc_pipe_close(pipe_hnd);

	talloc_destroy(mem_ctx);

	return status;
}

static BOOL net_sh_process(struct rpc_sh_ctx *ctx,
			   int argc, const char **argv)
{
	struct rpc_sh_cmd *c;
	struct rpc_sh_ctx *new_ctx;
	NTSTATUS status;

	if (argc == 0) {
		return True;
	}

	if (ctx == this_ctx) {

		/* We've been called from the cmd line */
		if (strequal(argv[0], "..") &&
		    (this_ctx->parent != NULL)) {
			new_ctx = this_ctx->parent;
			TALLOC_FREE(this_ctx);
			this_ctx = new_ctx;
			return True;
		}
	}

	if (strequal(argv[0], "exit") || strequal(argv[0], "quit")) {
		return False;
	}

	if (strequal(argv[0], "help") || strequal(argv[0], "?")) {
		for (c = ctx->cmds; c->name != NULL; c++) {
			if (ctx != this_ctx) {
				d_printf("%s ", ctx->whoami);
			}
			d_printf("%-15s %s\n", c->name, c->help);
		}
		return True;
	}

	for (c = ctx->cmds; c->name != NULL; c++) {
		if (strequal(c->name, argv[0])) {
			break;
		}
	}

	if (c->name == NULL) {
		/* None found */
		d_fprintf(stderr, "%s: unknown cmd\n", argv[0]);
		return True;
	}

	new_ctx = TALLOC_P(ctx, struct rpc_sh_ctx);
	if (new_ctx == NULL) {
		d_fprintf(stderr, "talloc failed\n");
		return False;
	}
	new_ctx->cli = ctx->cli;
	new_ctx->whoami = talloc_asprintf(new_ctx, "%s %s",
					  ctx->whoami, c->name);
	new_ctx->thiscmd = talloc_strdup(new_ctx, c->name);

	if (c->sub != NULL) {
		new_ctx->cmds = c->sub(new_ctx, ctx);
	} else {
		new_ctx->cmds = NULL;
	}

	new_ctx->parent = ctx;
	new_ctx->domain_name = ctx->domain_name;
	new_ctx->domain_sid = ctx->domain_sid;

	argc -= 1;
	argv += 1;

	if (c->sub != NULL) {
		if (argc == 0) {
			this_ctx = new_ctx;
			return True;
		}
		return net_sh_process(new_ctx, argc, argv);
	}

	status = net_sh_run(new_ctx, c, argc, argv);

	if (!NT_STATUS_IS_OK(status)) {
		d_fprintf(stderr, "%s failed: %s\n", new_ctx->whoami,
			  nt_errstr(status));
	}

	return True;
}

static struct rpc_sh_cmd sh_cmds[6] = {

	{ "info", NULL, PI_SAMR, rpc_sh_info,
	  "Print information about the domain connected to" },

	{ "rights", net_rpc_rights_cmds, 0, NULL,
	  "List/Grant/Revoke user rights" },

	{ "share", net_rpc_share_cmds, 0, NULL,
	  "List/Add/Remove etc shares" },

	{ "user", net_rpc_user_cmds, 0, NULL,
	  "List/Add/Remove user info" },

	{ "account", net_rpc_acct_cmds, 0, NULL,
	  "Show/Change account policy settings" },

	{ NULL, NULL, 0, NULL, NULL }
};

int net_rpc_shell(int argc, const char **argv)
{
	NTSTATUS status;
	struct rpc_sh_ctx *ctx;

	if (argc != 0) {
		d_fprintf(stderr, "usage: net rpc shell\n");
		return -1;
	}

	ctx = TALLOC_P(NULL, struct rpc_sh_ctx);
	if (ctx == NULL) {
		d_fprintf(stderr, "talloc failed\n");
		return -1;
	}

	ctx->cli = net_make_ipc_connection(0);
	if (ctx->cli == NULL) {
		d_fprintf(stderr, "Could not open connection\n");
		return -1;
	}

	ctx->cmds = sh_cmds;
	ctx->whoami = "net rpc";
	ctx->parent = NULL;

	status = net_get_remote_domain_sid(ctx->cli, ctx, &ctx->domain_sid,
					   &ctx->domain_name);
	if (!NT_STATUS_IS_OK(status)) {
		return -1;
	}

	d_printf("Talking to domain %s (%s)\n", ctx->domain_name,
		 sid_string_static(ctx->domain_sid));
	
	this_ctx = ctx;

	while(1) {
		char *prompt;
		char *line;
		int ret;

		asprintf(&prompt, "%s> ", this_ctx->whoami);

		line = smb_readline(prompt, NULL, completion_fn);
		SAFE_FREE(prompt);

		if (line == NULL) {
			break;
		}

		ret = poptParseArgvString(line, &argc, &argv);
		if (ret == POPT_ERROR_NOARG) {
			continue;
		}
		if (ret != 0) {
			d_fprintf(stderr, "cmdline invalid: %s\n",
				  poptStrerror(ret));
			return False;
		}

		if ((line[0] != '\n') &&
		    (!net_sh_process(this_ctx, argc, argv))) {
			break;
		}
	}

	cli_shutdown(ctx->cli);

	TALLOC_FREE(ctx);

	return 0;
}
