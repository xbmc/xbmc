/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Andrew Tridgell              1992-1997,
 *  Copyright (C) Luke Kenneth Casson Leighton 1996-1997,
 *  Copyright (C) Paul Ashton                       1997.
 *  Copyright (C) Jeremy Allison               1998-2001.
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

/* This is the implementation of the netlogon pipe. */

#include "includes.h"

extern userdom_struct current_user_info;

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

/*************************************************************************
 init_net_r_req_chal:
 *************************************************************************/

static void init_net_r_req_chal(NET_R_REQ_CHAL *r_c,
                                DOM_CHAL *srv_chal, NTSTATUS status)
{
	DEBUG(6,("init_net_r_req_chal: %d\n", __LINE__));
	memcpy(r_c->srv_chal.data, srv_chal->data, sizeof(srv_chal->data));
	r_c->status = status;
}

/*************************************************************************
 error messages cropping up when using nltest.exe...
 *************************************************************************/

#define ERROR_NO_SUCH_DOMAIN   0x54b
#define ERROR_NO_LOGON_SERVERS 0x51f
#define NO_ERROR               0x0

/*************************************************************************
 net_reply_logon_ctrl:
 *************************************************************************/

NTSTATUS _net_logon_ctrl(pipes_struct *p, NET_Q_LOGON_CTRL *q_u, 
		       NET_R_LOGON_CTRL *r_u)
{
	uint32 flags = 0x0;
	uint32 pdc_connection_status = 0x00; /* Maybe a win32 error code? */
	
	/* Setup the Logon Control response */

	init_net_r_logon_ctrl(r_u, q_u->query_level, flags, 
			      pdc_connection_status);

	return r_u->status;
}

/****************************************************************************
Send a message to smbd to do a sam synchronisation
**************************************************************************/

static void send_sync_message(void)
{
        TDB_CONTEXT *tdb;

        tdb = tdb_open_log(lock_path("connections.tdb"), 0,
                           TDB_DEFAULT, O_RDONLY, 0);

        if (!tdb) {
                DEBUG(3, ("send_sync_message(): failed to open connections "
                          "database\n"));
                return;
        }

        DEBUG(3, ("sending sam synchronisation message\n"));
        
        message_send_all(tdb, MSG_SMB_SAM_SYNC, NULL, 0, False, NULL);

        tdb_close(tdb);
}

/*************************************************************************
 net_reply_logon_ctrl2:
 *************************************************************************/

NTSTATUS _net_logon_ctrl2(pipes_struct *p, NET_Q_LOGON_CTRL2 *q_u, NET_R_LOGON_CTRL2 *r_u)
{
        uint32 flags = 0x0;
        uint32 pdc_connection_status = 0x0;
        uint32 logon_attempts = 0x0;
        uint32 tc_status;
	fstring servername, domain, dc_name, dc_name2;
	struct in_addr dc_ip;

	/* this should be \\global_myname() */
	unistr2_to_ascii(servername, &q_u->uni_server_name, sizeof(servername));

	r_u->status = NT_STATUS_OK;
	
	tc_status = ERROR_NO_SUCH_DOMAIN;
	fstrcpy( dc_name, "" );
	
	switch ( q_u->function_code ) {
		case NETLOGON_CONTROL_TC_QUERY:
			unistr2_to_ascii(domain, &q_u->info.info6.domain, sizeof(domain));
				
			if ( !is_trusted_domain( domain ) )
				break;
				
			if ( !get_dc_name( domain, NULL, dc_name2, &dc_ip ) ) {
				tc_status = ERROR_NO_LOGON_SERVERS;
				break;
			}

			fstr_sprintf( dc_name, "\\\\%s", dc_name2 );
				
			tc_status = NO_ERROR;
			
			break;
			
		case NETLOGON_CONTROL_REDISCOVER:
			unistr2_to_ascii(domain, &q_u->info.info6.domain, sizeof(domain));
				
			if ( !is_trusted_domain( domain ) )
				break;
				
			if ( !get_dc_name( domain, NULL, dc_name2, &dc_ip ) ) {
				tc_status = ERROR_NO_LOGON_SERVERS;
				break;
			}

			fstr_sprintf( dc_name, "\\\\%s", dc_name2 );
				
			tc_status = NO_ERROR;
			
			break;
			
		default:
			/* no idea what this should be */
			DEBUG(0,("_net_logon_ctrl2: unimplemented function level [%d]\n",
				q_u->function_code));
	}
	
	/* prepare the response */
	
	init_net_r_logon_ctrl2( r_u, q_u->query_level, flags, 
		pdc_connection_status, logon_attempts, tc_status, dc_name );

        if (lp_server_role() == ROLE_DOMAIN_BDC)
                send_sync_message();

	return r_u->status;
}

/*************************************************************************
 net_reply_trust_dom_list:
 *************************************************************************/

NTSTATUS _net_trust_dom_list(pipes_struct *p, NET_Q_TRUST_DOM_LIST *q_u, NET_R_TRUST_DOM_LIST *r_u)
{
	const char *trusted_domain = "test_domain";
	uint32 num_trust_domains = 1;

	DEBUG(6,("_net_trust_dom_list: %d\n", __LINE__));

	/* set up the Trusted Domain List response */
	init_r_trust_dom(r_u, num_trust_domains, trusted_domain);

	DEBUG(6,("_net_trust_dom_list: %d\n", __LINE__));

	return r_u->status;
}

/***********************************************************************************
 init_net_r_srv_pwset:
 ***********************************************************************************/

static void init_net_r_srv_pwset(NET_R_SRV_PWSET *r_s,
				 DOM_CRED *srv_cred, NTSTATUS status)  
{
	DEBUG(5,("init_net_r_srv_pwset: %d\n", __LINE__));

	memcpy(&r_s->srv_cred, srv_cred, sizeof(r_s->srv_cred));
	r_s->status = status;

	DEBUG(5,("init_net_r_srv_pwset: %d\n", __LINE__));
}

/******************************************************************
 gets a machine password entry.  checks access rights of the host.
 ******************************************************************/

static NTSTATUS get_md4pw(char *md4pw, char *mach_acct, uint16 sec_chan_type)
{
	struct samu *sampass = NULL;
	const uint8 *pass;
	BOOL ret;
	uint32 acct_ctrl;

#if 0
    /*
     * Currently this code is redundent as we already have a filter
     * by hostname list. What this code really needs to do is to 
     * get a hosts allowed/hosts denied list from the SAM database
     * on a per user basis, and make the access decision there.
     * I will leave this code here for now as a reminder to implement
     * this at a later date. JRA.
     */

	if (!allow_access(lp_domain_hostsdeny(), lp_domain_hostsallow(),
	                  client_name(), client_addr()))
	{
		DEBUG(0,("get_md4pw: Workstation %s denied access to domain\n", mach_acct));
		return False;
	}
#endif /* 0 */

	if ( !(sampass = samu_new( NULL )) ) {
		return NT_STATUS_NO_MEMORY;
	}

	/* JRA. This is ok as it is only used for generating the challenge. */
	become_root();
	ret = pdb_getsampwnam(sampass, mach_acct);
	unbecome_root();
 
 	if (!ret) {
 		DEBUG(0,("get_md4pw: Workstation %s: no account in domain\n", mach_acct));
		TALLOC_FREE(sampass);
		return NT_STATUS_ACCESS_DENIED;
	}

	acct_ctrl = pdb_get_acct_ctrl(sampass);
	if (acct_ctrl & ACB_DISABLED) {
		DEBUG(0,("get_md4pw: Workstation %s: account is disabled\n", mach_acct));
		TALLOC_FREE(sampass);
		return NT_STATUS_ACCOUNT_DISABLED;
	}

	if (!(acct_ctrl & ACB_SVRTRUST) &&
	    !(acct_ctrl & ACB_WSTRUST) &&
	    !(acct_ctrl & ACB_DOMTRUST)) 
	{
		DEBUG(0,("get_md4pw: Workstation %s: account is not a trust account\n", mach_acct));
		TALLOC_FREE(sampass);
		return NT_STATUS_NO_TRUST_SAM_ACCOUNT;
	}

	switch (sec_chan_type) {
		case SEC_CHAN_BDC:
			if (!(acct_ctrl & ACB_SVRTRUST)) {
				DEBUG(0,("get_md4pw: Workstation %s: BDC secure channel requested "
					 "but not a server trust account\n", mach_acct));
				TALLOC_FREE(sampass);
				return NT_STATUS_NO_TRUST_SAM_ACCOUNT;
			}
			break;
		case SEC_CHAN_WKSTA:
			if (!(acct_ctrl & ACB_WSTRUST)) {
				DEBUG(0,("get_md4pw: Workstation %s: WORKSTATION secure channel requested "
					 "but not a workstation trust account\n", mach_acct));
				TALLOC_FREE(sampass);
				return NT_STATUS_NO_TRUST_SAM_ACCOUNT;
			}
			break;
		case SEC_CHAN_DOMAIN:
			if (!(acct_ctrl & ACB_DOMTRUST)) {
				DEBUG(0,("get_md4pw: Workstation %s: DOMAIN secure channel requested "
					 "but not a interdomain trust account\n", mach_acct));
				TALLOC_FREE(sampass);
				return NT_STATUS_NO_TRUST_SAM_ACCOUNT;
			}
			break;
		default:
			break;
	}

	if ((pass = pdb_get_nt_passwd(sampass)) == NULL) {
		DEBUG(0,("get_md4pw: Workstation %s: account does not have a password\n", mach_acct));
		TALLOC_FREE(sampass);
		return NT_STATUS_LOGON_FAILURE;
	}

	memcpy(md4pw, pass, 16);
	dump_data(5, md4pw, 16);

	TALLOC_FREE(sampass);
	
	return NT_STATUS_OK;
 	

}

/*************************************************************************
 _net_req_chal
 *************************************************************************/

NTSTATUS _net_req_chal(pipes_struct *p, NET_Q_REQ_CHAL *q_u, NET_R_REQ_CHAL *r_u)
{
	if (!p->dc) {
		p->dc = TALLOC_ZERO_P(p->pipe_state_mem_ctx, struct dcinfo);
		if (!p->dc) {
			return NT_STATUS_NO_MEMORY;
		}
	} else {
		DEBUG(10,("_net_req_chal: new challenge requested. Clearing old state.\n"));
		ZERO_STRUCTP(p->dc);
	}

	rpcstr_pull(p->dc->remote_machine,
			q_u->uni_logon_clnt.buffer,
			sizeof(fstring),q_u->uni_logon_clnt.uni_str_len*2,0);

	/* Save the client challenge to the server. */
	memcpy(p->dc->clnt_chal.data, q_u->clnt_chal.data, sizeof(q_u->clnt_chal.data));

	/* Create a server challenge for the client */
	/* Set this to a random value. */
	generate_random_buffer(p->dc->srv_chal.data, 8);
	
	/* set up the LSA REQUEST CHALLENGE response */
	init_net_r_req_chal(r_u, &p->dc->srv_chal, NT_STATUS_OK);
	
	p->dc->challenge_sent = True;

	return NT_STATUS_OK;
}

/*************************************************************************
 init_net_r_auth:
 *************************************************************************/

static void init_net_r_auth(NET_R_AUTH *r_a, DOM_CHAL *resp_cred, NTSTATUS status)
{
	memcpy(r_a->srv_chal.data, resp_cred->data, sizeof(resp_cred->data));
	r_a->status = status;
}

/*************************************************************************
 _net_auth. Create the initial credentials.
 *************************************************************************/

NTSTATUS _net_auth(pipes_struct *p, NET_Q_AUTH *q_u, NET_R_AUTH *r_u)
{
	NTSTATUS status;
	fstring mach_acct;
	fstring remote_machine;
	DOM_CHAL srv_chal_out;

	if (!p->dc || !p->dc->challenge_sent) {
		return NT_STATUS_ACCESS_DENIED;
	}

	rpcstr_pull(mach_acct, q_u->clnt_id.uni_acct_name.buffer,sizeof(fstring),
				q_u->clnt_id.uni_acct_name.uni_str_len*2,0);
	rpcstr_pull(remote_machine, q_u->clnt_id.uni_comp_name.buffer,sizeof(fstring),
				q_u->clnt_id.uni_comp_name.uni_str_len*2,0);

	status = get_md4pw((char *)p->dc->mach_pw, mach_acct, q_u->clnt_id.sec_chan);
	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(0,("_net_auth: creds_server_check failed. Failed to "
			"get password for machine account %s "
			"from client %s: %s\n",
			mach_acct, remote_machine, nt_errstr(status) ));
		/* always return NT_STATUS_ACCESS_DENIED */
		return NT_STATUS_ACCESS_DENIED;
	}

	/* From the client / server challenges and md4 password, generate sess key */
	creds_server_init(0,			/* No neg flags. */
			p->dc,
			&p->dc->clnt_chal,	/* Stored client chal. */
			&p->dc->srv_chal,	/* Stored server chal. */
			p->dc->mach_pw,
			&srv_chal_out);	

	/* Check client credentials are valid. */
	if (!creds_server_check(p->dc, &q_u->clnt_chal)) {
		DEBUG(0,("_net_auth: creds_server_check failed. Rejecting auth "
			"request from client %s machine account %s\n",
			remote_machine, mach_acct ));
		return NT_STATUS_ACCESS_DENIED;
	}

	fstrcpy(p->dc->mach_acct, mach_acct);
	fstrcpy(p->dc->remote_machine, remote_machine);
	p->dc->authenticated = True;

	/* set up the LSA AUTH response */
	/* Return the server credentials. */
	init_net_r_auth(r_u, &srv_chal_out, NT_STATUS_OK);

	return r_u->status;
}

/*************************************************************************
 init_net_r_auth_2:
 *************************************************************************/

static void init_net_r_auth_2(NET_R_AUTH_2 *r_a,
                              DOM_CHAL *resp_cred, NEG_FLAGS *flgs, NTSTATUS status)
{
	memcpy(r_a->srv_chal.data, resp_cred->data, sizeof(resp_cred->data));
	memcpy(&r_a->srv_flgs, flgs, sizeof(r_a->srv_flgs));
	r_a->status = status;
}

/*************************************************************************
 _net_auth_2
 *************************************************************************/

NTSTATUS _net_auth_2(pipes_struct *p, NET_Q_AUTH_2 *q_u, NET_R_AUTH_2 *r_u)
{
	NTSTATUS status;
	NEG_FLAGS srv_flgs;
	fstring mach_acct;
	fstring remote_machine;
	DOM_CHAL srv_chal_out;

	rpcstr_pull(mach_acct, q_u->clnt_id.uni_acct_name.buffer,sizeof(fstring),
				q_u->clnt_id.uni_acct_name.uni_str_len*2,0);

	/* We use this as the key to store the creds. */
	rpcstr_pull(remote_machine, q_u->clnt_id.uni_comp_name.buffer,sizeof(fstring),
				q_u->clnt_id.uni_comp_name.uni_str_len*2,0);

	if (!p->dc || !p->dc->challenge_sent) {
		DEBUG(0,("_net_auth2: no challenge sent to client %s\n",
			remote_machine ));
		return NT_STATUS_ACCESS_DENIED;
	}

	if ( (lp_server_schannel() == True) &&
	     ((q_u->clnt_flgs.neg_flags & NETLOGON_NEG_SCHANNEL) == 0) ) {

		/* schannel must be used, but client did not offer it. */
		DEBUG(0,("_net_auth2: schannel required but client failed "
			"to offer it. Client was %s\n",
			mach_acct ));
		return NT_STATUS_ACCESS_DENIED;
	}

	status = get_md4pw((char *)p->dc->mach_pw, mach_acct, q_u->clnt_id.sec_chan);
	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(0,("_net_auth2: failed to get machine password for "
			"account %s: %s\n",
			mach_acct, nt_errstr(status) ));
		/* always return NT_STATUS_ACCESS_DENIED */
		return NT_STATUS_ACCESS_DENIED;
	}

	/* From the client / server challenges and md4 password, generate sess key */
	creds_server_init(q_u->clnt_flgs.neg_flags,
			p->dc,
			&p->dc->clnt_chal,	/* Stored client chal. */
			&p->dc->srv_chal,	/* Stored server chal. */
			p->dc->mach_pw,
			&srv_chal_out);	

	/* Check client credentials are valid. */
	if (!creds_server_check(p->dc, &q_u->clnt_chal)) {
		DEBUG(0,("_net_auth2: creds_server_check failed. Rejecting auth "
			"request from client %s machine account %s\n",
			remote_machine, mach_acct ));
		return NT_STATUS_ACCESS_DENIED;
	}

	srv_flgs.neg_flags = 0x000001ff;

	if (lp_server_schannel() != False) {
		srv_flgs.neg_flags |= NETLOGON_NEG_SCHANNEL;
	}

	/* set up the LSA AUTH 2 response */
	init_net_r_auth_2(r_u, &srv_chal_out, &srv_flgs, NT_STATUS_OK);

	fstrcpy(p->dc->mach_acct, mach_acct);
	fstrcpy(p->dc->remote_machine, remote_machine);
	fstrcpy(p->dc->domain, lp_workgroup() );

	p->dc->authenticated = True;

	/* Store off the state so we can continue after client disconnect. */
	become_root();
	secrets_store_schannel_session_info(p->mem_ctx,
					remote_machine,
					p->dc);
	unbecome_root();

	return r_u->status;
}

/*************************************************************************
 _net_srv_pwset
 *************************************************************************/

NTSTATUS _net_srv_pwset(pipes_struct *p, NET_Q_SRV_PWSET *q_u, NET_R_SRV_PWSET *r_u)
{
	fstring remote_machine;
	struct samu *sampass=NULL;
	BOOL ret = False;
	unsigned char pwd[16];
	int i;
	uint32 acct_ctrl;
	DOM_CRED cred_out;
	const uchar *old_pw;

	DEBUG(5,("_net_srv_pwset: %d\n", __LINE__));

	/* We need the remote machine name for the creds lookup. */
	rpcstr_pull(remote_machine,q_u->clnt_id.login.uni_comp_name.buffer,
		    sizeof(remote_machine),q_u->clnt_id.login.uni_comp_name.uni_str_len*2,0);

	if ( (lp_server_schannel() == True) && (p->auth.auth_type != PIPE_AUTH_TYPE_SCHANNEL) ) {
		/* 'server schannel = yes' should enforce use of
		   schannel, the client did offer it in auth2, but
		   obviously did not use it. */
		DEBUG(0,("_net_srv_pwset: client %s not using schannel for netlogon\n",
			remote_machine ));
		return NT_STATUS_ACCESS_DENIED;
	}

	if (!p->dc) {
		/* Restore the saved state of the netlogon creds. */
		become_root();
		ret = secrets_restore_schannel_session_info(p->pipe_state_mem_ctx,
							remote_machine,
							&p->dc);
		unbecome_root();
		if (!ret) {
			return NT_STATUS_INVALID_HANDLE;
		}
	}

	if (!p->dc || !p->dc->authenticated) {
		return NT_STATUS_INVALID_HANDLE;
	}

	DEBUG(3,("_net_srv_pwset: Server Password Set by remote machine:[%s] on account [%s]\n",
			remote_machine, p->dc->mach_acct));
	
	/* Step the creds chain forward. */
	if (!creds_server_step(p->dc, &q_u->clnt_id.cred, &cred_out)) {
		DEBUG(2,("_net_srv_pwset: creds_server_step failed. Rejecting auth "
			"request from client %s machine account %s\n",
			remote_machine, p->dc->mach_acct ));
		return NT_STATUS_INVALID_PARAMETER;
	}

	/* We must store the creds state after an update. */
	sampass = samu_new( NULL );
	if (!sampass) {
		return NT_STATUS_NO_MEMORY;
	}

	become_root();
	secrets_store_schannel_session_info(p->pipe_state_mem_ctx,
						remote_machine,
						p->dc);
	ret = pdb_getsampwnam(sampass, p->dc->mach_acct);
	unbecome_root();

	if (!ret) {
		TALLOC_FREE(sampass);
		return NT_STATUS_ACCESS_DENIED;
	}

	/* Ensure the account exists and is a machine account. */
	
	acct_ctrl = pdb_get_acct_ctrl(sampass);

	if (!(acct_ctrl & ACB_WSTRUST ||
		      acct_ctrl & ACB_SVRTRUST ||
		      acct_ctrl & ACB_DOMTRUST)) {
		TALLOC_FREE(sampass);
		return NT_STATUS_NO_SUCH_USER;
	}
	
	if (pdb_get_acct_ctrl(sampass) & ACB_DISABLED) {
		TALLOC_FREE(sampass);
		return NT_STATUS_ACCOUNT_DISABLED;
	}

	/* Woah - what does this to to the credential chain ? JRA */
	cred_hash3( pwd, q_u->pwd, p->dc->sess_key, 0);

	DEBUG(100,("Server password set : new given value was :\n"));
	for(i = 0; i < sizeof(pwd); i++)
		DEBUG(100,("%02X ", pwd[i]));
	DEBUG(100,("\n"));

	old_pw = pdb_get_nt_passwd(sampass);

	if (old_pw && memcmp(pwd, old_pw, 16) == 0) {
		/* Avoid backend modificiations and other fun if the 
		   client changed the password to the *same thing* */

		ret = True;
	} else {

		/* LM password should be NULL for machines */
		if (!pdb_set_lanman_passwd(sampass, NULL, PDB_CHANGED)) {
			TALLOC_FREE(sampass);
			return NT_STATUS_NO_MEMORY;
		}
		
		if (!pdb_set_nt_passwd(sampass, pwd, PDB_CHANGED)) {
			TALLOC_FREE(sampass);
			return NT_STATUS_NO_MEMORY;
		}
		
		if (!pdb_set_pass_changed_now(sampass)) {
			TALLOC_FREE(sampass);
			/* Not quite sure what this one qualifies as, but this will do */
			return NT_STATUS_UNSUCCESSFUL; 
		}
		
		become_root();
		r_u->status = pdb_update_sam_account(sampass);
		unbecome_root();
	}

	/* set up the LSA Server Password Set response */
	init_net_r_srv_pwset(r_u, &cred_out, r_u->status);

	TALLOC_FREE(sampass);
	return r_u->status;
}

/*************************************************************************
 _net_sam_logoff:
 *************************************************************************/

NTSTATUS _net_sam_logoff(pipes_struct *p, NET_Q_SAM_LOGOFF *q_u, NET_R_SAM_LOGOFF *r_u)
{
	fstring remote_machine;

	if ( (lp_server_schannel() == True) && (p->auth.auth_type != PIPE_AUTH_TYPE_SCHANNEL) ) {
		/* 'server schannel = yes' should enforce use of
		   schannel, the client did offer it in auth2, but
		   obviously did not use it. */
		DEBUG(0,("_net_sam_logoff: client %s not using schannel for netlogon\n",
			get_remote_machine_name() ));
		return NT_STATUS_ACCESS_DENIED;
	}


	if (!get_valid_user_struct(p->vuid))
		return NT_STATUS_NO_SUCH_USER;

	/* Get the remote machine name for the creds store. */
	rpcstr_pull(remote_machine,q_u->sam_id.client.login.uni_comp_name.buffer,
		    sizeof(remote_machine),q_u->sam_id.client.login.uni_comp_name.uni_str_len*2,0);

	if (!p->dc) {
		/* Restore the saved state of the netlogon creds. */
		BOOL ret;

		become_root();
		ret = secrets_restore_schannel_session_info(p->pipe_state_mem_ctx,
						remote_machine,
						&p->dc);
		unbecome_root();
		if (!ret) {
			return NT_STATUS_INVALID_HANDLE;
		}
	}

	if (!p->dc || !p->dc->authenticated) {
		return NT_STATUS_INVALID_HANDLE;
	}

	r_u->buffer_creds = 1; /* yes, we have valid server credentials */

	/* checks and updates credentials.  creates reply credentials */
	if (!creds_server_step(p->dc, &q_u->sam_id.client.cred, &r_u->srv_creds)) {
		DEBUG(2,("_net_sam_logoff: creds_server_step failed. Rejecting auth "
			"request from client %s machine account %s\n",
			remote_machine, p->dc->mach_acct ));
		return NT_STATUS_INVALID_PARAMETER;
	}

	/* We must store the creds state after an update. */
	become_root();
	secrets_store_schannel_session_info(p->pipe_state_mem_ctx,
					remote_machine,
					p->dc);
	unbecome_root();

	r_u->status = NT_STATUS_OK;
	return r_u->status;
}

/*******************************************************************
 gets a domain user's groups from their already-calculated NT_USER_TOKEN
 ********************************************************************/

static NTSTATUS nt_token_to_group_list(TALLOC_CTX *mem_ctx,
				       const DOM_SID *domain_sid,
				       size_t num_sids,
				       const DOM_SID *sids,
				       int *numgroups, DOM_GID **pgids) 
{
	int i;

	*numgroups=0;
	*pgids = NULL;

	for (i=0; i<num_sids; i++) {
		DOM_GID gid;
		if (!sid_peek_check_rid(domain_sid, &sids[i], &gid.g_rid)) {
			continue;
		}
		gid.attr = (SE_GROUP_MANDATORY|SE_GROUP_ENABLED_BY_DEFAULT|
			    SE_GROUP_ENABLED);
		ADD_TO_ARRAY(mem_ctx, DOM_GID, gid, pgids, numgroups);
		if (*pgids == NULL) {
			return NT_STATUS_NO_MEMORY;
		}
	}
	return NT_STATUS_OK;
}

/*************************************************************************
 _net_sam_logon
 *************************************************************************/

static NTSTATUS _net_sam_logon_internal(pipes_struct *p,
					NET_Q_SAM_LOGON *q_u,
					NET_R_SAM_LOGON *r_u,
					BOOL process_creds)
{
	NTSTATUS status = NT_STATUS_OK;
	NET_USER_INFO_3 *usr_info = NULL;
	NET_ID_INFO_CTR *ctr = q_u->sam_id.ctr;
	UNISTR2 *uni_samlogon_user = NULL;
	UNISTR2 *uni_samlogon_domain = NULL;
	UNISTR2 *uni_samlogon_workstation = NULL;
	fstring nt_username, nt_domain, nt_workstation;
	auth_usersupplied_info *user_info = NULL;
	auth_serversupplied_info *server_info = NULL;
	struct samu *sampw;
	struct auth_context *auth_context = NULL;
	 
	if ( (lp_server_schannel() == True) && (p->auth.auth_type != PIPE_AUTH_TYPE_SCHANNEL) ) {
		/* 'server schannel = yes' should enforce use of
		   schannel, the client did offer it in auth2, but
		   obviously did not use it. */
		DEBUG(0,("_net_sam_logon_internal: client %s not using schannel for netlogon\n",
			get_remote_machine_name() ));
		return NT_STATUS_ACCESS_DENIED;
	}

	usr_info = TALLOC_P(p->mem_ctx, NET_USER_INFO_3);
	if (!usr_info) {
		return NT_STATUS_NO_MEMORY;
	}

	ZERO_STRUCTP(usr_info);

 	/* store the user information, if there is any. */
	r_u->user = usr_info;
	r_u->auth_resp = 1; /* authoritative response */
	if (q_u->validation_level != 2 && q_u->validation_level != 3) {
		DEBUG(0,("_net_sam_logon: bad validation_level value %d.\n", (int)q_u->validation_level ));
		return NT_STATUS_ACCESS_DENIED;
	}
	/* We handle the return of USER_INFO_2 instead of 3 in the parse return. Sucks, I know... */
	r_u->switch_value = q_u->validation_level; /* indicates type of validation user info */
	r_u->buffer_creds = 1; /* Ensure we always return server creds. */
 
	if (!get_valid_user_struct(p->vuid))
		return NT_STATUS_NO_SUCH_USER;

	if (process_creds) {
		fstring remote_machine;

		/* Get the remote machine name for the creds store. */
		/* Note this is the remote machine this request is coming from (member server),
		   not neccessarily the workstation name the user is logging onto.
		*/
		rpcstr_pull(remote_machine,q_u->sam_id.client.login.uni_comp_name.buffer,
		    sizeof(remote_machine),q_u->sam_id.client.login.uni_comp_name.uni_str_len*2,0);

		if (!p->dc) {
			/* Restore the saved state of the netlogon creds. */
			BOOL ret;

			become_root();
			ret = secrets_restore_schannel_session_info(p->pipe_state_mem_ctx,
					remote_machine,
					&p->dc);
			unbecome_root();
			if (!ret) {
				return NT_STATUS_INVALID_HANDLE;
			}
		}

		if (!p->dc || !p->dc->authenticated) {
			return NT_STATUS_INVALID_HANDLE;
		}

		/* checks and updates credentials.  creates reply credentials */
		if (!creds_server_step(p->dc, &q_u->sam_id.client.cred,  &r_u->srv_creds)) {
			DEBUG(2,("_net_sam_logon: creds_server_step failed. Rejecting auth "
				"request from client %s machine account %s\n",
				remote_machine, p->dc->mach_acct ));
			return NT_STATUS_INVALID_PARAMETER;
		}

		/* We must store the creds state after an update. */
		become_root();
		secrets_store_schannel_session_info(p->pipe_state_mem_ctx,
					remote_machine,
					p->dc);
		unbecome_root();
	}

	switch (q_u->sam_id.logon_level) {
	case INTERACTIVE_LOGON_TYPE:
		uni_samlogon_user = &ctr->auth.id1.uni_user_name;
 		uni_samlogon_domain = &ctr->auth.id1.uni_domain_name;

                uni_samlogon_workstation = &ctr->auth.id1.uni_wksta_name;
            
		DEBUG(3,("SAM Logon (Interactive). Domain:[%s].  ", lp_workgroup()));
		break;
	case NET_LOGON_TYPE:
		uni_samlogon_user = &ctr->auth.id2.uni_user_name;
		uni_samlogon_domain = &ctr->auth.id2.uni_domain_name;
		uni_samlogon_workstation = &ctr->auth.id2.uni_wksta_name;
            
		DEBUG(3,("SAM Logon (Network). Domain:[%s].  ", lp_workgroup()));
		break;
	default:
		DEBUG(2,("SAM Logon: unsupported switch value\n"));
		return NT_STATUS_INVALID_INFO_CLASS;
	} /* end switch */

	rpcstr_pull(nt_username,uni_samlogon_user->buffer,sizeof(nt_username),uni_samlogon_user->uni_str_len*2,0);
	rpcstr_pull(nt_domain,uni_samlogon_domain->buffer,sizeof(nt_domain),uni_samlogon_domain->uni_str_len*2,0);
	rpcstr_pull(nt_workstation,uni_samlogon_workstation->buffer,sizeof(nt_workstation),uni_samlogon_workstation->uni_str_len*2,0);

	DEBUG(3,("User:[%s@%s] Requested Domain:[%s]\n", nt_username, nt_workstation, nt_domain));
	fstrcpy(current_user_info.smb_name, nt_username);
	sub_set_smb_name(nt_username);
     
	DEBUG(5,("Attempting validation level %d for unmapped username %s.\n", q_u->sam_id.ctr->switch_value, nt_username));

	status = NT_STATUS_OK;
	
	switch (ctr->switch_value) {
	case NET_LOGON_TYPE:
	{
		const char *wksname = nt_workstation;
		
		if (!NT_STATUS_IS_OK(status = make_auth_context_fixed(&auth_context, ctr->auth.id2.lm_chal))) {
			return status;
		}

		/* For a network logon, the workstation name comes in with two
		 * backslashes in the front. Strip them if they are there. */

		if (*wksname == '\\') wksname++;
		if (*wksname == '\\') wksname++;

		/* Standard challenge/response authenticaion */
		if (!make_user_info_netlogon_network(&user_info, 
						     nt_username, nt_domain, 
						     wksname,
						     ctr->auth.id2.param_ctrl,
						     ctr->auth.id2.lm_chal_resp.buffer,
						     ctr->auth.id2.lm_chal_resp.str_str_len,
						     ctr->auth.id2.nt_chal_resp.buffer,
						     ctr->auth.id2.nt_chal_resp.str_str_len)) {
			status = NT_STATUS_NO_MEMORY;
		}	
		break;
	}
	case INTERACTIVE_LOGON_TYPE:
		/* 'Interactive' authentication, supplies the password in its
		   MD4 form, encrypted with the session key.  We will convert
		   this to challenge/response for the auth subsystem to chew
		   on */
	{
		const uint8 *chal;
		
		if (!NT_STATUS_IS_OK(status = make_auth_context_subsystem(&auth_context))) {
			return status;
		}
		
		chal = auth_context->get_ntlm_challenge(auth_context);

		if (!make_user_info_netlogon_interactive(&user_info, 
							 nt_username, nt_domain, 
							 nt_workstation, 
							 ctr->auth.id1.param_ctrl,
							 chal,
							 ctr->auth.id1.lm_owf.data, 
							 ctr->auth.id1.nt_owf.data, 
							 p->dc->sess_key)) {
			status = NT_STATUS_NO_MEMORY;
		}
		break;
	}
	default:
		DEBUG(2,("SAM Logon: unsupported switch value\n"));
		return NT_STATUS_INVALID_INFO_CLASS;
	} /* end switch */
	
	if ( NT_STATUS_IS_OK(status) ) {
		status = auth_context->check_ntlm_password(auth_context, 
			user_info, &server_info);
	}

	(auth_context->free)(&auth_context);	
	free_user_info(&user_info);
	
	DEBUG(5, ("_net_sam_logon: check_password returned status %s\n", 
		  nt_errstr(status)));

	/* Check account and password */
    
	if (!NT_STATUS_IS_OK(status)) {
		/* If we don't know what this domain is, we need to 
		   indicate that we are not authoritative.  This 
		   allows the client to decide if it needs to try 
		   a local user.  Fix by jpjanosi@us.ibm.com, #2976 */
                if ( NT_STATUS_EQUAL(status, NT_STATUS_NO_SUCH_USER) 
		     && !strequal(nt_domain, get_global_sam_name())
		     && !is_trusted_domain(nt_domain) )
			r_u->auth_resp = 0; /* We are not authoritative */

		TALLOC_FREE(server_info);
		return status;
	}

	if (server_info->guest) {
		/* We don't like guest domain logons... */
		DEBUG(5,("_net_sam_logon: Attempted domain logon as GUEST "
			 "denied.\n"));
		TALLOC_FREE(server_info);
		return NT_STATUS_LOGON_FAILURE;
	}

	/* This is the point at which, if the login was successful, that
           the SAM Local Security Authority should record that the user is
           logged in to the domain.  */
    
	{
		DOM_GID *gids = NULL;
		const DOM_SID *user_sid = NULL;
		const DOM_SID *group_sid = NULL;
		DOM_SID domain_sid;
		uint32 user_rid, group_rid; 

		int num_gids = 0;
		pstring my_name;
		fstring user_sid_string;
		fstring group_sid_string;
		unsigned char user_session_key[16];
		unsigned char lm_session_key[16];
		unsigned char pipe_session_key[16];

		sampw = server_info->sam_account;

		/* set up pointer indicating user/password failed to be
		 * found */
		usr_info->ptr_user_info = 0;

		user_sid = pdb_get_user_sid(sampw);
		group_sid = pdb_get_group_sid(sampw);

		if ((user_sid == NULL) || (group_sid == NULL)) {
			DEBUG(1, ("_net_sam_logon: User without group or user SID\n"));
			return NT_STATUS_UNSUCCESSFUL;
		}

		sid_copy(&domain_sid, user_sid);
		sid_split_rid(&domain_sid, &user_rid);

		if (!sid_peek_check_rid(&domain_sid, group_sid, &group_rid)) {
			DEBUG(1, ("_net_sam_logon: user %s\\%s has user sid "
				  "%s\n but group sid %s.\n"
				  "The conflicting domain portions are not "
				  "supported for NETLOGON calls\n", 	    
				  pdb_get_domain(sampw),
				  pdb_get_username(sampw),
				  sid_to_string(user_sid_string, user_sid),
				  sid_to_string(group_sid_string, group_sid)));
			return NT_STATUS_UNSUCCESSFUL;
		}
		
		
		if(server_info->login_server) {
		        pstrcpy(my_name, server_info->login_server);
		} else {
		        pstrcpy(my_name, global_myname());
		}

		status = nt_token_to_group_list(p->mem_ctx, &domain_sid,
						server_info->num_sids,
						server_info->sids,
						&num_gids, &gids);

		if (!NT_STATUS_IS_OK(status)) {
			return status;
		}

		if (server_info->user_session_key.length) {
			memcpy(user_session_key,
			       server_info->user_session_key.data, 
			       MIN(sizeof(user_session_key),
				   server_info->user_session_key.length));
			if (process_creds) {
				/* Get the pipe session key from the creds. */
				memcpy(pipe_session_key, p->dc->sess_key, 16);
			} else {
				/* Get the pipe session key from the schannel. */
				if (p->auth.auth_type != PIPE_AUTH_TYPE_SCHANNEL || p->auth.a_u.schannel_auth == NULL) {
					return NT_STATUS_INVALID_HANDLE;
				}
				memcpy(pipe_session_key, p->auth.a_u.schannel_auth->sess_key, 16);
			}
			SamOEMhash(user_session_key, pipe_session_key, 16);
			memset(pipe_session_key, '\0', 16);
		}
		if (server_info->lm_session_key.length) {
			memcpy(lm_session_key,
			       server_info->lm_session_key.data, 
			       MIN(sizeof(lm_session_key),
				   server_info->lm_session_key.length));
			if (process_creds) {
				/* Get the pipe session key from the creds. */
				memcpy(pipe_session_key, p->dc->sess_key, 16);
			} else {
				/* Get the pipe session key from the schannel. */
				if (p->auth.auth_type != PIPE_AUTH_TYPE_SCHANNEL || p->auth.a_u.schannel_auth == NULL) {
					return NT_STATUS_INVALID_HANDLE;
				}
				memcpy(pipe_session_key, p->auth.a_u.schannel_auth->sess_key, 16);
			}
			SamOEMhash(lm_session_key, pipe_session_key, 16);
			memset(pipe_session_key, '\0', 16);
		}
		
		init_net_user_info3(p->mem_ctx, usr_info, 
				    user_rid,
				    group_rid,   
				    pdb_get_username(sampw),
				    pdb_get_fullname(sampw),
				    pdb_get_homedir(sampw),
				    pdb_get_dir_drive(sampw),
				    pdb_get_logon_script(sampw),
				    pdb_get_profile_path(sampw),
				    pdb_get_logon_time(sampw),
				    get_time_t_max(),
				    get_time_t_max(),
				    pdb_get_pass_last_set_time(sampw),
				    pdb_get_pass_can_change_time(sampw),
				    pdb_get_pass_must_change_time(sampw),
				    0, /* logon_count */
				    0, /* bad_pw_count */
				    num_gids,    /* uint32 num_groups */
				    gids    , /* DOM_GID *gids */
				    LOGON_EXTRA_SIDS, /* uint32 user_flgs (?) */
				    pdb_get_acct_ctrl(sampw),
				    server_info->user_session_key.length ? user_session_key : NULL,
				    server_info->lm_session_key.length ? lm_session_key : NULL,
				    my_name     , /* char *logon_srv */
				    pdb_get_domain(sampw),
				    &domain_sid);     /* DOM_SID *dom_sid */  
		ZERO_STRUCT(user_session_key);
		ZERO_STRUCT(lm_session_key);
	}
	TALLOC_FREE(server_info);
	return status;
}

/*************************************************************************
 _net_sam_logon
 *************************************************************************/

NTSTATUS _net_sam_logon(pipes_struct *p, NET_Q_SAM_LOGON *q_u, NET_R_SAM_LOGON *r_u)
{
	return _net_sam_logon_internal(p, q_u, r_u, True);
}
 
/*************************************************************************
 _net_sam_logon_ex - no credential chaining. Map into net sam logon.
 *************************************************************************/

NTSTATUS _net_sam_logon_ex(pipes_struct *p, NET_Q_SAM_LOGON_EX *q_u, NET_R_SAM_LOGON_EX *r_u)
{
	NET_Q_SAM_LOGON q;
	NET_R_SAM_LOGON r;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Only allow this if the pipe is protected. */
	if (p->auth.auth_type != PIPE_AUTH_TYPE_SCHANNEL) {
		DEBUG(0,("_net_sam_logon_ex: client %s not using schannel for netlogon\n",
			get_remote_machine_name() ));
		return NT_STATUS_INVALID_PARAMETER;
        }

	/* Map a NET_Q_SAM_LOGON_EX to NET_Q_SAM_LOGON. */
	q.validation_level = q_u->validation_level;

 	/* Map a DOM_SAM_INFO_EX into a DOM_SAM_INFO with no creds. */
	q.sam_id.client.login = q_u->sam_id.client;
	q.sam_id.logon_level = q_u->sam_id.logon_level;
	q.sam_id.ctr = q_u->sam_id.ctr;

	r_u->status = _net_sam_logon_internal(p, &q, &r, False);

	if (!NT_STATUS_IS_OK(r_u->status)) {
		return r_u->status;
	}

	/* Map the NET_R_SAM_LOGON to NET_R_SAM_LOGON_EX. */
	r_u->switch_value = r.switch_value;
	r_u->user = r.user;
	r_u->auth_resp = r.auth_resp;
	r_u->flags = 0; /* FIXME ! */
	return r_u->status;
}

/*************************************************************************
 _ds_enum_dom_trusts
 *************************************************************************/
#if 0	/* JERRY -- not correct */
NTSTATUS _ds_enum_dom_trusts(pipes_struct *p, DS_Q_ENUM_DOM_TRUSTS *q_u,
			     DS_R_ENUM_DOM_TRUSTS *r_u)
{
	NTSTATUS status = NT_STATUS_OK;

	/* TODO: According to MSDN, the can only be executed against a 
	   DC or domain member running Windows 2000 or later.  Need
	   to test against a standalone 2k server and see what it 
	   does.  A windows 2000 DC includes its own domain in the 
	   list.  --jerry */

	return status;
}
#endif	/* JERRY */
