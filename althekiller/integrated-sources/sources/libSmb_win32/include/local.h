/* Copyright (C) 1995-1998 Samba-Team */
/* Copyright (C) 1998 John H Terpstra <jht@aquasoft.com.au> */

/* local definitions for file server */
#ifndef _LOCAL_H
#define _LOCAL_H

/* The default workgroup - usually overridden in smb.conf */
#ifndef WORKGROUP
#define WORKGROUP "WORKGROUP"
#endif

/* the maximum debug level to compile into the code. This assumes a good 
   optimising compiler that can remove unused code 
   for embedded or low-memory systems set this to a value like 2 to get
   only important messages. This gives *much* smaller binaries
*/
#ifndef MAX_DEBUG_LEVEL
#define MAX_DEBUG_LEVEL 1000
#endif

/* This defines the section name in the configuration file that will contain */
/* global parameters - that is, parameters relating to the whole server, not */
/* just services. This name is then reserved, and may not be used as a       */
/* a service name. It will default to "global" if not defined here.          */
#define GLOBAL_NAME "global"
#define GLOBAL_NAME2 "globals"

/* This defines the section name in the configuration file that will
   refer to the special "homes" service */
#define HOMES_NAME "homes"

/* This defines the section name in the configuration file that will
   refer to the special "printers" service */
#define PRINTERS_NAME "printers"

/* Yves Gaige <yvesg@hptnodur.grenoble.hp.com> requested this set this 	     */
/* to a maximum of 8 if old smb clients break because of long printer names. */
#define MAXPRINTERLEN 15

/* max number of directories open at once */
/* note that with the new directory code this no longer requires a
   file handle per directory, but large numbers do use more memory */
#define MAX_OPEN_DIRECTORIES 256

/* max number of directory handles */
/* As this now uses the bitmap code this can be
   quite large. */
#define MAX_DIRECTORY_HANDLES 2048

/* maximum number of file caches per smbd */
#define MAX_WRITE_CACHES 10

/* define what facility to use for syslog */
#ifndef SYSLOG_FACILITY
#define SYSLOG_FACILITY LOG_DAEMON
#endif

/* 
 * Default number of maximum open files per smbd. This is
 * also limited by the maximum available file descriptors
 * per process and can also be set in smb.conf as "max open files"
 * in the [global] section.
 */

#ifndef MAX_OPEN_FILES
#define MAX_OPEN_FILES 10000
#endif
 
#define WORDMAX 0xFFFF

/* the maximum password length before we declare a likely attack */
#define MAX_PASS_LEN 200

/* separators for lists */
#define LIST_SEP " \t,;\n\r"

/* wchar separators for lists */
#define LIST_SEP_W wchar_list_sep

/* this is where browse lists are kept in the lock dir */
#define SERVER_LIST "browse.dat"

/* shall filenames with illegal chars in them get mangled in long
   filename listings? */
#define MANGLE_LONG_FILENAMES 

/* define this if you want to stop spoofing with .. and soft links
   NOTE: This also slows down the server considerably */
#define REDUCE_PATHS

/* the size of the directory cache */
#define DIRCACHESIZE 20

/* what default type of filesystem do we want this to show up as in a
   NT file manager window? */
#define FSTYPE_STRING "NTFS"

/* the default guest account - normally set in the Makefile or smb.conf */
#ifndef GUEST_ACCOUNT
#define GUEST_ACCOUNT "nobody"
#endif

/* user to test password server with as invalid in security=server mode. */
#ifndef INVALID_USER_PREFIX
#define INVALID_USER_PREFIX "sambatest"
#endif

/* the default pager to use for the client "more" command. Users can
   override this with the PAGER environment variable */
#ifndef PAGER
#define PAGER "more"
#endif

/* the size of the uid cache used to reduce valid user checks */
#define VUID_CACHE_SIZE 32

/* the following control timings of various actions. Don't change 
   them unless you know what you are doing. These are all in seconds */
#define DEFAULT_SMBD_TIMEOUT (60*60*24*7)
#define SMBD_RELOAD_CHECK (180)
#define IDLE_CLOSED_TIMEOUT (60)
#define DPTR_IDLE_TIMEOUT (120)
#define SMBD_SELECT_TIMEOUT (60)
#define NMBD_SELECT_LOOP (10)
#define BROWSE_INTERVAL (60)
#define REGISTRATION_INTERVAL (10*60)
#define NMBD_INETD_TIMEOUT (120)
#define NMBD_MAX_TTL (24*60*60)
#define LPQ_LOCK_TIMEOUT (5)
#define NMBD_INTERFACES_RELOAD (120)
#define NMBD_UNEXPECTED_TIMEOUT (15)

/* the following are in milliseconds */
#define LOCK_RETRY_TIMEOUT (100)

/* do you want to dump core (carefully!) when an internal error is
   encountered? Samba will be careful to make the core file only
   accessible to root */
#define DUMP_CORE 1

/* shall we support browse requests via a FIFO to nmbd? */
#define ENABLE_FIFO 1

/* how long (in miliseconds) to wait for a socket connect to happen */
#define LONG_CONNECT_TIMEOUT 30000
#define SHORT_CONNECT_TIMEOUT 5000

/* the default netbios keepalive timeout */
#define DEFAULT_KEEPALIVE 300

/* the directory to sit in when idle */
/* #define IDLE_DIR "/" */

/* Timout (in seconds) to wait for an oplock break
   message to return from the client. */

#define OPLOCK_BREAK_TIMEOUT 30

/* Timout (in seconds) to add to the oplock break timeout
   to wait for the smbd to smbd message to return. */

#define OPLOCK_BREAK_TIMEOUT_FUDGEFACTOR 2

/* the read preciction code has been disabled until some problems with
   it are worked out */
#define USE_READ_PREDICTION 0

/*
 * Default passwd chat script.
 */

#define DEFAULT_PASSWD_CHAT "*new*password* %n\\n *new*password* %n\\n *changed*"

/* Minimum length of allowed password when changing UNIX password. */
#define MINPASSWDLENGTH 5

/* maximum ID number used for session control. This cannot be larger
   than 62*62 for the current code */
#define MAX_SESSION_ID 3000

/* For the benifit of PAM and the 'session exec' scripts, we fake up a terminal
   name. This can be in one of two forms:  The first for systems not using
   utmp (and therefore not constrained as to length or the need for a number
   < 3000 or so) and the second for systems with this 'well behaved terminal
   like name' constraint.
*/

#ifndef SESSION_TEMPLATE
/* Paramaters are 'pid' and 'vuid' */
#define SESSION_TEMPLATE "smb/%lu/%d"
#endif

#ifndef SESSION_UTMP_TEMPLATE
#define SESSION_UTMP_TEMPLATE "smb/%d"
#endif

/* the maximum age in seconds of a password. Should be a lp_ parameter */
#define MAX_PASSWORD_AGE (21*24*60*60)

/* Default allocation roundup. */
#define SMB_ROUNDUP_ALLOCATION_SIZE 0x100000

/* shall we deny oplocks to clients that get timeouts? */
#define FASCIST_OPLOCK_BACKOFF 1

/* this enables the "rabbit pellet" fix for SMBwritebraw */
#define RABBIT_PELLET_FIX 1

/* Max number of jobs per print queue. */
#define PRINT_MAX_JOBID 10000

/* Max number of open RPC pipes. */
#define MAX_OPEN_PIPES 2048

/* Tuning for server auth mutex. */
#define CLI_AUTH_TIMEOUT 5000 /* In milli-seconds. */
#define NUM_CLI_AUTH_CONNECT_RETRIES 3
/* Number in seconds to wait for the mutex. This must be less than 30 seconds. */
#define SERVER_MUTEX_WAIT_TIME ( ((NUM_CLI_AUTH_CONNECT_RETRIES) * ((CLI_AUTH_TIMEOUT)/1000)) + 5)
/* Number in seconds for winbindd to wait for the mutex. Make this 2 * smbd wait time. */
#define WINBIND_SERVER_MUTEX_WAIT_TIME (( ((NUM_CLI_AUTH_CONNECT_RETRIES) * ((CLI_AUTH_TIMEOUT)/1000)) + 5)*2)

/* Max number of simultaneous winbindd socket connections. */
#define WINBINDD_MAX_SIMULTANEOUS_CLIENTS 200

/* Buffer size to use when printing backtraces */
#define BACKTRACE_STACK_SIZE 64

/* size of listen() backlog in smbd */
#define SMBD_LISTEN_BACKLOG 50

/* Number of microseconds to wait before a sharing violation. */
#define SHARING_VIOLATION_USEC_WAIT 950000

#define MAX_LDAP_REPLICATION_SLEEP_TIME 5000 /* In milliseconds. */

/* tdb hash size for the open database. */
#define SMB_OPEN_DATABASE_TDB_HASH_SIZE 10007

/* Characters we disallow in sharenames. */
#define INVALID_SHARENAME_CHARS "%<>*?|/\\+=;:\","

/* Seconds between connection attempts to a remote server. */
#define FAILED_CONNECTION_CACHE_TIMEOUT 30

/* Default hash size for the winbindd cache. */
#define WINBINDD_CACHE_TDB_DEFAULT_HASH_SIZE 5000

#endif
