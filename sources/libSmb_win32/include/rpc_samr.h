/* 
   Unix SMB/CIFS implementation.
   SMB parameters and setup
   Copyright (C) Andrew Tridgell              1992-2000
   Copyright (C) Luke Kenneth Casson Leighton 1996-2000
   Copyright (C) Paul Ashton                  1997-2000
   Copyright (C) Jean François Micouleau      1998-2001
   Copyright (C) Jim McDonough <jmcd@us.ibm.com> 2002
   
   
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

#ifndef _RPC_SAMR_H /* _RPC_SAMR_H */
#define _RPC_SAMR_H 

/*******************************************************************
 the following information comes from a QuickView on samsrv.dll,
 and gives an idea of exactly what is needed:
 
x SamrAddMemberToAlias
x SamrAddMemberToGroup
SamrAddMultipleMembersToAlias
x SamrChangePasswordUser
x SamrCloseHandle
x SamrConnect
x SamrCreateAliasInDomain
x SamrCreateGroupInDomain
x SamrCreateUserInDomain
? SamrDeleteAlias
SamrDeleteGroup
x SamrDeleteUser
x SamrEnumerateAliasesInDomain
SamrEnumerateDomainsInSamServer
x SamrEnumerateGroupsInDomain
x SamrEnumerateUsersInDomain
SamrGetUserDomainPasswordInformation
SamrLookupDomainInSamServer
? SamrLookupIdsInDomain
x SamrLookupNamesInDomain
x SamrOpenAlias
x SamrOpenDomain
x SamrOpenGroup
x SamrOpenUser
x SamrQueryDisplayInformation
x SamrQueryInformationAlias
SamrQueryInformationDomain
? SamrQueryInformationUser
x SamrQuerySecurityObject
SamrRemoveMemberFromAlias
SamrRemoveMemberFromForiegnDomain
SamrRemoveMemberFromGroup
SamrRemoveMultipleMembersFromAlias
x SamrSetInformationAlias
SamrSetInformationDomain
x SamrSetInformationGroup
x SamrSetInformationUser
SamrSetMemberAttributesOfGroup
SamrSetSecurityObject
SamrShutdownSamServer
SamrTestPrivateFunctionsDomain
SamrTestPrivateFunctionsUser

********************************************************************/

#define SAMR_CONNECT_ANON      0x00
#define SAMR_CLOSE_HND         0x01
#define SAMR_SET_SEC_OBJECT    0x02
#define SAMR_QUERY_SEC_OBJECT  0x03

#define SAMR_UNKNOWN_4         0x04 /* profile info? */
#define SAMR_LOOKUP_DOMAIN     0x05
#define SAMR_ENUM_DOMAINS      0x06
#define SAMR_OPEN_DOMAIN       0x07
#define SAMR_QUERY_DOMAIN_INFO 0x08
#define SAMR_SET_DOMAIN_INFO   0x09

#define SAMR_CREATE_DOM_GROUP  0x0a
#define SAMR_ENUM_DOM_GROUPS   0x0b
#define SAMR_ENUM_DOM_USERS    0x0d
#define SAMR_CREATE_DOM_ALIAS  0x0e
#define SAMR_ENUM_DOM_ALIASES  0x0f
#define SAMR_QUERY_USERALIASES 0x10

#define SAMR_LOOKUP_NAMES      0x11
#define SAMR_LOOKUP_RIDS       0x12

#define SAMR_OPEN_GROUP        0x13
#define SAMR_QUERY_GROUPINFO   0x14
#define SAMR_SET_GROUPINFO     0x15
#define SAMR_ADD_GROUPMEM      0x16
#define SAMR_DELETE_DOM_GROUP  0x17
#define SAMR_DEL_GROUPMEM      0x18
#define SAMR_QUERY_GROUPMEM    0x19
#define SAMR_UNKNOWN_1A        0x1a

#define SAMR_OPEN_ALIAS        0x1b
#define SAMR_QUERY_ALIASINFO   0x1c
#define SAMR_SET_ALIASINFO     0x1d
#define SAMR_DELETE_DOM_ALIAS  0x1e
#define SAMR_ADD_ALIASMEM      0x1f
#define SAMR_DEL_ALIASMEM      0x20
#define SAMR_QUERY_ALIASMEM    0x21

#define SAMR_OPEN_USER         0x22
#define SAMR_DELETE_DOM_USER   0x23
#define SAMR_QUERY_USERINFO    0x24
#define SAMR_SET_USERINFO2     0x25
#define SAMR_QUERY_USERGROUPS  0x27

#define SAMR_QUERY_DISPINFO    0x28
#define SAMR_UNKNOWN_29        0x29
#define SAMR_UNKNOWN_2a        0x2a
#define SAMR_UNKNOWN_2b        0x2b
#define SAMR_GET_USRDOM_PWINFO 0x2c
#define SAMR_REMOVE_SID_FOREIGN_DOMAIN        0x2d
#define SAMR_QUERY_DOMAIN_INFO2  0x2e /* looks like an alias for SAMR_QUERY_DOMAIN_INFO */
#define SAMR_UNKNOWN_2f        0x2f
#define SAMR_QUERY_DISPINFO3   0x30 /* Alias for SAMR_QUERY_DISPINFO
				       with info level 3 */
#define SAMR_UNKNOWN_31        0x31
#define SAMR_CREATE_USER       0x32
#define SAMR_QUERY_DISPINFO4   0x33 /* Alias for SAMR_QUERY_DISPINFO
				       with info level 4 */
#define SAMR_ADDMULTI_ALIASMEM 0x34

#define SAMR_UNKNOWN_35        0x35
#define SAMR_UNKNOWN_36        0x36
#define SAMR_CHGPASSWD_USER    0x37
#define SAMR_GET_DOM_PWINFO    0x38
#define SAMR_CONNECT           0x39
#define SAMR_SET_USERINFO      0x3A
#define SAMR_CONNECT4          0x3E
#define SAMR_CHGPASSWD_USER3   0x3F
#define SAMR_CONNECT5          0x40

typedef struct logon_hours_info
{
	uint32 max_len; /* normally 1260 bytes */
	uint32 offset;
	uint32 len; /* normally 21 bytes */
	uint8 hours[32];

} LOGON_HRS;

/* SAM_USER_INFO_23 */
typedef struct sam_user_info_23
{
	/* TIMES MAY NOT IN RIGHT ORDER!!!! */
	NTTIME logon_time;            /* logon time */
	NTTIME logoff_time;           /* logoff time */
	NTTIME kickoff_time;          /* kickoff time */
	NTTIME pass_last_set_time;    /* password last set time */
	NTTIME pass_can_change_time;  /* password can change time */
	NTTIME pass_must_change_time; /* password must change time */

	UNIHDR hdr_user_name;    /* NULL - user name unicode string header */
	UNIHDR hdr_full_name;    /* user's full name unicode string header */
	UNIHDR hdr_home_dir;     /* home directory unicode string header */
	UNIHDR hdr_dir_drive;    /* home drive unicode string header */
	UNIHDR hdr_logon_script; /* logon script unicode string header */
	UNIHDR hdr_profile_path; /* profile path unicode string header */
	UNIHDR hdr_acct_desc  ;  /* user description */
	UNIHDR hdr_workstations; /* comma-separated workstations user can log in from */
	UNIHDR hdr_unknown_str ; /* don't know what this is, yet. */
	UNIHDR hdr_munged_dial ; /* munged path name and dial-back tel number */

	uint8 lm_pwd[16];    /* lm user passwords */
	uint8 nt_pwd[16];    /* nt user passwords */

	uint32 user_rid;      /* Primary User ID */
	uint32 group_rid;     /* Primary Group ID */

	uint32 acb_info; /* account info (ACB_xxxx bit-mask) */

	uint32 fields_present; /* 0x09f8 27fa */

	uint16 logon_divs; /* 0x0000 00a8 which is 168 which is num hrs in a week */
	/* uint8 pad[2] */
	uint32 ptr_logon_hrs; /* pointer to logon hours */

	/* Was unknown_5. */
	uint16 bad_password_count;
	uint16 logon_count;

	uint8 padding1[6];
		
	uint8 passmustchange; /* 0x00 must change = 0x01 */

	uint8 padding2;

	uint8 pass[516];

	UNISTR2 uni_user_name;    /* NULL - username unicode string */
	UNISTR2 uni_full_name;    /* user's full name unicode string */
	UNISTR2 uni_home_dir;     /* home directory unicode string */
	UNISTR2 uni_dir_drive;    /* home directory drive unicode string */
	UNISTR2 uni_logon_script; /* logon script unicode string */
	UNISTR2 uni_profile_path; /* profile path unicode string */
	UNISTR2 uni_acct_desc  ;  /* user description unicode string */
	UNISTR2 uni_workstations; /* login from workstations unicode string */
	UNISTR2 uni_unknown_str ; /* don't know what this is, yet. */
	UNISTR2 uni_munged_dial ; /* munged path name and dial-back tel no */

	LOGON_HRS logon_hrs;

} SAM_USER_INFO_23;

/* SAM_USER_INFO_24 */
typedef struct sam_user_info_24
{
	uint8 pass[516];
	uint16 pw_len;
} SAM_USER_INFO_24;

/*
 * NB. This structure is *definately* incorrect. It's my best guess
 * currently for W2K SP2. The password field is encrypted in a different
 * way than normal... And there are definately other problems. JRA.
 */

/* SAM_USER_INFO_25 */
typedef struct sam_user_info_25
{
	/* TIMES MAY NOT IN RIGHT ORDER!!!! */
	NTTIME logon_time;            /* logon time */
	NTTIME logoff_time;           /* logoff time */
	NTTIME kickoff_time;          /* kickoff time */
	NTTIME pass_last_set_time;    /* password last set time */
	NTTIME pass_can_change_time;  /* password can change time */
	NTTIME pass_must_change_time; /* password must change time */

	UNIHDR hdr_user_name;    /* NULL - user name unicode string header */
	UNIHDR hdr_full_name;    /* user's full name unicode string header */
	UNIHDR hdr_home_dir;     /* home directory unicode string header */
	UNIHDR hdr_dir_drive;    /* home drive unicode string header */
	UNIHDR hdr_logon_script; /* logon script unicode string header */
	UNIHDR hdr_profile_path; /* profile path unicode string header */
	UNIHDR hdr_acct_desc  ;  /* user description */
	UNIHDR hdr_workstations; /* comma-separated workstations user can log in from */
	UNIHDR hdr_unknown_str ; /* don't know what this is, yet. */
	UNIHDR hdr_munged_dial ; /* munged path name and dial-back tel number */

	uint8 lm_pwd[16];    /* lm user passwords */
	uint8 nt_pwd[16];    /* nt user passwords */

	uint32 user_rid;      /* Primary User ID */
	uint32 group_rid;     /* Primary Group ID */

	uint32 acb_info; /* account info (ACB_xxxx bit-mask) */
	uint32 fields_present;

	uint32 unknown_5[5];

	uint8 pass[532];

	UNISTR2 uni_user_name;    /* NULL - username unicode string */
	UNISTR2 uni_full_name;    /* user's full name unicode string */
	UNISTR2 uni_home_dir;     /* home directory unicode string */
	UNISTR2 uni_dir_drive;    /* home directory drive unicode string */
	UNISTR2 uni_logon_script; /* logon script unicode string */
	UNISTR2 uni_profile_path; /* profile path unicode string */
	UNISTR2 uni_acct_desc  ;  /* user description unicode string */
	UNISTR2 uni_workstations; /* login from workstations unicode string */
	UNISTR2 uni_unknown_str ; /* don't know what this is, yet. */
	UNISTR2 uni_munged_dial ; /* munged path name and dial-back tel no */
} SAM_USER_INFO_25;

/* SAM_USER_INFO_26 */
typedef struct sam_user_info_26
{
	uint8 pass[532];
	uint8 pw_len;
} SAM_USER_INFO_26;


/* SAM_USER_INFO_21 */
typedef struct sam_user_info_21
{
	NTTIME logon_time;            /* logon time */
	NTTIME logoff_time;           /* logoff time */
	NTTIME kickoff_time;          /* kickoff time */
	NTTIME pass_last_set_time;    /* password last set time */
	NTTIME pass_can_change_time;  /* password can change time */
	NTTIME pass_must_change_time; /* password must change time */

	UNIHDR hdr_user_name;    /* username unicode string header */
	UNIHDR hdr_full_name;    /* user's full name unicode string header */
	UNIHDR hdr_home_dir;     /* home directory unicode string header */
	UNIHDR hdr_dir_drive;    /* home drive unicode string header */
	UNIHDR hdr_logon_script; /* logon script unicode string header */
	UNIHDR hdr_profile_path; /* profile path unicode string header */
	UNIHDR hdr_acct_desc  ;  /* user description */
	UNIHDR hdr_workstations; /* comma-separated workstations user can log in from */
	UNIHDR hdr_unknown_str ; /* don't know what this is, yet. */
	UNIHDR hdr_munged_dial ; /* munged path name and dial-back tel number */

	uint8 lm_pwd[16];    /* lm user passwords */
	uint8 nt_pwd[16];    /* nt user passwords */

	uint32 user_rid;      /* Primary User ID */
	uint32 group_rid;     /* Primary Group ID */

	uint32 acb_info; /* account info (ACB_xxxx bit-mask) */

	/* Was unknown_3 */
	uint32 fields_present; /* 0x00ff ffff */

	uint16 logon_divs; /* 0x0000 00a8 which is 168 which is num hrs in a week */
	/* uint8 pad[2] */
	uint32 ptr_logon_hrs; /* unknown pointer */

	/* Was unknown_5. */
	uint16 bad_password_count;
	uint16 logon_count;

	uint8 padding1[6];
		
	uint8 passmustchange; /* 0x00 must change = 0x01 */

	uint8 padding2;

	UNISTR2 uni_user_name;    /* username unicode string */
	UNISTR2 uni_full_name;    /* user's full name unicode string */
	UNISTR2 uni_home_dir;     /* home directory unicode string */
	UNISTR2 uni_dir_drive;    /* home directory drive unicode string */
	UNISTR2 uni_logon_script; /* logon script unicode string */
	UNISTR2 uni_profile_path; /* profile path unicode string */
	UNISTR2 uni_acct_desc  ;  /* user description unicode string */
	UNISTR2 uni_workstations; /* login from workstations unicode string */
	UNISTR2 uni_unknown_str ; /* don't know what this is, yet. */
	UNISTR2 uni_munged_dial ; /* munged path name and dial-back tel number */

	LOGON_HRS logon_hrs;

} SAM_USER_INFO_21;

#define PASS_MUST_CHANGE_AT_NEXT_LOGON	0x01
#define PASS_DONT_CHANGE_AT_NEXT_LOGON	0x00

/* SAM_USER_INFO_20 */
typedef struct sam_user_info_20
{
	UNIHDR hdr_munged_dial ; /* munged path name and dial-back tel number */

	UNISTR2 uni_munged_dial ; /* munged path name and dial-back tel number */

} SAM_USER_INFO_20;

/* SAM_USER_INFO_18 */
typedef struct sam_user_info_18
{
	uint8 lm_pwd[16];    /* lm user passwords */
	uint8 nt_pwd[16];    /* nt user passwords */

	uint8 lm_pwd_active; 
	uint8 nt_pwd_active; 

} SAM_USER_INFO_18;

/* SAM_USER_INFO_17 */
typedef struct sam_user_info_17
{
	uint8  padding_0[16];  /* 0 - padding 16 bytes */
	NTTIME expiry;         /* expiry time or something? */
	uint8  padding_1[24];  /* 0 - padding 24 bytes */

	UNIHDR hdr_mach_acct;  /* unicode header for machine account */
	uint32 padding_2;      /* 0 - padding 4 bytes */

	uint32 ptr_1;          /* pointer */
	uint8  padding_3[32];  /* 0 - padding 32 bytes */
	uint32 padding_4;      /* 0 - padding 4 bytes */

	uint32 ptr_2;          /* pointer */
	uint32 padding_5;      /* 0 - padding 4 bytes */

	uint32 ptr_3;          /* pointer */
	uint8  padding_6[32];  /* 0 - padding 32 bytes */

	uint32 rid_user;       /* user RID */
	uint32 rid_group;      /* group RID */

	uint16 acct_ctrl;      /* 0080 - ACB_XXXX */
	uint16 unknown_3;      /* 16 bit padding */

	uint16 unknown_4;      /* 0x003f      - 16 bit unknown */
	uint16 unknown_5;      /* 0x003c      - 16 bit unknown */

	uint8  padding_7[16];  /* 0 - padding 16 bytes */
	uint32 padding_8;      /* 0 - padding 4 bytes */
	
	UNISTR2 uni_mach_acct; /* unicode string for machine account */

	uint8  padding_9[48];  /* 0 - padding 48 bytes */

} SAM_USER_INFO_17;


/* SAM_USER_INFO_16 */
typedef struct sam_user_info_16
{
	uint32 acb_info;

} SAM_USER_INFO_16;


/* SAM_USER_INFO_7 */
typedef struct sam_user_info_7
{
	UNIHDR hdr_name;  /* unicode header for name */
	UNISTR2 uni_name; /* unicode string for name */

} SAM_USER_INFO_7;


/* SAM_USER_INFO_9 */
typedef struct sam_user_info_9
{
	uint32 rid_group;     /* Primary Group RID */
} SAM_USER_INFO_9;


/* SAMR_Q_CLOSE_HND - probably a policy handle close */
typedef struct q_samr_close_hnd_info
{
    POLICY_HND pol;          /* policy handle */

} SAMR_Q_CLOSE_HND;


/* SAMR_R_CLOSE_HND - probably a policy handle close */
typedef struct r_samr_close_hnd_info
{
	POLICY_HND pol;       /* policy handle */
	NTSTATUS status;         /* return status */

} SAMR_R_CLOSE_HND;


/****************************************************************************
SAMR_Q_GET_USRDOM_PWINFO - a "set user info" occurs just after this
*****************************************************************************/

/* SAMR_Q_GET_USRDOM_PWINFO */
typedef struct q_samr_usrdom_pwinfo_info
{
	POLICY_HND user_pol;          /* policy handle */

} SAMR_Q_GET_USRDOM_PWINFO;


/****************************************************************************
SAMR_R_GET_USRDOM_PWINFO - a "set user info" occurs just after this
*****************************************************************************/

/* SAMR_R_GET_USRDOM_PWINFO */
typedef struct r_samr_usrdom_pwinfo_info
{
	uint16 min_pwd_length;
	uint16 unknown_1; /* 0x0016 or 0x0015 */
	uint32 password_properties;
	NTSTATUS status; 

} SAMR_R_GET_USRDOM_PWINFO;

/****************************************************************************
SAMR_Q_SET_SEC_OBJ - info level 4.
*****************************************************************************/

/* SAMR_Q_SET_SEC_OBJ - */
typedef struct q_samr_set_sec_obj_info
{
	POLICY_HND pol;          /* policy handle */
	uint32 sec_info;         /* xxxx_SECURITY_INFORMATION 0x0000 0004 */
	SEC_DESC_BUF *buf;

} SAMR_Q_SET_SEC_OBJ;

/* SAMR_R_SET_SEC_OBJ - */
typedef struct r_samr_set_sec_obj_info
{
	NTSTATUS status;         /* return status */

} SAMR_R_SET_SEC_OBJ;


/****************************************************************************
SAMR_Q_QUERY_SEC_OBJ - info level 4.  returns SIDs.
*****************************************************************************/

/* SAMR_Q_QUERY_SEC_OBJ - probably get domain info... */
typedef struct q_samr_query_sec_obj_info
{
	POLICY_HND user_pol;          /* policy handle */
	uint32 sec_info;     /* xxxx_SECURITY_INFORMATION 0x0000 0004 */

} SAMR_Q_QUERY_SEC_OBJ;

/* SAMR_R_QUERY_SEC_OBJ - probably an open */
typedef struct r_samr_query_sec_obj_info
{
	uint32 ptr;
	SEC_DESC_BUF *buf;

	NTSTATUS status;         /* return status */

} SAMR_R_QUERY_SEC_OBJ;


/****************************************************************************
SAMR_Q_QUERY_DOMAIN_INFO - probably a query on domain group info.
*****************************************************************************/

/* SAMR_Q_QUERY_DOMAIN_INFO - */
typedef struct q_samr_query_domain_info
{
	POLICY_HND domain_pol;   /* policy handle */
	uint16 switch_value;     /* 0x0002, 0x0001 */

} SAMR_Q_QUERY_DOMAIN_INFO;

typedef struct sam_unknown_info_1_inf
{
	uint16 min_length_password;
	uint16 password_history;
	uint32 password_properties;
	NTTIME expire;
	NTTIME min_passwordage;

} SAM_UNK_INFO_1;

typedef struct sam_unknown_info_2_inf
{
	NTTIME logout; /* whether users are forcibly disconnected when logon hours expire */
	UNIHDR hdr_comment; /* comment according to samba4 idl */
	UNIHDR hdr_domain; /* domain name unicode header */
	UNIHDR hdr_server; /* server name unicode header */

	/* put all the data in here, at the moment, including what the above
	   pointer is referring to
	 */

	UINT64_S seq_num;
	
	uint32 unknown_4; /* 0x0000 0001 */
	uint32 server_role;
	uint32 unknown_6; /* 0x0000 0001 */
	uint32 num_domain_usrs; /* number of users in domain */
	uint32 num_domain_grps; /* number of domain groups in domain */
	uint32 num_local_grps; /* number of local groups in domain */

	UNISTR2 uni_comment; /* comment unicode string */
	UNISTR2 uni_domain; /* domain name unicode string */
	UNISTR2 uni_server; /* server name unicode string */

} SAM_UNK_INFO_2;

typedef struct sam_unknown_info_3_info
{
	NTTIME logout;	
	/* 0x8000 0000 */ /* DON'T forcibly disconnect remote users from server when logon hours expire*/
	/* 0x0000 0000 */ /* forcibly disconnect remote users from server when logon hours expire*/

} SAM_UNK_INFO_3;

typedef struct sam_unknown_info_4_inf
{
	UNIHDR hdr_comment; /* comment according to samba4 idl */
	UNISTR2 uni_comment; /* comment unicode string */

} SAM_UNK_INFO_4;

typedef struct sam_unknown_info_5_inf
{
	UNIHDR hdr_domain; /* domain name unicode header */
	UNISTR2 uni_domain; /* domain name unicode string */

} SAM_UNK_INFO_5;

typedef struct sam_unknown_info_6_info
{
	UNIHDR hdr_server; /* server name unicode header */
	UNISTR2 uni_server; /* server name unicode string */

} SAM_UNK_INFO_6;

typedef struct sam_unknown_info_7_info
{
	uint16 server_role;

} SAM_UNK_INFO_7;

typedef struct sam_unknown_info_8_info
{
	UINT64_S seq_num;
	NTTIME domain_create_time;

} SAM_UNK_INFO_8;

typedef struct sam_unknown_info_9_info
{
	uint32 unknown;

} SAM_UNK_INFO_9;

typedef struct sam_unknown_info_12_inf
{
	NTTIME duration;
	NTTIME reset_count;
	uint16 bad_attempt_lockout;

} SAM_UNK_INFO_12;

typedef struct sam_unknown_info_13_info
{
	UINT64_S seq_num;
	NTTIME domain_create_time;
	uint32 unknown1;
	uint32 unknown2;

} SAM_UNK_INFO_13;

typedef struct sam_unknown_ctr_info
{
	union
	{
		SAM_UNK_INFO_1 inf1;
		SAM_UNK_INFO_2 inf2;
		SAM_UNK_INFO_3 inf3;
		SAM_UNK_INFO_4 inf4;
		SAM_UNK_INFO_5 inf5;
		SAM_UNK_INFO_6 inf6;
		SAM_UNK_INFO_7 inf7;
		SAM_UNK_INFO_8 inf8;
		SAM_UNK_INFO_9 inf9;
		SAM_UNK_INFO_12 inf12;
		SAM_UNK_INFO_13 inf13;

	} info;

} SAM_UNK_CTR;


/* SAMR_R_QUERY_DOMAIN_INFO - */
typedef struct r_samr_query_domain_info
{
	uint32 ptr_0;
	uint16 switch_value; /* same as in query */

	SAM_UNK_CTR *ctr;

	NTSTATUS status;         /* return status */

} SAMR_R_QUERY_DOMAIN_INFO;


/* SAMR_Q_LOOKUP_DOMAIN - obtain SID for a local domain */
typedef struct q_samr_lookup_domain_info
{
	POLICY_HND connect_pol;

	UNIHDR  hdr_domain;
	UNISTR2 uni_domain;

} SAMR_Q_LOOKUP_DOMAIN;


/* SAMR_R_LOOKUP_DOMAIN */
typedef struct r_samr_lookup_domain_info
{
	uint32   ptr_sid;
	DOM_SID2 dom_sid;

	NTSTATUS status;

} SAMR_R_LOOKUP_DOMAIN;


/****************************************************************************
SAMR_Q_OPEN_DOMAIN - unknown_0 values seen associated with SIDs:

0x0000 03f1 and a specific   domain sid - S-1-5-21-44c01ca6-797e5c3d-33f83fd0
0x0000 0200 and a specific   domain sid - S-1-5-21-44c01ca6-797e5c3d-33f83fd0
*****************************************************************************/

/* SAMR_Q_OPEN_DOMAIN */
typedef struct q_samr_open_domain_info
{
	POLICY_HND pol;   /* policy handle */
	uint32 flags;               /* 0x2000 0000; 0x0000 0211; 0x0000 0280; 0x0000 0200 - flags? */
	DOM_SID2 dom_sid;         /* domain SID */

} SAMR_Q_OPEN_DOMAIN;


/* SAMR_R_OPEN_DOMAIN - probably an open */
typedef struct r_samr_open_domain_info
{
	POLICY_HND domain_pol; /* policy handle associated with the SID */
	NTSTATUS status;         /* return status */

} SAMR_R_OPEN_DOMAIN;

#define MAX_SAM_ENTRIES_W2K 0x400
#define MAX_SAM_ENTRIES_W95 50
/* The following should be the greater of the preceeding two. */
#define MAX_SAM_ENTRIES MAX_SAM_ENTRIES_W2K

typedef struct samr_entry_info
{
	uint32 rid;
	UNIHDR hdr_name;

} SAM_ENTRY;


/* SAMR_Q_ENUM_DOMAINS - SAM rids and names */
typedef struct q_samr_enum_domains_info
{
	POLICY_HND pol;     /* policy handle */

	uint32 start_idx;   /* enumeration handle */
	uint32 max_size;    /* 0x0000 ffff */

} SAMR_Q_ENUM_DOMAINS;

/* SAMR_R_ENUM_DOMAINS - SAM rids and Domain names */
typedef struct r_samr_enum_domains_info
{
	uint32 next_idx;     /* next starting index required for enum */
	uint32 ptr_entries1;  

	uint32 num_entries2;
	uint32 ptr_entries2;

	uint32 num_entries3;

	SAM_ENTRY *sam;
	UNISTR2 *uni_dom_name;

	uint32 num_entries4;

	NTSTATUS status;

} SAMR_R_ENUM_DOMAINS;

/* SAMR_Q_ENUM_DOM_USERS - SAM rids and names */
typedef struct q_samr_enum_dom_users_info
{
	POLICY_HND pol;          /* policy handle */

	uint32 start_idx;   /* number of values (0 indicates unlimited?) */
	uint32 acb_mask;          /* 0x0000 indicates all */

	uint32 max_size;              /* 0x0000 ffff */

} SAMR_Q_ENUM_DOM_USERS;


/* SAMR_R_ENUM_DOM_USERS - SAM rids and names */
typedef struct r_samr_enum_dom_users_info
{
	uint32 next_idx;     /* next starting index required for enum */
	uint32 ptr_entries1;  

	uint32 num_entries2;
	uint32 ptr_entries2;

	uint32 num_entries3;

	SAM_ENTRY *sam;
	UNISTR2 *uni_acct_name;

	uint32 num_entries4;

	NTSTATUS status;

} SAMR_R_ENUM_DOM_USERS;


/* SAMR_Q_ENUM_DOM_GROUPS - SAM rids and names */
typedef struct q_samr_enum_dom_groups_info
{
	POLICY_HND pol;          /* policy handle */

	/* this is possibly an enumeration context handle... */
	uint32 start_idx;         /* 0x0000 0000 */

	uint32 max_size;              /* 0x0000 ffff */

} SAMR_Q_ENUM_DOM_GROUPS;


/* SAMR_R_ENUM_DOM_GROUPS - SAM rids and names */
typedef struct r_samr_enum_dom_groups_info
{
	uint32 next_idx;
	uint32 ptr_entries1;

	uint32 num_entries2;
	uint32 ptr_entries2;

	uint32 num_entries3;

	SAM_ENTRY *sam;
	UNISTR2 *uni_grp_name;

	uint32 num_entries4;

	NTSTATUS status;

} SAMR_R_ENUM_DOM_GROUPS;


/* SAMR_Q_ENUM_DOM_ALIASES - SAM rids and names */
typedef struct q_samr_enum_dom_aliases_info
{
	POLICY_HND pol;          /* policy handle */

	/* this is possibly an enumeration context handle... */
	uint32 start_idx;         /* 0x0000 0000 */

	uint32 max_size;              /* 0x0000 ffff */

} SAMR_Q_ENUM_DOM_ALIASES;


/* SAMR_R_ENUM_DOM_ALIASES - SAM rids and names */
typedef struct r_samr_enum_dom_aliases_info
{
	uint32 next_idx;
	uint32 ptr_entries1;

	uint32 num_entries2;
	uint32 ptr_entries2;

	uint32 num_entries3;

	SAM_ENTRY *sam;
	UNISTR2 *uni_grp_name;

	uint32 num_entries4;

	NTSTATUS status;

} SAMR_R_ENUM_DOM_ALIASES;


/* -- Level 1 Display Info - User Information -- */

typedef struct samr_entry_info1
{
	uint32 user_idx;

	uint32 rid_user;
	uint32 acb_info;

	UNIHDR hdr_acct_name;
	UNIHDR hdr_user_name;
	UNIHDR hdr_user_desc;

} SAM_ENTRY1;

typedef struct samr_str_entry_info1
{
	UNISTR2 uni_acct_name;
	UNISTR2 uni_full_name;
	UNISTR2 uni_acct_desc;

} SAM_STR1;

typedef struct sam_entry_info_1
{
	SAM_ENTRY1 *sam;
	SAM_STR1   *str;

} SAM_DISPINFO_1;


/* -- Level 2 Display Info - Trust Account Information -- */

typedef struct samr_entry_info2
{
	uint32 user_idx;

	uint32 rid_user;
	uint32 acb_info;

	UNIHDR hdr_srv_name;
	UNIHDR hdr_srv_desc;

} SAM_ENTRY2;

typedef struct samr_str_entry_info2
{
	UNISTR2 uni_srv_name;
	UNISTR2 uni_srv_desc;

} SAM_STR2;

typedef struct sam_entry_info_2
{
	SAM_ENTRY2 *sam;
	SAM_STR2   *str;

} SAM_DISPINFO_2;


/* -- Level 3 Display Info - Domain Group Information -- */

typedef struct samr_entry_info3
{
	uint32 grp_idx;

	uint32 rid_grp;
	uint32 attr;     /* SE_GROUP_xxx, usually 7 */

	UNIHDR hdr_grp_name;
	UNIHDR hdr_grp_desc;

} SAM_ENTRY3;

typedef struct samr_str_entry_info3
{
	UNISTR2 uni_grp_name;
	UNISTR2 uni_grp_desc;

} SAM_STR3;

typedef struct sam_entry_info_3
{
	SAM_ENTRY3 *sam;
	SAM_STR3   *str;

} SAM_DISPINFO_3;


/* -- Level 4 Display Info - User List (ASCII) -- */

typedef struct samr_entry_info4
{
	uint32 user_idx;
	STRHDR hdr_acct_name;

} SAM_ENTRY4;

typedef struct samr_str_entry_info4
{
	STRING2 acct_name;

} SAM_STR4;

typedef struct sam_entry_info_4
{
	SAM_ENTRY4 *sam;
	SAM_STR4   *str;

} SAM_DISPINFO_4;


/* -- Level 5 Display Info - Group List (ASCII) -- */

typedef struct samr_entry_info5
{
	uint32 grp_idx;
	STRHDR hdr_grp_name;

} SAM_ENTRY5;

typedef struct samr_str_entry_info5
{
	STRING2 grp_name;

} SAM_STR5;

typedef struct sam_entry_info_5
{
	SAM_ENTRY5 *sam;
	SAM_STR5   *str;

} SAM_DISPINFO_5;


typedef struct sam_dispinfo_ctr_info
{
	union
	{
		SAM_DISPINFO_1 *info1; /* users/names/descriptions */
		SAM_DISPINFO_2 *info2; /* trust accounts */
		SAM_DISPINFO_3 *info3; /* domain groups/descriptions */
		SAM_DISPINFO_4 *info4; /* user list (ASCII) - used by Win95 */
		SAM_DISPINFO_5 *info5; /* group list (ASCII) */
		void       *info; /* allows assignment without typecasting, */

	} sam;

} SAM_DISPINFO_CTR;


/* SAMR_Q_QUERY_DISPINFO - SAM rids, names and descriptions */
typedef struct q_samr_query_disp_info
{
	POLICY_HND domain_pol;

	uint16 switch_level;    /* see SAM_DISPINFO_CTR above */
	/* align */

	uint32 start_idx;       /* start enumeration index */
	uint32 max_entries;     /* maximum number of entries to return */
	uint32 max_size;        /* recommended data size; if exceeded server
				   should return STATUS_MORE_ENTRIES */

} SAMR_Q_QUERY_DISPINFO;


/* SAMR_R_QUERY_DISPINFO  */
typedef struct r_samr_query_dispinfo_info
{
	uint32 total_size;     /* total data size for all matching entries
				  (0 = uncalculated) */
	uint32 data_size;      /* actual data size returned = size of SAM_ENTRY
				  structures + total length of strings */

	uint16 switch_level;   /* see SAM_DISPINFO_CTR above */
	/* align */

	uint32 num_entries;    /* number of entries returned */
	uint32 ptr_entries;
	uint32 num_entries2;

	SAM_DISPINFO_CTR *ctr;

	NTSTATUS status;

} SAMR_R_QUERY_DISPINFO;


/* SAMR_Q_DELETE_DOM_GROUP - delete domain group */
typedef struct q_samr_delete_dom_group_info
{
    POLICY_HND group_pol;          /* policy handle */

} SAMR_Q_DELETE_DOM_GROUP;


/* SAMR_R_DELETE_DOM_GROUP - delete domain group */
typedef struct r_samr_delete_dom_group_info
{
	POLICY_HND pol;       /* policy handle */
	NTSTATUS status;        /* return status */

} SAMR_R_DELETE_DOM_GROUP;


/* SAMR_Q_CREATE_DOM_GROUP - SAM create group */
typedef struct q_samr_create_dom_group_info
{
	POLICY_HND pol;        /* policy handle */

	UNIHDR hdr_acct_desc;
	UNISTR2 uni_acct_desc;

	uint32 access_mask;    

} SAMR_Q_CREATE_DOM_GROUP;

/* SAMR_R_CREATE_DOM_GROUP - SAM create group */
typedef struct r_samr_create_dom_group_info
{
	POLICY_HND pol;        /* policy handle */

	uint32 rid;    
	NTSTATUS status;    

} SAMR_R_CREATE_DOM_GROUP;

/* SAMR_Q_QUERY_GROUPINFO - SAM Group Info */
typedef struct q_samr_query_group_info
{
	POLICY_HND pol;        /* policy handle */

	uint16 switch_level;    /* 0x0001 seen */

} SAMR_Q_QUERY_GROUPINFO;

typedef struct samr_group_info1
{
	UNIHDR hdr_acct_name;

	uint32 group_attr; /* 0x0000 0003 - group attribute */
	uint32 num_members; /* 0x0000 0001 - number of group members? */

	UNIHDR hdr_acct_desc;

	UNISTR2 uni_acct_name;
	UNISTR2 uni_acct_desc;

} GROUP_INFO1;

typedef struct samr_group_info2
{
	uint16 level;
	UNIHDR hdr_acct_name;
	UNISTR2 uni_acct_name;

} GROUP_INFO2;

typedef struct samr_group_info3
{
	uint32 group_attr; /* 0x0000 0003 - group attribute */

} GROUP_INFO3;

typedef struct samr_group_info4
{
	uint16 level;
	UNIHDR hdr_acct_desc;
	UNISTR2 uni_acct_desc;

} GROUP_INFO4;

typedef struct samr_group_info5
{
	UNIHDR hdr_acct_name;

	uint32 group_attr; /* 0x0000 0003 - group attribute */
	uint32 num_members; /* 0x0000 0001 - number of group members? */

	UNIHDR hdr_acct_desc;

	UNISTR2 uni_acct_name;
	UNISTR2 uni_acct_desc;

} GROUP_INFO5;


/* GROUP_INFO_CTR */
typedef struct group_info_ctr
{
	uint16 switch_value1;

	union
 	{
		GROUP_INFO1 info1;
		GROUP_INFO2 info2;
		GROUP_INFO3 info3;
		GROUP_INFO4 info4;
		GROUP_INFO5 info5;
	} group;

} GROUP_INFO_CTR;

/* SAMR_R_QUERY_GROUPINFO - SAM Group Info */
typedef struct r_samr_query_groupinfo_info
{
	uint32 ptr;        
	GROUP_INFO_CTR *ctr;

	NTSTATUS status;

} SAMR_R_QUERY_GROUPINFO;


/* SAMR_Q_SET_GROUPINFO - SAM Group Info */
typedef struct q_samr_set_group_info
{
	POLICY_HND pol;        /* policy handle */
	GROUP_INFO_CTR *ctr;

} SAMR_Q_SET_GROUPINFO;

/* SAMR_R_SET_GROUPINFO - SAM Group Info */
typedef struct r_samr_set_group_info
{
	NTSTATUS status;

} SAMR_R_SET_GROUPINFO;


/* SAMR_Q_DELETE_DOM_ALIAS - delete domain alias */
typedef struct q_samr_delete_dom_alias_info
{
    POLICY_HND alias_pol;          /* policy handle */

} SAMR_Q_DELETE_DOM_ALIAS;


/* SAMR_R_DELETE_DOM_ALIAS - delete domain alias */
typedef struct r_samr_delete_dom_alias_info
{
	POLICY_HND pol;       /* policy handle */
	NTSTATUS status;        /* return status */

} SAMR_R_DELETE_DOM_ALIAS;


/* SAMR_Q_CREATE_DOM_ALIAS - SAM create alias */
typedef struct q_samr_create_dom_alias_info
{
	POLICY_HND dom_pol;        /* policy handle */

	UNIHDR hdr_acct_desc;
	UNISTR2 uni_acct_desc;

	uint32 access_mask;    /* 0x001f000f */

} SAMR_Q_CREATE_DOM_ALIAS;

/* SAMR_R_CREATE_DOM_ALIAS - SAM create alias */
typedef struct r_samr_create_dom_alias_info
{
	POLICY_HND alias_pol;        /* policy handle */

	uint32 rid;    
	NTSTATUS status;    

} SAMR_R_CREATE_DOM_ALIAS;


/********************************************************/

typedef struct {
	UNISTR4 name;
	UNISTR4 description;
	uint32 num_member;
} ALIAS_INFO1;

typedef struct {
	UNISTR4 name;
} ALIAS_INFO2;

typedef struct {
	UNISTR4 description;
} ALIAS_INFO3;

typedef struct {
	POLICY_HND pol;        /* policy handle */
	uint16 level;    /* 0x0003 seen */
} SAMR_Q_QUERY_ALIASINFO;

typedef struct {
	uint16 level;
	union {
		ALIAS_INFO1 info1;
		ALIAS_INFO2 info2;
		ALIAS_INFO3 info3;
	} alias;
} ALIAS_INFO_CTR;

typedef struct {
	ALIAS_INFO_CTR *ctr;
	NTSTATUS status;
} SAMR_R_QUERY_ALIASINFO;


/********************************************************/

typedef struct {
	POLICY_HND alias_pol;        /* policy handle */
	ALIAS_INFO_CTR ctr;
} SAMR_Q_SET_ALIASINFO;

typedef struct {
	NTSTATUS status;
} SAMR_R_SET_ALIASINFO;


/********************************************************/

/* SAMR_Q_QUERY_USERGROUPS - */
typedef struct q_samr_query_usergroup_info
{
	POLICY_HND pol;          /* policy handle associated with unknown id */

} SAMR_Q_QUERY_USERGROUPS;

/* SAMR_R_QUERY_USERGROUPS - probably a get sam info */
typedef struct r_samr_query_usergroup_info
{
	uint32 ptr_0;            /* pointer */
	uint32 num_entries;      /* number of RID groups */
	uint32 ptr_1;            /* pointer */
	uint32 num_entries2;     /* number of RID groups */

	DOM_GID *gid; /* group info */

	NTSTATUS status;         /* return status */

} SAMR_R_QUERY_USERGROUPS;

/* SAM_USERINFO_CTR - sam user info */
typedef struct sam_userinfo_ctr_info
{
	uint16 switch_value;      

	union
	{
		SAM_USER_INFO_7  *id7;
		SAM_USER_INFO_9  *id9;
		SAM_USER_INFO_16 *id16;
		SAM_USER_INFO_17 *id17;
		SAM_USER_INFO_18 *id18;
		SAM_USER_INFO_20 *id20;
		SAM_USER_INFO_21 *id21;
		SAM_USER_INFO_23 *id23;
		SAM_USER_INFO_24 *id24;
		SAM_USER_INFO_25 *id25;
		SAM_USER_INFO_26 *id26;
		void* id; /* to make typecasting easy */

	} info;

} SAM_USERINFO_CTR;


/* SAMR_Q_SET_USERINFO2 - set sam info */
typedef struct q_samr_set_user_info2
{
	POLICY_HND pol;          /* policy handle associated with user */
	uint16 switch_value;      /* 0x0010 */

	SAM_USERINFO_CTR *ctr;

} SAMR_Q_SET_USERINFO2;

/* SAMR_R_SET_USERINFO2 - set sam info */
typedef struct r_samr_set_user_info2
{
	NTSTATUS status;         /* return status */

} SAMR_R_SET_USERINFO2;

/* SAMR_Q_SET_USERINFO - set sam info */
typedef struct q_samr_set_user_info
{
	POLICY_HND pol;          /* policy handle associated with user */
	uint16 switch_value;
	SAM_USERINFO_CTR *ctr;

} SAMR_Q_SET_USERINFO;

/* SAMR_R_SET_USERINFO - set sam info */
typedef struct r_samr_set_user_info
{
	NTSTATUS status;         /* return status */

} SAMR_R_SET_USERINFO;


/* SAMR_Q_QUERY_USERINFO - probably a get sam info */
typedef struct q_samr_query_user_info
{
	POLICY_HND pol;          /* policy handle associated with unknown id */
	uint16 switch_value;         /* 0x0015, 0x0011 or 0x0010 - 16 bit unknown */

} SAMR_Q_QUERY_USERINFO;

/* SAMR_R_QUERY_USERINFO - probably a get sam info */
typedef struct r_samr_query_user_info
{
	uint32 ptr;            /* pointer */
	SAM_USERINFO_CTR *ctr;

	NTSTATUS status;         /* return status */

} SAMR_R_QUERY_USERINFO;


/****************************************************************************
SAMR_Q_QUERY_USERALIASES - do a conversion from name to RID.

the policy handle allocated by an "samr open secret" call is associated
with a SID.  this policy handle is what is queried here, *not* the SID
itself.  the response to the lookup rids is relative to this SID.
*****************************************************************************/
/* SAMR_Q_QUERY_USERALIASES */
typedef struct q_samr_query_useraliases_info
{
	POLICY_HND pol;       /* policy handle */

	uint32 num_sids1;      /* number of rids being looked up */
	uint32 ptr;            /* buffer pointer */
	uint32 num_sids2;      /* number of rids being looked up */

	uint32   *ptr_sid; /* pointers to sids to be looked up */
	DOM_SID2 *sid    ; /* sids to be looked up. */

} SAMR_Q_QUERY_USERALIASES;


/* SAMR_R_QUERY_USERALIASES */
typedef struct r_samr_query_useraliases_info
{
	uint32 num_entries;
	uint32 ptr; /* undocumented buffer pointer */

	uint32 num_entries2; 
	uint32 *rid; /* domain RIDs being looked up */

	NTSTATUS status; /* return code */

} SAMR_R_QUERY_USERALIASES;


/****************************************************************************
SAMR_Q_LOOKUP_NAMES - do a conversion from Names to RIDs+types.
*****************************************************************************/
/* SAMR_Q_LOOKUP_NAMES */
typedef struct q_samr_lookup_names_info
{
	POLICY_HND pol;       /* policy handle */

	uint32 num_names1;      /* number of names being looked up */
	uint32 flags;           /* 0x0000 03e8 - unknown */
	uint32 ptr;            /* 0x0000 0000 - 32 bit unknown */
	uint32 num_names2;      /* number of names being looked up */

	UNIHDR  *hdr_name; /* unicode account name header */
	UNISTR2 *uni_name; /* unicode account name string */

} SAMR_Q_LOOKUP_NAMES;


/* SAMR_R_LOOKUP_NAMES */
typedef struct r_samr_lookup_names_info
{
	uint32 num_rids1;      /* number of aliases being looked up */
	uint32 ptr_rids;       /* pointer to aliases */
	uint32 num_rids2;      /* number of aliases being looked up */

	uint32 *rids; /* rids */

	uint32 num_types1;      /* number of users in aliases being looked up */
	uint32 ptr_types;       /* pointer to users in aliases */
	uint32 num_types2;      /* number of users in aliases being looked up */

	uint32 *types; /* SID_ENUM type */

	NTSTATUS status; /* return code */

} SAMR_R_LOOKUP_NAMES;


/****************************************************************************
SAMR_Q_LOOKUP_RIDS - do a conversion from RID groups to something.

called to resolve domain RID groups.
*****************************************************************************/
/* SAMR_Q_LOOKUP_RIDS */
typedef struct q_samr_lookup_rids_info
{
	POLICY_HND pol;       /* policy handle */

	uint32 num_rids1;      /* number of rids being looked up */
	uint32 flags;          /* 0x0000 03e8 - unknown */
	uint32 ptr;            /* 0x0000 0000 - 32 bit unknown */
	uint32 num_rids2;      /* number of rids being looked up */

	uint32 *rid; /* domain RIDs being looked up */

} SAMR_Q_LOOKUP_RIDS;


/****************************************************************************
SAMR_R_LOOKUP_RIDS - do a conversion from group RID to names

*****************************************************************************/
/* SAMR_R_LOOKUP_RIDS */
typedef struct r_samr_lookup_rids_info
{
	uint32 num_names1;      /* number of aliases being looked up */
	uint32 ptr_names;       /* pointer to aliases */
	uint32 num_names2;      /* number of aliases being looked up */

	UNIHDR  *hdr_name; /* unicode account name header */
	UNISTR2 *uni_name; /* unicode account name string */

	uint32 num_types1;      /* number of users in aliases being looked up */
	uint32 ptr_types;       /* pointer to users in aliases */
	uint32 num_types2;      /* number of users in aliases being looked up */

	uint32 *type; /* SID_ENUM type */

	NTSTATUS status;

} SAMR_R_LOOKUP_RIDS;


/* SAMR_Q_OPEN_USER - probably an open */
typedef struct q_samr_open_user_info
{
	POLICY_HND domain_pol;       /* policy handle */
	uint32 access_mask;     /* 32 bit unknown - 0x02011b */
	uint32 user_rid;      /* user RID */

} SAMR_Q_OPEN_USER;


/* SAMR_R_OPEN_USER - probably an open */
typedef struct r_samr_open_user_info
{
	POLICY_HND user_pol;       /* policy handle associated with unknown id */
	NTSTATUS status;         /* return status */

} SAMR_R_OPEN_USER;


/* SAMR_Q_CREATE_USER - probably a create */
typedef struct q_samr_create_user_info
{
	POLICY_HND domain_pol;       /* policy handle */

	UNIHDR  hdr_name;       /* unicode account name header */
	UNISTR2 uni_name;       /* unicode account name */

	uint32 acb_info;      /* account control info */
	uint32 access_mask;     /* 0xe005 00b0 */

} SAMR_Q_CREATE_USER;


/* SAMR_R_CREATE_USER - probably a create */
typedef struct r_samr_create_user_info
{
	POLICY_HND user_pol;       /* policy handle associated with user */

	uint32 access_granted;
	uint32 user_rid;      /* user RID */
	NTSTATUS status;         /* return status */

} SAMR_R_CREATE_USER;


/* SAMR_Q_DELETE_DOM_USER - delete domain user */
typedef struct q_samr_delete_dom_user_info
{
    POLICY_HND user_pol;          /* policy handle */

} SAMR_Q_DELETE_DOM_USER;


/* SAMR_R_DELETE_DOM_USER - delete domain user */
typedef struct r_samr_delete_dom_user_info
{
	POLICY_HND pol;       /* policy handle */
	NTSTATUS status;        /* return status */

} SAMR_R_DELETE_DOM_USER;


/* SAMR_Q_QUERY_GROUPMEM - query group members */
typedef struct q_samr_query_groupmem_info
{
	POLICY_HND group_pol;        /* policy handle */

} SAMR_Q_QUERY_GROUPMEM;


/* SAMR_R_QUERY_GROUPMEM - query group members */
typedef struct r_samr_query_groupmem_info
{
	uint32 ptr;
	uint32 num_entries;

	uint32 ptr_rids;
	uint32 ptr_attrs;

	uint32 num_rids;
	uint32 *rid;

	uint32 num_attrs;
	uint32 *attr;

	NTSTATUS status;

} SAMR_R_QUERY_GROUPMEM;


/* SAMR_Q_DEL_GROUPMEM - probably an del group member */
typedef struct q_samr_del_group_mem_info
{
	POLICY_HND pol;       /* policy handle */
	uint32 rid;         /* rid */

} SAMR_Q_DEL_GROUPMEM;


/* SAMR_R_DEL_GROUPMEM - probably an del group member */
typedef struct r_samr_del_group_mem_info
{
	NTSTATUS status;         /* return status */

} SAMR_R_DEL_GROUPMEM;


/* SAMR_Q_ADD_GROUPMEM - probably an add group member */
typedef struct q_samr_add_group_mem_info
{
	POLICY_HND pol;       /* policy handle */

	uint32 rid;         /* rid */
	uint32 unknown;     /* 0x0000 0005 */

} SAMR_Q_ADD_GROUPMEM;


/* SAMR_R_ADD_GROUPMEM - probably an add group member */
typedef struct r_samr_add_group_mem_info
{
	NTSTATUS status;         /* return status */

} SAMR_R_ADD_GROUPMEM;


/* SAMR_Q_OPEN_GROUP - probably an open */
typedef struct q_samr_open_group_info
{
	POLICY_HND domain_pol;       /* policy handle */
	uint32 access_mask;         /* 0x0000 0001, 0x0000 0003, 0x0000 001f */
	uint32 rid_group;        /* rid */

} SAMR_Q_OPEN_GROUP;


/* SAMR_R_OPEN_GROUP - probably an open */
typedef struct r_samr_open_group_info
{
	POLICY_HND pol;       /* policy handle */
	NTSTATUS status;         /* return status */

} SAMR_R_OPEN_GROUP;


/* SAMR_Q_QUERY_ALIASMEM - query alias members */
typedef struct q_samr_query_aliasmem_info
{
	POLICY_HND alias_pol;        /* policy handle */

} SAMR_Q_QUERY_ALIASMEM;


/* SAMR_R_QUERY_ALIASMEM - query alias members */
typedef struct r_samr_query_aliasmem_info
{
	uint32 num_sids;
	uint32 ptr;
	uint32 num_sids1;

	DOM_SID2 *sid;

	NTSTATUS status;

} SAMR_R_QUERY_ALIASMEM;


/* SAMR_Q_ADD_ALIASMEM - add alias member */
typedef struct q_samr_add_alias_mem_info
{
	POLICY_HND alias_pol;       /* policy handle */

	DOM_SID2 sid; /* member sid to be added to the alias */

} SAMR_Q_ADD_ALIASMEM;


/* SAMR_R_ADD_ALIASMEM - add alias member */
typedef struct r_samr_add_alias_mem_info
{
	NTSTATUS status;         /* return status */

} SAMR_R_ADD_ALIASMEM;


/* SAMR_Q_DEL_ALIASMEM - add an add alias member */
typedef struct q_samr_del_alias_mem_info
{
	POLICY_HND alias_pol;       /* policy handle */

	DOM_SID2 sid; /* member sid to be added to alias */

} SAMR_Q_DEL_ALIASMEM;


/* SAMR_R_DEL_ALIASMEM - delete alias member */
typedef struct r_samr_del_alias_mem_info
{
	NTSTATUS status;         /* return status */

} SAMR_R_DEL_ALIASMEM;



/* SAMR_Q_OPEN_ALIAS - probably an open */
typedef struct q_samr_open_alias_info
{
	POLICY_HND dom_pol;

	uint32 access_mask;         
	uint32 rid_alias;

} SAMR_Q_OPEN_ALIAS;


/* SAMR_R_OPEN_ALIAS - probably an open */
typedef struct r_samr_open_alias_info
{
	POLICY_HND pol;       /* policy handle */
	NTSTATUS status;         /* return status */

} SAMR_R_OPEN_ALIAS;


/* SAMR_Q_CONNECT_ANON - probably an open */
typedef struct q_samr_connect_anon_info {
	uint32 ptr;                  /* ptr? */
	uint16 unknown_0;	     /* Only pushed if ptr is non-zero. */
	uint32 access_mask;
} SAMR_Q_CONNECT_ANON;

/* SAMR_R_CONNECT_ANON - probably an open */
typedef struct r_samr_connect_anon_info
{
	POLICY_HND connect_pol;       /* policy handle */
	NTSTATUS status;         /* return status */

} SAMR_R_CONNECT_ANON;

/* SAMR_Q_CONNECT - probably an open */
typedef struct q_samr_connect_info
{
	uint32 ptr_srv_name;         /* pointer (to server name?) */
	UNISTR2 uni_srv_name;        /* unicode server name starting with '\\' */

	uint32 access_mask;            

} SAMR_Q_CONNECT;


/* SAMR_R_CONNECT - probably an open */
typedef struct r_samr_connect_info
{
	POLICY_HND connect_pol;       /* policy handle */
	NTSTATUS status;         /* return status */

} SAMR_R_CONNECT;

/* SAMR_Q_CONNECT4 */
typedef struct q_samr_connect4_info
{
	uint32 ptr_srv_name; /* pointer to server name */
	UNISTR2 uni_srv_name;

	uint32 unk_0; /* possible server name type, 1 for IP num, 2 for name */
	uint32 access_mask;
} SAMR_Q_CONNECT4;

/* SAMR_R_CONNECT4 - same format as connect */
typedef struct r_samr_connect_info SAMR_R_CONNECT4;       

/* SAMR_Q_CONNECT5 */
typedef struct q_samr_connect5_info
{
	uint32 ptr_srv_name; /* pointer to server name */
	UNISTR2 uni_srv_name;
	uint32 access_mask;
	uint32 level;
	/* These following are acutally a level dependent
	   value. Fudge it for now. JRA */
	uint32 info1_unk1;
	uint32 info1_unk2;
} SAMR_Q_CONNECT5;

/* SAMR_R_CONNECT5 */
typedef struct r_samr_connect_info5
{
	uint32 level;
	uint32 info1_unk1;
	uint32 info1_unk2;
	POLICY_HND connect_pol;       /* policy handle */
	NTSTATUS status;         /* return status */

} SAMR_R_CONNECT5;


/* SAMR_Q_GET_DOM_PWINFO */
typedef struct q_samr_get_dom_pwinfo
{
	uint32 ptr; 
	UNIHDR  hdr_srv_name;
	UNISTR2 uni_srv_name;

} SAMR_Q_GET_DOM_PWINFO;

#define DOMAIN_PASSWORD_COMPLEX		0x00000001
#define DOMAIN_PASSWORD_NO_ANON_CHANGE	0x00000002
#define DOMAIN_PASSWORD_NO_CLEAR_CHANGE	0x00000004
#define DOMAIN_LOCKOUT_ADMINS		0x00000008
#define DOMAIN_PASSWORD_STORE_CLEARTEXT	0x00000010
#define DOMAIN_REFUSE_PASSWORD_CHANGE	0x00000020

/* SAMR_R_GET_DOM_PWINFO */
typedef struct r_samr_get_dom_pwinfo
{
	uint16 min_pwd_length;
	uint32 password_properties;
	NTSTATUS status;

} SAMR_R_GET_DOM_PWINFO;

/* SAMR_ENC_PASSWD */
typedef struct enc_passwd_info
{
	uint32 ptr;
	uint8 pass[516];

} SAMR_ENC_PASSWD;

/* SAMR_ENC_HASH */
typedef struct enc_hash_info
{
	uint32 ptr;
	uint8 hash[16];

} SAMR_ENC_HASH;

/* SAMR_Q_CHGPASSWD_USER */
typedef struct q_samr_chgpasswd_user_info
{
	uint32 ptr_0;

	UNIHDR hdr_dest_host; /* server name unicode header */
	UNISTR2 uni_dest_host; /* server name unicode string */

	UNIHDR hdr_user_name;    /* username unicode string header */
	UNISTR2 uni_user_name;    /* username unicode string */

	SAMR_ENC_PASSWD nt_newpass;
	SAMR_ENC_HASH nt_oldhash;

	uint32 unknown; /* 0x0000 0001 */

	SAMR_ENC_PASSWD lm_newpass;
	SAMR_ENC_HASH lm_oldhash;

} SAMR_Q_CHGPASSWD_USER;

/* SAMR_R_CHGPASSWD_USER */
typedef struct r_samr_chgpasswd_user_info
{
	NTSTATUS status; /* 0 == OK, C000006A (NT_STATUS_WRONG_PASSWORD) */

} SAMR_R_CHGPASSWD_USER;

/* SAMR_Q_CHGPASSWD3 */
typedef struct q_samr_chgpasswd_user3
{
	uint32 ptr_0;

	UNIHDR hdr_dest_host; /* server name unicode header */
	UNISTR2 uni_dest_host; /* server name unicode string */

	UNIHDR hdr_user_name;    /* username unicode string header */
	UNISTR2 uni_user_name;    /* username unicode string */

	SAMR_ENC_PASSWD nt_newpass;
	SAMR_ENC_HASH nt_oldhash;

	uint32 lm_change; /* 0x0000 0001 */

	SAMR_ENC_PASSWD lm_newpass;
	SAMR_ENC_HASH lm_oldhash;

	SAMR_ENC_PASSWD password3;

} SAMR_Q_CHGPASSWD_USER3;

#define REJECT_REASON_OTHER		0x00000000
#define REJECT_REASON_TOO_SHORT		0x00000001
#define REJECT_REASON_IN_HISTORY	0x00000002
#define REJECT_REASON_NOT_COMPLEX	0x00000005

/* SAMR_CHANGE_REJECT */
typedef struct samr_change_reject
{
	uint32 reject_reason;
	uint32 unknown1;
	uint32 unknown2;

} SAMR_CHANGE_REJECT;

/* SAMR_R_CHGPASSWD3 */
typedef struct r_samr_chgpasswd_user3
{
	uint32 ptr_info;
	uint32 ptr_reject;
	SAM_UNK_INFO_1 *info;
	SAMR_CHANGE_REJECT *reject;
	NTSTATUS status; /* 0 == OK, C000006A (NT_STATUS_WRONG_PASSWORD) */

} SAMR_R_CHGPASSWD_USER3;



/* SAMR_Q_REMOVE_SID_FOREIGN_DOMAIN */
typedef struct q_samr_remove_sid_foreign_domain_info
{
	POLICY_HND dom_pol;   /* policy handle */
	DOM_SID2 sid;         /* SID */

} SAMR_Q_REMOVE_SID_FOREIGN_DOMAIN;


/* SAMR_R_REMOVE_SID_FOREIGN_DOMAIN */
typedef struct r_samr_remove_sid_foreign_domain_info
{
	NTSTATUS status;         /* return status */

} SAMR_R_REMOVE_SID_FOREIGN_DOMAIN;



/* these are from the old rpc_samr.h - they are needed while the merge
   is still going on */
#define MAX_SAM_SIDS 15

/* DOM_SID3 - security id */
typedef struct sid_info_3
{
        uint16 len; /* length, bytes, including length of len :-) */
        /* uint8  pad[2]; */
        
        DOM_SID sid;

} DOM_SID3;

/* SAMR_Q_QUERY_DOMAIN_INFO2 */
typedef struct q_samr_query_domain_info2
{
	POLICY_HND domain_pol;   /* policy handle */
	uint16 switch_value;

} SAMR_Q_QUERY_DOMAIN_INFO2;

/* SAMR_R_QUERY_DOMAIN_INFO2 */
typedef struct r_samr_query_domain_info2
{
	uint32 ptr_0;
	uint16 switch_value;
	SAM_UNK_CTR *ctr;
	NTSTATUS status;         /* return status */

} SAMR_R_QUERY_DOMAIN_INFO2;

/* SAMR_Q_SET_DOMAIN_INFO */
typedef struct q_samr_set_domain_info
{
	POLICY_HND domain_pol;   /* policy handle */
	uint16 switch_value0;
	uint16 switch_value;
	SAM_UNK_CTR *ctr;

} SAMR_Q_SET_DOMAIN_INFO;

/* SAMR_R_SET_DOMAIN_INFO */
typedef struct r_samr_set_domain_info
{
	NTSTATUS status;         /* return status */

} SAMR_R_SET_DOMAIN_INFO;

#endif /* _RPC_SAMR_H */
