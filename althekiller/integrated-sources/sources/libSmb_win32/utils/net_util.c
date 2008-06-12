/*
 *  Unix SMB/CIFS implementation.
 *  Helper routines for net
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

BOOL is_valid_policy_hnd(const POLICY_HND *hnd)
{
	POLICY_HND tmp;
	ZERO_STRUCT(tmp);
	return (memcmp(&tmp, hnd, sizeof(tmp)) != 0);
}

NTSTATUS net_rpc_lookup_name(TALLOC_CTX *mem_ctx, struct cli_state *cli,
			     const char *name, const char **ret_domain,
			     const char **ret_name, DOM_SID *ret_sid,
			     enum SID_NAME_USE *ret_type)
{
	struct rpc_pipe_client *lsa_pipe;
	POLICY_HND pol;
	NTSTATUS result = NT_STATUS_OK;
	const char **dom_names;
	DOM_SID *sids;
	uint32_t *types;

	ZERO_STRUCT(pol);

	lsa_pipe = cli_rpc_pipe_open_noauth(cli, PI_LSARPC, &result);
	if (lsa_pipe == NULL) {
		d_fprintf(stderr, "Could not initialise lsa pipe\n");
		return result;
	}

	result = rpccli_lsa_open_policy(lsa_pipe, mem_ctx, False, 
					SEC_RIGHTS_MAXIMUM_ALLOWED,
					&pol);
	if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "open_policy failed: %s\n",
			  nt_errstr(result));
		return result;
	}

	result = rpccli_lsa_lookup_names(lsa_pipe, mem_ctx, &pol, 1,
					 &name, &dom_names, &sids, &types);

	if (!NT_STATUS_IS_OK(result)) {
		/* This can happen easily, don't log an error */
		goto done;
	}

	if (ret_domain != NULL) {
		*ret_domain = dom_names[0];
	}
	if (ret_name != NULL) {
		*ret_name = talloc_strdup(mem_ctx, name);
	}
	if (ret_sid != NULL) {
		sid_copy(ret_sid, &sids[0]);
	}
	if (ret_type != NULL) {
		*ret_type = types[0];
	}

 done:
	if (is_valid_policy_hnd(&pol)) {
		rpccli_lsa_close(lsa_pipe, mem_ctx, &pol);
	}
	cli_rpc_pipe_close(lsa_pipe);

	return result;
}
