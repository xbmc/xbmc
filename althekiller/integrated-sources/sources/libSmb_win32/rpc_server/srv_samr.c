/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Andrew Tridgell              1992-1997,
 *  Copyright (C) Luke Kenneth Casson Leighton 1996-1997,
 *  Copyright (C) Paul Ashton                       1997,
 *  Copyright (C) Marc Jacobsen			    1999,
 *  Copyright (C) Jean François Micouleau      1998-2001,
 *  Copyright (C) Jim McDonough <jmcd@us.ibm.com> 2002-2003.
 *	
 * 	Split into interface and implementation modules by, 
 *
 *  Copyright (C) Jeremy Allison                    2001.
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

/*
 * This is the interface to the SAMR code.
 */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

/*******************************************************************
 api_samr_close_hnd
 ********************************************************************/

static BOOL api_samr_close_hnd(pipes_struct *p)
{
	SAMR_Q_CLOSE_HND q_u;
	SAMR_R_CLOSE_HND r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!samr_io_q_close_hnd("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_close_hnd: unable to unmarshall SAMR_Q_CLOSE_HND.\n"));
		return False;
	}

	r_u.status = _samr_close_hnd(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_close_hnd("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_close_hnd: unable to marshall SAMR_R_CLOSE_HND.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_open_domain
 ********************************************************************/

static BOOL api_samr_open_domain(pipes_struct *p)
{
	SAMR_Q_OPEN_DOMAIN q_u;
	SAMR_R_OPEN_DOMAIN r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!samr_io_q_open_domain("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_open_domain: unable to unmarshall SAMR_Q_OPEN_DOMAIN.\n"));
		return False;
	}

	r_u.status = _samr_open_domain(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_open_domain("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_open_domain: unable to marshall SAMR_R_OPEN_DOMAIN.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_get_usrdom_pwinfo
 ********************************************************************/

static BOOL api_samr_get_usrdom_pwinfo(pipes_struct *p)
{
	SAMR_Q_GET_USRDOM_PWINFO q_u;
	SAMR_R_GET_USRDOM_PWINFO r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!samr_io_q_get_usrdom_pwinfo("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_get_usrdom_pwinfo: unable to unmarshall SAMR_Q_GET_USRDOM_PWINFO.\n"));
		return False;
	}

	r_u.status = _samr_get_usrdom_pwinfo(p, &q_u, &r_u);

	if(!samr_io_r_get_usrdom_pwinfo("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_get_usrdom_pwinfo: unable to marshall SAMR_R_GET_USRDOM_PWINFO.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_set_sec_obj
 ********************************************************************/

static BOOL api_samr_set_sec_obj(pipes_struct *p)
{
	SAMR_Q_SET_SEC_OBJ q_u;
	SAMR_R_SET_SEC_OBJ r_u;
	
	prs_struct *data  = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;
	
	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);
	
	if(!samr_io_q_set_sec_obj("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_set_sec_obj: unable to unmarshall SAMR_Q_SET_SEC_OBJ.\n"));
		return False;
	}

	r_u.status = _samr_set_sec_obj(p, &q_u, &r_u);

	if(!samr_io_r_set_sec_obj("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_set_sec_obj: unable to marshall SAMR_R_SET_SEC_OBJ.\n"));
		return False;
	}
	
	return True;
}

/*******************************************************************
 api_samr_query_sec_obj
 ********************************************************************/

static BOOL api_samr_query_sec_obj(pipes_struct *p)
{
	SAMR_Q_QUERY_SEC_OBJ q_u;
	SAMR_R_QUERY_SEC_OBJ r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!samr_io_q_query_sec_obj("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_query_sec_obj: unable to unmarshall SAMR_Q_QUERY_SEC_OBJ.\n"));
		return False;
	}

	r_u.status = _samr_query_sec_obj(p, &q_u, &r_u);

	if(!samr_io_r_query_sec_obj("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_query_sec_obj: unable to marshall SAMR_R_QUERY_SEC_OBJ.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_enum_dom_users
 ********************************************************************/

static BOOL api_samr_enum_dom_users(pipes_struct *p)
{
	SAMR_Q_ENUM_DOM_USERS q_u;
	SAMR_R_ENUM_DOM_USERS r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the samr open */
	if(!samr_io_q_enum_dom_users("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_enum_dom_users: unable to unmarshall SAMR_Q_ENUM_DOM_USERS.\n"));
		return False;
	}

	r_u.status = _samr_enum_dom_users(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_enum_dom_users("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_enum_dom_users: unable to marshall SAMR_R_ENUM_DOM_USERS.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_enum_dom_groups
 ********************************************************************/

static BOOL api_samr_enum_dom_groups(pipes_struct *p)
{
	SAMR_Q_ENUM_DOM_GROUPS q_u;
	SAMR_R_ENUM_DOM_GROUPS r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the samr open */
	if(!samr_io_q_enum_dom_groups("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_enum_dom_groups: unable to unmarshall SAMR_Q_ENUM_DOM_GROUPS.\n"));
		return False;
	}

	r_u.status = _samr_enum_dom_groups(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_enum_dom_groups("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_enum_dom_groups: unable to marshall SAMR_R_ENUM_DOM_GROUPS.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_enum_dom_aliases
 ********************************************************************/

static BOOL api_samr_enum_dom_aliases(pipes_struct *p)
{
	SAMR_Q_ENUM_DOM_ALIASES q_u;
	SAMR_R_ENUM_DOM_ALIASES r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;
	
	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the samr open */
	if(!samr_io_q_enum_dom_aliases("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_enum_dom_aliases: unable to unmarshall SAMR_Q_ENUM_DOM_ALIASES.\n"));
		return False;
	}

	r_u.status = _samr_enum_dom_aliases(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_enum_dom_aliases("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_enum_dom_aliases: unable to marshall SAMR_R_ENUM_DOM_ALIASES.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_query_dispinfo
 ********************************************************************/

static BOOL api_samr_query_dispinfo(pipes_struct *p)
{
	SAMR_Q_QUERY_DISPINFO q_u;
	SAMR_R_QUERY_DISPINFO r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!samr_io_q_query_dispinfo("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_query_dispinfo: unable to unmarshall SAMR_Q_QUERY_DISPINFO.\n"));
		return False;
	}

	r_u.status = _samr_query_dispinfo(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_query_dispinfo("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_query_dispinfo: unable to marshall SAMR_R_QUERY_DISPINFO.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_query_aliasinfo
 ********************************************************************/

static BOOL api_samr_query_aliasinfo(pipes_struct *p)
{
	SAMR_Q_QUERY_ALIASINFO q_u;
	SAMR_R_QUERY_ALIASINFO r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the samr open */
	if(!samr_io_q_query_aliasinfo("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_query_aliasinfo: unable to unmarshall SAMR_Q_QUERY_ALIASINFO.\n"));
		return False;
	}

	r_u.status = _samr_query_aliasinfo(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_query_aliasinfo("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_query_aliasinfo: unable to marshall SAMR_R_QUERY_ALIASINFO.\n"));
		return False;
	}
  
	return True;
}

/*******************************************************************
 api_samr_lookup_names
 ********************************************************************/

static BOOL api_samr_lookup_names(pipes_struct *p)
{
	SAMR_Q_LOOKUP_NAMES q_u;
	SAMR_R_LOOKUP_NAMES r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the samr lookup names */
	if(!samr_io_q_lookup_names("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_lookup_names: unable to unmarshall SAMR_Q_LOOKUP_NAMES.\n"));
		return False;
	}

	r_u.status = _samr_lookup_names(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_lookup_names("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_lookup_names: unable to marshall SAMR_R_LOOKUP_NAMES.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_chgpasswd_user
 ********************************************************************/

static BOOL api_samr_chgpasswd_user(pipes_struct *p)
{
	SAMR_Q_CHGPASSWD_USER q_u;
	SAMR_R_CHGPASSWD_USER r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* change password request */
	if (!samr_io_q_chgpasswd_user("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_chgpasswd_user: Failed to unmarshall SAMR_Q_CHGPASSWD_USER.\n"));
		return False;
	}

	r_u.status = _samr_chgpasswd_user(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_chgpasswd_user("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_chgpasswd_user: Failed to marshall SAMR_R_CHGPASSWD_USER.\n" ));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_lookup_rids
 ********************************************************************/

static BOOL api_samr_lookup_rids(pipes_struct *p)
{
	SAMR_Q_LOOKUP_RIDS q_u;
	SAMR_R_LOOKUP_RIDS r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the samr lookup names */
	if(!samr_io_q_lookup_rids("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_lookup_rids: unable to unmarshall SAMR_Q_LOOKUP_RIDS.\n"));
		return False;
	}

	r_u.status = _samr_lookup_rids(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_lookup_rids("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_lookup_rids: unable to marshall SAMR_R_LOOKUP_RIDS.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_open_user
 ********************************************************************/

static BOOL api_samr_open_user(pipes_struct *p)
{
	SAMR_Q_OPEN_USER q_u;
	SAMR_R_OPEN_USER r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!samr_io_q_open_user("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_open_user: unable to unmarshall SAMR_Q_OPEN_USER.\n"));
		return False;
	}

	r_u.status = _samr_open_user(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_open_user("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_open_user: unable to marshall SAMR_R_OPEN_USER.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_query_userinfo
 ********************************************************************/

static BOOL api_samr_query_userinfo(pipes_struct *p)
{
	SAMR_Q_QUERY_USERINFO q_u;
	SAMR_R_QUERY_USERINFO r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!samr_io_q_query_userinfo("", &q_u, data, 0)){
		DEBUG(0,("api_samr_query_userinfo: unable to unmarshall SAMR_Q_QUERY_USERINFO.\n"));
		return False;
	}

	r_u.status = _samr_query_userinfo(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_query_userinfo("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_query_userinfo: unable to marshall SAMR_R_QUERY_USERINFO.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_query_usergroups
 ********************************************************************/

static BOOL api_samr_query_usergroups(pipes_struct *p)
{
	SAMR_Q_QUERY_USERGROUPS q_u;
	SAMR_R_QUERY_USERGROUPS r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!samr_io_q_query_usergroups("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_query_usergroups: unable to unmarshall SAMR_Q_QUERY_USERGROUPS.\n"));
		return False;
	}

	r_u.status = _samr_query_usergroups(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_query_usergroups("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_query_usergroups: unable to marshall SAMR_R_QUERY_USERGROUPS.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_query_domain_info
 ********************************************************************/

static BOOL api_samr_query_domain_info(pipes_struct *p)
{
	SAMR_Q_QUERY_DOMAIN_INFO q_u;
	SAMR_R_QUERY_DOMAIN_INFO r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!samr_io_q_query_domain_info("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_query_domain_info: unable to unmarshall SAMR_Q_QUERY_DOMAIN_INFO.\n"));
		return False;
	}

	r_u.status = _samr_query_domain_info(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_query_domain_info("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_query_domain_info: unable to marshall SAMR_R_QUERY_DOMAIN_INFO.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_create_user
 ********************************************************************/

static BOOL api_samr_create_user(pipes_struct *p)
{
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	SAMR_Q_CREATE_USER q_u;
	SAMR_R_CREATE_USER r_u;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the samr create user */
	if (!samr_io_q_create_user("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_create_user: Unable to unmarshall SAMR_Q_CREATE_USER.\n"));
		return False;
	}

	r_u.status=_samr_create_user(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_create_user("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_create_user: Unable to marshall SAMR_R_CREATE_USER.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_connect_anon
 ********************************************************************/

static BOOL api_samr_connect_anon(pipes_struct *p)
{
	SAMR_Q_CONNECT_ANON q_u;
	SAMR_R_CONNECT_ANON r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the samr open policy */
	if(!samr_io_q_connect_anon("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_connect_anon: unable to unmarshall SAMR_Q_CONNECT_ANON.\n"));
		return False;
	}

	r_u.status = _samr_connect_anon(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_connect_anon("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_connect_anon: unable to marshall SAMR_R_CONNECT_ANON.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_connect
 ********************************************************************/

static BOOL api_samr_connect(pipes_struct *p)
{
	SAMR_Q_CONNECT q_u;
	SAMR_R_CONNECT r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the samr open policy */
	if(!samr_io_q_connect("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_connect: unable to unmarshall SAMR_Q_CONNECT.\n"));
		return False;
	}

	r_u.status = _samr_connect(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_connect("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_connect: unable to marshall SAMR_R_CONNECT.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_connect4
 ********************************************************************/

static BOOL api_samr_connect4(pipes_struct *p)
{
	SAMR_Q_CONNECT4 q_u;
	SAMR_R_CONNECT4 r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the samr open policy */
	if(!samr_io_q_connect4("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_connect4: unable to unmarshall SAMR_Q_CONNECT4.\n"));
		return False;
	}

	r_u.status = _samr_connect4(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_connect4("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_connect4: unable to marshall SAMR_R_CONNECT4.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_chgpasswd_user3
 ********************************************************************/

static BOOL api_samr_chgpasswd_user3(pipes_struct *p)
{
	SAMR_Q_CHGPASSWD_USER3 q_u;
	SAMR_R_CHGPASSWD_USER3 r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* change password request */
	if (!samr_io_q_chgpasswd_user3("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_chgpasswd_user3: Failed to unmarshall SAMR_Q_CHGPASSWD_USER3.\n"));
		return False;
	}

	r_u.status = _samr_chgpasswd_user3(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_chgpasswd_user3("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_chgpasswd_user3: Failed to marshall SAMR_R_CHGPASSWD_USER3.\n" ));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_connect5
 ********************************************************************/

static BOOL api_samr_connect5(pipes_struct *p)
{
	SAMR_Q_CONNECT5 q_u;
	SAMR_R_CONNECT5 r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the samr open policy */
	if(!samr_io_q_connect5("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_connect5: unable to unmarshall SAMR_Q_CONNECT5.\n"));
		return False;
	}

	r_u.status = _samr_connect5(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_connect5("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_connect5: unable to marshall SAMR_R_CONNECT5.\n"));
		return False;
	}

	return True;
}

/**********************************************************************
 api_samr_lookup_domain
 **********************************************************************/

static BOOL api_samr_lookup_domain(pipes_struct *p)
{
	SAMR_Q_LOOKUP_DOMAIN q_u;
	SAMR_R_LOOKUP_DOMAIN r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;
  
	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!samr_io_q_lookup_domain("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_lookup_domain: Unable to unmarshall SAMR_Q_LOOKUP_DOMAIN.\n"));
		return False;
	}

	r_u.status = _samr_lookup_domain(p, &q_u, &r_u);
	
	if(!samr_io_r_lookup_domain("", &r_u, rdata, 0)){
		DEBUG(0,("api_samr_lookup_domain: Unable to marshall SAMR_R_LOOKUP_DOMAIN.\n"));
		return False;
	}
	
	return True;
}

/**********************************************************************
 api_samr_enum_domains
 **********************************************************************/

static BOOL api_samr_enum_domains(pipes_struct *p)
{
	SAMR_Q_ENUM_DOMAINS q_u;
	SAMR_R_ENUM_DOMAINS r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;
  
	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!samr_io_q_enum_domains("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_enum_domains: Unable to unmarshall SAMR_Q_ENUM_DOMAINS.\n"));
		return False;
	}

	r_u.status = _samr_enum_domains(p, &q_u, &r_u);

	if(!samr_io_r_enum_domains("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_enum_domains: Unable to marshall SAMR_R_ENUM_DOMAINS.\n"));
		return False;
	}
	
	return True;
}

/*******************************************************************
 api_samr_open_alias
 ********************************************************************/

static BOOL api_samr_open_alias(pipes_struct *p)
{
	SAMR_Q_OPEN_ALIAS q_u;
	SAMR_R_OPEN_ALIAS r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the samr open policy */
	if(!samr_io_q_open_alias("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_open_alias: Unable to unmarshall SAMR_Q_OPEN_ALIAS.\n"));
		return False;
	}

	r_u.status=_samr_open_alias(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_open_alias("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_open_alias: Unable to marshall SAMR_R_OPEN_ALIAS.\n"));
		return False;
	}
	
	return True;
}

/*******************************************************************
 api_samr_set_userinfo
 ********************************************************************/

static BOOL api_samr_set_userinfo(pipes_struct *p)
{
	SAMR_Q_SET_USERINFO q_u;
	SAMR_R_SET_USERINFO r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!samr_io_q_set_userinfo("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_set_userinfo: Unable to unmarshall SAMR_Q_SET_USERINFO.\n"));
		/* Fix for W2K SP2 */
		/* what is that status-code ? - gd */
		if (q_u.switch_value == 0x1a) {
			setup_fault_pdu(p, NT_STATUS(DCERPC_FAULT_INVALID_TAG));
			return True;
		}
		return False;
	}

	r_u.status = _samr_set_userinfo(p, &q_u, &r_u);

	if(!samr_io_r_set_userinfo("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_set_userinfo: Unable to marshall SAMR_R_SET_USERINFO.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_set_userinfo2
 ********************************************************************/

static BOOL api_samr_set_userinfo2(pipes_struct *p)
{
	SAMR_Q_SET_USERINFO2 q_u;
	SAMR_R_SET_USERINFO2 r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!samr_io_q_set_userinfo2("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_set_userinfo2: Unable to unmarshall SAMR_Q_SET_USERINFO2.\n"));
		return False;
	}

	r_u.status = _samr_set_userinfo2(p, &q_u, &r_u);

	if(!samr_io_r_set_userinfo2("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_set_userinfo2: Unable to marshall SAMR_R_SET_USERINFO2.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_query_useraliases
 ********************************************************************/

static BOOL api_samr_query_useraliases(pipes_struct *p)
{
	SAMR_Q_QUERY_USERALIASES q_u;
	SAMR_R_QUERY_USERALIASES r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!samr_io_q_query_useraliases("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_query_useraliases:  Unable to unmarshall SAMR_Q_QUERY_USERALIASES.\n"));
		return False;
	}

	r_u.status = _samr_query_useraliases(p, &q_u, &r_u);

	if (! samr_io_r_query_useraliases("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_query_useraliases:  Unable to nmarshall SAMR_R_QUERY_USERALIASES.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_query_aliasmem
 ********************************************************************/

static BOOL api_samr_query_aliasmem(pipes_struct *p)
{
	SAMR_Q_QUERY_ALIASMEM q_u;
	SAMR_R_QUERY_ALIASMEM r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!samr_io_q_query_aliasmem("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_query_aliasmem: unable to unmarshall SAMR_Q_QUERY_ALIASMEM.\n"));
		return False;
	}

	r_u.status = _samr_query_aliasmem(p, &q_u, &r_u);

	if (!samr_io_r_query_aliasmem("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_query_aliasmem: unable to marshall SAMR_R_QUERY_ALIASMEM.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_query_groupmem
 ********************************************************************/

static BOOL api_samr_query_groupmem(pipes_struct *p)
{
	SAMR_Q_QUERY_GROUPMEM q_u;
	SAMR_R_QUERY_GROUPMEM r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!samr_io_q_query_groupmem("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_query_groupmem: unable to unmarshall SAMR_Q_QUERY_GROUPMEM.\n"));
		return False;
	}

	r_u.status = _samr_query_groupmem(p, &q_u, &r_u);

	if (!samr_io_r_query_groupmem("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_query_groupmem: unable to marshall SAMR_R_QUERY_GROUPMEM.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_add_aliasmem
 ********************************************************************/

static BOOL api_samr_add_aliasmem(pipes_struct *p)
{
	SAMR_Q_ADD_ALIASMEM q_u;
	SAMR_R_ADD_ALIASMEM r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!samr_io_q_add_aliasmem("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_add_aliasmem: unable to unmarshall SAMR_Q_ADD_ALIASMEM.\n"));
		return False;
	}

	r_u.status = _samr_add_aliasmem(p, &q_u, &r_u);

	if (!samr_io_r_add_aliasmem("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_add_aliasmem: unable to marshall SAMR_R_ADD_ALIASMEM.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_del_aliasmem
 ********************************************************************/

static BOOL api_samr_del_aliasmem(pipes_struct *p)
{
	SAMR_Q_DEL_ALIASMEM q_u;
	SAMR_R_DEL_ALIASMEM r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!samr_io_q_del_aliasmem("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_del_aliasmem: unable to unmarshall SAMR_Q_DEL_ALIASMEM.\n"));
		return False;
	}

	r_u.status = _samr_del_aliasmem(p, &q_u, &r_u);

	if (!samr_io_r_del_aliasmem("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_del_aliasmem: unable to marshall SAMR_R_DEL_ALIASMEM.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_add_groupmem
 ********************************************************************/

static BOOL api_samr_add_groupmem(pipes_struct *p)
{
	SAMR_Q_ADD_GROUPMEM q_u;
	SAMR_R_ADD_GROUPMEM r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!samr_io_q_add_groupmem("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_add_groupmem: unable to unmarshall SAMR_Q_ADD_GROUPMEM.\n"));
		return False;
	}

	r_u.status = _samr_add_groupmem(p, &q_u, &r_u);

	if (!samr_io_r_add_groupmem("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_add_groupmem: unable to marshall SAMR_R_ADD_GROUPMEM.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_del_groupmem
 ********************************************************************/

static BOOL api_samr_del_groupmem(pipes_struct *p)
{
	SAMR_Q_DEL_GROUPMEM q_u;
	SAMR_R_DEL_GROUPMEM r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!samr_io_q_del_groupmem("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_del_groupmem: unable to unmarshall SAMR_Q_DEL_GROUPMEM.\n"));
		return False;
	}

	r_u.status = _samr_del_groupmem(p, &q_u, &r_u);

	if (!samr_io_r_del_groupmem("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_del_groupmem: unable to marshall SAMR_R_DEL_GROUPMEM.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_delete_dom_user
 ********************************************************************/

static BOOL api_samr_delete_dom_user(pipes_struct *p)
{
	SAMR_Q_DELETE_DOM_USER q_u;
	SAMR_R_DELETE_DOM_USER r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!samr_io_q_delete_dom_user("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_delete_dom_user: unable to unmarshall SAMR_Q_DELETE_DOM_USER.\n"));
		return False;
	}

	r_u.status = _samr_delete_dom_user(p, &q_u, &r_u);

	if (!samr_io_r_delete_dom_user("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_delete_dom_user: unable to marshall SAMR_R_DELETE_DOM_USER.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_delete_dom_group
 ********************************************************************/

static BOOL api_samr_delete_dom_group(pipes_struct *p)
{
	SAMR_Q_DELETE_DOM_GROUP q_u;
	SAMR_R_DELETE_DOM_GROUP r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!samr_io_q_delete_dom_group("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_delete_dom_group: unable to unmarshall SAMR_Q_DELETE_DOM_GROUP.\n"));
		return False;
	}

	r_u.status = _samr_delete_dom_group(p, &q_u, &r_u);

	if (!samr_io_r_delete_dom_group("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_delete_dom_group: unable to marshall SAMR_R_DELETE_DOM_GROUP.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_delete_dom_alias
 ********************************************************************/

static BOOL api_samr_delete_dom_alias(pipes_struct *p)
{
	SAMR_Q_DELETE_DOM_ALIAS q_u;
	SAMR_R_DELETE_DOM_ALIAS r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!samr_io_q_delete_dom_alias("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_delete_dom_alias: unable to unmarshall SAMR_Q_DELETE_DOM_ALIAS.\n"));
		return False;
	}

	r_u.status = _samr_delete_dom_alias(p, &q_u, &r_u);

	if (!samr_io_r_delete_dom_alias("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_delete_dom_alias: unable to marshall SAMR_R_DELETE_DOM_ALIAS.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_create_dom_group
 ********************************************************************/

static BOOL api_samr_create_dom_group(pipes_struct *p)
{
	SAMR_Q_CREATE_DOM_GROUP q_u;
	SAMR_R_CREATE_DOM_GROUP r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!samr_io_q_create_dom_group("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_create_dom_group: unable to unmarshall SAMR_Q_CREATE_DOM_GROUP.\n"));
		return False;
	}

	r_u.status = _samr_create_dom_group(p, &q_u, &r_u);

	if (!samr_io_r_create_dom_group("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_create_dom_group: unable to marshall SAMR_R_CREATE_DOM_GROUP.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_create_dom_alias
 ********************************************************************/

static BOOL api_samr_create_dom_alias(pipes_struct *p)
{
	SAMR_Q_CREATE_DOM_ALIAS q_u;
	SAMR_R_CREATE_DOM_ALIAS r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!samr_io_q_create_dom_alias("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_create_dom_alias: unable to unmarshall SAMR_Q_CREATE_DOM_ALIAS.\n"));
		return False;
	}

	r_u.status = _samr_create_dom_alias(p, &q_u, &r_u);

	if (!samr_io_r_create_dom_alias("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_create_dom_alias: unable to marshall SAMR_R_CREATE_DOM_ALIAS.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_query_groupinfo
 ********************************************************************/

static BOOL api_samr_query_groupinfo(pipes_struct *p)
{
	SAMR_Q_QUERY_GROUPINFO q_u;
	SAMR_R_QUERY_GROUPINFO r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!samr_io_q_query_groupinfo("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_query_groupinfo: unable to unmarshall SAMR_Q_QUERY_GROUPINFO.\n"));
		return False;
	}

	r_u.status = _samr_query_groupinfo(p, &q_u, &r_u);

	if (!samr_io_r_query_groupinfo("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_query_groupinfo: unable to marshall SAMR_R_QUERY_GROUPINFO.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_set_groupinfo
 ********************************************************************/

static BOOL api_samr_set_groupinfo(pipes_struct *p)
{
	SAMR_Q_SET_GROUPINFO q_u;
	SAMR_R_SET_GROUPINFO r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!samr_io_q_set_groupinfo("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_set_groupinfo: unable to unmarshall SAMR_Q_SET_GROUPINFO.\n"));
		return False;
	}

	r_u.status = _samr_set_groupinfo(p, &q_u, &r_u);

	if (!samr_io_r_set_groupinfo("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_set_groupinfo: unable to marshall SAMR_R_SET_GROUPINFO.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_set_aliasinfo
 ********************************************************************/

static BOOL api_samr_set_aliasinfo(pipes_struct *p)
{
	SAMR_Q_SET_ALIASINFO q_u;
	SAMR_R_SET_ALIASINFO r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!samr_io_q_set_aliasinfo("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_set_aliasinfo: unable to unmarshall SAMR_Q_SET_ALIASINFO.\n"));
		return False;
	}

	r_u.status = _samr_set_aliasinfo(p, &q_u, &r_u);

	if (!samr_io_r_set_aliasinfo("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_set_aliasinfo: unable to marshall SAMR_R_SET_ALIASINFO.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_get_dom_pwinfo
 ********************************************************************/

static BOOL api_samr_get_dom_pwinfo(pipes_struct *p)
{
	SAMR_Q_GET_DOM_PWINFO q_u;
	SAMR_R_GET_DOM_PWINFO r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!samr_io_q_get_dom_pwinfo("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_get_dom_pwinfo: unable to unmarshall SAMR_Q_GET_DOM_PWINFO.\n"));
		return False;
	}

	r_u.status = _samr_get_dom_pwinfo(p, &q_u, &r_u);

	if (!samr_io_r_get_dom_pwinfo("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_get_dom_pwinfo: unable to marshall SAMR_R_GET_DOM_PWINFO.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_open_group
 ********************************************************************/

static BOOL api_samr_open_group(pipes_struct *p)
{
	SAMR_Q_OPEN_GROUP q_u;
	SAMR_R_OPEN_GROUP r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!samr_io_q_open_group("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_open_group: unable to unmarshall SAMR_Q_OPEN_GROUP.\n"));
		return False;
	}

	r_u.status = _samr_open_group(p, &q_u, &r_u);

	if (!samr_io_r_open_group("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_open_group: unable to marshall SAMR_R_OPEN_GROUP.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_remove_sid_foreign_domain
 ********************************************************************/

static BOOL api_samr_remove_sid_foreign_domain(pipes_struct *p)
{
	SAMR_Q_REMOVE_SID_FOREIGN_DOMAIN q_u;
	SAMR_R_REMOVE_SID_FOREIGN_DOMAIN r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!samr_io_q_remove_sid_foreign_domain("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_remove_sid_foreign_domain: unable to unmarshall SAMR_Q_REMOVE_SID_FOREIGN_DOMAIN.\n"));
		return False;
	}

	r_u.status = _samr_remove_sid_foreign_domain(p, &q_u, &r_u);

	if (!samr_io_r_remove_sid_foreign_domain("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_remove_sid_foreign_domain: unable to marshall SAMR_R_REMOVE_SID_FOREIGN_DOMAIN.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_query_dom_info2
 ********************************************************************/

static BOOL api_samr_query_domain_info2(pipes_struct *p)
{
	SAMR_Q_QUERY_DOMAIN_INFO2 q_u;
	SAMR_R_QUERY_DOMAIN_INFO2 r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!samr_io_q_query_domain_info2("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_query_domain_info2: unable to unmarshall SAMR_Q_QUERY_DOMAIN_INFO2.\n"));
		return False;
	}

	r_u.status = _samr_query_domain_info2(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_query_domain_info2("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_query_domain_info2: unable to marshall SAMR_R_QUERY_DOMAIN_INFO2.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 api_samr_set_dom_info
 ********************************************************************/

static BOOL api_samr_set_dom_info(pipes_struct *p)
{
	SAMR_Q_SET_DOMAIN_INFO q_u;
	SAMR_R_SET_DOMAIN_INFO r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!samr_io_q_set_domain_info("", &q_u, data, 0)) {
		DEBUG(0,("api_samr_set_dom_info: unable to unmarshall SAMR_Q_SET_DOMAIN_INFO.\n"));
		return False;
	}

	r_u.status = _samr_set_dom_info(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!samr_io_r_set_domain_info("", &r_u, rdata, 0)) {
		DEBUG(0,("api_samr_set_dom_info: unable to marshall SAMR_R_SET_DOMAIN_INFO.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 array of \PIPE\samr operations
 ********************************************************************/

static struct api_struct api_samr_cmds [] =
{
      {"SAMR_CLOSE_HND"         , SAMR_CLOSE_HND        , api_samr_close_hnd        },
      {"SAMR_CONNECT"           , SAMR_CONNECT          , api_samr_connect          },
      {"SAMR_CONNECT_ANON"      , SAMR_CONNECT_ANON     , api_samr_connect_anon     },
      {"SAMR_ENUM_DOMAINS"      , SAMR_ENUM_DOMAINS     , api_samr_enum_domains     },
      {"SAMR_ENUM_DOM_USERS"    , SAMR_ENUM_DOM_USERS   , api_samr_enum_dom_users   },
      
      {"SAMR_ENUM_DOM_GROUPS"   , SAMR_ENUM_DOM_GROUPS  , api_samr_enum_dom_groups  },
      {"SAMR_ENUM_DOM_ALIASES"  , SAMR_ENUM_DOM_ALIASES , api_samr_enum_dom_aliases },
      {"SAMR_QUERY_USERALIASES" , SAMR_QUERY_USERALIASES, api_samr_query_useraliases},
      {"SAMR_QUERY_ALIASMEM"    , SAMR_QUERY_ALIASMEM   , api_samr_query_aliasmem   },
      {"SAMR_QUERY_GROUPMEM"    , SAMR_QUERY_GROUPMEM   , api_samr_query_groupmem   },
      {"SAMR_ADD_ALIASMEM"      , SAMR_ADD_ALIASMEM     , api_samr_add_aliasmem     },
      {"SAMR_DEL_ALIASMEM"      , SAMR_DEL_ALIASMEM     , api_samr_del_aliasmem     },
      {"SAMR_ADD_GROUPMEM"      , SAMR_ADD_GROUPMEM     , api_samr_add_groupmem     },
      {"SAMR_DEL_GROUPMEM"      , SAMR_DEL_GROUPMEM     , api_samr_del_groupmem     },
      
      {"SAMR_DELETE_DOM_USER"   , SAMR_DELETE_DOM_USER  , api_samr_delete_dom_user  },
      {"SAMR_DELETE_DOM_GROUP"  , SAMR_DELETE_DOM_GROUP , api_samr_delete_dom_group },
      {"SAMR_DELETE_DOM_ALIAS"  , SAMR_DELETE_DOM_ALIAS , api_samr_delete_dom_alias },
      {"SAMR_CREATE_DOM_GROUP"  , SAMR_CREATE_DOM_GROUP , api_samr_create_dom_group },
      {"SAMR_CREATE_DOM_ALIAS"  , SAMR_CREATE_DOM_ALIAS , api_samr_create_dom_alias },
      {"SAMR_LOOKUP_NAMES"      , SAMR_LOOKUP_NAMES     , api_samr_lookup_names     },
      {"SAMR_OPEN_USER"         , SAMR_OPEN_USER        , api_samr_open_user        },
      {"SAMR_QUERY_USERINFO"    , SAMR_QUERY_USERINFO   , api_samr_query_userinfo   },
      {"SAMR_SET_USERINFO"      , SAMR_SET_USERINFO     , api_samr_set_userinfo     },
      {"SAMR_SET_USERINFO2"     , SAMR_SET_USERINFO2    , api_samr_set_userinfo2    },
      
      {"SAMR_QUERY_DOMAIN_INFO" , SAMR_QUERY_DOMAIN_INFO, api_samr_query_domain_info},
      {"SAMR_QUERY_USERGROUPS"  , SAMR_QUERY_USERGROUPS , api_samr_query_usergroups },
      {"SAMR_QUERY_DISPINFO"    , SAMR_QUERY_DISPINFO   , api_samr_query_dispinfo   },
      {"SAMR_QUERY_DISPINFO3"   , SAMR_QUERY_DISPINFO3  , api_samr_query_dispinfo   },
      {"SAMR_QUERY_DISPINFO4"   , SAMR_QUERY_DISPINFO4  , api_samr_query_dispinfo   },
      
      {"SAMR_QUERY_ALIASINFO"   , SAMR_QUERY_ALIASINFO  , api_samr_query_aliasinfo  },
      {"SAMR_QUERY_GROUPINFO"   , SAMR_QUERY_GROUPINFO  , api_samr_query_groupinfo  },
      {"SAMR_SET_GROUPINFO"     , SAMR_SET_GROUPINFO    , api_samr_set_groupinfo    },
      {"SAMR_SET_ALIASINFO"     , SAMR_SET_ALIASINFO    , api_samr_set_aliasinfo    },
      {"SAMR_CREATE_USER"       , SAMR_CREATE_USER      , api_samr_create_user      },
      {"SAMR_LOOKUP_RIDS"       , SAMR_LOOKUP_RIDS      , api_samr_lookup_rids      },
      {"SAMR_GET_DOM_PWINFO"    , SAMR_GET_DOM_PWINFO   , api_samr_get_dom_pwinfo   },
      {"SAMR_CHGPASSWD_USER"    , SAMR_CHGPASSWD_USER   , api_samr_chgpasswd_user   },
      {"SAMR_OPEN_ALIAS"        , SAMR_OPEN_ALIAS       , api_samr_open_alias       },
      {"SAMR_OPEN_GROUP"        , SAMR_OPEN_GROUP       , api_samr_open_group       },
      {"SAMR_OPEN_DOMAIN"       , SAMR_OPEN_DOMAIN      , api_samr_open_domain      },
      {"SAMR_REMOVE_SID_FOREIGN_DOMAIN"       , SAMR_REMOVE_SID_FOREIGN_DOMAIN      , api_samr_remove_sid_foreign_domain      },
      {"SAMR_LOOKUP_DOMAIN"     , SAMR_LOOKUP_DOMAIN    , api_samr_lookup_domain    },
      
      {"SAMR_QUERY_SEC_OBJECT"  , SAMR_QUERY_SEC_OBJECT , api_samr_query_sec_obj    },
      {"SAMR_SET_SEC_OBJECT"    , SAMR_SET_SEC_OBJECT   , api_samr_set_sec_obj      },
      {"SAMR_GET_USRDOM_PWINFO" , SAMR_GET_USRDOM_PWINFO, api_samr_get_usrdom_pwinfo},
      {"SAMR_QUERY_DOMAIN_INFO2", SAMR_QUERY_DOMAIN_INFO2, api_samr_query_domain_info2},
      {"SAMR_SET_DOMAIN_INFO"   , SAMR_SET_DOMAIN_INFO  , api_samr_set_dom_info     },
      {"SAMR_CONNECT4"          , SAMR_CONNECT4         , api_samr_connect4         },
      {"SAMR_CHGPASSWD_USER3"   , SAMR_CHGPASSWD_USER3  , api_samr_chgpasswd_user3  },
      {"SAMR_CONNECT5"          , SAMR_CONNECT5         , api_samr_connect5         }
};

void samr_get_pipe_fns( struct api_struct **fns, int *n_fns )
{
	*fns = api_samr_cmds;
	*n_fns = sizeof(api_samr_cmds) / sizeof(struct api_struct);
}


NTSTATUS rpc_samr_init(void)
{
  return rpc_pipe_register_commands(SMB_RPC_INTERFACE_VERSION, "samr", "lsass", api_samr_cmds,
				    sizeof(api_samr_cmds) / sizeof(struct api_struct));
}
