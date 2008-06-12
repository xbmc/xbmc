/* 
   Unix SMB/CIFS implementation.
   NT Domain Authentication SMB / MSRPC client
   Copyright (C) Andrew Tridgell 1994-1997
   Copyright (C) Luke Kenneth Casson Leighton 1996-1997
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

THIS IS NO LONGER USED - NEEDS REMOVAL.

#include "includes.h"

#define DEBUG_TESTING

extern struct cli_state *smb_cli;

extern FILE* out_hnd;


/****************************************************************************
workstation get info query
****************************************************************************/
void cmd_wks_query_info(struct client_info *info)
{
	fstring dest_wks;
	fstring tmp;
	WKS_INFO_100 ctr;
	uint32 info_level = 100;

	BOOL res = True;

	memset((char *)&ctr, '\0', sizeof(ctr));

	fstrcpy(dest_wks, "\\\\");
	fstrcat(dest_wks, info->dest_host);
	strupper_m(dest_wks);

	if (next_token_nr(NULL, tmp, NULL, sizeof(tmp)))
	{
		info_level = (uint32)strtol(tmp, (char**)NULL, 10);
	}

	DEBUG(4,("cmd_wks_query_info: server:%s info level: %d\n",
				dest_wks, info_level));

	DEBUG(5, ("cmd_wks_query_info: smb_cli->fd:%d\n", smb_cli->fd));

	/* open LSARPC session. */
	res = res ? cli_nt_session_open(smb_cli, PI_WKSSVC) : False;

	/* send info level: receive requested info.  hopefully. */
	res = res ? do_wks_query_info(smb_cli, 
				dest_wks, info_level, &ctr) : False;

	/* close the session */
	cli_nt_session_close(smb_cli);

	if (res)
	{
		DEBUG(5,("cmd_wks_query_info: query succeeded\n"));

#if 0
		display_wks_info_100(out_hnd, ACTION_HEADER   , &ctr);
		display_wks_info_100(out_hnd, ACTION_ENUMERATE, &ctr);
		display_wks_info_100(out_hnd, ACTION_FOOTER   , &ctr);
#endif

	}
	else
	{
		DEBUG(5,("cmd_wks_query_info: query failed\n"));
	}
}
