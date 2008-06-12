/* 
   Samba Unix/Linux SMB client library 
   Distributed SMB/CIFS Server Management Utility 
   Copyright (C) 2001 Andrew Bartlett (abartlet@samba.org)
   Copyright (C) Tim Potter     2001

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */
 
#include "includes.h"
#include "utils/net.h"

/* Macro for checking RPC error codes to make things more readable */

#define CHECK_RPC_ERR(rpc, msg) \
        if (!NT_STATUS_IS_OK(result = rpc)) { \
                DEBUG(0, (msg ": %s\n", nt_errstr(result))); \
                goto done; \
        }

#define CHECK_RPC_ERR_DEBUG(rpc, debug_args) \
        if (!NT_STATUS_IS_OK(result = rpc)) { \
                DEBUG(0, debug_args); \
                goto done; \
        }

/**
 * confirm that a domain join is still valid
 *
 * @return A shell status integer (0 for success)
 *
 **/
int net_rpc_join_ok(const char *domain, const char *server, struct in_addr *ip )
{
	uint32 neg_flags = NETLOGON_NEG_AUTH2_FLAGS|NETLOGON_NEG_SCHANNEL;
	struct cli_state *cli = NULL;
	struct rpc_pipe_client *pipe_hnd = NULL;
	struct rpc_pipe_client *netlogon_pipe = NULL;
	NTSTATUS ntret = NT_STATUS_UNSUCCESSFUL;

	/* Connect to remote machine */
	if (!(cli = net_make_ipc_connection_ex(domain, server, ip, (NET_FLAGS_ANONYMOUS|NET_FLAGS_PDC)))) {
		return -1;
	}

	/* Setup the creds as though we're going to do schannel... */
        netlogon_pipe = get_schannel_session_key(cli, domain, &neg_flags, &ntret);

	/* We return NT_STATUS_INVALID_NETWORK_RESPONSE if the server is refusing
	   to negotiate schannel, but the creds were set up ok. That'll have to do. */

        if (!netlogon_pipe) {
		if (NT_STATUS_EQUAL(ntret, NT_STATUS_INVALID_NETWORK_RESPONSE)) {
			cli_shutdown(cli);
			return 0;
		} else {
			DEBUG(0,("net_rpc_join_ok: failed to get schannel session "
					"key from server %s for domain %s. Error was %s\n",
				cli->desthost, domain, nt_errstr(ntret) ));
			cli_shutdown(cli);
			return -1;
		}
	}

	/* Only do the rest of the schannel test if the client is allowed to do this. */
	if (!lp_client_schannel()) {
		cli_shutdown(cli);
		/* We're good... */
		return 0;
	}

	pipe_hnd = cli_rpc_pipe_open_schannel_with_key(cli, PI_NETLOGON,
				PIPE_AUTH_LEVEL_PRIVACY,
				domain, netlogon_pipe->dc, &ntret);

	if (!pipe_hnd) {
		DEBUG(0,("net_rpc_join_ok: failed to open schannel session "
				"on netlogon pipe to server %s for domain %s. Error was %s\n",
			cli->desthost, domain, nt_errstr(ntret) ));
		cli_shutdown(cli);
		return -1;
	}

	cli_shutdown(cli);
	return 0;
}

/**
 * Join a domain using the administrator username and password
 *
 * @param argc  Standard main() style argc
 * @param argc  Standard main() style argv.  Initial components are already
 *              stripped.  Currently not used.
 * @return A shell status integer (0 for success)
 *
 **/

int net_rpc_join_newstyle(int argc, const char **argv) 
{

	/* libsmb variables */

	struct cli_state *cli;
	TALLOC_CTX *mem_ctx;
        uint32 acb_info = ACB_WSTRUST;
	uint32 neg_flags = NETLOGON_NEG_AUTH2_FLAGS|(lp_client_schannel() ? NETLOGON_NEG_SCHANNEL : 0);
	uint32 sec_channel_type;
	struct rpc_pipe_client *pipe_hnd = NULL;

	/* rpc variables */

	POLICY_HND lsa_pol, sam_pol, domain_pol, user_pol;
	DOM_SID *domain_sid;
	uint32 user_rid;

	/* Password stuff */

	char *clear_trust_password = NULL;
	uchar pwbuf[516];
	SAM_USERINFO_CTR ctr;
	SAM_USER_INFO_24 p24;
	SAM_USER_INFO_16 p16;
	uchar md4_trust_password[16];

	/* Misc */

	NTSTATUS result;
	int retval = 1;
	char *domain = NULL;
	uint32 num_rids, *name_types, *user_rids;
	uint32 flags = 0x3e8;
	char *acct_name;
	const char *const_acct_name;

	/* check what type of join */
	if (argc >= 0) {
		sec_channel_type = get_sec_channel_type(argv[0]);
	} else {
		sec_channel_type = get_sec_channel_type(NULL);
	}

	switch (sec_channel_type) {
	case SEC_CHAN_WKSTA:
		acb_info = ACB_WSTRUST;
		break;
	case SEC_CHAN_BDC:
		acb_info = ACB_SVRTRUST;
		break;
#if 0
	case SEC_CHAN_DOMAIN:
		acb_info = ACB_DOMTRUST;
		break;
#endif
	}

	/* Make authenticated connection to remote machine */

	if (!(cli = net_make_ipc_connection(NET_FLAGS_PDC))) 
		return 1;

	if (!(mem_ctx = talloc_init("net_rpc_join_newstyle"))) {
		DEBUG(0, ("Could not initialise talloc context\n"));
		goto done;
	}

	/* Fetch domain sid */

	pipe_hnd = cli_rpc_pipe_open_noauth(cli, PI_LSARPC, &result);
	if (!pipe_hnd) {
		DEBUG(0, ("Error connecting to LSA pipe. Error was %s\n",
			nt_errstr(result) ));
		goto done;
	}


	CHECK_RPC_ERR(rpccli_lsa_open_policy(pipe_hnd, mem_ctx, True,
					  SEC_RIGHTS_MAXIMUM_ALLOWED,
					  &lsa_pol),
		      "error opening lsa policy handle");

	CHECK_RPC_ERR(rpccli_lsa_query_info_policy(pipe_hnd, mem_ctx, &lsa_pol,
						5, &domain, &domain_sid),
		      "error querying info policy");

	rpccli_lsa_close(pipe_hnd, mem_ctx, &lsa_pol);
	cli_rpc_pipe_close(pipe_hnd); /* Done with this pipe */

	/* Bail out if domain didn't get set. */
	if (!domain) {
		DEBUG(0, ("Could not get domain name.\n"));
		goto done;
	}

	/* Create domain user */
	pipe_hnd = cli_rpc_pipe_open_noauth(cli, PI_SAMR, &result);
	if (!pipe_hnd) {
		DEBUG(0, ("Error connecting to SAM pipe. Error was %s\n",
			nt_errstr(result) ));
		goto done;
	}

	CHECK_RPC_ERR(rpccli_samr_connect(pipe_hnd, mem_ctx, 
				       SEC_RIGHTS_MAXIMUM_ALLOWED,
				       &sam_pol),
		      "could not connect to SAM database");

	
	CHECK_RPC_ERR(rpccli_samr_open_domain(pipe_hnd, mem_ctx, &sam_pol,
					   SEC_RIGHTS_MAXIMUM_ALLOWED,
					   domain_sid, &domain_pol),
		      "could not open domain");

	/* Create domain user */
	if ((acct_name = talloc_asprintf(mem_ctx, "%s$", global_myname())) == NULL) {
		result = NT_STATUS_NO_MEMORY;
		goto done;
	}
	strlower_m(acct_name);
	const_acct_name = acct_name;

	result = rpccli_samr_create_dom_user(pipe_hnd, mem_ctx, &domain_pol,
					  acct_name, acb_info,
					  0xe005000b, &user_pol, 
					  &user_rid);

	if (!NT_STATUS_IS_OK(result) && 
	    !NT_STATUS_EQUAL(result, NT_STATUS_USER_EXISTS)) {
		d_fprintf(stderr, "Creation of workstation account failed\n");

		/* If NT_STATUS_ACCESS_DENIED then we have a valid
		   username/password combo but the user does not have
		   administrator access. */

		if (NT_STATUS_V(result) == NT_STATUS_V(NT_STATUS_ACCESS_DENIED))
			d_fprintf(stderr, "User specified does not have administrator privileges\n");

		goto done;
	}

	/* We *must* do this.... don't ask... */

	if (NT_STATUS_IS_OK(result)) {
		rpccli_samr_close(pipe_hnd, mem_ctx, &user_pol);
	}

	CHECK_RPC_ERR_DEBUG(rpccli_samr_lookup_names(pipe_hnd, mem_ctx,
						  &domain_pol, flags,
						  1, &const_acct_name, 
						  &num_rids,
						  &user_rids, &name_types),
			    ("error looking up rid for user %s: %s\n",
			     acct_name, nt_errstr(result)));

	if (name_types[0] != SID_NAME_USER) {
		DEBUG(0, ("%s is not a user account (type=%d)\n", acct_name, name_types[0]));
		goto done;
	}

	user_rid = user_rids[0];
		
	/* Open handle on user */

	CHECK_RPC_ERR_DEBUG(
		rpccli_samr_open_user(pipe_hnd, mem_ctx, &domain_pol,
				   SEC_RIGHTS_MAXIMUM_ALLOWED,
				   user_rid, &user_pol),
		("could not re-open existing user %s: %s\n",
		 acct_name, nt_errstr(result)));
	
	/* Create a random machine account password */

	{ 
		char *str;
		str = generate_random_str(DEFAULT_TRUST_ACCOUNT_PASSWORD_LENGTH);
		clear_trust_password = SMB_STRDUP(str);
		E_md4hash(clear_trust_password, md4_trust_password);
	}

	encode_pw_buffer(pwbuf, clear_trust_password, STR_UNICODE);

	/* Set password on machine account */

	ZERO_STRUCT(ctr);
	ZERO_STRUCT(p24);

	init_sam_user_info24(&p24, (char *)pwbuf,24);

	ctr.switch_value = 24;
	ctr.info.id24 = &p24;

	CHECK_RPC_ERR(rpccli_samr_set_userinfo(pipe_hnd, mem_ctx, &user_pol, 24, 
					    &cli->user_session_key, &ctr),
		      "error setting trust account password");

	/* Why do we have to try to (re-)set the ACB to be the same as what
	   we passed in the samr_create_dom_user() call?  When a NT
	   workstation is joined to a domain by an administrator the
	   acb_info is set to 0x80.  For a normal user with "Add
	   workstations to the domain" rights the acb_info is 0x84.  I'm
	   not sure whether it is supposed to make a difference or not.  NT
	   seems to cope with either value so don't bomb out if the set
	   userinfo2 level 0x10 fails.  -tpot */

	ZERO_STRUCT(ctr);
	ctr.switch_value = 16;
	ctr.info.id16 = &p16;

	init_sam_user_info16(&p16, acb_info);

	/* Ignoring the return value is necessary for joining a domain
	   as a normal user with "Add workstation to domain" privilege. */

	result = rpccli_samr_set_userinfo2(pipe_hnd, mem_ctx, &user_pol, 16, 
					&cli->user_session_key, &ctr);

	rpccli_samr_close(pipe_hnd, mem_ctx, &user_pol);
	cli_rpc_pipe_close(pipe_hnd); /* Done with this pipe */

	/* Now check the whole process from top-to-bottom */

	pipe_hnd = cli_rpc_pipe_open_noauth(cli, PI_NETLOGON, &result);
	if (!pipe_hnd) {
		DEBUG(0,("Error connecting to NETLOGON pipe. Error was %s\n",
			nt_errstr(result) ));
		goto done;
	}

	result = rpccli_netlogon_setup_creds(pipe_hnd,
					cli->desthost, /* server name */
					domain,        /* domain */
					global_myname(), /* client name */
					global_myname(), /* machine account name */
                                        md4_trust_password,
                                        sec_channel_type,
                                        &neg_flags);

	if (!NT_STATUS_IS_OK(result)) {
		DEBUG(0, ("Error in domain join verification (credential setup failed): %s\n\n",
			  nt_errstr(result)));

		if ( NT_STATUS_EQUAL(result, NT_STATUS_ACCESS_DENIED) &&
		     (sec_channel_type == SEC_CHAN_BDC) ) {
			d_fprintf(stderr, "Please make sure that no computer account\n"
				 "named like this machine (%s) exists in the domain\n",
				 global_myname());
		}

		goto done;
	}

	/* We can only check the schannel connection if the client is allowed
	   to do this and the server supports it. If not, just assume success
	   (after all the rpccli_netlogon_setup_creds() succeeded, and we'll
	   do the same again (setup creds) in net_rpc_join_ok(). JRA. */

	if (lp_client_schannel() && (neg_flags & NETLOGON_NEG_SCHANNEL)) {
		struct rpc_pipe_client *netlogon_schannel_pipe = 
						cli_rpc_pipe_open_schannel_with_key(cli,
							PI_NETLOGON,
							PIPE_AUTH_LEVEL_PRIVACY,
							domain,
							pipe_hnd->dc,
							&result);

		if (!NT_STATUS_IS_OK(result)) {
			DEBUG(0, ("Error in domain join verification (schannel setup failed): %s\n\n",
				  nt_errstr(result)));

			if ( NT_STATUS_EQUAL(result, NT_STATUS_ACCESS_DENIED) &&
			     (sec_channel_type == SEC_CHAN_BDC) ) {
				d_fprintf(stderr, "Please make sure that no computer account\n"
					 "named like this machine (%s) exists in the domain\n",
					 global_myname());
			}

			goto done;
		}
		cli_rpc_pipe_close(netlogon_schannel_pipe);
	}

	cli_rpc_pipe_close(pipe_hnd);

	/* Now store the secret in the secrets database */

	strupper_m(domain);

	if (!secrets_store_domain_sid(domain, domain_sid)) {
		DEBUG(0, ("error storing domain sid for %s\n", domain));
		goto done;
	}

	if (!secrets_store_machine_password(clear_trust_password, domain, sec_channel_type)) {
		DEBUG(0, ("error storing plaintext domain secrets for %s\n", domain));
	}

	/* double-check, connection from scratch */
	retval = net_rpc_join_ok(domain, cli->desthost, &cli->dest_ip);
	
done:

	/* Display success or failure */

	if (domain) {
		if (retval != 0) {
			fprintf(stderr,"Unable to join domain %s.\n",domain);
		} else {
			printf("Joined domain %s.\n",domain);
		}
	}
	
	cli_shutdown(cli);

	SAFE_FREE(clear_trust_password);

	return retval;
}

/**
 * check that a join is OK
 *
 * @return A shell status integer (0 for success)
 *
 **/
int net_rpc_testjoin(int argc, const char **argv) 
{
	char *domain = smb_xstrdup(opt_target_workgroup);

	/* Display success or failure */
	if (net_rpc_join_ok(domain, NULL, NULL) != 0) {
		fprintf(stderr,"Join to domain '%s' is not valid\n",domain);
		free(domain);
		return -1;
	}

	printf("Join to '%s' is OK\n",domain);
	free(domain);
	return 0;
}
