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

static BOOL api_ntsvcs_get_version(pipes_struct *p)
{
	NTSVCS_Q_GET_VERSION q_u;
	NTSVCS_R_GET_VERSION r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!ntsvcs_io_q_get_version("", &q_u, data, 0))
		return False;

	r_u.status = _ntsvcs_get_version(p, &q_u, &r_u);

	if(!ntsvcs_io_r_get_version("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ********************************************************************/

static BOOL api_ntsvcs_get_device_list_size(pipes_struct *p)
{
	NTSVCS_Q_GET_DEVICE_LIST_SIZE q_u;
	NTSVCS_R_GET_DEVICE_LIST_SIZE r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!ntsvcs_io_q_get_device_list_size("", &q_u, data, 0))
		return False;

	r_u.status = _ntsvcs_get_device_list_size(p, &q_u, &r_u);

	if(!ntsvcs_io_r_get_device_list_size("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ********************************************************************/

static BOOL api_ntsvcs_get_device_list(pipes_struct *p)
{
	NTSVCS_Q_GET_DEVICE_LIST q_u;
	NTSVCS_R_GET_DEVICE_LIST r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!ntsvcs_io_q_get_device_list("", &q_u, data, 0))
		return False;

	r_u.status = _ntsvcs_get_device_list(p, &q_u, &r_u);

	if(!ntsvcs_io_r_get_device_list("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ********************************************************************/

static BOOL api_ntsvcs_validate_device_instance(pipes_struct *p)
{
	NTSVCS_Q_VALIDATE_DEVICE_INSTANCE q_u;
	NTSVCS_R_VALIDATE_DEVICE_INSTANCE r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!ntsvcs_io_q_validate_device_instance("", &q_u, data, 0))
		return False;

	r_u.status = _ntsvcs_validate_device_instance(p, &q_u, &r_u);

	if(!ntsvcs_io_r_validate_device_instance("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ********************************************************************/

static BOOL api_ntsvcs_get_device_reg_property(pipes_struct *p)
{
	NTSVCS_Q_GET_DEVICE_REG_PROPERTY q_u;
	NTSVCS_R_GET_DEVICE_REG_PROPERTY r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!ntsvcs_io_q_get_device_reg_property("", &q_u, data, 0))
		return False;

	r_u.status = _ntsvcs_get_device_reg_property(p, &q_u, &r_u);

	if(!ntsvcs_io_r_get_device_reg_property("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ********************************************************************/

static BOOL api_ntsvcs_get_hw_profile_info(pipes_struct *p)
{
	NTSVCS_Q_GET_HW_PROFILE_INFO q_u;
	NTSVCS_R_GET_HW_PROFILE_INFO r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!ntsvcs_io_q_get_hw_profile_info("", &q_u, data, 0))
		return False;

	r_u.status = _ntsvcs_get_hw_profile_info(p, &q_u, &r_u);

	if(!ntsvcs_io_r_get_hw_profile_info("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ********************************************************************/

static BOOL api_ntsvcs_hw_profile_flags(pipes_struct *p)
{
	NTSVCS_Q_HW_PROFILE_FLAGS q_u;
	NTSVCS_R_HW_PROFILE_FLAGS r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!ntsvcs_io_q_hw_profile_flags("", &q_u, data, 0))
		return False;

	r_u.status = _ntsvcs_hw_profile_flags(p, &q_u, &r_u);

	if(!ntsvcs_io_r_hw_profile_flags("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 \PIPE\svcctl commands
 ********************************************************************/

static struct api_struct api_ntsvcs_cmds[] =
{
      { "NTSVCS_GET_VERSION"              , NTSVCS_GET_VERSION              , api_ntsvcs_get_version },
      { "NTSVCS_GET_DEVICE_LIST_SIZE"     , NTSVCS_GET_DEVICE_LIST_SIZE     , api_ntsvcs_get_device_list_size },
      { "NTSVCS_GET_DEVICE_LIST"          , NTSVCS_GET_DEVICE_LIST          , api_ntsvcs_get_device_list },
      { "NTSVCS_VALIDATE_DEVICE_INSTANCE" , NTSVCS_VALIDATE_DEVICE_INSTANCE , api_ntsvcs_validate_device_instance },
      { "NTSVCS_GET_DEVICE_REG_PROPERTY"  , NTSVCS_GET_DEVICE_REG_PROPERTY  , api_ntsvcs_get_device_reg_property },
      { "NTSVCS_GET_HW_PROFILE_INFO"      , NTSVCS_GET_HW_PROFILE_INFO      , api_ntsvcs_get_hw_profile_info },
      { "NTSVCS_HW_PROFILE_FLAGS"         , NTSVCS_HW_PROFILE_FLAGS         , api_ntsvcs_hw_profile_flags }
};


void ntsvcs_get_pipe_fns( struct api_struct **fns, int *n_fns )
{
	*fns = api_ntsvcs_cmds;
	*n_fns = sizeof(api_ntsvcs_cmds) / sizeof(struct api_struct);
}

NTSTATUS rpc_ntsvcs_init(void)
{
  return rpc_pipe_register_commands(SMB_RPC_INTERFACE_VERSION, "ntsvcs", "ntsvcs", api_ntsvcs_cmds,
				    sizeof(api_ntsvcs_cmds) / sizeof(struct api_struct));
}
