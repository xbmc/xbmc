/* 
   Samba Unix/Linux SMB client library 
   Distributed SMB/CIFS Server Management Utility 
   Copyright (C) Gerald (Jerry) Carter          2004

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

/********************************************************************
********************************************************************/

static NTSTATUS sid_to_name(struct rpc_pipe_client *pipe_hnd,
				TALLOC_CTX *mem_ctx,
				DOM_SID *sid,
				fstring name)
{
	POLICY_HND pol;
	uint32 *sid_types;
	NTSTATUS result;
	char **domains, **names;

	result = rpccli_lsa_open_policy(pipe_hnd, mem_ctx, True, 
		SEC_RIGHTS_MAXIMUM_ALLOWED, &pol);
		
	if ( !NT_STATUS_IS_OK(result) )
		return result;

	result = rpccli_lsa_lookup_sids(pipe_hnd, mem_ctx, &pol, 1, sid, &domains, &names, &sid_types);
	
	if ( NT_STATUS_IS_OK(result) ) {
		if ( *domains[0] )
			fstr_sprintf( name, "%s\\%s", domains[0], names[0] );
		else
			fstrcpy( name, names[0] );
	}

	rpccli_lsa_close(pipe_hnd, mem_ctx, &pol);
	return result;
}

/********************************************************************
********************************************************************/

static NTSTATUS name_to_sid(struct rpc_pipe_client *pipe_hnd,
			    TALLOC_CTX *mem_ctx,
			    DOM_SID *sid, const char *name)
{
	POLICY_HND pol;
	uint32 *sid_types;
	NTSTATUS result;
	DOM_SID *sids;

	/* maybe its a raw SID */
	if ( strncmp(name, "S-", 2) == 0 && string_to_sid(sid, name) ) {
		return NT_STATUS_OK;
	}

	result = rpccli_lsa_open_policy(pipe_hnd, mem_ctx, True, 
		SEC_RIGHTS_MAXIMUM_ALLOWED, &pol);
		
	if ( !NT_STATUS_IS_OK(result) )
		return result;

	result = rpccli_lsa_lookup_names(pipe_hnd, mem_ctx, &pol, 1, &name,
					 NULL, &sids, &sid_types);
	
	if ( NT_STATUS_IS_OK(result) )
		sid_copy( sid, &sids[0] );

	rpccli_lsa_close(pipe_hnd, mem_ctx, &pol);
	return result;
}

/********************************************************************
********************************************************************/

static NTSTATUS enum_privileges(struct rpc_pipe_client *pipe_hnd,
				TALLOC_CTX *ctx,
				POLICY_HND *pol )
{
	NTSTATUS result;
	uint32 enum_context = 0;
	uint32 pref_max_length=0x1000;
	uint32 count=0;
	char   **privs_name;
	uint32 *privs_high;
	uint32 *privs_low;
	int i;
	uint16 lang_id=0;
	uint16 lang_id_sys=0;
	uint16 lang_id_desc;
	fstring description;

	result = rpccli_lsa_enum_privilege(pipe_hnd, ctx, pol, &enum_context, 
		pref_max_length, &count, &privs_name, &privs_high, &privs_low);

	if ( !NT_STATUS_IS_OK(result) )
		return result;

	/* Print results */
	
	for (i = 0; i < count; i++) {
		d_printf("%30s  ", privs_name[i] ? privs_name[i] : "*unknown*" );
		
		/* try to get the description */
		
		if ( !NT_STATUS_IS_OK(rpccli_lsa_get_dispname(pipe_hnd, ctx, pol, 
			privs_name[i], lang_id, lang_id_sys, description, &lang_id_desc)) )
		{
			d_printf("??????\n");
			continue;
		}
		
		d_printf("%s\n", description );		
	}

	return NT_STATUS_OK;
}

/********************************************************************
********************************************************************/

static NTSTATUS check_privilege_for_user(struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *ctx,
					POLICY_HND *pol,
					DOM_SID *sid,
					const char *right)
{
	NTSTATUS result;
	uint32 count;
	char **rights;
	int i;

	result = rpccli_lsa_enum_account_rights(pipe_hnd, ctx, pol, sid, &count, &rights);

	if (!NT_STATUS_IS_OK(result)) {
		return result;
	}

	if (count == 0) {
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;
	}
		
	for (i = 0; i < count; i++) {
		if (StrCaseCmp(rights[i], right) == 0) {
			return NT_STATUS_OK;
		}
	}

	return NT_STATUS_OBJECT_NAME_NOT_FOUND;
}

/********************************************************************
********************************************************************/

static NTSTATUS enum_privileges_for_user(struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *ctx,
					POLICY_HND *pol,
					DOM_SID *sid )
{
	NTSTATUS result;
	uint32 count;
	char **rights;
	int i;

	result = rpccli_lsa_enum_account_rights(pipe_hnd, ctx, pol, sid, &count, &rights);

	if (!NT_STATUS_IS_OK(result))
		return result;

	if ( count == 0 )
		d_printf("No privileges assigned\n");
		
	for (i = 0; i < count; i++) {
		printf("%s\n", rights[i]);
	}

	return NT_STATUS_OK;
}

/********************************************************************
********************************************************************/

static NTSTATUS enum_accounts_for_privilege(struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *ctx,
						POLICY_HND *pol,
						const char *privilege)
{
	NTSTATUS result;
	uint32 enum_context=0;
	uint32 pref_max_length=0x1000;
	DOM_SID *sids;
	uint32 count=0;
	int i;
	fstring name;

	result = rpccli_lsa_enum_sids(pipe_hnd, ctx, pol, &enum_context, 
		pref_max_length, &count, &sids);

	if (!NT_STATUS_IS_OK(result))
		return result;
		
	d_printf("%s:\n", privilege);

	for ( i=0; i<count; i++ ) {
	
		   
		result = check_privilege_for_user( pipe_hnd, ctx, pol, &sids[i], privilege);
		
		if ( ! NT_STATUS_IS_OK(result)) {
			if ( ! NT_STATUS_EQUAL(result, NT_STATUS_OBJECT_NAME_NOT_FOUND)) {
				return result;
			}
			continue;
		}

		/* try to convert the SID to a name.  Fall back to 
		   printing the raw SID if necessary */
		result = sid_to_name( pipe_hnd, ctx, &sids[i], name );
		if ( !NT_STATUS_IS_OK (result) )
			fstrcpy( name, sid_string_static(&sids[i]) );
			
		d_printf("  %s\n", name);
	}

	return NT_STATUS_OK;
}

/********************************************************************
********************************************************************/

static NTSTATUS enum_privileges_for_accounts(struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *ctx,
						POLICY_HND *pol)
{
	NTSTATUS result;
	uint32 enum_context=0;
	uint32 pref_max_length=0x1000;
	DOM_SID *sids;
	uint32 count=0;
	int i;
	fstring name;

	result = rpccli_lsa_enum_sids(pipe_hnd, ctx, pol, &enum_context, 
		pref_max_length, &count, &sids);

	if (!NT_STATUS_IS_OK(result))
		return result;
		
	for ( i=0; i<count; i++ ) {
	
		/* try to convert the SID to a name.  Fall back to 
		   printing the raw SID if necessary */
		   
		result = sid_to_name(pipe_hnd, ctx, &sids[i], name );
		if ( !NT_STATUS_IS_OK (result) )
			fstrcpy( name, sid_string_static(&sids[i]) );
			
		d_printf("%s\n", name);
		
		result = enum_privileges_for_user(pipe_hnd, ctx, pol, &sids[i] );
		
		if ( !NT_STATUS_IS_OK(result) )
			return result;

		d_printf("\n");
	}

	return NT_STATUS_OK;
}

/********************************************************************
********************************************************************/

static NTSTATUS rpc_rights_list_internal(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv )
{
	POLICY_HND pol;
	NTSTATUS result;
	DOM_SID sid;
	fstring privname;
	fstring description;
	uint16 lang_id = 0;
	uint16 lang_id_sys = 0;
	uint16 lang_id_desc;
	
	
	result = rpccli_lsa_open_policy(pipe_hnd, mem_ctx, True, 
		SEC_RIGHTS_MAXIMUM_ALLOWED, &pol);

	if ( !NT_STATUS_IS_OK(result) )
		return result;
	
	/* backwards compatibility; just list available privileges if no arguement */
	   
	if (argc == 0) {
		result = enum_privileges(pipe_hnd, mem_ctx, &pol );
		goto done;
	}

	if (strequal(argv[0], "privileges")) {
		int i = 1;

		if (argv[1] == NULL) {
			result = enum_privileges(pipe_hnd, mem_ctx, &pol );
			goto done;
		}

		while ( argv[i] != NULL ) {
			fstrcpy( privname, argv[i] );
			i++;
		
			/* verify that this is a valid privilege for error reporting */
			
			result = rpccli_lsa_get_dispname(pipe_hnd, mem_ctx, &pol, privname, lang_id, 
				lang_id_sys, description, &lang_id_desc);
			
			if ( !NT_STATUS_IS_OK(result) ) {
				if ( NT_STATUS_EQUAL( result, NT_STATUS_NO_SUCH_PRIVILEGE ) ) 
					d_fprintf(stderr, "No such privilege exists: %s.\n", privname);
				else
					d_fprintf(stderr, "Error resolving privilege display name [%s].\n", nt_errstr(result));
				continue;
			}
			
			result = enum_accounts_for_privilege(pipe_hnd, mem_ctx, &pol, privname);
			if (!NT_STATUS_IS_OK(result)) {
				d_fprintf(stderr, "Error enumerating accounts for privilege %s [%s].\n", 
					privname, nt_errstr(result));
				continue;
			}
		}
		goto done;
	}

	/* special case to enumerate all privileged SIDs with associated rights */
	
	if (strequal( argv[0], "accounts")) {
		int i = 1;

		if (argv[1] == NULL) {
			result = enum_privileges_for_accounts(pipe_hnd, mem_ctx, &pol);
			goto done;
		}

		while (argv[i] != NULL) {
			result = name_to_sid(pipe_hnd, mem_ctx, &sid, argv[i]);
			if (!NT_STATUS_IS_OK(result)) {
				goto done;
			}
			result = enum_privileges_for_user(pipe_hnd, mem_ctx, &pol, &sid);
			if (!NT_STATUS_IS_OK(result)) {
				goto done;
			}
			i++;
		}
		goto done;
	}

	/* backward comaptibility: if no keyword provided, treat the key
	   as an account name */
	if (argc > 1) {
		d_printf("Usage: net rpc rights list [[accounts|privileges] [name|SID]]\n");
		result = NT_STATUS_OK;
		goto done;
	}

	result = name_to_sid(pipe_hnd, mem_ctx, &sid, argv[0]);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}
	result = enum_privileges_for_user(pipe_hnd, mem_ctx, &pol, &sid );

done:
	rpccli_lsa_close(pipe_hnd, mem_ctx, &pol);

	return result;
}

/********************************************************************
********************************************************************/

static NTSTATUS rpc_rights_grant_internal(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv )
{
	POLICY_HND dom_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DOM_SID sid;

	if (argc < 2 ) {
		d_printf("Usage: net rpc rights grant <name|SID> <rights...>\n");
		return NT_STATUS_OK;
	}

	result = name_to_sid(pipe_hnd, mem_ctx, &sid, argv[0]);
	if (!NT_STATUS_IS_OK(result))
		return result;	

	result = rpccli_lsa_open_policy2(pipe_hnd, mem_ctx, True, 
				     SEC_RIGHTS_MAXIMUM_ALLOWED,
				     &dom_pol);

	if (!NT_STATUS_IS_OK(result))
		return result;	

	result = rpccli_lsa_add_account_rights(pipe_hnd, mem_ctx, &dom_pol, sid, 
					    argc-1, argv+1);

	if (!NT_STATUS_IS_OK(result))
		goto done;
		
	d_printf("Successfully granted rights.\n");

 done:
	if ( !NT_STATUS_IS_OK(result) ) {
		d_fprintf(stderr, "Failed to grant privileges for %s (%s)\n", 
			argv[0], nt_errstr(result));
	}
		
 	rpccli_lsa_close(pipe_hnd, mem_ctx, &dom_pol);
	
	return result;
}

/********************************************************************
********************************************************************/

static NTSTATUS rpc_rights_revoke_internal(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv )
{
	POLICY_HND dom_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DOM_SID sid;

	if (argc < 2 ) {
		d_printf("Usage: net rpc rights revoke <name|SID> <rights...>\n");
		return NT_STATUS_OK;
	}

	result = name_to_sid(pipe_hnd, mem_ctx, &sid, argv[0]);
	if (!NT_STATUS_IS_OK(result))
		return result;	

	result = rpccli_lsa_open_policy2(pipe_hnd, mem_ctx, True, 
				     SEC_RIGHTS_MAXIMUM_ALLOWED,
				     &dom_pol);

	if (!NT_STATUS_IS_OK(result))
		return result;	

	result = rpccli_lsa_remove_account_rights(pipe_hnd, mem_ctx, &dom_pol, sid, 
					       False, argc-1, argv+1);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	d_printf("Successfully revoked rights.\n");

done:
	if ( !NT_STATUS_IS_OK(result) ) {
		d_fprintf(stderr, "Failed to revoke privileges for %s (%s)\n", 
			argv[0], nt_errstr(result));
	}
	
	rpccli_lsa_close(pipe_hnd, mem_ctx, &dom_pol);

	return result;
}	


/********************************************************************
********************************************************************/

static int rpc_rights_list( int argc, const char **argv )
{
	return run_rpc_command( NULL, PI_LSARPC, 0, 
		rpc_rights_list_internal, argc, argv );
}

/********************************************************************
********************************************************************/

static int rpc_rights_grant( int argc, const char **argv )
{
	return run_rpc_command( NULL, PI_LSARPC, 0, 
		rpc_rights_grant_internal, argc, argv );
}

/********************************************************************
********************************************************************/

static int rpc_rights_revoke( int argc, const char **argv )
{
	return run_rpc_command( NULL, PI_LSARPC, 0, 
		rpc_rights_revoke_internal, argc, argv );
}

/********************************************************************
********************************************************************/

static int net_help_rights( int argc, const char **argv )
{
	d_printf("net rpc rights list [{accounts|privileges} [name|SID]]   View available or assigned privileges\n");
	d_printf("net rpc rights grant <name|SID> <right>                  Assign privilege[s]\n");
	d_printf("net rpc rights revoke <name|SID> <right>                 Revoke privilege[s]\n");
	
	d_printf("\nBoth 'grant' and 'revoke' require a SID and a list of privilege names.\n");
	d_printf("For example\n");
	d_printf("\n  net rpc rights grant 'VALE\\biddle' SePrintOperatorPrivilege SeDiskOperatorPrivilege\n");
	d_printf("\nwould grant the printer admin and disk manager rights to the user 'VALE\\biddle'\n\n");
	
	
	return -1;
}

/********************************************************************
********************************************************************/

int net_rpc_rights(int argc, const char **argv) 
{
	struct functable func[] = {
		{"list", rpc_rights_list},
		{"grant", rpc_rights_grant},
		{"revoke", rpc_rights_revoke},
		{NULL, NULL}
	};
	
	if ( argc )
		return net_run_function( argc, argv, func, net_help_rights );
		
	return net_help_rights( argc, argv );
}

static NTSTATUS rpc_sh_rights_list(TALLOC_CTX *mem_ctx, struct rpc_sh_ctx *ctx,
				   struct rpc_pipe_client *pipe_hnd,
				   int argc, const char **argv)
{
	return rpc_rights_list_internal(ctx->domain_sid, ctx->domain_name,
					ctx->cli, pipe_hnd, mem_ctx,
					argc, argv);
}

static NTSTATUS rpc_sh_rights_grant(TALLOC_CTX *mem_ctx,
				    struct rpc_sh_ctx *ctx,
				    struct rpc_pipe_client *pipe_hnd,
				    int argc, const char **argv)
{
	return rpc_rights_grant_internal(ctx->domain_sid, ctx->domain_name,
					 ctx->cli, pipe_hnd, mem_ctx,
					 argc, argv);
}

static NTSTATUS rpc_sh_rights_revoke(TALLOC_CTX *mem_ctx,
				     struct rpc_sh_ctx *ctx,
				     struct rpc_pipe_client *pipe_hnd,
				     int argc, const char **argv)
{
	return rpc_rights_revoke_internal(ctx->domain_sid, ctx->domain_name,
					  ctx->cli, pipe_hnd, mem_ctx,
					  argc, argv);
}

struct rpc_sh_cmd *net_rpc_rights_cmds(TALLOC_CTX *mem_ctx,
				       struct rpc_sh_ctx *ctx)
{
	static struct rpc_sh_cmd cmds[] = {

	{ "list", NULL, PI_LSARPC, rpc_sh_rights_list,
	  "View available or assigned privileges" },

	{ "grant", NULL, PI_LSARPC, rpc_sh_rights_grant,
	  "Assign privilege[s]" },

	{ "revoke", NULL, PI_LSARPC, rpc_sh_rights_revoke,
	  "Revoke privilege[s]" },

	{ NULL, NULL, 0, NULL, NULL }
	};

	return cmds;
}

