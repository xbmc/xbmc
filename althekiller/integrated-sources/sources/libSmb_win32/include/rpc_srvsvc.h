/*
   Unix SMB/CIFS implementation.
   SMB parameters and setup
   Copyright (C) Andrew Tridgell 1992-1997
   Copyright (C) Luke Kenneth Casson Leighton 1996-1997
   Copyright (C) Paul Ashton 1997
   Copyright (C) Nigel Williams 2001
   
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

#ifndef _RPC_SRVSVC_H /* _RPC_SRVSVC_H */
#define _RPC_SRVSVC_H 

/* srvsvc pipe */
#define SRV_NET_CONN_ENUM          0x08
#define SRV_NET_FILE_ENUM          0x09
#define SRV_NET_FILE_CLOSE         0x0b
#define SRV_NET_SESS_ENUM          0x0c
#define SRV_NET_SESS_DEL           0x0d
#define SRV_NET_SHARE_ADD          0x0e
#define SRV_NET_SHARE_ENUM_ALL     0x0f
#define SRV_NET_SHARE_GET_INFO     0x10
#define SRV_NET_SHARE_SET_INFO     0x11
#define SRV_NET_SHARE_DEL          0x12
#define SRV_NET_SHARE_DEL_STICKY   0x13
#define SRV_NET_SRV_GET_INFO       0x15
#define SRV_NET_SRV_SET_INFO       0x16
#define SRV_NET_DISK_ENUM          0x17
#define SRV_NET_REMOTE_TOD         0x1c
#define SRV_NET_NAME_VALIDATE      0x21
#define SRV_NET_SHARE_ENUM         0x24
#define SRV_NET_FILE_QUERY_SECDESC 0x27
#define SRV_NET_FILE_SET_SECDESC   0x28

#define MAX_SERVER_DISK_ENTRIES 15

typedef struct disk_info {
	uint32  unknown;
	UNISTR3 disk_name;
} DISK_INFO;

typedef struct disk_enum_container {
	uint32 level;
	uint32 entries_read;
	uint32 unknown;
	uint32 disk_info_ptr;
	DISK_INFO *disk_info;
} DISK_ENUM_CONTAINER;

typedef struct net_srv_disk_enum {
	uint32 ptr_srv_name;         /* pointer (to server name?) */
	UNISTR2 uni_srv_name;        /* server name */

	DISK_ENUM_CONTAINER disk_enum_ctr;

	uint32 preferred_len;        /* preferred maximum length (0xffff ffff) */
	uint32 total_entries;        /* total number of entries */
	ENUM_HND enum_hnd;
	WERROR status;               /* return status */
} SRV_Q_NET_DISK_ENUM, SRV_R_NET_DISK_ENUM;

typedef struct net_name_validate {
	uint32 ptr_srv_name;
	UNISTR2 uni_srv_name;
	UNISTR2 uni_name; /*name to validate*/
	uint32 type;
	uint32 flags;
	WERROR status;
} SRV_Q_NET_NAME_VALIDATE, SRV_R_NET_NAME_VALIDATE;

/* SESS_INFO_0 (pointers to level 0 session info strings) */
typedef struct ptr_sess_info0
{
	uint32 ptr_name; /* pointer to name. */

} SESS_INFO_0;

/* SESS_INFO_0_STR (level 0 session info strings) */
typedef struct str_sess_info0
{
	UNISTR2 uni_name; /* unicode string of name */

} SESS_INFO_0_STR;

/* oops - this is going to take up a *massive* amount of stack. */
/* the UNISTR2s already have 1024 uint16 chars in them... */
#define MAX_SESS_ENTRIES 32

/* SRV_SESS_INFO_0 */
typedef struct srv_sess_info_0_info
{
	uint32 num_entries_read;                     /* EntriesRead */
	uint32 ptr_sess_info;                       /* Buffer */
	uint32 num_entries_read2;                    /* EntriesRead */

	SESS_INFO_0     info_0    [MAX_SESS_ENTRIES]; /* session entry pointers */
	SESS_INFO_0_STR info_0_str[MAX_SESS_ENTRIES]; /* session entry strings */

} SRV_SESS_INFO_0;

/* SESS_INFO_1 (pointers to level 1 session info strings) */
typedef struct ptr_sess_info1
{
	uint32 ptr_name; /* pointer to name. */
	uint32 ptr_user; /* pointer to user name. */

	uint32 num_opens;
	uint32 open_time;
	uint32 idle_time;
	uint32 user_flags;

} SESS_INFO_1;

/* SESS_INFO_1_STR (level 1 session info strings) */
typedef struct str_sess_info1
{
	UNISTR2 uni_name; /* unicode string of name */
	UNISTR2 uni_user; /* unicode string of user */

} SESS_INFO_1_STR;

/* SRV_SESS_INFO_1 */
typedef struct srv_sess_info_1_info
{
	uint32 num_entries_read;                     /* EntriesRead */
	uint32 ptr_sess_info;                       /* Buffer */
	uint32 num_entries_read2;                    /* EntriesRead */

	SESS_INFO_1     info_1    [MAX_SESS_ENTRIES]; /* session entry pointers */
	SESS_INFO_1_STR info_1_str[MAX_SESS_ENTRIES]; /* session entry strings */

} SRV_SESS_INFO_1;

/* SRV_SESS_INFO_CTR */
typedef struct srv_sess_info_ctr_info
{
	uint32 switch_value;         /* switch value */
	uint32 ptr_sess_ctr;       /* pointer to sess info union */
	union
    {
		SRV_SESS_INFO_0 info0; /* session info level 0 */
		SRV_SESS_INFO_1 info1; /* session info level 1 */

    } sess;

} SRV_SESS_INFO_CTR;


/* SRV_Q_NET_SESS_ENUM */
typedef struct q_net_sess_enum_info
{
	uint32 ptr_srv_name;         /* pointer (to server name?) */
	UNISTR2 uni_srv_name;        /* server name */

	uint32 ptr_qual_name;         /* pointer (to qualifier name) */
	UNISTR2 uni_qual_name;        /* qualifier name "\\qualifier" */

	uint32 ptr_user_name;         /* pointer (to user name */
	UNISTR2 uni_user_name;        /* user name */

	uint32 sess_level;          /* session level */

	SRV_SESS_INFO_CTR *ctr;

	uint32 preferred_len;        /* preferred maximum length (0xffff ffff) */
	ENUM_HND enum_hnd;

} SRV_Q_NET_SESS_ENUM;

/* SRV_R_NET_SESS_ENUM */
typedef struct r_net_sess_enum_info
{
	uint32 sess_level;          /* share level */

	SRV_SESS_INFO_CTR *ctr;

	uint32 total_entries;                    /* total number of entries */
	ENUM_HND enum_hnd;

	WERROR status;               /* return status */

} SRV_R_NET_SESS_ENUM;

/* SRV_Q_NET_SESS_DEL */
typedef struct q_net_sess_del
{
	uint32 ptr_srv_name;         /* pointer (to server name?) */
	UNISTR2 uni_srv_name;        /* server name */

	uint32 ptr_cli_name;         /* pointer (to qualifier name) */
	UNISTR2 uni_cli_name;        /* qualifier name "\\qualifier" */

	uint32 ptr_user_name;         /* pointer (to user name */
	UNISTR2 uni_user_name;        /* user name */

} SRV_Q_NET_SESS_DEL;

/* SRV_R_NET_SESS_DEL */
typedef struct r_net_sess_del
{
	WERROR status;               /* return status */

} SRV_R_NET_SESS_DEL;

/* CONN_INFO_0 (pointers to level 0 connection info strings) */
typedef struct ptr_conn_info0
{
	uint32 id; /* connection id. */

} CONN_INFO_0;

/* oops - this is going to take up a *massive* amount of stack. */
/* the UNISTR2s already have 1024 uint16 chars in them... */
#define MAX_CONN_ENTRIES 32

/* SRV_CONN_INFO_0 */
typedef struct srv_conn_info_0_info
{
	uint32 num_entries_read;                     /* EntriesRead */
	uint32 ptr_conn_info;                       /* Buffer */
	uint32 num_entries_read2;                    /* EntriesRead */

	CONN_INFO_0     info_0    [MAX_CONN_ENTRIES]; /* connection entry pointers */

} SRV_CONN_INFO_0;

/* CONN_INFO_1 (pointers to level 1 connection info strings) */
typedef struct ptr_conn_info1
{
	uint32 id;   /* connection id */
	uint32 type; /* 0x3 */
	uint32 num_opens;
	uint32 num_users;
	uint32 open_time;

	uint32 ptr_usr_name; /* pointer to user name. */
	uint32 ptr_net_name; /* pointer to network name (e.g IPC$). */

} CONN_INFO_1;

/* CONN_INFO_1_STR (level 1 connection info strings) */
typedef struct str_conn_info1
{
	UNISTR2 uni_usr_name; /* unicode string of user */
	UNISTR2 uni_net_name; /* unicode string of name */

} CONN_INFO_1_STR;

/* SRV_CONN_INFO_1 */
typedef struct srv_conn_info_1_info
{
	uint32 num_entries_read;                     /* EntriesRead */
	uint32 ptr_conn_info;                       /* Buffer */
	uint32 num_entries_read2;                    /* EntriesRead */

	CONN_INFO_1     info_1    [MAX_CONN_ENTRIES]; /* connection entry pointers */
	CONN_INFO_1_STR info_1_str[MAX_CONN_ENTRIES]; /* connection entry strings */

} SRV_CONN_INFO_1;

/* SRV_CONN_INFO_CTR */
typedef struct srv_conn_info_ctr_info
{
	uint32 switch_value;         /* switch value */
	uint32 ptr_conn_ctr;       /* pointer to conn info union */
	union
    {
		SRV_CONN_INFO_0 info0; /* connection info level 0 */
		SRV_CONN_INFO_1 info1; /* connection info level 1 */

    } conn;

} SRV_CONN_INFO_CTR;


/* SRV_Q_NET_CONN_ENUM */
typedef struct q_net_conn_enum_info
{
	uint32 ptr_srv_name;         /* pointer (to server name) */
	UNISTR2 uni_srv_name;        /* server name "\\server" */

	uint32 ptr_qual_name;         /* pointer (to qualifier name) */
	UNISTR2 uni_qual_name;        /* qualifier name "\\qualifier" */

	uint32 conn_level;          /* connection level */

	SRV_CONN_INFO_CTR *ctr;

	uint32 preferred_len;        /* preferred maximum length (0xffff ffff) */
	ENUM_HND enum_hnd;

} SRV_Q_NET_CONN_ENUM;

/* SRV_R_NET_CONN_ENUM */
typedef struct r_net_conn_enum_info
{
	uint32 conn_level;          /* share level */

	SRV_CONN_INFO_CTR *ctr;

	uint32 total_entries;                    /* total number of entries */
	ENUM_HND enum_hnd;

	WERROR status;               /* return status */

} SRV_R_NET_CONN_ENUM;

/* SH_INFO_0 */
typedef struct ptr_share_info0
{
	uint32 ptr_netname; /* pointer to net name. */
} SH_INFO_0;

/* SH_INFO_0_STR (level 0 share info strings) */
typedef struct str_share_info0
{
        SH_INFO_0 *ptrs;

	UNISTR2 uni_netname; /* unicode string of net name */

} SH_INFO_0_STR;

/* SRV_SHARE_INFO_0 */
typedef struct share_info_0_info
{
	SH_INFO_0 info_0;
	SH_INFO_0_STR info_0_str;

} SRV_SHARE_INFO_0;

/* SH_INFO_1 (pointers to level 1 share info strings) */
typedef struct ptr_share_info1
{
	uint32 ptr_netname; /* pointer to net name. */
	uint32 type; /* ipc, print, disk ... */
	uint32 ptr_remark; /* pointer to comment. */

} SH_INFO_1;

/* SH_INFO_1_STR (level 1 share info strings) */
typedef struct str_share_info1
{
        SH_INFO_1 *ptrs;

	UNISTR2 uni_netname; /* unicode string of net name */
	UNISTR2 uni_remark; /* unicode string of comment */

} SH_INFO_1_STR;

/* SRV_SHARE_INFO_1 */
typedef struct share_info_1_info
{
	SH_INFO_1 info_1;
	SH_INFO_1_STR info_1_str;

} SRV_SHARE_INFO_1;

/* SH_INFO_2 (pointers to level 2 share info strings) */
typedef struct ptr_share_info2
{
	uint32 ptr_netname; /* pointer to net name. */
	uint32 type; /* ipc, print, disk ... */
	uint32 ptr_remark; /* pointer to comment. */
	uint32 perms;      /* permissions */
	uint32 max_uses;   /* maximum uses */
	uint32 num_uses;   /* current uses */
	uint32 ptr_path;   /* pointer to path name */
	uint32 ptr_passwd; /* pointer to password */

} SH_INFO_2;

/* SH_INFO_2_STR (level 2 share info strings) */
typedef struct str_share_info2
{
	SH_INFO_2 *ptrs;

	UNISTR2 uni_netname; /* unicode string of net name (e.g NETLOGON) */
	UNISTR2 uni_remark;  /* unicode string of comment (e.g "Logon server share") */
	UNISTR2 uni_path;    /* unicode string of local path (e.g c:\winnt\system32\repl\import\scripts) */
	UNISTR2 uni_passwd;  /* unicode string of password - presumably for share level security (e.g NULL) */

} SH_INFO_2_STR;

/* SRV_SHARE_INFO_2 */
typedef struct share_info_2_info
{
	SH_INFO_2 info_2;
	SH_INFO_2_STR info_2_str;

} SRV_SHARE_INFO_2;

typedef struct ptr_share_info501
{
	uint32 ptr_netname; /* pointer to net name */
	uint32 type;     /* ipc, print, disk */
	uint32 ptr_remark;  /* pointer to comment */
	uint32 csc_policy;  /* client-side offline caching policy << 4 */
} SH_INFO_501;

typedef struct str_share_info501
{
	UNISTR2 uni_netname; /* unicode string of net name */
	UNISTR2 uni_remark;  /* unicode string of comment */
} SH_INFO_501_STR;

/* SRV_SHARE_INFO_501 */
typedef struct share_info_501_info
{
	SH_INFO_501 info_501;
	SH_INFO_501_STR info_501_str;
} SRV_SHARE_INFO_501;

/* SH_INFO_502 (pointers to level 502 share info strings) */
typedef struct ptr_share_info502
{
	uint32 ptr_netname; /* pointer to net name. */
	uint32 type; /* ipc, print, disk ... */
	uint32 ptr_remark; /* pointer to comment. */
	uint32 perms;      /* permissions */
	uint32 max_uses;   /* maximum uses */
	uint32 num_uses;   /* current uses */
	uint32 ptr_path;   /* pointer to path name */
	uint32 ptr_passwd; /* pointer to password */
        uint32 reserved;    /* this holds the space taken by the sd in the rpc packet */
        uint32 reserved_offset;   /* required for _post operation when marshalling */
	uint32 sd_size;    /* size of security descriptor */
	uint32 ptr_sd;     /* pointer to security descriptor */

} SH_INFO_502;

/* SH_INFO_502_STR (level 502 share info strings) */
typedef struct str_share_info502
{
	SH_INFO_502 *ptrs;

	UNISTR2 uni_netname; /* unicode string of net name (e.g NETLOGON) */
	UNISTR2 uni_remark;  /* unicode string of comment (e.g "Logon server share") */
	UNISTR2 uni_path;    /* unicode string of local path (e.g c:\winnt\system32\repl\import\scripts) */
	UNISTR2 uni_passwd;  /* unicode string of password - presumably for share level security (e.g NULL) */

        uint32 reserved;
	uint32 sd_size;
	SEC_DESC *sd;

} SH_INFO_502_STR;

/* SRV_SHARE_INFO_502 */
typedef struct share_info_502_info
{
	SH_INFO_502 info_502;
	SH_INFO_502_STR info_502_str;

} SRV_SHARE_INFO_502;

typedef struct ptr_share_info1004
{
	uint32 ptr_remark;

} SH_INFO_1004;

typedef struct str_share_info1004
{
	SH_INFO_1004 *ptrs;

	UNISTR2 uni_remark;

} SH_INFO_1004_STR;

typedef struct ptr_info_1004_info
{
	SH_INFO_1004     info_1004; 
	SH_INFO_1004_STR info_1004_str; 
} SRV_SHARE_INFO_1004;

#define SHARE_1005_IN_DFS               0x00000001
#define SHARE_1005_DFS_ROOT             0x00000002
/* use the CSC policy mask and shift to match up with the smb.conf parm */
#define SHARE_1005_CSC_POLICY_MASK      0x00000030
#define SHARE_1005_CSC_POLICY_SHIFT     4

typedef struct share_info_1005_info
{
  uint32 share_info_flags; 
} SRV_SHARE_INFO_1005;

typedef struct share_info_1006_info
{
	uint32 max_uses; 
} SRV_SHARE_INFO_1006;

typedef struct ptr_share_info1007
{
	uint32 flags;
	uint32 ptr_AlternateDirectoryName;

} SH_INFO_1007;

typedef struct str_share_info1007
{
	SH_INFO_1007 *ptrs;

	UNISTR2 uni_AlternateDirectoryName;

} SH_INFO_1007_STR;

typedef struct ptr_info_1007_info
{
	SH_INFO_1007     info_1007; 
	SH_INFO_1007_STR info_1007_str; 
} SRV_SHARE_INFO_1007;

/* SRV_SHARE_INFO_1501 */
typedef struct share_info_1501_info
{
	SEC_DESC_BUF *sdb;
} SRV_SHARE_INFO_1501;

/* SRV_SHARE_INFO_CTR */
typedef struct srv_share_info_ctr_info
{
	uint32 info_level;
	uint32 switch_value;
	uint32 ptr_share_info;

	uint32 num_entries;
	uint32 ptr_entries;
	uint32 num_entries2;

	union {
		SRV_SHARE_INFO_0    *info0;
		SRV_SHARE_INFO_1    *info1;    /* share info level 1 */
		SRV_SHARE_INFO_2    *info2;    /* share info level 2 */
		SRV_SHARE_INFO_501  *info501;  /* share info level 501 */
		SRV_SHARE_INFO_502  *info502;  /* share info level 502 */
		SRV_SHARE_INFO_1004 *info1004;
		SRV_SHARE_INFO_1005 *info1005;
		SRV_SHARE_INFO_1006 *info1006;
		SRV_SHARE_INFO_1007 *info1007;
		SRV_SHARE_INFO_1501 *info1501;
		void *info;

	} share;

} SRV_SHARE_INFO_CTR;

/* SRV_Q_NET_SHARE_ENUM */
typedef struct q_net_share_enum_info
{
	uint32 ptr_srv_name;         /* pointer (to server name?) */
	UNISTR2 uni_srv_name;        /* server name */

	SRV_SHARE_INFO_CTR ctr;     /* share info container */

	uint32 preferred_len;        /* preferred maximum length (0xffff ffff) */

	ENUM_HND enum_hnd;

} SRV_Q_NET_SHARE_ENUM;


/* SRV_R_NET_SHARE_ENUM */
typedef struct r_net_share_enum_info
{
	SRV_SHARE_INFO_CTR ctr;     /* share info container */

	uint32 total_entries;                    /* total number of entries */
	ENUM_HND enum_hnd;

	WERROR status;               /* return status */

} SRV_R_NET_SHARE_ENUM;


/* SRV_Q_NET_SHARE_GET_INFO */
typedef struct q_net_share_get_info_info
{
	uint32 ptr_srv_name;
	UNISTR2 uni_srv_name;

	UNISTR2 uni_share_name;
	uint32 info_level;

} SRV_Q_NET_SHARE_GET_INFO;

/* SRV_SHARE_INFO */
typedef struct srv_share_info {
	uint32 switch_value;
	uint32 ptr_share_ctr;

	union {
		SRV_SHARE_INFO_0    info0;
		SRV_SHARE_INFO_1 info1;
		SRV_SHARE_INFO_2 info2;
		SRV_SHARE_INFO_501 info501;
		SRV_SHARE_INFO_502 info502;
		SRV_SHARE_INFO_1004 info1004;
		SRV_SHARE_INFO_1005 info1005;
		SRV_SHARE_INFO_1006 info1006;
		SRV_SHARE_INFO_1007 info1007;
		SRV_SHARE_INFO_1501 info1501;
	} share;
} SRV_SHARE_INFO;

/* SRV_R_NET_SHARE_GET_INFO */
typedef struct r_net_share_get_info_info
{
	SRV_SHARE_INFO info;
	WERROR status;

} SRV_R_NET_SHARE_GET_INFO;

/* SRV_Q_NET_SHARE_SET_INFO */
typedef struct q_net_share_set_info_info
{
	uint32 ptr_srv_name;
	UNISTR2 uni_srv_name;

	UNISTR2 uni_share_name;
	uint32 info_level;

	SRV_SHARE_INFO info;

        uint32 ptr_parm_error;
        uint32 parm_error;

} SRV_Q_NET_SHARE_SET_INFO;

/* SRV_R_NET_SHARE_SET_INFO */
typedef struct r_net_share_set_info
{
        uint32 ptr_parm_error;
        uint32 parm_error;

	WERROR status;               /* return status */

} SRV_R_NET_SHARE_SET_INFO;

/* SRV_Q_NET_SHARE_ADD */
typedef struct q_net_share_add
{
	uint32 ptr_srv_name;
	UNISTR2 uni_srv_name;

	uint32 info_level;

	SRV_SHARE_INFO info;

	uint32 ptr_err_index; /* pointer to error index */
	uint32 err_index;     /* index in info to field in error */

} SRV_Q_NET_SHARE_ADD;

/* SRV_R_NET_SHARE_ADD */
typedef struct r_net_share_add
{

        uint32 ptr_parm_error;
        uint32 parm_error;

	WERROR status;               /* return status */

} SRV_R_NET_SHARE_ADD;

/* SRV_Q_NET_SHARE_DEL */
typedef struct q_net_share_del
{
	uint32 ptr_srv_name;
	UNISTR2 uni_srv_name;
	UNISTR2 uni_share_name;
	uint32 reserved;

} SRV_Q_NET_SHARE_DEL;

/* SRV_R_NET_SHARE_DEL */
typedef struct r_net_share_del
{
	WERROR status;               /* return status */

} SRV_R_NET_SHARE_DEL;

/* FILE_INFO_3 (level 3 file info strings) */
typedef struct file_info3_info
{
	uint32 id;            /* file index */
	uint32 perms;         /* file permissions. don't know what format */
	uint32 num_locks;     /* file locks */
	uint32 ptr_path_name; /* file name */
	uint32 ptr_user_name; /* file owner */

} FILE_INFO_3;

/* FILE_INFO_3_STR (level 3 file info strings) */
typedef struct str_file_info3_info
{
	UNISTR2 uni_path_name; /* unicode string of file name */
	UNISTR2 uni_user_name; /* unicode string of file owner. */

} FILE_INFO_3_STR;

/* SRV_FILE_INFO_3 */
typedef struct srv_file_info_3
{
	uint32 num_entries_read;                     /* EntriesRead */
	uint32 ptr_file_info;                        /* Buffer */

	uint32 num_entries_read2;                    /* EntriesRead */
	FILE_INFO_3     info_3;     /* file entry details */
	FILE_INFO_3_STR info_3_str; /* file entry strings */
} SRV_FILE_INFO_3;

/* SRV_FILE_INFO_CTR */
typedef struct srv_file_info_3_info
{
	uint32 switch_value;         /* switch value */
	uint32 ptr_file_info;        /* pointer to file info union */

	uint32 num_entries;
	uint32 ptr_entries;
	uint32 num_entries2;
	union
	{
		SRV_FILE_INFO_3 *info3;
	} file;

} SRV_FILE_INFO_CTR;


/* SRV_Q_NET_FILE_ENUM */
typedef struct q_net_file_enum_info
{
	uint32 ptr_srv_name;         /* pointer (to server name?) */
	UNISTR2 uni_srv_name;        /* server name */

	uint32 ptr_qual_name;         /* pointer (to qualifier name) */
	UNISTR2 uni_qual_name;        /* qualifier name "\\qualifier" */

	uint32 ptr_user_name;         /* pointer (to user name) */
	UNISTR2 uni_user_name;        /* user name */

	uint32 file_level;          /* file level */

	SRV_FILE_INFO_CTR ctr;

	uint32 preferred_len; /* preferred maximum length (0xffff ffff) */
	ENUM_HND enum_hnd;

} SRV_Q_NET_FILE_ENUM;


/* SRV_R_NET_FILE_ENUM */
typedef struct r_net_file_enum_info
{
	uint32 file_level;          /* file level */

	SRV_FILE_INFO_CTR ctr;

	uint32 total_entries;                    /* total number of files */
	ENUM_HND enum_hnd;

	WERROR status;        /* return status */

} SRV_R_NET_FILE_ENUM;

/* SRV_Q_NET_FILE_CLOSE */
typedef struct q_net_file_close
{
	uint32 ptr_srv_name;         /* pointer to server name */
	UNISTR2 uni_srv_name;        /* server name */
	
	uint32 file_id;
} SRV_Q_NET_FILE_CLOSE;

/* SRV_R_NET_FILE_CLOSE */
typedef struct r_net_file_close
{
	WERROR status;               /* return status */
} SRV_R_NET_FILE_CLOSE;

/* SRV_INFO_100 */
typedef struct srv_info_100_info
{
	uint32 platform_id;     /* 0x500 */
	uint32 ptr_name;        /* pointer to server name */

	UNISTR2 uni_name;       /* server name "server" */

} SRV_INFO_100;

/* SRV_INFO_101 */
typedef struct srv_info_101_info
{
	uint32 platform_id;     /* 0x500 */
	uint32 ptr_name;        /* pointer to server name */
	uint32 ver_major;       /* 0x4 */
	uint32 ver_minor;       /* 0x2 */
	uint32 srv_type;        /* browse etc type */
	uint32 ptr_comment;     /* pointer to server comment */

	UNISTR2 uni_name;       /* server name "server" */
	UNISTR2 uni_comment;    /* server comment "samba x.x.x blah" */

} SRV_INFO_101;

/* SRV_INFO_102  */
typedef struct srv_info_102_info
{
	uint32 platform_id;     /* 0x500 */
	uint32 ptr_name;        /* pointer to server name */
	uint32 ver_major;       /* 0x4 */
	uint32 ver_minor;       /* 0x2 */
	uint32 srv_type;        /* browse etc type */
	uint32 ptr_comment;     /* pointer to server comment */
	uint32 users;           /* 0xffff ffff*/
	uint32 disc;            /* 0xf */
	uint32 hidden;          /* 0x0 */
	uint32 announce;        /* 240 */
	uint32 ann_delta;       /* 3000 */
	uint32 licenses;        /* 0 */
	uint32 ptr_usr_path;    /* pointer to user path */

	UNISTR2 uni_name;       /* server name "server" */
	UNISTR2 uni_comment;    /* server comment "samba x.x.x blah" */
	UNISTR2 uni_usr_path;   /* "c:\" (eh?) */

} SRV_INFO_102;


/* SRV_INFO_CTR */
typedef struct srv_info_ctr_info
{
	uint32 switch_value;         /* switch value */
	uint32 ptr_srv_ctr;         /* pointer to server info */
	union
    {
		SRV_INFO_102 sv102; /* server info level 102 */
		SRV_INFO_101 sv101; /* server info level 101 */
		SRV_INFO_100 sv100; /* server info level 100 */

    } srv;

} SRV_INFO_CTR;

/* SRV_Q_NET_SRV_GET_INFO */
typedef struct q_net_srv_get_info
{
	uint32  ptr_srv_name;
	UNISTR2 uni_srv_name; /* "\\server" */
	uint32  switch_value;

} SRV_Q_NET_SRV_GET_INFO;

/* SRV_R_NET_SRV_GET_INFO */
typedef struct r_net_srv_get_info
{
	SRV_INFO_CTR *ctr;

	WERROR status;               /* return status */

} SRV_R_NET_SRV_GET_INFO;

/* SRV_Q_NET_SRV_SET_INFO */
typedef struct q_net_srv_set_info
{
	uint32  ptr_srv_name;
	UNISTR2 uni_srv_name; /* "\\server" */
	uint32  switch_value;

	SRV_INFO_CTR *ctr;

} SRV_Q_NET_SRV_SET_INFO;


/* SRV_R_NET_SRV_SET_INFO */
typedef struct r_net_srv_set_info
{
	uint32 switch_value;         /* switch value */

	WERROR status;               /* return status */

} SRV_R_NET_SRV_SET_INFO;

/* SRV_Q_NET_REMOTE_TOD */
typedef struct q_net_remote_tod
{
	uint32  ptr_srv_name;
	UNISTR2 uni_srv_name; /* "\\server" */

} SRV_Q_NET_REMOTE_TOD;

/* TIME_OF_DAY_INFO */
typedef struct time_of_day_info
{
	uint32	elapsedt;
	uint32	msecs;
	uint32	hours;
	uint32	mins;
	uint32	secs;
	uint32	hunds;
	uint32	zone;
	uint32	tintervals;
	uint32	day;
	uint32	month;
	uint32	year;
	uint32	weekday;
	
} TIME_OF_DAY_INFO;

/* SRV_R_NET_REMOTE_TOD */
typedef struct r_net_remote_tod
{
	uint32 ptr_srv_tod;         /* pointer to TOD */
	TIME_OF_DAY_INFO *tod;
	
	WERROR status;               /* return status */

} SRV_R_NET_REMOTE_TOD;

/* SRV_Q_NET_FILE_QUERY_SECDESC */
typedef struct q_net_file_query_secdesc
{
	uint32  ptr_srv_name;
	UNISTR2 uni_srv_name;
	uint32  ptr_qual_name;
	UNISTR2 uni_qual_name;
	UNISTR2 uni_file_name;
	uint32  unknown1;
	uint32  unknown2;
	uint32  unknown3;
} SRV_Q_NET_FILE_QUERY_SECDESC;

/* SRV_R_NET_FILE_QUERY_SECDESC */
typedef struct r_net_file_query_secdesc
{
	uint32 ptr_response;
	uint32 size_response;
	uint32 ptr_secdesc;
	uint32 size_secdesc;
	SEC_DESC *sec_desc;
	WERROR status;
} SRV_R_NET_FILE_QUERY_SECDESC;

/* SRV_Q_NET_FILE_SET_SECDESC */
typedef struct q_net_file_set_secdesc
{
	uint32  ptr_srv_name;
	UNISTR2 uni_srv_name;
	uint32  ptr_qual_name;
	UNISTR2 uni_qual_name;
	UNISTR2 uni_file_name;
	uint32  sec_info;
	uint32  size_set;
	uint32  ptr_secdesc;
	uint32  size_secdesc;
	SEC_DESC *sec_desc;
} SRV_Q_NET_FILE_SET_SECDESC;

/* SRV_R_NET_FILE_SET_SECDESC */
typedef struct r_net_file_set_secdesc
{
	WERROR status;
} SRV_R_NET_FILE_SET_SECDESC;

#endif /* _RPC_SRVSVC_H */
