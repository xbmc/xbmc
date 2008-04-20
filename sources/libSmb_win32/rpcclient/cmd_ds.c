/* 
   Unix SMB/CIFS implementation.
   RPC pipe client

   Copyright (C) Gerald Carter 2002

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

/* Look up domain related information on a remote host */

static NTSTATUS cmd_ds_dsrole_getprimarydominfo(struct rpc_pipe_client *cli, 
				     TALLOC_CTX *mem_ctx, int argc, 
				     const char **argv) 
{
	NTSTATUS result;
	DS_DOMINFO_CTR	ctr;
	
	result = rpccli_ds_getprimarydominfo( cli, mem_ctx, DsRolePrimaryDomainInfoBasic, &ctr );
	if ( NT_STATUS_IS_OK(result) )
	{
		printf ("Machine Role = [%d]\n", ctr.basic->machine_role);
		
		if ( ctr.basic->flags & DSROLE_PRIMARY_DS_RUNNING )	{
			printf( "Directory Service is running.\n");
			printf( "Domain is in %s mode.\n", (ctr.basic->flags & DSROLE_PRIMARY_DS_MIXED_MODE) ? "mixed" : "native" );
		}
		else
			printf( "Directory Service not running on server\n");
	}
	
	return result;
}

static NTSTATUS cmd_ds_enum_domain_trusts(struct rpc_pipe_client *cli,
				     TALLOC_CTX *mem_ctx, int argc, 
				     const char **argv) 
{
	NTSTATUS 		result;
	uint32 			flags = DS_DOMAIN_IN_FOREST;
	struct ds_domain_trust	 *trusts = NULL;
	unsigned int 			num_domains = 0;
	int i;
	
	if (argc > 1) {
		flags = atoi(argv[1]);
	}

	result = rpccli_ds_enum_domain_trusts( cli, mem_ctx, cli->cli->desthost, flags, 
		&trusts, &num_domains );
	
	printf( "%d domains returned\n", num_domains );

	for (i=0; i<num_domains; i++ ) 
		printf("%s (%s)\n", trusts[i].dns_domain, trusts[i].netbios_domain);
	
	return result;
}

/* List of commands exported by this module */

struct cmd_set ds_commands[] = {

	{ "LSARPC-DS" },

	{ "dsroledominfo",   RPC_RTYPE_NTSTATUS, cmd_ds_dsrole_getprimarydominfo, NULL, PI_LSARPC_DS, NULL, "Get Primary Domain Information", "" },
	{ "dsenumdomtrusts", RPC_RTYPE_NTSTATUS, cmd_ds_enum_domain_trusts,       NULL, PI_NETLOGON,  NULL, "Enumerate all trusted domains in an AD forest", "" },

{ NULL }
};
