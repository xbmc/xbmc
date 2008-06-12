/* 
   Unix SMB/CIFS implementation.
   client quota functions
   Copyright (C) Stefan (metze) Metzmacher	2003
   
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

BOOL cli_get_quota_handle(struct cli_state *cli, int *quota_fnum)
{
	*quota_fnum = cli_nt_create_full(cli, FAKE_FILE_NAME_QUOTA_WIN32,
		 0x00000016, DESIRED_ACCESS_PIPE,
		 0x00000000, FILE_SHARE_READ|FILE_SHARE_WRITE,
		 FILE_OPEN, 0x00000000, 0x03);
		 
	if (*quota_fnum == (-1)) {
		return False;
	}

	return True;
}

void free_ntquota_list(SMB_NTQUOTA_LIST **qt_list)
{
	if (!qt_list)
		return;
		
	if ((*qt_list)->mem_ctx)
		talloc_destroy((*qt_list)->mem_ctx);

	(*qt_list) = NULL;

	return;	
}

static BOOL parse_user_quota_record(const char *rdata, unsigned int rdata_count, unsigned int *offset, SMB_NTQUOTA_STRUCT *pqt)
{
	int sid_len;
	SMB_NTQUOTA_STRUCT qt;

	ZERO_STRUCT(qt);

	if (!rdata||!offset||!pqt)
		smb_panic("parse_quota_record: called with NULL POINTER!\n");

	if (rdata_count < 40) {
		return False;
	}
		
	/* offset to next quota record.
	 * 4 bytes IVAL(rdata,0)
	 * unused here...
	 */
	*offset = IVAL(rdata,0);

	/* sid len */
	sid_len = IVAL(rdata,4);

	if (rdata_count < 40+sid_len) {
		return False;		
	}

	/* unknown 8 bytes in pdata 
	 * maybe its the change time in NTTIME
	 */

	/* the used space 8 bytes (SMB_BIG_UINT)*/
	qt.usedspace = (SMB_BIG_UINT)IVAL(rdata,16);
#ifdef LARGE_SMB_OFF_T
	qt.usedspace |= (((SMB_BIG_UINT)IVAL(rdata,20)) << 32);
#else /* LARGE_SMB_OFF_T */
	if ((IVAL(rdata,20) != 0)&&
		((qt.usedspace != 0xFFFFFFFF)||
		 (IVAL(rdata,20)!=0xFFFFFFFF))) {
		/* more than 32 bits? */
		return False;
	}
#endif /* LARGE_SMB_OFF_T */

	/* the soft quotas 8 bytes (SMB_BIG_UINT)*/
	qt.softlim = (SMB_BIG_UINT)IVAL(rdata,24);
#ifdef LARGE_SMB_OFF_T
	qt.softlim |= (((SMB_BIG_UINT)IVAL(rdata,28)) << 32);
#else /* LARGE_SMB_OFF_T */
	if ((IVAL(rdata,28) != 0)&&
		((qt.softlim != 0xFFFFFFFF)||
		 (IVAL(rdata,28)!=0xFFFFFFFF))) {
		/* more than 32 bits? */
		return False;
	}
#endif /* LARGE_SMB_OFF_T */

	/* the hard quotas 8 bytes (SMB_BIG_UINT)*/
	qt.hardlim = (SMB_BIG_UINT)IVAL(rdata,32);
#ifdef LARGE_SMB_OFF_T
	qt.hardlim |= (((SMB_BIG_UINT)IVAL(rdata,36)) << 32);
#else /* LARGE_SMB_OFF_T */
	if ((IVAL(rdata,36) != 0)&&
		((qt.hardlim != 0xFFFFFFFF)||
		 (IVAL(rdata,36)!=0xFFFFFFFF))) {
		/* more than 32 bits? */
		return False;
	}
#endif /* LARGE_SMB_OFF_T */
	
	sid_parse(rdata+40,sid_len,&qt.sid);

	qt.qtype = SMB_USER_QUOTA_TYPE;

	*pqt = qt;

	return True;
}

BOOL cli_get_user_quota(struct cli_state *cli, int quota_fnum, SMB_NTQUOTA_STRUCT *pqt)
{
	BOOL ret = False;
	uint16 setup;
	char params[16];
	unsigned int data_len;
	char data[SID_MAX_SIZE+8];
	char *rparam=NULL, *rdata=NULL;
	unsigned int rparam_count=0, rdata_count=0;
	unsigned int sid_len;
	unsigned int offset;

	if (!cli||!pqt)
		smb_panic("cli_get_user_quota() called with NULL Pointer!");

	setup = NT_TRANSACT_GET_USER_QUOTA;

	SSVAL(params, 0,quota_fnum);
	SSVAL(params, 2,TRANSACT_GET_USER_QUOTA_FOR_SID);
	SIVAL(params, 4,0x00000024);
	SIVAL(params, 8,0x00000000);
	SIVAL(params,12,0x00000024);
	
	sid_len = sid_size(&pqt->sid);
	data_len = sid_len+8;
	SIVAL(data, 0, 0x00000000);
	SIVAL(data, 4, sid_len);
	sid_linearize(data+8, sid_len, &pqt->sid);
	
	if (!cli_send_nt_trans(cli, 
			       NT_TRANSACT_GET_USER_QUOTA, 
			       0, 
			       &setup, 1, 0,
			       params, 16, 4,
			       data, data_len, 112)) {
		DEBUG(1,("Failed to send NT_TRANSACT_GET_USER_QUOTA\n"));
		goto cleanup;
	}


	if (!cli_receive_nt_trans(cli,
				  &rparam, &rparam_count,
				  &rdata, &rdata_count)) {
		DEBUG(1,("Failed to recv NT_TRANSACT_GET_USER_QUOTA\n"));
		goto cleanup;
	}

	if (cli_is_error(cli)) {
		ret = False;
		goto cleanup;
	} else {
		ret = True;
	}

	if ((rparam&&rdata)&&(rparam_count>=4&&rdata_count>=8)) {
		ret = parse_user_quota_record(rdata, rdata_count, &offset, pqt);
	} else {
		DEBUG(0,("Got INVALID NT_TRANSACT_GET_USER_QUOTA reply.\n"));
		ret = False; 
	}

 cleanup:
	SAFE_FREE(rparam);
	SAFE_FREE(rdata); 
	return ret;
}

BOOL cli_set_user_quota(struct cli_state *cli, int quota_fnum, SMB_NTQUOTA_STRUCT *pqt)
{
	BOOL ret = False;
	uint16 setup;
	char params[2];
	char data[112];
	char *rparam=NULL, *rdata=NULL;
	unsigned int rparam_count=0, rdata_count=0;
	unsigned int sid_len;	
	memset(data,'\0',112);
	
	if (!cli||!pqt)
		smb_panic("cli_set_user_quota() called with NULL Pointer!");

	setup = NT_TRANSACT_SET_USER_QUOTA;

	SSVAL(params,0,quota_fnum);

	sid_len = sid_size(&pqt->sid);
	SIVAL(data,0,0);
	SIVAL(data,4,sid_len);
	SBIG_UINT(data, 8,(SMB_BIG_UINT)0);
	SBIG_UINT(data,16,pqt->usedspace);
	SBIG_UINT(data,24,pqt->softlim);
	SBIG_UINT(data,32,pqt->hardlim);
	sid_linearize(data+40, sid_len, &pqt->sid);
	
	if (!cli_send_nt_trans(cli, 
			       NT_TRANSACT_SET_USER_QUOTA, 
			       0, 
			       &setup, 1, 0,
			       params, 2, 0,
			       data, 112, 0)) {
		DEBUG(1,("Failed to send NT_TRANSACT_SET_USER_QUOTA\n"));
		goto cleanup;
	}


	if (!cli_receive_nt_trans(cli, 
				  &rparam, &rparam_count,
				  &rdata, &rdata_count)) {
		DEBUG(1,("NT_TRANSACT_SET_USER_QUOTA failed\n"));
		goto cleanup;
	}

	if (cli_is_error(cli)) {
		ret = False;
		goto cleanup;
	} else {
		ret = True;
	}

  cleanup:
  	SAFE_FREE(rparam);
	SAFE_FREE(rdata);
	return ret;
}

BOOL cli_list_user_quota(struct cli_state *cli, int quota_fnum, SMB_NTQUOTA_LIST **pqt_list)
{
	BOOL ret = False;
	uint16 setup;
	char params[16];
	char *rparam=NULL, *rdata=NULL;
	unsigned int rparam_count=0, rdata_count=0;
	unsigned int offset;
	const char *curdata = NULL;
	unsigned int curdata_count = 0;
	TALLOC_CTX *mem_ctx = NULL;
	SMB_NTQUOTA_STRUCT qt;
	SMB_NTQUOTA_LIST *tmp_list_ent;

	if (!cli||!pqt_list)
		smb_panic("cli_list_user_quota() called with NULL Pointer!");

	setup = NT_TRANSACT_GET_USER_QUOTA;

	SSVAL(params, 0,quota_fnum);
	SSVAL(params, 2,TRANSACT_GET_USER_QUOTA_LIST_START);
	SIVAL(params, 4,0x00000000);
	SIVAL(params, 8,0x00000000);
	SIVAL(params,12,0x00000000);
	
	if (!cli_send_nt_trans(cli, 
			       NT_TRANSACT_GET_USER_QUOTA, 
			       0, 
			       &setup, 1, 0,
			       params, 16, 4,
			       NULL, 0, 2048)) {
		DEBUG(1,("Failed to send NT_TRANSACT_GET_USER_QUOTA\n"));
		goto cleanup;
	}


	if (!cli_receive_nt_trans(cli,
				  &rparam, &rparam_count,
				  &rdata, &rdata_count)) {
		DEBUG(1,("Failed to recv NT_TRANSACT_GET_USER_QUOTA\n"));
		goto cleanup;
	}

	if (cli_is_error(cli)) {
		ret = False;
		goto cleanup;
	} else {
		ret = True;
	}

	if (rdata_count == 0) {
		*pqt_list = NULL;
		return True;
	}

	if ((mem_ctx=talloc_init("SMB_USER_QUOTA_LIST"))==NULL) {
		DEBUG(0,("talloc_init() failed\n"));
		return (-1);
	}

	offset = 1;
	for (curdata=rdata,curdata_count=rdata_count;
		((curdata)&&(curdata_count>=8)&&(offset>0));
		curdata +=offset,curdata_count -= offset) {
		ZERO_STRUCT(qt);
		if (!parse_user_quota_record(curdata, curdata_count, &offset, &qt)) {
			DEBUG(1,("Failed to parse the quota record\n"));
			goto cleanup;
		}

		if ((tmp_list_ent=TALLOC_ZERO_P(mem_ctx,SMB_NTQUOTA_LIST))==NULL) {
			DEBUG(0,("talloc_zero() failed\n"));
			return (-1);
		}

		if ((tmp_list_ent->quotas=TALLOC_ZERO_P(mem_ctx,SMB_NTQUOTA_STRUCT))==NULL) {
			DEBUG(0,("talloc_zero() failed\n"));
			return (-1);
		}

		memcpy(tmp_list_ent->quotas,&qt,sizeof(qt));
		tmp_list_ent->mem_ctx = mem_ctx;		

		DLIST_ADD((*pqt_list),tmp_list_ent);
	}

	SSVAL(params, 2,TRANSACT_GET_USER_QUOTA_LIST_CONTINUE);	
	while(1) {
		if (!cli_send_nt_trans(cli, 
				       NT_TRANSACT_GET_USER_QUOTA, 
				       0, 
				       &setup, 1, 0,
				       params, 16, 4,
				       NULL, 0, 2048)) {
			DEBUG(1,("Failed to send NT_TRANSACT_GET_USER_QUOTA\n"));
			goto cleanup;
		}
		
		SAFE_FREE(rparam);
		SAFE_FREE(rdata);
		if (!cli_receive_nt_trans(cli,
					  &rparam, &rparam_count,
					  &rdata, &rdata_count)) {
			DEBUG(1,("Failed to recv NT_TRANSACT_GET_USER_QUOTA\n"));
			goto cleanup;
		}

		if (cli_is_error(cli)) {
			ret = False;
			goto cleanup;
		} else {
			ret = True;
		}
	
		if (rdata_count == 0) {
			break;	
		}

		offset = 1;
		for (curdata=rdata,curdata_count=rdata_count;
			((curdata)&&(curdata_count>=8)&&(offset>0));
			curdata +=offset,curdata_count -= offset) {
			ZERO_STRUCT(qt);
			if (!parse_user_quota_record(curdata, curdata_count, &offset, &qt)) {
				DEBUG(1,("Failed to parse the quota record\n"));
				goto cleanup;
			}

			if ((tmp_list_ent=TALLOC_ZERO_P(mem_ctx,SMB_NTQUOTA_LIST))==NULL) {
				DEBUG(0,("talloc_zero() failed\n"));
				talloc_destroy(mem_ctx);
				goto cleanup;
			}
	
			if ((tmp_list_ent->quotas=TALLOC_ZERO_P(mem_ctx,SMB_NTQUOTA_STRUCT))==NULL) {
				DEBUG(0,("talloc_zero() failed\n"));
				talloc_destroy(mem_ctx);
				goto cleanup;
			}
	
			memcpy(tmp_list_ent->quotas,&qt,sizeof(qt));
			tmp_list_ent->mem_ctx = mem_ctx;		
	
			DLIST_ADD((*pqt_list),tmp_list_ent);
		}
	}

 
	ret = True;
 cleanup:
	SAFE_FREE(rparam);
	SAFE_FREE(rdata);
 
	return ret;
}

BOOL cli_get_fs_quota_info(struct cli_state *cli, int quota_fnum, SMB_NTQUOTA_STRUCT *pqt)
{
	BOOL ret = False;
	uint16 setup;
	char param[2];
	char *rparam=NULL, *rdata=NULL;
	unsigned int rparam_count=0, rdata_count=0;
	SMB_NTQUOTA_STRUCT qt;
	ZERO_STRUCT(qt);

	if (!cli||!pqt)
		smb_panic("cli_get_fs_quota_info() called with NULL Pointer!");

	setup = TRANSACT2_QFSINFO;
	
	SSVAL(param,0,SMB_FS_QUOTA_INFORMATION);
	
	if (!cli_send_trans(cli, SMBtrans2, 
		    NULL, 
		    0, 0,
		    &setup, 1, 0,
		    param, 2, 0,
		    NULL, 0, 560)) {
		goto cleanup;
	}
	
	if (!cli_receive_trans(cli, SMBtrans2,
                              &rparam, &rparam_count,
                              &rdata, &rdata_count)) {
		goto cleanup;
	}

	if (cli_is_error(cli)) {
		ret = False;
		goto cleanup;
	} else {
		ret = True;
	}

	if (rdata_count < 48) {
		goto cleanup;
	}
	
	/* unknown_1 24 NULL bytes in pdata*/

	/* the soft quotas 8 bytes (SMB_BIG_UINT)*/
	qt.softlim = (SMB_BIG_UINT)IVAL(rdata,24);
#ifdef LARGE_SMB_OFF_T
	qt.softlim |= (((SMB_BIG_UINT)IVAL(rdata,28)) << 32);
#else /* LARGE_SMB_OFF_T */
	if ((IVAL(rdata,28) != 0)&&
		((qt.softlim != 0xFFFFFFFF)||
		 (IVAL(rdata,28)!=0xFFFFFFFF))) {
		/* more than 32 bits? */
		goto cleanup;
	}
#endif /* LARGE_SMB_OFF_T */

	/* the hard quotas 8 bytes (SMB_BIG_UINT)*/
	qt.hardlim = (SMB_BIG_UINT)IVAL(rdata,32);
#ifdef LARGE_SMB_OFF_T
	qt.hardlim |= (((SMB_BIG_UINT)IVAL(rdata,36)) << 32);
#else /* LARGE_SMB_OFF_T */
	if ((IVAL(rdata,36) != 0)&&
		((qt.hardlim != 0xFFFFFFFF)||
		 (IVAL(rdata,36)!=0xFFFFFFFF))) {
		/* more than 32 bits? */
		goto cleanup;
	}
#endif /* LARGE_SMB_OFF_T */

	/* quota_flags 2 bytes **/
	qt.qflags = SVAL(rdata,40);

	qt.qtype = SMB_USER_FS_QUOTA_TYPE;

	*pqt = qt;

	ret = True;
cleanup:
	SAFE_FREE(rparam);
	SAFE_FREE(rdata);

	return ret;	
}

BOOL cli_set_fs_quota_info(struct cli_state *cli, int quota_fnum, SMB_NTQUOTA_STRUCT *pqt)
{
	BOOL ret = False;
	uint16 setup;
	char param[4];
	char data[48];
	char *rparam=NULL, *rdata=NULL;
	unsigned int rparam_count=0, rdata_count=0;
	SMB_NTQUOTA_STRUCT qt;
	ZERO_STRUCT(qt);
	memset(data,'\0',48);

	if (!cli||!pqt)
		smb_panic("cli_set_fs_quota_info() called with NULL Pointer!");

	setup = TRANSACT2_SETFSINFO;

	SSVAL(param,0,quota_fnum);
	SSVAL(param,2,SMB_FS_QUOTA_INFORMATION);

	/* Unknown1 24 NULL bytes*/

	/* Default Soft Quota 8 bytes */
	SBIG_UINT(data,24,pqt->softlim);

	/* Default Hard Quota 8 bytes */
	SBIG_UINT(data,32,pqt->hardlim);

	/* Quota flag 2 bytes */
	SSVAL(data,40,pqt->qflags);

	/* Unknown3 6 NULL bytes */

	if (!cli_send_trans(cli, SMBtrans2, 
		    NULL, 
		    0, 0,
		    &setup, 1, 0,
		    param, 4, 0,
		    data, 48, 0)) {
		goto cleanup;
	}
	
	if (!cli_receive_trans(cli, SMBtrans2,
                              &rparam, &rparam_count,
                              &rdata, &rdata_count)) {
		goto cleanup;
	}

	if (cli_is_error(cli)) {
		ret = False;
		goto cleanup;
	} else {
		ret = True;
	}

cleanup:
	SAFE_FREE(rparam);
	SAFE_FREE(rdata);

	return ret;	
}

static char *quota_str_static(SMB_BIG_UINT val, BOOL special, BOOL _numeric)
{
	static fstring buffer;
	
	memset(buffer,'\0',sizeof(buffer));

	if (!_numeric&&special&&(val == SMB_NTQUOTAS_NO_LIMIT)) {
		fstr_sprintf(buffer,"NO LIMIT");
		return buffer;
	}
#if defined(HAVE_LONGLONG)
	fstr_sprintf(buffer,"%llu",val);
#else
	fstr_sprintf(buffer,"%lu",val);
#endif	
	return buffer;
}

void dump_ntquota(SMB_NTQUOTA_STRUCT *qt, BOOL _verbose, BOOL _numeric, void (*_sidtostring)(fstring str, DOM_SID *sid, BOOL _numeric))
{
	if (!qt)
		smb_panic("dump_ntquota() called with NULL pointer");

	switch (qt->qtype) {
		case SMB_USER_FS_QUOTA_TYPE:
			{
				d_printf("File System QUOTAS:\n");
				d_printf("Limits:\n");
				d_printf(" Default Soft Limit: %15s\n",quota_str_static(qt->softlim,True,_numeric));
				d_printf(" Default Hard Limit: %15s\n",quota_str_static(qt->hardlim,True,_numeric));
				d_printf("Quota Flags:\n");
				d_printf(" Quotas Enabled: %s\n",
					((qt->qflags&QUOTAS_ENABLED)||(qt->qflags&QUOTAS_DENY_DISK))?"On":"Off");
				d_printf(" Deny Disk:      %s\n",(qt->qflags&QUOTAS_DENY_DISK)?"On":"Off");
				d_printf(" Log Soft Limit: %s\n",(qt->qflags&QUOTAS_LOG_THRESHOLD)?"On":"Off");
				d_printf(" Log Hard Limit: %s\n",(qt->qflags&QUOTAS_LOG_LIMIT)?"On":"Off");
			}
			break;
		case SMB_USER_QUOTA_TYPE:
			{
				fstring username_str = {0};
				
				if (_sidtostring) {
					_sidtostring(username_str,&qt->sid,_numeric);
				} else {
					fstrcpy(username_str,sid_string_static(&qt->sid));
				}

				if (_verbose) {	
					d_printf("Quotas for User: %s\n",username_str);
					d_printf("Used Space: %15s\n",quota_str_static(qt->usedspace,False,_numeric));
					d_printf("Soft Limit: %15s\n",quota_str_static(qt->softlim,True,_numeric));
					d_printf("Hard Limit: %15s\n",quota_str_static(qt->hardlim,True,_numeric));
				} else {
					d_printf("%-30s: ",username_str);
					d_printf("%15s/",quota_str_static(qt->usedspace,False,_numeric));
					d_printf("%15s/",quota_str_static(qt->softlim,True,_numeric));
					d_printf("%15s\n",quota_str_static(qt->hardlim,True,_numeric));
				}
			}
			break;
		default:
			d_printf("dump_ntquota() invalid qtype(%d)\n",qt->qtype);
			return;
	}
}

void dump_ntquota_list(SMB_NTQUOTA_LIST **qtl, BOOL _verbose, BOOL _numeric, void (*_sidtostring)(fstring str, DOM_SID *sid, BOOL _numeric))
{
	SMB_NTQUOTA_LIST *cur;

	for (cur = *qtl;cur;cur = cur->next) {
		if (cur->quotas)
			dump_ntquota(cur->quotas,_verbose,_numeric,_sidtostring);
	}	
}
