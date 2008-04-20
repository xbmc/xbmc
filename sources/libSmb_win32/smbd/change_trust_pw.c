/* 
 *  Unix SMB/CIFS implementation.
 *  Periodic Trust account password changing.
 *  Copyright (C) Andrew Tridgell              1992-1997,
 *  Copyright (C) Luke Kenneth Casson Leighton 1996-1997,
 *  Copyright (C) Paul Ashton                       1997.
 *  Copyright (C) Jeremy Allison                    1998.
 *  Copyright (C) Andrew Bartlett                   2001.
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

/************************************************************************
 Change the trust account password for a domain.
************************************************************************/

NTSTATUS change_trust_account_password( const char *domain, const char *remote_machine)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	struct in_addr pdc_ip;
	fstring dc_name;
	struct cli_state *cli = NULL;
	struct rpc_pipe_client *netlogon_pipe = NULL;

	DEBUG(5,("change_trust_account_password: Attempting to change trust account password in domain %s....\n",
		domain));

	if (remote_machine == NULL || !strcmp(remote_machine, "*")) {
		/* Use the PDC *only* for this */
	
		if ( !get_pdc_ip(domain, &pdc_ip) ) {
			DEBUG(0,("Can't get IP for PDC for domain %s\n", domain));
			goto failed;
		}

		if ( !name_status_find( domain, 0x1b, 0x20, pdc_ip, dc_name) )
			goto failed;
	} else {
		/* supoport old deprecated "smbpasswd -j DOMAIN -r MACHINE" behavior */
		fstrcpy( dc_name, remote_machine );
	}
	
	/* if this next call fails, then give up.  We can't do
	   password changes on BDC's  --jerry */
	   
	if (!NT_STATUS_IS_OK(cli_full_connection(&cli, global_myname(), dc_name, 
					   NULL, 0,
					   "IPC$", "IPC",  
					   "", "",
					   "", 0, Undefined, NULL))) {
		DEBUG(0,("modify_trust_password: Connection to %s failed!\n", dc_name));
		nt_status = NT_STATUS_UNSUCCESSFUL;
		goto failed;
	}
      
	/*
	 * Ok - we have an anonymous connection to the IPC$ share.
	 * Now start the NT Domain stuff :-).
	 */

	/* Shouldn't we open this with schannel ? JRA. */

	netlogon_pipe = cli_rpc_pipe_open_noauth(cli, PI_NETLOGON, &nt_status);
	if (!netlogon_pipe) {
		DEBUG(0,("modify_trust_password: unable to open the domain client session to machine %s. Error was : %s.\n", 
			dc_name, nt_errstr(nt_status)));
		cli_shutdown(cli);
		goto failed;
	}

	nt_status = trust_pw_find_change_and_store_it(netlogon_pipe, cli->mem_ctx, domain);
  
	cli_shutdown(cli);
	
failed:
	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(0,("%s : change_trust_account_password: Failed to change password for domain %s.\n", 
			timestring(False), domain));
	}
	else
		DEBUG(5,("change_trust_account_password: sucess!\n"));
  
	return nt_status;
}
