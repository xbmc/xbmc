/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Almost completely rewritten by (C) Jeremy Allison 2005.
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

/*  this module apparently provides an implementation of DCE/RPC over a
 *  named pipe (IPC$ connection using SMBtrans).  details of DCE/RPC
 *  documentation are available (in on-line form) from the X-Open group.
 *
 *  this module should provide a level of abstraction between SMB
 *  and DCE/RPC, while minimising the amount of mallocs, unnecessary
 *  data copies, and network traffic.
 *
 */

#include "includes.h"

extern struct pipe_id_info pipe_names[];
extern struct current_user current_user;

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

static void free_pipe_ntlmssp_auth_data(struct pipe_auth_data *auth)
{
	AUTH_NTLMSSP_STATE *a = auth->a_u.auth_ntlmssp_state;

	if (a) {
		auth_ntlmssp_end(&a);
	}
	auth->a_u.auth_ntlmssp_state = NULL;
}

/*******************************************************************
 Generate the next PDU to be returned from the data in p->rdata. 
 Handle NTLMSSP.
 ********************************************************************/

static BOOL create_next_pdu_ntlmssp(pipes_struct *p)
{
	RPC_HDR_RESP hdr_resp;
	uint32 ss_padding_len = 0;
	uint32 data_space_available;
	uint32 data_len_left;
	uint32 data_len;
	prs_struct outgoing_pdu;
	NTSTATUS status;
	DATA_BLOB auth_blob;
	RPC_HDR_AUTH auth_info;
	uint8 auth_type, auth_level;
	AUTH_NTLMSSP_STATE *a = p->auth.a_u.auth_ntlmssp_state;

	/*
	 * If we're in the fault state, keep returning fault PDU's until
	 * the pipe gets closed. JRA.
	 */

	if(p->fault_state) {
		setup_fault_pdu(p, NT_STATUS(DCERPC_FAULT_OP_RNG_ERROR));
		return True;
	}

	memset((char *)&hdr_resp, '\0', sizeof(hdr_resp));

	/* Change the incoming request header to a response. */
	p->hdr.pkt_type = RPC_RESPONSE;

	/* Set up rpc header flags. */
	if (p->out_data.data_sent_length == 0) {
		p->hdr.flags = RPC_FLG_FIRST;
	} else {
		p->hdr.flags = 0;
	}

	/*
	 * Work out how much we can fit in a single PDU.
	 */

	data_len_left = prs_offset(&p->out_data.rdata) - p->out_data.data_sent_length;

	/*
	 * Ensure there really is data left to send.
	 */

	if(!data_len_left) {
		DEBUG(0,("create_next_pdu_ntlmssp: no data left to send !\n"));
		return False;
	}

	data_space_available = sizeof(p->out_data.current_pdu) - RPC_HEADER_LEN - RPC_HDR_RESP_LEN -
					RPC_HDR_AUTH_LEN - NTLMSSP_SIG_SIZE;

	/*
	 * The amount we send is the minimum of the available
	 * space and the amount left to send.
	 */

	data_len = MIN(data_len_left, data_space_available);

	/*
	 * Set up the alloc hint. This should be the data left to
	 * send.
	 */

	hdr_resp.alloc_hint = data_len_left;

	/*
	 * Work out if this PDU will be the last.
	 */

	if(p->out_data.data_sent_length + data_len >= prs_offset(&p->out_data.rdata)) {
		p->hdr.flags |= RPC_FLG_LAST;
		if (data_len_left % 8) {
			ss_padding_len = 8 - (data_len_left % 8);
			DEBUG(10,("create_next_pdu_ntlmssp: adding sign/seal padding of %u\n",
				ss_padding_len ));
		}
	}

	/*
	 * Set up the header lengths.
	 */

	p->hdr.frag_len = RPC_HEADER_LEN + RPC_HDR_RESP_LEN +
			data_len + ss_padding_len +
			RPC_HDR_AUTH_LEN + NTLMSSP_SIG_SIZE;
	p->hdr.auth_len = NTLMSSP_SIG_SIZE;


	/*
	 * Init the parse struct to point at the outgoing
	 * data.
	 */

	prs_init( &outgoing_pdu, 0, p->mem_ctx, MARSHALL);
	prs_give_memory( &outgoing_pdu, (char *)p->out_data.current_pdu, sizeof(p->out_data.current_pdu), False);

	/* Store the header in the data stream. */
	if(!smb_io_rpc_hdr("hdr", &p->hdr, &outgoing_pdu, 0)) {
		DEBUG(0,("create_next_pdu_ntlmssp: failed to marshall RPC_HDR.\n"));
		prs_mem_free(&outgoing_pdu);
		return False;
	}

	if(!smb_io_rpc_hdr_resp("resp", &hdr_resp, &outgoing_pdu, 0)) {
		DEBUG(0,("create_next_pdu_ntlmssp: failed to marshall RPC_HDR_RESP.\n"));
		prs_mem_free(&outgoing_pdu);
		return False;
	}

	/* Copy the data into the PDU. */

	if(!prs_append_some_prs_data(&outgoing_pdu, &p->out_data.rdata, p->out_data.data_sent_length, data_len)) {
		DEBUG(0,("create_next_pdu_ntlmssp: failed to copy %u bytes of data.\n", (unsigned int)data_len));
		prs_mem_free(&outgoing_pdu);
		return False;
	}

	/* Copy the sign/seal padding data. */
	if (ss_padding_len) {
		char pad[8];

		memset(pad, '\0', 8);
		if (!prs_copy_data_in(&outgoing_pdu, pad, ss_padding_len)) {
			DEBUG(0,("create_next_pdu_ntlmssp: failed to add %u bytes of pad data.\n",
					(unsigned int)ss_padding_len));
			prs_mem_free(&outgoing_pdu);
			return False;
		}
	}


	/* Now write out the auth header and null blob. */
	if (p->auth.auth_type == PIPE_AUTH_TYPE_NTLMSSP) {
		auth_type = RPC_NTLMSSP_AUTH_TYPE;
	} else {
		auth_type = RPC_SPNEGO_AUTH_TYPE;
	}
	if (p->auth.auth_level == PIPE_AUTH_LEVEL_PRIVACY) {
		auth_level = RPC_AUTH_LEVEL_PRIVACY;
	} else {
		auth_level = RPC_AUTH_LEVEL_INTEGRITY;
	}

	init_rpc_hdr_auth(&auth_info, auth_type, auth_level, ss_padding_len, 1 /* context id. */);
	if(!smb_io_rpc_hdr_auth("hdr_auth", &auth_info, &outgoing_pdu, 0)) {
		DEBUG(0,("create_next_pdu_ntlmssp: failed to marshall RPC_HDR_AUTH.\n"));
		prs_mem_free(&outgoing_pdu);
		return False;
	}

	/* Generate the sign blob. */

	switch (p->auth.auth_level) {
		case PIPE_AUTH_LEVEL_PRIVACY:
			/* Data portion is encrypted. */
			status = ntlmssp_seal_packet(a->ntlmssp_state,
							(unsigned char *)prs_data_p(&outgoing_pdu) + RPC_HEADER_LEN + RPC_HDR_RESP_LEN,
							data_len + ss_padding_len,
							(unsigned char *)prs_data_p(&outgoing_pdu),
							(size_t)prs_offset(&outgoing_pdu),
							&auth_blob);
			if (!NT_STATUS_IS_OK(status)) {
				data_blob_free(&auth_blob);
				prs_mem_free(&outgoing_pdu);
				return False;
			}
			break;
		case PIPE_AUTH_LEVEL_INTEGRITY:
			/* Data is signed. */
			status = ntlmssp_sign_packet(a->ntlmssp_state,
							(unsigned char *)prs_data_p(&outgoing_pdu) + RPC_HEADER_LEN + RPC_HDR_RESP_LEN,
							data_len + ss_padding_len,
							(unsigned char *)prs_data_p(&outgoing_pdu),
							(size_t)prs_offset(&outgoing_pdu),
							&auth_blob);
			if (!NT_STATUS_IS_OK(status)) {
				data_blob_free(&auth_blob);
				prs_mem_free(&outgoing_pdu);
				return False;
			}
			break;
		default:
			prs_mem_free(&outgoing_pdu);
			return False;
	}

	/* Append the auth blob. */
	if (!prs_copy_data_in(&outgoing_pdu, (char *)auth_blob.data, NTLMSSP_SIG_SIZE)) {
		DEBUG(0,("create_next_pdu_ntlmssp: failed to add %u bytes auth blob.\n",
				(unsigned int)NTLMSSP_SIG_SIZE));
		data_blob_free(&auth_blob);
		prs_mem_free(&outgoing_pdu);
		return False;
	}

	data_blob_free(&auth_blob);

	/*
	 * Setup the counts for this PDU.
	 */

	p->out_data.data_sent_length += data_len;
	p->out_data.current_pdu_len = p->hdr.frag_len;
	p->out_data.current_pdu_sent = 0;

	prs_mem_free(&outgoing_pdu);
	return True;
}

/*******************************************************************
 Generate the next PDU to be returned from the data in p->rdata. 
 Return an schannel authenticated fragment.
 ********************************************************************/

static BOOL create_next_pdu_schannel(pipes_struct *p)
{
	RPC_HDR_RESP hdr_resp;
	uint32 ss_padding_len = 0;
	uint32 data_len;
	uint32 data_space_available;
	uint32 data_len_left;
	prs_struct outgoing_pdu;
	uint32 data_pos;

	/*
	 * If we're in the fault state, keep returning fault PDU's until
	 * the pipe gets closed. JRA.
	 */

	if(p->fault_state) {
		setup_fault_pdu(p, NT_STATUS(DCERPC_FAULT_OP_RNG_ERROR));
		return True;
	}

	memset((char *)&hdr_resp, '\0', sizeof(hdr_resp));

	/* Change the incoming request header to a response. */
	p->hdr.pkt_type = RPC_RESPONSE;

	/* Set up rpc header flags. */
	if (p->out_data.data_sent_length == 0) {
		p->hdr.flags = RPC_FLG_FIRST;
	} else {
		p->hdr.flags = 0;
	}

	/*
	 * Work out how much we can fit in a single PDU.
	 */

	data_len_left = prs_offset(&p->out_data.rdata) - p->out_data.data_sent_length;

	/*
	 * Ensure there really is data left to send.
	 */

	if(!data_len_left) {
		DEBUG(0,("create_next_pdu_schannel: no data left to send !\n"));
		return False;
	}

	data_space_available = sizeof(p->out_data.current_pdu) - RPC_HEADER_LEN - RPC_HDR_RESP_LEN -
					RPC_HDR_AUTH_LEN - RPC_AUTH_SCHANNEL_SIGN_OR_SEAL_CHK_LEN;

	/*
	 * The amount we send is the minimum of the available
	 * space and the amount left to send.
	 */

	data_len = MIN(data_len_left, data_space_available);

	/*
	 * Set up the alloc hint. This should be the data left to
	 * send.
	 */

	hdr_resp.alloc_hint = data_len_left;

	/*
	 * Work out if this PDU will be the last.
	 */

	if(p->out_data.data_sent_length + data_len >= prs_offset(&p->out_data.rdata)) {
		p->hdr.flags |= RPC_FLG_LAST;
		if (data_len_left % 8) {
			ss_padding_len = 8 - (data_len_left % 8);
			DEBUG(10,("create_next_pdu_schannel: adding sign/seal padding of %u\n",
				ss_padding_len ));
		}
	}

	p->hdr.frag_len = RPC_HEADER_LEN + RPC_HDR_RESP_LEN + data_len + ss_padding_len +
				RPC_HDR_AUTH_LEN + RPC_AUTH_SCHANNEL_SIGN_OR_SEAL_CHK_LEN;
	p->hdr.auth_len = RPC_AUTH_SCHANNEL_SIGN_OR_SEAL_CHK_LEN;

	/*
	 * Init the parse struct to point at the outgoing
	 * data.
	 */

	prs_init( &outgoing_pdu, 0, p->mem_ctx, MARSHALL);
	prs_give_memory( &outgoing_pdu, (char *)p->out_data.current_pdu, sizeof(p->out_data.current_pdu), False);

	/* Store the header in the data stream. */
	if(!smb_io_rpc_hdr("hdr", &p->hdr, &outgoing_pdu, 0)) {
		DEBUG(0,("create_next_pdu_schannel: failed to marshall RPC_HDR.\n"));
		prs_mem_free(&outgoing_pdu);
		return False;
	}

	if(!smb_io_rpc_hdr_resp("resp", &hdr_resp, &outgoing_pdu, 0)) {
		DEBUG(0,("create_next_pdu_schannel: failed to marshall RPC_HDR_RESP.\n"));
		prs_mem_free(&outgoing_pdu);
		return False;
	}

	/* Store the current offset. */
	data_pos = prs_offset(&outgoing_pdu);

	/* Copy the data into the PDU. */

	if(!prs_append_some_prs_data(&outgoing_pdu, &p->out_data.rdata, p->out_data.data_sent_length, data_len)) {
		DEBUG(0,("create_next_pdu_schannel: failed to copy %u bytes of data.\n", (unsigned int)data_len));
		prs_mem_free(&outgoing_pdu);
		return False;
	}

	/* Copy the sign/seal padding data. */
	if (ss_padding_len) {
		char pad[8];
		memset(pad, '\0', 8);
		if (!prs_copy_data_in(&outgoing_pdu, pad, ss_padding_len)) {
			DEBUG(0,("create_next_pdu_schannel: failed to add %u bytes of pad data.\n", (unsigned int)ss_padding_len));
			prs_mem_free(&outgoing_pdu);
			return False;
		}
	}

	{
		/*
		 * Schannel processing.
		 */
		char *data;
		RPC_HDR_AUTH auth_info;
		RPC_AUTH_SCHANNEL_CHK verf;

		data = prs_data_p(&outgoing_pdu) + data_pos;
		/* Check it's the type of reply we were expecting to decode */

		init_rpc_hdr_auth(&auth_info,
				RPC_SCHANNEL_AUTH_TYPE,
				p->auth.auth_level == PIPE_AUTH_LEVEL_PRIVACY ?
					RPC_AUTH_LEVEL_PRIVACY : RPC_AUTH_LEVEL_INTEGRITY,
				ss_padding_len, 1);

		if(!smb_io_rpc_hdr_auth("hdr_auth", &auth_info, &outgoing_pdu, 0)) {
			DEBUG(0,("create_next_pdu_schannel: failed to marshall RPC_HDR_AUTH.\n"));
			prs_mem_free(&outgoing_pdu);
			return False;
		}

		schannel_encode(p->auth.a_u.schannel_auth, 
			      p->auth.auth_level,
			      SENDER_IS_ACCEPTOR,
			      &verf, data, data_len + ss_padding_len);

		if (!smb_io_rpc_auth_schannel_chk("", RPC_AUTH_SCHANNEL_SIGN_OR_SEAL_CHK_LEN, 
				&verf, &outgoing_pdu, 0)) {
			prs_mem_free(&outgoing_pdu);
			return False;
		}

		p->auth.a_u.schannel_auth->seq_num++;
	}

	/*
	 * Setup the counts for this PDU.
	 */

	p->out_data.data_sent_length += data_len;
	p->out_data.current_pdu_len = p->hdr.frag_len;
	p->out_data.current_pdu_sent = 0;

	prs_mem_free(&outgoing_pdu);
	return True;
}

/*******************************************************************
 Generate the next PDU to be returned from the data in p->rdata. 
 No authentication done.
********************************************************************/

static BOOL create_next_pdu_noauth(pipes_struct *p)
{
	RPC_HDR_RESP hdr_resp;
	uint32 data_len;
	uint32 data_space_available;
	uint32 data_len_left;
	prs_struct outgoing_pdu;

	/*
	 * If we're in the fault state, keep returning fault PDU's until
	 * the pipe gets closed. JRA.
	 */

	if(p->fault_state) {
		setup_fault_pdu(p, NT_STATUS(DCERPC_FAULT_OP_RNG_ERROR));
		return True;
	}

	memset((char *)&hdr_resp, '\0', sizeof(hdr_resp));

	/* Change the incoming request header to a response. */
	p->hdr.pkt_type = RPC_RESPONSE;

	/* Set up rpc header flags. */
	if (p->out_data.data_sent_length == 0) {
		p->hdr.flags = RPC_FLG_FIRST;
	} else {
		p->hdr.flags = 0;
	}

	/*
	 * Work out how much we can fit in a single PDU.
	 */

	data_len_left = prs_offset(&p->out_data.rdata) - p->out_data.data_sent_length;

	/*
	 * Ensure there really is data left to send.
	 */

	if(!data_len_left) {
		DEBUG(0,("create_next_pdu_noath: no data left to send !\n"));
		return False;
	}

	data_space_available = sizeof(p->out_data.current_pdu) - RPC_HEADER_LEN - RPC_HDR_RESP_LEN;

	/*
	 * The amount we send is the minimum of the available
	 * space and the amount left to send.
	 */

	data_len = MIN(data_len_left, data_space_available);

	/*
	 * Set up the alloc hint. This should be the data left to
	 * send.
	 */

	hdr_resp.alloc_hint = data_len_left;

	/*
	 * Work out if this PDU will be the last.
	 */

	if(p->out_data.data_sent_length + data_len >= prs_offset(&p->out_data.rdata)) {
		p->hdr.flags |= RPC_FLG_LAST;
	}

	/*
	 * Set up the header lengths.
	 */

	p->hdr.frag_len = RPC_HEADER_LEN + RPC_HDR_RESP_LEN + data_len;
	p->hdr.auth_len = 0;

	/*
	 * Init the parse struct to point at the outgoing
	 * data.
	 */

	prs_init( &outgoing_pdu, 0, p->mem_ctx, MARSHALL);
	prs_give_memory( &outgoing_pdu, (char *)p->out_data.current_pdu, sizeof(p->out_data.current_pdu), False);

	/* Store the header in the data stream. */
	if(!smb_io_rpc_hdr("hdr", &p->hdr, &outgoing_pdu, 0)) {
		DEBUG(0,("create_next_pdu_noath: failed to marshall RPC_HDR.\n"));
		prs_mem_free(&outgoing_pdu);
		return False;
	}

	if(!smb_io_rpc_hdr_resp("resp", &hdr_resp, &outgoing_pdu, 0)) {
		DEBUG(0,("create_next_pdu_noath: failed to marshall RPC_HDR_RESP.\n"));
		prs_mem_free(&outgoing_pdu);
		return False;
	}

	/* Copy the data into the PDU. */

	if(!prs_append_some_prs_data(&outgoing_pdu, &p->out_data.rdata, p->out_data.data_sent_length, data_len)) {
		DEBUG(0,("create_next_pdu_noauth: failed to copy %u bytes of data.\n", (unsigned int)data_len));
		prs_mem_free(&outgoing_pdu);
		return False;
	}

	/*
	 * Setup the counts for this PDU.
	 */

	p->out_data.data_sent_length += data_len;
	p->out_data.current_pdu_len = p->hdr.frag_len;
	p->out_data.current_pdu_sent = 0;

	prs_mem_free(&outgoing_pdu);
	return True;
}

/*******************************************************************
 Generate the next PDU to be returned from the data in p->rdata. 
********************************************************************/

BOOL create_next_pdu(pipes_struct *p)
{
	switch(p->auth.auth_level) {
		case PIPE_AUTH_LEVEL_NONE:
		case PIPE_AUTH_LEVEL_CONNECT:
			/* This is incorrect for auth level connect. Fixme. JRA */
			return create_next_pdu_noauth(p);
		
		default:
			switch(p->auth.auth_type) {
				case PIPE_AUTH_TYPE_NTLMSSP:
				case PIPE_AUTH_TYPE_SPNEGO_NTLMSSP:
					return create_next_pdu_ntlmssp(p);
				case PIPE_AUTH_TYPE_SCHANNEL:
					return create_next_pdu_schannel(p);
				default:
					break;
			}
	}

	DEBUG(0,("create_next_pdu: invalid internal auth level %u / type %u",
			(unsigned int)p->auth.auth_level,
			(unsigned int)p->auth.auth_type));
	return False;
}

/*******************************************************************
 Process an NTLMSSP authentication response.
 If this function succeeds, the user has been authenticated
 and their domain, name and calling workstation stored in
 the pipe struct.
*******************************************************************/

static BOOL pipe_ntlmssp_verify_final(pipes_struct *p, DATA_BLOB *p_resp_blob)
{
	DATA_BLOB reply;
	NTSTATUS status;
	AUTH_NTLMSSP_STATE *a = p->auth.a_u.auth_ntlmssp_state;

	DEBUG(5,("pipe_ntlmssp_verify_final: pipe %s checking user details\n", p->name));

	ZERO_STRUCT(reply);

	memset(p->user_name, '\0', sizeof(p->user_name));
	memset(p->pipe_user_name, '\0', sizeof(p->pipe_user_name));
	memset(p->domain, '\0', sizeof(p->domain));
	memset(p->wks, '\0', sizeof(p->wks));

	/* Set up for non-authenticated user. */
	TALLOC_FREE(p->pipe_user.nt_user_token);
	p->pipe_user.ut.ngroups = 0;
	SAFE_FREE( p->pipe_user.ut.groups);

	/* this has to be done as root in order to verify the password */
	become_root();
	status = auth_ntlmssp_update(a, *p_resp_blob, &reply);
	unbecome_root();

	/* Don't generate a reply. */
	data_blob_free(&reply);

	if (!NT_STATUS_IS_OK(status)) {
		return False;
	}

	/* Finally - if the pipe negotiated integrity (sign) or privacy (seal)
	   ensure the underlying NTLMSSP flags are also set. If not we should
	   refuse the bind. */

	if (p->auth.auth_level == PIPE_AUTH_LEVEL_INTEGRITY) {
		if (!(a->ntlmssp_state->neg_flags & NTLMSSP_NEGOTIATE_SIGN)) {
			DEBUG(0,("pipe_ntlmssp_verify_final: pipe %s : packet integrity requested "
				"but client declined signing.\n",
					p->name ));
			return False;
		}
	}
	if (p->auth.auth_level == PIPE_AUTH_LEVEL_PRIVACY) {
		if (!(a->ntlmssp_state->neg_flags & NTLMSSP_NEGOTIATE_SEAL)) {
			DEBUG(0,("pipe_ntlmssp_verify_final: pipe %s : packet privacy requested "
				"but client declined sealing.\n",
					p->name ));
			return False;
		}
	}
	
	fstrcpy(p->user_name, a->ntlmssp_state->user);
	fstrcpy(p->pipe_user_name, a->server_info->unix_name);
	fstrcpy(p->domain, a->ntlmssp_state->domain);
	fstrcpy(p->wks, a->ntlmssp_state->workstation);

	DEBUG(5,("pipe_ntlmssp_verify_final: OK: user: %s domain: %s workstation: %s\n",
		p->user_name, p->domain, p->wks));

	/*
	 * Store the UNIX credential data (uid/gid pair) in the pipe structure.
	 */

	p->pipe_user.ut.uid = a->server_info->uid;
	p->pipe_user.ut.gid = a->server_info->gid;
	
	/*
	 * Copy the session key from the ntlmssp state.
	 */

	data_blob_free(&p->session_key);
	p->session_key = data_blob(a->ntlmssp_state->session_key.data, a->ntlmssp_state->session_key.length);
	if (!p->session_key.data) {
		return False;
	}

	p->pipe_user.ut.ngroups = a->server_info->n_groups;
	if (p->pipe_user.ut.ngroups) {
		if (!(p->pipe_user.ut.groups = memdup(a->server_info->groups,
						sizeof(gid_t) * p->pipe_user.ut.ngroups))) {
			DEBUG(0,("failed to memdup group list to p->pipe_user.groups\n"));
			return False;
		}
	}

	if (a->server_info->ptok) {
		p->pipe_user.nt_user_token =
			dup_nt_token(NULL, a->server_info->ptok);
	} else {
		DEBUG(1,("Error: Authmodule failed to provide nt_user_token\n"));
		p->pipe_user.nt_user_token = NULL;
		return False;
	}

	return True;
}

/*******************************************************************
 The switch table for the pipe names and the functions to handle them.
*******************************************************************/

struct rpc_table {
	struct {
		const char *clnt;
		const char *srv;
	} pipe;
	struct api_struct *cmds;
	int n_cmds;
};

static struct rpc_table *rpc_lookup;
static int rpc_lookup_size;

/*******************************************************************
 This is the "stage3" NTLMSSP response after a bind request and reply.
*******************************************************************/

BOOL api_pipe_bind_auth3(pipes_struct *p, prs_struct *rpc_in_p)
{
	RPC_HDR_AUTH auth_info;
	uint32 pad;
	DATA_BLOB blob;

	ZERO_STRUCT(blob);

	DEBUG(5,("api_pipe_bind_auth3: decode request. %d\n", __LINE__));

	if (p->hdr.auth_len == 0) {
		DEBUG(0,("api_pipe_bind_auth3: No auth field sent !\n"));
		goto err;
	}

	/* 4 bytes padding. */
	if (!prs_uint32("pad", rpc_in_p, 0, &pad)) {
		DEBUG(0,("api_pipe_bind_auth3: unmarshall of 4 byte pad failed.\n"));
		goto err;
	}

	/*
	 * Decode the authentication verifier response.
	 */

	if(!smb_io_rpc_hdr_auth("", &auth_info, rpc_in_p, 0)) {
		DEBUG(0,("api_pipe_bind_auth3: unmarshall of RPC_HDR_AUTH failed.\n"));
		goto err;
	}

	if (auth_info.auth_type != RPC_NTLMSSP_AUTH_TYPE) {
		DEBUG(0,("api_pipe_bind_auth3: incorrect auth type (%u).\n",
			(unsigned int)auth_info.auth_type ));
		return False;
	}

	blob = data_blob(NULL,p->hdr.auth_len);

	if (!prs_copy_data_out((char *)blob.data, rpc_in_p, p->hdr.auth_len)) {
		DEBUG(0,("api_pipe_bind_auth3: Failed to pull %u bytes - the response blob.\n",
			(unsigned int)p->hdr.auth_len ));
		goto err;
	}

	/*
	 * The following call actually checks the challenge/response data.
	 * for correctness against the given DOMAIN\user name.
	 */
	
	if (!pipe_ntlmssp_verify_final(p, &blob)) {
		goto err;
	}

	data_blob_free(&blob);

	p->pipe_bound = True;

	return True;

 err:

	data_blob_free(&blob);
	free_pipe_ntlmssp_auth_data(&p->auth);
	p->auth.a_u.auth_ntlmssp_state = NULL;

	return False;
}

/*******************************************************************
 Marshall a bind_nak pdu.
*******************************************************************/

static BOOL setup_bind_nak(pipes_struct *p)
{
	prs_struct outgoing_rpc;
	RPC_HDR nak_hdr;
	uint16 zero = 0;

	/* Free any memory in the current return data buffer. */
	prs_mem_free(&p->out_data.rdata);

	/*
	 * Marshall directly into the outgoing PDU space. We
	 * must do this as we need to set to the bind response
	 * header and are never sending more than one PDU here.
	 */

	prs_init( &outgoing_rpc, 0, p->mem_ctx, MARSHALL);
	prs_give_memory( &outgoing_rpc, (char *)p->out_data.current_pdu, sizeof(p->out_data.current_pdu), False);

	/*
	 * Initialize a bind_nak header.
	 */

	init_rpc_hdr(&nak_hdr, RPC_BINDNACK, RPC_FLG_FIRST | RPC_FLG_LAST,
		p->hdr.call_id, RPC_HEADER_LEN + sizeof(uint16), 0);

	/*
	 * Marshall the header into the outgoing PDU.
	 */

	if(!smb_io_rpc_hdr("", &nak_hdr, &outgoing_rpc, 0)) {
		DEBUG(0,("setup_bind_nak: marshalling of RPC_HDR failed.\n"));
		prs_mem_free(&outgoing_rpc);
		return False;
	}

	/*
	 * Now add the reject reason.
	 */

	if(!prs_uint16("reject code", &outgoing_rpc, 0, &zero)) {
		prs_mem_free(&outgoing_rpc);
		return False;
	}

	p->out_data.data_sent_length = 0;
	p->out_data.current_pdu_len = prs_offset(&outgoing_rpc);
	p->out_data.current_pdu_sent = 0;

	if (p->auth.auth_data_free_func) {
		(*p->auth.auth_data_free_func)(&p->auth);
	}
	p->auth.auth_level = PIPE_AUTH_LEVEL_NONE;
	p->auth.auth_type = PIPE_AUTH_TYPE_NONE;
	p->pipe_bound = False;

	return True;
}

/*******************************************************************
 Marshall a fault pdu.
*******************************************************************/

BOOL setup_fault_pdu(pipes_struct *p, NTSTATUS status)
{
	prs_struct outgoing_pdu;
	RPC_HDR fault_hdr;
	RPC_HDR_RESP hdr_resp;
	RPC_HDR_FAULT fault_resp;

	/* Free any memory in the current return data buffer. */
	prs_mem_free(&p->out_data.rdata);

	/*
	 * Marshall directly into the outgoing PDU space. We
	 * must do this as we need to set to the bind response
	 * header and are never sending more than one PDU here.
	 */

	prs_init( &outgoing_pdu, 0, p->mem_ctx, MARSHALL);
	prs_give_memory( &outgoing_pdu, (char *)p->out_data.current_pdu, sizeof(p->out_data.current_pdu), False);

	/*
	 * Initialize a fault header.
	 */

	init_rpc_hdr(&fault_hdr, RPC_FAULT, RPC_FLG_FIRST | RPC_FLG_LAST | RPC_FLG_NOCALL,
            p->hdr.call_id, RPC_HEADER_LEN + RPC_HDR_RESP_LEN + RPC_HDR_FAULT_LEN, 0);

	/*
	 * Initialize the HDR_RESP and FAULT parts of the PDU.
	 */

	memset((char *)&hdr_resp, '\0', sizeof(hdr_resp));

	fault_resp.status = status;
	fault_resp.reserved = 0;

	/*
	 * Marshall the header into the outgoing PDU.
	 */

	if(!smb_io_rpc_hdr("", &fault_hdr, &outgoing_pdu, 0)) {
		DEBUG(0,("setup_fault_pdu: marshalling of RPC_HDR failed.\n"));
		prs_mem_free(&outgoing_pdu);
		return False;
	}

	if(!smb_io_rpc_hdr_resp("resp", &hdr_resp, &outgoing_pdu, 0)) {
		DEBUG(0,("setup_fault_pdu: failed to marshall RPC_HDR_RESP.\n"));
		prs_mem_free(&outgoing_pdu);
		return False;
	}

	if(!smb_io_rpc_hdr_fault("fault", &fault_resp, &outgoing_pdu, 0)) {
		DEBUG(0,("setup_fault_pdu: failed to marshall RPC_HDR_FAULT.\n"));
		prs_mem_free(&outgoing_pdu);
		return False;
	}

	p->out_data.data_sent_length = 0;
	p->out_data.current_pdu_len = prs_offset(&outgoing_pdu);
	p->out_data.current_pdu_sent = 0;

	prs_mem_free(&outgoing_pdu);
	return True;
}

#if 0
/*******************************************************************
 Marshall a cancel_ack pdu.
 We should probably check the auth-verifier here.
*******************************************************************/

BOOL setup_cancel_ack_reply(pipes_struct *p, prs_struct *rpc_in_p)
{
	prs_struct outgoing_pdu;
	RPC_HDR ack_reply_hdr;

	/* Free any memory in the current return data buffer. */
	prs_mem_free(&p->out_data.rdata);

	/*
	 * Marshall directly into the outgoing PDU space. We
	 * must do this as we need to set to the bind response
	 * header and are never sending more than one PDU here.
	 */

	prs_init( &outgoing_pdu, 0, p->mem_ctx, MARSHALL);
	prs_give_memory( &outgoing_pdu, (char *)p->out_data.current_pdu, sizeof(p->out_data.current_pdu), False);

	/*
	 * Initialize a cancel_ack header.
	 */

	init_rpc_hdr(&ack_reply_hdr, RPC_CANCEL_ACK, RPC_FLG_FIRST | RPC_FLG_LAST,
			p->hdr.call_id, RPC_HEADER_LEN, 0);

	/*
	 * Marshall the header into the outgoing PDU.
	 */

	if(!smb_io_rpc_hdr("", &ack_reply_hdr, &outgoing_pdu, 0)) {
		DEBUG(0,("setup_cancel_ack_reply: marshalling of RPC_HDR failed.\n"));
		prs_mem_free(&outgoing_pdu);
		return False;
	}

	p->out_data.data_sent_length = 0;
	p->out_data.current_pdu_len = prs_offset(&outgoing_pdu);
	p->out_data.current_pdu_sent = 0;

	prs_mem_free(&outgoing_pdu);
	return True;
}
#endif

/*******************************************************************
 Ensure a bind request has the correct abstract & transfer interface.
 Used to reject unknown binds from Win2k.
*******************************************************************/

BOOL check_bind_req(struct pipes_struct *p, RPC_IFACE* abstract,
                    RPC_IFACE* transfer, uint32 context_id)
{
	char *pipe_name = p->name;
	int i=0;
	fstring pname;
	
	fstrcpy(pname,"\\PIPE\\");
	fstrcat(pname,pipe_name);

	DEBUG(3,("check_bind_req for %s\n", pname));

	/* we have to check all now since win2k introduced a new UUID on the lsaprpc pipe */
		
	for ( i=0; pipe_names[i].client_pipe; i++ ) {
		DEBUG(10,("checking %s\n", pipe_names[i].client_pipe));
		if ( strequal(pipe_names[i].client_pipe, pname)
			&& (abstract->version == pipe_names[i].abstr_syntax.version) 
			&& (memcmp(&abstract->uuid, &pipe_names[i].abstr_syntax.uuid, sizeof(struct uuid)) == 0)
			&& (transfer->version == pipe_names[i].trans_syntax.version)
			&& (memcmp(&transfer->uuid, &pipe_names[i].trans_syntax.uuid, sizeof(struct uuid)) == 0) ) {
			struct api_struct 	*fns = NULL;
			int 			n_fns = 0;
			PIPE_RPC_FNS		*context_fns;
			
			if ( !(context_fns = SMB_MALLOC_P(PIPE_RPC_FNS)) ) {
				DEBUG(0,("check_bind_req: malloc() failed!\n"));
				return False;
			}
			
			/* save the RPC function table associated with this bind */
			
			get_pipe_fns(i, &fns, &n_fns);
			
			context_fns->cmds = fns;
			context_fns->n_cmds = n_fns;
			context_fns->context_id = context_id;
			
			/* add to the list of open contexts */
			
			DLIST_ADD( p->contexts, context_fns );
			
			break;
		}
	}

	if(pipe_names[i].client_pipe == NULL) {
		return False;
	}

	return True;
}

/*******************************************************************
 Register commands to an RPC pipe
*******************************************************************/

NTSTATUS rpc_pipe_register_commands(int version, const char *clnt, const char *srv, const struct api_struct *cmds, int size)
{
        struct rpc_table *rpc_entry;

	if (!clnt || !srv || !cmds) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	if (version != SMB_RPC_INTERFACE_VERSION) {
		DEBUG(0,("Can't register rpc commands!\n"
			 "You tried to register a rpc module with SMB_RPC_INTERFACE_VERSION %d"
			 ", while this version of samba uses version %d!\n", 
			 version,SMB_RPC_INTERFACE_VERSION));
		return NT_STATUS_OBJECT_TYPE_MISMATCH;
	}

	/* TODO: 
	 *
	 * we still need to make sure that don't register the same commands twice!!!
	 * 
	 * --metze
	 */

        /* We use a temporary variable because this call can fail and 
           rpc_lookup will still be valid afterwards.  It could then succeed if
           called again later */
	rpc_lookup_size++;
        rpc_entry = SMB_REALLOC_ARRAY_KEEP_OLD_ON_ERROR(rpc_lookup, struct rpc_table, rpc_lookup_size);
        if (NULL == rpc_entry) {
                rpc_lookup_size--;
                DEBUG(0, ("rpc_pipe_register_commands: memory allocation failed\n"));
                return NT_STATUS_NO_MEMORY;
        } else {
                rpc_lookup = rpc_entry;
        }
        
        rpc_entry = rpc_lookup + (rpc_lookup_size - 1);
        ZERO_STRUCTP(rpc_entry);
        rpc_entry->pipe.clnt = SMB_STRDUP(clnt);
        rpc_entry->pipe.srv = SMB_STRDUP(srv);
        rpc_entry->cmds = SMB_REALLOC_ARRAY(rpc_entry->cmds, struct api_struct, rpc_entry->n_cmds + size);
	if (!rpc_entry->cmds) {
		return NT_STATUS_NO_MEMORY;
	}
        memcpy(rpc_entry->cmds + rpc_entry->n_cmds, cmds, size * sizeof(struct api_struct));
        rpc_entry->n_cmds += size;
        
        return NT_STATUS_OK;
}

/*******************************************************************
 Handle a SPNEGO krb5 bind auth.
*******************************************************************/

static BOOL pipe_spnego_auth_bind_kerberos(pipes_struct *p, prs_struct *rpc_in_p, RPC_HDR_AUTH *pauth_info,
		DATA_BLOB *psecblob, prs_struct *pout_auth)
{
	return False;
}

/*******************************************************************
 Handle the first part of a SPNEGO bind auth.
*******************************************************************/

static BOOL pipe_spnego_auth_bind_negotiate(pipes_struct *p, prs_struct *rpc_in_p,
					RPC_HDR_AUTH *pauth_info, prs_struct *pout_auth)
{
	DATA_BLOB blob;
	DATA_BLOB secblob;
	DATA_BLOB response;
	DATA_BLOB chal;
	char *OIDs[ASN1_MAX_OIDS];
        int i;
	NTSTATUS status;
        BOOL got_kerberos_mechanism = False;
	AUTH_NTLMSSP_STATE *a = NULL;
	RPC_HDR_AUTH auth_info;

	ZERO_STRUCT(secblob);
	ZERO_STRUCT(chal);
	ZERO_STRUCT(response);

	/* Grab the SPNEGO blob. */
	blob = data_blob(NULL,p->hdr.auth_len);

	if (!prs_copy_data_out((char *)blob.data, rpc_in_p, p->hdr.auth_len)) {
		DEBUG(0,("pipe_spnego_auth_bind_negotiate: Failed to pull %u bytes - the SPNEGO auth header.\n",
			(unsigned int)p->hdr.auth_len ));
		goto err;
	}

	if (blob.data[0] != ASN1_APPLICATION(0)) {
		goto err;
	}

	/* parse out the OIDs and the first sec blob */
	if (!parse_negTokenTarg(blob, OIDs, &secblob)) {
		DEBUG(0,("pipe_spnego_auth_bind_negotiate: Failed to parse the security blob.\n"));
		goto err;
        }

	if (strcmp(OID_KERBEROS5, OIDs[0]) == 0 || strcmp(OID_KERBEROS5_OLD, OIDs[0]) == 0) {
		got_kerberos_mechanism = True;
	}

	for (i=0;OIDs[i];i++) {
		DEBUG(3,("pipe_spnego_auth_bind_negotiate: Got OID %s\n", OIDs[i]));
		SAFE_FREE(OIDs[i]);
	}
	DEBUG(3,("pipe_spnego_auth_bind_negotiate: Got secblob of size %lu\n", (unsigned long)secblob.length));

	if ( got_kerberos_mechanism && ((lp_security()==SEC_ADS) || lp_use_kerberos_keytab()) ) {
		BOOL ret = pipe_spnego_auth_bind_kerberos(p, rpc_in_p, pauth_info, &secblob, pout_auth);
		data_blob_free(&secblob);
		data_blob_free(&blob);
		return ret;
	}

	if (p->auth.auth_type == PIPE_AUTH_TYPE_SPNEGO_NTLMSSP && p->auth.a_u.auth_ntlmssp_state) {
		/* Free any previous auth type. */
		free_pipe_ntlmssp_auth_data(&p->auth);
	}

	/* Initialize the NTLM engine. */
	status = auth_ntlmssp_start(&a);
	if (!NT_STATUS_IS_OK(status)) {
		goto err;
	}

	/*
	 * Pass the first security blob of data to it.
	 * This can return an error or NT_STATUS_MORE_PROCESSING_REQUIRED
	 * which means we need another packet to complete the bind.
	 */

        status = auth_ntlmssp_update(a, secblob, &chal);

	if (!NT_STATUS_EQUAL(status, NT_STATUS_MORE_PROCESSING_REQUIRED)) {
		DEBUG(3,("pipe_spnego_auth_bind_negotiate: auth_ntlmssp_update failed.\n"));
		goto err;
	}

	/* Generate the response blob we need for step 2 of the bind. */
	response = spnego_gen_auth_response(&chal, status, OID_NTLMSSP);

	/* Copy the blob into the pout_auth parse struct */
	init_rpc_hdr_auth(&auth_info, RPC_SPNEGO_AUTH_TYPE, pauth_info->auth_level, RPC_HDR_AUTH_LEN, 1);
	if(!smb_io_rpc_hdr_auth("", &auth_info, pout_auth, 0)) {
		DEBUG(0,("pipe_spnego_auth_bind_negotiate: marshalling of RPC_HDR_AUTH failed.\n"));
		goto err;
	}

	if (!prs_copy_data_in(pout_auth, (char *)response.data, response.length)) {
		DEBUG(0,("pipe_spnego_auth_bind_negotiate: marshalling of data blob failed.\n"));
		goto err;
	}

	p->auth.a_u.auth_ntlmssp_state = a;
	p->auth.auth_data_free_func = &free_pipe_ntlmssp_auth_data;
	p->auth.auth_type = PIPE_AUTH_TYPE_SPNEGO_NTLMSSP;

	data_blob_free(&blob);
	data_blob_free(&secblob);
	data_blob_free(&chal);
	data_blob_free(&response);

	/* We can't set pipe_bound True yet - we need an RPC_ALTER_CONTEXT response packet... */
	return True;

 err:

	data_blob_free(&blob);
	data_blob_free(&secblob);
	data_blob_free(&chal);
	data_blob_free(&response);

	p->auth.a_u.auth_ntlmssp_state = NULL;

	return False;
}

/*******************************************************************
 Handle the second part of a SPNEGO bind auth.
*******************************************************************/

static BOOL pipe_spnego_auth_bind_continue(pipes_struct *p, prs_struct *rpc_in_p,
					RPC_HDR_AUTH *pauth_info, prs_struct *pout_auth)
{
	RPC_HDR_AUTH auth_info;
	DATA_BLOB spnego_blob;
	DATA_BLOB auth_blob;
	DATA_BLOB auth_reply;
	DATA_BLOB response;
	AUTH_NTLMSSP_STATE *a = p->auth.a_u.auth_ntlmssp_state;

	ZERO_STRUCT(spnego_blob);
	ZERO_STRUCT(auth_blob);
	ZERO_STRUCT(auth_reply);
	ZERO_STRUCT(response);

	if (p->auth.auth_type != PIPE_AUTH_TYPE_SPNEGO_NTLMSSP || !a) {
		DEBUG(0,("pipe_spnego_auth_bind_continue: not in NTLMSSP auth state.\n"));
		goto err;
	}

	/* Grab the SPNEGO blob. */
	spnego_blob = data_blob(NULL,p->hdr.auth_len);

	if (!prs_copy_data_out((char *)spnego_blob.data, rpc_in_p, p->hdr.auth_len)) {
		DEBUG(0,("pipe_spnego_auth_bind_continue: Failed to pull %u bytes - the SPNEGO auth header.\n",
			(unsigned int)p->hdr.auth_len ));
		goto err;
	}

	if (spnego_blob.data[0] != ASN1_CONTEXT(1)) {
		DEBUG(0,("pipe_spnego_auth_bind_continue: invalid SPNEGO blob type.\n"));
		goto err;
	}

	if (!spnego_parse_auth(spnego_blob, &auth_blob)) {
		DEBUG(0,("pipe_spnego_auth_bind_continue: invalid SPNEGO blob.\n"));
		goto err;
	}

	/*
	 * The following call actually checks the challenge/response data.
	 * for correctness against the given DOMAIN\user name.
	 */
	
	if (!pipe_ntlmssp_verify_final(p, &auth_blob)) {
		goto err;
	}

	data_blob_free(&spnego_blob);
	data_blob_free(&auth_blob);

	/* Generate the spnego "accept completed" blob - no incoming data. */
	response = spnego_gen_auth_response(&auth_reply, NT_STATUS_OK, OID_NTLMSSP);

	/* Copy the blob into the pout_auth parse struct */
	init_rpc_hdr_auth(&auth_info, RPC_SPNEGO_AUTH_TYPE, pauth_info->auth_level, RPC_HDR_AUTH_LEN, 1);
	if(!smb_io_rpc_hdr_auth("", &auth_info, pout_auth, 0)) {
		DEBUG(0,("pipe_spnego_auth_bind_continue: marshalling of RPC_HDR_AUTH failed.\n"));
		goto err;
	}

	if (!prs_copy_data_in(pout_auth, (char *)response.data, response.length)) {
		DEBUG(0,("pipe_spnego_auth_bind_continue: marshalling of data blob failed.\n"));
		goto err;
	}

	data_blob_free(&auth_reply);
	data_blob_free(&response);

	p->pipe_bound = True;

	return True;

 err:

	data_blob_free(&spnego_blob);
	data_blob_free(&auth_blob);
	data_blob_free(&auth_reply);
	data_blob_free(&response);

	free_pipe_ntlmssp_auth_data(&p->auth);
	p->auth.a_u.auth_ntlmssp_state = NULL;

	return False;
}

/*******************************************************************
 Handle an schannel bind auth.
*******************************************************************/

static BOOL pipe_schannel_auth_bind(pipes_struct *p, prs_struct *rpc_in_p,
					RPC_HDR_AUTH *pauth_info, prs_struct *pout_auth)
{
	RPC_HDR_AUTH auth_info;
	RPC_AUTH_SCHANNEL_NEG neg;
	RPC_AUTH_VERIFIER auth_verifier;
	BOOL ret;
	struct dcinfo *pdcinfo;
	uint32 flags;

	if (!smb_io_rpc_auth_schannel_neg("", &neg, rpc_in_p, 0)) {
		DEBUG(0,("pipe_schannel_auth_bind: Could not unmarshal SCHANNEL auth neg\n"));
		return False;
	}

	/*
	 * The neg.myname key here must match the remote computer name
	 * given in the DOM_CLNT_SRV.uni_comp_name used on all netlogon pipe
	 * operations that use credentials.
	 */

	become_root();
	ret = secrets_restore_schannel_session_info(p->mem_ctx, neg.myname, &pdcinfo);
	unbecome_root();

	if (!ret) {
		DEBUG(0, ("pipe_schannel_auth_bind: Attempt to bind using schannel without successful serverauth2\n"));
		return False;
	}

	p->auth.a_u.schannel_auth = TALLOC_P(p->pipe_state_mem_ctx, struct schannel_auth_struct);
	if (!p->auth.a_u.schannel_auth) {
		TALLOC_FREE(pdcinfo);
		return False;
	}

	memset(p->auth.a_u.schannel_auth->sess_key, 0, sizeof(p->auth.a_u.schannel_auth->sess_key));
	memcpy(p->auth.a_u.schannel_auth->sess_key, pdcinfo->sess_key,
			sizeof(pdcinfo->sess_key));

	TALLOC_FREE(pdcinfo);

	p->auth.a_u.schannel_auth->seq_num = 0;

	/*
	 * JRA. Should we also copy the schannel session key into the pipe session key p->session_key
	 * here ? We do that for NTLMSSP, but the session key is already set up from the vuser
	 * struct of the person who opened the pipe. I need to test this further. JRA.
	 */

	init_rpc_hdr_auth(&auth_info, RPC_SCHANNEL_AUTH_TYPE, pauth_info->auth_level, RPC_HDR_AUTH_LEN, 1);
	if(!smb_io_rpc_hdr_auth("", &auth_info, pout_auth, 0)) {
		DEBUG(0,("pipe_schannel_auth_bind: marshalling of RPC_HDR_AUTH failed.\n"));
		return False;
	}

	/*** SCHANNEL verifier ***/

	init_rpc_auth_verifier(&auth_verifier, "\001", 0x0);
	if(!smb_io_rpc_schannel_verifier("", &auth_verifier, pout_auth, 0)) {
		DEBUG(0,("pipe_schannel_auth_bind: marshalling of RPC_AUTH_VERIFIER failed.\n"));
		return False;
	}

	prs_align(pout_auth);

	flags = 5;
	if(!prs_uint32("flags ", pout_auth, 0, &flags)) {
		return False;
	}

	DEBUG(10,("pipe_schannel_auth_bind: schannel auth: domain [%s] myname [%s]\n",
		neg.domain, neg.myname));

	/* We're finished with this bind - no more packets. */
	p->auth.auth_data_free_func = NULL;
	p->auth.auth_type = PIPE_AUTH_TYPE_SCHANNEL;

	p->pipe_bound = True;

	return True;
}

/*******************************************************************
 Handle an NTLMSSP bind auth.
*******************************************************************/

static BOOL pipe_ntlmssp_auth_bind(pipes_struct *p, prs_struct *rpc_in_p,
					RPC_HDR_AUTH *pauth_info, prs_struct *pout_auth)
{
	RPC_HDR_AUTH auth_info;
        DATA_BLOB blob;
	DATA_BLOB response;
        NTSTATUS status;
	AUTH_NTLMSSP_STATE *a = NULL;

	ZERO_STRUCT(blob);
	ZERO_STRUCT(response);

	/* Grab the NTLMSSP blob. */
	blob = data_blob(NULL,p->hdr.auth_len);

	if (!prs_copy_data_out((char *)blob.data, rpc_in_p, p->hdr.auth_len)) {
		DEBUG(0,("pipe_ntlmssp_auth_bind: Failed to pull %u bytes - the NTLM auth header.\n",
			(unsigned int)p->hdr.auth_len ));
		goto err;
	}

	if (strncmp((char *)blob.data, "NTLMSSP", 7) != 0) {
		DEBUG(0,("pipe_ntlmssp_auth_bind: Failed to read NTLMSSP in blob\n"));
                goto err;
        }

	/* We have an NTLMSSP blob. */
	status = auth_ntlmssp_start(&a);
	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(0,("pipe_ntlmssp_auth_bind: auth_ntlmssp_start failed: %s\n",
			nt_errstr(status) ));
		goto err;
	}

	status = auth_ntlmssp_update(a, blob, &response);
	if (!NT_STATUS_EQUAL(status, NT_STATUS_MORE_PROCESSING_REQUIRED)) {
		DEBUG(0,("pipe_ntlmssp_auth_bind: auth_ntlmssp_update failed: %s\n",
			nt_errstr(status) ));
		goto err;
	}

	data_blob_free(&blob);

	/* Copy the blob into the pout_auth parse struct */
	init_rpc_hdr_auth(&auth_info, RPC_NTLMSSP_AUTH_TYPE, pauth_info->auth_level, RPC_HDR_AUTH_LEN, 1);
	if(!smb_io_rpc_hdr_auth("", &auth_info, pout_auth, 0)) {
		DEBUG(0,("pipe_ntlmssp_auth_bind: marshalling of RPC_HDR_AUTH failed.\n"));
		goto err;
	}

	if (!prs_copy_data_in(pout_auth, (char *)response.data, response.length)) {
		DEBUG(0,("pipe_ntlmssp_auth_bind: marshalling of data blob failed.\n"));
		goto err;
	}

	p->auth.a_u.auth_ntlmssp_state = a;
	p->auth.auth_data_free_func = &free_pipe_ntlmssp_auth_data;
	p->auth.auth_type = PIPE_AUTH_TYPE_NTLMSSP;

	data_blob_free(&blob);
	data_blob_free(&response);

	DEBUG(10,("pipe_ntlmssp_auth_bind: NTLMSSP auth started\n"));

	/* We can't set pipe_bound True yet - we need an RPC_AUTH3 response packet... */
	return True;

  err:

	data_blob_free(&blob);
	data_blob_free(&response);

	free_pipe_ntlmssp_auth_data(&p->auth);
	p->auth.a_u.auth_ntlmssp_state = NULL;
	return False;
}

/*******************************************************************
 Respond to a pipe bind request.
*******************************************************************/

BOOL api_pipe_bind_req(pipes_struct *p, prs_struct *rpc_in_p)
{
	RPC_HDR_BA hdr_ba;
	RPC_HDR_RB hdr_rb;
	RPC_HDR_AUTH auth_info;
	uint16 assoc_gid;
	fstring ack_pipe_name;
	prs_struct out_hdr_ba;
	prs_struct out_auth;
	prs_struct outgoing_rpc;
	int i = 0;
	int auth_len = 0;
	unsigned int auth_type = RPC_ANONYMOUS_AUTH_TYPE;

	/* No rebinds on a bound pipe - use alter context. */
	if (p->pipe_bound) {
		DEBUG(2,("api_pipe_bind_req: rejecting bind request on bound pipe %s.\n", p->pipe_srv_name));
		return setup_bind_nak(p);
	}

	prs_init( &outgoing_rpc, 0, p->mem_ctx, MARSHALL);

	/* 
	 * Marshall directly into the outgoing PDU space. We
	 * must do this as we need to set to the bind response
	 * header and are never sending more than one PDU here.
	 */

	prs_give_memory( &outgoing_rpc, (char *)p->out_data.current_pdu, sizeof(p->out_data.current_pdu), False);

	/*
	 * Setup the memory to marshall the ba header, and the
	 * auth footers.
	 */

	if(!prs_init(&out_hdr_ba, 1024, p->mem_ctx, MARSHALL)) {
		DEBUG(0,("api_pipe_bind_req: malloc out_hdr_ba failed.\n"));
		prs_mem_free(&outgoing_rpc);
		return False;
	}

	if(!prs_init(&out_auth, 1024, p->mem_ctx, MARSHALL)) {
		DEBUG(0,("api_pipe_bind_req: malloc out_auth failed.\n"));
		prs_mem_free(&outgoing_rpc);
		prs_mem_free(&out_hdr_ba);
		return False;
	}

	DEBUG(5,("api_pipe_bind_req: decode request. %d\n", __LINE__));

	/*
	 * Try and find the correct pipe name to ensure
	 * that this is a pipe name we support.
	 */


	for (i = 0; i < rpc_lookup_size; i++) {
	        if (strequal(rpc_lookup[i].pipe.clnt, p->name)) {
			DEBUG(3, ("api_pipe_bind_req: \\PIPE\\%s -> \\PIPE\\%s\n",
				rpc_lookup[i].pipe.clnt, rpc_lookup[i].pipe.srv));
			fstrcpy(p->pipe_srv_name, rpc_lookup[i].pipe.srv);
			break;
		}
	}

	if (i == rpc_lookup_size) {
		if (NT_STATUS_IS_ERR(smb_probe_module("rpc", p->name))) {
                       DEBUG(3,("api_pipe_bind_req: Unknown pipe name %s in bind request.\n",
                                p->name ));
			prs_mem_free(&outgoing_rpc);
			prs_mem_free(&out_hdr_ba);
			prs_mem_free(&out_auth);

			return setup_bind_nak(p);
                }

                for (i = 0; i < rpc_lookup_size; i++) {
                       if (strequal(rpc_lookup[i].pipe.clnt, p->name)) {
                               DEBUG(3, ("api_pipe_bind_req: \\PIPE\\%s -> \\PIPE\\%s\n",
                                         rpc_lookup[i].pipe.clnt, rpc_lookup[i].pipe.srv));
                               fstrcpy(p->pipe_srv_name, rpc_lookup[i].pipe.srv);
                               break;
                       }
                }

		if (i == rpc_lookup_size) {
			DEBUG(0, ("module %s doesn't provide functions for pipe %s!\n", p->name, p->name));
			goto err_exit;
		}
	}

	/* decode the bind request */
	if(!smb_io_rpc_hdr_rb("", &hdr_rb, rpc_in_p, 0))  {
		DEBUG(0,("api_pipe_bind_req: unable to unmarshall RPC_HDR_RB struct.\n"));
		goto err_exit;
	}

	/* name has to be \PIPE\xxxxx */
	fstrcpy(ack_pipe_name, "\\PIPE\\");
	fstrcat(ack_pipe_name, p->pipe_srv_name);

	DEBUG(5,("api_pipe_bind_req: make response. %d\n", __LINE__));

	/*
	 * Check if this is an authenticated bind request.
	 */

	if (p->hdr.auth_len) {
		/* 
		 * Decode the authentication verifier.
		 */

		if(!smb_io_rpc_hdr_auth("", &auth_info, rpc_in_p, 0)) {
			DEBUG(0,("api_pipe_bind_req: unable to unmarshall RPC_HDR_AUTH struct.\n"));
			goto err_exit;
		}

		auth_type = auth_info.auth_type;

		/* Work out if we have to sign or seal etc. */
		switch (auth_info.auth_level) {
			case RPC_AUTH_LEVEL_INTEGRITY:
				p->auth.auth_level = PIPE_AUTH_LEVEL_INTEGRITY;
				break;
			case RPC_AUTH_LEVEL_PRIVACY:
				p->auth.auth_level = PIPE_AUTH_LEVEL_PRIVACY;
				break;
			default:
				DEBUG(0,("api_pipe_bind_req: unexpected auth level (%u).\n",
					(unsigned int)auth_info.auth_level ));
				goto err_exit;
		}
	} else {
		ZERO_STRUCT(auth_info);
	}

	assoc_gid = hdr_rb.bba.assoc_gid ? hdr_rb.bba.assoc_gid : 0x53f0;

	switch(auth_type) {
		case RPC_NTLMSSP_AUTH_TYPE:
			if (!pipe_ntlmssp_auth_bind(p, rpc_in_p, &auth_info, &out_auth)) {
				goto err_exit;
			}
			assoc_gid = 0x7a77;
			break;

		case RPC_SCHANNEL_AUTH_TYPE:
			if (!pipe_schannel_auth_bind(p, rpc_in_p, &auth_info, &out_auth)) {
				goto err_exit;
			}
			break;

		case RPC_SPNEGO_AUTH_TYPE:
			if (!pipe_spnego_auth_bind_negotiate(p, rpc_in_p, &auth_info, &out_auth)) {
				goto err_exit;
			}
			break;

		case RPC_ANONYMOUS_AUTH_TYPE:
			/* Unauthenticated bind request. */
			/* We're finished - no more packets. */
			p->auth.auth_type = PIPE_AUTH_TYPE_NONE;
			/* We must set the pipe auth_level here also. */
			p->auth.auth_level = PIPE_AUTH_LEVEL_NONE;
			p->pipe_bound = True;
			break;

		default:
			DEBUG(0,("api_pipe_bind_req: unknown auth type %x requested.\n", auth_type ));
			goto err_exit;
	}

	/*
	 * Create the bind response struct.
	 */

	/* If the requested abstract synt uuid doesn't match our client pipe,
		reject the bind_ack & set the transfer interface synt to all 0's,
		ver 0 (observed when NT5 attempts to bind to abstract interfaces
		unknown to NT4)
		Needed when adding entries to a DACL from NT5 - SK */

	if(check_bind_req(p, &hdr_rb.rpc_context[0].abstract, &hdr_rb.rpc_context[0].transfer[0],
				hdr_rb.rpc_context[0].context_id )) {
		init_rpc_hdr_ba(&hdr_ba,
	                RPC_MAX_PDU_FRAG_LEN,
	                RPC_MAX_PDU_FRAG_LEN,
	                assoc_gid,
	                ack_pipe_name,
	                0x1, 0x0, 0x0,
	                &hdr_rb.rpc_context[0].transfer[0]);
	} else {
		RPC_IFACE null_interface;
		ZERO_STRUCT(null_interface);
		/* Rejection reason: abstract syntax not supported */
		init_rpc_hdr_ba(&hdr_ba, RPC_MAX_PDU_FRAG_LEN,
					RPC_MAX_PDU_FRAG_LEN, assoc_gid,
					ack_pipe_name, 0x1, 0x2, 0x1,
					&null_interface);
		p->pipe_bound = False;
	}

	/*
	 * and marshall it.
	 */

	if(!smb_io_rpc_hdr_ba("", &hdr_ba, &out_hdr_ba, 0)) {
		DEBUG(0,("api_pipe_bind_req: marshalling of RPC_HDR_BA failed.\n"));
		goto err_exit;
	}

	/*
	 * Create the header, now we know the length.
	 */

	if (prs_offset(&out_auth)) {
		auth_len = prs_offset(&out_auth) - RPC_HDR_AUTH_LEN;
	}

	init_rpc_hdr(&p->hdr, RPC_BINDACK, RPC_FLG_FIRST | RPC_FLG_LAST,
			p->hdr.call_id,
			RPC_HEADER_LEN + prs_offset(&out_hdr_ba) + prs_offset(&out_auth),
			auth_len);

	/*
	 * Marshall the header into the outgoing PDU.
	 */

	if(!smb_io_rpc_hdr("", &p->hdr, &outgoing_rpc, 0)) {
		DEBUG(0,("api_pipe_bind_req: marshalling of RPC_HDR failed.\n"));
		goto err_exit;
	}

	/*
	 * Now add the RPC_HDR_BA and any auth needed.
	 */

	if(!prs_append_prs_data( &outgoing_rpc, &out_hdr_ba)) {
		DEBUG(0,("api_pipe_bind_req: append of RPC_HDR_BA failed.\n"));
		goto err_exit;
	}

	if (auth_len && !prs_append_prs_data( &outgoing_rpc, &out_auth)) {
		DEBUG(0,("api_pipe_bind_req: append of auth info failed.\n"));
		goto err_exit;
	}

	/*
	 * Setup the lengths for the initial reply.
	 */

	p->out_data.data_sent_length = 0;
	p->out_data.current_pdu_len = prs_offset(&outgoing_rpc);
	p->out_data.current_pdu_sent = 0;

	prs_mem_free(&out_hdr_ba);
	prs_mem_free(&out_auth);

	return True;

  err_exit:

	prs_mem_free(&outgoing_rpc);
	prs_mem_free(&out_hdr_ba);
	prs_mem_free(&out_auth);
	return setup_bind_nak(p);
}

/****************************************************************************
 Deal with an alter context call. Can be third part of 3 leg auth request for
 SPNEGO calls.
****************************************************************************/

BOOL api_pipe_alter_context(pipes_struct *p, prs_struct *rpc_in_p)
{
	RPC_HDR_BA hdr_ba;
	RPC_HDR_RB hdr_rb;
	RPC_HDR_AUTH auth_info;
	uint16 assoc_gid;
	fstring ack_pipe_name;
	prs_struct out_hdr_ba;
	prs_struct out_auth;
	prs_struct outgoing_rpc;
	int auth_len = 0;

	prs_init( &outgoing_rpc, 0, p->mem_ctx, MARSHALL);

	/* 
	 * Marshall directly into the outgoing PDU space. We
	 * must do this as we need to set to the bind response
	 * header and are never sending more than one PDU here.
	 */

	prs_give_memory( &outgoing_rpc, (char *)p->out_data.current_pdu, sizeof(p->out_data.current_pdu), False);

	/*
	 * Setup the memory to marshall the ba header, and the
	 * auth footers.
	 */

	if(!prs_init(&out_hdr_ba, 1024, p->mem_ctx, MARSHALL)) {
		DEBUG(0,("api_pipe_alter_context: malloc out_hdr_ba failed.\n"));
		prs_mem_free(&outgoing_rpc);
		return False;
	}

	if(!prs_init(&out_auth, 1024, p->mem_ctx, MARSHALL)) {
		DEBUG(0,("api_pipe_alter_context: malloc out_auth failed.\n"));
		prs_mem_free(&outgoing_rpc);
		prs_mem_free(&out_hdr_ba);
		return False;
	}

	DEBUG(5,("api_pipe_alter_context: decode request. %d\n", __LINE__));

	/* decode the alter context request */
	if(!smb_io_rpc_hdr_rb("", &hdr_rb, rpc_in_p, 0))  {
		DEBUG(0,("api_pipe_alter_context: unable to unmarshall RPC_HDR_RB struct.\n"));
		goto err_exit;
	}

	/* secondary address CAN be NULL
	 * as the specs say it's ignored.
	 * It MUST be NULL to have the spoolss working.
	 */
	fstrcpy(ack_pipe_name,"");

	DEBUG(5,("api_pipe_alter_context: make response. %d\n", __LINE__));

	/*
	 * Check if this is an authenticated alter context request.
	 */

	if (p->hdr.auth_len != 0) {
		/* 
		 * Decode the authentication verifier.
		 */

		if(!smb_io_rpc_hdr_auth("", &auth_info, rpc_in_p, 0)) {
			DEBUG(0,("api_pipe_alter_context: unable to unmarshall RPC_HDR_AUTH struct.\n"));
			goto err_exit;
		}

		/*
		 * Currently only the SPNEGO auth type uses the alter ctx
		 * response in place of the NTLMSSP auth3 type.
		 */

		if (auth_info.auth_type == RPC_SPNEGO_AUTH_TYPE) {
			/* We can only finish if the pipe is unbound. */
			if (!p->pipe_bound) {
				if (!pipe_spnego_auth_bind_continue(p, rpc_in_p, &auth_info, &out_auth)) {
					goto err_exit;
				}
			} else {
				goto err_exit;
			}
		}
	} else {
		ZERO_STRUCT(auth_info);
	}

	assoc_gid = hdr_rb.bba.assoc_gid ? hdr_rb.bba.assoc_gid : 0x53f0;

	/*
	 * Create the bind response struct.
	 */

	/* If the requested abstract synt uuid doesn't match our client pipe,
		reject the bind_ack & set the transfer interface synt to all 0's,
		ver 0 (observed when NT5 attempts to bind to abstract interfaces
		unknown to NT4)
		Needed when adding entries to a DACL from NT5 - SK */

	if(check_bind_req(p, &hdr_rb.rpc_context[0].abstract, &hdr_rb.rpc_context[0].transfer[0],
				hdr_rb.rpc_context[0].context_id )) {
		init_rpc_hdr_ba(&hdr_ba,
	                RPC_MAX_PDU_FRAG_LEN,
	                RPC_MAX_PDU_FRAG_LEN,
	                assoc_gid,
	                ack_pipe_name,
	                0x1, 0x0, 0x0,
	                &hdr_rb.rpc_context[0].transfer[0]);
	} else {
		RPC_IFACE null_interface;
		ZERO_STRUCT(null_interface);
		/* Rejection reason: abstract syntax not supported */
		init_rpc_hdr_ba(&hdr_ba, RPC_MAX_PDU_FRAG_LEN,
					RPC_MAX_PDU_FRAG_LEN, assoc_gid,
					ack_pipe_name, 0x1, 0x2, 0x1,
					&null_interface);
		p->pipe_bound = False;
	}

	/*
	 * and marshall it.
	 */

	if(!smb_io_rpc_hdr_ba("", &hdr_ba, &out_hdr_ba, 0)) {
		DEBUG(0,("api_pipe_alter_context: marshalling of RPC_HDR_BA failed.\n"));
		goto err_exit;
	}

	/*
	 * Create the header, now we know the length.
	 */

	if (prs_offset(&out_auth)) {
		auth_len = prs_offset(&out_auth) - RPC_HDR_AUTH_LEN;
	}

	init_rpc_hdr(&p->hdr, RPC_ALTCONTRESP, RPC_FLG_FIRST | RPC_FLG_LAST,
			p->hdr.call_id,
			RPC_HEADER_LEN + prs_offset(&out_hdr_ba) + prs_offset(&out_auth),
			auth_len);

	/*
	 * Marshall the header into the outgoing PDU.
	 */

	if(!smb_io_rpc_hdr("", &p->hdr, &outgoing_rpc, 0)) {
		DEBUG(0,("api_pipe_alter_context: marshalling of RPC_HDR failed.\n"));
		goto err_exit;
	}

	/*
	 * Now add the RPC_HDR_BA and any auth needed.
	 */

	if(!prs_append_prs_data( &outgoing_rpc, &out_hdr_ba)) {
		DEBUG(0,("api_pipe_alter_context: append of RPC_HDR_BA failed.\n"));
		goto err_exit;
	}

	if (auth_len && !prs_append_prs_data( &outgoing_rpc, &out_auth)) {
		DEBUG(0,("api_pipe_alter_context: append of auth info failed.\n"));
		goto err_exit;
	}

	/*
	 * Setup the lengths for the initial reply.
	 */

	p->out_data.data_sent_length = 0;
	p->out_data.current_pdu_len = prs_offset(&outgoing_rpc);
	p->out_data.current_pdu_sent = 0;

	prs_mem_free(&out_hdr_ba);
	prs_mem_free(&out_auth);

	return True;

  err_exit:

	prs_mem_free(&outgoing_rpc);
	prs_mem_free(&out_hdr_ba);
	prs_mem_free(&out_auth);
	return setup_bind_nak(p);
}

/****************************************************************************
 Deal with NTLMSSP sign & seal processing on an RPC request.
****************************************************************************/

BOOL api_pipe_ntlmssp_auth_process(pipes_struct *p, prs_struct *rpc_in,
					uint32 *p_ss_padding_len, NTSTATUS *pstatus)
{
	RPC_HDR_AUTH auth_info;
	uint32 auth_len = p->hdr.auth_len;
	uint32 save_offset = prs_offset(rpc_in);
	AUTH_NTLMSSP_STATE *a = p->auth.a_u.auth_ntlmssp_state;
	unsigned char *data = NULL;
	size_t data_len;
	unsigned char *full_packet_data = NULL;
	size_t full_packet_data_len;
	DATA_BLOB auth_blob;
	
	*pstatus = NT_STATUS_OK;

	if (p->auth.auth_level == PIPE_AUTH_LEVEL_NONE || p->auth.auth_level == PIPE_AUTH_LEVEL_CONNECT) {
		return True;
	}

	if (!a) {
		*pstatus = NT_STATUS_INVALID_PARAMETER;
		return False;
	}

	/* Ensure there's enough data for an authenticated request. */
	if ((auth_len > RPC_MAX_SIGN_SIZE) ||
			(RPC_HEADER_LEN + RPC_HDR_REQ_LEN + RPC_HDR_AUTH_LEN + auth_len > p->hdr.frag_len)) {
		DEBUG(0,("api_pipe_ntlmssp_auth_process: auth_len %u is too large.\n",
			(unsigned int)auth_len ));
		*pstatus = NT_STATUS_INVALID_PARAMETER;
		return False;
	}

	/*
	 * We need the full packet data + length (minus auth stuff) as well as the packet data + length
	 * after the RPC header. 
 	 * We need to pass in the full packet (minus auth len) to the NTLMSSP sign and check seal
	 * functions as NTLMv2 checks the rpc headers also.
	 */

	data = (unsigned char *)(prs_data_p(rpc_in) + RPC_HDR_REQ_LEN);
	data_len = (size_t)(p->hdr.frag_len - RPC_HEADER_LEN - RPC_HDR_REQ_LEN - RPC_HDR_AUTH_LEN - auth_len);

	full_packet_data = p->in_data.current_in_pdu;
	full_packet_data_len = p->hdr.frag_len - auth_len;

	/* Pull the auth header and the following data into a blob. */
	if(!prs_set_offset(rpc_in, RPC_HDR_REQ_LEN + data_len)) {
		DEBUG(0,("api_pipe_ntlmssp_auth_process: cannot move offset to %u.\n",
			(unsigned int)RPC_HDR_REQ_LEN + (unsigned int)data_len ));
		*pstatus = NT_STATUS_INVALID_PARAMETER;
		return False;
	}

	if(!smb_io_rpc_hdr_auth("hdr_auth", &auth_info, rpc_in, 0)) {
		DEBUG(0,("api_pipe_ntlmssp_auth_process: failed to unmarshall RPC_HDR_AUTH.\n"));
		*pstatus = NT_STATUS_INVALID_PARAMETER;
		return False;
	}

	auth_blob.data = (unsigned char *)prs_data_p(rpc_in) + prs_offset(rpc_in);
	auth_blob.length = auth_len;
	
	switch (p->auth.auth_level) {
		case PIPE_AUTH_LEVEL_PRIVACY:
			/* Data is encrypted. */
			*pstatus = ntlmssp_unseal_packet(a->ntlmssp_state,
							data, data_len,
							full_packet_data,
							full_packet_data_len,
							&auth_blob);
			if (!NT_STATUS_IS_OK(*pstatus)) {
				return False;
			}
			break;
		case PIPE_AUTH_LEVEL_INTEGRITY:
			/* Data is signed. */
			*pstatus = ntlmssp_check_packet(a->ntlmssp_state,
							data, data_len,
							full_packet_data,
							full_packet_data_len,
							&auth_blob);
			if (!NT_STATUS_IS_OK(*pstatus)) {
				return False;
			}
			break;
		default:
			*pstatus = NT_STATUS_INVALID_PARAMETER;
			return False;
	}

	/*
	 * Return the current pointer to the data offset.
	 */

	if(!prs_set_offset(rpc_in, save_offset)) {
		DEBUG(0,("api_pipe_auth_process: failed to set offset back to %u\n",
			(unsigned int)save_offset ));
		*pstatus = NT_STATUS_INVALID_PARAMETER;
		return False;
	}

	/*
	 * Remember the padding length. We must remove it from the real data
	 * stream once the sign/seal is done.
	 */

	*p_ss_padding_len = auth_info.auth_pad_len;

	return True;
}

/****************************************************************************
 Deal with schannel processing on an RPC request.
****************************************************************************/

BOOL api_pipe_schannel_process(pipes_struct *p, prs_struct *rpc_in, uint32 *p_ss_padding_len)
{
	uint32 data_len;
	uint32 auth_len;
	uint32 save_offset = prs_offset(rpc_in);
	RPC_HDR_AUTH auth_info;
	RPC_AUTH_SCHANNEL_CHK schannel_chk;

	auth_len = p->hdr.auth_len;

	if (auth_len != RPC_AUTH_SCHANNEL_SIGN_OR_SEAL_CHK_LEN) {
		DEBUG(0,("Incorrect auth_len %u.\n", (unsigned int)auth_len ));
		return False;
	}

	/*
	 * The following is that length of the data we must verify or unseal.
	 * This doesn't include the RPC headers or the auth_len or the RPC_HDR_AUTH_LEN
	 * preceeding the auth_data.
	 */

	if (p->hdr.frag_len < RPC_HEADER_LEN + RPC_HDR_REQ_LEN + RPC_HDR_AUTH_LEN + auth_len) {
		DEBUG(0,("Incorrect frag %u, auth %u.\n",
			(unsigned int)p->hdr.frag_len,
			(unsigned int)auth_len ));
		return False;
	}

	data_len = p->hdr.frag_len - RPC_HEADER_LEN - RPC_HDR_REQ_LEN - 
		RPC_HDR_AUTH_LEN - auth_len;
	
	DEBUG(5,("data %d auth %d\n", data_len, auth_len));

	if(!prs_set_offset(rpc_in, RPC_HDR_REQ_LEN + data_len)) {
		DEBUG(0,("cannot move offset to %u.\n",
			 (unsigned int)RPC_HDR_REQ_LEN + data_len ));
		return False;
	}

	if(!smb_io_rpc_hdr_auth("hdr_auth", &auth_info, rpc_in, 0)) {
		DEBUG(0,("failed to unmarshall RPC_HDR_AUTH.\n"));
		return False;
	}

	if (auth_info.auth_type != RPC_SCHANNEL_AUTH_TYPE) {
		DEBUG(0,("Invalid auth info %d on schannel\n",
			 auth_info.auth_type));
		return False;
	}

	if(!smb_io_rpc_auth_schannel_chk("", RPC_AUTH_SCHANNEL_SIGN_OR_SEAL_CHK_LEN, &schannel_chk, rpc_in, 0)) {
		DEBUG(0,("failed to unmarshal RPC_AUTH_SCHANNEL_CHK.\n"));
		return False;
	}

	if (!schannel_decode(p->auth.a_u.schannel_auth,
			   p->auth.auth_level,
			   SENDER_IS_INITIATOR,
			   &schannel_chk,
			   prs_data_p(rpc_in)+RPC_HDR_REQ_LEN, data_len)) {
		DEBUG(3,("failed to decode PDU\n"));
		return False;
	}

	/*
	 * Return the current pointer to the data offset.
	 */

	if(!prs_set_offset(rpc_in, save_offset)) {
		DEBUG(0,("failed to set offset back to %u\n",
			 (unsigned int)save_offset ));
		return False;
	}

	/* The sequence number gets incremented on both send and receive. */
	p->auth.a_u.schannel_auth->seq_num++;

	/*
	 * Remember the padding length. We must remove it from the real data
	 * stream once the sign/seal is done.
	 */

	*p_ss_padding_len = auth_info.auth_pad_len;

	return True;
}

/****************************************************************************
 Return a user struct for a pipe user.
****************************************************************************/

struct current_user *get_current_user(struct current_user *user, pipes_struct *p)
{
	if (p->pipe_bound &&
			(p->auth.auth_type == PIPE_AUTH_TYPE_NTLMSSP ||
			(p->auth.auth_type == PIPE_AUTH_TYPE_SPNEGO_NTLMSSP))) {
		memcpy(user, &p->pipe_user, sizeof(struct current_user));
	} else {
		memcpy(user, &current_user, sizeof(struct current_user));
	}

	return user;
}

/****************************************************************************
 Find the set of RPC functions associated with this context_id
****************************************************************************/

static PIPE_RPC_FNS* find_pipe_fns_by_context( PIPE_RPC_FNS *list, uint32 context_id )
{
	PIPE_RPC_FNS *fns = NULL;
	PIPE_RPC_FNS *tmp = NULL;
	
	if ( !list ) {
		DEBUG(0,("find_pipe_fns_by_context: ERROR!  No context list for pipe!\n"));
		return NULL;
	}
	
	for (tmp=list; tmp; tmp=tmp->next ) {
		if ( tmp->context_id == context_id )
			break;
	}
	
	fns = tmp;
	
	return fns;
}

/****************************************************************************
 Memory cleanup.
****************************************************************************/

void free_pipe_rpc_context( PIPE_RPC_FNS *list )
{
	PIPE_RPC_FNS *tmp = list;
	PIPE_RPC_FNS *tmp2;
		
	while (tmp) {
		tmp2 = tmp->next;
		SAFE_FREE(tmp);
		tmp = tmp2;
	}

	return;	
}

/****************************************************************************
 Find the correct RPC function to call for this request.
 If the pipe is authenticated then become the correct UNIX user
 before doing the call.
****************************************************************************/

BOOL api_pipe_request(pipes_struct *p)
{
	BOOL ret = False;
	BOOL changed_user = False;
	PIPE_RPC_FNS *pipe_fns;
	
	if (p->pipe_bound &&
			((p->auth.auth_type == PIPE_AUTH_TYPE_NTLMSSP) ||
			 (p->auth.auth_type == PIPE_AUTH_TYPE_SPNEGO_NTLMSSP))) {
		if(!become_authenticated_pipe_user(p)) {
			prs_mem_free(&p->out_data.rdata);
			return False;
		}
		changed_user = True;
	}

	DEBUG(5, ("Requested \\PIPE\\%s\n", p->name));
	
	/* get the set of RPC functions for this context */
	
	pipe_fns = find_pipe_fns_by_context(p->contexts, p->hdr_req.context_id);
	
	if ( pipe_fns ) {
		set_current_rpc_talloc(p->mem_ctx);
		ret = api_rpcTNP(p, p->name, pipe_fns->cmds, pipe_fns->n_cmds);
		set_current_rpc_talloc(NULL);	
	}
	else {
		DEBUG(0,("api_pipe_request: No rpc function table associated with context [%d] on pipe [%s]\n",
			p->hdr_req.context_id, p->name));
	}

	if (changed_user) {
		unbecome_authenticated_pipe_user();
	}

	return ret;
}

/*******************************************************************
 Calls the underlying RPC function for a named pipe.
 ********************************************************************/

BOOL api_rpcTNP(pipes_struct *p, const char *rpc_name, 
		const struct api_struct *api_rpc_cmds, int n_cmds)
{
	int fn_num;
	fstring name;
	uint32 offset1, offset2;
 
	/* interpret the command */
	DEBUG(4,("api_rpcTNP: %s op 0x%x - ", rpc_name, p->hdr_req.opnum));

	slprintf(name, sizeof(name)-1, "in_%s", rpc_name);
	prs_dump(name, p->hdr_req.opnum, &p->in_data.data);

	for (fn_num = 0; fn_num < n_cmds; fn_num++) {
		if (api_rpc_cmds[fn_num].opnum == p->hdr_req.opnum && api_rpc_cmds[fn_num].fn != NULL) {
			DEBUG(3,("api_rpcTNP: rpc command: %s\n", api_rpc_cmds[fn_num].name));
			break;
		}
	}

	if (fn_num == n_cmds) {
		/*
		 * For an unknown RPC just return a fault PDU but
		 * return True to allow RPC's on the pipe to continue
		 * and not put the pipe into fault state. JRA.
		 */
		DEBUG(4, ("unknown\n"));
		setup_fault_pdu(p, NT_STATUS(DCERPC_FAULT_OP_RNG_ERROR));
		return True;
	}

	offset1 = prs_offset(&p->out_data.rdata);

        DEBUG(6, ("api_rpc_cmds[%d].fn == %p\n", 
                fn_num, api_rpc_cmds[fn_num].fn));
	/* do the actual command */
	if(!api_rpc_cmds[fn_num].fn(p)) {
		DEBUG(0,("api_rpcTNP: %s: %s failed.\n", rpc_name, api_rpc_cmds[fn_num].name));
		prs_mem_free(&p->out_data.rdata);
		return False;
	}

	if (p->bad_handle_fault_state) {
		DEBUG(4,("api_rpcTNP: bad handle fault return.\n"));
		p->bad_handle_fault_state = False;
		setup_fault_pdu(p, NT_STATUS(DCERPC_FAULT_CONTEXT_MISMATCH));
		return True;
	}

	slprintf(name, sizeof(name)-1, "out_%s", rpc_name);
	offset2 = prs_offset(&p->out_data.rdata);
	prs_set_offset(&p->out_data.rdata, offset1);
	prs_dump(name, p->hdr_req.opnum, &p->out_data.rdata);
	prs_set_offset(&p->out_data.rdata, offset2);

	DEBUG(5,("api_rpcTNP: called %s successfully\n", rpc_name));

	/* Check for buffer underflow in rpc parsing */

	if ((DEBUGLEVEL >= 10) && 
	    (prs_offset(&p->in_data.data) != prs_data_size(&p->in_data.data))) {
		size_t data_len = prs_data_size(&p->in_data.data) - prs_offset(&p->in_data.data);
		char *data = SMB_MALLOC(data_len);

		DEBUG(10, ("api_rpcTNP: rpc input buffer underflow (parse error?)\n"));
		if (data) {
			prs_uint8s(False, "", &p->in_data.data, 0, (unsigned char *)data, (uint32)data_len);
			SAFE_FREE(data);
		}

	}

	return True;
}

/*******************************************************************
*******************************************************************/

void get_pipe_fns( int idx, struct api_struct **fns, int *n_fns )
{
	struct api_struct *cmds = NULL;
	int               n_cmds = 0;

	switch ( idx ) {
		case PI_LSARPC:
			lsa_get_pipe_fns( &cmds, &n_cmds );
			break;
		case PI_LSARPC_DS:
			lsa_ds_get_pipe_fns( &cmds, &n_cmds );
			break;
		case PI_SAMR:
			samr_get_pipe_fns( &cmds, &n_cmds );
			break;
		case PI_NETLOGON:
			netlog_get_pipe_fns( &cmds, &n_cmds );
			break;
		case PI_SRVSVC:
			srvsvc_get_pipe_fns( &cmds, &n_cmds );
			break;
		case PI_WKSSVC:
			wkssvc_get_pipe_fns( &cmds, &n_cmds );
			break;
		case PI_WINREG:
			reg_get_pipe_fns( &cmds, &n_cmds );
			break;
		case PI_SPOOLSS:
			spoolss_get_pipe_fns( &cmds, &n_cmds );
			break;
		case PI_NETDFS:
			netdfs_get_pipe_fns( &cmds, &n_cmds );
			break;
		case PI_SVCCTL:
			svcctl_get_pipe_fns( &cmds, &n_cmds );
			break;
	        case PI_EVENTLOG:
			eventlog_get_pipe_fns( &cmds, &n_cmds );
			break;
		case PI_NTSVCS:
			ntsvcs_get_pipe_fns( &cmds, &n_cmds );
			break;
#ifdef DEVELOPER
		case PI_ECHO:
			echo_get_pipe_fns( &cmds, &n_cmds );
			break;
#endif
		default:
			DEBUG(0,("get_pipe_fns: Unknown pipe index! [%d]\n", idx));
	}

	*fns = cmds;
	*n_fns = n_cmds;

	return;
}
