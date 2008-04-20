/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Andrew Tridgell              1992-1997,
 *  Copyright (C) Luke Kenneth Casson Leighton 1996-1997,
 *  Copyright (C) Paul Ashton                       1997,
 *  Copyright (C) Jeremy Allison                    2001,
 *  Copyright (C) Jim McDonough <jmcd@us.ibm.com>   2003.
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

/* This is the interface to the srvsvc pipe. */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

/*******************************************************************
 api_srv_net_srv_get_info
********************************************************************/

static BOOL api_srv_net_srv_get_info(pipes_struct *p)
{
	SRV_Q_NET_SRV_GET_INFO q_u;
	SRV_R_NET_SRV_GET_INFO r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the net server get info */
	if (!srv_io_q_net_srv_get_info("", &q_u, data, 0))
		return False;

	r_u.status = _srv_net_srv_get_info(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if (!srv_io_r_net_srv_get_info("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 api_srv_net_srv_get_info
********************************************************************/

static BOOL api_srv_net_srv_set_info(pipes_struct *p)
{
	SRV_Q_NET_SRV_SET_INFO q_u;
	SRV_R_NET_SRV_SET_INFO r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the net server set info */
	if (!srv_io_q_net_srv_set_info("", &q_u, data, 0))
		return False;

	r_u.status = _srv_net_srv_set_info(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if (!srv_io_r_net_srv_set_info("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 api_srv_net_file_enum
********************************************************************/

static BOOL api_srv_net_file_enum(pipes_struct *p)
{
	SRV_Q_NET_FILE_ENUM q_u;
	SRV_R_NET_FILE_ENUM r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the net file enum */
	if (!srv_io_q_net_file_enum("", &q_u, data, 0))
		return False;

	r_u.status = _srv_net_file_enum(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!srv_io_r_net_file_enum("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 api_srv_net_conn_enum
********************************************************************/

static BOOL api_srv_net_conn_enum(pipes_struct *p)
{
	SRV_Q_NET_CONN_ENUM q_u;
	SRV_R_NET_CONN_ENUM r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the net server get enum */
	if (!srv_io_q_net_conn_enum("", &q_u, data, 0))
		return False;

	r_u.status = _srv_net_conn_enum(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if (!srv_io_r_net_conn_enum("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 Enumerate sessions.
********************************************************************/

static BOOL api_srv_net_sess_enum(pipes_struct *p)
{
	SRV_Q_NET_SESS_ENUM q_u;
	SRV_R_NET_SESS_ENUM r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the net server get enum */
	if (!srv_io_q_net_sess_enum("", &q_u, data, 0))
		return False;

	/* construct reply.  always indicate success */
	r_u.status = _srv_net_sess_enum(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if (!srv_io_r_net_sess_enum("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 Delete session.
********************************************************************/

static BOOL api_srv_net_sess_del(pipes_struct *p)
{
	SRV_Q_NET_SESS_DEL q_u;
	SRV_R_NET_SESS_DEL r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the net server get enum */
	if (!srv_io_q_net_sess_del("", &q_u, data, 0))
		return False;

	/* construct reply.  always indicate success */
	r_u.status = _srv_net_sess_del(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if (!srv_io_r_net_sess_del("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 RPC to enumerate shares.
********************************************************************/

static BOOL api_srv_net_share_enum_all(pipes_struct *p)
{
	SRV_Q_NET_SHARE_ENUM q_u;
	SRV_R_NET_SHARE_ENUM r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* Unmarshall the net server get enum. */
	if(!srv_io_q_net_share_enum("", &q_u, data, 0)) {
		DEBUG(0,("api_srv_net_share_enum_all: Failed to unmarshall SRV_Q_NET_SHARE_ENUM.\n"));
		return False;
	}

	r_u.status = _srv_net_share_enum_all(p, &q_u, &r_u);

	if (!srv_io_r_net_share_enum("", &r_u, rdata, 0)) {
		DEBUG(0,("api_srv_net_share_enum_all: Failed to marshall SRV_R_NET_SHARE_ENUM.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 RPC to enumerate shares.
********************************************************************/

static BOOL api_srv_net_share_enum(pipes_struct *p)
{
	SRV_Q_NET_SHARE_ENUM q_u;
	SRV_R_NET_SHARE_ENUM r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* Unmarshall the net server get enum. */
	if(!srv_io_q_net_share_enum("", &q_u, data, 0)) {
		DEBUG(0,("api_srv_net_share_enum: Failed to unmarshall SRV_Q_NET_SHARE_ENUM.\n"));
		return False;
	}

	r_u.status = _srv_net_share_enum(p, &q_u, &r_u);

	if (!srv_io_r_net_share_enum("", &r_u, rdata, 0)) {
		DEBUG(0,("api_srv_net_share_enum: Failed to marshall SRV_R_NET_SHARE_ENUM.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 RPC to return share information.
********************************************************************/

static BOOL api_srv_net_share_get_info(pipes_struct *p)
{
	SRV_Q_NET_SHARE_GET_INFO q_u;
	SRV_R_NET_SHARE_GET_INFO r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* Unmarshall the net server get info. */
	if(!srv_io_q_net_share_get_info("", &q_u, data, 0)) {
		DEBUG(0,("api_srv_net_share_get_info: Failed to unmarshall SRV_Q_NET_SHARE_GET_INFO.\n"));
		return False;
	}

	r_u.status = _srv_net_share_get_info(p, &q_u, &r_u);

	if(!srv_io_r_net_share_get_info("", &r_u, rdata, 0)) {
		DEBUG(0,("api_srv_net_share_get_info: Failed to marshall SRV_R_NET_SHARE_GET_INFO.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 RPC to set share information.
********************************************************************/

static BOOL api_srv_net_share_set_info(pipes_struct *p)
{
	SRV_Q_NET_SHARE_SET_INFO q_u;
	SRV_R_NET_SHARE_SET_INFO r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* Unmarshall the net server set info. */
	if(!srv_io_q_net_share_set_info("", &q_u, data, 0)) {
		DEBUG(0,("api_srv_net_share_set_info: Failed to unmarshall SRV_Q_NET_SHARE_SET_INFO.\n"));
		return False;
	}

	r_u.status = _srv_net_share_set_info(p, &q_u, &r_u);

	if(!srv_io_r_net_share_set_info("", &r_u, rdata, 0)) {
		DEBUG(0,("api_srv_net_share_set_info: Failed to marshall SRV_R_NET_SHARE_SET_INFO.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 RPC to add share information.
********************************************************************/

static BOOL api_srv_net_share_add(pipes_struct *p)
{
	SRV_Q_NET_SHARE_ADD q_u;
	SRV_R_NET_SHARE_ADD r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* Unmarshall the net server add info. */
	if(!srv_io_q_net_share_add("", &q_u, data, 0)) {
		DEBUG(0,("api_srv_net_share_add: Failed to unmarshall SRV_Q_NET_SHARE_ADD.\n"));
		return False;
	}

	r_u.status = _srv_net_share_add(p, &q_u, &r_u);

	if(!srv_io_r_net_share_add("", &r_u, rdata, 0)) {
		DEBUG(0,("api_srv_net_share_add: Failed to marshall SRV_R_NET_SHARE_ADD.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 RPC to delete share information.
********************************************************************/

static BOOL api_srv_net_share_del(pipes_struct *p)
{
	SRV_Q_NET_SHARE_DEL q_u;
	SRV_R_NET_SHARE_DEL r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* Unmarshall the net server del info. */
	if(!srv_io_q_net_share_del("", &q_u, data, 0)) {
		DEBUG(0,("api_srv_net_share_del: Failed to unmarshall SRV_Q_NET_SHARE_DEL.\n"));
		return False;
	}

	r_u.status = _srv_net_share_del(p, &q_u, &r_u);

	if(!srv_io_r_net_share_del("", &r_u, rdata, 0)) {
		DEBUG(0,("api_srv_net_share_del: Failed to marshall SRV_R_NET_SHARE_DEL.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 RPC to delete share information.
********************************************************************/

static BOOL api_srv_net_share_del_sticky(pipes_struct *p)
{
	SRV_Q_NET_SHARE_DEL q_u;
	SRV_R_NET_SHARE_DEL r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* Unmarshall the net server del info. */
	if(!srv_io_q_net_share_del("", &q_u, data, 0)) {
		DEBUG(0,("api_srv_net_share_del_sticky: Failed to unmarshall SRV_Q_NET_SHARE_DEL.\n"));
		return False;
	}

	r_u.status = _srv_net_share_del_sticky(p, &q_u, &r_u);

	if(!srv_io_r_net_share_del("", &r_u, rdata, 0)) {
		DEBUG(0,("api_srv_net_share_del_sticky: Failed to marshall SRV_R_NET_SHARE_DEL.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_srv_net_remote_tod
********************************************************************/

static BOOL api_srv_net_remote_tod(pipes_struct *p)
{
	SRV_Q_NET_REMOTE_TOD q_u;
	SRV_R_NET_REMOTE_TOD r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the net server get enum */
	if(!srv_io_q_net_remote_tod("", &q_u, data, 0))
		return False;

	r_u.status = _srv_net_remote_tod(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!srv_io_r_net_remote_tod("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 RPC to enumerate disks available on a server e.g. C:, D: ...
*******************************************************************/

static BOOL api_srv_net_disk_enum(pipes_struct *p) 
{
	SRV_Q_NET_DISK_ENUM q_u;
	SRV_R_NET_DISK_ENUM r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* Unmarshall the net server disk enum. */
	if(!srv_io_q_net_disk_enum("", &q_u, data, 0)) {
		DEBUG(0,("api_srv_net_disk_enum: Failed to unmarshall SRV_Q_NET_DISK_ENUM.\n"));
		return False;
	}

	r_u.status = _srv_net_disk_enum(p, &q_u, &r_u);

	if(!srv_io_r_net_disk_enum("", &r_u, rdata, 0)) {
		DEBUG(0,("api_srv_net_disk_enum: Failed to marshall SRV_R_NET_DISK_ENUM.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 NetValidateName (opnum 0x21) 
*******************************************************************/

static BOOL api_srv_net_name_validate(pipes_struct *p) 
{
	SRV_Q_NET_NAME_VALIDATE q_u;
	SRV_R_NET_NAME_VALIDATE r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;
 
	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
  
	/* Unmarshall the net server disk enum. */
	if(!srv_io_q_net_name_validate("", &q_u, data, 0)) {
		DEBUG(0,("api_srv_net_name_validate: Failed to unmarshall SRV_Q_NET_NAME_VALIDATE.\n"));
		return False;
	}

	r_u.status = _srv_net_name_validate(p, &q_u, &r_u);

	if(!srv_io_r_net_name_validate("", &r_u, rdata, 0)) {
		DEBUG(0,("api_srv_net_name_validate: Failed to marshall SRV_R_NET_NAME_VALIDATE.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 NetFileQuerySecdesc (opnum 0x27)
*******************************************************************/

static BOOL api_srv_net_file_query_secdesc(pipes_struct *p)
{
	SRV_Q_NET_FILE_QUERY_SECDESC q_u;
	SRV_R_NET_FILE_QUERY_SECDESC r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* Unmarshall the net file get info from Win9x */
	if(!srv_io_q_net_file_query_secdesc("", &q_u, data, 0)) {
		DEBUG(0,("api_srv_net_file_query_secdesc: Failed to unmarshall SRV_Q_NET_FILE_QUERY_SECDESC.\n"));
		return False;
	}

	r_u.status = _srv_net_file_query_secdesc(p, &q_u, &r_u);

	if(!srv_io_r_net_file_query_secdesc("", &r_u, rdata, 0)) {
		DEBUG(0,("api_srv_net_file_query_secdesc: Failed to marshall SRV_R_NET_FILE_QUERY_SECDESC.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 NetFileSetSecdesc (opnum 0x28)
*******************************************************************/

static BOOL api_srv_net_file_set_secdesc(pipes_struct *p)
{
	SRV_Q_NET_FILE_SET_SECDESC q_u;
	SRV_R_NET_FILE_SET_SECDESC r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* Unmarshall the net file set info from Win9x */
	if(!srv_io_q_net_file_set_secdesc("", &q_u, data, 0)) {
		DEBUG(0,("api_srv_net_file_set_secdesc: Failed to unmarshall SRV_Q_NET_FILE_SET_SECDESC.\n"));
		return False;
	}

	r_u.status = _srv_net_file_set_secdesc(p, &q_u, &r_u);

	if(!srv_io_r_net_file_set_secdesc("", &r_u, rdata, 0)) {
		DEBUG(0,("api_srv_net_file_set_secdesc: Failed to marshall SRV_R_NET_FILE_SET_SECDESC.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
\PIPE\srvsvc commands
********************************************************************/

static struct api_struct api_srv_cmds[] =
{
      { "SRV_NET_CONN_ENUM"         , SRV_NET_CONN_ENUM         , api_srv_net_conn_enum          },
      { "SRV_NET_SESS_ENUM"         , SRV_NET_SESS_ENUM         , api_srv_net_sess_enum          },
      { "SRV_NET_SESS_DEL"          , SRV_NET_SESS_DEL          , api_srv_net_sess_del           },
      { "SRV_NET_SHARE_ENUM_ALL"    , SRV_NET_SHARE_ENUM_ALL    , api_srv_net_share_enum_all     },
      { "SRV_NET_SHARE_ENUM"        , SRV_NET_SHARE_ENUM        , api_srv_net_share_enum         },
      { "SRV_NET_SHARE_ADD"         , SRV_NET_SHARE_ADD         , api_srv_net_share_add          },
      { "SRV_NET_SHARE_DEL"         , SRV_NET_SHARE_DEL         , api_srv_net_share_del          },
      { "SRV_NET_SHARE_DEL_STICKY"  , SRV_NET_SHARE_DEL_STICKY  , api_srv_net_share_del_sticky   },
      { "SRV_NET_SHARE_GET_INFO"    , SRV_NET_SHARE_GET_INFO    , api_srv_net_share_get_info     },
      { "SRV_NET_SHARE_SET_INFO"    , SRV_NET_SHARE_SET_INFO    , api_srv_net_share_set_info     },
      { "SRV_NET_FILE_ENUM"         , SRV_NET_FILE_ENUM         , api_srv_net_file_enum          },
      { "SRV_NET_SRV_GET_INFO"      , SRV_NET_SRV_GET_INFO      , api_srv_net_srv_get_info       },
      { "SRV_NET_SRV_SET_INFO"      , SRV_NET_SRV_SET_INFO      , api_srv_net_srv_set_info       },
      { "SRV_NET_REMOTE_TOD"        , SRV_NET_REMOTE_TOD        , api_srv_net_remote_tod         },
      { "SRV_NET_DISK_ENUM"         , SRV_NET_DISK_ENUM         , api_srv_net_disk_enum          },
      { "SRV_NET_NAME_VALIDATE"     , SRV_NET_NAME_VALIDATE     , api_srv_net_name_validate      },
      { "SRV_NET_FILE_QUERY_SECDESC", SRV_NET_FILE_QUERY_SECDESC, api_srv_net_file_query_secdesc },
      { "SRV_NET_FILE_SET_SECDESC"  , SRV_NET_FILE_SET_SECDESC  , api_srv_net_file_set_secdesc   }
};

void srvsvc_get_pipe_fns( struct api_struct **fns, int *n_fns )
{
	*fns = api_srv_cmds;
	*n_fns = sizeof(api_srv_cmds) / sizeof(struct api_struct);
}


NTSTATUS rpc_srv_init(void)
{
  return rpc_pipe_register_commands(SMB_RPC_INTERFACE_VERSION, "srvsvc", "ntsvcs", api_srv_cmds,
				    sizeof(api_srv_cmds) / sizeof(struct api_struct));
}
