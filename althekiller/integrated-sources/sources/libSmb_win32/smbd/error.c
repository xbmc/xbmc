/* 
   Unix SMB/CIFS implementation.
   error packet handling
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

#include "includes.h"

/* From lib/error.c */
extern struct unix_error_map unix_dos_nt_errmap[];

extern uint32 global_client_caps;
/* these can be set by some functions to override the error codes */
static int override_ERR_class;
static uint32 override_ERR_code;
static NTSTATUS override_ERR_ntstatus;

/****************************************************************************
 Setting eclass and ecode only and status to NT_STATUS_INVALID forces DOS errors.
 Setting status only and eclass and ecode to -1 forces NT errors.
****************************************************************************/
 
void set_saved_error_triple(int eclass, int ecode, NTSTATUS status)
{
	override_ERR_class = eclass;
	override_ERR_code = ecode;
	override_ERR_ntstatus = status;
}

void set_saved_ntstatus(NTSTATUS status)
{
	uint8 tmp_eclass;	/* Hmmm. override_ERR_class is not uint8... */
	override_ERR_ntstatus = status;
	ntstatus_to_dos(status, &tmp_eclass, &override_ERR_code);
	override_ERR_class = tmp_eclass;
	
}

/****************************************************************************
 Return the current settings of the error triple. Return True if any are set.
****************************************************************************/

NTSTATUS get_saved_ntstatus(void)
{
	return override_ERR_ntstatus;
}

/****************************************************************************
 Create an error packet from a cached error.
****************************************************************************/
 
int cached_error_packet(char *outbuf,files_struct *fsp,int line,const char *file)
{
	write_bmpx_struct *wbmpx = fsp->wbmpx_ptr;
	int32 eclass = wbmpx->wr_errclass;
	int32 err = wbmpx->wr_error;
	NTSTATUS ntstatus = wbmpx->wr_status;
 
	/* We can now delete the auxiliary struct */
	SAFE_FREE(fsp->wbmpx_ptr);
	return error_packet(outbuf,eclass,err,ntstatus,line,file);
}

/****************************************************************************
 Create an error packet from errno.
****************************************************************************/

int unix_error_packet(char *outbuf,int def_class,uint32 def_code, NTSTATUS def_status, int line, const char *file)
{
	int eclass=def_class;
	int ecode=def_code;
	NTSTATUS ntstatus = def_status;
	int i=0;

	if (errno != 0) {
		DEBUG(3,("unix_error_packet: error string = %s\n",strerror(errno)));
  
		while (unix_dos_nt_errmap[i].dos_class != 0) {
			if (unix_dos_nt_errmap[i].unix_error == errno) {
				eclass = unix_dos_nt_errmap[i].dos_class;
				ecode = unix_dos_nt_errmap[i].dos_code;
				ntstatus = unix_dos_nt_errmap[i].nt_error;
				break;
			}
			i++;
		}
	}

	return error_packet(outbuf,eclass,ecode,ntstatus,line,file);
}


/****************************************************************************
 Create an error packet. Normally called using the ERROR() macro.
 Setting eclass and ecode only and status to NT_STATUS_OK forces DOS errors.
 Setting status only and eclass and ecode to zero forces NT errors.
 If the override errors are set they take precedence over any passed in values.
****************************************************************************/

int error_packet(char *outbuf, uint8 eclass, uint32 ecode, NTSTATUS ntstatus, int line, const char *file)
{
	int outsize = set_message(outbuf,0,0,True);
	BOOL force_nt_status = False;
	BOOL force_dos_status = False;

	if (override_ERR_class != SMB_SUCCESS || !NT_STATUS_IS_OK(override_ERR_ntstatus)) {
		eclass = override_ERR_class;
		ecode = override_ERR_code;
		ntstatus = override_ERR_ntstatus;
		override_ERR_class = SMB_SUCCESS;
		override_ERR_code = 0;
		override_ERR_ntstatus = NT_STATUS_OK;
	}

	if (eclass == (uint8)-1) {
		force_nt_status = True;
	} else if (NT_STATUS_IS_INVALID(ntstatus)) {
		force_dos_status = True;
	}

	if (force_nt_status || (!force_dos_status && lp_nt_status_support() && (global_client_caps & CAP_STATUS32))) {
		/* We're returning an NT error. */
		if (NT_STATUS_V(ntstatus) == 0 && eclass) {
			ntstatus = dos_to_ntstatus(eclass, ecode);
		}
		SIVAL(outbuf,smb_rcls,NT_STATUS_V(ntstatus));
		SSVAL(outbuf,smb_flg2, SVAL(outbuf,smb_flg2)|FLAGS2_32_BIT_ERROR_CODES);
		DEBUG(3,("error packet at %s(%d) cmd=%d (%s) %s\n",
			 file, line,
			 (int)CVAL(outbuf,smb_com),
			 smb_fn_name(CVAL(outbuf,smb_com)),
			 nt_errstr(ntstatus)));
	} else {
		/* We're returning a DOS error only. */
		if (eclass == 0 && NT_STATUS_V(ntstatus)) {
			ntstatus_to_dos(ntstatus, &eclass, &ecode);
		}

		SSVAL(outbuf,smb_flg2, SVAL(outbuf,smb_flg2)&~FLAGS2_32_BIT_ERROR_CODES);
		SSVAL(outbuf,smb_rcls,eclass);
		SSVAL(outbuf,smb_err,ecode);  

		DEBUG(3,("error packet at %s(%d) cmd=%d (%s) eclass=%d ecode=%d\n",
			  file, line,
			  (int)CVAL(outbuf,smb_com),
			  smb_fn_name(CVAL(outbuf,smb_com)),
			  eclass,
			  ecode));
	}

	return outsize;
}
