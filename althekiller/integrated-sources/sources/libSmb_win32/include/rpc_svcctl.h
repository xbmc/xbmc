/* 
   Unix SMB/CIFS implementation.
   SMB parameters and setup
   Copyright (C) Andrew Tridgell              1992-1997,
   Copyright (C) Gerald (Jerry) Carter        2005
   
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

#ifndef _RPC_SVCCTL_H /* _RPC_SVCCTL_H */
#define _RPC_SVCCTL_H 

/* svcctl pipe */

#define SVCCTL_CLOSE_SERVICE			0x00
#define SVCCTL_CONTROL_SERVICE			0x01
#define SVCCTL_LOCK_SERVICE_DB			0x03
#define SVCCTL_QUERY_SERVICE_SEC		0x04
#define SVCCTL_SET_SERVICE_SEC			0x05
#define SVCCTL_QUERY_STATUS			0x06
#define SVCCTL_UNLOCK_SERVICE_DB		0x08
#define SVCCTL_ENUM_DEPENDENT_SERVICES_W	0x0d
#define SVCCTL_ENUM_SERVICES_STATUS_W		0x0e
#define SVCCTL_OPEN_SCMANAGER_W			0x0f
#define SVCCTL_OPEN_SERVICE_W			0x10
#define SVCCTL_QUERY_SERVICE_CONFIG_W		0x11
#define SVCCTL_START_SERVICE_W			0x13
#define SVCCTL_GET_DISPLAY_NAME			0x14
#define SVCCTL_QUERY_SERVICE_CONFIG2_W		0x27
#define SVCCTL_QUERY_SERVICE_STATUSEX_W         0x28

/* ANSI versions not implemented currently 
#define SVCCTL_ENUM_SERVICES_STATUS_A		0x0e
#define SVCCTL_OPEN_SCMANAGER_A			0x1b
*/

/* SERVER_STATUS - type */

#define SVCCTL_TYPE_WIN32		0x00000030
#define SVCCTL_TYPE_DRIVER		0x0000000f

/* SERVER_STATUS - state */
#define SVCCTL_STATE_ACTIVE		0x00000001
#define SVCCTL_STATE_INACTIVE		0x00000002
#define SVCCTL_STATE_ALL		( SVCCTL_STATE_ACTIVE | SVCCTL_STATE_INACTIVE )

/* SERVER_STATUS - CurrentState */

#define SVCCTL_STATE_UNKNOWN		0x00000000	/* only used internally to smbd */
#define SVCCTL_STOPPED			0x00000001
#define SVCCTL_START_PENDING		0x00000002
#define SVCCTL_STOP_PENDING		0x00000003
#define SVCCTL_RUNNING			0x00000004
#define SVCCTL_CONTINUE_PENDING		0x00000005
#define SVCCTL_PAUSE_PENDING		0x00000006
#define SVCCTL_PAUSED			0x00000007

/* SERVER_STATUS - ControlAccepted */

#define SVCCTL_ACCEPT_NONE			0x00000000
#define SVCCTL_ACCEPT_STOP			0x00000001
#define SVCCTL_ACCEPT_PAUSE_CONTINUE		0x00000002
#define SVCCTL_ACCEPT_SHUTDOWN			0x00000004
#define SVCCTL_ACCEPT_PARAMCHANGE		0x00000008
#define SVCCTL_ACCEPT_NETBINDCHANGE		0x00000010
#define SVCCTL_ACCEPT_HARDWAREPROFILECHANGE	0x00000020
#define SVCCTL_ACCEPT_POWEREVENT		0x00000040

/* SERVER_STATUS - ControlAccepted */
#define SVCCTL_SVC_ERROR_IGNORE                 0x00000000
#define SVCCTL_SVC_ERROR_NORMAL                 0x00000001
#define SVCCTL_SVC_ERROR_CRITICAL               0x00000002
#define SVCCTL_SVC_ERROR_SEVERE                 0x00000003

/* QueryServiceConfig2 options */
#define SERVICE_CONFIG_DESCRIPTION              0x00000001
#define SERVICE_CONFIG_FAILURE_ACTIONS          0x00000002


/* Service Config - values for ServiceType field*/

#define SVCCTL_KERNEL_DRVR                         0x00000001  /* doubtful we'll have these */
#define SVCCTL_FILE_SYSTEM_DRVR                    0x00000002  
#define SVCCTL_WIN32_OWN_PROC                      0x00000010
#define SVCCTL_WIN32_SHARED_PROC                   0x00000020
#define SVCCTL_WIN32_INTERACTIVE                   0x00000100 

/* Service Config - values for StartType field */
#define SVCCTL_BOOT_START                          0x00000000
#define SVCCTL_SYSTEM_START                        0x00000001
#define SVCCTL_AUTO_START                          0x00000002
#define SVCCTL_DEMAND_START                        0x00000003
#define SVCCTL_DISABLED                            0x00000004

/* Service Controls */

#define SVCCTL_CONTROL_STOP			0x00000001
#define SVCCTL_CONTROL_PAUSE			0x00000002
#define SVCCTL_CONTROL_CONTINUE			0x00000003
#define SVCCTL_CONTROL_INTERROGATE		0x00000004
#define SVCCTL_CONTROL_SHUTDOWN                 0x00000005

#define SVC_HANDLE_IS_SCM			0x0000001
#define SVC_HANDLE_IS_SERVICE			0x0000002
#define SVC_HANDLE_IS_DBLOCK			0x0000003

#define SVC_STATUS_PROCESS_INFO                 0x00000000

/* where we assume the location of the service control scripts */
#define SVCCTL_SCRIPT_DIR  "svcctl"

/* utility structures for RPCs */

typedef struct {
	uint32 type;
	uint32 state;
	uint32 controls_accepted;
	WERROR win32_exit_code;
	uint32 service_exit_code;
	uint32 check_point;
	uint32 wait_hint;
} SERVICE_STATUS;

typedef struct {
	SERVICE_STATUS status;
	uint32 process_id;
	uint32 service_flags;
} SERVICE_STATUS_PROCESS;


typedef struct {
	UNISTR servicename;
	UNISTR displayname;
	SERVICE_STATUS status;
} ENUM_SERVICES_STATUS;

typedef struct {
	uint32 service_type;
	uint32 start_type;
	uint32 error_control;
	UNISTR2 *executablepath;
	UNISTR2 *loadordergroup;
	uint32 tag_id;
	UNISTR2 *dependencies;
	UNISTR2 *startname;
	UNISTR2 *displayname;
} SERVICE_CONFIG;

typedef struct {
	uint32 unknown;	
        UNISTR description;
} SERVICE_DESCRIPTION;

typedef struct {
        uint32 type;
        uint32 delay;
} SC_ACTION;

typedef struct {
        uint32 reset_period;
        UNISTR2 *rebootmsg;	/* i have no idea if these are UNISTR2's.  I can't get a good trace */
        UNISTR2 *command;
        uint32  num_actions;
        SC_ACTION *actions;
} SERVICE_FAILURE_ACTIONS;

/* 
 * dispatch table of functions to handle the =ServiceControl API
 */ 
 
typedef struct {
	/* functions for enumerating subkeys and values */	
	WERROR 	(*stop_service)( const char *service, SERVICE_STATUS *status );
	WERROR 	(*start_service) ( const char *service );
	WERROR 	(*service_status)( const char *service, SERVICE_STATUS *status );
} SERVICE_CONTROL_OPS;

/* structure to store the service handle information  */

typedef struct _ServiceInfo {
	uint8			type;
	char			*name;
	uint32			access_granted;
	SERVICE_CONTROL_OPS	*ops;
} SERVICE_INFO;


/* rpc structures */

/**************************/

typedef struct {
	POLICY_HND handle;
} SVCCTL_Q_CLOSE_SERVICE;

typedef struct {
        POLICY_HND handle;
	WERROR status;
} SVCCTL_R_CLOSE_SERVICE;

/**************************/

typedef struct {
	UNISTR2 *servername;
	UNISTR2 *database; 
	uint32 access;
} SVCCTL_Q_OPEN_SCMANAGER;

typedef struct {
	POLICY_HND handle;
	WERROR status;
} SVCCTL_R_OPEN_SCMANAGER;

/**************************/

typedef struct {
	POLICY_HND handle;
	UNISTR2 servicename;
	uint32  display_name_len;
} SVCCTL_Q_GET_DISPLAY_NAME;

typedef struct {
	UNISTR2 displayname;
	uint32 display_name_len;
	WERROR status;
} SVCCTL_R_GET_DISPLAY_NAME;

/**************************/

typedef struct {
	POLICY_HND handle;
	UNISTR2 servicename;
	uint32 access;
} SVCCTL_Q_OPEN_SERVICE;

typedef struct {
	POLICY_HND handle;
	WERROR status;
} SVCCTL_R_OPEN_SERVICE;

/**************************/

typedef struct {
	POLICY_HND handle;
	uint32 parmcount;
	UNISTR4_ARRAY *parameters;
} SVCCTL_Q_START_SERVICE;

typedef struct {
	WERROR status;
} SVCCTL_R_START_SERVICE;

/**************************/

typedef struct {
	POLICY_HND handle;
	uint32 control;
} SVCCTL_Q_CONTROL_SERVICE;

typedef struct {
	SERVICE_STATUS svc_status;
	WERROR status;
} SVCCTL_R_CONTROL_SERVICE;

/**************************/

typedef struct {
	POLICY_HND handle;
} SVCCTL_Q_QUERY_STATUS;

typedef struct {
	SERVICE_STATUS svc_status;
	WERROR status;
} SVCCTL_R_QUERY_STATUS;

/**************************/

typedef struct {
	POLICY_HND handle;
	uint32 type;
	uint32 state;
	uint32 buffer_size;
	uint32 *resume;
} SVCCTL_Q_ENUM_SERVICES_STATUS;

typedef struct {
	RPC_BUFFER buffer;
	uint32 needed;
	uint32 returned;
	uint32 *resume;
	WERROR status;
} SVCCTL_R_ENUM_SERVICES_STATUS;

/**************************/

typedef struct {
	POLICY_HND handle;
	uint32 state;
	uint32 buffer_size;
} SVCCTL_Q_ENUM_DEPENDENT_SERVICES;

typedef struct {
	RPC_BUFFER buffer;
	uint32 needed;
	uint32 returned;
	WERROR status;
} SVCCTL_R_ENUM_DEPENDENT_SERVICES;


/**************************/

typedef struct {
	POLICY_HND handle;
	uint32 buffer_size;
} SVCCTL_Q_QUERY_SERVICE_CONFIG;

typedef struct {
	SERVICE_CONFIG config;
	uint32 needed;
	WERROR status;
} SVCCTL_R_QUERY_SERVICE_CONFIG;


/**************************/

typedef struct {
	POLICY_HND handle;
	uint32 level;
	uint32 buffer_size;
} SVCCTL_Q_QUERY_SERVICE_CONFIG2;

typedef struct {
	RPC_BUFFER buffer;
	uint32 needed;
	WERROR status;
} SVCCTL_R_QUERY_SERVICE_CONFIG2;


/**************************/

typedef struct {
	POLICY_HND handle;
        uint32 level;
	uint32 buffer_size;
} SVCCTL_Q_QUERY_SERVICE_STATUSEX;

typedef struct {
	RPC_BUFFER buffer;
	uint32 needed;
	WERROR status;
} SVCCTL_R_QUERY_SERVICE_STATUSEX;


/**************************/

typedef struct {
	POLICY_HND handle;
} SVCCTL_Q_LOCK_SERVICE_DB;

typedef struct {
	POLICY_HND h_lock;
	WERROR status;
} SVCCTL_R_LOCK_SERVICE_DB;


/**************************/

typedef struct {
	POLICY_HND h_lock;
} SVCCTL_Q_UNLOCK_SERVICE_DB;

typedef struct {
	WERROR status;
} SVCCTL_R_UNLOCK_SERVICE_DB;


/**************************/

typedef struct {
	POLICY_HND handle;
	uint32 security_flags;
	uint32 buffer_size;	
} SVCCTL_Q_QUERY_SERVICE_SEC;

typedef struct {
	RPC_BUFFER buffer;
	uint32 needed;
	WERROR status;
} SVCCTL_R_QUERY_SERVICE_SEC;

/**************************/

typedef struct {
	POLICY_HND handle; 
	uint32 security_flags;        
	RPC_BUFFER buffer;
	uint32 buffer_size;
} SVCCTL_Q_SET_SERVICE_SEC;

typedef struct {
	WERROR status;
} SVCCTL_R_SET_SERVICE_SEC;


#endif /* _RPC_SVCCTL_H */

