/* 
   Unix SMB/CIFS implementation.
   client print routines
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

/*****************************************************************************
 Convert a character pointer in a cli_call_api() response to a form we can use.
 This function contains code to prevent core dumps if the server returns 
 invalid data.
*****************************************************************************/
static const char *fix_char_ptr(unsigned int datap, unsigned int converter, 
			  char *rdata, int rdrcnt)
{
	if (datap == 0)	{	/* turn NULL pointers into zero length strings */
		return "";
	} else {
		unsigned int offset = datap - converter;

		if (offset >= rdrcnt) {
			DEBUG(1,("bad char ptr: datap=%u, converter=%u rdrcnt=%d>",
				 datap, converter, rdrcnt));
			return "<ERROR>";
		} else {
			return &rdata[offset];
		}
	}
}


/****************************************************************************
call fn() on each entry in a print queue
****************************************************************************/
int cli_print_queue(struct cli_state *cli, 
		    void (*fn)(struct print_job_info *))
{
	char *rparam = NULL;
	char *rdata = NULL;
	char *p;
	unsigned int rdrcnt, rprcnt;
	pstring param;
	int result_code=0;
	int i = -1;
	
	memset(param,'\0',sizeof(param));

	p = param;
	SSVAL(p,0,76);         /* API function number 76 (DosPrintJobEnum) */
	p += 2;
	pstrcpy_base(p,"zWrLeh", param);   /* parameter description? */
	p = skip_string(p,1);
	pstrcpy_base(p,"WWzWWDDzz", param);  /* returned data format */
	p = skip_string(p,1);
	pstrcpy_base(p,cli->share, param);    /* name of queue */
	p = skip_string(p,1);
	SSVAL(p,0,2);   /* API function level 2, PRJINFO_2 data structure */
	SSVAL(p,2,1000); /* size of bytes of returned data buffer */
	p += 4;
	pstrcpy_base(p,"", param);   /* subformat */
	p = skip_string(p,1);

	DEBUG(4,("doing cli_print_queue for %s\n", cli->share));

	if (cli_api(cli, 
		    param, PTR_DIFF(p,param), 1024,  /* Param, length, maxlen */
		    NULL, 0, CLI_BUFFER_SIZE,            /* data, length, maxlen */
		    &rparam, &rprcnt,                /* return params, length */
		    &rdata, &rdrcnt)) {               /* return data, length */
		int converter;
		result_code = SVAL(rparam,0);
		converter = SVAL(rparam,2);       /* conversion factor */

		if (result_code == 0) {
			struct print_job_info job;
			
			p = rdata; 

			for (i = 0; i < SVAL(rparam,4); ++i) {
				job.id = SVAL(p,0);
				job.priority = SVAL(p,2);
				fstrcpy(job.user,
					fix_char_ptr(SVAL(p,4), converter, 
						     rdata, rdrcnt));
				job.t = cli_make_unix_date3(cli, p + 12);
				job.size = IVAL(p,16);
				fstrcpy(job.name,fix_char_ptr(SVAL(p,24), 
							      converter, 
							      rdata, rdrcnt));
				fn(&job);				
				p += 28;
			}
		}
	}

	/* If any parameters or data were returned, free the storage. */
	SAFE_FREE(rparam);
	SAFE_FREE(rdata);

	return i;
}

/****************************************************************************
  cancel a print job
  ****************************************************************************/
int cli_printjob_del(struct cli_state *cli, int job)
{
	char *rparam = NULL;
	char *rdata = NULL;
	char *p;
	unsigned int rdrcnt,rprcnt;
	int ret = -1;
	pstring param;

	memset(param,'\0',sizeof(param));

	p = param;
	SSVAL(p,0,81);		/* DosPrintJobDel() */
	p += 2;
	pstrcpy_base(p,"W", param);
	p = skip_string(p,1);
	pstrcpy_base(p,"", param);
	p = skip_string(p,1);
	SSVAL(p,0,job);     
	p += 2;
	
	if (cli_api(cli, 
		    param, PTR_DIFF(p,param), 1024,  /* Param, length, maxlen */
		    NULL, 0, CLI_BUFFER_SIZE,            /* data, length, maxlen */
		    &rparam, &rprcnt,                /* return params, length */
		    &rdata, &rdrcnt)) {               /* return data, length */
		ret = SVAL(rparam,0);
	}

	SAFE_FREE(rparam);
	SAFE_FREE(rdata);

	return ret;
}


/****************************************************************************
 Open a spool file
****************************************************************************/

int cli_spl_open(struct cli_state *cli, const char *fname, int flags, int share_mode)
{
	char *p;
	unsigned openfn=0;
	unsigned accessmode=0;

	if (flags & O_CREAT)
		openfn |= (1<<4);
	if (!(flags & O_EXCL)) {
		if (flags & O_TRUNC)
			openfn |= (1<<1);
		else
			openfn |= (1<<0);
	}

	accessmode = (share_mode<<4);

	if ((flags & O_ACCMODE) == O_RDWR) {
		accessmode |= 2;
	} else if ((flags & O_ACCMODE) == O_WRONLY) {
		accessmode |= 1;
	} 

#if defined(O_SYNC)
	if ((flags & O_SYNC) == O_SYNC) {
		accessmode |= (1<<14);
	}
#endif /* O_SYNC */

	if (share_mode == DENY_FCB) {
		accessmode = 0xFF;
	}

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf,15,0,True);

	SCVAL(cli->outbuf,smb_com,SMBsplopen);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SSVAL(cli->outbuf,smb_vwv0,0xFF);
	SSVAL(cli->outbuf,smb_vwv2,0);  /* no additional info */
	SSVAL(cli->outbuf,smb_vwv3,accessmode);
	SSVAL(cli->outbuf,smb_vwv4,aSYSTEM | aHIDDEN);
	SSVAL(cli->outbuf,smb_vwv5,0);
	SSVAL(cli->outbuf,smb_vwv8,openfn);

	if (cli->use_oplocks) {
		/* if using oplocks then ask for a batch oplock via
                   core and extended methods */
		SCVAL(cli->outbuf,smb_flg, CVAL(cli->outbuf,smb_flg)|
			FLAG_REQUEST_OPLOCK|FLAG_REQUEST_BATCH_OPLOCK);
		SSVAL(cli->outbuf,smb_vwv2,SVAL(cli->outbuf,smb_vwv2) | 6);
	}
  
	p = smb_buf(cli->outbuf);
	p += clistr_push(cli, p, fname, -1, STR_TERMINATE);

	cli_setup_bcc(cli, p);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli)) {
		return -1;
	}

	if (cli_is_error(cli)) {
		return -1;
	}

	return SVAL(cli->inbuf,smb_vwv2);
}

/****************************************************************************
 Close a file.
****************************************************************************/

BOOL cli_spl_close(struct cli_state *cli, int fnum)
{
	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf,3,0,True);

	SCVAL(cli->outbuf,smb_com,SMBsplclose);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SSVAL(cli->outbuf,smb_vwv0,fnum);
	SIVALS(cli->outbuf,smb_vwv1,-1);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli)) {
		return False;
	}

	return !cli_is_error(cli);
}


