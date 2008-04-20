/*
   Unix SMB/CIFS implementation.

   Name lookup.

   Copyright (C) Jeremy Allison 2005

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
#include "utils/net.h"

/********************************************************
 Connection cachine struct. Goes away when ctx destroyed.
********************************************************/

struct con_struct {
	BOOL failed_connect;
	NTSTATUS err;
	struct cli_state *cli;
	struct rpc_pipe_client *lsapipe;
	POLICY_HND pol;
};

static struct con_struct *cs;

/********************************************************
 Close connection on context destruction.
********************************************************/

static int cs_destructor(void *p)
{
	if (cs->cli) {
		cli_shutdown(cs->cli);
	}
	cs = NULL;
	return 0;
}

/********************************************************
 Create the connection to localhost.
********************************************************/

static struct con_struct *create_cs(TALLOC_CTX *ctx, NTSTATUS *perr)
{
	NTSTATUS nt_status;
	struct in_addr loopback_ip = *interpret_addr2("127.0.0.1");

	*perr = NT_STATUS_OK;

	if (cs) {
		if (cs->failed_connect) {
			*perr = cs->err;
			return NULL;
		}
		return cs;
	}

	cs = TALLOC_P(ctx, struct con_struct);
	if (!cs) {
		*perr = NT_STATUS_NO_MEMORY;
		return NULL;
	}

	ZERO_STRUCTP(cs);
	talloc_set_destructor(cs, cs_destructor);

	/* Connect to localhost with given username/password. */
	/* JRA. Pretty sure we can just do this anonymously.... */
#if 0
	if (!opt_password && !opt_machine_pass) {
		char *pass = getpass("Password:");
		if (pass) {
			opt_password = SMB_STRDUP(pass);
		}
	}
#endif

	nt_status = cli_full_connection(&cs->cli, global_myname(), global_myname(),
					&loopback_ip, 0,
					"IPC$", "IPC",
#if 0
					opt_user_name,
					opt_workgroup,
					opt_password,
#else
					"",
					opt_workgroup,
					"",
#endif
					0,
					Undefined,
					NULL);

	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(2,("create_cs: Connect failed. Error was %s\n", nt_errstr(nt_status)));
		cs->failed_connect = True;
		cs->err = nt_status;
		*perr = nt_status;
		return NULL;
	}

	cs->lsapipe = cli_rpc_pipe_open_noauth(cs->cli,
					PI_LSARPC,
					&nt_status);

	if (cs->lsapipe == NULL) {
		DEBUG(2,("create_cs: open LSA pipe failed. Error was %s\n", nt_errstr(nt_status)));
		cs->failed_connect = True;
		cs->err = nt_status;
		*perr = nt_status;
		return NULL;
	}

	nt_status = rpccli_lsa_open_policy(cs->lsapipe, ctx, True,
				SEC_RIGHTS_MAXIMUM_ALLOWED,
				&cs->pol);

	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(2,("create_cs: rpccli_lsa_open_policy failed. Error was %s\n", nt_errstr(nt_status)));
		cs->failed_connect = True;
		cs->err = nt_status;
		*perr = nt_status;
		return NULL;
	}

	return cs;
}

/********************************************************
 Do a lookup_sids call to localhost.
 Check if the local machine is authoritative for this sid. We can't
 check if this is our SID as that's stored in the root-read-only
 secrets.tdb.
 The local smbd will also ask winbindd for us, so we don't have to.
********************************************************/

NTSTATUS net_lookup_name_from_sid(TALLOC_CTX *ctx,
				DOM_SID *psid,
				const char **ppdomain,
				const char **ppname)
{
	NTSTATUS nt_status;
	struct con_struct *csp = NULL;
	char **domains;
	char **names;
	uint32 *types;

	*ppdomain = NULL;
	*ppname = NULL;

	csp = create_cs(ctx, &nt_status);
	if (csp == NULL) {
		return nt_status;
	}

	nt_status = rpccli_lsa_lookup_sids(csp->lsapipe, ctx,
						&csp->pol,
						1, psid,
						&domains,
						&names,
						&types);

	if (!NT_STATUS_IS_OK(nt_status)) {
		return nt_status;
	}

	*ppdomain = domains[0];
	*ppname = names[0];
	/* Don't care about type here. */

        /* Converted OK */
        return NT_STATUS_OK;
}

/********************************************************
 Do a lookup_names call to localhost.
********************************************************/

NTSTATUS net_lookup_sid_from_name(TALLOC_CTX *ctx, const char *full_name, DOM_SID *pret_sid)
{
	NTSTATUS nt_status;
	struct con_struct *csp = NULL;
	DOM_SID *sids = NULL;
	uint32 *types = NULL;

	csp = create_cs(ctx, &nt_status);
	if (csp == NULL) {
		return nt_status;
	}

	nt_status = rpccli_lsa_lookup_names(csp->lsapipe, ctx,
						&csp->pol,
						1,
						&full_name,
					        NULL, &sids,
						&types);

	if (!NT_STATUS_IS_OK(nt_status)) {
		return nt_status;
	}

	*pret_sid = sids[0];

        /* Converted OK */
        return NT_STATUS_OK;
}
