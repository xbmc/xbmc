/* 
   Unix SMB/CIFS implementation.
   SMB messaging
   Copyright (C) Andrew Tridgell 1992-1998
   
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
/*
   This file handles the messaging system calls for winpopup style
   messages
*/


#include "includes.h"

extern userdom_struct current_user_info;

/* look in server.c for some explanation of these variables */
static char msgbuf[1600];
static int msgpos;
static fstring msgfrom;
static fstring msgto;

/****************************************************************************
 Deliver the message.
****************************************************************************/

static void msg_deliver(void)
{
	pstring name;
	int i;
	int fd;
	char *msg;
	int len;
	ssize_t sz;

	if (! (*lp_msg_command())) {
		DEBUG(1,("no messaging command specified\n"));
		msgpos = 0;
		return;
	}

	/* put it in a temporary file */
	slprintf(name,sizeof(name)-1, "%s/msg.XXXXXX",tmpdir());
	fd = smb_mkstemp(name);

	if (fd == -1) {
		DEBUG(1,("can't open message file %s\n",name));
		return;
	}

	/*
	 * Incoming message is in DOS codepage format. Convert to UNIX.
	 */
  
	if ((len = (int)convert_string_allocate(NULL,CH_DOS, CH_UNIX, msgbuf, msgpos, (void **)(void *)&msg, True)) < 0 || !msg) {
		DEBUG(3,("Conversion failed, delivering message in DOS codepage format\n"));
		for (i = 0; i < msgpos;) {
			if (msgbuf[i] == '\r' && i < (msgpos-1) && msgbuf[i+1] == '\n') {
				i++;
				continue;
			}
			sz = write(fd, &msgbuf[i++], 1);
			if ( sz != 1 ) {
				DEBUG(0,("Write error to fd %d: %ld(%d)\n",fd, (long)sz, errno ));
			}
		}
	} else {
		for (i = 0; i < len;) {
			if (msg[i] == '\r' && i < (len-1) && msg[i+1] == '\n') {
				i++;
				continue;
			}
			sz = write(fd, &msg[i++],1);
			if ( sz != 1 ) {
				DEBUG(0,("Write error to fd %d: %ld(%d)\n",fd, (long)sz, errno ));
			}
		}
		SAFE_FREE(msg);
	}
	close(fd);

	/* run the command */
	if (*lp_msg_command()) {
		fstring alpha_msgfrom;
		fstring alpha_msgto;
		pstring s;

		pstrcpy(s,lp_msg_command());
		pstring_sub(s,"%f",alpha_strcpy(alpha_msgfrom,msgfrom,NULL,sizeof(alpha_msgfrom)));
		pstring_sub(s,"%t",alpha_strcpy(alpha_msgto,msgto,NULL,sizeof(alpha_msgto)));
		standard_sub_basic(current_user_info.smb_name, s, sizeof(s));
		pstring_sub(s,"%s",name);
		smbrun(s,NULL);
	}

	msgpos = 0;
}

/****************************************************************************
 Reply to a sends.
 conn POINTER CAN BE NULL HERE !
****************************************************************************/

int reply_sends(connection_struct *conn, char *inbuf,char *outbuf, int dum_size, int dum_buffsize)
{
	int len;
	char *msg;
	int outsize = 0;
	char *p;

	START_PROFILE(SMBsends);

	msgpos = 0;

	if (! (*lp_msg_command())) {
		END_PROFILE(SMBsends);
		return(ERROR_DOS(ERRSRV,ERRmsgoff));
	}

	outsize = set_message(outbuf,0,0,True);

	p = smb_buf(inbuf)+1;
	p += srvstr_pull_buf(inbuf, msgfrom, p, sizeof(msgfrom), STR_ASCII|STR_TERMINATE) + 1;
	p += srvstr_pull_buf(inbuf, msgto, p, sizeof(msgto), STR_ASCII|STR_TERMINATE) + 1;

	msg = p;

	len = SVAL(msg,0);
	len = MIN(len,sizeof(msgbuf)-msgpos);

	memset(msgbuf,'\0',sizeof(msgbuf));

	memcpy(&msgbuf[msgpos],msg+2,len);
	msgpos += len;

	msg_deliver();

	END_PROFILE(SMBsends);
	return(outsize);
}

/****************************************************************************
 Reply to a sendstrt.
 conn POINTER CAN BE NULL HERE !
****************************************************************************/

int reply_sendstrt(connection_struct *conn, char *inbuf,char *outbuf, int dum_size, int dum_buffsize)
{
	int outsize = 0;
	char *p;

	START_PROFILE(SMBsendstrt);

	if (! (*lp_msg_command())) {
		END_PROFILE(SMBsendstrt);
		return(ERROR_DOS(ERRSRV,ERRmsgoff));
	}

	outsize = set_message(outbuf,1,0,True);

	memset(msgbuf,'\0',sizeof(msgbuf));
	msgpos = 0;

	p = smb_buf(inbuf)+1;
	p += srvstr_pull_buf(inbuf, msgfrom, p, sizeof(msgfrom), STR_ASCII|STR_TERMINATE) + 1;
	p += srvstr_pull_buf(inbuf, msgto, p, sizeof(msgto), STR_ASCII|STR_TERMINATE) + 1;

	DEBUG( 3, ( "SMBsendstrt (from %s to %s)\n", msgfrom, msgto ) );

	END_PROFILE(SMBsendstrt);
	return(outsize);
}

/****************************************************************************
 Reply to a sendtxt.
 conn POINTER CAN BE NULL HERE !
****************************************************************************/

int reply_sendtxt(connection_struct *conn, char *inbuf,char *outbuf, int dum_size, int dum_buffsize)
{
	int len;
	int outsize = 0;
	char *msg;
	START_PROFILE(SMBsendtxt);

	if (! (*lp_msg_command())) {
		END_PROFILE(SMBsendtxt);
		return(ERROR_DOS(ERRSRV,ERRmsgoff));
	}

	outsize = set_message(outbuf,0,0,True);

	msg = smb_buf(inbuf) + 1;

	len = SVAL(msg,0);
	len = MIN(len,sizeof(msgbuf)-msgpos);

	memcpy(&msgbuf[msgpos],msg+2,len);
	msgpos += len;

	DEBUG( 3, ( "SMBsendtxt\n" ) );

	END_PROFILE(SMBsendtxt);
	return(outsize);
}

/****************************************************************************
 Reply to a sendend.
 conn POINTER CAN BE NULL HERE !
****************************************************************************/

int reply_sendend(connection_struct *conn, char *inbuf,char *outbuf, int dum_size, int dum_buffsize)
{
	int outsize = 0;
	START_PROFILE(SMBsendend);

	if (! (*lp_msg_command())) {
		END_PROFILE(SMBsendend);
		return(ERROR_DOS(ERRSRV,ERRmsgoff));
	}

	outsize = set_message(outbuf,0,0,True);

	DEBUG(3,("SMBsendend\n"));

	msg_deliver();

	END_PROFILE(SMBsendend);
	return(outsize);
}
