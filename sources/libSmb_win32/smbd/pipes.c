/* 
   Unix SMB/CIFS implementation.
   Pipe SMB reply routines
   Copyright (C) Andrew Tridgell 1992-1998
   Copyright (C) Luke Kenneth Casson Leighton 1996-1998
   Copyright (C) Paul Ashton  1997-1998.
   Copyright (C) Jeremy Allison 2005.
   
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
   This file handles reply_ calls on named pipes that the server
   makes to handle specific protocols
*/


#include "includes.h"

#define	PIPE		"\\PIPE\\"
#define	PIPELEN		strlen(PIPE)

extern struct pipe_id_info pipe_names[];

/****************************************************************************
 Reply to an open and X on a named pipe.
 This code is basically stolen from reply_open_and_X with some
 wrinkles to handle pipes.
****************************************************************************/

int reply_open_pipe_and_X(connection_struct *conn,
			  char *inbuf,char *outbuf,int length,int bufsize)
{
	pstring fname;
	pstring pipe_name;
	uint16 vuid = SVAL(inbuf, smb_uid);
	smb_np_struct *p;
	int size=0,fmode=0,mtime=0,rmode=0;
	int i;

	/* XXXX we need to handle passed times, sattr and flags */
	srvstr_pull_buf(inbuf, pipe_name, smb_buf(inbuf), sizeof(pipe_name), STR_TERMINATE);

	/* If the name doesn't start \PIPE\ then this is directed */
	/* at a mailslot or something we really, really don't understand, */
	/* not just something we really don't understand. */
	if ( strncmp(pipe_name,PIPE,PIPELEN) != 0 ) {
		return(ERROR_DOS(ERRSRV,ERRaccess));
	}

	DEBUG(4,("Opening pipe %s.\n", pipe_name));

	/* See if it is one we want to handle. */
	for( i = 0; pipe_names[i].client_pipe ; i++ ) {
		if( strequal(pipe_name,pipe_names[i].client_pipe)) {
			break;
		}
	}

	if (pipe_names[i].client_pipe == NULL) {
		return(ERROR_BOTH(NT_STATUS_OBJECT_NAME_NOT_FOUND,ERRDOS,ERRbadpipe));
	}

	/* Strip \PIPE\ off the name. */
	pstrcpy(fname, pipe_name + PIPELEN);

#if 0
	/*
	 * Hack for NT printers... JRA.
	 */
    if(should_fail_next_srvsvc_open(fname))
      return(ERROR(ERRSRV,ERRaccess));
#endif

	/* Known pipes arrive with DIR attribs. Remove it so a regular file */
	/* can be opened and add it in after the open. */
	DEBUG(3,("Known pipe %s opening.\n",fname));

	p = open_rpc_pipe_p(fname, conn, vuid);
	if (!p) {
		return(ERROR_DOS(ERRSRV,ERRnofids));
	}

	/* Prepare the reply */
	set_message(outbuf,15,0,True);

	/* Mark the opened file as an existing named pipe in message mode. */
	SSVAL(outbuf,smb_vwv9,2);
	SSVAL(outbuf,smb_vwv10,0xc700);

	if (rmode == 2) {
		DEBUG(4,("Resetting open result to open from create.\n"));
		rmode = 1;
	}

	SSVAL(outbuf,smb_vwv2, p->pnum);
	SSVAL(outbuf,smb_vwv3,fmode);
	srv_put_dos_date3(outbuf,smb_vwv4,mtime);
	SIVAL(outbuf,smb_vwv6,size);
	SSVAL(outbuf,smb_vwv8,rmode);
	SSVAL(outbuf,smb_vwv11,0x0001);

	return chain_reply(inbuf,outbuf,length,bufsize);
}

/****************************************************************************
 Reply to a write on a pipe.
****************************************************************************/

int reply_pipe_write(char *inbuf,char *outbuf,int length,int dum_bufsize)
{
	smb_np_struct *p = get_rpc_pipe_p(inbuf,smb_vwv0);
	uint16 vuid = SVAL(inbuf,smb_uid);
	size_t numtowrite = SVAL(inbuf,smb_vwv1);
	int nwritten;
	int outsize;
	char *data;

	if (!p) {
		return(ERROR_DOS(ERRDOS,ERRbadfid));
	}

	if (p->vuid != vuid) {
		return ERROR_NT(NT_STATUS_INVALID_HANDLE);
	}

	data = smb_buf(inbuf) + 3;

	if (numtowrite == 0) {
		nwritten = 0;
	} else {
		nwritten = write_to_pipe(p, data, numtowrite);
	}

	if ((nwritten == 0 && numtowrite != 0) || (nwritten < 0)) {
		return (UNIXERROR(ERRDOS,ERRnoaccess));
	}
  
	outsize = set_message(outbuf,1,0,True);

	SSVAL(outbuf,smb_vwv0,nwritten);
  
	DEBUG(3,("write-IPC pnum=%04x nwritten=%d\n", p->pnum, nwritten));

	return(outsize);
}

/****************************************************************************
 Reply to a write and X.

 This code is basically stolen from reply_write_and_X with some
 wrinkles to handle pipes.
****************************************************************************/

int reply_pipe_write_and_X(char *inbuf,char *outbuf,int length,int bufsize)
{
	smb_np_struct *p = get_rpc_pipe_p(inbuf,smb_vwv2);
	uint16 vuid = SVAL(inbuf,smb_uid);
	size_t numtowrite = SVAL(inbuf,smb_vwv10);
	int nwritten = -1;
	int smb_doff = SVAL(inbuf, smb_vwv11);
	BOOL pipe_start_message_raw = ((SVAL(inbuf, smb_vwv7) & (PIPE_START_MESSAGE|PIPE_RAW_MODE)) ==
								(PIPE_START_MESSAGE|PIPE_RAW_MODE));
	char *data;

	if (!p) {
		return(ERROR_DOS(ERRDOS,ERRbadfid));
	}

	if (p->vuid != vuid) {
		return ERROR_NT(NT_STATUS_INVALID_HANDLE);
	}

	data = smb_base(inbuf) + smb_doff;

	if (numtowrite == 0) {
		nwritten = 0;
	} else {
		if(pipe_start_message_raw) {
			/*
			 * For the start of a message in named pipe byte mode,
			 * the first two bytes are a length-of-pdu field. Ignore
			 * them (we don't trust the client). JRA.
			 */
	 	       if(numtowrite < 2) {
				DEBUG(0,("reply_pipe_write_and_X: start of message set and not enough data sent.(%u)\n",
					(unsigned int)numtowrite ));
				return (UNIXERROR(ERRDOS,ERRnoaccess));
			}

			data += 2;
			numtowrite -= 2;
		}                        
		nwritten = write_to_pipe(p, data, numtowrite);
	}

	if ((nwritten == 0 && numtowrite != 0) || (nwritten < 0)) {
		return (UNIXERROR(ERRDOS,ERRnoaccess));
	}
  
	set_message(outbuf,6,0,True);

	nwritten = (pipe_start_message_raw ? nwritten + 2 : nwritten);
	SSVAL(outbuf,smb_vwv2,nwritten);
  
	DEBUG(3,("writeX-IPC pnum=%04x nwritten=%d\n", p->pnum, nwritten));

	return chain_reply(inbuf,outbuf,length,bufsize);
}

/****************************************************************************
 Reply to a read and X.
 This code is basically stolen from reply_read_and_X with some
 wrinkles to handle pipes.
****************************************************************************/

int reply_pipe_read_and_X(char *inbuf,char *outbuf,int length,int bufsize)
{
	smb_np_struct *p = get_rpc_pipe_p(inbuf,smb_vwv2);
	int smb_maxcnt = SVAL(inbuf,smb_vwv5);
	int smb_mincnt = SVAL(inbuf,smb_vwv6);
	int nread = -1;
	char *data;
	BOOL unused;

	/* we don't use the offset given to use for pipe reads. This
           is deliberate, instead we always return the next lump of
           data on the pipe */
#if 0
	uint32 smb_offs = IVAL(inbuf,smb_vwv3);
#endif

	if (!p) {
		return(ERROR_DOS(ERRDOS,ERRbadfid));
	}

	set_message(outbuf,12,0,True);
	data = smb_buf(outbuf);

	nread = read_from_pipe(p, data, smb_maxcnt, &unused);

	if (nread < 0) {
		return(UNIXERROR(ERRDOS,ERRnoaccess));
	}
  
	SSVAL(outbuf,smb_vwv5,nread);
	SSVAL(outbuf,smb_vwv6,smb_offset(data,outbuf));
	SSVAL(smb_buf(outbuf),-2,nread);
  
	DEBUG(3,("readX-IPC pnum=%04x min=%d max=%d nread=%d\n",
		 p->pnum, smb_mincnt, smb_maxcnt, nread));

	/* Ensure we set up the message length to include the data length read. */
	set_message_bcc(outbuf,nread);
	return chain_reply(inbuf,outbuf,length,bufsize);
}

/****************************************************************************
 Reply to a close.
****************************************************************************/

int reply_pipe_close(connection_struct *conn, char *inbuf,char *outbuf)
{
	smb_np_struct *p = get_rpc_pipe_p(inbuf,smb_vwv0);
	int outsize = set_message(outbuf,0,0,True);

	if (!p) {
		return(ERROR_DOS(ERRDOS,ERRbadfid));
	}

	DEBUG(5,("reply_pipe_close: pnum:%x\n", p->pnum));

	if (!close_rpc_pipe_hnd(p)) {
		return ERROR_DOS(ERRDOS,ERRbadfid);
	}

	return(outsize);
}
