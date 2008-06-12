/* 
   Unix SMB/CIFS implementation.
   Authenticate against a remote domain
   Copyright (C) Andrew Tridgell 1992-1998
   Copyright (C) Andrew Bartlett 2001
   
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

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_AUTH

extern BOOL global_machine_password_needs_changing;

/**
 * Connect to a remote server for (inter)domain security authenticaion.
 *
 * @param cli the cli to return containing the active connection
 * @param server either a machine name or text IP address to
 *               connect to.
 * @param setup_creds_as domain account to setup credentials as
 * @param sec_chan a switch value to distinguish between domain
 *                 member and interdomain authentication
 * @param trust_passwd the trust password to establish the
 *                     credentials with.
 *
 **/

static NTSTATUS connect_to_domain_password_server(struct cli_state **cli,
						const char *domain,
						const char *dc_name,
						struct in_addr dc_ip, 
						struct rpc_pipe_client **pipe_ret,
						BOOL *retry)
{
        NTSTATUS result;
	struct rpc_pipe_client *netlogon_pipe = NULL;

	*pipe_ret = NULL;

	/* TODO: Send a SAMLOGON request to determine whether this is a valid
	   logonserver.  We can avoid a 30-second timeout if the DC is down
	   if the SAMLOGON request fails as it is only over UDP. */

	/* we use a mutex to prevent two connections at once - when a 
	   Win2k PDC get two connections where one hasn't completed a 
	   session setup yet it will send a TCP reset to the first 
	   connection (tridge) */

	/*
	 * With NT4.x DC's *all* authentication must be serialized to avoid
	 * ACCESS_DENIED errors if 2 auths are done from the same machine. JRA.
	 */

	if (!grab_server_mutex(dc_name)) {
		return NT_STATUS_NO_LOGON_SERVERS;
	}
	
	/* Attempt connection */
	*retry = True;
	result = cli_full_connection(cli, global_myname(), dc_name, &dc_ip, 0, 
		"IPC$", "IPC", "", "", "", 0, Undefined, retry);

	if (!NT_STATUS_IS_OK(result)) {
		/* map to something more useful */
		if (NT_STATUS_EQUAL(result, NT_STATUS_UNSUCCESSFUL)) {
			result = NT_STATUS_NO_LOGON_SERVERS;
		}

		release_server_mutex();
		return result;
	}

	/*
	 * We now have an anonymous connection to IPC$ on the domain password server.
	 */

	/*
	 * Even if the connect succeeds we need to setup the netlogon
	 * pipe here. We do this as we may just have changed the domain
	 * account password on the PDC and yet we may be talking to
	 * a BDC that doesn't have this replicated yet. In this case
	 * a successful connect to a DC needs to take the netlogon connect
	 * into account also. This patch from "Bjart Kvarme" <bjart.kvarme@usit.uio.no>.
	 */

	/* open the netlogon pipe. */
	if (lp_client_schannel()) {
		/* We also setup the creds chain in the open_schannel call. */
		netlogon_pipe = cli_rpc_pipe_open_schannel(*cli, PI_NETLOGON,
					PIPE_AUTH_LEVEL_PRIVACY, domain, &result);
	} else {
		netlogon_pipe = cli_rpc_pipe_open_noauth(*cli, PI_NETLOGON, &result);
	}

	if(!netlogon_pipe) {
		DEBUG(0,("connect_to_domain_password_server: unable to open the domain client session to \
machine %s. Error was : %s.\n", dc_name, nt_errstr(result)));
		cli_shutdown(*cli);
		release_server_mutex();
		return result;
	}

	if (!lp_client_schannel()) {
		/* We need to set up a creds chain on an unauthenticated netlogon pipe. */
		uint32 neg_flags = NETLOGON_NEG_AUTH2_FLAGS;
		uint32 sec_chan_type = 0;
		unsigned char machine_pwd[16];

		if (!get_trust_pw(domain, machine_pwd, &sec_chan_type)) {
			DEBUG(0, ("connect_to_domain_password_server: could not fetch "
			"trust account password for domain '%s'\n",
				domain));
			cli_shutdown(*cli);
			release_server_mutex();
			return NT_STATUS_CANT_ACCESS_DOMAIN_INFO;
		}

		result = rpccli_netlogon_setup_creds(netlogon_pipe,
					dc_name, /* server name */
					domain, /* domain */
					global_myname(), /* client name */
					global_myname(), /* machine account name */
					machine_pwd,
					sec_chan_type,
					&neg_flags);

		if (!NT_STATUS_IS_OK(result)) {
			cli_shutdown(*cli);
			release_server_mutex();
			return result;
		}
	}

	if(!netlogon_pipe) {
		DEBUG(0,("connect_to_domain_password_server: unable to open the domain client session to \
machine %s. Error was : %s.\n", dc_name, cli_errstr(*cli)));
		cli_shutdown(*cli);
		release_server_mutex();
		return NT_STATUS_NO_LOGON_SERVERS;
	}

	/* We exit here with the mutex *locked*. JRA */

	*pipe_ret = netlogon_pipe;

	return NT_STATUS_OK;
}

/***********************************************************************
 Do the same as security=server, but using NT Domain calls and a session
 key from the machine password.  If the server parameter is specified
 use it, otherwise figure out a server from the 'password server' param.
************************************************************************/

static NTSTATUS domain_client_validate(TALLOC_CTX *mem_ctx,
					const auth_usersupplied_info *user_info, 
					const char *domain,
					uchar chal[8],
					auth_serversupplied_info **server_info, 
					const char *dc_name,
					struct in_addr dc_ip)

{
	NET_USER_INFO_3 info3;
	struct cli_state *cli = NULL;
	struct rpc_pipe_client *netlogon_pipe = NULL;
	NTSTATUS nt_status = NT_STATUS_NO_LOGON_SERVERS;
	int i;
	BOOL retry = True;

	/*
	 * At this point, smb_apasswd points to the lanman response to
	 * the challenge in local_challenge, and smb_ntpasswd points to
	 * the NT response to the challenge in local_challenge. Ship
	 * these over the secure channel to a domain controller and
	 * see if they were valid.
	 */

	/* rety loop for robustness */
	
	for (i = 0; !NT_STATUS_IS_OK(nt_status) && retry && (i < 3); i++) {
		nt_status = connect_to_domain_password_server(&cli,
							domain,
							dc_name,
							dc_ip,
							&netlogon_pipe,
							&retry);
	}

	if ( !NT_STATUS_IS_OK(nt_status) ) {
		DEBUG(0,("domain_client_validate: Domain password server not available.\n"));
		if (NT_STATUS_EQUAL(nt_status, NT_STATUS_ACCESS_DENIED)) {
			return NT_STATUS_TRUSTED_RELATIONSHIP_FAILURE;
		}
		return nt_status;
	}

	/* store a successful connection */

	saf_store( domain, cli->desthost );

	ZERO_STRUCT(info3);

        /*
         * If this call succeeds, we now have lots of info about the user
         * in the info3 structure.  
         */

	nt_status = rpccli_netlogon_sam_network_logon(netlogon_pipe,
						      mem_ctx,
						      user_info->logon_parameters,/* flags such as 'allow workstation logon' */ 
						      dc_name,                    /* server name */
						      user_info->smb_name,        /* user name logging on. */
						      user_info->domain,          /* domain name */
						      user_info->wksta_name,      /* workstation name */
						      chal,                       /* 8 byte challenge. */
						      user_info->lm_resp,         /* lanman 24 byte response */
						      user_info->nt_resp,         /* nt 24 byte response */
						      &info3);                    /* info3 out */

	/* Let go as soon as possible so we avoid any potential deadlocks
	   with winbind lookup up users or groups. */
	   
	release_server_mutex();

	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(0,("domain_client_validate: unable to validate password "
                         "for user %s in domain %s to Domain controller %s. "
                         "Error was %s.\n", user_info->smb_name,
                         user_info->domain, dc_name, 
                         nt_errstr(nt_status)));

		/* map to something more useful */
		if (NT_STATUS_EQUAL(nt_status, NT_STATUS_UNSUCCESSFUL)) {
			nt_status = NT_STATUS_NO_LOGON_SERVERS;
		}
	} else {
		nt_status = make_server_info_info3(mem_ctx,
						user_info->smb_name,
						domain,
						server_info,
						&info3);

		if (NT_STATUS_IS_OK(nt_status)) {
			(*server_info)->was_mapped |= user_info->was_mapped;
		}

		netsamlogon_cache_store( user_info->smb_name, &info3 );
	}

	/* Note - once the cli stream is shutdown the mem_ctx used
	   to allocate the other_sids and gids structures has been deleted - so
	   these pointers are no longer valid..... */

	cli_shutdown(cli);
	return nt_status;
}

/****************************************************************************
 Check for a valid username and password in security=domain mode.
****************************************************************************/

static NTSTATUS check_ntdomain_security(const struct auth_context *auth_context,
					void *my_private_data, 
					TALLOC_CTX *mem_ctx,
					const auth_usersupplied_info *user_info, 
					auth_serversupplied_info **server_info)
{
	NTSTATUS nt_status = NT_STATUS_LOGON_FAILURE;
	const char *domain = lp_workgroup();
	fstring dc_name;
	struct in_addr dc_ip;

	if ( lp_server_role() != ROLE_DOMAIN_MEMBER ) {
		DEBUG(0,("check_ntdomain_security: Configuration error!  Cannot use "
			"ntdomain auth method when not a member of a domain.\n"));
		return NT_STATUS_NOT_IMPLEMENTED;
	}

	if (!user_info || !server_info || !auth_context) {
		DEBUG(1,("check_ntdomain_security: Critical variables not present.  Failing.\n"));
		return NT_STATUS_INVALID_PARAMETER;
	}

	/* 
	 * Check that the requested domain is not our own machine name.
	 * If it is, we should never check the PDC here, we use our own local
	 * password file.
	 */

	if(strequal(get_global_sam_name(), user_info->domain)) {
		DEBUG(3,("check_ntdomain_security: Requested domain was for this machine.\n"));
		return NT_STATUS_NOT_IMPLEMENTED;
	}

	/* we need our DC to send the net_sam_logon() request to */

	if ( !get_dc_name(domain, NULL, dc_name, &dc_ip) ) {
		DEBUG(5,("check_ntdomain_security: unable to locate a DC for domain %s\n",
			user_info->domain));
		return NT_STATUS_NO_LOGON_SERVERS;
	}
	
	nt_status = domain_client_validate(mem_ctx,
					user_info,
					domain,
					(uchar *)auth_context->challenge.data,
					server_info,
					dc_name,
					dc_ip);
		
	return nt_status;
}

/* module initialisation */
static NTSTATUS auth_init_ntdomain(struct auth_context *auth_context, const char* param, auth_methods **auth_method) 
{
	if (!make_auth_methods(auth_context, auth_method)) {
		return NT_STATUS_NO_MEMORY;
	}

	(*auth_method)->name = "ntdomain";
	(*auth_method)->auth = check_ntdomain_security;
	return NT_STATUS_OK;
}


/****************************************************************************
 Check for a valid username and password in a trusted domain
****************************************************************************/

static NTSTATUS check_trustdomain_security(const struct auth_context *auth_context,
					   void *my_private_data, 
					   TALLOC_CTX *mem_ctx,
					   const auth_usersupplied_info *user_info, 
					   auth_serversupplied_info **server_info)
{
	NTSTATUS nt_status = NT_STATUS_LOGON_FAILURE;
	unsigned char trust_md4_password[16];
	char *trust_password;
	time_t last_change_time;
	DOM_SID sid;
	fstring dc_name;
	struct in_addr dc_ip;

	if (!user_info || !server_info || !auth_context) {
		DEBUG(1,("check_trustdomain_security: Critical variables not present.  Failing.\n"));
		return NT_STATUS_INVALID_PARAMETER;
	}

	/* 
	 * Check that the requested domain is not our own machine name or domain name.
	 */

	if( strequal(get_global_sam_name(), user_info->domain)) {
		DEBUG(3,("check_trustdomain_security: Requested domain [%s] was for this machine.\n",
			user_info->domain));
		return NT_STATUS_NOT_IMPLEMENTED;
	}

	/* No point is bothering if this is not a trusted domain.
	   This return makes "map to guest = bad user" work again.
	   The logic is that if we know nothing about the domain, that
	   user is not known to us and does not exist */
	
	if ( !is_trusted_domain( user_info->domain ) )
		return NT_STATUS_NOT_IMPLEMENTED;

	/*
	 * Get the trusted account password for the trusted domain
	 * No need to become_root() as secrets_init() is done at startup.
	 */

	if (!secrets_fetch_trusted_domain_password(user_info->domain, &trust_password,
				&sid, &last_change_time)) {
		DEBUG(0, ("check_trustdomain_security: could not fetch trust "
			  "account password for domain %s\n",
			  user_info->domain));
		return NT_STATUS_CANT_ACCESS_DOMAIN_INFO;
	}

#ifdef DEBUG_PASSWORD
	DEBUG(100, ("Trust password for domain %s is %s\n", user_info->domain,
		    trust_password));
#endif
	E_md4hash(trust_password, trust_md4_password);
	SAFE_FREE(trust_password);

#if 0
	/* Test if machine password is expired and need to be changed */
	if (time(NULL) > last_change_time + (time_t)lp_machine_password_timeout())
	{
		global_machine_password_needs_changing = True;
	}
#endif

	/* use get_dc_name() for consistency even through we know that it will be 
	   a netbios name */
	   
	if ( !get_dc_name(user_info->domain, NULL, dc_name, &dc_ip) ) {
		DEBUG(5,("check_trustdomain_security: unable to locate a DC for domain %s\n",
			user_info->domain));
		return NT_STATUS_NO_LOGON_SERVERS;
	}
	
	nt_status = domain_client_validate(mem_ctx,
					user_info,
					user_info->domain,
					(uchar *)auth_context->challenge.data,
					server_info,
					dc_name,
					dc_ip);

	return nt_status;
}

/* module initialisation */
static NTSTATUS auth_init_trustdomain(struct auth_context *auth_context, const char* param, auth_methods **auth_method) 
{
	if (!make_auth_methods(auth_context, auth_method)) {
		return NT_STATUS_NO_MEMORY;
	}

	(*auth_method)->name = "trustdomain";
	(*auth_method)->auth = check_trustdomain_security;
	return NT_STATUS_OK;
}

NTSTATUS auth_domain_init(void) 
{
	smb_register_auth(AUTH_INTERFACE_VERSION, "trustdomain", auth_init_trustdomain);
	smb_register_auth(AUTH_INTERFACE_VERSION, "ntdomain", auth_init_ntdomain);
	return NT_STATUS_OK;
}
