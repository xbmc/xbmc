/* 
   Unix SMB/CIFS implementation.

   Copyright (C) Andrew Tridgell                1992-1997
   Copyright (C) Luke Kenneth Casson Leighton   1996-1997
   Copyright (C) Paul Ashton                    1997
   Copyright (C) Gerald (Jerry) Carter          2005
   
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

#ifndef _RPC_MISC_H /* _RPC_MISC_H */
#define _RPC_MISC_H 

#define SMB_RPC_INTERFACE_VERSION 1
#define PRS_POINTER_CAST BOOL (*)(const char*, prs_struct*, int, void*)

enum unistr2_term_codes { UNI_FLAGS_NONE = 0, UNI_STR_TERMINATE = 1, UNI_MAXLEN_TERMINATE = 2, UNI_BROKEN_NON_NULL = 3, UNI_STR_DBLTERMINATE = 4 };



/********************************************************************** 
 * well-known RIDs - Relative IDs
 **********************************************************************/

/* RIDs - Well-known users ... */
#define DOMAIN_USER_RID_ADMIN          (0x000001F4L)
#define DOMAIN_USER_RID_GUEST          (0x000001F5L)
#define DOMAIN_USER_RID_KRBTGT         (0x000001F6L)

/* RIDs - well-known groups ... */
#define DOMAIN_GROUP_RID_ADMINS        (0x00000200L)
#define DOMAIN_GROUP_RID_USERS         (0x00000201L)
#define DOMAIN_GROUP_RID_GUESTS        (0x00000202L)
#define DOMAIN_GROUP_RID_COMPUTERS     (0x00000203L)

#define DOMAIN_GROUP_RID_CONTROLLERS   (0x00000204L)
#define DOMAIN_GROUP_RID_CERT_ADMINS   (0x00000205L)
#define DOMAIN_GROUP_RID_SCHEMA_ADMINS (0x00000206L)
#define DOMAIN_GROUP_RID_ENTERPRISE_ADMINS (0x00000207L)

/* is the following the right number? I bet it is  --simo
#define DOMAIN_GROUP_RID_POLICY_ADMINS (0x00000208L)
*/

/* RIDs - well-known aliases ... */
#define BUILTIN_ALIAS_RID_ADMINS        (0x00000220L)
#define BUILTIN_ALIAS_RID_USERS         (0x00000221L)
#define BUILTIN_ALIAS_RID_GUESTS        (0x00000222L)
#define BUILTIN_ALIAS_RID_POWER_USERS   (0x00000223L)

#define BUILTIN_ALIAS_RID_ACCOUNT_OPS   (0x00000224L)
#define BUILTIN_ALIAS_RID_SYSTEM_OPS    (0x00000225L)
#define BUILTIN_ALIAS_RID_PRINT_OPS     (0x00000226L)
#define BUILTIN_ALIAS_RID_BACKUP_OPS    (0x00000227L)

#define BUILTIN_ALIAS_RID_REPLICATOR    (0x00000228L)
#define BUILTIN_ALIAS_RID_RAS_SERVERS   (0x00000229L)
#define BUILTIN_ALIAS_RID_PRE_2K_ACCESS (0x0000022aL)


/********************************************************************** 
 * Masks for mappings between unix uid and gid types and
 * NT RIDS.
 **********************************************************************/

#define BASE_RID (0x000003E8L)

/* Take the bottom bit. */
#define RID_TYPE_MASK 		1
#define RID_MULTIPLIER 		2

/* The two common types. */
#define USER_RID_TYPE 		0
#define GROUP_RID_TYPE 		1



/********************************************************************** 
 * RPC policy handle used pretty much everywhere
 **********************************************************************/
 
typedef struct {
	uint32 ptr_hnd;          /* pointer to enumeration handle */
	uint32 handle;           /* enumeration handle */
} ENUM_HND;



/********************************************************************** 
 * RPC policy handle used pretty much everywhere
 **********************************************************************/

typedef struct {
	uint32 data1;
	uint32 data2;
	uint16 data3;
	uint16 data4;
	uint8 data5[8];
#ifdef __INSURE__

	/* To prevent the leakage of policy handles mallocate a bit of
	   memory when a policy handle is created and free it when the
	   handle is closed.  This should cause Insure to flag an error
	   when policy handles are overwritten or fall out of scope without
	   being freed. */

	char *marker;
#endif
} POLICY_HND;


/********************************************************************** 
 * Buffer Headers -- use by SEC_DESC_BUF in winreg and netlogon code
 **********************************************************************/
 
/* TODO: replace this with an encompassing buffer structure */
typedef struct {
	uint32 buf_max_len;
	uint32 buf_len;
} BUFHDR;

/* this is a BUFHDR + a pointer to a buffer */
typedef struct {
	uint32 info_level;
	uint32 length;		/* uint8 chars */
	uint32 buffer;
} BUFHDR2;

/* generic buffer ?  wrapped around void*? */
typedef struct {
	uint32 size;
	uint32 buffer;
} BUFHDR4;


/********************************************************************** 
 * Buffers 
 **********************************************************************/

/* buffer used by \winreg\ calls to fill in arbitrary REG_XXX values.
   It *may* look like a UNISTR2 but it is *not*.  This is not a goof
   by the winreg developers.  It is a generic buffer.  buffer length
   is stored in bytes (not # of uint16's) */

typedef struct {
	uint32 buf_max_len;
	uint32 offset;
	uint32 buf_len;
	uint16 *buffer;
} REGVAL_BUFFER;

/* generic rpc version of the DATA_BLOB.  Just a length and uint8 array */

typedef struct {
	uint32 buf_len;
	uint8 *buffer;
} RPC_DATA_BLOB;

/********************************************************************** 
 * Buffers use by spoolss (i might be able to replace it with
 * an RPC_DATA_BLOB)
 **********************************************************************/

typedef struct {
	uint32 buf_len;
	uint16 *buffer; /* data */
} BUFFER5;


/********************************************************************** 
 * Unicode and basic string headers 
 **********************************************************************/
 
typedef struct {
	uint16 str_str_len;
	uint16 str_max_len;
	uint32 buffer; /* non-zero */
} STRHDR;

typedef struct {
	uint16 uni_str_len;
	uint16 uni_max_len;
	uint32 buffer; 
} UNIHDR;

/********************************************************************** 
 * UNICODE string variations
 **********************************************************************/


typedef struct {		/* UNISTR - unicode string size and buffer */
	uint16 *buffer;		/* unicode characters. ***MUST*** be 
				   little-endian. ***MUST*** be null-terminated */
} UNISTR;

typedef struct {		/* UNISTR2 - unicode string size (in 
				   uint16 unicode chars) and buffer */
	uint32 uni_max_len;
	uint32 offset;
	uint32 uni_str_len;
	uint16 *buffer;		/* unicode characters. ***MUST*** be little-endian. 
				  **must** be null-terminated and the uni_str_len 
				  should include the NULL character */
} UNISTR2;

/* i think this is the same as a BUFFER5 used in the spoolss code --jerry */
/* not sure about how the termination matches between the uint16 buffers thought */

typedef struct {		/* UNISTR3 - XXXX not sure about this structure */
	uint32 uni_str_len;
	UNISTR str;
} UNISTR3;

typedef struct {		/* Buffer wrapped around a UNISTR2 */
	uint16 length;		/* number of bytes not counting NULL terminatation */
	uint16 size;		/* number of bytes including NULL terminatation */
	UNISTR2 *string;
} UNISTR4;

typedef struct {
	uint32 count;
	UNISTR4 *strings;
} UNISTR4_ARRAY;


/********************************************************************** 
 * String variations
 **********************************************************************/

typedef struct {		/* STRING2 - string size (in uint8 chars) and buffer */
	uint32 str_max_len;
	uint32 offset;
	uint32 str_str_len;
	uint8  *buffer; 	/* uint8 characters. **NOT** necessarily null-terminated */
} STRING2;




/********************************************************************** 
 * Domain SID structures
 **********************************************************************/

typedef struct {
	uint32 num_auths; /* length, bytes, including length of len :-) */
	DOM_SID sid;
} DOM_SID2;


/********************************************************************** 
 * Domain SID structures
 **********************************************************************/

/* DOM_RID - domain RID structure for ntlsa pipe */
typedef struct {
	uint16 type; /* value is SID_NAME_USE enum */
	uint32 rid;
	uint32 rid_idx; /* referenced domain index */
} DOM_RID;

/* DOM_RID2 - second domain RID structure for ntlsa pipe */
typedef struct {
	uint16 type; /* value is SID_NAME_USE enum */
	uint32 rid;
	uint32 rid_idx; /* referenced domain index */
	uint32 unknown;
} DOM_RID2;

typedef struct {		/* DOM_RID3 - domain RID structure for samr pipe */
	uint32 rid;        /* domain-relative (to a SID) id */
	uint32 type1;      /* value is 0x1 */
	uint32 ptr_type;   /* undocumented pointer */
	uint32 type2;      /* value is 0x1 */
	uint32 unk; /* value is 0x2 */
} DOM_RID3;

/* DOM_RID4 - rid + user attributes */
typedef struct domrid4_info
{
	uint32 unknown;
	uint16 attr;
	uint32 rid;  /* user RID */
} DOM_RID4;

/* DOM_GID - group id + user attributes */
typedef struct {
	uint32 g_rid;  /* a group RID */
	uint32 attr;
} DOM_GID;

/********************************************************************** 
 * ????
 **********************************************************************/

/* DOM_CLNT_SRV - client / server names */
typedef struct clnt_srv_info {
	uint32  undoc_buffer; /* undocumented 32 bit buffer pointer */
	UNISTR2 uni_logon_srv; /* logon server name */
	uint32  undoc_buffer2; /* undocumented 32 bit buffer pointer */
	UNISTR2 uni_comp_name; /* client machine name */
} DOM_CLNT_SRV;

/* DOM_LOG_INFO - login info */
typedef struct log_info {
	uint32  undoc_buffer; /* undocumented 32 bit buffer pointer */
	UNISTR2 uni_logon_srv; /* logon server name */
	UNISTR2 uni_acct_name; /* account name */
	uint16  sec_chan;      /* secure channel type */
	UNISTR2 uni_comp_name; /* client machine name */
} DOM_LOG_INFO;

/* DOM_CHAL - challenge info */
typedef struct chal_info {
	unsigned char data[8]; /* credentials */
} DOM_CHAL;
 
/* DOM_CREDs - timestamped client or server credentials */
typedef struct cred_info {
	DOM_CHAL challenge; /* credentials */
	UTIME timestamp;    /* credential time-stamp */
} DOM_CRED;

/* DOM_CLNT_INFO - client info */
typedef struct clnt_info {
	DOM_LOG_INFO login;
	DOM_CRED     cred;
} DOM_CLNT_INFO;

/* DOM_CLNT_INFO2 - client info */
typedef struct clnt_info2 {
	DOM_CLNT_SRV login;
	uint32        ptr_cred;
	DOM_CRED      cred;
} DOM_CLNT_INFO2;

/* DOM_LOGON_ID - logon id */
typedef struct logon_info {
	uint32 low;
	uint32 high;
} DOM_LOGON_ID;

/* OWF INFO */
typedef struct owf_info {
	uint8 data[16];
} OWF_INFO;

typedef struct uint64_s
{
	uint32 low;
	uint32 high;
} UINT64_S;

#endif /* _RPC_MISC_H */
