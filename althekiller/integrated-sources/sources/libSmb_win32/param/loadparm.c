/* 
   Unix SMB/CIFS implementation.
   Parameter loading functions
   Copyright (C) Karl Auer 1993-1998

   Largely re-written by Andrew Tridgell, September 1994

   Copyright (C) Simo Sorce 2001
   Copyright (C) Alexander Bokovoy 2002
   Copyright (C) Stefan (metze) Metzmacher 2002
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

/*
 *  Load parameters.
 *
 *  This module provides suitable callback functions for the params
 *  module. It builds the internal table of service details which is
 *  then used by the rest of the server.
 *
 * To add a parameter:
 *
 * 1) add it to the global or service structure definition
 * 2) add it to the parm_table
 * 3) add it to the list of available functions (eg: using FN_GLOBAL_STRING())
 * 4) If it's a global then initialise it in init_globals. If a local
 *    (ie. service) parameter then initialise it in the sDefault structure
 *  
 *
 * Notes:
 *   The configuration file is processed sequentially for speed. It is NOT
 *   accessed randomly as happens in 'real' Windows. For this reason, there
 *   is a fair bit of sequence-dependent code here - ie., code which assumes
 *   that certain things happen before others. In particular, the code which
 *   happens at the boundary between sections is delicately poised, so be
 *   careful!
 *
 */

#include "includes.h"

BOOL in_client = False;		/* Not in the client by default */
BOOL bLoaded = False;

#ifdef _XBOX
	static char* smbWORKGROUP = NULL;

	void xb_setSambaWorkgroup(char* workgroup)
	{
		SAFE_FREE(smbWORKGROUP);
		smbWORKGROUP = strdup(workgroup);
	}
#endif

extern pstring user_socket_options;
extern enum protocol_types Protocol;

#ifndef GLOBAL_NAME
#define GLOBAL_NAME "global"
#endif

#ifndef PRINTERS_NAME
#define PRINTERS_NAME "printers"
#endif

#ifndef HOMES_NAME
#define HOMES_NAME "homes"
#endif

/* some helpful bits */
#define LP_SNUM_OK(i) (((i) >= 0) && ((i) < iNumServices) && (ServicePtrs != NULL) && ServicePtrs[(i)]->valid)
#define VALID(i) (ServicePtrs != NULL && ServicePtrs[i]->valid)

#define USERSHARE_VALID 1
#define USERSHARE_PENDING_DELETE 2

int keepalive = DEFAULT_KEEPALIVE;
BOOL use_getwd_cache = True;

extern int extra_time_offset;

static BOOL defaults_saved = False;

typedef struct _param_opt_struct param_opt_struct;
struct _param_opt_struct {
	param_opt_struct *prev, *next;
	char *key;
	char *value;
	char **list;
};

/* 
 * This structure describes global (ie., server-wide) parameters.
 */
typedef struct {
	char *smb_ports;
	char *dos_charset;
	char *unix_charset;
	char *display_charset;
	char *szPrintcapname;
	char *szAddPortCommand;
	char *szEnumPortsCommand;
	char *szAddPrinterCommand;
	char *szDeletePrinterCommand;
	char *szOs2DriverMap;
	char *szLockDir;
	char *szPidDir;
	char *szRootdir;
	char *szDefaultService;
	char *szGetQuota;
	char *szSetQuota;
	char *szMsgCommand;
	char *szServerString;
	char *szAutoServices;
	char *szPasswdProgram;
	char *szPasswdChat;
	char *szLogFile;
	char *szConfigFile;
	char *szSMBPasswdFile;
	char *szPrivateDir;
	char *szPassdbBackend;
	char **szPreloadModules;
	char *szPasswordServer;
	char *szSocketOptions;
	char *szRealm;
	char *szAfsUsernameMap;
	int iAfsTokenLifetime;
 	char *szLogNtTokenCommand;
	char *szUsernameMap;
	char *szLogonScript;
	char *szLogonPath;
	char *szLogonDrive;
	char *szLogonHome;
	char **szWINSservers;
	char **szInterfaces;
	char *szRemoteAnnounce;
	char *szRemoteBrowseSync;
	char *szSocketAddress;
	char *szNISHomeMapName;
	char *szAnnounceVersion;	/* This is initialised in init_globals */
	char *szWorkgroup;
	char *szNetbiosName;
	char **szNetbiosAliases;
	char *szNetbiosScope;
	char *szNameResolveOrder;
	char *szPanicAction;
	char *szAddUserScript;
        char *szRenameUserScript;
	char *szDelUserScript;
	char *szAddGroupScript;
	char *szDelGroupScript;
	char *szAddUserToGroupScript;
	char *szDelUserFromGroupScript;
	char *szSetPrimaryGroupScript;
	char *szAddMachineScript;
	char *szShutdownScript;
	char *szAbortShutdownScript;
	char *szUsernameMapScript;
	char *szCheckPasswordScript;
	char *szWINSHook;
	char *szUtmpDir;
	char *szWtmpDir;
	BOOL bUtmp;
	char *szIdmapUID;
	char *szIdmapGID;
	BOOL bPassdbExpandExplicit;
	int AlgorithmicRidBase;
	char *szTemplateHomedir;
	char *szTemplateShell;
	char *szWinbindSeparator;
	BOOL bWinbindEnumUsers;
	BOOL bWinbindEnumGroups;
	BOOL bWinbindUseDefaultDomain;
	BOOL bWinbindTrustedDomainsOnly;
	BOOL bWinbindNestedGroups;
	BOOL bWinbindRefreshTickets;
	BOOL bWinbindOfflineLogon;
	char **szIdmapBackend;
	char *szAddShareCommand;
	char *szChangeShareCommand;
	char *szDeleteShareCommand;
        char **szEventLogs;
	char *szGuestaccount;
	char *szManglingMethod;
	char **szServicesList;
	char *szUsersharePath;
	char *szUsershareTemplateShare;
	char **szUsersharePrefixAllowList;
	char **szUsersharePrefixDenyList;
	int mangle_prefix;
	int max_log_size;
	char *szLogLevel;
	int max_xmit;
	int max_mux;
	int max_open_files;
	int open_files_db_hash_size;
	int pwordlevel;
	int unamelevel;
	int deadtime;
	int maxprotocol;
	int minprotocol;
	int security;
	char **AuthMethods;
	BOOL paranoid_server_security;
	int maxdisksize;
	int lpqcachetime;
	int iMaxSmbdProcesses;
	BOOL bDisableSpoolss;
	int syslog;
	int os_level;
	int enhanced_browsing;
	int max_ttl;
	int max_wins_ttl;
	int min_wins_ttl;
	int lm_announce;
	int lm_interval;
	int announce_as;	/* This is initialised in init_globals */
	int machine_password_timeout;
	int map_to_guest;
	int oplock_break_wait_time;
	int winbind_cache_time;
	int winbind_max_idle_children;
	char **szWinbindNssInfo;
	int iLockSpinCount;
	int iLockSpinTime;
	char *szLdapMachineSuffix;
	char *szLdapUserSuffix;
	char *szLdapIdmapSuffix;
	char *szLdapGroupSuffix;
	int ldap_ssl;
	char *szLdapSuffix;
	char *szLdapAdminDn;
	int iAclCompat;
	char *szCupsServer;
	char *szIPrintServer;
	int ldap_passwd_sync; 
	int ldap_replication_sleep;
	int ldap_timeout; /* This is initialised in init_globals */
	int ldap_page_size;
	BOOL ldap_delete_dn;
	BOOL bMsAddPrinterWizard;
	BOOL bDNSproxy;
	BOOL bWINSsupport;
	BOOL bWINSproxy;
	BOOL bLocalMaster;
	BOOL bPreferredMaster;
	BOOL bDomainMaster;
	BOOL bDomainLogons;
	BOOL bEncryptPasswords;
	BOOL bUpdateEncrypt;
	int  clientSchannel;
	int  serverSchannel;
	BOOL bNullPasswords;
	BOOL bObeyPamRestrictions;
	BOOL bLoadPrinters;
	int PrintcapCacheTime;
	BOOL bLargeReadwrite;
	BOOL bReadRaw;
	BOOL bWriteRaw;
	BOOL bReadbmpx;
	BOOL bSyslogOnly;
	BOOL bBrowseList;
	BOOL bNISHomeMap;
	BOOL bTimeServer;
	BOOL bBindInterfacesOnly;
	BOOL bPamPasswordChange;
	BOOL bUnixPasswdSync;
	BOOL bPasswdChatDebug;
	int iPasswdChatTimeout;
	BOOL bTimestampLogs;
	BOOL bNTSmbSupport;
	BOOL bNTPipeSupport;
	BOOL bNTStatusSupport;
	BOOL bStatCache;
	int iMaxStatCacheSize;
	BOOL bKernelOplocks;
	BOOL bAllowTrustedDomains;
	BOOL bLanmanAuth;
	BOOL bNTLMAuth;
	BOOL bUseSpnego;
	BOOL bClientLanManAuth;
	BOOL bClientNTLMv2Auth;
	BOOL bClientPlaintextAuth;
	BOOL bClientUseSpnego;
	BOOL bDebugHiresTimestamp;
	BOOL bDebugPid;
	BOOL bDebugUid;
	BOOL bEnableCoreFiles;
	BOOL bHostMSDfs;
	BOOL bUseMmap;
	BOOL bHostnameLookups;
	BOOL bUnixExtensions;
	BOOL bDisableNetbios;
	BOOL bKernelChangeNotify;
	BOOL bFamChangeNotify;
	BOOL bUseKerberosKeytab;
	BOOL bDeferSharingViolations;
	BOOL bEnablePrivileges;
	BOOL bASUSupport;
	BOOL bUsershareOwnerOnly;
	BOOL bUsershareAllowGuests;
	int restrict_anonymous;
	int name_cache_timeout;
	int client_signing;
	int server_signing;
	int iUsershareMaxShares;

	BOOL bResetOnZeroVC;
	param_opt_struct *param_opt;
} global;

static global Globals;

/* 
 * This structure describes a single service. 
 */
typedef struct {
	BOOL valid;
	BOOL autoloaded;
	int usershare;
	time_t usershare_last_mod;
	char *szService;
	char *szPath;
	char *szUsername;
	char **szInvalidUsers;
	char **szValidUsers;
	char **szAdminUsers;
	char *szCopy;
	char *szInclude;
	char *szPreExec;
	char *szPostExec;
	char *szRootPreExec;
	char *szRootPostExec;
	char *szCupsOptions;
	char *szPrintcommand;
	char *szLpqcommand;
	char *szLprmcommand;
	char *szLppausecommand;
	char *szLpresumecommand;
	char *szQueuepausecommand;
	char *szQueueresumecommand;
	char *szPrintername;
	char *szDontdescend;
	char **szHostsallow;
	char **szHostsdeny;
	char *szMagicScript;
	char *szMagicOutput;
	char *szMangledMap;
	char *szVetoFiles;
	char *szHideFiles;
	char *szVetoOplockFiles;
	char *comment;
	char *force_user;
	char *force_group;
	char **readlist;
	char **writelist;
	char **printer_admin;
	char *volume;
	char *fstype;
	char **szVfsObjects;
	char *szMSDfsProxy;
	char *szAioWriteBehind;
	char *szDfree;
	int iMinPrintSpace;
	int iMaxPrintJobs;
	int iMaxReportedPrintJobs;
	int iWriteCacheSize;
	int iCreate_mask;
	int iCreate_force_mode;
	int iSecurity_mask;
	int iSecurity_force_mode;
	int iDir_mask;
	int iDir_force_mode;
	int iDir_Security_mask;
	int iDir_Security_force_mode;
	int iMaxConnections;
	int iDefaultCase;
	int iPrinting;
	int iOplockContentionLimit;
	int iCSCPolicy;
	int iBlock_size;
	int iDfreeCacheTime;
	BOOL bPreexecClose;
	BOOL bRootpreexecClose;
	int  iCaseSensitive;
	BOOL bCasePreserve;
	BOOL bShortCasePreserve;
	BOOL bHideDotFiles;
	BOOL bHideSpecialFiles;
	BOOL bHideUnReadable;
	BOOL bHideUnWriteableFiles;
	BOOL bBrowseable;
	BOOL bAvailable;
	BOOL bRead_only;
	BOOL bNo_set_dir;
	BOOL bGuest_only;
	BOOL bGuest_ok;
	BOOL bPrint_ok;
	BOOL bMap_system;
	BOOL bMap_hidden;
	BOOL bMap_archive;
	BOOL bStoreDosAttributes;
	BOOL bDmapiSupport;
	BOOL bLocking;
	int iStrictLocking;
	BOOL bPosixLocking;
	BOOL bShareModes;
	BOOL bOpLocks;
	BOOL bLevel2OpLocks;
	BOOL bOnlyUser;
	BOOL bMangledNames;
	BOOL bWidelinks;
	BOOL bSymlinks;
	BOOL bSyncAlways;
	BOOL bStrictAllocate;
	BOOL bStrictSync;
	char magic_char;
	BOOL *copymap;
	BOOL bDeleteReadonly;
	BOOL bFakeOplocks;
	BOOL bDeleteVetoFiles;
	BOOL bDosFilemode;
	BOOL bDosFiletimes;
	BOOL bDosFiletimeResolution;
	BOOL bFakeDirCreateTimes;
	BOOL bBlockingLocks;
	BOOL bInheritPerms;
	BOOL bInheritACLS;
	BOOL bInheritOwner;
	BOOL bMSDfsRoot;
	BOOL bUseClientDriver;
	BOOL bDefaultDevmode;
	BOOL bForcePrintername;
	BOOL bNTAclSupport;
	BOOL bForceUnknownAclUser;
	BOOL bUseSendfile;
	BOOL bProfileAcls;
	BOOL bMap_acl_inherit;
	BOOL bAfs_Share;
	BOOL bEASupport;
	BOOL bAclCheckPermissions;
	BOOL bAclMapFullControl;
	BOOL bAclGroupControl;
	int iallocation_roundup_size;
	int iAioReadSize;
	int iAioWriteSize;
	int iMap_readonly;
	int ichange_notify_timeout;
	param_opt_struct *param_opt;

	char dummy[3];		/* for alignment */
} service;


/* This is a default service used to prime a services structure */
static service sDefault = {
	True,			/* valid */
	False,			/* not autoloaded */
	0,			/* not a usershare */
	(time_t)0,              /* No last mod time */
	NULL,			/* szService */
	NULL,			/* szPath */
	NULL,			/* szUsername */
	NULL,			/* szInvalidUsers */
	NULL,			/* szValidUsers */
	NULL,			/* szAdminUsers */
	NULL,			/* szCopy */
	NULL,			/* szInclude */
	NULL,			/* szPreExec */
	NULL,			/* szPostExec */
	NULL,			/* szRootPreExec */
	NULL,			/* szRootPostExec */
	NULL,			/* szCupsOptions */
	NULL,			/* szPrintcommand */
	NULL,			/* szLpqcommand */
	NULL,			/* szLprmcommand */
	NULL,			/* szLppausecommand */
	NULL,			/* szLpresumecommand */
	NULL,			/* szQueuepausecommand */
	NULL,			/* szQueueresumecommand */
	NULL,			/* szPrintername */
	NULL,			/* szDontdescend */
	NULL,			/* szHostsallow */
	NULL,			/* szHostsdeny */
	NULL,			/* szMagicScript */
	NULL,			/* szMagicOutput */
	NULL,			/* szMangledMap */
	NULL,			/* szVetoFiles */
	NULL,			/* szHideFiles */
	NULL,			/* szVetoOplockFiles */
	NULL,			/* comment */
	NULL,			/* force user */
	NULL,			/* force group */
	NULL,			/* readlist */
	NULL,			/* writelist */
	NULL,			/* printer admin */
	NULL,			/* volume */
	NULL,			/* fstype */
	NULL,			/* vfs objects */
	NULL,                   /* szMSDfsProxy */
	NULL,			/* szAioWriteBehind */
	NULL,			/* szDfree */
	0,			/* iMinPrintSpace */
	1000,			/* iMaxPrintJobs */
	0,			/* iMaxReportedPrintJobs */
	0,			/* iWriteCacheSize */
	0744,			/* iCreate_mask */
	0000,			/* iCreate_force_mode */
	0777,			/* iSecurity_mask */
	0,			/* iSecurity_force_mode */
	0755,			/* iDir_mask */
	0000,			/* iDir_force_mode */
	0777,			/* iDir_Security_mask */
	0,			/* iDir_Security_force_mode */
	0,			/* iMaxConnections */
	CASE_LOWER,		/* iDefaultCase */
	DEFAULT_PRINTING,	/* iPrinting */
	2,			/* iOplockContentionLimit */
	0,			/* iCSCPolicy */
	1024,           	/* iBlock_size */
	0,			/* iDfreeCacheTime */
	False,			/* bPreexecClose */
	False,			/* bRootpreexecClose */
	Auto,			/* case sensitive */
	True,			/* case preserve */
	True,			/* short case preserve */
	True,			/* bHideDotFiles */
	False,			/* bHideSpecialFiles */
	False,			/* bHideUnReadable */
	False,			/* bHideUnWriteableFiles */
	True,			/* bBrowseable */
	True,			/* bAvailable */
	True,			/* bRead_only */
	True,			/* bNo_set_dir */
	False,			/* bGuest_only */
	False,			/* bGuest_ok */
	False,			/* bPrint_ok */
	False,			/* bMap_system */
	False,			/* bMap_hidden */
	True,			/* bMap_archive */
	False,			/* bStoreDosAttributes */
	False,			/* bDmapiSupport */
	True,			/* bLocking */
	Auto,			/* iStrictLocking */
	True,			/* bPosixLocking */
	True,			/* bShareModes */
	True,			/* bOpLocks */
	True,			/* bLevel2OpLocks */
	False,			/* bOnlyUser */
	True,			/* bMangledNames */
	True,			/* bWidelinks */
	True,			/* bSymlinks */
	False,			/* bSyncAlways */
	False,			/* bStrictAllocate */
	False,			/* bStrictSync */
	'~',			/* magic char */
	NULL,			/* copymap */
	False,			/* bDeleteReadonly */
	False,			/* bFakeOplocks */
	False,			/* bDeleteVetoFiles */
	False,			/* bDosFilemode */
	True,			/* bDosFiletimes */
	False,			/* bDosFiletimeResolution */
	False,			/* bFakeDirCreateTimes */
	True,			/* bBlockingLocks */
	False,			/* bInheritPerms */
	False,			/* bInheritACLS */
	False,			/* bInheritOwner */
	True,			/* bMSDfsRoot */
	False,			/* bUseClientDriver */
	True,			/* bDefaultDevmode */
	False,			/* bForcePrintername */
	True,			/* bNTAclSupport */
	False,                  /* bForceUnknownAclUser */
	False,			/* bUseSendfile */
	False,			/* bProfileAcls */
	False,			/* bMap_acl_inherit */
	False,			/* bAfs_Share */
	False,			/* bEASupport */
	True,			/* bAclCheckPermissions */
	True,			/* bAclMapFullControl */
	False,			/* bAclGroupControl */
	SMB_ROUNDUP_ALLOCATION_SIZE,		/* iallocation_roundup_size */
	0,			/* iAioReadSize */
	0,			/* iAioWriteSize */
	MAP_READONLY_YES,	/* iMap_readonly */
	60,			/* ichange_notify_timeout = 1 minute default. */
	
	NULL,			/* Parametric options */

	""			/* dummy */
};

/* local variables */
static service **ServicePtrs = NULL;
static int iNumServices = 0;
static int iServiceIndex = 0;
static TDB_CONTEXT *ServiceHash;
static int *invalid_services = NULL;
static int num_invalid_services = 0;
static BOOL bInGlobalSection = True;
static BOOL bGlobalOnly = False;
static int server_role;
static int default_server_announce;

#define NUMPARAMETERS (sizeof(parm_table) / sizeof(struct parm_struct))

/* prototypes for the special type handlers */
static BOOL handle_include( int snum, const char *pszParmValue, char **ptr);
static BOOL handle_copy( int snum, const char *pszParmValue, char **ptr);
static BOOL handle_netbios_name( int snum, const char *pszParmValue, char **ptr);
static BOOL handle_idmap_uid( int snum, const char *pszParmValue, char **ptr);
static BOOL handle_idmap_gid( int snum, const char *pszParmValue, char **ptr);
static BOOL handle_debug_list( int snum, const char *pszParmValue, char **ptr );
static BOOL handle_workgroup( int snum, const char *pszParmValue, char **ptr );
static BOOL handle_netbios_aliases( int snum, const char *pszParmValue, char **ptr );
static BOOL handle_netbios_scope( int snum, const char *pszParmValue, char **ptr );
static BOOL handle_charset( int snum, const char *pszParmValue, char **ptr );
static BOOL handle_printing( int snum, const char *pszParmValue, char **ptr);

static void set_server_role(void);
static void set_default_server_announce_type(void);
static void set_allowed_client_auth(void);

static const struct enum_list enum_protocol[] = {
	{PROTOCOL_NT1, "NT1"},
	{PROTOCOL_LANMAN2, "LANMAN2"},
	{PROTOCOL_LANMAN1, "LANMAN1"},
	{PROTOCOL_CORE, "CORE"},
	{PROTOCOL_COREPLUS, "COREPLUS"},
	{PROTOCOL_COREPLUS, "CORE+"},
	{-1, NULL}
};

static const struct enum_list enum_security[] = {
	{SEC_SHARE, "SHARE"},
	{SEC_USER, "USER"},
	{SEC_SERVER, "SERVER"},
	{SEC_DOMAIN, "DOMAIN"},
#ifdef HAVE_ADS
	{SEC_ADS, "ADS"},
#endif
	{-1, NULL}
};

static const struct enum_list enum_printing[] = {
	{PRINT_SYSV, "sysv"},
	{PRINT_AIX, "aix"},
	{PRINT_HPUX, "hpux"},
	{PRINT_BSD, "bsd"},
	{PRINT_QNX, "qnx"},
	{PRINT_PLP, "plp"},
	{PRINT_LPRNG, "lprng"},
	{PRINT_CUPS, "cups"},
	{PRINT_IPRINT, "iprint"},
	{PRINT_LPRNT, "nt"},
	{PRINT_LPROS2, "os2"},
#ifdef DEVELOPER
	{PRINT_TEST, "test"},
	{PRINT_VLP, "vlp"},
#endif /* DEVELOPER */
	{-1, NULL}
};

static const struct enum_list enum_ldap_ssl[] = {
	{LDAP_SSL_OFF, "no"},
	{LDAP_SSL_OFF, "No"},
	{LDAP_SSL_OFF, "off"},
	{LDAP_SSL_OFF, "Off"},
	{LDAP_SSL_START_TLS, "start tls"},
	{LDAP_SSL_START_TLS, "Start_tls"},
	{-1, NULL}
};

static const struct enum_list enum_ldap_passwd_sync[] = {
	{LDAP_PASSWD_SYNC_OFF, "no"},
	{LDAP_PASSWD_SYNC_OFF, "No"},
	{LDAP_PASSWD_SYNC_OFF, "off"},
	{LDAP_PASSWD_SYNC_OFF, "Off"},
	{LDAP_PASSWD_SYNC_ON, "Yes"},
	{LDAP_PASSWD_SYNC_ON, "yes"},
	{LDAP_PASSWD_SYNC_ON, "on"},
	{LDAP_PASSWD_SYNC_ON, "On"},
	{LDAP_PASSWD_SYNC_ONLY, "Only"},
	{LDAP_PASSWD_SYNC_ONLY, "only"},
	{-1, NULL}
};

/* Types of machine we can announce as. */
#define ANNOUNCE_AS_NT_SERVER 1
#define ANNOUNCE_AS_WIN95 2
#define ANNOUNCE_AS_WFW 3
#define ANNOUNCE_AS_NT_WORKSTATION 4

static const struct enum_list enum_announce_as[] = {
	{ANNOUNCE_AS_NT_SERVER, "NT"},
	{ANNOUNCE_AS_NT_SERVER, "NT Server"},
	{ANNOUNCE_AS_NT_WORKSTATION, "NT Workstation"},
	{ANNOUNCE_AS_WIN95, "win95"},
	{ANNOUNCE_AS_WFW, "WfW"},
	{-1, NULL}
};

static const struct enum_list enum_map_readonly[] = {
	{MAP_READONLY_NO, "no"},
	{MAP_READONLY_NO, "false"},
	{MAP_READONLY_NO, "0"},
	{MAP_READONLY_YES, "yes"},
	{MAP_READONLY_YES, "true"},
	{MAP_READONLY_YES, "1"},
	{MAP_READONLY_PERMISSIONS, "permissions"},
	{MAP_READONLY_PERMISSIONS, "perms"},
	{-1, NULL}
};

static const struct enum_list enum_case[] = {
	{CASE_LOWER, "lower"},
	{CASE_UPPER, "upper"},
	{-1, NULL}
};

static const struct enum_list enum_bool_auto[] = {
	{False, "No"},
	{False, "False"},
	{False, "0"},
	{True, "Yes"},
	{True, "True"},
	{True, "1"},
	{Auto, "Auto"},
	{-1, NULL}
};

/* Client-side offline caching policy types */
#define CSC_POLICY_MANUAL 0
#define CSC_POLICY_DOCUMENTS 1
#define CSC_POLICY_PROGRAMS 2
#define CSC_POLICY_DISABLE 3

static const struct enum_list enum_csc_policy[] = {
	{CSC_POLICY_MANUAL, "manual"},
	{CSC_POLICY_DOCUMENTS, "documents"},
	{CSC_POLICY_PROGRAMS, "programs"},
	{CSC_POLICY_DISABLE, "disable"},
	{-1, NULL}
};

/* SMB signing types. */
static const struct enum_list enum_smb_signing_vals[] = {
	{False, "No"},
	{False, "False"},
	{False, "0"},
	{False, "Off"},
	{False, "disabled"},
	{True, "Yes"},
	{True, "True"},
	{True, "1"},
	{True, "On"},
	{True, "enabled"},
	{Auto, "auto"},
	{Required, "required"},
	{Required, "mandatory"},
	{Required, "force"},
	{Required, "forced"},
	{Required, "enforced"},
	{-1, NULL}
};

/* ACL compatibility options. */
static const struct enum_list enum_acl_compat_vals[] = {
    { ACL_COMPAT_AUTO, "auto" },
    { ACL_COMPAT_WINNT, "winnt" },
    { ACL_COMPAT_WIN2K, "win2k" },
    { -1, NULL}
};

/* 
   Do you want session setups at user level security with a invalid
   password to be rejected or allowed in as guest? WinNT rejects them
   but it can be a pain as it means "net view" needs to use a password

   You have 3 choices in the setting of map_to_guest:

   "Never" means session setups with an invalid password
   are rejected. This is the default.

   "Bad User" means session setups with an invalid password
   are rejected, unless the username does not exist, in which case it
   is treated as a guest login

   "Bad Password" means session setups with an invalid password
   are treated as a guest login

   Note that map_to_guest only has an effect in user or server
   level security.
*/

static const struct enum_list enum_map_to_guest[] = {
	{NEVER_MAP_TO_GUEST, "Never"},
	{MAP_TO_GUEST_ON_BAD_USER, "Bad User"},
	{MAP_TO_GUEST_ON_BAD_PASSWORD, "Bad Password"},
        {MAP_TO_GUEST_ON_BAD_UID, "Bad Uid"},
	{-1, NULL}
};

/* Note: We do not initialise the defaults union - it is not allowed in ANSI C
 *
 * The FLAG_HIDE is explicit. Paramters set this way do NOT appear in any edit
 * screen in SWAT. This is used to exclude parameters as well as to squash all
 * parameters that have been duplicated by pseudonyms.
 *
 * NOTE: To display a parameter in BASIC view set FLAG_BASIC
 *       Any parameter that does NOT have FLAG_ADVANCED will not disply at all
 *	 Set FLAG_SHARE and FLAG_PRINT to specifically display parameters in
 *        respective views.
 *
 * NOTE2: Handling of duplicated (synonym) paramters:
 *	Only the first occurance of a parameter should be enabled by FLAG_BASIC
 *	and/or FLAG_ADVANCED. All duplicates following the first mention should be
 *	set to FLAG_HIDE. ie: Make you must place the parameter that has the preferred
 *	name first, and all synonyms must follow it with the FLAG_HIDE attribute.
 */

static struct parm_struct parm_table[] = {
	{N_("Base Options"), P_SEP, P_SEPARATOR}, 

	{"dos charset", P_STRING, P_GLOBAL, &Globals.dos_charset, handle_charset, NULL, FLAG_ADVANCED}, 
	{"unix charset", P_STRING, P_GLOBAL, &Globals.unix_charset, handle_charset, NULL, FLAG_ADVANCED}, 
	{"display charset", P_STRING, P_GLOBAL, &Globals.display_charset, handle_charset, NULL, FLAG_ADVANCED}, 
	{"comment", P_STRING, P_LOCAL, &sDefault.comment, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_SHARE | FLAG_PRINT}, 
	{"path", P_STRING, P_LOCAL, &sDefault.szPath, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_SHARE | FLAG_PRINT}, 
	{"directory", P_STRING, P_LOCAL, &sDefault.szPath, NULL, NULL, FLAG_HIDE}, 
	{"workgroup", P_USTRING, P_GLOBAL, &Globals.szWorkgroup, handle_workgroup, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_WIZARD}, 
#ifdef WITH_ADS
	{"realm", P_USTRING, P_GLOBAL, &Globals.szRealm, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_WIZARD}, 
#endif
	{"netbios name", P_USTRING, P_GLOBAL, &Globals.szNetbiosName, handle_netbios_name, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_WIZARD}, 
	{"netbios aliases", P_LIST, P_GLOBAL, &Globals.szNetbiosAliases, handle_netbios_aliases,  NULL, FLAG_ADVANCED}, 
	{"netbios scope", P_USTRING, P_GLOBAL, &Globals.szNetbiosScope, handle_netbios_scope,  NULL, FLAG_ADVANCED}, 
	{"server string", P_STRING, P_GLOBAL, &Globals.szServerString, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED }, 
	{"interfaces", P_LIST, P_GLOBAL, &Globals.szInterfaces, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_WIZARD}, 
	{"bind interfaces only", P_BOOL, P_GLOBAL, &Globals.bBindInterfacesOnly, NULL, NULL, FLAG_ADVANCED | FLAG_WIZARD}, 

	{N_("Security Options"), P_SEP, P_SEPARATOR}, 

	{"security", P_ENUM, P_GLOBAL, &Globals.security, NULL, enum_security, FLAG_BASIC | FLAG_ADVANCED | FLAG_WIZARD}, 
	{"auth methods", P_LIST, P_GLOBAL, &Globals.AuthMethods, NULL, NULL, FLAG_ADVANCED}, 
	{"encrypt passwords", P_BOOL, P_GLOBAL, &Globals.bEncryptPasswords, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_WIZARD}, 
	{"update encrypted", P_BOOL, P_GLOBAL, &Globals.bUpdateEncrypt, NULL, NULL, FLAG_ADVANCED}, 
	{"client schannel", P_ENUM, P_GLOBAL, &Globals.clientSchannel, NULL, enum_bool_auto, FLAG_BASIC | FLAG_ADVANCED}, 
	{"server schannel", P_ENUM, P_GLOBAL, &Globals.serverSchannel, NULL, enum_bool_auto, FLAG_BASIC | FLAG_ADVANCED}, 
	{"allow trusted domains", P_BOOL, P_GLOBAL, &Globals.bAllowTrustedDomains, NULL, NULL, FLAG_ADVANCED}, 
	{"map to guest", P_ENUM, P_GLOBAL, &Globals.map_to_guest, NULL, enum_map_to_guest, FLAG_ADVANCED}, 
	{"null passwords", P_BOOL, P_GLOBAL, &Globals.bNullPasswords, NULL, NULL, FLAG_ADVANCED}, 
	{"obey pam restrictions", P_BOOL, P_GLOBAL, &Globals.bObeyPamRestrictions, NULL, NULL, FLAG_ADVANCED}, 
	{"password server", P_STRING, P_GLOBAL, &Globals.szPasswordServer, NULL, NULL, FLAG_ADVANCED | FLAG_WIZARD}, 
	{"smb passwd file", P_STRING, P_GLOBAL, &Globals.szSMBPasswdFile, NULL, NULL, FLAG_ADVANCED}, 
	{"private dir", P_STRING, P_GLOBAL, &Globals.szPrivateDir, NULL, NULL, FLAG_ADVANCED}, 
	{"passdb backend", P_STRING, P_GLOBAL, &Globals.szPassdbBackend, NULL, NULL, FLAG_ADVANCED | FLAG_WIZARD}, 
	{"algorithmic rid base", P_INTEGER, P_GLOBAL, &Globals.AlgorithmicRidBase, NULL, NULL, FLAG_ADVANCED}, 
	{"root directory", P_STRING, P_GLOBAL, &Globals.szRootdir, NULL, NULL, FLAG_ADVANCED}, 
	{"root dir", P_STRING, P_GLOBAL, &Globals.szRootdir, NULL, NULL, FLAG_HIDE}, 
	{"root", P_STRING, P_GLOBAL, &Globals.szRootdir, NULL, NULL, FLAG_HIDE}, 
	{"guest account", P_STRING, P_GLOBAL, &Globals.szGuestaccount, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED}, 
	{"enable privileges", P_BOOL, P_GLOBAL, &Globals.bEnablePrivileges, NULL, NULL, FLAG_ADVANCED}, 

	{"pam password change", P_BOOL, P_GLOBAL, &Globals.bPamPasswordChange, NULL, NULL, FLAG_ADVANCED}, 
	{"passwd program", P_STRING, P_GLOBAL, &Globals.szPasswdProgram, NULL, NULL, FLAG_ADVANCED}, 
	{"passwd chat", P_STRING, P_GLOBAL, &Globals.szPasswdChat, NULL, NULL, FLAG_ADVANCED}, 
	{"passwd chat debug", P_BOOL, P_GLOBAL, &Globals.bPasswdChatDebug, NULL, NULL, FLAG_ADVANCED}, 
	{"passwd chat timeout", P_INTEGER, P_GLOBAL, &Globals.iPasswdChatTimeout, NULL, NULL, FLAG_ADVANCED}, 
	{"check password script", P_STRING, P_GLOBAL, &Globals.szCheckPasswordScript, NULL, NULL, FLAG_ADVANCED}, 
	{"username map", P_STRING, P_GLOBAL, &Globals.szUsernameMap, NULL, NULL, FLAG_ADVANCED}, 
	{"password level", P_INTEGER, P_GLOBAL, &Globals.pwordlevel, NULL, NULL, FLAG_ADVANCED}, 
	{"username level", P_INTEGER, P_GLOBAL, &Globals.unamelevel, NULL, NULL, FLAG_ADVANCED}, 
	{"unix password sync", P_BOOL, P_GLOBAL, &Globals.bUnixPasswdSync, NULL, NULL, FLAG_ADVANCED}, 
	{"restrict anonymous", P_INTEGER, P_GLOBAL, &Globals.restrict_anonymous, NULL, NULL, FLAG_ADVANCED}, 
	{"lanman auth", P_BOOL, P_GLOBAL, &Globals.bLanmanAuth, NULL, NULL, FLAG_ADVANCED}, 
	{"ntlm auth", P_BOOL, P_GLOBAL, &Globals.bNTLMAuth, NULL, NULL, FLAG_ADVANCED}, 
	{"client NTLMv2 auth", P_BOOL, P_GLOBAL, &Globals.bClientNTLMv2Auth, NULL, NULL, FLAG_ADVANCED}, 
	{"client lanman auth", P_BOOL, P_GLOBAL, &Globals.bClientLanManAuth, NULL, NULL, FLAG_ADVANCED}, 
	{"client plaintext auth", P_BOOL, P_GLOBAL, &Globals.bClientPlaintextAuth, NULL, NULL, FLAG_ADVANCED}, 

	{"username", P_STRING, P_LOCAL, &sDefault.szUsername, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_SHARE}, 
	{"user", P_STRING, P_LOCAL, &sDefault.szUsername, NULL, NULL, FLAG_HIDE}, 
	{"users", P_STRING, P_LOCAL, &sDefault.szUsername, NULL, NULL, FLAG_HIDE}, 

	{"invalid users", P_LIST, P_LOCAL, &sDefault.szInvalidUsers, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_SHARE}, 
	{"valid users", P_LIST, P_LOCAL, &sDefault.szValidUsers, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_SHARE}, 
	{"admin users", P_LIST, P_LOCAL, &sDefault.szAdminUsers, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_SHARE}, 
	{"read list", P_LIST, P_LOCAL, &sDefault.readlist, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_SHARE}, 
	{"write list", P_LIST, P_LOCAL, &sDefault.writelist, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_SHARE}, 
	{"printer admin", P_LIST, P_LOCAL, &sDefault.printer_admin, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_PRINT | FLAG_DEPRECATED }, 
	{"force user", P_STRING, P_LOCAL, &sDefault.force_user, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"force group", P_STRING, P_LOCAL, &sDefault.force_group, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"group", P_STRING, P_LOCAL, &sDefault.force_group, NULL, NULL, FLAG_ADVANCED}, 

	{"read only", P_BOOL, P_LOCAL, &sDefault.bRead_only, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_SHARE}, 
	{"write ok", P_BOOLREV, P_LOCAL, &sDefault.bRead_only, NULL, NULL, FLAG_HIDE}, 
	{"writeable", P_BOOLREV, P_LOCAL, &sDefault.bRead_only, NULL, NULL, FLAG_HIDE}, 
	{"writable", P_BOOLREV, P_LOCAL, &sDefault.bRead_only, NULL, NULL, FLAG_HIDE}, 

	{"acl check permissions", P_BOOL, P_LOCAL, &sDefault.bAclCheckPermissions, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_SHARE},
	{"acl group control", P_BOOL, P_LOCAL, &sDefault.bAclGroupControl, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_SHARE | FLAG_DEPRECATED },
	{"acl map full control", P_BOOL, P_LOCAL, &sDefault.bAclMapFullControl, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_SHARE},
	{"create mask", P_OCTAL, P_LOCAL, &sDefault.iCreate_mask, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_SHARE}, 
	{"create mode", P_OCTAL, P_LOCAL, &sDefault.iCreate_mask, NULL, NULL, FLAG_HIDE}, 
	{"force create mode", P_OCTAL, P_LOCAL, &sDefault.iCreate_force_mode, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_SHARE}, 
	{"security mask", P_OCTAL, P_LOCAL, &sDefault.iSecurity_mask, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_SHARE}, 
	{"force security mode", P_OCTAL, P_LOCAL, &sDefault.iSecurity_force_mode, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_SHARE}, 
	{"directory mask", P_OCTAL, P_LOCAL, &sDefault.iDir_mask, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_SHARE}, 
	{"directory mode", P_OCTAL, P_LOCAL, &sDefault.iDir_mask, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL}, 
	{"force directory mode", P_OCTAL, P_LOCAL, &sDefault.iDir_force_mode, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_SHARE}, 
	{"directory security mask", P_OCTAL, P_LOCAL, &sDefault.iDir_Security_mask, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_SHARE}, 
	{"force directory security mode", P_OCTAL, P_LOCAL, &sDefault.iDir_Security_force_mode, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_SHARE}, 
	{"force unknown acl user", P_BOOL, P_LOCAL, &sDefault.bForceUnknownAclUser, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_SHARE},
	{"inherit permissions", P_BOOL, P_LOCAL, &sDefault.bInheritPerms, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"inherit acls", P_BOOL, P_LOCAL, &sDefault.bInheritACLS, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"inherit owner", P_BOOL, P_LOCAL, &sDefault.bInheritOwner, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"guest only", P_BOOL, P_LOCAL, &sDefault.bGuest_only, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"only guest", P_BOOL, P_LOCAL, &sDefault.bGuest_only, NULL, NULL, FLAG_HIDE}, 

	{"guest ok", P_BOOL, P_LOCAL, &sDefault.bGuest_ok, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_SHARE | FLAG_PRINT}, 
	{"public", P_BOOL, P_LOCAL, &sDefault.bGuest_ok, NULL, NULL, FLAG_HIDE}, 

	{"only user", P_BOOL, P_LOCAL, &sDefault.bOnlyUser, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_DEPRECATED}, 
	{"hosts allow", P_LIST, P_LOCAL, &sDefault.szHostsallow, NULL, NULL, FLAG_GLOBAL | FLAG_BASIC | FLAG_ADVANCED | FLAG_SHARE | FLAG_PRINT}, 
	{"allow hosts", P_LIST, P_LOCAL, &sDefault.szHostsallow, NULL, NULL, FLAG_HIDE}, 
	{"hosts deny", P_LIST, P_LOCAL, &sDefault.szHostsdeny, NULL, NULL, FLAG_GLOBAL | FLAG_BASIC | FLAG_ADVANCED | FLAG_SHARE | FLAG_PRINT}, 
	{"deny hosts", P_LIST, P_LOCAL, &sDefault.szHostsdeny, NULL, NULL, FLAG_HIDE}, 
	{"preload modules", P_LIST, P_GLOBAL, &Globals.szPreloadModules, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL}, 
	{"use kerberos keytab", P_BOOL, P_GLOBAL, &Globals.bUseKerberosKeytab, NULL, NULL, FLAG_ADVANCED}, 

	{N_("Logging Options"), P_SEP, P_SEPARATOR}, 

	{"log level", P_STRING, P_GLOBAL, &Globals.szLogLevel, handle_debug_list, NULL, FLAG_ADVANCED}, 
	{"debuglevel", P_STRING, P_GLOBAL, &Globals.szLogLevel, handle_debug_list, NULL, FLAG_HIDE}, 
	{"syslog", P_INTEGER, P_GLOBAL, &Globals.syslog, NULL, NULL, FLAG_ADVANCED}, 
	{"syslog only", P_BOOL, P_GLOBAL, &Globals.bSyslogOnly, NULL, NULL, FLAG_ADVANCED}, 
	{"log file", P_STRING, P_GLOBAL, &Globals.szLogFile, NULL, NULL, FLAG_ADVANCED}, 

	{"max log size", P_INTEGER, P_GLOBAL, &Globals.max_log_size, NULL, NULL, FLAG_ADVANCED}, 
	{"debug timestamp", P_BOOL, P_GLOBAL, &Globals.bTimestampLogs, NULL, NULL, FLAG_ADVANCED}, 
	{"timestamp logs", P_BOOL, P_GLOBAL, &Globals.bTimestampLogs, NULL, NULL, FLAG_ADVANCED}, 
	{"debug hires timestamp", P_BOOL, P_GLOBAL, &Globals.bDebugHiresTimestamp, NULL, NULL, FLAG_ADVANCED}, 
	{"debug pid", P_BOOL, P_GLOBAL, &Globals.bDebugPid, NULL, NULL, FLAG_ADVANCED}, 
	{"debug uid", P_BOOL, P_GLOBAL, &Globals.bDebugUid, NULL, NULL, FLAG_ADVANCED}, 
	{"enable core files", P_BOOL, P_GLOBAL, &Globals.bEnableCoreFiles, NULL, NULL, FLAG_ADVANCED},

	{N_("Protocol Options"), P_SEP, P_SEPARATOR}, 

	{"allocation roundup size", P_INTEGER, P_LOCAL, &sDefault.iallocation_roundup_size, NULL, NULL, FLAG_ADVANCED}, 
	{"aio read size", P_INTEGER, P_LOCAL, &sDefault.iAioReadSize, NULL, NULL, FLAG_ADVANCED}, 
	{"aio write size", P_INTEGER, P_LOCAL, &sDefault.iAioWriteSize, NULL, NULL, FLAG_ADVANCED}, 
	{"aio write behind", P_STRING, P_LOCAL, &sDefault.szAioWriteBehind, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL }, 
	{"smb ports", P_STRING, P_GLOBAL, &Globals.smb_ports, NULL, NULL, FLAG_ADVANCED}, 
	{"large readwrite", P_BOOL, P_GLOBAL, &Globals.bLargeReadwrite, NULL, NULL, FLAG_ADVANCED}, 
	{"max protocol", P_ENUM, P_GLOBAL, &Globals.maxprotocol, NULL, enum_protocol, FLAG_ADVANCED}, 
	{"protocol", P_ENUM, P_GLOBAL, &Globals.maxprotocol, NULL, enum_protocol, FLAG_ADVANCED}, 
	{"min protocol", P_ENUM, P_GLOBAL, &Globals.minprotocol, NULL, enum_protocol, FLAG_ADVANCED}, 
	{"read bmpx", P_BOOL, P_GLOBAL, &Globals.bReadbmpx, NULL, NULL, FLAG_ADVANCED}, 
	{"read raw", P_BOOL, P_GLOBAL, &Globals.bReadRaw, NULL, NULL, FLAG_ADVANCED}, 
	{"write raw", P_BOOL, P_GLOBAL, &Globals.bWriteRaw, NULL, NULL, FLAG_ADVANCED}, 
	{"disable netbios", P_BOOL, P_GLOBAL, &Globals.bDisableNetbios, NULL, NULL, FLAG_ADVANCED}, 
	{"reset on zero vc", P_BOOL, P_GLOBAL, &Globals.bResetOnZeroVC, NULL, NULL, FLAG_ADVANCED}, 

	{"acl compatibility", P_ENUM, P_GLOBAL, &Globals.iAclCompat, NULL,  enum_acl_compat_vals, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"defer sharing violations", P_BOOL, P_GLOBAL, &Globals.bDeferSharingViolations, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL},
	{"ea support", P_BOOL, P_LOCAL, &sDefault.bEASupport, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"nt acl support", P_BOOL, P_LOCAL, &sDefault.bNTAclSupport, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"nt pipe support", P_BOOL, P_GLOBAL, &Globals.bNTPipeSupport, NULL, NULL, FLAG_ADVANCED}, 
	{"nt status support", P_BOOL, P_GLOBAL, &Globals.bNTStatusSupport, NULL, NULL, FLAG_ADVANCED}, 
	{"profile acls", P_BOOL, P_LOCAL, &sDefault.bProfileAcls, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_SHARE}, 

	{"announce version", P_STRING, P_GLOBAL, &Globals.szAnnounceVersion, NULL, NULL, FLAG_ADVANCED}, 
	{"announce as", P_ENUM, P_GLOBAL, &Globals.announce_as, NULL, enum_announce_as,  FLAG_ADVANCED}, 
	{"map acl inherit", P_BOOL, P_LOCAL, &sDefault.bMap_acl_inherit, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"afs share", P_BOOL, P_LOCAL, &sDefault.bAfs_Share, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL},
	{"max mux", P_INTEGER, P_GLOBAL, &Globals.max_mux, NULL, NULL, FLAG_ADVANCED}, 
	{"max xmit", P_INTEGER, P_GLOBAL, &Globals.max_xmit, NULL, NULL, FLAG_ADVANCED}, 

	{"name resolve order", P_STRING, P_GLOBAL, &Globals.szNameResolveOrder, NULL, NULL, FLAG_ADVANCED | FLAG_WIZARD}, 
	{"max ttl", P_INTEGER, P_GLOBAL, &Globals.max_ttl, NULL, NULL, FLAG_ADVANCED}, 
	{"max wins ttl", P_INTEGER, P_GLOBAL, &Globals.max_wins_ttl, NULL, NULL, FLAG_ADVANCED}, 
	{"min wins ttl", P_INTEGER, P_GLOBAL, &Globals.min_wins_ttl, NULL, NULL, FLAG_ADVANCED}, 
	{"time server", P_BOOL, P_GLOBAL, &Globals.bTimeServer, NULL, NULL, FLAG_ADVANCED}, 
	{"unix extensions", P_BOOL, P_GLOBAL, &Globals.bUnixExtensions, NULL, NULL, FLAG_ADVANCED}, 
	{"use spnego", P_BOOL, P_GLOBAL, &Globals.bUseSpnego, NULL, NULL, FLAG_ADVANCED}, 
	{"client signing", P_ENUM, P_GLOBAL, &Globals.client_signing, NULL, enum_smb_signing_vals, FLAG_ADVANCED}, 
	{"server signing", P_ENUM, P_GLOBAL, &Globals.server_signing, NULL, enum_smb_signing_vals, FLAG_ADVANCED}, 
	{"client use spnego", P_BOOL, P_GLOBAL, &Globals.bClientUseSpnego, NULL, NULL, FLAG_ADVANCED}, 

	{"enable asu support", P_BOOL, P_GLOBAL, &Globals.bASUSupport, NULL, NULL, FLAG_ADVANCED}, 
	{"svcctl list", P_LIST, P_GLOBAL, &Globals.szServicesList, NULL, NULL, FLAG_ADVANCED},

	{N_("Tuning Options"), P_SEP, P_SEPARATOR}, 

	{"block size", P_INTEGER, P_LOCAL, &sDefault.iBlock_size, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"change notify timeout", P_INTEGER, P_LOCAL, &sDefault.ichange_notify_timeout, NULL, NULL, FLAG_ADVANCED}, 
	{"deadtime", P_INTEGER, P_GLOBAL, &Globals.deadtime, NULL, NULL, FLAG_ADVANCED}, 
	{"getwd cache", P_BOOL, P_GLOBAL, &use_getwd_cache, NULL, NULL, FLAG_ADVANCED}, 
	{"keepalive", P_INTEGER, P_GLOBAL, &keepalive, NULL, NULL, FLAG_ADVANCED}, 
	{"kernel change notify", P_BOOL, P_GLOBAL, &Globals.bKernelChangeNotify, NULL, NULL, FLAG_ADVANCED}, 
	{"fam change notify", P_BOOL, P_GLOBAL, &Globals.bFamChangeNotify, NULL, NULL, FLAG_ADVANCED},

	{"lpq cache time", P_INTEGER, P_GLOBAL, &Globals.lpqcachetime, NULL, NULL, FLAG_ADVANCED}, 
	{"max smbd processes", P_INTEGER, P_GLOBAL, &Globals.iMaxSmbdProcesses, NULL, NULL, FLAG_ADVANCED}, 
	{"max connections", P_INTEGER, P_LOCAL, &sDefault.iMaxConnections, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"paranoid server security", P_BOOL, P_GLOBAL, &Globals.paranoid_server_security, NULL, NULL, FLAG_ADVANCED}, 
	{"max disk size", P_INTEGER, P_GLOBAL, &Globals.maxdisksize, NULL, NULL, FLAG_ADVANCED}, 
	{"max open files", P_INTEGER, P_GLOBAL, &Globals.max_open_files, NULL, NULL, FLAG_ADVANCED}, 
	{"min print space", P_INTEGER, P_LOCAL, &sDefault.iMinPrintSpace, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT}, 
	{"open files database hash size", P_INTEGER, P_GLOBAL, &Globals.open_files_db_hash_size, NULL, NULL, FLAG_ADVANCED}, 

	{"socket options", P_GSTRING, P_GLOBAL, user_socket_options, NULL, NULL, FLAG_ADVANCED}, 
	{"strict allocate", P_BOOL, P_LOCAL, &sDefault.bStrictAllocate, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"strict sync", P_BOOL, P_LOCAL, &sDefault.bStrictSync, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"sync always", P_BOOL, P_LOCAL, &sDefault.bSyncAlways, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"use mmap", P_BOOL, P_GLOBAL, &Globals.bUseMmap, NULL, NULL, FLAG_ADVANCED}, 
	{"use sendfile", P_BOOL, P_LOCAL, &sDefault.bUseSendfile, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"hostname lookups", P_BOOL, P_GLOBAL, &Globals.bHostnameLookups, NULL, NULL, FLAG_ADVANCED}, 
	{"write cache size", P_INTEGER, P_LOCAL, &sDefault.iWriteCacheSize, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_DEPRECATED}, 

	{"name cache timeout", P_INTEGER, P_GLOBAL, &Globals.name_cache_timeout, NULL, NULL, FLAG_ADVANCED}, 

	{N_("Printing Options"), P_SEP, P_SEPARATOR}, 

	{"max reported print jobs", P_INTEGER, P_LOCAL, &sDefault.iMaxReportedPrintJobs, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT}, 
	{"max print jobs", P_INTEGER, P_LOCAL, &sDefault.iMaxPrintJobs, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT}, 
	{"load printers", P_BOOL, P_GLOBAL, &Globals.bLoadPrinters, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT}, 
	{"printcap cache time", P_INTEGER, P_GLOBAL, &Globals.PrintcapCacheTime, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT}, 
	{"printcap name", P_STRING, P_GLOBAL, &Globals.szPrintcapname, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT}, 
	{"printcap", P_STRING, P_GLOBAL, &Globals.szPrintcapname, NULL, NULL, FLAG_HIDE}, 
	{"printable", P_BOOL, P_LOCAL, &sDefault.bPrint_ok, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT}, 
	{"print ok", P_BOOL, P_LOCAL, &sDefault.bPrint_ok, NULL, NULL, FLAG_HIDE}, 
	{"printing", P_ENUM, P_LOCAL, &sDefault.iPrinting, handle_printing, enum_printing, FLAG_ADVANCED | FLAG_PRINT | FLAG_GLOBAL}, 
	{"cups options", P_STRING, P_LOCAL, &sDefault.szCupsOptions, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT | FLAG_GLOBAL}, 
	{"cups server", P_STRING, P_GLOBAL, &Globals.szCupsServer, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT | FLAG_GLOBAL}, 
	{"iprint server", P_STRING, P_GLOBAL, &Globals.szIPrintServer, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT | FLAG_GLOBAL}, 
	{"print command", P_STRING, P_LOCAL, &sDefault.szPrintcommand, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT | FLAG_GLOBAL}, 
	{"disable spoolss", P_BOOL, P_GLOBAL, &Globals.bDisableSpoolss, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT | FLAG_GLOBAL}, 
	{"enable spoolss", P_BOOLREV, P_GLOBAL, &Globals.bDisableSpoolss, NULL, NULL, FLAG_HIDE}, 
	{"lpq command", P_STRING, P_LOCAL, &sDefault.szLpqcommand, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT | FLAG_GLOBAL}, 
	{"lprm command", P_STRING, P_LOCAL, &sDefault.szLprmcommand, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT | FLAG_GLOBAL}, 
	{"lppause command", P_STRING, P_LOCAL, &sDefault.szLppausecommand, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT | FLAG_GLOBAL}, 
	{"lpresume command", P_STRING, P_LOCAL, &sDefault.szLpresumecommand, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT | FLAG_GLOBAL}, 
	{"queuepause command", P_STRING, P_LOCAL, &sDefault.szQueuepausecommand, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT | FLAG_GLOBAL}, 
	{"queueresume command", P_STRING, P_LOCAL, &sDefault.szQueueresumecommand, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT | FLAG_GLOBAL}, 

	{"addport command", P_STRING, P_GLOBAL, &Globals.szAddPortCommand, NULL, NULL, FLAG_ADVANCED}, 
	{"enumports command", P_STRING, P_GLOBAL, &Globals.szEnumPortsCommand, NULL, NULL, FLAG_ADVANCED}, 
	{"addprinter command", P_STRING, P_GLOBAL, &Globals.szAddPrinterCommand, NULL, NULL, FLAG_ADVANCED}, 
	{"deleteprinter command", P_STRING, P_GLOBAL, &Globals.szDeletePrinterCommand, NULL, NULL, FLAG_ADVANCED}, 
	{"show add printer wizard", P_BOOL, P_GLOBAL, &Globals.bMsAddPrinterWizard, NULL, NULL, FLAG_ADVANCED}, 
	{"os2 driver map", P_STRING, P_GLOBAL, &Globals.szOs2DriverMap, NULL, NULL, FLAG_ADVANCED}, 

	{"printer name", P_STRING, P_LOCAL, &sDefault.szPrintername, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT}, 
	{"printer", P_STRING, P_LOCAL, &sDefault.szPrintername, NULL, NULL, FLAG_HIDE}, 
	{"use client driver", P_BOOL, P_LOCAL, &sDefault.bUseClientDriver, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT}, 
	{"default devmode", P_BOOL, P_LOCAL, &sDefault.bDefaultDevmode, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT}, 
	{"force printername", P_BOOL, P_LOCAL, &sDefault.bForcePrintername, NULL, NULL, FLAG_ADVANCED | FLAG_PRINT}, 

	{N_("Filename Handling"), P_SEP, P_SEPARATOR}, 
	{"mangling method", P_STRING, P_GLOBAL, &Globals.szManglingMethod, NULL, NULL, FLAG_ADVANCED}, 
	{"mangle prefix", P_INTEGER, P_GLOBAL, &Globals.mangle_prefix, NULL, NULL, FLAG_ADVANCED}, 

	{"default case", P_ENUM, P_LOCAL, &sDefault.iDefaultCase, NULL, enum_case, FLAG_ADVANCED | FLAG_SHARE}, 
	{"case sensitive", P_ENUM, P_LOCAL, &sDefault.iCaseSensitive, NULL, enum_bool_auto, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"casesignames", P_ENUM, P_LOCAL, &sDefault.iCaseSensitive, NULL, enum_bool_auto, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL | FLAG_HIDE}, 
	{"preserve case", P_BOOL, P_LOCAL, &sDefault.bCasePreserve, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"short preserve case", P_BOOL, P_LOCAL, &sDefault.bShortCasePreserve, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"mangling char", P_CHAR, P_LOCAL, &sDefault.magic_char, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"hide dot files", P_BOOL, P_LOCAL, &sDefault.bHideDotFiles, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"hide special files", P_BOOL, P_LOCAL, &sDefault.bHideSpecialFiles, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"hide unreadable", P_BOOL, P_LOCAL, &sDefault.bHideUnReadable, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"hide unwriteable files", P_BOOL, P_LOCAL, &sDefault.bHideUnWriteableFiles, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"delete veto files", P_BOOL, P_LOCAL, &sDefault.bDeleteVetoFiles, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"veto files", P_STRING, P_LOCAL, &sDefault.szVetoFiles, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL }, 
	{"hide files", P_STRING, P_LOCAL, &sDefault.szHideFiles, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL }, 
	{"veto oplock files", P_STRING, P_LOCAL, &sDefault.szVetoOplockFiles, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL }, 
	{"map archive", P_BOOL, P_LOCAL, &sDefault.bMap_archive, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"map hidden", P_BOOL, P_LOCAL, &sDefault.bMap_hidden, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"map system", P_BOOL, P_LOCAL, &sDefault.bMap_system, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"map readonly", P_ENUM, P_LOCAL, &sDefault.iMap_readonly, NULL, enum_map_readonly, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"mangled names", P_BOOL, P_LOCAL, &sDefault.bMangledNames, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"mangled map", P_STRING, P_LOCAL, &sDefault.szMangledMap, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL | FLAG_DEPRECATED }, 
	{"max stat cache size", P_INTEGER, P_GLOBAL, &Globals.iMaxStatCacheSize, NULL, NULL, FLAG_ADVANCED}, 
	{"stat cache", P_BOOL, P_GLOBAL, &Globals.bStatCache, NULL, NULL, FLAG_ADVANCED}, 
	{"store dos attributes", P_BOOL, P_LOCAL, &sDefault.bStoreDosAttributes, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"dmapi support", P_BOOL, P_LOCAL, &sDefault.bDmapiSupport, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL},


	{N_("Domain Options"), P_SEP, P_SEPARATOR}, 

	{"machine password timeout", P_INTEGER, P_GLOBAL, &Globals.machine_password_timeout, NULL, NULL, FLAG_ADVANCED | FLAG_WIZARD}, 

	{N_("Logon Options"), P_SEP, P_SEPARATOR}, 

	{"add user script", P_STRING, P_GLOBAL, &Globals.szAddUserScript, NULL, NULL, FLAG_ADVANCED}, 
        {"rename user script", P_STRING, P_GLOBAL, &Globals.szRenameUserScript, NULL, NULL, FLAG_ADVANCED},
	{"delete user script", P_STRING, P_GLOBAL, &Globals.szDelUserScript, NULL, NULL, FLAG_ADVANCED}, 
	{"add group script", P_STRING, P_GLOBAL, &Globals.szAddGroupScript, NULL, NULL, FLAG_ADVANCED}, 
	{"delete group script", P_STRING, P_GLOBAL, &Globals.szDelGroupScript, NULL, NULL, FLAG_ADVANCED}, 
	{"add user to group script", P_STRING, P_GLOBAL, &Globals.szAddUserToGroupScript, NULL, NULL, FLAG_ADVANCED}, 
	{"delete user from group script", P_STRING, P_GLOBAL, &Globals.szDelUserFromGroupScript, NULL, NULL, FLAG_ADVANCED}, 
	{"set primary group script", P_STRING, P_GLOBAL, &Globals.szSetPrimaryGroupScript, NULL, NULL, FLAG_ADVANCED}, 
	{"add machine script", P_STRING, P_GLOBAL, &Globals.szAddMachineScript, NULL, NULL, FLAG_ADVANCED}, 
	{"shutdown script", P_STRING, P_GLOBAL, &Globals.szShutdownScript, NULL, NULL, FLAG_ADVANCED}, 
	{"abort shutdown script", P_STRING, P_GLOBAL, &Globals.szAbortShutdownScript, NULL, NULL, FLAG_ADVANCED}, 
	{"username map script", P_STRING, P_GLOBAL, &Globals.szUsernameMapScript, NULL, NULL, FLAG_ADVANCED}, 

	{"logon script", P_STRING, P_GLOBAL, &Globals.szLogonScript, NULL, NULL, FLAG_ADVANCED}, 
	{"logon path", P_STRING, P_GLOBAL, &Globals.szLogonPath, NULL, NULL, FLAG_ADVANCED}, 
	{"logon drive", P_STRING, P_GLOBAL, &Globals.szLogonDrive, NULL, NULL, FLAG_ADVANCED}, 
	{"logon home", P_STRING, P_GLOBAL, &Globals.szLogonHome, NULL, NULL, FLAG_ADVANCED}, 
	{"domain logons", P_BOOL, P_GLOBAL, &Globals.bDomainLogons, NULL, NULL, FLAG_ADVANCED}, 

	{N_("Browse Options"), P_SEP, P_SEPARATOR}, 

	{"os level", P_INTEGER, P_GLOBAL, &Globals.os_level, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED}, 
	{"lm announce", P_ENUM, P_GLOBAL, &Globals.lm_announce, NULL, enum_bool_auto, FLAG_ADVANCED}, 
	{"lm interval", P_INTEGER, P_GLOBAL, &Globals.lm_interval, NULL, NULL, FLAG_ADVANCED}, 
	{"preferred master", P_ENUM, P_GLOBAL, &Globals.bPreferredMaster, NULL, enum_bool_auto, FLAG_BASIC | FLAG_ADVANCED}, 
	{"prefered master", P_ENUM, P_GLOBAL, &Globals.bPreferredMaster, NULL, enum_bool_auto, FLAG_HIDE}, 
	{"local master", P_BOOL, P_GLOBAL, &Globals.bLocalMaster, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED}, 
	{"domain master", P_ENUM, P_GLOBAL, &Globals.bDomainMaster, NULL, enum_bool_auto, FLAG_BASIC | FLAG_ADVANCED}, 
	{"browse list", P_BOOL, P_GLOBAL, &Globals.bBrowseList, NULL, NULL, FLAG_ADVANCED}, 
	{"browseable", P_BOOL, P_LOCAL, &sDefault.bBrowseable, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_SHARE | FLAG_PRINT}, 
	{"browsable", P_BOOL, P_LOCAL, &sDefault.bBrowseable, NULL, NULL, FLAG_HIDE}, 
	{"enhanced browsing", P_BOOL, P_GLOBAL, &Globals.enhanced_browsing, NULL, NULL, FLAG_ADVANCED}, 

	{N_("WINS Options"), P_SEP, P_SEPARATOR}, 

	{"dns proxy", P_BOOL, P_GLOBAL, &Globals.bDNSproxy, NULL, NULL, FLAG_ADVANCED}, 
	{"wins proxy", P_BOOL, P_GLOBAL, &Globals.bWINSproxy, NULL, NULL, FLAG_ADVANCED}, 

	{"wins server", P_LIST, P_GLOBAL, &Globals.szWINSservers, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_WIZARD}, 
	{"wins support", P_BOOL, P_GLOBAL, &Globals.bWINSsupport, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_WIZARD}, 
	{"wins hook", P_STRING, P_GLOBAL, &Globals.szWINSHook, NULL, NULL, FLAG_ADVANCED}, 

	{N_("Locking Options"), P_SEP, P_SEPARATOR}, 

	{"blocking locks", P_BOOL, P_LOCAL, &sDefault.bBlockingLocks, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"csc policy", P_ENUM, P_LOCAL, &sDefault.iCSCPolicy, NULL, enum_csc_policy, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"fake oplocks", P_BOOL, P_LOCAL, &sDefault.bFakeOplocks, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"kernel oplocks", P_BOOL, P_GLOBAL, &Globals.bKernelOplocks, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL}, 
	{"locking", P_BOOL, P_LOCAL, &sDefault.bLocking, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"lock spin count", P_INTEGER, P_GLOBAL, &Globals.iLockSpinCount, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL}, 
	{"lock spin time", P_INTEGER, P_GLOBAL, &Globals.iLockSpinTime, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL}, 

	{"oplocks", P_BOOL, P_LOCAL, &sDefault.bOpLocks, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"level2 oplocks", P_BOOL, P_LOCAL, &sDefault.bLevel2OpLocks, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"oplock break wait time", P_INTEGER, P_GLOBAL, &Globals.oplock_break_wait_time, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL}, 
	{"oplock contention limit", P_INTEGER, P_LOCAL, &sDefault.iOplockContentionLimit, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"posix locking", P_BOOL, P_LOCAL, &sDefault.bPosixLocking, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"strict locking", P_ENUM, P_LOCAL, &sDefault.iStrictLocking, NULL, enum_bool_auto, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"share modes", P_BOOL, P_LOCAL,  &sDefault.bShareModes, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 

	{N_("Ldap Options"), P_SEP, P_SEPARATOR}, 

	{"ldap admin dn", P_STRING, P_GLOBAL, &Globals.szLdapAdminDn, NULL, NULL, FLAG_ADVANCED}, 
	{"ldap delete dn", P_BOOL, P_GLOBAL, &Globals.ldap_delete_dn, NULL, NULL, FLAG_ADVANCED}, 
	{"ldap group suffix", P_STRING, P_GLOBAL, &Globals.szLdapGroupSuffix, NULL, NULL, FLAG_ADVANCED}, 
	{"ldap idmap suffix", P_STRING, P_GLOBAL, &Globals.szLdapIdmapSuffix, NULL, NULL, FLAG_ADVANCED}, 
	{"ldap machine suffix", P_STRING, P_GLOBAL, &Globals.szLdapMachineSuffix, NULL, NULL, FLAG_ADVANCED}, 
	{"ldap passwd sync", P_ENUM, P_GLOBAL, &Globals.ldap_passwd_sync, NULL, enum_ldap_passwd_sync, FLAG_ADVANCED}, 
	{"ldap password sync", P_ENUM, P_GLOBAL, &Globals.ldap_passwd_sync, NULL, enum_ldap_passwd_sync, FLAG_HIDE}, 
	{"ldap replication sleep", P_INTEGER, P_GLOBAL, &Globals.ldap_replication_sleep, NULL, NULL, FLAG_ADVANCED},
	{"ldap suffix", P_STRING, P_GLOBAL, &Globals.szLdapSuffix, NULL, NULL, FLAG_ADVANCED}, 
	{"ldap ssl", P_ENUM, P_GLOBAL, &Globals.ldap_ssl, NULL, enum_ldap_ssl, FLAG_ADVANCED}, 
	{"ldap timeout", P_INTEGER, P_GLOBAL, &Globals.ldap_timeout, NULL, NULL, FLAG_ADVANCED},
	{"ldap page size", P_INTEGER, P_GLOBAL, &Globals.ldap_page_size, NULL, NULL, FLAG_ADVANCED},
	{"ldap user suffix", P_STRING, P_GLOBAL, &Globals.szLdapUserSuffix, NULL, NULL, FLAG_ADVANCED}, 

	{N_("Miscellaneous Options"), P_SEP, P_SEPARATOR}, 
	{"add share command", P_STRING, P_GLOBAL, &Globals.szAddShareCommand, NULL, NULL, FLAG_ADVANCED}, 
	{"change share command", P_STRING, P_GLOBAL, &Globals.szChangeShareCommand, NULL, NULL, FLAG_ADVANCED}, 
	{"delete share command", P_STRING, P_GLOBAL, &Globals.szDeleteShareCommand, NULL, NULL, FLAG_ADVANCED}, 

	{N_("EventLog Options"), P_SEP, P_SEPARATOR}, 
	{"eventlog list",  P_LIST, P_GLOBAL, &Globals.szEventLogs, NULL, NULL, FLAG_ADVANCED | FLAG_GLOBAL | FLAG_SHARE}, 
	
	{"config file", P_STRING, P_GLOBAL, &Globals.szConfigFile, NULL, NULL, FLAG_HIDE}, 
	{"preload", P_STRING, P_GLOBAL, &Globals.szAutoServices, NULL, NULL, FLAG_ADVANCED}, 
	{"auto services", P_STRING, P_GLOBAL, &Globals.szAutoServices, NULL, NULL, FLAG_ADVANCED}, 
	{"lock directory", P_STRING, P_GLOBAL, &Globals.szLockDir, NULL, NULL, FLAG_ADVANCED}, 
	{"lock dir", P_STRING, P_GLOBAL, &Globals.szLockDir, NULL, NULL, FLAG_HIDE}, 
	{"pid directory", P_STRING, P_GLOBAL, &Globals.szPidDir, NULL, NULL, FLAG_ADVANCED}, 
#ifdef WITH_UTMP
	{"utmp directory", P_STRING, P_GLOBAL, &Globals.szUtmpDir, NULL, NULL, FLAG_ADVANCED}, 
	{"wtmp directory", P_STRING, P_GLOBAL, &Globals.szWtmpDir, NULL, NULL, FLAG_ADVANCED}, 
	{"utmp", P_BOOL, P_GLOBAL, &Globals.bUtmp, NULL, NULL, FLAG_ADVANCED}, 
#endif

	{"default service", P_STRING, P_GLOBAL, &Globals.szDefaultService, NULL, NULL, FLAG_ADVANCED}, 
	{"default", P_STRING, P_GLOBAL, &Globals.szDefaultService, NULL, NULL, FLAG_ADVANCED}, 
	{"message command", P_STRING, P_GLOBAL, &Globals.szMsgCommand, NULL, NULL, FLAG_ADVANCED}, 
	{"dfree cache time", P_INTEGER, P_LOCAL, &sDefault.iDfreeCacheTime, NULL, NULL, FLAG_ADVANCED}, 
	{"dfree command", P_STRING, P_LOCAL, &sDefault.szDfree, NULL, NULL, FLAG_ADVANCED}, 
	{"get quota command", P_STRING, P_GLOBAL, &Globals.szGetQuota, NULL, NULL, FLAG_ADVANCED}, 
	{"set quota command", P_STRING, P_GLOBAL, &Globals.szSetQuota, NULL, NULL, FLAG_ADVANCED}, 
	{"remote announce", P_STRING, P_GLOBAL, &Globals.szRemoteAnnounce, NULL, NULL, FLAG_ADVANCED}, 
	{"remote browse sync", P_STRING, P_GLOBAL, &Globals.szRemoteBrowseSync, NULL, NULL, FLAG_ADVANCED}, 
	{"socket address", P_STRING, P_GLOBAL, &Globals.szSocketAddress, NULL, NULL, FLAG_ADVANCED}, 
	{"homedir map", P_STRING, P_GLOBAL, &Globals.szNISHomeMapName, NULL, NULL, FLAG_ADVANCED}, 
	{"afs username map", P_STRING, P_GLOBAL, &Globals.szAfsUsernameMap, NULL, NULL, FLAG_ADVANCED}, 
	{"afs token lifetime", P_INTEGER, P_GLOBAL, &Globals.iAfsTokenLifetime, NULL, NULL, FLAG_ADVANCED},
	{"log nt token command", P_STRING, P_GLOBAL, &Globals.szLogNtTokenCommand, NULL, NULL, FLAG_ADVANCED},
	{"time offset", P_INTEGER, P_GLOBAL, &extra_time_offset, NULL, NULL, FLAG_ADVANCED}, 
	{"NIS homedir", P_BOOL, P_GLOBAL, &Globals.bNISHomeMap, NULL, NULL, FLAG_ADVANCED}, 
	{"-valid", P_BOOL, P_LOCAL, &sDefault.valid, NULL, NULL, FLAG_HIDE}, 

	{"copy", P_STRING, P_LOCAL, &sDefault.szCopy, handle_copy, NULL, FLAG_HIDE}, 
	{"include", P_STRING, P_LOCAL, &sDefault.szInclude, handle_include, NULL, FLAG_HIDE}, 
	{"preexec", P_STRING, P_LOCAL, &sDefault.szPreExec, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_PRINT}, 
	{"exec", P_STRING, P_LOCAL, &sDefault.szPreExec, NULL, NULL, FLAG_ADVANCED}, 

	{"preexec close", P_BOOL, P_LOCAL, &sDefault.bPreexecClose, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"postexec", P_STRING, P_LOCAL, &sDefault.szPostExec, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_PRINT}, 
	{"root preexec", P_STRING, P_LOCAL, &sDefault.szRootPreExec, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_PRINT}, 
	{"root preexec close", P_BOOL, P_LOCAL, &sDefault.bRootpreexecClose, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"root postexec", P_STRING, P_LOCAL, &sDefault.szRootPostExec, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_PRINT}, 
	{"available", P_BOOL, P_LOCAL, &sDefault.bAvailable, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_SHARE | FLAG_PRINT}, 
	{"usershare allow guests", P_BOOL, P_GLOBAL, &Globals.bUsershareAllowGuests, NULL, NULL, FLAG_ADVANCED},
	{"usershare max shares", P_INTEGER, P_GLOBAL, &Globals.iUsershareMaxShares, NULL, NULL, FLAG_ADVANCED},
	{"usershare owner only", P_BOOL, P_GLOBAL, &Globals.bUsershareOwnerOnly, NULL, NULL, FLAG_ADVANCED}, 
	{"usershare path", P_STRING, P_GLOBAL, &Globals.szUsersharePath, NULL, NULL, FLAG_ADVANCED},
	{"usershare prefix allow list", P_LIST, P_GLOBAL, &Globals.szUsersharePrefixAllowList, NULL, NULL, FLAG_ADVANCED}, 
	{"usershare prefix deny list", P_LIST, P_GLOBAL, &Globals.szUsersharePrefixDenyList, NULL, NULL, FLAG_ADVANCED}, 
	{"usershare template share", P_STRING, P_GLOBAL, &Globals.szUsershareTemplateShare, NULL, NULL, FLAG_ADVANCED},
	{"volume", P_STRING, P_LOCAL, &sDefault.volume, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE }, 
	{"fstype", P_STRING, P_LOCAL, &sDefault.fstype, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"set directory", P_BOOLREV, P_LOCAL, &sDefault.bNo_set_dir, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"wide links", P_BOOL, P_LOCAL, &sDefault.bWidelinks, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"follow symlinks", P_BOOL, P_LOCAL, &sDefault.bSymlinks, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"dont descend", P_STRING, P_LOCAL, &sDefault.szDontdescend, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"magic script", P_STRING, P_LOCAL, &sDefault.szMagicScript, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"magic output", P_STRING, P_LOCAL, &sDefault.szMagicOutput, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"delete readonly", P_BOOL, P_LOCAL, &sDefault.bDeleteReadonly, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"dos filemode", P_BOOL, P_LOCAL, &sDefault.bDosFilemode, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"dos filetimes", P_BOOL, P_LOCAL, &sDefault.bDosFiletimes, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"dos filetime resolution", P_BOOL, P_LOCAL, &sDefault.bDosFiletimeResolution, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 

	{"fake directory create times", P_BOOL, P_LOCAL, &sDefault.bFakeDirCreateTimes, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE | FLAG_GLOBAL}, 
	{"panic action", P_STRING, P_GLOBAL, &Globals.szPanicAction, NULL, NULL, FLAG_ADVANCED}, 

	{N_("VFS module options"), P_SEP, P_SEPARATOR}, 

	{"vfs objects", P_LIST, P_LOCAL, &sDefault.szVfsObjects, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"vfs object", P_LIST, P_LOCAL, &sDefault.szVfsObjects, NULL, NULL, FLAG_HIDE}, 


	{"msdfs root", P_BOOL, P_LOCAL, &sDefault.bMSDfsRoot, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"msdfs proxy", P_STRING, P_LOCAL, &sDefault.szMSDfsProxy, NULL, NULL, FLAG_ADVANCED | FLAG_SHARE}, 
	{"host msdfs", P_BOOL, P_GLOBAL, &Globals.bHostMSDfs, NULL, NULL, FLAG_ADVANCED}, 

	{N_("Winbind options"), P_SEP, P_SEPARATOR}, 

	{"passdb expand explicit", P_BOOL, P_GLOBAL, &Globals.bPassdbExpandExplicit, NULL, NULL, FLAG_ADVANCED},
	{"idmap backend", P_LIST, P_GLOBAL, &Globals.szIdmapBackend, NULL, NULL, FLAG_ADVANCED}, 
	{"idmap uid", P_STRING, P_GLOBAL, &Globals.szIdmapUID, handle_idmap_uid, NULL, FLAG_ADVANCED}, 
	{"winbind uid", P_STRING, P_GLOBAL, &Globals.szIdmapUID, handle_idmap_uid, NULL, FLAG_HIDE}, 
	{"idmap gid", P_STRING, P_GLOBAL, &Globals.szIdmapGID, handle_idmap_gid, NULL, FLAG_ADVANCED}, 
	{"winbind gid", P_STRING, P_GLOBAL, &Globals.szIdmapGID, handle_idmap_gid, NULL, FLAG_HIDE}, 
	{"template homedir", P_STRING, P_GLOBAL, &Globals.szTemplateHomedir, NULL, NULL, FLAG_ADVANCED}, 
	{"template shell", P_STRING, P_GLOBAL, &Globals.szTemplateShell, NULL, NULL, FLAG_ADVANCED}, 
	{"winbind separator", P_STRING, P_GLOBAL, &Globals.szWinbindSeparator, NULL, NULL, FLAG_ADVANCED}, 
	{"winbind cache time", P_INTEGER, P_GLOBAL, &Globals.winbind_cache_time, NULL, NULL, FLAG_ADVANCED}, 
	{"winbind enum users", P_BOOL, P_GLOBAL, &Globals.bWinbindEnumUsers, NULL, NULL, FLAG_ADVANCED}, 
	{"winbind enum groups", P_BOOL, P_GLOBAL, &Globals.bWinbindEnumGroups, NULL, NULL, FLAG_ADVANCED}, 
	{"winbind use default domain", P_BOOL, P_GLOBAL, &Globals.bWinbindUseDefaultDomain, NULL, NULL, FLAG_ADVANCED}, 
	{"winbind trusted domains only", P_BOOL, P_GLOBAL, &Globals.bWinbindTrustedDomainsOnly, NULL, NULL, FLAG_ADVANCED}, 
	{"winbind nested groups", P_BOOL, P_GLOBAL, &Globals.bWinbindNestedGroups, NULL, NULL, FLAG_ADVANCED}, 
	{"winbind nss info", P_LIST, P_GLOBAL, &Globals.szWinbindNssInfo, NULL, NULL, FLAG_ADVANCED}, 
	{"winbind refresh tickets", P_BOOL, P_GLOBAL, &Globals.bWinbindRefreshTickets, NULL, NULL, FLAG_ADVANCED}, 
	{"winbind offline logon", P_BOOL, P_GLOBAL, &Globals.bWinbindOfflineLogon, NULL, NULL, FLAG_ADVANCED},

	{NULL,  P_BOOL,  P_NONE,  NULL,  NULL,  NULL,  0}
};

/***************************************************************************
 Initialise the sDefault parameter structure for the printer values.
***************************************************************************/

static void init_printer_values(service *pService)
{
	/* choose defaults depending on the type of printing */
	switch (pService->iPrinting) {
		case PRINT_BSD:
		case PRINT_AIX:
		case PRINT_LPRNT:
		case PRINT_LPROS2:
			string_set(&pService->szLpqcommand, "lpq -P'%p'");
			string_set(&pService->szLprmcommand, "lprm -P'%p' %j");
			string_set(&pService->szPrintcommand, "lpr -r -P'%p' %s");
			break;

		case PRINT_LPRNG:
		case PRINT_PLP:
			string_set(&pService->szLpqcommand, "lpq -P'%p'");
			string_set(&pService->szLprmcommand, "lprm -P'%p' %j");
			string_set(&pService->szPrintcommand, "lpr -r -P'%p' %s");
			string_set(&pService->szQueuepausecommand, "lpc stop '%p'");
			string_set(&pService->szQueueresumecommand, "lpc start '%p'");
			string_set(&pService->szLppausecommand, "lpc hold '%p' %j");
			string_set(&pService->szLpresumecommand, "lpc release '%p' %j");
			break;

		case PRINT_CUPS:
		case PRINT_IPRINT:
#ifdef HAVE_CUPS
			/* set the lpq command to contain the destination printer
			   name only.  This is used by cups_queue_get() */
			string_set(&pService->szLpqcommand, "%p");
			string_set(&pService->szLprmcommand, "");
			string_set(&pService->szPrintcommand, "");
			string_set(&pService->szLppausecommand, "");
			string_set(&pService->szLpresumecommand, "");
			string_set(&pService->szQueuepausecommand, "");
			string_set(&pService->szQueueresumecommand, "");
#else
			string_set(&pService->szLpqcommand, "lpq -P'%p'");
			string_set(&pService->szLprmcommand, "lprm -P'%p' %j");
			string_set(&pService->szPrintcommand, "lpr -P'%p' %s; rm %s");
			string_set(&pService->szLppausecommand, "lp -i '%p-%j' -H hold");
			string_set(&pService->szLpresumecommand, "lp -i '%p-%j' -H resume");
			string_set(&pService->szQueuepausecommand, "disable '%p'");
			string_set(&pService->szQueueresumecommand, "enable '%p'");
#endif /* HAVE_CUPS */
			break;

		case PRINT_SYSV:
		case PRINT_HPUX:
			string_set(&pService->szLpqcommand, "lpstat -o%p");
			string_set(&pService->szLprmcommand, "cancel %p-%j");
			string_set(&pService->szPrintcommand, "lp -c -d%p %s; rm %s");
			string_set(&pService->szQueuepausecommand, "disable %p");
			string_set(&pService->szQueueresumecommand, "enable %p");
#ifndef HPUX
			string_set(&pService->szLppausecommand, "lp -i %p-%j -H hold");
			string_set(&pService->szLpresumecommand, "lp -i %p-%j -H resume");
#endif /* HPUX */
			break;

		case PRINT_QNX:
			string_set(&pService->szLpqcommand, "lpq -P%p");
			string_set(&pService->szLprmcommand, "lprm -P%p %j");
			string_set(&pService->szPrintcommand, "lp -r -P%p %s");
			break;

#ifdef DEVELOPER
	case PRINT_TEST:
	case PRINT_VLP:
		string_set(&pService->szPrintcommand, "vlp print %p %s");
		string_set(&pService->szLpqcommand, "vlp lpq %p");
		string_set(&pService->szLprmcommand, "vlp lprm %p %j");
		string_set(&pService->szLppausecommand, "vlp lppause %p %j");
		string_set(&pService->szLpresumecommand, "vlp lpresum %p %j");
		string_set(&pService->szQueuepausecommand, "vlp queuepause %p");
		string_set(&pService->szQueueresumecommand, "vlp queueresume %p");
		break;
#endif /* DEVELOPER */

	}
}

/***************************************************************************
 Initialise the global parameter structure.
***************************************************************************/

static void init_globals(BOOL first_time_only)
{
	static BOOL done_init = False;
	pstring s;

        /* If requested to initialize only once and we've already done it... */
        if (first_time_only && done_init) {
                /* ... then we have nothing more to do */
                return;
        }

	if (!done_init) {
		int i;

		/* The logfile can be set before this is invoked. Free it if so. */
		if (Globals.szLogFile != NULL) {
			string_free(&Globals.szLogFile);
			Globals.szLogFile = NULL;
		}

		memset((void *)&Globals, '\0', sizeof(Globals));

		for (i = 0; parm_table[i].label; i++)
			if ((parm_table[i].type == P_STRING ||
			     parm_table[i].type == P_USTRING) &&
			    parm_table[i].ptr)
				string_set((char **)parm_table[i].ptr, "");

		string_set(&sDefault.fstype, FSTYPE_STRING);

		init_printer_values(&sDefault);

		done_init = True;
	}


	DEBUG(3, ("Initialising global parameters\n"));

	string_set(&Globals.szSMBPasswdFile, dyn_SMB_PASSWD_FILE);
	string_set(&Globals.szPrivateDir, dyn_PRIVATE_DIR);

	/* use the new 'hash2' method by default, with a prefix of 1 */
	string_set(&Globals.szManglingMethod, "hash2");
	Globals.mangle_prefix = 1;

	string_set(&Globals.szGuestaccount, GUEST_ACCOUNT);

	/* using UTF8 by default allows us to support all chars */
	string_set(&Globals.unix_charset, DEFAULT_UNIX_CHARSET);

#if defined(HAVE_NL_LANGINFO) && defined(CODESET)
	/* If the system supports nl_langinfo(), try to grab the value
	   from the user's locale */
	string_set(&Globals.display_charset, "LOCALE");
#else
	string_set(&Globals.display_charset, DEFAULT_DISPLAY_CHARSET);
#endif

	/* Use codepage 850 as a default for the dos character set */
	string_set(&Globals.dos_charset, DEFAULT_DOS_CHARSET);

	/*
	 * Allow the default PASSWD_CHAT to be overridden in local.h.
	 */
	string_set(&Globals.szPasswdChat, DEFAULT_PASSWD_CHAT);
	
	set_global_myname(myhostname());
	string_set(&Globals.szNetbiosName,global_myname());

#ifdef _XBOX
	if (smbWORKGROUP) set_global_myworkgroup(smbWORKGROUP);
	else set_global_myworkgroup("WORKGROUP");
#else if
	set_global_myworkgroup(WORKGROUP);
#endif
	string_set(&Globals.szWorkgroup, lp_workgroup());
	
	string_set(&Globals.szPasswdProgram, "");
	string_set(&Globals.szPidDir, dyn_PIDDIR);
	string_set(&Globals.szLockDir, dyn_LOCKDIR);
	string_set(&Globals.szSocketAddress, "0.0.0.0");
	pstrcpy(s, "Samba ");
	pstrcat(s, SAMBA_VERSION_STRING);
	string_set(&Globals.szServerString, s);
	slprintf(s, sizeof(s) - 1, "%d.%d", DEFAULT_MAJOR_VERSION,
		 DEFAULT_MINOR_VERSION);
	string_set(&Globals.szAnnounceVersion, s);
#ifdef DEVELOPER
	string_set(&Globals.szPanicAction, "/bin/sleep 999999999");
#endif

	pstrcpy(user_socket_options, DEFAULT_SOCKET_OPTIONS);

	string_set(&Globals.szLogonDrive, "");
	/* %N is the NIS auto.home server if -DAUTOHOME is used, else same as %L */
	string_set(&Globals.szLogonHome, "\\\\%N\\%U");
	string_set(&Globals.szLogonPath, "\\\\%N\\%U\\profile");

	string_set(&Globals.szNameResolveOrder, "lmhosts wins host bcast");
	string_set(&Globals.szPasswordServer, "*");

	Globals.AlgorithmicRidBase = BASE_RID;

	Globals.bLoadPrinters = True;
	Globals.PrintcapCacheTime = 750; 	/* 12.5 minutes */

	/* Was 65535 (0xFFFF). 0x4101 matches W2K and causes major speed improvements... */
	/* Discovered by 2 days of pain by Don McCall @ HP :-). */
	Globals.max_xmit = 0x4104;
	Globals.max_mux = 50;	/* This is *needed* for profile support. */
	Globals.lpqcachetime = 30;	/* changed to handle large print servers better -- jerry */
	Globals.bDisableSpoolss = False;
	Globals.iMaxSmbdProcesses = 0;/* no limit specified */
	Globals.pwordlevel = 0;
	Globals.unamelevel = 0;
	Globals.deadtime = 0;
	Globals.bLargeReadwrite = True;
	Globals.max_log_size = 5000;
	Globals.max_open_files = MAX_OPEN_FILES;
	Globals.open_files_db_hash_size = SMB_OPEN_DATABASE_TDB_HASH_SIZE;
	Globals.maxprotocol = PROTOCOL_NT1;
	Globals.minprotocol = PROTOCOL_CORE;
	Globals.security = SEC_USER;
	Globals.paranoid_server_security = True;
	Globals.bEncryptPasswords = True;
	Globals.bUpdateEncrypt = False;
	Globals.clientSchannel = Auto;
	Globals.serverSchannel = Auto;
	Globals.bReadRaw = True;
	Globals.bWriteRaw = True;
	Globals.bReadbmpx = False;
	Globals.bNullPasswords = False;
	Globals.bObeyPamRestrictions = False;
	Globals.syslog = 1;
	Globals.bSyslogOnly = False;
	Globals.bTimestampLogs = True;
	string_set(&Globals.szLogLevel, "0");
	Globals.bDebugHiresTimestamp = False;
	Globals.bDebugPid = False;
	Globals.bDebugUid = False;
	Globals.bEnableCoreFiles = True;
	Globals.max_ttl = 60 * 60 * 24 * 3;	/* 3 days default. */
	Globals.max_wins_ttl = 60 * 60 * 24 * 6;	/* 6 days default. */
	Globals.min_wins_ttl = 60 * 60 * 6;	/* 6 hours default. */
	Globals.machine_password_timeout = 60 * 60 * 24 * 7;	/* 7 days default. */
	Globals.bKernelChangeNotify = True;	/* On if we have it. */
	Globals.bFamChangeNotify = True;	/* On if we have it. */
	Globals.lm_announce = 2;	/* = Auto: send only if LM clients found */
	Globals.lm_interval = 60;
	Globals.announce_as = ANNOUNCE_AS_NT_SERVER;
#if (defined(HAVE_NETGROUP) && defined(WITH_AUTOMOUNT))
	Globals.bNISHomeMap = False;
#ifdef WITH_NISPLUS_HOME
	string_set(&Globals.szNISHomeMapName, "auto_home.org_dir");
#else
	string_set(&Globals.szNISHomeMapName, "auto.home");
#endif
#endif
	Globals.bTimeServer = False;
	Globals.bBindInterfacesOnly = False;
	Globals.bUnixPasswdSync = False;
	Globals.bPamPasswordChange = False;
	Globals.bPasswdChatDebug = False;
	Globals.iPasswdChatTimeout = 2; /* 2 second default. */
	Globals.bNTPipeSupport = True;	/* Do NT pipes by default. */
	Globals.bNTStatusSupport = True; /* Use NT status by default. */
	Globals.bStatCache = True;	/* use stat cache by default */
	Globals.iMaxStatCacheSize = 0;	/* unlimited size in kb by default. */
	Globals.restrict_anonymous = 0;
	Globals.bClientLanManAuth = True;	/* Do use the LanMan hash if it is available */
	Globals.bClientPlaintextAuth = True;	/* Do use a plaintext password if is requested by the server */
	Globals.bLanmanAuth = True;	/* Do use the LanMan hash if it is available */
	Globals.bNTLMAuth = True;	/* Do use NTLMv1 if it is available (otherwise NTLMv2) */
	Globals.bClientNTLMv2Auth = False; /* Client should not use NTLMv2, as we can't tell that the server supports it. */
	/* Note, that we will use NTLM2 session security (which is different), if it is available */

	Globals.map_to_guest = 0;	/* By Default, "Never" */
	Globals.oplock_break_wait_time = 0;	/* By Default, 0 msecs. */
	Globals.enhanced_browsing = True; 
	Globals.iLockSpinCount = 3; /* Try 3 times. */
	Globals.iLockSpinTime = 10; /* usec. */
#ifdef MMAP_BLACKLIST
	Globals.bUseMmap = False;
#else
	Globals.bUseMmap = True;
#endif
	Globals.bUnixExtensions = True;
	Globals.bResetOnZeroVC = False;

	/* hostname lookups can be very expensive and are broken on
	   a large number of sites (tridge) */
	Globals.bHostnameLookups = False;

	string_set(&Globals.szPassdbBackend, "smbpasswd");
	string_set(&Globals.szLdapSuffix, "");
	string_set(&Globals.szLdapMachineSuffix, "");
	string_set(&Globals.szLdapUserSuffix, "");
	string_set(&Globals.szLdapGroupSuffix, "");
	string_set(&Globals.szLdapIdmapSuffix, "");

	string_set(&Globals.szLdapAdminDn, "");
	Globals.ldap_ssl = LDAP_SSL_ON;
	Globals.ldap_passwd_sync = LDAP_PASSWD_SYNC_OFF;
	Globals.ldap_delete_dn = False;
	Globals.ldap_replication_sleep = 1000; /* wait 1 sec for replication */
	Globals.ldap_timeout = LDAP_CONNECT_DEFAULT_TIMEOUT;
	Globals.ldap_page_size = LDAP_PAGE_SIZE;

	/* This is what we tell the afs client. in reality we set the token 
	 * to never expire, though, when this runs out the afs client will 
	 * forget the token. Set to 0 to get NEVERDATE.*/
	Globals.iAfsTokenLifetime = 604800;

/* these parameters are set to defaults that are more appropriate
   for the increasing samba install base:

   as a member of the workgroup, that will possibly become a
   _local_ master browser (lm = True).  this is opposed to a forced
   local master browser startup (pm = True).

   doesn't provide WINS server service by default (wsupp = False),
   and doesn't provide domain master browser services by default, either.

*/

	Globals.bMsAddPrinterWizard = True;
	Globals.bPreferredMaster = Auto;	/* depending on bDomainMaster */
	Globals.os_level = 20;
	Globals.bLocalMaster = True;
	Globals.bDomainMaster = Auto;	/* depending on bDomainLogons */
	Globals.bDomainLogons = False;
	Globals.bBrowseList = True;
	Globals.bWINSsupport = False;
	Globals.bWINSproxy = False;

	Globals.bDNSproxy = True;

	/* this just means to use them if they exist */
	Globals.bKernelOplocks = True;

	Globals.bAllowTrustedDomains = True;

	string_set(&Globals.szTemplateShell, "/bin/false");
	string_set(&Globals.szTemplateHomedir, "/home/%D/%U");
	string_set(&Globals.szWinbindSeparator, "\\");
	string_set(&Globals.szCupsServer, "");
	string_set(&Globals.szIPrintServer, "");

	Globals.winbind_cache_time = 300;	/* 5 minutes */
	Globals.bWinbindEnumUsers = False;
	Globals.bWinbindEnumGroups = False;
	Globals.bWinbindUseDefaultDomain = False;
	Globals.bWinbindTrustedDomainsOnly = False;
	Globals.bWinbindNestedGroups = True;
	Globals.szWinbindNssInfo = str_list_make("template", NULL);
	Globals.bWinbindRefreshTickets = False;
	Globals.bWinbindOfflineLogon = False;

	Globals.bPassdbExpandExplicit = False;

	Globals.name_cache_timeout = 660; /* In seconds */

	Globals.bUseSpnego = True;
	Globals.bClientUseSpnego = True;

	Globals.client_signing = Auto;
	Globals.server_signing = False;

	Globals.bDeferSharingViolations = True;
	string_set(&Globals.smb_ports, SMB_PORTS);

	Globals.bEnablePrivileges = True;
	Globals.bHostMSDfs        = True;
	Globals.bASUSupport       = False;
	
	/* User defined shares. */
	pstrcpy(s, dyn_LOCKDIR);
	pstrcat(s, "/usershares");
	string_set(&Globals.szUsersharePath, s);
	string_set(&Globals.szUsershareTemplateShare, "");
	Globals.iUsershareMaxShares = 0;
	/* By default disallow sharing of directories not owned by the sharer. */
	Globals.bUsershareOwnerOnly = True;
	/* By default disallow guest access to usershares. */
	Globals.bUsershareAllowGuests = False;
}

static TALLOC_CTX *lp_talloc;

/******************************************************************* a
 Free up temporary memory - called from the main loop.
********************************************************************/

void lp_TALLOC_FREE(void)
{
	if (!lp_talloc)
		return;
	TALLOC_FREE(lp_talloc);
	lp_talloc = NULL;
}

TALLOC_CTX *tmp_talloc_ctx(void)
{
	if (lp_talloc == NULL) {
		lp_talloc = talloc_init("tmp_talloc_ctx");
	}

	if (lp_talloc == NULL) {
		smb_panic("Could not create temporary talloc context\n");
	}

	return lp_talloc;
}

/*******************************************************************
 Convenience routine to grab string parameters into temporary memory
 and run standard_sub_basic on them. The buffers can be written to by
 callers without affecting the source string.
********************************************************************/

static char *lp_string(const char *s)
{
	char *ret, *tmpstr;

	/* The follow debug is useful for tracking down memory problems
	   especially if you have an inner loop that is calling a lp_*()
	   function that returns a string.  Perhaps this debug should be
	   present all the time? */

#if 0
	DEBUG(10, ("lp_string(%s)\n", s));
#endif

	if (!lp_talloc)
		lp_talloc = talloc_init("lp_talloc");

	tmpstr = alloc_sub_basic(get_current_username(), s);
	if (trim_char(tmpstr, '\"', '\"')) {
		if (strchr(tmpstr,'\"') != NULL) {
			SAFE_FREE(tmpstr);
			tmpstr = alloc_sub_basic(get_current_username(),s);
		}
	}
	ret = talloc_strdup(lp_talloc, tmpstr);
	SAFE_FREE(tmpstr);
			
	return (ret);
}

/*
   In this section all the functions that are used to access the 
   parameters from the rest of the program are defined 
*/

#define FN_GLOBAL_STRING(fn_name,ptr) \
 char *fn_name(void) {return(lp_string(*(char **)(ptr) ? *(char **)(ptr) : ""));}
#define FN_GLOBAL_CONST_STRING(fn_name,ptr) \
 const char *fn_name(void) {return(*(const char **)(ptr) ? *(const char **)(ptr) : "");}
#define FN_GLOBAL_LIST(fn_name,ptr) \
 const char **fn_name(void) {return(*(const char ***)(ptr));}
#define FN_GLOBAL_BOOL(fn_name,ptr) \
 BOOL fn_name(void) {return(*(BOOL *)(ptr));}
#define FN_GLOBAL_CHAR(fn_name,ptr) \
 char fn_name(void) {return(*(char *)(ptr));}
#define FN_GLOBAL_INTEGER(fn_name,ptr) \
 int fn_name(void) {return(*(int *)(ptr));}

#define FN_LOCAL_STRING(fn_name,val) \
 char *fn_name(int i) {return(lp_string((LP_SNUM_OK(i) && ServicePtrs[(i)]->val) ? ServicePtrs[(i)]->val : sDefault.val));}
#define FN_LOCAL_CONST_STRING(fn_name,val) \
 const char *fn_name(int i) {return (const char *)((LP_SNUM_OK(i) && ServicePtrs[(i)]->val) ? ServicePtrs[(i)]->val : sDefault.val);}
#define FN_LOCAL_LIST(fn_name,val) \
 const char **fn_name(int i) {return(const char **)(LP_SNUM_OK(i)? ServicePtrs[(i)]->val : sDefault.val);}
#define FN_LOCAL_BOOL(fn_name,val) \
 BOOL fn_name(int i) {return(LP_SNUM_OK(i)? ServicePtrs[(i)]->val : sDefault.val);}
#define FN_LOCAL_CHAR(fn_name,val) \
 char fn_name(int i) {return(LP_SNUM_OK(i)? ServicePtrs[(i)]->val : sDefault.val);}
#define FN_LOCAL_INTEGER(fn_name,val) \
 int fn_name(int i) {return(LP_SNUM_OK(i)? ServicePtrs[(i)]->val : sDefault.val);}

FN_GLOBAL_STRING(lp_smb_ports, &Globals.smb_ports)
FN_GLOBAL_STRING(lp_dos_charset, &Globals.dos_charset)
FN_GLOBAL_STRING(lp_unix_charset, &Globals.unix_charset)
FN_GLOBAL_STRING(lp_display_charset, &Globals.display_charset)
FN_GLOBAL_STRING(lp_logfile, &Globals.szLogFile)
FN_GLOBAL_STRING(lp_configfile, &Globals.szConfigFile)
FN_GLOBAL_STRING(lp_smb_passwd_file, &Globals.szSMBPasswdFile)
FN_GLOBAL_STRING(lp_private_dir, &Globals.szPrivateDir)
FN_GLOBAL_STRING(lp_serverstring, &Globals.szServerString)
FN_GLOBAL_INTEGER(lp_printcap_cache_time, &Globals.PrintcapCacheTime)
FN_GLOBAL_STRING(lp_addport_cmd, &Globals.szAddPortCommand)
FN_GLOBAL_STRING(lp_enumports_cmd, &Globals.szEnumPortsCommand)
FN_GLOBAL_STRING(lp_addprinter_cmd, &Globals.szAddPrinterCommand)
FN_GLOBAL_STRING(lp_deleteprinter_cmd, &Globals.szDeletePrinterCommand)
FN_GLOBAL_STRING(lp_os2_driver_map, &Globals.szOs2DriverMap)
FN_GLOBAL_STRING(lp_lockdir, &Globals.szLockDir)
FN_GLOBAL_STRING(lp_piddir, &Globals.szPidDir)
FN_GLOBAL_STRING(lp_mangling_method, &Globals.szManglingMethod)
FN_GLOBAL_INTEGER(lp_mangle_prefix, &Globals.mangle_prefix)
FN_GLOBAL_STRING(lp_utmpdir, &Globals.szUtmpDir)
FN_GLOBAL_STRING(lp_wtmpdir, &Globals.szWtmpDir)
FN_GLOBAL_BOOL(lp_utmp, &Globals.bUtmp)
FN_GLOBAL_STRING(lp_rootdir, &Globals.szRootdir)
FN_GLOBAL_STRING(lp_defaultservice, &Globals.szDefaultService)
FN_GLOBAL_STRING(lp_msg_command, &Globals.szMsgCommand)
FN_GLOBAL_STRING(lp_get_quota_command, &Globals.szGetQuota)
FN_GLOBAL_STRING(lp_set_quota_command, &Globals.szSetQuota)
FN_GLOBAL_STRING(lp_auto_services, &Globals.szAutoServices)
FN_GLOBAL_STRING(lp_passwd_program, &Globals.szPasswdProgram)
FN_GLOBAL_STRING(lp_passwd_chat, &Globals.szPasswdChat)
FN_GLOBAL_STRING(lp_passwordserver, &Globals.szPasswordServer)
FN_GLOBAL_STRING(lp_name_resolve_order, &Globals.szNameResolveOrder)
FN_GLOBAL_STRING(lp_realm, &Globals.szRealm)
FN_GLOBAL_CONST_STRING(lp_afs_username_map, &Globals.szAfsUsernameMap)
FN_GLOBAL_INTEGER(lp_afs_token_lifetime, &Globals.iAfsTokenLifetime)
FN_GLOBAL_STRING(lp_log_nt_token_command, &Globals.szLogNtTokenCommand)
FN_GLOBAL_STRING(lp_username_map, &Globals.szUsernameMap)
FN_GLOBAL_CONST_STRING(lp_logon_script, &Globals.szLogonScript)
FN_GLOBAL_CONST_STRING(lp_logon_path, &Globals.szLogonPath)
FN_GLOBAL_CONST_STRING(lp_logon_drive, &Globals.szLogonDrive)
FN_GLOBAL_CONST_STRING(lp_logon_home, &Globals.szLogonHome)
FN_GLOBAL_STRING(lp_remote_announce, &Globals.szRemoteAnnounce)
FN_GLOBAL_STRING(lp_remote_browse_sync, &Globals.szRemoteBrowseSync)
FN_GLOBAL_LIST(lp_wins_server_list, &Globals.szWINSservers)
FN_GLOBAL_LIST(lp_interfaces, &Globals.szInterfaces)
FN_GLOBAL_STRING(lp_socket_address, &Globals.szSocketAddress)
FN_GLOBAL_STRING(lp_nis_home_map_name, &Globals.szNISHomeMapName)
static FN_GLOBAL_STRING(lp_announce_version, &Globals.szAnnounceVersion)
FN_GLOBAL_LIST(lp_netbios_aliases, &Globals.szNetbiosAliases)
FN_GLOBAL_STRING(lp_passdb_backend, &Globals.szPassdbBackend)
FN_GLOBAL_LIST(lp_preload_modules, &Globals.szPreloadModules)
FN_GLOBAL_STRING(lp_panic_action, &Globals.szPanicAction)
FN_GLOBAL_STRING(lp_adduser_script, &Globals.szAddUserScript)
FN_GLOBAL_STRING(lp_renameuser_script, &Globals.szRenameUserScript)
FN_GLOBAL_STRING(lp_deluser_script, &Globals.szDelUserScript)

FN_GLOBAL_CONST_STRING(lp_guestaccount, &Globals.szGuestaccount)
FN_GLOBAL_STRING(lp_addgroup_script, &Globals.szAddGroupScript)
FN_GLOBAL_STRING(lp_delgroup_script, &Globals.szDelGroupScript)
FN_GLOBAL_STRING(lp_addusertogroup_script, &Globals.szAddUserToGroupScript)
FN_GLOBAL_STRING(lp_deluserfromgroup_script, &Globals.szDelUserFromGroupScript)
FN_GLOBAL_STRING(lp_setprimarygroup_script, &Globals.szSetPrimaryGroupScript)

FN_GLOBAL_STRING(lp_addmachine_script, &Globals.szAddMachineScript)

FN_GLOBAL_STRING(lp_shutdown_script, &Globals.szShutdownScript)
FN_GLOBAL_STRING(lp_abort_shutdown_script, &Globals.szAbortShutdownScript)
FN_GLOBAL_STRING(lp_username_map_script, &Globals.szUsernameMapScript)

FN_GLOBAL_STRING(lp_check_password_script, &Globals.szCheckPasswordScript)

FN_GLOBAL_STRING(lp_wins_hook, &Globals.szWINSHook)
FN_GLOBAL_CONST_STRING(lp_template_homedir, &Globals.szTemplateHomedir)
FN_GLOBAL_CONST_STRING(lp_template_shell, &Globals.szTemplateShell)
FN_GLOBAL_CONST_STRING(lp_winbind_separator, &Globals.szWinbindSeparator)
FN_GLOBAL_INTEGER(lp_acl_compatibility, &Globals.iAclCompat)
FN_GLOBAL_BOOL(lp_winbind_enum_users, &Globals.bWinbindEnumUsers)
FN_GLOBAL_BOOL(lp_winbind_enum_groups, &Globals.bWinbindEnumGroups)
FN_GLOBAL_BOOL(lp_winbind_use_default_domain, &Globals.bWinbindUseDefaultDomain)
FN_GLOBAL_BOOL(lp_winbind_trusted_domains_only, &Globals.bWinbindTrustedDomainsOnly)
FN_GLOBAL_BOOL(lp_winbind_nested_groups, &Globals.bWinbindNestedGroups)
FN_GLOBAL_BOOL(lp_winbind_refresh_tickets, &Globals.bWinbindRefreshTickets)
FN_GLOBAL_BOOL(lp_winbind_offline_logon, &Globals.bWinbindOfflineLogon)

FN_GLOBAL_LIST(lp_idmap_backend, &Globals.szIdmapBackend)
FN_GLOBAL_BOOL(lp_passdb_expand_explicit, &Globals.bPassdbExpandExplicit)

FN_GLOBAL_STRING(lp_ldap_suffix, &Globals.szLdapSuffix)
FN_GLOBAL_STRING(lp_ldap_admin_dn, &Globals.szLdapAdminDn)
FN_GLOBAL_INTEGER(lp_ldap_ssl, &Globals.ldap_ssl)
FN_GLOBAL_INTEGER(lp_ldap_passwd_sync, &Globals.ldap_passwd_sync)
FN_GLOBAL_BOOL(lp_ldap_delete_dn, &Globals.ldap_delete_dn)
FN_GLOBAL_INTEGER(lp_ldap_replication_sleep, &Globals.ldap_replication_sleep)
FN_GLOBAL_INTEGER(lp_ldap_timeout, &Globals.ldap_timeout)
FN_GLOBAL_INTEGER(lp_ldap_page_size, &Globals.ldap_page_size)
FN_GLOBAL_STRING(lp_add_share_cmd, &Globals.szAddShareCommand)
FN_GLOBAL_STRING(lp_change_share_cmd, &Globals.szChangeShareCommand)
FN_GLOBAL_STRING(lp_delete_share_cmd, &Globals.szDeleteShareCommand)
FN_GLOBAL_STRING(lp_usershare_path, &Globals.szUsersharePath)
FN_GLOBAL_LIST(lp_usershare_prefix_allow_list, &Globals.szUsersharePrefixAllowList)
FN_GLOBAL_LIST(lp_usershare_prefix_deny_list, &Globals.szUsersharePrefixDenyList)

FN_GLOBAL_LIST(lp_eventlog_list, &Globals.szEventLogs)

FN_GLOBAL_BOOL(lp_usershare_allow_guests, &Globals.bUsershareAllowGuests)
FN_GLOBAL_BOOL(lp_usershare_owner_only, &Globals.bUsershareOwnerOnly)
FN_GLOBAL_BOOL(lp_disable_netbios, &Globals.bDisableNetbios)
FN_GLOBAL_BOOL(lp_reset_on_zero_vc, &Globals.bResetOnZeroVC)
FN_GLOBAL_BOOL(lp_ms_add_printer_wizard, &Globals.bMsAddPrinterWizard)
FN_GLOBAL_BOOL(lp_dns_proxy, &Globals.bDNSproxy)
FN_GLOBAL_BOOL(lp_wins_support, &Globals.bWINSsupport)
FN_GLOBAL_BOOL(lp_we_are_a_wins_server, &Globals.bWINSsupport)
FN_GLOBAL_BOOL(lp_wins_proxy, &Globals.bWINSproxy)
FN_GLOBAL_BOOL(lp_local_master, &Globals.bLocalMaster)
FN_GLOBAL_BOOL(lp_domain_logons, &Globals.bDomainLogons)
FN_GLOBAL_BOOL(lp_load_printers, &Globals.bLoadPrinters)
FN_GLOBAL_BOOL(lp_readbmpx, &Globals.bReadbmpx)
FN_GLOBAL_BOOL(lp_readraw, &Globals.bReadRaw)
FN_GLOBAL_BOOL(lp_large_readwrite, &Globals.bLargeReadwrite)
FN_GLOBAL_BOOL(lp_writeraw, &Globals.bWriteRaw)
FN_GLOBAL_BOOL(lp_null_passwords, &Globals.bNullPasswords)
FN_GLOBAL_BOOL(lp_obey_pam_restrictions, &Globals.bObeyPamRestrictions)
FN_GLOBAL_BOOL(lp_encrypted_passwords, &Globals.bEncryptPasswords)
FN_GLOBAL_BOOL(lp_update_encrypted, &Globals.bUpdateEncrypt)
FN_GLOBAL_INTEGER(lp_client_schannel, &Globals.clientSchannel)
FN_GLOBAL_INTEGER(lp_server_schannel, &Globals.serverSchannel)
FN_GLOBAL_BOOL(lp_syslog_only, &Globals.bSyslogOnly)
FN_GLOBAL_BOOL(lp_timestamp_logs, &Globals.bTimestampLogs)
FN_GLOBAL_BOOL(lp_debug_hires_timestamp, &Globals.bDebugHiresTimestamp)
FN_GLOBAL_BOOL(lp_debug_pid, &Globals.bDebugPid)
FN_GLOBAL_BOOL(lp_debug_uid, &Globals.bDebugUid)
FN_GLOBAL_BOOL(lp_enable_core_files, &Globals.bEnableCoreFiles)
FN_GLOBAL_BOOL(lp_browse_list, &Globals.bBrowseList)
FN_GLOBAL_BOOL(lp_nis_home_map, &Globals.bNISHomeMap)
static FN_GLOBAL_BOOL(lp_time_server, &Globals.bTimeServer)
FN_GLOBAL_BOOL(lp_bind_interfaces_only, &Globals.bBindInterfacesOnly)
FN_GLOBAL_BOOL(lp_pam_password_change, &Globals.bPamPasswordChange)
FN_GLOBAL_BOOL(lp_unix_password_sync, &Globals.bUnixPasswdSync)
FN_GLOBAL_BOOL(lp_passwd_chat_debug, &Globals.bPasswdChatDebug)
FN_GLOBAL_INTEGER(lp_passwd_chat_timeout, &Globals.iPasswdChatTimeout)
FN_GLOBAL_BOOL(lp_nt_pipe_support, &Globals.bNTPipeSupport)
FN_GLOBAL_BOOL(lp_nt_status_support, &Globals.bNTStatusSupport)
FN_GLOBAL_BOOL(lp_stat_cache, &Globals.bStatCache)
FN_GLOBAL_INTEGER(lp_max_stat_cache_size, &Globals.iMaxStatCacheSize)
FN_GLOBAL_BOOL(lp_allow_trusted_domains, &Globals.bAllowTrustedDomains)
FN_GLOBAL_INTEGER(lp_restrict_anonymous, &Globals.restrict_anonymous)
FN_GLOBAL_BOOL(lp_lanman_auth, &Globals.bLanmanAuth)
FN_GLOBAL_BOOL(lp_ntlm_auth, &Globals.bNTLMAuth)
FN_GLOBAL_BOOL(lp_client_plaintext_auth, &Globals.bClientPlaintextAuth)
FN_GLOBAL_BOOL(lp_client_lanman_auth, &Globals.bClientLanManAuth)
FN_GLOBAL_BOOL(lp_client_ntlmv2_auth, &Globals.bClientNTLMv2Auth)
FN_GLOBAL_BOOL(lp_host_msdfs, &Globals.bHostMSDfs)
FN_GLOBAL_BOOL(lp_kernel_oplocks, &Globals.bKernelOplocks)
FN_GLOBAL_BOOL(lp_enhanced_browsing, &Globals.enhanced_browsing)
FN_GLOBAL_BOOL(lp_use_mmap, &Globals.bUseMmap)
FN_GLOBAL_BOOL(lp_unix_extensions, &Globals.bUnixExtensions)
FN_GLOBAL_BOOL(lp_use_spnego, &Globals.bUseSpnego)
FN_GLOBAL_BOOL(lp_client_use_spnego, &Globals.bClientUseSpnego)
FN_GLOBAL_BOOL(lp_hostname_lookups, &Globals.bHostnameLookups)
FN_GLOBAL_BOOL(lp_kernel_change_notify, &Globals.bKernelChangeNotify)
FN_GLOBAL_BOOL(lp_fam_change_notify, &Globals.bFamChangeNotify)
FN_GLOBAL_BOOL(lp_use_kerberos_keytab, &Globals.bUseKerberosKeytab)
FN_GLOBAL_BOOL(lp_defer_sharing_violations, &Globals.bDeferSharingViolations)
FN_GLOBAL_BOOL(lp_enable_privileges, &Globals.bEnablePrivileges)
FN_GLOBAL_BOOL(lp_enable_asu_support, &Globals.bASUSupport)
FN_GLOBAL_INTEGER(lp_os_level, &Globals.os_level)
FN_GLOBAL_INTEGER(lp_max_ttl, &Globals.max_ttl)
FN_GLOBAL_INTEGER(lp_max_wins_ttl, &Globals.max_wins_ttl)
FN_GLOBAL_INTEGER(lp_min_wins_ttl, &Globals.min_wins_ttl)
FN_GLOBAL_INTEGER(lp_max_log_size, &Globals.max_log_size)
FN_GLOBAL_INTEGER(lp_max_open_files, &Globals.max_open_files)
FN_GLOBAL_INTEGER(lp_open_files_db_hash_size, &Globals.open_files_db_hash_size)
FN_GLOBAL_INTEGER(lp_maxxmit, &Globals.max_xmit)
FN_GLOBAL_INTEGER(lp_maxmux, &Globals.max_mux)
FN_GLOBAL_INTEGER(lp_passwordlevel, &Globals.pwordlevel)
FN_GLOBAL_INTEGER(lp_usernamelevel, &Globals.unamelevel)
FN_GLOBAL_INTEGER(lp_deadtime, &Globals.deadtime)
FN_GLOBAL_INTEGER(lp_maxprotocol, &Globals.maxprotocol)
FN_GLOBAL_INTEGER(lp_minprotocol, &Globals.minprotocol)
FN_GLOBAL_INTEGER(lp_security, &Globals.security)
FN_GLOBAL_LIST(lp_auth_methods, &Globals.AuthMethods)
FN_GLOBAL_BOOL(lp_paranoid_server_security, &Globals.paranoid_server_security)
FN_GLOBAL_INTEGER(lp_maxdisksize, &Globals.maxdisksize)
FN_GLOBAL_INTEGER(lp_lpqcachetime, &Globals.lpqcachetime)
FN_GLOBAL_INTEGER(lp_max_smbd_processes, &Globals.iMaxSmbdProcesses)
FN_GLOBAL_INTEGER(_lp_disable_spoolss, &Globals.bDisableSpoolss)
FN_GLOBAL_INTEGER(lp_syslog, &Globals.syslog)
static FN_GLOBAL_INTEGER(lp_announce_as, &Globals.announce_as)
FN_GLOBAL_INTEGER(lp_lm_announce, &Globals.lm_announce)
FN_GLOBAL_INTEGER(lp_lm_interval, &Globals.lm_interval)
FN_GLOBAL_INTEGER(lp_machine_password_timeout, &Globals.machine_password_timeout)
FN_GLOBAL_INTEGER(lp_map_to_guest, &Globals.map_to_guest)
FN_GLOBAL_INTEGER(lp_oplock_break_wait_time, &Globals.oplock_break_wait_time)
FN_GLOBAL_INTEGER(lp_lock_spin_count, &Globals.iLockSpinCount)
FN_GLOBAL_INTEGER(lp_lock_sleep_time, &Globals.iLockSpinTime)
FN_GLOBAL_INTEGER(lp_usershare_max_shares, &Globals.iUsershareMaxShares)

FN_LOCAL_STRING(lp_preexec, szPreExec)
FN_LOCAL_STRING(lp_postexec, szPostExec)
FN_LOCAL_STRING(lp_rootpreexec, szRootPreExec)
FN_LOCAL_STRING(lp_rootpostexec, szRootPostExec)
FN_LOCAL_STRING(lp_servicename, szService)
FN_LOCAL_CONST_STRING(lp_const_servicename, szService)
FN_LOCAL_STRING(lp_pathname, szPath)
FN_LOCAL_STRING(lp_dontdescend, szDontdescend)
FN_LOCAL_STRING(lp_username, szUsername)
FN_LOCAL_LIST(lp_invalid_users, szInvalidUsers)
FN_LOCAL_LIST(lp_valid_users, szValidUsers)
FN_LOCAL_LIST(lp_admin_users, szAdminUsers)
FN_GLOBAL_LIST(lp_svcctl_list, &Globals.szServicesList)
FN_LOCAL_STRING(lp_cups_options, szCupsOptions)
FN_GLOBAL_STRING(lp_cups_server, &Globals.szCupsServer)
FN_GLOBAL_STRING(lp_iprint_server, &Globals.szIPrintServer)
FN_LOCAL_STRING(lp_printcommand, szPrintcommand)
FN_LOCAL_STRING(lp_lpqcommand, szLpqcommand)
FN_LOCAL_STRING(lp_lprmcommand, szLprmcommand)
FN_LOCAL_STRING(lp_lppausecommand, szLppausecommand)
FN_LOCAL_STRING(lp_lpresumecommand, szLpresumecommand)
FN_LOCAL_STRING(lp_queuepausecommand, szQueuepausecommand)
FN_LOCAL_STRING(lp_queueresumecommand, szQueueresumecommand)
static FN_LOCAL_STRING(_lp_printername, szPrintername)
FN_LOCAL_LIST(lp_hostsallow, szHostsallow)
FN_LOCAL_LIST(lp_hostsdeny, szHostsdeny)
FN_LOCAL_STRING(lp_magicscript, szMagicScript)
FN_LOCAL_STRING(lp_magicoutput, szMagicOutput)
FN_LOCAL_STRING(lp_comment, comment)
FN_LOCAL_STRING(lp_force_user, force_user)
FN_LOCAL_STRING(lp_force_group, force_group)
FN_LOCAL_LIST(lp_readlist, readlist)
FN_LOCAL_LIST(lp_writelist, writelist)
FN_LOCAL_LIST(lp_printer_admin, printer_admin)
FN_LOCAL_STRING(lp_fstype, fstype)
FN_LOCAL_LIST(lp_vfs_objects, szVfsObjects)
FN_LOCAL_STRING(lp_msdfs_proxy, szMSDfsProxy)
static FN_LOCAL_STRING(lp_volume, volume)
FN_LOCAL_STRING(lp_mangled_map, szMangledMap)
FN_LOCAL_STRING(lp_veto_files, szVetoFiles)
FN_LOCAL_STRING(lp_hide_files, szHideFiles)
FN_LOCAL_STRING(lp_veto_oplocks, szVetoOplockFiles)
FN_LOCAL_BOOL(lp_msdfs_root, bMSDfsRoot)
FN_LOCAL_STRING(lp_aio_write_behind, szAioWriteBehind)
FN_LOCAL_STRING(lp_dfree_command, szDfree)
FN_LOCAL_BOOL(lp_autoloaded, autoloaded)
FN_LOCAL_BOOL(lp_preexec_close, bPreexecClose)
FN_LOCAL_BOOL(lp_rootpreexec_close, bRootpreexecClose)
FN_LOCAL_INTEGER(lp_casesensitive, iCaseSensitive)
FN_LOCAL_BOOL(lp_preservecase, bCasePreserve)
FN_LOCAL_BOOL(lp_shortpreservecase, bShortCasePreserve)
FN_LOCAL_BOOL(lp_hide_dot_files, bHideDotFiles)
FN_LOCAL_BOOL(lp_hide_special_files, bHideSpecialFiles)
FN_LOCAL_BOOL(lp_hideunreadable, bHideUnReadable)
FN_LOCAL_BOOL(lp_hideunwriteable_files, bHideUnWriteableFiles)
FN_LOCAL_BOOL(lp_browseable, bBrowseable)
FN_LOCAL_BOOL(lp_readonly, bRead_only)
FN_LOCAL_BOOL(lp_no_set_dir, bNo_set_dir)
FN_LOCAL_BOOL(lp_guest_ok, bGuest_ok)
FN_LOCAL_BOOL(lp_guest_only, bGuest_only)
FN_LOCAL_BOOL(lp_print_ok, bPrint_ok)
FN_LOCAL_BOOL(lp_map_hidden, bMap_hidden)
FN_LOCAL_BOOL(lp_map_archive, bMap_archive)
FN_LOCAL_BOOL(lp_store_dos_attributes, bStoreDosAttributes)
FN_LOCAL_BOOL(lp_dmapi_support, bDmapiSupport)
FN_LOCAL_BOOL(lp_locking, bLocking)
FN_LOCAL_INTEGER(lp_strict_locking, iStrictLocking)
FN_LOCAL_BOOL(lp_posix_locking, bPosixLocking)
FN_LOCAL_BOOL(lp_share_modes, bShareModes)
FN_LOCAL_BOOL(lp_oplocks, bOpLocks)
FN_LOCAL_BOOL(lp_level2_oplocks, bLevel2OpLocks)
FN_LOCAL_BOOL(lp_onlyuser, bOnlyUser)
FN_LOCAL_BOOL(lp_manglednames, bMangledNames)
FN_LOCAL_BOOL(lp_widelinks, bWidelinks)
FN_LOCAL_BOOL(lp_symlinks, bSymlinks)
FN_LOCAL_BOOL(lp_syncalways, bSyncAlways)
FN_LOCAL_BOOL(lp_strict_allocate, bStrictAllocate)
FN_LOCAL_BOOL(lp_strict_sync, bStrictSync)
FN_LOCAL_BOOL(lp_map_system, bMap_system)
FN_LOCAL_BOOL(lp_delete_readonly, bDeleteReadonly)
FN_LOCAL_BOOL(lp_fake_oplocks, bFakeOplocks)
FN_LOCAL_BOOL(lp_recursive_veto_delete, bDeleteVetoFiles)
FN_LOCAL_BOOL(lp_dos_filemode, bDosFilemode)
FN_LOCAL_BOOL(lp_dos_filetimes, bDosFiletimes)
FN_LOCAL_BOOL(lp_dos_filetime_resolution, bDosFiletimeResolution)
FN_LOCAL_BOOL(lp_fake_dir_create_times, bFakeDirCreateTimes)
FN_LOCAL_BOOL(lp_blocking_locks, bBlockingLocks)
FN_LOCAL_BOOL(lp_inherit_perms, bInheritPerms)
FN_LOCAL_BOOL(lp_inherit_acls, bInheritACLS)
FN_LOCAL_BOOL(lp_inherit_owner, bInheritOwner)
FN_LOCAL_BOOL(lp_use_client_driver, bUseClientDriver)
FN_LOCAL_BOOL(lp_default_devmode, bDefaultDevmode)
FN_LOCAL_BOOL(lp_force_printername, bForcePrintername)
FN_LOCAL_BOOL(lp_nt_acl_support, bNTAclSupport)
FN_LOCAL_BOOL(lp_force_unknown_acl_user, bForceUnknownAclUser)
FN_LOCAL_BOOL(lp_ea_support, bEASupport)
FN_LOCAL_BOOL(_lp_use_sendfile, bUseSendfile)
FN_LOCAL_BOOL(lp_profile_acls, bProfileAcls)
FN_LOCAL_BOOL(lp_map_acl_inherit, bMap_acl_inherit)
FN_LOCAL_BOOL(lp_afs_share, bAfs_Share)
FN_LOCAL_BOOL(lp_acl_check_permissions, bAclCheckPermissions)
FN_LOCAL_BOOL(lp_acl_group_control, bAclGroupControl)
FN_LOCAL_BOOL(lp_acl_map_full_control, bAclMapFullControl)
FN_LOCAL_INTEGER(lp_create_mask, iCreate_mask)
FN_LOCAL_INTEGER(lp_force_create_mode, iCreate_force_mode)
FN_LOCAL_INTEGER(lp_security_mask, iSecurity_mask)
FN_LOCAL_INTEGER(lp_force_security_mode, iSecurity_force_mode)
FN_LOCAL_INTEGER(lp_dir_mask, iDir_mask)
FN_LOCAL_INTEGER(lp_force_dir_mode, iDir_force_mode)
FN_LOCAL_INTEGER(lp_dir_security_mask, iDir_Security_mask)
FN_LOCAL_INTEGER(lp_force_dir_security_mode, iDir_Security_force_mode)
FN_LOCAL_INTEGER(lp_max_connections, iMaxConnections)
FN_LOCAL_INTEGER(lp_defaultcase, iDefaultCase)
FN_LOCAL_INTEGER(lp_minprintspace, iMinPrintSpace)
FN_LOCAL_INTEGER(lp_printing, iPrinting)
FN_LOCAL_INTEGER(lp_max_reported_jobs, iMaxReportedPrintJobs)
FN_LOCAL_INTEGER(lp_oplock_contention_limit, iOplockContentionLimit)
FN_LOCAL_INTEGER(lp_csc_policy, iCSCPolicy)
FN_LOCAL_INTEGER(lp_write_cache_size, iWriteCacheSize)
FN_LOCAL_INTEGER(lp_block_size, iBlock_size)
FN_LOCAL_INTEGER(lp_dfree_cache_time, iDfreeCacheTime)
FN_LOCAL_INTEGER(lp_allocation_roundup_size, iallocation_roundup_size)
FN_LOCAL_INTEGER(lp_aio_read_size, iAioReadSize)
FN_LOCAL_INTEGER(lp_aio_write_size, iAioWriteSize)
FN_LOCAL_INTEGER(lp_map_readonly, iMap_readonly)
FN_LOCAL_INTEGER(lp_change_notify_timeout, ichange_notify_timeout)
FN_LOCAL_CHAR(lp_magicchar, magic_char)
FN_GLOBAL_INTEGER(lp_winbind_cache_time, &Globals.winbind_cache_time)
FN_GLOBAL_LIST(lp_winbind_nss_info, &Globals.szWinbindNssInfo)
FN_GLOBAL_INTEGER(lp_algorithmic_rid_base, &Globals.AlgorithmicRidBase)
FN_GLOBAL_INTEGER(lp_name_cache_timeout, &Globals.name_cache_timeout)
FN_GLOBAL_INTEGER(lp_client_signing, &Globals.client_signing)
FN_GLOBAL_INTEGER(lp_server_signing, &Globals.server_signing)

/* local prototypes */

static int map_parameter(const char *pszParmName);
static BOOL set_boolean(BOOL *pb, const char *pszParmValue);
static int getservicebyname(const char *pszServiceName,
			    service * pserviceDest);
static void copy_service(service * pserviceDest,
			 service * pserviceSource, BOOL *pcopymapDest);
static BOOL service_ok(int iService);
static BOOL do_parameter(const char *pszParmName, const char *pszParmValue);
static BOOL do_section(const char *pszSectionName);
static void init_copymap(service * pservice);
static BOOL hash_a_service(const char *name, int number);
static void free_service_byindex(int iService);
static char * canonicalize_servicename(const char *name);

/* This is a helper function for parametrical options support. */
/* It returns a pointer to parametrical option value if it exists or NULL otherwise */
/* Actual parametrical functions are quite simple */
static param_opt_struct *get_parametrics(int snum, const char *type, const char *option)
{
	BOOL global_section = False;
	char* param_key;
        param_opt_struct *data;
	
	if (snum >= iNumServices) return NULL;
	
	if (snum < 0) { 
		data = Globals.param_opt;
		global_section = True;
	} else {
		data = ServicePtrs[snum]->param_opt;
	}
    
	asprintf(&param_key, "%s:%s", type, option);
	if (!param_key) {
		DEBUG(0,("asprintf failed!\n"));
		return NULL;
	}

	while (data) {
		if (strcmp(data->key, param_key) == 0) {
			string_free(&param_key);
			return data;
		}
		data = data->next;
	}

	if (!global_section) {
		/* Try to fetch the same option but from globals */
		/* but only if we are not already working with Globals */
		data = Globals.param_opt;
		while (data) {
		        if (strcmp(data->key, param_key) == 0) {
			        string_free(&param_key);
				return data;
			}
			data = data->next;
		}
	}

	string_free(&param_key);
	
	return NULL;
}


#define MISSING_PARAMETER(name) \
    DEBUG(0, ("%s(): value is NULL or empty!\n", #name))

/*******************************************************************
convenience routine to return int parameters.
********************************************************************/
static int lp_int(const char *s)
{

	if (!s || !*s) {
		MISSING_PARAMETER(lp_int);
		return (-1);
	}

	return atoi(s); 
}

/*******************************************************************
convenience routine to return unsigned long parameters.
********************************************************************/
static unsigned long lp_ulong(const char *s)
{

	if (!s || !*s) {
		MISSING_PARAMETER(lp_ulong);
		return (0);
	}

	return strtoul(s, NULL, 10);
}

/*******************************************************************
convenience routine to return boolean parameters.
********************************************************************/
static BOOL lp_bool(const char *s)
{
	BOOL ret = False;

	if (!s || !*s) {
		MISSING_PARAMETER(lp_bool);
		return False;
	}
	
	if (!set_boolean(&ret,s)) {
		DEBUG(0,("lp_bool(%s): value is not boolean!\n",s));
		return False;
	}

	return ret;
}

/*******************************************************************
convenience routine to return enum parameters.
********************************************************************/
static int lp_enum(const char *s,const struct enum_list *_enum)
{
	int i;

	if (!s || !*s || !_enum) {
		MISSING_PARAMETER(lp_enum);
		return (-1);
	}
	
	for (i=0; _enum[i].name; i++) {
		if (strequal(_enum[i].name,s))
			return _enum[i].value;
	}

	DEBUG(0,("lp_enum(%s,enum): value is not in enum_list!\n",s));
	return (-1);
}

#undef MISSING_PARAMETER

/* DO NOT USE lp_parm_string ANYMORE!!!!
 * use lp_parm_const_string or lp_parm_talloc_string
 *
 * lp_parm_string is only used to let old modules find this symbol
 */
#undef lp_parm_string
 char *lp_parm_string(const char *servicename, const char *type, const char *option)
{
	return lp_parm_talloc_string(lp_servicenumber(servicename), type, option, NULL);
}

/* Return parametric option from a given service. Type is a part of option before ':' */
/* Parametric option has following syntax: 'Type: option = value' */
/* the returned value is talloced in lp_talloc */
char *lp_parm_talloc_string(int snum, const char *type, const char *option, const char *def)
{
	param_opt_struct *data = get_parametrics(snum, type, option);
	
	if (data == NULL||data->value==NULL) {
		if (def) {
			return lp_string(def);
		} else {
			return NULL;
		}
	}

	return lp_string(data->value);
}

/* Return parametric option from a given service. Type is a part of option before ':' */
/* Parametric option has following syntax: 'Type: option = value' */
const char *lp_parm_const_string(int snum, const char *type, const char *option, const char *def)
{
	param_opt_struct *data = get_parametrics(snum, type, option);
	
	if (data == NULL||data->value==NULL)
		return def;
		
	return data->value;
}

/* Return parametric option from a given service. Type is a part of option before ':' */
/* Parametric option has following syntax: 'Type: option = value' */

const char **lp_parm_string_list(int snum, const char *type, const char *option, const char **def)
{
	param_opt_struct *data = get_parametrics(snum, type, option);

	if (data == NULL||data->value==NULL)
		return (const char **)def;
		
	if (data->list==NULL) {
		data->list = str_list_make(data->value, NULL);
	}

	return (const char **)data->list;
}

/* Return parametric option from a given service. Type is a part of option before ':' */
/* Parametric option has following syntax: 'Type: option = value' */

int lp_parm_int(int snum, const char *type, const char *option, int def)
{
	param_opt_struct *data = get_parametrics(snum, type, option);
	
	if (data && data->value && *data->value)
		return lp_int(data->value);

	return def;
}

/* Return parametric option from a given service. Type is a part of option before ':' */
/* Parametric option has following syntax: 'Type: option = value' */

unsigned long lp_parm_ulong(int snum, const char *type, const char *option, unsigned long def)
{
	param_opt_struct *data = get_parametrics(snum, type, option);
	
	if (data && data->value && *data->value)
		return lp_ulong(data->value);

	return def;
}

/* Return parametric option from a given service. Type is a part of option before ':' */
/* Parametric option has following syntax: 'Type: option = value' */

BOOL lp_parm_bool(int snum, const char *type, const char *option, BOOL def)
{
	param_opt_struct *data = get_parametrics(snum, type, option);
	
	if (data && data->value && *data->value)
		return lp_bool(data->value);

	return def;
}

/* Return parametric option from a given service. Type is a part of option before ':' */
/* Parametric option has following syntax: 'Type: option = value' */

int lp_parm_enum(int snum, const char *type, const char *option,
		 const struct enum_list *_enum, int def)
{
	param_opt_struct *data = get_parametrics(snum, type, option);
	
	if (data && data->value && *data->value && _enum)
		return lp_enum(data->value, _enum);

	return def;
}


/***************************************************************************
 Initialise a service to the defaults.
***************************************************************************/

static void init_service(service * pservice)
{
	memset((char *)pservice, '\0', sizeof(service));
	copy_service(pservice, &sDefault, NULL);
}

/***************************************************************************
 Free the dynamically allocated parts of a service struct.
***************************************************************************/

static void free_service(service *pservice)
{
	int i;
        param_opt_struct *data, *pdata;
	if (!pservice)
		return;

	if (pservice->szService)
		DEBUG(5, ("free_service: Freeing service %s\n",
		       pservice->szService));

	string_free(&pservice->szService);
	SAFE_FREE(pservice->copymap);

	for (i = 0; parm_table[i].label; i++) {
		if ((parm_table[i].type == P_STRING ||
		     parm_table[i].type == P_USTRING) &&
		    parm_table[i].p_class == P_LOCAL)
			string_free((char **)
				    (((char *)pservice) +
				     PTR_DIFF(parm_table[i].ptr, &sDefault)));
		else if (parm_table[i].type == P_LIST &&
			 parm_table[i].p_class == P_LOCAL)
			     str_list_free((char ***)
			     		    (((char *)pservice) +
					     PTR_DIFF(parm_table[i].ptr, &sDefault)));
	}

	data = pservice->param_opt;
	if (data)
		DEBUG(5,("Freeing parametrics:\n"));
	while (data) {
		DEBUG(5,("[%s = %s]\n", data->key, data->value));
		string_free(&data->key);
		string_free(&data->value);
		str_list_free(&data->list);
		pdata = data->next;
		SAFE_FREE(data);
		data = pdata;
	}

	ZERO_STRUCTP(pservice);
}


/***************************************************************************
 remove a service indexed in the ServicePtrs array from the ServiceHash
 and free the dynamically allocated parts
***************************************************************************/

static void free_service_byindex(int idx)
{
	if ( !LP_SNUM_OK(idx) ) 
		return;

	ServicePtrs[idx]->valid = False;
	invalid_services[num_invalid_services++] = idx;

	/* we have to cleanup the hash record */

	if (ServicePtrs[idx]->szService) {
		char *canon_name = canonicalize_servicename( ServicePtrs[idx]->szService );
		
		tdb_delete_bystring(ServiceHash, canon_name );
	}

	free_service(ServicePtrs[idx]);
}

/***************************************************************************
 Add a new service to the services array initialising it with the given 
 service. 
***************************************************************************/

static int add_a_service(const service *pservice, const char *name)
{
	int i;
	service tservice;
	int num_to_alloc = iNumServices + 1;
	param_opt_struct *data, *pdata;

	tservice = *pservice;

	/* it might already exist */
	if (name) {
		i = getservicebyname(name, NULL);
		if (i >= 0) {
			/* Clean all parametric options for service */
			/* They will be added during parsing again */
			data = ServicePtrs[i]->param_opt;
			while (data) {
				string_free(&data->key);
				string_free(&data->value);
				str_list_free(&data->list);
				pdata = data->next;
				SAFE_FREE(data);
				data = pdata;
			}
			ServicePtrs[i]->param_opt = NULL;
			return (i);
		}
	}

	/* find an invalid one */
	i = iNumServices;
	if (num_invalid_services > 0) {
		i = invalid_services[--num_invalid_services];
	}

	/* if not, then create one */
	if (i == iNumServices) {
		service **tsp;
		int *tinvalid;
		
		tsp = SMB_REALLOC_ARRAY_KEEP_OLD_ON_ERROR(ServicePtrs, service *, num_to_alloc);
		if (tsp == NULL) {
			DEBUG(0,("add_a_service: failed to enlarge ServicePtrs!\n"));
			return (-1);
		}
		ServicePtrs = tsp;
		ServicePtrs[iNumServices] = SMB_MALLOC_P(service);
		if (!ServicePtrs[iNumServices]) {
			DEBUG(0,("add_a_service: out of memory!\n"));
			return (-1);
		}
		iNumServices++;

		/* enlarge invalid_services here for now... */
		tinvalid = SMB_REALLOC_ARRAY_KEEP_OLD_ON_ERROR(invalid_services, int,
					     num_to_alloc);
		if (tinvalid == NULL) {
			DEBUG(0,("add_a_service: failed to enlarge "
				 "invalid_services!\n"));
			return (-1);
		}
		invalid_services = tinvalid;
	} else {
		free_service_byindex(i);
	}

	ServicePtrs[i]->valid = True;

	init_service(ServicePtrs[i]);
	copy_service(ServicePtrs[i], &tservice, NULL);
	if (name)
		string_set(&ServicePtrs[i]->szService, name);
		
	DEBUG(8,("add_a_service: Creating snum = %d for %s\n", 
		i, ServicePtrs[i]->szService));

	if (!hash_a_service(ServicePtrs[i]->szService, i)) {
		return (-1);
	}
		
	return (i);
}

/***************************************************************************
  Convert a string to uppercase and remove whitespaces.
***************************************************************************/

static char *canonicalize_servicename(const char *src)
{
	static fstring canon; /* is fstring large enough? */

	if ( !src ) {
		DEBUG(0,("canonicalize_servicename: NULL source name!\n"));
		return NULL;
	}

	fstrcpy( canon, src );
	strlower_m( canon );

	return canon;
}

/***************************************************************************
  Add a name/index pair for the services array to the hash table.
***************************************************************************/

static BOOL hash_a_service(const char *name, int idx)
{
	char *canon_name;

	if ( !ServiceHash ) {
		DEBUG(10,("hash_a_service: creating tdb servicehash\n"));
		ServiceHash = tdb_open("servicehash", 1031, TDB_INTERNAL, 
                                        (O_RDWR|O_CREAT), 0600);
		if ( !ServiceHash ) {
			DEBUG(0,("hash_a_service: open tdb servicehash failed!\n"));
			return False;
		}
	}

	DEBUG(10,("hash_a_service: hashing index %d for service name %s\n",
		idx, name));

	if ( !(canon_name = canonicalize_servicename( name )) )
		return False;

        tdb_store_int32(ServiceHash, canon_name, idx);

	return True;
}

/***************************************************************************
 Add a new home service, with the specified home directory, defaults coming 
 from service ifrom.
***************************************************************************/

BOOL lp_add_home(const char *pszHomename, int iDefaultService, 
		 const char *user, const char *pszHomedir)
{
	int i;
	pstring newHomedir;

	i = add_a_service(ServicePtrs[iDefaultService], pszHomename);

	if (i < 0)
		return (False);

	if (!(*(ServicePtrs[iDefaultService]->szPath))
	    || strequal(ServicePtrs[iDefaultService]->szPath, lp_pathname(GLOBAL_SECTION_SNUM))) {
		pstrcpy(newHomedir, pszHomedir);
		string_set(&ServicePtrs[i]->szPath, newHomedir);
	} 

	if (!(*(ServicePtrs[i]->comment))) {
		pstring comment;
		slprintf(comment, sizeof(comment) - 1,
			 "Home directory of %s", user);
		string_set(&ServicePtrs[i]->comment, comment);
	}

	/* set the browseable flag from the global default */

	ServicePtrs[i]->bBrowseable = sDefault.bBrowseable;

	ServicePtrs[i]->autoloaded = True;

	DEBUG(3, ("adding home's share [%s] for user '%s' at '%s'\n", pszHomename, 
	       user, ServicePtrs[i]->szPath ));
	
	return (True);
}

/***************************************************************************
 Add a new service, based on an old one.
***************************************************************************/

int lp_add_service(const char *pszService, int iDefaultService)
{
	return (add_a_service(ServicePtrs[iDefaultService], pszService));
}

/***************************************************************************
 Add the IPC service.
***************************************************************************/

static BOOL lp_add_ipc(const char *ipc_name, BOOL guest_ok)
{
	pstring comment;
	int i = add_a_service(&sDefault, ipc_name);

	if (i < 0)
		return (False);

	slprintf(comment, sizeof(comment) - 1,
		 "IPC Service (%s)", Globals.szServerString);

	string_set(&ServicePtrs[i]->szPath, tmpdir());
	string_set(&ServicePtrs[i]->szUsername, "");
	string_set(&ServicePtrs[i]->comment, comment);
	string_set(&ServicePtrs[i]->fstype, "IPC");
	ServicePtrs[i]->iMaxConnections = 0;
	ServicePtrs[i]->bAvailable = True;
	ServicePtrs[i]->bRead_only = True;
	ServicePtrs[i]->bGuest_only = False;
	ServicePtrs[i]->bGuest_ok = guest_ok;
	ServicePtrs[i]->bPrint_ok = False;
	ServicePtrs[i]->bBrowseable = sDefault.bBrowseable;

	DEBUG(3, ("adding IPC service\n"));

	return (True);
}

/***************************************************************************
 Add a new printer service, with defaults coming from service iFrom.
***************************************************************************/

BOOL lp_add_printer(const char *pszPrintername, int iDefaultService)
{
	const char *comment = "From Printcap";
	int i = add_a_service(ServicePtrs[iDefaultService], pszPrintername);

	if (i < 0)
		return (False);

	/* note that we do NOT default the availability flag to True - */
	/* we take it from the default service passed. This allows all */
	/* dynamic printers to be disabled by disabling the [printers] */
	/* entry (if/when the 'available' keyword is implemented!).    */

	/* the printer name is set to the service name. */
	string_set(&ServicePtrs[i]->szPrintername, pszPrintername);
	string_set(&ServicePtrs[i]->comment, comment);

	/* set the browseable flag from the gloabl default */
	ServicePtrs[i]->bBrowseable = sDefault.bBrowseable;

	/* Printers cannot be read_only. */
	ServicePtrs[i]->bRead_only = False;
	/* No share modes on printer services. */
	ServicePtrs[i]->bShareModes = False;
	/* No oplocks on printer services. */
	ServicePtrs[i]->bOpLocks = False;
	/* Printer services must be printable. */
	ServicePtrs[i]->bPrint_ok = True;
	
	DEBUG(3, ("adding printer service %s\n", pszPrintername));

	return (True);
}

/***************************************************************************
 Map a parameter's string representation to something we can use. 
 Returns False if the parameter string is not recognised, else TRUE.
***************************************************************************/

static int map_parameter(const char *pszParmName)
{
	int iIndex;

	if (*pszParmName == '-')
		return (-1);

	for (iIndex = 0; parm_table[iIndex].label; iIndex++)
		if (strwicmp(parm_table[iIndex].label, pszParmName) == 0)
			return (iIndex);

	/* Warn only if it isn't parametric option */
	if (strchr(pszParmName, ':') == NULL)
		DEBUG(0, ("Unknown parameter encountered: \"%s\"\n", pszParmName));
	/* We do return 'fail' for parametric options as well because they are
	   stored in different storage
	 */
	return (-1);
}

/***************************************************************************
 Show all parameter's name, type, [values,] and flags.
***************************************************************************/

void show_parameter_list(void)
{
	int classIndex, parmIndex, enumIndex, flagIndex;
	BOOL hadFlag;
	const char *section_names[] = { "local", "global", NULL};
	const char *type[] = { "P_BOOL", "P_BOOLREV", "P_CHAR", "P_INTEGER",
		"P_OCTAL", "P_LIST", "P_STRING", "P_USTRING", "P_GSTRING",
		"P_UGSTRING", "P_ENUM", "P_SEP"};
	unsigned flags[] = { FLAG_BASIC, FLAG_SHARE, FLAG_PRINT, FLAG_GLOBAL,
		FLAG_WIZARD, FLAG_ADVANCED, FLAG_DEVELOPER, FLAG_DEPRECATED,
		FLAG_HIDE, FLAG_DOS_STRING};
	const char *flag_names[] = { "FLAG_BASIC", "FLAG_SHARE", "FLAG_PRINT",
		"FLAG_GLOBAL", "FLAG_WIZARD", "FLAG_ADVANCED", "FLAG_DEVELOPER",
		"FLAG_DEPRECATED", "FLAG_HIDE", "FLAG_DOS_STRING", NULL};

	for ( classIndex=0; section_names[classIndex]; classIndex++) {
		printf("[%s]\n", section_names[classIndex]);
		for (parmIndex = 0; parm_table[parmIndex].label; parmIndex++) {
			if (parm_table[parmIndex].p_class == classIndex) {
				printf("%s=%s", 
					parm_table[parmIndex].label,
					type[parm_table[parmIndex].type]);
				switch (parm_table[parmIndex].type) {
				case P_ENUM:
					printf(",");
					for (enumIndex=0; parm_table[parmIndex].enum_list[enumIndex].name; enumIndex++)
						printf("%s%s",
							enumIndex ? "|" : "",
							parm_table[parmIndex].enum_list[enumIndex].name);
					break;
				default:
					break;
				}
				printf(",");
				hadFlag = False;
				for ( flagIndex=0; flag_names[flagIndex]; flagIndex++ ) {
					if (parm_table[parmIndex].flags & flags[flagIndex]) {
						printf("%s%s",
							hadFlag ? "|" : "",
							flag_names[flagIndex]);
						hadFlag = True;
					}
				}
				printf("\n");
			}
		}
	}
}

/***************************************************************************
 Set a boolean variable from the text value stored in the passed string.
 Returns True in success, False if the passed string does not correctly 
 represent a boolean.
***************************************************************************/

static BOOL set_boolean(BOOL *pb, const char *pszParmValue)
{
	BOOL bRetval;

	bRetval = True;
	if (strwicmp(pszParmValue, "yes") == 0 ||
	    strwicmp(pszParmValue, "true") == 0 ||
	    strwicmp(pszParmValue, "1") == 0)
		*pb = True;
	else if (strwicmp(pszParmValue, "no") == 0 ||
		    strwicmp(pszParmValue, "False") == 0 ||
		    strwicmp(pszParmValue, "0") == 0)
		*pb = False;
	else {
		DEBUG(0,
		      ("ERROR: Badly formed boolean in configuration file: \"%s\".\n",
		       pszParmValue));
		bRetval = False;
	}
	return (bRetval);
}

/***************************************************************************
Find a service by name. Otherwise works like get_service.
***************************************************************************/

static int getservicebyname(const char *pszServiceName, service * pserviceDest)
{
	int iService = -1;
	char *canon_name;

	if (ServiceHash != NULL) {
		if ( !(canon_name = canonicalize_servicename( pszServiceName )) )
			return -1;

		iService = tdb_fetch_int32(ServiceHash, canon_name );

		if (LP_SNUM_OK(iService)) {
			if (pserviceDest != NULL) {
				copy_service(pserviceDest, ServicePtrs[iService], NULL);
			}
		} else {
			iService = -1;
		}
	}

	return (iService);
}

/***************************************************************************
 Copy a service structure to another.
 If pcopymapDest is NULL then copy all fields
***************************************************************************/

static void copy_service(service * pserviceDest, service * pserviceSource, BOOL *pcopymapDest)
{
	int i;
	BOOL bcopyall = (pcopymapDest == NULL);
	param_opt_struct *data, *pdata, *paramo;
	BOOL not_added;

	for (i = 0; parm_table[i].label; i++)
		if (parm_table[i].ptr && parm_table[i].p_class == P_LOCAL &&
		    (bcopyall || pcopymapDest[i])) {
			void *def_ptr = parm_table[i].ptr;
			void *src_ptr =
				((char *)pserviceSource) + PTR_DIFF(def_ptr,
								    &sDefault);
			void *dest_ptr =
				((char *)pserviceDest) + PTR_DIFF(def_ptr,
								  &sDefault);

			switch (parm_table[i].type) {
				case P_BOOL:
				case P_BOOLREV:
					*(BOOL *)dest_ptr = *(BOOL *)src_ptr;
					break;

				case P_INTEGER:
				case P_ENUM:
				case P_OCTAL:
					*(int *)dest_ptr = *(int *)src_ptr;
					break;

				case P_CHAR:
					*(char *)dest_ptr = *(char *)src_ptr;
					break;

				case P_STRING:
					string_set((char **)dest_ptr,
						   *(char **)src_ptr);
					break;

				case P_USTRING:
					string_set((char **)dest_ptr,
						   *(char **)src_ptr);
					strupper_m(*(char **)dest_ptr);
					break;
				case P_LIST:
					str_list_free((char ***)dest_ptr);
					str_list_copy((char ***)dest_ptr, *(const char ***)src_ptr);
					break;
				default:
					break;
			}
		}

	if (bcopyall) {
		init_copymap(pserviceDest);
		if (pserviceSource->copymap)
			memcpy((void *)pserviceDest->copymap,
			       (void *)pserviceSource->copymap,
			       sizeof(BOOL) * NUMPARAMETERS);
	}
	
	data = pserviceSource->param_opt;
	while (data) {
		not_added = True;
		pdata = pserviceDest->param_opt;
		/* Traverse destination */
		while (pdata) {
			/* If we already have same option, override it */
			if (strcmp(pdata->key, data->key) == 0) {
				string_free(&pdata->value);
				str_list_free(&data->list);
				pdata->value = SMB_STRDUP(data->value);
				not_added = False;
				break;
			}
			pdata = pdata->next;
		}
		if (not_added) {
		    paramo = SMB_XMALLOC_P(param_opt_struct);
		    paramo->key = SMB_STRDUP(data->key);
		    paramo->value = SMB_STRDUP(data->value);
		    paramo->list = NULL;
		    DLIST_ADD(pserviceDest->param_opt, paramo);
		}
		data = data->next;
	}
}

/***************************************************************************
Check a service for consistency. Return False if the service is in any way
incomplete or faulty, else True.
***************************************************************************/

static BOOL service_ok(int iService)
{
	BOOL bRetval;

	bRetval = True;
	if (ServicePtrs[iService]->szService[0] == '\0') {
		DEBUG(0, ("The following message indicates an internal error:\n"));
		DEBUG(0, ("No service name in service entry.\n"));
		bRetval = False;
	}

	/* The [printers] entry MUST be printable. I'm all for flexibility, but */
	/* I can't see why you'd want a non-printable printer service...        */
	if (strwicmp(ServicePtrs[iService]->szService, PRINTERS_NAME) == 0) {
		if (!ServicePtrs[iService]->bPrint_ok) {
			DEBUG(0, ("WARNING: [%s] service MUST be printable!\n",
			       ServicePtrs[iService]->szService));
			ServicePtrs[iService]->bPrint_ok = True;
		}
		/* [printers] service must also be non-browsable. */
		if (ServicePtrs[iService]->bBrowseable)
			ServicePtrs[iService]->bBrowseable = False;
	}

	if (ServicePtrs[iService]->szPath[0] == '\0' &&
	    strwicmp(ServicePtrs[iService]->szService, HOMES_NAME) != 0 &&
	    ServicePtrs[iService]->szMSDfsProxy[0] == '\0'
	    ) {
		DEBUG(0, ("WARNING: No path in service %s - making it unavailable!\n",
			ServicePtrs[iService]->szService));
		ServicePtrs[iService]->bAvailable = False;
	}

	/* If a service is flagged unavailable, log the fact at level 0. */
	if (!ServicePtrs[iService]->bAvailable)
		DEBUG(1, ("NOTE: Service %s is flagged unavailable.\n",
			  ServicePtrs[iService]->szService));

	return (bRetval);
}

static struct file_lists {
	struct file_lists *next;
	char *name;
	char *subfname;
	time_t modtime;
} *file_lists = NULL;

/*******************************************************************
 Keep a linked list of all config files so we know when one has changed 
 it's date and needs to be reloaded.
********************************************************************/

static void add_to_file_list(const char *fname, const char *subfname)
{
	struct file_lists *f = file_lists;

	while (f) {
		if (f->name && !strcmp(f->name, fname))
			break;
		f = f->next;
	}

	if (!f) {
		f = SMB_MALLOC_P(struct file_lists);
		if (!f)
			return;
		f->next = file_lists;
		f->name = SMB_STRDUP(fname);
		if (!f->name) {
			SAFE_FREE(f);
			return;
		}
		f->subfname = SMB_STRDUP(subfname);
		if (!f->subfname) {
			SAFE_FREE(f);
			return;
		}
		file_lists = f;
		f->modtime = file_modtime(subfname);
	} else {
		time_t t = file_modtime(subfname);
		if (t)
			f->modtime = t;
	}
}

/*******************************************************************
 Check if a config file has changed date.
********************************************************************/

BOOL lp_file_list_changed(void)
{
	struct file_lists *f = file_lists;

 	DEBUG(6, ("lp_file_list_changed()\n"));

	while (f) {
		pstring n2;
		time_t mod_time;

		pstrcpy(n2, f->name);
		standard_sub_basic( get_current_username(), n2, sizeof(n2) );

		DEBUGADD(6, ("file %s -> %s  last mod_time: %s\n",
			     f->name, n2, ctime(&f->modtime)));

		mod_time = file_modtime(n2);

		if (mod_time && ((f->modtime != mod_time) || (f->subfname == NULL) || (strcmp(n2, f->subfname) != 0))) {
			DEBUGADD(6,
				 ("file %s modified: %s\n", n2,
				  ctime(&mod_time)));
			f->modtime = mod_time;
			SAFE_FREE(f->subfname);
			f->subfname = SMB_STRDUP(n2);
			return (True);
		}
		f = f->next;
	}
	return (False);
}

/***************************************************************************
 Run standard_sub_basic on netbios name... needed because global_myname
 is not accessed through any lp_ macro.
 Note: We must *NOT* use string_set() here as ptr points to global_myname.
***************************************************************************/

static BOOL handle_netbios_name(int snum, const char *pszParmValue, char **ptr)
{
	BOOL ret;
	pstring netbios_name;

	pstrcpy(netbios_name, pszParmValue);

	standard_sub_basic(get_current_username(), netbios_name,sizeof(netbios_name));

	ret = set_global_myname(netbios_name);
	string_set(&Globals.szNetbiosName,global_myname());
	
	DEBUG(4, ("handle_netbios_name: set global_myname to: %s\n",
	       global_myname()));

	return ret;
}

static BOOL handle_charset(int snum, const char *pszParmValue, char **ptr)
{
	if (strcmp(*ptr, pszParmValue) != 0) {
		string_set(ptr, pszParmValue);
		init_iconv();
	}
	return True;
}



static BOOL handle_workgroup(int snum, const char *pszParmValue, char **ptr)
{
	BOOL ret;
	
	ret = set_global_myworkgroup(pszParmValue);
	string_set(&Globals.szWorkgroup,lp_workgroup());
	
	return ret;
}

static BOOL handle_netbios_scope(int snum, const char *pszParmValue, char **ptr)
{
	BOOL ret;
	
	ret = set_global_scope(pszParmValue);
	string_set(&Globals.szNetbiosScope,global_scope());

	return ret;
}

static BOOL handle_netbios_aliases(int snum, const char *pszParmValue, char **ptr)
{
	str_list_free(&Globals.szNetbiosAliases);
	Globals.szNetbiosAliases = str_list_make(pszParmValue, NULL);
	return set_netbios_aliases((const char **)Globals.szNetbiosAliases);
}

/***************************************************************************
 Handle the include operation.
***************************************************************************/

static BOOL handle_include(int snum, const char *pszParmValue, char **ptr)
{
	pstring fname;
	pstrcpy(fname, pszParmValue);

	standard_sub_basic(get_current_username(), fname,sizeof(fname));

	add_to_file_list(pszParmValue, fname);

	string_set(ptr, fname);

	if (file_exist(fname, NULL))
		return (pm_process(fname, do_section, do_parameter));

	DEBUG(2, ("Can't find include file %s\n", fname));

	return (False);
}

/***************************************************************************
 Handle the interpretation of the copy parameter.
***************************************************************************/

static BOOL handle_copy(int snum, const char *pszParmValue, char **ptr)
{
	BOOL bRetval;
	int iTemp;
	service serviceTemp;

	string_set(ptr, pszParmValue);

	init_service(&serviceTemp);

	bRetval = False;

	DEBUG(3, ("Copying service from service %s\n", pszParmValue));

	if ((iTemp = getservicebyname(pszParmValue, &serviceTemp)) >= 0) {
		if (iTemp == iServiceIndex) {
			DEBUG(0, ("Can't copy service %s - unable to copy self!\n", pszParmValue));
		} else {
			copy_service(ServicePtrs[iServiceIndex],
				     &serviceTemp,
				     ServicePtrs[iServiceIndex]->copymap);
			bRetval = True;
		}
	} else {
		DEBUG(0, ("Unable to copy service - source not found: %s\n", pszParmValue));
		bRetval = False;
	}

	free_service(&serviceTemp);
	return (bRetval);
}

/***************************************************************************
 Handle idmap/non unix account uid and gid allocation parameters.  The format of these
 parameters is:

 [global]

        idmap uid = 1000-1999
        idmap gid = 700-899

 We only do simple parsing checks here.  The strings are parsed into useful
 structures in the idmap daemon code.

***************************************************************************/

/* Some lp_ routines to return idmap [ug]id information */

static uid_t idmap_uid_low, idmap_uid_high;
static gid_t idmap_gid_low, idmap_gid_high;

BOOL lp_idmap_uid(uid_t *low, uid_t *high)
{
        if (idmap_uid_low == 0 || idmap_uid_high == 0)
                return False;

        if (low)
                *low = idmap_uid_low;

        if (high)
                *high = idmap_uid_high;

        return True;
}

BOOL lp_idmap_gid(gid_t *low, gid_t *high)
{
        if (idmap_gid_low == 0 || idmap_gid_high == 0)
                return False;

        if (low)
                *low = idmap_gid_low;

        if (high)
                *high = idmap_gid_high;

        return True;
}

/* Do some simple checks on "idmap [ug]id" parameter values */

static BOOL handle_idmap_uid(int snum, const char *pszParmValue, char **ptr)
{
	uint32 low, high;

	if (sscanf(pszParmValue, "%u - %u", &low, &high) != 2 || high < low)
		return False;

	/* Parse OK */

	string_set(ptr, pszParmValue);

        idmap_uid_low = low;
        idmap_uid_high = high;

	return True;
}

static BOOL handle_idmap_gid(int snum, const char *pszParmValue, char **ptr)
{
	uint32 low, high;

	if (sscanf(pszParmValue, "%u - %u", &low, &high) != 2 || high < low)
		return False;

	/* Parse OK */

	string_set(ptr, pszParmValue);

        idmap_gid_low = low;
        idmap_gid_high = high;

	return True;
}

/***************************************************************************
 Handle the DEBUG level list.
***************************************************************************/

static BOOL handle_debug_list( int snum, const char *pszParmValueIn, char **ptr )
{
	pstring pszParmValue;

	pstrcpy(pszParmValue, pszParmValueIn);
	string_set(ptr, pszParmValueIn);
	return debug_parse_levels( pszParmValue );
}

/***************************************************************************
 Handle ldap suffixes - default to ldapsuffix if sub-suffixes are not defined.
***************************************************************************/

static const char *append_ldap_suffix( const char *str )
{
	const char *suffix_string;


	if (!lp_talloc)
		lp_talloc = talloc_init("lp_talloc");

	suffix_string = talloc_asprintf( lp_talloc, "%s,%s", str, Globals.szLdapSuffix );
	if ( !suffix_string ) {
		DEBUG(0,("append_ldap_suffix: talloc_asprintf() failed!\n"));
		return "";
	}

	return suffix_string;
}

const char *lp_ldap_machine_suffix(void)
{
	if (Globals.szLdapMachineSuffix[0])
		return append_ldap_suffix(Globals.szLdapMachineSuffix);

	return lp_string(Globals.szLdapSuffix);
}

const char *lp_ldap_user_suffix(void)
{
	if (Globals.szLdapUserSuffix[0])
		return append_ldap_suffix(Globals.szLdapUserSuffix);

	return lp_string(Globals.szLdapSuffix);
}

const char *lp_ldap_group_suffix(void)
{
	if (Globals.szLdapGroupSuffix[0])
		return append_ldap_suffix(Globals.szLdapGroupSuffix);

	return lp_string(Globals.szLdapSuffix);
}

const char *lp_ldap_idmap_suffix(void)
{
	if (Globals.szLdapIdmapSuffix[0])
		return append_ldap_suffix(Globals.szLdapIdmapSuffix);

	return lp_string(Globals.szLdapSuffix);
}

/****************************************************************************
 set the value for a P_ENUM
 ***************************************************************************/

static void lp_set_enum_parm( struct parm_struct *parm, const char *pszParmValue,
                              int *ptr )
{
	int i;

	for (i = 0; parm->enum_list[i].name; i++) {
		if ( strequal(pszParmValue, parm->enum_list[i].name)) {
			*ptr = parm->enum_list[i].value;
			break;
		}
	}
}

/***************************************************************************
***************************************************************************/

static BOOL handle_printing(int snum, const char *pszParmValue, char **ptr)
{
	static int parm_num = -1;
	service *s;

	if ( parm_num == -1 )
		parm_num = map_parameter( "printing" );

	lp_set_enum_parm( &parm_table[parm_num], pszParmValue, (int*)ptr );

	if ( snum < 0 )
		s = &sDefault;
	else
		s = ServicePtrs[snum];

	init_printer_values( s );

	return True;
}


/***************************************************************************
 Initialise a copymap.
***************************************************************************/

static void init_copymap(service * pservice)
{
	int i;
	SAFE_FREE(pservice->copymap);
	pservice->copymap = SMB_MALLOC_ARRAY(BOOL,NUMPARAMETERS);
	if (!pservice->copymap)
		DEBUG(0,
		      ("Couldn't allocate copymap!! (size %d)\n",
		       (int)NUMPARAMETERS));
	else
		for (i = 0; i < NUMPARAMETERS; i++)
			pservice->copymap[i] = True;
}

/***************************************************************************
 Return the local pointer to a parameter given the service number and the 
 pointer into the default structure.
***************************************************************************/

void *lp_local_ptr(int snum, void *ptr)
{
	return (void *)(((char *)ServicePtrs[snum]) + PTR_DIFF(ptr, &sDefault));
}

/***************************************************************************
 Process a parameter for a particular service number. If snum < 0
 then assume we are in the globals.
***************************************************************************/

BOOL lp_do_parameter(int snum, const char *pszParmName, const char *pszParmValue)
{
	int parmnum, i, slen;
	void *parm_ptr = NULL;	/* where we are going to store the result */
	void *def_ptr = NULL;
	pstring param_key;
	char *sep;
	param_opt_struct *paramo, *data;
	BOOL not_added;

	parmnum = map_parameter(pszParmName);

	if (parmnum < 0) {
		if ((sep=strchr(pszParmName, ':')) != NULL) {
			*sep = '\0';
			ZERO_STRUCT(param_key);
			pstr_sprintf(param_key, "%s:", pszParmName);
			slen = strlen(param_key);
			pstrcat(param_key, sep+1);
			trim_char(param_key+slen, ' ', ' ');
			not_added = True;
			data = (snum < 0) ? Globals.param_opt : 
				ServicePtrs[snum]->param_opt;
			/* Traverse destination */
			while (data) {
				/* If we already have same option, override it */
				if (strcmp(data->key, param_key) == 0) {
					string_free(&data->value);
					str_list_free(&data->list);
					data->value = SMB_STRDUP(pszParmValue);
					not_added = False;
					break;
				}
				data = data->next;
			}
			if (not_added) {
				paramo = SMB_XMALLOC_P(param_opt_struct);
				paramo->key = SMB_STRDUP(param_key);
				paramo->value = SMB_STRDUP(pszParmValue);
				paramo->list = NULL;
				if (snum < 0) {
					DLIST_ADD(Globals.param_opt, paramo);
				} else {
					DLIST_ADD(ServicePtrs[snum]->param_opt, paramo);
				}
			}

			*sep = ':';
			return (True);
		}
		DEBUG(0, ("Ignoring unknown parameter \"%s\"\n", pszParmName));
		return (True);
	}

	if (parm_table[parmnum].flags & FLAG_DEPRECATED) {
		DEBUG(1, ("WARNING: The \"%s\" option is deprecated\n",
			  pszParmName));
	}

	def_ptr = parm_table[parmnum].ptr;

	/* we might point at a service, the default service or a global */
	if (snum < 0) {
		parm_ptr = def_ptr;
	} else {
		if (parm_table[parmnum].p_class == P_GLOBAL) {
			DEBUG(0,
			      ("Global parameter %s found in service section!\n",
			       pszParmName));
			return (True);
		}
		parm_ptr =
			((char *)ServicePtrs[snum]) + PTR_DIFF(def_ptr,
							    &sDefault);
	}

	if (snum >= 0) {
		if (!ServicePtrs[snum]->copymap)
			init_copymap(ServicePtrs[snum]);

		/* this handles the aliases - set the copymap for other entries with
		   the same data pointer */
		for (i = 0; parm_table[i].label; i++)
			if (parm_table[i].ptr == parm_table[parmnum].ptr)
				ServicePtrs[snum]->copymap[i] = False;
	}

	/* if it is a special case then go ahead */
	if (parm_table[parmnum].special) {
		parm_table[parmnum].special(snum, pszParmValue, (char **)parm_ptr);
		return (True);
	}

	/* now switch on the type of variable it is */
	switch (parm_table[parmnum].type)
	{
		case P_BOOL:
			*(BOOL *)parm_ptr = lp_bool(pszParmValue);
			break;

		case P_BOOLREV:
			*(BOOL *)parm_ptr = !lp_bool(pszParmValue);
			break;

		case P_INTEGER:
			*(int *)parm_ptr = lp_int(pszParmValue);
			break;

		case P_CHAR:
			*(char *)parm_ptr = *pszParmValue;
			break;

		case P_OCTAL:
			i = sscanf(pszParmValue, "%o", (int *)parm_ptr);
			if ( i != 1 ) {
			    DEBUG ( 0, ("Invalid octal number %s\n", pszParmName ));
			}
			break;

		case P_LIST:
			str_list_free((char ***)parm_ptr);
			*(char ***)parm_ptr = str_list_make(pszParmValue, NULL);
			break;

		case P_STRING:
			string_set((char **)parm_ptr, pszParmValue);
			break;

		case P_USTRING:
			string_set((char **)parm_ptr, pszParmValue);
			strupper_m(*(char **)parm_ptr);
			break;

		case P_GSTRING:
			pstrcpy((char *)parm_ptr, pszParmValue);
			break;

		case P_UGSTRING:
			pstrcpy((char *)parm_ptr, pszParmValue);
			strupper_m((char *)parm_ptr);
			break;

		case P_ENUM:
			lp_set_enum_parm( &parm_table[parmnum], pszParmValue, (int*)parm_ptr );
			break;
		case P_SEP:
			break;
	}

	return (True);
}

/***************************************************************************
 Process a parameter.
***************************************************************************/

static BOOL do_parameter(const char *pszParmName, const char *pszParmValue)
{
	if (!bInGlobalSection && bGlobalOnly)
		return (True);

	DEBUGADD(4, ("doing parameter %s = %s\n", pszParmName, pszParmValue));

	return (lp_do_parameter(bInGlobalSection ? -2 : iServiceIndex,
				pszParmName, pszParmValue));
}

/***************************************************************************
 Print a parameter of the specified type.
***************************************************************************/

static void print_parameter(struct parm_struct *p, void *ptr, FILE * f)
{
	int i;
	switch (p->type)
	{
		case P_ENUM:
			for (i = 0; p->enum_list[i].name; i++) {
				if (*(int *)ptr == p->enum_list[i].value) {
					fprintf(f, "%s",
						p->enum_list[i].name);
					break;
				}
			}
			break;

		case P_BOOL:
			fprintf(f, "%s", BOOLSTR(*(BOOL *)ptr));
			break;

		case P_BOOLREV:
			fprintf(f, "%s", BOOLSTR(!*(BOOL *)ptr));
			break;

		case P_INTEGER:
			fprintf(f, "%d", *(int *)ptr);
			break;

		case P_CHAR:
			fprintf(f, "%c", *(char *)ptr);
			break;

		case P_OCTAL:
			fprintf(f, "%s", octal_string(*(int *)ptr));
			break;

		case P_LIST:
			if ((char ***)ptr && *(char ***)ptr) {
				char **list = *(char ***)ptr;
				
				for (; *list; list++) {
					/* surround strings with whitespace in double quotes */
					if ( strchr_m( *list, ' ' ) )
						fprintf(f, "\"%s\"%s", *list, ((*(list+1))?", ":""));
					else
						fprintf(f, "%s%s", *list, ((*(list+1))?", ":""));
				}
			}
			break;

		case P_GSTRING:
		case P_UGSTRING:
			if ((char *)ptr) {
				fprintf(f, "%s", (char *)ptr);
			}
			break;

		case P_STRING:
		case P_USTRING:
			if (*(char **)ptr) {
				fprintf(f, "%s", *(char **)ptr);
			}
			break;
		case P_SEP:
			break;
	}
}

/***************************************************************************
 Check if two parameters are equal.
***************************************************************************/

static BOOL equal_parameter(parm_type type, void *ptr1, void *ptr2)
{
	switch (type) {
		case P_BOOL:
		case P_BOOLREV:
			return (*((BOOL *)ptr1) == *((BOOL *)ptr2));

		case P_INTEGER:
		case P_ENUM:
		case P_OCTAL:
			return (*((int *)ptr1) == *((int *)ptr2));

		case P_CHAR:
			return (*((char *)ptr1) == *((char *)ptr2));
		
		case P_LIST:
			return str_list_compare(*(char ***)ptr1, *(char ***)ptr2);

		case P_GSTRING:
		case P_UGSTRING:
		{
			char *p1 = (char *)ptr1, *p2 = (char *)ptr2;
			if (p1 && !*p1)
				p1 = NULL;
			if (p2 && !*p2)
				p2 = NULL;
			return (p1 == p2 || strequal(p1, p2));
		}
		case P_STRING:
		case P_USTRING:
		{
			char *p1 = *(char **)ptr1, *p2 = *(char **)ptr2;
			if (p1 && !*p1)
				p1 = NULL;
			if (p2 && !*p2)
				p2 = NULL;
			return (p1 == p2 || strequal(p1, p2));
		}
		case P_SEP:
			break;
	}
	return (False);
}

/***************************************************************************
 Initialize any local varients in the sDefault table.
***************************************************************************/

void init_locals(void)
{
	/* None as yet. */
}

/***************************************************************************
 Process a new section (service). At this stage all sections are services.
 Later we'll have special sections that permit server parameters to be set.
 Returns True on success, False on failure. 
***************************************************************************/

static BOOL do_section(const char *pszSectionName)
{
	BOOL bRetval;
	BOOL isglobal = ((strwicmp(pszSectionName, GLOBAL_NAME) == 0) ||
			 (strwicmp(pszSectionName, GLOBAL_NAME2) == 0));
	bRetval = False;

	/* if we were in a global section then do the local inits */
	if (bInGlobalSection && !isglobal)
		init_locals();

	/* if we've just struck a global section, note the fact. */
	bInGlobalSection = isglobal;

	/* check for multiple global sections */
	if (bInGlobalSection) {
		DEBUG(3, ("Processing section \"[%s]\"\n", pszSectionName));
		return (True);
	}

	if (!bInGlobalSection && bGlobalOnly)
		return (True);

	/* if we have a current service, tidy it up before moving on */
	bRetval = True;

	if (iServiceIndex >= 0)
		bRetval = service_ok(iServiceIndex);

	/* if all is still well, move to the next record in the services array */
	if (bRetval) {
		/* We put this here to avoid an odd message order if messages are */
		/* issued by the post-processing of a previous section. */
		DEBUG(2, ("Processing section \"[%s]\"\n", pszSectionName));

		if ((iServiceIndex = add_a_service(&sDefault, pszSectionName))
		    < 0) {
			DEBUG(0, ("Failed to add a new service\n"));
			return (False);
		}
	}

	return (bRetval);
}


/***************************************************************************
 Determine if a partcular base parameter is currentl set to the default value.
***************************************************************************/

static BOOL is_default(int i)
{
	if (!defaults_saved)
		return False;
	switch (parm_table[i].type) {
		case P_LIST:
			return str_list_compare (parm_table[i].def.lvalue, 
						*(char ***)parm_table[i].ptr);
		case P_STRING:
		case P_USTRING:
			return strequal(parm_table[i].def.svalue,
					*(char **)parm_table[i].ptr);
		case P_GSTRING:
		case P_UGSTRING:
			return strequal(parm_table[i].def.svalue,
					(char *)parm_table[i].ptr);
		case P_BOOL:
		case P_BOOLREV:
			return parm_table[i].def.bvalue ==
				*(BOOL *)parm_table[i].ptr;
		case P_CHAR:
			return parm_table[i].def.cvalue ==
				*(char *)parm_table[i].ptr;
		case P_INTEGER:
		case P_OCTAL:
		case P_ENUM:
			return parm_table[i].def.ivalue ==
				*(int *)parm_table[i].ptr;
		case P_SEP:
			break;
	}
	return False;
}

/***************************************************************************
Display the contents of the global structure.
***************************************************************************/

static void dump_globals(FILE *f)
{
	int i;
	param_opt_struct *data;
	
	fprintf(f, "[global]\n");

	for (i = 0; parm_table[i].label; i++)
		if (parm_table[i].p_class == P_GLOBAL &&
		    parm_table[i].ptr &&
		    (i == 0 || (parm_table[i].ptr != parm_table[i - 1].ptr))) {
			if (defaults_saved && is_default(i))
				continue;
			fprintf(f, "\t%s = ", parm_table[i].label);
			print_parameter(&parm_table[i], parm_table[i].ptr, f);
			fprintf(f, "\n");
	}
	if (Globals.param_opt != NULL) {
		data = Globals.param_opt;
		while(data) {
			fprintf(f, "\t%s = %s\n", data->key, data->value);
			data = data->next;
		}
        }

}

/***************************************************************************
 Return True if a local parameter is currently set to the global default.
***************************************************************************/

BOOL lp_is_default(int snum, struct parm_struct *parm)
{
	int pdiff = PTR_DIFF(parm->ptr, &sDefault);

	return equal_parameter(parm->type,
			       ((char *)ServicePtrs[snum]) + pdiff,
			       ((char *)&sDefault) + pdiff);
}

/***************************************************************************
 Display the contents of a single services record.
***************************************************************************/

static void dump_a_service(service * pService, FILE * f)
{
	int i;
	param_opt_struct *data;
	
	if (pService != &sDefault)
		fprintf(f, "[%s]\n", pService->szService);

	for (i = 0; parm_table[i].label; i++) {

		if (parm_table[i].p_class == P_LOCAL &&
		    parm_table[i].ptr &&
		    (*parm_table[i].label != '-') &&
		    (i == 0 || (parm_table[i].ptr != parm_table[i - 1].ptr))) 
		{
		
			int pdiff = PTR_DIFF(parm_table[i].ptr, &sDefault);

			if (pService == &sDefault) {
				if (defaults_saved && is_default(i))
					continue;
			} else {
				if (equal_parameter(parm_table[i].type,
						    ((char *)pService) +
						    pdiff,
						    ((char *)&sDefault) +
						    pdiff))
					continue;
			}

			fprintf(f, "\t%s = ", parm_table[i].label);
			print_parameter(&parm_table[i],
					((char *)pService) + pdiff, f);
			fprintf(f, "\n");
		}
	}

		if (pService->param_opt != NULL) {
			data = pService->param_opt;
			while(data) {
				fprintf(f, "\t%s = %s\n", data->key, data->value);
				data = data->next;
			}
        	}
}

/***************************************************************************
 Display the contents of a parameter of a single services record.
***************************************************************************/

BOOL dump_a_parameter(int snum, char *parm_name, FILE * f, BOOL isGlobal)
{
	service * pService = ServicePtrs[snum];
	int i;
	BOOL result = False;
	parm_class p_class;
	unsigned flag = 0;
	fstring local_parm_name;
	char *parm_opt;
	const char *parm_opt_value;

	/* check for parametrical option */
	fstrcpy( local_parm_name, parm_name);
	parm_opt = strchr( local_parm_name, ':');

	if (parm_opt) {
		*parm_opt = '\0';
		parm_opt++;
		if (strlen(parm_opt)) {
			parm_opt_value = lp_parm_const_string( snum,
				local_parm_name, parm_opt, NULL);
			if (parm_opt_value) {
				printf( "%s\n", parm_opt_value);
				result = True;
			}
		}
		return result;
	}

	/* check for a key and print the value */
	if (isGlobal) {
		p_class = P_GLOBAL;
		flag = FLAG_GLOBAL;
	} else
		p_class = P_LOCAL;

	for (i = 0; parm_table[i].label; i++) {
		if (strwicmp(parm_table[i].label, parm_name) == 0 &&
		    (parm_table[i].p_class == p_class || parm_table[i].flags & flag) &&
		    parm_table[i].ptr &&
		    (*parm_table[i].label != '-') &&
		    (i == 0 || (parm_table[i].ptr != parm_table[i - 1].ptr))) 
		{
			void *ptr;

			if (isGlobal)
				ptr = parm_table[i].ptr;
			else
				ptr = ((char *)pService) +
					PTR_DIFF(parm_table[i].ptr, &sDefault);

			print_parameter(&parm_table[i],
					ptr, f);
			fprintf(f, "\n");
			result = True;
			break;
		}
	}

	return result;
}

/***************************************************************************
 Return info about the next service  in a service. snum==GLOBAL_SECTION_SNUM gives the globals.
 Return NULL when out of parameters.
***************************************************************************/

struct parm_struct *lp_next_parameter(int snum, int *i, int allparameters)
{
	if (snum < 0) {
		/* do the globals */
		for (; parm_table[*i].label; (*i)++) {
			if (parm_table[*i].p_class == P_SEPARATOR)
				return &parm_table[(*i)++];

			if (!parm_table[*i].ptr
			    || (*parm_table[*i].label == '-'))
				continue;

			if ((*i) > 0
			    && (parm_table[*i].ptr ==
				parm_table[(*i) - 1].ptr))
				continue;

			return &parm_table[(*i)++];
		}
	} else {
		service *pService = ServicePtrs[snum];

		for (; parm_table[*i].label; (*i)++) {
			if (parm_table[*i].p_class == P_SEPARATOR)
				return &parm_table[(*i)++];

			if (parm_table[*i].p_class == P_LOCAL &&
			    parm_table[*i].ptr &&
			    (*parm_table[*i].label != '-') &&
			    ((*i) == 0 ||
			     (parm_table[*i].ptr !=
			      parm_table[(*i) - 1].ptr)))
			{
				int pdiff =
					PTR_DIFF(parm_table[*i].ptr,
						 &sDefault);

				if (allparameters ||
				    !equal_parameter(parm_table[*i].type,
						     ((char *)pService) +
						     pdiff,
						     ((char *)&sDefault) +
						     pdiff))
				{
					return &parm_table[(*i)++];
				}
			}
		}
	}

	return NULL;
}


#if 0
/***************************************************************************
 Display the contents of a single copy structure.
***************************************************************************/
static void dump_copy_map(BOOL *pcopymap)
{
	int i;
	if (!pcopymap)
		return;

	printf("\n\tNon-Copied parameters:\n");

	for (i = 0; parm_table[i].label; i++)
		if (parm_table[i].p_class == P_LOCAL &&
		    parm_table[i].ptr && !pcopymap[i] &&
		    (i == 0 || (parm_table[i].ptr != parm_table[i - 1].ptr)))
		{
			printf("\t\t%s\n", parm_table[i].label);
		}
}
#endif

/***************************************************************************
 Return TRUE if the passed service number is within range.
***************************************************************************/

BOOL lp_snum_ok(int iService)
{
	return (LP_SNUM_OK(iService) && ServicePtrs[iService]->bAvailable);
}

/***************************************************************************
 Auto-load some home services.
***************************************************************************/

static void lp_add_auto_services(char *str)
{
	char *s;
	char *p;
	int homes;

	if (!str)
		return;

	s = SMB_STRDUP(str);
	if (!s)
		return;

	homes = lp_servicenumber(HOMES_NAME);

	for (p = strtok(s, LIST_SEP); p; p = strtok(NULL, LIST_SEP)) {
		char *home = get_user_home_dir(p);

		if (lp_servicenumber(p) >= 0)
			continue;

		if (home && homes >= 0)
			lp_add_home(p, homes, p, home);
	}
	SAFE_FREE(s);
}

/***************************************************************************
 Auto-load one printer.
***************************************************************************/

void lp_add_one_printer(char *name, char *comment)
{
	int printers = lp_servicenumber(PRINTERS_NAME);
	int i;

	if (lp_servicenumber(name) < 0) {
		lp_add_printer(name, printers);
		if ((i = lp_servicenumber(name)) >= 0) {
			string_set(&ServicePtrs[i]->comment, comment);
			ServicePtrs[i]->autoloaded = True;
		}
	}
}

/***************************************************************************
 Have we loaded a services file yet?
***************************************************************************/

BOOL lp_loaded(void)
{
	return (bLoaded);
}

/***************************************************************************
 Unload unused services.
***************************************************************************/

void lp_killunused(BOOL (*snumused) (int))
{
	int i;
	for (i = 0; i < iNumServices; i++) {
		if (!VALID(i))
			continue;

		/* don't kill autoloaded or usershare services */
		if ( ServicePtrs[i]->autoloaded ||
				ServicePtrs[i]->usershare == USERSHARE_VALID) {
			continue;
		}

		if (!snumused || !snumused(i)) {
			free_service_byindex(i);
		}
	}
}

/***************************************************************************
 Unload a service.
***************************************************************************/

void lp_killservice(int iServiceIn)
{
	if (VALID(iServiceIn)) {
		free_service_byindex(iServiceIn);
	}
}

/***************************************************************************
 Save the curent values of all global and sDefault parameters into the 
 defaults union. This allows swat and testparm to show only the
 changed (ie. non-default) parameters.
***************************************************************************/

static void lp_save_defaults(void)
{
	int i;
	for (i = 0; parm_table[i].label; i++) {
		if (i > 0 && parm_table[i].ptr == parm_table[i - 1].ptr)
			continue;
		switch (parm_table[i].type) {
			case P_LIST:
				str_list_copy(&(parm_table[i].def.lvalue),
					    *(const char ***)parm_table[i].ptr);
				break;
			case P_STRING:
			case P_USTRING:
				if (parm_table[i].ptr) {
					parm_table[i].def.svalue = SMB_STRDUP(*(char **)parm_table[i].ptr);
				} else {
					parm_table[i].def.svalue = NULL;
				}
				break;
			case P_GSTRING:
			case P_UGSTRING:
				if (parm_table[i].ptr) {
					parm_table[i].def.svalue = SMB_STRDUP((char *)parm_table[i].ptr);
				} else {
					parm_table[i].def.svalue = NULL;
				}
				break;
			case P_BOOL:
			case P_BOOLREV:
				parm_table[i].def.bvalue =
					*(BOOL *)parm_table[i].ptr;
				break;
			case P_CHAR:
				parm_table[i].def.cvalue =
					*(char *)parm_table[i].ptr;
				break;
			case P_INTEGER:
			case P_OCTAL:
			case P_ENUM:
				parm_table[i].def.ivalue =
					*(int *)parm_table[i].ptr;
				break;
			case P_SEP:
				break;
		}
	}
	defaults_saved = True;
}

/*******************************************************************
 Set the server type we will announce as via nmbd.
********************************************************************/

static const struct srv_role_tab {
	uint32 role;
	const char *role_str;
} srv_role_tab [] = {
	{ ROLE_STANDALONE, "ROLE_STANDALONE" },
	{ ROLE_DOMAIN_MEMBER, "ROLE_DOMAIN_MEMBER" },
	{ ROLE_DOMAIN_BDC, "ROLE_DOMAIN_BDC" },
	{ ROLE_DOMAIN_PDC, "ROLE_DOMAIN_PDC" },
	{ 0, NULL }
};

const char* server_role_str(uint32 role)
{
	int i = 0;
	for (i=0; srv_role_tab[i].role_str; i++) {
		if (role == srv_role_tab[i].role) {
			return srv_role_tab[i].role_str;
		}
	}
	return NULL;
}

static void set_server_role(void)
{
	server_role = ROLE_STANDALONE;

	switch (lp_security()) {
		case SEC_SHARE:
			if (lp_domain_logons())
				DEBUG(0, ("Server's Role (logon server) conflicts with share-level security\n"));
			break;
		case SEC_SERVER:
			if (lp_domain_logons())
				DEBUG(0, ("Server's Role (logon server) conflicts with server-level security\n"));
			server_role = ROLE_DOMAIN_MEMBER;
			break;
		case SEC_DOMAIN:
			if (lp_domain_logons()) {
				DEBUG(1, ("Server's Role (logon server) NOT ADVISED with domain-level security\n"));
				server_role = ROLE_DOMAIN_BDC;
				break;
			}
			server_role = ROLE_DOMAIN_MEMBER;
			break;
		case SEC_ADS:
			if (lp_domain_logons()) {
				server_role = ROLE_DOMAIN_PDC;
				break;
			}
			server_role = ROLE_DOMAIN_MEMBER;
			break;
		case SEC_USER:
			if (lp_domain_logons()) {

				if (Globals.bDomainMaster) /* auto or yes */ 
					server_role = ROLE_DOMAIN_PDC;
				else
					server_role = ROLE_DOMAIN_BDC;
			}
			break;
		default:
			DEBUG(0, ("Server's Role undefined due to unknown security mode\n"));
			break;
	}

	DEBUG(10, ("set_server_role: role = %s\n", server_role_str(server_role)));
}

/***********************************************************
 If we should send plaintext/LANMAN passwords in the clinet
************************************************************/

static void set_allowed_client_auth(void)
{
	if (Globals.bClientNTLMv2Auth) {
		Globals.bClientLanManAuth = False;
	}
	if (!Globals.bClientLanManAuth) {
		Globals.bClientPlaintextAuth = False;
	}
}

/***************************************************************************
 JRA.
 The following code allows smbd to read a user defined share file.
 Yes, this is my intent. Yes, I'm comfortable with that...

 THE FOLLOWING IS SECURITY CRITICAL CODE.

 It washes your clothes, it cleans your house, it guards you while you sleep...
 Do not f%^k with it....
***************************************************************************/

#define MAX_USERSHARE_FILE_SIZE (10*1024)

/***************************************************************************
 Check allowed stat state of a usershare file.
 Ensure we print out who is dicking with us so the admin can
 get their sorry ass fired.
***************************************************************************/

static BOOL check_usershare_stat(const char *fname, SMB_STRUCT_STAT *psbuf)
{
	if (!S_ISREG(psbuf->st_mode)) {
		DEBUG(0,("check_usershare_stat: file %s owned by uid %u is "
			"not a regular file\n",
			fname, (unsigned int)psbuf->st_uid ));
		return False;
	}

	/* Ensure this doesn't have the other write bit set. */
	if (psbuf->st_mode & S_IWOTH) {
		DEBUG(0,("check_usershare_stat: file %s owned by uid %u allows "
			"public write. Refusing to allow as a usershare file.\n",
			fname, (unsigned int)psbuf->st_uid ));
		return False;
	}

	/* Should be 10k or less. */
	if (psbuf->st_size > MAX_USERSHARE_FILE_SIZE) {
		DEBUG(0,("check_usershare_stat: file %s owned by uid %u is "
			"too large (%u) to be a user share file.\n",
			fname, (unsigned int)psbuf->st_uid,
			(unsigned int)psbuf->st_size ));
		return False;
	}

	return True;
}

/***************************************************************************
 Parse the contents of a usershare file.
***************************************************************************/

enum usershare_err parse_usershare_file(TALLOC_CTX *ctx, 
			SMB_STRUCT_STAT *psbuf,
			const char *servicename,
			int snum,
			char **lines,
			int numlines,
			pstring sharepath,
			pstring comment,
			SEC_DESC **ppsd,
			BOOL *pallow_guest)
{
	const char **prefixallowlist = lp_usershare_prefix_allow_list();
	const char **prefixdenylist = lp_usershare_prefix_deny_list();
	int us_vers;
	SMB_STRUCT_DIR *dp;
	SMB_STRUCT_STAT sbuf;

	*pallow_guest = False;

	if (numlines < 4) {
		return USERSHARE_MALFORMED_FILE;
	}

	if (strcmp(lines[0], "#VERSION 1") == 0) {
		us_vers = 1;
	} else if (strcmp(lines[0], "#VERSION 2") == 0) {
		us_vers = 2;
		if (numlines < 5) {
			return USERSHARE_MALFORMED_FILE;
		}
	} else {
		return USERSHARE_BAD_VERSION;
	}

	if (strncmp(lines[1], "path=", 5) != 0) {
		return USERSHARE_MALFORMED_PATH;
	}

	pstrcpy(sharepath, &lines[1][5]);
	trim_string(sharepath, " ", " ");

	if (strncmp(lines[2], "comment=", 8) != 0) {
		return USERSHARE_MALFORMED_COMMENT_DEF;
	}

	pstrcpy(comment, &lines[2][8]);
	trim_string(comment, " ", " ");
	trim_char(comment, '"', '"');

	if (strncmp(lines[3], "usershare_acl=", 14) != 0) {
		return USERSHARE_MALFORMED_ACL_DEF;
	}

	if (!parse_usershare_acl(ctx, &lines[3][14], ppsd)) {
		return USERSHARE_ACL_ERR;
	}

	if (us_vers == 2) {
		if (strncmp(lines[4], "guest_ok=", 9) != 0) {
			return USERSHARE_MALFORMED_ACL_DEF;
		}
		if (lines[4][9] == 'y') {
			*pallow_guest = True;
		}
	}

	if (snum != -1 && (strcmp(sharepath, ServicePtrs[snum]->szPath) == 0)) {
		/* Path didn't change, no checks needed. */
		return USERSHARE_OK;
	}

	/* The path *must* be absolute. */
	if (sharepath[0] != '/') {
		DEBUG(2,("parse_usershare_file: share %s: path %s is not an absolute path.\n",
			servicename, sharepath));
		return USERSHARE_PATH_NOT_ABSOLUTE;
	}

	/* If there is a usershare prefix deny list ensure one of these paths
	   doesn't match the start of the user given path. */
	if (prefixdenylist) {
		int i;
		for ( i=0; prefixdenylist[i]; i++ ) {
			DEBUG(10,("parse_usershare_file: share %s : checking prefixdenylist[%d]='%s' against %s\n",
				servicename, i, prefixdenylist[i], sharepath ));
			if (memcmp( sharepath, prefixdenylist[i], strlen(prefixdenylist[i])) == 0) {
				DEBUG(2,("parse_usershare_file: share %s path %s starts with one of the "
					"usershare prefix deny list entries.\n",
					servicename, sharepath));
				return USERSHARE_PATH_IS_DENIED;
			}
		}
	}

	/* If there is a usershare prefix allow list ensure one of these paths
	   does match the start of the user given path. */

	if (prefixallowlist) {
		int i;
		for ( i=0; prefixallowlist[i]; i++ ) {
			DEBUG(10,("parse_usershare_file: share %s checking prefixallowlist[%d]='%s' against %s\n",
				servicename, i, prefixallowlist[i], sharepath ));
			if (memcmp( sharepath, prefixallowlist[i], strlen(prefixallowlist[i])) == 0) {
				break;
			}
		}
		if (prefixallowlist[i] == NULL) {
			DEBUG(2,("parse_usershare_file: share %s path %s doesn't start with one of the "
				"usershare prefix allow list entries.\n",
				servicename, sharepath));
			return USERSHARE_PATH_NOT_ALLOWED;
		}
        }

	/* Ensure this is pointing to a directory. */
	dp = sys_opendir(sharepath);

	if (!dp) {
		DEBUG(2,("parse_usershare_file: share %s path %s is not a directory.\n",
			servicename, sharepath));
		return USERSHARE_PATH_NOT_DIRECTORY;
	}

	/* Ensure the owner of the usershare file has permission to share
	   this directory. */

	if (sys_stat(sharepath, &sbuf) == -1) {
		DEBUG(2,("parse_usershare_file: share %s : stat failed on path %s. %s\n",
			servicename, sharepath, strerror(errno) ));
		sys_closedir(dp);
		return USERSHARE_POSIX_ERR;
	}

	sys_closedir(dp);

	if (!S_ISDIR(sbuf.st_mode)) {
		DEBUG(2,("parse_usershare_file: share %s path %s is not a directory.\n",
			servicename, sharepath ));
		return USERSHARE_PATH_NOT_DIRECTORY;
	}

	/* Check if sharing is restricted to owner-only. */
	/* psbuf is the stat of the usershare definition file,
	   sbuf is the stat of the target directory to be shared. */

	if (lp_usershare_owner_only()) {
		/* root can share anything. */
		if ((psbuf->st_uid != 0) && (sbuf.st_uid != psbuf->st_uid)) {
			return USERSHARE_PATH_NOT_ALLOWED;
		}
	}

	return USERSHARE_OK;
}

/***************************************************************************
 Deal with a usershare file.
 Returns:
	>= 0 - snum
	-1 - Bad name, invalid contents.
	   - service name already existed and not a usershare, problem
	    with permissions to share directory etc.
***************************************************************************/

static int process_usershare_file(const char *dir_name, const char *file_name, int snum_template)
{
	SMB_STRUCT_STAT sbuf;
	SMB_STRUCT_STAT lsbuf;
	pstring fname;
	pstring sharepath;
	pstring comment;
	fstring service_name;
	char **lines = NULL;
	int numlines = 0;
	int fd = -1;
	int iService = -1;
	TALLOC_CTX *ctx = NULL;
	SEC_DESC *psd = NULL;
	BOOL guest_ok = False;

	/* Ensure share name doesn't contain invalid characters. */
	if (!validate_net_name(file_name, INVALID_SHARENAME_CHARS, strlen(file_name))) {
		DEBUG(0,("process_usershare_file: share name %s contains "
			"invalid characters (any of %s)\n",
			file_name, INVALID_SHARENAME_CHARS ));
		return -1;
	}

	fstrcpy(service_name, file_name);

	pstrcpy(fname, dir_name);
	pstrcat(fname, "/");
	pstrcat(fname, file_name);

	/* Minimize the race condition by doing an lstat before we
	   open and fstat. Ensure this isn't a symlink link. */

	if (sys_lstat(fname, &lsbuf) != 0) {
		DEBUG(0,("process_usershare_file: stat of %s failed. %s\n",
			fname, strerror(errno) ));
		return -1;
	}

	/* This must be a regular file, not a symlink, directory or
	   other strange filetype. */
	if (!check_usershare_stat(fname, &lsbuf)) {
		return -1;
	}

	/* See if there is already a servicenum for this name. */
	/* tdb_fetch_int32 returns -1 if not found. */
	iService = (int)tdb_fetch_int32(ServiceHash, canonicalize_servicename(service_name) );

	if (iService != -1 && ServicePtrs[iService]->usershare_last_mod == lsbuf.st_mtime) {
		/* Nothing changed - Mark valid and return. */
		DEBUG(10,("process_usershare_file: service %s not changed.\n",
			service_name ));
		ServicePtrs[iService]->usershare = USERSHARE_VALID;
		return iService;
	}

	/* Try and open the file read only - no symlinks allowed. */
#ifdef O_NOFOLLOW
	fd = sys_open(fname, O_RDONLY|O_NOFOLLOW, 0);
#else
	fd = sys_open(fname, O_RDONLY, 0);
#endif

	if (fd == -1) {
		DEBUG(0,("process_usershare_file: unable to open %s. %s\n",
			fname, strerror(errno) ));
		return -1;
	}

	/* Now fstat to be *SURE* it's a regular file. */
	if (sys_fstat(fd, &sbuf) != 0) {
		close(fd);
		DEBUG(0,("process_usershare_file: fstat of %s failed. %s\n",
			fname, strerror(errno) ));
		return -1;
	}

	/* Is it the same dev/inode as was lstated ? */
	if (lsbuf.st_dev != sbuf.st_dev || lsbuf.st_ino != sbuf.st_ino) {
		close(fd);
		DEBUG(0,("process_usershare_file: fstat of %s is a different file from lstat. "
			"Symlink spoofing going on ?\n", fname ));
		return -1;
	}

	/* This must be a regular file, not a symlink, directory or
	   other strange filetype. */
	if (!check_usershare_stat(fname, &sbuf)) {
		return -1;
	}

	lines = fd_lines_load(fd, &numlines, MAX_USERSHARE_FILE_SIZE);

	close(fd);
	if (lines == NULL) {
		DEBUG(0,("process_usershare_file: loading file %s owned by %u failed.\n",
			fname, (unsigned int)sbuf.st_uid ));
		return -1;
	}

	/* Should we allow printers to be shared... ? */
	ctx = talloc_init("usershare_sd_xctx");
	if (!ctx) {
		SAFE_FREE(lines);
		return 1;
	}

	if (parse_usershare_file(ctx, &sbuf, service_name,
			iService, lines, numlines, sharepath,
			comment, &psd, &guest_ok) != USERSHARE_OK) {
		talloc_destroy(ctx);
		SAFE_FREE(lines);
		return -1;
	}

	SAFE_FREE(lines);

	/* Everything ok - add the service possibly using a template. */
	if (iService < 0) {
		const service *sp = &sDefault;
		if (snum_template != -1) {
			sp = ServicePtrs[snum_template];
		}

		if ((iService = add_a_service(sp, service_name)) < 0) {
			DEBUG(0, ("process_usershare_file: Failed to add "
				"new service %s\n", service_name));
			talloc_destroy(ctx);
			return -1;
		}

		/* Read only is controlled by usershare ACL below. */
		ServicePtrs[iService]->bRead_only = False;
	}

	/* Write the ACL of the new/modified share. */
	if (!set_share_security(ctx, service_name, psd)) {
		 DEBUG(0, ("process_usershare_file: Failed to set share "
			"security for user share %s\n",
			service_name ));
		lp_remove_service(iService);
		talloc_destroy(ctx);
		return -1;
	}

	talloc_destroy(ctx);

	/* If from a template it may be marked invalid. */
	ServicePtrs[iService]->valid = True;

	/* Set the service as a valid usershare. */
	ServicePtrs[iService]->usershare = USERSHARE_VALID;

	/* Set guest access. */
	if (lp_usershare_allow_guests()) {
		ServicePtrs[iService]->bGuest_ok = guest_ok;
	}

	/* And note when it was loaded. */
	ServicePtrs[iService]->usershare_last_mod = sbuf.st_mtime;
	string_set(&ServicePtrs[iService]->szPath, sharepath);
	string_set(&ServicePtrs[iService]->comment, comment);

	return iService;
}

/***************************************************************************
 Checks if a usershare entry has been modified since last load.
***************************************************************************/

static BOOL usershare_exists(int iService, time_t *last_mod)
{
	SMB_STRUCT_STAT lsbuf;
	const char *usersharepath = Globals.szUsersharePath;
	pstring fname;

	pstrcpy(fname, usersharepath);
	pstrcat(fname, "/");
	pstrcat(fname, ServicePtrs[iService]->szService);

	if (sys_lstat(fname, &lsbuf) != 0) {
		return False;
	}

	if (!S_ISREG(lsbuf.st_mode)) {
		return False;
	}

	*last_mod = lsbuf.st_mtime;
	return True;
}

/***************************************************************************
 Load a usershare service by name. Returns a valid servicenumber or -1.
***************************************************************************/

int load_usershare_service(const char *servicename)
{
	SMB_STRUCT_STAT sbuf;
	const char *usersharepath = Globals.szUsersharePath;
	int max_user_shares = Globals.iUsershareMaxShares;
	int snum_template = -1;

	if (*usersharepath == 0 ||  max_user_shares == 0) {
		return -1;
	}

	if (sys_stat(usersharepath, &sbuf) != 0) {
		DEBUG(0,("load_usershare_service: stat of %s failed. %s\n",
			usersharepath, strerror(errno) ));
		return -1;
	}

	if (!S_ISDIR(sbuf.st_mode)) {
		DEBUG(0,("load_usershare_service: %s is not a directory.\n",
			usersharepath ));
		return -1;
	}

	/*
	 * This directory must be owned by root, and have the 't' bit set.
	 * It also must not be writable by "other".
	 */

#ifdef S_ISVTX
	if (sbuf.st_uid != 0 || !(sbuf.st_mode & S_ISVTX) || (sbuf.st_mode & S_IWOTH)) {
#else
	if (sbuf.st_uid != 0 || (sbuf.st_mode & S_IWOTH)) {
#endif
		DEBUG(0,("load_usershare_service: directory %s is not owned by root "
			"or does not have the sticky bit 't' set or is writable by anyone.\n",
			usersharepath ));
		return -1;
	}

	/* Ensure the template share exists if it's set. */
	if (Globals.szUsershareTemplateShare[0]) {
		/* We can't use lp_servicenumber here as we are recommending that
		   template shares have -valid=False set. */
		for (snum_template = iNumServices - 1; snum_template >= 0; snum_template--) {
			if (ServicePtrs[snum_template]->szService &&
					strequal(ServicePtrs[snum_template]->szService,
						Globals.szUsershareTemplateShare)) {
				break;
			}
		}

		if (snum_template == -1) {
			DEBUG(0,("load_usershare_service: usershare template share %s "
				"does not exist.\n",
				Globals.szUsershareTemplateShare ));
			return -1;
		}
	}

	return process_usershare_file(usersharepath, servicename, snum_template);
}

/***************************************************************************
 Load all user defined shares from the user share directory.
 We only do this if we're enumerating the share list.
 This is the function that can delete usershares that have
 been removed.
***************************************************************************/

int load_usershare_shares(void)
{
	SMB_STRUCT_DIR *dp;
	SMB_STRUCT_STAT sbuf;
	SMB_STRUCT_DIRENT *de;
	int num_usershares = 0;
	int max_user_shares = Globals.iUsershareMaxShares;
	unsigned int num_dir_entries, num_bad_dir_entries, num_tmp_dir_entries;
	unsigned int allowed_bad_entries = ((2*max_user_shares)/10);
	unsigned int allowed_tmp_entries = ((2*max_user_shares)/10);
	int iService;
	int snum_template = -1;
	const char *usersharepath = Globals.szUsersharePath;
	int ret = lp_numservices();

	if (max_user_shares == 0 || *usersharepath == '\0') {
		return lp_numservices();
	}

	if (sys_stat(usersharepath, &sbuf) != 0) {
		DEBUG(0,("load_usershare_shares: stat of %s failed. %s\n",
			usersharepath, strerror(errno) ));
		return ret;
	}

	/*
	 * This directory must be owned by root, and have the 't' bit set.
	 * It also must not be writable by "other".
	 */

#ifdef S_ISVTX
	if (sbuf.st_uid != 0 || !(sbuf.st_mode & S_ISVTX) || (sbuf.st_mode & S_IWOTH)) {
#else
	if (sbuf.st_uid != 0 || (sbuf.st_mode & S_IWOTH)) {
#endif
		DEBUG(0,("load_usershare_shares: directory %s is not owned by root "
			"or does not have the sticky bit 't' set or is writable by anyone.\n",
			usersharepath ));
		return ret;
	}

	/* Ensure the template share exists if it's set. */
	if (Globals.szUsershareTemplateShare[0]) {
		/* We can't use lp_servicenumber here as we are recommending that
		   template shares have -valid=False set. */
		for (snum_template = iNumServices - 1; snum_template >= 0; snum_template--) {
			if (ServicePtrs[snum_template]->szService &&
					strequal(ServicePtrs[snum_template]->szService,
						Globals.szUsershareTemplateShare)) {
				break;
			}
		}

		if (snum_template == -1) {
			DEBUG(0,("load_usershare_shares: usershare template share %s "
				"does not exist.\n",
				Globals.szUsershareTemplateShare ));
			return ret;
		}
	}

	/* Mark all existing usershares as pending delete. */
	for (iService = iNumServices - 1; iService >= 0; iService--) {
		if (VALID(iService) && ServicePtrs[iService]->usershare) {
			ServicePtrs[iService]->usershare = USERSHARE_PENDING_DELETE;
		}
	}

	dp = sys_opendir(usersharepath);
	if (!dp) {
		DEBUG(0,("load_usershare_shares:: failed to open directory %s. %s\n",
			usersharepath, strerror(errno) ));
		return ret;
	}

	for (num_dir_entries = 0, num_bad_dir_entries = 0, num_tmp_dir_entries = 0;
			(de = sys_readdir(dp));
			num_dir_entries++ ) {
		int r;
		const char *n = de->d_name;

		/* Ignore . and .. */
		if (*n == '.') {
			if ((n[1] == '\0') || (n[1] == '.' && n[2] == '\0')) {
				continue;
			}
		}

		if (n[0] == ':') {
			/* Temporary file used when creating a share. */
			num_tmp_dir_entries++;
		}

		/* Allow 20% tmp entries. */
		if (num_tmp_dir_entries > allowed_tmp_entries) {
			DEBUG(0,("load_usershare_shares: too many temp entries (%u) "
				"in directory %s\n",
				num_tmp_dir_entries, usersharepath));
			break;
		}

		r = process_usershare_file(usersharepath, n, snum_template);
		if (r == 0) {
			/* Update the services count. */
			num_usershares++;
			if (num_usershares >= max_user_shares) {
				DEBUG(0,("load_usershare_shares: max user shares reached "
					"on file %s in directory %s\n",
					n, usersharepath ));
				break;
			}
		} else if (r == -1) {
			num_bad_dir_entries++;
		}

		/* Allow 20% bad entries. */
		if (num_bad_dir_entries > allowed_bad_entries) {
			DEBUG(0,("load_usershare_shares: too many bad entries (%u) "
				"in directory %s\n",
				num_bad_dir_entries, usersharepath));
			break;
		}

		/* Allow 20% bad entries. */
		if (num_dir_entries > max_user_shares + allowed_bad_entries) {
			DEBUG(0,("load_usershare_shares: too many total entries (%u) "
			"in directory %s\n",
			num_dir_entries, usersharepath));
			break;
		}
	}

	sys_closedir(dp);

	/* Sweep through and delete any non-refreshed usershares that are
	   not currently in use. */
	for (iService = iNumServices - 1; iService >= 0; iService--) {
		if (VALID(iService) && (ServicePtrs[iService]->usershare == USERSHARE_PENDING_DELETE)) {
			if (conn_snum_used(iService)) {
				continue;
			}
			/* Remove from the share ACL db. */
			DEBUG(10,("load_usershare_shares: Removing deleted usershare %s\n",
				lp_servicename(iService) ));
			delete_share_security(iService);
			free_service_byindex(iService);
		}
	}

	return lp_numservices();
}

/********************************************************
 Destroy global resources allocated in this file
********************************************************/

void gfree_loadparm(void)
{
	struct file_lists *f;
	struct file_lists *next;
	int i;

	lp_TALLOC_FREE();

	/* Free the file lists */

	f = file_lists;
	while( f ) {
		next = f->next;
		SAFE_FREE( f->name );
		SAFE_FREE( f->subfname );
		SAFE_FREE( f );
		f = next;
	}

	/* Free resources allocated to services */

	for ( i = 0; i < iNumServices; i++ ) {
		if ( VALID(i) ) {
			free_service_byindex(i);
		}
	}

	SAFE_FREE( ServicePtrs );
	iNumServices = 0;

	/* Now release all resources allocated to global
	   parameters and the default service */

	for (i = 0; parm_table[i].label; i++) 
	{
		if ( parm_table[i].type == P_STRING 
			|| parm_table[i].type == P_USTRING ) 
		{
			string_free( (char**)parm_table[i].ptr );
		}
		else if (parm_table[i].type == P_LIST) {
			str_list_free( (char***)parm_table[i].ptr );
		}
	}
}

/***************************************************************************
 Load the services array from the services file. Return True on success, 
 False on failure.
***************************************************************************/

BOOL lp_load(const char *pszFname,
             BOOL global_only,
             BOOL save_defaults,
	     BOOL add_ipc,
             BOOL initialize_globals)
{
	pstring n2;
	BOOL bRetval;
	param_opt_struct *data, *pdata;

	pstrcpy(n2, pszFname);
	
	standard_sub_basic( get_current_username(), n2,sizeof(n2) );

	add_to_file_list(pszFname, n2);

	bRetval = False;

	DEBUG(3, ("lp_load: refreshing parameters\n"));
	
	bInGlobalSection = True;
	bGlobalOnly = global_only;

	init_globals(! initialize_globals);
	debug_init();

	if (save_defaults) {
		init_locals();
		lp_save_defaults();
	}

	if (Globals.param_opt != NULL) {
		data = Globals.param_opt;
		while (data) {
			string_free(&data->key);
			string_free(&data->value);
			str_list_free(&data->list);
			pdata = data->next;
			SAFE_FREE(data);
			data = pdata;
		}
		Globals.param_opt = NULL;
	}
	
	/* We get sections first, so have to start 'behind' to make up */
	iServiceIndex = -1;
	bRetval = pm_process(n2, do_section, do_parameter);

	/* finish up the last section */
	DEBUG(4, ("pm_process() returned %s\n", BOOLSTR(bRetval)));
	if (bRetval)
		if (iServiceIndex >= 0)
			bRetval = service_ok(iServiceIndex);

	lp_add_auto_services(lp_auto_services());

	if (add_ipc) {
		/* When 'restrict anonymous = 2' guest connections to ipc$
		   are denied */
		lp_add_ipc("IPC$", (lp_restrict_anonymous() < 2));
		if ( lp_enable_asu_support() )
			lp_add_ipc("ADMIN$", False);
	}

	set_server_role();
	set_default_server_announce_type();
	set_allowed_client_auth();

	bLoaded = True;

	/* Now we check bWINSsupport and set szWINSserver to 127.0.0.1 */
	/* if bWINSsupport is true and we are in the client            */
	if (in_client && Globals.bWINSsupport) {
		lp_do_parameter(GLOBAL_SECTION_SNUM, "wins server", "127.0.0.1");
	}

	init_iconv();

	return (bRetval);
}

/***************************************************************************
 Reset the max number of services.
***************************************************************************/

void lp_resetnumservices(void)
{
	iNumServices = 0;
}

/***************************************************************************
 Return the max number of services.
***************************************************************************/

int lp_numservices(void)
{
	return (iNumServices);
}

/***************************************************************************
Display the contents of the services array in human-readable form.
***************************************************************************/

void lp_dump(FILE *f, BOOL show_defaults, int maxtoprint)
{
	int iService;

	if (show_defaults)
		defaults_saved = False;

	dump_globals(f);

	dump_a_service(&sDefault, f);

	for (iService = 0; iService < maxtoprint; iService++) {
		fprintf(f,"\n");
		lp_dump_one(f, show_defaults, iService);
	}
}

/***************************************************************************
Display the contents of one service in human-readable form.
***************************************************************************/

void lp_dump_one(FILE * f, BOOL show_defaults, int snum)
{
	if (VALID(snum)) {
		if (ServicePtrs[snum]->szService[0] == '\0')
			return;
		dump_a_service(ServicePtrs[snum], f);
	}
}

/***************************************************************************
Return the number of the service with the given name, or -1 if it doesn't
exist. Note that this is a DIFFERENT ANIMAL from the internal function
getservicebyname()! This works ONLY if all services have been loaded, and
does not copy the found service.
***************************************************************************/

int lp_servicenumber(const char *pszServiceName)
{
	int iService;
        fstring serviceName;
        
        if (!pszServiceName) {
        	return GLOBAL_SECTION_SNUM;
	}
        
	for (iService = iNumServices - 1; iService >= 0; iService--) {
		if (VALID(iService) && ServicePtrs[iService]->szService) {
			/*
			 * The substitution here is used to support %U is
			 * service names
			 */
			fstrcpy(serviceName, ServicePtrs[iService]->szService);
			standard_sub_basic(get_current_username(), serviceName,sizeof(serviceName));
			if (strequal(serviceName, pszServiceName)) {
				break;
			}
		}
	}

	if (iService >= 0 && ServicePtrs[iService]->usershare == USERSHARE_VALID) {
		time_t last_mod;

		if (!usershare_exists(iService, &last_mod)) {
			/* Remove the share security tdb entry for it. */
			delete_share_security(iService);
			/* Remove it from the array. */
			free_service_byindex(iService);
			/* Doesn't exist anymore. */
			return GLOBAL_SECTION_SNUM;
		}

		/* Has it been modified ? If so delete and reload. */
		if (ServicePtrs[iService]->usershare_last_mod < last_mod) {
			/* Remove it from the array. */
			free_service_byindex(iService);
			/* and now reload it. */
			iService = load_usershare_service(pszServiceName);
		}
	}

	if (iService < 0) {
		DEBUG(7,("lp_servicenumber: couldn't find %s\n", pszServiceName));
		return GLOBAL_SECTION_SNUM;
	}

	return (iService);
}

/*******************************************************************
 A useful volume label function. 
********************************************************************/

char *volume_label(int snum)
{
	char *ret = lp_volume(snum);
	if (!*ret)
		return lp_servicename(snum);
	return (ret);
}


/*******************************************************************
 Set the server type we will announce as via nmbd.
********************************************************************/

static void set_default_server_announce_type(void)
{
	default_server_announce = 0;
	default_server_announce |= SV_TYPE_WORKSTATION;
	default_server_announce |= SV_TYPE_SERVER;
	default_server_announce |= SV_TYPE_SERVER_UNIX;

	/* note that the flag should be set only if we have a 
	   printer service but nmbd doesn't actually load the 
	   services so we can't tell   --jerry */

	default_server_announce |= SV_TYPE_PRINTQ_SERVER;

	switch (lp_announce_as()) {
		case ANNOUNCE_AS_NT_SERVER:
			default_server_announce |= SV_TYPE_SERVER_NT;
			/* fall through... */
		case ANNOUNCE_AS_NT_WORKSTATION:
			default_server_announce |= SV_TYPE_NT;
			break;
		case ANNOUNCE_AS_WIN95:
			default_server_announce |= SV_TYPE_WIN95_PLUS;
			break;
		case ANNOUNCE_AS_WFW:
			default_server_announce |= SV_TYPE_WFW;
			break;
		default:
			break;
	}

	switch (lp_server_role()) {
		case ROLE_DOMAIN_MEMBER:
			default_server_announce |= SV_TYPE_DOMAIN_MEMBER;
			break;
		case ROLE_DOMAIN_PDC:
			default_server_announce |= SV_TYPE_DOMAIN_CTRL;
			break;
		case ROLE_DOMAIN_BDC:
			default_server_announce |= SV_TYPE_DOMAIN_BAKCTRL;
			break;
		case ROLE_STANDALONE:
		default:
			break;
	}
	if (lp_time_server())
		default_server_announce |= SV_TYPE_TIME_SOURCE;

	if (lp_host_msdfs())
		default_server_announce |= SV_TYPE_DFS_SERVER;
}

/***********************************************************
 returns role of Samba server
************************************************************/

int lp_server_role(void)
{
	return server_role;
}

/***********************************************************
 If we are PDC then prefer us as DMB
************************************************************/

BOOL lp_domain_master(void)
{
	if (Globals.bDomainMaster == Auto)
		return (lp_server_role() == ROLE_DOMAIN_PDC);

	return Globals.bDomainMaster;
}

/***********************************************************
 If we are DMB then prefer us as LMB
************************************************************/

BOOL lp_preferred_master(void)
{
	if (Globals.bPreferredMaster == Auto)
		return (lp_local_master() && lp_domain_master());

	return Globals.bPreferredMaster;
}

/*******************************************************************
 Remove a service.
********************************************************************/

void lp_remove_service(int snum)
{
	ServicePtrs[snum]->valid = False;
	invalid_services[num_invalid_services++] = snum;
}

/*******************************************************************
 Copy a service.
********************************************************************/

void lp_copy_service(int snum, const char *new_name)
{
	do_section(new_name);
	if (snum >= 0) {
		snum = lp_servicenumber(new_name);
		if (snum >= 0)
			lp_do_parameter(snum, "copy", lp_servicename(snum));
	}
}


/*******************************************************************
 Get the default server type we will announce as via nmbd.
********************************************************************/

int lp_default_server_announce(void)
{
	return default_server_announce;
}

/*******************************************************************
 Split the announce version into major and minor numbers.
********************************************************************/

int lp_major_announce_version(void)
{
	static BOOL got_major = False;
	static int major_version = DEFAULT_MAJOR_VERSION;
	char *vers;
	char *p;

	if (got_major)
		return major_version;

	got_major = True;
	if ((vers = lp_announce_version()) == NULL)
		return major_version;

	if ((p = strchr_m(vers, '.')) == 0)
		return major_version;

	*p = '\0';
	major_version = atoi(vers);
	return major_version;
}

int lp_minor_announce_version(void)
{
	static BOOL got_minor = False;
	static int minor_version = DEFAULT_MINOR_VERSION;
	char *vers;
	char *p;

	if (got_minor)
		return minor_version;

	got_minor = True;
	if ((vers = lp_announce_version()) == NULL)
		return minor_version;

	if ((p = strchr_m(vers, '.')) == 0)
		return minor_version;

	p++;
	minor_version = atoi(p);
	return minor_version;
}

/***********************************************************
 Set the global name resolution order (used in smbclient).
************************************************************/

void lp_set_name_resolve_order(const char *new_order)
{
	string_set(&Globals.szNameResolveOrder, new_order);
}

const char *lp_printername(int snum)
{
	const char *ret = _lp_printername(snum);
	if (ret == NULL || (ret != NULL && *ret == '\0'))
		ret = lp_const_servicename(snum);

	return ret;
}


/***********************************************************
 Allow daemons such as winbindd to fix their logfile name.
************************************************************/

void lp_set_logfile(const char *name)
{
	string_set(&Globals.szLogFile, name);
	pstrcpy(debugf, name);
}

/*******************************************************************
 Return the max print jobs per queue.
********************************************************************/

int lp_maxprintjobs(int snum)
{
	int maxjobs = LP_SNUM_OK(snum) ? ServicePtrs[snum]->iMaxPrintJobs : sDefault.iMaxPrintJobs;
	if (maxjobs <= 0 || maxjobs >= PRINT_MAX_JOBID)
		maxjobs = PRINT_MAX_JOBID - 1;

	return maxjobs;
}

const char *lp_printcapname(void)
{
	if ((Globals.szPrintcapname != NULL) &&
	    (Globals.szPrintcapname[0] != '\0'))
		return Globals.szPrintcapname;

	if (sDefault.iPrinting == PRINT_CUPS) {
#ifdef HAVE_CUPS
		return "cups";
#else
		return "lpstat";
#endif
	}

	if (sDefault.iPrinting == PRINT_BSD)
		return "/etc/printcap";

	return PRINTCAP_NAME;
}

/*******************************************************************
 Ensure we don't use sendfile if server smb signing is active.
********************************************************************/

static uint32 spoolss_state;

BOOL lp_disable_spoolss( void )
{
	if ( spoolss_state == SVCCTL_STATE_UNKNOWN )
		spoolss_state = _lp_disable_spoolss() ? SVCCTL_STOPPED : SVCCTL_RUNNING;

	return spoolss_state == SVCCTL_STOPPED ? True : False;
}

void lp_set_spoolss_state( uint32 state )
{
	SMB_ASSERT( (state == SVCCTL_STOPPED) || (state == SVCCTL_RUNNING) );

	spoolss_state = state;
}

uint32 lp_get_spoolss_state( void )
{
	return lp_disable_spoolss() ? SVCCTL_STOPPED : SVCCTL_RUNNING;
}

/*******************************************************************
 Ensure we don't use sendfile if server smb signing is active.
********************************************************************/

BOOL lp_use_sendfile(int snum)
{
	/* Using sendfile blows the brains out of any DOS or Win9x TCP stack... JRA. */
	if (Protocol < PROTOCOL_NT1) {
		return False;
	}
	return (_lp_use_sendfile(snum) && (get_remote_arch() != RA_WIN95) && !srv_is_signing_active());
}

/*******************************************************************
 Turn off sendfile if we find the underlying OS doesn't support it.
********************************************************************/

void set_use_sendfile(int snum, BOOL val)
{
	if (LP_SNUM_OK(snum))
		ServicePtrs[snum]->bUseSendfile = val;
	else
		sDefault.bUseSendfile = val;
}

/*******************************************************************
 Turn off storing DOS attributes if this share doesn't support it.
********************************************************************/

void set_store_dos_attributes(int snum, BOOL val)
{
	if (!LP_SNUM_OK(snum))
		return;
	ServicePtrs[(snum)]->bStoreDosAttributes = val;
}

void lp_set_mangling_method(const char *new_method)
{
	string_set(&Globals.szManglingMethod, new_method);
}

/*******************************************************************
 Global state for POSIX pathname processing.
********************************************************************/

static BOOL posix_pathnames;

BOOL lp_posix_pathnames(void)
{
	return posix_pathnames;
}

/*******************************************************************
 Change everything needed to ensure POSIX pathname processing (currently
 not much).
********************************************************************/

void lp_set_posix_pathnames(void)
{
	posix_pathnames = True;
}

/*******************************************************************
 Global state for POSIX lock processing - CIFS unix extensions.
********************************************************************/

static enum brl_flavour posix_cifsx_locktype; /* By default 0 == WINDOWS_LOCK */

enum brl_flavour lp_posix_cifsu_locktype(void)
{
	return posix_cifsx_locktype;
}

/*******************************************************************
********************************************************************/

void lp_set_posix_cifsx_locktype(enum brl_flavour val)
{
	posix_cifsx_locktype = val;
}
