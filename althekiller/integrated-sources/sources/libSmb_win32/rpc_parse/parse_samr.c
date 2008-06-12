/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Andrew Tridgell              1992-2000,
 *  Copyright (C) Luke Kenneth Casson Leighton 1996-2000,
 *  Copyright (C) Paul Ashton                  1997-2000,
 *  Copyright (C) Elrond                            2000,
 *  Copyright (C) Jeremy Allison                    2001,
 *  Copyright (C) Jean François Micouleau      1998-2001,
 *  Copyright (C) Jim McDonough <jmcd@us.ibm.com>   2002.
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_PARSE

/*******************************************************************
inits a SAMR_Q_CLOSE_HND structure.
********************************************************************/

void init_samr_q_close_hnd(SAMR_Q_CLOSE_HND * q_c, POLICY_HND *hnd)
{
	DEBUG(5, ("init_samr_q_close_hnd\n"));
	
	q_c->pol = *hnd;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_close_hnd(const char *desc, SAMR_Q_CLOSE_HND * q_u,
			 prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_close_hnd");
	depth++;

	if(!prs_align(ps))
		return False;

	return smb_io_pol_hnd("pol", &q_u->pol, ps, depth);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_close_hnd(const char *desc, SAMR_R_CLOSE_HND * r_u,
			 prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_close_hnd");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &r_u->pol, ps, depth))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_LOOKUP_DOMAIN structure.
********************************************************************/

void init_samr_q_lookup_domain(SAMR_Q_LOOKUP_DOMAIN * q_u,
			       POLICY_HND *pol, char *dom_name)
{
	DEBUG(5, ("init_samr_q_lookup_domain\n"));

	q_u->connect_pol = *pol;

	init_unistr2(&q_u->uni_domain, dom_name, UNI_FLAGS_NONE);
	init_uni_hdr(&q_u->hdr_domain, &q_u->uni_domain);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/
BOOL samr_io_q_lookup_domain(const char *desc, SAMR_Q_LOOKUP_DOMAIN * q_u,
			     prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_lookup_domain");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("connect_pol", &q_u->connect_pol, ps, depth))
		return False;

	if(!smb_io_unihdr("hdr_domain", &q_u->hdr_domain, ps, depth))
		return False;

	if(!smb_io_unistr2("uni_domain", &q_u->uni_domain, q_u->hdr_domain.buffer, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_R_LOOKUP_DOMAIN structure.
********************************************************************/

void init_samr_r_lookup_domain(SAMR_R_LOOKUP_DOMAIN * r_u,
			       DOM_SID *dom_sid, NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_lookup_domain\n"));

	r_u->status = status;
	r_u->ptr_sid = 0;
	if (NT_STATUS_IS_OK(status)) {
		r_u->ptr_sid = 1;
		init_dom_sid2(&r_u->dom_sid, dom_sid);
	}
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_lookup_domain(const char *desc, SAMR_R_LOOKUP_DOMAIN * r_u,
			     prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_lookup_domain");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr", ps, depth, &r_u->ptr_sid))
		return False;

	if (r_u->ptr_sid != 0) {
		if(!smb_io_dom_sid2("sid", &r_u->dom_sid, ps, depth))
			return False;
		if(!prs_align(ps))
			return False;
	}

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

void init_samr_q_remove_sid_foreign_domain(SAMR_Q_REMOVE_SID_FOREIGN_DOMAIN * q_u, POLICY_HND *dom_pol, DOM_SID *sid)
{
	DEBUG(5, ("samr_init_samr_q_remove_sid_foreign_domain\n"));

	q_u->dom_pol = *dom_pol;
	init_dom_sid2(&q_u->sid, sid);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_remove_sid_foreign_domain(const char *desc, SAMR_Q_REMOVE_SID_FOREIGN_DOMAIN * q_u,
			  prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_remove_sid_foreign_domain");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("domain_pol", &q_u->dom_pol, ps, depth))
		return False;

	if(!smb_io_dom_sid2("sid", &q_u->sid, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_remove_sid_foreign_domain(const char *desc, SAMR_R_REMOVE_SID_FOREIGN_DOMAIN * r_u,
			  prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_remove_sid_foreign_domain");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

void init_samr_q_open_domain(SAMR_Q_OPEN_DOMAIN * q_u,
			     POLICY_HND *pol, uint32 flags,
			     const DOM_SID *sid)
{
	DEBUG(5, ("samr_init_samr_q_open_domain\n"));

	q_u->pol = *pol;
	q_u->flags = flags;
	init_dom_sid2(&q_u->dom_sid, sid);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_open_domain(const char *desc, SAMR_Q_OPEN_DOMAIN * q_u,
			   prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_open_domain");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &q_u->pol, ps, depth))
		return False;

	if(!prs_uint32("flags", ps, depth, &q_u->flags))
		return False;

	if(!smb_io_dom_sid2("sid", &q_u->dom_sid, ps, depth))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_open_domain(const char *desc, SAMR_R_OPEN_DOMAIN * r_u,
			   prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_open_domain");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("domain_pol", &r_u->domain_pol, ps, depth))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

void init_samr_q_get_usrdom_pwinfo(SAMR_Q_GET_USRDOM_PWINFO * q_u,
				   POLICY_HND *user_pol)
{
	DEBUG(5, ("samr_init_samr_q_get_usrdom_pwinfo\n"));

	q_u->user_pol = *user_pol;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_get_usrdom_pwinfo(const char *desc, SAMR_Q_GET_USRDOM_PWINFO * q_u,
				 prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_get_usrdom_pwinfo");
	depth++;

	if(!prs_align(ps))
		return False;

	return smb_io_pol_hnd("user_pol", &q_u->user_pol, ps, depth);
}

/*******************************************************************
 Init.
********************************************************************/

void init_samr_r_get_usrdom_pwinfo(SAMR_R_GET_USRDOM_PWINFO *r_u, NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_get_usrdom_pwinfo\n"));
	
	r_u->min_pwd_length = 0x0000;

	/*
	 * used to be 	
	 * r_u->unknown_1 = 0x0015;
	 * but for trusts.
	 */
	r_u->unknown_1 = 0x01D1;
	r_u->unknown_1 = 0x0015;

	r_u->password_properties = 0x00000000;

	r_u->status = status;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_get_usrdom_pwinfo(const char *desc, SAMR_R_GET_USRDOM_PWINFO * r_u,
				 prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_get_usrdom_pwinfo");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint16("min_pwd_length", ps, depth, &r_u->min_pwd_length))
		return False;
	if(!prs_uint16("unknown_1", ps, depth, &r_u->unknown_1))
		return False;
	if(!prs_uint32("password_properties", ps, depth, &r_u->password_properties))
		return False;

	if(!prs_ntstatus("status   ", ps, depth, &r_u->status))
		return False;

	return True;
}


/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_set_sec_obj(const char *desc, SAMR_Q_SET_SEC_OBJ * q_u,
			     prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_set_sec_obj");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &q_u->pol, ps, depth))
		return False;

	if(!prs_uint32("sec_info", ps, depth, &q_u->sec_info))
		return False;
		
	if(!sec_io_desc_buf("sec_desc", &q_u->buf, ps, depth))
		return False;
	
	return True;
}


/*******************************************************************
reads or writes a structure.
********************************************************************/

void init_samr_q_query_sec_obj(SAMR_Q_QUERY_SEC_OBJ * q_u,
			       POLICY_HND *user_pol, uint32 sec_info)
{
	DEBUG(5, ("samr_init_samr_q_query_sec_obj\n"));

	q_u->user_pol = *user_pol;
	q_u->sec_info = sec_info;
}


/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_query_sec_obj(const char *desc, SAMR_Q_QUERY_SEC_OBJ * q_u,
			     prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_query_sec_obj");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("user_pol", &q_u->user_pol, ps, depth))
		return False;

	if(!prs_uint32("sec_info", ps, depth, &q_u->sec_info))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

void init_samr_q_query_domain_info(SAMR_Q_QUERY_DOMAIN_INFO * q_u,
				   POLICY_HND *domain_pol, uint16 switch_value)
{
	DEBUG(5, ("samr_init_samr_q_query_domain_info\n"));

	q_u->domain_pol = *domain_pol;
	q_u->switch_value = switch_value;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_query_domain_info(const char *desc, SAMR_Q_QUERY_DOMAIN_INFO * q_u,
				 prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_query_domain_info");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("domain_pol", &q_u->domain_pol, ps, depth))
		return False;

	if(!prs_uint16("switch_value", ps, depth, &q_u->switch_value))
		return False;

	return True;
}

/*******************************************************************
inits a structure.
********************************************************************/

void init_unk_info1(SAM_UNK_INFO_1 *u_1, uint16 min_pass_len, uint16 pass_hist, 
		    uint32 password_properties, NTTIME nt_expire, NTTIME nt_min_age)
{
	u_1->min_length_password = min_pass_len;
	u_1->password_history = pass_hist;
	
	if (lp_check_password_script() && *lp_check_password_script()) {
		password_properties |= DOMAIN_PASSWORD_COMPLEX;
	}
	u_1->password_properties = password_properties;

	/* password never expire */
	u_1->expire.high = nt_expire.high;
	u_1->expire.low = nt_expire.low;

	/* can change the password now */
	u_1->min_passwordage.high = nt_min_age.high;
	u_1->min_passwordage.low = nt_min_age.low;
	
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_unk_info1(const char *desc, SAM_UNK_INFO_1 * u_1,
			     prs_struct *ps, int depth)
{
	if (u_1 == NULL)
	  return False;

	prs_debug(ps, depth, desc, "sam_io_unk_info1");
	depth++;

	if(!prs_uint16("min_length_password", ps, depth, &u_1->min_length_password))
		return False;
	if(!prs_uint16("password_history", ps, depth, &u_1->password_history))
		return False;
	if(!prs_uint32("password_properties", ps, depth, &u_1->password_properties))
		return False;
	if(!smb_io_time("expire", &u_1->expire, ps, depth))
		return False;
	if(!smb_io_time("min_passwordage", &u_1->min_passwordage, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a structure.
********************************************************************/

void init_unk_info2(SAM_UNK_INFO_2 * u_2,
			const char *comment, const char *domain, const char *server,
			uint32 seq_num, uint32 num_users, uint32 num_groups, uint32 num_alias, NTTIME nt_logout, uint32 server_role)
{
	u_2->logout.low = nt_logout.low;
	u_2->logout.high = nt_logout.high;

	u_2->seq_num.low = seq_num;
	u_2->seq_num.high = 0x00000000;


	u_2->unknown_4 = 0x00000001;
	u_2->server_role = server_role;
	u_2->unknown_6 = 0x00000001;
	u_2->num_domain_usrs = num_users;
	u_2->num_domain_grps = num_groups;
	u_2->num_local_grps = num_alias;

	init_unistr2(&u_2->uni_comment, comment, UNI_FLAGS_NONE);
	init_uni_hdr(&u_2->hdr_comment, &u_2->uni_comment);
	init_unistr2(&u_2->uni_domain, domain, UNI_FLAGS_NONE);
	init_uni_hdr(&u_2->hdr_domain, &u_2->uni_domain);
	init_unistr2(&u_2->uni_server, server, UNI_FLAGS_NONE);
	init_uni_hdr(&u_2->hdr_server, &u_2->uni_server);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_unk_info2(const char *desc, SAM_UNK_INFO_2 * u_2,
			     prs_struct *ps, int depth)
{
	if (u_2 == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_unk_info2");
	depth++;

	if(!smb_io_time("logout", &u_2->logout, ps, depth))
		return False;
	if(!smb_io_unihdr("hdr_comment", &u_2->hdr_comment, ps, depth))
		return False;
	if(!smb_io_unihdr("hdr_domain", &u_2->hdr_domain, ps, depth))
		return False;
	if(!smb_io_unihdr("hdr_server", &u_2->hdr_server, ps, depth))
		return False;

	/* put all the data in here, at the moment, including what the above
	   pointer is referring to
	 */

	if(!prs_uint64("seq_num ", ps, depth, &u_2->seq_num))
		return False;

	if(!prs_uint32("unknown_4 ", ps, depth, &u_2->unknown_4)) /* 0x0000 0001 */
		return False;
	if(!prs_uint32("server_role ", ps, depth, &u_2->server_role))
		return False;
	if(!prs_uint32("unknown_6 ", ps, depth, &u_2->unknown_6)) /* 0x0000 0001 */
		return False;
	if(!prs_uint32("num_domain_usrs ", ps, depth, &u_2->num_domain_usrs))
		return False;
	if(!prs_uint32("num_domain_grps", ps, depth, &u_2->num_domain_grps))
		return False;
	if(!prs_uint32("num_local_grps", ps, depth, &u_2->num_local_grps))
		return False;

	if(!smb_io_unistr2("uni_comment", &u_2->uni_comment, u_2->hdr_comment.buffer, ps, depth))
		return False;
	if(!smb_io_unistr2("uni_domain", &u_2->uni_domain, u_2->hdr_domain.buffer, ps, depth))
		return False;
	if(!smb_io_unistr2("uni_server", &u_2->uni_server, u_2->hdr_server.buffer, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a structure.
********************************************************************/

void init_unk_info3(SAM_UNK_INFO_3 *u_3, NTTIME nt_logout)
{
	u_3->logout.low = nt_logout.low;
	u_3->logout.high = nt_logout.high;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_unk_info3(const char *desc, SAM_UNK_INFO_3 * u_3,
			     prs_struct *ps, int depth)
{
	if (u_3 == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_unk_info3");
	depth++;

	if(!smb_io_time("logout", &u_3->logout, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a structure.
********************************************************************/

void init_unk_info4(SAM_UNK_INFO_4 * u_4,const char *comment)
{
	init_unistr2(&u_4->uni_comment, comment, UNI_FLAGS_NONE);
	init_uni_hdr(&u_4->hdr_comment, &u_4->uni_comment);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_unk_info4(const char *desc, SAM_UNK_INFO_4 * u_4,
			     prs_struct *ps, int depth)
{
	if (u_4 == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_unk_info4");
	depth++;

	if(!smb_io_unihdr("hdr_comment", &u_4->hdr_comment, ps, depth))
		return False;

	if(!smb_io_unistr2("uni_comment", &u_4->uni_comment, u_4->hdr_comment.buffer, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a structure.
********************************************************************/

void init_unk_info5(SAM_UNK_INFO_5 * u_5,const char *domain)
{
	init_unistr2(&u_5->uni_domain, domain, UNI_FLAGS_NONE);
	init_uni_hdr(&u_5->hdr_domain, &u_5->uni_domain);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_unk_info5(const char *desc, SAM_UNK_INFO_5 * u_5,
			     prs_struct *ps, int depth)
{
	if (u_5 == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_unk_info5");
	depth++;

	if(!smb_io_unihdr("hdr_domain", &u_5->hdr_domain, ps, depth))
		return False;

	if(!smb_io_unistr2("uni_domain", &u_5->uni_domain, u_5->hdr_domain.buffer, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a structure.
********************************************************************/

void init_unk_info6(SAM_UNK_INFO_6 * u_6, const char *server)
{
	init_unistr2(&u_6->uni_server, server, UNI_FLAGS_NONE);
	init_uni_hdr(&u_6->hdr_server, &u_6->uni_server);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_unk_info6(const char *desc, SAM_UNK_INFO_6 * u_6,
			     prs_struct *ps, int depth)
{
	if (u_6 == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_unk_info6");
	depth++;

	if(!smb_io_unihdr("hdr_server", &u_6->hdr_server, ps, depth))
		return False;

	if(!smb_io_unistr2("uni_server", &u_6->uni_server, u_6->hdr_server.buffer, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a structure.
********************************************************************/

void init_unk_info7(SAM_UNK_INFO_7 * u_7, uint32 server_role)
{
	u_7->server_role = server_role;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_unk_info7(const char *desc, SAM_UNK_INFO_7 * u_7,
			     prs_struct *ps, int depth)
{
	if (u_7 == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_unk_info7");
	depth++;

	if(!prs_uint16("server_role", ps, depth, &u_7->server_role))
		return False;

	return True;
}

/*******************************************************************
inits a structure.
********************************************************************/

void init_unk_info8(SAM_UNK_INFO_8 * u_8, uint32 seq_num)
{
	unix_to_nt_time(&u_8->domain_create_time, 0);
	u_8->seq_num.low = seq_num;
	u_8->seq_num.high = 0x0000;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_unk_info8(const char *desc, SAM_UNK_INFO_8 * u_8,
			     prs_struct *ps, int depth)
{
	if (u_8 == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_unk_info8");
	depth++;

	if (!prs_uint64("seq_num", ps, depth, &u_8->seq_num))
		return False;

	if(!smb_io_time("domain_create_time", &u_8->domain_create_time, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a structure.
********************************************************************/

void init_unk_info9(SAM_UNK_INFO_9 * u_9, uint32 unknown)
{
	u_9->unknown = unknown;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_unk_info9(const char *desc, SAM_UNK_INFO_9 * u_9,
			     prs_struct *ps, int depth)
{
	if (u_9 == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_unk_info9");
	depth++;

	if (!prs_uint32("unknown", ps, depth, &u_9->unknown))
		return False;

	return True;
}

/*******************************************************************
inits a structure.
********************************************************************/

void init_unk_info12(SAM_UNK_INFO_12 * u_12, NTTIME nt_lock_duration, NTTIME nt_reset_time, uint16 lockout)
{
	u_12->duration.low = nt_lock_duration.low;
	u_12->duration.high = nt_lock_duration.high;
	u_12->reset_count.low = nt_reset_time.low;
	u_12->reset_count.high = nt_reset_time.high;

	u_12->bad_attempt_lockout = lockout;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_unk_info12(const char *desc, SAM_UNK_INFO_12 * u_12,
			      prs_struct *ps, int depth)
{
	if (u_12 == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_unk_info12");
	depth++;

	if(!smb_io_time("duration", &u_12->duration, ps, depth))
		return False;
	if(!smb_io_time("reset_count", &u_12->reset_count, ps, depth))
		return False;
	if(!prs_uint16("bad_attempt_lockout", ps, depth, &u_12->bad_attempt_lockout))
		return False;

	return True;
}

/*******************************************************************
inits a structure.
********************************************************************/

void init_unk_info13(SAM_UNK_INFO_13 * u_13, uint32 seq_num)
{
	unix_to_nt_time(&u_13->domain_create_time, 0);
	u_13->seq_num.low = seq_num;
	u_13->seq_num.high = 0x0000;
	u_13->unknown1 = 0;
	u_13->unknown2 = 0;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_unk_info13(const char *desc, SAM_UNK_INFO_13 * u_13,
			     prs_struct *ps, int depth)
{
	if (u_13 == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_unk_info13");
	depth++;

	if (!prs_uint64("seq_num", ps, depth, &u_13->seq_num))
		return False;

	if(!smb_io_time("domain_create_time", &u_13->domain_create_time, ps, depth))
		return False;

	if (!prs_uint32("unknown1", ps, depth, &u_13->unknown1))
		return False;
	if (!prs_uint32("unknown2", ps, depth, &u_13->unknown2))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_R_QUERY_DOMAIN_INFO structure.
********************************************************************/

void init_samr_r_query_domain_info(SAMR_R_QUERY_DOMAIN_INFO * r_u,
				   uint16 switch_value, SAM_UNK_CTR * ctr,
				   NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_query_domain_info\n"));

	r_u->ptr_0 = 0;
	r_u->switch_value = 0;
	r_u->status = status;	/* return status */

	if (NT_STATUS_IS_OK(status)) {
		r_u->switch_value = switch_value;
		r_u->ptr_0 = 1;
		r_u->ctr = ctr;
	}
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_query_domain_info(const char *desc, SAMR_R_QUERY_DOMAIN_INFO * r_u,
				 prs_struct *ps, int depth)
{
        if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_query_domain_info");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_0 ", ps, depth, &r_u->ptr_0))
		return False;

	if (r_u->ptr_0 != 0 && r_u->ctr != NULL) {
		if(!prs_uint16("switch_value", ps, depth, &r_u->switch_value))
			return False;
		if(!prs_align(ps))
			return False;

		switch (r_u->switch_value) {
		case 0x0d:
			if(!sam_io_unk_info13("unk_inf13", &r_u->ctr->info.inf13, ps, depth))
				return False;
			break;
		case 0x0c:
			if(!sam_io_unk_info12("unk_inf12", &r_u->ctr->info.inf12, ps, depth))
				return False;
			break;
		case 0x09:
			if(!sam_io_unk_info9("unk_inf9",&r_u->ctr->info.inf9, ps,depth))
				return False;
			break;
		case 0x08:
			if(!sam_io_unk_info8("unk_inf8",&r_u->ctr->info.inf8, ps,depth))
				return False;
			break;
		case 0x07:
			if(!sam_io_unk_info7("unk_inf7",&r_u->ctr->info.inf7, ps,depth))
				return False;
			break;
		case 0x06:
			if(!sam_io_unk_info6("unk_inf6",&r_u->ctr->info.inf6, ps,depth))
				return False;
			break;
		case 0x05:
			if(!sam_io_unk_info5("unk_inf5",&r_u->ctr->info.inf5, ps,depth))
				return False;
			break;
		case 0x04:
			if(!sam_io_unk_info4("unk_inf4",&r_u->ctr->info.inf4, ps,depth))
				return False;
			break;
		case 0x03:
			if(!sam_io_unk_info3("unk_inf3",&r_u->ctr->info.inf3, ps,depth))
				return False;
			break;
		case 0x02:
			if(!sam_io_unk_info2("unk_inf2",&r_u->ctr->info.inf2, ps,depth))
				return False;
			break;
		case 0x01:
			if(!sam_io_unk_info1("unk_inf1",&r_u->ctr->info.inf1, ps,depth))
				return False;
			break;
		default:
			DEBUG(0, ("samr_io_r_query_domain_info: unknown switch level 0x%x\n",
				r_u->switch_value));
			r_u->status = NT_STATUS_INVALID_INFO_CLASS;
			return False;
		}
	}
	
	if(!prs_align(ps))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;
	
	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

void init_samr_q_set_sec_obj(SAMR_Q_SET_SEC_OBJ * q_u,
			     POLICY_HND *pol, uint32 sec_info, SEC_DESC_BUF *buf)
{
	DEBUG(5, ("samr_init_samr_q_set_sec_obj\n"));

	q_u->pol = *pol;
	q_u->sec_info = sec_info;
	q_u->buf = buf;
}


/*******************************************************************
reads or writes a SAMR_R_SET_SEC_OBJ structure.
********************************************************************/

BOOL samr_io_r_set_sec_obj(const char *desc, SAMR_R_SET_SEC_OBJ * r_u,
			     prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;
  
	prs_debug(ps, depth, desc, "samr_io_r_set_sec_obj");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
reads or writes a SAMR_R_QUERY_SEC_OBJ structure.
********************************************************************/

BOOL samr_io_r_query_sec_obj(const char *desc, SAMR_R_QUERY_SEC_OBJ * r_u,
			     prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;
  
	prs_debug(ps, depth, desc, "samr_io_r_query_sec_obj");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr", ps, depth, &r_u->ptr))
		return False;
	if (r_u->ptr != 0) {
		if(!sec_io_desc_buf("sec", &r_u->buf, ps, depth))
			return False;
	}

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
reads or writes a SAM_STR1 structure.
********************************************************************/

static BOOL sam_io_sam_str1(const char *desc, SAM_STR1 * sam, uint32 acct_buf,
			    uint32 name_buf, uint32 desc_buf,
			    prs_struct *ps, int depth)
{
	if (sam == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_sam_str1");
	depth++;

	if(!prs_align(ps))
		return False;
	if (!smb_io_unistr2("name", &sam->uni_acct_name, acct_buf, ps, depth))
		return False;

	if (!smb_io_unistr2("desc", &sam->uni_acct_desc, desc_buf, ps, depth))
		return False;

	if (!smb_io_unistr2("full", &sam->uni_full_name, name_buf, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a SAM_ENTRY1 structure.
********************************************************************/

static void init_sam_entry1(SAM_ENTRY1 *sam, uint32 user_idx,
			    UNISTR2 *sam_name, UNISTR2 *sam_full,
			    UNISTR2 *sam_desc, uint32 rid_user,
			    uint32 acb_info)
{
	DEBUG(5, ("init_sam_entry1\n"));

	ZERO_STRUCTP(sam);

	sam->user_idx = user_idx;
	sam->rid_user = rid_user;
	sam->acb_info = acb_info;

	init_uni_hdr(&sam->hdr_acct_name, sam_name);
	init_uni_hdr(&sam->hdr_user_name, sam_full);
	init_uni_hdr(&sam->hdr_user_desc, sam_desc);
}

/*******************************************************************
reads or writes a SAM_ENTRY1 structure.
********************************************************************/

static BOOL sam_io_sam_entry1(const char *desc, SAM_ENTRY1 * sam,
			      prs_struct *ps, int depth)
{
	if (sam == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_sam_entry1");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("user_idx ", ps, depth, &sam->user_idx))
		return False;

	if(!prs_uint32("rid_user ", ps, depth, &sam->rid_user))
		return False;
	if(!prs_uint32("acb_info ", ps, depth, &sam->acb_info))
		return False;

	if (!smb_io_unihdr("hdr_acct_name", &sam->hdr_acct_name, ps, depth))
		return False;
	if (!smb_io_unihdr("hdr_user_desc", &sam->hdr_user_desc, ps, depth))
		return False;
	if (!smb_io_unihdr("hdr_user_name", &sam->hdr_user_name, ps, depth))
		return False;

	return True;
}

/*******************************************************************
reads or writes a SAM_STR2 structure.
********************************************************************/

static BOOL sam_io_sam_str2(const char *desc, SAM_STR2 * sam, uint32 acct_buf,
			    uint32 desc_buf, prs_struct *ps, int depth)
{
	if (sam == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_sam_str2");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_unistr2("uni_srv_name", &sam->uni_srv_name, acct_buf, ps, depth)) /* account name unicode string */
		return False;
	if(!smb_io_unistr2("uni_srv_desc", &sam->uni_srv_desc, desc_buf, ps, depth))	/* account desc unicode string */
		return False;

	return True;
}

/*******************************************************************
inits a SAM_ENTRY2 structure.
********************************************************************/
static void init_sam_entry2(SAM_ENTRY2 * sam, uint32 user_idx,
			    UNISTR2 *sam_name, UNISTR2 *sam_desc,
			    uint32 rid_user, uint32 acb_info)
{
	DEBUG(5, ("init_sam_entry2\n"));

	sam->user_idx = user_idx;
	sam->rid_user = rid_user;
	sam->acb_info = acb_info;

	init_uni_hdr(&sam->hdr_srv_name, sam_name);
	init_uni_hdr(&sam->hdr_srv_desc, sam_desc);
}

/*******************************************************************
reads or writes a SAM_ENTRY2 structure.
********************************************************************/

static BOOL sam_io_sam_entry2(const char *desc, SAM_ENTRY2 * sam,
			      prs_struct *ps, int depth)
{
	if (sam == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_sam_entry2");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("user_idx ", ps, depth, &sam->user_idx))
		return False;

	if(!prs_uint32("rid_user ", ps, depth, &sam->rid_user))
		return False;
	if(!prs_uint32("acb_info ", ps, depth, &sam->acb_info))
		return False;

	if(!smb_io_unihdr("unihdr", &sam->hdr_srv_name, ps, depth))	/* account name unicode string header */
		return False;
	if(!smb_io_unihdr("unihdr", &sam->hdr_srv_desc, ps, depth))	/* account name unicode string header */
		return False;

	return True;
}

/*******************************************************************
reads or writes a SAM_STR3 structure.
********************************************************************/

static BOOL sam_io_sam_str3(const char *desc, SAM_STR3 * sam, uint32 acct_buf,
			    uint32 desc_buf, prs_struct *ps, int depth)
{
	if (sam == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_sam_str3");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_unistr2("uni_grp_name", &sam->uni_grp_name, acct_buf, ps, depth))	/* account name unicode string */
		return False;
	if(!smb_io_unistr2("uni_grp_desc", &sam->uni_grp_desc, desc_buf, ps, depth))	/* account desc unicode string */
		return False;

	return True;
}

/*******************************************************************
inits a SAM_ENTRY3 structure.
********************************************************************/

static void init_sam_entry3(SAM_ENTRY3 * sam, uint32 grp_idx,
			    UNISTR2 *grp_name, UNISTR2 *grp_desc,
			    uint32 rid_grp)
{
	DEBUG(5, ("init_sam_entry3\n"));

	sam->grp_idx = grp_idx;
	sam->rid_grp = rid_grp;
	sam->attr = 0x07;	/* group rid attributes - gets ignored by nt 4.0 */

	init_uni_hdr(&sam->hdr_grp_name, grp_name);
	init_uni_hdr(&sam->hdr_grp_desc, grp_desc);
}

/*******************************************************************
reads or writes a SAM_ENTRY3 structure.
********************************************************************/

static BOOL sam_io_sam_entry3(const char *desc, SAM_ENTRY3 * sam,
			      prs_struct *ps, int depth)
{
	if (sam == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_sam_entry3");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("grp_idx", ps, depth, &sam->grp_idx))
		return False;

	if(!prs_uint32("rid_grp", ps, depth, &sam->rid_grp))
		return False;
	if(!prs_uint32("attr   ", ps, depth, &sam->attr))
		return False;

	if(!smb_io_unihdr("unihdr", &sam->hdr_grp_name, ps, depth))	/* account name unicode string header */
		return False;
	if(!smb_io_unihdr("unihdr", &sam->hdr_grp_desc, ps, depth))	/* account name unicode string header */
		return False;

	return True;
}

/*******************************************************************
inits a SAM_ENTRY4 structure.
********************************************************************/

static void init_sam_entry4(SAM_ENTRY4 * sam, uint32 user_idx,
			    uint32 len_acct_name)
{
	DEBUG(5, ("init_sam_entry4\n"));

	sam->user_idx = user_idx;
	init_str_hdr(&sam->hdr_acct_name, len_acct_name+1, len_acct_name, len_acct_name != 0);
}

/*******************************************************************
reads or writes a SAM_ENTRY4 structure.
********************************************************************/

static BOOL sam_io_sam_entry4(const char *desc, SAM_ENTRY4 * sam,
			      prs_struct *ps, int depth)
{
	if (sam == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_sam_entry4");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("user_idx", ps, depth, &sam->user_idx))
		return False;
	if(!smb_io_strhdr("strhdr", &sam->hdr_acct_name, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a SAM_ENTRY5 structure.
********************************************************************/

static void init_sam_entry5(SAM_ENTRY5 * sam, uint32 grp_idx,
			    uint32 len_grp_name)
{
	DEBUG(5, ("init_sam_entry5\n"));

	sam->grp_idx = grp_idx;
	init_str_hdr(&sam->hdr_grp_name, len_grp_name, len_grp_name,
		     len_grp_name != 0);
}

/*******************************************************************
reads or writes a SAM_ENTRY5 structure.
********************************************************************/

static BOOL sam_io_sam_entry5(const char *desc, SAM_ENTRY5 * sam,
			      prs_struct *ps, int depth)
{
	if (sam == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_sam_entry5");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("grp_idx", ps, depth, &sam->grp_idx))
		return False;
	if(!smb_io_strhdr("strhdr", &sam->hdr_grp_name, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a SAM_ENTRY structure.
********************************************************************/

void init_sam_entry(SAM_ENTRY *sam, UNISTR2 *uni2, uint32 rid)
{
	DEBUG(10, ("init_sam_entry: %d\n", rid));

	sam->rid = rid;
	init_uni_hdr(&sam->hdr_name, uni2);
}

/*******************************************************************
reads or writes a SAM_ENTRY structure.
********************************************************************/

static BOOL sam_io_sam_entry(const char *desc, SAM_ENTRY * sam,
			     prs_struct *ps, int depth)
{
	if (sam == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_sam_entry");
	depth++;

	if(!prs_align(ps))
		return False;
	if(!prs_uint32("rid", ps, depth, &sam->rid))
		return False;
	if(!smb_io_unihdr("unihdr", &sam->hdr_name, ps, depth))	/* account name unicode string header */
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_ENUM_DOM_USERS structure.
********************************************************************/

void init_samr_q_enum_dom_users(SAMR_Q_ENUM_DOM_USERS * q_e, POLICY_HND *pol,
				uint32 start_idx,
				uint32 acb_mask, uint32 size)
{
	DEBUG(5, ("init_samr_q_enum_dom_users\n"));

	q_e->pol = *pol;

	q_e->start_idx = start_idx;	/* zero indicates lots */
	q_e->acb_mask = acb_mask;
	q_e->max_size = size;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_enum_dom_users(const char *desc, SAMR_Q_ENUM_DOM_USERS * q_e,
			      prs_struct *ps, int depth)
{
	if (q_e == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_enum_dom_users");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("domain_pol", &q_e->pol, ps, depth))
		return False;

	if(!prs_uint32("start_idx", ps, depth, &q_e->start_idx))
		return False;
	if(!prs_uint32("acb_mask ", ps, depth, &q_e->acb_mask))
		return False;

	if(!prs_uint32("max_size ", ps, depth, &q_e->max_size))
		return False;

	return True;
}


/*******************************************************************
inits a SAMR_R_ENUM_DOM_USERS structure.
********************************************************************/

void init_samr_r_enum_dom_users(SAMR_R_ENUM_DOM_USERS * r_u,
				uint32 next_idx, uint32 num_sam_entries)
{
	DEBUG(5, ("init_samr_r_enum_dom_users\n"));

	r_u->next_idx = next_idx;

	if (num_sam_entries != 0) {
		r_u->ptr_entries1 = 1;
		r_u->ptr_entries2 = 1;
		r_u->num_entries2 = num_sam_entries;
		r_u->num_entries3 = num_sam_entries;

		r_u->num_entries4 = num_sam_entries;
	} else {
		r_u->ptr_entries1 = 0;
		r_u->num_entries2 = num_sam_entries;
		r_u->ptr_entries2 = 1;
	}
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_enum_dom_users(const char *desc, SAMR_R_ENUM_DOM_USERS * r_u,
			      prs_struct *ps, int depth)
{
	uint32 i;

	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_enum_dom_users");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("next_idx    ", ps, depth, &r_u->next_idx))
		return False;
	if(!prs_uint32("ptr_entries1", ps, depth, &r_u->ptr_entries1))
		return False;

	if (r_u->ptr_entries1 != 0) {
		if(!prs_uint32("num_entries2", ps, depth, &r_u->num_entries2))
			return False;
		if(!prs_uint32("ptr_entries2", ps, depth, &r_u->ptr_entries2))
			return False;
		if(!prs_uint32("num_entries3", ps, depth, &r_u->num_entries3))
			return False;

		if (UNMARSHALLING(ps) && (r_u->num_entries2 != 0)) {
			r_u->sam = PRS_ALLOC_MEM(ps,SAM_ENTRY, r_u->num_entries2);
			r_u->uni_acct_name = PRS_ALLOC_MEM(ps,UNISTR2, r_u->num_entries2);
		}

		if ((r_u->sam == NULL || r_u->uni_acct_name == NULL) && r_u->num_entries2 != 0) {
			DEBUG(0,("NULL pointers in SAMR_R_ENUM_DOM_USERS\n"));
			r_u->num_entries4 = 0;
			r_u->status = NT_STATUS_MEMORY_NOT_ALLOCATED;
			return False;
		}

		for (i = 0; i < r_u->num_entries2; i++) {
			if(!sam_io_sam_entry("", &r_u->sam[i], ps, depth))
				return False;
		}

		for (i = 0; i < r_u->num_entries2; i++) {
			if(!smb_io_unistr2("", &r_u->uni_acct_name[i],r_u->sam[i].hdr_name.buffer, ps,depth))
				return False;
		}

	}

	if(!prs_align(ps))
		return False;
		
	if(!prs_uint32("num_entries4", ps, depth, &r_u->num_entries4))
		return False;
	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_QUERY_DISPINFO structure.
********************************************************************/

void init_samr_q_query_dispinfo(SAMR_Q_QUERY_DISPINFO * q_e, POLICY_HND *pol,
				uint16 switch_level, uint32 start_idx,
				uint32 max_entries, uint32 max_size)
{
	DEBUG(5, ("init_samr_q_query_dispinfo\n"));

	q_e->domain_pol = *pol;

	q_e->switch_level = switch_level;

	q_e->start_idx = start_idx;
	q_e->max_entries = max_entries;
	q_e->max_size = max_size;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_query_dispinfo(const char *desc, SAMR_Q_QUERY_DISPINFO * q_e,
			      prs_struct *ps, int depth)
{
	if (q_e == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_query_dispinfo");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("domain_pol", &q_e->domain_pol, ps, depth))
		return False;

	if(!prs_uint16("switch_level", ps, depth, &q_e->switch_level))
		return False;
	if(!prs_align(ps))
		return False;

	if(!prs_uint32("start_idx   ", ps, depth, &q_e->start_idx))
		return False;
	if(!prs_uint32("max_entries ", ps, depth, &q_e->max_entries))
		return False;
	if(!prs_uint32("max_size    ", ps, depth, &q_e->max_size))
		return False;

	return True;
}

/*******************************************************************
inits a SAM_DISPINFO_1 structure.
********************************************************************/

NTSTATUS init_sam_dispinfo_1(TALLOC_CTX *ctx, SAM_DISPINFO_1 **sam,
			     uint32 num_entries, uint32 start_idx,
			     struct samr_displayentry *entries)
{
	uint32 i;

	DEBUG(10, ("init_sam_dispinfo_1: num_entries: %d\n", num_entries));

	if (num_entries==0)
		return NT_STATUS_OK;

	*sam = TALLOC_ZERO_ARRAY(ctx, SAM_DISPINFO_1, num_entries);
	if (*sam == NULL)
		return NT_STATUS_NO_MEMORY;

	(*sam)->sam=TALLOC_ARRAY(ctx, SAM_ENTRY1, num_entries);
	if ((*sam)->sam == NULL)
		return NT_STATUS_NO_MEMORY;

	(*sam)->str=TALLOC_ARRAY(ctx, SAM_STR1, num_entries);
	if ((*sam)->str == NULL)
		return NT_STATUS_NO_MEMORY;

	for (i = 0; i < num_entries ; i++) {
		init_unistr2(&(*sam)->str[i].uni_acct_name,
			     entries[i].account_name, UNI_FLAGS_NONE);
		init_unistr2(&(*sam)->str[i].uni_full_name,
			     entries[i].fullname, UNI_FLAGS_NONE);
		init_unistr2(&(*sam)->str[i].uni_acct_desc,
			     entries[i].description, UNI_FLAGS_NONE);

		init_sam_entry1(&(*sam)->sam[i], start_idx+i+1,
				&(*sam)->str[i].uni_acct_name,
				&(*sam)->str[i].uni_full_name,
				&(*sam)->str[i].uni_acct_desc,
				entries[i].rid, entries[i].acct_flags);
	}

	return NT_STATUS_OK;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_sam_dispinfo_1(const char *desc, SAM_DISPINFO_1 * sam,
				  uint32 num_entries,
				  prs_struct *ps, int depth)
{
	uint32 i;

	prs_debug(ps, depth, desc, "sam_io_sam_dispinfo_1");
	depth++;

	if(!prs_align(ps))
		return False;

	if (UNMARSHALLING(ps) && num_entries > 0) {

		if ((sam->sam = PRS_ALLOC_MEM(ps, SAM_ENTRY1, num_entries)) == NULL) {
			DEBUG(0, ("out of memory allocating SAM_ENTRY1\n"));
			return False;
		}

		if ((sam->str = PRS_ALLOC_MEM(ps, SAM_STR1, num_entries)) == NULL) {
			DEBUG(0, ("out of memory allocating SAM_STR1\n"));
			return False;
		}
	}

	for (i = 0; i < num_entries; i++) {
		if(!sam_io_sam_entry1("", &sam->sam[i], ps, depth))
			return False;
	}

	for (i = 0; i < num_entries; i++) {
		if(!sam_io_sam_str1("", &sam->str[i],
			      sam->sam[i].hdr_acct_name.buffer,
			      sam->sam[i].hdr_user_name.buffer,
			      sam->sam[i].hdr_user_desc.buffer, ps, depth))
			return False;
	}

	return True;
}

/*******************************************************************
inits a SAM_DISPINFO_2 structure.
********************************************************************/

NTSTATUS init_sam_dispinfo_2(TALLOC_CTX *ctx, SAM_DISPINFO_2 **sam,
			     uint32 num_entries, uint32 start_idx,
			     struct samr_displayentry *entries)
{
	uint32 i;

	DEBUG(10, ("init_sam_dispinfo_2: num_entries: %d\n", num_entries));

	if (num_entries==0)
		return NT_STATUS_OK;

	*sam = TALLOC_ZERO_ARRAY(ctx, SAM_DISPINFO_2, num_entries);
	if (*sam == NULL)
		return NT_STATUS_NO_MEMORY;

	(*sam)->sam = TALLOC_ARRAY(ctx, SAM_ENTRY2, num_entries);
	if ((*sam)->sam == NULL)
		return NT_STATUS_NO_MEMORY;

	(*sam)->str=TALLOC_ARRAY(ctx, SAM_STR2, num_entries);
	if ((*sam)->str == NULL)
		return NT_STATUS_NO_MEMORY;

	for (i = 0; i < num_entries; i++) {
		init_unistr2(&(*sam)->str[i].uni_srv_name,
			     entries[i].account_name, UNI_FLAGS_NONE);
		init_unistr2(&(*sam)->str[i].uni_srv_desc,
			     entries[i].description, UNI_FLAGS_NONE);

		init_sam_entry2(&(*sam)->sam[i], start_idx + i + 1,
				&(*sam)->str[i].uni_srv_name,
				&(*sam)->str[i].uni_srv_desc,
				entries[i].rid, entries[i].acct_flags);
	}

	return NT_STATUS_OK;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_sam_dispinfo_2(const char *desc, SAM_DISPINFO_2 * sam,
				  uint32 num_entries,
				  prs_struct *ps, int depth)
{
	uint32 i;

	if (sam == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_sam_dispinfo_2");
	depth++;

	if(!prs_align(ps))
		return False;

	if (UNMARSHALLING(ps) && num_entries > 0) {

		if ((sam->sam = PRS_ALLOC_MEM(ps, SAM_ENTRY2, num_entries)) == NULL) {
			DEBUG(0, ("out of memory allocating SAM_ENTRY2\n"));
			return False;
		}

		if ((sam->str = PRS_ALLOC_MEM(ps, SAM_STR2, num_entries)) == NULL) {
			DEBUG(0, ("out of memory allocating SAM_STR2\n"));
			return False;
		}
	}

	for (i = 0; i < num_entries; i++) {
		if(!sam_io_sam_entry2("", &sam->sam[i], ps, depth))
			return False;
	}

	for (i = 0; i < num_entries; i++) {
		if(!sam_io_sam_str2("", &sam->str[i],
			      sam->sam[i].hdr_srv_name.buffer,
			      sam->sam[i].hdr_srv_desc.buffer, ps, depth))
			return False;
	}

	return True;
}

/*******************************************************************
inits a SAM_DISPINFO_3 structure.
********************************************************************/

NTSTATUS init_sam_dispinfo_3(TALLOC_CTX *ctx, SAM_DISPINFO_3 **sam,
			     uint32 num_entries, uint32 start_idx,
			     struct samr_displayentry *entries)
{
	uint32 i;

	DEBUG(5, ("init_sam_dispinfo_3: num_entries: %d\n", num_entries));

	if (num_entries==0)
		return NT_STATUS_OK;

	*sam = TALLOC_ZERO_ARRAY(ctx, SAM_DISPINFO_3, num_entries);
	if (*sam == NULL)
		return NT_STATUS_NO_MEMORY;

	if (!((*sam)->sam=TALLOC_ARRAY(ctx, SAM_ENTRY3, num_entries)))
		return NT_STATUS_NO_MEMORY;

	if (!((*sam)->str=TALLOC_ARRAY(ctx, SAM_STR3, num_entries)))
		return NT_STATUS_NO_MEMORY;

	for (i = 0; i < num_entries; i++) {
		DEBUG(11, ("init_sam_dispinfo_3: entry: %d\n",i));

		init_unistr2(&(*sam)->str[i].uni_grp_name,
			     entries[i].account_name, UNI_FLAGS_NONE);
		init_unistr2(&(*sam)->str[i].uni_grp_desc,
			     entries[i].description, UNI_FLAGS_NONE);

		init_sam_entry3(&(*sam)->sam[i], start_idx+i+1,
				&(*sam)->str[i].uni_grp_name,
				&(*sam)->str[i].uni_grp_desc,
				entries[i].rid);
	}

	return NT_STATUS_OK;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_sam_dispinfo_3(const char *desc, SAM_DISPINFO_3 * sam,
				  uint32 num_entries,
				  prs_struct *ps, int depth)
{
	uint32 i;

	if (sam == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_sam_dispinfo_3");
	depth++;

	if(!prs_align(ps))
		return False;

	if (UNMARSHALLING(ps) && num_entries > 0) {

		if ((sam->sam = PRS_ALLOC_MEM(ps, SAM_ENTRY3, num_entries)) == NULL) {
			DEBUG(0, ("out of memory allocating SAM_ENTRY3\n"));
			return False;
		}

		if ((sam->str = PRS_ALLOC_MEM(ps, SAM_STR3, num_entries)) == NULL) {
			DEBUG(0, ("out of memory allocating SAM_STR3\n"));
			return False;
		}
	}

	for (i = 0; i < num_entries; i++) {
		if(!sam_io_sam_entry3("", &sam->sam[i], ps, depth))
			return False;
	}

	for (i = 0; i < num_entries; i++) {
		if(!sam_io_sam_str3("", &sam->str[i],
			      sam->sam[i].hdr_grp_name.buffer,
			      sam->sam[i].hdr_grp_desc.buffer, ps, depth))
			return False;
	}

	return True;
}

/*******************************************************************
inits a SAM_DISPINFO_4 structure.
********************************************************************/

NTSTATUS init_sam_dispinfo_4(TALLOC_CTX *ctx, SAM_DISPINFO_4 **sam,
			     uint32 num_entries, uint32 start_idx,
			     struct samr_displayentry *entries)
{
	uint32 i;

	DEBUG(5, ("init_sam_dispinfo_4: num_entries: %d\n", num_entries));

	if (num_entries==0)
		return NT_STATUS_OK;

	*sam = TALLOC_ZERO_ARRAY(ctx, SAM_DISPINFO_4, num_entries);
	if (*sam == NULL)
		return NT_STATUS_NO_MEMORY;

	(*sam)->sam = TALLOC_ARRAY(ctx, SAM_ENTRY4, num_entries);
	if ((*sam)->sam == NULL)
		return NT_STATUS_NO_MEMORY;

	(*sam)->str=TALLOC_ARRAY(ctx, SAM_STR4, num_entries);
	if ((*sam)->str == NULL)
		return NT_STATUS_NO_MEMORY;

	for (i = 0; i < num_entries; i++) {
		size_t len_sam_name = strlen(entries[i].account_name);

		DEBUG(11, ("init_sam_dispinfo_2: entry: %d\n",i));
	  
		init_sam_entry4(&(*sam)->sam[i], start_idx + i + 1,
				len_sam_name);

		init_string2(&(*sam)->str[i].acct_name,
			     entries[i].account_name, len_sam_name+1,
			     len_sam_name);
	}
	
	return NT_STATUS_OK;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_sam_dispinfo_4(const char *desc, SAM_DISPINFO_4 * sam,
				  uint32 num_entries,
				  prs_struct *ps, int depth)
{
	uint32 i;

	if (sam == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_sam_dispinfo_4");
	depth++;

	if(!prs_align(ps))
		return False;

	if (UNMARSHALLING(ps) && num_entries > 0) {

		if ((sam->sam = PRS_ALLOC_MEM(ps, SAM_ENTRY4, num_entries)) == NULL) {
			DEBUG(0, ("out of memory allocating SAM_ENTRY4\n"));
			return False;
		}

		if ((sam->str = PRS_ALLOC_MEM(ps, SAM_STR4, num_entries)) == NULL) {
			DEBUG(0, ("out of memory allocating SAM_STR4\n"));
			return False;
		}
	}

	for (i = 0; i < num_entries; i++) {
		if(!sam_io_sam_entry4("", &sam->sam[i], ps, depth))
			return False;
	}

	for (i = 0; i < num_entries; i++) {
		if(!smb_io_string2("acct_name", &sam->str[i].acct_name,
			     sam->sam[i].hdr_acct_name.buffer, ps, depth))
			return False;
	}

	return True;
}

/*******************************************************************
inits a SAM_DISPINFO_5 structure.
********************************************************************/

NTSTATUS init_sam_dispinfo_5(TALLOC_CTX *ctx, SAM_DISPINFO_5 **sam,
			     uint32 num_entries, uint32 start_idx,
			     struct samr_displayentry *entries)
{
	uint32 len_sam_name;
	uint32 i;

	DEBUG(5, ("init_sam_dispinfo_5: num_entries: %d\n", num_entries));

	if (num_entries==0)
		return NT_STATUS_OK;

	*sam = TALLOC_ZERO_ARRAY(ctx, SAM_DISPINFO_5, num_entries);
	if (*sam == NULL)
		return NT_STATUS_NO_MEMORY;

	if (!((*sam)->sam=TALLOC_ARRAY(ctx, SAM_ENTRY5, num_entries)))
		return NT_STATUS_NO_MEMORY;

	if (!((*sam)->str=TALLOC_ARRAY(ctx, SAM_STR5, num_entries)))
		return NT_STATUS_NO_MEMORY;

	for (i = 0; i < num_entries; i++) {
		DEBUG(11, ("init_sam_dispinfo_5: entry: %d\n",i));

		len_sam_name = strlen(entries[i].account_name);
	  
		init_sam_entry5(&(*sam)->sam[i], start_idx+i+1, len_sam_name);
		init_string2(&(*sam)->str[i].grp_name, entries[i].account_name,
			     len_sam_name+1, len_sam_name);
	}

	return NT_STATUS_OK;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_sam_dispinfo_5(const char *desc, SAM_DISPINFO_5 * sam,
				  uint32 num_entries,
				  prs_struct *ps, int depth)
{
	uint32 i;

	if (sam == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_sam_dispinfo_5");
	depth++;

	if(!prs_align(ps))
		return False;

	if (UNMARSHALLING(ps) && num_entries > 0) {

		if ((sam->sam = PRS_ALLOC_MEM(ps, SAM_ENTRY5, num_entries)) == NULL) {
			DEBUG(0, ("out of memory allocating SAM_ENTRY5\n"));
			return False;
		}

		if ((sam->str = PRS_ALLOC_MEM(ps, SAM_STR5, num_entries)) == NULL) {
			DEBUG(0, ("out of memory allocating SAM_STR5\n"));
			return False;
		}
	}

	for (i = 0; i < num_entries; i++) {
		if(!sam_io_sam_entry5("", &sam->sam[i], ps, depth))
			return False;
	}

	for (i = 0; i < num_entries; i++) {
		if(!smb_io_string2("grp_name", &sam->str[i].grp_name,
			     sam->sam[i].hdr_grp_name.buffer, ps, depth))
			return False;
	}

	return True;
}

/*******************************************************************
inits a SAMR_R_QUERY_DISPINFO structure.
********************************************************************/

void init_samr_r_query_dispinfo(SAMR_R_QUERY_DISPINFO * r_u,
				uint32 num_entries, uint32 total_size, uint32 data_size,
				uint16 switch_level, SAM_DISPINFO_CTR * ctr,
				NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_query_dispinfo: level %d\n", switch_level));

	r_u->total_size = total_size;

	r_u->data_size = data_size;

	r_u->switch_level = switch_level;
	r_u->num_entries = num_entries;

	if (num_entries==0)
		r_u->ptr_entries = 0;
	else
		r_u->ptr_entries = 1;

	r_u->num_entries2 = num_entries;
	r_u->ctr = ctr;

	r_u->status = status;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_query_dispinfo(const char *desc, SAMR_R_QUERY_DISPINFO * r_u,
			      prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_query_dispinfo");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("total_size  ", ps, depth, &r_u->total_size))
		return False;
	if(!prs_uint32("data_size   ", ps, depth, &r_u->data_size))
		return False;
	if(!prs_uint16("switch_level", ps, depth, &r_u->switch_level))
		return False;
	if(!prs_align(ps))
		return False;

	if(!prs_uint32("num_entries ", ps, depth, &r_u->num_entries))
		return False;
	if(!prs_uint32("ptr_entries ", ps, depth, &r_u->ptr_entries))
		return False;

	if (r_u->ptr_entries==0) {
		if(!prs_align(ps))
			return False;
		if(!prs_ntstatus("status", ps, depth, &r_u->status))
			return False;

		return True;
	}

	if(!prs_uint32("num_entries2", ps, depth, &r_u->num_entries2))
		return False;

	switch (r_u->switch_level) {
	case 0x1:
		if(!sam_io_sam_dispinfo_1("users", r_u->ctr->sam.info1,
				r_u->num_entries, ps, depth))
			return False;
		break;
	case 0x2:
		if(!sam_io_sam_dispinfo_2("servers", r_u->ctr->sam.info2,
				r_u->num_entries, ps, depth))
			return False;
		break;
	case 0x3:
		if(!sam_io_sam_dispinfo_3("groups", r_u->ctr->sam.info3,
				    r_u->num_entries, ps, depth))
			return False;
		break;
	case 0x4:
		if(!sam_io_sam_dispinfo_4("user list",
				    r_u->ctr->sam.info4,
				    r_u->num_entries, ps, depth))
			return False;
		break;
	case 0x5:
		if(!sam_io_sam_dispinfo_5("group list",
				    r_u->ctr->sam.info5,
				    r_u->num_entries, ps, depth))
			return False;
		break;
	default:
		DEBUG(0,("samr_io_r_query_dispinfo: unknown switch value\n"));
		break;
	}
	
	if(!prs_align(ps))
		return False;
	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_OPEN_GROUP structure.
********************************************************************/

void init_samr_q_open_group(SAMR_Q_OPEN_GROUP * q_c,
			    POLICY_HND *hnd,
			    uint32 access_mask, uint32 rid)
{
	DEBUG(5, ("init_samr_q_open_group\n"));

	q_c->domain_pol = *hnd;
	q_c->access_mask = access_mask;
	q_c->rid_group = rid;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_open_group(const char *desc, SAMR_Q_OPEN_GROUP * q_u,
			  prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_open_group");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("domain_pol", &q_u->domain_pol, ps, depth))
		return False;

	if(!prs_uint32("access_mask", ps, depth, &q_u->access_mask))
		return False;
	if(!prs_uint32("rid_group", ps, depth, &q_u->rid_group))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_open_group(const char *desc, SAMR_R_OPEN_GROUP * r_u,
			  prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_open_group");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &r_u->pol, ps, depth))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a GROUP_INFO1 structure.
********************************************************************/

void init_samr_group_info1(GROUP_INFO1 * gr1,
			   char *acct_name, char *acct_desc,
			   uint32 num_members)
{
	DEBUG(5, ("init_samr_group_info1\n"));

	gr1->group_attr = (SE_GROUP_MANDATORY|SE_GROUP_ENABLED_BY_DEFAULT); /* why not | SE_GROUP_ENABLED ? */
	gr1->num_members = num_members;

	init_unistr2(&gr1->uni_acct_name, acct_name, UNI_FLAGS_NONE);
	init_uni_hdr(&gr1->hdr_acct_name, &gr1->uni_acct_name);
	init_unistr2(&gr1->uni_acct_desc, acct_desc, UNI_FLAGS_NONE);
	init_uni_hdr(&gr1->hdr_acct_desc, &gr1->uni_acct_desc);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_group_info1(const char *desc, GROUP_INFO1 * gr1,
			 prs_struct *ps, int depth)
{
	uint16 dummy = 1;

	if (gr1 == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_group_info1");
	depth++;

	if(!prs_uint16("level", ps, depth, &dummy))
		return False;

	if(!prs_align(ps))
		return False;

	if(!smb_io_unihdr("hdr_acct_name", &gr1->hdr_acct_name, ps, depth))
		return False;

	if(!prs_uint32("group_attr", ps, depth, &gr1->group_attr))
		return False;
	if(!prs_uint32("num_members", ps, depth, &gr1->num_members))
		return False;

	if(!smb_io_unihdr("hdr_acct_desc", &gr1->hdr_acct_desc, ps, depth))
		return False;

	if(!smb_io_unistr2("uni_acct_name", &gr1->uni_acct_name,
			   gr1->hdr_acct_name.buffer, ps, depth))
		return False;

	if(!smb_io_unistr2("uni_acct_desc", &gr1->uni_acct_desc,
			   gr1->hdr_acct_desc.buffer, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a GROUP_INFO2 structure.
********************************************************************/

void init_samr_group_info2(GROUP_INFO2 * gr2, const char *acct_name)
{
	DEBUG(5, ("init_samr_group_info2\n"));

	gr2->level = 2;
	init_unistr2(&gr2->uni_acct_name, acct_name, UNI_FLAGS_NONE);
	init_uni_hdr(&gr2->hdr_acct_name, &gr2->uni_acct_name);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_group_info2(const char *desc, GROUP_INFO2 *gr2, prs_struct *ps, int depth)
{
	if (gr2 == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_group_info2");
	depth++;

	if(!prs_uint16("hdr_level", ps, depth, &gr2->level))
		return False;

	if(!smb_io_unihdr("hdr_acct_name", &gr2->hdr_acct_name, ps, depth))
		return False;
	if(!smb_io_unistr2("uni_acct_name", &gr2->uni_acct_name,
			   gr2->hdr_acct_name.buffer, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a GROUP_INFO3 structure.
********************************************************************/

void init_samr_group_info3(GROUP_INFO3 *gr3)
{
	DEBUG(5, ("init_samr_group_info3\n"));

	gr3->group_attr = (SE_GROUP_MANDATORY|SE_GROUP_ENABLED_BY_DEFAULT); /* why not | SE_GROUP_ENABLED ? */
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_group_info3(const char *desc, GROUP_INFO3 *gr3, prs_struct *ps, int depth)
{
	if (gr3 == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_group_info3");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("group_attr", ps, depth, &gr3->group_attr))
		return False;

	return True;
}

/*******************************************************************
inits a GROUP_INFO4 structure.
********************************************************************/

void init_samr_group_info4(GROUP_INFO4 * gr4, const char *acct_desc)
{
	DEBUG(5, ("init_samr_group_info4\n"));

	gr4->level = 4;
	init_unistr2(&gr4->uni_acct_desc, acct_desc, UNI_FLAGS_NONE);
	init_uni_hdr(&gr4->hdr_acct_desc, &gr4->uni_acct_desc);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_group_info4(const char *desc, GROUP_INFO4 * gr4,
			 prs_struct *ps, int depth)
{
	if (gr4 == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_group_info4");
	depth++;

	if(!prs_uint16("hdr_level", ps, depth, &gr4->level))
		return False;
	if(!smb_io_unihdr("hdr_acct_desc", &gr4->hdr_acct_desc, ps, depth))
		return False;
	if(!smb_io_unistr2("uni_acct_desc", &gr4->uni_acct_desc,
			   gr4->hdr_acct_desc.buffer, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a GROUP_INFO5 structure.
********************************************************************/

void init_samr_group_info5(GROUP_INFO5 * gr5,
			   char *acct_name, char *acct_desc,
			   uint32 num_members)
{
	DEBUG(5, ("init_samr_group_info5\n"));

	gr5->group_attr = (SE_GROUP_MANDATORY|SE_GROUP_ENABLED_BY_DEFAULT); /* why not | SE_GROUP_ENABLED ? */
	gr5->num_members = num_members;

	init_unistr2(&gr5->uni_acct_name, acct_name, UNI_FLAGS_NONE);
	init_uni_hdr(&gr5->hdr_acct_name, &gr5->uni_acct_name);
	init_unistr2(&gr5->uni_acct_desc, acct_desc, UNI_FLAGS_NONE);
	init_uni_hdr(&gr5->hdr_acct_desc, &gr5->uni_acct_desc);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_group_info5(const char *desc, GROUP_INFO5 * gr5,
			 prs_struct *ps, int depth)
{
	uint16 dummy = 1;

	if (gr5 == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_group_info5");
	depth++;

	if(!prs_uint16("level", ps, depth, &dummy))
		return False;

	if(!prs_align(ps))
		return False;

	if(!smb_io_unihdr("hdr_acct_name", &gr5->hdr_acct_name, ps, depth))
		return False;

	if(!prs_uint32("group_attr", ps, depth, &gr5->group_attr))
		return False;
	if(!prs_uint32("num_members", ps, depth, &gr5->num_members))
		return False;

	if(!smb_io_unihdr("hdr_acct_desc", &gr5->hdr_acct_desc, ps, depth))
		return False;

	if(!smb_io_unistr2("uni_acct_name", &gr5->uni_acct_name,
			   gr5->hdr_acct_name.buffer, ps, depth))
		return False;

	if(!smb_io_unistr2("uni_acct_desc", &gr5->uni_acct_desc,
			   gr5->hdr_acct_desc.buffer, ps, depth))
		return False;

	return True;
}


/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL samr_group_info_ctr(const char *desc, GROUP_INFO_CTR **ctr,
				prs_struct *ps, int depth)
{
	if (UNMARSHALLING(ps))
		*ctr = PRS_ALLOC_MEM(ps,GROUP_INFO_CTR,1);

	if (*ctr == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_group_info_ctr");
	depth++;

	if(!prs_uint16("switch_value1", ps, depth, &(*ctr)->switch_value1))
		return False;

	switch ((*ctr)->switch_value1) {
	case 1:
		if(!samr_io_group_info1("group_info1", &(*ctr)->group.info1, ps, depth))
			return False;
		break;
	case 2:
		if(!samr_io_group_info2("group_info2", &(*ctr)->group.info2, ps, depth))
			return False;
		break;
	case 3:
		if(!samr_io_group_info3("group_info3", &(*ctr)->group.info3, ps, depth))
			return False;
		break;
	case 4:
		if(!samr_io_group_info4("group_info4", &(*ctr)->group.info4, ps, depth))
			return False;
		break;
	case 5:
		if(!samr_io_group_info5("group_info5", &(*ctr)->group.info5, ps, depth))
			return False;
		break;
	default:
		DEBUG(0,("samr_group_info_ctr: unsupported switch level\n"));
		break;
	}

	return True;
}

/*******************************************************************
inits a SAMR_Q_CREATE_DOM_GROUP structure.
********************************************************************/

void init_samr_q_create_dom_group(SAMR_Q_CREATE_DOM_GROUP * q_e,
				  POLICY_HND *pol, const char *acct_desc,
				  uint32 access_mask)
{
	DEBUG(5, ("init_samr_q_create_dom_group\n"));

	q_e->pol = *pol;

	init_unistr2(&q_e->uni_acct_desc, acct_desc, UNI_FLAGS_NONE);
	init_uni_hdr(&q_e->hdr_acct_desc, &q_e->uni_acct_desc);

	q_e->access_mask = access_mask;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_create_dom_group(const char *desc, SAMR_Q_CREATE_DOM_GROUP * q_e,
				prs_struct *ps, int depth)
{
	if (q_e == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_create_dom_group");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &q_e->pol, ps, depth))
		return False;

	if(!smb_io_unihdr("hdr_acct_desc", &q_e->hdr_acct_desc, ps, depth))
		return False;
	if(!smb_io_unistr2("uni_acct_desc", &q_e->uni_acct_desc,
		       q_e->hdr_acct_desc.buffer, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;
	if(!prs_uint32("access", ps, depth, &q_e->access_mask))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_create_dom_group(const char *desc, SAMR_R_CREATE_DOM_GROUP * r_u,
				prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_create_dom_group");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &r_u->pol, ps, depth))
		return False;

	if(!prs_uint32("rid   ", ps, depth, &r_u->rid))
		return False;
	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_DELETE_DOM_GROUP structure.
********************************************************************/

void init_samr_q_delete_dom_group(SAMR_Q_DELETE_DOM_GROUP * q_c,
				  POLICY_HND *hnd)
{
	DEBUG(5, ("init_samr_q_delete_dom_group\n"));

	q_c->group_pol = *hnd;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_delete_dom_group(const char *desc, SAMR_Q_DELETE_DOM_GROUP * q_u,
				prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_delete_dom_group");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("group_pol", &q_u->group_pol, ps, depth))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_delete_dom_group(const char *desc, SAMR_R_DELETE_DOM_GROUP * r_u,
				prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_delete_dom_group");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &r_u->pol, ps, depth))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_DEL_GROUPMEM structure.
********************************************************************/

void init_samr_q_del_groupmem(SAMR_Q_DEL_GROUPMEM * q_e,
			      POLICY_HND *pol, uint32 rid)
{
	DEBUG(5, ("init_samr_q_del_groupmem\n"));

	q_e->pol = *pol;
	q_e->rid = rid;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_del_groupmem(const char *desc, SAMR_Q_DEL_GROUPMEM * q_e,
			    prs_struct *ps, int depth)
{
	if (q_e == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_del_groupmem");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &q_e->pol, ps, depth))
		return False;

	if(!prs_uint32("rid", ps, depth, &q_e->rid))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_R_DEL_GROUPMEM structure.
********************************************************************/

void init_samr_r_del_groupmem(SAMR_R_DEL_GROUPMEM * r_u, POLICY_HND *pol,
			      NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_del_groupmem\n"));

	r_u->status = status;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_del_groupmem(const char *desc, SAMR_R_DEL_GROUPMEM * r_u,
			    prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_del_groupmem");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_ADD_GROUPMEM structure.
********************************************************************/

void init_samr_q_add_groupmem(SAMR_Q_ADD_GROUPMEM * q_e,
			      POLICY_HND *pol, uint32 rid)
{
	DEBUG(5, ("init_samr_q_add_groupmem\n"));

	q_e->pol = *pol;
	q_e->rid = rid;
	q_e->unknown = 0x0005;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_add_groupmem(const char *desc, SAMR_Q_ADD_GROUPMEM * q_e,
			    prs_struct *ps, int depth)
{
	if (q_e == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_add_groupmem");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &q_e->pol, ps, depth))
		return False;

	if(!prs_uint32("rid    ", ps, depth, &q_e->rid))
		return False;
	if(!prs_uint32("unknown", ps, depth, &q_e->unknown))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_R_ADD_GROUPMEM structure.
********************************************************************/

void init_samr_r_add_groupmem(SAMR_R_ADD_GROUPMEM * r_u, POLICY_HND *pol,
			      NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_add_groupmem\n"));

	r_u->status = status;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_add_groupmem(const char *desc, SAMR_R_ADD_GROUPMEM * r_u,
			    prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_add_groupmem");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_SET_GROUPINFO structure.
********************************************************************/

void init_samr_q_set_groupinfo(SAMR_Q_SET_GROUPINFO * q_e,
			       POLICY_HND *pol, GROUP_INFO_CTR * ctr)
{
	DEBUG(5, ("init_samr_q_set_groupinfo\n"));

	q_e->pol = *pol;
	q_e->ctr = ctr;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_set_groupinfo(const char *desc, SAMR_Q_SET_GROUPINFO * q_e,
			     prs_struct *ps, int depth)
{
	if (q_e == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_set_groupinfo");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &q_e->pol, ps, depth))
		return False;
	
	if(!samr_group_info_ctr("ctr", &q_e->ctr, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_R_SET_GROUPINFO structure.
********************************************************************/

void init_samr_r_set_groupinfo(SAMR_R_SET_GROUPINFO * r_u, NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_set_groupinfo\n"));

	r_u->status = status;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_set_groupinfo(const char *desc, SAMR_R_SET_GROUPINFO * r_u,
			     prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_set_groupinfo");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_QUERY_GROUPINFO structure.
********************************************************************/

void init_samr_q_query_groupinfo(SAMR_Q_QUERY_GROUPINFO * q_e,
				 POLICY_HND *pol, uint16 switch_level)
{
	DEBUG(5, ("init_samr_q_query_groupinfo\n"));

	q_e->pol = *pol;

	q_e->switch_level = switch_level;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_query_groupinfo(const char *desc, SAMR_Q_QUERY_GROUPINFO * q_e,
			       prs_struct *ps, int depth)
{
	if (q_e == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_query_groupinfo");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &q_e->pol, ps, depth))
		return False;

	if(!prs_uint16("switch_level", ps, depth, &q_e->switch_level))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_R_QUERY_GROUPINFO structure.
********************************************************************/

void init_samr_r_query_groupinfo(SAMR_R_QUERY_GROUPINFO * r_u,
				 GROUP_INFO_CTR * ctr, NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_query_groupinfo\n"));

	r_u->ptr = (NT_STATUS_IS_OK(status) && ctr != NULL) ? 1 : 0;
	r_u->ctr = ctr;
	r_u->status = status;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_query_groupinfo(const char *desc, SAMR_R_QUERY_GROUPINFO * r_u,
			       prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_query_groupinfo");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr", ps, depth, &r_u->ptr))
		return False;

	if (r_u->ptr != 0) {
		if(!samr_group_info_ctr("ctr", &r_u->ctr, ps, depth))
			return False;
	}

	if(!prs_align(ps))
		return False;
	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_QUERY_GROUPMEM structure.
********************************************************************/

void init_samr_q_query_groupmem(SAMR_Q_QUERY_GROUPMEM * q_c, POLICY_HND *hnd)
{
	DEBUG(5, ("init_samr_q_query_groupmem\n"));

	q_c->group_pol = *hnd;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_query_groupmem(const char *desc, SAMR_Q_QUERY_GROUPMEM * q_u,
			      prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_query_groupmem");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("group_pol", &q_u->group_pol, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_R_QUERY_GROUPMEM structure.
********************************************************************/

void init_samr_r_query_groupmem(SAMR_R_QUERY_GROUPMEM * r_u,
				uint32 num_entries, uint32 *rid,
				uint32 *attr, NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_query_groupmem\n"));

	if (NT_STATUS_IS_OK(status)) {
		r_u->ptr = 1;
		r_u->num_entries = num_entries;

		r_u->ptr_attrs = attr != NULL ? 1 : 0;
		r_u->ptr_rids = rid != NULL ? 1 : 0;

		r_u->num_rids = num_entries;
		r_u->rid = rid;

		r_u->num_attrs = num_entries;
		r_u->attr = attr;
	} else {
		r_u->ptr = 0;
		r_u->num_entries = 0;
	}

	r_u->status = status;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_query_groupmem(const char *desc, SAMR_R_QUERY_GROUPMEM * r_u,
			      prs_struct *ps, int depth)
{
	uint32 i;

	if (r_u == NULL)
		return False;

	if (UNMARSHALLING(ps))
		ZERO_STRUCTP(r_u);

	prs_debug(ps, depth, desc, "samr_io_r_query_groupmem");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr", ps, depth, &r_u->ptr))
		return False;
	if(!prs_uint32("num_entries ", ps, depth, &r_u->num_entries))
		return False;

	if (r_u->ptr != 0) {
		if(!prs_uint32("ptr_rids ", ps, depth, &r_u->ptr_rids))
			return False;
		if(!prs_uint32("ptr_attrs", ps, depth, &r_u->ptr_attrs))
			return False;

		if (r_u->ptr_rids != 0)	{
			if(!prs_uint32("num_rids", ps, depth, &r_u->num_rids))
				return False;
			if (UNMARSHALLING(ps) && r_u->num_rids != 0) {
				r_u->rid = PRS_ALLOC_MEM(ps,uint32,r_u->num_rids);
				if (r_u->rid == NULL)
					return False;
			}

			for (i = 0; i < r_u->num_rids; i++) {
				if(!prs_uint32("", ps, depth, &r_u->rid[i]))
					return False;
			}
		}

		if (r_u->ptr_attrs != 0) {
			if(!prs_uint32("num_attrs", ps, depth, &r_u->num_attrs))
				return False;

			if (UNMARSHALLING(ps) && r_u->num_attrs != 0) {
				r_u->attr = PRS_ALLOC_MEM(ps,uint32,r_u->num_attrs);
				if (r_u->attr == NULL)
					return False;
			}

			for (i = 0; i < r_u->num_attrs; i++) {
				if(!prs_uint32("", ps, depth, &r_u->attr[i]))
					return False;
			}
		}
	}

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_QUERY_USERGROUPS structure.
********************************************************************/

void init_samr_q_query_usergroups(SAMR_Q_QUERY_USERGROUPS * q_u,
				  POLICY_HND *hnd)
{
	DEBUG(5, ("init_samr_q_query_usergroups\n"));

	q_u->pol = *hnd;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_query_usergroups(const char *desc, SAMR_Q_QUERY_USERGROUPS * q_u,
				prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_query_usergroups");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &q_u->pol, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_R_QUERY_USERGROUPS structure.
********************************************************************/

void init_samr_r_query_usergroups(SAMR_R_QUERY_USERGROUPS * r_u,
				  uint32 num_gids, DOM_GID * gid,
				  NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_query_usergroups\n"));

	if (NT_STATUS_IS_OK(status)) {
		r_u->ptr_0 = 1;
		r_u->num_entries = num_gids;
		r_u->ptr_1 = (num_gids != 0) ? 1 : 0;
		r_u->num_entries2 = num_gids;

		r_u->gid = gid;
	} else {
		r_u->ptr_0 = 0;
		r_u->num_entries = 0;
		r_u->ptr_1 = 0;
		r_u->gid = NULL;
	}

	r_u->status = status;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_gids(const char *desc, uint32 *num_gids, DOM_GID ** gid,
		  prs_struct *ps, int depth)
{
	uint32 i;
	if (gid == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_gids");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("num_gids", ps, depth, num_gids))
		return False;

	if ((*num_gids) != 0) {
		if (UNMARSHALLING(ps)) {
			(*gid) = PRS_ALLOC_MEM(ps,DOM_GID,*num_gids);
		}

		if ((*gid) == NULL) {
			return False;
		}

		for (i = 0; i < (*num_gids); i++) {
			if(!smb_io_gid("gids", &(*gid)[i], ps, depth))
				return False;
		}
	}

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_query_usergroups(const char *desc, SAMR_R_QUERY_USERGROUPS * r_u,
				prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_query_usergroups");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_0       ", ps, depth, &r_u->ptr_0))
		return False;

	if (r_u->ptr_0 != 0) {
		if(!prs_uint32("num_entries ", ps, depth, &r_u->num_entries))
			return False;
		if(!prs_uint32("ptr_1       ", ps, depth, &r_u->ptr_1))
			return False;

		if (r_u->num_entries != 0 && r_u->ptr_1 != 0) {
			if(!samr_io_gids("gids", &r_u->num_entries2, &r_u->gid, ps, depth))
				return False;
		}
	}

	if(!prs_align(ps))
		return False;
	if(!prs_ntstatus("status", ps, depth, &r_u->status))
	  return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_ENUM_DOMAINS structure.
********************************************************************/

void init_samr_q_enum_domains(SAMR_Q_ENUM_DOMAINS * q_e,
			      POLICY_HND *pol,
			      uint32 start_idx, uint32 size)
{
	DEBUG(5, ("init_samr_q_enum_domains\n"));

	q_e->pol = *pol;

	q_e->start_idx = start_idx;
	q_e->max_size = size;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_enum_domains(const char *desc, SAMR_Q_ENUM_DOMAINS * q_e,
			    prs_struct *ps, int depth)
{
	if (q_e == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_enum_domains");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &q_e->pol, ps, depth))
		return False;

	if(!prs_uint32("start_idx", ps, depth, &q_e->start_idx))
		return False;
	if(!prs_uint32("max_size ", ps, depth, &q_e->max_size))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_R_ENUM_DOMAINS structure.
********************************************************************/

void init_samr_r_enum_domains(SAMR_R_ENUM_DOMAINS * r_u,
			      uint32 next_idx, uint32 num_sam_entries)
{
	DEBUG(5, ("init_samr_r_enum_domains\n"));

	r_u->next_idx = next_idx;

	if (num_sam_entries != 0) {
		r_u->ptr_entries1 = 1;
		r_u->ptr_entries2 = 1;
		r_u->num_entries2 = num_sam_entries;
		r_u->num_entries3 = num_sam_entries;

		r_u->num_entries4 = num_sam_entries;
	} else {
		r_u->ptr_entries1 = 0;
		r_u->num_entries2 = num_sam_entries;
		r_u->ptr_entries2 = 1;
	}
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_enum_domains(const char *desc, SAMR_R_ENUM_DOMAINS * r_u,
			    prs_struct *ps, int depth)
{
	uint32 i;

	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_enum_domains");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("next_idx    ", ps, depth, &r_u->next_idx))
		return False;
	if(!prs_uint32("ptr_entries1", ps, depth, &r_u->ptr_entries1))
		return False;

	if (r_u->ptr_entries1 != 0) {
		if(!prs_uint32("num_entries2", ps, depth, &r_u->num_entries2))
			return False;
		if(!prs_uint32("ptr_entries2", ps, depth, &r_u->ptr_entries2))
			return False;
		if(!prs_uint32("num_entries3", ps, depth, &r_u->num_entries3))
			return False;

		if (UNMARSHALLING(ps)) {
			r_u->sam = PRS_ALLOC_MEM(ps,SAM_ENTRY,r_u->num_entries2);
			r_u->uni_dom_name = PRS_ALLOC_MEM(ps,UNISTR2,r_u->num_entries2);
		}

		if ((r_u->sam == NULL || r_u->uni_dom_name == NULL) && r_u->num_entries2 != 0) {
			DEBUG(0, ("NULL pointers in SAMR_R_ENUM_DOMAINS\n"));
			r_u->num_entries4 = 0;
			r_u->status = NT_STATUS_MEMORY_NOT_ALLOCATED;
			return False;
		}

		for (i = 0; i < r_u->num_entries2; i++)	{
			fstring tmp;
			slprintf(tmp, sizeof(tmp) - 1, "dom[%d]", i);
			if(!sam_io_sam_entry(tmp, &r_u->sam[i], ps, depth))
				return False;
		}

		for (i = 0; i < r_u->num_entries2; i++)	{
			fstring tmp;
			slprintf(tmp, sizeof(tmp) - 1, "dom[%d]", i);
			if(!smb_io_unistr2(tmp, &r_u->uni_dom_name[i],
				       r_u->sam[i].hdr_name.buffer, ps,
				       depth))
				return False;
		}

	}

	if(!prs_align(ps))
		return False;
	if(!prs_uint32("num_entries4", ps, depth, &r_u->num_entries4))
		return False;
	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_ENUM_DOM_GROUPS structure.
********************************************************************/

void init_samr_q_enum_dom_groups(SAMR_Q_ENUM_DOM_GROUPS * q_e,
				 POLICY_HND *pol,
				 uint32 start_idx, uint32 size)
{
	DEBUG(5, ("init_samr_q_enum_dom_groups\n"));

	q_e->pol = *pol;

	q_e->start_idx = start_idx;
	q_e->max_size = size;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_enum_dom_groups(const char *desc, SAMR_Q_ENUM_DOM_GROUPS * q_e,
			       prs_struct *ps, int depth)
{
	if (q_e == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_enum_dom_groups");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &(q_e->pol), ps, depth))
		return False;

	if(!prs_uint32("start_idx", ps, depth, &q_e->start_idx))
		return False;
	if(!prs_uint32("max_size ", ps, depth, &q_e->max_size))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_R_ENUM_DOM_GROUPS structure.
********************************************************************/

void init_samr_r_enum_dom_groups(SAMR_R_ENUM_DOM_GROUPS * r_u,
				 uint32 next_idx, uint32 num_sam_entries)
{
	DEBUG(5, ("init_samr_r_enum_dom_groups\n"));

	r_u->next_idx = next_idx;

	if (num_sam_entries != 0) {
		r_u->ptr_entries1 = 1;
		r_u->ptr_entries2 = 1;
		r_u->num_entries2 = num_sam_entries;
		r_u->num_entries3 = num_sam_entries;

		r_u->num_entries4 = num_sam_entries;
	} else {
		r_u->ptr_entries1 = 0;
		r_u->num_entries2 = num_sam_entries;
		r_u->ptr_entries2 = 1;
	}
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_enum_dom_groups(const char *desc, SAMR_R_ENUM_DOM_GROUPS * r_u,
			       prs_struct *ps, int depth)
{
	uint32 i;

	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_enum_dom_groups");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("next_idx    ", ps, depth, &r_u->next_idx))
		return False;
	if(!prs_uint32("ptr_entries1", ps, depth, &r_u->ptr_entries1))
		return False;

	if (r_u->ptr_entries1 != 0) {
		if(!prs_uint32("num_entries2", ps, depth, &r_u->num_entries2))
			return False;
		if(!prs_uint32("ptr_entries2", ps, depth, &r_u->ptr_entries2))
			return False;
		if(!prs_uint32("num_entries3", ps, depth, &r_u->num_entries3))
			return False;

		if (UNMARSHALLING(ps)) {
			r_u->sam = PRS_ALLOC_MEM(ps,SAM_ENTRY,r_u->num_entries2);
			r_u->uni_grp_name = PRS_ALLOC_MEM(ps,UNISTR2,r_u->num_entries2);
		}

		if ((r_u->sam == NULL || r_u->uni_grp_name == NULL) && r_u->num_entries2 != 0) {
			DEBUG(0,
			      ("NULL pointers in SAMR_R_ENUM_DOM_GROUPS\n"));
			r_u->num_entries4 = 0;
			r_u->status = NT_STATUS_MEMORY_NOT_ALLOCATED;
			return False;
		}

		for (i = 0; i < r_u->num_entries2; i++)	{
			if(!sam_io_sam_entry("", &r_u->sam[i], ps, depth))
				return False;
		}

		for (i = 0; i < r_u->num_entries2; i++)	{
			if(!smb_io_unistr2("", &r_u->uni_grp_name[i],
				       r_u->sam[i].hdr_name.buffer, ps, depth))
				return False;
		}
	}

	if(!prs_align(ps))
		return False;
	if(!prs_uint32("num_entries4", ps, depth, &r_u->num_entries4))
		return False;
	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_ENUM_DOM_ALIASES structure.
********************************************************************/

void init_samr_q_enum_dom_aliases(SAMR_Q_ENUM_DOM_ALIASES * q_e,
				  POLICY_HND *pol, uint32 start_idx,
				  uint32 size)
{
	DEBUG(5, ("init_samr_q_enum_dom_aliases\n"));

	q_e->pol = *pol;

	q_e->start_idx = start_idx;
	q_e->max_size = size;
}


/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_enum_dom_aliases(const char *desc, SAMR_Q_ENUM_DOM_ALIASES * q_e,
				prs_struct *ps, int depth)
{
	if (q_e == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_enum_dom_aliases");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &q_e->pol, ps, depth))
		return False;

	if(!prs_uint32("start_idx", ps, depth, &q_e->start_idx))
		return False;
	if(!prs_uint32("max_size ", ps, depth, &q_e->max_size))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_R_ENUM_DOM_ALIASES structure.
********************************************************************/

void init_samr_r_enum_dom_aliases(SAMR_R_ENUM_DOM_ALIASES *r_u, uint32 next_idx, uint32 num_sam_entries)
{
	DEBUG(5, ("init_samr_r_enum_dom_aliases\n"));

	r_u->next_idx = next_idx;

	if (num_sam_entries != 0) {
		r_u->ptr_entries1 = 1;
		r_u->ptr_entries2 = 1;
		r_u->num_entries2 = num_sam_entries;
		r_u->num_entries3 = num_sam_entries;

		r_u->num_entries4 = num_sam_entries;
	} else {
		r_u->ptr_entries1 = 0;
		r_u->num_entries2 = num_sam_entries;
		r_u->ptr_entries2 = 1;
	}
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_enum_dom_aliases(const char *desc, SAMR_R_ENUM_DOM_ALIASES * r_u,
				prs_struct *ps, int depth)
{
	uint32 i;

	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_enum_dom_aliases");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("next_idx    ", ps, depth, &r_u->next_idx))
		return False;
	if(!prs_uint32("ptr_entries1", ps, depth, &r_u->ptr_entries1))
		return False;

	if (r_u->ptr_entries1 != 0) {
		if(!prs_uint32("num_entries2", ps, depth, &r_u->num_entries2))
			return False;
		if(!prs_uint32("ptr_entries2", ps, depth, &r_u->ptr_entries2))
			return False;
		if(!prs_uint32("num_entries3", ps, depth, &r_u->num_entries3))
			return False;

		if (UNMARSHALLING(ps) && (r_u->num_entries2 > 0)) {
			r_u->sam = PRS_ALLOC_MEM(ps,SAM_ENTRY,r_u->num_entries2);
			r_u->uni_grp_name = PRS_ALLOC_MEM(ps,UNISTR2,r_u->num_entries2);
		}

		if (r_u->num_entries2 != 0 && 
		    (r_u->sam == NULL || r_u->uni_grp_name == NULL)) {
			DEBUG(0,("NULL pointers in SAMR_R_ENUM_DOM_ALIASES\n"));
			r_u->num_entries4 = 0;
			r_u->status = NT_STATUS_MEMORY_NOT_ALLOCATED;
			return False;
		}

		for (i = 0; i < r_u->num_entries2; i++) {
			if(!sam_io_sam_entry("", &r_u->sam[i], ps, depth))
				return False;
		}

		for (i = 0; i < r_u->num_entries2; i++) {
			if(!smb_io_unistr2("", &r_u->uni_grp_name[i],
				       r_u->sam[i].hdr_name.buffer, ps,
				       depth))
				return False;
		}
	}

	if(!prs_align(ps))
		return False;
	if(!prs_uint32("num_entries4", ps, depth, &r_u->num_entries4))
		return False;
	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a ALIAS_INFO1 structure.
********************************************************************/

void init_samr_alias_info1(ALIAS_INFO1 * al1, char *acct_name, uint32 num_member, char *acct_desc)
{
	DEBUG(5, ("init_samr_alias_info1\n"));

	init_unistr4(&al1->name, acct_name, UNI_FLAGS_NONE);
	al1->num_member = num_member;
	init_unistr4(&al1->description, acct_desc, UNI_FLAGS_NONE);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_alias_info1(const char *desc, ALIAS_INFO1 * al1,
			 prs_struct *ps, int depth)
{
	if (al1 == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_alias_info1");
	depth++;

	if(!prs_align(ps))
		return False;

	if ( !prs_unistr4_hdr("name", ps, depth, &al1->name) )
		return False;
	if ( !prs_uint32("num_member", ps, depth, &al1->num_member) )
		return False;
	if ( !prs_unistr4_hdr("description", ps, depth, &al1->description) )
		return False;

	if ( !prs_unistr4_str("name", ps, depth, &al1->name) )
		return False;
	if ( !prs_align(ps) )
		return False;
	if ( !prs_unistr4_str("description", ps, depth, &al1->description) )
		return False;
	if ( !prs_align(ps) )
		return False;

	return True;
}

/*******************************************************************
inits a ALIAS_INFO3 structure.
********************************************************************/

void init_samr_alias_info3(ALIAS_INFO3 * al3, const char *acct_desc)
{
	DEBUG(5, ("init_samr_alias_info3\n"));

	init_unistr4(&al3->description, acct_desc, UNI_FLAGS_NONE);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_alias_info3(const char *desc, ALIAS_INFO3 *al3,
			 prs_struct *ps, int depth)
{
	if (al3 == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_alias_info3");
	depth++;

	if(!prs_align(ps))
		return False;

	if (!prs_unistr4("description", ps, depth, &al3->description))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_alias_info2(const char *desc, ALIAS_INFO2 *al2,
			 prs_struct *ps, int depth)
{
	if (al2 == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_alias_info2");
	depth++;

	if(!prs_align(ps))
		return False;

	if (!prs_unistr4("name", ps, depth, &al2->name))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_alias_info_ctr(const char *desc, prs_struct *ps, int depth, ALIAS_INFO_CTR * ctr)
{
	if ( !ctr )
		return False;

	prs_debug(ps, depth, desc, "samr_alias_info_ctr");
	depth++;

	if ( !prs_uint16("level", ps, depth, &ctr->level) )
		return False;

	if(!prs_align(ps))
		return False;
	switch (ctr->level) {
	case 1: 
		if(!samr_io_alias_info1("alias_info1", &ctr->alias.info1, ps, depth))
			return False;
		break;
	case 2: 
		if(!samr_io_alias_info2("alias_info2", &ctr->alias.info2, ps, depth))
			return False;
		break;
	case 3: 
		if(!samr_io_alias_info3("alias_info3", &ctr->alias.info3, ps, depth))
			return False;
		break;
	default:
		DEBUG(0,("samr_alias_info_ctr: unsupported switch level\n"));
		break;
	}

	return True;
}

/*******************************************************************
inits a SAMR_Q_QUERY_ALIASINFO structure.
********************************************************************/

void init_samr_q_query_aliasinfo(SAMR_Q_QUERY_ALIASINFO * q_e,
				 POLICY_HND *pol, uint32 switch_level)
{
	DEBUG(5, ("init_samr_q_query_aliasinfo\n"));

	q_e->pol = *pol;
	q_e->level = switch_level;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_query_aliasinfo(const char *desc, SAMR_Q_QUERY_ALIASINFO *in,
			       prs_struct *ps, int depth)
{
	if ( !in )
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_query_aliasinfo");
	depth++;

	if(!prs_align(ps))
		return False;

	if ( !smb_io_pol_hnd("pol", &(in->pol), ps, depth) )
		return False;

	if ( !prs_uint16("level", ps, depth, &in->level) )
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_R_QUERY_ALIASINFO structure.
********************************************************************/

void init_samr_r_query_aliasinfo(SAMR_R_QUERY_ALIASINFO *out,
				 ALIAS_INFO_CTR * ctr, NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_query_aliasinfo\n"));

	out->ctr = ctr;
	out->status = status;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_query_aliasinfo(const char *desc, SAMR_R_QUERY_ALIASINFO *out,
			       prs_struct *ps, int depth)
{
	if ( !out )
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_query_aliasinfo");
	depth++;

	if(!prs_align(ps))
		return False;

	if ( !prs_pointer("alias", ps, depth, (void**)&out->ctr, sizeof(ALIAS_INFO_CTR), (PRS_POINTER_CAST)samr_alias_info_ctr))
		return False;
	if(!prs_align(ps))
		return False;

	if(!prs_ntstatus("status", ps, depth, &out->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_SET_ALIASINFO structure.
********************************************************************/

void init_samr_q_set_aliasinfo(SAMR_Q_SET_ALIASINFO * q_u,
			       POLICY_HND *hnd, ALIAS_INFO_CTR * ctr)
{
	DEBUG(5, ("init_samr_q_set_aliasinfo\n"));

	q_u->alias_pol = *hnd;
	q_u->ctr = *ctr;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_set_aliasinfo(const char *desc, SAMR_Q_SET_ALIASINFO * q_u,
			     prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_set_aliasinfo");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("alias_pol", &q_u->alias_pol, ps, depth))
		return False;
	if(!samr_alias_info_ctr("ctr", ps, depth, &q_u->ctr))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_set_aliasinfo(const char *desc, SAMR_R_SET_ALIASINFO * r_u,
			     prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_set_aliasinfo");
	depth++;

	if(!prs_align(ps))
		return False;
	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_QUERY_USERALIASES structure.
********************************************************************/

void init_samr_q_query_useraliases(SAMR_Q_QUERY_USERALIASES * q_u,
				   POLICY_HND *hnd,
				   uint32 num_sids,
				   uint32 *ptr_sid, DOM_SID2 * sid)
{
	DEBUG(5, ("init_samr_q_query_useraliases\n"));

	q_u->pol = *hnd;

	q_u->num_sids1 = num_sids;
	q_u->ptr = 1;
	q_u->num_sids2 = num_sids;

	q_u->ptr_sid = ptr_sid;
	q_u->sid = sid;
}

/*******************************************************************
reads or writes a SAMR_Q_QUERY_USERALIASES structure.
********************************************************************/

BOOL samr_io_q_query_useraliases(const char *desc, SAMR_Q_QUERY_USERALIASES * q_u,
				 prs_struct *ps, int depth)
{
	fstring tmp;
	uint32 i;

	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_query_useraliases");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &q_u->pol, ps, depth))
		return False;

	if(!prs_uint32("num_sids1", ps, depth, &q_u->num_sids1))
		return False;
	if(!prs_uint32("ptr      ", ps, depth, &q_u->ptr))
		return False;

	if (q_u->ptr==0)
		return True;

	if(!prs_uint32("num_sids2", ps, depth, &q_u->num_sids2))
		return False;

	if (UNMARSHALLING(ps) && (q_u->num_sids2 != 0)) {
		q_u->ptr_sid = PRS_ALLOC_MEM(ps,uint32,q_u->num_sids2);
		if (q_u->ptr_sid == NULL)
			return False;

		q_u->sid = PRS_ALLOC_MEM(ps, DOM_SID2, q_u->num_sids2);
		if (q_u->sid == NULL)
			return False;
	}

	for (i = 0; i < q_u->num_sids2; i++) {
		slprintf(tmp, sizeof(tmp) - 1, "ptr[%02d]", i);
		if(!prs_uint32(tmp, ps, depth, &q_u->ptr_sid[i]))
			return False;
	}

	for (i = 0; i < q_u->num_sids2; i++) {
		if (q_u->ptr_sid[i] != 0) {
			slprintf(tmp, sizeof(tmp) - 1, "sid[%02d]", i);
			if(!smb_io_dom_sid2(tmp, &q_u->sid[i], ps, depth))
				return False;
		}
	}

	return True;
}

/*******************************************************************
inits a SAMR_R_QUERY_USERALIASES structure.
********************************************************************/

void init_samr_r_query_useraliases(SAMR_R_QUERY_USERALIASES * r_u,
				   uint32 num_rids, uint32 *rid,
				   NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_query_useraliases\n"));

	if (NT_STATUS_IS_OK(status)) {
		r_u->num_entries = num_rids;
		r_u->ptr = 1;
		r_u->num_entries2 = num_rids;

		r_u->rid = rid;
	} else {
		r_u->num_entries = 0;
		r_u->ptr = 0;
		r_u->num_entries2 = 0;
	}

	r_u->status = status;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_rids(const char *desc, uint32 *num_rids, uint32 **rid,
		  prs_struct *ps, int depth)
{
	fstring tmp;
	uint32 i;
	if (rid == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_rids");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("num_rids", ps, depth, num_rids))
		return False;

	if ((*num_rids) != 0) {
		if (UNMARSHALLING(ps)) {
			/* reading */
			(*rid) = PRS_ALLOC_MEM(ps,uint32, *num_rids);
		}
		if ((*rid) == NULL)
			return False;

		for (i = 0; i < (*num_rids); i++) {
			slprintf(tmp, sizeof(tmp) - 1, "rid[%02d]", i);
			if(!prs_uint32(tmp, ps, depth, &((*rid)[i])))
				return False;
		}
	}

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_query_useraliases(const char *desc, SAMR_R_QUERY_USERALIASES * r_u,
				 prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_query_useraliases");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("num_entries", ps, depth, &r_u->num_entries))
		return False;
	if(!prs_uint32("ptr        ", ps, depth, &r_u->ptr))
		return False;

	if (r_u->ptr != 0) {
		if(!samr_io_rids("rids", &r_u->num_entries2, &r_u->rid, ps, depth))
			return False;
	}

	if(!prs_align(ps))
		return False;
	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_OPEN_ALIAS structure.
********************************************************************/

void init_samr_q_open_alias(SAMR_Q_OPEN_ALIAS * q_u, POLICY_HND *pol,
			    uint32 access_mask, uint32 rid)
{
	DEBUG(5, ("init_samr_q_open_alias\n"));

	q_u->dom_pol = *pol;
	q_u->access_mask = access_mask;
	q_u->rid_alias = rid;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_open_alias(const char *desc, SAMR_Q_OPEN_ALIAS * q_u,
			  prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_open_alias");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("domain_pol", &q_u->dom_pol, ps, depth))
		return False;

	if(!prs_uint32("access_mask", ps, depth, &q_u->access_mask))
		return False;
	if(!prs_uint32("rid_alias", ps, depth, &q_u->rid_alias))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_open_alias(const char *desc, SAMR_R_OPEN_ALIAS * r_u,
			  prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_open_alias");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &r_u->pol, ps, depth))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_LOOKUP_RIDS structure.
********************************************************************/

void init_samr_q_lookup_rids(TALLOC_CTX *ctx, SAMR_Q_LOOKUP_RIDS * q_u,
			     POLICY_HND *pol, uint32 flags,
			     uint32 num_rids, uint32 *rid)
{
	DEBUG(5, ("init_samr_q_lookup_rids\n"));

	q_u->pol = *pol;

	q_u->num_rids1 = num_rids;
	q_u->flags = flags;
	q_u->ptr = 0;
	q_u->num_rids2 = num_rids;
	q_u->rid = TALLOC_ZERO_ARRAY(ctx, uint32, num_rids );
	if (q_u->rid == NULL) {
		q_u->num_rids1 = 0;
		q_u->num_rids2 = 0;
	} else {
		memcpy(q_u->rid, rid, num_rids * sizeof(q_u->rid[0]));
	}
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_lookup_rids(const char *desc, SAMR_Q_LOOKUP_RIDS * q_u,
			   prs_struct *ps, int depth)
{
	uint32 i;
	fstring tmp;

	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_lookup_rids");
	depth++;

	if (UNMARSHALLING(ps))
		ZERO_STRUCTP(q_u);

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &q_u->pol, ps, depth))
		return False;

	if(!prs_uint32("num_rids1", ps, depth, &q_u->num_rids1))
		return False;
	if(!prs_uint32("flags    ", ps, depth, &q_u->flags))
		return False;
	if(!prs_uint32("ptr      ", ps, depth, &q_u->ptr))
		return False;
	if(!prs_uint32("num_rids2", ps, depth, &q_u->num_rids2))
		return False;

	if (UNMARSHALLING(ps) && (q_u->num_rids2 != 0)) {
		q_u->rid = PRS_ALLOC_MEM(ps, uint32, q_u->num_rids2);
		if (q_u->rid == NULL)
			return False;
	}

	for (i = 0; i < q_u->num_rids2; i++) {
		slprintf(tmp, sizeof(tmp) - 1, "rid[%02d]  ", i);
		if(!prs_uint32(tmp, ps, depth, &q_u->rid[i]))
			return False;
	}

	return True;
}

/*******************************************************************
inits a SAMR_R_LOOKUP_RIDS structure.
********************************************************************/

void init_samr_r_lookup_rids(SAMR_R_LOOKUP_RIDS * r_u,
			     uint32 num_names, UNIHDR * hdr_name,
			     UNISTR2 *uni_name, uint32 *type)
{
	DEBUG(5, ("init_samr_r_lookup_rids\n"));

	r_u->hdr_name = NULL;
	r_u->uni_name = NULL;
	r_u->type = NULL;

	if (num_names != 0) {
		r_u->num_names1 = num_names;
		r_u->ptr_names = 1;
		r_u->num_names2 = num_names;

		r_u->num_types1 = num_names;
		r_u->ptr_types = 1;
		r_u->num_types2 = num_names;

		r_u->hdr_name = hdr_name;
		r_u->uni_name = uni_name;
		r_u->type = type;
	} else {
		r_u->num_names1 = num_names;
		r_u->ptr_names = 0;
		r_u->num_names2 = num_names;

		r_u->num_types1 = num_names;
		r_u->ptr_types = 0;
		r_u->num_types2 = num_names;
	}
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_lookup_rids(const char *desc, SAMR_R_LOOKUP_RIDS * r_u,
			   prs_struct *ps, int depth)
{
	uint32 i;
	fstring tmp;
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_lookup_rids");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("num_names1", ps, depth, &r_u->num_names1))
		return False;
	if(!prs_uint32("ptr_names ", ps, depth, &r_u->ptr_names))
		return False;

	if (r_u->ptr_names != 0) {

		if(!prs_uint32("num_names2", ps, depth, &r_u->num_names2))
			return False;


		if (UNMARSHALLING(ps) && (r_u->num_names2 != 0)) {
			r_u->hdr_name = PRS_ALLOC_MEM(ps, UNIHDR, r_u->num_names2);
			if (r_u->hdr_name == NULL)
				return False;

			r_u->uni_name = PRS_ALLOC_MEM(ps, UNISTR2, r_u->num_names2);
			if (r_u->uni_name == NULL)
				return False;
		}
		
		for (i = 0; i < r_u->num_names2; i++) {
			slprintf(tmp, sizeof(tmp) - 1, "hdr[%02d]  ", i);
			if(!smb_io_unihdr("", &r_u->hdr_name[i], ps, depth))
				return False;
		}
		for (i = 0; i < r_u->num_names2; i++) {
			slprintf(tmp, sizeof(tmp) - 1, "str[%02d]  ", i);
			if(!smb_io_unistr2("", &r_u->uni_name[i], r_u->hdr_name[i].buffer, ps, depth))
				return False;
		}

	}
	
	if(!prs_align(ps))
		return False;
	if(!prs_uint32("num_types1", ps, depth, &r_u->num_types1))
		return False;
	if(!prs_uint32("ptr_types ", ps, depth, &r_u->ptr_types))
		return False;

	if (r_u->ptr_types != 0) {

		if(!prs_uint32("num_types2", ps, depth, &r_u->num_types2))
			return False;

		if (UNMARSHALLING(ps) && (r_u->num_types2 != 0)) {
			r_u->type = PRS_ALLOC_MEM(ps, uint32, r_u->num_types2);
			if (r_u->type == NULL)
				return False;
		}

		for (i = 0; i < r_u->num_types2; i++) {
			slprintf(tmp, sizeof(tmp) - 1, "type[%02d]  ", i);
			if(!prs_uint32(tmp, ps, depth, &r_u->type[i]))
				return False;
		}
	}

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_OPEN_ALIAS structure.
********************************************************************/

void init_samr_q_delete_alias(SAMR_Q_DELETE_DOM_ALIAS * q_u, POLICY_HND *hnd)
{
	DEBUG(5, ("init_samr_q_delete_alias\n"));

	q_u->alias_pol = *hnd;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_delete_alias(const char *desc, SAMR_Q_DELETE_DOM_ALIAS * q_u,
			    prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_delete_alias");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("alias_pol", &q_u->alias_pol, ps, depth))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_delete_alias(const char *desc, SAMR_R_DELETE_DOM_ALIAS * r_u,
			    prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_delete_alias");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &r_u->pol, ps, depth))
		return False;
	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_CREATE_DOM_ALIAS structure.
********************************************************************/

void init_samr_q_create_dom_alias(SAMR_Q_CREATE_DOM_ALIAS * q_u,
				  POLICY_HND *hnd, const char *acct_desc)
{
	DEBUG(5, ("init_samr_q_create_dom_alias\n"));

	q_u->dom_pol = *hnd;

	init_unistr2(&q_u->uni_acct_desc, acct_desc, UNI_FLAGS_NONE);
	init_uni_hdr(&q_u->hdr_acct_desc, &q_u->uni_acct_desc);

	q_u->access_mask = MAXIMUM_ALLOWED_ACCESS;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_create_dom_alias(const char *desc, SAMR_Q_CREATE_DOM_ALIAS * q_u,
				prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_create_dom_alias");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("dom_pol", &q_u->dom_pol, ps, depth))
		return False;

	if(!smb_io_unihdr("hdr_acct_desc", &q_u->hdr_acct_desc, ps, depth))
		return False;
	if(!smb_io_unistr2("uni_acct_desc", &q_u->uni_acct_desc,
		       q_u->hdr_acct_desc.buffer, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;
	if(!prs_uint32("access_mask", ps, depth, &q_u->access_mask))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_create_dom_alias(const char *desc, SAMR_R_CREATE_DOM_ALIAS * r_u,
				prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_create_dom_alias");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("alias_pol", &r_u->alias_pol, ps, depth))
		return False;

	if(!prs_uint32("rid", ps, depth, &r_u->rid))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_ADD_ALIASMEM structure.
********************************************************************/

void init_samr_q_add_aliasmem(SAMR_Q_ADD_ALIASMEM * q_u, POLICY_HND *hnd,
			      DOM_SID *sid)
{
	DEBUG(5, ("init_samr_q_add_aliasmem\n"));

	q_u->alias_pol = *hnd;
	init_dom_sid2(&q_u->sid, sid);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_add_aliasmem(const char *desc, SAMR_Q_ADD_ALIASMEM * q_u,
			    prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_add_aliasmem");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("alias_pol", &q_u->alias_pol, ps, depth))
		return False;
	if(!smb_io_dom_sid2("sid      ", &q_u->sid, ps, depth))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_add_aliasmem(const char *desc, SAMR_R_ADD_ALIASMEM * r_u,
			    prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_add_aliasmem");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_DEL_ALIASMEM structure.
********************************************************************/

void init_samr_q_del_aliasmem(SAMR_Q_DEL_ALIASMEM * q_u, POLICY_HND *hnd,
			      DOM_SID *sid)
{
	DEBUG(5, ("init_samr_q_del_aliasmem\n"));

	q_u->alias_pol = *hnd;
	init_dom_sid2(&q_u->sid, sid);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_del_aliasmem(const char *desc, SAMR_Q_DEL_ALIASMEM * q_u,
			    prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_del_aliasmem");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("alias_pol", &q_u->alias_pol, ps, depth))
		return False;
	if(!smb_io_dom_sid2("sid      ", &q_u->sid, ps, depth))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_del_aliasmem(const char *desc, SAMR_R_DEL_ALIASMEM * r_u,
			    prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_del_aliasmem");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_DELETE_DOM_ALIAS structure.
********************************************************************/

void init_samr_q_delete_dom_alias(SAMR_Q_DELETE_DOM_ALIAS * q_c,
				  POLICY_HND *hnd)
{
	DEBUG(5, ("init_samr_q_delete_dom_alias\n"));

	q_c->alias_pol = *hnd;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_delete_dom_alias(const char *desc, SAMR_Q_DELETE_DOM_ALIAS * q_u,
				prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_delete_dom_alias");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("alias_pol", &q_u->alias_pol, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_R_DELETE_DOM_ALIAS structure.
********************************************************************/

void init_samr_r_delete_dom_alias(SAMR_R_DELETE_DOM_ALIAS * r_u,
				  NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_delete_dom_alias\n"));

	r_u->status = status;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_delete_dom_alias(const char *desc, SAMR_R_DELETE_DOM_ALIAS * r_u,
				prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_delete_dom_alias");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &r_u->pol, ps, depth))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_QUERY_ALIASMEM structure.
********************************************************************/

void init_samr_q_query_aliasmem(SAMR_Q_QUERY_ALIASMEM * q_c,
				POLICY_HND *hnd)
{
	DEBUG(5, ("init_samr_q_query_aliasmem\n"));

	q_c->alias_pol = *hnd;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_query_aliasmem(const char *desc, SAMR_Q_QUERY_ALIASMEM * q_u,
			      prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_query_aliasmem");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("alias_pol", &q_u->alias_pol, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_R_QUERY_ALIASMEM structure.
********************************************************************/

void init_samr_r_query_aliasmem(SAMR_R_QUERY_ALIASMEM * r_u,
				uint32 num_sids, DOM_SID2 * sid,
				NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_query_aliasmem\n"));

	if (NT_STATUS_IS_OK(status)) {
		r_u->num_sids = num_sids;
		r_u->ptr = (num_sids != 0) ? 1 : 0;
		r_u->num_sids1 = num_sids;

		r_u->sid = sid;
	} else {
		r_u->ptr = 0;
		r_u->num_sids = 0;
	}

	r_u->status = status;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_query_aliasmem(const char *desc, SAMR_R_QUERY_ALIASMEM * r_u,
			      prs_struct *ps, int depth)
{
	uint32 i;

	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_query_aliasmem");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("num_sids ", ps, depth, &r_u->num_sids))
		return False;
	if(!prs_uint32("ptr", ps, depth, &r_u->ptr))
		return False;

	if (r_u->ptr != 0 && r_u->num_sids != 0) {
		uint32 *ptr_sid = NULL;

		if(!prs_uint32("num_sids1", ps, depth, &r_u->num_sids1))
			return False;

		ptr_sid = TALLOC_ARRAY(ps->mem_ctx, uint32, r_u->num_sids1);
		if (!ptr_sid) {
			return False;
		}
		
		for (i = 0; i < r_u->num_sids1; i++) {
			ptr_sid[i] = 1;
			if(!prs_uint32("ptr_sid", ps, depth, &ptr_sid[i]))
				return False;
		}
		
		if (UNMARSHALLING(ps)) {
			r_u->sid = TALLOC_ARRAY(ps->mem_ctx, DOM_SID2, r_u->num_sids1);
		}
		
		for (i = 0; i < r_u->num_sids1; i++) {
			if (ptr_sid[i] != 0) {
				if(!smb_io_dom_sid2("sid", &r_u->sid[i], ps, depth))
					return False;
			}
		}
	}

	if(!prs_align(ps))
		return False;
	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_LOOKUP_NAMES structure.
********************************************************************/

NTSTATUS init_samr_q_lookup_names(TALLOC_CTX *ctx, SAMR_Q_LOOKUP_NAMES * q_u,
			      POLICY_HND *pol, uint32 flags,
			      uint32 num_names, const char **name)
{
	uint32 i;

	DEBUG(5, ("init_samr_q_lookup_names\n"));

	q_u->pol = *pol;

	q_u->num_names1 = num_names;
	q_u->flags = flags;
	q_u->ptr = 0;
	q_u->num_names2 = num_names;

	if (!(q_u->hdr_name = TALLOC_ZERO_ARRAY(ctx, UNIHDR, num_names)))
		return NT_STATUS_NO_MEMORY;

	if (!(q_u->uni_name = TALLOC_ZERO_ARRAY(ctx, UNISTR2, num_names)))
		return NT_STATUS_NO_MEMORY;

	for (i = 0; i < num_names; i++) {
		init_unistr2(&q_u->uni_name[i], name[i], UNI_FLAGS_NONE);	/* unicode string for machine account */
		init_uni_hdr(&q_u->hdr_name[i], &q_u->uni_name[i]);	/* unicode header for user_name */
	}

	return NT_STATUS_OK;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_lookup_names(const char *desc, SAMR_Q_LOOKUP_NAMES * q_u,
			    prs_struct *ps, int depth)
{
	uint32 i;

	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_lookup_names");
	depth++;

	if (UNMARSHALLING(ps))
		ZERO_STRUCTP(q_u);

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &q_u->pol, ps, depth))
		return False;

	if(!prs_uint32("num_names1", ps, depth, &q_u->num_names1))
		return False;
	if(!prs_uint32("flags     ", ps, depth, &q_u->flags))
		return False;
	if(!prs_uint32("ptr       ", ps, depth, &q_u->ptr))
		return False;
	if(!prs_uint32("num_names2", ps, depth, &q_u->num_names2))
		return False;

	if (UNMARSHALLING(ps) && (q_u->num_names2 != 0)) {
		q_u->hdr_name = PRS_ALLOC_MEM(ps, UNIHDR, q_u->num_names2);
		q_u->uni_name = PRS_ALLOC_MEM(ps, UNISTR2, q_u->num_names2);
		if (!q_u->hdr_name || !q_u->uni_name)
			return False;
	}

	for (i = 0; i < q_u->num_names2; i++) {
		if(!smb_io_unihdr("", &q_u->hdr_name[i], ps, depth))
			return False;
	}

	for (i = 0; i < q_u->num_names2; i++) {
		if(!smb_io_unistr2("", &q_u->uni_name[i], q_u->hdr_name[i].buffer, ps, depth))
			return False;
	}

	return True;
}

/*******************************************************************
inits a SAMR_R_LOOKUP_NAMES structure.
********************************************************************/

NTSTATUS init_samr_r_lookup_names(TALLOC_CTX *ctx, SAMR_R_LOOKUP_NAMES * r_u,
			      uint32 num_rids,
			      uint32 *rid, uint32 *type,
			      NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_lookup_names\n"));

	if (NT_STATUS_IS_OK(status) && (num_rids != 0))	{
		uint32 i;

		r_u->num_types1 = num_rids;
		r_u->ptr_types = 1;
		r_u->num_types2 = num_rids;

		r_u->num_rids1 = num_rids;
		r_u->ptr_rids = 1;
		r_u->num_rids2 = num_rids;

		if (!(r_u->rids = TALLOC_ZERO_ARRAY(ctx, uint32, num_rids)))
			return NT_STATUS_NO_MEMORY;
		if (!(r_u->types = TALLOC_ZERO_ARRAY(ctx, uint32, num_rids)))
			return NT_STATUS_NO_MEMORY;

		if (!r_u->rids || !r_u->types)
			goto empty;

		for (i = 0; i < num_rids; i++) {
			r_u->rids[i] = rid[i];
			r_u->types[i] = type[i];
		}
	} else {

  empty:
		r_u->num_types1 = 0;
		r_u->ptr_types = 0;
		r_u->num_types2 = 0;

		r_u->num_rids1 = 0;
		r_u->ptr_rids = 0;
		r_u->num_rids2 = 0;

		r_u->rids = NULL;
		r_u->types = NULL;
	}

	r_u->status = status;

	return NT_STATUS_OK;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_lookup_names(const char *desc, SAMR_R_LOOKUP_NAMES * r_u,
			    prs_struct *ps, int depth)
{
	uint32 i;
	fstring tmp;

	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_lookup_names");
	depth++;

	if (UNMARSHALLING(ps))
		ZERO_STRUCTP(r_u);

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("num_rids1", ps, depth, &r_u->num_rids1))
		return False;
	if(!prs_uint32("ptr_rids ", ps, depth, &r_u->ptr_rids))
		return False;

	if (r_u->ptr_rids != 0)	{
		if(!prs_uint32("num_rids2", ps, depth, &r_u->num_rids2))
			return False;

		if (r_u->num_rids2 != r_u->num_rids1) {
			/* RPC fault */
			return False;
		}

		if (UNMARSHALLING(ps))
			r_u->rids = PRS_ALLOC_MEM(ps, uint32, r_u->num_rids2);

		if (!r_u->rids) {
			DEBUG(0, ("NULL rids in samr_io_r_lookup_names\n"));
			return False;
		}

		for (i = 0; i < r_u->num_rids2; i++) {
			slprintf(tmp, sizeof(tmp) - 1, "rid[%02d]  ", i);
			if(!prs_uint32(tmp, ps, depth, &r_u->rids[i]))
				return False;
		}
	}

	if(!prs_uint32("num_types1", ps, depth, &r_u->num_types1))
		return False;
	if(!prs_uint32("ptr_types ", ps, depth, &r_u->ptr_types))
		return False;

	if (r_u->ptr_types != 0) {
		if(!prs_uint32("num_types2", ps, depth, &r_u->num_types2))
			return False;

		if (r_u->num_types2 != r_u->num_types1) {
			/* RPC fault */
			return False;
		}

		if (UNMARSHALLING(ps))
			r_u->types = PRS_ALLOC_MEM(ps, uint32, r_u->num_types2);

		if (!r_u->types) {
			DEBUG(0, ("NULL types in samr_io_r_lookup_names\n"));
			return False;
		}

		for (i = 0; i < r_u->num_types2; i++) {
			slprintf(tmp, sizeof(tmp) - 1, "type[%02d]  ", i);
			if(!prs_uint32(tmp, ps, depth, &r_u->types[i]))
				return False;
		}
	}

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_DELETE_DOM_USER structure.
********************************************************************/

void init_samr_q_delete_dom_user(SAMR_Q_DELETE_DOM_USER * q_c,
				 POLICY_HND *hnd)
{
	DEBUG(5, ("init_samr_q_delete_dom_user\n"));

	q_c->user_pol = *hnd;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_delete_dom_user(const char *desc, SAMR_Q_DELETE_DOM_USER * q_u,
			       prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_delete_dom_user");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("user_pol", &q_u->user_pol, ps, depth))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_delete_dom_user(const char *desc, SAMR_R_DELETE_DOM_USER * r_u,
			       prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_delete_dom_user");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &r_u->pol, ps, depth))
		return False;
	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

void init_samr_q_open_user(SAMR_Q_OPEN_USER * q_u,
			   POLICY_HND *pol,
			   uint32 access_mask, uint32 rid)
{
	DEBUG(5, ("samr_init_samr_q_open_user\n"));

	q_u->domain_pol = *pol;
	q_u->access_mask = access_mask;
	q_u->user_rid = rid;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_open_user(const char *desc, SAMR_Q_OPEN_USER * q_u,
			 prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_open_user");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("domain_pol", &q_u->domain_pol, ps, depth))
		return False;

	if(!prs_uint32("access_mask", ps, depth, &q_u->access_mask))
		return False;
	if(!prs_uint32("user_rid ", ps, depth, &q_u->user_rid))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_open_user(const char *desc, SAMR_R_OPEN_USER * r_u,
			 prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_open_user");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("user_pol", &r_u->user_pol, ps, depth))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}


/*******************************************************************
reads or writes a structure.
********************************************************************/

void init_samr_q_create_user(SAMR_Q_CREATE_USER * q_u,
			     POLICY_HND *pol,
			     const char *name,
			     uint32 acb_info, uint32 access_mask)
{
	DEBUG(5, ("samr_init_samr_q_create_user\n"));

	q_u->domain_pol = *pol;

	init_unistr2(&q_u->uni_name, name, UNI_FLAGS_NONE);
	init_uni_hdr(&q_u->hdr_name, &q_u->uni_name);

	q_u->acb_info = acb_info;
	q_u->access_mask = access_mask;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_create_user(const char *desc, SAMR_Q_CREATE_USER * q_u,
			   prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_create_user");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("domain_pol", &q_u->domain_pol, ps, depth))
		return False;

	if(!smb_io_unihdr("hdr_name", &q_u->hdr_name, ps, depth))
		return False;
	if(!smb_io_unistr2("uni_name", &q_u->uni_name, q_u->hdr_name.buffer, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;
	if(!prs_uint32("acb_info   ", ps, depth, &q_u->acb_info))
		return False;
	if(!prs_uint32("access_mask", ps, depth, &q_u->access_mask))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_create_user(const char *desc, SAMR_R_CREATE_USER * r_u,
			   prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_create_user");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("user_pol", &r_u->user_pol, ps, depth))
		return False;

	if(!prs_uint32("access_granted", ps, depth, &r_u->access_granted))
		return False;
	if(!prs_uint32("user_rid ", ps, depth, &r_u->user_rid))
		return False;
	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_QUERY_USERINFO structure.
********************************************************************/

void init_samr_q_query_userinfo(SAMR_Q_QUERY_USERINFO * q_u,
				const POLICY_HND *hnd, uint16 switch_value)
{
	DEBUG(5, ("init_samr_q_query_userinfo\n"));

	q_u->pol = *hnd;
	q_u->switch_value = switch_value;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_query_userinfo(const char *desc, SAMR_Q_QUERY_USERINFO * q_u,
			      prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_query_userinfo");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &q_u->pol, ps, depth))
		return False;

	if(!prs_uint16("switch_value", ps, depth, &q_u->switch_value)) /* 0x0015 or 0x0011 */
		return False;

	return True;
}

/*******************************************************************
reads or writes a LOGON_HRS structure.
********************************************************************/

static BOOL sam_io_logon_hrs(const char *desc, LOGON_HRS * hrs,
			     prs_struct *ps, int depth)
{
	if (hrs == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_logon_hrs");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("maxlen", ps, depth, &hrs->max_len))
		return False;

	if(!prs_uint32("offset", ps, depth, &hrs->offset))
		return False;

	if(!prs_uint32("len  ", ps, depth, &hrs->len))
		return False;

	if (hrs->len > sizeof(hrs->hours)) {
		DEBUG(3, ("sam_io_logon_hrs: truncating length from %d\n", hrs->len));
		hrs->len = sizeof(hrs->hours);
	}

	if(!prs_uint8s(False, "hours", ps, depth, hrs->hours, hrs->len))
		return False;

	return True;
}

/*******************************************************************
inits a SAM_USER_INFO_18 structure.
********************************************************************/

void init_sam_user_info18(SAM_USER_INFO_18 * usr,
			  const uint8 lm_pwd[16], const uint8 nt_pwd[16])
{
	DEBUG(5, ("init_sam_user_info18\n"));

	usr->lm_pwd_active =
		memcpy(usr->lm_pwd, lm_pwd, sizeof(usr->lm_pwd)) ? 1 : 0;
	usr->nt_pwd_active =
		memcpy(usr->nt_pwd, nt_pwd, sizeof(usr->nt_pwd)) ? 1 : 0;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_user_info18(const char *desc, SAM_USER_INFO_18 * u,
			prs_struct *ps, int depth)
{
	if (u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_user_info18");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint8s(False, "lm_pwd", ps, depth, u->lm_pwd, sizeof(u->lm_pwd)))
		return False;
	if(!prs_uint8s(False, "nt_pwd", ps, depth, u->nt_pwd, sizeof(u->nt_pwd)))
		return False;

	if(!prs_uint8("lm_pwd_active", ps, depth, &u->lm_pwd_active))
		return False;
	if(!prs_uint8("nt_pwd_active", ps, depth, &u->nt_pwd_active))
		return False;

	return True;
}

/*******************************************************************
inits a SAM_USER_INFO_7 structure.
********************************************************************/

void init_sam_user_info7(SAM_USER_INFO_7 * usr, const char *name)
{
	DEBUG(5, ("init_sam_user_info7\n"));

	init_unistr2(&usr->uni_name, name, UNI_FLAGS_NONE);	/* unicode string for name */
	init_uni_hdr(&usr->hdr_name, &usr->uni_name);		/* unicode header for name */

}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_user_info7(const char *desc, SAM_USER_INFO_7 * usr,
			prs_struct *ps, int depth)
{
	if (usr == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_user_info7");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_unihdr("unihdr", &usr->hdr_name, ps, depth))
		return False;

	if(!smb_io_unistr2("unistr2", &usr->uni_name, True, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a SAM_USER_INFO_9 structure.
********************************************************************/

void init_sam_user_info9(SAM_USER_INFO_9 * usr, uint32 rid_group)
{
	DEBUG(5, ("init_sam_user_info9\n"));

	usr->rid_group = rid_group;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_user_info9(const char *desc, SAM_USER_INFO_9 * usr,
			prs_struct *ps, int depth)
{
	if (usr == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_user_info9");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("rid_group", ps, depth, &usr->rid_group))
		return False;

	return True;
}

/*******************************************************************
inits a SAM_USER_INFO_16 structure.
********************************************************************/

void init_sam_user_info16(SAM_USER_INFO_16 * usr, uint32 acb_info)
{
	DEBUG(5, ("init_sam_user_info16\n"));

	usr->acb_info = acb_info;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_user_info16(const char *desc, SAM_USER_INFO_16 * usr,
			prs_struct *ps, int depth)
{
	if (usr == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_user_info16");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("acb_info", ps, depth, &usr->acb_info))
		return False;

	return True;
}

/*******************************************************************
inits a SAM_USER_INFO_17 structure.
********************************************************************/

void init_sam_user_info17(SAM_USER_INFO_17 * usr,
			  NTTIME * expiry,
			  char *mach_acct,
			  uint32 rid_user, uint32 rid_group, uint16 acct_ctrl)
{
	DEBUG(5, ("init_sam_user_info17\n"));

	memcpy(&usr->expiry, expiry, sizeof(usr->expiry));	/* expiry time or something? */
	ZERO_STRUCT(usr->padding_1);	/* 0 - padding 24 bytes */

	usr->padding_2 = 0;	/* 0 - padding 4 bytes */

	usr->ptr_1 = 1;		/* pointer */
	ZERO_STRUCT(usr->padding_3);	/* 0 - padding 32 bytes */
	usr->padding_4 = 0;	/* 0 - padding 4 bytes */

	usr->ptr_2 = 1;		/* pointer */
	usr->padding_5 = 0;	/* 0 - padding 4 bytes */

	usr->ptr_3 = 1;		/* pointer */
	ZERO_STRUCT(usr->padding_6);	/* 0 - padding 32 bytes */

	usr->rid_user = rid_user;
	usr->rid_group = rid_group;

	usr->acct_ctrl = acct_ctrl;
	usr->unknown_3 = 0x0000;

	usr->unknown_4 = 0x003f;	/* 0x003f      - 16 bit unknown */
	usr->unknown_5 = 0x003c;	/* 0x003c      - 16 bit unknown */

	ZERO_STRUCT(usr->padding_7);	/* 0 - padding 16 bytes */
	usr->padding_8 = 0;	/* 0 - padding 4 bytes */

	init_unistr2(&usr->uni_mach_acct, mach_acct, UNI_FLAGS_NONE);	/* unicode string for machine account */
	init_uni_hdr(&usr->hdr_mach_acct, &usr->uni_mach_acct);	/* unicode header for machine account */
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_user_info17(const char *desc, SAM_USER_INFO_17 * usr,
			prs_struct *ps, int depth)
{
	if (usr == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_unknown_17");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint8s(False, "padding_0", ps, depth, usr->padding_0, sizeof(usr->padding_0)))
		return False;

	if(!smb_io_time("time", &usr->expiry, ps, depth))
		return False;

	if(!prs_uint8s(False, "padding_1", ps, depth, usr->padding_1, sizeof(usr->padding_1)))
		return False;

	if(!smb_io_unihdr("unihdr", &usr->hdr_mach_acct, ps, depth))
		return False;

	if(!prs_uint32("padding_2", ps, depth, &usr->padding_2))
		return False;

	if(!prs_uint32("ptr_1    ", ps, depth, &usr->ptr_1))
		return False;
	if(!prs_uint8s(False, "padding_3", ps, depth, usr->padding_3, sizeof(usr->padding_3)))
		return False;

	if(!prs_uint32("padding_4", ps, depth, &usr->padding_4))
		return False;

	if(!prs_uint32("ptr_2    ", ps, depth, &usr->ptr_2))
		return False;
	if(!prs_uint32("padding_5", ps, depth, &usr->padding_5))
		return False;

	if(!prs_uint32("ptr_3    ", ps, depth, &usr->ptr_3))
		return False;
	if(!prs_uint8s(False, "padding_6", ps, depth, usr->padding_6,sizeof(usr->padding_6)))
		return False;

	if(!prs_uint32("rid_user ", ps, depth, &usr->rid_user))
		return False;
	if(!prs_uint32("rid_group", ps, depth, &usr->rid_group))
		return False;
	if(!prs_uint16("acct_ctrl", ps, depth, &usr->acct_ctrl))
		return False;
	if(!prs_uint16("unknown_3", ps, depth, &usr->unknown_3))
		return False;
	if(!prs_uint16("unknown_4", ps, depth, &usr->unknown_4))
		return False;
	if(!prs_uint16("unknown_5", ps, depth, &usr->unknown_5))
		return False;

	if(!prs_uint8s(False, "padding_7", ps, depth, usr->padding_7, sizeof(usr->padding_7)))
		return False;

	if(!prs_uint32("padding_8", ps, depth, &(usr->padding_8)))
		return False;

	if(!smb_io_unistr2("unistr2", &usr->uni_mach_acct, True, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint8s(False, "padding_9", ps, depth, usr->padding_9, sizeof(usr->padding_9)))
		return False;

	return True;
}

/*************************************************************************
 init_sam_user_infoa
 *************************************************************************/

void init_sam_user_info24(SAM_USER_INFO_24 * usr, char newpass[516], uint16 pw_len)
{
	DEBUG(10, ("init_sam_user_info24:\n"));
	memcpy(usr->pass, newpass, sizeof(usr->pass));
	usr->pw_len = pw_len;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_user_info24(const char *desc, SAM_USER_INFO_24 * usr,
			       prs_struct *ps, int depth)
{
	if (usr == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_user_info24");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint8s(False, "password", ps, depth, usr->pass, 
		       sizeof(usr->pass)))
		return False;
	
	if (MARSHALLING(ps) && (usr->pw_len != 0)) {
		if (!prs_uint16("pw_len", ps, depth, &usr->pw_len))
			return False;
	} else if (UNMARSHALLING(ps)) {
		if (!prs_uint16("pw_len", ps, depth, &usr->pw_len))
			return False;
	}

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_user_info26(const char *desc, SAM_USER_INFO_26 * usr,
			       prs_struct *ps, int depth)
{
	if (usr == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_user_info26");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint8s(False, "password", ps, depth, usr->pass, 
		       sizeof(usr->pass)))
		return False;
	
	if (!prs_uint8("pw_len", ps, depth, &usr->pw_len))
		return False;

	return True;
}


/*************************************************************************
 init_sam_user_info23

 unknown_6 = 0x0000 04ec 

 *************************************************************************/

void init_sam_user_info23W(SAM_USER_INFO_23 * usr, NTTIME * logon_time,	/* all zeros */
			NTTIME * logoff_time,	/* all zeros */
			NTTIME * kickoff_time,	/* all zeros */
			NTTIME * pass_last_set_time,	/* all zeros */
			NTTIME * pass_can_change_time,	/* all zeros */
			NTTIME * pass_must_change_time,	/* all zeros */
			UNISTR2 *user_name,
			UNISTR2 *full_name,
			UNISTR2 *home_dir,
			UNISTR2 *dir_drive,
			UNISTR2 *log_scr,
			UNISTR2 *prof_path,
			UNISTR2 *desc,
			UNISTR2 *wkstas,
			UNISTR2 *unk_str,
			UNISTR2 *mung_dial,
			uint32 user_rid,	/* 0x0000 0000 */
			uint32 group_rid,
			uint32 acb_info,
			uint32 fields_present,
			uint16 logon_divs,
			LOGON_HRS * hrs,
			uint16 bad_password_count,
			uint16 logon_count,
			char newpass[516])
{
	usr->logon_time = *logon_time;	/* all zeros */
	usr->logoff_time = *logoff_time;	/* all zeros */
	usr->kickoff_time = *kickoff_time;	/* all zeros */
	usr->pass_last_set_time = *pass_last_set_time;	/* all zeros */
	usr->pass_can_change_time = *pass_can_change_time;	/* all zeros */
	usr->pass_must_change_time = *pass_must_change_time;	/* all zeros */

	ZERO_STRUCT(usr->nt_pwd);
	ZERO_STRUCT(usr->lm_pwd);

	usr->user_rid = user_rid;	/* 0x0000 0000 */
	usr->group_rid = group_rid;
	usr->acb_info = acb_info;
	usr->fields_present = fields_present;	/* 09f8 27fa */

	usr->logon_divs = logon_divs;	/* should be 168 (hours/week) */
	usr->ptr_logon_hrs = hrs ? 1 : 0;

	if (nt_time_is_zero(pass_must_change_time)) {
		usr->passmustchange=PASS_MUST_CHANGE_AT_NEXT_LOGON;
	} else {
		usr->passmustchange=0;
	}

	ZERO_STRUCT(usr->padding1);
	ZERO_STRUCT(usr->padding2);

	usr->bad_password_count = bad_password_count;
	usr->logon_count = logon_count;

	memcpy(usr->pass, newpass, sizeof(usr->pass));

	copy_unistr2(&usr->uni_user_name, user_name);
	init_uni_hdr(&usr->hdr_user_name, &usr->uni_user_name);

	copy_unistr2(&usr->uni_full_name, full_name);
	init_uni_hdr(&usr->hdr_full_name, &usr->uni_full_name);

	copy_unistr2(&usr->uni_home_dir, home_dir);
	init_uni_hdr(&usr->hdr_home_dir, &usr->uni_home_dir);

	copy_unistr2(&usr->uni_dir_drive, dir_drive);
	init_uni_hdr(&usr->hdr_dir_drive, &usr->uni_dir_drive);

	copy_unistr2(&usr->uni_logon_script, log_scr);
	init_uni_hdr(&usr->hdr_logon_script, &usr->uni_logon_script);

	copy_unistr2(&usr->uni_profile_path, prof_path);
	init_uni_hdr(&usr->hdr_profile_path, &usr->uni_profile_path);

	copy_unistr2(&usr->uni_acct_desc, desc);
	init_uni_hdr(&usr->hdr_acct_desc, &usr->uni_acct_desc);

	copy_unistr2(&usr->uni_workstations, wkstas);
	init_uni_hdr(&usr->hdr_workstations, &usr->uni_workstations);

	copy_unistr2(&usr->uni_unknown_str, unk_str);
	init_uni_hdr(&usr->hdr_unknown_str, &usr->uni_unknown_str);

	copy_unistr2(&usr->uni_munged_dial, mung_dial);
	init_uni_hdr(&usr->hdr_munged_dial, &usr->uni_munged_dial);

	if (hrs) {
		memcpy(&usr->logon_hrs, hrs, sizeof(usr->logon_hrs));
	} else {
		ZERO_STRUCT(usr->logon_hrs);
	}
}

/*************************************************************************
 init_sam_user_info23

 unknown_6 = 0x0000 04ec 

 *************************************************************************/

void init_sam_user_info23A(SAM_USER_INFO_23 * usr, NTTIME * logon_time,	/* all zeros */
			   NTTIME * logoff_time,	/* all zeros */
			   NTTIME * kickoff_time,	/* all zeros */
			   NTTIME * pass_last_set_time,	/* all zeros */
			   NTTIME * pass_can_change_time,	/* all zeros */
			   NTTIME * pass_must_change_time,	/* all zeros */
			   char *user_name,	/* NULL */
			   char *full_name,
			   char *home_dir, char *dir_drive, char *log_scr,
			   char *prof_path, const char *desc, char *wkstas,
			   char *unk_str, char *mung_dial, uint32 user_rid,	/* 0x0000 0000 */
			   uint32 group_rid, uint32 acb_info,
			   uint32 fields_present, uint16 logon_divs,
			   LOGON_HRS * hrs, uint16 bad_password_count, uint16 logon_count,
			   char newpass[516])
{
	DATA_BLOB blob = base64_decode_data_blob(mung_dial);
	
	usr->logon_time = *logon_time;	/* all zeros */
	usr->logoff_time = *logoff_time;	/* all zeros */
	usr->kickoff_time = *kickoff_time;	/* all zeros */
	usr->pass_last_set_time = *pass_last_set_time;	/* all zeros */
	usr->pass_can_change_time = *pass_can_change_time;	/* all zeros */
	usr->pass_must_change_time = *pass_must_change_time;	/* all zeros */

	ZERO_STRUCT(usr->nt_pwd);
	ZERO_STRUCT(usr->lm_pwd);

	usr->user_rid = user_rid;	/* 0x0000 0000 */
	usr->group_rid = group_rid;
	usr->acb_info = acb_info;
	usr->fields_present = fields_present;	/* 09f8 27fa */

	usr->logon_divs = logon_divs;	/* should be 168 (hours/week) */
	usr->ptr_logon_hrs = hrs ? 1 : 0;

	if (nt_time_is_zero(pass_must_change_time)) {
		usr->passmustchange=PASS_MUST_CHANGE_AT_NEXT_LOGON;
	} else {
		usr->passmustchange=0;
	}

	ZERO_STRUCT(usr->padding1);
	ZERO_STRUCT(usr->padding2);

	usr->bad_password_count = bad_password_count;
	usr->logon_count = logon_count;

	memcpy(usr->pass, newpass, sizeof(usr->pass));

	init_unistr2(&usr->uni_user_name, user_name, UNI_FLAGS_NONE);
	init_uni_hdr(&usr->hdr_user_name, &usr->uni_user_name);

	init_unistr2(&usr->uni_full_name, full_name, UNI_FLAGS_NONE);
	init_uni_hdr(&usr->hdr_full_name, &usr->uni_full_name);

	init_unistr2(&usr->uni_home_dir, home_dir, UNI_FLAGS_NONE);
	init_uni_hdr(&usr->hdr_home_dir, &usr->uni_home_dir);

	init_unistr2(&usr->uni_dir_drive, dir_drive, UNI_FLAGS_NONE);
	init_uni_hdr(&usr->hdr_dir_drive, &usr->uni_dir_drive);

	init_unistr2(&usr->uni_logon_script, log_scr, UNI_FLAGS_NONE);
	init_uni_hdr(&usr->hdr_logon_script, &usr->uni_logon_script);

	init_unistr2(&usr->uni_profile_path, prof_path, UNI_FLAGS_NONE);
	init_uni_hdr(&usr->hdr_profile_path, &usr->uni_profile_path);

	init_unistr2(&usr->uni_acct_desc, desc, UNI_FLAGS_NONE);
	init_uni_hdr(&usr->hdr_acct_desc, &usr->uni_acct_desc);

	init_unistr2(&usr->uni_workstations, wkstas, UNI_FLAGS_NONE);
	init_uni_hdr(&usr->hdr_workstations, &usr->uni_workstations);

	init_unistr2(&usr->uni_unknown_str, unk_str, UNI_FLAGS_NONE);
	init_uni_hdr(&usr->hdr_unknown_str, &usr->uni_unknown_str);

	init_unistr2_from_datablob(&usr->uni_munged_dial, &blob);
	init_uni_hdr(&usr->hdr_munged_dial, &usr->uni_munged_dial);

	data_blob_free(&blob);
	
	if (hrs) {
		memcpy(&usr->logon_hrs, hrs, sizeof(usr->logon_hrs));
	} else {
		ZERO_STRUCT(usr->logon_hrs);
	}
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_user_info23(const char *desc, SAM_USER_INFO_23 * usr,
			       prs_struct *ps, int depth)
{
	if (usr == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_user_info23");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_time("logon_time           ", &usr->logon_time, ps, depth))
		return False;
	if(!smb_io_time("logoff_time          ", &usr->logoff_time, ps, depth))
		return False;
	if(!smb_io_time("kickoff_time         ", &usr->kickoff_time, ps, depth))
		return False;
	if(!smb_io_time("pass_last_set_time   ", &usr->pass_last_set_time, ps, depth))
		return False;
	if(!smb_io_time("pass_can_change_time ", &usr->pass_can_change_time, ps, depth))
		return False;
	if(!smb_io_time("pass_must_change_time", &usr->pass_must_change_time, ps, depth))
		return False;

	if(!smb_io_unihdr("hdr_user_name   ", &usr->hdr_user_name, ps, depth))	/* username unicode string header */
		return False;
	if(!smb_io_unihdr("hdr_full_name   ", &usr->hdr_full_name, ps, depth))	/* user's full name unicode string header */
		return False;
	if(!smb_io_unihdr("hdr_home_dir    ", &usr->hdr_home_dir, ps, depth))	/* home directory unicode string header */
		return False;
	if(!smb_io_unihdr("hdr_dir_drive   ", &usr->hdr_dir_drive, ps, depth))	/* home directory drive */
		return False;
	if(!smb_io_unihdr("hdr_logon_script", &usr->hdr_logon_script, ps, depth))	/* logon script unicode string header */
		return False;
	if(!smb_io_unihdr("hdr_profile_path", &usr->hdr_profile_path, ps, depth))	/* profile path unicode string header */
		return False;
	if(!smb_io_unihdr("hdr_acct_desc   ", &usr->hdr_acct_desc, ps, depth))	/* account desc */
		return False;
	if(!smb_io_unihdr("hdr_workstations", &usr->hdr_workstations, ps, depth))	/* wkstas user can log on from */
		return False;
	if(!smb_io_unihdr("hdr_unknown_str ", &usr->hdr_unknown_str, ps, depth))	/* unknown string */
		return False;
	if(!smb_io_unihdr("hdr_munged_dial ", &usr->hdr_munged_dial, ps, depth))	/* wkstas user can log on from */
		return False;

	if(!prs_uint8s(False, "lm_pwd        ", ps, depth, usr->lm_pwd, sizeof(usr->lm_pwd)))
		return False;
	if(!prs_uint8s(False, "nt_pwd        ", ps, depth, usr->nt_pwd, sizeof(usr->nt_pwd)))
		return False;

	if(!prs_uint32("user_rid      ", ps, depth, &usr->user_rid))	/* User ID */
		return False;
	if(!prs_uint32("group_rid     ", ps, depth, &usr->group_rid))	/* Group ID */
		return False;
	if(!prs_uint32("acb_info      ", ps, depth, &usr->acb_info))
		return False;

	if(!prs_uint32("fields_present ", ps, depth, &usr->fields_present))
		return False;
	if(!prs_uint16("logon_divs    ", ps, depth, &usr->logon_divs))	/* logon divisions per week */
		return False;
	if(!prs_align(ps))
		return False;
	if(!prs_uint32("ptr_logon_hrs ", ps, depth, &usr->ptr_logon_hrs))
		return False;

	if(!prs_uint16("bad_password_count     ", ps, depth, &usr->bad_password_count))
		return False;
	if(!prs_uint16("logon_count     ", ps, depth, &usr->logon_count))
		return False;

	if(!prs_uint8s(False, "padding1      ", ps, depth, usr->padding1, sizeof(usr->padding1)))
		return False;
	if(!prs_uint8("passmustchange ", ps, depth, &usr->passmustchange))
		return False;
	if(!prs_uint8("padding2       ", ps, depth, &usr->padding2))
		return False;


	if(!prs_uint8s(False, "password      ", ps, depth, usr->pass, sizeof(usr->pass)))
		return False;

	/* here begins pointed-to data */

	if(!smb_io_unistr2("uni_user_name   ", &usr->uni_user_name, usr->hdr_user_name.buffer, ps, depth))	/* username unicode string */
		return False;

	if(!smb_io_unistr2("uni_full_name   ", &usr->uni_full_name, usr->hdr_full_name.buffer, ps, depth))	/* user's full name unicode string */
		return False;

	if(!smb_io_unistr2("uni_home_dir    ", &usr->uni_home_dir, usr->hdr_home_dir.buffer, ps, depth))	/* home directory unicode string */
		return False;

	if(!smb_io_unistr2("uni_dir_drive   ", &usr->uni_dir_drive, usr->hdr_dir_drive.buffer, ps, depth))	/* home directory drive unicode string */
		return False;

	if(!smb_io_unistr2("uni_logon_script", &usr->uni_logon_script, usr->hdr_logon_script.buffer, ps, depth))	/* logon script unicode string */
		return False;

	if(!smb_io_unistr2("uni_profile_path", &usr->uni_profile_path, usr->hdr_profile_path.buffer, ps, depth))	/* profile path unicode string */
		return False;

	if(!smb_io_unistr2("uni_acct_desc   ", &usr->uni_acct_desc, usr->hdr_acct_desc.buffer, ps, depth))	/* user desc unicode string */
		return False;

	if(!smb_io_unistr2("uni_workstations", &usr->uni_workstations, usr->hdr_workstations.buffer, ps, depth))	/* worksations user can log on from */
		return False;

	if(!smb_io_unistr2("uni_unknown_str ", &usr->uni_unknown_str, usr->hdr_unknown_str.buffer, ps, depth))	/* unknown string */
		return False;

	if(!smb_io_unistr2("uni_munged_dial ", &usr->uni_munged_dial, usr->hdr_munged_dial.buffer, ps, depth))
		return False;

	/* ok, this is only guess-work (as usual) */
	if (usr->ptr_logon_hrs) {
		if(!sam_io_logon_hrs("logon_hrs", &usr->logon_hrs, ps, depth))
			return False;
	} 

	return True;
}

/*******************************************************************
 reads or writes a structure.
 NB. This structure is *definately* incorrect. It's my best guess
 currently for W2K SP2. The password field is encrypted in a different
 way than normal... And there are definately other problems. JRA.
********************************************************************/

static BOOL sam_io_user_info25(const char *desc, SAM_USER_INFO_25 * usr, prs_struct *ps, int depth)
{
	if (usr == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_user_info25");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_time("logon_time           ", &usr->logon_time, ps, depth))
		return False;
	if(!smb_io_time("logoff_time          ", &usr->logoff_time, ps, depth))
		return False;
	if(!smb_io_time("kickoff_time         ", &usr->kickoff_time, ps, depth))
		return False;
	if(!smb_io_time("pass_last_set_time   ", &usr->pass_last_set_time, ps, depth))
		return False;
	if(!smb_io_time("pass_can_change_time ", &usr->pass_can_change_time, ps, depth))
		return False;
	if(!smb_io_time("pass_must_change_time", &usr->pass_must_change_time, ps, depth))
		return False;

	if(!smb_io_unihdr("hdr_user_name   ", &usr->hdr_user_name, ps, depth))	/* username unicode string header */
		return False;
	if(!smb_io_unihdr("hdr_full_name   ", &usr->hdr_full_name, ps, depth))	/* user's full name unicode string header */
		return False;
	if(!smb_io_unihdr("hdr_home_dir    ", &usr->hdr_home_dir, ps, depth))	/* home directory unicode string header */
		return False;
	if(!smb_io_unihdr("hdr_dir_drive   ", &usr->hdr_dir_drive, ps, depth))	/* home directory drive */
		return False;
	if(!smb_io_unihdr("hdr_logon_script", &usr->hdr_logon_script, ps, depth))	/* logon script unicode string header */
		return False;
	if(!smb_io_unihdr("hdr_profile_path", &usr->hdr_profile_path, ps, depth))	/* profile path unicode string header */
		return False;
	if(!smb_io_unihdr("hdr_acct_desc   ", &usr->hdr_acct_desc, ps, depth))	/* account desc */
		return False;
	if(!smb_io_unihdr("hdr_workstations", &usr->hdr_workstations, ps, depth))	/* wkstas user can log on from */
		return False;
	if(!smb_io_unihdr("hdr_unknown_str ", &usr->hdr_unknown_str, ps, depth))	/* unknown string */
		return False;
	if(!smb_io_unihdr("hdr_munged_dial ", &usr->hdr_munged_dial, ps, depth))	/* wkstas user can log on from */
		return False;

	if(!prs_uint8s(False, "lm_pwd        ", ps, depth, usr->lm_pwd, sizeof(usr->lm_pwd)))
		return False;
	if(!prs_uint8s(False, "nt_pwd        ", ps, depth, usr->nt_pwd, sizeof(usr->nt_pwd)))
		return False;

	if(!prs_uint32("user_rid      ", ps, depth, &usr->user_rid))	/* User ID */
		return False;
	if(!prs_uint32("group_rid     ", ps, depth, &usr->group_rid))	/* Group ID */
		return False;
	if(!prs_uint32("acb_info      ", ps, depth, &usr->acb_info))
		return False;
	if(!prs_uint32("fields_present ", ps, depth, &usr->fields_present))
		return False;

	if(!prs_uint32s(False, "unknown_5      ", ps, depth, usr->unknown_5, 5))
		return False;

	if(!prs_uint8s(False, "password      ", ps, depth, usr->pass, sizeof(usr->pass)))
		return False;

	/* here begins pointed-to data */

	if(!smb_io_unistr2("uni_user_name   ", &usr->uni_user_name, usr->hdr_user_name.buffer, ps, depth))	/* username unicode string */
		return False;

	if(!smb_io_unistr2("uni_full_name   ", &usr->uni_full_name, usr->hdr_full_name.buffer, ps, depth))	/* user's full name unicode string */
		return False;

	if(!smb_io_unistr2("uni_home_dir    ", &usr->uni_home_dir, usr->hdr_home_dir.buffer, ps, depth))	/* home directory unicode string */
		return False;

	if(!smb_io_unistr2("uni_dir_drive   ", &usr->uni_dir_drive, usr->hdr_dir_drive.buffer, ps, depth))	/* home directory drive unicode string */
		return False;

	if(!smb_io_unistr2("uni_logon_script", &usr->uni_logon_script, usr->hdr_logon_script.buffer, ps, depth))	/* logon script unicode string */
		return False;

	if(!smb_io_unistr2("uni_profile_path", &usr->uni_profile_path, usr->hdr_profile_path.buffer, ps, depth))	/* profile path unicode string */
		return False;

	if(!smb_io_unistr2("uni_acct_desc   ", &usr->uni_acct_desc, usr->hdr_acct_desc.buffer, ps, depth))	/* user desc unicode string */
		return False;

	if(!smb_io_unistr2("uni_workstations", &usr->uni_workstations, usr->hdr_workstations.buffer, ps, depth))	/* worksations user can log on from */
		return False;

	if(!smb_io_unistr2("uni_unknown_str ", &usr->uni_unknown_str, usr->hdr_unknown_str.buffer, ps, depth))	/* unknown string */
		return False;

	if(!smb_io_unistr2("uni_munged_dial ", &usr->uni_munged_dial, usr->hdr_munged_dial.buffer, ps, depth))
		return False;

#if 0 /* JRA - unknown... */
	/* ok, this is only guess-work (as usual) */
	if (usr->ptr_logon_hrs) {
		if(!sam_io_logon_hrs("logon_hrs", &usr->logon_hrs, ps, depth))
			return False;
	} 
#endif

	return True;
}


/*************************************************************************
 init_sam_user_info21W

 unknown_6 = 0x0000 04ec 

 *************************************************************************/

void init_sam_user_info21W(SAM_USER_INFO_21 * usr,
			   NTTIME * logon_time,
			   NTTIME * logoff_time,
			   NTTIME * kickoff_time,
			   NTTIME * pass_last_set_time,
			   NTTIME * pass_can_change_time,
			   NTTIME * pass_must_change_time,
			   UNISTR2 *user_name,
			   UNISTR2 *full_name,
			   UNISTR2 *home_dir,
			   UNISTR2 *dir_drive,
			   UNISTR2 *log_scr,
			   UNISTR2 *prof_path,
			   UNISTR2 *desc,
			   UNISTR2 *wkstas,
			   UNISTR2 *unk_str,
			   UNISTR2 *mung_dial,
			   uchar lm_pwd[16],
			   uchar nt_pwd[16],
			   uint32 user_rid,
			   uint32 group_rid,
			   uint32 acb_info,
			   uint32 fields_present,
			   uint16 logon_divs,
			   LOGON_HRS * hrs,
			   uint16 bad_password_count,
			   uint16 logon_count)
{
	usr->logon_time = *logon_time;
	usr->logoff_time = *logoff_time;
	usr->kickoff_time = *kickoff_time;
	usr->pass_last_set_time = *pass_last_set_time;
	usr->pass_can_change_time = *pass_can_change_time;
	usr->pass_must_change_time = *pass_must_change_time;

	memcpy(usr->lm_pwd, lm_pwd, sizeof(usr->lm_pwd));
	memcpy(usr->nt_pwd, nt_pwd, sizeof(usr->nt_pwd));

	usr->user_rid = user_rid;
	usr->group_rid = group_rid;
	usr->acb_info = acb_info;
	usr->fields_present = fields_present;	/* 0x00ff ffff */

	usr->logon_divs = logon_divs;	/* should be 168 (hours/week) */
	usr->ptr_logon_hrs = hrs ? 1 : 0;
	usr->bad_password_count = bad_password_count;
	usr->logon_count = logon_count;

	if (nt_time_is_zero(pass_must_change_time)) {
		usr->passmustchange=PASS_MUST_CHANGE_AT_NEXT_LOGON;
	} else {
		usr->passmustchange=0;
	}

	ZERO_STRUCT(usr->padding1);
	ZERO_STRUCT(usr->padding2);

	copy_unistr2(&usr->uni_user_name, user_name);
	init_uni_hdr(&usr->hdr_user_name, &usr->uni_user_name);

	copy_unistr2(&usr->uni_full_name, full_name);
	init_uni_hdr(&usr->hdr_full_name, &usr->uni_full_name);

	copy_unistr2(&usr->uni_home_dir, home_dir);
	init_uni_hdr(&usr->hdr_home_dir, &usr->uni_home_dir);

	copy_unistr2(&usr->uni_dir_drive, dir_drive);
	init_uni_hdr(&usr->hdr_dir_drive, &usr->uni_dir_drive);

	copy_unistr2(&usr->uni_logon_script, log_scr);
	init_uni_hdr(&usr->hdr_logon_script, &usr->uni_logon_script);

	copy_unistr2(&usr->uni_profile_path, prof_path);
	init_uni_hdr(&usr->hdr_profile_path, &usr->uni_profile_path);

	copy_unistr2(&usr->uni_acct_desc, desc);
	init_uni_hdr(&usr->hdr_acct_desc, &usr->uni_acct_desc);

	copy_unistr2(&usr->uni_workstations, wkstas);
	init_uni_hdr(&usr->hdr_workstations, &usr->uni_workstations);

	copy_unistr2(&usr->uni_unknown_str, unk_str);
	init_uni_hdr(&usr->hdr_unknown_str, &usr->uni_unknown_str);

	copy_unistr2(&usr->uni_munged_dial, mung_dial);
	init_uni_hdr(&usr->hdr_munged_dial, &usr->uni_munged_dial);

	if (hrs) {
		memcpy(&usr->logon_hrs, hrs, sizeof(usr->logon_hrs));
	} else {
		ZERO_STRUCT(usr->logon_hrs);
	}
}

/*************************************************************************
 init_sam_user_info21

 unknown_6 = 0x0000 04ec 

 *************************************************************************/

NTSTATUS init_sam_user_info21A(SAM_USER_INFO_21 *usr, struct samu *pw, DOM_SID *domain_sid)
{
	NTTIME 		logon_time, logoff_time, kickoff_time,
			pass_last_set_time, pass_can_change_time,
			pass_must_change_time;
			
	const char*		user_name = pdb_get_username(pw);
	const char*		full_name = pdb_get_fullname(pw);
	const char*		home_dir  = pdb_get_homedir(pw);
	const char*		dir_drive = pdb_get_dir_drive(pw);
	const char*		logon_script = pdb_get_logon_script(pw);
	const char*		profile_path = pdb_get_profile_path(pw);
	const char*		description = pdb_get_acct_desc(pw);
	const char*		workstations = pdb_get_workstations(pw);
	const char*		munged_dial = pdb_get_munged_dial(pw);
	DATA_BLOB 		munged_dial_blob;

	uint32 user_rid;
	const DOM_SID *user_sid;

	uint32 group_rid;
	const DOM_SID *group_sid;

	if (munged_dial) {
		munged_dial_blob = base64_decode_data_blob(munged_dial);
	} else {
		munged_dial_blob = data_blob(NULL, 0);
	}

	/* Create NTTIME structs */
	unix_to_nt_time (&logon_time, 		pdb_get_logon_time(pw));
	unix_to_nt_time (&logoff_time, 		pdb_get_logoff_time(pw));
	unix_to_nt_time (&kickoff_time, 	pdb_get_kickoff_time(pw));
	unix_to_nt_time (&pass_last_set_time, 	pdb_get_pass_last_set_time(pw));
	unix_to_nt_time (&pass_can_change_time,	pdb_get_pass_can_change_time(pw));
	unix_to_nt_time (&pass_must_change_time,pdb_get_pass_must_change_time(pw));
	
	/* structure assignment */
	usr->logon_time            = logon_time;
	usr->logoff_time           = logoff_time;
	usr->kickoff_time          = kickoff_time;
	usr->pass_last_set_time    = pass_last_set_time;
	usr->pass_can_change_time  = pass_can_change_time;
	usr->pass_must_change_time = pass_must_change_time;

	ZERO_STRUCT(usr->nt_pwd);
	ZERO_STRUCT(usr->lm_pwd);

	user_sid = pdb_get_user_sid(pw);
	
	if (!sid_peek_check_rid(domain_sid, user_sid, &user_rid)) {
		fstring user_sid_string;
		fstring domain_sid_string;
		DEBUG(0, ("init_sam_user_info_21A: User %s has SID %s, \nwhich conflicts with "
			  "the domain sid %s.  Failing operation.\n", 
			  user_name, 
			  sid_to_string(user_sid_string, user_sid),
			  sid_to_string(domain_sid_string, domain_sid)));
		data_blob_free(&munged_dial_blob);
		return NT_STATUS_UNSUCCESSFUL;
	}

	group_sid = pdb_get_group_sid(pw);
	
	if (!sid_peek_check_rid(domain_sid, group_sid, &group_rid)) {
		fstring group_sid_string;
		fstring domain_sid_string;
		DEBUG(0, ("init_sam_user_info_21A: User %s has Primary Group SID %s, \n"
			  "which conflicts with the domain sid %s.  Failing operation.\n", 
			  user_name, 
			  sid_to_string(group_sid_string, group_sid),
			  sid_to_string(domain_sid_string, domain_sid)));
		data_blob_free(&munged_dial_blob);
		return NT_STATUS_UNSUCCESSFUL;
	}

	usr->user_rid  = user_rid;
	usr->group_rid = group_rid;
	usr->acb_info  = pdb_get_acct_ctrl(pw);

	/*
	  Look at a user on a real NT4 PDC with usrmgr, press
	  'ok'. Then you will see that fields_present is set to
	  0x08f827fa. Look at the user immediately after that again,
	  and you will see that 0x00fffff is returned. This solves
	  the problem that you get access denied after having looked
	  at the user.
	  -- Volker
	*/
	usr->fields_present = pdb_build_fields_present(pw);

	usr->logon_divs = pdb_get_logon_divs(pw); 
	usr->ptr_logon_hrs = pdb_get_hours(pw) ? 1 : 0;
	usr->bad_password_count = pdb_get_bad_password_count(pw);
	usr->logon_count = pdb_get_logon_count(pw);

	if (pdb_get_pass_must_change_time(pw) == 0) {
		usr->passmustchange=PASS_MUST_CHANGE_AT_NEXT_LOGON;
	} else {
		usr->passmustchange=0;
	}

	ZERO_STRUCT(usr->padding1);
	ZERO_STRUCT(usr->padding2);

	init_unistr2(&usr->uni_user_name, user_name, UNI_STR_TERMINATE);
	init_uni_hdr(&usr->hdr_user_name, &usr->uni_user_name);

	init_unistr2(&usr->uni_full_name, full_name, UNI_STR_TERMINATE);
	init_uni_hdr(&usr->hdr_full_name, &usr->uni_full_name);

	init_unistr2(&usr->uni_home_dir, home_dir, UNI_STR_TERMINATE);
	init_uni_hdr(&usr->hdr_home_dir, &usr->uni_home_dir);

	init_unistr2(&usr->uni_dir_drive, dir_drive, UNI_STR_TERMINATE);
	init_uni_hdr(&usr->hdr_dir_drive, &usr->uni_dir_drive);

	init_unistr2(&usr->uni_logon_script, logon_script, UNI_STR_TERMINATE);
	init_uni_hdr(&usr->hdr_logon_script, &usr->uni_logon_script);

	init_unistr2(&usr->uni_profile_path, profile_path, UNI_STR_TERMINATE);
	init_uni_hdr(&usr->hdr_profile_path, &usr->uni_profile_path);

	init_unistr2(&usr->uni_acct_desc, description, UNI_STR_TERMINATE);
	init_uni_hdr(&usr->hdr_acct_desc, &usr->uni_acct_desc);

	init_unistr2(&usr->uni_workstations, workstations, UNI_STR_TERMINATE);
	init_uni_hdr(&usr->hdr_workstations, &usr->uni_workstations);

	init_unistr2(&usr->uni_unknown_str, NULL, UNI_STR_TERMINATE);
	init_uni_hdr(&usr->hdr_unknown_str, &usr->uni_unknown_str);

	init_unistr2_from_datablob(&usr->uni_munged_dial, &munged_dial_blob);
	init_uni_hdr(&usr->hdr_munged_dial, &usr->uni_munged_dial);
	data_blob_free(&munged_dial_blob);

	if (pdb_get_hours(pw)) {
		usr->logon_hrs.max_len = 1260;
		usr->logon_hrs.offset = 0;
		usr->logon_hrs.len = pdb_get_hours_len(pw);
		memcpy(&usr->logon_hrs.hours, pdb_get_hours(pw), MAX_HOURS_LEN);
	} else {
		usr->logon_hrs.max_len = 1260;
		usr->logon_hrs.offset = 0;
		usr->logon_hrs.len = 0;
		memset(&usr->logon_hrs, 0xff, sizeof(usr->logon_hrs));
	}

	return NT_STATUS_OK;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_user_info21(const char *desc, SAM_USER_INFO_21 * usr,
			prs_struct *ps, int depth)
{
	if (usr == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_user_info21");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_time("logon_time           ", &usr->logon_time, ps, depth))
		return False;
	if(!smb_io_time("logoff_time          ", &usr->logoff_time, ps, depth))
		return False;
	if(!smb_io_time("pass_last_set_time   ", &usr->pass_last_set_time, ps,depth))
		return False;
	if(!smb_io_time("kickoff_time         ", &usr->kickoff_time, ps, depth))
		return False;
	if(!smb_io_time("pass_can_change_time ", &usr->pass_can_change_time, ps,depth))
		return False;
	if(!smb_io_time("pass_must_change_time", &usr->pass_must_change_time,  ps, depth))
		return False;

	if(!smb_io_unihdr("hdr_user_name   ", &usr->hdr_user_name, ps, depth))	/* username unicode string header */
		return False;
	if(!smb_io_unihdr("hdr_full_name   ", &usr->hdr_full_name, ps, depth))	/* user's full name unicode string header */
		return False;
	if(!smb_io_unihdr("hdr_home_dir    ", &usr->hdr_home_dir, ps, depth))	/* home directory unicode string header */
		return False;
	if(!smb_io_unihdr("hdr_dir_drive   ", &usr->hdr_dir_drive, ps, depth))	/* home directory drive */
		return False;
	if(!smb_io_unihdr("hdr_logon_script", &usr->hdr_logon_script, ps, depth))	/* logon script unicode string header */
		return False;
	if(!smb_io_unihdr("hdr_profile_path", &usr->hdr_profile_path, ps, depth))	/* profile path unicode string header */
		return False;
	if(!smb_io_unihdr("hdr_acct_desc   ", &usr->hdr_acct_desc, ps, depth))	/* account desc */
		return False;
	if(!smb_io_unihdr("hdr_workstations", &usr->hdr_workstations, ps, depth))	/* wkstas user can log on from */
		return False;
	if(!smb_io_unihdr("hdr_unknown_str ", &usr->hdr_unknown_str, ps, depth))	/* unknown string */
		return False;
	if(!smb_io_unihdr("hdr_munged_dial ", &usr->hdr_munged_dial, ps, depth))	/* wkstas user can log on from */
		return False;

	if(!prs_uint8s(False, "lm_pwd        ", ps, depth, usr->lm_pwd, sizeof(usr->lm_pwd)))
		return False;
	if(!prs_uint8s(False, "nt_pwd        ", ps, depth, usr->nt_pwd, sizeof(usr->nt_pwd)))
		return False;

	if(!prs_uint32("user_rid      ", ps, depth, &usr->user_rid))	/* User ID */
		return False;
	if(!prs_uint32("group_rid     ", ps, depth, &usr->group_rid))	/* Group ID */
		return False;
	if(!prs_uint32("acb_info      ", ps, depth, &usr->acb_info))
		return False;

	if(!prs_uint32("fields_present ", ps, depth, &usr->fields_present))
		return False;
	if(!prs_uint16("logon_divs    ", ps, depth, &usr->logon_divs))	/* logon divisions per week */
		return False;
	if(!prs_align(ps))
		return False;
	if(!prs_uint32("ptr_logon_hrs ", ps, depth, &usr->ptr_logon_hrs))
		return False;

	if(!prs_uint16("bad_password_count     ", ps, depth, &usr->bad_password_count))
		return False;
	if(!prs_uint16("logon_count     ", ps, depth, &usr->logon_count))
		return False;

	if(!prs_uint8s(False, "padding1      ", ps, depth, usr->padding1, sizeof(usr->padding1)))
		return False;
	if(!prs_uint8("passmustchange ", ps, depth, &usr->passmustchange))
		return False;
	if(!prs_uint8("padding2       ", ps, depth, &usr->padding2))
		return False;

	/* here begins pointed-to data */

	if(!smb_io_unistr2("uni_user_name   ", &usr->uni_user_name,usr->hdr_user_name.buffer, ps, depth))	/* username unicode string */
		return False;
	if(!smb_io_unistr2("uni_full_name   ", &usr->uni_full_name, usr->hdr_full_name.buffer, ps, depth))	/* user's full name unicode string */
		return False;
	if(!smb_io_unistr2("uni_home_dir    ", &usr->uni_home_dir, usr->hdr_home_dir.buffer, ps, depth))	/* home directory unicode string */
		return False;
	if(!smb_io_unistr2("uni_dir_drive   ", &usr->uni_dir_drive, usr->hdr_dir_drive.buffer, ps, depth))	/* home directory drive unicode string */
		return False;
	if(!smb_io_unistr2("uni_logon_script", &usr->uni_logon_script, usr->hdr_logon_script.buffer, ps, depth))	/* logon script unicode string */
		return False;
	if(!smb_io_unistr2("uni_profile_path", &usr->uni_profile_path, usr->hdr_profile_path.buffer, ps, depth))	/* profile path unicode string */
		return False;
	if(!smb_io_unistr2("uni_acct_desc   ", &usr->uni_acct_desc, usr->hdr_acct_desc.buffer, ps, depth))	/* user desc unicode string */
		return False;
	if(!smb_io_unistr2("uni_workstations", &usr->uni_workstations, usr->hdr_workstations.buffer, ps, depth))	/* worksations user can log on from */
		return False;
	if(!smb_io_unistr2("uni_unknown_str ", &usr->uni_unknown_str, usr->hdr_unknown_str.buffer, ps, depth))	/* unknown string */
		return False;
	if(!smb_io_unistr2("uni_munged_dial ", &usr->uni_munged_dial,usr->hdr_munged_dial.buffer, ps, depth))	/* worksations user can log on from */
		return False;

	/* ok, this is only guess-work (as usual) */
	if (usr->ptr_logon_hrs) {
		if(!sam_io_logon_hrs("logon_hrs", &usr->logon_hrs, ps, depth))
			return False;
	}

	return True;
}

void init_sam_user_info20A(SAM_USER_INFO_20 *usr, struct samu *pw)
{
	const char *munged_dial = pdb_get_munged_dial(pw);
	DATA_BLOB blob;

	if (munged_dial) {
		blob = base64_decode_data_blob(munged_dial);
	} else {
		blob = data_blob(NULL, 0);
	}

	init_unistr2_from_datablob(&usr->uni_munged_dial, &blob);
	init_uni_hdr(&usr->hdr_munged_dial, &usr->uni_munged_dial);
	data_blob_free(&blob);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL sam_io_user_info20(const char *desc, SAM_USER_INFO_20 *usr,
			prs_struct *ps, int depth)
{
	if (usr == NULL)
		return False;

	prs_debug(ps, depth, desc, "sam_io_user_info20");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_unihdr("hdr_munged_dial ", &usr->hdr_munged_dial, ps, depth))	/* wkstas user can log on from */
		return False;

	if(!smb_io_unistr2("uni_munged_dial ", &usr->uni_munged_dial,usr->hdr_munged_dial.buffer, ps, depth))	/* worksations user can log on from */
		return False;

	return True;
}

/*******************************************************************
inits a SAM_USERINFO_CTR structure.
********************************************************************/

NTSTATUS make_samr_userinfo_ctr_usr21(TALLOC_CTX *ctx, SAM_USERINFO_CTR * ctr,
				    uint16 switch_value,
				    SAM_USER_INFO_21 * usr)
{
	DEBUG(5, ("make_samr_userinfo_ctr_usr21\n"));

	ctr->switch_value = switch_value;
	ctr->info.id = NULL;

	switch (switch_value) {
	case 16:
		ctr->info.id16 = TALLOC_ZERO_P(ctx,SAM_USER_INFO_16);
		if (ctr->info.id16 == NULL)
			return NT_STATUS_NO_MEMORY;

		init_sam_user_info16(ctr->info.id16, usr->acb_info);
		break;
#if 0
/* whoops - got this wrong.  i think.  or don't understand what's happening. */
	case 17:
		{
			NTTIME expire;
			info = (void *)&id11;

			expire.low = 0xffffffff;
			expire.high = 0x7fffffff;

			ctr->info.id = TALLOC_ZERO_P(ctx,SAM_USER_INFO_17);
			init_sam_user_info11(ctr->info.id17, &expire,
					     "BROOKFIELDS$",	/* name */
					     0x03ef,	/* user rid */
					     0x201,	/* group rid */
					     0x0080);	/* acb info */

			break;
		}
#endif
	case 18:
		ctr->info.id18 = TALLOC_ZERO_P(ctx,SAM_USER_INFO_18);
		if (ctr->info.id18 == NULL)
			return NT_STATUS_NO_MEMORY;

		init_sam_user_info18(ctr->info.id18, usr->lm_pwd, usr->nt_pwd);
		break;
	case 21:
		{
			SAM_USER_INFO_21 *cusr;
			cusr = TALLOC_ZERO_P(ctx,SAM_USER_INFO_21);
			ctr->info.id21 = cusr;
			if (ctr->info.id21 == NULL)
				return NT_STATUS_NO_MEMORY;
			memcpy(cusr, usr, sizeof(*usr));
			memset(cusr->lm_pwd, 0, sizeof(cusr->lm_pwd));
			memset(cusr->nt_pwd, 0, sizeof(cusr->nt_pwd));
			break;
		}
	default:
		DEBUG(4,("make_samr_userinfo_ctr: unsupported info\n"));
		return NT_STATUS_INVALID_INFO_CLASS;
	}

	return NT_STATUS_OK;
}

/*******************************************************************
inits a SAM_USERINFO_CTR structure.
********************************************************************/

static void init_samr_userinfo_ctr(SAM_USERINFO_CTR * ctr, DATA_BLOB *sess_key,
				   uint16 switch_value, void *info)
{
	DEBUG(5, ("init_samr_userinfo_ctr\n"));

	ctr->switch_value = switch_value;
	ctr->info.id = info;

	switch (switch_value) {
	case 0x18:
		SamOEMhashBlob(ctr->info.id24->pass, 516, sess_key);
		dump_data(100, (char *)sess_key->data, sess_key->length);
		dump_data(100, (char *)ctr->info.id24->pass, 516);
		break;
	case 0x17:
		SamOEMhashBlob(ctr->info.id23->pass, 516, sess_key);
		dump_data(100, (char *)sess_key->data, sess_key->length);
		dump_data(100, (char *)ctr->info.id23->pass, 516);
		break;
	case 0x07:
		break;
	default:
		DEBUG(4,("init_samr_userinfo_ctr: unsupported switch level: %d\n", switch_value));
	}
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

static BOOL samr_io_userinfo_ctr(const char *desc, SAM_USERINFO_CTR **ppctr,
				 prs_struct *ps, int depth)
{
	BOOL ret;
	SAM_USERINFO_CTR *ctr;

	prs_debug(ps, depth, desc, "samr_io_userinfo_ctr");
	depth++;

	if (UNMARSHALLING(ps)) {
		ctr = PRS_ALLOC_MEM(ps,SAM_USERINFO_CTR,1);
		if (ctr == NULL)
			return False;
		*ppctr = ctr;
	} else {
		ctr = *ppctr;
	}

	/* lkclXXXX DO NOT ALIGN BEFORE READING SWITCH VALUE! */

	if(!prs_uint16("switch_value", ps, depth, &ctr->switch_value))
		return False;
	if(!prs_align(ps))
		return False;

	ret = False;

	switch (ctr->switch_value) {
	case 7:
		if (UNMARSHALLING(ps))
			ctr->info.id7 = PRS_ALLOC_MEM(ps,SAM_USER_INFO_7,1);
		if (ctr->info.id7 == NULL) {
			DEBUG(2,("samr_io_userinfo_ctr: info pointer not initialised\n"));
			return False;
		}
		ret = sam_io_user_info7("", ctr->info.id7, ps, depth);
		break;
	case 9:
		if (UNMARSHALLING(ps))
			ctr->info.id9 = PRS_ALLOC_MEM(ps,SAM_USER_INFO_9,1);
		if (ctr->info.id9 == NULL) {
			DEBUG(2,("samr_io_userinfo_ctr: info pointer not initialised\n"));
			return False;
		}
		ret = sam_io_user_info9("", ctr->info.id9, ps, depth);
		break;
	case 16:
		if (UNMARSHALLING(ps))
			ctr->info.id16 = PRS_ALLOC_MEM(ps,SAM_USER_INFO_16,1);
		if (ctr->info.id16 == NULL) {
			DEBUG(2,("samr_io_userinfo_ctr: info pointer not initialised\n"));
			return False;
		}
		ret = sam_io_user_info16("", ctr->info.id16, ps, depth);
		break;
	case 17:
		if (UNMARSHALLING(ps))
			ctr->info.id17 = PRS_ALLOC_MEM(ps,SAM_USER_INFO_17,1);

		if (ctr->info.id17 == NULL) {
			DEBUG(2,("samr_io_userinfo_ctr: info pointer not initialised\n"));
			return False;
		}
		ret = sam_io_user_info17("", ctr->info.id17, ps, depth);
		break;
	case 18:
		if (UNMARSHALLING(ps))
			ctr->info.id18 = PRS_ALLOC_MEM(ps,SAM_USER_INFO_18,1);

		if (ctr->info.id18 == NULL) {
			DEBUG(2,("samr_io_userinfo_ctr: info pointer not initialised\n"));
			return False;
		}
		ret = sam_io_user_info18("", ctr->info.id18, ps, depth);
		break;
	case 20:
		if (UNMARSHALLING(ps))
			ctr->info.id20 = PRS_ALLOC_MEM(ps,SAM_USER_INFO_20,1);

		if (ctr->info.id20 == NULL) {
			DEBUG(2,("samr_io_userinfo_ctr: info pointer not initialised\n"));
			return False;
		}
		ret = sam_io_user_info20("", ctr->info.id20, ps, depth);
		break;
	case 21:
		if (UNMARSHALLING(ps))
			ctr->info.id21 = PRS_ALLOC_MEM(ps,SAM_USER_INFO_21,1);

		if (ctr->info.id21 == NULL) {
			DEBUG(2,("samr_io_userinfo_ctr: info pointer not initialised\n"));
			return False;
		}
		ret = sam_io_user_info21("", ctr->info.id21, ps, depth);
		break;
	case 23:
		if (UNMARSHALLING(ps))
			ctr->info.id23 = PRS_ALLOC_MEM(ps,SAM_USER_INFO_23,1);

		if (ctr->info.id23 == NULL) {
			DEBUG(2,("samr_io_userinfo_ctr: info pointer not initialised\n"));
			return False;
		}
		ret = sam_io_user_info23("", ctr->info.id23, ps, depth);
		break;
	case 24:
		if (UNMARSHALLING(ps))
			ctr->info.id24 = PRS_ALLOC_MEM(ps,SAM_USER_INFO_24,1);

		if (ctr->info.id24 == NULL) {
			DEBUG(2,("samr_io_userinfo_ctr: info pointer not initialised\n"));
			return False;
		}
		ret = sam_io_user_info24("", ctr->info.id24, ps,  depth);
		break;
	case 25:
		if (UNMARSHALLING(ps))
			ctr->info.id25 = PRS_ALLOC_MEM(ps,SAM_USER_INFO_25,1);

		if (ctr->info.id25 == NULL) {
			DEBUG(2,("samr_io_userinfo_ctr: info pointer not initialised\n"));
			return False;
		}
		ret = sam_io_user_info25("", ctr->info.id25, ps, depth);
		break;
	case 26:
		if (UNMARSHALLING(ps))
			ctr->info.id26 = PRS_ALLOC_MEM(ps,SAM_USER_INFO_26,1);

		if (ctr->info.id26 == NULL) {
			DEBUG(2,("samr_io_userinfo_ctr: info pointer not initialised\n"));
			return False;
		}
		ret = sam_io_user_info26("", ctr->info.id26, ps,  depth);
		break;
	default:
		DEBUG(2, ("samr_io_userinfo_ctr: unknown switch level 0x%x\n", ctr->switch_value));
		ret = False;
		break;
	}

	return ret;
}

/*******************************************************************
inits a SAMR_R_QUERY_USERINFO structure.
********************************************************************/

void init_samr_r_query_userinfo(SAMR_R_QUERY_USERINFO * r_u,
				SAM_USERINFO_CTR * ctr, NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_query_userinfo\n"));

	r_u->ptr = 0;
	r_u->ctr = NULL;

	if (NT_STATUS_IS_OK(status)) {
		r_u->ptr = 1;
		r_u->ctr = ctr;
	}

	r_u->status = status;	/* return status */
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_query_userinfo(const char *desc, SAMR_R_QUERY_USERINFO * r_u,
			      prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_query_userinfo");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr", ps, depth, &r_u->ptr))
		return False;

	if (r_u->ptr != 0) {
		if(!samr_io_userinfo_ctr("ctr", &r_u->ctr, ps, depth))
			return False;
	}

	if(!prs_align(ps))
		return False;
	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_SET_USERINFO structure.
********************************************************************/

void init_samr_q_set_userinfo(SAMR_Q_SET_USERINFO * q_u,
			      const POLICY_HND *hnd, DATA_BLOB *sess_key,
			      uint16 switch_value, void *info)
{
	DEBUG(5, ("init_samr_q_set_userinfo\n"));

	q_u->pol = *hnd;
	q_u->switch_value = switch_value;
	init_samr_userinfo_ctr(q_u->ctr, sess_key, switch_value, info);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_set_userinfo(const char *desc, SAMR_Q_SET_USERINFO * q_u,
			    prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_set_userinfo");
	depth++;

	if(!prs_align(ps))
		return False;

	smb_io_pol_hnd("pol", &(q_u->pol), ps, depth);

	if(!prs_uint16("switch_value", ps, depth, &q_u->switch_value))
		return False;
	if(!samr_io_userinfo_ctr("ctr", &q_u->ctr, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_R_SET_USERINFO structure.
********************************************************************/

void init_samr_r_set_userinfo(SAMR_R_SET_USERINFO * r_u, NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_set_userinfo\n"));

	r_u->status = status;	/* return status */
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_set_userinfo(const char *desc, SAMR_R_SET_USERINFO * r_u,
			    prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_set_userinfo");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_SET_USERINFO2 structure.
********************************************************************/

void init_samr_q_set_userinfo2(SAMR_Q_SET_USERINFO2 * q_u,
			       const POLICY_HND *hnd, DATA_BLOB *sess_key,
			       uint16 switch_value, SAM_USERINFO_CTR * ctr)
{
	DEBUG(5, ("init_samr_q_set_userinfo2\n"));

	q_u->pol = *hnd;
	q_u->switch_value = switch_value;
	q_u->ctr = ctr;

	q_u->ctr->switch_value = switch_value;

	switch (switch_value) {
	case 18:
		SamOEMhashBlob(ctr->info.id18->lm_pwd, 16, sess_key);
		SamOEMhashBlob(ctr->info.id18->nt_pwd, 16, sess_key);
		dump_data(100, (char *)sess_key->data, sess_key->length);
		dump_data(100, (char *)ctr->info.id18->lm_pwd, 16);
		dump_data(100, (char *)ctr->info.id18->nt_pwd, 16);
		break;
	}
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_set_userinfo2(const char *desc, SAMR_Q_SET_USERINFO2 * q_u,
			     prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_set_userinfo2");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("pol", &q_u->pol, ps, depth))
		return False;

	if(!prs_uint16("switch_value", ps, depth, &q_u->switch_value))
		return False;
	if(!samr_io_userinfo_ctr("ctr", &q_u->ctr, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_R_SET_USERINFO2 structure.
********************************************************************/

void init_samr_r_set_userinfo2(SAMR_R_SET_USERINFO2 * r_u, NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_set_userinfo2\n"));

	r_u->status = status;	/* return status */
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_set_userinfo2(const char *desc, SAMR_R_SET_USERINFO2 * r_u,
			     prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_set_userinfo2");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_CONNECT structure.
********************************************************************/

void init_samr_q_connect(SAMR_Q_CONNECT * q_u,
			 char *srv_name, uint32 access_mask)
{
	DEBUG(5, ("init_samr_q_connect\n"));

	/* make PDC server name \\server */
	q_u->ptr_srv_name = (srv_name != NULL && *srv_name) ? 1 : 0;
	init_unistr2(&q_u->uni_srv_name, srv_name, UNI_STR_TERMINATE);

	/* example values: 0x0000 0002 */
	q_u->access_mask = access_mask;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_connect(const char *desc, SAMR_Q_CONNECT * q_u,
		       prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_connect");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_srv_name", ps, depth, &q_u->ptr_srv_name))
		return False;
	if(!smb_io_unistr2("", &q_u->uni_srv_name, q_u->ptr_srv_name, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;
	if(!prs_uint32("access_mask", ps, depth, &q_u->access_mask))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_connect(const char *desc, SAMR_R_CONNECT * r_u,
		       prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_connect");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("connect_pol", &r_u->connect_pol, ps, depth))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_CONNECT4 structure.
********************************************************************/

void init_samr_q_connect4(SAMR_Q_CONNECT4 * q_u,
			  char *srv_name, uint32 access_mask)
{
	DEBUG(5, ("init_samr_q_connect4\n"));

	/* make PDC server name \\server */
	q_u->ptr_srv_name = (srv_name != NULL && *srv_name) ? 1 : 0;
	init_unistr2(&q_u->uni_srv_name, srv_name, UNI_STR_TERMINATE);

	/* Only value we've seen, possibly an address type ? */
	q_u->unk_0 = 2;

	/* example values: 0x0000 0002 */
	q_u->access_mask = access_mask;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_connect4(const char *desc, SAMR_Q_CONNECT4 * q_u,
			prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_connect4");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_srv_name", ps, depth, &q_u->ptr_srv_name))
		return False;
	if(!smb_io_unistr2("", &q_u->uni_srv_name, q_u->ptr_srv_name, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;
	if(!prs_uint32("unk_0", ps, depth, &q_u->unk_0))
		return False;
	if(!prs_uint32("access_mask", ps, depth, &q_u->access_mask))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_connect4(const char *desc, SAMR_R_CONNECT4 * r_u,
			prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_connect4");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("connect_pol", &r_u->connect_pol, ps, depth))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_CONNECT5 structure.
********************************************************************/

void init_samr_q_connect5(SAMR_Q_CONNECT5 * q_u,
			  char *srv_name, uint32 access_mask)
{
	DEBUG(5, ("init_samr_q_connect5\n"));

	/* make PDC server name \\server */
	q_u->ptr_srv_name = (srv_name != NULL && *srv_name) ? 1 : 0;
	init_unistr2(&q_u->uni_srv_name, srv_name, UNI_STR_TERMINATE);

	/* example values: 0x0000 0002 */
	q_u->access_mask = access_mask;

	q_u->level = 1;
	q_u->info1_unk1 = 3;
	q_u->info1_unk2 = 0;
}

/*******************************************************************
inits a SAMR_R_CONNECT5 structure.
********************************************************************/

void init_samr_r_connect5(SAMR_R_CONNECT5 * r_u, POLICY_HND *pol, NTSTATUS status)
{
	DEBUG(5, ("init_samr_q_connect5\n"));

	r_u->level = 1;
	r_u->info1_unk1 = 3;
	r_u->info1_unk2 = 0;

	r_u->connect_pol = *pol;
	r_u->status = status;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_connect5(const char *desc, SAMR_Q_CONNECT5 * q_u,
			prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_connect5");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_srv_name", ps, depth, &q_u->ptr_srv_name))
		return False;
	if(!smb_io_unistr2("", &q_u->uni_srv_name, q_u->ptr_srv_name, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;
	if(!prs_uint32("access_mask", ps, depth, &q_u->access_mask))
		return False;

	if(!prs_uint32("level", ps, depth, &q_u->level))
		return False;
	if(!prs_uint32("level", ps, depth, &q_u->level))
		return False;
	
	if(!prs_uint32("info1_unk1", ps, depth, &q_u->info1_unk1))
		return False;
	if(!prs_uint32("info1_unk2", ps, depth, &q_u->info1_unk2))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_connect5(const char *desc, SAMR_R_CONNECT5 * r_u,
			prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_connect5");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("level", ps, depth, &r_u->level))
		return False;
	if(!prs_uint32("level", ps, depth, &r_u->level))
		return False;
	if(!prs_uint32("info1_unk1", ps, depth, &r_u->info1_unk1))
		return False;
	if(!prs_uint32("info1_unk2", ps, depth, &r_u->info1_unk2))
		return False;

	if(!smb_io_pol_hnd("connect_pol", &r_u->connect_pol, ps, depth))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_CONNECT_ANON structure.
********************************************************************/

void init_samr_q_connect_anon(SAMR_Q_CONNECT_ANON * q_u)
{
	DEBUG(5, ("init_samr_q_connect_anon\n"));

	q_u->ptr = 1;
	q_u->unknown_0 = 0x5c;	/* server name (?!!) */
	q_u->access_mask = MAXIMUM_ALLOWED_ACCESS;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_connect_anon(const char *desc, SAMR_Q_CONNECT_ANON * q_u,
			    prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_connect_anon");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr      ", ps, depth, &q_u->ptr))
		return False;
	if (q_u->ptr) {
		if(!prs_uint16("unknown_0", ps, depth, &q_u->unknown_0))
			return False;
	}
	if(!prs_align(ps))
		return False;
	if(!prs_uint32("access_mask", ps, depth, &q_u->access_mask))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_connect_anon(const char *desc, SAMR_R_CONNECT_ANON * r_u,
			    prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_connect_anon");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("connect_pol", &r_u->connect_pol, ps, depth))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_GET_DOM_PWINFO structure.
********************************************************************/

void init_samr_q_get_dom_pwinfo(SAMR_Q_GET_DOM_PWINFO * q_u,
				char *srv_name)
{
	DEBUG(5, ("init_samr_q_get_dom_pwinfo\n"));

	q_u->ptr = 1;
	init_unistr2(&q_u->uni_srv_name, srv_name, UNI_FLAGS_NONE);
	init_uni_hdr(&q_u->hdr_srv_name, &q_u->uni_srv_name);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_get_dom_pwinfo(const char *desc, SAMR_Q_GET_DOM_PWINFO * q_u,
			      prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_get_dom_pwinfo");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr", ps, depth, &q_u->ptr))
		return False;
	if (q_u->ptr != 0) {
		if(!smb_io_unihdr("", &q_u->hdr_srv_name, ps, depth))
			return False;
		if(!smb_io_unistr2("", &q_u->uni_srv_name, q_u->hdr_srv_name.buffer, ps, depth))
			return False;
	}

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_get_dom_pwinfo(const char *desc, SAMR_R_GET_DOM_PWINFO * r_u,
			      prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_get_dom_pwinfo");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint16("min_pwd_length", ps, depth, &r_u->min_pwd_length))
		return False;
	if(!prs_align(ps))
		return False;
	if(!prs_uint32("password_properties", ps, depth, &r_u->password_properties))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
make a SAMR_ENC_PASSWD structure.
********************************************************************/

void init_enc_passwd(SAMR_ENC_PASSWD * pwd, const char pass[512])
{
	ZERO_STRUCTP(pwd);

	if (pass == NULL) {
		pwd->ptr = 0;
	} else {
		pwd->ptr = 1;
		memcpy(pwd->pass, pass, sizeof(pwd->pass));
	}
}

/*******************************************************************
reads or writes a SAMR_ENC_PASSWD structure.
********************************************************************/

BOOL samr_io_enc_passwd(const char *desc, SAMR_ENC_PASSWD * pwd,
			prs_struct *ps, int depth)
{
	if (pwd == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_enc_passwd");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr", ps, depth, &pwd->ptr))
		return False;

	if (pwd->ptr != 0) {
		if(!prs_uint8s(False, "pwd", ps, depth, pwd->pass, sizeof(pwd->pass)))
			return False;
	}

	return True;
}

/*******************************************************************
inits a SAMR_ENC_HASH structure.
********************************************************************/

void init_enc_hash(SAMR_ENC_HASH * hsh, const uchar hash[16])
{
	ZERO_STRUCTP(hsh);

	if (hash == NULL) {
		hsh->ptr = 0;
	} else {
		hsh->ptr = 1;
		memcpy(hsh->hash, hash, sizeof(hsh->hash));
	}
}

/*******************************************************************
reads or writes a SAMR_ENC_HASH structure.
********************************************************************/

BOOL samr_io_enc_hash(const char *desc, SAMR_ENC_HASH * hsh,
		      prs_struct *ps, int depth)
{
	if (hsh == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_enc_hash");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr ", ps, depth, &hsh->ptr))
		return False;
	if (hsh->ptr != 0) {
		if(!prs_uint8s(False, "hash", ps, depth, hsh->hash,sizeof(hsh->hash)))
			return False;
	}

	return True;
}

/*******************************************************************
inits a SAMR_Q_CHGPASSWD_USER structure.
********************************************************************/

void init_samr_q_chgpasswd_user(SAMR_Q_CHGPASSWD_USER * q_u,
				const char *dest_host, const char *user_name,
				const uchar nt_newpass[516],
				const uchar nt_oldhash[16],
				const uchar lm_newpass[516],
				const uchar lm_oldhash[16])
{
	DEBUG(5, ("init_samr_q_chgpasswd_user\n"));

	q_u->ptr_0 = 1;
	init_unistr2(&q_u->uni_dest_host, dest_host, UNI_FLAGS_NONE);
	init_uni_hdr(&q_u->hdr_dest_host, &q_u->uni_dest_host);

	init_unistr2(&q_u->uni_user_name, user_name, UNI_FLAGS_NONE);
	init_uni_hdr(&q_u->hdr_user_name, &q_u->uni_user_name);

	init_enc_passwd(&q_u->nt_newpass, (const char *)nt_newpass);
	init_enc_hash(&q_u->nt_oldhash, nt_oldhash);

	q_u->unknown = 0x01;

	init_enc_passwd(&q_u->lm_newpass, (const char *)lm_newpass);
	init_enc_hash(&q_u->lm_oldhash, lm_oldhash);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_chgpasswd_user(const char *desc, SAMR_Q_CHGPASSWD_USER * q_u,
			      prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_chgpasswd_user");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_0", ps, depth, &q_u->ptr_0))
		return False;

	if(!smb_io_unihdr("", &q_u->hdr_dest_host, ps, depth))
		return False;
	if(!smb_io_unistr2("", &q_u->uni_dest_host, q_u->hdr_dest_host.buffer, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;
	if(!smb_io_unihdr("", &q_u->hdr_user_name, ps, depth))
		return False;
	if(!smb_io_unistr2("", &q_u->uni_user_name, q_u->hdr_user_name.buffer,ps, depth))
		return False;

	if(!samr_io_enc_passwd("nt_newpass", &q_u->nt_newpass, ps, depth))
		return False;
	if(!samr_io_enc_hash("nt_oldhash", &q_u->nt_oldhash, ps, depth))
		return False;

	if(!prs_uint32("unknown", ps, depth, &q_u->unknown))
		return False;

	if(!samr_io_enc_passwd("lm_newpass", &q_u->lm_newpass, ps, depth))
		return False;
	if(!samr_io_enc_hash("lm_oldhash", &q_u->lm_oldhash, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_R_CHGPASSWD_USER structure.
********************************************************************/

void init_samr_r_chgpasswd_user(SAMR_R_CHGPASSWD_USER * r_u, NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_chgpasswd_user\n"));

	r_u->status = status;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_chgpasswd_user(const char *desc, SAMR_R_CHGPASSWD_USER * r_u,
			      prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_chgpasswd_user");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_Q_CHGPASSWD3 structure.
********************************************************************/

void init_samr_q_chgpasswd_user3(SAMR_Q_CHGPASSWD_USER3 * q_u,
				 const char *dest_host, const char *user_name,
				 const uchar nt_newpass[516],
				 const uchar nt_oldhash[16],
				 const uchar lm_newpass[516],
				 const uchar lm_oldhash[16])
{
	DEBUG(5, ("init_samr_q_chgpasswd_user3\n"));

	q_u->ptr_0 = 1;
	init_unistr2(&q_u->uni_dest_host, dest_host, UNI_FLAGS_NONE);
	init_uni_hdr(&q_u->hdr_dest_host, &q_u->uni_dest_host);

	init_unistr2(&q_u->uni_user_name, user_name, UNI_FLAGS_NONE);
	init_uni_hdr(&q_u->hdr_user_name, &q_u->uni_user_name);

	init_enc_passwd(&q_u->nt_newpass, (const char *)nt_newpass);
	init_enc_hash(&q_u->nt_oldhash, nt_oldhash);

	q_u->lm_change = 0x01;

	init_enc_passwd(&q_u->lm_newpass, (const char *)lm_newpass);
	init_enc_hash(&q_u->lm_oldhash, lm_oldhash);

	init_enc_passwd(&q_u->password3, NULL);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_chgpasswd_user3(const char *desc, SAMR_Q_CHGPASSWD_USER3 * q_u,
			       prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_chgpasswd_user3");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_0", ps, depth, &q_u->ptr_0))
		return False;

	if(!smb_io_unihdr("", &q_u->hdr_dest_host, ps, depth))
		return False;
	if(!smb_io_unistr2("", &q_u->uni_dest_host, q_u->hdr_dest_host.buffer, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;
	if(!smb_io_unihdr("", &q_u->hdr_user_name, ps, depth))
		return False;
	if(!smb_io_unistr2("", &q_u->uni_user_name, q_u->hdr_user_name.buffer,ps, depth))
		return False;

	if(!samr_io_enc_passwd("nt_newpass", &q_u->nt_newpass, ps, depth))
		return False;
	if(!samr_io_enc_hash("nt_oldhash", &q_u->nt_oldhash, ps, depth))
		return False;

	if(!prs_uint32("lm_change", ps, depth, &q_u->lm_change))
		return False;

	if(!samr_io_enc_passwd("lm_newpass", &q_u->lm_newpass, ps, depth))
		return False;
	if(!samr_io_enc_hash("lm_oldhash", &q_u->lm_oldhash, ps, depth))
		return False;

	if(!samr_io_enc_passwd("password3", &q_u->password3, ps, depth))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_R_CHGPASSWD_USER3 structure.
********************************************************************/

void init_samr_r_chgpasswd_user3(SAMR_R_CHGPASSWD_USER3 *r_u, NTSTATUS status, 
				 SAMR_CHANGE_REJECT *reject, SAM_UNK_INFO_1 *info)
{
	DEBUG(5, ("init_samr_r_chgpasswd_user3\n"));

	r_u->status = status;
	r_u->info = 0;
	r_u->ptr_info = 0;
	r_u->reject = 0;
	r_u->ptr_reject = 0;

	if (info) {
		r_u->info = info;
		r_u->ptr_info = 1;
	}
	if (reject && (reject->reject_reason != Undefined)) {
		r_u->reject = reject;
		r_u->ptr_reject = 1;
	}
}

/*******************************************************************
 Reads or writes an SAMR_CHANGE_REJECT structure.
********************************************************************/

BOOL samr_io_change_reject(const char *desc, SAMR_CHANGE_REJECT *reject, prs_struct *ps, int depth)
{
	if (reject == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_change_reject");
	depth++;

	if(!prs_align(ps))
		return False;

	if(UNMARSHALLING(ps))
		ZERO_STRUCTP(reject);
	
	if (!prs_uint32("reject_reason", ps, depth, &reject->reject_reason))
		return False;
		
	if (!prs_uint32("unknown1", ps, depth, &reject->unknown1))
		return False;

	if (!prs_uint32("unknown2", ps, depth, &reject->unknown2))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_chgpasswd_user3(const char *desc, SAMR_R_CHGPASSWD_USER3 *r_u,
			       prs_struct *ps, int depth)
{
	if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_chgpasswd_user3");
	depth++;

	if (!prs_align(ps))
		return False;

	if (!prs_uint32("ptr_info", ps, depth, &r_u->ptr_info))
		return False;

	if (r_u->ptr_info && r_u->info != NULL) {
		/* SAM_UNK_INFO_1 */
		if (!sam_io_unk_info1("info", r_u->info, ps, depth))
			return False;
	}

	if (!prs_uint32("ptr_reject", ps, depth, &r_u->ptr_reject))
		return False;
			     
	if (r_u->ptr_reject && r_u->reject != NULL) {
		/* SAMR_CHANGE_REJECT */
		if (!samr_io_change_reject("reject", r_u->reject, ps, depth))
			return False;
	}

	if (!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

void init_samr_q_query_domain_info2(SAMR_Q_QUERY_DOMAIN_INFO2 *q_u,
				POLICY_HND *domain_pol, uint16 switch_value)
{
	DEBUG(5, ("init_samr_q_query_domain_info2\n"));

	q_u->domain_pol = *domain_pol;
	q_u->switch_value = switch_value;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_query_domain_info2(const char *desc, SAMR_Q_QUERY_DOMAIN_INFO2 *q_u,
			      prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_query_domain_info2");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("domain_pol", &q_u->domain_pol, ps, depth))
		return False;

	if(!prs_uint16("switch_value", ps, depth, &q_u->switch_value))
		return False;

	return True;
}

/*******************************************************************
inits a SAMR_R_QUERY_DOMAIN_INFO structure.
********************************************************************/

void init_samr_r_query_domain_info2(SAMR_R_QUERY_DOMAIN_INFO2 * r_u,
				    uint16 switch_value, SAM_UNK_CTR * ctr,
				    NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_query_domain_info2\n"));

	r_u->ptr_0 = 0;
	r_u->switch_value = 0;
	r_u->status = status;	/* return status */

	if (NT_STATUS_IS_OK(status)) {
		r_u->switch_value = switch_value;
		r_u->ptr_0 = 1;
		r_u->ctr = ctr;
	}
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_query_domain_info2(const char *desc, SAMR_R_QUERY_DOMAIN_INFO2 * r_u,
				  prs_struct *ps, int depth)
{
        if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_query_domain_info2");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("ptr_0 ", ps, depth, &r_u->ptr_0))
		return False;

	if (r_u->ptr_0 != 0 && r_u->ctr != NULL) {
		if(!prs_uint16("switch_value", ps, depth, &r_u->switch_value))
			return False;
		if(!prs_align(ps))
			return False;

		switch (r_u->switch_value) {
		case 0x0d:
			if(!sam_io_unk_info13("unk_inf13", &r_u->ctr->info.inf13, ps, depth))
				return False;
			break;
		case 0x0c:
			if(!sam_io_unk_info12("unk_inf12", &r_u->ctr->info.inf12, ps, depth))
				return False;
			break;
		case 0x09:
			if(!sam_io_unk_info9("unk_inf9",&r_u->ctr->info.inf9, ps,depth))
				return False;
			break;
		case 0x08:
			if(!sam_io_unk_info8("unk_inf8",&r_u->ctr->info.inf8, ps,depth))
				return False;
			break;
		case 0x07:
			if(!sam_io_unk_info7("unk_inf7",&r_u->ctr->info.inf7, ps,depth))
				return False;
			break;
		case 0x06:
			if(!sam_io_unk_info6("unk_inf6",&r_u->ctr->info.inf6, ps,depth))
				return False;
			break;
		case 0x05:
			if(!sam_io_unk_info5("unk_inf5",&r_u->ctr->info.inf5, ps,depth))
				return False;
			break;
		case 0x04:
			if(!sam_io_unk_info4("unk_inf4",&r_u->ctr->info.inf4, ps,depth))
				return False;
			break;
		case 0x03:
			if(!sam_io_unk_info3("unk_inf3",&r_u->ctr->info.inf3, ps,depth))
				return False;
			break;
		case 0x02:
			if(!sam_io_unk_info2("unk_inf2",&r_u->ctr->info.inf2, ps,depth))
				return False;
			break;
		case 0x01:
			if(!sam_io_unk_info1("unk_inf1",&r_u->ctr->info.inf1, ps,depth))
				return False;
			break;
		default:
			DEBUG(0, ("samr_io_r_query_domain_info2: unknown switch level 0x%x\n",
				r_u->switch_value));
			r_u->status = NT_STATUS_INVALID_INFO_CLASS;
			return False;
		}
	}
	
	if(!prs_align(ps))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;
	
	return True;
}


/*******************************************************************
reads or writes a structure.
********************************************************************/

void init_samr_q_set_domain_info(SAMR_Q_SET_DOMAIN_INFO *q_u,
				POLICY_HND *domain_pol, uint16 switch_value, SAM_UNK_CTR *ctr)
{
	DEBUG(5, ("init_samr_q_set_domain_info\n"));

	q_u->domain_pol = *domain_pol;
	q_u->switch_value0 = switch_value;

	q_u->switch_value = switch_value;
	q_u->ctr = ctr;
	
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_q_set_domain_info(const char *desc, SAMR_Q_SET_DOMAIN_INFO *q_u,
			      prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_q_set_domain_info");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("domain_pol", &q_u->domain_pol, ps, depth))
		return False;

	if(!prs_uint16("switch_value0", ps, depth, &q_u->switch_value0))
		return False;

	if(!prs_uint16("switch_value", ps, depth, &q_u->switch_value))
		return False;

	if(!prs_align(ps))
		return False;

	if (UNMARSHALLING(ps)) {
		if ((q_u->ctr = PRS_ALLOC_MEM(ps, SAM_UNK_CTR, 1)) == NULL)
			return False;
	}
	
	switch (q_u->switch_value) {

	case 0x0c:
		if(!sam_io_unk_info12("unk_inf12", &q_u->ctr->info.inf12, ps, depth))
			return False;
		break;
	case 0x07:
		if(!sam_io_unk_info7("unk_inf7",&q_u->ctr->info.inf7, ps,depth))
			return False;
		break;
	case 0x06:
		if(!sam_io_unk_info6("unk_inf6",&q_u->ctr->info.inf6, ps,depth))
			return False;
		break;
	case 0x05:
		if(!sam_io_unk_info5("unk_inf5",&q_u->ctr->info.inf5, ps,depth))
			return False;
		break;
	case 0x03:
		if(!sam_io_unk_info3("unk_inf3",&q_u->ctr->info.inf3, ps,depth))
			return False;
		break;
	case 0x02:
		if(!sam_io_unk_info2("unk_inf2",&q_u->ctr->info.inf2, ps,depth))
			return False;
		break;
	case 0x01:
		if(!sam_io_unk_info1("unk_inf1",&q_u->ctr->info.inf1, ps,depth))
			return False;
		break;
	default:
		DEBUG(0, ("samr_io_r_samr_unknown_2e: unknown switch level 0x%x\n",
			q_u->switch_value));
		return False;
	}

	return True;
}

/*******************************************************************
inits a SAMR_R_QUERY_DOMAIN_INFO structure.
********************************************************************/

void init_samr_r_set_domain_info(SAMR_R_SET_DOMAIN_INFO * r_u, NTSTATUS status)
{
	DEBUG(5, ("init_samr_r_set_domain_info\n"));

	r_u->status = status;	/* return status */
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL samr_io_r_set_domain_info(const char *desc, SAMR_R_SET_DOMAIN_INFO * r_u,
			      prs_struct *ps, int depth)
{
        if (r_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "samr_io_r_samr_unknown_2e");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_ntstatus("status", ps, depth, &r_u->status))
		return False;
	
	return True;
}
