#ifndef _INCLUDES_H
#define _INCLUDES_H
/* 
   Unix SMB/CIFS implementation.
   Machine customisation and include handling
   Copyright (C) Andrew Tridgell 1994-1998
   Copyright (C) 2002 by Martin Pool <mbp@samba.org>
   
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

/* work around broken krb5.h on sles9 */
#ifdef SIZEOF_LONG
#undef SIZEOF_LONG
#endif

#ifndef NO_CONFIG_H /* for some tests */
#include "config.h"
#endif

#ifdef _XBOX
#define GENERIC_MAPPING GENERIC_MAPPING_MS
#define PRIVILEGE_SET PRIVILEGE_SET_MS
#define SYSTEMTIME SYSTEMTIME_MS
#define LUID LUID_MS
#ifdef _WIN32
#define NOCRYPT
#define _WINSPOOL_
#define _WINPERF_
#define _WINSVC_
#include <windows.h>
#else
#include <xtl.h>
#endif
#undef LUID
#undef SYSTEMTIME 
#undef PRIVILEGE_SET
#undef GENERIC_MAPPING

#include <io.h>
#include <fcntl.h>

#include "xb_defines.h"
#ifndef _WIN32
#include "xb_socketutil.h"
#endif
#include "xb_util.h"
#include "dirent.h"

#undef ERROR_INVALID_PARAMETER
#undef ERROR_INSUFFICIENT_BUFFER
#undef ERROR_INVALID_DATATYPE

#undef close

#endif // _XBOX


/* only do the C++ reserved word check when we compile
   to include --with-developer since too many systems
   still have comflicts with their header files (e.g. IRIX 6.4) */

#if !defined(__cplusplus) && defined(DEVELOPER)
#define class #error DONT_USE_CPLUSPLUS_RESERVED_NAMES
#define private #error DONT_USE_CPLUSPLUS_RESERVED_NAMES
#define public #error DONT_USE_CPLUSPLUS_RESERVED_NAMES
#define protected #error DONT_USE_CPLUSPLUS_RESERVED_NAMES
#define template #error DONT_USE_CPLUSPLUS_RESERVED_NAMES
#define this #error DONT_USE_CPLUSPLUS_RESERVED_NAMES
#define new #error DONT_USE_CPLUSPLUS_RESERVED_NAMES
#define delete #error DONT_USE_CPLUSPLUS_RESERVED_NAMES
#define friend #error DONT_USE_CPLUSPLUS_RESERVED_NAMES
#endif

#include "local.h"

#ifdef AIX
#define DEFAULT_PRINTING PRINT_AIX
#define PRINTCAP_NAME "/etc/qconfig"
#endif

#ifdef HPUX
#define DEFAULT_PRINTING PRINT_HPUX
#endif

#ifdef QNX
#define DEFAULT_PRINTING PRINT_QNX
#endif

#ifdef SUNOS4
/* on SUNOS4 termios.h conflicts with sys/ioctl.h */
#undef HAVE_TERMIOS_H
#endif

#if (__GNUC__ >= 3 ) && (__GNUC_MINOR__ >= 1 )
/** Use gcc attribute to check printf fns.  a1 is the 1-based index of
 * the parameter containing the format, and a2 the index of the first
 * argument. Note that some gcc 2.x versions don't handle this
 * properly **/
#define PRINTF_ATTRIBUTE(a1, a2) __attribute__ ((format (__printf__, a1, a2)))
#else
#define PRINTF_ATTRIBUTE(a1, a2)
#endif

#if defined(__GNUC__) && !defined(__cplusplus)
/** gcc attribute used on function parameters so that it does not emit
 * warnings about them being unused. **/
#  define UNUSED(param) param __attribute__ ((unused))
#else
#  define UNUSED(param) param
/** Feel free to add definitions for other compilers here. */
#endif

#ifdef RELIANTUNIX
/*
 * <unistd.h> has to be included before any other to get
 * large file support on Reliant UNIX. Yes, it's broken :-).
 */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif /* RELIANTUNIX */

#include <sys/types.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#include <stddef.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_UNIXSOCKET
#include <sys/un.h>
#endif

#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#elif HAVE_SYSCALL_H
#include <syscall.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#ifdef HAVE_SYS_FCNTL_H
#include <sys/fcntl.h>
#endif
#endif

#include <sys/stat.h>

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

#include <signal.h>

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif
#ifdef HAVE_GRP_H
#include <grp.h>
#endif
#ifdef HAVE_SYS_PRIV_H
#include <sys/priv.h>
#endif
#ifdef HAVE_SYS_ID_H
#include <sys/id.h>
#endif

#include <errno.h>

#ifdef HAVE_UTIME_H
#include <utime.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef HAVE_SYS_MODE_H
/* apparently AIX needs this for S_ISLNK */
#ifndef S_ISLNK
#include <sys/mode.h>
#endif
#endif

#ifdef HAVE_GLOB_H
#include <glob.h>
#endif

#ifdef _XBOX
#elif
#include <pwd.h>
#endif

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#ifdef _XBOX

#elif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#else
#ifdef HAVE_SYS_SYSLOG_H
#include <sys/syslog.h>
#endif
#endif

#ifdef _XBOX
#elif
#include <sys/file.h>
#endif

#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

/*
 * The next three defines are needed to access the IPTOS_* options
 * on some systems.
 */

#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif

#ifdef HAVE_NETINET_IN_IP_H
#include <netinet/in_ip.h>
#endif

#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif

#if defined(HAVE_TERMIOS_H)
/* POSIX terminal handling. */
#include <termios.h>
#elif defined(HAVE_TERMIO_H)
/* Older SYSV terminal handling - don't use if we can avoid it. */
#include <termio.h>
#elif defined(HAVE_SYS_TERMIO_H)
/* Older SYSV terminal handling - don't use if we can avoid it. */
#include <sys/termio.h>
#endif

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif


#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif

#ifdef HAVE_SYS_ACL_H
#include <sys/acl.h>
#endif

#ifdef HAVE_SYS_FS_S5PARAM_H 
#include <sys/fs/s5param.h>
#endif

#if defined (HAVE_SYS_FILSYS_H) && !defined (_CRAY)
#include <sys/filsys.h> 
#endif

#ifdef HAVE_SYS_STATFS_H
# include <sys/statfs.h>
#endif

#ifdef HAVE_DUSTAT_H              
#include <sys/dustat.h>
#endif

#ifdef HAVE_SYS_STATVFS_H          
#include <sys/statvfs.h>
#endif

#ifdef HAVE_SHADOW_H
/*
 * HP-UX 11.X has TCP_NODELAY and TCP_MAXSEG defined in <netinet/tcp.h> which
 * was included above.  However <rpc/rpc.h> includes <sys/xti.h> which defines
 * them again without checking if they already exsist.  This generates
 * two "Redefinition of macro" warnings for every single .c file that is
 * compiled.
 */
#if defined(HPUX) && defined(TCP_NODELAY)
#undef TCP_NODELAY
#endif
#if defined(HPUX) && defined(TCP_MAXSEG)
#undef TCP_MAXSEG
#endif
#include <shadow.h>
#endif

#ifdef HAVE_GETPWANAM
#include <sys/label.h>
#include <sys/audit.h>
#include <pwdadj.h>
#endif

#ifdef HAVE_SYS_SECURITY_H
#include <sys/security.h>
#include <prot.h>
#define PASSWORD_LENGTH 16
#endif  /* HAVE_SYS_SECURITY_H */

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif

#ifdef HAVE_POLL_H
#include <poll.h>
#endif

#if defined(HAVE_RPC_RPC_H)
/*
 * Check for AUTH_ERROR define conflict with rpc/rpc.h in prot.h.
 */
#if defined(HAVE_SYS_SECURITY_H) && defined(HAVE_RPC_AUTH_ERROR_CONFLICT)
#undef AUTH_ERROR
#endif
/*
 * HP-UX 11.X has TCP_NODELAY and TCP_MAXSEG defined in <netinet/tcp.h> which
 * was included above.  However <rpc/rpc.h> includes <sys/xti.h> which defines
 * them again without checking if they already exsist.  This generates
 * two "Redefinition of macro" warnings for every single .c file that is
 * compiled.
 */
#if defined(HPUX) && defined(TCP_NODELAY)
#undef TCP_NODELAY
#endif
#if defined(HPUX) && defined(TCP_MAXSEG)
#undef TCP_MAXSEG
#endif
#include <rpc/rpc.h>
#endif

#if defined(HAVE_YP_GET_DEFAULT_DOMAIN) && defined(HAVE_SETNETGRENT) && defined(HAVE_ENDNETGRENT) && defined(HAVE_GETNETGRENT)
#define HAVE_NETGROUP 1
#endif

#if defined (HAVE_NETGROUP)
#if defined(HAVE_RPCSVC_YP_PROT_H)
/*
 * HP-UX 11.X has TCP_NODELAY and TCP_MAXSEG defined in <netinet/tcp.h> which
 * was included above.  However <rpc/rpc.h> includes <sys/xti.h> which defines
 * them again without checking if they already exsist.  This generates
 * two "Redefinition of macro" warnings for every single .c file that is
 * compiled.
 */
#if defined(HPUX) && defined(TCP_NODELAY)
#undef TCP_NODELAY
#endif
#if defined(HPUX) && defined(TCP_MAXSEG)
#undef TCP_MAXSEG
#endif
#include <rpcsvc/yp_prot.h>
#endif
#if defined(HAVE_RPCSVC_YPCLNT_H)
#include <rpcsvc/ypclnt.h>
#endif
#endif /* HAVE_NETGROUP */

#if defined(HAVE_SYS_IPC_H)
#include <sys/ipc.h>
#endif /* HAVE_SYS_IPC_H */

#if defined(HAVE_SYS_SHM_H)
#include <sys/shm.h>
#endif /* HAVE_SYS_SHM_H */

#ifdef HAVE_NATIVE_ICONV
#ifdef HAVE_ICONV
#include <iconv.h>
#endif
#ifdef HAVE_GICONV
#include <giconv.h>
#endif
#ifdef HAVE_BICONV
#include <biconv.h>
#endif
#endif

#if HAVE_KRB5_H
#include <krb5.h>
#else
#undef HAVE_KRB5
#endif

#if HAVE_LBER_H
#include <lber.h>
#ifndef LBER_USE_DER
#define LBER_USE_DER 0x01
#endif
#endif

#if HAVE_LDAP_H
#include <ldap.h>
#ifndef LDAP_CONST
#define LDAP_CONST const
#endif
#ifndef LDAP_OPT_SUCCESS
#define LDAP_OPT_SUCCESS 0
#endif
/* Solaris 8 and maybe other LDAP implementations spell this "..._INPROGRESS": */
#if defined(LDAP_SASL_BIND_INPROGRESS) && !defined(LDAP_SASL_BIND_IN_PROGRESS)
#define LDAP_SASL_BIND_IN_PROGRESS LDAP_SASL_BIND_INPROGRESS
#endif
/* Solaris 8 defines SSL_LDAP_PORT, not LDAPS_PORT and it only does so if
   LDAP_SSL is defined - but SSL is not working. We just want the
   port number! Let's just define LDAPS_PORT correct. */
#if !defined(LDAPS_PORT)
#define LDAPS_PORT 636
#endif
#else
#undef HAVE_LDAP
#endif

#if HAVE_GSSAPI_H
#include <gssapi.h>
#elif HAVE_GSSAPI_GSSAPI_H
#include <gssapi/gssapi.h>
#elif HAVE_GSSAPI_GSSAPI_GENERIC_H
#include <gssapi/gssapi_generic.h>
#endif

#if HAVE_COM_ERR_H
#include <com_err.h>
#endif

#if HAVE_SYS_ATTRIBUTES_H
#include <sys/attributes.h>
#endif

/* mutually exclusive (SuSE 8.2) */
#if HAVE_ATTR_XATTR_H
#include <attr/xattr.h>
#elif HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#ifdef HAVE_SYS_EA_H
#include <sys/ea.h>
#endif

#ifdef HAVE_SYS_EXTATTR_H
#include <sys/extattr.h>
#endif

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#if HAVE_LOCALE_H
#include <locale.h>
#endif

#if HAVE_LANGINFO_H
#include <langinfo.h>
#endif

#if defined(HAVE_AIO_H) && defined(WITH_AIO)
#include <aio.h>
#endif

/* skip valgrind headers on 64bit AMD boxes */
#ifndef HAVE_64BIT_LINUX
/* Special macros that are no-ops except when run under Valgrind on
 * x86.  They've moved a little bit from valgrind 1.0.4 to 1.9.4 */
#if HAVE_VALGRIND_MEMCHECK_H
        /* memcheck.h includes valgrind.h */
#include <valgrind/memcheck.h>
#elif HAVE_VALGRIND_H
#include <valgrind.h>
#endif
#endif

/* If we have --enable-developer and the valgrind header is present,
 * then we're OK to use it.  Set a macro so this logic can be done only
 * once. */
#if defined(DEVELOPER) && !defined(HAVE_64BIT_LINUX)
#if (HAVE_VALGRIND_H || HAVE_VALGRIND_VALGRIND_H)
#define VALGRIND
#endif
#endif


/* we support ADS if we want it and have krb5 and ldap libs */
#if defined(WITH_ADS) && defined(HAVE_KRB5) && defined(HAVE_LDAP)
#define HAVE_ADS
#endif

/*
 * Define VOLATILE if needed.
 */

#if defined(HAVE_VOLATILE)
#define VOLATILE volatile
#else
#define VOLATILE
#endif

/*
 * Define additional missing types
 */
#if defined(HAVE_SIG_ATOMIC_T_TYPE) && defined(AIX)
typedef sig_atomic_t SIG_ATOMIC_T;
#elif defined(HAVE_SIG_ATOMIC_T_TYPE) && !defined(AIX)
typedef sig_atomic_t VOLATILE SIG_ATOMIC_T;
#else
typedef int VOLATILE SIG_ATOMIC_T;
#endif

#ifndef HAVE_SOCKLEN_T_TYPE
#define HAVE_SOCKLEN_T_TYPE
typedef int socklen_t;
#endif


#ifndef uchar
#define uchar unsigned char
#endif

#ifdef HAVE_UNSIGNED_CHAR
#define schar signed char
#else
#define schar char
#endif

/*
   Samba needs type definitions for int16, int32, uint16 and uint32.

   Normally these are signed and unsigned 16 and 32 bit integers, but
   they actually only need to be at least 16 and 32 bits
   respectively. Thus if your word size is 8 bytes just defining them
   as signed and unsigned int will work.
*/

#ifndef uint8
#define uint8 unsigned char
#endif

#if !defined(int16) && !defined(HAVE_INT16_FROM_RPC_RPC_H)
#  if (SIZEOF_SHORT == 4)
#    define int16 __ERROR___CANNOT_DETERMINE_TYPE_FOR_INT16;
#  else /* SIZEOF_SHORT != 4 */
#    define int16 short
#  endif /* SIZEOF_SHORT != 4 */
   /* needed to work around compile issue on HP-UX 11.x */
#  define _INT16	1
#endif

/*
 * Note we duplicate the size tests in the unsigned 
 * case as int16 may be a typedef from rpc/rpc.h
 */

#if !defined(uint16) && !defined(HAVE_UINT16_FROM_RPC_RPC_H)
#if (SIZEOF_SHORT == 4)
#define uint16 __ERROR___CANNOT_DETERMINE_TYPE_FOR_INT16;
#else /* SIZEOF_SHORT != 4 */
#define uint16 unsigned short
#endif /* SIZEOF_SHORT != 4 */
#endif

#if !defined(int32) && !defined(HAVE_INT32_FROM_RPC_RPC_H)
#  if (SIZEOF_INT == 4)
#    define int32 int
#  elif (SIZEOF_LONG == 4)
#    define int32 long
#  elif (SIZEOF_SHORT == 4)
#    define int32 short
#  else
     /* uggh - no 32 bit type?? probably a CRAY. just hope this works ... */
#    define int32 int
#  endif
   /* needed to work around compile issue on HP-UX 11.x */
#  define _INT32	1
#endif

/*
 * Note we duplicate the size tests in the unsigned 
 * case as int32 may be a typedef from rpc/rpc.h
 */

#if !defined(uint32) && !defined(HAVE_UINT32_FROM_RPC_RPC_H)
#if (SIZEOF_INT == 4)
#define uint32 unsigned int
#elif (SIZEOF_LONG == 4)
#define uint32 unsigned long
#elif (SIZEOF_SHORT == 4)
#define uint32 unsigned short
#else
/* uggh - no 32 bit type?? probably a CRAY. just hope this works ... */
#define uint32 unsigned
#endif
#endif

/*
 * check for 8 byte long long
 */

#if !defined(uint64)
#if (SIZEOF_LONG == 8)
#define uint64 unsigned long
#elif (SIZEOF_LONG_LONG == 8)
#define uint64 unsigned long long
#endif	/* don't lie.  If we don't have it, then don't use it */
#endif

#if !defined(int64)
#if (SIZEOF_LONG == 8)
#define int64 long
#elif (SIZEOF_LONG_LONG == 8)
#define int64 long long
#endif	/* don't lie.  If we don't have it, then don't use it */
#endif


/*
 * Types for devices, inodes and offsets.
 */

#ifndef SMB_DEV_T
#  if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_DEV64_T)
#    define SMB_DEV_T dev64_t
#  else
#    define SMB_DEV_T dev_t
#  endif
#endif

#ifndef LARGE_SMB_DEV_T
#  if (defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_DEV64_T)) || (defined(SIZEOF_DEV_T) && (SIZEOF_DEV_T == 8))
#    define LARGE_SMB_DEV_T 1
#  endif
#endif

#ifdef LARGE_SMB_DEV_T
#define SDEV_T_VAL(p, ofs, v) (SIVAL((p),(ofs),(v)&0xFFFFFFFF), SIVAL((p),(ofs)+4,(v)>>32))
#define DEV_T_VAL(p, ofs) ((SMB_DEV_T)(((SMB_BIG_UINT)(IVAL((p),(ofs))))| (((SMB_BIG_UINT)(IVAL((p),(ofs)+4))) << 32)))
#else 
#define SDEV_T_VAL(p, ofs, v) (SIVAL((p),(ofs),v),SIVAL((p),(ofs)+4,0))
#define DEV_T_VAL(p, ofs) ((SMB_DEV_T)(IVAL((p),(ofs))))
#endif

/*
 * Setup the correctly sized inode type.
 */

#ifndef SMB_INO_T
#  if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_INO64_T)
#    define SMB_INO_T ino64_t
#  else
#    define SMB_INO_T ino_t
#  endif
#endif

#ifndef LARGE_SMB_INO_T
#  if (defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_INO64_T)) || (defined(SIZEOF_INO_T) && (SIZEOF_INO_T == 8))
#    define LARGE_SMB_INO_T 1
#  endif
#endif

#ifdef LARGE_SMB_INO_T
#define SINO_T_VAL(p, ofs, v) (SIVAL((p),(ofs),(v)&0xFFFFFFFF), SIVAL((p),(ofs)+4,(v)>>32))
#define INO_T_VAL(p, ofs) ((SMB_INO_T)(((SMB_BIG_UINT)(IVAL(p,ofs)))| (((SMB_BIG_UINT)(IVAL(p,(ofs)+4))) << 32)))
#else 
#define SINO_T_VAL(p, ofs, v) (SIVAL(p,ofs,v),SIVAL(p,(ofs)+4,0))
#define INO_T_VAL(p, ofs) ((SMB_INO_T)(IVAL((p),(ofs))))
#endif

#ifndef SMB_OFF_T
#  if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_OFF64_T)
#    define SMB_OFF_T long long
#  else
#    define SMB_OFF_T off_t
#  endif
#endif

#if defined(HAVE_LONGLONG)
#define SMB_BIG_UINT unsigned long long
#define SMB_BIG_INT long long
#define SBIG_UINT(p, ofs, v) (SIVAL(p,ofs,(v)&0xFFFFFFFF), SIVAL(p,(ofs)+4,(v)>>32))
#else
#define SMB_BIG_UINT unsigned long
#define SMB_BIG_INT long
#define SBIG_UINT(p, ofs, v) (SIVAL(p,ofs,v),SIVAL(p,(ofs)+4,0))
#endif

#define SMB_BIG_UINT_BITS (sizeof(SMB_BIG_UINT)*8)

/* this should really be a 64 bit type if possible */
#define br_off SMB_BIG_UINT

#define SMB_OFF_T_BITS (sizeof(SMB_OFF_T)*8)

/*
 * Set the define that tells us if we can do 64 bit
 * NT SMB calls.
 */

#ifndef LARGE_SMB_OFF_T
#  if (defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_OFF64_T)) || (defined(SIZEOF_OFF_T) && (SIZEOF_OFF_T == 8))
#    define LARGE_SMB_OFF_T 1
#  endif
#endif

#ifdef LARGE_SMB_OFF_T
#define SOFF_T(p, ofs, v) (SIVAL(p,ofs,(v)&0xFFFFFFFF), SIVAL(p,(ofs)+4,(v)>>32))
#define SOFF_T_R(p, ofs, v) (SIVAL(p,(ofs)+4,(v)&0xFFFFFFFF), SIVAL(p,ofs,(v)>>32))
#define IVAL_TO_SMB_OFF_T(buf,off) ((SMB_OFF_T)(( ((SMB_BIG_UINT)(IVAL((buf),(off)))) & ((SMB_BIG_UINT)0xFFFFFFFF) )))
#define IVAL2_TO_SMB_BIG_UINT(buf,off) ( (((SMB_BIG_UINT)(IVAL((buf),(off)))) & ((SMB_BIG_UINT)0xFFFFFFFF)) | \
		(( ((SMB_BIG_UINT)(IVAL((buf),(off+4)))) & ((SMB_BIG_UINT)0xFFFFFFFF) ) << 32 ) )
#else 
#define SOFF_T(p, ofs, v) (SIVAL(p,ofs,v),SIVAL(p,(ofs)+4,0))
#define SOFF_T_R(p, ofs, v) (SIVAL(p,(ofs)+4,v),SIVAL(p,ofs,0))
#define IVAL_TO_SMB_OFF_T(buf,off) ((SMB_OFF_T)(( ((uint32)(IVAL((buf),(off)))) & 0xFFFFFFFF )))
#define IVAL2_TO_SMB_BIG_UINT(buf,off) ( (((SMB_BIG_UINT)(IVAL((buf),(off)))) & ((SMB_BIG_UINT)0xFFFFFFFF)) | \
		                (( ((SMB_BIG_UINT)(IVAL((buf),(off+4)))) & ((SMB_BIG_UINT)0xFFFFFFFF) ) << 32 ) )
#endif

/*
 * Type for stat structure.
 */

#ifndef SMB_STRUCT_STAT
#  if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_STAT64) && defined(HAVE_OFF64_T)
#    define SMB_STRUCT_STAT struct __stat64
#  else
#    define SMB_STRUCT_STAT struct stat
#  endif
#endif

/*
 * Type for dirent structure.
 */

#ifndef SMB_STRUCT_DIRENT
#  if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_STRUCT_DIRENT64)
#    define SMB_STRUCT_DIRENT struct dirent64
#  else
#    define SMB_STRUCT_DIRENT struct dirent
#  endif
#endif

/*
 * Type for DIR structure.
 */

#ifndef SMB_STRUCT_DIR
#  if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_STRUCT_DIR64)
#    define SMB_STRUCT_DIR DIR64
#  else
#    define SMB_STRUCT_DIR DIR
#  endif
#endif

/*
 * Defines for 64 bit fcntl locks.
 */

#ifndef SMB_STRUCT_FLOCK
#  if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_STRUCT_FLOCK64) && defined(HAVE_OFF64_T)
#    define SMB_STRUCT_FLOCK struct flock64
#  else
#    define SMB_STRUCT_FLOCK struct flock
#  endif
#endif

#ifndef SMB_F_SETLKW
#  if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_STRUCT_FLOCK64) && defined(HAVE_OFF64_T)
#    define SMB_F_SETLKW F_SETLKW64
#  else
#    define SMB_F_SETLKW F_SETLKW
#  endif
#endif

#ifndef SMB_F_SETLK
#  if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_STRUCT_FLOCK64) && defined(HAVE_OFF64_T)
#    define SMB_F_SETLK F_SETLK64
#  else
#    define SMB_F_SETLK F_SETLK
#  endif
#endif

#ifndef SMB_F_GETLK
#  if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_STRUCT_FLOCK64) && defined(HAVE_OFF64_T)
#    define SMB_F_GETLK F_GETLK64
#  else
#    define SMB_F_GETLK F_GETLK
#  endif
#endif

/*
 * Type for aiocb structure.
 */

#ifndef SMB_STRUCT_AIOCB
#  if defined(WITH_AIO)
#    if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_AIOCB64)
#      define SMB_STRUCT_AIOCB struct aiocb64
#    else
#      define SMB_STRUCT_AIOCB struct aiocb
#    endif
#  else
#    define SMB_STRUCT_AIOCB int /* AIO not being used but we still need the define.... */
#  endif
#endif

#ifndef HAVE_STRUCT_TIMESPEC
struct timespec {
	time_t tv_sec;            /* Seconds.  */
	long tv_nsec;           /* Nanoseconds.  */
};
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(i) sys_errlist[i]
#endif

#ifndef HAVE_ERRNO_DECL
extern int errno;
#endif

#ifdef HAVE_BROKEN_GETGROUPS
#define GID_T int
#else
#define GID_T gid_t
#endif

#ifndef NGROUPS_MAX
#define NGROUPS_MAX 32 /* Guess... */
#endif

#ifdef SOCKET_WRAPPER
#define SOCKET_WRAPPER_REPLACE
#include "include/socket_wrapper.h"
#endif

/* Our own pstrings and fstrings */
#include "pstring.h"

/* Lists, trees, caching, database... */
#include "xfile.h"
#include "intl.h"
#include "dlinklist.h"
#include "tdb/tdb.h"
#include "tdb/spinlock.h"
#include "tdb/tdbutil.h"

#include "talloc.h"
/* And a little extension. Abort on type mismatch */
#define talloc_get_type_abort(ptr, type) \
	(type *)talloc_check_name_abort(ptr, #type)

#include "nt_status.h"
#include "ads.h"
#include "ads_dns.h"
#include "interfaces.h"
#include "trans2.h"
#include "nterr.h"
#include "ntioctl.h"
#include "messages.h"
#include "charset.h"
#include "dynconfig.h"
#include "util_getent.h"
#include "debugparse.h"
#include "version.h"
#include "privileges.h"
#include "smb.h"
#include "ads_cldap.h"
#include "nameserv.h"
#include "secrets.h"
#include "byteorder.h"
#include "privileges.h"
#include "rpc_misc.h"
#include "rpc_dce.h"
#include "mapping.h"
#include "passdb.h"
#include "rpc_secdes.h"
#include "authdata.h"
#include "msdfs.h"
#include "rap.h"
#include "md5.h"
#include "hmacmd5.h"
#include "ntlmssp.h"
#include "auth.h"
#include "ntdomain.h"
#include "rpc_svcctl.h"
#include "rpc_ntsvcs.h"
#include "rpc_lsa.h"
#include "rpc_netlogon.h"
#include "reg_objects.h"
#include "rpc_reg.h"
#include "rpc_samr.h"
#include "rpc_srvsvc.h"
#include "rpc_wkssvc.h"
#include "rpc_spoolss.h"
#include "rpc_eventlog.h"
#include "rpc_dfs.h"
#include "rpc_ds.h"
#include "rpc_echo.h"
#include "rpc_shutdown.h"
#include "rpc_perfcount.h"
#include "rpc_perfcount_defs.h"
#include "nt_printing.h"
#include "idmap.h"
#include "client.h"

#ifdef WITH_SMBWRAPPER
#include "smbw.h"
#endif

#include "session.h"
#include "asn_1.h"
#include "popt.h"
#include "mangle.h"
#include "module.h"
#include "nsswitch/winbind_client.h"
#include "spnego.h"
#include "rpc_client.h"
#include "event.h"

/*
 * Type for wide character dirent structure.
 * Only d_name is defined by POSIX.
 */

typedef struct smb_wdirent {
	wpstring        d_name;
} SMB_STRUCT_WDIRENT;

/*
 * Type for wide character passwd structure.
 */

typedef struct smb_wpasswd {
	wfstring       pw_name;
	char           *pw_passwd;
	uid_t          pw_uid;
	gid_t          pw_gid;
	wpstring       pw_gecos;
	wpstring       pw_dir;
	wpstring       pw_shell;
} SMB_STRUCT_WPASSWD;

/* used in net.c */
struct functable {
	const char *funcname;
	int (*fn)(int argc, const char **argv);
};

struct functable2 {
	const char *funcname;
	int (*fn)(int argc, const char **argv);
	const char *helptext;
};

/* Defines for wisXXX functions. */
#define UNI_UPPER    0x1
#define UNI_LOWER    0x2
#define UNI_DIGIT    0x4
#define UNI_XDIGIT   0x8
#define UNI_SPACE    0x10

#include "nsswitch/winbind_nss.h"

/* forward declaration from printing.h to get around 
   header file dependencies */

struct printjob;

struct smb_ldap_privates;

/* forward declarations from smbldap.c */

#include "smbldap.h"

#include "smb_ldap.h"

/*
 * Reasons for cache flush.
 */

enum flush_reason_enum {
    SEEK_FLUSH,
    READ_FLUSH,
    WRITE_FLUSH,
    READRAW_FLUSH,
    OPLOCK_RELEASE_FLUSH,
    CLOSE_FLUSH,
    SYNC_FLUSH,
    SIZECHANGE_FLUSH,
    /* NUM_FLUSH_REASONS must remain the last value in the enumeration. */
    NUM_FLUSH_REASONS};

/***** automatically generated prototypes *****/
#ifndef NO_PROTO_H
#include "proto.h"
#endif

/* We need this after proto.h to reference GetTimeOfDay(). */
#include "smbprofile.h"

/* String routines */

#include "srvstr.h"
#include "safe_string.h"

#ifdef __COMPAR_FN_T
#define QSORT_CAST (__compar_fn_t)
#endif

#ifndef QSORT_CAST
#define QSORT_CAST (int (*)(const void *, const void *))
#endif

#ifndef DEFAULT_PRINTING
#ifdef HAVE_CUPS
#define DEFAULT_PRINTING PRINT_CUPS
#define PRINTCAP_NAME "cups"
#elif defined(SYSV)
#define DEFAULT_PRINTING PRINT_SYSV
#define PRINTCAP_NAME "lpstat"
#else
#define DEFAULT_PRINTING PRINT_BSD
#define PRINTCAP_NAME "/etc/printcap"
#endif
#endif

#ifndef PRINTCAP_NAME
#define PRINTCAP_NAME "/etc/printcap"
#endif

#ifndef SIGCLD
#define SIGCLD SIGCHLD
#endif

#ifndef SIGRTMIN
#define SIGRTMIN 32
#endif

#ifndef MAP_FILE
#define MAP_FILE 0
#endif

#if defined(HAVE_PUTPRPWNAM) && defined(AUTH_CLEARTEXT_SEG_CHARS)
#define OSF1_ENH_SEC 1
#endif

#ifndef ALLOW_CHANGE_PASSWORD
#if (defined(HAVE_TERMIOS_H) && defined(HAVE_DUP2) && defined(HAVE_SETSID))
#define ALLOW_CHANGE_PASSWORD 1
#endif
#endif

/* what is the longest significant password available on your system? 
 Knowing this speeds up password searches a lot */
#ifndef PASSWORD_LENGTH
#define PASSWORD_LENGTH 8
#endif

#ifdef REPLACE_INET_NTOA
#define inet_ntoa rep_inet_ntoa
#endif

#ifndef HAVE_PIPE
#define SYNC_DNS 1
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK 0x7f000001
#endif

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

#ifndef HAVE_CRYPT
#define crypt ufc_crypt
#endif

#ifndef O_ACCMODE
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)
#endif

#if defined(HAVE_CRYPT16) && defined(HAVE_GETAUTHUID)
#define ULTRIX_AUTH 1
#endif

#ifndef HAVE_STRDUP
char *strdup(const char *s);
#endif

#ifndef HAVE_STRNDUP
char *strndup(const char *s, size_t size);
#endif

#ifndef HAVE_MEMMOVE
void *memmove(void *dest,const void *src,int size);
#endif

#ifndef HAVE_INITGROUPS
int initgroups(char *name,gid_t id);
#endif

#ifndef HAVE_RENAME
int rename(const char *zfrom, const char *zto);
#endif

#ifndef HAVE_MKTIME
time_t mktime(struct tm *t);
#endif

#ifndef HAVE_STRLCPY
size_t strlcpy(char *d, const char *s, size_t bufsize);
#endif

#ifndef HAVE_STRLCAT
size_t strlcat(char *d, const char *s, size_t bufsize);
#endif

#ifndef HAVE_FTRUNCATE
int ftruncate(int f,long l);
#endif

#ifndef HAVE_STRNDUP
char *strndup(const char *s, size_t n);
#endif

#ifndef HAVE_STRNLEN
size_t strnlen(const char *s, size_t n);
#endif

#ifndef HAVE_STRTOUL
unsigned long strtoul(const char *nptr, char **endptr, int base);
#endif

#ifndef HAVE_SETENV
int setenv(const char *name, const char *value, int overwrite); 
#endif

#if (defined(USE_SETRESUID) && !defined(HAVE_SETRESUID_DECL))
/* stupid glibc */
int setresuid(uid_t ruid, uid_t euid, uid_t suid);
#endif
#if (defined(USE_SETRESUID) && !defined(HAVE_SETRESGID_DECL))
int setresgid(gid_t rgid, gid_t egid, gid_t sgid);
#endif
#ifndef HAVE_VASPRINTF_DECL
int vasprintf(char **ptr, const char *format, va_list ap);
#endif

#ifdef REPLACE_GETPASS
#define getpass(prompt) getsmbpass((prompt))
#endif

/*
 * Some older systems seem not to have MAXHOSTNAMELEN
 * defined.
 */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 254
#endif

/* yuck, I'd like a better way of doing this */
#define DIRP_SIZE (256 + 32)

/*
 * glibc on linux doesn't seem to have MSG_WAITALL
 * defined. I think the kernel has it though..
 */

#ifndef MSG_WAITALL
#define MSG_WAITALL 0
#endif

/* default socket options. Dave Miller thinks we should default to TCP_NODELAY
   given the socket IO pattern that Samba uses */
#ifdef TCP_NODELAY
#define DEFAULT_SOCKET_OPTIONS "TCP_NODELAY"
#else
#define DEFAULT_SOCKET_OPTIONS ""
#endif

/* Load header file for dynamic linking stuff */

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

/* dmalloc -- free heap debugger (dmalloc.org).  This should be near
 * the *bottom* of include files so as not to conflict. */
#ifdef ENABLE_DMALLOC
#  include <dmalloc.h>
#endif


/* Some POSIX definitions for those without */
 
#ifdef _XBOX

#define S_IFIFO			0010000
#define S_ISUID			0004000
#define S_ISGID			0002000
#define S_ISREG(mode)		((mode & S_IFMT) == S_IFREG)
#define S_ISLNK(mode) 0


#endif

#ifndef S_IFDIR
#define S_IFDIR         0x4000
#endif
#ifndef S_ISDIR
#define S_ISDIR(mode)   ((mode & 0xF000) == S_IFDIR)
#endif
#ifndef S_IRWXU
#define S_IRWXU 00700           /* read, write, execute: owner */
#endif
#ifndef S_IRUSR
#define S_IRUSR 00400           /* read permission: owner */
#endif
#ifndef S_IWUSR
#define S_IWUSR 00200           /* write permission: owner */
#endif
#ifndef S_IXUSR
#define S_IXUSR 00100           /* execute permission: owner */
#endif
#ifndef S_IRWXG
#define S_IRWXG 00070           /* read, write, execute: group */
#endif
#ifndef S_IRGRP
#define S_IRGRP 00040           /* read permission: group */
#endif
#ifndef S_IWGRP
#define S_IWGRP 00020           /* write permission: group */
#endif
#ifndef S_IXGRP
#define S_IXGRP 00010           /* execute permission: group */
#endif
#ifndef S_IRWXO
#define S_IRWXO 00007           /* read, write, execute: other */
#endif
#ifndef S_IROTH
#define S_IROTH 00004           /* read permission: other */
#endif
#ifndef S_IWOTH
#define S_IWOTH 00002           /* write permission: other */
#endif
#ifndef S_IXOTH
#define S_IXOTH 00001           /* execute permission: other */
#endif

/* For sys_adminlog(). */
#ifndef LOG_EMERG
#define LOG_EMERG       0       /* system is unusable */
#endif

#ifndef LOG_ALERT
#define LOG_ALERT       1       /* action must be taken immediately */
#endif

#ifndef LOG_CRIT
#define LOG_CRIT        2       /* critical conditions */
#endif

#ifndef LOG_ERR
#define LOG_ERR         3       /* error conditions */
#endif

#ifndef LOG_WARNING
#define LOG_WARNING     4       /* warning conditions */
#endif

#ifndef LOG_NOTICE
#define LOG_NOTICE      5       /* normal but significant condition */
#endif

#ifndef LOG_INFO
#define LOG_INFO        6       /* informational */
#endif

#ifndef LOG_DEBUG
#define LOG_DEBUG       7       /* debug-level messages */
#endif

#if HAVE_KERNEL_SHARE_MODES
#ifndef LOCK_MAND 
#define LOCK_MAND	32	/* This is a mandatory flock */
#define LOCK_READ	64	/* ... Which allows concurrent read operations */
#define LOCK_WRITE	128	/* ... Which allows concurrent write operations */
#define LOCK_RW		192	/* ... Which allows concurrent read & write ops */
#endif
#endif

extern int DEBUGLEVEL;

#define MAX_SEC_CTX_DEPTH 8    /* Maximum number of security contexts */


#ifdef GLIBC_HACK_FCNTL64
/* this is a gross hack. 64 bit locking is completely screwed up on
   i386 Linux in glibc 2.1.95 (which ships with RedHat 7.0). This hack
   "fixes" the problem with the current 2.4.0test kernels 
*/
#define fcntl fcntl64
#undef F_SETLKW 
#undef F_SETLK 
#define F_SETLK 13
#define F_SETLKW 14
#endif


/* Needed for sys_dlopen/sys_dlsym/sys_dlclose */
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif

#ifndef RTLD_LAZY
#define RTLD_LAZY 0
#endif

#ifndef RTLD_NOW
#define RTLD_NOW 0
#endif

/* needed for some systems without iconv. Doesn't really matter
   what error code we use */
#ifndef EILSEQ
#define EILSEQ EIO
#endif

/* add varargs prototypes with printf checking */
/*PRINTFLIKE2 */
int fdprintf(int , const char *, ...) PRINTF_ATTRIBUTE(2,3);
/*PRINTFLIKE1 */
int d_printf(const char *, ...) PRINTF_ATTRIBUTE(1,2);
/*PRINTFLIKE2 */
int d_fprintf(FILE *f, const char *, ...) PRINTF_ATTRIBUTE(2,3);
#ifndef HAVE_SNPRINTF_DECL
/*PRINTFLIKE3 */
int snprintf(char *,size_t ,const char *, ...) PRINTF_ATTRIBUTE(3,4);
#endif
#ifndef HAVE_ASPRINTF_DECL
/*PRINTFLIKE2 */
int asprintf(char **,const char *, ...) PRINTF_ATTRIBUTE(2,3);
#endif

/* Fix prototype problem with non-C99 compliant snprintf implementations, esp
   HPUX 11.  Don't change the sense of this #if statement.  Read the comments
   in lib/snprint.c if you think you need to.  See also bugzilla bug 174. */

#if !defined(HAVE_SNPRINTF) || !defined(HAVE_C99_VSNPRINTF)
#define snprintf smb_snprintf
#define vsnprintf smb_vsnprintf

/* PRINTFLIKE3 */
int smb_snprintf(char *str,size_t count,const char *fmt,...);
int smb_vsnprintf (char *str, size_t count, const char *fmt, va_list args);

#endif

/* PRINTFLIKE2 */
void sys_adminlog(int priority, const char *format_str, ...) PRINTF_ATTRIBUTE(2,3);

/* PRINTFLIKE2 */
int pstr_sprintf(pstring s, const char *fmt, ...) PRINTF_ATTRIBUTE(2,3);
/* PRINTFLIKE2 */
int fstr_sprintf(fstring s, const char *fmt, ...) PRINTF_ATTRIBUTE(2,3);

int d_vfprintf(FILE *f, const char *format, va_list ap) PRINTF_ATTRIBUTE(2,0);

int smb_xvasprintf(char **ptr, const char *format, va_list ap) PRINTF_ATTRIBUTE(2,0);

/* we used to use these fns, but now we have good replacements
   for snprintf and vsnprintf */
#define slprintf snprintf
#define vslprintf vsnprintf

/* we need to use __va_copy() on some platforms */
#ifdef HAVE_VA_COPY
#define VA_COPY(dest, src) va_copy(dest, src)
#else
#ifdef HAVE___VA_COPY
#define VA_COPY(dest, src) __va_copy(dest, src)
#else
#define VA_COPY(dest, src) (dest) = (src)
#endif
#endif

#ifndef HAVE_VSYSLOG
void vsyslog (int facility_priority, const char *format, va_list arglist);
#endif

#ifndef HAVE_TIMEGM
time_t timegm(struct tm *tm);
#endif

/*
 * Veritas File System.  Often in addition to native.
 * Quotas different.
 */
#if defined(HAVE_SYS_FS_VX_QUOTA_H)
#define VXFS_QUOTA
#endif

#if defined(HAVE_KRB5)

krb5_error_code smb_krb5_parse_name(krb5_context context,
				const char *name, /* in unix charset */
                                krb5_principal *principal);

krb5_error_code smb_krb5_unparse_name(krb5_context context,
				krb5_const_principal principal,
				char **unix_name);

#ifndef HAVE_KRB5_SET_REAL_TIME
krb5_error_code krb5_set_real_time(krb5_context context, int32_t seconds, int32_t microseconds);
#endif

#ifndef HAVE_KRB5_SET_DEFAULT_TGS_KTYPES
krb5_error_code krb5_set_default_tgs_ktypes(krb5_context ctx, const krb5_enctype *enc);
#endif

#if defined(HAVE_KRB5_AUTH_CON_SETKEY) && !defined(HAVE_KRB5_AUTH_CON_SETUSERUSERKEY)
krb5_error_code krb5_auth_con_setuseruserkey(krb5_context context, krb5_auth_context auth_context, krb5_keyblock *keyblock);
#endif

#ifndef HAVE_KRB5_FREE_UNPARSED_NAME
void krb5_free_unparsed_name(krb5_context ctx, char *val);
#endif

/* Samba wrapper function for krb5 functionality. */
void setup_kaddr( krb5_address *pkaddr, struct sockaddr *paddr);
int create_kerberos_key_from_string(krb5_context context, krb5_principal host_princ, krb5_data *password, krb5_keyblock *key, krb5_enctype enctype);
int create_kerberos_key_from_string_direct(krb5_context context, krb5_principal host_princ, krb5_data *password, krb5_keyblock *key, krb5_enctype enctype);
BOOL get_auth_data_from_tkt(TALLOC_CTX *mem_ctx, DATA_BLOB *auth_data, krb5_ticket *tkt);
krb5_const_principal get_principal_from_tkt(krb5_ticket *tkt);
krb5_error_code krb5_locate_kdc(krb5_context ctx, const krb5_data *realm, struct sockaddr **addr_pp, int *naddrs, int get_masters);
krb5_error_code get_kerberos_allowed_etypes(krb5_context context, krb5_enctype **enctypes);
void free_kerberos_etypes(krb5_context context, krb5_enctype *enctypes);
BOOL get_krb5_smb_session_key(krb5_context context, krb5_auth_context auth_context, DATA_BLOB *session_key, BOOL remote);
krb5_error_code smb_krb5_kt_free_entry(krb5_context context, krb5_keytab_entry *kt_entry);
krb5_principal kerberos_fetch_salt_princ_for_host_princ(krb5_context context, krb5_principal host_princ, int enctype);
void kerberos_set_creds_enctype(krb5_creds *pcreds, int enctype);
BOOL kerberos_compatible_enctypes(krb5_context context, krb5_enctype enctype1, krb5_enctype enctype2);
void kerberos_free_data_contents(krb5_context context, krb5_data *pdata);
NTSTATUS decode_pac_data(TALLOC_CTX *mem_ctx,
			 DATA_BLOB *pac_data_blob,
			 krb5_context context, 
			 krb5_keyblock *service_keyblock,
			 krb5_const_principal client_principal,
			 time_t tgs_authtime,
			 PAC_DATA **pac_data);
void smb_krb5_checksum_from_pac_sig(krb5_checksum *cksum, 
				    PAC_SIGNATURE_DATA *sig);
krb5_error_code smb_krb5_verify_checksum(krb5_context context,
					 krb5_keyblock *keyblock,
					 krb5_keyusage usage,
					 krb5_checksum *cksum,
					 uint8 *data,
					 size_t length);
time_t get_authtime_from_tkt(krb5_ticket *tkt);
void smb_krb5_free_ap_req(krb5_context context, 
			  krb5_ap_req *ap_req);
krb5_error_code smb_krb5_get_keyinfo_from_ap_req(krb5_context context, 
						 const krb5_data *inbuf, 
						 krb5_kvno *kvno, 
						 krb5_enctype *enctype);
krb5_error_code krb5_rd_req_return_keyblock_from_keytab(krb5_context context,
							krb5_auth_context *auth_context,
							const krb5_data *inbuf,
							krb5_const_principal server,
							krb5_keytab keytab,
							krb5_flags *ap_req_options,
							krb5_ticket **ticket, 
							krb5_keyblock **keyblock);
krb5_error_code smb_krb5_parse_name_norealm(krb5_context context, 
					    const char *name, 
					    krb5_principal *principal);
BOOL smb_krb5_principal_compare_any_realm(krb5_context context, 
					  krb5_const_principal princ1, 
					  krb5_const_principal princ2);
int cli_krb5_get_ticket(const char *principal, time_t time_offset, 
			DATA_BLOB *ticket, DATA_BLOB *session_key_krb5, uint32 extra_ap_opts, const char *ccname);
PAC_LOGON_INFO *get_logon_info_from_pac(PAC_DATA *pac_data);
krb5_error_code smb_krb5_renew_ticket(const char *ccache_string, const char *client_string, const char *service_string, time_t *new_start_time);
krb5_error_code kpasswd_err_to_krb5_err(krb5_error_code res_code);
krb5_error_code smb_krb5_gen_netbios_krb5_address(smb_krb5_addresses **kerb_addr);
krb5_error_code smb_krb5_free_addresses(krb5_context context, smb_krb5_addresses *addr);
NTSTATUS krb5_to_nt_status(krb5_error_code kerberos_error);
krb5_error_code nt_status_to_krb5(NTSTATUS nt_status);
void smb_krb5_free_error(krb5_context context, krb5_error *krberror);
krb5_error_code handle_krberror_packet(krb5_context context,
                                         krb5_data *packet);
#endif /* HAVE_KRB5 */


#ifdef HAVE_LDAP

/* function declarations not included in proto.h */
LDAP *ldap_open_with_timeout(const char *server, int port, unsigned int to);

#endif	/* HAVE_LDAP */


/* TRUE and FALSE are part of the C99 standard and gcc, but
   unfortunately many vendor compilers don't support them.  Use True
   and False instead. */

#ifdef TRUE
#undef TRUE
#endif
#define TRUE __ERROR__XX__DONT_USE_TRUE

#ifdef FALSE
#undef FALSE
#endif
#define FALSE __ERROR__XX__DONT_USE_FALSE

/* If we have blacklisted mmap() try to avoid using it accidentally by
   undefining the HAVE_MMAP symbol. */

#ifdef MMAP_BLACKLIST
#undef HAVE_MMAP
#endif

#define CONST_DISCARD(type, ptr)      ((type) ((void *) (ptr)))
#define CONST_ADD(type, ptr)          ((type) ((const void *) (ptr)))

#ifndef NORETURN_ATTRIBUTE
#if (__GNUC__ >= 3)
#define NORETURN_ATTRIBUTE __attribute__ ((noreturn))
#else
#define NORETURN_ATTRIBUTE
#endif
#endif

void smb_panic( const char *why ) NORETURN_ATTRIBUTE ;
void dump_core(void) NORETURN_ATTRIBUTE ;
void exit_server(const char *const reason) NORETURN_ATTRIBUTE ;
void exit_server_cleanly(const char *const reason) NORETURN_ATTRIBUTE ;
void exit_server_fault(void) NORETURN_ATTRIBUTE ;

#ifdef HAVE_LIBNSCD
#include "libnscd.h"
#endif

#endif /* _INCLUDES_H */
