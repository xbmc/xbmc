/* 
   Unix SMB/CIFS implementation.
   Kerberos authorization data
   Copyright (C) Jim McDonough <jmcd@us.ibm.com> 2003
   
   
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

#ifndef _AUTHDATA_H
#define _AUTHDATA_H 

#include "rpc_misc.h"
#include "rpc_netlogon.h"

#define PAC_TYPE_LOGON_INFO 1
#define PAC_TYPE_SERVER_CHECKSUM 6
#define PAC_TYPE_PRIVSVR_CHECKSUM 7
#define PAC_TYPE_LOGON_NAME 10

#ifndef KRB5_AUTHDATA_WIN2K_PAC
#define KRB5_AUTHDATA_WIN2K_PAC 128
#endif

#ifndef KRB5_AUTHDATA_IF_RELEVANT
#define KRB5_AUTHDATA_IF_RELEVANT 1
#endif


typedef struct pac_logon_name {
	NTTIME logon_time;
	uint16 len;
	uint8 *username; /* Actually always little-endian. might not be null terminated, so not UNISTR */
} PAC_LOGON_NAME;

typedef struct pac_signature_data {
	uint32 type;
	RPC_DATA_BLOB signature; /* this not the on-wire-format (!) */
} PAC_SIGNATURE_DATA;

typedef struct group_membership {
	uint32 rid;
	uint32 attrs;
} GROUP_MEMBERSHIP;

typedef struct group_membership_array {
	uint32 count;
	GROUP_MEMBERSHIP *group_membership;
} GROUP_MEMBERSHIP_ARRAY;

#if 0 /* Unused, replaced by NET_USER_INFO_3 - Guenther */

typedef struct krb_sid_and_attrs {
	uint32 sid_ptr;
	uint32 attrs;
	DOM_SID2 *sid;
} KRB_SID_AND_ATTRS;

typedef struct krb_sid_and_attr_array {
	uint32 count;
	KRB_SID_AND_ATTRS *krb_sid_and_attrs;
} KRB_SID_AND_ATTR_ARRAY;
	

/* This is awfully similar to a samr_user_info_23, but not identical.
   Many of the field names have been swiped from there, because it is
   so similar that they are likely the same, but many have been verified.
   Some are in a different order, though... */
typedef struct pac_logon_info {	
	NTTIME logon_time;            /* logon time */
	NTTIME logoff_time;           /* logoff time */
	NTTIME kickoff_time;          /* kickoff time */
	NTTIME pass_last_set_time;    /* password last set time */
	NTTIME pass_can_change_time;  /* password can change time */
	NTTIME pass_must_change_time; /* password must change time */

	UNIHDR hdr_user_name;    /* user name unicode string header */
	UNIHDR hdr_full_name;    /* user's full name unicode string header */
	UNIHDR hdr_logon_script; /* these last 4 appear to be in a different */
	UNIHDR hdr_profile_path; /* order than in the info23 */
	UNIHDR hdr_home_dir;    
	UNIHDR hdr_dir_drive;   

	uint16 logon_count; /* number of times user has logged onto domain */
	uint16 bad_password_count;	/* samba4 idl */

	uint32 user_rid;
	uint32 group_rid;
	uint32 group_count;
	uint32 group_membership_ptr;
	uint32 user_flags;

	uint8 session_key[16];		/* samba4 idl */
	UNIHDR hdr_dom_controller;
	UNIHDR hdr_dom_name;

	uint32 ptr_dom_sid;

	uint8 lm_session_key[8];	/* samba4 idl */
	uint32 acct_flags;		/* samba4 idl */
	uint32 unknown[7];

	uint32 sid_count;
	uint32 ptr_extra_sids;

	uint32 ptr_res_group_dom_sid;
	uint32 res_group_count;
	uint32 ptr_res_groups;

	UNISTR2 uni_user_name;    /* user name unicode string header */
	UNISTR2 uni_full_name;    /* user's full name unicode string header */
	UNISTR2 uni_logon_script; /* these last 4 appear to be in a different*/
	UNISTR2 uni_profile_path; /* order than in the info23 */
	UNISTR2 uni_home_dir;    
	UNISTR2 uni_dir_drive;   
	UNISTR2 uni_dom_controller;
	UNISTR2 uni_dom_name;
	DOM_SID2 dom_sid;
	GROUP_MEMBERSHIP_ARRAY groups;
	KRB_SID_AND_ATTR_ARRAY extra_sids;
	DOM_SID2 res_group_dom_sid;
	GROUP_MEMBERSHIP_ARRAY res_groups;

} PAC_LOGON_INFO;
#endif

typedef struct pac_logon_info {	
	NET_USER_INFO_3 info3;
	DOM_SID2 res_group_dom_sid;
	GROUP_MEMBERSHIP_ARRAY res_groups;

} PAC_LOGON_INFO;

typedef struct pac_info_ctr
{
	union
	{
		PAC_LOGON_INFO *logon_info;
		PAC_SIGNATURE_DATA *srv_cksum;
		PAC_SIGNATURE_DATA *privsrv_cksum;
		PAC_LOGON_NAME *logon_name;
	} pac;
} PAC_INFO_CTR;

typedef struct pac_buffer {
	uint32 type;
	uint32 size;
	uint32 offset;
	uint32 offsethi;
	PAC_INFO_CTR *ctr;
	uint32 pad;
} PAC_BUFFER;

typedef struct pac_data {
	uint32 num_buffers;
	uint32 version;
	PAC_BUFFER *pac_buffer;
} PAC_DATA;


#endif
