/* 
   Unix SMB/CIFS implementation.
   client RAP calls
   Copyright (C) Andrew Tridgell         1994-1998
   Copyright (C) Gerald (Jerry) Carter   2004
   
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
Call a remote api on an arbitrary pipe.  takes param, data and setup buffers.
****************************************************************************/
BOOL cli_api_pipe(struct cli_state *cli, const char *pipe_name, 
                  uint16 *setup, uint32 setup_count, uint32 max_setup_count,
                  char *params, uint32 param_count, uint32 max_param_count,
                  char *data, uint32 data_count, uint32 max_data_count,
                  char **rparam, uint32 *rparam_count,
                  char **rdata, uint32 *rdata_count)
{
  cli_send_trans(cli, SMBtrans, 
                 pipe_name, 
                 0,0,                         /* fid, flags */
                 setup, setup_count, max_setup_count,
                 params, param_count, max_param_count,
                 data, data_count, max_data_count);

  return (cli_receive_trans(cli, SMBtrans, 
                            rparam, (unsigned int *)rparam_count,
                            rdata, (unsigned int *)rdata_count));
}

/****************************************************************************
call a remote api
****************************************************************************/
BOOL cli_api(struct cli_state *cli,
	     char *param, int prcnt, int mprcnt,
	     char *data, int drcnt, int mdrcnt,
	     char **rparam, unsigned int *rprcnt,
	     char **rdata, unsigned int *rdrcnt)
{
  cli_send_trans(cli,SMBtrans,
                 PIPE_LANMAN,             /* Name */
                 0,0,                     /* fid, flags */
                 NULL,0,0,                /* Setup, length, max */
                 param, prcnt, mprcnt,    /* Params, length, max */
                 data, drcnt, mdrcnt      /* Data, length, max */ 
                );

  return (cli_receive_trans(cli,SMBtrans,
                            rparam, rprcnt,
                            rdata, rdrcnt));
}


/****************************************************************************
perform a NetWkstaUserLogon
****************************************************************************/
BOOL cli_NetWkstaUserLogon(struct cli_state *cli,char *user, char *workstation)
{
	char *rparam = NULL;
	char *rdata = NULL;
	char *p;
	unsigned int rdrcnt,rprcnt;
	pstring param;

	memset(param, 0, sizeof(param));
	
	/* send a SMBtrans command with api NetWkstaUserLogon */
	p = param;
	SSVAL(p,0,132); /* api number */
	p += 2;
	pstrcpy_base(p,"OOWb54WrLh",param);
	p = skip_string(p,1);
	pstrcpy_base(p,"WB21BWDWWDDDDDDDzzzD",param);
	p = skip_string(p,1);
	SSVAL(p,0,1);
	p += 2;
	pstrcpy_base(p,user,param);
	strupper_m(p);
	p += 21;
	p++;
	p += 15;
	p++; 
	pstrcpy_base(p, workstation, param);
	strupper_m(p);
	p += 16;
	SSVAL(p, 0, CLI_BUFFER_SIZE);
	p += 2;
	SSVAL(p, 0, CLI_BUFFER_SIZE);
	p += 2;
	
	if (cli_api(cli, 
                    param, PTR_DIFF(p,param),1024,  /* param, length, max */
                    NULL, 0, CLI_BUFFER_SIZE,           /* data, length, max */
                    &rparam, &rprcnt,               /* return params, return size */
                    &rdata, &rdrcnt                 /* return data, return size */
                   )) {
		cli->rap_error = rparam? SVAL(rparam,0) : -1;
		p = rdata;
		
		if (cli->rap_error == 0) {
			DEBUG(4,("NetWkstaUserLogon success\n"));
			cli->privileges = SVAL(p, 24);
			/* The cli->eff_name field used to be set here
	                   but it wasn't used anywhere else. */
		} else {
			DEBUG(1,("NetwkstaUserLogon gave error %d\n", cli->rap_error));
		}
	}
	
	SAFE_FREE(rparam);
	SAFE_FREE(rdata);
	return (cli->rap_error == 0);
}

/****************************************************************************
call a NetShareEnum - try and browse available connections on a host
****************************************************************************/
int cli_RNetShareEnum(struct cli_state *cli, void (*fn)(const char *, uint32, const char *, void *), void *state)
{
	char *rparam = NULL;
	char *rdata = NULL;
	char *p;
	unsigned int rdrcnt,rprcnt;
	pstring param;
	int count = -1;

	/* now send a SMBtrans command with api RNetShareEnum */
	p = param;
	SSVAL(p,0,0); /* api number */
	p += 2;
	pstrcpy_base(p,"WrLeh",param);
	p = skip_string(p,1);
	pstrcpy_base(p,"B13BWz",param);
	p = skip_string(p,1);
	SSVAL(p,0,1);
	/*
	 * Win2k needs a *smaller* buffer than 0xFFFF here -
	 * it returns "out of server memory" with 0xFFFF !!! JRA.
	 */
	SSVAL(p,2,0xFFE0);
	p += 4;
	
	if (cli_api(cli, 
		    param, PTR_DIFF(p,param), 1024,  /* Param, length, maxlen */
		    NULL, 0, 0xFFE0,            /* data, length, maxlen - Win2k needs a small buffer here too ! */
		    &rparam, &rprcnt,                /* return params, length */
		    &rdata, &rdrcnt))                /* return data, length */
		{
			int res = rparam? SVAL(rparam,0) : -1;
			
			if (res == 0 || res == ERRmoredata) {
				int converter=SVAL(rparam,2);
				int i;
				
				count=SVAL(rparam,4);
				p = rdata;
				
				for (i=0;i<count;i++,p+=20) {
					char *sname = p;
					int type = SVAL(p,14);
					int comment_offset = IVAL(p,16) & 0xFFFF;
					const char *cmnt = comment_offset?(rdata+comment_offset-converter):"";
					pstring s1, s2;

					pull_ascii_pstring(s1, sname);
					pull_ascii_pstring(s2, cmnt);

					fn(s1, type, s2, state);
				}
			} else {
				DEBUG(4,("NetShareEnum res=%d\n", res));
			}      
		} else {
			DEBUG(4,("NetShareEnum failed\n"));
		}
  
	SAFE_FREE(rparam);
	SAFE_FREE(rdata);
	
	return count;
}


/****************************************************************************
call a NetServerEnum for the specified workgroup and servertype mask.  This
function then calls the specified callback function for each name returned.

The callback function takes 4 arguments: the machine name, the server type,
the comment and a state pointer.
****************************************************************************/
BOOL cli_NetServerEnum(struct cli_state *cli, char *workgroup, uint32 stype,
		       void (*fn)(const char *, uint32, const char *, void *),
		       void *state)
{
	char *rparam = NULL;
	char *rdata = NULL;
	unsigned int rdrcnt,rprcnt;
	char *p;
	pstring param;
	int uLevel = 1;
	int count = -1;

	errno = 0; /* reset */

	/* send a SMBtrans command with api NetServerEnum */
	p = param;
	SSVAL(p,0,0x68); /* api number */
	p += 2;
	pstrcpy_base(p,"WrLehDz", param);
	p = skip_string(p,1);
  
	pstrcpy_base(p,"B16BBDz", param);

	p = skip_string(p,1);
	SSVAL(p,0,uLevel);
	SSVAL(p,2,CLI_BUFFER_SIZE);
	p += 4;
	SIVAL(p,0,stype);
	p += 4;

	p += push_ascii(p, workgroup, sizeof(pstring)-PTR_DIFF(p,param)-1, STR_TERMINATE|STR_UPPER);
	
	if (cli_api(cli, 
                    param, PTR_DIFF(p,param), 8,        /* params, length, max */
                    NULL, 0, CLI_BUFFER_SIZE,               /* data, length, max */
                    &rparam, &rprcnt,                   /* return params, return size */
                    &rdata, &rdrcnt                     /* return data, return size */
                   )) {
		int res = rparam? SVAL(rparam,0) : -1;
			
		if (res == 0 || res == ERRmoredata) {
			int i;
			int converter=SVAL(rparam,2);

			count=SVAL(rparam,4);
			p = rdata;
					
			for (i = 0;i < count;i++, p += 26) {
				char *sname = p;
				int comment_offset = (IVAL(p,22) & 0xFFFF)-converter;
				const char *cmnt = comment_offset?(rdata+comment_offset):"";
				pstring s1, s2;

				if (comment_offset < 0 || comment_offset > (int)rdrcnt) continue;

				stype = IVAL(p,18) & ~SV_TYPE_LOCAL_LIST_ONLY;

				pull_ascii_pstring(s1, sname);
				pull_ascii_pstring(s2, cmnt);
				fn(s1, stype, s2, state);
			}
		}
	}
  
	SAFE_FREE(rparam);
	SAFE_FREE(rdata);

	if (count < 0) {
	    errno = cli_errno(cli);
	} else {
	    if (!count) {
		/* this is a very special case, when the domain master for the 
		   work group isn't part of the work group itself, there is something
		   wild going on */
		errno = ENOENT;
	    }
	}
			
	return(count > 0);
}



/****************************************************************************
Send a SamOEMChangePassword command
****************************************************************************/
BOOL cli_oem_change_password(struct cli_state *cli, const char *user, const char *new_password,
                             const char *old_password)
{
  pstring param;
  unsigned char data[532];
  char *p = param;
  unsigned char old_pw_hash[16];
  unsigned char new_pw_hash[16];
  unsigned int data_len;
  unsigned int param_len = 0;
  char *rparam = NULL;
  char *rdata = NULL;
  unsigned int rprcnt, rdrcnt;

  if (strlen(user) >= sizeof(fstring)-1) {
    DEBUG(0,("cli_oem_change_password: user name %s is too long.\n", user));
    return False;
  }

  SSVAL(p,0,214); /* SamOEMChangePassword command. */
  p += 2;
  pstrcpy_base(p, "zsT", param);
  p = skip_string(p,1);
  pstrcpy_base(p, "B516B16", param);
  p = skip_string(p,1);
  pstrcpy_base(p,user, param);
  p = skip_string(p,1);
  SSVAL(p,0,532);
  p += 2;

  param_len = PTR_DIFF(p,param);

  /*
   * Get the Lanman hash of the old password, we
   * use this as the key to make_oem_passwd_hash().
   */
  E_deshash(old_password, old_pw_hash);

  encode_pw_buffer(data, new_password, STR_ASCII);
  
#ifdef DEBUG_PASSWORD
  DEBUG(100,("make_oem_passwd_hash\n"));
  dump_data(100, (char *)data, 516);
#endif
  SamOEMhash( (unsigned char *)data, (unsigned char *)old_pw_hash, 516);

  /* 
   * Now place the old password hash in the data.
   */
  E_deshash(new_password, new_pw_hash);

  E_old_pw_hash( new_pw_hash, old_pw_hash, (uchar *)&data[516]);

  data_len = 532;
    
  if (cli_send_trans(cli,SMBtrans,
                    PIPE_LANMAN,                          /* name */
                    0,0,                                  /* fid, flags */
                    NULL,0,0,                             /* setup, length, max */
                    param,param_len,2,                    /* param, length, max */
                    (char *)data,data_len,0                       /* data, length, max */
                   ) == False) {
    DEBUG(0,("cli_oem_change_password: Failed to send password change for user %s\n",
              user ));
    return False;
  }

  if (!cli_receive_trans(cli,SMBtrans,
                       &rparam, &rprcnt,
                       &rdata, &rdrcnt)) {
	  DEBUG(0,("cli_oem_change_password: Failed to recieve reply to password change for user %s\n",
		   user ));
	  return False;
  }
  
  if (rparam)
	  cli->rap_error = SVAL(rparam,0);
  
  SAFE_FREE(rparam);
  SAFE_FREE(rdata);

  return (cli->rap_error == 0);
}


/****************************************************************************
send a qpathinfo call
****************************************************************************/
BOOL cli_qpathinfo(struct cli_state *cli, const char *fname, 
		   time_t *c_time, time_t *a_time, time_t *m_time, 
		   SMB_OFF_T *size, uint16 *mode)
{
	unsigned int data_len = 0;
	unsigned int param_len = 0;
	unsigned int rparam_len, rdata_len;
	uint16 setup = TRANSACT2_QPATHINFO;
	pstring param;
	char *rparam=NULL, *rdata=NULL;
	int count=8;
	BOOL ret;
	time_t (*date_fn)(struct cli_state *, void *);
	char *p;

	p = param;
	memset(p, 0, 6);
	SSVAL(p, 0, SMB_INFO_STANDARD);
	p += 6;
	p += clistr_push(cli, p, fname, sizeof(pstring)-6, STR_TERMINATE);

	param_len = PTR_DIFF(p, param);

	do {
		ret = (cli_send_trans(cli, SMBtrans2, 
				      NULL,           /* Name */
				      -1, 0,          /* fid, flags */
				      &setup, 1, 0,   /* setup, length, max */
				      param, param_len, 10, /* param, length, max */
				      NULL, data_len, cli->max_xmit /* data, length, max */
				      ) &&
		       cli_receive_trans(cli, SMBtrans2, 
					 &rparam, &rparam_len,
					 &rdata, &rdata_len));
		if (!cli_is_dos_error(cli)) break;
		if (!ret) {
			/* we need to work around a Win95 bug - sometimes
			   it gives ERRSRV/ERRerror temprarily */
			uint8 eclass;
			uint32 ecode;
			cli_dos_error(cli, &eclass, &ecode);
			if (eclass != ERRSRV || ecode != ERRerror) break;
			smb_msleep(100);
		}
	} while (count-- && ret==False);

	if (!ret || !rdata || rdata_len < 22) {
		return False;
	}

	if (cli->win95) {
		date_fn = cli_make_unix_date;
	} else {
		date_fn = cli_make_unix_date2;
	}

	if (c_time) {
		*c_time = date_fn(cli, rdata+0);
	}
	if (a_time) {
		*a_time = date_fn(cli, rdata+4);
	}
	if (m_time) {
		*m_time = date_fn(cli, rdata+8);
	}
	if (size) {
		*size = IVAL(rdata, 12);
	}
	if (mode) {
		*mode = SVAL(rdata,l1_attrFile);
	}

	SAFE_FREE(rdata);
	SAFE_FREE(rparam);
	return True;
}


/****************************************************************************
send a setpathinfo call
****************************************************************************/
BOOL cli_setpathinfo(struct cli_state *cli, const char *fname, 
                     time_t c_time, time_t a_time, time_t m_time, uint16 mode)
{
	unsigned int data_len = 0;
	unsigned int param_len = 0;
	unsigned int rparam_len, rdata_len;
	uint16 setup = TRANSACT2_SETPATHINFO;
	pstring param;
        pstring data;
	char *rparam=NULL, *rdata=NULL;
	int count=8;
	BOOL ret;
	char *p;

	memset(param, 0, sizeof(param));
	memset(data, 0, sizeof(data));

        p = param;

        /* Add the information level */
	SSVAL(p, 0, SMB_FILE_BASIC_INFORMATION);

        /* Skip reserved */
	p += 6;

        /* Add the file name */
	p += clistr_push(cli, p, fname, sizeof(pstring)-6, STR_TERMINATE);

	param_len = PTR_DIFF(p, param);

        p = data;

        /*
         * Add the create, last access, modification, and status change times
         */
        
        /* Don't set create time, at offset 0 */
        p += 8;

        put_long_date(p, a_time);
        p += 8;
        
        put_long_date(p, m_time);
        p += 8;
        
        put_long_date(p, c_time);
        p += 8;

        /* Add attributes */
        SIVAL(p, 0, mode);
        p += 4;

        /* Add padding */
        SIVAL(p, 0, 0);
        p += 4;

        data_len = PTR_DIFF(p, data);

	do {
		ret = (cli_send_trans(cli, SMBtrans2, 
				      NULL,           /* Name */
				      -1, 0,          /* fid, flags */
				      &setup, 1, 0,   /* setup, length, max */
				      param, param_len, 10, /* param, length, max */
				      data, data_len, cli->max_xmit /* data, length, max */
				      ) &&
		       cli_receive_trans(cli, SMBtrans2, 
					 &rparam, &rparam_len,
					 &rdata, &rdata_len));
		if (!cli_is_dos_error(cli)) break;
		if (!ret) {
			/* we need to work around a Win95 bug - sometimes
			   it gives ERRSRV/ERRerror temprarily */
			uint8 eclass;
			uint32 ecode;
			cli_dos_error(cli, &eclass, &ecode);
			if (eclass != ERRSRV || ecode != ERRerror) break;
			smb_msleep(100);
		}
	} while (count-- && ret==False);

	if (!ret) {
		return False;
	}

	SAFE_FREE(rdata);
	SAFE_FREE(rparam);
	return True;
}


/****************************************************************************
send a qpathinfo call with the SMB_QUERY_FILE_ALL_INFO info level
****************************************************************************/
BOOL cli_qpathinfo2(struct cli_state *cli, const char *fname, 
		    time_t *create_time, time_t *access_time, time_t *write_time, 
		    time_t *change_time, SMB_OFF_T *size, uint16 *mode,
		    SMB_INO_T *ino)
{
	unsigned int data_len = 0;
	unsigned int param_len = 0;
	uint16 setup = TRANSACT2_QPATHINFO;
	pstring param;
	char *rparam=NULL, *rdata=NULL;
	char *p;

	p = param;
	memset(p, 0, 6);
	SSVAL(p, 0, SMB_QUERY_FILE_ALL_INFO);
	p += 6;
	p += clistr_push(cli, p, fname, sizeof(pstring)-6, STR_TERMINATE);

	param_len = PTR_DIFF(p, param);

	if (!cli_send_trans(cli, SMBtrans2, 
                            NULL,                         /* name */
                            -1, 0,                        /* fid, flags */
                            &setup, 1, 0,                 /* setup, length, max */
                            param, param_len, 10,         /* param, length, max */
                            NULL, data_len, cli->max_xmit /* data, length, max */
                           )) {
		return False;
	}

	if (!cli_receive_trans(cli, SMBtrans2,
                               &rparam, &param_len,
                               &rdata, &data_len)) {
		return False;
	}

	if (!rdata || data_len < 22) {
		return False;
	}
        
	if (create_time) {
                *create_time = interpret_long_date(rdata+0);
	}
	if (access_time) {
		*access_time = interpret_long_date(rdata+8);
	}
	if (write_time) {
		*write_time = interpret_long_date(rdata+16);
	}
	if (change_time) {
		*change_time = interpret_long_date(rdata+24);
	}
	if (mode) {
		*mode = SVAL(rdata, 32);
	}
	if (size) {
                *size = IVAL2_TO_SMB_BIG_UINT(rdata,48);
	}
	if (ino) {
		*ino = IVAL(rdata, 64);
	}

	SAFE_FREE(rdata);
	SAFE_FREE(rparam);
	return True;
}


/****************************************************************************
send a qfileinfo QUERY_FILE_NAME_INFO call
****************************************************************************/
BOOL cli_qfilename(struct cli_state *cli, int fnum, 
		   pstring name)
{
	unsigned int data_len = 0;
	unsigned int param_len = 0;
	uint16 setup = TRANSACT2_QFILEINFO;
	pstring param;
	char *rparam=NULL, *rdata=NULL;

	param_len = 4;
	memset(param, 0, param_len);
	SSVAL(param, 0, fnum);
	SSVAL(param, 2, SMB_QUERY_FILE_NAME_INFO);

	if (!cli_send_trans(cli, SMBtrans2, 
                            NULL,                         /* name */
                            -1, 0,                        /* fid, flags */
                            &setup, 1, 0,                 /* setup, length, max */
                            param, param_len, 2,          /* param, length, max */
                            NULL, data_len, cli->max_xmit /* data, length, max */
                           )) {
		return False;
	}

	if (!cli_receive_trans(cli, SMBtrans2,
                               &rparam, &param_len,
                               &rdata, &data_len)) {
		return False;
	}

	if (!rdata || data_len < 4) {
		return False;
	}

	clistr_pull(cli, name, rdata+4, sizeof(pstring), IVAL(rdata, 0), STR_UNICODE);

	return True;
}


/****************************************************************************
send a qfileinfo call
****************************************************************************/
BOOL cli_qfileinfo(struct cli_state *cli, int fnum, 
		   uint16 *mode, SMB_OFF_T *size,
		   time_t *create_time, time_t *access_time, time_t *write_time, 
		   time_t *change_time, SMB_INO_T *ino)
{
	unsigned int data_len = 0;
	unsigned int param_len = 0;
	uint16 setup = TRANSACT2_QFILEINFO;
	pstring param;
	char *rparam=NULL, *rdata=NULL;

	/* if its a win95 server then fail this - win95 totally screws it
	   up */
	if (cli->win95) return False;

	param_len = 4;

	memset(param, 0, param_len);
	SSVAL(param, 0, fnum);
	SSVAL(param, 2, SMB_QUERY_FILE_ALL_INFO);

	if (!cli_send_trans(cli, SMBtrans2, 
                            NULL,                         /* name */
                            -1, 0,                        /* fid, flags */
                            &setup, 1, 0,                 /* setup, length, max */
                            param, param_len, 2,          /* param, length, max */
                            NULL, data_len, cli->max_xmit /* data, length, max */
                           )) {
		return False;
	}

	if (!cli_receive_trans(cli, SMBtrans2,
                               &rparam, &param_len,
                               &rdata, &data_len)) {
		return False;
	}

	if (!rdata || data_len < 68) {
		return False;
	}

	if (create_time) {
		*create_time = interpret_long_date(rdata+0);
	}
	if (access_time) {
		*access_time = interpret_long_date(rdata+8);
	}
	if (write_time) {
		*write_time = interpret_long_date(rdata+16);
	}
	if (change_time) {
		*change_time = interpret_long_date(rdata+24);
	}
	if (mode) {
		*mode = SVAL(rdata, 32);
	}
	if (size) {
                *size = IVAL2_TO_SMB_BIG_UINT(rdata,48);
	}
	if (ino) {
		*ino = IVAL(rdata, 64);
	}

	SAFE_FREE(rdata);
	SAFE_FREE(rparam);
	return True;
}


/****************************************************************************
send a qpathinfo BASIC_INFO call
****************************************************************************/
BOOL cli_qpathinfo_basic( struct cli_state *cli, const char *name, 
                          SMB_STRUCT_STAT *sbuf, uint32 *attributes )
{
	unsigned int param_len = 0;
	unsigned int data_len = 0;
	uint16 setup = TRANSACT2_QPATHINFO;
	char param[sizeof(pstring)+6];
	char *rparam=NULL, *rdata=NULL;
	char *p;
	pstring path;
	int len;
	
	/* send full paths to dfs root shares */
	
	if ( cli->dfsroot )
		pstr_sprintf(path, "\\%s\\%s\\%s", cli->desthost, cli->share, name );
	else
		pstrcpy( path, name );
	
	/* cleanup */
	
	len = strlen( path );
	if ( path[len] == '\\' )
		path[len] = '\0';

	p = param;
	memset(p, 0, 6);
	SSVAL(p, 0, SMB_QUERY_FILE_BASIC_INFO);
	p += 6;
	p += clistr_push(cli, p, path, sizeof(pstring)-6, STR_TERMINATE);
	param_len = PTR_DIFF(p, param);

	if (!cli_send_trans(cli, SMBtrans2,
		NULL,                        /* name */
		-1, 0,                       /* fid, flags */
		&setup, 1, 0,                /* setup, length, max */
		param, param_len, 2,         /* param, length, max */
		NULL,  0, cli->max_xmit      /* data, length, max */
		)) {
			return False;
	}

	if (!cli_receive_trans(cli, SMBtrans2,
		&rparam, &param_len,
		&rdata, &data_len)) {
			return False;
	}

	if (data_len < 36) {
		SAFE_FREE(rdata);
		SAFE_FREE(rparam);
		return False;
	}

	sbuf->st_atime = interpret_long_date( rdata+8 ); /* Access time. */
	sbuf->st_mtime = interpret_long_date( rdata+16 ); /* Write time. */
	sbuf->st_ctime = interpret_long_date( rdata+24 ); /* Change time. */
	
	*attributes = IVAL( rdata, 32 );
	
	SAFE_FREE(rparam);
	SAFE_FREE(rdata);
	
	return True;
}

/****************************************************************************
send a qfileinfo call
****************************************************************************/

BOOL cli_qfileinfo_test(struct cli_state *cli, int fnum, int level, char **poutdata, uint32 *poutlen)
{
	unsigned int data_len = 0;
	unsigned int param_len = 0;
	uint16 setup = TRANSACT2_QFILEINFO;
	pstring param;
	char *rparam=NULL, *rdata=NULL;

	*poutdata = NULL;
	*poutlen = 0;

	/* if its a win95 server then fail this - win95 totally screws it
	   up */
	if (cli->win95)
		return False;

	param_len = 4;

	memset(param, 0, param_len);
	SSVAL(param, 0, fnum);
	SSVAL(param, 2, level);

	if (!cli_send_trans(cli, SMBtrans2, 
                            NULL,                           /* name */
                            -1, 0,                          /* fid, flags */
                            &setup, 1, 0,                   /* setup, length, max */
                            param, param_len, 2,            /* param, length, max */
                            NULL, data_len, cli->max_xmit   /* data, length, max */
                           )) {
		return False;
	}

	if (!cli_receive_trans(cli, SMBtrans2,
                               &rparam, &param_len,
                               &rdata, &data_len)) {
		return False;
	}

	*poutdata = memdup(rdata, data_len);
	*poutlen = data_len;

	SAFE_FREE(rdata);
	SAFE_FREE(rparam);
	return True;
}



/****************************************************************************
send a qpathinfo SMB_QUERY_FILE_ALT_NAME_INFO call
****************************************************************************/
NTSTATUS cli_qpathinfo_alt_name(struct cli_state *cli, const char *fname, fstring alt_name)
{
	unsigned int data_len = 0;
	unsigned int param_len = 0;
	uint16 setup = TRANSACT2_QPATHINFO;
	pstring param;
	char *rparam=NULL, *rdata=NULL;
	int count=8;
	char *p;
	BOOL ret;
	unsigned int len;

	p = param;
	memset(p, 0, 6);
	SSVAL(p, 0, SMB_QUERY_FILE_ALT_NAME_INFO);
	p += 6;
	p += clistr_push(cli, p, fname, sizeof(pstring)-6, STR_TERMINATE);

	param_len = PTR_DIFF(p, param);

	do {
		ret = (cli_send_trans(cli, SMBtrans2, 
				      NULL,           /* Name */
				      -1, 0,          /* fid, flags */
				      &setup, 1, 0,   /* setup, length, max */
				      param, param_len, 10, /* param, length, max */
				      NULL, data_len, cli->max_xmit /* data, length, max */
				      ) &&
		       cli_receive_trans(cli, SMBtrans2, 
					 &rparam, &param_len,
					 &rdata, &data_len));
		if (!ret && cli_is_dos_error(cli)) {
			/* we need to work around a Win95 bug - sometimes
			   it gives ERRSRV/ERRerror temprarily */
			uint8 eclass;
			uint32 ecode;
			cli_dos_error(cli, &eclass, &ecode);
			if (eclass != ERRSRV || ecode != ERRerror) break;
			smb_msleep(100);
		}
	} while (count-- && ret==False);

	if (!ret || !rdata || data_len < 4) {
		return NT_STATUS_UNSUCCESSFUL;
	}

	len = IVAL(rdata, 0);

	if (len > data_len - 4) {
		return NT_STATUS_INVALID_NETWORK_RESPONSE;
	}

	clistr_pull(cli, alt_name, rdata+4, sizeof(fstring), len, STR_UNICODE);

	SAFE_FREE(rdata);
	SAFE_FREE(rparam);

	return NT_STATUS_OK;
}
