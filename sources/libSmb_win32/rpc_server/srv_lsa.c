/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Andrew Tridgell              1992-1997,
 *  Copyright (C) Luke Kenneth Casson Leighton 1996-1997,
 *  Copyright (C) Paul Ashton                       1997,
 *  Copyright (C) Jeremy Allison                    2001,
 *  Copyright (C) Jim McDonough <jmcd@us.ibm.com> 2002-2003.
 *  Copyright (C) Gerald (Jerry) Carter             2005
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

/* This is the interface to the lsa server code. */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

/***************************************************************************
 api_lsa_open_policy2
 ***************************************************************************/

static BOOL api_lsa_open_policy2(pipes_struct *p)
{
	LSA_Q_OPEN_POL2 q_u;
	LSA_R_OPEN_POL2 r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the server, object attributes and desired access flag...*/
	if(!lsa_io_q_open_pol2("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_open_policy2: unable to unmarshall LSA_Q_OPEN_POL2.\n"));
		return False;
	}

	r_u.status = _lsa_open_policy2(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_open_pol2("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_open_policy2: unable to marshall LSA_R_OPEN_POL2.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
api_lsa_open_policy
 ***************************************************************************/

static BOOL api_lsa_open_policy(pipes_struct *p)
{
	LSA_Q_OPEN_POL q_u;
	LSA_R_OPEN_POL r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the server, object attributes and desired access flag...*/
	if(!lsa_io_q_open_pol("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_open_policy: unable to unmarshall LSA_Q_OPEN_POL.\n"));
		return False;
	}

	r_u.status = _lsa_open_policy(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_open_pol("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_open_policy: unable to marshall LSA_R_OPEN_POL.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_enum_trust_dom
 ***************************************************************************/

static BOOL api_lsa_enum_trust_dom(pipes_struct *p)
{
	LSA_Q_ENUM_TRUST_DOM q_u;
	LSA_R_ENUM_TRUST_DOM r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the enum trust domain context etc. */
	if(!lsa_io_q_enum_trust_dom("", &q_u, data, 0))
		return False;

	/* get required trusted domains information */
	r_u.status = _lsa_enum_trust_dom(p, &q_u, &r_u);

	/* prepare the response */
	if(!lsa_io_r_enum_trust_dom("", &r_u, rdata, 0))
		return False;

	return True;
}

/***************************************************************************
 api_lsa_query_info
 ***************************************************************************/

static BOOL api_lsa_query_info(pipes_struct *p)
{
	LSA_Q_QUERY_INFO q_u;
	LSA_R_QUERY_INFO r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the info class and policy handle */
	if(!lsa_io_q_query("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_query_info: failed to unmarshall LSA_Q_QUERY_INFO.\n"));
		return False;
	}

	r_u.status = _lsa_query_info(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_query("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_query_info: failed to marshall LSA_R_QUERY_INFO.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_lookup_sids
 ***************************************************************************/

static BOOL api_lsa_lookup_sids(pipes_struct *p)
{
	LSA_Q_LOOKUP_SIDS q_u;
	LSA_R_LOOKUP_SIDS r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the info class and policy handle */
	if(!lsa_io_q_lookup_sids("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_lookup_sids: failed to unmarshall LSA_Q_LOOKUP_SIDS.\n"));
		return False;
	}

	r_u.status = _lsa_lookup_sids(p, &q_u, &r_u);

	if(!lsa_io_r_lookup_sids("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_lookup_sids: Failed to marshall LSA_R_LOOKUP_SIDS.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_lookup_names
 ***************************************************************************/

static BOOL api_lsa_lookup_names(pipes_struct *p)
{
	LSA_Q_LOOKUP_NAMES q_u;
	LSA_R_LOOKUP_NAMES r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the info class and policy handle */
	if(!lsa_io_q_lookup_names("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_lookup_names: failed to unmarshall LSA_Q_LOOKUP_NAMES.\n"));
		return False;
	}

	r_u.status = _lsa_lookup_names(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_lookup_names("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_lookup_names: Failed to marshall LSA_R_LOOKUP_NAMES.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_close.
 ***************************************************************************/

static BOOL api_lsa_close(pipes_struct *p)
{
	LSA_Q_CLOSE q_u;
	LSA_R_CLOSE r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if (!lsa_io_q_close("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_close: lsa_io_q_close failed.\n"));
		return False;
	}

	r_u.status = _lsa_close(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if (!lsa_io_r_close("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_close: lsa_io_r_close failed.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_open_secret.
 ***************************************************************************/

static BOOL api_lsa_open_secret(pipes_struct *p)
{
	LSA_Q_OPEN_SECRET q_u;
	LSA_R_OPEN_SECRET r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_open_secret("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_open_secret: failed to unmarshall LSA_Q_OPEN_SECRET.\n"));
		return False;
	}

	r_u.status = _lsa_open_secret(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_open_secret("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_open_secret: Failed to marshall LSA_R_OPEN_SECRET.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_open_secret.
 ***************************************************************************/

static BOOL api_lsa_enum_privs(pipes_struct *p)
{
	LSA_Q_ENUM_PRIVS q_u;
	LSA_R_ENUM_PRIVS r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_enum_privs("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_enum_privs: failed to unmarshall LSA_Q_ENUM_PRIVS.\n"));
		return False;
	}

	r_u.status = _lsa_enum_privs(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_enum_privs("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_enum_privs: Failed to marshall LSA_R_ENUM_PRIVS.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_open_secret.
 ***************************************************************************/

static BOOL api_lsa_priv_get_dispname(pipes_struct *p)
{
	LSA_Q_PRIV_GET_DISPNAME q_u;
	LSA_R_PRIV_GET_DISPNAME r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_priv_get_dispname("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_priv_get_dispname: failed to unmarshall LSA_Q_PRIV_GET_DISPNAME.\n"));
		return False;
	}

	r_u.status = _lsa_priv_get_dispname(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_priv_get_dispname("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_priv_get_dispname: Failed to marshall LSA_R_PRIV_GET_DISPNAME.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_open_secret.
 ***************************************************************************/

static BOOL api_lsa_enum_accounts(pipes_struct *p)
{
	LSA_Q_ENUM_ACCOUNTS q_u;
	LSA_R_ENUM_ACCOUNTS r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_enum_accounts("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_enum_accounts: failed to unmarshall LSA_Q_ENUM_ACCOUNTS.\n"));
		return False;
	}

	r_u.status = _lsa_enum_accounts(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_enum_accounts("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_enum_accounts: Failed to marshall LSA_R_ENUM_ACCOUNTS.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_UNK_GET_CONNUSER
 ***************************************************************************/

static BOOL api_lsa_unk_get_connuser(pipes_struct *p)
{
	LSA_Q_UNK_GET_CONNUSER q_u;
	LSA_R_UNK_GET_CONNUSER r_u;
	
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_unk_get_connuser("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_unk_get_connuser: failed to unmarshall LSA_Q_UNK_GET_CONNUSER.\n"));
		return False;
	}

	r_u.status = _lsa_unk_get_connuser(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_unk_get_connuser("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_unk_get_connuser: Failed to marshall LSA_R_UNK_GET_CONNUSER.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_create_user
 ***************************************************************************/

static BOOL api_lsa_create_account(pipes_struct *p)
{
	LSA_Q_CREATEACCOUNT q_u;
	LSA_R_CREATEACCOUNT r_u;
	
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_create_account("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_create_account: failed to unmarshall LSA_Q_CREATEACCOUNT.\n"));
		return False;
	}

	r_u.status = _lsa_create_account(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_create_account("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_create_account: Failed to marshall LSA_R_CREATEACCOUNT.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_open_user
 ***************************************************************************/

static BOOL api_lsa_open_account(pipes_struct *p)
{
	LSA_Q_OPENACCOUNT q_u;
	LSA_R_OPENACCOUNT r_u;
	
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_open_account("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_open_account: failed to unmarshall LSA_Q_OPENACCOUNT.\n"));
		return False;
	}

	r_u.status = _lsa_open_account(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_open_account("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_open_account: Failed to marshall LSA_R_OPENACCOUNT.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_get_privs
 ***************************************************************************/

static BOOL api_lsa_enum_privsaccount(pipes_struct *p)
{
	LSA_Q_ENUMPRIVSACCOUNT q_u;
	LSA_R_ENUMPRIVSACCOUNT r_u;
	
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_enum_privsaccount("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_enum_privsaccount: failed to unmarshall LSA_Q_ENUMPRIVSACCOUNT.\n"));
		return False;
	}

	r_u.status = _lsa_enum_privsaccount(p, rdata, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_enum_privsaccount("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_enum_privsaccount: Failed to marshall LSA_R_ENUMPRIVSACCOUNT.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_getsystemaccount
 ***************************************************************************/

static BOOL api_lsa_getsystemaccount(pipes_struct *p)
{
	LSA_Q_GETSYSTEMACCOUNT q_u;
	LSA_R_GETSYSTEMACCOUNT r_u;
	
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_getsystemaccount("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_getsystemaccount: failed to unmarshall LSA_Q_GETSYSTEMACCOUNT.\n"));
		return False;
	}

	r_u.status = _lsa_getsystemaccount(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_getsystemaccount("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_getsystemaccount: Failed to marshall LSA_R_GETSYSTEMACCOUNT.\n"));
		return False;
	}

	return True;
}


/***************************************************************************
 api_lsa_setsystemaccount
 ***************************************************************************/

static BOOL api_lsa_setsystemaccount(pipes_struct *p)
{
	LSA_Q_SETSYSTEMACCOUNT q_u;
	LSA_R_SETSYSTEMACCOUNT r_u;
	
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_setsystemaccount("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_setsystemaccount: failed to unmarshall LSA_Q_SETSYSTEMACCOUNT.\n"));
		return False;
	}

	r_u.status = _lsa_setsystemaccount(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_setsystemaccount("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_setsystemaccount: Failed to marshall LSA_R_SETSYSTEMACCOUNT.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_addprivs
 ***************************************************************************/

static BOOL api_lsa_addprivs(pipes_struct *p)
{
	LSA_Q_ADDPRIVS q_u;
	LSA_R_ADDPRIVS r_u;
	
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_addprivs("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_addprivs: failed to unmarshall LSA_Q_ADDPRIVS.\n"));
		return False;
	}

	r_u.status = _lsa_addprivs(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_addprivs("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_addprivs: Failed to marshall LSA_R_ADDPRIVS.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_removeprivs
 ***************************************************************************/

static BOOL api_lsa_removeprivs(pipes_struct *p)
{
	LSA_Q_REMOVEPRIVS q_u;
	LSA_R_REMOVEPRIVS r_u;
	
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_removeprivs("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_removeprivs: failed to unmarshall LSA_Q_REMOVEPRIVS.\n"));
		return False;
	}

	r_u.status = _lsa_removeprivs(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_removeprivs("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_removeprivs: Failed to marshall LSA_R_REMOVEPRIVS.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_query_secobj
 ***************************************************************************/

static BOOL api_lsa_query_secobj(pipes_struct *p)
{
	LSA_Q_QUERY_SEC_OBJ q_u;
	LSA_R_QUERY_SEC_OBJ r_u;
	
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_query_sec_obj("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_query_secobj: failed to unmarshall LSA_Q_QUERY_SEC_OBJ.\n"));
		return False;
	}

	r_u.status = _lsa_query_secobj(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_query_sec_obj("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_query_secobj: Failed to marshall LSA_R_QUERY_SEC_OBJ.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_add_acct_rights
 ***************************************************************************/

static BOOL api_lsa_add_acct_rights(pipes_struct *p)
{
	LSA_Q_ADD_ACCT_RIGHTS q_u;
	LSA_R_ADD_ACCT_RIGHTS r_u;
	
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_add_acct_rights("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_add_acct_rights: failed to unmarshall LSA_Q_ADD_ACCT_RIGHTS.\n"));
		return False;
	}

	r_u.status = _lsa_add_acct_rights(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_add_acct_rights("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_add_acct_rights: Failed to marshall LSA_R_ADD_ACCT_RIGHTS.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_remove_acct_rights
 ***************************************************************************/

static BOOL api_lsa_remove_acct_rights(pipes_struct *p)
{
	LSA_Q_REMOVE_ACCT_RIGHTS q_u;
	LSA_R_REMOVE_ACCT_RIGHTS r_u;
	
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_remove_acct_rights("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_remove_acct_rights: failed to unmarshall LSA_Q_REMOVE_ACCT_RIGHTS.\n"));
		return False;
	}

	r_u.status = _lsa_remove_acct_rights(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_remove_acct_rights("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_remove_acct_rights: Failed to marshall LSA_R_REMOVE_ACCT_RIGHTS.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_enum_acct_rights
 ***************************************************************************/

static BOOL api_lsa_enum_acct_rights(pipes_struct *p)
{
	LSA_Q_ENUM_ACCT_RIGHTS q_u;
	LSA_R_ENUM_ACCT_RIGHTS r_u;
	
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_enum_acct_rights("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_enum_acct_rights: failed to unmarshall LSA_Q_ENUM_ACCT_RIGHTS.\n"));
		return False;
	}

	r_u.status = _lsa_enum_acct_rights(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_enum_acct_rights("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_enum_acct_rights: Failed to marshall LSA_R_ENUM_ACCT_RIGHTS.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_lookup_priv_value
 ***************************************************************************/

static BOOL api_lsa_lookup_priv_value(pipes_struct *p)
{
	LSA_Q_LOOKUP_PRIV_VALUE q_u;
	LSA_R_LOOKUP_PRIV_VALUE r_u;
	
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_lookup_priv_value("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_lookup_priv_value: failed to unmarshall LSA_Q_LOOKUP_PRIV_VALUE .\n"));
		return False;
	}

	r_u.status = _lsa_lookup_priv_value(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_lookup_priv_value("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_lookup_priv_value: Failed to marshall LSA_R_LOOKUP_PRIV_VALUE.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 ***************************************************************************/

static BOOL api_lsa_open_trust_dom(pipes_struct *p)
{
	LSA_Q_OPEN_TRUSTED_DOMAIN q_u;
	LSA_R_OPEN_TRUSTED_DOMAIN r_u;
	
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_open_trusted_domain("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_open_trust_dom: failed to unmarshall LSA_Q_OPEN_TRUSTED_DOMAIN .\n"));
		return False;
	}

	r_u.status = _lsa_open_trusted_domain(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_open_trusted_domain("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_open_trust_dom: Failed to marshall LSA_R_OPEN_TRUSTED_DOMAIN.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 ***************************************************************************/

static BOOL api_lsa_create_trust_dom(pipes_struct *p)
{
	LSA_Q_CREATE_TRUSTED_DOMAIN q_u;
	LSA_R_CREATE_TRUSTED_DOMAIN r_u;
	
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_create_trusted_domain("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_create_trust_dom: failed to unmarshall LSA_Q_CREATE_TRUSTED_DOMAIN .\n"));
		return False;
	}

	r_u.status = _lsa_create_trusted_domain(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_create_trusted_domain("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_create_trust_dom: Failed to marshall LSA_R_CREATE_TRUSTED_DOMAIN.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 ***************************************************************************/

static BOOL api_lsa_create_secret(pipes_struct *p)
{
	LSA_Q_CREATE_SECRET q_u;
	LSA_R_CREATE_SECRET r_u;
	
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_create_secret("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_create_secret: failed to unmarshall LSA_Q_CREATE_SECRET.\n"));
		return False;
	}

	r_u.status = _lsa_create_secret(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_create_secret("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_create_secret: Failed to marshall LSA_R_CREATE_SECRET.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 ***************************************************************************/

static BOOL api_lsa_set_secret(pipes_struct *p)
{
	LSA_Q_SET_SECRET q_u;
	LSA_R_SET_SECRET r_u;
	
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_set_secret("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_set_secret: failed to unmarshall LSA_Q_SET_SECRET.\n"));
		return False;
	}

	r_u.status = _lsa_set_secret(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_set_secret("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_set_secret: Failed to marshall LSA_R_SET_SECRET.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 ***************************************************************************/

static BOOL api_lsa_delete_object(pipes_struct *p)
{
	LSA_Q_DELETE_OBJECT q_u;
	LSA_R_DELETE_OBJECT r_u;
	
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_delete_object("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_delete_object: failed to unmarshall LSA_Q_DELETE_OBJECT.\n"));
		return False;
	}

	r_u.status = _lsa_delete_object(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_delete_object("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_delete_object: Failed to marshall LSA_R_DELETE_OBJECT.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_lookup_sids2
 ***************************************************************************/

static BOOL api_lsa_lookup_sids2(pipes_struct *p)
{
	LSA_Q_LOOKUP_SIDS2 q_u;
	LSA_R_LOOKUP_SIDS2 r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the info class and policy handle */
	if(!lsa_io_q_lookup_sids2("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_lookup_sids2: failed to unmarshall LSA_Q_LOOKUP_SIDS2.\n"));
		return False;
	}

	r_u.status = _lsa_lookup_sids2(p, &q_u, &r_u);

	if(!lsa_io_r_lookup_sids2("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_lookup_sids2: Failed to marshall LSA_R_LOOKUP_SIDS2.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_lookup_sids3
 ***************************************************************************/

static BOOL api_lsa_lookup_sids3(pipes_struct *p)
{
	LSA_Q_LOOKUP_SIDS3 q_u;
	LSA_R_LOOKUP_SIDS3 r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the info class and policy handle */
	if(!lsa_io_q_lookup_sids3("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_lookup_sids3: failed to unmarshall LSA_Q_LOOKUP_SIDS3.\n"));
		return False;
	}

	r_u.status = _lsa_lookup_sids3(p, &q_u, &r_u);

	if(!lsa_io_r_lookup_sids3("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_lookup_sids3: Failed to marshall LSA_R_LOOKUP_SIDS3.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_lookup_names2
 ***************************************************************************/

static BOOL api_lsa_lookup_names2(pipes_struct *p)
{
	LSA_Q_LOOKUP_NAMES2 q_u;
	LSA_R_LOOKUP_NAMES2 r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the info class and policy handle */
	if(!lsa_io_q_lookup_names2("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_lookup_names2: failed to unmarshall LSA_Q_LOOKUP_NAMES2.\n"));
		return False;
	}

	r_u.status = _lsa_lookup_names2(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_lookup_names2("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_lookup_names2: Failed to marshall LSA_R_LOOKUP_NAMES2.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_lookup_names3
 ***************************************************************************/

static BOOL api_lsa_lookup_names3(pipes_struct *p)
{
	LSA_Q_LOOKUP_NAMES3 q_u;
	LSA_R_LOOKUP_NAMES3 r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the info class and policy handle */
	if(!lsa_io_q_lookup_names3("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_lookup_names3: failed to unmarshall LSA_Q_LOOKUP_NAMES3.\n"));
		return False;
	}

	r_u.status = _lsa_lookup_names3(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_lookup_names3("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_lookup_names3: Failed to marshall LSA_R_LOOKUP_NAMES3.\n"));
		return False;
	}

	return True;
}

/***************************************************************************
 api_lsa_lookup_names4
 ***************************************************************************/

static BOOL api_lsa_lookup_names4(pipes_struct *p)
{
	LSA_Q_LOOKUP_NAMES4 q_u;
	LSA_R_LOOKUP_NAMES4 r_u;
	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	/* grab the info class and policy handle */
	if(!lsa_io_q_lookup_names4("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_lookup_names4: failed to unmarshall LSA_Q_LOOKUP_NAMES4.\n"));
		return False;
	}

	r_u.status = _lsa_lookup_names4(p, &q_u, &r_u);

	/* store the response in the SMB stream */
	if(!lsa_io_r_lookup_names4("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_lookup_names4: Failed to marshall LSA_R_LOOKUP_NAMES4.\n"));
		return False;
	}

	return True;
}

#if 0	/* AD DC work in ongoing in Samba 4 */

/***************************************************************************
 api_lsa_query_info2
 ***************************************************************************/

static BOOL api_lsa_query_info2(pipes_struct *p)
{
	LSA_Q_QUERY_INFO2 q_u;
	LSA_R_QUERY_INFO2 r_u;

	prs_struct *data = &p->in_data.data;
	prs_struct *rdata = &p->out_data.rdata;

	ZERO_STRUCT(q_u);
	ZERO_STRUCT(r_u);

	if(!lsa_io_q_query_info2("", &q_u, data, 0)) {
		DEBUG(0,("api_lsa_query_info2: failed to unmarshall LSA_Q_QUERY_INFO2.\n"));
		return False;
	}

	r_u.status = _lsa_query_info2(p, &q_u, &r_u);

	if (!lsa_io_r_query_info2("", &r_u, rdata, 0)) {
		DEBUG(0,("api_lsa_query_info2: failed to marshall LSA_R_QUERY_INFO2.\n"));
		return False;
	}

	return True;
}
#endif	/* AD DC work in ongoing in Samba 4 */

/***************************************************************************
 \PIPE\ntlsa commands
 ***************************************************************************/
 
static struct api_struct api_lsa_cmds[] =
{
	{ "LSA_OPENPOLICY2"     , LSA_OPENPOLICY2     , api_lsa_open_policy2     },
	{ "LSA_OPENPOLICY"      , LSA_OPENPOLICY      , api_lsa_open_policy      },
	{ "LSA_QUERYINFOPOLICY" , LSA_QUERYINFOPOLICY , api_lsa_query_info       },
	{ "LSA_ENUMTRUSTDOM"    , LSA_ENUMTRUSTDOM    , api_lsa_enum_trust_dom   },
	{ "LSA_CLOSE"           , LSA_CLOSE           , api_lsa_close            },
	{ "LSA_OPENSECRET"      , LSA_OPENSECRET      , api_lsa_open_secret      },
	{ "LSA_LOOKUPSIDS"      , LSA_LOOKUPSIDS      , api_lsa_lookup_sids      },
	{ "LSA_LOOKUPNAMES"     , LSA_LOOKUPNAMES     , api_lsa_lookup_names     },
	{ "LSA_ENUM_PRIVS"      , LSA_ENUM_PRIVS      , api_lsa_enum_privs       },
	{ "LSA_PRIV_GET_DISPNAME",LSA_PRIV_GET_DISPNAME,api_lsa_priv_get_dispname},
	{ "LSA_ENUM_ACCOUNTS"   , LSA_ENUM_ACCOUNTS   , api_lsa_enum_accounts    },
	{ "LSA_UNK_GET_CONNUSER", LSA_UNK_GET_CONNUSER, api_lsa_unk_get_connuser },
	{ "LSA_CREATEACCOUNT"   , LSA_CREATEACCOUNT   , api_lsa_create_account   },
	{ "LSA_OPENACCOUNT"     , LSA_OPENACCOUNT     , api_lsa_open_account     },
	{ "LSA_ENUMPRIVSACCOUNT", LSA_ENUMPRIVSACCOUNT, api_lsa_enum_privsaccount},
	{ "LSA_GETSYSTEMACCOUNT", LSA_GETSYSTEMACCOUNT, api_lsa_getsystemaccount },
	{ "LSA_SETSYSTEMACCOUNT", LSA_SETSYSTEMACCOUNT, api_lsa_setsystemaccount },
	{ "LSA_ADDPRIVS"        , LSA_ADDPRIVS        , api_lsa_addprivs         },
	{ "LSA_REMOVEPRIVS"     , LSA_REMOVEPRIVS     , api_lsa_removeprivs      },
	{ "LSA_ADDACCTRIGHTS"   , LSA_ADDACCTRIGHTS   , api_lsa_add_acct_rights    },
	{ "LSA_REMOVEACCTRIGHTS", LSA_REMOVEACCTRIGHTS, api_lsa_remove_acct_rights },
	{ "LSA_ENUMACCTRIGHTS"  , LSA_ENUMACCTRIGHTS  , api_lsa_enum_acct_rights },
	{ "LSA_QUERYSECOBJ"     , LSA_QUERYSECOBJ     , api_lsa_query_secobj     },
	{ "LSA_LOOKUPPRIVVALUE" , LSA_LOOKUPPRIVVALUE , api_lsa_lookup_priv_value },
	{ "LSA_OPENTRUSTDOM"    , LSA_OPENTRUSTDOM    , api_lsa_open_trust_dom },
	{ "LSA_OPENSECRET"      , LSA_OPENSECRET      , api_lsa_open_secret },
	{ "LSA_CREATETRUSTDOM"  , LSA_CREATETRUSTDOM  , api_lsa_create_trust_dom },
	{ "LSA_CREATSECRET"     , LSA_CREATESECRET    , api_lsa_create_secret },
	{ "LSA_SETSECRET"       , LSA_SETSECRET       , api_lsa_set_secret },
	{ "LSA_DELETEOBJECT"    , LSA_DELETEOBJECT    , api_lsa_delete_object },
	{ "LSA_LOOKUPSIDS2"     , LSA_LOOKUPSIDS2     , api_lsa_lookup_sids2 },
	{ "LSA_LOOKUPNAMES2"	, LSA_LOOKUPNAMES2    , api_lsa_lookup_names2 },
	{ "LSA_LOOKUPNAMES3"	, LSA_LOOKUPNAMES3    , api_lsa_lookup_names3 },
	{ "LSA_LOOKUPSIDS3"     , LSA_LOOKUPSIDS3     , api_lsa_lookup_sids3 },
	{ "LSA_LOOKUPNAMES4"	, LSA_LOOKUPNAMES4    , api_lsa_lookup_names4 }
#if 0	/* AD DC work in ongoing in Samba 4 */
	/* be careful of the adding of new RPC's.  See commentrs below about
	   ADS DC capabilities                                               */
	{ "LSA_QUERYINFO2"      , LSA_QUERYINFO2      , api_lsa_query_info2      }
#endif	/* AD DC work in ongoing in Samba 4 */
};

static int count_fns(void)
{
	int funcs = sizeof(api_lsa_cmds) / sizeof(struct api_struct);
	
#if 0	/* AD DC work is on going in Samba 4 */
	/*
	 * NOTE: Certain calls can not be enabled if we aren't an ADS DC.  Make sure
	 * these calls are always last and that you decrement by the amount of calls
	 * to disable.
	 */
	if (!(SEC_ADS == lp_security() && ROLE_DOMAIN_PDC == lp_server_role())) {
		funcs -= 1;
	}
#endif	/* AD DC work in ongoing in Samba 4 */

	return funcs;
}
void lsa_get_pipe_fns( struct api_struct **fns, int *n_fns )
{
	*fns = api_lsa_cmds;
	*n_fns = count_fns();
}


NTSTATUS rpc_lsa_init(void)
{
	int funcs = count_fns();

	return rpc_pipe_register_commands(SMB_RPC_INTERFACE_VERSION, "lsarpc", "lsass", api_lsa_cmds, 
		funcs);
}
