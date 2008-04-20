/*
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Gerald Carter		2003
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

#if 0	/* disabled */
/*******************************************************************
 api_reg_open_entry
 ********************************************************************/

static BOOL api_dsrole_get_primary_dominfo(pipes_struct *p)
{
	DS_Q_GETPRIMDOMINFO q_u;
	DS_R_GETPRIMDOMINFO r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the request */
	if ( !ds_io_q_getprimdominfo("", data, 0, &q_u) )
		return False;

	/* construct reply. */
	r_u.status = _dsrole_get_primary_dominfo( p, &q_u, &r_u );

	if ( !ds_io_r_getprimdominfo("", rdata, 0, &r_u) )
		return False;

	return True;
}
#endif

/*******************************************************************
 stub functions for unimplemented RPC
*******************************************************************/

static BOOL api_dsrole_stub( pipes_struct *p )
{
	DEBUG(0,("api_dsrole_stub:  Hmmm....didn't know this RPC existed...\n"));

	return False;
}


/*******************************************************************
 array of \PIPE\lsass (new windows 2000 UUID)  operations
********************************************************************/
static struct api_struct api_lsa_ds_cmds[] = {
	{ "DS_NOP", 			DS_NOP, 		api_dsrole_stub }

#if 0	/* disabled due to breakage with viewing domain users and groups 
	   on a Samba PDC from win2k clients  --jerry CIFS 2003 */
	{ "DS_GETPRIMDOMINFO", 		DS_GETPRIMDOMINFO, 	api_dsrole_get_primary_dominfo	}
#endif

};

void lsa_ds_get_pipe_fns( struct api_struct **fns, int *n_fns )
{
	*fns = api_lsa_ds_cmds;
	*n_fns = sizeof(api_lsa_ds_cmds) / sizeof(struct api_struct);
}


NTSTATUS rpc_lsa_ds_init(void)
{
	return rpc_pipe_register_commands(SMB_RPC_INTERFACE_VERSION, "lsa_ds", "lsa_ds", api_lsa_ds_cmds,
		sizeof(api_lsa_ds_cmds) / sizeof(struct api_struct));
}
