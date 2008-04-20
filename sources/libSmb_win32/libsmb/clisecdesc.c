/* 
   Unix SMB/CIFS implementation.
   client security descriptor functions
   Copyright (C) Andrew Tridgell 2000
   
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
  query the security descriptor for a open file
 ****************************************************************************/
SEC_DESC *cli_query_secdesc(struct cli_state *cli, int fnum, 
			    TALLOC_CTX *mem_ctx)
{
	char param[8];
	char *rparam=NULL, *rdata=NULL;
	unsigned int rparam_count=0, rdata_count=0;
	prs_struct pd;
	BOOL pd_initialized = False;
	SEC_DESC *psd = NULL;

	SIVAL(param, 0, fnum);
	SIVAL(param, 4, 0x7);

	if (!cli_send_nt_trans(cli, 
			       NT_TRANSACT_QUERY_SECURITY_DESC, 
			       0, 
			       NULL, 0, 0,
			       param, 8, 4,
			       NULL, 0, 0x10000)) {
		DEBUG(1,("Failed to send NT_TRANSACT_QUERY_SECURITY_DESC\n"));
		goto cleanup;
	}


	if (!cli_receive_nt_trans(cli, 
				  &rparam, &rparam_count,
				  &rdata, &rdata_count)) {
		DEBUG(1,("Failed to recv NT_TRANSACT_QUERY_SECURITY_DESC\n"));
		goto cleanup;
	}

	if (cli_is_error(cli))
		goto cleanup;

	if (!prs_init(&pd, rdata_count, mem_ctx, UNMARSHALL)) {
		goto cleanup;
	}
	pd_initialized = True;
	prs_copy_data_in(&pd, rdata, rdata_count);
	prs_set_offset(&pd,0);

	if (!sec_io_desc("sd data", &psd, &pd, 1)) {
		DEBUG(1,("Failed to parse secdesc\n"));
		goto cleanup;
	}

 cleanup:

	SAFE_FREE(rparam);
	SAFE_FREE(rdata);

	if (pd_initialized)
		prs_mem_free(&pd);
	return psd;
}

/****************************************************************************
  set the security descriptor for a open file
 ****************************************************************************/
BOOL cli_set_secdesc(struct cli_state *cli, int fnum, SEC_DESC *sd)
{
	char param[8];
	char *rparam=NULL, *rdata=NULL;
	unsigned int rparam_count=0, rdata_count=0;
	uint32 sec_info = 0;
	TALLOC_CTX *mem_ctx;
	prs_struct pd;
	BOOL ret = False;

	if ((mem_ctx = talloc_init("cli_set_secdesc")) == NULL) {
		DEBUG(0,("talloc_init failed.\n"));
		goto cleanup;
	}

	prs_init(&pd, 0, mem_ctx, MARSHALL);
	prs_give_memory(&pd, NULL, 0, True);

	if (!sec_io_desc("sd data", &sd, &pd, 1)) {
		DEBUG(1,("Failed to marshall secdesc\n"));
		goto cleanup;
	}

	SIVAL(param, 0, fnum);

	if (sd->off_dacl)
		sec_info |= DACL_SECURITY_INFORMATION;
	if (sd->off_owner_sid)
		sec_info |= OWNER_SECURITY_INFORMATION;
	if (sd->off_grp_sid)
		sec_info |= GROUP_SECURITY_INFORMATION;
	SSVAL(param, 4, sec_info);

	if (!cli_send_nt_trans(cli, 
			       NT_TRANSACT_SET_SECURITY_DESC, 
			       0, 
			       NULL, 0, 0,
			       param, 8, 0,
			       prs_data_p(&pd), prs_offset(&pd), 0)) {
		DEBUG(1,("Failed to send NT_TRANSACT_SET_SECURITY_DESC\n"));
		goto cleanup;
	}


	if (!cli_receive_nt_trans(cli, 
				  &rparam, &rparam_count,
				  &rdata, &rdata_count)) {
		DEBUG(1,("NT_TRANSACT_SET_SECURITY_DESC failed\n"));
		goto cleanup;
	}

	ret = True;

  cleanup:

	SAFE_FREE(rparam);
	SAFE_FREE(rdata);

	talloc_destroy(mem_ctx);

	prs_mem_free(&pd);
	return ret;
}
