/* 
   Unix SMB/CIFS implementation.
   SMB client oplock functions
   Copyright (C) Andrew Tridgell 2001
   
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

#include "includes.h"

/****************************************************************************
send an ack for an oplock break request
****************************************************************************/
BOOL cli_oplock_ack(struct cli_state *cli, int fnum, unsigned char level)
{
	char *oldbuf = cli->outbuf;
	pstring buf;
	BOOL ret;

	cli->outbuf = buf;

        memset(buf,'\0',smb_size);
        set_message(buf,8,0,True);

        SCVAL(buf,smb_com,SMBlockingX);
	SSVAL(buf,smb_tid, cli->cnum);
        cli_setup_packet(cli);
	SSVAL(buf,smb_vwv0,0xFF);
	SSVAL(buf,smb_vwv1,0);
	SSVAL(buf,smb_vwv2,fnum);
	if (level == 1)
		SSVAL(buf,smb_vwv3,0x102); /* levelII oplock break ack */
	else
		SSVAL(buf,smb_vwv3,2); /* exclusive oplock break ack */
	SIVAL(buf,smb_vwv4,0); /* timoeut */
	SSVAL(buf,smb_vwv6,0); /* unlockcount */
	SSVAL(buf,smb_vwv7,0); /* lockcount */

        ret = cli_send_smb(cli);	

	cli->outbuf = oldbuf;

	return ret;
}


/****************************************************************************
set the oplock handler for a connection
****************************************************************************/
void cli_oplock_handler(struct cli_state *cli, 
			BOOL (*handler)(struct cli_state *, int, unsigned char))
{
	cli->oplock_handler = handler;
}
