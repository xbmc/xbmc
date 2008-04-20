/* 
   Samba Unix/Linux SMB client library 
   net ads commands
   Copyright (C) 2001 Andrew Tridgell (tridge@samba.org)
   Copyright (C) 2001 Remus Koos (remuskoos@yahoo.com)
   Copyright (C) 2002 Jim McDonough (jmcd@us.ibm.com)
   Copyright (C) 2006 Gerald (Jerry) Carter (jerry@samba.org)

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

/*******************************************************************
 Leave an AD domain.  Windows XP disables the machine account.
 We'll try the same.  The old code would do an LDAP delete.
 That only worked using the machine creds because added the machine
 with full control to the computer object's ACL.
*******************************************************************/

NTSTATUS netdom_leave_domain( TALLOC_CTX *mem_ctx, struct cli_state *cli, 
                         DOM_SID *dom_sid )
{	
	struct rpc_pipe_client *pipe_hnd = NULL;
	POLICY_HND sam_pol, domain_pol, user_pol;
	NTSTATUS status = NT_STATUS_UNSUCCESSFUL;
	char *acct_name;
	uint32 flags = 0x3e8;
	const char *const_acct_name;
	uint32 user_rid;
	uint32 num_rids, *name_types, *user_rids;
	SAM_USERINFO_CTR ctr, *qctr = NULL;
	SAM_USER_INFO_16 p16;

	/* Open the domain */
	
	if ( (pipe_hnd = cli_rpc_pipe_open_noauth(cli, PI_SAMR, &status)) == NULL ) {
		DEBUG(0, ("Error connecting to SAM pipe. Error was %s\n",
			nt_errstr(status) ));
		return status;
	}

	status = rpccli_samr_connect(pipe_hnd, mem_ctx, 
			SEC_RIGHTS_MAXIMUM_ALLOWED, &sam_pol);
	if ( !NT_STATUS_IS_OK(status) )
		return status;

	
	status = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &sam_pol,
			SEC_RIGHTS_MAXIMUM_ALLOWED, dom_sid, &domain_pol);
	if ( !NT_STATUS_IS_OK(status) )
		return status;

	/* Create domain user */
	
	acct_name = talloc_asprintf(mem_ctx, "%s$", global_myname()); 
	strlower_m(acct_name);
	const_acct_name = acct_name;

	status = rpccli_samr_lookup_names(pipe_hnd, mem_ctx,
			&domain_pol, flags, 1, &const_acct_name, 
			&num_rids, &user_rids, &name_types);
	if ( !NT_STATUS_IS_OK(status) )
		return status;

	if ( name_types[0] != SID_NAME_USER) {
		DEBUG(0, ("%s is not a user account (type=%d)\n", acct_name, name_types[0]));
		return NT_STATUS_INVALID_WORKSTATION;
	}

	user_rid = user_rids[0];
		
	/* Open handle on user */

	status = rpccli_samr_open_user(pipe_hnd, mem_ctx, &domain_pol,
			SEC_RIGHTS_MAXIMUM_ALLOWED, user_rid, &user_pol);
	if ( !NT_STATUS_IS_OK(status) ) {
		goto done;
	}
	
	/* Get user info */

	status = rpccli_samr_query_userinfo(pipe_hnd, mem_ctx, &user_pol, 16, &qctr);
	if ( !NT_STATUS_IS_OK(status) ) {
		rpccli_samr_close(pipe_hnd, mem_ctx, &user_pol);
		goto done;
	}

	/* now disable and setuser info */
	
	ZERO_STRUCT(ctr);
	ctr.switch_value = 16;
	ctr.info.id16 = &p16;

	p16.acb_info = qctr->info.id16->acb_info | ACB_DISABLED;

	status = rpccli_samr_set_userinfo2(pipe_hnd, mem_ctx, &user_pol, 16, 
					&cli->user_session_key, &ctr);

	rpccli_samr_close(pipe_hnd, mem_ctx, &user_pol);

done:
	rpccli_samr_close(pipe_hnd, mem_ctx, &domain_pol);
	rpccli_samr_close(pipe_hnd, mem_ctx, &sam_pol);
	
	cli_rpc_pipe_close(pipe_hnd); /* Done with this pipe */
	
	return status;
}

/*******************************************************************
 Store the machine password and domain SID
 ********************************************************************/

int netdom_store_machine_account( const char *domain, DOM_SID *sid, const char *pw )
{
	if (!secrets_store_domain_sid(domain, sid)) {
		DEBUG(1,("Failed to save domain sid\n"));
		return -1;
	}

	if (!secrets_store_machine_password(pw, domain, SEC_CHAN_WKSTA)) {
		DEBUG(1,("Failed to save machine password\n"));
		return -1;
	}

	return 0;
}

/*******************************************************************
 ********************************************************************/

NTSTATUS netdom_get_domain_sid( TALLOC_CTX *mem_ctx, struct cli_state *cli, DOM_SID **sid )
{
	struct rpc_pipe_client *pipe_hnd = NULL;
	POLICY_HND lsa_pol;
	NTSTATUS status = NT_STATUS_UNSUCCESSFUL;
	char *domain = NULL;

	if ( (pipe_hnd = cli_rpc_pipe_open_noauth(cli, PI_LSARPC, &status)) == NULL ) {
		DEBUG(0, ("Error connecting to LSA pipe. Error was %s\n",
			nt_errstr(status) ));
		return status;
	}

	status = rpccli_lsa_open_policy(pipe_hnd, mem_ctx, True,
			SEC_RIGHTS_MAXIMUM_ALLOWED, &lsa_pol);
	if ( !NT_STATUS_IS_OK(status) )
		return status;

	status = rpccli_lsa_query_info_policy(pipe_hnd, mem_ctx, 
			&lsa_pol, 5, &domain, sid);
	if ( !NT_STATUS_IS_OK(status) )
		return status;

	rpccli_lsa_close(pipe_hnd, mem_ctx, &lsa_pol);
	cli_rpc_pipe_close(pipe_hnd); /* Done with this pipe */

	/* Bail out if domain didn't get set. */
	if (!domain) {
		DEBUG(0, ("Could not get domain name.\n"));
		return NT_STATUS_UNSUCCESSFUL;
	}
	
	return NT_STATUS_OK;
}

/*******************************************************************
 Do the domain join
 ********************************************************************/
 
NTSTATUS netdom_join_domain( TALLOC_CTX *mem_ctx, struct cli_state *cli, 
                           DOM_SID *dom_sid, const char *clear_pw,
                           enum netdom_domain_t dom_type )
{	
	struct rpc_pipe_client *pipe_hnd = NULL;
	POLICY_HND sam_pol, domain_pol, user_pol;
	NTSTATUS status = NT_STATUS_UNSUCCESSFUL;
	char *acct_name;
	const char *const_acct_name;
	uint32 user_rid;
	uint32 num_rids, *name_types, *user_rids;
	uint32 flags = 0x3e8;
	uint32 acb_info = ACB_WSTRUST;
	uchar pwbuf[516];
	SAM_USERINFO_CTR ctr;
	SAM_USER_INFO_24 p24;
	SAM_USER_INFO_16 p16;
	uchar md4_trust_password[16];

	/* Open the domain */
	
	if ( (pipe_hnd = cli_rpc_pipe_open_noauth(cli, PI_SAMR, &status)) == NULL ) {
		DEBUG(0, ("Error connecting to SAM pipe. Error was %s\n",
			nt_errstr(status) ));
		return status;
	}

	status = rpccli_samr_connect(pipe_hnd, mem_ctx, 
			SEC_RIGHTS_MAXIMUM_ALLOWED, &sam_pol);
	if ( !NT_STATUS_IS_OK(status) )
		return status;

	
	status = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &sam_pol,
			SEC_RIGHTS_MAXIMUM_ALLOWED, dom_sid, &domain_pol);
	if ( !NT_STATUS_IS_OK(status) )
		return status;

	/* Create domain user */
	
	acct_name = talloc_asprintf(mem_ctx, "%s$", global_myname()); 
	strlower_m(acct_name);
	const_acct_name = acct_name;

	/* Don't try to set any acb_info flags other than ACB_WSTRUST */

	status = rpccli_samr_create_dom_user(pipe_hnd, mem_ctx, &domain_pol,
			acct_name, acb_info, 0xe005000b, &user_pol, &user_rid);

	if ( !NT_STATUS_IS_OK(status) 
		&& !NT_STATUS_EQUAL(status, NT_STATUS_USER_EXISTS)) 
	{
		d_fprintf(stderr, "Creation of workstation account failed\n");

		/* If NT_STATUS_ACCESS_DENIED then we have a valid
		   username/password combo but the user does not have
		   administrator access. */

		if (NT_STATUS_V(status) == NT_STATUS_V(NT_STATUS_ACCESS_DENIED))
			d_fprintf(stderr, "User specified does not have administrator privileges\n");

		return status;
	}

	/* We *must* do this.... don't ask... */

	if (NT_STATUS_IS_OK(status)) {
		rpccli_samr_close(pipe_hnd, mem_ctx, &user_pol);
	}

	status = rpccli_samr_lookup_names(pipe_hnd, mem_ctx,
			&domain_pol, flags, 1, &const_acct_name, 
			&num_rids, &user_rids, &name_types);
	if ( !NT_STATUS_IS_OK(status) )
		return status;

	if ( name_types[0] != SID_NAME_USER) {
		DEBUG(0, ("%s is not a user account (type=%d)\n", acct_name, name_types[0]));
		return NT_STATUS_INVALID_WORKSTATION;
	}

	user_rid = user_rids[0];
		
	/* Open handle on user */

	status = rpccli_samr_open_user(pipe_hnd, mem_ctx, &domain_pol,
			SEC_RIGHTS_MAXIMUM_ALLOWED, user_rid, &user_pol);
	
	/* Create a random machine account password */

	E_md4hash( clear_pw, md4_trust_password);
	encode_pw_buffer(pwbuf, clear_pw, STR_UNICODE);

	/* Set password on machine account */

	ZERO_STRUCT(ctr);
	ZERO_STRUCT(p24);

	init_sam_user_info24(&p24, (char *)pwbuf,24);

	ctr.switch_value = 24;
	ctr.info.id24 = &p24;

	status = rpccli_samr_set_userinfo(pipe_hnd, mem_ctx, &user_pol, 
			24, &cli->user_session_key, &ctr);

	if ( !NT_STATUS_IS_OK(status) ) {
		d_fprintf( stderr, "Failed to set password for machine account (%s)\n", 
			nt_errstr(status));
		return status;
	}


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

	/* Fill in the additional account flags now */

	acb_info |= ACB_PWNOEXP;
	if ( dom_type == ND_TYPE_AD ) {
#if !defined(ENCTYPE_ARCFOUR_HMAC)
		acb_info |= ACB_USE_DES_KEY_ONLY;
#endif
		;;
	}

	init_sam_user_info16(&p16, acb_info);

	status = rpccli_samr_set_userinfo2(pipe_hnd, mem_ctx, &user_pol, 16, 
					&cli->user_session_key, &ctr);

	rpccli_samr_close(pipe_hnd, mem_ctx, &user_pol);
	cli_rpc_pipe_close(pipe_hnd); /* Done with this pipe */
	
	return status;
}

