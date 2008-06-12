/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines for rpcecho
 *  Copyright (C) Tim Potter                   2003.
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

/* This is the interface to the rpcecho pipe. */

#include "includes.h"
#include "nterr.h"

#ifdef DEVELOPER

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

static BOOL api_add_one(pipes_struct *p)
{
	ECHO_Q_ADD_ONE q_u;
	ECHO_R_ADD_ONE r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!echo_io_q_add_one("", &q_u, data, 0))
		return False;
	
	_echo_add_one(p, &q_u, &r_u);
	
	if(!echo_io_r_add_one("", &r_u, rdata, 0))
		return False;

	return True;
}

static BOOL api_echo_data(pipes_struct *p)
{
	ECHO_Q_ECHO_DATA q_u;
	ECHO_R_ECHO_DATA r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!echo_io_q_echo_data("", &q_u, data, 0))
		return False;
	
	_echo_data(p, &q_u, &r_u);
	
	if(!echo_io_r_echo_data("", &r_u, rdata, 0))
		return False;

	return True;
}

static BOOL api_source_data(pipes_struct *p)
{
	ECHO_Q_SOURCE_DATA q_u;
	ECHO_R_SOURCE_DATA r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!echo_io_q_source_data("", &q_u, data, 0))
		return False;
	
	_source_data(p, &q_u, &r_u);
	
	if(!echo_io_r_source_data("", &r_u, rdata, 0))
		return False;

	return True;
}

static BOOL api_sink_data(pipes_struct *p)
{
	ECHO_Q_SINK_DATA q_u;
	ECHO_R_SINK_DATA r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!echo_io_q_sink_data("", &q_u, data, 0))
		return False;
	
	_sink_data(p, &q_u, &r_u);
	
	if(!echo_io_r_sink_data("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
\pipe\rpcecho commands
********************************************************************/

struct api_struct api_echo_cmds[] = {
	{"ADD_ONE",       ECHO_ADD_ONE,     api_add_one },
	{"ECHO_DATA",     ECHO_DATA,        api_echo_data },
	{"SOURCE_DATA",   ECHO_SOURCE_DATA, api_source_data },
	{"SINK_DATA",     ECHO_SINK_DATA,   api_sink_data },
};


void echo_get_pipe_fns( struct api_struct **fns, int *n_fns )
{
	*fns = api_echo_cmds;
	*n_fns = sizeof(api_echo_cmds) / sizeof(struct api_struct);
}

NTSTATUS rpc_echo_init(void)
{
	return rpc_pipe_register_commands(SMB_RPC_INTERFACE_VERSION,
		"rpcecho", "rpcecho", api_echo_cmds,
		sizeof(api_echo_cmds) / sizeof(struct api_struct));
}

#else /* DEVELOPER */

NTSTATUS rpc_echo_init(void)
{
	return NT_STATUS_OK;
}
#endif /* DEVELOPER */
