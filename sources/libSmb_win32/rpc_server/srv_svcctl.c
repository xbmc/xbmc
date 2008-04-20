/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Gerald Carter                   2005.
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

/*******************************************************************
 ********************************************************************/

static BOOL api_svcctl_close_service(pipes_struct *p)
{
	SVCCTL_Q_CLOSE_SERVICE q_u;
	SVCCTL_R_CLOSE_SERVICE r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!svcctl_io_q_close_service("", &q_u, data, 0))
		return False;

	r_u.status = _svcctl_close_service(p, &q_u, &r_u);

	if(!svcctl_io_r_close_service("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ********************************************************************/

static BOOL api_svcctl_open_scmanager(pipes_struct *p)
{
	SVCCTL_Q_OPEN_SCMANAGER q_u;
	SVCCTL_R_OPEN_SCMANAGER r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!svcctl_io_q_open_scmanager("", &q_u, data, 0))
		return False;

	r_u.status = _svcctl_open_scmanager(p, &q_u, &r_u);

	if(!svcctl_io_r_open_scmanager("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ********************************************************************/

static BOOL api_svcctl_open_service(pipes_struct *p)
{
	SVCCTL_Q_OPEN_SERVICE q_u;
	SVCCTL_R_OPEN_SERVICE r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!svcctl_io_q_open_service("", &q_u, data, 0))
		return False;

	r_u.status = _svcctl_open_service(p, &q_u, &r_u);

	if(!svcctl_io_r_open_service("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ********************************************************************/

static BOOL api_svcctl_get_display_name(pipes_struct *p)
{
	SVCCTL_Q_GET_DISPLAY_NAME q_u;
	SVCCTL_R_GET_DISPLAY_NAME r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!svcctl_io_q_get_display_name("", &q_u, data, 0))
		return False;

	r_u.status = _svcctl_get_display_name(p, &q_u, &r_u);

	if(!svcctl_io_r_get_display_name("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ********************************************************************/

static BOOL api_svcctl_query_status(pipes_struct *p)
{
	SVCCTL_Q_QUERY_STATUS q_u;
	SVCCTL_R_QUERY_STATUS r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!svcctl_io_q_query_status("", &q_u, data, 0))
		return False;

	r_u.status = _svcctl_query_status(p, &q_u, &r_u);

	if(!svcctl_io_r_query_status("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ********************************************************************/

static BOOL api_svcctl_enum_services_status(pipes_struct *p)
{
	SVCCTL_Q_ENUM_SERVICES_STATUS q_u;
	SVCCTL_R_ENUM_SERVICES_STATUS r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!svcctl_io_q_enum_services_status("", &q_u, data, 0))
		return False;

	r_u.status = _svcctl_enum_services_status(p, &q_u, &r_u);

	if(!svcctl_io_r_enum_services_status("", &r_u, rdata, 0))
		return False;

	return True;
}
/*******************************************************************
 ********************************************************************/

static BOOL api_svcctl_query_service_status_ex(pipes_struct *p)
{
	SVCCTL_Q_QUERY_SERVICE_STATUSEX q_u;
	SVCCTL_R_QUERY_SERVICE_STATUSEX r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!svcctl_io_q_query_service_status_ex("", &q_u, data, 0))
		return False;

	r_u.status = _svcctl_query_service_status_ex(p, &q_u, &r_u);

	if(!svcctl_io_r_query_service_status_ex("", &r_u, rdata, 0))
		return False;

	return True;
}
/*******************************************************************
 ********************************************************************/

static BOOL api_svcctl_enum_dependent_services(pipes_struct *p)
{
	SVCCTL_Q_ENUM_DEPENDENT_SERVICES q_u;
	SVCCTL_R_ENUM_DEPENDENT_SERVICES r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!svcctl_io_q_enum_dependent_services("", &q_u, data, 0))
		return False;

	r_u.status = _svcctl_enum_dependent_services(p, &q_u, &r_u);

	if(!svcctl_io_r_enum_dependent_services("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ********************************************************************/

static BOOL api_svcctl_start_service(pipes_struct *p)
{
	SVCCTL_Q_START_SERVICE q_u;
	SVCCTL_R_START_SERVICE r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!svcctl_io_q_start_service("", &q_u, data, 0))
		return False;

	r_u.status = _svcctl_start_service(p, &q_u, &r_u);

	if(!svcctl_io_r_start_service("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ********************************************************************/

static BOOL api_svcctl_control_service(pipes_struct *p)
{
	SVCCTL_Q_CONTROL_SERVICE q_u;
	SVCCTL_R_CONTROL_SERVICE r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!svcctl_io_q_control_service("", &q_u, data, 0))
		return False;

	r_u.status = _svcctl_control_service(p, &q_u, &r_u);

	if(!svcctl_io_r_control_service("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ********************************************************************/

static BOOL api_svcctl_query_service_config(pipes_struct *p)
{
	SVCCTL_Q_QUERY_SERVICE_CONFIG q_u;
	SVCCTL_R_QUERY_SERVICE_CONFIG r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!svcctl_io_q_query_service_config("", &q_u, data, 0))
		return False;

	r_u.status = _svcctl_query_service_config(p, &q_u, &r_u);

	if(!svcctl_io_r_query_service_config("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ********************************************************************/

static BOOL api_svcctl_query_service_config2(pipes_struct *p)
{
	SVCCTL_Q_QUERY_SERVICE_CONFIG2 q_u;
	SVCCTL_R_QUERY_SERVICE_CONFIG2 r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!svcctl_io_q_query_service_config2("", &q_u, data, 0))
		return False;

	r_u.status = _svcctl_query_service_config2(p, &q_u, &r_u);

	if(!svcctl_io_r_query_service_config2("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ********************************************************************/

static BOOL api_svcctl_lock_service_db(pipes_struct *p)
{
	SVCCTL_Q_LOCK_SERVICE_DB q_u;
	SVCCTL_R_LOCK_SERVICE_DB r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!svcctl_io_q_lock_service_db("", &q_u, data, 0))
		return False;

	r_u.status = _svcctl_lock_service_db(p, &q_u, &r_u);

	if(!svcctl_io_r_lock_service_db("", &r_u, rdata, 0))
		return False;

	return True;
}


/*******************************************************************
 ********************************************************************/

static BOOL api_svcctl_unlock_service_db(pipes_struct *p)
{
	SVCCTL_Q_UNLOCK_SERVICE_DB q_u;
	SVCCTL_R_UNLOCK_SERVICE_DB r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!svcctl_io_q_unlock_service_db("", &q_u, data, 0))
		return False;

	r_u.status = _svcctl_unlock_service_db(p, &q_u, &r_u);

	if(!svcctl_io_r_unlock_service_db("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ********************************************************************/

static BOOL api_svcctl_query_security_sec(pipes_struct *p)
{
	SVCCTL_Q_QUERY_SERVICE_SEC q_u;
	SVCCTL_R_QUERY_SERVICE_SEC r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!svcctl_io_q_query_service_sec("", &q_u, data, 0))
		return False;

	r_u.status = _svcctl_query_service_sec(p, &q_u, &r_u);

	if(!svcctl_io_r_query_service_sec("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ********************************************************************/

static BOOL api_svcctl_set_security_sec(pipes_struct *p)
{
	SVCCTL_Q_SET_SERVICE_SEC q_u;
	SVCCTL_R_SET_SERVICE_SEC r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!svcctl_io_q_set_service_sec("", &q_u, data, 0))
		return False;

	r_u.status = _svcctl_set_service_sec(p, &q_u, &r_u);

	if(!svcctl_io_r_set_service_sec("", &r_u, rdata, 0))
		return False;

	return True;
}


/*******************************************************************
 \PIPE\svcctl commands
 ********************************************************************/

static struct api_struct api_svcctl_cmds[] =
{
      { "SVCCTL_CLOSE_SERVICE"              , SVCCTL_CLOSE_SERVICE              , api_svcctl_close_service },
      { "SVCCTL_OPEN_SCMANAGER_W"           , SVCCTL_OPEN_SCMANAGER_W           , api_svcctl_open_scmanager },
      { "SVCCTL_OPEN_SERVICE_W"             , SVCCTL_OPEN_SERVICE_W             , api_svcctl_open_service },
      { "SVCCTL_GET_DISPLAY_NAME"           , SVCCTL_GET_DISPLAY_NAME           , api_svcctl_get_display_name },
      { "SVCCTL_QUERY_STATUS"               , SVCCTL_QUERY_STATUS               , api_svcctl_query_status },
      { "SVCCTL_QUERY_SERVICE_CONFIG_W"     , SVCCTL_QUERY_SERVICE_CONFIG_W     , api_svcctl_query_service_config },
      { "SVCCTL_QUERY_SERVICE_CONFIG2_W"    , SVCCTL_QUERY_SERVICE_CONFIG2_W    , api_svcctl_query_service_config2 },
      { "SVCCTL_ENUM_SERVICES_STATUS_W"     , SVCCTL_ENUM_SERVICES_STATUS_W     , api_svcctl_enum_services_status },
      { "SVCCTL_ENUM_DEPENDENT_SERVICES_W"  , SVCCTL_ENUM_DEPENDENT_SERVICES_W  , api_svcctl_enum_dependent_services },
      { "SVCCTL_START_SERVICE_W"            , SVCCTL_START_SERVICE_W            , api_svcctl_start_service },
      { "SVCCTL_CONTROL_SERVICE"            , SVCCTL_CONTROL_SERVICE            , api_svcctl_control_service },
      { "SVCCTL_QUERY_SERVICE_STATUSEX_W"   , SVCCTL_QUERY_SERVICE_STATUSEX_W   , api_svcctl_query_service_status_ex },
      { "SVCCTL_LOCK_SERVICE_DB"            , SVCCTL_LOCK_SERVICE_DB            , api_svcctl_lock_service_db },
      { "SVCCTL_UNLOCK_SERVICE_DB"          , SVCCTL_UNLOCK_SERVICE_DB          , api_svcctl_unlock_service_db },
      { "SVCCTL_QUERY_SERVICE_SEC"          , SVCCTL_QUERY_SERVICE_SEC          , api_svcctl_query_security_sec },
      { "SVCCTL_SET_SERVICE_SEC"            , SVCCTL_SET_SERVICE_SEC            , api_svcctl_set_security_sec }
};


void svcctl_get_pipe_fns( struct api_struct **fns, int *n_fns )
{
	*fns = api_svcctl_cmds;
	*n_fns = sizeof(api_svcctl_cmds) / sizeof(struct api_struct);
}

NTSTATUS rpc_svcctl_init(void)
{
  return rpc_pipe_register_commands(SMB_RPC_INTERFACE_VERSION, "svcctl", "ntsvcs", api_svcctl_cmds,
				    sizeof(api_svcctl_cmds) / sizeof(struct api_struct));
}
