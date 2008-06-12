/* 
   Unix SMB/CIFS implementation.
   client file read/write routines
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
Issue a single SMBread and don't wait for a reply.
****************************************************************************/

static BOOL cli_issue_read(struct cli_state *cli, int fnum, SMB_OFF_T offset, 
			   size_t size, int i)
{
	BOOL bigoffset = False;

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	if ((SMB_BIG_UINT)offset >> 32) 
		bigoffset = True;

	set_message(cli->outbuf,bigoffset ? 12 : 10,0,True);
		
	SCVAL(cli->outbuf,smb_com,SMBreadX);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SCVAL(cli->outbuf,smb_vwv0,0xFF);
	SSVAL(cli->outbuf,smb_vwv2,fnum);
	SIVAL(cli->outbuf,smb_vwv3,offset);
	SSVAL(cli->outbuf,smb_vwv5,size);
	SSVAL(cli->outbuf,smb_vwv6,size);
	SSVAL(cli->outbuf,smb_vwv7,((size >> 16) & 1));
	SSVAL(cli->outbuf,smb_mid,cli->mid + i);

	if (bigoffset) {
		SIVAL(cli->outbuf,smb_vwv10,(((SMB_BIG_UINT)offset)>>32) & 0xffffffff);
	}

	return cli_send_smb(cli);
}

/****************************************************************************
  Read size bytes at offset offset using SMBreadX.
****************************************************************************/

ssize_t cli_read(struct cli_state *cli, int fnum, char *buf, SMB_OFF_T offset, size_t size)
{
	char *p;
	int size2;
	int readsize;
	ssize_t total = 0;

	if (size == 0) 
		return 0;

	/*
	 * Set readsize to the maximum size we can handle in one readX,
	 * rounded down to a multiple of 1024.
	 */

	if (cli->capabilities & CAP_LARGE_READX) {
		if (cli->is_samba) {
			readsize = CLI_SAMBA_MAX_LARGE_READX_SIZE;
		} else {
			readsize = CLI_WINDOWS_MAX_LARGE_READX_SIZE;
		}
	} else {
		readsize = (cli->max_xmit - (smb_size+32)) & ~1023;
	}

	while (total < size) {
		readsize = MIN(readsize, size-total);

		/* Issue a read and receive a reply */

		if (!cli_issue_read(cli, fnum, offset, readsize, 0))
			return -1;

		if (!cli_receive_smb(cli))
			return -1;

		/* Check for error.  Make sure to check for DOS and NT
                   errors. */

                if (cli_is_error(cli)) {
			BOOL recoverable_error = False;
                        NTSTATUS status = NT_STATUS_OK;
                        uint8 eclass = 0;
			uint32 ecode = 0;

                        if (cli_is_nt_error(cli))
                                status = cli_nt_error(cli);
                        else
                                cli_dos_error(cli, &eclass, &ecode);

			/*
			 * ERRDOS ERRmoredata or STATUS_MORE_ENRTIES is a
			 * recoverable error, plus we have valid data in the
			 * packet so don't error out here.
			 */

                        if ((eclass == ERRDOS && ecode == ERRmoredata) ||
                            NT_STATUS_V(status) == NT_STATUS_V(STATUS_MORE_ENTRIES))
				recoverable_error = True;

			if (!recoverable_error)
                                return -1;
		}

		size2 = SVAL(cli->inbuf, smb_vwv5);
		size2 |= (((unsigned int)(SVAL(cli->inbuf, smb_vwv7) & 1)) << 16);

		if (size2 > readsize) {
			DEBUG(5,("server returned more than we wanted!\n"));
			return -1;
		} else if (size2 < 0) {
			DEBUG(5,("read return < 0!\n"));
			return -1;
		}

		/* Copy data into buffer */

		p = smb_base(cli->inbuf) + SVAL(cli->inbuf,smb_vwv6);
		memcpy(buf + total, p, size2);

		total += size2;
		offset += size2;

		/*
		 * If the server returned less than we asked for we're at EOF.
		 */

		if (size2 < readsize)
			break;
	}

	return total;
}

#if 0  /* relies on client_receive_smb(), now a static in libsmb/clientgen.c */

/* This call is INCOMPATIBLE with SMB signing.  If you remove the #if 0
   you must fix ensure you don't attempt to sign the packets - data
   *will* be currupted */

/****************************************************************************
Issue a single SMBreadraw and don't wait for a reply.
****************************************************************************/

static BOOL cli_issue_readraw(struct cli_state *cli, int fnum, SMB_OFF_T offset, 
			   size_t size, int i)
{

	if (!cli->sign_info.use_smb_signing) {
		DEBUG(0, ("Cannot use readraw and SMB Signing\n"));
		return False;
	}
	
	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf,10,0,True);
		
	SCVAL(cli->outbuf,smb_com,SMBreadbraw);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SSVAL(cli->outbuf,smb_vwv0,fnum);
	SIVAL(cli->outbuf,smb_vwv1,offset);
	SSVAL(cli->outbuf,smb_vwv2,size);
	SSVAL(cli->outbuf,smb_vwv3,size);
	SSVAL(cli->outbuf,smb_mid,cli->mid + i);

	return cli_send_smb(cli);
}

/****************************************************************************
 Tester for the readraw call.
****************************************************************************/

ssize_t cli_readraw(struct cli_state *cli, int fnum, char *buf, SMB_OFF_T offset, size_t size)
{
	char *p;
	int size2;
	size_t readsize;
	ssize_t total = 0;

	if (size == 0) 
		return 0;

	/*
	 * Set readsize to the maximum size we can handle in one readraw.
	 */

	readsize = 0xFFFF;

	while (total < size) {
		readsize = MIN(readsize, size-total);

		/* Issue a read and receive a reply */

		if (!cli_issue_readraw(cli, fnum, offset, readsize, 0))
			return -1;

		if (!client_receive_smb(cli->fd, cli->inbuf, cli->timeout))
			return -1;

		size2 = smb_len(cli->inbuf);

		if (size2 > readsize) {
			DEBUG(5,("server returned more than we wanted!\n"));
			return -1;
		} else if (size2 < 0) {
			DEBUG(5,("read return < 0!\n"));
			return -1;
		}

		/* Copy data into buffer */

		if (size2) {
			p = cli->inbuf + 4;
			memcpy(buf + total, p, size2);
		}

		total += size2;
		offset += size2;

		/*
		 * If the server returned less than we asked for we're at EOF.
		 */

		if (size2 < readsize)
			break;
	}

	return total;
}
#endif
/****************************************************************************
issue a single SMBwrite and don't wait for a reply
****************************************************************************/

static BOOL cli_issue_write(struct cli_state *cli, int fnum, SMB_OFF_T offset, 
			    uint16 mode, const char *buf,
			    size_t size, int i)
{
	char *p;
	BOOL large_writex = False;

	if (size > cli->bufsize) {
		cli->outbuf = SMB_REALLOC(cli->outbuf, size + 1024);
		if (!cli->outbuf) {
			return False;
		}
		cli->inbuf = SMB_REALLOC(cli->inbuf, size + 1024);
		if (cli->inbuf == NULL) {
			SAFE_FREE(cli->outbuf);
			return False;
		}
		cli->bufsize = size + 1024;
	}

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	if (((SMB_BIG_UINT)offset >> 32) || (size > 0xFFFF)) {
		large_writex = True;
	}

	if (large_writex)
		set_message(cli->outbuf,14,0,True);
	else
		set_message(cli->outbuf,12,0,True);
	
	SCVAL(cli->outbuf,smb_com,SMBwriteX);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);
	
	SCVAL(cli->outbuf,smb_vwv0,0xFF);
	SSVAL(cli->outbuf,smb_vwv2,fnum);

	SIVAL(cli->outbuf,smb_vwv3,offset);
	SIVAL(cli->outbuf,smb_vwv5,0);
	SSVAL(cli->outbuf,smb_vwv7,mode);

	SSVAL(cli->outbuf,smb_vwv8,(mode & 0x0008) ? size : 0);
	/*
	 * According to CIFS-TR-1p00, this following field should only
	 * be set if CAP_LARGE_WRITEX is set. We should check this
	 * locally. However, this check might already have been
	 * done by our callers.
	 */
	SSVAL(cli->outbuf,smb_vwv9,((size>>16)&1));
	SSVAL(cli->outbuf,smb_vwv10,size);
	SSVAL(cli->outbuf,smb_vwv11,
	      smb_buf(cli->outbuf) - smb_base(cli->outbuf));

	if (large_writex) {
		SIVAL(cli->outbuf,smb_vwv12,(((SMB_BIG_UINT)offset)>>32) & 0xffffffff);
	}
	
	p = smb_base(cli->outbuf) + SVAL(cli->outbuf,smb_vwv11);
	memcpy(p, buf, size);
	cli_setup_bcc(cli, p+size);

	SSVAL(cli->outbuf,smb_mid,cli->mid + i);
	
	show_msg(cli->outbuf);
	return cli_send_smb(cli);
}

/****************************************************************************
  write to a file
  write_mode: 0x0001 disallow write cacheing
              0x0002 return bytes remaining
              0x0004 use raw named pipe protocol
              0x0008 start of message mode named pipe protocol
****************************************************************************/

ssize_t cli_write(struct cli_state *cli,
    	         int fnum, uint16 write_mode,
		 const char *buf, SMB_OFF_T offset, size_t size)
{
	ssize_t bwritten = 0;
	unsigned int issued = 0;
	unsigned int received = 0;
	int mpx = 1;
	int block = cli->max_xmit - (smb_size+32);
	int blocks = (size + (block-1)) / block;

	if(cli->max_mux > 1) {
		mpx = cli->max_mux-1;
	} else {
		mpx = 1;
	}

	while (received < blocks) {

		while ((issued - received < mpx) && (issued < blocks)) {
			ssize_t bsent = issued * block;
			ssize_t size1 = MIN(block, size - bsent);

			if (!cli_issue_write(cli, fnum, offset + bsent,
			                write_mode,
			                buf + bsent,
					size1, issued))
				return -1;
			issued++;
		}

		if (!cli_receive_smb(cli))
			return bwritten;

		received++;

		if (cli_is_error(cli))
			break;

		bwritten += SVAL(cli->inbuf, smb_vwv2);
		bwritten += (((int)(SVAL(cli->inbuf, smb_vwv4)))<<16);
	}

	while (received < issued && cli_receive_smb(cli))
		received++;
	
	return bwritten;
}

/****************************************************************************
  write to a file using a SMBwrite and not bypassing 0 byte writes
****************************************************************************/

ssize_t cli_smbwrite(struct cli_state *cli,
		     int fnum, char *buf, SMB_OFF_T offset, size_t size1)
{
	char *p;
	ssize_t total = 0;

	do {
		size_t size = MIN(size1, cli->max_xmit - 48);
		
		memset(cli->outbuf,'\0',smb_size);
		memset(cli->inbuf,'\0',smb_size);

		set_message(cli->outbuf,5, 0,True);

		SCVAL(cli->outbuf,smb_com,SMBwrite);
		SSVAL(cli->outbuf,smb_tid,cli->cnum);
		cli_setup_packet(cli);
		
		SSVAL(cli->outbuf,smb_vwv0,fnum);
		SSVAL(cli->outbuf,smb_vwv1,size);
		SIVAL(cli->outbuf,smb_vwv2,offset);
		SSVAL(cli->outbuf,smb_vwv4,0);
		
		p = smb_buf(cli->outbuf);
		*p++ = 1;
		SSVAL(p, 0, size); p += 2;
		memcpy(p, buf, size); p += size;

		cli_setup_bcc(cli, p);
		
		if (!cli_send_smb(cli))
			return -1;

		if (!cli_receive_smb(cli))
			return -1;
		
		if (cli_is_error(cli))
			return -1;

		size = SVAL(cli->inbuf,smb_vwv0);
		if (size == 0)
			break;

		size1 -= size;
		total += size;
		offset += size;

	} while (size1);

	return total;
}
