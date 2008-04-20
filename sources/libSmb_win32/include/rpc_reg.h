/* 
   Unix SMB/CIFS implementation.
   SMB parameters and setup
   Copyright (C) Andrew Tridgell                 1992-1997.
   Copyright (C) Luke Kenneth Casson Leighton    1996-1997.
   Copyright (C) Paul Ashton                          1997.
   Copyright (C) Jeremy Cooper                        2004.
   Copyright (C) Gerald Carter                   2002-2005.
   
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

#ifndef _RPC_REG_H /* _RPC_REG_H */
#define _RPC_REG_H 

/* RPC opnum */

#define REG_OPEN_HKCR		0x00
#define REG_OPEN_HKLM		0x02
#define REG_OPEN_HKPD		0x03
#define REG_OPEN_HKU		0x04
#define REG_CLOSE		0x05
#define REG_CREATE_KEY_EX	0x06
#define REG_DELETE_KEY		0x07
#define REG_DELETE_VALUE	0x08
#define REG_ENUM_KEY		0x09
#define REG_ENUM_VALUE		0x0a
#define REG_FLUSH_KEY		0x0b
#define REG_GET_KEY_SEC		0x0c
#define REG_OPEN_ENTRY		0x0f
#define REG_QUERY_KEY		0x10
#define REG_QUERY_VALUE		0x11
#define REG_RESTORE_KEY		0x13
#define REG_SAVE_KEY		0x14
#define REG_SET_KEY_SEC		0x15
#define REG_SET_VALUE		0x16
#define REG_SHUTDOWN		0x18
#define REG_ABORT_SHUTDOWN	0x19
#define REG_OPEN_HKPT		0x20
#define REG_GETVERSION		0x1a
#define REG_SHUTDOWN_EX		0x1e


#define HKEY_CLASSES_ROOT	0x80000000
#define HKEY_CURRENT_USER	0x80000001
#define HKEY_LOCAL_MACHINE 	0x80000002
#define HKEY_USERS         	0x80000003
#define HKEY_PERFORMANCE_DATA	0x80000004

#define KEY_HKLM		"HKLM"
#define KEY_HKU			"HKU"
#define KEY_HKCR		"HKCR"
#define KEY_HKPD		"HKPD"
#define KEY_HKPT		"HKPT"
#define KEY_SERVICES		"HKLM\\SYSTEM\\CurrentControlSet\\Services"
#define KEY_PRINTING 		"HKLM\\SYSTEM\\CurrentControlSet\\Control\\Print"
#define KEY_PRINTING_2K		"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Print\\Printers"
#define KEY_PRINTING_PORTS	"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Ports"
#define KEY_EVENTLOG 		"HKLM\\SYSTEM\\CurrentControlSet\\Services\\Eventlog"
#define KEY_SHARES		"HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Shares"
#define KEY_TREE_ROOT		""

/* Registry data types */

#define REG_NONE                       0
#define REG_SZ		               1
#define REG_EXPAND_SZ                  2
#define REG_BINARY 	               3
#define REG_DWORD	               4
#define REG_DWORD_LE	               4	/* DWORD, little endian */
#define REG_DWORD_BE	               5	/* DWORD, big endian */
#define REG_LINK                       6
#define REG_MULTI_SZ  	               7
#define REG_RESOURCE_LIST              8
#define REG_FULL_RESOURCE_DESCRIPTOR   9
#define REG_RESOURCE_REQUIREMENTS_LIST 10

/*
 * Registry key types
 *	Most keys are going to be GENERIC -- may need a better name?
 *	HKPD and HKPT are used by reg_perfcount.c
 *		they are special keys that contain performance data
 */
#define REG_KEY_GENERIC		0
#define REG_KEY_HKPD		1
#define REG_KEY_HKPT		2

/* 
 * container for function pointers to enumeration routines
 * for virtual registry view 
 */ 
 
typedef struct {
	/* functions for enumerating subkeys and values */	
	int 	(*fetch_subkeys)( const char *key, REGSUBKEY_CTR *subkeys);
	int 	(*fetch_values) ( const char *key, REGVAL_CTR *val );
	BOOL 	(*store_subkeys)( const char *key, REGSUBKEY_CTR *subkeys );
	BOOL 	(*store_values)( const char *key, REGVAL_CTR *val );
	BOOL	(*reg_access_check)( const char *keyname, uint32 requested, uint32 *granted, NT_USER_TOKEN *token );
} REGISTRY_OPS;

typedef struct {
	const char	*keyname;	/* full path to name of key */
	REGISTRY_OPS	*ops;		/* registry function hooks */
} REGISTRY_HOOK;


/* structure to store the registry handles */

typedef struct _RegistryKey {
	uint32		type;
	char		*name; 		/* full name of registry key */
	uint32 		access_granted;
	REGISTRY_HOOK	*hook;	
} REGISTRY_KEY;

/*
 * RPC REGISTRY STRUCTURES
 */

/***********************************************/

typedef struct {
	uint16 *server;
	uint32 access;     
} REG_Q_OPEN_HIVE;

typedef struct {
	POLICY_HND pol;
	WERROR status; 
} REG_R_OPEN_HIVE;


/***********************************************/

typedef struct {
	POLICY_HND pol;
} REG_Q_FLUSH_KEY;

typedef struct {
	WERROR status; 
} REG_R_FLUSH_KEY;


/***********************************************/

typedef struct {
	POLICY_HND handle;
	uint32 sec_info;
	uint32 ptr; 
	BUFHDR hdr_sec;
	SEC_DESC_BUF *data;
} REG_Q_SET_KEY_SEC;

typedef struct {
	WERROR status;
} REG_R_SET_KEY_SEC;


/***********************************************/

typedef struct {
	POLICY_HND handle;
	uint32 sec_info;
	uint32 ptr; 
	BUFHDR hdr_sec; 
	SEC_DESC_BUF *data; 
} REG_Q_GET_KEY_SEC;

typedef struct {
	uint32 sec_info; 
	uint32 ptr; 
	BUFHDR hdr_sec; 
	SEC_DESC_BUF *data;
	WERROR status;
} REG_R_GET_KEY_SEC;

/***********************************************/

typedef struct {
	POLICY_HND handle;   
	UNISTR4 name;   	
	uint32 type;  
	RPC_DATA_BLOB value; 
	uint32 size;
} REG_Q_SET_VALUE;

typedef struct { 
	WERROR status;
} REG_R_SET_VALUE;

/***********************************************/

typedef struct {
	POLICY_HND pol;
	uint32 val_index;
	UNISTR4 name;
	uint32 *type;  
	REGVAL_BUFFER *value; /* value, in byte buffer */
	uint32 *buffer_len; 
	uint32 *name_len; 
} REG_Q_ENUM_VALUE;

typedef struct { 
	UNISTR4 name;
	uint32 *type;
	REGVAL_BUFFER *value;
	uint32 *buffer_len1;
	uint32 *buffer_len2;
	WERROR status;
} REG_R_ENUM_VALUE;

/***********************************************/

typedef struct {
	POLICY_HND handle;
	UNISTR4 name;
	UNISTR4 key_class;
	uint32 options;
	uint32 access;
	
	/* FIXME!  collapse all this into one structure */
	uint32 *sec_info;
	uint32 ptr2;
	BUFHDR hdr_sec;
	uint32 ptr3;
	SEC_DESC_BUF *data;

	uint32 *disposition; 
} REG_Q_CREATE_KEY_EX;

typedef struct {
	POLICY_HND handle;
	uint32 disposition;
	WERROR status; 
} REG_R_CREATE_KEY_EX;

/***********************************************/

typedef struct {
	POLICY_HND handle;
	UNISTR4 name;
} REG_Q_DELETE_KEY;

typedef struct {
	WERROR status; 
} REG_R_DELETE_KEY;

/***********************************************/

typedef struct {
	POLICY_HND handle;
	UNISTR4 name;
} REG_Q_DELETE_VALUE;

typedef struct {
	WERROR status;
} REG_R_DELETE_VALUE;

/***********************************************/

typedef struct {
	POLICY_HND pol;
	UNISTR4 key_class;
} REG_Q_QUERY_KEY;

typedef struct {
	UNISTR4 key_class;
	uint32 num_subkeys;
	uint32 max_subkeylen;
	uint32 reserved; 	/* 0x0000 0000 - according to MSDN (max_subkeysize?) */
	uint32 num_values;
	uint32 max_valnamelen;
	uint32 max_valbufsize; 
	uint32 sec_desc; 	/* 0x0000 0078 */
	NTTIME mod_time;  	/* modified time */
	WERROR status;         
} REG_R_QUERY_KEY;


/***********************************************/

typedef struct {
	POLICY_HND pol;
} REG_Q_GETVERSION;

typedef struct {
	uint32 win_version;
	WERROR status;
} REG_R_GETVERSION;


/***********************************************/

typedef struct {
	POLICY_HND pol; 
	UNISTR4 filename;
	uint32 flags;
} REG_Q_RESTORE_KEY;

typedef struct {
	WERROR status;         /* return status */
} REG_R_RESTORE_KEY;


/***********************************************/


/* I have no idea if this is correct since I 
   have not seen the full structure on the wire 
   as of yet */
   
typedef struct {
	uint32 max_len;
	uint32 len;
	SEC_DESC *secdesc;
} REG_SEC_DESC_BUF;

typedef struct {
	uint32 size;		/* size in bytes of security descriptor */
	REG_SEC_DESC_BUF secdesc;
	uint8  inherit;		/* see MSDN for a description */
} SECURITY_ATTRIBUTE;

typedef struct {
	POLICY_HND pol; 
	UNISTR4 filename;
	SECURITY_ATTRIBUTE *sec_attr;
} REG_Q_SAVE_KEY;

typedef struct {
	WERROR status;         /* return status */
} REG_R_SAVE_KEY;


/***********************************************/

typedef struct {
	POLICY_HND pol; /* policy handle */
} REG_Q_CLOSE;

typedef struct {
	POLICY_HND pol; 
	WERROR status; 
} REG_R_CLOSE;


/***********************************************/

typedef struct {
	POLICY_HND pol; 
	uint32 key_index;       
	uint16 key_name_len;   
	uint16 unknown_1;       /* 0x0414 */
	uint32 ptr1;          
	uint32 unknown_2;       /* 0x0000 020A */
	uint8  pad1[8];        
	uint32 ptr2;           
	uint8  pad2[8];        
	uint32 ptr3;           
	NTTIME time;           
} REG_Q_ENUM_KEY;

typedef struct { 
	UNISTR4 keyname;
	UNISTR4 *classname;
	NTTIME *time;            
	WERROR status;         /* return status */
} REG_R_ENUM_KEY;


/***********************************************/

typedef struct {
	POLICY_HND pol;		/* policy handle */
	UNISTR4  name;

	uint32 ptr_reserved;	/* pointer */
  
	uint32 ptr_buf;		/* the next three fields follow if ptr_buf != 0 */
	uint32 ptr_bufsize;
	uint32 bufsize;
	uint32 buf_unk;

	uint32 unk1;
	uint32 ptr_buflen;
	uint32 buflen;
  
	uint32 ptr_buflen2;
	uint32 buflen2;

} REG_Q_QUERY_VALUE;

typedef struct { 
	uint32 *type;
	REGVAL_BUFFER *value;	/* key value */
	uint32 *buf_max_len;
	uint32 *buf_len;
	WERROR status;	/* return status */
} REG_R_QUERY_VALUE;


/***********************************************/

typedef struct {
	POLICY_HND pol;
	UNISTR4 name; 
	uint32 unknown_0;       /* 32 bit unknown - 0x0000 0000 */
	uint32 access; 
} REG_Q_OPEN_ENTRY;

typedef struct {
	POLICY_HND handle;
	WERROR status;
} REG_R_OPEN_ENTRY;

/***********************************************/
 
typedef struct {
	uint16 *server;
	UNISTR4 *message; 	
	uint32 timeout;		/* in seconds */
	uint8 force;		/* boolean: force shutdown */
	uint8 reboot;		/* boolean: reboot on shutdown */		
} REG_Q_SHUTDOWN;

typedef struct {
	WERROR status;		/* return status */
} REG_R_SHUTDOWN;

/***********************************************/
 
typedef struct {
	uint16 *server;
	UNISTR4 *message; 	
	uint32 timeout;		/* in seconds */
	uint8 force;		/* boolean: force shutdown */
	uint8 reboot;		/* boolean: reboot on shutdown */
	uint32 reason;		/* reason - must be defined code */
} REG_Q_SHUTDOWN_EX;

typedef struct {
	WERROR status;
} REG_R_SHUTDOWN_EX;

/***********************************************/

typedef struct {
	uint16 *server;
} REG_Q_ABORT_SHUTDOWN;

typedef struct { 
	WERROR status; 
} REG_R_ABORT_SHUTDOWN;


#endif /* _RPC_REG_H */

