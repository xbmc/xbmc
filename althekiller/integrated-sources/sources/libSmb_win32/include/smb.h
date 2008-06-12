/* 
   Unix SMB/CIFS implementation.
   SMB parameters and setup, plus a whole lot more.
   
   Copyright (C) Andrew Tridgell              1992-2000
   Copyright (C) John H Terpstra              1996-2002
   Copyright (C) Luke Kenneth Casson Leighton 1996-2000
   Copyright (C) Paul Ashton                  1998-2000
   Copyright (C) Simo Sorce                   2001-2002
   Copyright (C) Martin Pool		      2002
   
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

#ifndef _SMB_H
#define _SMB_H

#ifdef _XBOX
#pragma warning(disable : 4005)
#endif

/* logged when starting the various Samba daemons */
#define COPYRIGHT_STARTUP_MESSAGE	"Copyright Andrew Tridgell and the Samba Team 1992-2006"


#if defined(LARGE_SMB_OFF_T)
#define BUFFER_SIZE (128*1024)
#else /* no large readwrite possible */
#define BUFFER_SIZE (0xFFFF)
#endif

#define SAFETY_MARGIN 1024
#define LARGE_WRITEX_HDR_SIZE 65

#define NMB_PORT 137
#define DGRAM_PORT 138
#define SMB_PORT1 445
#define SMB_PORT2 139
#define SMB_PORTS "445 139"

#define Undefined (-1)
#define False (0)
#define True (1)
#define Auto (2)
#define Required (3)

#ifndef _BOOL
typedef int BOOL;
#define _BOOL       /* So we don't typedef BOOL again in vfs.h */
#endif

#define SIZEOFWORD 2

#ifndef DEF_CREATE_MASK
#define DEF_CREATE_MASK (0755)
#endif

/* string manipulation flags - see clistr.c and srvstr.c */
#define STR_TERMINATE 1
#define STR_UPPER 2
#define STR_ASCII 4
#define STR_UNICODE 8
#define STR_NOALIGN 16
#define STR_TERMINATE_ASCII 128

/* how long to wait for secondary SMB packets (milli-seconds) */
#define SMB_SECONDARY_WAIT (60*1000)

/* Debugging stuff */
#include "debug.h"

/* this defines the error codes that receive_smb can put in smb_read_error */
#define READ_TIMEOUT 1
#define READ_EOF 2
#define READ_ERROR 3
#define WRITE_ERROR 4 /* This error code can go into the client smb_rw_error. */
#define READ_BAD_SIG 5
#define DO_NOT_DO_TDIS 6 /* cli_close_connection() check for this when smbfs wants to keep tree connected */

#define DIR_STRUCT_SIZE 43

/* these define the attribute byte as seen by DOS */
#define aRONLY (1L<<0)		/* 0x01 */
#define aHIDDEN (1L<<1)		/* 0x02 */
#define aSYSTEM (1L<<2)		/* 0x04 */
#define aVOLID (1L<<3)		/* 0x08 */
#define aDIR (1L<<4)		/* 0x10 */
#define aARCH (1L<<5)		/* 0x20 */

/* deny modes */
#define DENY_DOS 0
#define DENY_ALL 1
#define DENY_WRITE 2
#define DENY_READ 3
#define DENY_NONE 4
#define DENY_FCB 7

/* open modes */
#define DOS_OPEN_RDONLY 0
#define DOS_OPEN_WRONLY 1
#define DOS_OPEN_RDWR 2
#define DOS_OPEN_EXEC 3
#define DOS_OPEN_FCB 0xF

/* define shifts and masks for share and open modes. */
#define OPENX_MODE_MASK 0xF
#define DENY_MODE_SHIFT 4
#define DENY_MODE_MASK 0x7
#define GET_OPENX_MODE(x) ((x) & OPENX_MODE_MASK)
#define SET_OPENX_MODE(x) ((x) & OPENX_MODE_MASK)
#define GET_DENY_MODE(x) (((x)>>DENY_MODE_SHIFT) & DENY_MODE_MASK)
#define SET_DENY_MODE(x) (((x) & DENY_MODE_MASK) <<DENY_MODE_SHIFT)

/* Sync on open file (not sure if used anymore... ?) */
#define FILE_SYNC_OPENMODE (1<<14)
#define GET_FILE_SYNC_OPENMODE(x) (((x) & FILE_SYNC_OPENMODE) ? True : False)

/* open disposition values */
#define OPENX_FILE_EXISTS_FAIL 0
#define OPENX_FILE_EXISTS_OPEN 1
#define OPENX_FILE_EXISTS_TRUNCATE 2

/* mask for open disposition. */
#define OPENX_FILE_OPEN_MASK 0x3

#define GET_FILE_OPENX_DISPOSITION(x) ((x) & FILE_OPEN_MASK)
#define SET_FILE_OPENX_DISPOSITION(x) ((x) & FILE_OPEN_MASK)

/* The above can be OR'ed with... */
#define OPENX_FILE_CREATE_IF_NOT_EXIST 0x10
#define OPENX_FILE_FAIL_IF_NOT_EXIST 0

/* share types */
#define STYPE_DISKTREE  0	/* Disk drive */
#define STYPE_PRINTQ    1	/* Spooler queue */
#define STYPE_DEVICE    2	/* Serial device */
#define STYPE_IPC       3	/* Interprocess communication (IPC) */
#define STYPE_HIDDEN    0x80000000 /* share is a hidden one (ends with $) */

#include "doserr.h"

typedef union unid_t {
	uid_t uid;
	gid_t gid;
} unid_t;

/*
 * SMB UCS2 (16-bit unicode) internal type.
 * smb_ucs2_t is *always* in little endian format.
 */

typedef uint16 smb_ucs2_t;

/* ucs2 string types. */
typedef smb_ucs2_t wpstring[PSTRING_LEN];
typedef smb_ucs2_t wfstring[FSTRING_LEN];

#ifdef WORDS_BIGENDIAN
#define UCS2_SHIFT 8
#else
#define UCS2_SHIFT 0
#endif

/* turn a 7 bit character into a ucs2 character */
#define UCS2_CHAR(c) ((c) << UCS2_SHIFT)

/* return an ascii version of a ucs2 character */
#define UCS2_TO_CHAR(c) (((c) >> UCS2_SHIFT) & 0xff)

/* Copy into a smb_ucs2_t from a possibly unaligned buffer. Return the copied smb_ucs2_t */
#define COPY_UCS2_CHAR(dest,src) (((unsigned char *)(dest))[0] = ((unsigned char *)(src))[0],\
				((unsigned char *)(dest))[1] = ((unsigned char *)(src))[1], (dest))

/* pipe string names */
#define PIPE_LANMAN   "\\PIPE\\LANMAN"
#define PIPE_SRVSVC   "\\PIPE\\srvsvc"
#define PIPE_SAMR     "\\PIPE\\samr"
#define PIPE_WINREG   "\\PIPE\\winreg"
#define PIPE_WKSSVC   "\\PIPE\\wkssvc"
#define PIPE_NETLOGON "\\PIPE\\NETLOGON"
#define PIPE_NTLSA    "\\PIPE\\ntlsa"
#define PIPE_NTSVCS   "\\PIPE\\ntsvcs"
#define PIPE_LSASS    "\\PIPE\\lsass"
#define PIPE_LSARPC   "\\PIPE\\lsarpc"
#define PIPE_SPOOLSS  "\\PIPE\\spoolss"
#define PIPE_NETDFS   "\\PIPE\\netdfs"
#define PIPE_ECHO     "\\PIPE\\rpcecho"
#define PIPE_SHUTDOWN "\\PIPE\\initshutdown"
#define PIPE_EPM      "\\PIPE\\epmapper"
#define PIPE_SVCCTL   "\\PIPE\\svcctl"
#define PIPE_EVENTLOG "\\PIPE\\eventlog"

#define PIPE_NETLOGON_PLAIN "\\NETLOGON"

#define PI_LSARPC		0
#define PI_LSARPC_DS		1
#define PI_SAMR			2
#define PI_NETLOGON		3
#define PI_SRVSVC		4
#define PI_WKSSVC		5
#define PI_WINREG		6
#define PI_SPOOLSS		7
#define PI_NETDFS		8
#define PI_ECHO 		9
#define PI_SHUTDOWN		10
#define PI_SVCCTL		11
#define PI_EVENTLOG 		12
#define PI_NTSVCS		13
#define PI_MAX_PIPES		14

/* 64 bit time (100usec) since ????? - cifs6.txt, section 3.5, page 30 */
typedef struct nttime_info {
	uint32 low;
	uint32 high;
} NTTIME;


/* Allowable account control bits */
#define ACB_DISABLED			0x00000001  /* 1 = User account disabled */
#define ACB_HOMDIRREQ			0x00000002  /* 1 = Home directory required */
#define ACB_PWNOTREQ			0x00000004  /* 1 = User password not required */
#define ACB_TEMPDUP			0x00000008  /* 1 = Temporary duplicate account */
#define ACB_NORMAL			0x00000010  /* 1 = Normal user account */
#define ACB_MNS				0x00000020  /* 1 = MNS logon user account */
#define ACB_DOMTRUST			0x00000040  /* 1 = Interdomain trust account */
#define ACB_WSTRUST			0x00000080  /* 1 = Workstation trust account */
#define ACB_SVRTRUST			0x00000100  /* 1 = Server trust account (BDC) */
#define ACB_PWNOEXP			0x00000200  /* 1 = User password does not expire */
#define ACB_AUTOLOCK			0x00000400  /* 1 = Account auto locked */

/* only valid for > Windows 2000 */
#define ACB_ENC_TXT_PWD_ALLOWED		0x00000800  /* 1 = Text password encryped */
#define ACB_SMARTCARD_REQUIRED		0x00001000  /* 1 = Smart Card required */
#define ACB_TRUSTED_FOR_DELEGATION	0x00002000  /* 1 = Trusted for Delegation */
#define ACB_NOT_DELEGATED		0x00004000  /* 1 = Not delegated */
#define ACB_USE_DES_KEY_ONLY		0x00008000  /* 1 = Use DES key only */
#define ACB_DONT_REQUIRE_PREAUTH	0x00010000  /* 1 = Preauth not required */
#define ACB_PWEXPIRED			0x00020000  /* 1 = Password is expired */
#define ACB_NO_AUTH_DATA_REQD		0x00080000  /* 1 = No authorization data required */

#define MAX_HOURS_LEN 32

#ifndef MAXSUBAUTHS
#define MAXSUBAUTHS 15 /* max sub authorities in a SID */
#endif

#define SID_MAX_SIZE ((size_t)(8+(MAXSUBAUTHS*4)))

/* SID Types */
enum SID_NAME_USE {
	SID_NAME_USE_NONE = 0,
	SID_NAME_USER    = 1, /* user */
	SID_NAME_DOM_GRP,     /* domain group */
	SID_NAME_DOMAIN,      /* domain sid */
	SID_NAME_ALIAS,       /* local group */
	SID_NAME_WKN_GRP,     /* well-known group */
	SID_NAME_DELETED,     /* deleted account: needed for c2 rating */
	SID_NAME_INVALID,     /* invalid account */
	SID_NAME_UNKNOWN,     /* unknown sid type */
	SID_NAME_COMPUTER     /* sid for a computer */
};

#define LOOKUP_NAME_ISOLATED 1	/* Look up unqualified names */
#define LOOKUP_NAME_REMOTE   2  /* Ask others */
#define LOOKUP_NAME_ALL (LOOKUP_NAME_ISOLATED|LOOKUP_NAME_REMOTE)

#define LOOKUP_NAME_GROUP    4  /* (unused) This is a NASTY hack for valid users = @foo
				 * where foo also exists in as user. */

/**
 * @brief Security Identifier
 *
 * @sa http://msdn.microsoft.com/library/default.asp?url=/library/en-us/security/accctrl_38yn.asp
 **/
typedef struct sid_info {
	uint8  sid_rev_num;             /**< SID revision number */
	uint8  num_auths;               /**< Number of sub-authorities */
	uint8  id_auth[6];              /**< Identifier Authority */
	/*
	 *  Pointer to sub-authorities.
	 *
	 * @note The values in these uint32's are in *native* byteorder, not
	 * neccessarily little-endian...... JRA.
	 */
	uint32 sub_auths[MAXSUBAUTHS];  
} DOM_SID;

struct lsa_dom_info {
	BOOL valid;
	DOM_SID sid;
	const char *name;
	int num_idxs;
	int *idxs;
};

struct lsa_name_info {
	uint32 rid;
	enum SID_NAME_USE type;
	const char *name;
	int dom_idx;
};

/* Some well-known SIDs */
extern const DOM_SID global_sid_World_Domain;
extern const DOM_SID global_sid_World;
extern const DOM_SID global_sid_Creator_Owner_Domain;
extern const DOM_SID global_sid_NT_Authority;
extern const DOM_SID global_sid_System;
extern const DOM_SID global_sid_NULL;
extern const DOM_SID global_sid_Authenticated_Users;
extern const DOM_SID global_sid_Network;
extern const DOM_SID global_sid_Creator_Owner;
extern const DOM_SID global_sid_Creator_Group;
extern const DOM_SID global_sid_Anonymous;
extern const DOM_SID global_sid_Builtin;
extern const DOM_SID global_sid_Builtin_Administrators;
extern const DOM_SID global_sid_Builtin_Users;
extern const DOM_SID global_sid_Builtin_Guests;
extern const DOM_SID global_sid_Builtin_Power_Users;
extern const DOM_SID global_sid_Builtin_Account_Operators;
extern const DOM_SID global_sid_Builtin_Server_Operators;
extern const DOM_SID global_sid_Builtin_Print_Operators;
extern const DOM_SID global_sid_Builtin_Backup_Operators;
extern const DOM_SID global_sid_Builtin_Replicator;
extern const DOM_SID global_sid_Builtin_PreWin2kAccess;
extern const DOM_SID global_sid_Unix_Users;
extern const DOM_SID global_sid_Unix_Groups;

/*
 * The complete list of SIDS belonging to this user.
 * Created when a vuid is registered.
 * The definition of the user_sids array is as follows :
 *
 * token->user_sids[0] = primary user SID.
 * token->user_sids[1] = primary group SID.
 * token->user_sids[2..num_sids] = supplementary group SIDS.
 */

#define PRIMARY_USER_SID_INDEX 0
#define PRIMARY_GROUP_SID_INDEX 1

typedef struct nt_user_token {
	size_t num_sids;
	DOM_SID *user_sids;
	SE_PRIV privileges;
} NT_USER_TOKEN;

typedef struct _unix_token {
	uid_t uid;
	gid_t gid;
	int ngroups;
	gid_t *groups;
} UNIX_USER_TOKEN;

/* 32 bit time (sec) since 01jan1970 - cifs6.txt, section 3.5, page 30 */
typedef struct time_info {
	uint32 time;
} UTIME;

/* Structure used when SMBwritebmpx is active */
typedef struct {
	size_t wr_total_written; /* So we know when to discard this */
	int32 wr_timeout;
	int32 wr_errclass; /* Cached errors */
	int32 wr_error; /* Cached errors */
	NTSTATUS wr_status; /* Cached errors */
	BOOL  wr_mode; /* write through mode) */
	BOOL  wr_discard; /* discard all further data */
} write_bmpx_struct;

typedef struct write_cache {
	SMB_OFF_T file_size;
	SMB_OFF_T offset;
	size_t alloc_size;
	size_t data_size;
	char *data;
} write_cache;

typedef struct {
	smb_ucs2_t *origname;
	smb_ucs2_t *filename;
	SMB_STRUCT_STAT *statinfo;
} smb_filename;

#include "fake_file.h"

struct fd_handle {
	size_t ref_count;
	int fd;
	SMB_BIG_UINT position_information;
	SMB_OFF_T pos;
	uint32 private_options;	/* NT Create options, but we only look at
				 * NTCREATEX_OPTIONS_PRIVATE_DENY_DOS and
				 * NTCREATEX_OPTIONS_PRIVATE_DENY_FCB (Except
				 * for print files *only*, where
				 * DELETE_ON_CLOSE is not stored in the share
				 * mode database.
				 */
	unsigned long file_id;
};

struct timed_event;
struct idle_event;
struct share_mode_entry;

typedef struct files_struct {
	struct files_struct *next, *prev;
	int fnum;
	struct connection_struct *conn;
	struct fd_handle *fh;
	unsigned int num_smb_operations;
	uint16 rap_print_jobid;
	SMB_DEV_T dev;
	SMB_INO_T inode;
	SMB_BIG_UINT initial_allocation_size; /* Faked up initial allocation on disk. */
	mode_t mode;
	uint16 file_pid;
	uint16 vuid;
	write_bmpx_struct *wbmpx_ptr;
	write_cache *wcp;
	struct timeval open_time;
	uint32 access_mask;		/* NTCreateX access bits (FILE_READ_DATA etc.) */
	uint32 share_access;		/* NTCreateX share constants (FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE). */
	BOOL pending_modtime_owner;
	time_t pending_modtime;
	time_t last_write_time;
	int oplock_type;
	int sent_oplock_break;
	struct timed_event *oplock_timeout;

	struct share_mode_entry *pending_break_messages;
	int num_pending_break_messages;

	BOOL can_lock;
	BOOL can_read;
	BOOL can_write;
	BOOL print_file;
	BOOL modified;
	BOOL is_directory;
	BOOL is_stat;
	BOOL aio_write_behind;
	BOOL lockdb_clean;
	char *fsp_name;
 	FAKE_FILE_HANDLE *fake_file_handle;
} files_struct;

#include "ntquotas.h"
#include "sysquotas.h"

/* used to hold an arbitrary blob of data */
typedef struct data_blob_ {
	uint8 *data;
	size_t length;
	void (*free)(struct data_blob_ *data_blob);
} DATA_BLOB;

/*
 * Structure used to keep directory state information around.
 * Used in NT change-notify code.
 */

typedef struct {
	time_t modify_time;
	time_t status_time;
} dir_status_struct;

struct vuid_cache_entry {
	uint16 vuid;
	BOOL read_only;
	BOOL admin_user;
};

struct vuid_cache {
	unsigned int entries;
	struct vuid_cache_entry array[VUID_CACHE_SIZE];
};

typedef struct {
	char *name;
	BOOL is_wild;
} name_compare_entry;

struct trans_state {
	struct trans_state *next, *prev;
	uint16 vuid;
	uint16 mid;

	uint32 max_param_return;
	uint32 max_data_return;
	uint32 max_setup_return;

	uint8 cmd;		/* SMBtrans or SMBtrans2 */

	fstring name;		/* for trans requests */
	uint16 call;		/* for trans2 and nttrans requests */

	BOOL close_on_completion;
	BOOL one_way;

	unsigned int setup_count;
	uint16 *setup;

	size_t received_data;
	size_t received_param;

	size_t total_param;
	char *param;

	size_t total_data;
	char *data;
};

/* Include VFS stuff */

#include "smb_acls.h"
#include "vfs.h"

struct dfree_cached_info {
	time_t last_dfree_time;
	SMB_BIG_UINT dfree_ret;
	SMB_BIG_UINT bsize;
	SMB_BIG_UINT dfree;
	SMB_BIG_UINT dsize;
};

struct dptr_struct;

typedef struct connection_struct {
	struct connection_struct *next, *prev;
	TALLOC_CTX *mem_ctx;
	unsigned cnum; /* an index passed over the wire */
	int service;
	BOOL force_user;
	BOOL force_group;
	struct vuid_cache vuid_cache;
	struct dptr_struct *dirptr;
	BOOL printer;
	BOOL ipc;
	BOOL read_only; /* Attributes for the current user of the share. */
	BOOL admin_user; /* Attributes for the current user of the share. */
	char *dirpath;
	char *connectpath;
	char *origpath;

	struct vfs_ops vfs;                   /* Filesystem operations */
	struct vfs_ops vfs_opaque;			/* OPAQUE Filesystem operations */
	struct vfs_handle_struct *vfs_handles;		/* for the new plugins */

	char *user; /* name of user who *opened* this connection */
	uid_t uid; /* uid of user who *opened* this connection */
	gid_t gid; /* gid of user who *opened* this connection */
	char client_address[18]; /* String version of client IP address. */

	uint16 vuid; /* vuid of user who *opened* this connection, or UID_FIELD_INVALID */

	/* following groups stuff added by ih */

	/* This groups info is valid for the user that *opened* the connection */
	size_t ngroups;
	gid_t *groups;
	NT_USER_TOKEN *nt_user_token;
	
	time_t lastused;
	time_t lastused_count;
	BOOL used;
	int num_files_open;
	unsigned int num_smb_operations; /* Count of smb operations on this tree. */

	BOOL case_sensitive;
	BOOL case_preserve;
	BOOL short_case_preserve;

	name_compare_entry *hide_list; /* Per-share list of files to return as hidden. */
	name_compare_entry *veto_list; /* Per-share list of files to veto (never show). */
	name_compare_entry *veto_oplock_list; /* Per-share list of files to refuse oplocks on. */       
	name_compare_entry *aio_write_behind_list; /* Per-share list of files to use aio write behind on. */       
	struct dfree_cached_info *dfree_info;
	struct trans_state *pending_trans;
} connection_struct;

struct current_user {
	connection_struct *conn;
	uint16 vuid;
	UNIX_USER_TOKEN ut;
	NT_USER_TOKEN *nt_user_token;
};

/* Defines for the sent_oplock_break field above. */
#define NO_BREAK_SENT 0
#define BREAK_TO_NONE_SENT 1
#define LEVEL_II_BREAK_SENT 2

typedef struct {
	fstring smb_name; /* user name from the client */
	fstring unix_name; /* unix user name of a validated user */
	fstring full_name; /* to store full name (such as "Joe Bloggs") from gecos field of password file */
	fstring domain; /* domain that the client specified */
} userdom_struct;

/* Extra fields above "LPQ_PRINTING" are used to map extra NT status codes. */

enum {LPQ_QUEUED=0,LPQ_PAUSED,LPQ_SPOOLING,LPQ_PRINTING,LPQ_ERROR,LPQ_DELETING,
      LPQ_OFFLINE,LPQ_PAPEROUT,LPQ_PRINTED,LPQ_DELETED,LPQ_BLOCKED,LPQ_USER_INTERVENTION};

typedef struct _print_queue_struct {
	int job;		/* normally the UNIX jobid -- see note in 
				   printing.c:traverse_fn_delete() */
	int size;
	int page_count;
	int status;
	int priority;
	time_t time;
	fstring fs_user;
	fstring fs_file;
} print_queue_struct;

enum {LPSTAT_OK, LPSTAT_STOPPED, LPSTAT_ERROR};

typedef struct {
	fstring message;
	int qcount;
	int status;
}  print_status_struct;

/* used for server information: client, nameserv and ipc */
struct server_info_struct {
	fstring name;
	uint32 type;
	fstring comment;
	fstring domain; /* used ONLY in ipc.c NOT namework.c */
	BOOL server_added; /* used ONLY in ipc.c NOT namework.c */
};

#ifdef _XBOX
#undef interface
#define interface SMB_INTERFACE
#endif

/* used for network interfaces */
struct interface {
	struct interface *next, *prev;
	struct in_addr ip;
	struct in_addr bcast;
	struct in_addr nmask;
};

/* Internal message queue for deferred opens. */
struct pending_message_list {
	struct pending_message_list *next, *prev;
	struct timeval request_time; /* When was this first issued? */
	struct timeval end_time; /* When does this time out? */
	DATA_BLOB buf;
	DATA_BLOB private_data;
};

/* struct returned by get_share_modes */
struct share_mode_entry {
	struct process_id pid;
	uint16 op_mid;
	uint16 op_type;
	uint32 access_mask;		/* NTCreateX access bits (FILE_READ_DATA etc.) */
	uint32 share_access;		/* NTCreateX share constants (FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE). */
	uint32 private_options;	/* NT Create options, but we only look at
				 * NTCREATEX_OPTIONS_PRIVATE_DENY_DOS and
				 * NTCREATEX_OPTIONS_PRIVATE_DENY_FCB for
				 * smbstatus and swat */
	struct timeval time;
	SMB_DEV_T dev;
	SMB_INO_T inode;
	unsigned long share_file_id;
	uint32 uid;		/* uid of file opener. */
};

/* oplock break message definition - linearization of share_mode_entry.

Offset  Data			length.
0	struct process_id pid	4
4	uint16 op_mid		2
6	uint16 op_type		2
8	uint32 access_mask	4
12	uint32 share_access	4
16	uint32 private_options	4
20	uint32 time sec		4
24	uint32 time usec	4
28	SMB_DEV_T dev		8 bytes.
36	SMB_INO_T inode		8 bytes
44	unsigned long file_id	4 bytes
48	uint32 uid		4 bytes
52

*/
#define MSG_SMB_SHARE_MODE_ENTRY_SIZE 52

struct share_mode_lock {
	const char *servicepath; /* canonicalized. */
	const char *filename;
	SMB_DEV_T dev;
	SMB_INO_T ino;
	int num_share_modes;
	struct share_mode_entry *share_modes;
	UNIX_USER_TOKEN *delete_token;
	BOOL delete_on_close;
	BOOL initial_delete_on_close;
	BOOL fresh;
	BOOL modified;
};

/*
 * Internal structure of locking.tdb share mode db.
 * Used by locking.c and libsmbsharemodes.c
 */

struct locking_data {
	union {
		struct {
			int num_share_mode_entries;
			BOOL delete_on_close;
			BOOL initial_delete_on_close; /* Only set at NTCreateX if file was created. */
			uint32 delete_token_size; /* Only valid if either of
						     the two previous fields
						     are True. */
		} s;
		struct share_mode_entry dummy; /* Needed for alignment. */
	} u;
	/* The following four entries are implicit
	   struct share_mode_entry modes[num_share_mode_entries];
	   char unix_token[delete_token_size] (divisible by 4).
	   char share_name[];
	   char file_name[];
        */
};

#define NT_HASH_LEN 16
#define LM_HASH_LEN 16

/* Password history contants. */
#define PW_HISTORY_SALT_LEN 16
#define SALTED_MD5_HASH_LEN 16
#define PW_HISTORY_ENTRY_LEN (PW_HISTORY_SALT_LEN+SALTED_MD5_HASH_LEN)
#define MAX_PW_HISTORY_LEN 24

/*
 * Flags for account policy.
 */
#define AP_MIN_PASSWORD_LEN 		1
#define AP_PASSWORD_HISTORY		2
#define AP_USER_MUST_LOGON_TO_CHG_PASS	3
#define AP_MAX_PASSWORD_AGE		4
#define AP_MIN_PASSWORD_AGE		5
#define AP_LOCK_ACCOUNT_DURATION	6
#define AP_RESET_COUNT_TIME		7
#define AP_BAD_ATTEMPT_LOCKOUT		8
#define AP_TIME_TO_LOGOUT		9
#define AP_REFUSE_MACHINE_PW_CHANGE	10

/*
 * Flags for local user manipulation.
 */

#define LOCAL_ADD_USER 0x1
#define LOCAL_DELETE_USER 0x2
#define LOCAL_DISABLE_USER 0x4
#define LOCAL_ENABLE_USER 0x8
#define LOCAL_TRUST_ACCOUNT 0x10
#define LOCAL_SET_NO_PASSWORD 0x20
#define LOCAL_SET_PASSWORD 0x40
#define LOCAL_SET_LDAP_ADMIN_PW 0x80
#define LOCAL_INTERDOM_ACCOUNT 0x100
#define LOCAL_AM_ROOT 0x200  /* Act as root */

/* key and data in the connections database - used in smbstatus and smbd */
struct connections_key {
	struct process_id pid;
	int cnum;
	fstring name;
};

struct connections_data {
	int magic;
	struct process_id pid;
	int cnum;
	uid_t uid;
	gid_t gid;
	char name[24];
	char addr[24];
	char machine[FSTRING_LEN];
	time_t start;
	uint32 bcast_msg_flags;
};


/* the following are used by loadparm for option lists */
typedef enum {
	P_BOOL,P_BOOLREV,P_CHAR,P_INTEGER,P_OCTAL,P_LIST,
	P_STRING,P_USTRING,P_GSTRING,P_UGSTRING,P_ENUM,P_SEP
} parm_type;

typedef enum {
	P_LOCAL,P_GLOBAL,P_SEPARATOR,P_NONE
} parm_class;

struct enum_list {
	int value;
	const char *name;
};

struct parm_struct {
	const char *label;
	parm_type type;
	parm_class p_class;
	void *ptr;
	BOOL (*special)(int snum, const char *, char **);
	const struct enum_list *enum_list;
	unsigned flags;
	union {
		BOOL bvalue;
		int ivalue;
		char *svalue;
		char cvalue;
		char **lvalue;
	} def;
};

/* The following flags are used in SWAT */
#define FLAG_BASIC 	0x0001 /* Display only in BASIC view */
#define FLAG_SHARE 	0x0002 /* file sharing options */
#define FLAG_PRINT 	0x0004 /* printing options */
#define FLAG_GLOBAL 	0x0008 /* local options that should be globally settable in SWAT */
#define FLAG_WIZARD 	0x0010 /* Parameters that the wizard will operate on */
#define FLAG_ADVANCED 	0x0020 /* Parameters that will be visible in advanced view */
#define FLAG_DEVELOPER 	0x0040 /* No longer used */
#define FLAG_DEPRECATED 0x1000 /* options that should no longer be used */
#define FLAG_HIDE  	0x2000 /* options that should be hidden in SWAT */
#define FLAG_DOS_STRING 0x4000 /* convert from UNIX to DOS codepage when reading this string. */

/* passed to br lock code - the UNLOCK_LOCK should never be stored into the tdb
   and is used in calculating POSIX unlock ranges only. */

enum brl_type {READ_LOCK, WRITE_LOCK, PENDING_LOCK, UNLOCK_LOCK};
enum brl_flavour {WINDOWS_LOCK = 0, POSIX_LOCK = 1};

/* The key used in the brlock database. */

struct lock_key {
	SMB_DEV_T device;
	SMB_INO_T inode;
};

struct byte_range_lock {
	files_struct *fsp;
	unsigned int num_locks;
	BOOL modified;
	struct lock_key key;
	void *lock_data;
};

#define BRLOCK_FN_CAST() \
	void (*)(SMB_DEV_T dev, SMB_INO_T ino, struct process_id pid, \
				 enum brl_type lock_type, \
				 enum brl_flavour lock_flav, \
				 br_off start, br_off size)

#define BRLOCK_FN(fn) \
	void (*fn)(SMB_DEV_T dev, SMB_INO_T ino, struct process_id pid, \
				 enum brl_type lock_type, \
				 enum brl_flavour lock_flav, \
				 br_off start, br_off size)

#define LOCKING_FN_CAST() \
	void (*)(struct share_mode_entry *, const char *, const char *)

#define LOCKING_FN(fn) \
	void (*fn)(struct share_mode_entry *, const char *, const char *)

struct bitmap {
	uint32 *b;
	unsigned int n;
};

#ifndef LOCKING_VERSION
#define LOCKING_VERSION 4
#endif /* LOCKING_VERSION */

/* the basic packet size, assuming no words or bytes */
#define smb_size 39

/* offsets into message for common items */
#define smb_com 8
#define smb_rcls 9
#define smb_reh 10
#define smb_err 11
#define smb_flg 13
#define smb_flg2 14
#define smb_pidhigh 16
#define smb_ss_field 18
#define smb_tid 28
#define smb_pid 30
#define smb_uid 32
#define smb_mid 34
#define smb_wct 36
#define smb_vwv 37
#define smb_vwv0 37
#define smb_vwv1 39
#define smb_vwv2 41
#define smb_vwv3 43
#define smb_vwv4 45
#define smb_vwv5 47
#define smb_vwv6 49
#define smb_vwv7 51
#define smb_vwv8 53
#define smb_vwv9 55
#define smb_vwv10 57
#define smb_vwv11 59
#define smb_vwv12 61
#define smb_vwv13 63
#define smb_vwv14 65
#define smb_vwv15 67
#define smb_vwv16 69
#define smb_vwv17 71

/* flag defines. CIFS spec 3.1.1 */
#define FLAG_SUPPORT_LOCKREAD       0x01
#define FLAG_CLIENT_BUF_AVAIL       0x02
#define FLAG_RESERVED               0x04
#define FLAG_CASELESS_PATHNAMES     0x08
#define FLAG_CANONICAL_PATHNAMES    0x10
#define FLAG_REQUEST_OPLOCK         0x20
#define FLAG_REQUEST_BATCH_OPLOCK   0x40
#define FLAG_REPLY                  0x80

/* the complete */
#define SMBmkdir      0x00   /* create directory */
#define SMBrmdir      0x01   /* delete directory */
#define SMBopen       0x02   /* open file */
#define SMBcreate     0x03   /* create file */
#define SMBclose      0x04   /* close file */
#define SMBflush      0x05   /* flush file */
#define SMBunlink     0x06   /* delete file */
#define SMBmv         0x07   /* rename file */
#define SMBgetatr     0x08   /* get file attributes */
#define SMBsetatr     0x09   /* set file attributes */
#define SMBread       0x0A   /* read from file */
#define SMBwrite      0x0B   /* write to file */
#define SMBlock       0x0C   /* lock byte range */
#define SMBunlock     0x0D   /* unlock byte range */
#define SMBctemp      0x0E   /* create temporary file */
#define SMBmknew      0x0F   /* make new file */
#define SMBchkpth     0x10   /* check directory path */
#define SMBexit       0x11   /* process exit */
#define SMBlseek      0x12   /* seek */
#define SMBtcon       0x70   /* tree connect */
#define SMBtconX      0x75   /* tree connect and X*/
#define SMBtdis       0x71   /* tree disconnect */
#define SMBnegprot    0x72   /* negotiate protocol */
#define SMBdskattr    0x80   /* get disk attributes */
#define SMBsearch     0x81   /* search directory */
#define SMBsplopen    0xC0   /* open print spool file */
#define SMBsplwr      0xC1   /* write to print spool file */
#define SMBsplclose   0xC2   /* close print spool file */
#define SMBsplretq    0xC3   /* return print queue */
#define SMBsends      0xD0   /* send single block message */
#define SMBsendb      0xD1   /* send broadcast message */
#define SMBfwdname    0xD2   /* forward user name */
#define SMBcancelf    0xD3   /* cancel forward */
#define SMBgetmac     0xD4   /* get machine name */
#define SMBsendstrt   0xD5   /* send start of multi-block message */
#define SMBsendend    0xD6   /* send end of multi-block message */
#define SMBsendtxt    0xD7   /* send text of multi-block message */

/* Core+ protocol */
#define SMBlockread	  0x13   /* Lock a range and read */
#define SMBwriteunlock 0x14 /* Unlock a range then write */
#define SMBreadbraw   0x1a  /* read a block of data with no smb header */
#define SMBwritebraw  0x1d  /* write a block of data with no smb header */
#define SMBwritec     0x20  /* secondary write request */
#define SMBwriteclose 0x2c  /* write a file then close it */

/* dos extended protocol */
#define SMBreadBraw      0x1A   /* read block raw */
#define SMBreadBmpx      0x1B   /* read block multiplexed */
#define SMBreadBs        0x1C   /* read block (secondary response) */
#define SMBwriteBraw     0x1D   /* write block raw */
#define SMBwriteBmpx     0x1E   /* write block multiplexed */
#define SMBwriteBs       0x1F   /* write block (secondary request) */
#define SMBwriteC        0x20   /* write complete response */
#define SMBsetattrE      0x22   /* set file attributes expanded */
#define SMBgetattrE      0x23   /* get file attributes expanded */
#define SMBlockingX      0x24   /* lock/unlock byte ranges and X */
#define SMBtrans         0x25   /* transaction - name, bytes in/out */
#define SMBtranss        0x26   /* transaction (secondary request/response) */
#define SMBioctl         0x27   /* IOCTL */
#define SMBioctls        0x28   /* IOCTL  (secondary request/response) */
#define SMBcopy          0x29   /* copy */
#define SMBmove          0x2A   /* move */
#define SMBecho          0x2B   /* echo */
#define SMBopenX         0x2D   /* open and X */
#define SMBreadX         0x2E   /* read and X */
#define SMBwriteX        0x2F   /* write and X */
#define SMBsesssetupX    0x73   /* Session Set Up & X (including User Logon) */
#define SMBffirst        0x82   /* find first */
#define SMBfunique       0x83   /* find unique */
#define SMBfclose        0x84   /* find close */
#define SMBkeepalive     0x85   /* keepalive */
#define SMBinvalid       0xFE   /* invalid command */

/* Extended 2.0 protocol */
#define SMBtrans2        0x32   /* TRANS2 protocol set */
#define SMBtranss2       0x33   /* TRANS2 protocol set, secondary command */
#define SMBfindclose     0x34   /* Terminate a TRANSACT2_FINDFIRST */
#define SMBfindnclose    0x35   /* Terminate a TRANSACT2_FINDNOTIFYFIRST */
#define SMBulogoffX      0x74   /* user logoff */

/* NT SMB extensions. */
#define SMBnttrans       0xA0   /* NT transact */
#define SMBnttranss      0xA1   /* NT transact secondary */
#define SMBntcreateX     0xA2   /* NT create and X */
#define SMBntcancel      0xA4   /* NT cancel */
#define SMBntrename      0xA5   /* NT rename */

/* These are the trans subcommands */
#define TRANSACT_SETNAMEDPIPEHANDLESTATE  0x01 
#define TRANSACT_DCERPCCMD                0x26
#define TRANSACT_WAITNAMEDPIPEHANDLESTATE 0x53

/* These are the TRANS2 sub commands */
#define TRANSACT2_OPEN				0x00
#define TRANSACT2_FINDFIRST			0x01
#define TRANSACT2_FINDNEXT			0x02
#define TRANSACT2_QFSINFO			0x03
#define TRANSACT2_SETFSINFO			0x04
#define TRANSACT2_QPATHINFO			0x05
#define TRANSACT2_SETPATHINFO			0x06
#define TRANSACT2_QFILEINFO			0x07
#define TRANSACT2_SETFILEINFO			0x08
#define TRANSACT2_FSCTL				0x09
#define TRANSACT2_IOCTL				0x0A
#define TRANSACT2_FINDNOTIFYFIRST		0x0B
#define TRANSACT2_FINDNOTIFYNEXT		0x0C
#define TRANSACT2_MKDIR				0x0D
#define TRANSACT2_SESSION_SETUP			0x0E
#define TRANSACT2_GET_DFS_REFERRAL		0x10
#define TRANSACT2_REPORT_DFS_INCONSISTANCY	0x11

/* These are the NT transact sub commands. */
#define NT_TRANSACT_CREATE                1
#define NT_TRANSACT_IOCTL                 2
#define NT_TRANSACT_SET_SECURITY_DESC     3
#define NT_TRANSACT_NOTIFY_CHANGE         4
#define NT_TRANSACT_RENAME                5
#define NT_TRANSACT_QUERY_SECURITY_DESC   6
#define NT_TRANSACT_GET_USER_QUOTA	  7
#define NT_TRANSACT_SET_USER_QUOTA	  8

/* These are the NT transact_get_user_quota sub commands */
#define TRANSACT_GET_USER_QUOTA_LIST_CONTINUE	0x0000
#define TRANSACT_GET_USER_QUOTA_LIST_START	0x0100
#define TRANSACT_GET_USER_QUOTA_FOR_SID		0x0101

/* Relevant IOCTL codes */
#define IOCTL_QUERY_JOB_INFO      0x530060

/* these are the trans2 sub fields for primary requests */
#define smb_tpscnt smb_vwv0
#define smb_tdscnt smb_vwv1
#define smb_mprcnt smb_vwv2
#define smb_mdrcnt smb_vwv3
#define smb_msrcnt smb_vwv4
#define smb_flags smb_vwv5
#define smb_timeout smb_vwv6
#define smb_pscnt smb_vwv9
#define smb_psoff smb_vwv10
#define smb_dscnt smb_vwv11
#define smb_dsoff smb_vwv12
#define smb_suwcnt smb_vwv13
#define smb_setup smb_vwv14
#define smb_setup0 smb_setup
#define smb_setup1 (smb_setup+2)
#define smb_setup2 (smb_setup+4)

/* these are for the secondary requests */
#define smb_spscnt smb_vwv2
#define smb_spsoff smb_vwv3
#define smb_spsdisp smb_vwv4
#define smb_sdscnt smb_vwv5
#define smb_sdsoff smb_vwv6
#define smb_sdsdisp smb_vwv7
#define smb_sfid smb_vwv8

/* and these for responses */
#define smb_tprcnt smb_vwv0
#define smb_tdrcnt smb_vwv1
#define smb_prcnt smb_vwv3
#define smb_proff smb_vwv4
#define smb_prdisp smb_vwv5
#define smb_drcnt smb_vwv6
#define smb_droff smb_vwv7
#define smb_drdisp smb_vwv8

/* these are for the NT trans primary request. */
#define smb_nt_MaxSetupCount smb_vwv0
#define smb_nt_Flags (smb_vwv0 + 1)
#define smb_nt_TotalParameterCount (smb_vwv0 + 3)
#define smb_nt_TotalDataCount (smb_vwv0 + 7)
#define smb_nt_MaxParameterCount (smb_vwv0 + 11)
#define smb_nt_MaxDataCount (smb_vwv0 + 15)
#define smb_nt_ParameterCount (smb_vwv0 + 19)
#define smb_nt_ParameterOffset (smb_vwv0 + 23)
#define smb_nt_DataCount (smb_vwv0 + 27)
#define smb_nt_DataOffset (smb_vwv0 + 31)
#define smb_nt_SetupCount (smb_vwv0 + 35)
#define smb_nt_Function (smb_vwv0 + 36)
#define smb_nt_SetupStart (smb_vwv0 + 38)

/* these are for the NT trans secondary request. */
#define smb_nts_TotalParameterCount (smb_vwv0 + 3)
#define smb_nts_TotalDataCount (smb_vwv0 + 7)
#define smb_nts_ParameterCount (smb_vwv0 + 11)
#define smb_nts_ParameterOffset (smb_vwv0 + 15)
#define smb_nts_ParameterDisplacement (smb_vwv0 + 19)
#define smb_nts_DataCount (smb_vwv0 + 23)
#define smb_nts_DataOffset (smb_vwv0 + 27)
#define smb_nts_DataDisplacement (smb_vwv0 + 31)

/* these are for the NT trans reply. */
#define smb_ntr_TotalParameterCount (smb_vwv0 + 3)
#define smb_ntr_TotalDataCount (smb_vwv0 + 7)
#define smb_ntr_ParameterCount (smb_vwv0 + 11)
#define smb_ntr_ParameterOffset (smb_vwv0 + 15)
#define smb_ntr_ParameterDisplacement (smb_vwv0 + 19)
#define smb_ntr_DataCount (smb_vwv0 + 23)
#define smb_ntr_DataOffset (smb_vwv0 + 27)
#define smb_ntr_DataDisplacement (smb_vwv0 + 31)

/* these are for the NT create_and_X */
#define smb_ntcreate_NameLength (smb_vwv0 + 5)
#define smb_ntcreate_Flags (smb_vwv0 + 7)
#define smb_ntcreate_RootDirectoryFid (smb_vwv0 + 11)
#define smb_ntcreate_DesiredAccess (smb_vwv0 + 15)
#define smb_ntcreate_AllocationSize (smb_vwv0 + 19)
#define smb_ntcreate_FileAttributes (smb_vwv0 + 27)
#define smb_ntcreate_ShareAccess (smb_vwv0 + 31)
#define smb_ntcreate_CreateDisposition (smb_vwv0 + 35)
#define smb_ntcreate_CreateOptions (smb_vwv0 + 39)
#define smb_ntcreate_ImpersonationLevel (smb_vwv0 + 43)
#define smb_ntcreate_SecurityFlags (smb_vwv0 + 47)

/* this is used on a TConX. I'm not sure the name is very helpful though */
#define SMB_SUPPORT_SEARCH_BITS        0x0001
#define SMB_SHARE_IN_DFS               0x0002

/* Named pipe write mode flags. Used in writeX calls. */
#define PIPE_RAW_MODE 0x4
#define PIPE_START_MESSAGE 0x8

/* File Specific access rights */
#define FILE_READ_DATA        0x00000001
#define FILE_WRITE_DATA       0x00000002
#define FILE_APPEND_DATA      0x00000004
#define FILE_READ_EA          0x00000008 /* File and directory */
#define FILE_WRITE_EA         0x00000010 /* File and directory */
#define FILE_EXECUTE          0x00000020
#define FILE_DELETE_CHILD     0x00000040
#define FILE_READ_ATTRIBUTES  0x00000080
#define FILE_WRITE_ATTRIBUTES 0x00000100

#define FILE_ALL_ACCESS       0x000001FF

/* Directory specific access rights */
#define FILE_LIST_DIRECTORY   0x00000001
#define FILE_ADD_FILE         0x00000002
#define FILE_ADD_SUBDIRECTORY 0x00000004
#define FILE_TRAVERSE         0x00000020
#define FILE_DELETE_CHILD     0x00000040

/* the desired access to use when opening a pipe */
#define DESIRED_ACCESS_PIPE 0x2019f
 
/* Generic access masks & rights. */
#define DELETE_ACCESS        0x00010000 /* (1L<<16) */
#define READ_CONTROL_ACCESS  0x00020000 /* (1L<<17) */
#define WRITE_DAC_ACCESS     0x00040000 /* (1L<<18) */
#define WRITE_OWNER_ACCESS   0x00080000 /* (1L<<19) */
#define SYNCHRONIZE_ACCESS   0x00100000 /* (1L<<20) */

#define SYSTEM_SECURITY_ACCESS 0x01000000 /* (1L<<24) */
#define MAXIMUM_ALLOWED_ACCESS 0x02000000 /* (1L<<25) */
#define GENERIC_ALL_ACCESS     0x10000000 /* (1<<28) */
#define GENERIC_EXECUTE_ACCESS 0x20000000 /* (1<<29) */
#define GENERIC_WRITE_ACCESS   0x40000000 /* (1<<30) */
#define GENERIC_READ_ACCESS    ((unsigned)0x80000000) /* (((unsigned)1)<<31) */

/* Mapping of generic access rights for files to specific rights. */

#define FILE_GENERIC_ALL (STANDARD_RIGHTS_REQUIRED_ACCESS| SYNCHRONIZE_ACCESS|FILE_ALL_ACCESS)

#define FILE_GENERIC_READ (STANDARD_RIGHTS_READ_ACCESS|FILE_READ_DATA|FILE_READ_ATTRIBUTES|\
							FILE_READ_EA|SYNCHRONIZE_ACCESS)

#define FILE_GENERIC_WRITE (STD_RIGHT_READ_CONTROL_ACCESS|FILE_WRITE_DATA|FILE_WRITE_ATTRIBUTES|\
							FILE_WRITE_EA|FILE_APPEND_DATA|SYNCHRONIZE_ACCESS)

#define FILE_GENERIC_EXECUTE (STANDARD_RIGHTS_EXECUTE_ACCESS|\
								FILE_EXECUTE|SYNCHRONIZE_ACCESS)

/* Mapping of access rights to UNIX perms. */
#define UNIX_ACCESS_RWX		FILE_GENERIC_ALL
#define UNIX_ACCESS_R 		FILE_GENERIC_READ
#define UNIX_ACCESS_W		FILE_GENERIC_WRITE
#define UNIX_ACCESS_X		FILE_GENERIC_EXECUTE

/* Mapping of access rights to UNIX perms. for a UNIX directory. */
#define UNIX_DIRECTORY_ACCESS_RWX		FILE_GENERIC_ALL
#define UNIX_DIRECTORY_ACCESS_R 		FILE_GENERIC_READ
#define UNIX_DIRECTORY_ACCESS_W			FILE_GENERIC_WRITE
#define UNIX_DIRECTORY_ACCESS_X			FILE_GENERIC_EXECUTE

#if 0
/*
 * This is the old mapping we used to use. To get W2KSP2 profiles
 * working we need to map to the canonical file perms.
 */
#define UNIX_ACCESS_RWX (UNIX_ACCESS_R|UNIX_ACCESS_W|UNIX_ACCESS_X)
#define UNIX_ACCESS_R (READ_CONTROL_ACCESS|SYNCHRONIZE_ACCESS|\
			FILE_READ_ATTRIBUTES|FILE_READ_EA|FILE_READ_DATA)
#define UNIX_ACCESS_W (READ_CONTROL_ACCESS|SYNCHRONIZE_ACCESS|\
			FILE_WRITE_ATTRIBUTES|FILE_WRITE_EA|\
			FILE_APPEND_DATA|FILE_WRITE_DATA)
#define UNIX_ACCESS_X (READ_CONTROL_ACCESS|SYNCHRONIZE_ACCESS|\
			FILE_EXECUTE|FILE_READ_ATTRIBUTES)
#endif

#define UNIX_ACCESS_NONE (WRITE_OWNER_ACCESS)

/* Flags field. */
#define REQUEST_OPLOCK 2
#define REQUEST_BATCH_OPLOCK 4
#define OPEN_DIRECTORY 8
#define EXTENDED_RESPONSE_REQUIRED 0x10

/* ShareAccess field. */
#define FILE_SHARE_NONE 0 /* Cannot be used in bitmask. */
#define FILE_SHARE_READ 1
#define FILE_SHARE_WRITE 2
#define FILE_SHARE_DELETE 4

/* FileAttributesField */
#define FILE_ATTRIBUTE_READONLY		0x001L
#define FILE_ATTRIBUTE_HIDDEN		0x002L
#define FILE_ATTRIBUTE_SYSTEM		0x004L
#define FILE_ATTRIBUTE_DIRECTORY	0x010L
#define FILE_ATTRIBUTE_ARCHIVE		0x020L
#define FILE_ATTRIBUTE_NORMAL		0x080L
#define FILE_ATTRIBUTE_TEMPORARY	0x100L
#define FILE_ATTRIBUTE_SPARSE		0x200L
#define FILE_ATTRIBUTE_REPARSE_POINT    0x400L
#define FILE_ATTRIBUTE_COMPRESSED	0x800L
#define FILE_ATTRIBUTE_OFFLINE          0x1000L
#define FILE_ATTRIBUTE_NONINDEXED	0x2000L
#define FILE_ATTRIBUTE_ENCRYPTED        0x4000L
#define SAMBA_ATTRIBUTES_MASK		0x7F

/* Flags - combined with attributes. */
#define FILE_FLAG_WRITE_THROUGH    0x80000000L
#define FILE_FLAG_NO_BUFFERING     0x20000000L
#define FILE_FLAG_RANDOM_ACCESS    0x10000000L
#define FILE_FLAG_SEQUENTIAL_SCAN  0x08000000L
#define FILE_FLAG_DELETE_ON_CLOSE  0x04000000L
#define FILE_FLAG_BACKUP_SEMANTICS 0x02000000L
#define FILE_FLAG_POSIX_SEMANTICS  0x01000000L

/* CreateDisposition field. */
#define FILE_SUPERSEDE 0		/* File exists overwrite/supersede. File not exist create. */
#define FILE_OPEN 1			/* File exists open. File not exist fail. */
#define FILE_CREATE 2			/* File exists fail. File not exist create. */
#define FILE_OPEN_IF 3			/* File exists open. File not exist create. */
#define FILE_OVERWRITE 4		/* File exists overwrite. File not exist fail. */
#define FILE_OVERWRITE_IF 5		/* File exists overwrite. File not exist create. */

/* CreateOptions field. */
#define FILE_DIRECTORY_FILE       0x0001
#define FILE_WRITE_THROUGH        0x0002
#define FILE_SEQUENTIAL_ONLY      0x0004
#define FILE_NON_DIRECTORY_FILE   0x0040
#define FILE_NO_EA_KNOWLEDGE      0x0200
#define FILE_EIGHT_DOT_THREE_ONLY 0x0400
#define FILE_RANDOM_ACCESS        0x0800
#define FILE_DELETE_ON_CLOSE      0x1000
#define FILE_OPEN_BY_FILE_ID	  0x2000

/* Private create options used by the ntcreatex processing code. From Samba4. */
#define NTCREATEX_OPTIONS_PRIVATE_DENY_DOS     0x01000000
#define NTCREATEX_OPTIONS_PRIVATE_DENY_FCB     0x02000000

/* Responses when opening a file. */
#define FILE_WAS_SUPERSEDED 0
#define FILE_WAS_OPENED 1
#define FILE_WAS_CREATED 2
#define FILE_WAS_OVERWRITTEN 3

/* File type flags */
#define FILE_TYPE_DISK  0
#define FILE_TYPE_BYTE_MODE_PIPE 1
#define FILE_TYPE_MESSAGE_MODE_PIPE 2
#define FILE_TYPE_PRINTER 3
#define FILE_TYPE_COMM_DEVICE 4
#define FILE_TYPE_UNKNOWN 0xFFFF

/* Flag for NT transact rename call. */
#define RENAME_REPLACE_IF_EXISTS 1

/* flags for SMBntrename call (from Samba4) */
#define RENAME_FLAG_MOVE_CLUSTER_INFORMATION 0x102 /* ???? */
#define RENAME_FLAG_HARD_LINK                0x103
#define RENAME_FLAG_RENAME                   0x104
#define RENAME_FLAG_COPY                     0x105

/* Filesystem Attributes. */
#define FILE_CASE_SENSITIVE_SEARCH      0x00000001
#define FILE_CASE_PRESERVED_NAMES       0x00000002
#define FILE_UNICODE_ON_DISK            0x00000004
/* According to cifs9f, this is 4, not 8 */
/* Acconding to testing, this actually sets the security attribute! */
#define FILE_PERSISTENT_ACLS            0x00000008
#define FILE_FILE_COMPRESSION           0x00000010
#define FILE_VOLUME_QUOTAS              0x00000020
#define FILE_SUPPORTS_SPARSE_FILES      0x00000040
#define FILE_SUPPORTS_REPARSE_POINTS    0x00000080
#define FILE_SUPPORTS_REMOTE_STORAGE    0x00000100
#define FS_LFN_APIS                     0x00004000
#define FILE_VOLUME_IS_COMPRESSED       0x00008000
#define FILE_SUPPORTS_OBJECT_IDS        0x00010000
#define FILE_SUPPORTS_ENCRYPTION        0x00020000
#define FILE_NAMED_STREAMS              0x00040000
#define FILE_READ_ONLY_VOLUME           0x00080000

/* ChangeNotify flags. */
#define FILE_NOTIFY_CHANGE_FILE        0x001
#define FILE_NOTIFY_CHANGE_DIR_NAME    0x002
#define FILE_NOTIFY_CHANGE_ATTRIBUTES  0x004
#define FILE_NOTIFY_CHANGE_SIZE        0x008
#define FILE_NOTIFY_CHANGE_LAST_WRITE  0x010
#define FILE_NOTIFY_CHANGE_LAST_ACCESS 0x020
#define FILE_NOTIFY_CHANGE_CREATION    0x040
#define FILE_NOTIFY_CHANGE_EA          0x080
#define FILE_NOTIFY_CHANGE_SECURITY    0x100
#define FILE_NOTIFY_CHANGE_FILE_NAME   0x200

/* where to find the base of the SMB packet proper */
#define smb_base(buf) (((char *)(buf))+4)

/* we don't allow server strings to be longer than 48 characters as
   otherwise NT will not honour the announce packets */
#define MAX_SERVER_STRING_LENGTH 48


#define SMB_SUCCESS 0  /* The request was successful. */

#ifdef WITH_DFS
void dfs_unlogin(void);
extern int dcelogin_atmost_once;
#endif

#ifdef NOSTRDUP
char *strdup(char *s);
#endif

#ifndef SIGNAL_CAST
#define SIGNAL_CAST (RETSIGTYPE (*)(int))
#endif

#ifndef SELECT_CAST
#define SELECT_CAST
#endif

/* these are used in NetServerEnum to choose what to receive */
#define SV_TYPE_WORKSTATION         0x00000001
#define SV_TYPE_SERVER              0x00000002
#define SV_TYPE_SQLSERVER           0x00000004
#define SV_TYPE_DOMAIN_CTRL         0x00000008
#define SV_TYPE_DOMAIN_BAKCTRL      0x00000010
#define SV_TYPE_TIME_SOURCE         0x00000020
#define SV_TYPE_AFP                 0x00000040
#define SV_TYPE_NOVELL              0x00000080
#define SV_TYPE_DOMAIN_MEMBER       0x00000100
#define SV_TYPE_PRINTQ_SERVER       0x00000200
#define SV_TYPE_DIALIN_SERVER       0x00000400
#define SV_TYPE_SERVER_UNIX         0x00000800
#define SV_TYPE_NT                  0x00001000
#define SV_TYPE_WFW                 0x00002000
#define SV_TYPE_SERVER_MFPN         0x00004000
#define SV_TYPE_SERVER_NT           0x00008000
#define SV_TYPE_POTENTIAL_BROWSER   0x00010000
#define SV_TYPE_BACKUP_BROWSER      0x00020000
#define SV_TYPE_MASTER_BROWSER      0x00040000
#define SV_TYPE_DOMAIN_MASTER       0x00080000
#define SV_TYPE_SERVER_OSF          0x00100000
#define SV_TYPE_SERVER_VMS          0x00200000
#define SV_TYPE_WIN95_PLUS          0x00400000
#define SV_TYPE_DFS_SERVER	    0x00800000
#define SV_TYPE_ALTERNATE_XPORT     0x20000000  
#define SV_TYPE_LOCAL_LIST_ONLY     0x40000000  
#define SV_TYPE_DOMAIN_ENUM         0x80000000
#define SV_TYPE_ALL                 0xFFFFFFFF  

/* This was set by JHT in liaison with Jeremy Allison early 1997
 * History:
 * Version 4.0 - never made public
 * Version 4.10 - New to 1.9.16p2, lost in space 1.9.16p3 to 1.9.16p9
 *              - Reappeared in 1.9.16p11 with fixed smbd services
 * Version 4.20 - To indicate that nmbd and browsing now works better
 * Version 4.50 - Set at release of samba-2.2.0 by JHT
 *
 *  Note: In the presence of NT4.X do not set above 4.9
 *        Setting this above 4.9 can have undesired side-effects.
 *        This may change again in Samba-3.0 after further testing. JHT
 */
 
#define DEFAULT_MAJOR_VERSION 0x04
#define DEFAULT_MINOR_VERSION 0x09

/* Browser Election Values */
#define BROWSER_ELECTION_VERSION	0x010f
#define BROWSER_CONSTANT	0xaa55

/* Sercurity mode bits. */
#define NEGOTIATE_SECURITY_USER_LEVEL		0x01
#define NEGOTIATE_SECURITY_CHALLENGE_RESPONSE	0x02
#define NEGOTIATE_SECURITY_SIGNATURES_ENABLED	0x04
#define NEGOTIATE_SECURITY_SIGNATURES_REQUIRED	0x08

/* NT Flags2 bits - cifs6.txt section 3.1.2 */
   
#define FLAGS2_LONG_PATH_COMPONENTS    0x0001
#define FLAGS2_EXTENDED_ATTRIBUTES     0x0002
#define FLAGS2_SMB_SECURITY_SIGNATURES 0x0004
#define FLAGS2_IS_LONG_NAME            0x0040
#define FLAGS2_EXTENDED_SECURITY       0x0800 
#define FLAGS2_DFS_PATHNAMES           0x1000
#define FLAGS2_READ_PERMIT_EXECUTE     0x2000
#define FLAGS2_32_BIT_ERROR_CODES      0x4000 
#define FLAGS2_UNICODE_STRINGS         0x8000

#define FLAGS2_WIN2K_SIGNATURE         0xC852 /* Hack alert ! For now... JRA. */

/* Capabilities.  see ftp.microsoft.com/developr/drg/cifs/cifs/cifs4.txt */

#define CAP_RAW_MODE         0x0001
#define CAP_MPX_MODE         0x0002
#define CAP_UNICODE          0x0004
#define CAP_LARGE_FILES      0x0008
#define CAP_NT_SMBS          0x0010
#define CAP_RPC_REMOTE_APIS  0x0020
#define CAP_STATUS32         0x0040
#define CAP_LEVEL_II_OPLOCKS 0x0080
#define CAP_LOCK_AND_READ    0x0100
#define CAP_NT_FIND          0x0200
#define CAP_DFS              0x1000
#define CAP_W2K_SMBS         0x2000
#define CAP_LARGE_READX      0x4000
#define CAP_LARGE_WRITEX     0x8000
#define CAP_UNIX             0x800000 /* Capabilities for UNIX extensions. Created by HP. */
#define CAP_EXTENDED_SECURITY 0x80000000

/* protocol types. It assumes that higher protocols include lower protocols
   as subsets */
enum protocol_types {PROTOCOL_NONE,PROTOCOL_CORE,PROTOCOL_COREPLUS,PROTOCOL_LANMAN1,PROTOCOL_LANMAN2,PROTOCOL_NT1};

/* security levels */
enum security_types {SEC_SHARE,SEC_USER,SEC_SERVER,SEC_DOMAIN,SEC_ADS};

/* server roles */
enum server_types {
	ROLE_STANDALONE,
	ROLE_DOMAIN_MEMBER,
	ROLE_DOMAIN_BDC,
	ROLE_DOMAIN_PDC
};

/* printing types */
enum printing_types {PRINT_BSD,PRINT_SYSV,PRINT_AIX,PRINT_HPUX,
		     PRINT_QNX,PRINT_PLP,PRINT_LPRNG,PRINT_SOFTQ,
		     PRINT_CUPS,PRINT_LPRNT,PRINT_LPROS2,PRINT_IPRINT
#ifdef DEVELOPER
,PRINT_TEST,PRINT_VLP
#endif /* DEVELOPER */
};

/* LDAP schema types */
enum schema_types {SCHEMA_COMPAT, SCHEMA_AD, SCHEMA_SAMBA};

/* LDAP SSL options */
enum ldap_ssl_types {LDAP_SSL_ON, LDAP_SSL_OFF, LDAP_SSL_START_TLS};

/* LDAP PASSWD SYNC methods */
enum ldap_passwd_sync_types {LDAP_PASSWD_SYNC_ON, LDAP_PASSWD_SYNC_OFF, LDAP_PASSWD_SYNC_ONLY};

/* Remote architectures we know about. */
enum remote_arch_types {RA_UNKNOWN, RA_WFWG, RA_OS2, RA_WIN95, RA_WINNT,
			RA_WIN2K, RA_WINXP, RA_WIN2K3, RA_SAMBA, RA_CIFSFS};

/* case handling */
enum case_handling {CASE_LOWER,CASE_UPPER};

/* ACL compatibility */
enum acl_compatibility {ACL_COMPAT_AUTO, ACL_COMPAT_WINNT, ACL_COMPAT_WIN2K};
/*
 * Global value meaing that the smb_uid field should be
 * ingored (in share level security and protocol level == CORE)
 */

#define UID_FIELD_INVALID 0
#define VUID_OFFSET 100 /* Amount to bias returned vuid numbers */

/* 
 * Size of buffer to use when moving files across filesystems. 
 */
#define COPYBUF_SIZE (8*1024)

/*
 * Used in chaining code.
 */
extern int chain_size;

/*
 * Map the Core and Extended Oplock requesst bits down
 * to common bits (EXCLUSIVE_OPLOCK & BATCH_OPLOCK).
 */

/*
 * Core protocol.
 */
#define CORE_OPLOCK_REQUEST(inbuf) \
    ((CVAL(inbuf,smb_flg)&(FLAG_REQUEST_OPLOCK|FLAG_REQUEST_BATCH_OPLOCK))>>5)

/*
 * Extended protocol.
 */
#define EXTENDED_OPLOCK_REQUEST(inbuf) ((SVAL(inbuf,smb_vwv2)&((1<<1)|(1<<2)))>>1)

/* Lock types. */
#define LOCKING_ANDX_SHARED_LOCK 0x1
#define LOCKING_ANDX_OPLOCK_RELEASE 0x2
#define LOCKING_ANDX_CHANGE_LOCKTYPE 0x4
#define LOCKING_ANDX_CANCEL_LOCK 0x8
#define LOCKING_ANDX_LARGE_FILES 0x10

/*
 * Bits we test with.
 * Note these must fit into 16-bits.
 */

#define NO_OPLOCK 0
#define EXCLUSIVE_OPLOCK 1
#define BATCH_OPLOCK 2
#define LEVEL_II_OPLOCK 4

/* The following are Samba-private. */
#define INTERNAL_OPEN_ONLY 8
#define FAKE_LEVEL_II_OPLOCK 16	/* Client requested no_oplock, but we have to
				 * inform potential level2 holders on
				 * write. */
#define DEFERRED_OPEN_ENTRY 32
#define UNUSED_SHARE_MODE_ENTRY 64
#define FORCE_OPLOCK_BREAK_TO_NONE 128

/* None of the following should ever appear in fsp->oplock_request. */
#define SAMBA_PRIVATE_OPLOCK_MASK (INTERNAL_OPEN_ONLY|DEFERRED_OPEN_ENTRY|UNUSED_SHARE_MODE_ENTRY|FORCE_OPLOCK_BREAK_TO_NONE)

#define EXCLUSIVE_OPLOCK_TYPE(lck) ((lck) & ((unsigned int)EXCLUSIVE_OPLOCK|(unsigned int)BATCH_OPLOCK))
#define BATCH_OPLOCK_TYPE(lck) ((lck) & (unsigned int)BATCH_OPLOCK)
#define LEVEL_II_OPLOCK_TYPE(lck) ((lck) & ((unsigned int)LEVEL_II_OPLOCK|(unsigned int)FAKE_LEVEL_II_OPLOCK))

struct inform_level2_message {
	SMB_DEV_T dev;
	SMB_INO_T inode;
	uint16 mid;
	unsigned long target_file_id;
	unsigned long source_file_id;
};

/* kernel_oplock_message definition.

struct kernel_oplock_message {
	SMB_DEV_T dev;
	SMB_INO_T inode;
	unsigned long file_id;
};

Offset  Data                  length.
0     SMB_DEV_T dev           8 bytes.
8     SMB_INO_T inode         8 bytes
16    unsigned long file_id   4 bytes
20

*/
#define MSG_SMB_KERNEL_BREAK_SIZE 20

/* file_renamed_message definition.

struct file_renamed_message {
	SMB_DEV_T dev;
	SMB_INO_T inode;
	char names[1]; A variable area containing sharepath and filename.
};

Offset  Data			length.
0	SMB_DEV_T dev		8 bytes.
8	SMB_INO_T inode		8 bytes
16	char [] name		zero terminated namelen bytes
minimum length == 18.

*/

#define MSG_FILE_RENAMED_MIN_SIZE 16

/*
 * On the wire return values for oplock types.
 */

#define CORE_OPLOCK_GRANTED (1<<5)
#define EXTENDED_OPLOCK_GRANTED (1<<15)

#define NO_OPLOCK_RETURN 0
#define EXCLUSIVE_OPLOCK_RETURN 1
#define BATCH_OPLOCK_RETURN 2
#define LEVEL_II_OPLOCK_RETURN 3

/* Oplock levels */
#define OPLOCKLEVEL_NONE 0
#define OPLOCKLEVEL_II 1

/*
 * Capabilities abstracted for different systems.
 */

enum smbd_capability {
    KERNEL_OPLOCK_CAPABILITY,
    DMAPI_ACCESS_CAPABILITY
};

/* if a kernel does support oplocks then a structure of the following
   typee is used to describe how to interact with the kernel */
struct kernel_oplocks {
	files_struct * (*receive_message)(fd_set *fds);
	BOOL (*set_oplock)(files_struct *fsp, int oplock_type);
	void (*release_oplock)(files_struct *fsp);
	BOOL (*msg_waiting)(fd_set *fds);
	int notification_fd;
};


/* this structure defines the functions for doing change notify in
   various implementations */
struct cnotify_fns {
	void * (*register_notify)(connection_struct *conn, char *path, uint32 flags);
	BOOL (*check_notify)(connection_struct *conn, uint16 vuid, char *path, uint32 flags, void *data, time_t t);
	void (*remove_notify)(void *data);
	int select_time;
	int notification_fd;
};

#include "smb_macros.h"

#define MAX_NETBIOSNAME_LEN 16
/* DOS character, NetBIOS namestring. Type used on the wire. */
typedef char nstring[MAX_NETBIOSNAME_LEN];
/* Unix character, NetBIOS namestring. Type used to manipulate name in nmbd. */
typedef char unstring[MAX_NETBIOSNAME_LEN*4];

/* A netbios name structure. */
struct nmb_name {
	nstring      name;
	char         scope[64];
	unsigned int name_type;
};

/* A netbios node status array element. */
typedef struct node_status_ {
	nstring name;
	unsigned char type;
	unsigned char flags;
} NODE_STATUS_STRUCT;

/* The extra info from a NetBIOS node status query */
struct node_status_extra {
	unsigned char mac_addr[6];
	/* There really is more here ... */ 
};

struct pwd_info {
	BOOL null_pwd;
	BOOL cleartext;

	fstring password;
};

typedef struct user_struct {
	struct user_struct *next, *prev;
	uint16 vuid; /* Tag for this entry. */
	uid_t uid; /* uid of a validated user */
	gid_t gid; /* gid of a validated user */

	userdom_struct user;
	char *homedir;
	char *unix_homedir;
	char *logon_script;
	
	BOOL guest;

	/* following groups stuff added by ih */
	/* This groups info is needed for when we become_user() for this uid */
	int n_groups;
	gid_t *groups;

	NT_USER_TOKEN *nt_user_token;

	DATA_BLOB session_key;

	char *session_keystr; /* used by utmp and pam session code.  
				 TDB key string */
	int homes_snum;

	struct auth_serversupplied_info *server_info;

	struct auth_ntlmssp_state *auth_ntlmssp_state;

} user_struct;

struct unix_error_map {
	int unix_error;
	int dos_class;
	int dos_code;
	NTSTATUS nt_error;
};

/*
 * Size of new password account encoding string.  This is enough space to
 * hold 11 ACB characters, plus the surrounding [] and a terminating null.
 * Do not change unless you are adding new ACB bits!
 */

#define NEW_PW_FORMAT_SPACE_PADDED_LEN 14

/*
   Do you want session setups at user level security with a invalid
   password to be rejected or allowed in as guest? WinNT rejects them
   but it can be a pain as it means "net view" needs to use a password

   You have 3 choices in the setting of map_to_guest:

   "NEVER_MAP_TO_GUEST" means session setups with an invalid password
   are rejected. This is the default.

   "MAP_TO_GUEST_ON_BAD_USER" means session setups with an invalid password
   are rejected, unless the username does not exist, in which case it
   is treated as a guest login

   "MAP_TO_GUEST_ON_BAD_PASSWORD" means session setups with an invalid password
   are treated as a guest login

   Note that map_to_guest only has an effect in user or server
   level security.
*/

#define NEVER_MAP_TO_GUEST 		0
#define MAP_TO_GUEST_ON_BAD_USER 	1
#define MAP_TO_GUEST_ON_BAD_PASSWORD 	2
#define MAP_TO_GUEST_ON_BAD_UID 	3

#define SAFE_NETBIOS_CHARS ". -_"

/* generic iconv conversion structure */
typedef struct _smb_iconv_t {
	size_t (*direct)(void *cd, const char **inbuf, size_t *inbytesleft,
			 char **outbuf, size_t *outbytesleft);
	size_t (*pull)(void *cd, const char **inbuf, size_t *inbytesleft,
		       char **outbuf, size_t *outbytesleft);
	size_t (*push)(void *cd, const char **inbuf, size_t *inbytesleft,
		       char **outbuf, size_t *outbytesleft);
	void *cd_direct, *cd_pull, *cd_push;
	char *from_name, *to_name;
} *smb_iconv_t;

/* The maximum length of a trust account password.
   Used when we randomly create it, 15 char passwords
   exceed NT4's max password length */

#define DEFAULT_TRUST_ACCOUNT_PASSWORD_LENGTH 14

#include "popt_common.h"

#define PORT_NONE	0
#ifndef LDAP_PORT
#define LDAP_PORT	389
#endif

/* used by the IP comparison function */
struct ip_service {
	struct in_addr ip;
	unsigned port;
};

/* Used by the SMB signing functions. */

typedef struct smb_sign_info {
	void (*sign_outgoing_message)(char *outbuf, struct smb_sign_info *si);
	BOOL (*check_incoming_message)(char *inbuf, struct smb_sign_info *si, BOOL must_be_ok);
	void (*free_signing_context)(struct smb_sign_info *si);
	void *signing_context;

	BOOL negotiated_smb_signing;
	BOOL allow_smb_signing;
	BOOL doing_signing;
	BOOL mandatory_signing;
	BOOL seen_valid; /* Have I ever seen a validly signed packet? */
} smb_sign_info;

struct ea_struct {
	uint8 flags;
	char *name;
	DATA_BLOB value;
};

struct ea_list {
	struct ea_list *next, *prev;
	struct ea_struct ea;
};

/* EA names used internally in Samba. KEEP UP TO DATE with prohibited_ea_names in trans2.c !. */
#define SAMBA_POSIX_INHERITANCE_EA_NAME "user.SAMBA_PAI"
/* EA to use for DOS attributes */
#define SAMBA_XATTR_DOS_ATTRIB "user.DOSATTRIB"

struct uuid {
	uint32 time_low;
	uint16 time_mid;
	uint16 time_hi_and_version;
	uint8  clock_seq[2];
	uint8  node[6];
};
#define UUID_SIZE 16

#define UUID_FLAT_SIZE 16
typedef struct uuid_flat {
	uint8 info[UUID_FLAT_SIZE];
} UUID_FLAT;

/* map readonly options */
enum mapreadonly_options {MAP_READONLY_NO, MAP_READONLY_YES, MAP_READONLY_PERMISSIONS};

/* usershare error codes. */
enum usershare_err {
		USERSHARE_OK=0,
		USERSHARE_MALFORMED_FILE,
		USERSHARE_BAD_VERSION,
		USERSHARE_MALFORMED_PATH,
		USERSHARE_MALFORMED_COMMENT_DEF,
		USERSHARE_MALFORMED_ACL_DEF,
		USERSHARE_ACL_ERR,
		USERSHARE_PATH_NOT_ABSOLUTE,
		USERSHARE_PATH_IS_DENIED,
		USERSHARE_PATH_NOT_ALLOWED,
		USERSHARE_PATH_NOT_DIRECTORY,
		USERSHARE_POSIX_ERR
};

/* Different reasons for closing a file. */
enum file_close_type {NORMAL_CLOSE=0,SHUTDOWN_CLOSE,ERROR_CLOSE};

#endif /* _SMB_H */
