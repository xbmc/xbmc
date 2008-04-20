/* 
   Unix SMB/CIFS implementation.
   client message handling routines
   Copyright (C) Andrew Tridgell 1994-1998
   
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
start a message sequence
****************************************************************************/
int cli_message_start_build(struct cli_state *cli, char *host, char *username)
{
	char *p;

	/* construct a SMBsendstrt command */
	memset(cli->outbuf,'\0',smb_size);
	set_message(cli->outbuf,0,0,True);
	SCVAL(cli->outbuf,smb_com,SMBsendstrt);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);
	
	p = smb_buf(cli->outbuf);
	*p++ = 4;
	p += clistr_push(cli, p, username, -1, STR_ASCII|STR_TERMINATE);
	*p++ = 4;
	p += clistr_push(cli, p, host, -1, STR_ASCII|STR_TERMINATE);
	
	cli_setup_bcc(cli, p);

	return(PTR_DIFF(p, cli->outbuf));
}

BOOL cli_message_start(struct cli_state *cli, char *host, char *username,
			      int *grp)
{
	cli_message_start_build(cli, host, username);
	
	cli_send_smb(cli);	
	
	if (!cli_receive_smb(cli)) {
		return False;
	}

	if (cli_is_error(cli)) return False;

	*grp = SVAL(cli->inbuf,smb_vwv0);

	return True;
}


/****************************************************************************
send a message 
****************************************************************************/
int cli_message_text_build(struct cli_state *cli, char *msg, int len, int grp)
{
	char *msgdos;
	int lendos;
	char *p;

	memset(cli->outbuf,'\0',smb_size);
	set_message(cli->outbuf,1,0,True);
	SCVAL(cli->outbuf,smb_com,SMBsendtxt);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SSVAL(cli->outbuf,smb_vwv0,grp);
	
	p = smb_buf(cli->outbuf);
	*p++ = 1;

	if ((lendos = (int)convert_string_allocate(NULL,CH_UNIX, CH_DOS, msg,len, (void **)(void *)&msgdos, True)) < 0 || !msgdos) {
		DEBUG(3,("Conversion failed, sending message in UNIX charset\n"));
		SSVAL(p, 0, len); p += 2;
		memcpy(p, msg, len);
		p += len;
	} else {
		SSVAL(p, 0, lendos); p += 2;
		memcpy(p, msgdos, lendos);
		p += lendos;
		SAFE_FREE(msgdos);
	}

	cli_setup_bcc(cli, p);

	return(PTR_DIFF(p, cli->outbuf));
}

BOOL cli_message_text(struct cli_state *cli, char *msg, int len, int grp)
{
	cli_message_text_build(cli, msg, len, grp);

	cli_send_smb(cli);

	if (!cli_receive_smb(cli)) {
		return False;
	}

	if (cli_is_error(cli)) return False;

	return True;
}      

/****************************************************************************
end a message 
****************************************************************************/
int cli_message_end_build(struct cli_state *cli, int grp)
{
	char *p;

	memset(cli->outbuf,'\0',smb_size);
	set_message(cli->outbuf,1,0,True);
	SCVAL(cli->outbuf,smb_com,SMBsendend);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);

	SSVAL(cli->outbuf,smb_vwv0,grp);

	cli_setup_packet(cli);

	p = smb_buf(cli->outbuf);

	return(PTR_DIFF(p, cli->outbuf));
}

BOOL cli_message_end(struct cli_state *cli, int grp)
{
	cli_message_end_build(cli, grp);

	cli_send_smb(cli);

	if (!cli_receive_smb(cli)) {
		return False;
	}

	if (cli_is_error(cli)) return False;

	return True;
}      
