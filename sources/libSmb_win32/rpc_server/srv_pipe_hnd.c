/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Andrew Tridgell              1992-1998,
 *  Largely re-written : 2005
 *  Copyright (C) Jeremy Allison		1998 - 2005
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

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

#define	PIPE		"\\PIPE\\"
#define	PIPELEN		strlen(PIPE)

static smb_np_struct *chain_p;
static int pipes_open;

/*
 * Sometimes I can't decide if I hate Windows printer driver
 * writers more than I hate the Windows spooler service driver
 * writers. This gets around a combination of bugs in the spooler
 * and the HP 8500 PCL driver that causes a spooler spin. JRA.
 *
 * bumped up from 20 -> 64 after viewing traffic from WordPerfect
 * 2002 running on NT 4.- SP6
 * bumped up from 64 -> 256 after viewing traffic from con2prt
 * for lots of printers on a WinNT 4.x SP6 box.
 */
 
#ifndef MAX_OPEN_SPOOLSS_PIPES
#define MAX_OPEN_SPOOLSS_PIPES 256
#endif
static int current_spoolss_pipes_open;

static smb_np_struct *Pipes;
static pipes_struct *InternalPipes;
static struct bitmap *bmap;

/* TODO
 * the following prototypes are declared here to avoid
 * code being moved about too much for a patch to be
 * disrupted / less obvious.
 *
 * these functions, and associated functions that they
 * call, should be moved behind a .so module-loading
 * system _anyway_.  so that's the next step...
 */

static ssize_t read_from_internal_pipe(void *np_conn, char *data, size_t n,
		BOOL *is_data_outstanding);
static ssize_t write_to_internal_pipe(void *np_conn, char *data, size_t n);
static BOOL close_internal_rpc_pipe_hnd(void *np_conn);
static void *make_internal_rpc_pipe_p(char *pipe_name, 
			      connection_struct *conn, uint16 vuid);

/****************************************************************************
 Pipe iterator functions.
****************************************************************************/

smb_np_struct *get_first_pipe(void)
{
	return Pipes;
}

smb_np_struct *get_next_pipe(smb_np_struct *p)
{
	return p->next;
}

/****************************************************************************
 Internal Pipe iterator functions.
****************************************************************************/

pipes_struct *get_first_internal_pipe(void)
{
	return InternalPipes;
}

pipes_struct *get_next_internal_pipe(pipes_struct *p)
{
	return p->next;
}

/* this must be larger than the sum of the open files and directories */
static int pipe_handle_offset;

/****************************************************************************
 Set the pipe_handle_offset. Called from smbd/files.c
****************************************************************************/

void set_pipe_handle_offset(int max_open_files)
{
	if(max_open_files < 0x7000) {
		pipe_handle_offset = 0x7000;
	} else {
		pipe_handle_offset = max_open_files + 10; /* For safety. :-) */
	}
}

/****************************************************************************
 Reset pipe chain handle number.
****************************************************************************/

void reset_chain_p(void)
{
	chain_p = NULL;
}

/****************************************************************************
 Initialise pipe handle states.
****************************************************************************/

void init_rpc_pipe_hnd(void)
{
	bmap = bitmap_allocate(MAX_OPEN_PIPES);
	if (!bmap) {
		exit_server("out of memory in init_rpc_pipe_hnd");
	}
}

/****************************************************************************
 Initialise an outgoing packet.
****************************************************************************/

static BOOL pipe_init_outgoing_data(pipes_struct *p)
{
	output_data *o_data = &p->out_data;

	/* Reset the offset counters. */
	o_data->data_sent_length = 0;
	o_data->current_pdu_len = 0;
	o_data->current_pdu_sent = 0;

	memset(o_data->current_pdu, '\0', sizeof(o_data->current_pdu));

	/* Free any memory in the current return data buffer. */
	prs_mem_free(&o_data->rdata);

	/*
	 * Initialize the outgoing RPC data buffer.
	 * we will use this as the raw data area for replying to rpc requests.
	 */	
	if(!prs_init(&o_data->rdata, RPC_MAX_PDU_FRAG_LEN, p->mem_ctx, MARSHALL)) {
		DEBUG(0,("pipe_init_outgoing_data: malloc fail.\n"));
		return False;
	}

	return True;
}

/****************************************************************************
 Find first available pipe slot.
****************************************************************************/

smb_np_struct *open_rpc_pipe_p(char *pipe_name, 
			      connection_struct *conn, uint16 vuid)
{
	int i;
	smb_np_struct *p, *p_it;
	static int next_pipe;
	BOOL is_spoolss_pipe = False;

	DEBUG(4,("Open pipe requested %s (pipes_open=%d)\n",
		 pipe_name, pipes_open));

	if (strstr(pipe_name, "spoolss")) {
		is_spoolss_pipe = True;
	}
 
	if (is_spoolss_pipe && current_spoolss_pipes_open >= MAX_OPEN_SPOOLSS_PIPES) {
		DEBUG(10,("open_rpc_pipe_p: spooler bug workaround. Denying open on pipe %s\n",
			pipe_name ));
		return NULL;
	}

	/* not repeating pipe numbers makes it easier to track things in 
	   log files and prevents client bugs where pipe numbers are reused
	   over connection restarts */

	if (next_pipe == 0) {
		next_pipe = (sys_getpid() ^ time(NULL)) % MAX_OPEN_PIPES;
	}

	i = bitmap_find(bmap, next_pipe);

	if (i == -1) {
		DEBUG(0,("ERROR! Out of pipe structures\n"));
		return NULL;
	}

	next_pipe = (i+1) % MAX_OPEN_PIPES;

	for (p = Pipes; p; p = p->next) {
		DEBUG(5,("open_rpc_pipe_p: name %s pnum=%x\n", p->name, p->pnum));  
	}

	p = SMB_MALLOC_P(smb_np_struct);
	if (!p) {
		DEBUG(0,("ERROR! no memory for pipes_struct!\n"));
		return NULL;
	}

	ZERO_STRUCTP(p);

	/* add a dso mechanism instead of this, here */

	p->namedpipe_create = make_internal_rpc_pipe_p;
	p->namedpipe_read = read_from_internal_pipe;
	p->namedpipe_write = write_to_internal_pipe;
	p->namedpipe_close = close_internal_rpc_pipe_hnd;

	p->np_state = p->namedpipe_create(pipe_name, conn, vuid);

	if (p->np_state == NULL) {
		DEBUG(0,("open_rpc_pipe_p: make_internal_rpc_pipe_p failed.\n"));
		SAFE_FREE(p);
		return NULL;
	}

	DLIST_ADD(Pipes, p);

	/*
	 * Initialize the incoming RPC data buffer with one PDU worth of memory.
	 * We cheat here and say we're marshalling, as we intend to add incoming
	 * data directly into the prs_struct and we want it to auto grow. We will
	 * change the type to UNMARSALLING before processing the stream.
	 */

	bitmap_set(bmap, i);
	i += pipe_handle_offset;

	pipes_open++;

	p->pnum = i;

	p->open = True;
	p->device_state = 0;
	p->priority = 0;
	p->conn = conn;
	p->vuid  = vuid;

	p->max_trans_reply = 0;
	
	fstrcpy(p->name, pipe_name);
	
	DEBUG(4,("Opened pipe %s with handle %x (pipes_open=%d)\n",
		 pipe_name, i, pipes_open));
	
	chain_p = p;
	
	/* Iterate over p_it as a temp variable, to display all open pipes */ 
	for (p_it = Pipes; p_it; p_it = p_it->next) {
		DEBUG(5,("open pipes: name %s pnum=%x\n", p_it->name, p_it->pnum));  
	}

	return chain_p;
}

/****************************************************************************
 Make an internal namedpipes structure
****************************************************************************/

static void *make_internal_rpc_pipe_p(char *pipe_name, 
			      connection_struct *conn, uint16 vuid)
{
	pipes_struct *p;
	user_struct *vuser = get_valid_user_struct(vuid);

	DEBUG(4,("Create pipe requested %s\n", pipe_name));

	if (!vuser && vuid != UID_FIELD_INVALID) {
		DEBUG(0,("ERROR! vuid %d did not map to a valid vuser struct!\n", vuid));
		return NULL;
	}

	p = SMB_MALLOC_P(pipes_struct);

	if (!p) {
		DEBUG(0,("ERROR! no memory for pipes_struct!\n"));
		return NULL;
	}

	ZERO_STRUCTP(p);

	if ((p->mem_ctx = talloc_init("pipe %s %p", pipe_name, p)) == NULL) {
		DEBUG(0,("open_rpc_pipe_p: talloc_init failed.\n"));
		SAFE_FREE(p);
		return NULL;
	}

	if ((p->pipe_state_mem_ctx = talloc_init("pipe_state %s %p", pipe_name, p)) == NULL) {
		DEBUG(0,("open_rpc_pipe_p: talloc_init failed.\n"));
		talloc_destroy(p->mem_ctx);
		SAFE_FREE(p);
		return NULL;
	}

	if (!init_pipe_handle_list(p, pipe_name)) {
		DEBUG(0,("open_rpc_pipe_p: init_pipe_handles failed.\n"));
		talloc_destroy(p->mem_ctx);
		talloc_destroy(p->pipe_state_mem_ctx);
		SAFE_FREE(p);
		return NULL;
	}

	/*
	 * Initialize the incoming RPC data buffer with one PDU worth of memory.
	 * We cheat here and say we're marshalling, as we intend to add incoming
	 * data directly into the prs_struct and we want it to auto grow. We will
	 * change the type to UNMARSALLING before processing the stream.
	 */

	if(!prs_init(&p->in_data.data, RPC_MAX_PDU_FRAG_LEN, p->mem_ctx, MARSHALL)) {
		DEBUG(0,("open_rpc_pipe_p: malloc fail for in_data struct.\n"));
		talloc_destroy(p->mem_ctx);
		talloc_destroy(p->pipe_state_mem_ctx);
		close_policy_by_pipe(p);
		SAFE_FREE(p);
		return NULL;
	}

	DLIST_ADD(InternalPipes, p);

	p->conn = conn;

	p->vuid  = vuid;

	p->endian = RPC_LITTLE_ENDIAN;

	ZERO_STRUCT(p->pipe_user);

	p->pipe_user.ut.uid = (uid_t)-1;
	p->pipe_user.ut.gid = (gid_t)-1;
	
	/* Store the session key and NT_TOKEN */
	if (vuser) {
		p->session_key = data_blob(vuser->session_key.data, vuser->session_key.length);
		p->pipe_user.nt_user_token = dup_nt_token(
			NULL, vuser->nt_user_token);
	}

	/*
	 * Initialize the outgoing RPC data buffer with no memory.
	 */	
	prs_init(&p->out_data.rdata, 0, p->mem_ctx, MARSHALL);
	
	fstrcpy(p->name, pipe_name);
	
	DEBUG(4,("Created internal pipe %s (pipes_open=%d)\n",
		 pipe_name, pipes_open));

	return (void*)p;
}

/****************************************************************************
 Sets the fault state on incoming packets.
****************************************************************************/

static void set_incoming_fault(pipes_struct *p)
{
	prs_mem_free(&p->in_data.data);
	p->in_data.pdu_needed_len = 0;
	p->in_data.pdu_received_len = 0;
	p->fault_state = True;
	DEBUG(10,("set_incoming_fault: Setting fault state on pipe %s : vuid = 0x%x\n",
		p->name, p->vuid ));
}

/****************************************************************************
 Ensures we have at least RPC_HEADER_LEN amount of data in the incoming buffer.
****************************************************************************/

static ssize_t fill_rpc_header(pipes_struct *p, char *data, size_t data_to_copy)
{
	size_t len_needed_to_complete_hdr = MIN(data_to_copy, RPC_HEADER_LEN - p->in_data.pdu_received_len);

	DEBUG(10,("fill_rpc_header: data_to_copy = %u, len_needed_to_complete_hdr = %u, receive_len = %u\n",
			(unsigned int)data_to_copy, (unsigned int)len_needed_to_complete_hdr,
			(unsigned int)p->in_data.pdu_received_len ));

	memcpy((char *)&p->in_data.current_in_pdu[p->in_data.pdu_received_len], data, len_needed_to_complete_hdr);
	p->in_data.pdu_received_len += len_needed_to_complete_hdr;

	return (ssize_t)len_needed_to_complete_hdr;
}

/****************************************************************************
 Unmarshalls a new PDU header. Assumes the raw header data is in current_in_pdu.
****************************************************************************/

static ssize_t unmarshall_rpc_header(pipes_struct *p)
{
	/*
	 * Unmarshall the header to determine the needed length.
	 */

	prs_struct rpc_in;

	if(p->in_data.pdu_received_len != RPC_HEADER_LEN) {
		DEBUG(0,("unmarshall_rpc_header: assert on rpc header length failed.\n"));
		set_incoming_fault(p);
		return -1;
	}

	prs_init( &rpc_in, 0, p->mem_ctx, UNMARSHALL);
	prs_set_endian_data( &rpc_in, p->endian);

	prs_give_memory( &rpc_in, (char *)&p->in_data.current_in_pdu[0],
					p->in_data.pdu_received_len, False);

	/*
	 * Unmarshall the header as this will tell us how much
	 * data we need to read to get the complete pdu.
	 * This also sets the endian flag in rpc_in.
	 */

	if(!smb_io_rpc_hdr("", &p->hdr, &rpc_in, 0)) {
		DEBUG(0,("unmarshall_rpc_header: failed to unmarshall RPC_HDR.\n"));
		set_incoming_fault(p);
		prs_mem_free(&rpc_in);
		return -1;
	}

	/*
	 * Validate the RPC header.
	 */

	if(p->hdr.major != 5 && p->hdr.minor != 0) {
		DEBUG(0,("unmarshall_rpc_header: invalid major/minor numbers in RPC_HDR.\n"));
		set_incoming_fault(p);
		prs_mem_free(&rpc_in);
		return -1;
	}

	/*
	 * If there's not data in the incoming buffer this should be the start of a new RPC.
	 */

	if(prs_offset(&p->in_data.data) == 0) {

		/*
		 * AS/U doesn't set FIRST flag in a BIND packet it seems.
		 */

		if ((p->hdr.pkt_type == RPC_REQUEST) && !(p->hdr.flags & RPC_FLG_FIRST)) {
			/*
			 * Ensure that the FIRST flag is set. If not then we have
			 * a stream missmatch.
			 */

			DEBUG(0,("unmarshall_rpc_header: FIRST flag not set in first PDU !\n"));
			set_incoming_fault(p);
			prs_mem_free(&rpc_in);
			return -1;
		}

		/*
		 * If this is the first PDU then set the endianness
		 * flag in the pipe. We will need this when parsing all
		 * data in this RPC.
		 */

		p->endian = rpc_in.bigendian_data;

		DEBUG(5,("unmarshall_rpc_header: using %sendian RPC\n",
				p->endian == RPC_LITTLE_ENDIAN ? "little-" : "big-" ));

	} else {

		/*
		 * If this is *NOT* the first PDU then check the endianness
		 * flag in the pipe is the same as that in the PDU.
		 */

		if (p->endian != rpc_in.bigendian_data) {
			DEBUG(0,("unmarshall_rpc_header: FIRST endianness flag (%d) different in next PDU !\n", (int)p->endian));
			set_incoming_fault(p);
			prs_mem_free(&rpc_in);
			return -1;
		}
	}

	/*
	 * Ensure that the pdu length is sane.
	 */

	if((p->hdr.frag_len < RPC_HEADER_LEN) || (p->hdr.frag_len > RPC_MAX_PDU_FRAG_LEN)) {
		DEBUG(0,("unmarshall_rpc_header: assert on frag length failed.\n"));
		set_incoming_fault(p);
		prs_mem_free(&rpc_in);
		return -1;
	}

	DEBUG(10,("unmarshall_rpc_header: type = %u, flags = %u\n", (unsigned int)p->hdr.pkt_type,
			(unsigned int)p->hdr.flags ));

	p->in_data.pdu_needed_len = (uint32)p->hdr.frag_len - RPC_HEADER_LEN;

	prs_mem_free(&rpc_in);

	return 0; /* No extra data processed. */
}

/****************************************************************************
 Call this to free any talloc'ed memory. Do this before and after processing
 a complete PDU.
****************************************************************************/

static void free_pipe_context(pipes_struct *p)
{
	if (p->mem_ctx) {
		DEBUG(3,("free_pipe_context: destroying talloc pool of size "
			 "%lu\n", (unsigned long)talloc_total_size(p->mem_ctx) ));
		talloc_free_children(p->mem_ctx);
	} else {
		p->mem_ctx = talloc_init("pipe %s %p", p->name, p);
		if (p->mem_ctx == NULL) {
			p->fault_state = True;
		}
	}
}

/****************************************************************************
 Processes a request pdu. This will do auth processing if needed, and
 appends the data into the complete stream if the LAST flag is not set.
****************************************************************************/

static BOOL process_request_pdu(pipes_struct *p, prs_struct *rpc_in_p)
{
	uint32 ss_padding_len = 0;
	size_t data_len = p->hdr.frag_len - RPC_HEADER_LEN - RPC_HDR_REQ_LEN -
				(p->hdr.auth_len ? RPC_HDR_AUTH_LEN : 0) - p->hdr.auth_len;

	if(!p->pipe_bound) {
		DEBUG(0,("process_request_pdu: rpc request with no bind.\n"));
		set_incoming_fault(p);
		return False;
	}

	/*
	 * Check if we need to do authentication processing.
	 * This is only done on requests, not binds.
	 */

	/*
	 * Read the RPC request header.
	 */

	if(!smb_io_rpc_hdr_req("req", &p->hdr_req, rpc_in_p, 0)) {
		DEBUG(0,("process_request_pdu: failed to unmarshall RPC_HDR_REQ.\n"));
		set_incoming_fault(p);
		return False;
	}

	switch(p->auth.auth_type) {
		case PIPE_AUTH_TYPE_NONE:
			break;

		case PIPE_AUTH_TYPE_SPNEGO_NTLMSSP:
		case PIPE_AUTH_TYPE_NTLMSSP:
		{
			NTSTATUS status;
			if(!api_pipe_ntlmssp_auth_process(p, rpc_in_p, &ss_padding_len, &status)) {
				DEBUG(0,("process_request_pdu: failed to do auth processing.\n"));
				DEBUG(0,("process_request_pdu: error was %s.\n", nt_errstr(status) ));
				set_incoming_fault(p);
				return False;
			}
			break;
		}

		case PIPE_AUTH_TYPE_SCHANNEL:
			if (!api_pipe_schannel_process(p, rpc_in_p, &ss_padding_len)) {
				DEBUG(3,("process_request_pdu: failed to do schannel processing.\n"));
				set_incoming_fault(p);
				return False;
			}
			break;

		default:
			DEBUG(0,("process_request_pdu: unknown auth type %u set.\n", (unsigned int)p->auth.auth_type ));
			set_incoming_fault(p);
			return False;
	}

	/* Now we've done the sign/seal we can remove any padding data. */
	if (data_len > ss_padding_len) {
		data_len -= ss_padding_len;
	}

	/*
	 * Check the data length doesn't go over the 15Mb limit.
	 * increased after observing a bug in the Windows NT 4.0 SP6a
	 * spoolsv.exe when the response to a GETPRINTERDRIVER2 RPC
	 * will not fit in the initial buffer of size 0x1068   --jerry 22/01/2002
	 */
	
	if(prs_offset(&p->in_data.data) + data_len > 15*1024*1024) {
		DEBUG(0,("process_request_pdu: rpc data buffer too large (%u) + (%u)\n",
				(unsigned int)prs_data_size(&p->in_data.data), (unsigned int)data_len ));
		set_incoming_fault(p);
		return False;
	}

	/*
	 * Append the data portion into the buffer and return.
	 */

	if(!prs_append_some_prs_data(&p->in_data.data, rpc_in_p, prs_offset(rpc_in_p), data_len)) {
		DEBUG(0,("process_request_pdu: Unable to append data size %u to parse buffer of size %u.\n",
				(unsigned int)data_len, (unsigned int)prs_data_size(&p->in_data.data) ));
		set_incoming_fault(p);
		return False;
	}

	if(p->hdr.flags & RPC_FLG_LAST) {
		BOOL ret = False;
		/*
		 * Ok - we finally have a complete RPC stream.
		 * Call the rpc command to process it.
		 */

		/*
		 * Ensure the internal prs buffer size is *exactly* the same
		 * size as the current offset.
		 */

 		if(!prs_set_buffer_size(&p->in_data.data, prs_offset(&p->in_data.data))) {
			DEBUG(0,("process_request_pdu: Call to prs_set_buffer_size failed!\n"));
			set_incoming_fault(p);
			return False;
		}

		/*
		 * Set the parse offset to the start of the data and set the
		 * prs_struct to UNMARSHALL.
		 */

		prs_set_offset(&p->in_data.data, 0);
		prs_switch_type(&p->in_data.data, UNMARSHALL);

		/*
		 * Process the complete data stream here.
		 */

		free_pipe_context(p);

		if(pipe_init_outgoing_data(p)) {
			ret = api_pipe_request(p);
		}

		free_pipe_context(p);

		/*
		 * We have consumed the whole data stream. Set back to
		 * marshalling and set the offset back to the start of
		 * the buffer to re-use it (we could also do a prs_mem_free()
		 * and then re_init on the next start of PDU. Not sure which
		 * is best here.... JRA.
		 */

		prs_switch_type(&p->in_data.data, MARSHALL);
		prs_set_offset(&p->in_data.data, 0);
		return ret;
	}

	return True;
}

/****************************************************************************
 Processes a finished PDU stored in current_in_pdu. The RPC_HEADER has
 already been parsed and stored in p->hdr.
****************************************************************************/

static void process_complete_pdu(pipes_struct *p)
{
	prs_struct rpc_in;
	size_t data_len = p->in_data.pdu_received_len - RPC_HEADER_LEN;
	char *data_p = (char *)&p->in_data.current_in_pdu[RPC_HEADER_LEN];
	BOOL reply = False;

	if(p->fault_state) {
		DEBUG(10,("process_complete_pdu: pipe %s in fault state.\n",
			p->name ));
		set_incoming_fault(p);
		setup_fault_pdu(p, NT_STATUS(DCERPC_FAULT_OP_RNG_ERROR));
		return;
	}

	prs_init( &rpc_in, 0, p->mem_ctx, UNMARSHALL);

	/*
	 * Ensure we're using the corrent endianness for both the 
	 * RPC header flags and the raw data we will be reading from.
	 */

	prs_set_endian_data( &rpc_in, p->endian);
	prs_set_endian_data( &p->in_data.data, p->endian);

	prs_give_memory( &rpc_in, data_p, (uint32)data_len, False);

	DEBUG(10,("process_complete_pdu: processing packet type %u\n",
			(unsigned int)p->hdr.pkt_type ));

	switch (p->hdr.pkt_type) {
		case RPC_REQUEST:
			reply = process_request_pdu(p, &rpc_in);
			break;

		case RPC_PING: /* CL request - ignore... */
			DEBUG(0,("process_complete_pdu: Error. Connectionless packet type %u received on pipe %s.\n",
				(unsigned int)p->hdr.pkt_type, p->name));
			break;

		case RPC_RESPONSE: /* No responses here. */
			DEBUG(0,("process_complete_pdu: Error. RPC_RESPONSE received from client on pipe %s.\n",
				p->name ));
			break;

		case RPC_FAULT:
		case RPC_WORKING: /* CL request - reply to a ping when a call in process. */
		case RPC_NOCALL: /* CL - server reply to a ping call. */
		case RPC_REJECT:
		case RPC_ACK:
		case RPC_CL_CANCEL:
		case RPC_FACK:
		case RPC_CANCEL_ACK:
			DEBUG(0,("process_complete_pdu: Error. Connectionless packet type %u received on pipe %s.\n",
				(unsigned int)p->hdr.pkt_type, p->name));
			break;

		case RPC_BIND:
			/*
			 * We assume that a pipe bind is only in one pdu.
			 */
			if(pipe_init_outgoing_data(p)) {
				reply = api_pipe_bind_req(p, &rpc_in);
			}
			break;

		case RPC_BINDACK:
		case RPC_BINDNACK:
			DEBUG(0,("process_complete_pdu: Error. RPC_BINDACK/RPC_BINDNACK packet type %u received on pipe %s.\n",
				(unsigned int)p->hdr.pkt_type, p->name));
			break;


		case RPC_ALTCONT:
			/*
			 * We assume that a pipe bind is only in one pdu.
			 */
			if(pipe_init_outgoing_data(p)) {
				reply = api_pipe_alter_context(p, &rpc_in);
			}
			break;

		case RPC_ALTCONTRESP:
			DEBUG(0,("process_complete_pdu: Error. RPC_ALTCONTRESP on pipe %s: Should only be server -> client.\n",
				p->name));
			break;

		case RPC_AUTH3:
			/*
			 * The third packet in an NTLMSSP auth exchange.
			 */
			if(pipe_init_outgoing_data(p)) {
				reply = api_pipe_bind_auth3(p, &rpc_in);
			}
			break;

		case RPC_SHUTDOWN:
			DEBUG(0,("process_complete_pdu: Error. RPC_SHUTDOWN on pipe %s: Should only be server -> client.\n",
				p->name));
			break;

		case RPC_CO_CANCEL:
			/* For now just free all client data and continue processing. */
			DEBUG(3,("process_complete_pdu: RPC_ORPHANED. Abandoning rpc call.\n"));
			/* As we never do asynchronous RPC serving, we can never cancel a
			   call (as far as I know). If we ever did we'd have to send a cancel_ack
			   reply. For now, just free all client data and continue processing. */
			reply = True;
			break;
#if 0
			/* Enable this if we're doing async rpc. */
			/* We must check the call-id matches the outstanding callid. */
			if(pipe_init_outgoing_data(p)) {
				/* Send a cancel_ack PDU reply. */
				/* We should probably check the auth-verifier here. */
				reply = setup_cancel_ack_reply(p, &rpc_in);
			}
			break;
#endif

		case RPC_ORPHANED:
			/* We should probably check the auth-verifier here.
			   For now just free all client data and continue processing. */
			DEBUG(3,("process_complete_pdu: RPC_ORPHANED. Abandoning rpc call.\n"));
			reply = True;
			break;

		default:
			DEBUG(0,("process_complete_pdu: Unknown rpc type = %u received.\n", (unsigned int)p->hdr.pkt_type ));
			break;
	}

	/* Reset to little endian. Probably don't need this but it won't hurt. */
	prs_set_endian_data( &p->in_data.data, RPC_LITTLE_ENDIAN);

	if (!reply) {
		DEBUG(3,("process_complete_pdu: DCE/RPC fault sent on pipe %s\n", p->pipe_srv_name));
		set_incoming_fault(p);
		setup_fault_pdu(p, NT_STATUS(DCERPC_FAULT_OP_RNG_ERROR));
		prs_mem_free(&rpc_in);
	} else {
		/*
		 * Reset the lengths. We're ready for a new pdu.
		 */
		p->in_data.pdu_needed_len = 0;
		p->in_data.pdu_received_len = 0;
	}

	prs_mem_free(&rpc_in);
}

/****************************************************************************
 Accepts incoming data on an rpc pipe. Processes the data in pdu sized units.
****************************************************************************/

static ssize_t process_incoming_data(pipes_struct *p, char *data, size_t n)
{
	size_t data_to_copy = MIN(n, RPC_MAX_PDU_FRAG_LEN - p->in_data.pdu_received_len);

	DEBUG(10,("process_incoming_data: Start: pdu_received_len = %u, pdu_needed_len = %u, incoming data = %u\n",
		(unsigned int)p->in_data.pdu_received_len, (unsigned int)p->in_data.pdu_needed_len,
		(unsigned int)n ));

	if(data_to_copy == 0) {
		/*
		 * This is an error - data is being received and there is no
		 * space in the PDU. Free the received data and go into the fault state.
		 */
		DEBUG(0,("process_incoming_data: No space in incoming pdu buffer. Current size = %u \
incoming data size = %u\n", (unsigned int)p->in_data.pdu_received_len, (unsigned int)n ));
		set_incoming_fault(p);
		return -1;
	}

	/*
	 * If we have no data already, wait until we get at least a RPC_HEADER_LEN
	 * number of bytes before we can do anything.
	 */

	if((p->in_data.pdu_needed_len == 0) && (p->in_data.pdu_received_len < RPC_HEADER_LEN)) {
		/*
		 * Always return here. If we have more data then the RPC_HEADER
		 * will be processed the next time around the loop.
		 */
		return fill_rpc_header(p, data, data_to_copy);
	}

	/*
	 * At this point we know we have at least an RPC_HEADER_LEN amount of data
	 * stored in current_in_pdu.
	 */

	/*
	 * If pdu_needed_len is zero this is a new pdu. 
	 * Unmarshall the header so we know how much more
	 * data we need, then loop again.
	 */

	if(p->in_data.pdu_needed_len == 0) {
		ssize_t rret = unmarshall_rpc_header(p);
		if (rret == -1 || p->in_data.pdu_needed_len > 0) {
			return rret;
		}
		/* If rret == 0 and pdu_needed_len == 0 here we have a PDU that consists
		   of an RPC_HEADER only. This is a RPC_SHUTDOWN, RPC_CO_CANCEL or RPC_ORPHANED
		   pdu type. Deal with this in process_complete_pdu(). */
	}

	/*
	 * Ok - at this point we have a valid RPC_HEADER in p->hdr.
	 * Keep reading until we have a full pdu.
	 */

	data_to_copy = MIN(data_to_copy, p->in_data.pdu_needed_len);

	/*
	 * Copy as much of the data as we need into the current_in_pdu buffer.
	 * pdu_needed_len becomes zero when we have a complete pdu.
	 */

	memcpy( (char *)&p->in_data.current_in_pdu[p->in_data.pdu_received_len], data, data_to_copy);
	p->in_data.pdu_received_len += data_to_copy;
	p->in_data.pdu_needed_len -= data_to_copy;

	/*
	 * Do we have a complete PDU ?
	 * (return the number of bytes handled in the call)
	 */

	if(p->in_data.pdu_needed_len == 0) {
		process_complete_pdu(p);
		return data_to_copy;
	}

	DEBUG(10,("process_incoming_data: not a complete PDU yet. pdu_received_len = %u, pdu_needed_len = %u\n",
		(unsigned int)p->in_data.pdu_received_len, (unsigned int)p->in_data.pdu_needed_len ));

	return (ssize_t)data_to_copy;
}

/****************************************************************************
 Accepts incoming data on an rpc pipe.
****************************************************************************/

ssize_t write_to_pipe(smb_np_struct *p, char *data, size_t n)
{
	DEBUG(6,("write_to_pipe: %x", p->pnum));

	DEBUG(6,(" name: %s open: %s len: %d\n",
		 p->name, BOOLSTR(p->open), (int)n));

	dump_data(50, data, n);

	return p->namedpipe_write(p->np_state, data, n);
}

/****************************************************************************
 Accepts incoming data on an internal rpc pipe.
****************************************************************************/

static ssize_t write_to_internal_pipe(void *np_conn, char *data, size_t n)
{
	pipes_struct *p = (pipes_struct*)np_conn;
	size_t data_left = n;

	while(data_left) {
		ssize_t data_used;

		DEBUG(10,("write_to_pipe: data_left = %u\n", (unsigned int)data_left ));

		data_used = process_incoming_data(p, data, data_left);

		DEBUG(10,("write_to_pipe: data_used = %d\n", (int)data_used ));

		if(data_used < 0) {
			return -1;
		}

		data_left -= data_used;
		data += data_used;
	}	

	return n;
}

/****************************************************************************
 Replies to a request to read data from a pipe.

 Headers are interspersed with the data at PDU intervals. By the time
 this function is called, the start of the data could possibly have been
 read by an SMBtrans (file_offset != 0).

 Calling create_rpc_reply() here is a hack. The data should already
 have been prepared into arrays of headers + data stream sections.
****************************************************************************/

ssize_t read_from_pipe(smb_np_struct *p, char *data, size_t n,
		BOOL *is_data_outstanding)
{
	if (!p || !p->open) {
		DEBUG(0,("read_from_pipe: pipe not open\n"));
		return -1;		
	}

	DEBUG(6,("read_from_pipe: %x", p->pnum));

	return p->namedpipe_read(p->np_state, data, n, is_data_outstanding);
}

/****************************************************************************
 Replies to a request to read data from a pipe.

 Headers are interspersed with the data at PDU intervals. By the time
 this function is called, the start of the data could possibly have been
 read by an SMBtrans (file_offset != 0).

 Calling create_rpc_reply() here is a hack. The data should already
 have been prepared into arrays of headers + data stream sections.
****************************************************************************/

static ssize_t read_from_internal_pipe(void *np_conn, char *data, size_t n,
		BOOL *is_data_outstanding)
{
	pipes_struct *p = (pipes_struct*)np_conn;
	uint32 pdu_remaining = 0;
	ssize_t data_returned = 0;

	if (!p) {
		DEBUG(0,("read_from_pipe: pipe not open\n"));
		return -1;		
	}

	DEBUG(6,(" name: %s len: %u\n", p->name, (unsigned int)n));

	/*
	 * We cannot return more than one PDU length per
	 * read request.
	 */

	/*
	 * This condition should result in the connection being closed.  
	 * Netapp filers seem to set it to 0xffff which results in domain
	 * authentications failing.  Just ignore it so things work.
	 */

	if(n > RPC_MAX_PDU_FRAG_LEN) {
                DEBUG(5,("read_from_pipe: too large read (%u) requested on \
pipe %s. We can only service %d sized reads.\n", (unsigned int)n, p->name, RPC_MAX_PDU_FRAG_LEN ));
	}

	/*
 	 * Determine if there is still data to send in the
	 * pipe PDU buffer. Always send this first. Never
	 * send more than is left in the current PDU. The
	 * client should send a new read request for a new
	 * PDU.
	 */

	if((pdu_remaining = p->out_data.current_pdu_len - p->out_data.current_pdu_sent) > 0) {
		data_returned = (ssize_t)MIN(n, pdu_remaining);

		DEBUG(10,("read_from_pipe: %s: current_pdu_len = %u, current_pdu_sent = %u \
returning %d bytes.\n", p->name, (unsigned int)p->out_data.current_pdu_len, 
			(unsigned int)p->out_data.current_pdu_sent, (int)data_returned));

		memcpy( data, &p->out_data.current_pdu[p->out_data.current_pdu_sent], (size_t)data_returned);
		p->out_data.current_pdu_sent += (uint32)data_returned;
		goto out;
	}

	/*
	 * At this point p->current_pdu_len == p->current_pdu_sent (which
	 * may of course be zero if this is the first return fragment.
	 */

	DEBUG(10,("read_from_pipe: %s: fault_state = %d : data_sent_length \
= %u, prs_offset(&p->out_data.rdata) = %u.\n",
		p->name, (int)p->fault_state, (unsigned int)p->out_data.data_sent_length, (unsigned int)prs_offset(&p->out_data.rdata) ));

	if(p->out_data.data_sent_length >= prs_offset(&p->out_data.rdata)) {
		/*
		 * We have sent all possible data, return 0.
		 */
		data_returned = 0;
		goto out;
	}

	/*
	 * We need to create a new PDU from the data left in p->rdata.
	 * Create the header/data/footers. This also sets up the fields
	 * p->current_pdu_len, p->current_pdu_sent, p->data_sent_length
	 * and stores the outgoing PDU in p->current_pdu.
	 */

	if(!create_next_pdu(p)) {
		DEBUG(0,("read_from_pipe: %s: create_next_pdu failed.\n", p->name));
		return -1;
	}

	data_returned = MIN(n, p->out_data.current_pdu_len);

	memcpy( data, p->out_data.current_pdu, (size_t)data_returned);
	p->out_data.current_pdu_sent += (uint32)data_returned;

  out:

	(*is_data_outstanding) = p->out_data.current_pdu_len > n;
	return data_returned;
}

/****************************************************************************
 Wait device state on a pipe. Exactly what this is for is unknown...
****************************************************************************/

BOOL wait_rpc_pipe_hnd_state(smb_np_struct *p, uint16 priority)
{
	if (p == NULL) {
		return False;
	}

	if (p->open) {
		DEBUG(3,("wait_rpc_pipe_hnd_state: Setting pipe wait state priority=%x on pipe (name=%s)\n",
		         priority, p->name));

		p->priority = priority;
		
		return True;
	} 

	DEBUG(3,("wait_rpc_pipe_hnd_state: Error setting pipe wait state priority=%x (name=%s)\n",
		 priority, p->name));
	return False;
}


/****************************************************************************
 Set device state on a pipe. Exactly what this is for is unknown...
****************************************************************************/

BOOL set_rpc_pipe_hnd_state(smb_np_struct *p, uint16 device_state)
{
	if (p == NULL) {
		return False;
	}

	if (p->open) {
		DEBUG(3,("set_rpc_pipe_hnd_state: Setting pipe device state=%x on pipe (name=%s)\n",
		         device_state, p->name));

		p->device_state = device_state;
		
		return True;
	} 

	DEBUG(3,("set_rpc_pipe_hnd_state: Error setting pipe device state=%x (name=%s)\n",
		 device_state, p->name));
	return False;
}


/****************************************************************************
 Close an rpc pipe.
****************************************************************************/

BOOL close_rpc_pipe_hnd(smb_np_struct *p)
{
	if (!p) {
		DEBUG(0,("Invalid pipe in close_rpc_pipe_hnd\n"));
		return False;
	}

	p->namedpipe_close(p->np_state);

	bitmap_clear(bmap, p->pnum - pipe_handle_offset);

	pipes_open--;

	DEBUG(4,("closed pipe name %s pnum=%x (pipes_open=%d)\n", 
		 p->name, p->pnum, pipes_open));  

	DLIST_REMOVE(Pipes, p);

	ZERO_STRUCTP(p);

	SAFE_FREE(p);

	return True;
}

/****************************************************************************
 Close all pipes on a connection.
****************************************************************************/

void pipe_close_conn(connection_struct *conn)
{
	smb_np_struct *p, *next;

	for (p=Pipes;p;p=next) {
		next = p->next;
		if (p->conn == conn) {
			close_rpc_pipe_hnd(p);
		}
	}
}

/****************************************************************************
 Close an rpc pipe.
****************************************************************************/

static BOOL close_internal_rpc_pipe_hnd(void *np_conn)
{
	pipes_struct *p = (pipes_struct *)np_conn;
	if (!p) {
		DEBUG(0,("Invalid pipe in close_internal_rpc_pipe_hnd\n"));
		return False;
	}

	prs_mem_free(&p->out_data.rdata);
	prs_mem_free(&p->in_data.data);

	if (p->auth.auth_data_free_func) {
		(*p->auth.auth_data_free_func)(&p->auth);
	}

	if (p->mem_ctx) {
		talloc_destroy(p->mem_ctx);
	}

	if (p->pipe_state_mem_ctx) {
		talloc_destroy(p->pipe_state_mem_ctx);
	}

	free_pipe_rpc_context( p->contexts );

	/* Free the handles database. */
	close_policy_by_pipe(p);

	TALLOC_FREE(p->pipe_user.nt_user_token);
	data_blob_free(&p->session_key);
	SAFE_FREE(p->pipe_user.ut.groups);

	DLIST_REMOVE(InternalPipes, p);

	ZERO_STRUCTP(p);

	SAFE_FREE(p);
	
	return True;
}

/****************************************************************************
 Find an rpc pipe given a pipe handle in a buffer and an offset.
****************************************************************************/

smb_np_struct *get_rpc_pipe_p(char *buf, int where)
{
	int pnum = SVAL(buf,where);

	if (chain_p) {
		return chain_p;
	}

	return get_rpc_pipe(pnum);
}

/****************************************************************************
 Find an rpc pipe given a pipe handle.
****************************************************************************/

smb_np_struct *get_rpc_pipe(int pnum)
{
	smb_np_struct *p;

	DEBUG(4,("search for pipe pnum=%x\n", pnum));

	for (p=Pipes;p;p=p->next) {
		DEBUG(5,("pipe name %s pnum=%x (pipes_open=%d)\n", 
		          p->name, p->pnum, pipes_open));  
	}

	for (p=Pipes;p;p=p->next) {
		if (p->pnum == pnum) {
			chain_p = p;
			return p;
		}
	}

	return NULL;
}
