/* 
   Unix SMB/CIFS implementation.
   NT Domain Authentication SMB / MSRPC client
   Copyright (C) Andrew Tridgell 1992-2000
   Copyright (C) Jeremy Allison                    1998.
   Largely re-written by Jeremy Allison (C)	   2005.

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

/* LSA Request Challenge. Sends our challenge to server, then gets
   server response. These are used to generate the credentials.
 The sent and received challenges are stored in the netlog pipe
 private data. Only call this via rpccli_netlogon_setup_creds(). JRA.
*/

static NTSTATUS rpccli_net_req_chal(struct rpc_pipe_client *cli,
				TALLOC_CTX *mem_ctx,
				const char *server_name,
				const char *clnt_name,
				const DOM_CHAL *clnt_chal_in,
				DOM_CHAL *srv_chal_out)
{
	prs_struct qbuf, rbuf;
	NET_Q_REQ_CHAL q;
	NET_R_REQ_CHAL r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	/* create and send a MSRPC command with api NET_REQCHAL */

	DEBUG(4,("cli_net_req_chal: LSA Request Challenge from %s to %s\n",
		clnt_name, server_name));
        
	/* store the parameters */
	init_q_req_chal(&q, server_name, clnt_name, clnt_chal_in);

	/* Marshall data and send request */
	CLI_DO_RPC(cli, mem_ctx, PI_NETLOGON, NET_REQCHAL,
		q, r,
		qbuf, rbuf,
		net_io_q_req_chal,
		net_io_r_req_chal,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;

	/* Return result */

	if (NT_STATUS_IS_OK(result)) {
		/* Store the returned server challenge. */
		*srv_chal_out = r.srv_chal;
	}

	return result;
}

#if 0
/****************************************************************************
LSA Authenticate 2

Send the client credential, receive back a server credential.
Ensure that the server credential returned matches the session key 
encrypt of the server challenge originally received. JRA.
****************************************************************************/

  NTSTATUS rpccli_net_auth2(struct rpc_pipe_client *cli, 
		       uint16 sec_chan, 
		       uint32 *neg_flags, DOM_CHAL *srv_chal)
{
        prs_struct qbuf, rbuf;
        NET_Q_AUTH_2 q;
        NET_R_AUTH_2 r;
        NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	fstring machine_acct;

	if ( sec_chan == SEC_CHAN_DOMAIN )
		fstr_sprintf( machine_acct, "%s$", lp_workgroup() );
	else
		fstrcpy( machine_acct, cli->mach_acct );
	
        /* create and send a MSRPC command with api NET_AUTH2 */

        DEBUG(4,("cli_net_auth2: srv:%s acct:%s sc:%x mc: %s chal %s neg: %x\n",
                 cli->srv_name_slash, machine_acct, sec_chan, global_myname(),
                 credstr(cli->clnt_cred.challenge.data), *neg_flags));

        /* store the parameters */

        init_q_auth_2(&q, cli->srv_name_slash, machine_acct, 
                      sec_chan, global_myname(), &cli->clnt_cred.challenge, 
                      *neg_flags);

        /* turn parameters into data stream */

	CLI_DO_RPC(cli, mem_ctx, PI_NETLOGON, NET_AUTH2,
		q, r,
		qbuf, rbuf,
		net_io_q_auth_2,
		net_io_r_auth_2,
		NT_STATUS_UNSUCCESSFUL);

        result = r.status;

        if (NT_STATUS_IS_OK(result)) {
                UTIME zerotime;
                
                /*
                 * Check the returned value using the initial
                 * server received challenge.
                 */

                zerotime.time = 0;
                if (cred_assert( &r.srv_chal, cli->sess_key, srv_chal, zerotime) == 0) {

                        /*
                         * Server replied with bad credential. Fail.
                         */
                        DEBUG(0,("cli_net_auth2: server %s replied with bad credential (bad machine \
password ?).\n", cli->cli->desthost ));
			return NT_STATUS_ACCESS_DENIED;
                }
		*neg_flags = r.srv_flgs.neg_flags;
        }

        return result;
}
#endif

/****************************************************************************
 LSA Authenticate 2

 Send the client credential, receive back a server credential.
 The caller *must* ensure that the server credential returned matches the session key 
 encrypt of the server challenge originally received. JRA.
****************************************************************************/

static NTSTATUS rpccli_net_auth2(struct rpc_pipe_client *cli,
			TALLOC_CTX *mem_ctx,
			const char *server_name,
			const char *account_name,
			uint16 sec_chan_type,
			const char *computer_name,
			uint32 *neg_flags_inout,
			const DOM_CHAL *clnt_chal_in,
			DOM_CHAL *srv_chal_out)
{
        prs_struct qbuf, rbuf;
        NET_Q_AUTH_2 q;
        NET_R_AUTH_2 r;
        NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

        /* create and send a MSRPC command with api NET_AUTH2 */

        DEBUG(4,("cli_net_auth2: srv:%s acct:%s sc:%x mc: %s neg: %x\n",
                 server_name, account_name, sec_chan_type, computer_name,
                 *neg_flags_inout));

        /* store the parameters */

        init_q_auth_2(&q, server_name, account_name, sec_chan_type,
		      computer_name, clnt_chal_in, *neg_flags_inout);

        /* turn parameters into data stream */

	CLI_DO_RPC(cli, mem_ctx, PI_NETLOGON, NET_AUTH2,
		q, r,
		qbuf, rbuf,
		net_io_q_auth_2,
		net_io_r_auth_2,
		NT_STATUS_UNSUCCESSFUL);

        result = r.status;

        if (NT_STATUS_IS_OK(result)) {
		*srv_chal_out = r.srv_chal;
		*neg_flags_inout = r.srv_flgs.neg_flags;
        }

        return result;
}

#if 0	/* not currebntly used */
/****************************************************************************
 LSA Authenticate 3

 Send the client credential, receive back a server credential.
 The caller *must* ensure that the server credential returned matches the session key 
 encrypt of the server challenge originally received. JRA.
****************************************************************************/

static NTSTATUS rpccli_net_auth3(struct rpc_pipe_client *cli, 
			TALLOC_CTX *mem_ctx,
			const char *server_name,
			const char *account_name,
			uint16 sec_chan_type,
			const char *computer_name,
			uint32 *neg_flags_inout,
			const DOM_CHAL *clnt_chal_in,
			DOM_CHAL *srv_chal_out)
{
        prs_struct qbuf, rbuf;
        NET_Q_AUTH_3 q;
        NET_R_AUTH_3 r;
        NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

        /* create and send a MSRPC command with api NET_AUTH2 */

        DEBUG(4,("cli_net_auth3: srv:%s acct:%s sc:%x mc: %s chal %s neg: %x\n",
		server_name, account_name, sec_chan_type, computer_name,
		credstr(clnt_chal_in->data), *neg_flags_inout));

        /* store the parameters */
        init_q_auth_3(&q, server_name, account_name, sec_chan_type,
			computer_name, clnt_chal_in, *neg_flags_inout);

        /* turn parameters into data stream */

	CLI_DO_RPC(cli, mem_ctx, PI_NETLOGON, NET_AUTH3,
		q, r,
		qbuf, rbuf,
		net_io_q_auth_3,
		net_io_r_auth_3,
		NT_STATUS_UNSUCCESSFUL);

        if (NT_STATUS_IS_OK(result)) {
		*srv_chal_out = r.srv_chal;
		*neg_flags_inout = r.srv_flgs.neg_flags;
        }

        return result;
}
#endif 	/* not currebntly used */

/****************************************************************************
 Wrapper function that uses the auth and auth2 calls to set up a NETLOGON
 credentials chain. Stores the credentials in the struct dcinfo in the
 netlogon pipe struct.
****************************************************************************/

NTSTATUS rpccli_netlogon_setup_creds(struct rpc_pipe_client *cli,
				const char *server_name,
				const char *domain,
				const char *clnt_name,
				const char *machine_account,
				const unsigned char machine_pwd[16],
				uint32 sec_chan_type,
				uint32 *neg_flags_inout)
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	DOM_CHAL clnt_chal_send;
	DOM_CHAL srv_chal_recv;
	struct dcinfo *dc;

	SMB_ASSERT(cli->pipe_idx == PI_NETLOGON);

	dc = cli->dc;
	if (!dc) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	/* Ensure we don't reuse any of this state. */
	ZERO_STRUCTP(dc);

	/* Store the machine account password we're going to use. */
	memcpy(dc->mach_pw, machine_pwd, 16);

	fstrcpy(dc->remote_machine, "\\\\");
	fstrcat(dc->remote_machine, server_name);

	fstrcpy(dc->domain, domain);

	fstr_sprintf( dc->mach_acct, "%s$", machine_account);

	/* Create the client challenge. */
	generate_random_buffer(clnt_chal_send.data, 8);

	/* Get the server challenge. */
	result = rpccli_net_req_chal(cli,
				cli->mem_ctx,
				dc->remote_machine,
				clnt_name,
				&clnt_chal_send,
				&srv_chal_recv);

	if (!NT_STATUS_IS_OK(result)) {
		return result;
	}

	/* Calculate the session key and client credentials */
	creds_client_init(*neg_flags_inout,
			dc,
			&clnt_chal_send,
			&srv_chal_recv,
			machine_pwd,
			&clnt_chal_send);

        /*  
         * Send client auth-2 challenge and receive server repy.
         */

	result = rpccli_net_auth2(cli,
			cli->mem_ctx,
			dc->remote_machine,
			dc->mach_acct,
			sec_chan_type,
			clnt_name,
			neg_flags_inout,
			&clnt_chal_send, /* input. */
			&srv_chal_recv); /* output */

	if (!NT_STATUS_IS_OK(result)) {
		return result;
	}

	/*
	 * Check the returned value using the initial
	 * server received challenge.
	 */

	if (!creds_client_check(dc, &srv_chal_recv)) {
		/*
		 * Server replied with bad credential. Fail.
		 */
		DEBUG(0,("rpccli_netlogon_setup_creds: server %s "
			"replied with bad credential\n",
			cli->cli->desthost ));
		return NT_STATUS_ACCESS_DENIED;
	}

	DEBUG(5,("rpccli_netlogon_setup_creds: server %s credential "
		"chain established.\n",
		cli->cli->desthost ));

	return NT_STATUS_OK;
}

/* Logon Control 2 */

NTSTATUS rpccli_netlogon_logon_ctrl2(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                                  uint32 query_level)
{
	prs_struct qbuf, rbuf;
	NET_Q_LOGON_CTRL2 q;
	NET_R_LOGON_CTRL2 r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	fstring server;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Initialise input parameters */

	slprintf(server, sizeof(fstring)-1, "\\\\%s", cli->cli->desthost);
	init_net_q_logon_ctrl2(&q, server, query_level);

	/* Marshall data and send request */

	CLI_DO_RPC(cli, mem_ctx, PI_NETLOGON, NET_LOGON_CTRL2,
		q, r,
		qbuf, rbuf,
		net_io_q_logon_ctrl2,
		net_io_r_logon_ctrl2,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;
	return result;
}

/* GetDCName */

WERROR rpccli_netlogon_getdcname(struct rpc_pipe_client *cli,
				 TALLOC_CTX *mem_ctx, const char *mydcname,
				 const char *domainname, fstring newdcname)
{
	prs_struct qbuf, rbuf;
	NET_Q_GETDCNAME q;
	NET_R_GETDCNAME r;
	WERROR result;
	fstring mydcname_slash;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Initialise input parameters */

	slprintf(mydcname_slash, sizeof(fstring)-1, "\\\\%s", mydcname);
	init_net_q_getdcname(&q, mydcname_slash, domainname);

	/* Marshall data and send request */

	CLI_DO_RPC_WERR(cli, mem_ctx, PI_NETLOGON, NET_GETDCNAME,
		q, r,
		qbuf, rbuf,
		net_io_q_getdcname,
		net_io_r_getdcname,
		WERR_GENERAL_FAILURE);

	result = r.status;

	if (W_ERROR_IS_OK(result)) {
		rpcstr_pull_unistr2_fstring(newdcname, &r.uni_dcname);
	}

	return result;
}

/* Dsr_GetDCName */

WERROR rpccli_netlogon_dsr_getdcname(struct rpc_pipe_client *cli,
				     TALLOC_CTX *mem_ctx,
				     const char *server_name,
				     const char *domain_name,
				     struct uuid *domain_guid,
				     struct uuid *site_guid,
				     uint32_t flags,
				     char **dc_unc, char **dc_address,
				     int32 *dc_address_type,
				     struct uuid *domain_guid_out,
				     char **domain_name_out,
				     char **forest_name,
				     uint32 *dc_flags,
				     char **dc_site_name,
				     char **client_site_name)
{
	prs_struct qbuf, rbuf;
	NET_Q_DSR_GETDCNAME q;
	NET_R_DSR_GETDCNAME r;
	char *tmp_str;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Initialize input parameters */

	tmp_str = talloc_asprintf(mem_ctx, "\\\\%s", server_name);
	if (tmp_str == NULL) {
		return WERR_NOMEM;
	}

	init_net_q_dsr_getdcname(&q, tmp_str, domain_name, domain_guid,
				 site_guid, flags);

	/* Marshall data and send request */

	CLI_DO_RPC_WERR(cli, mem_ctx, PI_NETLOGON, NET_DSR_GETDCNAME,
			q, r,
			qbuf, rbuf,
			net_io_q_dsr_getdcname,
			net_io_r_dsr_getdcname,
			WERR_GENERAL_FAILURE);

	if (!W_ERROR_IS_OK(r.result)) {
		return r.result;
	}

	if (dc_unc != NULL) {
		char *tmp;
		tmp = rpcstr_pull_unistr2_talloc(mem_ctx, &r.uni_dc_unc);
		if (tmp == NULL) {
			return WERR_GENERAL_FAILURE;
		}
		if (*tmp == '\\') tmp += 1;
		if (*tmp == '\\') tmp += 1;

		/* We have to talloc_strdup, otherwise a talloc_steal would
		   fail */
		*dc_unc = talloc_strdup(mem_ctx, tmp);
		if (*dc_unc == NULL) {
			return WERR_NOMEM;
		}
	}

	if (dc_address != NULL) {
		char *tmp;
		tmp = rpcstr_pull_unistr2_talloc(mem_ctx, &r.uni_dc_address);
		if (tmp == NULL) {
			return WERR_GENERAL_FAILURE;
		}
		if (*tmp == '\\') tmp += 1;
		if (*tmp == '\\') tmp += 1;

		/* We have to talloc_strdup, otherwise a talloc_steal would
		   fail */
		*dc_address = talloc_strdup(mem_ctx, tmp);
		if (*dc_address == NULL) {
			return WERR_NOMEM;
		}
	}

	if (dc_address_type != NULL) {
		*dc_address_type = r.dc_address_type;
	}

	if (domain_guid_out != NULL) {
		*domain_guid_out = r.domain_guid;
	}

	if ((domain_name_out != NULL) &&
	    ((*domain_name_out = rpcstr_pull_unistr2_talloc(
		    mem_ctx, &r.uni_domain_name)) == NULL)) {
		return WERR_GENERAL_FAILURE;
	}

	if ((forest_name != NULL) &&
	    ((*forest_name = rpcstr_pull_unistr2_talloc(
		      mem_ctx, &r.uni_forest_name)) == NULL)) {
		return WERR_GENERAL_FAILURE;
	}

	if (dc_flags != NULL) {
		*dc_flags = r.dc_flags;
	}

	if ((dc_site_name != NULL) &&
	    ((*dc_site_name = rpcstr_pull_unistr2_talloc(
		      mem_ctx, &r.uni_dc_site_name)) == NULL)) {
		return WERR_GENERAL_FAILURE;
	}

	if ((client_site_name != NULL) &&
	    ((*client_site_name = rpcstr_pull_unistr2_talloc(
		      mem_ctx, &r.uni_client_site_name)) == NULL)) {
		return WERR_GENERAL_FAILURE;
	}

	return WERR_OK;
}

/* Dsr_GetSiteName */

WERROR rpccli_netlogon_dsr_getsitename(struct rpc_pipe_client *cli,
				       TALLOC_CTX *mem_ctx,
				       const char *computer_name,
				       char **site_name)
{
	prs_struct qbuf, rbuf;
	NET_Q_DSR_GETSITENAME q;
	NET_R_DSR_GETSITENAME r;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Initialize input parameters */

	init_net_q_dsr_getsitename(&q, computer_name);

	/* Marshall data and send request */

	CLI_DO_RPC_WERR(cli, mem_ctx, PI_NETLOGON, NET_DSR_GETSITENAME,
			q, r,
			qbuf, rbuf,
			net_io_q_dsr_getsitename,
			net_io_r_dsr_getsitename,
			WERR_GENERAL_FAILURE);

	if (!W_ERROR_IS_OK(r.result)) {
		return r.result;
	}

	if ((site_name != NULL) &&
	    ((*site_name = rpcstr_pull_unistr2_talloc(
		      mem_ctx, &r.uni_site_name)) == NULL)) {
		return WERR_GENERAL_FAILURE;
	}

	return WERR_OK;
}



/* Sam synchronisation */

NTSTATUS rpccli_netlogon_sam_sync(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                               uint32 database_id, uint32 next_rid, uint32 *num_deltas,
                               SAM_DELTA_HDR **hdr_deltas, 
                               SAM_DELTA_CTR **deltas)
{
	prs_struct qbuf, rbuf;
	NET_Q_SAM_SYNC q;
	NET_R_SAM_SYNC r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
        DOM_CRED clnt_creds;
        DOM_CRED ret_creds;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	ZERO_STRUCT(ret_creds);

	/* Initialise input parameters */

	creds_client_step(cli->dc, &clnt_creds);

	init_net_q_sam_sync(&q, cli->dc->remote_machine, global_myname(),
                            &clnt_creds, &ret_creds, database_id, next_rid);

	/* Marshall data and send request */

	CLI_DO_RPC_COPY_SESS_KEY(cli, mem_ctx, PI_NETLOGON, NET_SAM_SYNC,
		q, r,
		qbuf, rbuf,
		net_io_q_sam_sync,
		net_io_r_sam_sync,
		NT_STATUS_UNSUCCESSFUL);

        /* Return results */

	result = r.status;
        *num_deltas = r.num_deltas2;
        *hdr_deltas = r.hdr_deltas;
        *deltas = r.deltas;

	if (!NT_STATUS_IS_ERR(result)) {
		/* Check returned credentials. */
		if (!creds_client_check(cli->dc, &r.srv_creds.challenge)) {
			DEBUG(0,("cli_netlogon_sam_sync: credentials chain check failed\n"));
			return NT_STATUS_ACCESS_DENIED;
		}
	}

	return result;
}

/* Sam synchronisation */

NTSTATUS rpccli_netlogon_sam_deltas(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                                 uint32 database_id, UINT64_S seqnum,
                                 uint32 *num_deltas, 
                                 SAM_DELTA_HDR **hdr_deltas, 
                                 SAM_DELTA_CTR **deltas)
{
	prs_struct qbuf, rbuf;
	NET_Q_SAM_DELTAS q;
	NET_R_SAM_DELTAS r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
        DOM_CRED clnt_creds;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Initialise input parameters */

	creds_client_step(cli->dc, &clnt_creds);

	init_net_q_sam_deltas(&q, cli->dc->remote_machine,
                              global_myname(), &clnt_creds, 
                              database_id, seqnum);

	/* Marshall data and send request */

	CLI_DO_RPC(cli, mem_ctx, PI_NETLOGON, NET_SAM_DELTAS,
		q, r,
		qbuf, rbuf,
		net_io_q_sam_deltas,
		net_io_r_sam_deltas,
		NT_STATUS_UNSUCCESSFUL);

        /* Return results */

	result = r.status;
        *num_deltas = r.num_deltas2;
        *hdr_deltas = r.hdr_deltas;
        *deltas = r.deltas;

	if (!NT_STATUS_IS_ERR(result)) {
		/* Check returned credentials. */
		if (!creds_client_check(cli->dc, &r.srv_creds.challenge)) {
			DEBUG(0,("cli_netlogon_sam_sync: credentials chain check failed\n"));
			return NT_STATUS_ACCESS_DENIED;
		}
	}

	return result;
}

/* Logon domain user */

NTSTATUS rpccli_netlogon_sam_logon(struct rpc_pipe_client *cli,
				   TALLOC_CTX *mem_ctx,
				   uint32 logon_parameters,
				   const char *domain,
				   const char *username,
				   const char *password,
				   int logon_type)
{
	prs_struct qbuf, rbuf;
	NET_Q_SAM_LOGON q;
	NET_R_SAM_LOGON r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	DOM_CRED clnt_creds;
	DOM_CRED ret_creds;
        NET_ID_INFO_CTR ctr;
        NET_USER_INFO_3 user;
        int validation_level = 3;
	fstring clnt_name_slash;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);
	ZERO_STRUCT(ret_creds);

	fstr_sprintf( clnt_name_slash, "\\\\%s", global_myname() );

        /* Initialise input parameters */

	creds_client_step(cli->dc, &clnt_creds);

        q.validation_level = validation_level;

        ctr.switch_value = logon_type;

        switch (logon_type) {
        case INTERACTIVE_LOGON_TYPE: {
                unsigned char lm_owf_user_pwd[16], nt_owf_user_pwd[16];

                nt_lm_owf_gen(password, nt_owf_user_pwd, lm_owf_user_pwd);

                init_id_info1(&ctr.auth.id1, domain, 
			      logon_parameters, /* param_ctrl */
                              0xdead, 0xbeef, /* LUID? */
                              username, clnt_name_slash,
                              (const char *)cli->dc->sess_key, lm_owf_user_pwd,
                              nt_owf_user_pwd);

                break;
        }
        case NET_LOGON_TYPE: {
                uint8 chal[8];
                unsigned char local_lm_response[24];
                unsigned char local_nt_response[24];

                generate_random_buffer(chal, 8);

                SMBencrypt(password, chal, local_lm_response);
                SMBNTencrypt(password, chal, local_nt_response);

                init_id_info2(&ctr.auth.id2, domain, 
			      logon_parameters, /* param_ctrl */
                              0xdead, 0xbeef, /* LUID? */
                              username, clnt_name_slash, chal,
                              local_lm_response, 24, local_nt_response, 24);
                break;
        }
        default:
                DEBUG(0, ("switch value %d not supported\n", 
                          ctr.switch_value));
                return NT_STATUS_INVALID_INFO_CLASS;
        }

        r.user = &user;

        init_sam_info(&q.sam_id, cli->dc->remote_machine, global_myname(),
                      &clnt_creds, &ret_creds, logon_type,
                      &ctr);

        /* Marshall data and send request */

	CLI_DO_RPC(cli, mem_ctx, PI_NETLOGON, NET_SAMLOGON,
		q, r,
		qbuf, rbuf,
		net_io_q_sam_logon,
		net_io_r_sam_logon,
		NT_STATUS_UNSUCCESSFUL);

        /* Return results */

	result = r.status;

	if (r.buffer_creds) {
		/* Check returned credentials if present. */
		if (!creds_client_check(cli->dc, &r.srv_creds.challenge)) {
			DEBUG(0,("rpccli_netlogon_sam_logon: credentials chain check failed\n"));
			return NT_STATUS_ACCESS_DENIED;
		}
	}

        return result;
}


/** 
 * Logon domain user with an 'network' SAM logon 
 *
 * @param info3 Pointer to a NET_USER_INFO_3 already allocated by the caller.
 **/

NTSTATUS rpccli_netlogon_sam_network_logon(struct rpc_pipe_client *cli,
					   TALLOC_CTX *mem_ctx,
					   uint32 logon_parameters,
					   const char *server,
					   const char *username,
					   const char *domain,
					   const char *workstation, 
					   const uint8 chal[8], 
					   DATA_BLOB lm_response,
					   DATA_BLOB nt_response,
					   NET_USER_INFO_3 *info3)
{
	prs_struct qbuf, rbuf;
	NET_Q_SAM_LOGON q;
	NET_R_SAM_LOGON r;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	NET_ID_INFO_CTR ctr;
	int validation_level = 3;
	const char *workstation_name_slash;
	const char *server_name_slash;
	static uint8 zeros[16];
	DOM_CRED clnt_creds;
	DOM_CRED ret_creds;
	int i;
	
	ZERO_STRUCT(q);
	ZERO_STRUCT(r);
	ZERO_STRUCT(ret_creds);

	creds_client_step(cli->dc, &clnt_creds);

	if (server[0] != '\\' && server[1] != '\\') {
		server_name_slash = talloc_asprintf(mem_ctx, "\\\\%s", server);
	} else {
		server_name_slash = server;
	}

	if (workstation[0] != '\\' && workstation[1] != '\\') {
		workstation_name_slash = talloc_asprintf(mem_ctx, "\\\\%s", workstation);
	} else {
		workstation_name_slash = workstation;
	}

	if (!workstation_name_slash || !server_name_slash) {
		DEBUG(0, ("talloc_asprintf failed!\n"));
		return NT_STATUS_NO_MEMORY;
	}

	/* Initialise input parameters */

	q.validation_level = validation_level;

        ctr.switch_value = NET_LOGON_TYPE;

	init_id_info2(&ctr.auth.id2, domain,
		      logon_parameters, /* param_ctrl */
		      0xdead, 0xbeef, /* LUID? */
		      username, workstation_name_slash, (const uchar*)chal,
		      lm_response.data, lm_response.length, nt_response.data, nt_response.length);
 
        init_sam_info(&q.sam_id, server_name_slash, global_myname(),
                      &clnt_creds, &ret_creds, NET_LOGON_TYPE,
                      &ctr);

        r.user = info3;

        /* Marshall data and send request */

	CLI_DO_RPC(cli, mem_ctx, PI_NETLOGON, NET_SAMLOGON,
		q, r,
		qbuf, rbuf,
		net_io_q_sam_logon,
		net_io_r_sam_logon,
		NT_STATUS_UNSUCCESSFUL);

	if (memcmp(zeros, info3->user_sess_key, 16) != 0) {
		SamOEMhash(info3->user_sess_key, cli->dc->sess_key, 16);
	} else {
		memset(info3->user_sess_key, '\0', 16);
	}

	if (memcmp(zeros, info3->lm_sess_key, 8) != 0) {
		SamOEMhash(info3->lm_sess_key, cli->dc->sess_key, 8);
	} else {
		memset(info3->lm_sess_key, '\0', 8);
	}

	for (i=0; i < 7; i++) {
		memset(&info3->unknown[i], '\0', 4);
	}

        /* Return results */

	result = r.status;

	if (r.buffer_creds) {
		/* Check returned credentials if present. */
		if (!creds_client_check(cli->dc, &r.srv_creds.challenge)) {
			DEBUG(0,("rpccli_netlogon_sam_network_logon: credentials chain check failed\n"));
			return NT_STATUS_ACCESS_DENIED;
		}
	}

        return result;
}

/***************************************************************************
LSA Server Password Set.
****************************************************************************/

NTSTATUS rpccli_net_srv_pwset(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, 
			   const char *machine_name, const uint8 hashed_mach_pwd[16])
{
	prs_struct rbuf;
	prs_struct qbuf; 
	DOM_CRED clnt_creds;
	NET_Q_SRV_PWSET q;
	NET_R_SRV_PWSET r;
	uint16 sec_chan_type = 2;
	NTSTATUS result;

	creds_client_step(cli->dc, &clnt_creds);
	
	DEBUG(4,("cli_net_srv_pwset: srv:%s acct:%s sc: %d mc: %s\n",
		 cli->dc->remote_machine, cli->dc->mach_acct, sec_chan_type, machine_name));
	
        /* store the parameters */
	init_q_srv_pwset(&q, cli->dc->remote_machine, (const char *)cli->dc->sess_key,
			 cli->dc->mach_acct, sec_chan_type, machine_name, 
			 &clnt_creds, hashed_mach_pwd);
	
	CLI_DO_RPC(cli, mem_ctx, PI_NETLOGON, NET_SRVPWSET,
		q, r,
		qbuf, rbuf,
		net_io_q_srv_pwset,
		net_io_r_srv_pwset,
		NT_STATUS_UNSUCCESSFUL);

	result = r.status;

	if (!NT_STATUS_IS_OK(result)) {
		/* report error code */
		DEBUG(0,("cli_net_srv_pwset: %s\n", nt_errstr(result)));
	}

	/* Always check returned credentials. */
	if (!creds_client_check(cli->dc, &r.srv_cred.challenge)) {
		DEBUG(0,("rpccli_net_srv_pwset: credentials chain check failed\n"));
		return NT_STATUS_ACCESS_DENIED;
	}

	return result;
}
