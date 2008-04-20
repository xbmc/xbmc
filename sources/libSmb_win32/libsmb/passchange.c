/* 
   Unix SMB/CIFS implementation.
   SMB client password change routine
   Copyright (C) Andrew Tridgell 1994-1998
   
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

/*************************************************************
 Change a password on a remote machine using IPC calls.
*************************************************************/

NTSTATUS remote_password_change(const char *remote_machine, const char *user_name, 
			    const char *old_passwd, const char *new_passwd,
			    char *err_str, size_t err_str_len)
{
	struct nmb_name calling, called;
	struct cli_state cli;
	struct rpc_pipe_client *pipe_hnd;
	struct in_addr ip;

	NTSTATUS result;
	BOOL pass_must_change = False;

	*err_str = '\0';

	if(!resolve_name( remote_machine, &ip, 0x20)) {
		slprintf(err_str, err_str_len-1, "unable to find an IP address for machine %s.\n",
			remote_machine );
		return NT_STATUS_UNSUCCESSFUL;
	}
 
	ZERO_STRUCT(cli);
 
	if (!cli_initialise(&cli) || !cli_connect(&cli, remote_machine, &ip)) {
		slprintf(err_str, err_str_len-1, "unable to connect to SMB server on machine %s. Error was : %s.\n",
			remote_machine, cli_errstr(&cli) );
		return NT_STATUS_UNSUCCESSFUL;
	}
  
	make_nmb_name(&calling, global_myname() , 0x0);
	make_nmb_name(&called , remote_machine, 0x20);
	
	if (!cli_session_request(&cli, &calling, &called)) {
		slprintf(err_str, err_str_len-1, "machine %s rejected the session setup. Error was : %s.\n",
			remote_machine, cli_errstr(&cli) );
		cli_shutdown(&cli);
		return NT_STATUS_UNSUCCESSFUL;
	}
  
	cli.protocol = PROTOCOL_NT1;

	if (!cli_negprot(&cli)) {
		slprintf(err_str, err_str_len-1, "machine %s rejected the negotiate protocol. Error was : %s.\n",        
			remote_machine, cli_errstr(&cli) );
		result = cli_nt_error(&cli);
		cli_shutdown(&cli);
		return result;
	}
  
	/* Given things like SMB signing, restrict anonymous and the like, 
	   try an authenticated connection first */
	if (!cli_session_setup(&cli, user_name, old_passwd, strlen(old_passwd)+1, old_passwd, strlen(old_passwd)+1, "")) {

		result = cli_nt_error(&cli);

		if (!NT_STATUS_IS_OK(result)) {

			/* Password must change or Password expired are the only valid
			 * error conditions here from where we can proceed, the rest like
			 * account locked out or logon failure will lead to errors later
			 * anyway */

			if (!NT_STATUS_EQUAL(result, NT_STATUS_PASSWORD_MUST_CHANGE) &&
			    !NT_STATUS_EQUAL(result, NT_STATUS_PASSWORD_EXPIRED)) {

				slprintf(err_str, err_str_len-1, "Could not "
					 "connect to machine %s: %s\n",
					 remote_machine, cli_errstr(&cli));
				cli_shutdown(&cli);
				return result;
			}

			pass_must_change = True;
		}

		/*
		 * We should connect as the anonymous user here, in case
		 * the server has "must change password" checked...
		 * Thanks to <Nicholas.S.Jenkins@cdc.com> for this fix.
		 */

		if (!cli_session_setup(&cli, "", "", 0, "", 0, "")) {
			slprintf(err_str, err_str_len-1, "machine %s rejected the session setup. Error was : %s.\n",        
				 remote_machine, cli_errstr(&cli) );
			result = cli_nt_error(&cli);
			cli_shutdown(&cli);
			return result;
		}

		cli_init_creds(&cli, "", "", NULL);
	} else {
		cli_init_creds(&cli, user_name, "", old_passwd);
	}

	if (!cli_send_tconX(&cli, "IPC$", "IPC", "", 1)) {
		slprintf(err_str, err_str_len-1, "machine %s rejected the tconX on the IPC$ share. Error was : %s.\n",
			remote_machine, cli_errstr(&cli) );
		result = cli_nt_error(&cli);
		cli_shutdown(&cli);
		return result;
	}

	/* Try not to give the password away too easily */

	if (!pass_must_change) {
		pipe_hnd = cli_rpc_pipe_open_ntlmssp(&cli,
						PI_SAMR,
						PIPE_AUTH_LEVEL_PRIVACY,
						"", /* what domain... ? */
						user_name,
						old_passwd,
						&result);
	} else {
		/*
		 * If the user password must be changed the ntlmssp bind will
		 * fail the same way as the session setup above did. The
		 * difference ist that with a pipe bind we don't get a good
		 * error message, the result will be that the rpc call below
		 * will just fail. So we do it anonymously, there's no other
		 * way.
		 */
		pipe_hnd = cli_rpc_pipe_open_noauth(&cli, PI_SAMR, &result);
	}

	if (!pipe_hnd) {
		if (lp_client_lanman_auth()) {
			/* Use the old RAP method. */
			if (!cli_oem_change_password(&cli, user_name, new_passwd, old_passwd)) {
				slprintf(err_str, err_str_len-1, "machine %s rejected the password change: Error was : %s.\n",
					 remote_machine, cli_errstr(&cli) );
				result = cli_nt_error(&cli);
				cli_shutdown(&cli);
				return result;
			}
		} else {
			slprintf(err_str, err_str_len-1,
				"SAMR connection to machine %s failed. Error was %s, "
				"but LANMAN password changed are disabled\n",
				nt_errstr(result), remote_machine);
			result = cli_nt_error(&cli);
			cli_shutdown(&cli);
			return result;
		}
	}

	if (NT_STATUS_IS_OK(result = rpccli_samr_chgpasswd_user(pipe_hnd, cli.mem_ctx, user_name, 
							     new_passwd, old_passwd))) {
		/* Great - it all worked! */
		cli_shutdown(&cli);
		return NT_STATUS_OK;

	} else if (!(NT_STATUS_EQUAL(result, NT_STATUS_ACCESS_DENIED) 
		     || NT_STATUS_EQUAL(result, NT_STATUS_UNSUCCESSFUL))) {
		/* it failed, but for reasons such as wrong password, too short etc ... */
		
		slprintf(err_str, err_str_len-1, "machine %s rejected the password change: Error was : %s.\n",
			 remote_machine, get_friendly_nt_error_msg(result));
		cli_shutdown(&cli);
		return result;
	}

	/* OK, that failed, so try again... */
	cli_rpc_pipe_close(pipe_hnd);
	
	/* Try anonymous NTLMSSP... */
	cli_init_creds(&cli, "", "", NULL);
	
	result = NT_STATUS_UNSUCCESSFUL;
	
	/* OK, this is ugly, but... try an anonymous pipe. */
	pipe_hnd = cli_rpc_pipe_open_noauth(&cli, PI_SAMR, &result);

	if ( pipe_hnd &&
		(NT_STATUS_IS_OK(result = rpccli_samr_chgpasswd_user(pipe_hnd,
						cli.mem_ctx,
						user_name, 
						new_passwd,
						old_passwd)))) {
		/* Great - it all worked! */
		cli_shutdown(&cli);
		return NT_STATUS_OK;
	} else {
		if (!(NT_STATUS_EQUAL(result, NT_STATUS_ACCESS_DENIED) 
		      || NT_STATUS_EQUAL(result, NT_STATUS_UNSUCCESSFUL))) {
			/* it failed, but again it was due to things like new password too short */

			slprintf(err_str, err_str_len-1, 
				 "machine %s rejected the (anonymous) password change: Error was : %s.\n",
				 remote_machine, get_friendly_nt_error_msg(result));
			cli_shutdown(&cli);
			return result;
		}
		
		/* We have failed to change the user's password, and we think the server
		   just might not support SAMR password changes, so fall back */
		
		if (lp_client_lanman_auth()) {
			/* Use the old RAP method. */
			if (cli_oem_change_password(&cli, user_name, new_passwd, old_passwd)) {
				/* SAMR failed, but the old LanMan protocol worked! */

				cli_shutdown(&cli);
				return NT_STATUS_OK;
			}
			slprintf(err_str, err_str_len-1, 
				 "machine %s rejected the password change: Error was : %s.\n",
				 remote_machine, cli_errstr(&cli) );
			result = cli_nt_error(&cli);
			cli_shutdown(&cli);
			return result;
		} else {
			slprintf(err_str, err_str_len-1,
				"SAMR connection to machine %s failed. Error was %s, "
				"but LANMAN password changed are disabled\n",
				nt_errstr(result), remote_machine);
			cli_shutdown(&cli);
			return NT_STATUS_UNSUCCESSFUL;
		}
	}
}
