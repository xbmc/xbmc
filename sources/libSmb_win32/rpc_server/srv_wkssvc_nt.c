/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Andrew Tridgell              1992-1997,
 *  Copyright (C) Luke Kenneth Casson Leighton 1996-1997,
 *  Copyright (C) Paul Ashton                       1997.
 *  Copyright (C) Jeremy Allison					2001.
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

/* This is the implementation of the wks interface. */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

/*******************************************************************
 create_wks_info_100
 ********************************************************************/

static void create_wks_info_100(WKS_INFO_100 *inf)
{
	pstring my_name;
	pstring domain;

	DEBUG(5,("create_wks_info_100: %d\n", __LINE__));

	pstrcpy (my_name, global_myname());
	strupper_m(my_name);

	pstrcpy (domain, lp_workgroup());
	strupper_m(domain);

	init_wks_info_100(inf,
	                  0x000001f4, /* platform id info */
	                  lp_major_announce_version(),
	                  lp_minor_announce_version(),
	                  my_name, domain);
}

/*******************************************************************
 wks_reply_query_info
 
 only supports info level 100 at the moment.

 ********************************************************************/

NTSTATUS _wks_query_info(pipes_struct *p, WKS_Q_QUERY_INFO *q_u, WKS_R_QUERY_INFO *r_u)
{
	WKS_INFO_100 *wks100 = NULL;

	DEBUG(5,("_wks_query_info: %d\n", __LINE__));

	wks100 = TALLOC_ZERO_P(p->mem_ctx, WKS_INFO_100);

	if (!wks100)
		return NT_STATUS_NO_MEMORY;

	create_wks_info_100(wks100);
	init_wks_r_query_info(r_u, q_u->switch_value, wks100, NT_STATUS_OK);

	DEBUG(5,("_wks_query_info: %d\n", __LINE__));

	return r_u->status;
}
