/* 
   Unix SMB/CIFS implementation.
   FS info functions
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

/****************************************************************************
 Get UNIX extensions version info.
****************************************************************************/
                                                                                                                   
BOOL cli_unix_extensions_version(struct cli_state *cli, uint16 *pmajor, uint16 *pminor,
                                        uint32 *pcaplow, uint32 *pcaphigh)
{
	BOOL ret = False;
	uint16 setup;
	char param[2];
	char *rparam=NULL, *rdata=NULL;
	unsigned int rparam_count=0, rdata_count=0;

	setup = TRANSACT2_QFSINFO;
	
	SSVAL(param,0,SMB_QUERY_CIFS_UNIX_INFO);

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

	if (rdata_count < 12) {
		goto cleanup;
	}

	*pmajor = SVAL(rdata,0);
	*pminor = SVAL(rdata,2);
	*pcaplow = IVAL(rdata,4);
	*pcaphigh = IVAL(rdata,8);

	/* todo: but not yet needed 
	 *       return the other stuff
	 */

cleanup:
	SAFE_FREE(rparam);
	SAFE_FREE(rdata);

	return ret;	
}

BOOL cli_get_fs_attr_info(struct cli_state *cli, uint32 *fs_attr)
{
	BOOL ret = False;
	uint16 setup;
	char param[2];
	char *rparam=NULL, *rdata=NULL;
	unsigned int rparam_count=0, rdata_count=0;

	if (!cli||!fs_attr)
		smb_panic("cli_get_fs_attr_info() called with NULL Pionter!");

	setup = TRANSACT2_QFSINFO;
	
	SSVAL(param,0,SMB_QUERY_FS_ATTRIBUTE_INFO);

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

	if (rdata_count < 12) {
		goto cleanup;
	}

	*fs_attr = IVAL(rdata,0);

	/* todo: but not yet needed 
	 *       return the other stuff
	 */

cleanup:
	SAFE_FREE(rparam);
	SAFE_FREE(rdata);

	return ret;	
}

BOOL cli_get_fs_volume_info_old(struct cli_state *cli, fstring volume_name, uint32 *pserial_number)
{
	BOOL ret = False;
	uint16 setup;
	char param[2];
	char *rparam=NULL, *rdata=NULL;
	unsigned int rparam_count=0, rdata_count=0;
	unsigned char nlen;

	setup = TRANSACT2_QFSINFO;
	
	SSVAL(param,0,SMB_INFO_VOLUME);

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

	if (rdata_count < 5) {
		goto cleanup;
	}

	if (pserial_number) {
		*pserial_number = IVAL(rdata,0);
	}
	nlen = CVAL(rdata,l2_vol_cch);
	clistr_pull(cli, volume_name, rdata + l2_vol_szVolLabel, sizeof(fstring), nlen, STR_NOALIGN);

	/* todo: but not yet needed 
	 *       return the other stuff
	 */

cleanup:
	SAFE_FREE(rparam);
	SAFE_FREE(rdata);

	return ret;	
}

BOOL cli_get_fs_volume_info(struct cli_state *cli, fstring volume_name, uint32 *pserial_number, time_t *pdate)
{
	BOOL ret = False;
	uint16 setup;
	char param[2];
	char *rparam=NULL, *rdata=NULL;
	unsigned int rparam_count=0, rdata_count=0;
	unsigned int nlen;

	setup = TRANSACT2_QFSINFO;
	
	SSVAL(param,0,SMB_QUERY_FS_VOLUME_INFO);

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

	if (rdata_count < 19) {
		goto cleanup;
	}

	if (pdate) {
		*pdate = interpret_long_date(rdata);
	}
	if (pserial_number) {
		*pserial_number = IVAL(rdata,8);
	}
	nlen = IVAL(rdata,12);
	clistr_pull(cli, volume_name, rdata + 18, sizeof(fstring), nlen, STR_UNICODE);

	/* todo: but not yet needed 
	 *       return the other stuff
	 */

cleanup:
	SAFE_FREE(rparam);
	SAFE_FREE(rdata);

	return ret;	
}
