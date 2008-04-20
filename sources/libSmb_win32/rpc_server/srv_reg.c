/*
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Andrew Tridgell              1992-1997,
 *  Copyright (C) Luke Kenneth Casson Leighton 1996-1997,
 *  Copyright (C) Paul Ashton                       1997,
 *  Copyright (C) Marc Jacobsen	   		    2000,
 *  Copyright (C) Jeremy Allison		    2001,
 *  Copyright (C) Gerald Carter 		    2002,
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

/* This is the interface for the registry functions. */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

/*******************************************************************
 api_reg_close
 ********************************************************************/

static BOOL api_reg_close(pipes_struct *p)
{
	REG_Q_CLOSE q_u;
	REG_R_CLOSE r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the reg unknown 1 */
	if(!reg_io_q_close("", &q_u, data, 0))
		return False;

	r_u.status = _reg_close(p, &q_u, &r_u);

	if(!reg_io_r_close("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 api_reg_open_khlm
 ********************************************************************/

static BOOL api_reg_open_hklm(pipes_struct *p)
{
	REG_Q_OPEN_HIVE q_u;
	REG_R_OPEN_HIVE r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the reg open */
	if(!reg_io_q_open_hive("", &q_u, data, 0))
		return False;

	r_u.status = _reg_open_hklm(p, &q_u, &r_u);

	if(!reg_io_r_open_hive("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 api_reg_open_khu
 ********************************************************************/

static BOOL api_reg_open_hku(pipes_struct *p)
{
	REG_Q_OPEN_HIVE q_u;
	REG_R_OPEN_HIVE r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the reg open */
	if(!reg_io_q_open_hive("", &q_u, data, 0))
		return False;

	r_u.status = _reg_open_hku(p, &q_u, &r_u);

	if(!reg_io_r_open_hive("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 api_reg_open_khcr
 ********************************************************************/

static BOOL api_reg_open_hkcr(pipes_struct *p)
{
	REG_Q_OPEN_HIVE q_u;
	REG_R_OPEN_HIVE r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the reg open */
	if(!reg_io_q_open_hive("", &q_u, data, 0))
		return False;

	r_u.status = _reg_open_hkcr(p, &q_u, &r_u);

	if(!reg_io_r_open_hive("", &r_u, rdata, 0))
		return False;

	return True;
}


/*******************************************************************
 api_reg_open_entry
 ********************************************************************/

static BOOL api_reg_open_entry(pipes_struct *p)
{
	REG_Q_OPEN_ENTRY q_u;
	REG_R_OPEN_ENTRY r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the reg open entry */
	if(!reg_io_q_open_entry("", &q_u, data, 0))
		return False;

	/* construct reply. */
	r_u.status = _reg_open_entry(p, &q_u, &r_u);

	if(!reg_io_r_open_entry("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 api_reg_query_value
 ********************************************************************/

static BOOL api_reg_query_value(pipes_struct *p)
{
	REG_Q_QUERY_VALUE q_u;
	REG_R_QUERY_VALUE r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the reg unknown 0x11*/
	if(!reg_io_q_query_value("", &q_u, data, 0))
		return False;

	r_u.status = _reg_query_value(p, &q_u, &r_u);

	if(!reg_io_r_query_value("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 api_reg_shutdown
 ********************************************************************/

static BOOL api_reg_shutdown(pipes_struct *p)
{
	REG_Q_SHUTDOWN q_u;
	REG_R_SHUTDOWN r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the reg shutdown */
	if(!reg_io_q_shutdown("", &q_u, data, 0))
		return False;

	r_u.status = _reg_shutdown(p, &q_u, &r_u);

	if(!reg_io_r_shutdown("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 api_reg_shutdown_ex
 ********************************************************************/

static BOOL api_reg_shutdown_ex(pipes_struct *p)
{
	REG_Q_SHUTDOWN_EX q_u;
	REG_R_SHUTDOWN_EX r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the reg shutdown ex */
	if(!reg_io_q_shutdown_ex("", &q_u, data, 0))
		return False;

	r_u.status = _reg_shutdown_ex(p, &q_u, &r_u);

	if(!reg_io_r_shutdown_ex("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 api_reg_abort_shutdown
 ********************************************************************/

static BOOL api_reg_abort_shutdown(pipes_struct *p)
{
	REG_Q_ABORT_SHUTDOWN q_u;
	REG_R_ABORT_SHUTDOWN r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the reg shutdown */
	if(!reg_io_q_abort_shutdown("", &q_u, data, 0))
		return False;

	r_u.status = _reg_abort_shutdown(p, &q_u, &r_u);

	if(!reg_io_r_abort_shutdown("", &r_u, rdata, 0))
		return False;

	return True;
}


/*******************************************************************
 api_reg_query_key
 ********************************************************************/

static BOOL api_reg_query_key(pipes_struct *p)
{
	REG_Q_QUERY_KEY q_u;
	REG_R_QUERY_KEY r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!reg_io_q_query_key("", &q_u, data, 0))
		return False;

	r_u.status = _reg_query_key(p, &q_u, &r_u);

	if(!reg_io_r_query_key("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 api_reg_getversion
 ********************************************************************/

static BOOL api_reg_getversion(pipes_struct *p)
{
	REG_Q_GETVERSION q_u;
	REG_R_GETVERSION r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!reg_io_q_getversion("", &q_u, data, 0))
		return False;

	r_u.status = _reg_getversion(p, &q_u, &r_u);

	if(!reg_io_r_getversion("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 api_reg_enum_key
 ********************************************************************/

static BOOL api_reg_enum_key(pipes_struct *p)
{
	REG_Q_ENUM_KEY q_u;
	REG_R_ENUM_KEY r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!reg_io_q_enum_key("", &q_u, data, 0))
		return False;

	r_u.status = _reg_enum_key(p, &q_u, &r_u);

	if(!reg_io_r_enum_key("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 api_reg_enum_value
 ********************************************************************/

static BOOL api_reg_enum_value(pipes_struct *p)
{
	REG_Q_ENUM_VALUE q_u;
	REG_R_ENUM_VALUE r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!reg_io_q_enum_val("", &q_u, data, 0))
		return False;
		
	r_u.status = _reg_enum_value(p, &q_u, &r_u);

	if(!reg_io_r_enum_val("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ******************************************************************/

static BOOL api_reg_restore_key(pipes_struct *p)
{
	REG_Q_RESTORE_KEY q_u;
	REG_R_RESTORE_KEY r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!reg_io_q_restore_key("", &q_u, data, 0))
		return False;

	r_u.status = _reg_restore_key(p, &q_u, &r_u);

	if(!reg_io_r_restore_key("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ********************************************************************/

static BOOL api_reg_save_key(pipes_struct *p)
{
	REG_Q_SAVE_KEY q_u;
	REG_R_SAVE_KEY r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!reg_io_q_save_key("", &q_u, data, 0))
		return False;
		
	r_u.status = _reg_save_key(p, &q_u, &r_u);

	if(!reg_io_r_save_key("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 api_reg_open_hkpd
 ********************************************************************/

static BOOL api_reg_open_hkpd(pipes_struct *p)
{
	REG_Q_OPEN_HIVE q_u;
	REG_R_OPEN_HIVE r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the reg open */
	if(!reg_io_q_open_hive("", &q_u, data, 0))
		return False;

	r_u.status = _reg_open_hkpd(p, &q_u, &r_u);

	if(!reg_io_r_open_hive("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 api_reg_open_hkpd
 ********************************************************************/
static BOOL api_reg_open_hkpt(pipes_struct *p)
{
	REG_Q_OPEN_HIVE q_u;
	REG_R_OPEN_HIVE r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the reg open */
	if(!reg_io_q_open_hive("", &q_u, data, 0))
		return False;

	r_u.status = _reg_open_hkpt(p, &q_u, &r_u);

	if(!reg_io_r_open_hive("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ******************************************************************/

static BOOL api_reg_create_key_ex(pipes_struct *p)
{
	REG_Q_CREATE_KEY_EX q_u;
	REG_R_CREATE_KEY_EX r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!reg_io_q_create_key_ex("", &q_u, data, 0))
		return False;
		
	r_u.status = _reg_create_key_ex(p, &q_u, &r_u);

	if(!reg_io_r_create_key_ex("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ******************************************************************/

static BOOL api_reg_set_value(pipes_struct *p)
{
	REG_Q_SET_VALUE q_u;
	REG_R_SET_VALUE r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!reg_io_q_set_value("", &q_u, data, 0))
		return False;
		
	r_u.status = _reg_set_value(p, &q_u, &r_u);

	if(!reg_io_r_set_value("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ******************************************************************/

static BOOL api_reg_delete_key(pipes_struct *p)
{
	REG_Q_DELETE_KEY q_u;
	REG_R_DELETE_KEY r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!reg_io_q_delete_key("", &q_u, data, 0))
		return False;
		
	r_u.status = _reg_delete_key(p, &q_u, &r_u);

	if(!reg_io_r_delete_key("", &r_u, rdata, 0))
		return False;

	return True;
}

/*******************************************************************
 ******************************************************************/

static BOOL api_reg_delete_value(pipes_struct *p)
{
	REG_Q_DELETE_VALUE q_u;
	REG_R_DELETE_VALUE r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!reg_io_q_delete_value("", &q_u, data, 0))
		return False;
		
	r_u.status = _reg_delete_value(p, &q_u, &r_u);

	if(!reg_io_r_delete_value("", &r_u, rdata, 0))
		return False;

	return True;
}


/*******************************************************************
 ******************************************************************/

static BOOL api_reg_get_key_sec(pipes_struct *p)
{
	REG_Q_GET_KEY_SEC q_u;
	REG_R_GET_KEY_SEC r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!reg_io_q_get_key_sec("", &q_u, data, 0))
		return False;
		
	r_u.status = _reg_get_key_sec(p, &q_u, &r_u);

	if(!reg_io_r_get_key_sec("", &r_u, rdata, 0))
		return False;

	return True;
}


/*******************************************************************
 ******************************************************************/

static BOOL api_reg_set_key_sec(pipes_struct *p)
{
	REG_Q_SET_KEY_SEC q_u;
	REG_R_SET_KEY_SEC r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!reg_io_q_set_key_sec("", &q_u, data, 0))
		return False;
		
	r_u.status = _reg_set_key_sec(p, &q_u, &r_u);

	if(!reg_io_r_set_key_sec("", &r_u, rdata, 0))
		return False;

	return True;
}


/*******************************************************************
 array of \PIPE\reg operations
 ********************************************************************/

static struct api_struct api_reg_cmds[] =
{
      { "REG_CLOSE"              , REG_CLOSE              , api_reg_close            },
      { "REG_OPEN_ENTRY"         , REG_OPEN_ENTRY         , api_reg_open_entry       },
      { "REG_OPEN_HKCR"          , REG_OPEN_HKCR          , api_reg_open_hkcr        },
      { "REG_OPEN_HKLM"          , REG_OPEN_HKLM          , api_reg_open_hklm        },
      { "REG_OPEN_HKPD"          , REG_OPEN_HKPD          , api_reg_open_hkpd        },
      { "REG_OPEN_HKPT"          , REG_OPEN_HKPT          , api_reg_open_hkpt        },
      { "REG_OPEN_HKU"           , REG_OPEN_HKU           , api_reg_open_hku         },
      { "REG_ENUM_KEY"           , REG_ENUM_KEY           , api_reg_enum_key         },
      { "REG_ENUM_VALUE"         , REG_ENUM_VALUE         , api_reg_enum_value       },
      { "REG_QUERY_KEY"          , REG_QUERY_KEY          , api_reg_query_key        },
      { "REG_QUERY_VALUE"        , REG_QUERY_VALUE        , api_reg_query_value      },
      { "REG_SHUTDOWN"           , REG_SHUTDOWN           , api_reg_shutdown         },
      { "REG_SHUTDOWN_EX"        , REG_SHUTDOWN_EX        , api_reg_shutdown_ex      },
      { "REG_ABORT_SHUTDOWN"     , REG_ABORT_SHUTDOWN     , api_reg_abort_shutdown   },
      { "REG_GETVERSION"         , REG_GETVERSION         , api_reg_getversion       },
      { "REG_SAVE_KEY"           , REG_SAVE_KEY           , api_reg_save_key         },
      { "REG_RESTORE_KEY"        , REG_RESTORE_KEY        , api_reg_restore_key      },
      { "REG_CREATE_KEY_EX"      , REG_CREATE_KEY_EX      , api_reg_create_key_ex    },
      { "REG_SET_VALUE"          , REG_SET_VALUE          , api_reg_set_value        },
      { "REG_DELETE_KEY"         , REG_DELETE_KEY         , api_reg_delete_key       },
      { "REG_DELETE_VALUE"       , REG_DELETE_VALUE       , api_reg_delete_value     },
      { "REG_GET_KEY_SEC"        , REG_GET_KEY_SEC        , api_reg_get_key_sec      },
      { "REG_SET_KEY_SEC"        , REG_SET_KEY_SEC        , api_reg_set_key_sec      }
};

void reg_get_pipe_fns( struct api_struct **fns, int *n_fns )
{
	*fns = api_reg_cmds;
	*n_fns = sizeof(api_reg_cmds) / sizeof(struct api_struct);
}
    
NTSTATUS rpc_reg_init(void)
{

  return rpc_pipe_register_commands(SMB_RPC_INTERFACE_VERSION, "winreg", "winreg", api_reg_cmds,
				    sizeof(api_reg_cmds) / sizeof(struct api_struct));
}
