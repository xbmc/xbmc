/* 
   Unix SMB/CIFS implementation.
   SMB parameters and setup
   Copyright (C) Andrew Tridgell 1992-1997
   Copyright (C) Luke Kenneth Casson Leighton 1996-1997
   Copyright (C) Paul Ashton 1997
   Copyright (C) Jean François Micouleau 2002
   
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

#ifndef _RPC_NETLOGON_H /* _RPC_NETLOGON_H */
#define _RPC_NETLOGON_H 


/* NETLOGON pipe */
#define NET_SAMLOGON		0x02
#define NET_SAMLOGOFF		0x03
#define NET_REQCHAL		0x04
#define NET_AUTH		0x05
#define NET_SRVPWSET		0x06
#define NET_SAM_DELTAS		0x07
#define NET_LOGON_CTRL		0x0c
#define NET_GETDCNAME		0x0d
#define NET_AUTH2		0x0f
#define NET_LOGON_CTRL2		0x0e
#define NET_SAM_SYNC		0x10
#define NET_TRUST_DOM_LIST	0x13
#define NET_DSR_GETDCNAME	0x14
#define NET_AUTH3		0x1a
#define NET_DSR_GETSITENAME	0x1c
#define NET_SAMLOGON_EX		0x27

/* Secure Channel types.  used in NetrServerAuthenticate negotiation */
#define SEC_CHAN_WKSTA   2
#define SEC_CHAN_DOMAIN  4
#define SEC_CHAN_BDC     6

/* Returned delta types */
#define SAM_DELTA_DOMAIN_INFO    0x01
#define SAM_DELTA_GROUP_INFO     0x02
#define SAM_DELTA_RENAME_GROUP   0x04
#define SAM_DELTA_ACCOUNT_INFO   0x05
#define SAM_DELTA_RENAME_USER    0x07
#define SAM_DELTA_GROUP_MEM      0x08
#define SAM_DELTA_ALIAS_INFO     0x09
#define SAM_DELTA_RENAME_ALIAS   0x0b
#define SAM_DELTA_ALIAS_MEM      0x0c
#define SAM_DELTA_POLICY_INFO    0x0d
#define SAM_DELTA_TRUST_DOMS     0x0e
#define SAM_DELTA_PRIVS_INFO     0x10 /* DT_DELTA_ACCOUNTS */
#define SAM_DELTA_SECRET_INFO    0x12
#define SAM_DELTA_DELETE_GROUP   0x14
#define SAM_DELTA_DELETE_USER    0x15
#define SAM_DELTA_MODIFIED_COUNT 0x16

/* SAM database types */
#define SAM_DATABASE_DOMAIN    0x00 /* Domain users and groups */
#define SAM_DATABASE_BUILTIN   0x01 /* BUILTIN users and groups */
#define SAM_DATABASE_PRIVS     0x02 /* Privileges */

/* flags use when sending a NETLOGON_CONTROL request */

#define NETLOGON_CONTROL_SYNC			0x2
#define NETLOGON_CONTROL_REDISCOVER		0x5
#define NETLOGON_CONTROL_TC_QUERY		0x6
#define NETLOGON_CONTROL_TRANSPORT_NOTIFY	0x7
#define NETLOGON_CONTROL_SET_DBFLAG		0xfffe

/* Some flag values reverse engineered from NLTEST.EXE */
/* used in the NETLOGON_CONTROL[2] reply */

#define NL_CTRL_IN_SYNC          0x0000
#define NL_CTRL_REPL_NEEDED      0x0001
#define NL_CTRL_REPL_IN_PROGRESS 0x0002
#define NL_CTRL_FULL_SYNC        0x0004

#define LOGON_GUEST			0x00000001
#define LOGON_NOENCRYPTION		0x00000002
#define LOGON_CACHED_ACCOUNT		0x00000004
#define LOGON_USED_LM_PASSWORD		0x00000008
#define LOGON_EXTRA_SIDS		0x00000020
#define LOGON_SUBAUTH_SESSION_KEY	0x00000040
#define LOGON_SERVER_TRUST_ACCOUNT	0x00000080
#define LOGON_NTLMV2_ENABLED		0x00000100
#define LOGON_RESOURCE_GROUPS		0x00000200
#define LOGON_PROFILE_PATH_RETURNED	0x00000400
#define LOGON_GRACE_LOGON		0x01000000

#define SE_GROUP_MANDATORY		0x00000001
#define SE_GROUP_ENABLED_BY_DEFAULT	0x00000002
#define SE_GROUP_ENABLED		0x00000004
#define SE_GROUP_OWNER 			0x00000008
#define SE_GROUP_USE_FOR_DENY_ONLY 	0x00000010
#define SE_GROUP_LOGON_ID 		0xC0000000
#define SE_GROUP_RESOURCE 		0x20000000

/* Flags for controlling the behaviour of a particular logon */

/* sets LOGON_SERVER_TRUST_ACCOUNT user_flag */
#define MSV1_0_ALLOW_SERVER_TRUST_ACCOUNT	0x00000020
#define MSV1_0_ALLOW_WORKSTATION_TRUST_ACCOUNT	0x00000800

/* updates the "logon time" on network logon */
#define MSV1_0_UPDATE_LOGON_STATISTICS		0x00000004

/* returns the user parameters in the driveletter */
#define MSV1_0_RETURN_USER_PARAMETERS		0x00000008

/* returns the profilepath in the driveletter and 
 * sets LOGON_PROFILE_PATH_RETURNED user_flag */
#define MSV1_0_RETURN_PROFILE_PATH		0x00000200

#if 0
/* I think this is correct - it's what gets parsed on the wire. JRA. */
/* NET_USER_INFO_2 */
typedef struct net_user_info_2 {
	uint32 ptr_user_info;

	NTTIME logon_time;            /* logon time */
	NTTIME logoff_time;           /* logoff time */
	NTTIME kickoff_time;          /* kickoff time */
	NTTIME pass_last_set_time;    /* password last set time */
	NTTIME pass_can_change_time;  /* password can change time */
	NTTIME pass_must_change_time; /* password must change time */

	UNIHDR hdr_user_name;    /* username unicode string header */
	UNIHDR hdr_full_name;    /* user's full name unicode string header */
	UNIHDR hdr_logon_script; /* logon script unicode string header */
	UNIHDR hdr_profile_path; /* profile path unicode string header */
	UNIHDR hdr_home_dir;     /* home directory unicode string header */
	UNIHDR hdr_dir_drive;    /* home directory drive unicode string header */

	uint16 logon_count;  /* logon count */
	uint16 bad_pw_count; /* bad password count */

	uint32 user_id;       /* User ID */
	uint32 group_id;      /* Group ID */
	uint32 num_groups;    /* num groups */
	uint32 buffer_groups; /* undocumented buffer pointer to groups. */
	uint32 user_flgs;     /* user flags */

	uint8 user_sess_key[16]; /* unused user session key */

	UNIHDR hdr_logon_srv; /* logon server unicode string header */
	UNIHDR hdr_logon_dom; /* logon domain unicode string header */

	uint32 buffer_dom_id; /* undocumented logon domain id pointer */
	uint8 padding[40];    /* unused padding bytes.  expansion room */

	UNISTR2 uni_user_name;    /* username unicode string */
	UNISTR2 uni_full_name;    /* user's full name unicode string */
	UNISTR2 uni_logon_script; /* logon script unicode string */
	UNISTR2 uni_profile_path; /* profile path unicode string */
	UNISTR2 uni_home_dir;     /* home directory unicode string */
	UNISTR2 uni_dir_drive;    /* home directory drive unicode string */

	uint32 num_groups2;        /* num groups */
	DOM_GID *gids; /* group info */

	UNISTR2 uni_logon_srv; /* logon server unicode string */
	UNISTR2 uni_logon_dom; /* logon domain unicode string */

	DOM_SID2 dom_sid;           /* domain SID */

	uint32 num_other_groups;        /* other groups */
	DOM_GID *other_gids; /* group info */
	DOM_SID2 *other_sids; /* undocumented - domain SIDs */

} NET_USER_INFO_2;
#endif

/* NET_USER_INFO_2 */
typedef struct net_user_info_2 {
	uint32 ptr_user_info;

	NTTIME logon_time;            /* logon time */
	NTTIME logoff_time;           /* logoff time */
	NTTIME kickoff_time;          /* kickoff time */
	NTTIME pass_last_set_time;    /* password last set time */
	NTTIME pass_can_change_time;  /* password can change time */
	NTTIME pass_must_change_time; /* password must change time */

	UNIHDR hdr_user_name;    /* username unicode string header */
	UNIHDR hdr_full_name;    /* user's full name unicode string header */
	UNIHDR hdr_logon_script; /* logon script unicode string header */
	UNIHDR hdr_profile_path; /* profile path unicode string header */
	UNIHDR hdr_home_dir;     /* home directory unicode string header */
	UNIHDR hdr_dir_drive;    /* home directory drive unicode string header */

	uint16 logon_count;  /* logon count */
	uint16 bad_pw_count; /* bad password count */

	uint32 user_rid;       /* User RID */
	uint32 group_rid;      /* Group RID */

	uint32 num_groups;    /* num groups */
	uint32 buffer_groups; /* undocumented buffer pointer to groups. */
	uint32 user_flgs;     /* user flags */

	uint8 user_sess_key[16]; /* user session key */

	UNIHDR hdr_logon_srv; /* logon server unicode string header */
	UNIHDR hdr_logon_dom; /* logon domain unicode string header */

	uint32 buffer_dom_id; /* undocumented logon domain id pointer */
	uint8 lm_sess_key[8];	/* lm session key */
	uint32 acct_flags;	/* account flags */
	uint32 unknown[7];	/* unknown */

	UNISTR2 uni_user_name;    /* username unicode string */
	UNISTR2 uni_full_name;    /* user's full name unicode string */
	UNISTR2 uni_logon_script; /* logon script unicode string */
	UNISTR2 uni_profile_path; /* profile path unicode string */
	UNISTR2 uni_home_dir;     /* home directory unicode string */
	UNISTR2 uni_dir_drive;    /* home directory drive unicode string */

	UNISTR2 uni_logon_srv; /* logon server unicode string */
	UNISTR2 uni_logon_dom; /* logon domain unicode string */

	DOM_SID2 dom_sid;           /* domain SID */
} NET_USER_INFO_2;

/* NET_USER_INFO_3 */
typedef struct net_user_info_3 {
	uint32 ptr_user_info;

	NTTIME logon_time;            /* logon time */
	NTTIME logoff_time;           /* logoff time */
	NTTIME kickoff_time;          /* kickoff time */
	NTTIME pass_last_set_time;    /* password last set time */
	NTTIME pass_can_change_time;  /* password can change time */
	NTTIME pass_must_change_time; /* password must change time */

	UNIHDR hdr_user_name;    /* username unicode string header */
	UNIHDR hdr_full_name;    /* user's full name unicode string header */
	UNIHDR hdr_logon_script; /* logon script unicode string header */
	UNIHDR hdr_profile_path; /* profile path unicode string header */
	UNIHDR hdr_home_dir;     /* home directory unicode string header */
	UNIHDR hdr_dir_drive;    /* home directory drive unicode string header */

	uint16 logon_count;  /* logon count */
	uint16 bad_pw_count; /* bad password count */

	uint32 user_rid;       /* User RID */
	uint32 group_rid;      /* Group RID */

	uint32 num_groups;    /* num groups */
	uint32 buffer_groups; /* undocumented buffer pointer to groups. */
	uint32 user_flgs;     /* user flags */

	uint8 user_sess_key[16]; /* user session key */

	UNIHDR hdr_logon_srv; /* logon server unicode string header */
	UNIHDR hdr_logon_dom; /* logon domain unicode string header */

	uint32 buffer_dom_id; /* undocumented logon domain id pointer */
	uint8 lm_sess_key[8];	/* lm session key */
	uint32 acct_flags;	/* account flags */
	uint32 unknown[7];	/* unknown */

	uint32 num_other_sids; /* number of foreign/trusted domain sids */
	uint32 buffer_other_sids;
	
	/* The next three uint32 are not really part of user_info_3 but here
	 * for parsing convenience.  They are only valid in Kerberos PAC
	 * parsing - Guenther */
	uint32 ptr_res_group_dom_sid;
	uint32 res_group_count;
	uint32 ptr_res_groups;

	UNISTR2 uni_user_name;    /* username unicode string */
	UNISTR2 uni_full_name;    /* user's full name unicode string */
	UNISTR2 uni_logon_script; /* logon script unicode string */
	UNISTR2 uni_profile_path; /* profile path unicode string */
	UNISTR2 uni_home_dir;     /* home directory unicode string */
	UNISTR2 uni_dir_drive;    /* home directory drive unicode string */

	uint32 num_groups2;        /* num groups */
	DOM_GID *gids; /* group info */

	UNISTR2 uni_logon_srv; /* logon server unicode string */
	UNISTR2 uni_logon_dom; /* logon domain unicode string */

	DOM_SID2 dom_sid;           /* domain SID */

	DOM_SID2 *other_sids; /* foreign/trusted domain SIDs */
	uint32 *other_sids_attrib;
} NET_USER_INFO_3;


/* NETLOGON_INFO_1 - pdc status info, i presume */
typedef struct netlogon_1_info {
	uint32 flags;            /* 0x0 - undocumented */
	uint32 pdc_status;       /* 0x0 - undocumented */
} NETLOGON_INFO_1;

/* NETLOGON_INFO_2 - pdc status info, plus trusted domain info */
typedef struct netlogon_2_info {
	uint32  flags;            /* 0x0 - undocumented */
	uint32  pdc_status;       /* 0x0 - undocumented */
	uint32  ptr_trusted_dc_name; /* pointer to trusted domain controller name */
	uint32  tc_status;           
	UNISTR2 uni_trusted_dc_name; /* unicode string - trusted dc name */
} NETLOGON_INFO_2;

/* NETLOGON_INFO_3 - logon status info, i presume */
typedef struct netlogon_3_info {
	uint32 flags;            /* 0x0 - undocumented */
	uint32 logon_attempts;   /* number of logon attempts */
	uint32 reserved_1;       /* 0x0 - undocumented */
	uint32 reserved_2;       /* 0x0 - undocumented */
	uint32 reserved_3;       /* 0x0 - undocumented */
	uint32 reserved_4;       /* 0x0 - undocumented */
	uint32 reserved_5;       /* 0x0 - undocumented */
} NETLOGON_INFO_3;

/********************************************************
 Logon Control Query

 This is generated by a nltest /bdc_query:DOMAIN

 query_level 0x1, function_code 0x1

 ********************************************************/

/* NET_Q_LOGON_CTRL - LSA Netr Logon Control */

typedef struct net_q_logon_ctrl_info {
	uint32 ptr;
	UNISTR2 uni_server_name;
	uint32 function_code;
	uint32 query_level;
} NET_Q_LOGON_CTRL;

/* NET_R_LOGON_CTRL - LSA Netr Logon Control */

typedef struct net_r_logon_ctrl_info {
	uint32 switch_value;
	uint32 ptr;

	union {
		NETLOGON_INFO_1 info1;
	} logon;

	NTSTATUS status;
} NET_R_LOGON_CTRL;


typedef struct ctrl_data_info_5 {
	uint32 		function_code;
	
	uint32		ptr_domain;
	UNISTR2		domain;
} CTRL_DATA_INFO_5;

typedef struct ctrl_data_info_6 {
	uint32 		function_code;
	
	uint32		ptr_domain;
	UNISTR2		domain;
} CTRL_DATA_INFO_6;


/********************************************************
 Logon Control2 Query

 query_level 0x1 - pdc status
 query_level 0x3 - number of logon attempts.

 ********************************************************/

/* NET_Q_LOGON_CTRL2 - LSA Netr Logon Control 2 */
typedef struct net_q_logon_ctrl2_info {
	uint32       	ptr;             /* undocumented buffer pointer */
	UNISTR2      	uni_server_name; /* server name, starting with two '\'s */
	
	uint32       	function_code; 
	uint32       	query_level;   
	union {
		CTRL_DATA_INFO_5 info5;
		CTRL_DATA_INFO_6 info6;
	} info;
} NET_Q_LOGON_CTRL2;

/*******************************************************
 Logon Control Response

 switch_value is same as query_level in request 
 *******************************************************/

/* NET_R_LOGON_CTRL2 - response to LSA Logon Control2 */
typedef struct net_r_logon_ctrl2_info {
	uint32       switch_value;  /* 0x1, 0x3 */
	uint32       ptr;

	union
	{
		NETLOGON_INFO_1 info1;
		NETLOGON_INFO_2 info2;
		NETLOGON_INFO_3 info3;

	} logon;

	NTSTATUS status; /* return code */
} NET_R_LOGON_CTRL2;

/* NET_Q_GETDCNAME - Ask a DC for a trusted DC name */

typedef struct net_q_getdcname {
	uint32  ptr_logon_server;
	UNISTR2 uni_logon_server;
	uint32  ptr_domainname;
	UNISTR2 uni_domainname;
} NET_Q_GETDCNAME;

/* NET_R_GETDCNAME - Ask a DC for a trusted DC name */

typedef struct net_r_getdcname {
	uint32  ptr_dcname;
	UNISTR2 uni_dcname;
	WERROR status;
} NET_R_GETDCNAME;

/* NET_Q_TRUST_DOM_LIST - LSA Query Trusted Domains */
typedef struct net_q_trust_dom_info {
	uint32       ptr;             /* undocumented buffer pointer */
	UNISTR2      uni_server_name; /* server name, starting with two '\'s */
} NET_Q_TRUST_DOM_LIST;

#define MAX_TRUST_DOMS 1

/* NET_R_TRUST_DOM_LIST - response to LSA Trusted Domains */
typedef struct net_r_trust_dom_info {
	UNISTR2 uni_trust_dom_name[MAX_TRUST_DOMS];

	NTSTATUS status; /* return code */
} NET_R_TRUST_DOM_LIST;


/* NEG_FLAGS */
typedef struct neg_flags_info {
	uint32 neg_flags; /* negotiated flags */
} NEG_FLAGS;


/* NET_Q_REQ_CHAL */
typedef struct net_q_req_chal_info {
	uint32  undoc_buffer; /* undocumented buffer pointer */
	UNISTR2 uni_logon_srv; /* logon server unicode string */
	UNISTR2 uni_logon_clnt; /* logon client unicode string */
	DOM_CHAL clnt_chal; /* client challenge */
} NET_Q_REQ_CHAL;


/* NET_R_REQ_CHAL */
typedef struct net_r_req_chal_info {
	DOM_CHAL srv_chal; /* server challenge */
	NTSTATUS status; /* return code */
} NET_R_REQ_CHAL;

/* NET_Q_AUTH */
typedef struct net_q_auth_info {
	DOM_LOG_INFO clnt_id; /* client identification info */
	DOM_CHAL clnt_chal;     /* client-calculated credentials */
} NET_Q_AUTH;

/* NET_R_AUTH */
typedef struct net_r_auth_info {
	DOM_CHAL srv_chal;     /* server-calculated credentials */
	NTSTATUS status; /* return code */
} NET_R_AUTH;

/* NET_Q_AUTH_2 */
typedef struct net_q_auth2_info {
	DOM_LOG_INFO clnt_id; /* client identification info */
	DOM_CHAL clnt_chal;     /* client-calculated credentials */

	NEG_FLAGS clnt_flgs; /* usually 0x0000 01ff */
} NET_Q_AUTH_2;


/* NET_R_AUTH_2 */
typedef struct net_r_auth2_info {
	DOM_CHAL srv_chal;     /* server-calculated credentials */
	NEG_FLAGS srv_flgs; /* usually 0x0000 01ff */
	NTSTATUS status; /* return code */
} NET_R_AUTH_2;

/* NET_Q_AUTH_3 */
typedef struct net_q_auth3_info {
	DOM_LOG_INFO clnt_id;	/* client identification info */
	DOM_CHAL clnt_chal;		/* client-calculated credentials */
	NEG_FLAGS clnt_flgs;	/* usually 0x6007 ffff */
} NET_Q_AUTH_3;

/* NET_R_AUTH_3 */
typedef struct net_r_auth3_info {
	DOM_CHAL srv_chal;	/* server-calculated credentials */
	NEG_FLAGS srv_flgs;	/* usually 0x6007 ffff */
	uint32 unknown;		/* 0x0000045b */
	NTSTATUS status;	/* return code */
} NET_R_AUTH_3;


/* NET_Q_SRV_PWSET */
typedef struct net_q_srv_pwset_info {
	DOM_CLNT_INFO clnt_id; /* client identification/authentication info */
	uint8 pwd[16]; /* new password - undocumented. */
} NET_Q_SRV_PWSET;
    
/* NET_R_SRV_PWSET */
typedef struct net_r_srv_pwset_info {
	DOM_CRED srv_cred;     /* server-calculated credentials */

	NTSTATUS status; /* return code */
} NET_R_SRV_PWSET;

/* NET_ID_INFO_2 */
typedef struct net_network_info_2 {
	uint32            ptr_id_info2;        /* pointer to id_info_2 */
	UNIHDR            hdr_domain_name;     /* domain name unicode header */
	uint32            param_ctrl;          /* param control (0x2) */
	DOM_LOGON_ID      logon_id;            /* logon ID */
	UNIHDR            hdr_user_name;       /* user name unicode header */
	UNIHDR            hdr_wksta_name;      /* workstation name unicode header */
	uint8             lm_chal[8];          /* lan manager 8 byte challenge */
	STRHDR            hdr_nt_chal_resp;    /* nt challenge response */
	STRHDR            hdr_lm_chal_resp;    /* lm challenge response */

	UNISTR2           uni_domain_name;     /* domain name unicode string */
	UNISTR2           uni_user_name;       /* user name unicode string */
	UNISTR2           uni_wksta_name;      /* workgroup name unicode string */
	STRING2           nt_chal_resp;        /* nt challenge response */
	STRING2           lm_chal_resp;        /* lm challenge response */
} NET_ID_INFO_2;

/* NET_ID_INFO_1 */
typedef struct id_info_1 {
	uint32            ptr_id_info1;        /* pointer to id_info_1 */
	UNIHDR            hdr_domain_name;     /* domain name unicode header */
	uint32            param_ctrl;          /* param control */
	DOM_LOGON_ID      logon_id;            /* logon ID */
	UNIHDR            hdr_user_name;       /* user name unicode header */
	UNIHDR            hdr_wksta_name;      /* workstation name unicode header */
	OWF_INFO          lm_owf;              /* LM OWF Password */
	OWF_INFO          nt_owf;              /* NT OWF Password */
	UNISTR2           uni_domain_name;     /* domain name unicode string */
	UNISTR2           uni_user_name;       /* user name unicode string */
	UNISTR2           uni_wksta_name;      /* workgroup name unicode string */
} NET_ID_INFO_1;

#define INTERACTIVE_LOGON_TYPE 1
#define NET_LOGON_TYPE 2

/* NET_ID_INFO_CTR */
typedef struct net_id_info_ctr_info {
	uint16         switch_value;
  
	union {
		NET_ID_INFO_1 id1; /* auth-level 1 - interactive user login */
		NET_ID_INFO_2 id2; /* auth-level 2 - workstation referred login */
	} auth;
} NET_ID_INFO_CTR;

/* SAM_INFO - sam logon/off id structure */
typedef struct sam_info {
	DOM_CLNT_INFO2  client;
	uint32          ptr_rtn_cred; /* pointer to return credentials */
	DOM_CRED        rtn_cred; /* return credentials */
	uint16          logon_level;
	NET_ID_INFO_CTR *ctr;
} DOM_SAM_INFO;

/* SAM_INFO - sam logon/off id structure - no creds */
typedef struct sam_info_ex {
	DOM_CLNT_SRV	client;
	uint16          logon_level;
	NET_ID_INFO_CTR *ctr;
} DOM_SAM_INFO_EX;

/* NET_Q_SAM_LOGON */
typedef struct net_q_sam_logon_info {
	DOM_SAM_INFO sam_id;
	uint16          validation_level;
} NET_Q_SAM_LOGON;

/* NET_Q_SAM_LOGON_EX */
typedef struct net_q_sam_logon_info_ex {
	DOM_SAM_INFO_EX sam_id;
	uint16          validation_level;
	uint32 flags;
} NET_Q_SAM_LOGON_EX;

/* NET_R_SAM_LOGON */
typedef struct net_r_sam_logon_info {
	uint32 buffer_creds; /* undocumented buffer pointer */
	DOM_CRED srv_creds; /* server credentials.  server time stamp appears to be ignored. */
    
	uint16 switch_value; /* 3 - indicates type of USER INFO */
	NET_USER_INFO_3 *user;

	uint32 auth_resp; /* 1 - Authoritative response; 0 - Non-Auth? */

	NTSTATUS status; /* return code */
} NET_R_SAM_LOGON;

/* NET_R_SAM_LOGON_EX */
typedef struct net_r_sam_logon_info_ex {
	uint16 switch_value; /* 3 - indicates type of USER INFO */
	NET_USER_INFO_3 *user;

	uint32 auth_resp; /* 1 - Authoritative response; 0 - Non-Auth? */
	uint32 flags;

	NTSTATUS status; /* return code */
} NET_R_SAM_LOGON_EX;


/* NET_Q_SAM_LOGOFF */
typedef struct net_q_sam_logoff_info {
	DOM_SAM_INFO sam_id;
} NET_Q_SAM_LOGOFF;

/* NET_R_SAM_LOGOFF */
typedef struct net_r_sam_logoff_info {
	uint32 buffer_creds; /* undocumented buffer pointer */
	DOM_CRED srv_creds; /* server credentials.  server time stamp appears to be ignored. */
	NTSTATUS status; /* return code */
} NET_R_SAM_LOGOFF;

/* NET_Q_SAM_SYNC */
typedef struct net_q_sam_sync_info {
	UNISTR2 uni_srv_name; /* \\PDC */
	UNISTR2 uni_cli_name; /* BDC */
	DOM_CRED cli_creds;
	DOM_CRED ret_creds;

	uint32 database_id;
	uint32 restart_state;
	uint32 sync_context;

	uint32 max_size;       /* preferred maximum length */
} NET_Q_SAM_SYNC;

/* SAM_DELTA_HDR */
typedef struct sam_delta_hdr_info {
	uint16 type;  /* type of structure attached */
	uint16 type2;
	uint32 target_rid;

	uint32 type3;
	uint32 ptr_delta;
} SAM_DELTA_HDR;

/* LOCKOUT_STRING */
typedef struct account_lockout_string {
	uint32 array_size;
	uint32 offset;
	uint32 length;
/*	uint16 *bindata;	*/
	UINT64_S lockout_duration;
	UINT64_S reset_count;
	uint32 bad_attempt_lockout;
	uint32 dummy;
} LOCKOUT_STRING;

/* HDR_LOCKOUT_STRING */
typedef struct hdr_account_lockout_string {
	uint16 size;
	uint16 length;
	uint32 buffer;
} HDR_LOCKOUT_STRING;

/* SAM_DOMAIN_INFO (0x1) */
typedef struct sam_domain_info_info {
	UNIHDR hdr_dom_name;
	UNIHDR hdr_oem_info;

	UINT64_S force_logoff;
	uint16   min_pwd_len;
	uint16   pwd_history_len;
	UINT64_S max_pwd_age;
	UINT64_S min_pwd_age;
	UINT64_S dom_mod_count;
	NTTIME   creation_time;
	uint32   security_information;

	BUFHDR4 hdr_sec_desc; /* security descriptor */

	HDR_LOCKOUT_STRING hdr_account_lockout;

	UNIHDR hdr_unknown2;
	UNIHDR hdr_unknown3;
	UNIHDR hdr_unknown4;

	UNISTR2 uni_dom_name;
	UNISTR2 buf_oem_info; 

	RPC_DATA_BLOB buf_sec_desc;

	LOCKOUT_STRING account_lockout;

	UNISTR2 buf_unknown2;
	UNISTR2 buf_unknown3;
	UNISTR2 buf_unknown4;

	uint32 logon_chgpass;
	uint32 unknown6;
	uint32 unknown7;
	uint32 unknown8;
} SAM_DOMAIN_INFO;

/* SAM_GROUP_INFO (0x2) */
typedef struct sam_group_info_info {
	UNIHDR hdr_grp_name;
	DOM_GID gid;
	UNIHDR hdr_grp_desc;
	BUFHDR2 hdr_sec_desc;  /* security descriptor */
	uint8 reserved[48];

	UNISTR2 uni_grp_name;
	UNISTR2 uni_grp_desc;
	RPC_DATA_BLOB buf_sec_desc;
} SAM_GROUP_INFO;

/* SAM_PWD */
typedef struct sam_passwd_info {
	/* this structure probably contains password history */
	/* this is probably a count of lm/nt pairs */
	uint32 unk_0; /* 0x0000 0002 */

	UNIHDR hdr_lm_pwd;
	uint8  buf_lm_pwd[16];

	UNIHDR hdr_nt_pwd;
	uint8  buf_nt_pwd[16];

	UNIHDR hdr_empty_lm;
	UNIHDR hdr_empty_nt;
} SAM_PWD;

/* SAM_ACCOUNT_INFO (0x5) */
typedef struct sam_account_info_info {
	UNIHDR hdr_acct_name;
	UNIHDR hdr_full_name;

	uint32 user_rid;
	uint32 group_rid;

	UNIHDR hdr_home_dir;
	UNIHDR hdr_dir_drive;
	UNIHDR hdr_logon_script;
	UNIHDR hdr_acct_desc;
	UNIHDR hdr_workstations;

	NTTIME logon_time;
	NTTIME logoff_time;

	uint32 logon_divs; /* 0xA8 */
	uint32 ptr_logon_hrs;

	uint16 bad_pwd_count;
	uint16 logon_count;
	NTTIME pwd_last_set_time;
	NTTIME acct_expiry_time;

	uint32 acb_info;
	uint8 nt_pwd[16];
	uint8 lm_pwd[16];
	uint8 nt_pwd_present;
	uint8 lm_pwd_present;
	uint8 pwd_expired;

	UNIHDR hdr_comment;
	UNIHDR hdr_parameters;
	uint16 country;
	uint16 codepage;

	BUFHDR2 hdr_sec_desc;  /* security descriptor */

	UNIHDR  hdr_profile;
	UNIHDR  hdr_reserved[3];  /* space for more strings */
	uint32  dw_reserved[4];   /* space for more data - first two seem to
				     be an NTTIME */

	UNISTR2 uni_acct_name;
	UNISTR2 uni_full_name;
	UNISTR2 uni_home_dir;
	UNISTR2 uni_dir_drive;
	UNISTR2 uni_logon_script;
	UNISTR2 uni_acct_desc;
	UNISTR2 uni_workstations;

	uint32 unknown1; /* 0x4EC */
	uint32 unknown2; /* 0 */

	RPC_DATA_BLOB buf_logon_hrs;
	UNISTR2 uni_comment;
	UNISTR2 uni_parameters;
	SAM_PWD pass;
	RPC_DATA_BLOB buf_sec_desc;
	UNISTR2 uni_profile;
} SAM_ACCOUNT_INFO;

/* SAM_GROUP_MEM_INFO (0x8) */
typedef struct sam_group_mem_info_info {
	uint32 ptr_rids;
	uint32 ptr_attribs;
	uint32 num_members;
	uint8 unknown[16];

	uint32 num_members2;
	uint32 *rids;

	uint32 num_members3;
	uint32 *attribs;

} SAM_GROUP_MEM_INFO;

/* SAM_ALIAS_INFO (0x9) */
typedef struct sam_alias_info_info {
	UNIHDR hdr_als_name;
	uint32 als_rid;
	BUFHDR2 hdr_sec_desc;  /* security descriptor */
	UNIHDR hdr_als_desc;
	uint8 reserved[40];

	UNISTR2 uni_als_name;
	RPC_DATA_BLOB buf_sec_desc;
	UNISTR2 uni_als_desc;
} SAM_ALIAS_INFO;

/* SAM_ALIAS_MEM_INFO (0xC) */
typedef struct sam_alias_mem_info_info {
	uint32 num_members;
	uint32 ptr_members;
	uint8 unknown[16];

	uint32 num_sids;
	uint32 *ptr_sids;
	DOM_SID2 *sids;
} SAM_ALIAS_MEM_INFO;


/* SAM_DELTA_POLICY (0x0D) */
typedef struct {
	uint32   max_log_size; /* 0x5000 */
	UINT64_S audit_retention_period; /* 0 */
	uint32   auditing_mode; /* 0 */
	uint32   num_events;
	uint32   ptr_events;
	UNIHDR   hdr_dom_name;
	uint32   sid_ptr;

	uint32   paged_pool_limit; /* 0x02000000 */
	uint32   non_paged_pool_limit; /* 0x00100000 */
	uint32   min_workset_size; /* 0x00010000 */
	uint32   max_workset_size; /* 0x0f000000 */
	uint32   page_file_limit; /* 0 */
	UINT64_S time_limit; /* 0 */
	NTTIME   modify_time; /* 0x3c*/
	NTTIME   create_time; /* a7080110 */
	BUFHDR2  hdr_sec_desc;

	uint32   num_event_audit_options;
	uint32   event_audit_option;

	UNISTR2  domain_name;
	DOM_SID2 domain_sid;

	RPC_DATA_BLOB  buf_sec_desc;
} SAM_DELTA_POLICY;

/* SAM_DELTA_TRUST_DOMS */
typedef struct {
	uint32 buf_size;
	SEC_DESC *sec_desc;
	DOM_SID2 sid;
	UNIHDR hdr_domain;
	
	uint32 unknown0;
	uint32 unknown1;
	uint32 unknown2;
	
	uint32 buf_size2;
	uint32 ptr;

	uint32 unknown3;
	UNISTR2 domain;
} SAM_DELTA_TRUSTDOMS;

/* SAM_DELTA_PRIVS (0x10) */
typedef struct {
	DOM_SID2 sid;

	uint32 priv_count;
	uint32 priv_control;

	uint32 priv_attr_ptr;
	uint32 priv_name_ptr;

	uint32   paged_pool_limit; /* 0x02000000 */
	uint32   non_paged_pool_limit; /* 0x00100000 */
	uint32   min_workset_size; /* 0x00010000 */
	uint32   max_workset_size; /* 0x0f000000 */
	uint32   page_file_limit; /* 0 */
	UINT64_S time_limit; /* 0 */
	uint32   system_flags; /* 1 */
	BUFHDR2  hdr_sec_desc;
	
	uint32 buf_size2;
	
	uint32 attribute_count;
	uint32 *attributes;
	
	uint32 privlist_count;
	UNIHDR *hdr_privslist;
	UNISTR2 *uni_privslist;

	RPC_DATA_BLOB buf_sec_desc;
} SAM_DELTA_PRIVS;

/* SAM_DELTA_SECRET */
typedef struct {
	uint32 buf_size;
	SEC_DESC *sec_desc;
	UNISTR2 secret;

	uint32 count1;
	uint32 count2;
	uint32 ptr;
	NTTIME time1;
	uint32 count3;
	uint32 count4;
	uint32 ptr2;
	NTTIME time2;
	uint32 unknow1;

	uint32 buf_size2;
	uint32 ptr3;
	uint32 unknow2; /* 0x0 12 times */

	uint32 chal_len;
	uint32 reserved1; /* 0 */
	uint32 chal_len2;
	uint8 chal[16];

	uint32 key_len;
	uint32 reserved2; /* 0 */
	uint32 key_len2;
	uint8 key[8];

	uint32 buf_size3;
	SEC_DESC *sec_desc2;
} SAM_DELTA_SECRET;

/* SAM_DELTA_MOD_COUNT (0x16) */
typedef struct {
        uint32 seqnum;
        uint32 dom_mod_count_ptr;
	UINT64_S dom_mod_count;  /* domain mod count at last sync */
} SAM_DELTA_MOD_COUNT;

typedef union sam_delta_ctr_info {
	SAM_DOMAIN_INFO    domain_info ;
	SAM_GROUP_INFO     group_info  ;
	SAM_ACCOUNT_INFO   account_info;
	SAM_GROUP_MEM_INFO grp_mem_info;
	SAM_ALIAS_INFO     alias_info  ;
	SAM_ALIAS_MEM_INFO als_mem_info;
	SAM_DELTA_POLICY   policy_info;
	SAM_DELTA_PRIVS    privs_info;
	SAM_DELTA_MOD_COUNT mod_count;
	SAM_DELTA_TRUSTDOMS trustdoms_info;
	SAM_DELTA_SECRET   secret_info;
} SAM_DELTA_CTR;

/* NET_R_SAM_SYNC */
typedef struct net_r_sam_sync_info {
	DOM_CRED srv_creds;

	uint32 sync_context;

	uint32 ptr_deltas;
	uint32 num_deltas;
	uint32 ptr_deltas2;
	uint32 num_deltas2;

	SAM_DELTA_HDR *hdr_deltas;
	SAM_DELTA_CTR *deltas;

	NTSTATUS status;
} NET_R_SAM_SYNC;

/* NET_Q_SAM_DELTAS */
typedef struct net_q_sam_deltas_info {
	UNISTR2 uni_srv_name;
	UNISTR2 uni_cli_name;
	DOM_CRED cli_creds;
	DOM_CRED ret_creds;

	uint32 database_id;
	UINT64_S dom_mod_count;  /* domain mod count at last sync */

	uint32 max_size;       /* preferred maximum length */
} NET_Q_SAM_DELTAS;

/* NET_R_SAM_DELTAS */
typedef struct net_r_sam_deltas_info {
	DOM_CRED srv_creds;

	UINT64_S dom_mod_count;   /* new domain mod count */

	uint32 ptr_deltas;
	uint32 num_deltas;
	uint32 num_deltas2;

	SAM_DELTA_HDR *hdr_deltas;
	SAM_DELTA_CTR *deltas;

	NTSTATUS status;
} NET_R_SAM_DELTAS;

/* NET_Q_DSR_GETDCNAME - Ask a DC for a trusted DC name and its address */
typedef struct net_q_dsr_getdcname {
	uint32 ptr_server_unc;
	UNISTR2 uni_server_unc;
	uint32 ptr_domain_name;
	UNISTR2 uni_domain_name;
	uint32 ptr_domain_guid;
	struct uuid *domain_guid;
	uint32 ptr_site_guid;
	struct uuid *site_guid;
	uint32 flags;
} NET_Q_DSR_GETDCNAME;

/* NET_R_DSR_GETDCNAME - Ask a DC for a trusted DC name and its address */
typedef struct net_r_dsr_getdcname {
	uint32 ptr_dc_unc;
	UNISTR2 uni_dc_unc;
	uint32 ptr_dc_address;
	UNISTR2 uni_dc_address;
	int32 dc_address_type;
	struct uuid domain_guid;
	uint32 ptr_domain_name;
	UNISTR2 uni_domain_name;
	uint32 ptr_forest_name;
	UNISTR2 uni_forest_name;
	uint32 dc_flags;
	uint32 ptr_dc_site_name;
	UNISTR2 uni_dc_site_name;
	uint32 ptr_client_site_name;
	UNISTR2 uni_client_site_name;
	WERROR result;
} NET_R_DSR_GETDCNAME;

/* NET_Q_DSR_GESITENAME */
typedef struct net_q_dsr_getsitename {
	uint32 ptr_computer_name;
	UNISTR2 uni_computer_name;
} NET_Q_DSR_GETSITENAME;

/* NET_R_DSR_GETSITENAME */
typedef struct net_r_dsr_getsitename {
	uint32 ptr_site_name;
	UNISTR2 uni_site_name;
	WERROR result;
} NET_R_DSR_GETSITENAME;


#endif /* _RPC_NETLOGON_H */
