/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Marcin Krzysztof Porwit    2005.
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

static BOOL api_eventlog_open_eventlog(pipes_struct *p)
{
	EVENTLOG_Q_OPEN_EVENTLOG q_u;
	EVENTLOG_R_OPEN_EVENTLOG r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!(eventlog_io_q_open_eventlog("", &q_u, data, 0))) {
		DEBUG(0, ("eventlog_io_q_open_eventlog: unable to unmarshall EVENTLOG_Q_OPEN_EVENTLOG.\n"));
		return False;
	}
	
	r_u.status = _eventlog_open_eventlog(p, &q_u, &r_u);

	if (!(eventlog_io_r_open_eventlog("", &r_u, rdata, 0))) {
		DEBUG(0, ("eventlog_io_r_open_eventlog: unable to marshall EVENTLOG_R_OPEN_EVENTLOG.\n"));
		return False;
	}

	return True;
}

static BOOL api_eventlog_close_eventlog(pipes_struct *p)
{
	EVENTLOG_Q_CLOSE_EVENTLOG q_u;
	EVENTLOG_R_CLOSE_EVENTLOG r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!(eventlog_io_q_close_eventlog("", &q_u, data, 0))) {
		DEBUG(0, ("eventlog_io_q_close_eventlog: unable to unmarshall EVENTLOG_Q_CLOSE_EVENTLOG.\n"));
		return False;
	}

	r_u.status = _eventlog_close_eventlog(p, &q_u, &r_u);

	if (!(eventlog_io_r_close_eventlog("", &r_u, rdata, 0))) {
		DEBUG(0, ("eventlog_io_r_close_eventlog: unable to marshall EVENTLOG_R_CLOSE_EVENTLOG.\n"));
		return False;
	}

	return True;
}

static BOOL api_eventlog_get_num_records(pipes_struct *p)
{
	EVENTLOG_Q_GET_NUM_RECORDS q_u;
	EVENTLOG_R_GET_NUM_RECORDS r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!(eventlog_io_q_get_num_records("", &q_u, data, 0))) {
		DEBUG(0, ("eventlog_io_q_get_num_records: unable to unmarshall EVENTLOG_Q_GET_NUM_RECORDS.\n"));
		return False;
	}
    
	r_u.status = _eventlog_get_num_records(p, &q_u, &r_u);
    
	if (!(eventlog_io_r_get_num_records("", &r_u, rdata, 0))) {
		DEBUG(0, ("eventlog_io_r_get_num_records: unable to marshall EVENTLOG_R_GET_NUM_RECORDS.\n"));
		return False;
	}

	return True;
}

static BOOL api_eventlog_get_oldest_entry(pipes_struct *p)
{
	EVENTLOG_Q_GET_OLDEST_ENTRY q_u;
	EVENTLOG_R_GET_OLDEST_ENTRY r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!(eventlog_io_q_get_oldest_entry("", &q_u, data, 0))) {
		DEBUG(0, ("eventlog_io_q_get_oldest_entry: unable to unmarshall EVENTLOG_Q_GET_OLDEST_ENTRY.\n"));
		return False;
	}

	r_u.status = _eventlog_get_oldest_entry(p, &q_u, &r_u);
    
	if (!(eventlog_io_r_get_oldest_entry("", &r_u, rdata, 0))) {
		DEBUG(0, ("eventlog_io_r_get_oldest_entry: unable to marshall EVENTLOG_R_GET_OLDEST_ENTRY.\n"));
		return False;
	}
    
	return True;
}

static BOOL api_eventlog_read_eventlog(pipes_struct *p)
{
	EVENTLOG_Q_READ_EVENTLOG q_u;
	EVENTLOG_R_READ_EVENTLOG r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!(eventlog_io_q_read_eventlog("", &q_u, data, 0))) {
		DEBUG(0, ("eventlog_io_q_read_eventlog: unable to unmarshall EVENTLOG_Q_READ_EVENTLOG.\n"));
		return False;
	}

	r_u.status = _eventlog_read_eventlog(p, &q_u, &r_u);

	if (!(eventlog_io_r_read_eventlog("", &q_u, &r_u, rdata, 0))) {
		DEBUG(0, ("eventlog_io_r_read_eventlog: unable to marshall EVENTLOG_R_READ_EVENTLOG.\n"));
		return False;
	}

	return True;
}

static BOOL api_eventlog_clear_eventlog(pipes_struct *p)
{
	EVENTLOG_Q_CLEAR_EVENTLOG q_u;
	EVENTLOG_R_CLEAR_EVENTLOG r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!(eventlog_io_q_clear_eventlog("", &q_u, data, 0))) {
		DEBUG(0, ("eventlog_io_q_clear_eventlog: unable to unmarshall EVENTLOG_Q_CLEAR_EVENTLOG.\n"));
		return False;
	}

	r_u.status = _eventlog_clear_eventlog(p, &q_u, &r_u);

	if (!(eventlog_io_r_clear_eventlog("", &r_u, rdata, 0))) {
		DEBUG(0, ("eventlog_io_q_clear_eventlog: unable to marshall EVENTLOG_Q_CLEAR_EVENTLOG.\n"));
		return False;
	}

	return True;
}

/*
 \pipe\eventlog commands
*/
struct api_struct api_eventlog_cmds[] =
{
	{"EVENTLOG_OPENEVENTLOG", 	EVENTLOG_OPENEVENTLOG, 		api_eventlog_open_eventlog    },
	{"EVENTLOG_CLOSEEVENTLOG", 	EVENTLOG_CLOSEEVENTLOG, 	api_eventlog_close_eventlog   },
	{"EVENTLOG_GETNUMRECORDS", 	EVENTLOG_GETNUMRECORDS, 	api_eventlog_get_num_records  },
	{"EVENTLOG_GETOLDESTENTRY", 	EVENTLOG_GETOLDESTENTRY, 	api_eventlog_get_oldest_entry },
	{"EVENTLOG_READEVENTLOG", 	EVENTLOG_READEVENTLOG, 		api_eventlog_read_eventlog    },
	{"EVENTLOG_CLEAREVENTLOG", 	EVENTLOG_CLEAREVENTLOG, 	api_eventlog_clear_eventlog   }
};

NTSTATUS rpc_eventlog_init(void)
{
	return rpc_pipe_register_commands(SMB_RPC_INTERFACE_VERSION, 
		"eventlog", "eventlog", api_eventlog_cmds,
		sizeof(api_eventlog_cmds)/sizeof(struct api_struct));
}

void eventlog_get_pipe_fns(struct api_struct **fns, int *n_fns)
{
	*fns = api_eventlog_cmds;
	*n_fns = sizeof(api_eventlog_cmds) / sizeof(struct api_struct);
}
