/* 
   Unix SMB/CIFS implementation.
   NT Domain Authentication SMB / MSRPC client
   Copyright (C) Andrew Tridgell 1994-2000
   Copyright (C) Tim Potter 2001
   Copyright (C) Jim McDonough <jmcd@us.ibm.com> 2002
   Copyright (C) Jeremy Allison  2005.

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

WERROR rpccli_srvsvc_net_srv_get_info(struct rpc_pipe_client *cli, 
				   TALLOC_CTX *mem_ctx,
				   uint32 switch_value, SRV_INFO_CTR *ctr)
{
	prs_struct qbuf, rbuf;
	SRV_Q_NET_SRV_GET_INFO q;
	SRV_R_NET_SRV_GET_INFO r;
	WERROR result = W_ERROR(ERRgeneral);
	fstring server;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Initialise input parameters */

	slprintf(server, sizeof(fstring)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(server);

	init_srv_q_net_srv_get_info(&q, server, switch_value);
	r.ctr = ctr;

	/* Marshall data and send request */

	CLI_DO_RPC_WERR(cli, mem_ctx, PI_SRVSVC, SRV_NET_SRV_GET_INFO,
		q, r,
		qbuf, rbuf,
		srv_io_q_net_srv_get_info,
		srv_io_r_net_srv_get_info,
		WERR_GENERAL_FAILURE);

	result = r.status;
	return result;
}

WERROR rpccli_srvsvc_net_share_enum(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				 uint32 info_level, SRV_SHARE_INFO_CTR *ctr,
				 int preferred_len, ENUM_HND *hnd)
{
	prs_struct qbuf, rbuf;
	SRV_Q_NET_SHARE_ENUM q;
	SRV_R_NET_SHARE_ENUM r;
	WERROR result = W_ERROR(ERRgeneral);
	fstring server;
	int i;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Initialise input parameters */

	slprintf(server, sizeof(fstring)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(server);

	init_srv_q_net_share_enum(&q, server, info_level, preferred_len, hnd);

	/* Marshall data and send request */

	CLI_DO_RPC_WERR(cli, mem_ctx, PI_SRVSVC, SRV_NET_SHARE_ENUM_ALL,
		q, r,
		qbuf, rbuf,
		srv_io_q_net_share_enum,
		srv_io_r_net_share_enum,
		WERR_GENERAL_FAILURE);

	result = r.status;

	if (!W_ERROR_IS_OK(result))
		goto done;

	/* Oh yuck yuck yuck - we have to copy all the info out of the
	   SRV_SHARE_INFO_CTR in the SRV_R_NET_SHARE_ENUM as when we do a
	   prs_mem_free() it will all be invalidated.  The various share
	   info structures suck badly too.  This really is gross. */

	ZERO_STRUCTP(ctr);

	if (!r.ctr.num_entries)
		goto done;

	ctr->info_level = info_level;
	ctr->num_entries = r.ctr.num_entries;

	switch(info_level) {
	case 1:
		ctr->share.info1 = TALLOC_ARRAY(mem_ctx, SRV_SHARE_INFO_1, ctr->num_entries);
		if (ctr->share.info1 == NULL) {
			return WERR_NOMEM;
		}
		
		memset(ctr->share.info1, 0, sizeof(SRV_SHARE_INFO_1));

		for (i = 0; i < ctr->num_entries; i++) {
			SRV_SHARE_INFO_1 *info1 = &ctr->share.info1[i];
			char *s;
			
			/* Copy pointer crap */

			memcpy(&info1->info_1, &r.ctr.share.info1[i].info_1, 
			       sizeof(SH_INFO_1));

			/* Duplicate strings */

			s = unistr2_tdup(mem_ctx, &r.ctr.share.info1[i].info_1_str.uni_netname);
			if (s)
				init_unistr2(&info1->info_1_str.uni_netname, s, UNI_STR_TERMINATE);
		
			s = unistr2_tdup(mem_ctx, &r.ctr.share.info1[i].info_1_str.uni_remark);
			if (s)
				init_unistr2(&info1->info_1_str.uni_remark, s, UNI_STR_TERMINATE);

		}		

		break;
	case 2:
		ctr->share.info2 = TALLOC_ARRAY(mem_ctx, SRV_SHARE_INFO_2, ctr->num_entries);
		if (ctr->share.info2 == NULL) {
			return WERR_NOMEM;
		}
		
		memset(ctr->share.info2, 0, sizeof(SRV_SHARE_INFO_2));

		for (i = 0; i < ctr->num_entries; i++) {
			SRV_SHARE_INFO_2 *info2 = &ctr->share.info2[i];
			char *s;
			
			/* Copy pointer crap */

			memcpy(&info2->info_2, &r.ctr.share.info2[i].info_2, 
			       sizeof(SH_INFO_2));

			/* Duplicate strings */

			s = unistr2_tdup(mem_ctx, &r.ctr.share.info2[i].info_2_str.uni_netname);
			if (s)
				init_unistr2(&info2->info_2_str.uni_netname, s, UNI_STR_TERMINATE);

			s = unistr2_tdup(mem_ctx, &r.ctr.share.info2[i].info_2_str.uni_remark);
			if (s)
				init_unistr2(&info2->info_2_str.uni_remark, s, UNI_STR_TERMINATE);

			s = unistr2_tdup(mem_ctx, &r.ctr.share.info2[i].info_2_str.uni_path);
			if (s)
				init_unistr2(&info2->info_2_str.uni_path, s, UNI_STR_TERMINATE);

			s = unistr2_tdup(mem_ctx, &r.ctr.share.info2[i].info_2_str.uni_passwd);
			if (s)
				init_unistr2(&info2->info_2_str.uni_passwd, s, UNI_STR_TERMINATE);
		}
		break;
	/* adding info-level 502 here */
	case 502:
		ctr->share.info502 = TALLOC_ARRAY(mem_ctx, SRV_SHARE_INFO_502, ctr->num_entries);

		if (ctr->share.info502 == NULL) {
			return WERR_NOMEM;
		}
		
		memset(ctr->share.info502, 0, sizeof(SRV_SHARE_INFO_502));

		for (i = 0; i < ctr->num_entries; i++) {
			SRV_SHARE_INFO_502 *info502 = &ctr->share.info502[i];
			char *s;
			
			/* Copy pointer crap */
			memcpy(&info502->info_502, &r.ctr.share.info502[i].info_502, 
			       sizeof(SH_INFO_502));

			/* Duplicate strings */

			s = unistr2_tdup(mem_ctx, &r.ctr.share.info502[i].info_502_str.uni_netname);
			if (s)
				init_unistr2(&info502->info_502_str.uni_netname, s, UNI_STR_TERMINATE);

			s = unistr2_tdup(mem_ctx, &r.ctr.share.info502[i].info_502_str.uni_remark);
			if (s)
				init_unistr2(&info502->info_502_str.uni_remark, s, UNI_STR_TERMINATE);

			s = unistr2_tdup(mem_ctx, &r.ctr.share.info502[i].info_502_str.uni_path);
			if (s)
				init_unistr2(&info502->info_502_str.uni_path, s, UNI_STR_TERMINATE);

			s = unistr2_tdup(mem_ctx, &r.ctr.share.info502[i].info_502_str.uni_passwd);
			if (s)
				init_unistr2(&info502->info_502_str.uni_passwd, s, UNI_STR_TERMINATE);
		
			info502->info_502_str.sd = dup_sec_desc(mem_ctx, r.ctr.share.info502[i].info_502_str.sd);
		}
		break;
	}

  done:

	return result;
}

WERROR rpccli_srvsvc_net_share_get_info(struct rpc_pipe_client *cli,
				     TALLOC_CTX *mem_ctx,
				     const char *sharename,
				     uint32 info_level,
				     SRV_SHARE_INFO *info)
{
	prs_struct qbuf, rbuf;
	SRV_Q_NET_SHARE_GET_INFO q;
	SRV_R_NET_SHARE_GET_INFO r;
	WERROR result = W_ERROR(ERRgeneral);
	fstring server;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Initialise input parameters */

	slprintf(server, sizeof(fstring)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(server);

	init_srv_q_net_share_get_info(&q, server, sharename, info_level);

	/* Marshall data and send request */

	CLI_DO_RPC_WERR(cli, mem_ctx, PI_SRVSVC, SRV_NET_SHARE_GET_INFO,
		q, r,
		qbuf, rbuf,
		srv_io_q_net_share_get_info,
		srv_io_r_net_share_get_info,
		WERR_GENERAL_FAILURE);

	result = r.status;

	if (!W_ERROR_IS_OK(result))
		goto done;

	ZERO_STRUCTP(info);

	info->switch_value = info_level;

	switch(info_level) {
	case 1:
	{
		SRV_SHARE_INFO_1 *info1 = &info->share.info1;
		SH_INFO_1_STR *info1_str = &info1->info_1_str;
		
		char *s;

		info->share.info1 = r.info.share.info1;

		/* Duplicate strings */

		s = unistr2_tdup(mem_ctx, &info1_str->uni_netname);
		if (s)
			init_unistr2(&info1_str->uni_netname,
				     s, UNI_STR_TERMINATE);

		s = unistr2_tdup(mem_ctx, &info1_str->uni_remark);
		if (s)
			init_unistr2(&info1_str->uni_remark,
				     s, UNI_STR_TERMINATE);

		break;
	}
	case 2:
	{
		SRV_SHARE_INFO_2 *info2 = &info->share.info2;
		SH_INFO_2_STR *info2_str = &info2->info_2_str;
		
		char *s;

		info->share.info2 = r.info.share.info2;

		/* Duplicate strings */

		s = unistr2_tdup(mem_ctx, &info2_str->uni_netname);
		if (s)
			init_unistr2(&info2_str->uni_netname,
				     s, UNI_STR_TERMINATE);

		s = unistr2_tdup(mem_ctx, &info2_str->uni_remark);
		if (s)
			init_unistr2(&info2_str->uni_remark,
				     s, UNI_STR_TERMINATE);

		s = unistr2_tdup(mem_ctx, &info2_str->uni_path);
		if (s)
			init_unistr2(&info2_str->uni_path,
				     s, UNI_STR_TERMINATE);

		s = unistr2_tdup(mem_ctx, &info2_str->uni_passwd);
		if (s)
			init_unistr2(&info2_str->uni_passwd,
				     s, UNI_STR_TERMINATE);


		break;
	}
	case 502:
	{
		SRV_SHARE_INFO_502 *info502 = &info->share.info502;
		SH_INFO_502_STR *info502_str = &info502->info_502_str;
		
		char *s;

		info->share.info502 = r.info.share.info502;

		/* Duplicate strings */

		s = unistr2_tdup(mem_ctx, &info502_str->uni_netname);
		if (s)
			init_unistr2(&info502_str->uni_netname,
				     s, UNI_STR_TERMINATE);

		s = unistr2_tdup(mem_ctx, &info502_str->uni_remark);
		if (s)
			init_unistr2(&info502_str->uni_remark,
				     s, UNI_STR_TERMINATE);

		s = unistr2_tdup(mem_ctx, &info502_str->uni_path);
		if (s)
			init_unistr2(&info502_str->uni_path,
				     s, UNI_STR_TERMINATE);

		s = unistr2_tdup(mem_ctx, &info502_str->uni_passwd);
		if (s)
			init_unistr2(&info502_str->uni_passwd,
				     s, UNI_STR_TERMINATE);

		info502_str->sd = dup_sec_desc(mem_ctx, info502_str->sd);
		break;
	}
	default:
		DEBUG(0,("unimplemented info-level: %d\n", info_level));
		break;
	}

  done:

	return result;
}

WERROR rpccli_srvsvc_net_share_set_info(struct rpc_pipe_client *cli,
				     TALLOC_CTX *mem_ctx,
				     const char *sharename,
				     uint32 info_level,
				     SRV_SHARE_INFO *info)
{
	prs_struct qbuf, rbuf;
	SRV_Q_NET_SHARE_SET_INFO q;
	SRV_R_NET_SHARE_SET_INFO r;
	WERROR result = W_ERROR(ERRgeneral);
	fstring server;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Initialise input parameters */

	slprintf(server, sizeof(fstring)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(server);

	init_srv_q_net_share_set_info(&q, server, sharename, info_level, info);

	/* Marshall data and send request */

	CLI_DO_RPC_WERR(cli, mem_ctx, PI_SRVSVC, SRV_NET_SHARE_SET_INFO,
		q, r,
		qbuf, rbuf,
		srv_io_q_net_share_set_info,
		srv_io_r_net_share_set_info,
		WERR_GENERAL_FAILURE);

	result = r.status;
	return result;
}

WERROR rpccli_srvsvc_net_share_del(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				const char *sharename)
{
	prs_struct qbuf, rbuf;
	SRV_Q_NET_SHARE_DEL q;
	SRV_R_NET_SHARE_DEL r;
	WERROR result = W_ERROR(ERRgeneral);
	fstring server;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Initialise input parameters */

	slprintf(server, sizeof(fstring)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(server);

	init_srv_q_net_share_del(&q, server, sharename);

	/* Marshall data and send request */

	CLI_DO_RPC_WERR(cli, mem_ctx, PI_SRVSVC, SRV_NET_SHARE_DEL,
		q, r,
		qbuf, rbuf,
		srv_io_q_net_share_del,
		srv_io_r_net_share_del,
		WERR_GENERAL_FAILURE);

	result = r.status;
	return result;
}

WERROR rpccli_srvsvc_net_share_add(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				const char *netname, uint32 type, 
				const char *remark, uint32 perms, 
				uint32 max_uses, uint32 num_uses, 
				const char *path, const char *passwd,
				int level, SEC_DESC *sd)
{
	prs_struct qbuf, rbuf;
	SRV_Q_NET_SHARE_ADD q;
	SRV_R_NET_SHARE_ADD r;
	WERROR result = W_ERROR(ERRgeneral);
	fstring server;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	slprintf(server, sizeof(fstring)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(server);

	init_srv_q_net_share_add(&q,server, netname, type, remark,
				 perms, max_uses, num_uses, path, passwd, 
				 level, sd);

	/* Marshall data and send request */

	CLI_DO_RPC_WERR(cli, mem_ctx, PI_SRVSVC, SRV_NET_SHARE_ADD,
		q, r,
		qbuf, rbuf,
		srv_io_q_net_share_add,
		srv_io_r_net_share_add,
		WERR_GENERAL_FAILURE);

	result = r.status;
	return result;	
}

WERROR rpccli_srvsvc_net_remote_tod(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				 char *server, TIME_OF_DAY_INFO *tod)
{
	prs_struct qbuf, rbuf;
	SRV_Q_NET_REMOTE_TOD q;
	SRV_R_NET_REMOTE_TOD r;
	WERROR result = W_ERROR(ERRgeneral);
	fstring server_slash;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Initialise input parameters */

	slprintf(server_slash, sizeof(fstring)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(server_slash);

	init_srv_q_net_remote_tod(&q, server_slash);
	r.tod = tod;

	/* Marshall data and send request */

	CLI_DO_RPC_WERR(cli, mem_ctx, PI_SRVSVC, SRV_NET_REMOTE_TOD,
		q, r,
		qbuf, rbuf,
		srv_io_q_net_remote_tod,
		srv_io_r_net_remote_tod,
		WERR_GENERAL_FAILURE);

	result = r.status;
	return result;	
}

WERROR rpccli_srvsvc_net_file_enum(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				uint32 file_level, const char *user_name,
				SRV_FILE_INFO_CTR *ctr,	int preferred_len,
				ENUM_HND *hnd)
{
	prs_struct qbuf, rbuf;
	SRV_Q_NET_FILE_ENUM q;
	SRV_R_NET_FILE_ENUM r;
	WERROR result = W_ERROR(ERRgeneral);
	fstring server;
	int i;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Initialise input parameters */

	slprintf(server, sizeof(fstring)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(server);

	init_srv_q_net_file_enum(&q, server, NULL, user_name, 
				 file_level, ctr, preferred_len, hnd);

	/* Marshall data and send request */

	CLI_DO_RPC_WERR(cli, mem_ctx, PI_SRVSVC, SRV_NET_FILE_ENUM,
		q, r,
		qbuf, rbuf,
		srv_io_q_net_file_enum,
		srv_io_r_net_file_enum,
		WERR_GENERAL_FAILURE);

	result = r.status;

	if (!W_ERROR_IS_OK(result))
		goto done;

	/* copy the data over to the ctr */

	ZERO_STRUCTP(ctr);

	ctr->switch_value = file_level;

	ctr->num_entries = ctr->num_entries2 = r.ctr.num_entries;
	
	switch(file_level) {
	case 3:
		ctr->file.info3 = TALLOC_ARRAY(mem_ctx, SRV_FILE_INFO_3, ctr->num_entries);
		if (ctr->file.info3 == NULL) {
			return WERR_NOMEM;
		}

		memset(ctr->file.info3, 0, 
		       sizeof(SRV_FILE_INFO_3) * ctr->num_entries);

		for (i = 0; i < r.ctr.num_entries; i++) {
			SRV_FILE_INFO_3 *info3 = &ctr->file.info3[i];
			char *s;
			
			/* Copy pointer crap */

			memcpy(&info3->info_3, &r.ctr.file.info3[i].info_3, 
			       sizeof(FILE_INFO_3));

			/* Duplicate strings */

			s = unistr2_tdup(mem_ctx, &r.ctr.file.info3[i].info_3_str.uni_path_name);
			if (s)
				init_unistr2(&info3->info_3_str.uni_path_name, s, UNI_STR_TERMINATE);
		
			s = unistr2_tdup(mem_ctx, &r.ctr.file.info3[i].info_3_str.uni_user_name);
			if (s)
				init_unistr2(&info3->info_3_str.uni_user_name, s, UNI_STR_TERMINATE);

		}		

		break;
	}

  done:
	return result;
}

WERROR rpccli_srvsvc_net_file_close(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				 uint32 file_id)
{
	prs_struct qbuf, rbuf;
	SRV_Q_NET_FILE_CLOSE q;
	SRV_R_NET_FILE_CLOSE r;
	WERROR result = W_ERROR(ERRgeneral);
	fstring server;

	ZERO_STRUCT(q);
	ZERO_STRUCT(r);

	/* Initialise input parameters */

	slprintf(server, sizeof(fstring)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(server);

	init_srv_q_net_file_close(&q, server, file_id);

	/* Marshall data and send request */

	CLI_DO_RPC_WERR(cli, mem_ctx, PI_SRVSVC, SRV_NET_FILE_CLOSE,
		q, r,
		qbuf, rbuf,
		srv_io_q_net_file_close,
		srv_io_r_net_file_close,
		WERR_GENERAL_FAILURE);

	result = r.status;
	return result;
}
