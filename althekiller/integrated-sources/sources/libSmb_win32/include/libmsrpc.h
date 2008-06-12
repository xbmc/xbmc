/* 
 *  Unix SMB/CIFS implementation.
 *  MS-RPC client library API definitions/prototypes
 *
 *  Copyright (C) Chris Nicholls              2005.
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

#ifndef LIBMSRPC_H
#define LIBMSRPC_H


#include "includes.h"
#include "libsmbclient.h"
#include "libsmb_internal.h"

/*server capability levels*/
#define SRV_WIN_NT4     1
#define SRV_WIN_2K      2
#define SRV_WIN_2K_SP3  3
#define SRV_WIN_2K3     4

/**@defgroup handle Server Handle*/
/**@defgroup Library_Functions Library/Utility Functions*/
/**@defgroup lsa_defs LSA Definitions*/
/**@defgroup LSA_Functions LSA Functions*/
/**@defgroup reg_defs Registry Definitions*/
/**@defgroup Reg_Functions Registry Functions*/
/**@defgroup sam_defs SAM Definitions*/
/**@defgroup SAM_Functions SAM Functions*/
/**@defgroup svc_defs Service Control Definitions*/
/**@defgroup SCM_Functions Service Control Functions*/

/**Operation was unsuccessful*/
#define CAC_FAILURE           0
/**Operation was successful*/
#define CAC_SUCCESS           1
/**Operation was only partially successful
 *  an example of this is if you try to lookup a list of accounts to SIDs and not all accounts can be resolved*/
#define CAC_PARTIAL_SUCCESS   2

/**@ingroup CAC_errors Use this to see if the last operation failed - useful for enumeration functions that use multiple calls*/
#define CAC_OP_FAILED(status) !NT_STATUS_IS_OK(status) && \
                              NT_STATUS_V(status) != NT_STATUS_V(STATUS_SOME_UNMAPPED) && \
                              NT_STATUS_V(status) != NT_STATUS_V(STATUS_NO_MORE_FILES) && \
                              NT_STATUS_V(status) != NT_STATUS_V(NT_STATUS_NO_MORE_ENTRIES) && \
                              NT_STATUS_V(status) != NT_STATUS_V(NT_STATUS_NONE_MAPPED) && \
                              NT_STATUS_V(status) != NT_STATUS_V(NT_STATUS_GUIDS_EXHAUSTED)


/**Privilege string constants*/
#define CAC_SE_CREATE_TOKEN            "SeCreateTokenPrivilege"
#define CAC_SE_ASSIGN_PRIMARY_TOKEN    "SeAssignPrimaryTokenPrivilege"
#define CAC_SE_LOCK_MEMORY             "SeLockMemoryPrivilege"
#define CAC_SE_INCREASE_QUOTA          "SeIncreaseQuotaPrivilege"
#define CAC_SE_MACHINE_ACCOUNT         "SeMachineAccountPrivilege"
#define CAC_SE_TCB                     "SeTcbPrivilege"
#define CAC_SE_SECURITY                "SeSecurityPrivilege"
#define CAC_SE_TAKE_OWNERSHIP          "SeTakeOwnershipPrivilege"
#define CAC_SE_LOAD_DRIVER             "SeLoadDriverPrivilege"
#define CAC_SE_SYSTEM_PROFILE          "SeSystemProfilePrivilege"
#define CAC_SE_SYSTEM_TIME             "SeSystemtimePrivilege"
#define CAC_SE_PROFILE_SINGLE_PROC     "SeProfileSingleProcessPrivilege"
#define CAC_SE_INCREASE_BASE_PRIORITY  "SeIncreaseBasePriorityPrivilege"
#define CAC_SE_CREATE_PAGEFILE         "SeCreatePagefilePrivilege"
#define CAC_SE_CREATE_PERMANENT        "SeCreatePermanentPrivilege"
#define CAC_SE_BACKUP                  "SeBackupPrivilege"
#define CAC_SE_RESTORE                 "SeRestorePrivilege"
#define CAC_SE_SHUTDOWN                "SeShutdownPrivilege"
#define CAC_SE_DEBUG                   "SeDebugPrivilege"
#define CAC_SE_AUDIT                   "SeAuditPrivilege"
#define CAC_SE_SYSTEM_ENV              "SeSystemEnvironmentPrivilege"
#define CAC_SE_CHANGE_NOTIFY           "SeChangeNotifyPrivilege"
#define CAC_SE_REMOTE_SHUTDOWN         "SeRemoteShutdownPrivilege"
#define CAC_SE_UNDOCK                  "SeUndockPrivilege"
#define CAC_SE_SYNC_AGENT              "SeSyncAgentPrivilege"
#define CAC_SE_ENABLE_DELEGATION       "SeEnableDelegationPrivilege"
#define CAC_SE_MANAGE_VOLUME           "SeManageVolumePrivilege"
#define CAC_SE_IMPERSONATE             "SeImpersonatePrivilege"
#define CAC_SE_CREATE_GLOBAL           "SeCreateGlobalPrivilege"
#define CAC_SE_PRINT_OPERATOR          "SePrintOperatorPrivilege"
#define CAC_SE_NETWORK_LOGON           "SeNetworkLogonRight"
#define CAC_SE_INTERACTIVE_LOGON       "SeInteractiveLogonRight"
#define CAC_SE_BATCH_LOGON             "SeBatchLogonRight"
#define CAC_SE_SERVICE_LOGON           "SeServiceLogonRight"
#define CAC_SE_ADD_USERS               "SeAddUsersPrivilege"
#define CAC_SE_DISK_OPERATOR           "SeDiskOperatorPrivilege"

/**
 * @addtogroup lsa_defs
 * @{
 */
/**used to specify what data to retrieve using cac_LsaQueryTrustedDomainInformation*/
#define CAC_INFO_TRUSTED_DOMAIN_NAME         0x1
#define CAC_INFO_TRUSTED_DOMAIN_POSIX_OFFSET 0x3
#define CAC_INFO_TRUSTED_DOMAIN_PASSWORD     0x4

/**Used when requesting machine domain information*/
#define CAC_DOMAIN_INFO 0x0003

/**Used when requesting machine local information*/
#define CAC_LOCAL_INFO  0x0005

/**Stores information about a SID*/
typedef struct _CACSIDINFO {
   /**The actual SID*/
   DOM_SID sid;
   
   /**The name of the object which maps to this SID*/
   char *name;

   /**The domain the SID belongs to*/
   char *domain;
} CacSidInfo;
/* @} */

/**
 * @addtogroup reg_defs
 * @{
 */
/**Null terminated string*/
typedef char*  REG_SZ_DATA;

/**Null terminated string with windows environment variables that should be expanded*/
typedef char*  REG_EXPAND_SZ_DATA;

/**Binary data of some kind*/
typedef struct _REGBINARYDATA {
   uint32 data_length;
   uint8 * data;
} REG_BINARY_DATA;
   
/**32-bit (little endian) number*/
typedef uint32 REG_DWORD_DATA;

/**32-bit big endian number*/
typedef uint32 REG_DWORD_BE_DATA;

/**array of strings*/
typedef struct _REGMULTISZDATA {
   uint32 num_strings;

   char **strings;
} REG_MULTI_SZ_DATA;

typedef union _REGVALUEDATA {
   REG_SZ_DATA          reg_sz;
   REG_EXPAND_SZ_DATA   reg_expand_sz;
   REG_BINARY_DATA      reg_binary;
   REG_DWORD_DATA       reg_dword;
   REG_DWORD_BE_DATA    reg_dword_be;
   REG_MULTI_SZ_DATA    reg_multi_sz;
} REG_VALUE_DATA;
/**@}*/

/**
 * @addtogroup sam_defs
 * @{
 */

#define CAC_USER_RID  0x1
#define CAC_GROUP_RID 0x2

typedef struct _CACLOOKUPRIDSRECORD {
   char *name;
   uint32 rid;

   /**If found, this will be one of:
    * - CAC_USER_RID
    * - CAC_GROUP_RID
    */
   uint32 type;
   
   /*if the name or RID was looked up, then found = True*/
   BOOL found;
} CacLookupRidsRecord;

typedef struct _CACUSERINFO {
   /**Last logon time*/
   time_t logon_time;

   /**Last logoff time*/
   time_t logoff_time;

   /**Last kickoff time*/
   time_t kickoff_time;

   /**Last password set time*/
   time_t pass_last_set_time;

   /**Time password can change*/
   time_t pass_can_change_time;

   /**Time password must change*/
   time_t pass_must_change_time;

   /**LM user password*/
   uint8 lm_password[8];

   /**NT user password*/
   uint8 nt_password[8];

   /**User's RID*/
   uint32 rid;

   /**RID of primary group*/
   uint32 group_rid;

   /**User's ACB mask*/
   uint32 acb_mask;

   /**Bad password count*/
   uint16 bad_passwd_count;

   /**Number of logons*/
   uint16 logon_count;

   /**Change password at next logon?*/
   BOOL pass_must_change;

   /**Username*/
   char *username;

   /**User's full name*/
   char *full_name;

   /**User's home directory*/
   char *home_dir;

   /**Home directory drive*/
   char *home_drive;

   /**Logon script*/
   char *logon_script;

   /**Path to profile*/
   char *profile_path;

   /**Account description*/
   char *description;

   /**Login from workstations*/
   char *workstations;

   char *dial;

   /**Possible logon hours*/
   LOGON_HRS *logon_hours;

} CacUserInfo;

typedef struct _CACGROUPINFO {
   /**Group name*/
   char *name;

   /**Description*/
   char *description;
 
   /**Number of members*/
   uint32 num_members;
} CacGroupInfo, CacAliasInfo;

/**Represents a period (duration) of time*/
typedef struct _CACTIME {
   /**Number of days*/
   uint32 days;

   /**Number of hours*/
   uint32 hours;

   /**Number of minutes*/
   uint32 minutes;

   /**number of seconds*/
   uint32 seconds;
} CacTime;


typedef struct _CACDOMINFO {
   /**The server role. Should be one of:
    *   ROLE_STANDALONE
    *   ROLE_DOMAIN_MEMBER
    *   ROLE_DOMAIN_BDC
    *   ROLE_DOMAIN_PDC
    *   see include/smb.h
    */
   uint32 server_role;

   /**Number of domain users*/
   uint32 num_users;

   /**Number of domain groups*/
   uint32 num_domain_groups;
   
   /**Number of local groups*/
   uint32 num_local_groups;

   /**Comment*/
   char *comment;

   /**Domain name*/
   char *domain_name;

   /**Server name*/
   char *server_name;

   /**Minimum password length*/
   uint16 min_pass_length;

   /**How many previous passwords to remember - ie, password cannot be the same as N previous passwords*/
   uint16 pass_history;

   /**How long (from now) before passwords expire*/
   CacTime expire;

   /**How long (from now) before passwords can be changed*/
   CacTime min_pass_age;

   /**How long users are locked out for too many bad password attempts*/
   CacTime lockout_duration;

   /**How long before lockouts are reset*/
   CacTime lockout_reset;

   /**How many bad password attempts before lockout occurs*/
   uint16 num_bad_attempts;
} CacDomainInfo;

/**@}*/ /*sam_defs*/

/**@addtogroup svc_defs
 * @{
 */
typedef struct _CACSERVICE {
   /**The service name*/
   char *service_name;

   /**The display name of the service*/
   char *display_name;
   
   /**Current status of the service - see include/rpc_svcctl.h for SERVICE_STATUS definition*/
   SERVICE_STATUS status;
} CacService;

typedef struct __CACSERVICECONFIG {
   /**The service type*/
   uint32 type;

   /**The start type. Should be one of:
    * - SVCCTL_BOOT_START
    * - SVCCTL_SYSTEM_START
    * - SVCCTL_AUTO_START
    * - SVCCTL_DEMAND_START
    */
   uint32 start_type;

   uint32 error_control;

   /**Path to executable*/
   char *exe_path;

   /***/
   char *load_order_group;

   uint32 tag_id;

   /**Any dependencies for the service*/
   char *dependencies;

   /**Run as...*/
   char *start_name;

   /**Service display name*/
   char *display_name;
   
} CacServiceConfig;
/**@}*/ /*svc_defs*/

#include "libmsrpc_internal.h"

/**
 * @addtogroup handle
 * @{
 */

/**
 * Server handle used to keep track of client/server/pipe information. Use cac_NewServerHandle() to allocate. 
 * Initiliaze as many values as possible before calling cac_Connect(). 
 * 
 * @note When allocating memory for the fields, use SMB_MALLOC() (or equivalent) instead of talloc() (or equivalent) -  
 * If memory is not allocated for a field, cac_Connect will allocate sizeof(fstring) bytes for it.
 * 
 * @note It may be wise to allocate large buffers for these fields and strcpy data into them.
 *
 * @see cac_NewServerHandle()
 * @see cac_FreeHandle()
 */
typedef struct _CACSERVERHANDLE {
   /** debug level
    */
   int debug;

   /** netbios name used to make connections
    */
   char *netbios_name;

   /** domain name used to make connections
    */
   char *domain;

   /** username used to make connections
    */
   char *username;

   /** user's password plain text string
    */
   char *password;

   /** name or IP address of server we are currently working with
    */
   char *server;

   /**stores the latest NTSTATUS code
    */
   NTSTATUS status;
   
   /** internal. do not modify!
    */
   struct CacServerHandleInternal _internal;

} CacServerHandle;

/*@}*/

/**internal function. do not call this function*/
SMBCSRV *cac_GetServer(CacServerHandle *hnd);


/** @addtogroup Library_Functions
 * @{
 */
/**
 * Initializes the library - do not need to call this function.  Open's smb.conf as well as initializes logging.
 * @param debug Debug level for library to use
 */

void cac_Init(int debug);

/**
 * Creates an un-initialized CacServerHandle
 * @param allocate_fields If True, the function will allocate sizeof(fstring) bytes for all char * fields in the handle
 * @return - un-initialized server handle
 *         - NULL if no memory could be allocated
 */
CacServerHandle * cac_NewServerHandle(BOOL allocate_fields);

/**
 * Specifies the smbc_get_auth_data_fn to use if you do not want to use the default.
 * @param hnd non-NULL server handle
 * @param auth_fn  auth_data_fn to set in server handle
 */

void cac_SetAuthDataFn(CacServerHandle *hnd, smbc_get_auth_data_fn auth_fn);

/** Use your own libsmbclient context - not necessary. 
 * @note You must still call cac_Connect() after specifying your own libsmbclient context
 * @param hnd Initialized, but not connected CacServerHandle
 * @param ctx The libsmbclient context you would like to use.
 */
void cac_SetSmbcContext(CacServerHandle *hnd, SMBCCTX *ctx);

/** Connects to a specified server.  If there is already a connection to a different server, 
 *    it will be cleaned up before connecting to the new server.
 * @param hnd   Pre-initialized CacServerHandle
 * @param srv   (Optional) Name or IP of the server to connect to.  If NULL, server from the CacServerHandle will be used.
 *
 * @return CAC_FAILURE if the operation could not be completed successfully (hnd->status will also be set with a NTSTATUS code)
 * @return CAC_SUCCESS if the operation succeeded
 */            
int cac_Connect(CacServerHandle *hnd, const char *srv);


/**
 * Cleans up any data used by the CacServerHandle. If the libsmbclient context was set using cac_SetSmbcContext(), it will not be free'd. 
 * @param hnd the CacServerHandle to destroy
 */
void cac_FreeHandle(CacServerHandle * hnd);

/**
 * Initializes a CacTime structure based on an NTTIME structure
 *  If the function fails, then the CacTime structure will be zero'd out
 */
void cac_InitCacTime(CacTime *cactime, NTTIME nttime);

/**
 * Called by cac_NewServerHandle() if allocate_fields = True. You can call this if you want to, allocates sizeof(fstring) char's for every char * field
 * @param hnd Uninitialized server handle
 * @return CAC_FAILURE Memory could not be allocated
 * @return CAC_SUCCESS Memory was allocated
 */
int cac_InitHandleMem(CacServerHandle *hnd);

/**
 * Default smbc_get_auth_data_fn for libmsrpc. This function is called when libmsrpc needs to get more information about the 
 * client (username/password, workgroup). 
 * This function provides simple prompts to the user to enter the information. This description his here so you know how to re-define this function.
 * @see cac_SetAuthDataFn()
 * @param pServer Name/IP of the server to connect to. 
 * @param pShare Share name to connect to
 * @param pWorkgroup libmsrpc passes in the workgroup/domain name from hnd->domain. It can be modified in the function.
 * @param maxLenWorkgroup The maximum length of a string pWogroup can hold.
 * @param pUsername libmsrpc passes in the username from hnd->username. It can be modified in the function.
 * @param maxLenUsername The maximum length of a string pUsername can hold.
 * @param pPassword libmsrpc pass in the password from hnd->password. It can be modified in the function.
 * @param maxLenPassword The maximum length  of a string pPassword can hold.
 */
void cac_GetAuthDataFn(const char * pServer,
                 const char * pShare,
                 char * pWorkgroup,
                 int maxLenWorkgroup,
                 char * pUsername,
                 int maxLenUsername,
                 char * pPassword,
                 int maxLenPassword);


/**@}*/

/*****************
 * LSA Functions *
 *****************/

/** @addtogroup LSA_Functions
 * @{
 */

struct LsaOpenPolicy {
   /**Inputs*/
   struct {
      /**Access Mask. Refer to Security Access Masks in include/rpc_secdes.h*/
      uint32 access;

      /**Use security quality of service? (True/False)*/
      BOOL security_qos;
   } in;

   /**Outputs*/
   struct {
      /**Handle to the open policy (needed for all other operations)*/
      POLICY_HND *pol;
   } out;
};

/** 
 * Opens a policy handle on a remote machine.
 * @param hnd fully initialized CacServerHandle for remote machine
 * @param mem_ctx Talloc context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE if the policy could not be opened. hnd->status set with appropriate NTSTATUS
 * @return CAC_SUCCESS if the policy could be opened, the policy handle can be found
 */
int cac_LsaOpenPolicy(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaOpenPolicy *op);


/** 
 * Closes an  LSA policy handle (Retrieved using cac_LsaOpenPolicy).
 *   If successful, the handle will be closed on the server, and memory for pol will be freed
 * @param hnd - An initialized and connected server handle
 * @param mem_ctx Talloc context for memory allocation
 * @param pol - the policy handle to close
 * @return CAC_FAILURE could not close the policy handle, hnd->status is set to the appropriate NTSTATUS error code
 * @return CAC_SUCCESS the policy handle was closed 
 */
int cac_LsaClosePolicy(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *pol);


struct LsaGetNamesFromSids {
   struct {
      /**handle to and open LSA policy*/
      POLICY_HND *pol;
      
      /**the number of SIDs to lookup*/
      uint32 num_sids;
      
      /**array of SIDs to lookup*/
      DOM_SID *sids;
   } in;

   struct {
      /**The number of names returned (in case of CAC_PARTIAL_SUCCESS)*/
      uint32 num_found;

      /**array of SID info each index is one sid */
      CacSidInfo *sids;

      /**in case of partial success, an array of SIDs that could not be looked up (NULL if all sids were looked up)*/
      DOM_SID *unknown;
   } out;
};

/** 
 * Looks up the names for a list of SIDS
 * @param hnd initialized and connected server handle
 * @param mem_ctx Talloc context for memory allocation
 * @param op  input and output parameters 
 * @return CAC_FAILURE none of the SIDs could be looked up hnd->status is set with appropriate NTSTATUS error code
 * @return CAC_SUCCESS all of the SIDs were translated and a list of names has been output
 * @return CAC_PARTIAL_SUCCESS not all of the SIDs were translated, as a result the number of returned names is less than the original list of SIDs
 */
int cac_LsaGetNamesFromSids(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaGetNamesFromSids *op);

struct LsaGetSidsFromNames {
   struct {
      /**handle to an open LSA policy*/
      POLICY_HND *pol;

      /**number of SIDs to lookup*/
      uint32 num_names;

      /**array of strings listing the names*/
      char **names;
   } in;

   struct {
      /**The number of SIDs returned (in case of partial success*/
      uint32 num_found;

      /**array of SID info for the looked up names*/
      CacSidInfo *sids;

      /**in case of partial success, the names that were not looked up*/
      char **unknown;
   } out;
};

/** 
 * Looks up the SIDs for a list of names
 * @param hnd initialized and connected server handle
 * @param mem_ctx Talloc context for memory allocation
 * @param op  input and output parameters 
 * @return CAC_FAILURE none of the SIDs could be looked up hnd->status is set with appropriate NTSTATUS error code
 * @return CAC_SUCCESS all of the SIDs were translated and a list of names has been output
 * @return CAC_PARTIAL_SUCCESS not all of the SIDs were translated, as a result the number of returned names is less than the original list of SIDs
 */
int cac_LsaGetSidsFromNames(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaGetSidsFromNames *op);

struct LsaFetchSid {
   struct {
      /**handle to an open LSA policy*/
      POLICY_HND *pol;

      /**can be CAC_LOCAL_INFO, CAC_DOMAIN_INFO, or (CAC_LOCAL_INFO | CAC_DOMAIN_INFO)*/
      uint16 info_class;
   } in;

   struct {
      /**the machine's local SID and domain name (NULL if not asked for)*/
      CacSidInfo *local_sid;

      /**the machine's domain SID and name (NULL if not asked for)*/
      CacSidInfo *domain_sid;
      
   } out;
};

/** 
 * Looks up the domain or local sid of a machine with an open LSA policy handle
 * @param hnd initialized and connected server handle
 * @param mem_ctx Talloc context for memory allocation
 * @param op input and output parameters
 * @return CAC_FAILURE if the SID could not be fetched
 * @return CAC_SUCCESS if the SID was fetched
 * @return CAC_PARTIAL_SUCCESS if you asked for both local and domain sids but only one was returned
 */
int cac_LsaFetchSid(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaFetchSid *op);

struct LsaQueryInfoPolicy {
   struct {
      /**Open LSA policy handle on remote server*/
      POLICY_HND *pol;
   } in;

   struct {
      /**remote server's domain name*/
      char *domain_name;

      /**remote server's dns name*/
      char *dns_name;

      /**remote server's forest name*/
      char *forest_name;

      /**remote server's domain guid*/
      struct uuid *domain_guid;

      /**remote server's domain SID*/
      DOM_SID *domain_sid;
   } out;
};

/** 
 * Retrieves information about the LSA machine/domain
 * @param hnd initialized and connected server handle
 * @param mem_ctx Talloc context for memory allocation
 * @param op input and output parameters
 *           Note: for pre-Windows 2000 machines, only op->out.SID and op->out.domain will be set. @see cac_LsaFetchSid
 * @return - CAC_FAILURE if the operation was not successful. hnd->status will be set with an accurate NT_STATUS code
 * @return CAC_SUCCESS the operation was successful.
 */
int cac_LsaQueryInfoPolicy(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaQueryInfoPolicy *op);

struct LsaEnumSids {
   struct {
      /**Open LSA Policy handle*/
      POLICY_HND *pol;

      /**The prefered maximum number of SIDs returned per call*/
      uint32 pref_max_sids;
   } in;

   struct {
      /**used to keep track of how many sids have been retrieved over multiple calls
       *  should be set to zero via ZERO_STRUCT() befrore the first call. Use the same struct LsaEnumSids for multiple calls*/
      uint32 resume_idx;

      /**The number of sids returned this call*/
      uint32 num_sids;

      /**Array of sids returned*/
      DOM_SID *sids;

   } out;
};

/** 
 * Enumerates the SIDs in the LSA.  Can be enumerated in blocks by calling the function multiple times.
 *  Example: while(cac_LsaEnumSids(hnd, mem_ctx, op) { ... }
 * @param hnd - An initialized and connected server handle
 * @param mem_ctx Talloc context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE there was an error during operations OR there are no more results
 * @return CAC_SUCCESS the operation completed and results were returned
 */
int cac_LsaEnumSids(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaEnumSids *op);

struct LsaEnumAccountRights {
   struct {
      /**Open LSA Policy handle*/
      POLICY_HND *pol;

      /**(Optional) SID of the account - must supply either sid or name*/
      DOM_SID *sid;

      /**(Optional) name of the account - must supply either sid or name*/
      char *name;
   } in;

   struct {
      /**Count of rights for this account*/
      uint32 num_privs;

      /**array of privilege names*/
      char **priv_names;
   } out;
};

/** 
 * Enumerates rights assigned to a given account. Takes a SID instead of account handle as input
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters
 * @return CAC_FAILURE the rights could not be retrieved. hnd->status is set with NT_STATUS code
 * @return CAC_SUCCESS the operation was successful. 
 */

int cac_LsaEnumAccountRights(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaEnumAccountRights *op);

struct LsaEnumTrustedDomains {
   struct {
      /**Open LSA policy handle*/
      POLICY_HND *pol;
   } in;

   struct {
      /**used to keep track of how many domains have been retrieved over multiple calls
       *  should be set to zero via ZERO_STRUCT() before the first call. Use the same struct LsaEnumSids for multiple calls*/
      uint32 resume_idx;
      
      /**The number of domains returned by the remote server this call*/
      uint32 num_domains;

      /**array of trusted domain names returned by the remote server*/
      char **domain_names;

      /**array of trusted domain sids returned by the remote server*/
      DOM_SID *domain_sids;
   } out;
};
     
/** 
 * Enumerates the trusted domains in the LSA.  
 * @param hnd - An initialized and connected server handle
 * @param mem_ctx Talloc context for memory allocation
 * @param op - initialized parameters
 * @return CAC_FAILURE there was an error during operations OR there are no more results
 * @return CAC_SUCCESS the operation completed and results were returned
 */
int cac_LsaEnumTrustedDomains(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaEnumTrustedDomains *op);

struct LsaOpenTrustedDomain {
   struct {
      /**an open LSA policy handle*/
      POLICY_HND *pol;

      /**SID of the trusted domain to open*/
      DOM_SID *domain_sid;

      /**Desired access on the open domain*/
      uint32 access;
   } in;

   struct {
      /**A handle to the policy that is opened*/
      POLICY_HND *domain_pol;
   } out;
};

/** 
 * Opens a trusted domain by SID.
 * @param hnd An initialized and connected server handle
 * @param mem_ctx Talloc context for memory allocation
 * @param op initialized I/O parameters
 * @return CAC_FAILURE a handle to the domain could not be opened. hnd->status is set with approriate NT_STATUS code
 * @return CAC_SUCCESS the domain was opened successfully
 */
int cac_LsaOpenTrustedDomain(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaOpenTrustedDomain *op);

struct LsaQueryTrustedDomainInfo {
   struct {
      /**Open LSA policy handle*/
      POLICY_HND *pol;

      /**Info class of returned data*/
      uint16 info_class;

      /**(Optional)SID of trusted domain to query (must specify either SID or name of trusted domain)*/
      DOM_SID *domain_sid;

      /**(Optional)Name of trusted domain to query (must specify either SID or name of trusted domain)*/
      char *domain_name;
   } in;

   struct {
      /**information about the trusted domain*/
      LSA_TRUSTED_DOMAIN_INFO *info;
   } out;
};

/** 
 * Retrieves information a trusted domain.
 * @param hnd An initialized and connected server handle
 * @param mem_ctx Talloc context for memory allocation
 * @param op initialized I/O parameters
 * @return CAC_FAILURE a handle to the domain could not be opened. hnd->status is set with approriate NT_STATUS code
 * @return CAC_SUCCESS the domain was opened successfully
 */

int cac_LsaQueryTrustedDomainInfo(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaQueryTrustedDomainInfo *op);

struct LsaEnumPrivileges {
   struct {
      /**An open LSA policy handle*/
      POLICY_HND *pol;

      /**The _preferred_ maxinum number of privileges returned per call*/
      uint32 pref_max_privs;
   } in;

   struct {
      /**Used to keep track of how many privileges have been retrieved over multiple calls. Do not modify this value between calls*/
      uint32 resume_idx;

      /**The number of privileges returned this call*/
      uint32 num_privs;

      /**Array of privilege names*/
      char **priv_names;

      /**Array of high bits for privilege LUID*/
      uint32 *high_bits;

      /**Array of low bits for privilege LUID*/
      uint32 *low_bits;
   } out; 
};

/** 
 * Enumerates the Privileges supported by the LSA.  Can be enumerated in blocks by calling the function multiple times.
 *  Example: while(cac_LsaEnumPrivileges(hnd, mem_ctx, op) { ... }
 * @param hnd An initialized and connected server handle
 * @param mem_ctx Talloc context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE there was an error during operations OR there are no more results
 * @return CAC_SUCCESS the operation completed and results were returned
 * @see CAC_OP_FAILED()
 */
int cac_LsaEnumPrivileges(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaEnumPrivileges *op);

struct LsaOpenAccount {
   struct {
      /**An open LSA policy handle*/
      POLICY_HND *pol;

      /**(Optional) account SID - must supply either sid or name*/
      DOM_SID *sid;

      /**(Optional) account name - must supply either sid or name*/
      char *name;

      /**desired access for the handle*/
      uint32 access;
   } in;

   struct {
      /**A handle to the opened user*/
      POLICY_HND *user;
   } out;
};

/** 
 * Opens a handle to an account in the LSA
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters
 * @return CAC_FAILURE the account could not be opened. hnd->status has appropriate NT_STATUS code
 * @return CAC_SUCCESS the account was opened
 */
int cac_LsaOpenAccount(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaOpenAccount *op);

struct LsaAddPrivileges {
   struct {
      /**An open LSA policy handle*/
      POLICY_HND *pol;

      /**(Optional) The user's SID (must specify at least sid or name)*/
      DOM_SID *sid;

      /**(Optional) The user's name (must specify at least sid or name)*/
      char *name;

      /**The privilege names of the privileges to add for the account*/
      char **priv_names;
      
      /**The number of privileges in the priv_names array*/
      uint32 num_privs;

   } in;
};

/** 
 * Adds Privileges an account.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters
 * @return CAC_FAILURE the privileges could not be set. hnd->status has appropriate NT_STATUS code
 * @return CAC_SUCCESS the privileges were set.
 */
int cac_LsaAddPrivileges(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaAddPrivileges *op);

struct LsaRemovePrivileges {
   struct {
      /**An open handle to the LSA*/
      POLICY_HND *pol;

      /**(Optional) The account SID (must specify at least sid or name)*/
      DOM_SID *sid;

      /**(Optional) The account name (must specify at least sid or name)*/
      char *name;

      /**The privilege names of the privileges to remove from the account*/
      char **priv_names;

      /**The number of privileges in the priv_names array*/
      uint32 num_privs;

   } in;

};

/** 
 * Removes a _specific_ set of privileges from an account
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters
 * @return CAC_FAILURE the privileges could not be removed. hnd->status is set with NT_STATUS code
 * @return CAC_SUCCESS the privileges were removed 
 */
int cac_LsaRemovePrivileges(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaRemovePrivileges *op);

struct LsaClearPrivileges {
   struct {
      /**An open handle to the LSA*/
      POLICY_HND *pol;

      /**(Optional) The user's SID (must specify at least sid or name)*/
      DOM_SID *sid;

      /**(Optional) The user's name (must specify at least sid or name)*/
      char *name;
   } in;

};

/** 
 * Removes ALL privileges from an account
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters
 * @return CAC_FAILURE the operation was not successful, hnd->status set with NT_STATUS code
 * @return CAC_SUCCESS the opeartion was successful.
 */
int cac_LsaClearPrivileges(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaClearPrivileges *op);

/** 
 * Sets an accounts priviliges. Removes all privileges and then adds specified privileges.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters 
 * @return CAC_FAILURE The operation could not complete successfully
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_LsaSetPrivileges(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaAddPrivileges *op);

struct LsaGetSecurityObject {
   struct {
      /**Open LSA policy handle*/
      POLICY_HND *pol;
   } in;

   struct {
      /**Returned security descriptor information*/
      SEC_DESC_BUF *sec;
   } out;
};

/**
 * Retrieves Security Descriptor information about the LSA
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters 
 * @return CAC_FAILURE The operation could not complete successfully
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_LsaGetSecurityObject(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct LsaGetSecurityObject *op);


/**@}*/ /*LSA_Functions*/
      
/**********************
 * Registry Functions *
 *********************/

/**@addtogroup Reg_Functions
 * @{
 */

struct RegConnect {
   struct {
      /** must be one of : 
       *    HKEY_CLASSES_ROOT, 
       *    HKEY_LOCAL_MACHINE, 
       *    HKEY_USERS, 
       *    HKEY_PERFORMANCE_DATA,
       */
      int root;

      /**desired access on the root key
       * combination of: 
       * REG_KEY_READ,
       * REG_KEY_WRITE,
       * REG_KEY_EXECUTE,
       * REG_KEY_ALL,
       * found in include/rpc_secdes.h*/
      uint32 access;
   } in;

   struct {
      POLICY_HND *key;
   } out;
};

/** 
 * Opens a handle to the registry on the server
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters 
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_RegConnect(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegConnect *op);

/**
 * Closes an open registry handle
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param key The Key/Handle to close 
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_RegClose(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *key);

struct RegOpenKey {
   struct {
      /**(Optional)parent key. 
       * If this is NULL, then cac_RegOpenKey() will attempt to connect to the registry, name MUST start with something like:<br>
       *  HKEY_LOCAL_MACHINE\  or an abbreviation like HKCR\
       *
       *  supported root names:
       *   - HKEY_LOCAL_MACHINE\ or HKLM\
       *   - HKEY_CLASSES_ROOT\ or HKCR\
       *   - HKEY_USERS\ or HKU\
       *   - HKEY_PERFORMANCE_DATA or HKPD\
       */
      POLICY_HND *parent_key;

      /**name/path of key*/
      char *name;

      /**desired access on this key*/
      uint32 access;
   } in;

   struct {
      POLICY_HND *key;
   } out;
};
      
/**
 * Opens a registry key
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */

int cac_RegOpenKey(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegOpenKey *op);

struct RegEnumKeys {
   struct {
      /**enumerate subkeys of this key*/
      POLICY_HND *key;

      /**maximum number of keys to enumerate each call*/
      uint32 max_keys;
   } in;

   struct {
      /**keeps track of the index to resume enumerating*/
      uint32 resume_idx;

      /**the number of keys returned this call*/
      uint32 num_keys;

      /**array of key names*/
      char **key_names;

      /**class names of the keys*/
      char **class_names;

      /**last modification time of the key*/
      time_t *mod_times;
   } out;
};

/**
 * Enumerates Subkeys of a given key. Can be run in a loop. Example: while(cac_RegEnumKeys(hnd, mem_ctx, op)) { ... }
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters
 * @see CAC_OP_FAILED()
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_RegEnumKeys(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegEnumKeys *op);


struct RegCreateKey {
   struct {
      /**create a subkey of parent_key*/
      POLICY_HND *parent_key;

      /**name of the key to create*/
      char *key_name;

      /**class of the key*/
      char *class_name;

      /**Access mask to open the key with. See REG_KEY_* in include/rpc_secdes.h*/
      uint32 access;
   } in;

   struct {
      /**Open handle to the key*/
      POLICY_HND *key;
   } out;
};

/**
 * Creates a registry key, if the key already exists, it will be opened __Creating keys is not currently working__.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parmeters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_RegCreateKey(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegCreateKey *op);

struct RegDeleteKey {
   struct {
      /**handle to open registry key*/
      POLICY_HND *parent_key;

      /**name of the key to delete*/
      char *name;

      /**delete recursively. WARNING: this might not always work as planned*/
      BOOL recursive;
   } in;

};

/**
 * Deletes a subkey of an open key. Note: if you run this with op->in.recursive == True, and the operation fails, it may leave the key in an inconsistent state.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */

int cac_RegDeleteKey(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegDeleteKey *op);

struct RegDeleteValue {
   struct {
      /**handle to open registry key*/
      POLICY_HND *parent_key;

      /**name of the value to delete*/
      char *name;
   } in;
};

/**
 * Deletes a registry value.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_RegDeleteValue(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegDeleteValue *op);

struct RegQueryKeyInfo {
   struct {
      /**Open handle to the key to query*/
      POLICY_HND *key;
   } in;

   struct {
      /**name of the key class*/
      char *class_name;

      /**number of subkeys of the key*/
      uint32 num_subkeys;

      /**length (in characters) of the longest subkey name*/
      uint32 longest_subkey;

      /**length (in characters) of the longest class name*/
      uint32 longest_class;

      /**number of values in this key*/
      uint32 num_values;

      /**length (in characters) of the longest value name*/
      uint32 longest_value_name;

      /**length (in bytes) of the biggest value data*/
      uint32 longest_value_data;

      /**size (in bytes) of the security descriptor*/
      uint32 security_desc_size;

      /**time of the last write*/
      time_t last_write_time;
   } out;
};

/**
 * Retrieves information about an open key
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */

int cac_RegQueryKeyInfo(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegQueryKeyInfo *op);

struct RegSaveKey {
   struct {
      /**Open key to be saved*/
      POLICY_HND *key;

      /**The path (on the remote computer) to save the file to*/
      char *filename;
   } in;
};

/**
 * Saves a key to a file on the remote machine __Not currently working__.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */

int cac_RegSaveKey(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegSaveKey *op);

struct RegQueryValue {
   struct {
      /**handle to open registry key*/
      POLICY_HND *key;

      /**name of the value to query*/
      char *val_name;
   } in;

   struct {
      /**Value type.
       * One of:
       *  - REG_DWORD (equivalent to REG_DWORD_LE)
       *  - REG_DWORD_BE
       *  - REG_SZ
       *  - REG_EXPAND_SZ
       *  - REG_MULTI_SZ
       *  - REG_BINARY
       */
      uint32 type;

      /**The value*/
      REG_VALUE_DATA *data;
   } out;
};

/**
 * Retrieves a value (type and data) _not currently working_.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */

int cac_RegQueryValue(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegQueryValue *op);

struct RegEnumValues {
   struct {
      /**handle to open key*/
      POLICY_HND *key;

      /**max number of values returned per call*/
      uint32 max_values;

   } in;

   struct {
      /**keeps track of the index to resume from - used over multiple calls*/
      uint32 resume_idx;

      /**the number of values that were returned this call*/
      uint32 num_values;

      /**Array of value types. A type can be one of:
       *  - REG_DWORD (equivalent to REG_DWORD_LE)
       *  - REG_DWORD_BE
       *  - REG_SZ
       *  - REG_EXPAND_SZ
       *  - REG_MULTI_SZ
       *  - REG_BINARY
       */
      uint32 *types;

      /**array of strings storing the names of the values*/
      char **value_names;

      /**array of pointers to the value data returned*/
      REG_VALUE_DATA **values;
   } out;
};

/**
 * Enumerates a number of Registry values in an open registry key.
 * Can be run in a loop. Example: while(cac_RegEnumValues(hnd, mem_ctx, op)) { ... }
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters
 * @see CAC_OP_FAILED()
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_RegEnumValues(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegEnumValues *op);

struct RegSetValue {
   struct {
      /**Handle to open registry key*/
      POLICY_HND *key;

      /**Name of the value*/
      char *val_name;

      /**Value type.
       * One of:
       *  - REG_DWORD (equivalent to REG_DWORD_LE)
       *  - REG_DWORD_BE
       *  - REG_SZ
       *  - REG_EXPAND_SZ
       *  - REG_MULTI_SZ
       *  - REG_BINARY
       */
      uint32 type;

      /**the value*/
      REG_VALUE_DATA value;
   } in;
};

/**
 * Sets or creates value (type and data).
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_RegSetValue(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegSetValue *op);

struct RegGetVersion {
   struct {
      /**open registry key*/
      POLICY_HND *key;
   } in;

   struct {
      /**version number*/
      uint32 version;
   } out;
};

/**
 * Retrieves the registry version number
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_RegGetVersion(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegGetVersion *op);

struct RegGetKeySecurity {
   struct {
      /**Handle to key to query*/
      POLICY_HND *key;

      /**Info that you want. Should be a combination of (1 or more or'd):
       * - OWNER_SECURITY_INFORMATION
       * - GROUP_SECURITY_INFORMATION
       * - DACL_SECURITY_INFORMATION
       * - SACL_SECURITY_INFORMATION
       * - UNPROTECTED_SACL_SECURITY_INFORMATION
       * - UNPROTECTED_DACL_SECURITY_INFORMATION
       * - PROTECTED_SACL_SECURITY_INFORMATION
       * - PROTECTED_DACL_SECURITY_INFORMATION
       *
       * or use:
       * - ALL_SECURITY_INFORMATION
       *
       * all definitions from include/rpc_secdes.h
       */
      uint32 info_type;
   } in;

   struct {
      /**size of the data returned*/
      uint32 size;

      /**Security descriptor*/
      SEC_DESC *descriptor;
   } out;
};

/**
 * Retrieves a key security descriptor.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */

int cac_RegGetKeySecurity(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegGetKeySecurity *op);

struct RegSetKeySecurity {
   struct {
      /**Handle to key to query*/
      POLICY_HND *key;

      /**Info that you want. Should be a combination of (1 or more or'd):
       * - OWNER_SECURITY_INFORMATION
       * - GROUP_SECURITY_INFORMATION
       * - DACL_SECURITY_INFORMATION
       * - SACL_SECURITY_INFORMATION
       * - UNPROTECTED_SACL_SECURITY_INFORMATION
       * - UNPROTECTED_DACL_SECURITY_INFORMATION
       * - PROTECTED_SACL_SECURITY_INFORMATION
       * - PROTECTED_DACL_SECURITY_INFORMATION
       *
       * or use:
       * - ALL_SECURITY_INFORMATION
       *
       * all definitions from include/rpc_secdes.h
       */
      uint32 info_type;
      
      /**size of the descriptor*/
      size_t size;

      /**Security descriptor*/
      SEC_DESC *descriptor;
   } in;
};

/**
 * Sets the key security descriptor.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_RegSetKeySecurity(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct RegSetKeySecurity *op);

/**@}*/ /*Reg_Functions*/

struct Shutdown {
   struct {
      /**the message to display (can be NULL)*/
      char *message;

      /**timeout in seconds*/
      uint32 timeout;

      /**False = shutdown, True = reboot*/
      BOOL reboot;
      
      /**force the*/
      BOOL force;

      /*FIXME: make this useful*/
      uint32 reason;
   } in;
};


/**
 * Shutdown the server _not currently working_. 
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_Shutdown(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct Shutdown *op);

/**
 * Attempt to abort initiated shutdown on the server _not currently working_. 
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_AbortShutdown(CacServerHandle *hnd, TALLOC_CTX *mem_ctx);

/*****************
 * SAM Functions *
 *****************/

/**@addtogroup SAM_Functions
 * @{
 */
struct SamConnect {
   struct {
      /**Access mask to open with
       * see generic access masks in include/smb.h*/
      uint32 access;
   } in;

   struct {
      POLICY_HND *sam;
   } out;
};

/** 
 * Connects to the SAM. This can be skipped by just calling cac_SamOpenDomain()
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */

int cac_SamConnect(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamConnect *op);


/** 
 * Closes any (SAM, domain, user, group, etc.) SAM handle. 
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param sam Handle to close
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */

int cac_SamClose(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *sam);

struct SamOpenDomain {
   struct {
      /**The desired access. See generic access masks - include/smb.h*/
      uint32 access;

      /**(Optional) An open handle to the SAM. If it is NULL, the function will connect to the SAM with the access mask above*/
      POLICY_HND *sam;

      /**(Optional) The SID of the domain to open. 
       *  If this this is NULL, the function will attempt to open the domain specified in hnd->domain */
      DOM_SID *sid;
   } in;

   struct {
      /**handle to the open domain*/
      POLICY_HND *dom_hnd;

      /**Handle to the open SAM*/
      POLICY_HND *sam;
   } out;
};

/** 
 * Opens a handle to a domain. This must be called before any other SAM functions 
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamOpenDomain(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamOpenDomain *op);

struct SamCreateUser {
   struct {
      /**Open domain handle*/
      POLICY_HND *dom_hnd;

      /**Username*/
      char *name;

      /**See Allowable account control bits in include/smb.h*/
      uint32 acb_mask;
   } in;

   struct {
      /**handle to the user*/
      POLICY_HND *user_hnd;

      /**rid of the user*/
      uint32 rid;
   } out;
};

/** 
 * Creates a new domain user, if the account already exists it will _not_ be opened and hnd->status will be NT_STATUS_USER_EXISTS
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */

int cac_SamCreateUser(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamCreateUser *op);

struct SamOpenUser {
   struct {
      /**Handle to open SAM connection*/
      POLICY_HND *dom_hnd;

      /**desired access - see generic access masks in include/smb.h*/
      uint32 access;

      /**RID of the user*/
      uint32 rid;

      /**(Optional) name of the user - must supply either RID or user name*/
      char *name;
   } in;

   struct {
      /**Handle to the user*/
      POLICY_HND *user_hnd;
   } out;    
};

/** 
 * Opens a domain user.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamOpenUser(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamOpenUser *op);

/** 
 * Deletes a domain user.  
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param user_hnd Open handle to the user
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamDeleteUser(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *user_hnd);


struct SamEnumUsers {
   struct {
      /**Open handle to a domain*/
      POLICY_HND *dom_hnd;

      /**Enumerate users with specific ACB. If 0, all users will be enumerated*/
      uint32 acb_mask;
   } in;

   struct {
      /**where to resume from. Used over multiple calls*/
      uint32 resume_idx;

      /**the number of users returned this call*/
      uint32 num_users;

      /**Array storing the rids of the returned users*/
      uint32 *rids;

      /**Array storing the names of all the users returned*/
      char **names;

      BOOL done;
   } out;
};

/** 
 * Enumerates domain users. Can be used as a loop condition. Example: while(cac_SamEnumUsers(hnd, mem_ctx, op)) { ... }
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamEnumUsers(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamEnumUsers *op);

struct SamGetNamesFromRids {
   struct {
      /**An open handle to the domain SAM from cac_SamOpenDomain()*/
      POLICY_HND *dom_hnd;

      /**Number of RIDs to resolve*/
      uint32 num_rids;

      /**Array of RIDs to resolve*/
      uint32 *rids;
   } in;

   struct {
      /**the number of names returned - if this is 0, the map is NULL*/
      uint32 num_names;

      /**array contiaing the Names and RIDs*/
      CacLookupRidsRecord *map;
   } out;
};

/** 
 * Returns a list of names which map to a list of RIDs.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamGetNamesFromRids(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetNamesFromRids *op);

struct SamGetRidsFromNames {
   struct {
      /**An open handle to the domain SAM from cac_SamOpenDomain()*/
      POLICY_HND *dom_hnd;

      /**Number of names to resolve*/
      uint32 num_names;

      /**Array of names to resolve*/
      char **names;
   } in;

   struct {
      /**the number of names returned - if this is 0, then map is NULL*/
      uint32 num_rids;

      /**array contiaing the Names and RIDs*/
      CacLookupRidsRecord *map;
   } out;
};

/** 
 * Returns a list of RIDs which map to a list of names.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamGetRidsFromNames(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetRidsFromNames *op);

struct SamGetGroupsForUser {
   struct {
      /**An open handle to the user*/
      POLICY_HND *user_hnd;
   } in;

   struct {
      /**The number of groups the user is a member of*/
      uint32 num_groups;

      /**The RIDs of the groups*/
      uint32 *rids;

      /**The attributes of the groups*/ 
      uint32 *attributes;
   } out;
};
/** 
 * Retrieves a list of groups that a user is a member of.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamGetGroupsForUser(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetGroupsForUser *op);

struct SamOpenGroup {
   struct {
      /**Open handle to the domain SAM*/
      POLICY_HND *dom_hnd;

      /**Desired access to open the group with. See Generic access masks in include/smb.h*/
      uint32 access;

      /**rid of the group*/
      uint32 rid;
   } in;

   struct {
      /**Handle to the group*/
      POLICY_HND *group_hnd;
   } out;
};

/** 
 * Opens a domain group.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamOpenGroup(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamOpenGroup *op);

struct SamCreateGroup {
   struct {
      /**Open handle to the domain SAM*/
      POLICY_HND *dom_hnd;

      /**Desired access to open the group with. See Generic access masks in include/smb.h*/
      uint32 access;

      /**The name of the group*/
      char *name;
   } in;

   struct {
      /**Handle to the group*/
      POLICY_HND *group_hnd;
   } out;
};

/** 
 * Creates a group. If the group already exists it will not be opened.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamCreateGroup(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamCreateGroup *op);

/** 
 * Deletes a domain group.  
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param group_hnd Open handle to the group.
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamDeleteGroup(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *group_hnd);

struct SamGetGroupMembers {
   struct {
      /**Open handle to a group*/
      POLICY_HND *group_hnd;
   } in;

   struct {
      /**The number of members in the group*/
      uint32 num_members;

      /**An array storing the RIDs of the users*/
      uint32 *rids;

      /**The attributes*/
      uint32 *attributes;
   } out;
};

/** 
 * Retrives a list of users in a group.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamGetGroupMembers(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetGroupMembers *op);

struct SamAddGroupMember {
   struct {
      /**Open handle to a group*/
      POLICY_HND *group_hnd;

      /**RID of new member*/
      uint32 rid;
   } in;
};

/** 
 * Adds a user to a group.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamAddGroupMember(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamAddGroupMember *op);

struct SamRemoveGroupMember {
   struct {
      /**Open handle to a group*/
      POLICY_HND *group_hnd;

      /**RID of member to remove*/
      uint32 rid;
   } in;
};

/** 
 * Removes a user from a group.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamRemoveGroupMember(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamRemoveGroupMember *op);

/**
 * Removes all the members of a group - warning: if this function fails is is possible that some but not all members were removed
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param group_hnd Open handle to the group to clear
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamClearGroupMembers(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *group_hnd);

struct SamSetGroupMembers {
   struct {
      /**Open handle to the group*/
      POLICY_HND *group_hnd;

      /**Number of members in the group - if this is 0, all members of the group will be removed*/
      uint32 num_members;

      /**The RIDs of the users to add*/
      uint32 *rids;
   } in;
};

/**
 * Clears the members of a group and adds a list of members to the group
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamSetGroupMembers(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamSetGroupMembers *op);

struct SamEnumGroups {
   struct {
      /**Open handle to a domain*/
      POLICY_HND *dom_hnd;
   } in;

   struct {
      /**Where to resume from _do not_ modify this value. Used over multiple calls.*/
      uint32 resume_idx;

      /**the number of users returned this call*/
      uint32 num_groups;

      /**Array storing the rids of the returned groups*/
      uint32 *rids;

      /**Array storing the names of all the groups returned*/
      char **names;

      /**Array storing the descriptions of all the groups returned*/
      char **descriptions;

      BOOL done;
   } out;
};

/** 
 * Enumerates domain groups. Can be used as a loop condition. Example: while(cac_SamEnumGroups(hnd, mem_ctx, op)) { ... }
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamEnumGroups(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamEnumGroups *op);

struct SamEnumAliases {
   struct {
      /**Open handle to a domain*/
      POLICY_HND *dom_hnd;
   } in;

   struct {
      /**where to resume from. Used over multiple calls*/
      uint32 resume_idx;

      /**the number of users returned this call*/
      uint32 num_aliases;

      /**Array storing the rids of the returned groups*/
      uint32 *rids;

      /**Array storing the names of all the groups returned*/
      char **names;

      /**Array storing the descriptions of all the groups returned*/
      char **descriptions;

      BOOL done;
   } out;
};

/** 
 * Enumerates domain aliases. Can be used as a loop condition. Example: while(cac_SamEnumAliases(hnd, mem_ctx, op)) { ... }
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamEnumAliases(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamEnumAliases *op);

struct SamCreateAlias {
   struct {
      /**Open handle to the domain SAM*/
      POLICY_HND *dom_hnd;

      /**The name of the alias*/
      char *name;
   } in;

   struct {
      /**Handle to the group*/
      POLICY_HND *alias_hnd;
   } out;
};

/** 
 * Creates an alias. If the alias already exists it will not be opened.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */

int cac_SamCreateAlias(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamCreateAlias *op);

struct SamOpenAlias {
   struct {
      /**Open handle to the domain SAM*/
      POLICY_HND *dom_hnd;

      /**Desired access to open the group with. See Generic access masks in include/smb.h*/
      uint32 access;

      /**rid of the alias*/
      uint32 rid;
   } in;

   struct {
      /**Handle to the alias*/
      POLICY_HND *alias_hnd;
   } out;
};

/** 
 * Opens a handle to an alias.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamOpenAlias(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamOpenAlias *op);

/** 
 * Deletes an alias.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param alias_hnd Open handle to the alias
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamDeleteAlias(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *alias_hnd);

struct SamAddAliasMember {
   struct {
      /**Open handle to a alias*/
      POLICY_HND *alias_hnd;

      /**SID of new member*/
      DOM_SID *sid;
   } in;
};

/** 
 * Adds an account to an alias.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamAddAliasMember(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamAddAliasMember *op);

struct SamRemoveAliasMember {
   struct {
      /**Open handle to the alias*/
      POLICY_HND *alias_hnd;

      /**The SID of the member*/
      DOM_SID *sid;
   } in;
};

/** 
 * Removes an account from an alias.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamRemoveAliasMember(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamRemoveAliasMember *op);

struct SamGetAliasMembers {
   struct {
      /**Open handle to the alias*/
      POLICY_HND *alias_hnd;
   } in;

   struct {
      /**The number of members*/
      uint32 num_members;

      /**An array storing the SIDs of the accounts*/
      DOM_SID *sids;
   } out;
};

/** 
 * Retrieves a list of all accounts in an alias.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamGetAliasMembers(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetAliasMembers *op);

/**
 * Removes all the members of an alias  - warning: if this function fails is is possible that some but not all members were removed
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param alias_hnd Handle to the alias to clear
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */

int cac_SamClearAliasMembers(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *alias_hnd);

struct SamSetAliasMembers {
   struct {
      /**Open handle to the group*/
      POLICY_HND *alias_hnd;

      /**Number of members in the group - if this is 0, all members of the group will be removed*/
      uint32 num_members;

      /**The SIDs of the accounts to add*/
      DOM_SID *sids;
   } in;
};

/**
 * Clears the members of an alias and adds a list of members to the alias
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamSetAliasMembers(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamSetAliasMembers *op);


struct SamUserChangePasswd {
   struct {
      /**The username*/
      char *username;

      /**The current password*/
      char *password;

      /**The new password*/
      char *new_password;
   } in;
};
/**Used by a user to change their password*/
int cac_SamUserChangePasswd(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamUserChangePasswd *op);

/**
 * Enables a user
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param user_hnd Open handle to the user to enable
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamEnableUser(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *user_hnd);

/**
 * Disables a user
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param user_hnd Open handle to the user to disables
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamDisableUser(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *user_hnd);

struct SamSetPassword {
   struct {
      /**Open handle to a user*/
      POLICY_HND *user_hnd;

      /**The new password*/
      char *password;
   } in;
};

/**
 * Sets a user's password
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */

int cac_SamSetPassword(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamSetPassword *op);

struct SamGetUserInfo {
   struct {
      /**Open Handle to a user*/
      POLICY_HND *user_hnd;
   } in;

   struct {
      CacUserInfo *info;
   } out;
};

/**
 * Retrieves user information using a CacUserInfo structure. If you would like to use a SAM_USERINFO_CTR directly, use cac_SamGetUserInfoCtr()
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @see cac_SamGetUserInfoCtr()
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamGetUserInfo(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetUserInfo *op);

struct SamSetUserInfo {
   struct {
      /**Open handle to a user*/
      POLICY_HND *user_hnd;

      /**Structure containing the data you would like to set*/
      CacUserInfo *info;
   } in;
};

/**
 * Sets the user info using a CacUserInfo structure. If you would like to use a SAM_USERINFO_CTR directly use cac_SamSetUserInfoCtr().
 * @note All fields in the CacUserInfo structure will be set. Best to call cac_GetUserInfo() modify fields that you want, and then call cac_SetUserInfo().
 * @note When calling this, you _must_ set the user's password.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @see cac_SamSetUserInfoCtr()
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamSetUserInfo(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamSetUserInfo *op);

struct SamGetUserInfoCtr {
   struct {
      /**Open handle to a user*/
      POLICY_HND *user_hnd;

      /**What USER_INFO structure you want. See include/rpc_samr.h*/
      uint16 info_class;
   } in;

   struct {
      /**returned user info*/
      SAM_USERINFO_CTR *ctr;
   } out;
};

/**
 * Retrieves user information using a SAM_USERINFO_CTR structure. If you don't want to use this structure, user SamGetUserInfo()
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @see cac_SamGetUserInfo()
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamGetUserInfoCtr(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetUserInfoCtr *op);

struct SamSetUserInfoCtr {
   struct {
      /**Open handle to a user*/
      POLICY_HND *user_hnd;

      /**user info - make sure ctr->switch_value is set properly*/
      SAM_USERINFO_CTR *ctr;
   } in;
};

/**
 * Sets the user info using a SAM_USERINFO_CTR structure. If you don't want to use this structure, use cac_SamSetUserInfo()
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @see cac_SamSetUserInfo()
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */

int cac_SamSetUserInfoCtr(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamSetUserInfoCtr *op);

struct SamRenameUser {
   struct {
      /**Open handle to user*/
      POLICY_HND *user_hnd;

      /**New user name*/
      char *new_name;
   } in;
};

/**
 * Changes the name of a user.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamRenameUser(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamRenameUser *op);

struct SamGetGroupInfo {
   struct {
      /**Open handle to a group*/
      POLICY_HND *group_hnd;
   } in;

   struct {
      /**Returned info about the group*/
      CacGroupInfo *info;
   } out;
};

/**
 * Retrieves information about a group.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamGetGroupInfo(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetGroupInfo *op);

struct SamSetGroupInfo {
   struct {
      /**Open handle to a group*/
      POLICY_HND *group_hnd;

      /**group info*/
      CacGroupInfo *info;
   } in;
};

/**
 * Sets information about a group.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamSetGroupInfo(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamSetGroupInfo *op);

struct SamRenameGroup {
   struct {
      /**Open handle to a group*/
      POLICY_HND *group_hnd;

      /**New name*/
      char *new_name;
   } in;
};

/**
 * Changes the name of a group
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */

int cac_SamRenameGroup(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamRenameGroup *op);

struct SamGetAliasInfo {
   struct {
      /**Open handle to an alias*/
      POLICY_HND *alias_hnd;
   } in;

   struct {
      /**Returned alias info*/
      CacAliasInfo *info;
   } out;
};

/**
 * Retrieves information about an alias.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamGetAliasInfo(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetAliasInfo *op);

struct SamSetAliasInfo {
   struct {
      /**Open handle to an alias*/
      POLICY_HND *alias_hnd;
      
      /**Returned alias info*/
      CacAliasInfo *info;
   } in;
};

/**
 * Sets information about an alias.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE The operation could not complete successfully. hnd->status is set with appropriate NTSTATUS code
 * @return CAC_SUCCESS The operation completed successfully
 */
int cac_SamSetAliasInfo(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamSetAliasInfo *op);

struct SamGetDomainInfo {
   struct {
      /**Open handle to the domain SAM*/
      POLICY_HND *dom_hnd;
   } in;

   struct {
      /**Returned domain info*/
      CacDomainInfo *info;
   } out;
};

/**
 * Gets domain information in the form of a CacDomainInfo structure. 
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @see SamGetDomainInfoCtr()
 * @return CAC_FAILURE - the operation was not successful hnd->status is set appropriately
 * @return CAC_SUCCESS - the operation was successful
 * @return CAC_PARTIAL_SUCCESS - This function makes 3 rpc calls, if one or two fail and the rest succeed, 
 *                                  not all fields in the CacDomainInfo structure will be filled
 */
int cac_SamGetDomainInfo(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetDomainInfo *op);

struct SamGetDomainInfoCtr {
   struct {
      /**Open handle to domain*/
      POLICY_HND *dom_hnd;

      /**What info level you want*/
      uint16 info_class;
   } in;

   struct {
      SAM_UNK_CTR *info;
   } out;
};

/**
 * Gets domain information in the form of a SAM_UNK_CTR structure. 
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @see SamGetDomainInfo()
 * @return CAC_FAILURE - the operation was not successful hnd->status is set appropriately
 * @return CAC_SUCCESS - the operation was successful
 */
int cac_SamGetDomainInfoCtr(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetDomainInfoCtr *op);

struct SamGetDisplayInfo {
   struct {
      /**Open handle to domain*/
      POLICY_HND *dom_hnd;

      /**What type of data*/
      uint16 info_class;

      /**(Optional)If 0, max_entries and max_size will be filled in by the function*/
      uint32 max_entries;
      
      /**(Optional)If 0, max_entries and max_size will be filled in by the function*/
      uint32 max_size;
   } in;

   struct {
      /**Do not modify this value, use the same value between multiple calls (ie in while loop)*/
      uint32 resume_idx;

      /**Number of entries returned*/
      uint32 num_entries;

      /**Returned display info*/
      SAM_DISPINFO_CTR ctr;

      /**Internal value. Do not modify.*/
      uint32 loop_count;

      BOOL done;
   } out;
};

/**
 * Gets dislpay information using a SAM_DISPINFO_CTR.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE - the operation was not successful hnd->status is set appropriately
 * @return CAC_SUCCESS - the operation was successful
 */
int cac_SamGetDisplayInfo(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetDisplayInfo *op);

struct SamLookupDomain {
   struct {
      /**Open handle to the sam (opened with cac_SamConnect() or cac_SamOpenDomain()*/
      POLICY_HND *sam;

      /**Name of the domain to lookup*/
      char *name;
   } in;

   struct {
      /**SID of the domain*/
      DOM_SID *sid;
   } out;
};

/**
 * Looks up a Domain SID given it's name.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE - the operation was not successful hnd->status is set appropriately
 * @return CAC_SUCCESS - the operation was successful
 */
int cac_SamLookupDomain(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamLookupDomain *op);

struct SamGetSecurityObject {
   struct {
      /**An open handle (SAM, domain or user)*/
      POLICY_HND *pol;
   } in;

   struct {
      SEC_DESC_BUF *sec;
   } out;
};

/**
 * Retrievies Security descriptor information for a SAM/Domain/user
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE - the operation was not successful hnd->status is set appropriately
 * @return CAC_SUCCESS - the operation was successful
 */
int cac_SamGetSecurityObject(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamGetSecurityObject *op);

struct SamFlush {
   struct {
      /**Open handle to the domain SAM*/
      POLICY_HND *dom_hnd;

      /**(Optional)Domain SID. If NULL, the domain in hnd->domain will be opened*/
      DOM_SID *sid;

      /**(Optional)Desired access to re-open the domain with. If 0, MAXIMUM_ALLOWED_ACCESS is used.*/
      uint32 access;
   } in;
};

/**
 * Closes the domain handle, then re-opens it - effectively flushing any changes made.
 * WARNING: if this fails you will no longer have an open handle to the domain SAM.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters
 * @return CAC_FAILURE - the operation was not successful hnd->status is set appropriately
 * @return CAC_SUCCESS - the operation was successful
 */
int cac_SamFlush(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SamFlush *op);

/**@}*/ /*SAM_Functions*/

/**@addtogroup SCM_Functions
 * @{
 */

struct SvcOpenScm {
   struct {
      /**Desired access to open the Handle with. See SC_RIGHT_MGR_* or SC_MANAGER_* in include/rpc_secdes.h*/
      uint32 access;
   } in;

   struct {
      /**Handle to the SCM*/
      POLICY_HND *scm_hnd;
   } out;
};

/**
 * Opens a handle to the SCM on the remote machine.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE - the operation was not successful hnd->status is set appropriately
 * @return CAC_SUCCESS - the operation was successful
 */
int cac_SvcOpenScm(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcOpenScm *op);

/**
 * Closes an Svc handle (SCM or Service)
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param scm_hnd The handle to close
 * @return CAC_FAILURE - the operation was not successful hnd->status is set appropriately
 * @return CAC_SUCCESS - the operation was successful
 */
int cac_SvcClose(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, POLICY_HND *scm_hnd);

struct SvcEnumServices {
   struct {
      /**Open handle to the SCM*/
      POLICY_HND *scm_hnd;

      /**(Optional)Type of service to enumerate. Possible values:
       *  - SVCCTL_TYPE_WIN32
       *  - SVCCTL_TYPE_DRIVER
       *  If this is 0, (SVCCTL_TYPE_DRIVER | SVCCTL_TYPE_WIN32) is assumed.
       */
      uint32 type;

      /**(Optional)State of service to enumerate. Possible values:
       *  - SVCCTL_STATE_ACTIVE
       *  - SVCCTL_STATE_INACTIVE
       *  - SVCCTL_STATE_ALL
       *  If this is 0, SVCCTL_STATE_ALL is assumed.
       */
      uint32 state;
   } in;
   
   struct {
      /**Number of services returned*/
      uint32 num_services;

      /**Array of service structures*/
      CacService *services;
   } out;
};

/**
 * Enumerates services on the remote machine.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized parameters
 * @return CAC_FAILURE - the operation was not successful hnd->status is set appropriately
 * @return CAC_SUCCESS - the operation was successful
 */
int cac_SvcEnumServices(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcEnumServices *op);

struct SvcOpenService {
   struct {
      /**Handle to the Service Control Manager*/
      POLICY_HND *scm_hnd;

      /**Access mask to open service with see SERVICE_* or SC_RIGHT_SVC_* in include/rpc_secdes.h*/
      uint32 access;

      /**The name of the service. _not_ the display name*/
      char *name;
   } in;

   struct {
      /**Handle to the open service*/
      POLICY_HND *svc_hnd;
   } out;
};

/**
 * Opens a handle to a service.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters
 * @return CAC_FAILURE - the operation was not successful hnd->status is set appropriately
 * @return CAC_SUCCESS - the operation was successful
 */

int cac_SvcOpenService(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcOpenService *op);

struct SvcGetStatus {
   struct {
      /**Open handle to the service to query*/
      POLICY_HND *svc_hnd;
   } in;

   struct {
      /**The status of the service. See include/rpc_svcctl.h for SERVICE_STATUS definition.*/
      SERVICE_STATUS status;
   } out;
};

/**
 * Retrieves the status of a service.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters
 * @return CAC_FAILURE - the operation was not successful hnd->status is set appropriately
 * @return CAC_SUCCESS - the operation was successful
 */
int cac_SvcGetStatus(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcGetStatus *op);

struct SvcStartService {
   struct {
      /**open handle to the service*/
      POLICY_HND *svc_hnd;

      /**Array of parameters to start the service with. Can be NULL if num_parms is 0*/
      char **parms;

      /**Number of parameters in the parms array*/
      uint32 num_parms;

      /**Number of seconds to wait for the service to actually start. If this is 0, then the status will not be checked after the initial call*/
      uint32 timeout;
   } in;
};

/**
 * Attempts to start a service.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters
 * @return CAC_FAILURE - the operation was not successful hnd->status is set appropriately
 * @return CAC_SUCCESS - the operation was successful
 */

int cac_SvcStartService(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcStartService *op);

struct SvcControlService {
   struct {
      /**Open handle to the service to control*/
      POLICY_HND *svc_hnd;

      /**The control operation to perform. Possible values (from include/rpc_svcctl.h):
       * - SVCCTL_CONTROL_STOP
       * - SVCCTL_CONTROL_PAUSE
       * - SVCCTL_CONTROL_CONTINUE
       * - SVCCTL_CONTROL_SHUTDOWN
       */
      uint32 control;
   } in;

   struct {
      /**The returned status of the service, _immediately_ after the call*/
      SERVICE_STATUS *status;
   } out;
};

/**
 * Performs a control operation on a service and _immediately_ returns.
 * @see cac_SvcStopService()
 * @see cac_SvcPauseService()
 * @see cac_SvcContinueService()
 * @see cac_SvcShutdownService()
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters
 * @return CAC_FAILURE - the operation was not successful hnd->status is set appropriately
 * @return CAC_SUCCESS - the operation was successful
 */
int cac_SvcControlService(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcControlService *op);

struct SvcStopService {
   struct {
      /**Open handle to the service*/
      POLICY_HND *svc_hnd;

      /**Number of seconds to wait for the service to actually start. 
       * If this is 0, then the status will not be checked after the initial call and CAC_SUCCESS might be returned if the status isn't actually started
       */
      uint32 timeout;
   } in;

   struct {
      /**Status of the service after the operation*/
      SERVICE_STATUS status;
   } out;
};

/**
 * Attempts to stop a service.
 * @see cacSvcControlService()
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters
 * @return CAC_FAILURE - the operation was not successful. If hnd->status is NT_STATUS_OK, then a timeout occured.
 * @return CAC_SUCCESS - the operation was successful
 */
int cac_SvcStopService(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcStopService *op);

struct SvcPauseService {
   struct {
      /**Open handle to the service*/
      POLICY_HND *svc_hnd;

      /**Number of seconds to wait for the service to actually start. 
       * If this is 0, then the status will not be checked after the initial call and CAC_SUCCESS might be returned if the status isn't actually started
       */
      uint32 timeout;
   } in;

   struct {
      /**Status of the service after the operation*/
      SERVICE_STATUS status;
   } out;
};

/**
 * Attempts to pause a service.
 * @see cacSvcControlService()
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters
 * @return CAC_FAILURE - the operation was not successful. If hnd->status is NT_STATUS_OK, then a timeout occured.
 * @return CAC_SUCCESS - the operation was successful
 */
int cac_SvcPauseService(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcPauseService *op);

struct SvcContinueService {
   struct {
      /**Open handle to the service*/
      POLICY_HND *svc_hnd;

      /**Number of seconds to wait for the service to actually start. 
       * If this is 0, then the status will not be checked after the initial call and CAC_SUCCESS might be returned if the status isn't actually started
       */
      uint32 timeout;
   } in;

   struct {
      /**Status of the service after the operation*/
      SERVICE_STATUS status;
   } out;
};

/**
 * Attempts to continue a paused service.
 * @see cacSvcControlService()
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters
 * @return CAC_FAILURE - the operation was not successful. If hnd->status is NT_STATUS_OK, then a timeout occured.
 * @return CAC_SUCCESS - the operation was successful
 */
int cac_SvcContinueService(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcContinueService *op);

struct SvcGetDisplayName {
   struct {
      /**Open handle to the service*/
      POLICY_HND *svc_hnd;
   } in;

   struct {
      /**The returned display name of the service*/
      char *display_name;
   } out;
};

/**
 * Retrieves the display name of a service _not currently working_
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters
 * @return CAC_FAILURE - the operation was not successful hnd->status is set appropriately
 * @return CAC_SUCCESS - the operation was successful
 */
int cac_SvcGetDisplayName(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcGetDisplayName *op);

struct SvcGetServiceConfig {
   struct {
      /**Open handle to the service*/
      POLICY_HND *svc_hnd;
   } in;

   struct {
      /**Returned Configuration information*/
      CacServiceConfig config;
   } out;
};

/**
 * Retrieves configuration information about a service.
 * @param hnd Initialized and connected server handle
 * @param mem_ctx Context for memory allocation
 * @param op Initialized Parameters
 * @return CAC_FAILURE - the operation was not successful hnd->status is set appropriately
 * @return CAC_SUCCESS - the operation was successful
 */
int cac_SvcGetServiceConfig(CacServerHandle *hnd, TALLOC_CTX *mem_ctx, struct SvcGetServiceConfig *op);

/**@}*/ /*SCM_Functions*/

struct rpc_pipe_client *cac_GetPipe(CacServerHandle *hnd, int pi_idx);

#endif /* LIBMSRPC_H */


