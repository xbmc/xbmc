/* 
   Samba Unix/Linux SMB client library 
   Distributed SMB/CIFS Server Management Utility 
   Copyright (C) 2001 Andrew Bartlett (abartlet@samba.org)
   Copyright (C) 2002 Jim McDonough (jmcd@us.ibm.com)
   Copyright (C) 2004 Guenther Deschner (gd@samba.org)
   Copyright (C) 2005 Jeremy Allison (jra@samba.org)

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

static int net_mode_share;

/**
 * @file net_rpc.c
 *
 * @brief RPC based subcommands for the 'net' utility.
 *
 * This file should contain much of the functionality that used to
 * be found in rpcclient, execpt that the commands should change 
 * less often, and the fucntionality should be sane (the user is not 
 * expected to know a rid/sid before they conduct an operation etc.)
 *
 * @todo Perhaps eventually these should be split out into a number
 * of files, as this could get quite big.
 **/


/**
 * Many of the RPC functions need the domain sid.  This function gets
 *  it at the start of every run 
 *
 * @param cli A cli_state already connected to the remote machine
 *
 * @return The Domain SID of the remote machine.
 **/

NTSTATUS net_get_remote_domain_sid(struct cli_state *cli, TALLOC_CTX *mem_ctx,
				   DOM_SID **domain_sid, char **domain_name)
{
	struct rpc_pipe_client *lsa_pipe;
	POLICY_HND pol;
	NTSTATUS result = NT_STATUS_OK;
	uint32 info_class = 5;
	
	lsa_pipe = cli_rpc_pipe_open_noauth(cli, PI_LSARPC, &result);
	if (!lsa_pipe) {
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

	result = rpccli_lsa_query_info_policy(lsa_pipe, mem_ctx, &pol,
					      info_class, domain_name,
					      domain_sid);
	if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "lsaquery failed: %s\n",
			  nt_errstr(result));
		return result;
	}

	rpccli_lsa_close(lsa_pipe, mem_ctx, &pol);
	cli_rpc_pipe_close(lsa_pipe);

	return NT_STATUS_OK;
}

/**
 * Run a single RPC command, from start to finish.
 *
 * @param pipe_name the pipe to connect to (usually a PIPE_ constant)
 * @param conn_flag a NET_FLAG_ combination.  Passed to 
 *                   net_make_ipc_connection.
 * @param argc  Standard main() style argc
 * @param argc  Standard main() style argv.  Initial components are already
 *              stripped
 * @return A shell status integer (0 for success)
 */

int run_rpc_command(struct cli_state *cli_arg,
			const int pipe_idx,
			int conn_flags,
			rpc_command_fn fn,
			int argc,
			const char **argv) 
{
	struct cli_state *cli = NULL;
	struct rpc_pipe_client *pipe_hnd = NULL;
	TALLOC_CTX *mem_ctx;
	NTSTATUS nt_status;
	DOM_SID *domain_sid;
	char *domain_name;

	/* make use of cli_state handed over as an argument, if possible */
	if (!cli_arg) {
		cli = net_make_ipc_connection(conn_flags);
	} else {
		cli = cli_arg;
	}

	if (!cli) {
		return -1;
	}

	/* Create mem_ctx */
	
	if (!(mem_ctx = talloc_init("run_rpc_command"))) {
		DEBUG(0, ("talloc_init() failed\n"));
		cli_shutdown(cli);
		return -1;
	}
	
	nt_status = net_get_remote_domain_sid(cli, mem_ctx, &domain_sid,
					      &domain_name);
	if (!NT_STATUS_IS_OK(nt_status)) {
		cli_shutdown(cli);
		return -1;
	}

	if (!(conn_flags & NET_FLAGS_NO_PIPE)) {
		if (lp_client_schannel() && (pipe_idx == PI_NETLOGON)) {
			/* Always try and create an schannel netlogon pipe. */
			pipe_hnd = cli_rpc_pipe_open_schannel(cli, pipe_idx,
							PIPE_AUTH_LEVEL_PRIVACY,
							domain_name,
							&nt_status);
			if (!pipe_hnd) {
				DEBUG(0, ("Could not initialise schannel netlogon pipe. Error was %s\n",
					nt_errstr(nt_status) ));
				cli_shutdown(cli);
				return -1;
			}
		} else {
			pipe_hnd = cli_rpc_pipe_open_noauth(cli, pipe_idx, &nt_status);
			if (!pipe_hnd) {
				DEBUG(0, ("Could not initialise pipe %s. Error was %s\n",
					cli_get_pipe_name(pipe_idx),
					nt_errstr(nt_status) ));
				cli_shutdown(cli);
				return -1;
			}
		}
	}
	
	nt_status = fn(domain_sid, domain_name, cli, pipe_hnd, mem_ctx, argc, argv);
	
	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(1, ("rpc command function failed! (%s)\n", nt_errstr(nt_status)));
	} else {
		DEBUG(5, ("rpc command function succedded\n"));
	}
		
	if (!(conn_flags & NET_FLAGS_NO_PIPE)) {
		if (pipe_hnd) {
			cli_rpc_pipe_close(pipe_hnd);
		}
	}

	/* close the connection only if it was opened here */
	if (!cli_arg) {
		cli_shutdown(cli);
	}
	
	talloc_destroy(mem_ctx);
	return (!NT_STATUS_IS_OK(nt_status));
}

/** 
 * Force a change of the trust acccount password.
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid aquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on compleation of the function.
 * @param argc  Standard main() style argc
 * @param argc  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_changetrustpw_internals(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv)
{
	
	return trust_pw_find_change_and_store_it(pipe_hnd, mem_ctx, opt_target_workgroup);
}

/** 
 * Force a change of the trust acccount password.
 *
 * @param argc  Standard main() style argc
 * @param argc  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/

int net_rpc_changetrustpw(int argc, const char **argv) 
{
	return run_rpc_command(NULL, PI_NETLOGON, NET_FLAGS_ANONYMOUS | NET_FLAGS_PDC, 
			       rpc_changetrustpw_internals,
			       argc, argv);
}

/** 
 * Join a domain, the old way.
 *
 * This uses 'machinename' as the inital password, and changes it. 
 *
 * The password should be created with 'server manager' or equiv first.
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid aquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on compleation of the function.
 * @param argc  Standard main() style argc
 * @param argc  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_oldjoin_internals(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli, 
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv)
{
	
	fstring trust_passwd;
	unsigned char orig_trust_passwd_hash[16];
	NTSTATUS result;
	uint32 sec_channel_type;

	pipe_hnd = cli_rpc_pipe_open_noauth(cli, PI_NETLOGON, &result);
	if (!pipe_hnd) {
		DEBUG(0,("rpc_oldjoin_internals: netlogon pipe open to machine %s failed. "
			"error was %s\n",
			cli->desthost,
			nt_errstr(result) ));
		return result;
	}

	/* 
	   check what type of join - if the user want's to join as
	   a BDC, the server must agree that we are a BDC.
	*/
	if (argc >= 0) {
		sec_channel_type = get_sec_channel_type(argv[0]);
	} else {
		sec_channel_type = get_sec_channel_type(NULL);
	}
	
	fstrcpy(trust_passwd, global_myname());
	strlower_m(trust_passwd);

	/*
	 * Machine names can be 15 characters, but the max length on
	 * a password is 14.  --jerry
	 */

	trust_passwd[14] = '\0';

	E_md4hash(trust_passwd, orig_trust_passwd_hash);

	result = trust_pw_change_and_store_it(pipe_hnd, mem_ctx, opt_target_workgroup,
					      orig_trust_passwd_hash,
					      sec_channel_type);

	if (NT_STATUS_IS_OK(result))
		printf("Joined domain %s.\n",opt_target_workgroup);


	if (!secrets_store_domain_sid(opt_target_workgroup, domain_sid)) {
		DEBUG(0, ("error storing domain sid for %s\n", opt_target_workgroup));
		result = NT_STATUS_UNSUCCESSFUL;
	}

	return result;
}

/** 
 * Join a domain, the old way.
 *
 * @param argc  Standard main() style argc
 * @param argc  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/

static int net_rpc_perform_oldjoin(int argc, const char **argv)
{
	return run_rpc_command(NULL, PI_NETLOGON, 
			       NET_FLAGS_NO_PIPE | NET_FLAGS_ANONYMOUS | NET_FLAGS_PDC, 
			       rpc_oldjoin_internals,
			       argc, argv);
}

/** 
 * Join a domain, the old way.  This function exists to allow
 * the message to be displayed when oldjoin was explicitly 
 * requested, but not when it was implied by "net rpc join"
 *
 * @param argc  Standard main() style argc
 * @param argc  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/

static int net_rpc_oldjoin(int argc, const char **argv) 
{
	int rc = net_rpc_perform_oldjoin(argc, argv);

	if (rc) {
		d_fprintf(stderr, "Failed to join domain\n");
	}

	return rc;
}

/** 
 * Basic usage function for 'net rpc join'
 * @param argc  Standard main() style argc
 * @param argc  Standard main() style argv.  Initial components are already
 *              stripped
 **/

static int rpc_join_usage(int argc, const char **argv) 
{	
	d_printf("net rpc join -U <username>[%%password] <type>[options]\n"\
		 "\t to join a domain with admin username & password\n"\
		 "\t\t password will be prompted if needed and none is specified\n"\
		 "\t <type> can be (default MEMBER)\n"\
		 "\t\t BDC - Join as a BDC\n"\
		 "\t\t PDC - Join as a PDC\n"\
		 "\t\t MEMBER - Join as a MEMBER server\n");

	net_common_flags_usage(argc, argv);
	return -1;
}

/** 
 * 'net rpc join' entrypoint.
 * @param argc  Standard main() style argc
 * @param argc  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * Main 'net_rpc_join()' (where the admain username/password is used) is 
 * in net_rpc_join.c
 * Try to just change the password, but if that doesn't work, use/prompt
 * for a username/password.
 **/

int net_rpc_join(int argc, const char **argv) 
{
	if (lp_server_role() == ROLE_STANDALONE) {
		d_printf("cannot join as standalone machine\n");
		return -1;
	}

	if (strlen(global_myname()) > 15) {
		d_printf("Our netbios name can be at most 15 chars long, "
			 "\"%s\" is %u chars long\n",
			 global_myname(), (unsigned int)strlen(global_myname()));
		return -1;
	}

	if ((net_rpc_perform_oldjoin(argc, argv) == 0))
		return 0;
	
	return net_rpc_join_newstyle(argc, argv);
}

/** 
 * display info about a rpc domain
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passed through. 
 *
 * @param domain_sid The domain sid acquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

NTSTATUS rpc_info_internals(const DOM_SID *domain_sid,
			const char *domain_name, 
			struct cli_state *cli,
			struct rpc_pipe_client *pipe_hnd,
			TALLOC_CTX *mem_ctx,
			int argc,
			const char **argv)
{
	POLICY_HND connect_pol, domain_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	SAM_UNK_CTR ctr;
	fstring sid_str;

	sid_to_string(sid_str, domain_sid);

	/* Get sam policy handle */	
	result = rpccli_samr_connect(pipe_hnd, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				  &connect_pol);
	if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "Could not connect to SAM: %s\n", nt_errstr(result));
		goto done;
	}
	
	/* Get domain policy handle */
	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      domain_sid, &domain_pol);
	if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "Could not open domain: %s\n", nt_errstr(result));
		goto done;
	}

	ZERO_STRUCT(ctr);
	result = rpccli_samr_query_dom_info(pipe_hnd, mem_ctx, &domain_pol,
					 2, &ctr);
	if (NT_STATUS_IS_OK(result)) {
		TALLOC_CTX *ctx = talloc_init("rpc_info_internals");
		d_printf("Domain Name: %s\n", unistr2_tdup(ctx, &ctr.info.inf2.uni_domain));
		d_printf("Domain SID: %s\n", sid_str);
		d_printf("Sequence number: %u\n", ctr.info.inf2.seq_num.low);
		d_printf("Num users: %u\n", ctr.info.inf2.num_domain_usrs);
		d_printf("Num domain groups: %u\n", ctr.info.inf2.num_domain_grps);
		d_printf("Num local groups: %u\n", ctr.info.inf2.num_local_grps);
		talloc_destroy(ctx);
	}

 done:
	return result;
}

/** 
 * 'net rpc info' entrypoint.
 * @param argc  Standard main() style argc
 * @param argc  Standard main() style argv.  Initial components are already
 *              stripped
 **/

int net_rpc_info(int argc, const char **argv) 
{
	return run_rpc_command(NULL, PI_SAMR, NET_FLAGS_PDC, 
			       rpc_info_internals,
			       argc, argv);
}

/** 
 * Fetch domain SID into the local secrets.tdb
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid acquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_getsid_internals(const DOM_SID *domain_sid,
			const char *domain_name, 
			struct cli_state *cli,
			struct rpc_pipe_client *pipe_hnd,
			TALLOC_CTX *mem_ctx,
			int argc,
			const char **argv)
{
	fstring sid_str;

	sid_to_string(sid_str, domain_sid);
	d_printf("Storing SID %s for Domain %s in secrets.tdb\n",
		 sid_str, domain_name);

	if (!secrets_store_domain_sid(domain_name, domain_sid)) {
		DEBUG(0,("Can't store domain SID\n"));
		return NT_STATUS_UNSUCCESSFUL;
	}

	return NT_STATUS_OK;
}

/** 
 * 'net rpc getsid' entrypoint.
 * @param argc  Standard main() style argc
 * @param argc  Standard main() style argv.  Initial components are already
 *              stripped
 **/

int net_rpc_getsid(int argc, const char **argv) 
{
	return run_rpc_command(NULL, PI_SAMR, NET_FLAGS_ANONYMOUS | NET_FLAGS_PDC, 
			       rpc_getsid_internals,
			       argc, argv);
}

/****************************************************************************/

/**
 * Basic usage function for 'net rpc user'
 * @param argc	Standard main() style argc.
 * @param argv	Standard main() style argv.  Initial components are already
 *		stripped.
 **/

static int rpc_user_usage(int argc, const char **argv)
{
	return net_help_user(argc, argv);
}

/** 
 * Add a new user to a remote RPC server
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid acquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_user_add_internals(const DOM_SID *domain_sid,
				const char *domain_name, 
				struct cli_state *cli,
				struct rpc_pipe_client *pipe_hnd,
				TALLOC_CTX *mem_ctx, 
				int argc, const char **argv)
{
	
	POLICY_HND connect_pol, domain_pol, user_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	const char *acct_name;
	uint32 acb_info;
	uint32 unknown, user_rid;

	if (argc < 1) {
		d_printf("User must be specified\n");
		rpc_user_usage(argc, argv);
		return NT_STATUS_OK;
	}

	acct_name = argv[0];

	/* Get sam policy handle */
	
	result = rpccli_samr_connect(pipe_hnd, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				  &connect_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}
	
	/* Get domain policy handle */
	
	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      domain_sid, &domain_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	/* Create domain user */

	acb_info = ACB_NORMAL;
	unknown = 0xe005000b; /* No idea what this is - a permission mask? */

	result = rpccli_samr_create_dom_user(pipe_hnd, mem_ctx, &domain_pol,
					  acct_name, acb_info, unknown,
					  &user_pol, &user_rid);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	if (argc == 2) {

		uint32 *user_rids, num_rids, *name_types;
		uint32 flags = 0x000003e8; /* Unknown */
		SAM_USERINFO_CTR ctr;
		SAM_USER_INFO_24 p24;
		uchar pwbuf[516];

		result = rpccli_samr_lookup_names(pipe_hnd, mem_ctx, &domain_pol,
						  flags, 1, &acct_name,
						  &num_rids, &user_rids,
						  &name_types);

		if (!NT_STATUS_IS_OK(result)) {
			goto done;
		}

		result = rpccli_samr_open_user(pipe_hnd, mem_ctx, &domain_pol,
					       MAXIMUM_ALLOWED_ACCESS,
					       user_rids[0], &user_pol);

		if (!NT_STATUS_IS_OK(result)) {
			goto done;
		}

		/* Set password on account */

		ZERO_STRUCT(ctr);
		ZERO_STRUCT(p24);

		encode_pw_buffer(pwbuf, argv[1], STR_UNICODE);

		init_sam_user_info24(&p24, (char *)pwbuf,24);

		ctr.switch_value = 24;
		ctr.info.id24 = &p24;

		result = rpccli_samr_set_userinfo(pipe_hnd, mem_ctx, &user_pol, 24, 
					       &cli->user_session_key, &ctr);

		if (!NT_STATUS_IS_OK(result)) {
			d_fprintf(stderr, "Failed to set password for user %s - %s\n", 
				 acct_name, nt_errstr(result));

			result = rpccli_samr_delete_dom_user(pipe_hnd, mem_ctx, &user_pol);

			if (!NT_STATUS_IS_OK(result)) {
				d_fprintf(stderr, "Failed to delete user %s - %s\n", 
					 acct_name, nt_errstr(result));
				 return result;
			}
		}

	}
 done:
	if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "Failed to add user %s - %s\n", acct_name, 
			 nt_errstr(result));
	} else {
		d_printf("Added user %s\n", acct_name);
	}
	return result;
}

/** 
 * Add a new user to a remote RPC server
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/

static int rpc_user_add(int argc, const char **argv) 
{
	return run_rpc_command(NULL, PI_SAMR, 0, rpc_user_add_internals,
			       argc, argv);
}

/** 
 * Delete a user from a remote RPC server
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid acquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_user_del_internals(const DOM_SID *domain_sid, 
					const char *domain_name, 
					struct cli_state *cli, 
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv)
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	POLICY_HND connect_pol, domain_pol, user_pol;

	if (argc < 1) {
		d_printf("User must be specified\n");
		rpc_user_usage(argc, argv);
		return NT_STATUS_OK;
	}
	/* Get sam policy and domain handles */

	result = rpccli_samr_connect(pipe_hnd, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				  &connect_pol);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      domain_sid, &domain_pol);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	/* Get handle on user */

	{
		uint32 *user_rids, num_rids, *name_types;
		uint32 flags = 0x000003e8; /* Unknown */

		result = rpccli_samr_lookup_names(pipe_hnd, mem_ctx, &domain_pol,
					       flags, 1, &argv[0],
					       &num_rids, &user_rids,
					       &name_types);

		if (!NT_STATUS_IS_OK(result)) {
			goto done;
		}

		result = rpccli_samr_open_user(pipe_hnd, mem_ctx, &domain_pol,
					    MAXIMUM_ALLOWED_ACCESS,
					    user_rids[0], &user_pol);

		if (!NT_STATUS_IS_OK(result)) {
			goto done;
		}
	}

	/* Delete user */

	result = rpccli_samr_delete_dom_user(pipe_hnd, mem_ctx, &user_pol);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	/* Display results */
	if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "Failed to delete user account - %s\n", nt_errstr(result));
	} else {
		d_printf("Deleted user account\n");
	}

 done:
	return result;
}

/** 
 * Rename a user on a remote RPC server
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid acquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_user_rename_internals(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv)
{
	POLICY_HND connect_pol, domain_pol, user_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 info_level = 7;
	const char *old_name, *new_name;
	uint32 *user_rid;
	uint32 flags = 0x000003e8; /* Unknown */
	uint32 num_rids, *name_types;
	uint32 num_names = 1;
	const char **names;
	SAM_USERINFO_CTR *user_ctr;
	SAM_USERINFO_CTR ctr;
	SAM_USER_INFO_7 info7;

	if (argc != 2) {
		d_printf("Old and new username must be specified\n");
		rpc_user_usage(argc, argv);
		return NT_STATUS_OK;
	}

	old_name = argv[0];
	new_name = argv[1];

	ZERO_STRUCT(ctr);
	ZERO_STRUCT(user_ctr);

	/* Get sam policy handle */
	
	result = rpccli_samr_connect(pipe_hnd, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				  &connect_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}
	
	/* Get domain policy handle */
	
	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      domain_sid, &domain_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	if ((names = TALLOC_ARRAY(mem_ctx, const char *, num_names)) == NULL) {
		result = NT_STATUS_NO_MEMORY;
		goto done;
	}
	names[0] = old_name;
	result = rpccli_samr_lookup_names(pipe_hnd, mem_ctx, &domain_pol,
				       flags, num_names, names,
				       &num_rids, &user_rid, &name_types);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	/* Open domain user */
	result = rpccli_samr_open_user(pipe_hnd, mem_ctx, &domain_pol,
				    MAXIMUM_ALLOWED_ACCESS, user_rid[0], &user_pol);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	/* Query user info */
	result = rpccli_samr_query_userinfo(pipe_hnd, mem_ctx, &user_pol,
					 info_level, &user_ctr);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	ctr.switch_value = info_level;
	ctr.info.id7 = &info7;

	init_sam_user_info7(&info7, new_name);

	/* Set new name */
	result = rpccli_samr_set_userinfo(pipe_hnd, mem_ctx, &user_pol,
				       info_level, &cli->user_session_key, &ctr);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

 done:
	if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "Failed to rename user from %s to %s - %s\n", old_name, new_name, 
			 nt_errstr(result));
	} else {
		d_printf("Renamed user from %s to %s\n", old_name, new_name);
	}
	return result;
}

/** 
 * Rename a user on a remote RPC server
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/

static int rpc_user_rename(int argc, const char **argv) 
{
	return run_rpc_command(NULL, PI_SAMR, 0, rpc_user_rename_internals,
			       argc, argv);
}

/** 
 * Delete a user from a remote RPC server
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/

static int rpc_user_delete(int argc, const char **argv) 
{
	return run_rpc_command(NULL, PI_SAMR, 0, rpc_user_del_internals,
			       argc, argv);
}

/** 
 * Set a password for a user on a remote RPC server
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid acquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_user_password_internals(const DOM_SID *domain_sid, 
					const char *domain_name, 
					struct cli_state *cli, 
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv)
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	POLICY_HND connect_pol, domain_pol, user_pol;
	SAM_USERINFO_CTR ctr;
	SAM_USER_INFO_24 p24;
	uchar pwbuf[516];
	const char *user;
	const char *new_password;
	char *prompt = NULL;

	if (argc < 1) {
		d_printf("User must be specified\n");
		rpc_user_usage(argc, argv);
		return NT_STATUS_OK;
	}
	
	user = argv[0];

	if (argv[1]) {
		new_password = argv[1];
	} else {
		asprintf(&prompt, "Enter new password for %s:", user);
		new_password = getpass(prompt);
		SAFE_FREE(prompt);
	}

	/* Get sam policy and domain handles */

	result = rpccli_samr_connect(pipe_hnd, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				  &connect_pol);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      domain_sid, &domain_pol);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	/* Get handle on user */

	{
		uint32 *user_rids, num_rids, *name_types;
		uint32 flags = 0x000003e8; /* Unknown */

		result = rpccli_samr_lookup_names(pipe_hnd, mem_ctx, &domain_pol,
					       flags, 1, &user,
					       &num_rids, &user_rids,
					       &name_types);

		if (!NT_STATUS_IS_OK(result)) {
			goto done;
		}

		result = rpccli_samr_open_user(pipe_hnd, mem_ctx, &domain_pol,
					    MAXIMUM_ALLOWED_ACCESS,
					    user_rids[0], &user_pol);

		if (!NT_STATUS_IS_OK(result)) {
			goto done;
		}
	}

	/* Set password on account */

	ZERO_STRUCT(ctr);
	ZERO_STRUCT(p24);

	encode_pw_buffer(pwbuf, new_password, STR_UNICODE);

	init_sam_user_info24(&p24, (char *)pwbuf,24);

	ctr.switch_value = 24;
	ctr.info.id24 = &p24;

	result = rpccli_samr_set_userinfo(pipe_hnd, mem_ctx, &user_pol, 24, 
				       &cli->user_session_key, &ctr);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	/* Display results */

 done:
	return result;

}	

/** 
 * Set a user's password on a remote RPC server
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/

static int rpc_user_password(int argc, const char **argv) 
{
	return run_rpc_command(NULL, PI_SAMR, 0, rpc_user_password_internals,
			       argc, argv);
}

/** 
 * List user's groups on a remote RPC server
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid acquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_user_info_internals(const DOM_SID *domain_sid,
			const char *domain_name, 
			struct cli_state *cli,
			struct rpc_pipe_client *pipe_hnd,
			TALLOC_CTX *mem_ctx,
			int argc,
			const char **argv)
{
	POLICY_HND connect_pol, domain_pol, user_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 *rids, num_rids, *name_types, num_names;
	uint32 flags = 0x000003e8; /* Unknown */
	int i;
	char **names;
	DOM_GID *user_gids;

	if (argc < 1) {
		d_printf("User must be specified\n");
		rpc_user_usage(argc, argv);
		return NT_STATUS_OK;
	}
	/* Get sam policy handle */
	
	result = rpccli_samr_connect(pipe_hnd, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				  &connect_pol);
	if (!NT_STATUS_IS_OK(result)) goto done;
	
	/* Get domain policy handle */
	
	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      domain_sid, &domain_pol);
	if (!NT_STATUS_IS_OK(result)) goto done;

	/* Get handle on user */

	result = rpccli_samr_lookup_names(pipe_hnd, mem_ctx, &domain_pol,
				       flags, 1, &argv[0],
				       &num_rids, &rids, &name_types);

	if (!NT_STATUS_IS_OK(result)) goto done;

	result = rpccli_samr_open_user(pipe_hnd, mem_ctx, &domain_pol,
				    MAXIMUM_ALLOWED_ACCESS,
				    rids[0], &user_pol);
	if (!NT_STATUS_IS_OK(result)) goto done;

	result = rpccli_samr_query_usergroups(pipe_hnd, mem_ctx, &user_pol,
					   &num_rids, &user_gids);

	if (!NT_STATUS_IS_OK(result)) goto done;

	/* Look up rids */

	if (num_rids) {
		if ((rids = TALLOC_ARRAY(mem_ctx, uint32, num_rids)) == NULL) {
			result = NT_STATUS_NO_MEMORY;
			goto done;
		}

		for (i = 0; i < num_rids; i++)
                	rids[i] = user_gids[i].g_rid;

		result = rpccli_samr_lookup_rids(pipe_hnd, mem_ctx, &domain_pol,
				      	      num_rids, rids,
				      	      &num_names, &names, &name_types);

		if (!NT_STATUS_IS_OK(result)) {
			goto done;
		}

		/* Display results */

		for (i = 0; i < num_names; i++)
			printf("%s\n", names[i]);
	}
 done:
	return result;
}

/** 
 * List a user's groups from a remote RPC server
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/

static int rpc_user_info(int argc, const char **argv) 
{
	return run_rpc_command(NULL, PI_SAMR, 0, rpc_user_info_internals,
			       argc, argv);
}

/** 
 * List users on a remote RPC server
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid acquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_user_list_internals(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					int argc,
					const char **argv)
{
	POLICY_HND connect_pol, domain_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 start_idx=0, num_entries, i, loop_count = 0;
	SAM_DISPINFO_CTR ctr;
	SAM_DISPINFO_1 info1;

	/* Get sam policy handle */
	
	result = rpccli_samr_connect(pipe_hnd, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				  &connect_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}
	
	/* Get domain policy handle */
	
	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      domain_sid, &domain_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	/* Query domain users */
	ZERO_STRUCT(ctr);
	ZERO_STRUCT(info1);
	ctr.sam.info1 = &info1;
	if (opt_long_list_entries)
		d_printf("\nUser name             Comment"\
			 "\n-----------------------------\n");
	do {
		fstring user, desc;
		uint32 max_entries, max_size;

		get_query_dispinfo_params(
			loop_count, &max_entries, &max_size);

		result = rpccli_samr_query_dispinfo(pipe_hnd, mem_ctx, &domain_pol,
						 &start_idx, 1, &num_entries,
						 max_entries, max_size, &ctr);
		loop_count++;

		for (i = 0; i < num_entries; i++) {
			unistr2_to_ascii(user, &(&ctr.sam.info1->str[i])->uni_acct_name, sizeof(user)-1);
			if (opt_long_list_entries) 
				unistr2_to_ascii(desc, &(&ctr.sam.info1->str[i])->uni_acct_desc, sizeof(desc)-1);
			
			if (opt_long_list_entries)
				printf("%-21.21s %s\n", user, desc);
			else
				printf("%s\n", user);
		}
	} while (NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES));

 done:
	return result;
}

/** 
 * 'net rpc user' entrypoint.
 * @param argc  Standard main() style argc
 * @param argc  Standard main() style argv.  Initial components are already
 *              stripped
 **/

int net_rpc_user(int argc, const char **argv) 
{
	struct functable func[] = {
		{"add", rpc_user_add},
		{"info", rpc_user_info},
		{"delete", rpc_user_delete},
		{"password", rpc_user_password},
		{"rename", rpc_user_rename},
		{NULL, NULL}
	};
	
	if (argc == 0) {
		return run_rpc_command(NULL,PI_SAMR, 0, 
				       rpc_user_list_internals,
				       argc, argv);
	}

	return net_run_function(argc, argv, func, rpc_user_usage);
}

static NTSTATUS rpc_sh_user_list(TALLOC_CTX *mem_ctx,
				 struct rpc_sh_ctx *ctx,
				 struct rpc_pipe_client *pipe_hnd,
				 int argc, const char **argv)
{
	return rpc_user_list_internals(ctx->domain_sid, ctx->domain_name,
				       ctx->cli, pipe_hnd, mem_ctx,
				       argc, argv);
}

static NTSTATUS rpc_sh_user_info(TALLOC_CTX *mem_ctx,
				 struct rpc_sh_ctx *ctx,
				 struct rpc_pipe_client *pipe_hnd,
				 int argc, const char **argv)
{
	return rpc_user_info_internals(ctx->domain_sid, ctx->domain_name,
				       ctx->cli, pipe_hnd, mem_ctx,
				       argc, argv);
}

static NTSTATUS rpc_sh_handle_user(TALLOC_CTX *mem_ctx,
				   struct rpc_sh_ctx *ctx,
				   struct rpc_pipe_client *pipe_hnd,
				   int argc, const char **argv,
				   NTSTATUS (*fn)(
					   TALLOC_CTX *mem_ctx,
					   struct rpc_sh_ctx *ctx,
					   struct rpc_pipe_client *pipe_hnd,
					   const POLICY_HND *user_hnd,
					   int argc, const char **argv))
					   
{
	POLICY_HND connect_pol, domain_pol, user_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	DOM_SID sid;
	uint32 rid;
	enum SID_NAME_USE type;

	if (argc == 0) {
		d_fprintf(stderr, "usage: %s <username>\n", ctx->whoami);
		return NT_STATUS_INVALID_PARAMETER;
	}

	ZERO_STRUCT(connect_pol);
	ZERO_STRUCT(domain_pol);
	ZERO_STRUCT(user_pol);

	result = net_rpc_lookup_name(mem_ctx, pipe_hnd->cli, argv[0],
				     NULL, NULL, &sid, &type);
	if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "Could not lookup %s: %s\n", argv[0],
			  nt_errstr(result));
		goto done;
	}

	if (type != SID_NAME_USER) {
		d_fprintf(stderr, "%s is a %s, not a user\n", argv[0],
			  sid_type_lookup(type));
		result = NT_STATUS_NO_SUCH_USER;
		goto done;
	}

	if (!sid_peek_check_rid(ctx->domain_sid, &sid, &rid)) {
		d_fprintf(stderr, "%s is not in our domain\n", argv[0]);
		result = NT_STATUS_NO_SUCH_USER;
		goto done;
	}

	result = rpccli_samr_connect(pipe_hnd, mem_ctx,
				     MAXIMUM_ALLOWED_ACCESS, &connect_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
					 MAXIMUM_ALLOWED_ACCESS,
					 ctx->domain_sid, &domain_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_samr_open_user(pipe_hnd, mem_ctx, &domain_pol,
				       MAXIMUM_ALLOWED_ACCESS,
				       rid, &user_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = fn(mem_ctx, ctx, pipe_hnd, &user_pol, argc-1, argv+1);

 done:
	if (is_valid_policy_hnd(&user_pol)) {
		rpccli_samr_close(pipe_hnd, mem_ctx, &user_pol);
	}
	if (is_valid_policy_hnd(&domain_pol)) {
		rpccli_samr_close(pipe_hnd, mem_ctx, &domain_pol);
	}
	if (is_valid_policy_hnd(&connect_pol)) {
		rpccli_samr_close(pipe_hnd, mem_ctx, &connect_pol);
	}
	return result;
}

static NTSTATUS rpc_sh_user_show_internals(TALLOC_CTX *mem_ctx,
					   struct rpc_sh_ctx *ctx,
					   struct rpc_pipe_client *pipe_hnd,
					   const POLICY_HND *user_hnd,
					   int argc, const char **argv)
{
	NTSTATUS result;
	SAM_USERINFO_CTR *ctr;
	SAM_USER_INFO_21 *info;

	if (argc != 0) {
		d_fprintf(stderr, "usage: %s show <username>\n", ctx->whoami);
		return NT_STATUS_INVALID_PARAMETER;
	}

	result = rpccli_samr_query_userinfo(pipe_hnd, mem_ctx, user_hnd,
					    21, &ctr);
	if (!NT_STATUS_IS_OK(result)) {
		return result;
	}

	info = ctr->info.id21;

	d_printf("user rid: %d, group rid: %d\n", info->user_rid,
		 info->group_rid);

	return result;
}

static NTSTATUS rpc_sh_user_show(TALLOC_CTX *mem_ctx,
				 struct rpc_sh_ctx *ctx,
				 struct rpc_pipe_client *pipe_hnd,
				 int argc, const char **argv)
{
	return rpc_sh_handle_user(mem_ctx, ctx, pipe_hnd, argc, argv,
				  rpc_sh_user_show_internals);
}

#define FETCHSTR(name, rec) \
do { if (strequal(ctx->thiscmd, name)) { \
	oldval = rpcstr_pull_unistr2_talloc(mem_ctx, &usr->uni_##rec); } \
} while (0);

#define SETSTR(name, rec, flag) \
do { if (strequal(ctx->thiscmd, name)) { \
	init_unistr2(&usr->uni_##rec, argv[0], UNI_STR_TERMINATE); \
	init_uni_hdr(&usr->hdr_##rec, &usr->uni_##rec); \
	usr->fields_present |= ACCT_##flag; } \
} while (0);

static NTSTATUS rpc_sh_user_str_edit_internals(TALLOC_CTX *mem_ctx,
					       struct rpc_sh_ctx *ctx,
					       struct rpc_pipe_client *pipe_hnd,
					       const POLICY_HND *user_hnd,
					       int argc, const char **argv)
{
	NTSTATUS result;
	SAM_USERINFO_CTR *ctr;
	SAM_USER_INFO_21 *usr;
	const char *username;
	const char *oldval = "";

	if (argc > 1) {
		d_fprintf(stderr, "usage: %s <username> [new value|NULL]\n",
			  ctx->whoami);
		return NT_STATUS_INVALID_PARAMETER;
	}

	result = rpccli_samr_query_userinfo(pipe_hnd, mem_ctx, user_hnd,
					    21, &ctr);
	if (!NT_STATUS_IS_OK(result)) {
		return result;
	}

	usr = ctr->info.id21;

	username = rpcstr_pull_unistr2_talloc(mem_ctx, &usr->uni_user_name);

	FETCHSTR("fullname", full_name);
	FETCHSTR("homedir", home_dir);
	FETCHSTR("homedrive", dir_drive);
	FETCHSTR("logonscript", logon_script);
	FETCHSTR("profilepath", profile_path);
	FETCHSTR("description", acct_desc);

	if (argc == 0) {
		d_printf("%s's %s: [%s]\n", username, ctx->thiscmd, oldval);
		goto done;
	}

	ZERO_STRUCTP(usr);

	if (strcmp(argv[0], "NULL") == 0) {
		argv[0] = "";
	}

	SETSTR("fullname", full_name, FULL_NAME);
	SETSTR("homedir", home_dir, HOME_DIR);
	SETSTR("homedrive", dir_drive, HOME_DRIVE);
	SETSTR("logonscript", logon_script, LOGON_SCRIPT);
	SETSTR("profilepath", profile_path, PROFILE);
	SETSTR("description", acct_desc, DESCRIPTION);

	result = rpccli_samr_set_userinfo2(
		pipe_hnd, mem_ctx, user_hnd, 21,
		&pipe_hnd->cli->user_session_key, ctr);

	d_printf("Set %s's %s from [%s] to [%s]\n", username,
		 ctx->thiscmd, oldval, argv[0]);

 done:

	return result;
}

#define HANDLEFLG(name, rec) \
do { if (strequal(ctx->thiscmd, name)) { \
	oldval = (oldflags & ACB_##rec) ? "yes" : "no"; \
	if (newval) { \
		newflags = oldflags | ACB_##rec; \
	} else { \
		newflags = oldflags & ~ACB_##rec; \
	} } } while (0);

static NTSTATUS rpc_sh_user_str_edit(TALLOC_CTX *mem_ctx,
				     struct rpc_sh_ctx *ctx,
				     struct rpc_pipe_client *pipe_hnd,
				     int argc, const char **argv)
{
	return rpc_sh_handle_user(mem_ctx, ctx, pipe_hnd, argc, argv,
				  rpc_sh_user_str_edit_internals);
}

static NTSTATUS rpc_sh_user_flag_edit_internals(TALLOC_CTX *mem_ctx,
						struct rpc_sh_ctx *ctx,
						struct rpc_pipe_client *pipe_hnd,
						const POLICY_HND *user_hnd,
						int argc, const char **argv)
{
	NTSTATUS result;
	SAM_USERINFO_CTR *ctr;
	SAM_USER_INFO_21 *usr;
	const char *username;
	const char *oldval = "unknown";
	uint32 oldflags, newflags;
	BOOL newval;

	if ((argc > 1) ||
	    ((argc == 1) && !strequal(argv[0], "yes") &&
	     !strequal(argv[0], "no"))) {
		d_fprintf(stderr, "usage: %s <username> [yes|no]\n",
			  ctx->whoami);
		return NT_STATUS_INVALID_PARAMETER;
	}

	newval = strequal(argv[0], "yes");

	result = rpccli_samr_query_userinfo(pipe_hnd, mem_ctx, user_hnd,
					    21, &ctr);
	if (!NT_STATUS_IS_OK(result)) {
		return result;
	}

	usr = ctr->info.id21;

	username = rpcstr_pull_unistr2_talloc(mem_ctx, &usr->uni_user_name);
	oldflags = usr->acb_info;
	newflags = usr->acb_info;

	HANDLEFLG("disabled", DISABLED);
	HANDLEFLG("pwnotreq", PWNOTREQ);
	HANDLEFLG("autolock", AUTOLOCK);
	HANDLEFLG("pwnoexp", PWNOEXP);

	if (argc == 0) {
		d_printf("%s's %s flag: %s\n", username, ctx->thiscmd, oldval);
		goto done;
	}

	ZERO_STRUCTP(usr);

	usr->acb_info = newflags;
	usr->fields_present = ACCT_FLAGS;

	result = rpccli_samr_set_userinfo2(
		pipe_hnd, mem_ctx, user_hnd, 21,
		&pipe_hnd->cli->user_session_key, ctr);

	if (NT_STATUS_IS_OK(result)) {
		d_printf("Set %s's %s flag from [%s] to [%s]\n", username,
			 ctx->thiscmd, oldval, argv[0]);
	}

 done:

	return result;
}

static NTSTATUS rpc_sh_user_flag_edit(TALLOC_CTX *mem_ctx,
				      struct rpc_sh_ctx *ctx,
				      struct rpc_pipe_client *pipe_hnd,
				      int argc, const char **argv)
{
	return rpc_sh_handle_user(mem_ctx, ctx, pipe_hnd, argc, argv,
				  rpc_sh_user_flag_edit_internals);
}

struct rpc_sh_cmd *net_rpc_user_edit_cmds(TALLOC_CTX *mem_ctx,
					  struct rpc_sh_ctx *ctx)
{
	static struct rpc_sh_cmd cmds[] = {

		{ "fullname", NULL, PI_SAMR, rpc_sh_user_str_edit,
		  "Show/Set a user's full name" },

		{ "homedir", NULL, PI_SAMR, rpc_sh_user_str_edit,
		  "Show/Set a user's home directory" },

		{ "homedrive", NULL, PI_SAMR, rpc_sh_user_str_edit,
		  "Show/Set a user's home drive" },

		{ "logonscript", NULL, PI_SAMR, rpc_sh_user_str_edit,
		  "Show/Set a user's logon script" },

		{ "profilepath", NULL, PI_SAMR, rpc_sh_user_str_edit,
		  "Show/Set a user's profile path" },

		{ "description", NULL, PI_SAMR, rpc_sh_user_str_edit,
		  "Show/Set a user's description" },

		{ "disabled", NULL, PI_SAMR, rpc_sh_user_flag_edit,
		  "Show/Set whether a user is disabled" },

		{ "autolock", NULL, PI_SAMR, rpc_sh_user_flag_edit,
		  "Show/Set whether a user locked out" },

		{ "pwnotreq", NULL, PI_SAMR, rpc_sh_user_flag_edit,
		  "Show/Set whether a user does not need a password" },

		{ "pwnoexp", NULL, PI_SAMR, rpc_sh_user_flag_edit,
		  "Show/Set whether a user's password does not expire" },

		{ NULL, NULL, 0, NULL, NULL }
	};

	return cmds;
}

struct rpc_sh_cmd *net_rpc_user_cmds(TALLOC_CTX *mem_ctx,
				     struct rpc_sh_ctx *ctx)
{
	static struct rpc_sh_cmd cmds[] = {

		{ "list", NULL, PI_SAMR, rpc_sh_user_list,
		  "List available users" },

		{ "info", NULL, PI_SAMR, rpc_sh_user_info,
		  "List the domain groups a user is member of" },

		{ "show", NULL, PI_SAMR, rpc_sh_user_show,
		  "Show info about a user" },

		{ "edit", net_rpc_user_edit_cmds, 0, NULL, 
		  "Show/Modify a user's fields" },

		{ NULL, NULL, 0, NULL, NULL }
	};

	return cmds;
}

/****************************************************************************/

/**
 * Basic usage function for 'net rpc group'
 * @param argc	Standard main() style argc.
 * @param argv	Standard main() style argv.  Initial components are already
 *		stripped.
 **/

static int rpc_group_usage(int argc, const char **argv)
{
	return net_help_group(argc, argv);
}

/**
 * Delete group on a remote RPC server
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through.
 *
 * @param domain_sid The domain sid acquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/
                                                                                                             
static NTSTATUS rpc_group_delete_internals(const DOM_SID *domain_sid,
					const char *domain_name,
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					int argc,
					const char **argv)
{
	POLICY_HND connect_pol, domain_pol, group_pol, user_pol;
	BOOL group_is_primary = False;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	uint32 *group_rids, num_rids, *name_types, num_members, 
               *group_attrs, group_rid;
	uint32 flags = 0x000003e8; /* Unknown */
	/* char **names; */
	int i;
	/* DOM_GID *user_gids; */
	SAM_USERINFO_CTR *user_ctr;
	fstring temp;

	if (argc < 1) {
        	d_printf("specify group\n");
		rpc_group_usage(argc,argv);
		return NT_STATUS_OK; /* ok? */
	}

        result = rpccli_samr_connect(pipe_hnd, mem_ctx, MAXIMUM_ALLOWED_ACCESS,
                                  &connect_pol);

        if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "Request samr_connect failed\n");
        	goto done;
        }
        
        result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
                                      MAXIMUM_ALLOWED_ACCESS,
                                      domain_sid, &domain_pol);
        
        if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "Request open_domain failed\n");
        	goto done;
        }
	
	result = rpccli_samr_lookup_names(pipe_hnd, mem_ctx, &domain_pol,
				       flags, 1, &argv[0],
				       &num_rids, &group_rids,
				       &name_types);

	if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "Lookup of '%s' failed\n",argv[0]);
   		goto done;
	}

	switch (name_types[0])
	{
	case SID_NAME_DOM_GRP:
		result = rpccli_samr_open_group(pipe_hnd, mem_ctx, &domain_pol,
					     MAXIMUM_ALLOWED_ACCESS,
					     group_rids[0], &group_pol);
		if (!NT_STATUS_IS_OK(result)) {
			d_fprintf(stderr, "Request open_group failed");
   			goto done;
		}
                
		group_rid = group_rids[0];
                
		result = rpccli_samr_query_groupmem(pipe_hnd, mem_ctx, &group_pol,
                                 &num_members, &group_rids,
                                 &group_attrs);
		
		if (!NT_STATUS_IS_OK(result)) {
			d_fprintf(stderr, "Unable to query group members of %s",argv[0]);
   			goto done;
		}
		
		if (opt_verbose) {
			d_printf("Domain Group %s (rid: %d) has %d members\n",
				argv[0],group_rid,num_members);
		}

		/* Check if group is anyone's primary group */
                for (i = 0; i < num_members; i++)
		{
	                result = rpccli_samr_open_user(pipe_hnd, mem_ctx, &domain_pol,
					            MAXIMUM_ALLOWED_ACCESS,
					            group_rids[i], &user_pol);
	
	        	if (!NT_STATUS_IS_OK(result)) {
				d_fprintf(stderr, "Unable to open group member %d\n",group_rids[i]);
	           		goto done;
	        	}
	
	                ZERO_STRUCT(user_ctr);

	                result = rpccli_samr_query_userinfo(pipe_hnd, mem_ctx, &user_pol,
	                                                 21, &user_ctr);
	
	        	if (!NT_STATUS_IS_OK(result)) {
				d_fprintf(stderr, "Unable to lookup userinfo for group member %d\n",group_rids[i]);
	           		goto done;
	        	}
	
			if (user_ctr->info.id21->group_rid == group_rid) {
				unistr2_to_ascii(temp, &(user_ctr->info.id21)->uni_user_name, 
						sizeof(temp)-1);
				if (opt_verbose) 
					d_printf("Group is primary group of %s\n",temp);
				group_is_primary = True;
                        }

			rpccli_samr_close(pipe_hnd, mem_ctx, &user_pol);
		}
                
		if (group_is_primary) {
			d_fprintf(stderr, "Unable to delete group because some "
				 "of it's members have it as primary group\n");
			result = NT_STATUS_MEMBERS_PRIMARY_GROUP;
			goto done;
		}
     
		/* remove all group members */
		for (i = 0; i < num_members; i++)
		{
			if (opt_verbose) 
				d_printf("Remove group member %d...",group_rids[i]);
			result = rpccli_samr_del_groupmem(pipe_hnd, mem_ctx, &group_pol, group_rids[i]);

			if (NT_STATUS_IS_OK(result)) {
				if (opt_verbose)
					d_printf("ok\n");
			} else {
				if (opt_verbose)
					d_printf("failed\n");
				goto done;
			}	
		}

		result = rpccli_samr_delete_dom_group(pipe_hnd, mem_ctx, &group_pol);

		break;
	/* removing a local group is easier... */
	case SID_NAME_ALIAS:
		result = rpccli_samr_open_alias(pipe_hnd, mem_ctx, &domain_pol,
					     MAXIMUM_ALLOWED_ACCESS,
					     group_rids[0], &group_pol);

		if (!NT_STATUS_IS_OK(result)) {
			d_fprintf(stderr, "Request open_alias failed\n");
   			goto done;
		}
		
		result = rpccli_samr_delete_dom_alias(pipe_hnd, mem_ctx, &group_pol);
		break;
	default:
		d_fprintf(stderr, "%s is of type %s. This command is only for deleting local or global groups\n",
			argv[0],sid_type_lookup(name_types[0]));
		result = NT_STATUS_UNSUCCESSFUL;
		goto done;
	}
         
	
	if (NT_STATUS_IS_OK(result)) {
		if (opt_verbose)
			d_printf("Deleted %s '%s'\n",sid_type_lookup(name_types[0]),argv[0]);
	} else {
		d_fprintf(stderr, "Deleting of %s failed: %s\n",argv[0],
			get_friendly_nt_error_msg(result));
	}
	
 done:
	return result;	
        
}

static int rpc_group_delete(int argc, const char **argv)
{
	return run_rpc_command(NULL, PI_SAMR, 0, rpc_group_delete_internals,
                               argc,argv);
}

static NTSTATUS rpc_group_add_internals(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					int argc,
					const char **argv)
{
	POLICY_HND connect_pol, domain_pol, group_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	GROUP_INFO_CTR group_info;

	if (argc != 1) {
		d_printf("Group name must be specified\n");
		rpc_group_usage(argc, argv);
		return NT_STATUS_OK;
	}

	/* Get sam policy handle */
	
	result = rpccli_samr_connect(pipe_hnd, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				  &connect_pol);
	if (!NT_STATUS_IS_OK(result)) goto done;
	
	/* Get domain policy handle */
	
	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      domain_sid, &domain_pol);
	if (!NT_STATUS_IS_OK(result)) goto done;

	/* Create the group */

	result = rpccli_samr_create_dom_group(pipe_hnd, mem_ctx, &domain_pol,
					   argv[0], MAXIMUM_ALLOWED_ACCESS,
					   &group_pol);
	if (!NT_STATUS_IS_OK(result)) goto done;

	if (strlen(opt_comment) == 0) goto done;

	/* We've got a comment to set */

	group_info.switch_value1 = 4;
	init_samr_group_info4(&group_info.group.info4, opt_comment);

	result = rpccli_samr_set_groupinfo(pipe_hnd, mem_ctx, &group_pol, &group_info);
	if (!NT_STATUS_IS_OK(result)) goto done;
	
 done:
	if (NT_STATUS_IS_OK(result))
		DEBUG(5, ("add group succeeded\n"));
	else
		d_fprintf(stderr, "add group failed: %s\n", nt_errstr(result));

	return result;
}

static NTSTATUS rpc_alias_add_internals(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					int argc,
					const char **argv)
{
	POLICY_HND connect_pol, domain_pol, alias_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	ALIAS_INFO_CTR alias_info;

	if (argc != 1) {
		d_printf("Alias name must be specified\n");
		rpc_group_usage(argc, argv);
		return NT_STATUS_OK;
	}

	/* Get sam policy handle */
	
	result = rpccli_samr_connect(pipe_hnd, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				  &connect_pol);
	if (!NT_STATUS_IS_OK(result)) goto done;
	
	/* Get domain policy handle */
	
	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      domain_sid, &domain_pol);
	if (!NT_STATUS_IS_OK(result)) goto done;

	/* Create the group */

	result = rpccli_samr_create_dom_alias(pipe_hnd, mem_ctx, &domain_pol,
					   argv[0], &alias_pol);
	if (!NT_STATUS_IS_OK(result)) goto done;

	if (strlen(opt_comment) == 0) goto done;

	/* We've got a comment to set */

	alias_info.level = 3;
	init_samr_alias_info3(&alias_info.alias.info3, opt_comment);

	result = rpccli_samr_set_aliasinfo(pipe_hnd, mem_ctx, &alias_pol, &alias_info);
	if (!NT_STATUS_IS_OK(result)) goto done;
	
 done:
	if (NT_STATUS_IS_OK(result))
		DEBUG(5, ("add alias succeeded\n"));
	else
		d_fprintf(stderr, "add alias failed: %s\n", nt_errstr(result));

	return result;
}

static int rpc_group_add(int argc, const char **argv)
{
	if (opt_localgroup)
		return run_rpc_command(NULL, PI_SAMR, 0,
				       rpc_alias_add_internals,
				       argc, argv);

	return run_rpc_command(NULL, PI_SAMR, 0,
			       rpc_group_add_internals,
			       argc, argv);
}

static NTSTATUS get_sid_from_name(struct cli_state *cli,
				TALLOC_CTX *mem_ctx,
				const char *name,
				DOM_SID *sid,
				enum SID_NAME_USE *type)
{
	DOM_SID *sids = NULL;
	uint32 *types = NULL;
	struct rpc_pipe_client *pipe_hnd;
	POLICY_HND lsa_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	pipe_hnd = cli_rpc_pipe_open_noauth(cli, PI_LSARPC, &result);
	if (!pipe_hnd) {
		goto done;
	}

	result = rpccli_lsa_open_policy(pipe_hnd, mem_ctx, False,
				     SEC_RIGHTS_MAXIMUM_ALLOWED, &lsa_pol);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_lsa_lookup_names(pipe_hnd, mem_ctx, &lsa_pol, 1,
				      &name, NULL, &sids, &types);

	if (NT_STATUS_IS_OK(result)) {
		sid_copy(sid, &sids[0]);
		*type = types[0];
	}

	rpccli_lsa_close(pipe_hnd, mem_ctx, &lsa_pol);

 done:
	if (pipe_hnd) {
		cli_rpc_pipe_close(pipe_hnd);
	}

	if (!NT_STATUS_IS_OK(result) && (StrnCaseCmp(name, "S-", 2) == 0)) {

		/* Try as S-1-5-whatever */

		DOM_SID tmp_sid;

		if (string_to_sid(&tmp_sid, name)) {
			sid_copy(sid, &tmp_sid);
			*type = SID_NAME_UNKNOWN;
			result = NT_STATUS_OK;
		}
	}

	return result;
}

static NTSTATUS rpc_add_groupmem(struct rpc_pipe_client *pipe_hnd,
				TALLOC_CTX *mem_ctx,
				const DOM_SID *group_sid,
				const char *member)
{
	POLICY_HND connect_pol, domain_pol;
	NTSTATUS result;
	uint32 group_rid;
	POLICY_HND group_pol;

	uint32 num_rids;
	uint32 *rids = NULL;
	uint32 *rid_types = NULL;

	DOM_SID sid;

	sid_copy(&sid, group_sid);

	if (!sid_split_rid(&sid, &group_rid)) {
		return NT_STATUS_UNSUCCESSFUL;
	}

	/* Get sam policy handle */	
	result = rpccli_samr_connect(pipe_hnd, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				  &connect_pol);
	if (!NT_STATUS_IS_OK(result)) {
		return result;
	}
	
	/* Get domain policy handle */
	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      &sid, &domain_pol);
	if (!NT_STATUS_IS_OK(result)) {
		return result;
	}

	result = rpccli_samr_lookup_names(pipe_hnd, mem_ctx, &domain_pol, 1000,
				       1, &member,
				       &num_rids, &rids, &rid_types);

	if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "Could not lookup up group member %s\n", member);
		goto done;
	}

	result = rpccli_samr_open_group(pipe_hnd, mem_ctx, &domain_pol,
				     MAXIMUM_ALLOWED_ACCESS,
				     group_rid, &group_pol);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_samr_add_groupmem(pipe_hnd, mem_ctx, &group_pol, rids[0]);

 done:
	rpccli_samr_close(pipe_hnd, mem_ctx, &connect_pol);
	return result;
}

static NTSTATUS rpc_add_aliasmem(struct rpc_pipe_client *pipe_hnd,
				TALLOC_CTX *mem_ctx,
				const DOM_SID *alias_sid,
				const char *member)
{
	POLICY_HND connect_pol, domain_pol;
	NTSTATUS result;
	uint32 alias_rid;
	POLICY_HND alias_pol;

	DOM_SID member_sid;
	enum SID_NAME_USE member_type;

	DOM_SID sid;

	sid_copy(&sid, alias_sid);

	if (!sid_split_rid(&sid, &alias_rid)) {
		return NT_STATUS_UNSUCCESSFUL;
	}

	result = get_sid_from_name(pipe_hnd->cli, mem_ctx, member,
				   &member_sid, &member_type);

	if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "Could not lookup up group member %s\n", member);
		return result;
	}

	/* Get sam policy handle */	
	result = rpccli_samr_connect(pipe_hnd, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				  &connect_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}
	
	/* Get domain policy handle */
	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      &sid, &domain_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_samr_open_alias(pipe_hnd, mem_ctx, &domain_pol,
				     MAXIMUM_ALLOWED_ACCESS,
				     alias_rid, &alias_pol);

	if (!NT_STATUS_IS_OK(result)) {
		return result;
	}

	result = rpccli_samr_add_aliasmem(pipe_hnd, mem_ctx, &alias_pol, &member_sid);

	if (!NT_STATUS_IS_OK(result)) {
		return result;
	}

 done:
	rpccli_samr_close(pipe_hnd, mem_ctx, &connect_pol);
	return result;
}

static NTSTATUS rpc_group_addmem_internals(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					int argc,
					const char **argv)
{
	DOM_SID group_sid;
	enum SID_NAME_USE group_type;

	if (argc != 2) {
		d_printf("Usage: 'net rpc group addmem <group> <member>\n");
		return NT_STATUS_UNSUCCESSFUL;
	}

	if (!NT_STATUS_IS_OK(get_sid_from_name(cli, mem_ctx, argv[0],
					       &group_sid, &group_type))) {
		d_fprintf(stderr, "Could not lookup group name %s\n", argv[0]);
		return NT_STATUS_UNSUCCESSFUL;
	}

	if (group_type == SID_NAME_DOM_GRP) {
		NTSTATUS result = rpc_add_groupmem(pipe_hnd, mem_ctx,
						   &group_sid, argv[1]);

		if (!NT_STATUS_IS_OK(result)) {
			d_fprintf(stderr, "Could not add %s to %s: %s\n",
				 argv[1], argv[0], nt_errstr(result));
		}
		return result;
	}

	if (group_type == SID_NAME_ALIAS) {
		NTSTATUS result = rpc_add_aliasmem(pipe_hnd, mem_ctx,
						   &group_sid, argv[1]);

		if (!NT_STATUS_IS_OK(result)) {
			d_fprintf(stderr, "Could not add %s to %s: %s\n",
				 argv[1], argv[0], nt_errstr(result));
		}
		return result;
	}

	d_fprintf(stderr, "Can only add members to global or local groups "
		 "which %s is not\n", argv[0]);

	return NT_STATUS_UNSUCCESSFUL;
}

static int rpc_group_addmem(int argc, const char **argv)
{
	return run_rpc_command(NULL, PI_SAMR, 0,
			       rpc_group_addmem_internals,
			       argc, argv);
}

static NTSTATUS rpc_del_groupmem(struct rpc_pipe_client *pipe_hnd,
				TALLOC_CTX *mem_ctx,
				const DOM_SID *group_sid,
				const char *member)
{
	POLICY_HND connect_pol, domain_pol;
	NTSTATUS result;
	uint32 group_rid;
	POLICY_HND group_pol;

	uint32 num_rids;
	uint32 *rids = NULL;
	uint32 *rid_types = NULL;

	DOM_SID sid;

	sid_copy(&sid, group_sid);

	if (!sid_split_rid(&sid, &group_rid))
		return NT_STATUS_UNSUCCESSFUL;

	/* Get sam policy handle */	
	result = rpccli_samr_connect(pipe_hnd, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				  &connect_pol);
	if (!NT_STATUS_IS_OK(result))
		return result;
	
	/* Get domain policy handle */
	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      &sid, &domain_pol);
	if (!NT_STATUS_IS_OK(result))
		return result;

	result = rpccli_samr_lookup_names(pipe_hnd, mem_ctx, &domain_pol, 1000,
				       1, &member,
				       &num_rids, &rids, &rid_types);

	if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "Could not lookup up group member %s\n", member);
		goto done;
	}

	result = rpccli_samr_open_group(pipe_hnd, mem_ctx, &domain_pol,
				     MAXIMUM_ALLOWED_ACCESS,
				     group_rid, &group_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_samr_del_groupmem(pipe_hnd, mem_ctx, &group_pol, rids[0]);

 done:
	rpccli_samr_close(pipe_hnd, mem_ctx, &connect_pol);
	return result;
}

static NTSTATUS rpc_del_aliasmem(struct rpc_pipe_client *pipe_hnd,
				TALLOC_CTX *mem_ctx,
				const DOM_SID *alias_sid,
				const char *member)
{
	POLICY_HND connect_pol, domain_pol;
	NTSTATUS result;
	uint32 alias_rid;
	POLICY_HND alias_pol;

	DOM_SID member_sid;
	enum SID_NAME_USE member_type;

	DOM_SID sid;

	sid_copy(&sid, alias_sid);

	if (!sid_split_rid(&sid, &alias_rid))
		return NT_STATUS_UNSUCCESSFUL;

	result = get_sid_from_name(pipe_hnd->cli, mem_ctx, member,
				   &member_sid, &member_type);

	if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "Could not lookup up group member %s\n", member);
		return result;
	}

	/* Get sam policy handle */	
	result = rpccli_samr_connect(pipe_hnd, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				  &connect_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}
	
	/* Get domain policy handle */
	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      &sid, &domain_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_samr_open_alias(pipe_hnd, mem_ctx, &domain_pol,
				     MAXIMUM_ALLOWED_ACCESS,
				     alias_rid, &alias_pol);

	if (!NT_STATUS_IS_OK(result))
		return result;

	result = rpccli_samr_del_aliasmem(pipe_hnd, mem_ctx, &alias_pol, &member_sid);

	if (!NT_STATUS_IS_OK(result))
		return result;

 done:
	rpccli_samr_close(pipe_hnd, mem_ctx, &connect_pol);
	return result;
}

static NTSTATUS rpc_group_delmem_internals(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					int argc,
					const char **argv)
{
	DOM_SID group_sid;
	enum SID_NAME_USE group_type;

	if (argc != 2) {
		d_printf("Usage: 'net rpc group delmem <group> <member>\n");
		return NT_STATUS_UNSUCCESSFUL;
	}

	if (!NT_STATUS_IS_OK(get_sid_from_name(cli, mem_ctx, argv[0],
					       &group_sid, &group_type))) {
		d_fprintf(stderr, "Could not lookup group name %s\n", argv[0]);
		return NT_STATUS_UNSUCCESSFUL;
	}

	if (group_type == SID_NAME_DOM_GRP) {
		NTSTATUS result = rpc_del_groupmem(pipe_hnd, mem_ctx,
						   &group_sid, argv[1]);

		if (!NT_STATUS_IS_OK(result)) {
			d_fprintf(stderr, "Could not del %s from %s: %s\n",
				 argv[1], argv[0], nt_errstr(result));
		}
		return result;
	}

	if (group_type == SID_NAME_ALIAS) {
		NTSTATUS result = rpc_del_aliasmem(pipe_hnd, mem_ctx, 
						   &group_sid, argv[1]);

		if (!NT_STATUS_IS_OK(result)) {
			d_fprintf(stderr, "Could not del %s from %s: %s\n",
				 argv[1], argv[0], nt_errstr(result));
		}
		return result;
	}

	d_fprintf(stderr, "Can only delete members from global or local groups "
		 "which %s is not\n", argv[0]);

	return NT_STATUS_UNSUCCESSFUL;
}

static int rpc_group_delmem(int argc, const char **argv)
{
	return run_rpc_command(NULL, PI_SAMR, 0,
			       rpc_group_delmem_internals,
			       argc, argv);
}

/** 
 * List groups on a remote RPC server
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid acquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_group_list_internals(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					int argc,
					const char **argv)
{
	POLICY_HND connect_pol, domain_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 start_idx=0, max_entries=250, num_entries, i, loop_count = 0;
	struct acct_info *groups;
	BOOL global = False;
	BOOL local = False;
	BOOL builtin = False;

	if (argc == 0) {
		global = True;
		local = True;
		builtin = True;
	}

	for (i=0; i<argc; i++) {
		if (strequal(argv[i], "global"))
			global = True;

		if (strequal(argv[i], "local"))
			local = True;

		if (strequal(argv[i], "builtin"))
			builtin = True;
	}

	/* Get sam policy handle */
	
	result = rpccli_samr_connect(pipe_hnd, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				  &connect_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}
	
	/* Get domain policy handle */
	
	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      domain_sid, &domain_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	/* Query domain groups */
	if (opt_long_list_entries)
		d_printf("\nGroup name            Comment"\
			 "\n-----------------------------\n");
	do {
		SAM_DISPINFO_CTR ctr;
		SAM_DISPINFO_3 info3;
		uint32 max_size;

		ZERO_STRUCT(ctr);
		ZERO_STRUCT(info3);
		ctr.sam.info3 = &info3;

		if (!global) break;

		get_query_dispinfo_params(
			loop_count, &max_entries, &max_size);

		result = rpccli_samr_query_dispinfo(pipe_hnd, mem_ctx, &domain_pol,
						 &start_idx, 3, &num_entries,
						 max_entries, max_size, &ctr);

		if (!NT_STATUS_IS_OK(result) &&
		    !NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES))
			break;
						 
		for (i = 0; i < num_entries; i++) {

			fstring group, desc;

			unistr2_to_ascii(group, &(&ctr.sam.info3->str[i])->uni_grp_name, sizeof(group)-1);
			unistr2_to_ascii(desc, &(&ctr.sam.info3->str[i])->uni_grp_desc, sizeof(desc)-1);
			
			if (opt_long_list_entries)
				printf("%-21.21s %-50.50s\n",
				       group, desc);
			else
				printf("%s\n", group);
		}
	} while (NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES));
	/* query domain aliases */
	start_idx = 0;
	do {
		if (!local) break;

		/* The max_size field in cli_samr_enum_als_groups is more like
		 * an account_control field with indiviual bits what to
		 * retrieve. Set this to 0xffff as NT4 usrmgr.exe does to get
		 * everything. I'm too lazy (sorry) to get this through to
		 * rpc_parse/ etc.  Volker */

		result = rpccli_samr_enum_als_groups(pipe_hnd, mem_ctx, &domain_pol,
						  &start_idx, 0xffff,
						  &groups, &num_entries);

		if (!NT_STATUS_IS_OK(result) &&
		    !NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES))
			break;
						 
		for (i = 0; i < num_entries; i++) {

			char *description = NULL;

			if (opt_long_list_entries) {

				POLICY_HND alias_pol;
				ALIAS_INFO_CTR ctr;

				if ((NT_STATUS_IS_OK(rpccli_samr_open_alias(pipe_hnd, mem_ctx,
									 &domain_pol,
									 0x8,
									 groups[i].rid,
									 &alias_pol))) &&
				    (NT_STATUS_IS_OK(rpccli_samr_query_alias_info(pipe_hnd, mem_ctx,
									       &alias_pol, 3,
									       &ctr))) &&
				    (NT_STATUS_IS_OK(rpccli_samr_close(pipe_hnd, mem_ctx,
								    &alias_pol)))) {
					description = unistr2_tdup(mem_ctx,
								   ctr.alias.info3.description.string);
				}
			}
			
			if (description != NULL) {
				printf("%-21.21s %-50.50s\n", 
				       groups[i].acct_name,
				       description);
			} else {
				printf("%s\n", groups[i].acct_name);
			}
		}
	} while (NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES));
	rpccli_samr_close(pipe_hnd, mem_ctx, &domain_pol);
	/* Get builtin policy handle */
	
	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      &global_sid_Builtin, &domain_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}
	/* query builtin aliases */
	start_idx = 0;
	do {
		if (!builtin) break;

		result = rpccli_samr_enum_als_groups(pipe_hnd, mem_ctx, &domain_pol,
						  &start_idx, max_entries,
						  &groups, &num_entries);
						 
		if (!NT_STATUS_IS_OK(result) &&
		    !NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES))
			break;
						 
		for (i = 0; i < num_entries; i++) {

			char *description = NULL;

			if (opt_long_list_entries) {

				POLICY_HND alias_pol;
				ALIAS_INFO_CTR ctr;

				if ((NT_STATUS_IS_OK(rpccli_samr_open_alias(pipe_hnd, mem_ctx,
									 &domain_pol,
									 0x8,
									 groups[i].rid,
									 &alias_pol))) &&
				    (NT_STATUS_IS_OK(rpccli_samr_query_alias_info(pipe_hnd, mem_ctx,
									       &alias_pol, 3,
									       &ctr))) &&
				    (NT_STATUS_IS_OK(rpccli_samr_close(pipe_hnd, mem_ctx,
								    &alias_pol)))) {
					description = unistr2_tdup(mem_ctx,
								   ctr.alias.info3.description.string);
				}
			}
			
			if (description != NULL) {
				printf("%-21.21s %-50.50s\n", 
				       groups[i].acct_name,
				       description);
			} else {
				printf("%s\n", groups[i].acct_name);
			}
		}
	} while (NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES));

 done:
	return result;
}

static int rpc_group_list(int argc, const char **argv)
{
	return run_rpc_command(NULL, PI_SAMR, 0,
			       rpc_group_list_internals,
			       argc, argv);
}

static NTSTATUS rpc_list_group_members(struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					const char *domain_name,
					const DOM_SID *domain_sid,
					POLICY_HND *domain_pol,
					uint32 rid)
{
	NTSTATUS result;
	POLICY_HND group_pol;
	uint32 num_members, *group_rids, *group_attrs;
	uint32 num_names;
	char **names;
	uint32 *name_types;
	int i;

	fstring sid_str;
	sid_to_string(sid_str, domain_sid);

	result = rpccli_samr_open_group(pipe_hnd, mem_ctx, domain_pol,
				     MAXIMUM_ALLOWED_ACCESS,
				     rid, &group_pol);

	if (!NT_STATUS_IS_OK(result))
		return result;

	result = rpccli_samr_query_groupmem(pipe_hnd, mem_ctx, &group_pol,
					 &num_members, &group_rids,
					 &group_attrs);

	if (!NT_STATUS_IS_OK(result))
		return result;

	while (num_members > 0) {
		int this_time = 512;

		if (num_members < this_time)
			this_time = num_members;

		result = rpccli_samr_lookup_rids(pipe_hnd, mem_ctx, domain_pol,
					      this_time, group_rids,
					      &num_names, &names, &name_types);

		if (!NT_STATUS_IS_OK(result))
			return result;

		/* We only have users as members, but make the output
		   the same as the output of alias members */

		for (i = 0; i < this_time; i++) {

			if (opt_long_list_entries) {
				printf("%s-%d %s\\%s %d\n", sid_str,
				       group_rids[i], domain_name, names[i],
				       SID_NAME_USER);
			} else {
				printf("%s\\%s\n", domain_name, names[i]);
			}
		}

		num_members -= this_time;
		group_rids += 512;
	}

	return NT_STATUS_OK;
}

static NTSTATUS rpc_list_alias_members(struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					POLICY_HND *domain_pol,
					uint32 rid)
{
	NTSTATUS result;
	struct rpc_pipe_client *lsa_pipe;
	POLICY_HND alias_pol, lsa_pol;
	uint32 num_members;
	DOM_SID *alias_sids;
	char **domains;
	char **names;
	uint32 *types;
	int i;

	result = rpccli_samr_open_alias(pipe_hnd, mem_ctx, domain_pol,
				     MAXIMUM_ALLOWED_ACCESS, rid, &alias_pol);

	if (!NT_STATUS_IS_OK(result))
		return result;

	result = rpccli_samr_query_aliasmem(pipe_hnd, mem_ctx, &alias_pol,
					 &num_members, &alias_sids);

	if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "Couldn't list alias members\n");
		return result;
	}

	if (num_members == 0) {
		return NT_STATUS_OK;
	}

	lsa_pipe = cli_rpc_pipe_open_noauth(pipe_hnd->cli, PI_LSARPC, &result);
	if (!lsa_pipe) {
		d_fprintf(stderr, "Couldn't open LSA pipe. Error was %s\n",
			nt_errstr(result) );
		return result;
	}

	result = rpccli_lsa_open_policy(lsa_pipe, mem_ctx, True,
				     SEC_RIGHTS_MAXIMUM_ALLOWED, &lsa_pol);

	if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "Couldn't open LSA policy handle\n");
		cli_rpc_pipe_close(lsa_pipe);
		return result;
	}

	result = rpccli_lsa_lookup_sids(lsa_pipe, mem_ctx, &lsa_pol, num_members,
				     alias_sids, 
				     &domains, &names, &types);

	if (!NT_STATUS_IS_OK(result) &&
	    !NT_STATUS_EQUAL(result, STATUS_SOME_UNMAPPED)) {
		d_fprintf(stderr, "Couldn't lookup SIDs\n");
		cli_rpc_pipe_close(lsa_pipe);
		return result;
	}

	for (i = 0; i < num_members; i++) {
		fstring sid_str;
		sid_to_string(sid_str, &alias_sids[i]);

		if (opt_long_list_entries) {
			printf("%s %s\\%s %d\n", sid_str, 
			       domains[i] ? domains[i] : "*unknown*", 
			       names[i] ? names[i] : "*unknown*", types[i]);
		} else {
			if (domains[i])
				printf("%s\\%s\n", domains[i], names[i]);
			else
				printf("%s\n", sid_str);
		}
	}

	cli_rpc_pipe_close(lsa_pipe);
	return NT_STATUS_OK;
}
 
static NTSTATUS rpc_group_members_internals(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					int argc,
					const char **argv)
{
	NTSTATUS result;
	POLICY_HND connect_pol, domain_pol;
	uint32 num_rids, *rids, *rid_types;

	/* Get sam policy handle */
	
	result = rpccli_samr_connect(pipe_hnd, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				  &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		return result;
	
	/* Get domain policy handle */
	
	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      domain_sid, &domain_pol);

	if (!NT_STATUS_IS_OK(result))
		return result;

	result = rpccli_samr_lookup_names(pipe_hnd, mem_ctx, &domain_pol, 1000,
				       1, argv, &num_rids, &rids, &rid_types);

	if (!NT_STATUS_IS_OK(result)) {

		/* Ok, did not find it in the global sam, try with builtin */

		DOM_SID sid_Builtin;

		rpccli_samr_close(pipe_hnd, mem_ctx, &domain_pol);

		string_to_sid(&sid_Builtin, "S-1-5-32");		

		result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
					      MAXIMUM_ALLOWED_ACCESS,
					      &sid_Builtin, &domain_pol);

		if (!NT_STATUS_IS_OK(result)) {
			d_fprintf(stderr, "Couldn't find group %s\n", argv[0]);
			return result;
		}

		result = rpccli_samr_lookup_names(pipe_hnd, mem_ctx, &domain_pol, 1000,
					       1, argv, &num_rids,
					       &rids, &rid_types);

		if (!NT_STATUS_IS_OK(result)) {
			d_fprintf(stderr, "Couldn't find group %s\n", argv[0]);
			return result;
		}
	}

	if (num_rids != 1) {
		d_fprintf(stderr, "Couldn't find group %s\n", argv[0]);
		return result;
	}

	if (rid_types[0] == SID_NAME_DOM_GRP) {
		return rpc_list_group_members(pipe_hnd, mem_ctx, domain_name,
					      domain_sid, &domain_pol,
					      rids[0]);
	}

	if (rid_types[0] == SID_NAME_ALIAS) {
		return rpc_list_alias_members(pipe_hnd, mem_ctx, &domain_pol,
					      rids[0]);
	}

	return NT_STATUS_NO_SUCH_GROUP;
}

static int rpc_group_members(int argc, const char **argv)
{
	if (argc != 1) {
		return rpc_group_usage(argc, argv);
	}

	return run_rpc_command(NULL, PI_SAMR, 0,
			       rpc_group_members_internals,
			       argc, argv);
}

static NTSTATUS rpc_group_rename_internals(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					int argc,
					const char **argv)
{
	NTSTATUS result;
	POLICY_HND connect_pol, domain_pol, group_pol;
	uint32 num_rids, *rids, *rid_types;
	GROUP_INFO_CTR ctr;

	if (argc != 2) {
		d_printf("Usage: 'net rpc group rename group newname'\n");
		return NT_STATUS_UNSUCCESSFUL;
	}

	/* Get sam policy handle */
	
	result = rpccli_samr_connect(pipe_hnd, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				  &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		return result;
	
	/* Get domain policy handle */
	
	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      domain_sid, &domain_pol);

	if (!NT_STATUS_IS_OK(result))
		return result;

	result = rpccli_samr_lookup_names(pipe_hnd, mem_ctx, &domain_pol, 1000,
				       1, argv, &num_rids, &rids, &rid_types);

	if (num_rids != 1) {
		d_fprintf(stderr, "Couldn't find group %s\n", argv[0]);
		return result;
	}

	if (rid_types[0] != SID_NAME_DOM_GRP) {
		d_fprintf(stderr, "Can only rename domain groups\n");
		return NT_STATUS_UNSUCCESSFUL;
	}

	result = rpccli_samr_open_group(pipe_hnd, mem_ctx, &domain_pol,
				     MAXIMUM_ALLOWED_ACCESS,
				     rids[0], &group_pol);

	if (!NT_STATUS_IS_OK(result))
		return result;

	ZERO_STRUCT(ctr);

	ctr.switch_value1 = 2;
	init_samr_group_info2(&ctr.group.info2, argv[1]);

	result = rpccli_samr_set_groupinfo(pipe_hnd, mem_ctx, &group_pol, &ctr);

	if (!NT_STATUS_IS_OK(result))
		return result;

	return NT_STATUS_NO_SUCH_GROUP;
}

static int rpc_group_rename(int argc, const char **argv)
{
	if (argc != 2) {
		return rpc_group_usage(argc, argv);
	}

	return run_rpc_command(NULL, PI_SAMR, 0,
			       rpc_group_rename_internals,
			       argc, argv);
}

/** 
 * 'net rpc group' entrypoint.
 * @param argc  Standard main() style argc
 * @param argc  Standard main() style argv.  Initial components are already
 *              stripped
 **/

int net_rpc_group(int argc, const char **argv) 
{
	struct functable func[] = {
		{"add", rpc_group_add},
		{"delete", rpc_group_delete},
		{"addmem", rpc_group_addmem},
		{"delmem", rpc_group_delmem},
		{"list", rpc_group_list},
		{"members", rpc_group_members},
		{"rename", rpc_group_rename},
		{NULL, NULL}
	};
	
	if (argc == 0) {
		return run_rpc_command(NULL, PI_SAMR, 0, 
				       rpc_group_list_internals,
				       argc, argv);
	}

	return net_run_function(argc, argv, func, rpc_group_usage);
}

/****************************************************************************/

static int rpc_share_usage(int argc, const char **argv)
{
	return net_help_share(argc, argv);
}

/** 
 * Add a share on a remote RPC server
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid acquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/
static NTSTATUS rpc_share_add_internals(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,int argc,
					const char **argv)
{
	WERROR result;
	char *sharename;
	char *path;
	uint32 type = STYPE_DISKTREE; /* only allow disk shares to be added */
	uint32 num_users=0, perms=0;
	char *password=NULL; /* don't allow a share password */
	uint32 level = 2;

	if ((sharename = talloc_strdup(mem_ctx, argv[0])) == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	path = strchr(sharename, '=');
	if (!path)
		return NT_STATUS_UNSUCCESSFUL;
	*path++ = '\0';

	result = rpccli_srvsvc_net_share_add(pipe_hnd, mem_ctx, sharename, type,
					  opt_comment, perms, opt_maxusers,
					  num_users, path, password, 
					  level, NULL);
	return werror_to_ntstatus(result);
}

static int rpc_share_add(int argc, const char **argv)
{
	if ((argc < 1) || !strchr(argv[0], '=')) {
		DEBUG(1,("Sharename or path not specified on add\n"));
		return rpc_share_usage(argc, argv);
	}
	return run_rpc_command(NULL, PI_SRVSVC, 0, 
			       rpc_share_add_internals,
			       argc, argv);
}

/** 
 * Delete a share on a remote RPC server
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid acquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/
static NTSTATUS rpc_share_del_internals(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					int argc,
					const char **argv)
{
	WERROR result;

	result = rpccli_srvsvc_net_share_del(pipe_hnd, mem_ctx, argv[0]);
	return W_ERROR_IS_OK(result) ? NT_STATUS_OK : NT_STATUS_UNSUCCESSFUL;
}

/** 
 * Delete a share on a remote RPC server
 *
 * @param domain_sid The domain sid acquired from the remote server
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/
static int rpc_share_delete(int argc, const char **argv)
{
	if (argc < 1) {
		DEBUG(1,("Sharename not specified on delete\n"));
		return rpc_share_usage(argc, argv);
	}
	return run_rpc_command(NULL, PI_SRVSVC, 0, 
			       rpc_share_del_internals,
			       argc, argv);
}

/**
 * Formatted print of share info
 *
 * @param info1  pointer to SRV_SHARE_INFO_1 to format
 **/
 
static void display_share_info_1(SRV_SHARE_INFO_1 *info1)
{
	fstring netname = "", remark = "";

	rpcstr_pull_unistr2_fstring(netname, &info1->info_1_str.uni_netname);
	rpcstr_pull_unistr2_fstring(remark, &info1->info_1_str.uni_remark);

	if (opt_long_list_entries) {
		d_printf("%-12s %-8.8s %-50s\n",
			 netname, share_type[info1->info_1.type], remark);
	} else {
		d_printf("%s\n", netname);
	}

}

static WERROR get_share_info(struct rpc_pipe_client *pipe_hnd,
				TALLOC_CTX *mem_ctx, 
				uint32 level,
				int argc,
				const char **argv, 
				SRV_SHARE_INFO_CTR *ctr)
{
	WERROR result;
	SRV_SHARE_INFO info;

	/* no specific share requested, enumerate all */
	if (argc == 0) {

		ENUM_HND hnd;
		uint32 preferred_len = 0xffffffff;

		init_enum_hnd(&hnd, 0);

		return rpccli_srvsvc_net_share_enum(pipe_hnd, mem_ctx, level, ctr, 
						 preferred_len, &hnd);
	}

	/* request just one share */
	result = rpccli_srvsvc_net_share_get_info(pipe_hnd, mem_ctx, argv[0], level, &info);

	if (!W_ERROR_IS_OK(result))
		goto done;

	/* construct ctr */
	ZERO_STRUCTP(ctr);

	ctr->info_level = ctr->switch_value = level;
	ctr->ptr_share_info = ctr->ptr_entries = 1;
	ctr->num_entries = ctr->num_entries2 = 1;

	switch (level) {
	case 1:
	{
		char *s;
		SRV_SHARE_INFO_1 *info1;
		
		ctr->share.info1 = TALLOC_ARRAY(mem_ctx, SRV_SHARE_INFO_1, 1);
		if (ctr->share.info1 == NULL) {
			result = WERR_NOMEM;
			goto done;
		}
		info1 = ctr->share.info1;
				
		memset(ctr->share.info1, 0, sizeof(SRV_SHARE_INFO_1));

		/* Copy pointer crap */

		memcpy(&info1->info_1, &info.share.info1.info_1, sizeof(SH_INFO_1));

		/* Duplicate strings */

		s = unistr2_tdup(mem_ctx, &info.share.info1.info_1_str.uni_netname);
		if (s)
			init_unistr2(&info1->info_1_str.uni_netname, s, UNI_STR_TERMINATE);

		s = unistr2_tdup(mem_ctx, &info.share.info1.info_1_str.uni_remark);
		if (s)
			init_unistr2(&info1->info_1_str.uni_remark, s, UNI_STR_TERMINATE);
	}
	case 2:
	{
		char *s;
		SRV_SHARE_INFO_2 *info2;
		
		ctr->share.info2 = TALLOC_ARRAY(mem_ctx, SRV_SHARE_INFO_2, 1);
		if (ctr->share.info2 == NULL) {
			result = WERR_NOMEM;
			goto done;
		}
		info2 = ctr->share.info2;
				
		memset(ctr->share.info2, 0, sizeof(SRV_SHARE_INFO_2));

		/* Copy pointer crap */

		memcpy(&info2->info_2, &info.share.info2.info_2, sizeof(SH_INFO_2));

		/* Duplicate strings */

		s = unistr2_tdup(mem_ctx, &info.share.info2.info_2_str.uni_netname);
		if (s)
			init_unistr2(&info2->info_2_str.uni_netname, s, UNI_STR_TERMINATE);

		s = unistr2_tdup(mem_ctx, &info.share.info2.info_2_str.uni_remark);
		if (s)
			init_unistr2(&info2->info_2_str.uni_remark, s, UNI_STR_TERMINATE);

		s = unistr2_tdup(mem_ctx, &info.share.info2.info_2_str.uni_path);
		if (s)
			init_unistr2(&info2->info_2_str.uni_path, s, UNI_STR_TERMINATE);

		s = unistr2_tdup(mem_ctx, &info.share.info2.info_2_str.uni_passwd);
		if (s)
			init_unistr2(&info2->info_2_str.uni_passwd, s, UNI_STR_TERMINATE);
	}
	case 502:
	{
		char *s;
		SRV_SHARE_INFO_502 *info502;

		ctr->share.info502 = TALLOC_ARRAY(mem_ctx, SRV_SHARE_INFO_502, 1);
		if (ctr->share.info502 == NULL) {
			result = WERR_NOMEM;
			goto done;
		}
		info502 = ctr->share.info502;

		memset(ctr->share.info502, 0, sizeof(SRV_SHARE_INFO_502));

		/* Copy pointer crap */

		memcpy(&info502->info_502, &info.share.info502.info_502, sizeof(SH_INFO_502));

		/* Duplicate strings */

		s = unistr2_tdup(mem_ctx, &info.share.info502.info_502_str.uni_netname);
		if (s)
			init_unistr2(&info502->info_502_str.uni_netname, s, UNI_STR_TERMINATE);

		s = unistr2_tdup(mem_ctx, &info.share.info502.info_502_str.uni_remark);
		if (s)
			init_unistr2(&info502->info_502_str.uni_remark, s, UNI_STR_TERMINATE);

		s = unistr2_tdup(mem_ctx, &info.share.info502.info_502_str.uni_path);
		if (s)
			init_unistr2(&info502->info_502_str.uni_path, s, UNI_STR_TERMINATE);

		s = unistr2_tdup(mem_ctx, &info.share.info502.info_502_str.uni_passwd);
		if (s)
			init_unistr2(&info502->info_502_str.uni_passwd, s, UNI_STR_TERMINATE);

		info502->info_502_str.sd = dup_sec_desc(mem_ctx, info.share.info502.info_502_str.sd);
				
	}

	} /* switch */

done:
	return result;
}

/** 
 * List shares on a remote RPC server
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid acquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_share_list_internals(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					int argc,
					const char **argv)
{
	SRV_SHARE_INFO_CTR ctr;
	WERROR result;
	uint32 i, level = 1;

	result = get_share_info(pipe_hnd, mem_ctx, level, argc, argv, &ctr);
	if (!W_ERROR_IS_OK(result))
		goto done;

	/* Display results */

	if (opt_long_list_entries) {
		d_printf(
	"\nEnumerating shared resources (exports) on remote server:\n\n"\
	"\nShare name   Type     Description\n"\
	"----------   ----     -----------\n");
	}
	for (i = 0; i < ctr.num_entries; i++)
		display_share_info_1(&ctr.share.info1[i]);
 done:
	return W_ERROR_IS_OK(result) ? NT_STATUS_OK : NT_STATUS_UNSUCCESSFUL;
}

/*** 
 * 'net rpc share list' entrypoint.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 **/
static int rpc_share_list(int argc, const char **argv)
{
	return run_rpc_command(NULL, PI_SRVSVC, 0, rpc_share_list_internals, argc, argv);
}

static BOOL check_share_availability(struct cli_state *cli, const char *netname)
{
	if (!cli_send_tconX(cli, netname, "A:", "", 0)) {
		d_printf("skipping   [%s]: not a file share.\n", netname);
		return False;
	}

	if (!cli_tdis(cli)) 
		return False;

	return True;
}

static BOOL check_share_sanity(struct cli_state *cli, fstring netname, uint32 type)
{
	/* only support disk shares */
	if (! ( type == STYPE_DISKTREE || type == (STYPE_DISKTREE | STYPE_HIDDEN)) ) {
		printf("share [%s] is not a diskshare (type: %x)\n", netname, type);
		return False;
	}

	/* skip builtin shares */
	/* FIXME: should print$ be added too ? */
	if (strequal(netname,"IPC$") || strequal(netname,"ADMIN$") || 
	    strequal(netname,"global")) 
		return False;

	if (opt_exclude && in_list(netname, opt_exclude, False)) {
		printf("excluding  [%s]\n", netname);
		return False;
	}

	return check_share_availability(cli, netname);
}

/** 
 * Migrate shares from a remote RPC server to the local RPC srever
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid acquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_share_migrate_shares_internals(const DOM_SID *domain_sid,
						const char *domain_name, 
						struct cli_state *cli,
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv)
{
	WERROR result;
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	SRV_SHARE_INFO_CTR ctr_src;
	uint32 type = STYPE_DISKTREE; /* only allow disk shares to be added */
	char *password = NULL; /* don't allow a share password */
	uint32 i;
	struct rpc_pipe_client *srvsvc_pipe = NULL;
	struct cli_state *cli_dst = NULL;
	uint32 level = 502; /* includes secdesc */

	result = get_share_info(pipe_hnd, mem_ctx, level, argc, argv, &ctr_src);
	if (!W_ERROR_IS_OK(result))
		goto done;

	/* connect destination PI_SRVSVC */
        nt_status = connect_dst_pipe(&cli_dst, &srvsvc_pipe, PI_SRVSVC);
        if (!NT_STATUS_IS_OK(nt_status))
                return nt_status;


	for (i = 0; i < ctr_src.num_entries; i++) {

		fstring netname = "", remark = "", path = "";
		/* reset error-code */
		nt_status = NT_STATUS_UNSUCCESSFUL;

		rpcstr_pull_unistr2_fstring(
			netname, &ctr_src.share.info502[i].info_502_str.uni_netname);
		rpcstr_pull_unistr2_fstring(
			remark, &ctr_src.share.info502[i].info_502_str.uni_remark);
		rpcstr_pull_unistr2_fstring(
			path, &ctr_src.share.info502[i].info_502_str.uni_path);

		if (!check_share_sanity(cli, netname, ctr_src.share.info502[i].info_502.type))
			continue;

		/* finally add the share on the dst server */ 

		printf("migrating: [%s], path: %s, comment: %s, without share-ACLs\n", 
			netname, path, remark);

		result = rpccli_srvsvc_net_share_add(srvsvc_pipe, mem_ctx, netname, type, remark,
						  ctr_src.share.info502[i].info_502.perms,
						  ctr_src.share.info502[i].info_502.max_uses,
						  ctr_src.share.info502[i].info_502.num_uses,
						  path, password, level, 
						  NULL);
	
                if (W_ERROR_V(result) == W_ERROR_V(WERR_ALREADY_EXISTS)) {
			printf("           [%s] does already exist\n", netname);
			continue;
		}

		if (!W_ERROR_IS_OK(result)) {
			printf("cannot add share: %s\n", dos_errstr(result));
			goto done;
		}

	}

	nt_status = NT_STATUS_OK;

done:
	if (cli_dst) {
		cli_shutdown(cli_dst);
	}

	return nt_status;

}

/** 
 * Migrate shares from a rpc-server to another
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/
static int rpc_share_migrate_shares(int argc, const char **argv)
{

	if (!opt_host) {
		printf("no server to migrate\n");
		return -1;
	}

	return run_rpc_command(NULL, PI_SRVSVC, 0, 
			       rpc_share_migrate_shares_internals,
			       argc, argv);
}

/**
 * Copy a file/dir 
 *
 * @param f	file_info
 * @param mask	current search mask
 * @param state	arg-pointer
 *
 **/
static void copy_fn(const char *mnt, file_info *f, const char *mask, void *state)
{
	static NTSTATUS nt_status;
	static struct copy_clistate *local_state;
	static fstring filename, new_mask;
	fstring dir;
	char *old_dir;

	local_state = (struct copy_clistate *)state;
	nt_status = NT_STATUS_UNSUCCESSFUL;

	if (strequal(f->name, ".") || strequal(f->name, ".."))
		return;

	DEBUG(3,("got mask: %s, name: %s\n", mask, f->name));

	/* DIRECTORY */
	if (f->mode & aDIR) {

		DEBUG(3,("got dir: %s\n", f->name));

		fstrcpy(dir, local_state->cwd);
		fstrcat(dir, "\\");
		fstrcat(dir, f->name);

		switch (net_mode_share)
		{
		case NET_MODE_SHARE_MIGRATE:
			/* create that directory */
			nt_status = net_copy_file(local_state->mem_ctx,
						  local_state->cli_share_src,
						  local_state->cli_share_dst,
						  dir, dir,
						  opt_acls? True : False,
						  opt_attrs? True : False,
						  opt_timestamps? True : False,
						  False);
			break;
		default:
			d_fprintf(stderr, "Unsupported mode %d\n", net_mode_share);
			return;
		}

		if (!NT_STATUS_IS_OK(nt_status)) 
			printf("could not handle dir %s: %s\n", 
				dir, nt_errstr(nt_status));

		/* search below that directory */
		fstrcpy(new_mask, dir);
		fstrcat(new_mask, "\\*");

		old_dir = local_state->cwd;
		local_state->cwd = dir;
		if (!sync_files(local_state, new_mask))
			printf("could not handle files\n");
		local_state->cwd = old_dir;

		return;
	}


	/* FILE */
	fstrcpy(filename, local_state->cwd);
	fstrcat(filename, "\\");
	fstrcat(filename, f->name);

	DEBUG(3,("got file: %s\n", filename));

	switch (net_mode_share)
	{
	case NET_MODE_SHARE_MIGRATE:
		nt_status = net_copy_file(local_state->mem_ctx, 
					  local_state->cli_share_src, 
					  local_state->cli_share_dst, 
					  filename, filename, 
					  opt_acls? True : False, 
					  opt_attrs? True : False,
					  opt_timestamps? True: False,
					  True);
		break;
	default:
		d_fprintf(stderr, "Unsupported file mode %d\n", net_mode_share);
		return;
	}

	if (!NT_STATUS_IS_OK(nt_status)) 
		printf("could not handle file %s: %s\n", 
			filename, nt_errstr(nt_status));

}

/**
 * sync files, can be called recursivly to list files 
 * and then call copy_fn for each file 
 *
 * @param cp_clistate	pointer to the copy_clistate we work with
 * @param mask		the current search mask
 *
 * @return 		Boolean result
 **/
BOOL sync_files(struct copy_clistate *cp_clistate, pstring mask)
{

	DEBUG(3,("calling cli_list with mask: %s\n", mask));

	if (cli_list(cp_clistate->cli_share_src, mask, cp_clistate->attribute, copy_fn, cp_clistate) == -1) {
		d_fprintf(stderr, "listing %s failed with error: %s\n", 
			mask, cli_errstr(cp_clistate->cli_share_src));
		return False;
	}

	return True;
}


/**
 * Set the top level directory permissions before we do any further copies.
 * Should set up ACL inheritance.
 **/

BOOL copy_top_level_perms(struct copy_clistate *cp_clistate, 
				const char *sharename)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;

	switch (net_mode_share) {
	case NET_MODE_SHARE_MIGRATE:
		DEBUG(3,("calling net_copy_fileattr for '.' directory in share %s\n", sharename));
		nt_status = net_copy_fileattr(cp_clistate->mem_ctx,
						cp_clistate->cli_share_src, 
						cp_clistate->cli_share_dst,
						"\\", "\\",
						opt_acls? True : False, 
						opt_attrs? True : False,
						opt_timestamps? True: False,
						False);
		break;
	default:
		d_fprintf(stderr, "Unsupported mode %d\n", net_mode_share);
		break;
	}

	if (!NT_STATUS_IS_OK(nt_status))  {
		printf("Could handle directory attributes for top level directory of share %s. Error %s\n", 
			sharename, nt_errstr(nt_status));
		return False;
	}

	return True;
}

/** 
 * Sync all files inside a remote share to another share (over smb)
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid acquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_share_migrate_files_internals(const DOM_SID *domain_sid,
						const char *domain_name, 
						struct cli_state *cli,
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx,
						int argc,
						const char **argv)
{
	WERROR result;
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	SRV_SHARE_INFO_CTR ctr_src;
	uint32 i;
	uint32 level = 502;
	struct copy_clistate cp_clistate;
	BOOL got_src_share = False;
	BOOL got_dst_share = False;
	pstring mask = "\\*";
	char *dst = NULL;

	dst = SMB_STRDUP(opt_destination?opt_destination:"127.0.0.1");

	result = get_share_info(pipe_hnd, mem_ctx, level, argc, argv, &ctr_src);

	if (!W_ERROR_IS_OK(result))
		goto done;

	for (i = 0; i < ctr_src.num_entries; i++) {

		fstring netname = "";

		rpcstr_pull_unistr2_fstring(
			netname, &ctr_src.share.info502[i].info_502_str.uni_netname);

		if (!check_share_sanity(cli, netname, ctr_src.share.info502[i].info_502.type))
			continue;

		/* one might not want to mirror whole discs :) */
		if (strequal(netname, "print$") || netname[1] == '$') {
			d_printf("skipping   [%s]: builtin/hidden share\n", netname);
			continue;
		}

		switch (net_mode_share)
		{
		case NET_MODE_SHARE_MIGRATE:
			printf("syncing");
			break;
		default:
			d_fprintf(stderr, "Unsupported mode %d\n", net_mode_share);
			break;
		}
		printf("    [%s] files and directories %s ACLs, %s DOS Attributes %s\n", 
			netname, 
			opt_acls ? "including" : "without", 
			opt_attrs ? "including" : "without",
			opt_timestamps ? "(preserving timestamps)" : "");

		cp_clistate.mem_ctx = mem_ctx;
		cp_clistate.cli_share_src = NULL;
		cp_clistate.cli_share_dst = NULL;
		cp_clistate.cwd = NULL;
		cp_clistate.attribute = aSYSTEM | aHIDDEN | aDIR;

	        /* open share source */
		nt_status = connect_to_service(&cp_clistate.cli_share_src,
					       &cli->dest_ip, cli->desthost,
					       netname, "A:");
		if (!NT_STATUS_IS_OK(nt_status))
			goto done;

		got_src_share = True;

		if (net_mode_share == NET_MODE_SHARE_MIGRATE) {
			/* open share destination */
			nt_status = connect_to_service(&cp_clistate.cli_share_dst,
						       NULL, dst, netname, "A:");
			if (!NT_STATUS_IS_OK(nt_status))
				goto done;

			got_dst_share = True;
		}

		if (!copy_top_level_perms(&cp_clistate, netname)) {
			d_fprintf(stderr, "Could not handle the top level directory permissions for the share: %s\n", netname);
			nt_status = NT_STATUS_UNSUCCESSFUL;
			goto done;
		}

		if (!sync_files(&cp_clistate, mask)) {
			d_fprintf(stderr, "could not handle files for share: %s\n", netname);
			nt_status = NT_STATUS_UNSUCCESSFUL;
			goto done;
		}
	}

	nt_status = NT_STATUS_OK;

done:

	if (got_src_share)
		cli_shutdown(cp_clistate.cli_share_src);

	if (got_dst_share)
		cli_shutdown(cp_clistate.cli_share_dst);

	return nt_status;

}

static int rpc_share_migrate_files(int argc, const char **argv)
{

	if (!opt_host) {
		printf("no server to migrate\n");
		return -1;
	}

	return run_rpc_command(NULL, PI_SRVSVC, 0, 
			       rpc_share_migrate_files_internals,
			       argc, argv);
}

/** 
 * Migrate share-ACLs from a remote RPC server to the local RPC srever
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid acquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_share_migrate_security_internals(const DOM_SID *domain_sid,
						const char *domain_name, 
						struct cli_state *cli,
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv)
{
	WERROR result;
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	SRV_SHARE_INFO_CTR ctr_src;
	SRV_SHARE_INFO info;
	uint32 i;
	struct rpc_pipe_client *srvsvc_pipe = NULL;
	struct cli_state *cli_dst = NULL;
	uint32 level = 502; /* includes secdesc */

	result = get_share_info(pipe_hnd, mem_ctx, level, argc, argv, &ctr_src);

	if (!W_ERROR_IS_OK(result))
		goto done;

	/* connect destination PI_SRVSVC */
        nt_status = connect_dst_pipe(&cli_dst, &srvsvc_pipe, PI_SRVSVC);
        if (!NT_STATUS_IS_OK(nt_status))
                return nt_status;


	for (i = 0; i < ctr_src.num_entries; i++) {

		fstring netname = "", remark = "", path = "";
		/* reset error-code */
		nt_status = NT_STATUS_UNSUCCESSFUL;

		rpcstr_pull_unistr2_fstring(
			netname, &ctr_src.share.info502[i].info_502_str.uni_netname);
		rpcstr_pull_unistr2_fstring(
			remark, &ctr_src.share.info502[i].info_502_str.uni_remark);
		rpcstr_pull_unistr2_fstring(
			path, &ctr_src.share.info502[i].info_502_str.uni_path);

		if (!check_share_sanity(cli, netname, ctr_src.share.info502[i].info_502.type))
			continue;

		printf("migrating: [%s], path: %s, comment: %s, including share-ACLs\n", 
			netname, path, remark);

		if (opt_verbose)
			display_sec_desc(ctr_src.share.info502[i].info_502_str.sd);

		/* init info */
		ZERO_STRUCT(info);

		info.switch_value = level;
		info.ptr_share_ctr = 1;

		/* FIXME: shouldn't we be able to just set the security descriptor ? */
		info.share.info502 = ctr_src.share.info502[i];

		/* finally modify the share on the dst server */
		result = rpccli_srvsvc_net_share_set_info(srvsvc_pipe, mem_ctx, netname, level, &info);
	
		if (!W_ERROR_IS_OK(result)) {
			printf("cannot set share-acl: %s\n", dos_errstr(result));
			goto done;
		}

	}

	nt_status = NT_STATUS_OK;

done:
	if (cli_dst) {
		cli_shutdown(cli_dst);
	}

	return nt_status;

}

/** 
 * Migrate share-acls from a rpc-server to another
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/
static int rpc_share_migrate_security(int argc, const char **argv)
{

	if (!opt_host) {
		printf("no server to migrate\n");
		return -1;
	}

	return run_rpc_command(NULL, PI_SRVSVC, 0, 
			       rpc_share_migrate_security_internals,
			       argc, argv);
}

/** 
 * Migrate shares (including share-definitions, share-acls and files with acls/attrs)
 * from one server to another
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 *
 **/
static int rpc_share_migrate_all(int argc, const char **argv)
{
	int ret;

	if (!opt_host) {
		printf("no server to migrate\n");
		return -1;
	}

	/* order is important. we don't want to be locked out by the share-acl
	 * before copying files - gd */
	
	ret = run_rpc_command(NULL, PI_SRVSVC, 0, rpc_share_migrate_shares_internals, argc, argv);
	if (ret)
		return ret;

	ret = run_rpc_command(NULL, PI_SRVSVC, 0, rpc_share_migrate_files_internals, argc, argv);
	if (ret)
		return ret;
	
	return run_rpc_command(NULL, PI_SRVSVC, 0, rpc_share_migrate_security_internals, argc, argv);
}


/** 
 * 'net rpc share migrate' entrypoint.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 **/
static int rpc_share_migrate(int argc, const char **argv)
{

	struct functable func[] = {
		{"all", 	rpc_share_migrate_all},
		{"files", 	rpc_share_migrate_files},
		{"help",	rpc_share_usage},
		{"security", 	rpc_share_migrate_security},
		{"shares", 	rpc_share_migrate_shares},
		{NULL, NULL}
	};

	net_mode_share = NET_MODE_SHARE_MIGRATE;

	return net_run_function(argc, argv, func, rpc_share_usage);
}

struct full_alias {
	DOM_SID sid;
	uint32 num_members;
	DOM_SID *members;
};

static int num_server_aliases;
static struct full_alias *server_aliases;

/*
 * Add an alias to the static list.
 */
static void push_alias(TALLOC_CTX *mem_ctx, struct full_alias *alias)
{
	if (server_aliases == NULL)
		server_aliases = SMB_MALLOC_ARRAY(struct full_alias, 100);

	server_aliases[num_server_aliases] = *alias;
	num_server_aliases += 1;
}

/*
 * For a specific domain on the server, fetch all the aliases
 * and their members. Add all of them to the server_aliases.
 */

static NTSTATUS rpc_fetch_domain_aliases(struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					POLICY_HND *connect_pol,
					const DOM_SID *domain_sid)
{
	uint32 start_idx, max_entries, num_entries, i;
	struct acct_info *groups;
	NTSTATUS result;
	POLICY_HND domain_pol;

	/* Get domain policy handle */
	
	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      domain_sid, &domain_pol);
	if (!NT_STATUS_IS_OK(result))
		return result;

	start_idx = 0;
	max_entries = 250;

	do {
		result = rpccli_samr_enum_als_groups(pipe_hnd, mem_ctx, &domain_pol,
						  &start_idx, max_entries,
						  &groups, &num_entries);

		for (i = 0; i < num_entries; i++) {

			POLICY_HND alias_pol;
			struct full_alias alias;
			DOM_SID *members;
			int j;

			result = rpccli_samr_open_alias(pipe_hnd, mem_ctx, &domain_pol,
						     MAXIMUM_ALLOWED_ACCESS,
						     groups[i].rid,
						     &alias_pol);
			if (!NT_STATUS_IS_OK(result))
				goto done;

			result = rpccli_samr_query_aliasmem(pipe_hnd, mem_ctx,
							 &alias_pol,
							 &alias.num_members,
							 &members);
			if (!NT_STATUS_IS_OK(result))
				goto done;

			result = rpccli_samr_close(pipe_hnd, mem_ctx, &alias_pol);
			if (!NT_STATUS_IS_OK(result))
				goto done;

			alias.members = NULL;

			if (alias.num_members > 0) {
				alias.members = SMB_MALLOC_ARRAY(DOM_SID, alias.num_members);

				for (j = 0; j < alias.num_members; j++)
					sid_copy(&alias.members[j],
						 &members[j]);
			}

			sid_copy(&alias.sid, domain_sid);
			sid_append_rid(&alias.sid, groups[i].rid);

			push_alias(mem_ctx, &alias);
		}
	} while (NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES));

	result = NT_STATUS_OK;

 done:
	rpccli_samr_close(pipe_hnd, mem_ctx, &domain_pol);

	return result;
}

/*
 * Dump server_aliases as names for debugging purposes.
 */

static NTSTATUS rpc_aliaslist_dump(const DOM_SID *domain_sid,
				const char *domain_name,
				struct cli_state *cli,
				struct rpc_pipe_client *pipe_hnd,
				TALLOC_CTX *mem_ctx, 
				int argc,
				const char **argv)
{
	int i;
	NTSTATUS result;
	POLICY_HND lsa_pol;

	result = rpccli_lsa_open_policy(pipe_hnd, mem_ctx, True, 
				     SEC_RIGHTS_MAXIMUM_ALLOWED,
				     &lsa_pol);
	if (!NT_STATUS_IS_OK(result))
		return result;

	for (i=0; i<num_server_aliases; i++) {
		char **names;
		char **domains;
		uint32 *types;
		int j;

		struct full_alias *alias = &server_aliases[i];

		result = rpccli_lsa_lookup_sids(pipe_hnd, mem_ctx, &lsa_pol, 1,
					     &alias->sid,
					     &domains, &names, &types);
		if (!NT_STATUS_IS_OK(result))
			continue;

		DEBUG(1, ("%s\\%s %d: ", domains[0], names[0], types[0]));

		if (alias->num_members == 0) {
			DEBUG(1, ("\n"));
			continue;
		}

		result = rpccli_lsa_lookup_sids(pipe_hnd, mem_ctx, &lsa_pol,
					     alias->num_members,
					     alias->members,
					     &domains, &names, &types);

		if (!NT_STATUS_IS_OK(result) &&
		    !NT_STATUS_EQUAL(result, STATUS_SOME_UNMAPPED))
			continue;

		for (j=0; j<alias->num_members; j++)
			DEBUG(1, ("%s\\%s (%d); ",
				  domains[j] ? domains[j] : "*unknown*", 
				  names[j] ? names[j] : "*unknown*",types[j]));
		DEBUG(1, ("\n"));
	}

	rpccli_lsa_close(pipe_hnd, mem_ctx, &lsa_pol);

	return NT_STATUS_OK;
}

/*
 * Fetch a list of all server aliases and their members into
 * server_aliases.
 */

static NTSTATUS rpc_aliaslist_internals(const DOM_SID *domain_sid,
					const char *domain_name,
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv)
{
	NTSTATUS result;
	POLICY_HND connect_pol;

	result = rpccli_samr_connect(pipe_hnd, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				  &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;
	
	result = rpc_fetch_domain_aliases(pipe_hnd, mem_ctx, &connect_pol,
					  &global_sid_Builtin);

	if (!NT_STATUS_IS_OK(result))
		goto done;
	
	result = rpc_fetch_domain_aliases(pipe_hnd, mem_ctx, &connect_pol,
					  domain_sid);

	rpccli_samr_close(pipe_hnd, mem_ctx, &connect_pol);
 done:
	return result;
}

static void init_user_token(NT_USER_TOKEN *token, DOM_SID *user_sid)
{
	token->num_sids = 4;

	token->user_sids = SMB_MALLOC_ARRAY(DOM_SID, 4);

	token->user_sids[0] = *user_sid;
	sid_copy(&token->user_sids[1], &global_sid_World);
	sid_copy(&token->user_sids[2], &global_sid_Network);
	sid_copy(&token->user_sids[3], &global_sid_Authenticated_Users);
}

static void free_user_token(NT_USER_TOKEN *token)
{
	SAFE_FREE(token->user_sids);
}

static BOOL is_sid_in_token(NT_USER_TOKEN *token, DOM_SID *sid)
{
	int i;

	for (i=0; i<token->num_sids; i++) {
		if (sid_compare(sid, &token->user_sids[i]) == 0)
			return True;
	}
	return False;
}

static void add_sid_to_token(NT_USER_TOKEN *token, DOM_SID *sid)
{
	if (is_sid_in_token(token, sid))
		return;

	token->user_sids = SMB_REALLOC_ARRAY(token->user_sids, DOM_SID, token->num_sids+1);
	if (!token->user_sids) {
		return;
	}

	sid_copy(&token->user_sids[token->num_sids], sid);

	token->num_sids += 1;
}

struct user_token {
	fstring name;
	NT_USER_TOKEN token;
};

static void dump_user_token(struct user_token *token)
{
	int i;

	d_printf("%s\n", token->name);

	for (i=0; i<token->token.num_sids; i++) {
		d_printf(" %s\n", sid_string_static(&token->token.user_sids[i]));
	}
}

static BOOL is_alias_member(DOM_SID *sid, struct full_alias *alias)
{
	int i;

	for (i=0; i<alias->num_members; i++) {
		if (sid_compare(sid, &alias->members[i]) == 0)
			return True;
	}

	return False;
}

static void collect_sid_memberships(NT_USER_TOKEN *token, DOM_SID sid)
{
	int i;

	for (i=0; i<num_server_aliases; i++) {
		if (is_alias_member(&sid, &server_aliases[i]))
			add_sid_to_token(token, &server_aliases[i].sid);
	}
}

/*
 * We got a user token with all the SIDs we can know about without asking the
 * server directly. These are the user and domain group sids. All of these can
 * be members of aliases. So scan the list of aliases for each of the SIDs and
 * add them to the token.
 */

static void collect_alias_memberships(NT_USER_TOKEN *token)
{
	int num_global_sids = token->num_sids;
	int i;

	for (i=0; i<num_global_sids; i++) {
		collect_sid_memberships(token, token->user_sids[i]);
	}
}

static BOOL get_user_sids(const char *domain, const char *user, NT_USER_TOKEN *token)
{
	struct winbindd_request request;
	struct winbindd_response response;
	fstring full_name;
	NSS_STATUS result;

	DOM_SID user_sid;

	int i;

	fstr_sprintf(full_name, "%s%c%s",
		     domain, *lp_winbind_separator(), user);

	/* First let's find out the user sid */

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	fstrcpy(request.data.name.dom_name, domain);
	fstrcpy(request.data.name.name, user);

	result = winbindd_request_response(WINBINDD_LOOKUPNAME, &request, &response);

	if (result != NSS_STATUS_SUCCESS) {
		DEBUG(1, ("winbind could not find %s\n", full_name));
		return False;
	}

	if (response.data.sid.type != SID_NAME_USER) {
		DEBUG(1, ("%s is not a user\n", full_name));
		return False;
	}

	string_to_sid(&user_sid, response.data.sid.sid);

	init_user_token(token, &user_sid);

	/* And now the groups winbind knows about */

	ZERO_STRUCT(response);

	fstrcpy(request.data.username, full_name);

	result = winbindd_request_response(WINBINDD_GETGROUPS, &request, &response);

	if (result != NSS_STATUS_SUCCESS) {
		DEBUG(1, ("winbind could not get groups of %s\n", full_name));
		return False;
	}

	for (i = 0; i < response.data.num_entries; i++) {
		gid_t gid = ((gid_t *)response.extra_data.data)[i];
		DOM_SID sid;

		struct winbindd_request sidrequest;
		struct winbindd_response sidresponse;

		ZERO_STRUCT(sidrequest);
		ZERO_STRUCT(sidresponse);

		sidrequest.data.gid = gid;

		result = winbindd_request_response(WINBINDD_GID_TO_SID,
					  &sidrequest, &sidresponse);

		if (result != NSS_STATUS_SUCCESS) {
			DEBUG(1, ("winbind could not find SID of gid %d\n",
				  gid));
			return False;
		}

		DEBUG(3, (" %s\n", sidresponse.data.sid.sid));

		string_to_sid(&sid, sidresponse.data.sid.sid);
		add_sid_to_token(token, &sid);
	}

	SAFE_FREE(response.extra_data.data);

	return True;
}
	
/**
 * Get a list of all user tokens we want to look at
 **/

static BOOL get_user_tokens(int *num_tokens, struct user_token **user_tokens)
{
	struct winbindd_request request;
	struct winbindd_response response;
	const char *extra_data;
	fstring name;
	int i;
	struct user_token *result;

	if (lp_winbind_use_default_domain() &&
	    (opt_target_workgroup == NULL)) {
		d_fprintf(stderr, "winbind use default domain = yes set, "
			 "please specify a workgroup\n");
		return False;
	}

	/* Send request to winbind daemon */

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);
	
	if (winbindd_request_response(WINBINDD_LIST_USERS, &request, &response) !=
	    NSS_STATUS_SUCCESS)
		return False;

	/* Look through extra data */

	if (!response.extra_data.data)
		return False;

	extra_data = (const char *)response.extra_data.data;
	*num_tokens = 0;

	while(next_token(&extra_data, name, ",", sizeof(fstring))) {
		*num_tokens += 1;
	}

	result = SMB_MALLOC_ARRAY(struct user_token, *num_tokens);

	if (result == NULL) {
		DEBUG(1, ("Could not malloc sid array\n"));
		return False;
	}

	extra_data = (const char *)response.extra_data.data;
	i=0;

	while(next_token(&extra_data, name, ",", sizeof(fstring))) {

		fstring domain, user;
		char *p;

		fstrcpy(result[i].name, name);

		p = strchr(name, *lp_winbind_separator());

		DEBUG(3, ("%s\n", name));

		if (p == NULL) {
			fstrcpy(domain, opt_target_workgroup);
			fstrcpy(user, name);
		} else {
			*p++ = '\0';
			fstrcpy(domain, name);
			strupper_m(domain);
			fstrcpy(user, p);
		}

		get_user_sids(domain, user, &(result[i].token));
		i+=1;
	}
	
	SAFE_FREE(response.extra_data.data);

	*user_tokens = result;

	return True;
}

static BOOL get_user_tokens_from_file(FILE *f,
				      int *num_tokens,
				      struct user_token **tokens)
{
	struct user_token *token = NULL;

	while (!feof(f)) {
		fstring line;

		if (fgets(line, sizeof(line)-1, f) == NULL) {
			return True;
		}

		if (line[strlen(line)-1] == '\n')
			line[strlen(line)-1] = '\0';

		if (line[0] == ' ') {
			/* We have a SID */

			DOM_SID sid;
			string_to_sid(&sid, &line[1]);

			if (token == NULL) {
				DEBUG(0, ("File does not begin with username"));
				return False;
			}

			add_sid_to_token(&token->token, &sid);
			continue;
		}

		/* And a new user... */

		*num_tokens += 1;
		*tokens = SMB_REALLOC_ARRAY(*tokens, struct user_token, *num_tokens);
		if (*tokens == NULL) {
			DEBUG(0, ("Could not realloc tokens\n"));
			return False;
		}

		token = &((*tokens)[*num_tokens-1]);

		fstrcpy(token->name, line);
		token->token.num_sids = 0;
		token->token.user_sids = NULL;
		continue;
	}
	
	return False;
}


/*
 * Show the list of all users that have access to a share
 */

static void show_userlist(struct rpc_pipe_client *pipe_hnd,
			TALLOC_CTX *mem_ctx,
			const char *netname,
			int num_tokens,
			struct user_token *tokens)
{
	int fnum;
	SEC_DESC *share_sd = NULL;
	SEC_DESC *root_sd = NULL;
	struct cli_state *cli = pipe_hnd->cli;
	int i;
	SRV_SHARE_INFO info;
	WERROR result;
	uint16 cnum;

	result = rpccli_srvsvc_net_share_get_info(pipe_hnd, mem_ctx, netname,
					       502, &info);

	if (!W_ERROR_IS_OK(result)) {
		DEBUG(1, ("Coult not query secdesc for share %s\n",
			  netname));
		return;
	}

	share_sd = info.share.info502.info_502_str.sd;
	if (share_sd == NULL) {
		DEBUG(1, ("Got no secdesc for share %s\n",
			  netname));
	}

	cnum = cli->cnum;

	if (!cli_send_tconX(cli, netname, "A:", "", 0)) {
		return;
	}

	fnum = cli_nt_create(cli, "\\", READ_CONTROL_ACCESS);

	if (fnum != -1) {
		root_sd = cli_query_secdesc(cli, fnum, mem_ctx);
	}

	for (i=0; i<num_tokens; i++) {
		uint32 acc_granted;
		NTSTATUS status;

		if (share_sd != NULL) {
			if (!se_access_check(share_sd, &tokens[i].token,
					     1, &acc_granted, &status)) {
				DEBUG(1, ("Could not check share_sd for "
					  "user %s\n",
					  tokens[i].name));
				continue;
			}

			if (!NT_STATUS_IS_OK(status))
				continue;
		}

		if (root_sd == NULL) {
			d_printf(" %s\n", tokens[i].name);
			continue;
		}

		if (!se_access_check(root_sd, &tokens[i].token,
				     1, &acc_granted, &status)) {
			DEBUG(1, ("Could not check root_sd for user %s\n",
				  tokens[i].name));
			continue;
		}

		if (!NT_STATUS_IS_OK(status))
			continue;

		d_printf(" %s\n", tokens[i].name);
	}

	if (fnum != -1)
		cli_close(cli, fnum);
	cli_tdis(cli);
	cli->cnum = cnum;
	
	return;
}

struct share_list {
	int num_shares;
	char **shares;
};

static void collect_share(const char *name, uint32 m,
			  const char *comment, void *state)
{
	struct share_list *share_list = (struct share_list *)state;

	if (m != STYPE_DISKTREE)
		return;

	share_list->num_shares += 1;
	share_list->shares = SMB_REALLOC_ARRAY(share_list->shares, char *, share_list->num_shares);
	if (!share_list->shares) {
		share_list->num_shares = 0;
		return;
	}
	share_list->shares[share_list->num_shares-1] = SMB_STRDUP(name);
}

static void rpc_share_userlist_usage(void)
{
	return;
}
	
/** 
 * List shares on a remote RPC server, including the security descriptors
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid acquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_share_allowedusers_internals(const DOM_SID *domain_sid,
						const char *domain_name,
						struct cli_state *cli,
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx,
						int argc,
						const char **argv)
{
	int ret;
	BOOL r;
	ENUM_HND hnd;
	uint32 i;
	FILE *f;

	struct user_token *tokens = NULL;
	int num_tokens = 0;

	struct share_list share_list;

	if (argc > 1) {
		rpc_share_userlist_usage();
		return NT_STATUS_UNSUCCESSFUL;
	}

	if (argc == 0) {
		f = stdin;
	} else {
		f = fopen(argv[0], "r");
	}

	if (f == NULL) {
		DEBUG(0, ("Could not open userlist: %s\n", strerror(errno)));
		return NT_STATUS_UNSUCCESSFUL;
	}

	r = get_user_tokens_from_file(f, &num_tokens, &tokens);

	if (f != stdin)
		fclose(f);

	if (!r) {
		DEBUG(0, ("Could not read users from file\n"));
		return NT_STATUS_UNSUCCESSFUL;
	}

	for (i=0; i<num_tokens; i++)
		collect_alias_memberships(&tokens[i].token);

	init_enum_hnd(&hnd, 0);

	share_list.num_shares = 0;
	share_list.shares = NULL;

	ret = cli_RNetShareEnum(cli, collect_share, &share_list);

	if (ret == -1) {
		DEBUG(0, ("Error returning browse list: %s\n",
			  cli_errstr(cli)));
		goto done;
	}

	for (i = 0; i < share_list.num_shares; i++) {
		char *netname = share_list.shares[i];

		if (netname[strlen(netname)-1] == '$')
			continue;

		d_printf("%s\n", netname);

		show_userlist(pipe_hnd, mem_ctx, netname,
			      num_tokens, tokens);
	}
 done:
	for (i=0; i<num_tokens; i++) {
		free_user_token(&tokens[i].token);
	}
	SAFE_FREE(tokens);
	SAFE_FREE(share_list.shares);

	return NT_STATUS_OK;
}

static int rpc_share_allowedusers(int argc, const char **argv)
{
	int result;

	result = run_rpc_command(NULL, PI_SAMR, 0,
				 rpc_aliaslist_internals,
				 argc, argv);
	if (result != 0)
		return result;

	result = run_rpc_command(NULL, PI_LSARPC, 0,
				 rpc_aliaslist_dump,
				 argc, argv);
	if (result != 0)
		return result;

	return run_rpc_command(NULL, PI_SRVSVC, 0,
			       rpc_share_allowedusers_internals,
			       argc, argv);
}

int net_usersidlist(int argc, const char **argv)
{
	int num_tokens = 0;
	struct user_token *tokens = NULL;
	int i;

	if (argc != 0) {
		net_usersidlist_usage(argc, argv);
		return 0;
	}

	if (!get_user_tokens(&num_tokens, &tokens)) {
		DEBUG(0, ("Could not get the user/sid list\n"));
		return 0;
	}

	for (i=0; i<num_tokens; i++) {
		dump_user_token(&tokens[i]);
		free_user_token(&tokens[i].token);
	}

	SAFE_FREE(tokens);
	return 1;
}

int net_usersidlist_usage(int argc, const char **argv)
{
	d_printf("net usersidlist\n"
		 "\tprints out a list of all users the running winbind knows\n"
		 "\tabout, together with all their SIDs. This is used as\n"
		 "\tinput to the 'net rpc share allowedusers' command.\n\n");

	net_common_flags_usage(argc, argv);
	return -1;
}

/** 
 * 'net rpc share' entrypoint.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 **/

int net_rpc_share(int argc, const char **argv) 
{
	struct functable func[] = {
		{"add", rpc_share_add},
		{"delete", rpc_share_delete},
		{"allowedusers", rpc_share_allowedusers},
		{"migrate", rpc_share_migrate},
		{"list", rpc_share_list},
		{NULL, NULL}
	};

	if (argc == 0)
		return run_rpc_command(NULL, PI_SRVSVC, 0, 
				       rpc_share_list_internals,
				       argc, argv);

	return net_run_function(argc, argv, func, rpc_share_usage);
}

static NTSTATUS rpc_sh_share_list(TALLOC_CTX *mem_ctx,
				  struct rpc_sh_ctx *ctx,
				  struct rpc_pipe_client *pipe_hnd,
				  int argc, const char **argv)
{
	return rpc_share_list_internals(ctx->domain_sid, ctx->domain_name,
					ctx->cli, pipe_hnd, mem_ctx,
					argc, argv);
}

static NTSTATUS rpc_sh_share_add(TALLOC_CTX *mem_ctx,
				 struct rpc_sh_ctx *ctx,
				 struct rpc_pipe_client *pipe_hnd,
				 int argc, const char **argv)
{
	WERROR result;

	if ((argc < 2) || (argc > 3)) {
		d_fprintf(stderr, "usage: %s <share> <path> [comment]\n",
			  ctx->whoami);
		return NT_STATUS_INVALID_PARAMETER;
	}

	result = rpccli_srvsvc_net_share_add(
		pipe_hnd, mem_ctx, argv[0], STYPE_DISKTREE,
		(argc == 3) ? argv[2] : "",
		0, 0, 0, argv[1], NULL, 2, NULL);
					     
	return werror_to_ntstatus(result);
}

static NTSTATUS rpc_sh_share_delete(TALLOC_CTX *mem_ctx,
				    struct rpc_sh_ctx *ctx,
				    struct rpc_pipe_client *pipe_hnd,
				    int argc, const char **argv)
{
	WERROR result;

	if (argc != 1) {
		d_fprintf(stderr, "usage: %s <share>\n", ctx->whoami);
		return NT_STATUS_INVALID_PARAMETER;
	}

	result = rpccli_srvsvc_net_share_del(pipe_hnd, mem_ctx, argv[0]);
	return werror_to_ntstatus(result);
}

static NTSTATUS rpc_sh_share_info(TALLOC_CTX *mem_ctx,
				  struct rpc_sh_ctx *ctx,
				  struct rpc_pipe_client *pipe_hnd,
				  int argc, const char **argv)
{
	SRV_SHARE_INFO info;
	SRV_SHARE_INFO_2 *info2 = &info.share.info2;
	WERROR result;

	if (argc != 1) {
		d_fprintf(stderr, "usage: %s <share>\n", ctx->whoami);
		return NT_STATUS_INVALID_PARAMETER;
	}

	result = rpccli_srvsvc_net_share_get_info(
		pipe_hnd, mem_ctx, argv[0], 2, &info);
	if (!W_ERROR_IS_OK(result)) {
		goto done;
	}

	d_printf("Name:     %s\n",
		 rpcstr_pull_unistr2_talloc(mem_ctx,
					    &info2->info_2_str.uni_netname));
	d_printf("Comment:  %s\n",
		 rpcstr_pull_unistr2_talloc(mem_ctx,
					    &info2->info_2_str.uni_remark));
	
	d_printf("Path:     %s\n",
		 rpcstr_pull_unistr2_talloc(mem_ctx,
					    &info2->info_2_str.uni_path));
	d_printf("Password: %s\n",
		 rpcstr_pull_unistr2_talloc(mem_ctx,
					    &info2->info_2_str.uni_passwd));

 done:
	return werror_to_ntstatus(result);
}

struct rpc_sh_cmd *net_rpc_share_cmds(TALLOC_CTX *mem_ctx,
				      struct rpc_sh_ctx *ctx)
{
	static struct rpc_sh_cmd cmds[] = {

	{ "list", NULL, PI_SRVSVC, rpc_sh_share_list,
	  "List available shares" },

	{ "add", NULL, PI_SRVSVC, rpc_sh_share_add,
	  "Add a share" },

	{ "delete", NULL, PI_SRVSVC, rpc_sh_share_delete,
	  "Delete a share" },

	{ "info", NULL, PI_SRVSVC, rpc_sh_share_info,
	  "Get information about a share" },

	{ NULL, NULL, 0, NULL, NULL }
	};

	return cmds;
}

/****************************************************************************/

static int rpc_file_usage(int argc, const char **argv)
{
	return net_help_file(argc, argv);
}

/** 
 * Close a file on a remote RPC server
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid acquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/
static NTSTATUS rpc_file_close_internals(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					int argc,
					const char **argv)
{
	WERROR result;
	result = rpccli_srvsvc_net_file_close(pipe_hnd, mem_ctx, atoi(argv[0]));
	return W_ERROR_IS_OK(result) ? NT_STATUS_OK : NT_STATUS_UNSUCCESSFUL;
}

/** 
 * Close a file on a remote RPC server
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/
static int rpc_file_close(int argc, const char **argv)
{
	if (argc < 1) {
		DEBUG(1, ("No fileid given on close\n"));
		return(rpc_file_usage(argc, argv));
	}

	return run_rpc_command(NULL, PI_SRVSVC, 0, 
			       rpc_file_close_internals,
			       argc, argv);
}

/** 
 * Formatted print of open file info 
 *
 * @param info3  FILE_INFO_3 contents
 * @param str3   strings for FILE_INFO_3
 **/

static void display_file_info_3(FILE_INFO_3 *info3, FILE_INFO_3_STR *str3)
{
	fstring user = "", path = "";

	rpcstr_pull_unistr2_fstring(user, &str3->uni_user_name);
	rpcstr_pull_unistr2_fstring(path, &str3->uni_path_name);

	d_printf("%-7.1d %-20.20s 0x%-4.2x %-6.1d %s\n",
		 info3->id, user, info3->perms, info3->num_locks, path);
}

/** 
 * List open files on a remote RPC server
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid acquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_file_list_internals(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					int argc,
					const char **argv)
{
	SRV_FILE_INFO_CTR ctr;
	WERROR result;
	ENUM_HND hnd;
	uint32 preferred_len = 0xffffffff, i;
	const char *username=NULL;

	init_enum_hnd(&hnd, 0);

	/* if argc > 0, must be user command */
	if (argc > 0)
		username = smb_xstrdup(argv[0]);
		
	result = rpccli_srvsvc_net_file_enum(pipe_hnd,
					mem_ctx, 3, username, &ctr, preferred_len, &hnd);

	if (!W_ERROR_IS_OK(result))
		goto done;

	/* Display results */

	d_printf(
		 "\nEnumerating open files on remote server:\n\n"\
		 "\nFileId  Opened by            Perms  Locks  Path"\
		 "\n------  ---------            -----  -----  ---- \n");
	for (i = 0; i < ctr.num_entries; i++)
		display_file_info_3(&ctr.file.info3[i].info_3, 
				    &ctr.file.info3[i].info_3_str);
 done:
	return W_ERROR_IS_OK(result) ? NT_STATUS_OK : NT_STATUS_UNSUCCESSFUL;
}

/** 
 * List files for a user on a remote RPC server
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/

static int rpc_file_user(int argc, const char **argv)
{
	if (argc < 1) {
		DEBUG(1, ("No username given\n"));
		return(rpc_file_usage(argc, argv));
	}

	return run_rpc_command(NULL, PI_SRVSVC, 0, 
			       rpc_file_list_internals,
			       argc, argv);
}

/** 
 * 'net rpc file' entrypoint.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 **/

int net_rpc_file(int argc, const char **argv) 
{
	struct functable func[] = {
		{"close", rpc_file_close},
		{"user", rpc_file_user},
#if 0
		{"info", rpc_file_info},
#endif
		{NULL, NULL}
	};

	if (argc == 0)
		return run_rpc_command(NULL, PI_SRVSVC, 0, 
				       rpc_file_list_internals,
				       argc, argv);

	return net_run_function(argc, argv, func, rpc_file_usage);
}

/** 
 * ABORT the shutdown of a remote RPC Server over, initshutdown pipe
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passed through. 
 *
 * @param domain_sid The domain sid aquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on compleation of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_shutdown_abort_internals(const DOM_SID *domain_sid, 
					const char *domain_name, 
					struct cli_state *cli, 
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv) 
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	
	result = rpccli_shutdown_abort(pipe_hnd, mem_ctx);
	
	if (NT_STATUS_IS_OK(result)) {
		d_printf("\nShutdown successfully aborted\n");
		DEBUG(5,("cmd_shutdown_abort: query succeeded\n"));
	} else
		DEBUG(5,("cmd_shutdown_abort: query failed\n"));
	
	return result;
}

/** 
 * ABORT the shutdown of a remote RPC Server,  over winreg pipe
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passed through. 
 *
 * @param domain_sid The domain sid aquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on compleation of the function.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_reg_shutdown_abort_internals(const DOM_SID *domain_sid, 
						const char *domain_name, 
						struct cli_state *cli, 
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv) 
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	
	result = werror_to_ntstatus(rpccli_reg_abort_shutdown(pipe_hnd, mem_ctx));
	
	if (NT_STATUS_IS_OK(result)) {
		d_printf("\nShutdown successfully aborted\n");
		DEBUG(5,("cmd_reg_abort_shutdown: query succeeded\n"));
	} else
		DEBUG(5,("cmd_reg_abort_shutdown: query failed\n"));
	
	return result;
}

/** 
 * ABORT the Shut down of a remote RPC server
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/

static int rpc_shutdown_abort(int argc, const char **argv) 
{
	int rc = run_rpc_command(NULL, PI_SHUTDOWN, 0, 
				 rpc_shutdown_abort_internals,
				 argc, argv);

	if (rc == 0)
		return rc;

	DEBUG(1, ("initshutdown pipe didn't work, trying winreg pipe\n"));

	return run_rpc_command(NULL, PI_WINREG, 0, 
			       rpc_reg_shutdown_abort_internals,
			       argc, argv);
}

/** 
 * Shut down a remote RPC Server via initshutdown pipe
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid aquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on compleation of the function.
 * @param argc  Standard main() style argc
 * @param argc  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_init_shutdown_internals(const DOM_SID *domain_sid, 
						const char *domain_name, 
						struct cli_state *cli, 
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv) 
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
        const char *msg = "This machine will be shutdown shortly";
	uint32 timeout = 20;

	if (opt_comment) {
		msg = opt_comment;
	}
	if (opt_timeout) {
		timeout = opt_timeout;
	}

	/* create an entry */
	result = rpccli_shutdown_init(pipe_hnd, mem_ctx, msg, timeout, opt_reboot, 
				   opt_force);

	if (NT_STATUS_IS_OK(result)) {
		d_printf("\nShutdown of remote machine succeeded\n");
		DEBUG(5,("Shutdown of remote machine succeeded\n"));
	} else {
		DEBUG(1,("Shutdown of remote machine failed!\n"));
	}
	return result;
}

/** 
 * Shut down a remote RPC Server via winreg pipe
 *
 * All parameters are provided by the run_rpc_command function, except for
 * argc, argv which are passes through. 
 *
 * @param domain_sid The domain sid aquired from the remote server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on compleation of the function.
 * @param argc  Standard main() style argc
 * @param argc  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return Normal NTSTATUS return.
 **/

static NTSTATUS rpc_reg_shutdown_internals(const DOM_SID *domain_sid, 
						const char *domain_name, 
						struct cli_state *cli, 
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv) 
{
	WERROR result;
        const char *msg = "This machine will be shutdown shortly";
	uint32 timeout = 20;
#if 0
	poptContext pc;
	int rc;

	struct poptOption long_options[] = {
		{"message",    'm', POPT_ARG_STRING, &msg},
		{"timeout",    't', POPT_ARG_INT,    &timeout},
		{"reboot",     'r', POPT_ARG_NONE,   &reboot},
		{"force",      'f', POPT_ARG_NONE,   &force},
		{ 0, 0, 0, 0}
	};

	pc = poptGetContext(NULL, argc, (const char **) argv, long_options, 
			    POPT_CONTEXT_KEEP_FIRST);

	rc = poptGetNextOpt(pc);
	
	if (rc < -1) {
		/* an error occurred during option processing */
		DEBUG(0, ("%s: %s\n",
			  poptBadOption(pc, POPT_BADOPTION_NOALIAS),
			  poptStrerror(rc)));
		return NT_STATUS_INVALID_PARAMETER;
	}
#endif
	if (opt_comment) {
		msg = opt_comment;
	}
	if (opt_timeout) {
		timeout = opt_timeout;
	}

	/* create an entry */
	result = rpccli_reg_shutdown(pipe_hnd, mem_ctx, msg, timeout, opt_reboot, opt_force);

	if (W_ERROR_IS_OK(result)) {
		d_printf("\nShutdown of remote machine succeeded\n");
	} else {
		d_fprintf(stderr, "\nShutdown of remote machine failed\n");
		if (W_ERROR_EQUAL(result,WERR_MACHINE_LOCKED))
			d_fprintf(stderr, "\nMachine locked, use -f switch to force\n");
		else
			d_fprintf(stderr, "\nresult was: %s\n", dos_errstr(result));
	}

	return werror_to_ntstatus(result);
}

/** 
 * Shut down a remote RPC server
 *
 * @param argc  Standard main() style argc
 * @param argc  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/

static int rpc_shutdown(int argc, const char **argv) 
{
	int rc = run_rpc_command(NULL, PI_SHUTDOWN, 0, 
				 rpc_init_shutdown_internals,
				 argc, argv);

	if (rc) {
		DEBUG(1, ("initshutdown pipe failed, trying winreg pipe\n"));
		rc = run_rpc_command(NULL, PI_WINREG, 0, 
				     rpc_reg_shutdown_internals, argc, argv);
	}

	return rc;
}

/***************************************************************************
  NT Domain trusts code (i.e. 'net rpc trustdom' functionality)
  
 ***************************************************************************/

/**
 * Add interdomain trust account to the RPC server.
 * All parameters (except for argc and argv) are passed by run_rpc_command
 * function.
 *
 * @param domain_sid The domain sid acquired from the server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argc  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return normal NTSTATUS return code
 */

static NTSTATUS rpc_trustdom_add_internals(const DOM_SID *domain_sid, 
						const char *domain_name, 
						struct cli_state *cli,
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv)
{
	POLICY_HND connect_pol, domain_pol, user_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	char *acct_name;
	uint32 acb_info;
	uint32 unknown, user_rid;

	if (argc != 2) {
		d_printf("Usage: net rpc trustdom add <domain_name> <pw>\n");
		return NT_STATUS_INVALID_PARAMETER;
	}

	/* 
	 * Make valid trusting domain account (ie. uppercased and with '$' appended)
	 */
	 
	if (asprintf(&acct_name, "%s$", argv[0]) < 0) {
		return NT_STATUS_NO_MEMORY;
	}

	strupper_m(acct_name);

	/* Get samr policy handle */
	result = rpccli_samr_connect(pipe_hnd, mem_ctx, MAXIMUM_ALLOWED_ACCESS,
				  &connect_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}
	
	/* Get domain policy handle */
	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      domain_sid, &domain_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	/* Create trusting domain's account */
	acb_info = ACB_NORMAL; 
	unknown = 0xe00500b0; /* No idea what this is - a permission mask?
	                         mimir: yes, most probably it is */

	result = rpccli_samr_create_dom_user(pipe_hnd, mem_ctx, &domain_pol,
					  acct_name, acb_info, unknown,
					  &user_pol, &user_rid);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	{
		SAM_USERINFO_CTR ctr;
		SAM_USER_INFO_23 p23;
		NTTIME notime;
		char nostr[] = "";
		LOGON_HRS hrs;
		uchar pwbuf[516];

		encode_pw_buffer(pwbuf, argv[1], STR_UNICODE);

		ZERO_STRUCT(ctr);
		ZERO_STRUCT(p23);
		ZERO_STRUCT(notime);
		hrs.max_len = 1260;
		hrs.offset = 0;
		hrs.len = 21;
		memset(hrs.hours, 0xFF, sizeof(hrs.hours));
		acb_info = ACB_DOMTRUST;

		init_sam_user_info23A(&p23, &notime, &notime, &notime,
				      &notime, &notime, &notime,
				      nostr, nostr, nostr, nostr, nostr,
				      nostr, nostr, nostr, nostr, nostr,
				      0, 0, acb_info, ACCT_FLAGS, 168, &hrs, 
				      0, 0, (char *)pwbuf);
		ctr.switch_value = 23;
		ctr.info.id23 = &p23;
		p23.passmustchange = 0;

		result = rpccli_samr_set_userinfo(pipe_hnd, mem_ctx, &user_pol, 23,
					       &cli->user_session_key, &ctr);

		if (!NT_STATUS_IS_OK(result)) {
			DEBUG(0,("Could not set trust account password: %s\n",
				 nt_errstr(result)));
			goto done;
		}
	}

 done:
	SAFE_FREE(acct_name);
	return result;
}

/**
 * Create interdomain trust account for a remote domain.
 *
 * @param argc standard argc
 * @param argv standard argv without initial components
 *
 * @return Integer status (0 means success)
 **/

static int rpc_trustdom_add(int argc, const char **argv)
{
	if (argc > 0) {
		return run_rpc_command(NULL, PI_SAMR, 0, rpc_trustdom_add_internals,
		                       argc, argv);
	} else {
		d_printf("Usage: net rpc trustdom add <domain>\n");
		return -1;
	}
}


/**
 * Remove interdomain trust account from the RPC server.
 * All parameters (except for argc and argv) are passed by run_rpc_command
 * function.
 *
 * @param domain_sid The domain sid acquired from the server
 * @param cli A cli_state connected to the server.
 * @param mem_ctx Talloc context, destoyed on completion of the function.
 * @param argc  Standard main() style argc
 * @param argc  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return normal NTSTATUS return code
 */

static NTSTATUS rpc_trustdom_del_internals(const DOM_SID *domain_sid, 
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv)
{
	POLICY_HND connect_pol, domain_pol, user_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	char *acct_name;
	const char **names;
	DOM_SID trust_acct_sid;
	uint32 *user_rids, num_rids, *name_types;
	uint32 flags = 0x000003e8; /* Unknown */

	if (argc != 1) {
		d_printf("Usage: net rpc trustdom del <domain_name>\n");
		return NT_STATUS_INVALID_PARAMETER;
	}

	/* 
	 * Make valid trusting domain account (ie. uppercased and with '$' appended)
	 */
	acct_name = talloc_asprintf(mem_ctx, "%s$", argv[0]);

	if (acct_name == NULL)
		return NT_STATUS_NO_MEMORY;

	strupper_m(acct_name);

	if ((names = TALLOC_ARRAY(mem_ctx, const char *, 1)) == NULL) {
		return NT_STATUS_NO_MEMORY;
	}
	names[0] = acct_name;


	/* Get samr policy handle */
	result = rpccli_samr_connect(pipe_hnd, mem_ctx, MAXIMUM_ALLOWED_ACCESS,
				  &connect_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}
	
	/* Get domain policy handle */
	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      domain_sid, &domain_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_samr_lookup_names(pipe_hnd, mem_ctx, &domain_pol, flags, 1,
				       names, &num_rids,
				       &user_rids, &name_types);
	
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_samr_open_user(pipe_hnd, mem_ctx, &domain_pol,
				    MAXIMUM_ALLOWED_ACCESS,
				    user_rids[0], &user_pol);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	/* append the rid to the domain sid */
	sid_copy(&trust_acct_sid, domain_sid);
	if (!sid_append_rid(&trust_acct_sid, user_rids[0])) {
		goto done;
	}

	/* remove the sid */

	result = rpccli_samr_remove_sid_foreign_domain(pipe_hnd, mem_ctx, &user_pol,
						    &trust_acct_sid);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	/* Delete user */

	result = rpccli_samr_delete_dom_user(pipe_hnd, mem_ctx, &user_pol);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	if (!NT_STATUS_IS_OK(result)) {
	  DEBUG(0,("Could not set trust account password: %s\n",
		   nt_errstr(result)));
	  goto done;
	}

 done:
	return result;
}

/**
 * Delete interdomain trust account for a remote domain.
 *
 * @param argc standard argc
 * @param argv standard argv without initial components
 *
 * @return Integer status (0 means success)
 **/

static int rpc_trustdom_del(int argc, const char **argv)
{
	if (argc > 0) {
		return run_rpc_command(NULL, PI_SAMR, 0, rpc_trustdom_del_internals,
		                       argc, argv);
	} else {
		d_printf("Usage: net rpc trustdom del <domain>\n");
		return -1;
	}
}
 

/**
 * Establish trust relationship to a trusting domain.
 * Interdomain account must already be created on remote PDC.
 *
 * @param argc standard argc
 * @param argv standard argv without initial components
 *
 * @return Integer status (0 means success)
 **/

static int rpc_trustdom_establish(int argc, const char **argv)
{
	struct cli_state *cli = NULL;
	struct in_addr server_ip;
	struct rpc_pipe_client *pipe_hnd = NULL;
	POLICY_HND connect_hnd;
	TALLOC_CTX *mem_ctx;
	NTSTATUS nt_status;
	DOM_SID *domain_sid;
	
	char* domain_name;
	char* domain_name_pol;
	char* acct_name;
	fstring pdc_name;

	/*
	 * Connect to \\server\ipc$ as 'our domain' account with password
	 */

	if (argc != 1) {
		d_printf("Usage: net rpc trustdom establish <domain_name>\n");
		return -1;
	}

	domain_name = smb_xstrdup(argv[0]);
	strupper_m(domain_name);

	/* account name used at first is our domain's name with '$' */
	asprintf(&acct_name, "%s$", lp_workgroup());
	strupper_m(acct_name);
	
	/*
	 * opt_workgroup will be used by connection functions further,
	 * hence it should be set to remote domain name instead of ours
	 */
	if (opt_workgroup) {
		opt_workgroup = smb_xstrdup(domain_name);
	};
	
	opt_user_name = acct_name;

	/* find the domain controller */
	if (!net_find_pdc(&server_ip, pdc_name, domain_name)) {
		DEBUG(0, ("Couldn't find domain controller for domain %s\n", domain_name));
		return -1;
	}

	/* connect to ipc$ as username/password */
	nt_status = connect_to_ipc(&cli, &server_ip, pdc_name);
	if (!NT_STATUS_EQUAL(nt_status, NT_STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT)) {

		/* Is it trusting domain account for sure ? */
		DEBUG(0, ("Couldn't verify trusting domain account. Error was %s\n",
			nt_errstr(nt_status)));
		return -1;
	}

	/* store who we connected to */

	saf_store( domain_name, pdc_name );
	
	/*
	 * Connect to \\server\ipc$ again (this time anonymously)
	 */
	
	nt_status = connect_to_ipc_anonymous(&cli, &server_ip, (char*)pdc_name);
	
	if (NT_STATUS_IS_ERR(nt_status)) {
		DEBUG(0, ("Couldn't connect to domain %s controller. Error was %s.\n",
			domain_name, nt_errstr(nt_status)));
	}

	/*
	 * Use NetServerEnum2 to make sure we're talking to a proper server
	 */
	 
	if (!cli_get_pdc_name(cli, domain_name, (char*)pdc_name)) {
		DEBUG(0, ("NetServerEnum2 error: Couldn't find primary domain controller\
			 for domain %s\n", domain_name));
	}
	 
	if (!(mem_ctx = talloc_init("establishing trust relationship to "
				    "domain %s", domain_name))) {
		DEBUG(0, ("talloc_init() failed\n"));
		cli_shutdown(cli);
		return -1;
	}

	/*
	 * Call LsaOpenPolicy and LsaQueryInfo
	 */
	 
	pipe_hnd = cli_rpc_pipe_open_noauth(cli, PI_LSARPC, &nt_status);
	if (!pipe_hnd) {
		DEBUG(0, ("Could not initialise lsa pipe. Error was %s\n", nt_errstr(nt_status) ));
		cli_shutdown(cli);
		return -1;
	}

	nt_status = rpccli_lsa_open_policy2(pipe_hnd, mem_ctx, True, SEC_RIGHTS_QUERY_VALUE,
	                                 &connect_hnd);
	if (NT_STATUS_IS_ERR(nt_status)) {
		DEBUG(0, ("Couldn't open policy handle. Error was %s\n",
			nt_errstr(nt_status)));
		cli_shutdown(cli);
		return -1;
	}

	/* Querying info level 5 */
	
	nt_status = rpccli_lsa_query_info_policy(pipe_hnd, mem_ctx, &connect_hnd,
	                                      5 /* info level */,
					      &domain_name_pol, &domain_sid);
	if (NT_STATUS_IS_ERR(nt_status)) {
		DEBUG(0, ("LSA Query Info failed. Returned error was %s\n",
			nt_errstr(nt_status)));
		cli_shutdown(cli);
		return -1;
	}

	/* There should be actually query info level 3 (following nt serv behaviour),
	   but I still don't know if it's _really_ necessary */
			
	/*
	 * Store the password in secrets db
	 */

	if (!secrets_store_trusted_domain_password(domain_name,
						   opt_password,
						   domain_sid)) {
		DEBUG(0, ("Storing password for trusted domain failed.\n"));
		cli_shutdown(cli);
		return -1;
	}
	
	/*
	 * Close the pipes and clean up
	 */
	 
	nt_status = rpccli_lsa_close(pipe_hnd, mem_ctx, &connect_hnd);
	if (NT_STATUS_IS_ERR(nt_status)) {
		DEBUG(0, ("Couldn't close LSA pipe. Error was %s\n",
			nt_errstr(nt_status)));
		cli_shutdown(cli);
		return -1;
	}

	cli_shutdown(cli);
	 
	talloc_destroy(mem_ctx);
	 
	d_printf("Trust to domain %s established\n", domain_name);
	return 0;
}

/**
 * Revoke trust relationship to the remote domain
 *
 * @param argc standard argc
 * @param argv standard argv without initial components
 *
 * @return Integer status (0 means success)
 **/

static int rpc_trustdom_revoke(int argc, const char **argv)
{
	char* domain_name;

	if (argc < 1) return -1;
	
	/* generate upper cased domain name */
	domain_name = smb_xstrdup(argv[0]);
	strupper_m(domain_name);

	/* delete password of the trust */
	if (!trusted_domain_password_delete(domain_name)) {
		DEBUG(0, ("Failed to revoke relationship to the trusted domain %s\n",
			  domain_name));
		return -1;
	};
	
	return 0;
}

/**
 * Usage for 'net rpc trustdom' command
 *
 * @param argc standard argc
 * @param argv standard argv without inital components
 *
 * @return Integer status returned to shell
 **/
 
static int rpc_trustdom_usage(int argc, const char **argv)
{
	d_printf("  net rpc trustdom add \t\t add trusting domain's account\n");
	d_printf("  net rpc trustdom del \t\t delete trusting domain's account\n");
	d_printf("  net rpc trustdom establish \t establish relationship to trusted domain\n");
	d_printf("  net rpc trustdom revoke \t abandon relationship to trusted domain\n");
	d_printf("  net rpc trustdom list \t show current interdomain trust relationships\n");
	d_printf("  net rpc trustdom vampire \t vampire interdomain trust relationships from remote server\n");
	return -1;
}


static NTSTATUS rpc_query_domain_sid(const DOM_SID *domain_sid, 
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx,
					int argc,
					const char **argv)
{
	fstring str_sid;
	sid_to_string(str_sid, domain_sid);
	d_printf("%s\n", str_sid);
	return NT_STATUS_OK;
}

static void print_trusted_domain(DOM_SID *dom_sid, const char *trusted_dom_name)
{
	fstring ascii_sid, padding;
	int pad_len, col_len = 20;

	/* convert sid into ascii string */
	sid_to_string(ascii_sid, dom_sid);

	/* calculate padding space for d_printf to look nicer */
	pad_len = col_len - strlen(trusted_dom_name);
	padding[pad_len] = 0;
	do padding[--pad_len] = ' '; while (pad_len);
			
	d_printf("%s%s%s\n", trusted_dom_name, padding, ascii_sid);
}

static NTSTATUS vampire_trusted_domain(struct rpc_pipe_client *pipe_hnd,
				      TALLOC_CTX *mem_ctx, 
				      POLICY_HND *pol, 
				      DOM_SID dom_sid, 
				      const char *trusted_dom_name)
{
	NTSTATUS nt_status;
	LSA_TRUSTED_DOMAIN_INFO *info;
	char *cleartextpwd = NULL;
	DATA_BLOB data;

	nt_status = rpccli_lsa_query_trusted_domain_info_by_sid(pipe_hnd, mem_ctx, pol, 4, &dom_sid, &info);
	
	if (NT_STATUS_IS_ERR(nt_status)) {
		DEBUG(0,("Could not query trusted domain info. Error was %s\n",
		nt_errstr(nt_status)));
		goto done;
	}

	data = data_blob(NULL, info->password.password.length);

	memcpy(data.data, info->password.password.data, info->password.password.length);
	data.length 	= info->password.password.length;
				
	cleartextpwd = decrypt_trustdom_secret(pipe_hnd->cli->pwd.password, &data);

	if (cleartextpwd == NULL) {
		DEBUG(0,("retrieved NULL password\n"));
		nt_status = NT_STATUS_UNSUCCESSFUL;
		goto done;
	}
	
	if (!secrets_store_trusted_domain_password(trusted_dom_name,
						   cleartextpwd,
						   &dom_sid)) {
		DEBUG(0, ("Storing password for trusted domain failed.\n"));
		nt_status = NT_STATUS_UNSUCCESSFUL;
		goto done;
	}

#ifdef DEBUG_PASSWORD
	DEBUG(100,("sucessfully vampired trusted domain [%s], sid: [%s], password: [%s]\n",  
		trusted_dom_name, sid_string_static(&dom_sid), cleartextpwd));
#endif

done:
	SAFE_FREE(cleartextpwd);
	data_blob_free(&data);

	return nt_status;
}

static int rpc_trustdom_vampire(int argc, const char **argv)
{
	/* common variables */
	TALLOC_CTX* mem_ctx;
	struct cli_state *cli = NULL;
	struct rpc_pipe_client *pipe_hnd = NULL;
	NTSTATUS nt_status;
	const char *domain_name = NULL;
	DOM_SID *queried_dom_sid;
	POLICY_HND connect_hnd;

	/* trusted domains listing variables */
	unsigned int num_domains, enum_ctx = 0;
	int i;
	DOM_SID *domain_sids;
	char **trusted_dom_names;
	fstring pdc_name;
	char *dummy;

	/*
	 * Listing trusted domains (stored in secrets.tdb, if local)
	 */

	mem_ctx = talloc_init("trust relationships vampire");

	/*
	 * set domain and pdc name to local samba server (default)
	 * or to remote one given in command line
	 */

	if (StrCaseCmp(opt_workgroup, lp_workgroup())) {
		domain_name = opt_workgroup;
		opt_target_workgroup = opt_workgroup;
	} else {
		fstrcpy(pdc_name, global_myname());
		domain_name = talloc_strdup(mem_ctx, lp_workgroup());
		opt_target_workgroup = domain_name;
	};

	/* open \PIPE\lsarpc and open policy handle */
	if (!(cli = net_make_ipc_connection(NET_FLAGS_PDC))) {
		DEBUG(0, ("Couldn't connect to domain controller\n"));
		return -1;
	};

	pipe_hnd = cli_rpc_pipe_open_noauth(cli, PI_LSARPC, &nt_status);
	if (!pipe_hnd) {
		DEBUG(0, ("Could not initialise lsa pipe. Error was %s\n",
			nt_errstr(nt_status) ));
		cli_shutdown(cli);
		return -1;
	};

	nt_status = rpccli_lsa_open_policy2(pipe_hnd, mem_ctx, False, SEC_RIGHTS_QUERY_VALUE,
					&connect_hnd);
	if (NT_STATUS_IS_ERR(nt_status)) {
		DEBUG(0, ("Couldn't open policy handle. Error was %s\n",
 			nt_errstr(nt_status)));
		cli_shutdown(cli);
		return -1;
	};

	/* query info level 5 to obtain sid of a domain being queried */
	nt_status = rpccli_lsa_query_info_policy(
		pipe_hnd, mem_ctx, &connect_hnd, 5 /* info level */, 
		&dummy, &queried_dom_sid);

	if (NT_STATUS_IS_ERR(nt_status)) {
		DEBUG(0, ("LSA Query Info failed. Returned error was %s\n",
			nt_errstr(nt_status)));
		cli_shutdown(cli);
		return -1;
	}

	/*
	 * Keep calling LsaEnumTrustdom over opened pipe until
	 * the end of enumeration is reached
	 */

	d_printf("Vampire trusted domains:\n\n");

	do {
		nt_status = rpccli_lsa_enum_trust_dom(pipe_hnd, mem_ctx, &connect_hnd, &enum_ctx,
						   &num_domains,
						   &trusted_dom_names, &domain_sids);
		
		if (NT_STATUS_IS_ERR(nt_status)) {
			DEBUG(0, ("Couldn't enumerate trusted domains. Error was %s\n",
				nt_errstr(nt_status)));
			cli_shutdown(cli);
			return -1;
		};
		
		for (i = 0; i < num_domains; i++) {

			print_trusted_domain(&(domain_sids[i]), trusted_dom_names[i]);

			nt_status = vampire_trusted_domain(pipe_hnd, mem_ctx, &connect_hnd, 
							   domain_sids[i], trusted_dom_names[i]);
			if (!NT_STATUS_IS_OK(nt_status)) {
				cli_shutdown(cli);
				return -1;
			}
		};

		/*
		 * in case of no trusted domains say something rather
		 * than just display blank line
		 */
		if (!num_domains) d_printf("none\n");

	} while (NT_STATUS_EQUAL(nt_status, STATUS_MORE_ENTRIES));

	/* close this connection before doing next one */
	nt_status = rpccli_lsa_close(pipe_hnd, mem_ctx, &connect_hnd);
	if (NT_STATUS_IS_ERR(nt_status)) {
		DEBUG(0, ("Couldn't properly close lsa policy handle. Error was %s\n",
			nt_errstr(nt_status)));
		cli_shutdown(cli);
		return -1;
	};

	/* close lsarpc pipe and connection to IPC$ */
	cli_shutdown(cli);

	talloc_destroy(mem_ctx);	 
	return 0;
}

static int rpc_trustdom_list(int argc, const char **argv)
{
	/* common variables */
	TALLOC_CTX* mem_ctx;
	struct cli_state *cli = NULL, *remote_cli = NULL;
	struct rpc_pipe_client *pipe_hnd = NULL;
	NTSTATUS nt_status;
	const char *domain_name = NULL;
	DOM_SID *queried_dom_sid;
	fstring padding;
	int ascii_dom_name_len;
	POLICY_HND connect_hnd;
	
	/* trusted domains listing variables */
	unsigned int num_domains, enum_ctx = 0;
	int i, pad_len, col_len = 20;
	DOM_SID *domain_sids;
	char **trusted_dom_names;
	fstring pdc_name;
	char *dummy;
	
	/* trusting domains listing variables */
	POLICY_HND domain_hnd;
	char **trusting_dom_names;
	uint32 *trusting_dom_rids;
	
	/*
	 * Listing trusted domains (stored in secrets.tdb, if local)
	 */

	mem_ctx = talloc_init("trust relationships listing");

	/*
	 * set domain and pdc name to local samba server (default)
	 * or to remote one given in command line
	 */
	
	if (StrCaseCmp(opt_workgroup, lp_workgroup())) {
		domain_name = opt_workgroup;
		opt_target_workgroup = opt_workgroup;
	} else {
		fstrcpy(pdc_name, global_myname());
		domain_name = talloc_strdup(mem_ctx, lp_workgroup());
		opt_target_workgroup = domain_name;
	};

	/* open \PIPE\lsarpc and open policy handle */
	if (!(cli = net_make_ipc_connection(NET_FLAGS_PDC))) {
		DEBUG(0, ("Couldn't connect to domain controller\n"));
		return -1;
	};

	pipe_hnd = cli_rpc_pipe_open_noauth(cli, PI_LSARPC, &nt_status);
	if (!pipe_hnd) {
		DEBUG(0, ("Could not initialise lsa pipe. Error was %s\n",
			nt_errstr(nt_status) ));
		return -1;
	};

	nt_status = rpccli_lsa_open_policy2(pipe_hnd, mem_ctx, False, SEC_RIGHTS_QUERY_VALUE,
					&connect_hnd);
	if (NT_STATUS_IS_ERR(nt_status)) {
		DEBUG(0, ("Couldn't open policy handle. Error was %s\n",
 			nt_errstr(nt_status)));
		return -1;
	};
	
	/* query info level 5 to obtain sid of a domain being queried */
	nt_status = rpccli_lsa_query_info_policy(
		pipe_hnd, mem_ctx, &connect_hnd, 5 /* info level */, 
		&dummy, &queried_dom_sid);

	if (NT_STATUS_IS_ERR(nt_status)) {
		DEBUG(0, ("LSA Query Info failed. Returned error was %s\n",
			nt_errstr(nt_status)));
		return -1;
	}
		
	/*
	 * Keep calling LsaEnumTrustdom over opened pipe until
	 * the end of enumeration is reached
	 */
	 
	d_printf("Trusted domains list:\n\n");

	do {
		nt_status = rpccli_lsa_enum_trust_dom(pipe_hnd, mem_ctx, &connect_hnd, &enum_ctx,
						   &num_domains,
						   &trusted_dom_names, &domain_sids);
		
		if (NT_STATUS_IS_ERR(nt_status)) {
			DEBUG(0, ("Couldn't enumerate trusted domains. Error was %s\n",
				nt_errstr(nt_status)));
			return -1;
		};
		
		for (i = 0; i < num_domains; i++) {
			print_trusted_domain(&(domain_sids[i]), trusted_dom_names[i]);
		};
		
		/*
		 * in case of no trusted domains say something rather
		 * than just display blank line
		 */
		if (!num_domains) d_printf("none\n");

	} while (NT_STATUS_EQUAL(nt_status, STATUS_MORE_ENTRIES));

	/* close this connection before doing next one */
	nt_status = rpccli_lsa_close(pipe_hnd, mem_ctx, &connect_hnd);
	if (NT_STATUS_IS_ERR(nt_status)) {
		DEBUG(0, ("Couldn't properly close lsa policy handle. Error was %s\n",
			nt_errstr(nt_status)));
		return -1;
	};
	
	cli_rpc_pipe_close(pipe_hnd);

	/*
	 * Listing trusting domains (stored in passdb backend, if local)
	 */
	
	d_printf("\nTrusting domains list:\n\n");

	/*
	 * Open \PIPE\samr and get needed policy handles
	 */
	pipe_hnd = cli_rpc_pipe_open_noauth(cli, PI_SAMR, &nt_status);
	if (!pipe_hnd) {
		DEBUG(0, ("Could not initialise samr pipe. Error was %s\n", nt_errstr(nt_status)));
		return -1;
	};
	
	/* SamrConnect */
	nt_status = rpccli_samr_connect(pipe_hnd, mem_ctx, SA_RIGHT_SAM_OPEN_DOMAIN,
								 &connect_hnd);
	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(0, ("Couldn't open SAMR policy handle. Error was %s\n",
			nt_errstr(nt_status)));
		return -1;
	};
	
	/* SamrOpenDomain - we have to open domain policy handle in order to be
	   able to enumerate accounts*/
	nt_status = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_hnd,
					 SA_RIGHT_DOMAIN_ENUM_ACCOUNTS,
					 queried_dom_sid, &domain_hnd);									 
	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(0, ("Couldn't open domain object. Error was %s\n",
			nt_errstr(nt_status)));
		return -1;
	};
	
	/*
	 * perform actual enumeration
	 */
	 
	enum_ctx = 0;	/* reset enumeration context from last enumeration */
	do {
			
		nt_status = rpccli_samr_enum_dom_users(pipe_hnd, mem_ctx, &domain_hnd,
		                                    &enum_ctx, ACB_DOMTRUST, 0xffff,
		                                    &trusting_dom_names, &trusting_dom_rids,
		                                    &num_domains);
		if (NT_STATUS_IS_ERR(nt_status)) {
			DEBUG(0, ("Couldn't enumerate accounts. Error was: %s\n",
				nt_errstr(nt_status)));
			return -1;
		};
		
		for (i = 0; i < num_domains; i++) {

			/*
			 * get each single domain's sid (do we _really_ need this ?):
			 *  1) connect to domain's pdc
			 *  2) query the pdc for domain's sid
			 */

			/* get rid of '$' tail */
			ascii_dom_name_len = strlen(trusting_dom_names[i]);
			if (ascii_dom_name_len && ascii_dom_name_len < FSTRING_LEN)
				trusting_dom_names[i][ascii_dom_name_len - 1] = '\0';
			
			/* calculate padding space for d_printf to look nicer */
			pad_len = col_len - strlen(trusting_dom_names[i]);
			padding[pad_len] = 0;
			do padding[--pad_len] = ' '; while (pad_len);

			/* set opt_* variables to remote domain */
			strupper_m(trusting_dom_names[i]);
			opt_workgroup = talloc_strdup(mem_ctx, trusting_dom_names[i]);
			opt_target_workgroup = opt_workgroup;
			
			d_printf("%s%s", trusting_dom_names[i], padding);
			
			/* connect to remote domain controller */
			remote_cli = net_make_ipc_connection(NET_FLAGS_PDC | NET_FLAGS_ANONYMOUS);
			if (remote_cli) {			
				/* query for domain's sid */
				if (run_rpc_command(remote_cli, PI_LSARPC, 0, rpc_query_domain_sid, argc, argv))
					d_fprintf(stderr, "couldn't get domain's sid\n");

				cli_shutdown(remote_cli);
			
			} else {
				d_fprintf(stderr, "domain controller is not responding\n");
			};
		};
		
		if (!num_domains) d_printf("none\n");
		
	} while (NT_STATUS_EQUAL(nt_status, STATUS_MORE_ENTRIES));

	/* close opened samr and domain policy handles */
	nt_status = rpccli_samr_close(pipe_hnd, mem_ctx, &domain_hnd);
	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(0, ("Couldn't properly close domain policy handle for domain %s\n", domain_name));
	};
	
	nt_status = rpccli_samr_close(pipe_hnd, mem_ctx, &connect_hnd);
	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(0, ("Couldn't properly close samr policy handle for domain %s\n", domain_name));
	};
	
	/* close samr pipe and connection to IPC$ */
	cli_shutdown(cli);

	talloc_destroy(mem_ctx);	 
	return 0;
}

/**
 * Entrypoint for 'net rpc trustdom' code
 *
 * @param argc standard argc
 * @param argv standard argv without initial components
 *
 * @return Integer status (0 means success)
 */

static int rpc_trustdom(int argc, const char **argv)
{
	struct functable func[] = {
		{"add", rpc_trustdom_add},
		{"del", rpc_trustdom_del},
		{"establish", rpc_trustdom_establish},
		{"revoke", rpc_trustdom_revoke},
		{"help", rpc_trustdom_usage},
		{"list", rpc_trustdom_list},
		{"vampire", rpc_trustdom_vampire},
		{NULL, NULL}
	};

	if (argc == 0) {
		rpc_trustdom_usage(argc, argv);
		return -1;
	}

	return (net_run_function(argc, argv, func, rpc_user_usage));
}

/**
 * Check if a server will take rpc commands
 * @param flags	Type of server to connect to (PDC, DMB, localhost)
 *		if the host is not explicitly specified
 * @return  BOOL (true means rpc supported)
 */
BOOL net_rpc_check(unsigned flags)
{
	struct cli_state cli;
	BOOL ret = False;
	struct in_addr server_ip;
	char *server_name = NULL;

	/* flags (i.e. server type) may depend on command */
	if (!net_find_server(NULL, flags, &server_ip, &server_name))
		return False;

	ZERO_STRUCT(cli);
	if (cli_initialise(&cli) == False)
		return False;

	if (!cli_connect(&cli, server_name, &server_ip))
		goto done;
	if (!attempt_netbios_session_request(&cli, global_myname(), 
					     server_name, &server_ip))
		goto done;
	if (!cli_negprot(&cli))
		goto done;
	if (cli.protocol < PROTOCOL_NT1)
		goto done;

	ret = True;
 done:
	cli_shutdown(&cli);
	return ret;
}

/* dump sam database via samsync rpc calls */
static int rpc_samdump(int argc, const char **argv) {
		return run_rpc_command(NULL, PI_NETLOGON, NET_FLAGS_ANONYMOUS, rpc_samdump_internals,
			       argc, argv);
}

/* syncronise sam database via samsync rpc calls */
static int rpc_vampire(int argc, const char **argv) {
	return run_rpc_command(NULL, PI_NETLOGON, NET_FLAGS_ANONYMOUS, rpc_vampire_internals,
			       argc, argv);
}

/** 
 * Migrate everything from a print-server
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 *
 * The order is important !
 * To successfully add drivers the print-queues have to exist !
 * Applying ACLs should be the last step, because you're easily locked out
 *
 **/
static int rpc_printer_migrate_all(int argc, const char **argv)
{
	int ret;

	if (!opt_host) {
		printf("no server to migrate\n");
		return -1;
	}

	ret = run_rpc_command(NULL, PI_SPOOLSS, 0, rpc_printer_migrate_printers_internals, argc, argv);
	if (ret)
		return ret;

	ret = run_rpc_command(NULL, PI_SPOOLSS, 0, rpc_printer_migrate_drivers_internals, argc, argv);
	if (ret)
		return ret;

	ret = run_rpc_command(NULL, PI_SPOOLSS, 0, rpc_printer_migrate_forms_internals, argc, argv);
	if (ret)
		return ret;

	ret = run_rpc_command(NULL, PI_SPOOLSS, 0, rpc_printer_migrate_settings_internals, argc, argv);
	if (ret)
		return ret;

	return run_rpc_command(NULL, PI_SPOOLSS, 0, rpc_printer_migrate_security_internals, argc, argv);

}

/** 
 * Migrate print-drivers from a print-server
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/
static int rpc_printer_migrate_drivers(int argc, const char **argv)
{
	if (!opt_host) {
		printf("no server to migrate\n");
		return -1;
	}

	return run_rpc_command(NULL, PI_SPOOLSS, 0, 
			       rpc_printer_migrate_drivers_internals,
			       argc, argv);
}

/** 
 * Migrate print-forms from a print-server
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/
static int rpc_printer_migrate_forms(int argc, const char **argv)
{
	if (!opt_host) {
		printf("no server to migrate\n");
		return -1;
	}

	return run_rpc_command(NULL, PI_SPOOLSS, 0, 
			       rpc_printer_migrate_forms_internals,
			       argc, argv);
}

/** 
 * Migrate printers from a print-server
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/
static int rpc_printer_migrate_printers(int argc, const char **argv)
{
	if (!opt_host) {
		printf("no server to migrate\n");
		return -1;
	}

	return run_rpc_command(NULL, PI_SPOOLSS, 0, 
			       rpc_printer_migrate_printers_internals,
			       argc, argv);
}

/** 
 * Migrate printer-ACLs from a print-server
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/
static int rpc_printer_migrate_security(int argc, const char **argv)
{
	if (!opt_host) {
		printf("no server to migrate\n");
		return -1;
	}

	return run_rpc_command(NULL, PI_SPOOLSS, 0, 
			       rpc_printer_migrate_security_internals,
			       argc, argv);
}

/** 
 * Migrate printer-settings from a print-server
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/
static int rpc_printer_migrate_settings(int argc, const char **argv)
{
	if (!opt_host) {
		printf("no server to migrate\n");
		return -1;
	}

	return run_rpc_command(NULL, PI_SPOOLSS, 0, 
			       rpc_printer_migrate_settings_internals,
			       argc, argv);
}

/** 
 * 'net rpc printer' entrypoint.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 **/

int rpc_printer_migrate(int argc, const char **argv) 
{

	/* ouch: when addriver and setdriver are called from within
	   rpc_printer_migrate_drivers_internals, the printer-queue already
	   *has* to exist */

	struct functable func[] = {
		{"all", 	rpc_printer_migrate_all},
		{"drivers", 	rpc_printer_migrate_drivers},
		{"forms", 	rpc_printer_migrate_forms},
		{"help", 	rpc_printer_usage},
		{"printers", 	rpc_printer_migrate_printers},
		{"security", 	rpc_printer_migrate_security},
		{"settings", 	rpc_printer_migrate_settings},
		{NULL, NULL}
	};

	return net_run_function(argc, argv, func, rpc_printer_usage);
}


/** 
 * List printers on a remote RPC server
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/
static int rpc_printer_list(int argc, const char **argv)
{

	return run_rpc_command(NULL, PI_SPOOLSS, 0, 
			       rpc_printer_list_internals,
			       argc, argv);
}

/** 
 * List printer-drivers on a remote RPC server
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/
static int rpc_printer_driver_list(int argc, const char **argv)
{

	return run_rpc_command(NULL, PI_SPOOLSS, 0, 
			       rpc_printer_driver_list_internals,
			       argc, argv);
}

/** 
 * Publish printer in ADS via MSRPC
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/
static int rpc_printer_publish_publish(int argc, const char **argv)
{

	return run_rpc_command(NULL, PI_SPOOLSS, 0, 
			       rpc_printer_publish_publish_internals,
			       argc, argv);
}

/** 
 * Update printer in ADS via MSRPC
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/
static int rpc_printer_publish_update(int argc, const char **argv)
{

	return run_rpc_command(NULL, PI_SPOOLSS, 0, 
			       rpc_printer_publish_update_internals,
			       argc, argv);
}

/** 
 * UnPublish printer in ADS via MSRPC
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/
static int rpc_printer_publish_unpublish(int argc, const char **argv)
{

	return run_rpc_command(NULL, PI_SPOOLSS, 0, 
			       rpc_printer_publish_unpublish_internals,
			       argc, argv);
}

/** 
 * List published printers via MSRPC
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/
static int rpc_printer_publish_list(int argc, const char **argv)
{

	return run_rpc_command(NULL, PI_SPOOLSS, 0, 
			       rpc_printer_publish_list_internals,
			       argc, argv);
}


/** 
 * Publish printer in ADS
 *
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 *
 * @return A shell status integer (0 for success)
 **/
static int rpc_printer_publish(int argc, const char **argv)
{

	struct functable func[] = {
		{"publish", 	rpc_printer_publish_publish},
		{"update", 	rpc_printer_publish_update},
		{"unpublish", 	rpc_printer_publish_unpublish},
		{"list", 	rpc_printer_publish_list},
		{"help", 	rpc_printer_usage},
		{NULL, NULL}
	};

	if (argc == 0)
		return run_rpc_command(NULL, PI_SPOOLSS, 0, 
			       rpc_printer_publish_list_internals,
			       argc, argv);

	return net_run_function(argc, argv, func, rpc_printer_usage);

}


/** 
 * Display rpc printer help page.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 **/
int rpc_printer_usage(int argc, const char **argv)
{
        return net_help_printer(argc, argv);
}

/** 
 * 'net rpc printer' entrypoint.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 **/
int net_rpc_printer(int argc, const char **argv) 
{
	struct functable func[] = {
		{"list", rpc_printer_list},
		{"migrate", rpc_printer_migrate},
		{"driver", rpc_printer_driver_list},
		{"publish", rpc_printer_publish},
		{NULL, NULL}
	};

	if (argc == 0)
		return run_rpc_command(NULL, PI_SPOOLSS, 0, 
			       rpc_printer_list_internals,
			       argc, argv);

	return net_run_function(argc, argv, func, rpc_printer_usage);
}

/****************************************************************************/


/** 
 * Basic usage function for 'net rpc'
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 **/

int net_rpc_usage(int argc, const char **argv) 
{
	d_printf("  net rpc info \t\t\tshow basic info about a domain \n");
	d_printf("  net rpc join \t\t\tto join a domain \n");
	d_printf("  net rpc oldjoin \t\t\tto join a domain created in server manager\n");
	d_printf("  net rpc testjoin \t\ttests that a join is valid\n");
	d_printf("  net rpc user \t\t\tto add, delete and list users\n");
	d_printf("  net rpc password <username> [<password>] -Uadmin_username%%admin_pass\n");
	d_printf("  net rpc group \t\tto list groups\n");
	d_printf("  net rpc share \t\tto add, delete, list and migrate shares\n");
	d_printf("  net rpc printer \t\tto list and migrate printers\n");
	d_printf("  net rpc file \t\t\tto list open files\n");
	d_printf("  net rpc changetrustpw \tto change the trust account password\n");
	d_printf("  net rpc getsid \t\tfetch the domain sid into the local secrets.tdb\n");
	d_printf("  net rpc vampire \t\tsyncronise an NT PDC's users and groups into the local passdb\n");
	d_printf("  net rpc samdump \t\tdiplay an NT PDC's users, groups and other data\n");
	d_printf("  net rpc trustdom \t\tto create trusting domain's account or establish trust\n");
	d_printf("  net rpc abortshutdown \tto abort the shutdown of a remote server\n");
	d_printf("  net rpc shutdown \t\tto shutdown a remote server\n");
	d_printf("  net rpc rights\t\tto manage privileges assigned to SIDs\n");
	d_printf("  net rpc registry\t\tto manage registry hives\n");
	d_printf("  net rpc service\t\tto start, stop and query services\n");
	d_printf("  net rpc audit\t\t\tto modify global auditing settings\n");
	d_printf("  net rpc shell\t\t\tto open an interactive shell for remote server/account management\n");
	d_printf("\n");
	d_printf("'net rpc shutdown' also accepts the following miscellaneous options:\n"); /* misc options */
	d_printf("\t-r or --reboot\trequest remote server reboot on shutdown\n");
	d_printf("\t-f or --force\trequest the remote server force its shutdown\n");
	d_printf("\t-t or --timeout=<timeout>\tnumber of seconds before shutdown\n");
	d_printf("\t-C or --comment=<message>\ttext message to display on impending shutdown\n");
	return -1;
}


/**
 * Help function for 'net rpc'.  Calls command specific help if requested
 * or displays usage of net rpc
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 **/

int net_rpc_help(int argc, const char **argv)
{
	struct functable func[] = {
		{"join", rpc_join_usage},
		{"user", rpc_user_usage},
		{"group", rpc_group_usage},
		{"share", rpc_share_usage},
		/*{"changetrustpw", rpc_changetrustpw_usage}, */
		{"trustdom", rpc_trustdom_usage},
		/*{"abortshutdown", rpc_shutdown_abort_usage},*/
		/*{"shutdown", rpc_shutdown_usage}, */
		{"vampire", rpc_vampire_usage},
		{NULL, NULL}
	};

	if (argc == 0) {
		net_rpc_usage(argc, argv);
		return -1;
	}

	return (net_run_function(argc, argv, func, rpc_user_usage));
}

/** 
 * 'net rpc' entrypoint.
 * @param argc  Standard main() style argc
 * @param argv  Standard main() style argv.  Initial components are already
 *              stripped
 **/

int net_rpc(int argc, const char **argv)
{
	struct functable func[] = {
		{"audit", net_rpc_audit},
		{"info", net_rpc_info},
		{"join", net_rpc_join},
		{"oldjoin", net_rpc_oldjoin},
		{"testjoin", net_rpc_testjoin},
		{"user", net_rpc_user},
		{"password", rpc_user_password},
		{"group", net_rpc_group},
		{"share", net_rpc_share},
		{"file", net_rpc_file},
		{"printer", net_rpc_printer},
		{"changetrustpw", net_rpc_changetrustpw},
		{"trustdom", rpc_trustdom},
		{"abortshutdown", rpc_shutdown_abort},
		{"shutdown", rpc_shutdown},
		{"samdump", rpc_samdump},
		{"vampire", rpc_vampire},
		{"getsid", net_rpc_getsid},
		{"rights", net_rpc_rights},
		{"service", net_rpc_service},
		{"registry", net_rpc_registry},
		{"shell", net_rpc_shell},
		{"help", net_rpc_help},
		{NULL, NULL}
	};
	return net_run_function(argc, argv, func, net_rpc_usage);
}
