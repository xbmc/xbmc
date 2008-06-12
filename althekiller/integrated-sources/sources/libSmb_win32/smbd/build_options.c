/* 
   Unix SMB/CIFS implementation.
   Build Options for Samba Suite
   Copyright (C) Vance Lankhaar <vlankhaar@linux.ca> 2003
   Copyright (C) Andrew Bartlett <abartlet@samba.org> 2001
   
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

#include "includes.h"
#include "build_env.h"
#include "dynconfig.h"

static void output(BOOL screen, const char *format, ...) PRINTF_ATTRIBUTE(2,3);


/****************************************************************************
helper function for build_options
****************************************************************************/
static void output(BOOL screen, const char *format, ...)
{
       char *ptr;
       va_list ap;
       
       va_start(ap, format);
       vasprintf(&ptr,format,ap);
       va_end(ap);

       if (screen) {
              d_printf("%s", ptr);
       } else {
	       DEBUG(4,("%s", ptr));
       }
       
       SAFE_FREE(ptr);
}

/****************************************************************************
options set at build time for the samba suite
****************************************************************************/
void build_options(BOOL screen)
{
       if ((DEBUGLEVEL < 4) && (!screen)) {
	       return;
       }

#ifdef _BUILD_ENV_H
       /* Output information about the build environment */
       output(screen,"Build environment:\n");
       output(screen,"   Built by:    %s@%s\n",BUILD_ENV_USER,BUILD_ENV_HOST);
       output(screen,"   Built on:    %s\n",BUILD_ENV_DATE);

       output(screen,"   Built using: %s\n",BUILD_ENV_COMPILER);
       output(screen,"   Build host:  %s\n",BUILD_ENV_UNAME);
       output(screen,"   SRCDIR:      %s\n",BUILD_ENV_SRCDIR);
       output(screen,"   BUILDDIR:    %s\n",BUILD_ENV_BUILDDIR);

     
#endif

       /* Output various paths to files and directories */
       output(screen,"\nPaths:\n");
       output(screen,"   SBINDIR: %s\n", dyn_SBINDIR);
       output(screen,"   BINDIR: %s\n", dyn_BINDIR);
       output(screen,"   SWATDIR: %s\n", dyn_SWATDIR);
       output(screen,"   CONFIGFILE: %s\n", dyn_CONFIGFILE);
       output(screen,"   LOGFILEBASE: %s\n", dyn_LOGFILEBASE);
       output(screen,"   LMHOSTSFILE: %s\n",dyn_LMHOSTSFILE);
       output(screen,"   LIBDIR: %s\n",dyn_LIBDIR);
       output(screen,"   SHLIBEXT: %s\n",dyn_SHLIBEXT);
       output(screen,"   LOCKDIR: %s\n",dyn_LOCKDIR);
       output(screen,"   PIDDIR: %s\n", dyn_PIDDIR);
       output(screen,"   SMB_PASSWD_FILE: %s\n",dyn_SMB_PASSWD_FILE);
       output(screen,"   PRIVATE_DIR: %s\n",dyn_PRIVATE_DIR);

/* Output various other options (as gleaned from include/config.h.in) */

	/* Show System Headers */
	output(screen, "\n System Headers:\n");

#ifdef HAVE_SYS_ACL_H
	output(screen, "   HAVE_SYS_ACL_H\n");
#endif
#ifdef HAVE_SYS_ATTRIBUTES_H
	output(screen, "   HAVE_SYS_ATTRIBUTES_H\n");
#endif
#ifdef HAVE_SYS_CAPABILITY_H
	output(screen, "   HAVE_SYS_CAPABILITY_H\n");
#endif
#ifdef HAVE_SYS_CDEFS_H
	output(screen, "   HAVE_SYS_CDEFS_H\n");
#endif
#ifdef HAVE_SYS_DIR_H
	output(screen, "   HAVE_SYS_DIR_H\n");
#endif
#ifdef HAVE_SYS_DMAPI_H
	output(screen, "   HAVE_SYS_DMAPI_H\n");
#endif
#ifdef HAVE_SYS_DMI_H
	output(screen, "   HAVE_SYS_DMI_H\n");
#endif
#ifdef HAVE_SYS_DUSTAT_H
	output(screen, "   HAVE_SYS_DUSTAT_H\n");
#endif
#ifdef HAVE_SYS_EA_H
	output(screen, "   HAVE_SYS_EA_H\n");
#endif
#ifdef HAVE_SYS_EXTATTR_H
	output(screen, "   HAVE_SYS_EXTATTR_H\n");
#endif
#ifdef HAVE_SYS_FCNTL_H
	output(screen, "   HAVE_SYS_FCNTL_H\n");
#endif
#ifdef HAVE_SYS_FILIO_H
	output(screen, "   HAVE_SYS_FILIO_H\n");
#endif
#ifdef HAVE_SYS_FILSYS_H
	output(screen, "   HAVE_SYS_FILSYS_H\n");
#endif
#ifdef HAVE_SYS_FS_S5PARAM_H
	output(screen, "   HAVE_SYS_FS_S5PARAM_H\n");
#endif
#ifdef HAVE_SYS_FS_VX_QUOTA_H
	output(screen, "   HAVE_SYS_FS_VX_QUOTA_H\n");
#endif
#ifdef HAVE_SYS_ID_H
	output(screen, "   HAVE_SYS_ID_H\n");
#endif
#ifdef HAVE_SYS_IOCTL_H
	output(screen, "   HAVE_SYS_IOCTL_H\n");
#endif
#ifdef HAVE_SYS_IPC_H
	output(screen, "   HAVE_SYS_IPC_H\n");
#endif
#ifdef HAVE_SYS_JFSDMAPI_H
	output(screen, "   HAVE_SYS_JFSDMAPI_H\n");
#endif
#ifdef HAVE_SYS_MMAN_H
	output(screen, "   HAVE_SYS_MMAN_H\n");
#endif
#ifdef HAVE_SYS_MODE_H
	output(screen, "   HAVE_SYS_MODE_H\n");
#endif
#ifdef HAVE_SYS_MOUNT_H
	output(screen, "   HAVE_SYS_MOUNT_H\n");
#endif
#ifdef HAVE_SYS_NDIR_H
	output(screen, "   HAVE_SYS_NDIR_H\n");
#endif
#ifdef HAVE_SYS_PARAM_H
	output(screen, "   HAVE_SYS_PARAM_H\n");
#endif
#ifdef HAVE_SYS_PRCTL_H
	output(screen, "   HAVE_SYS_PRCTL_H\n");
#endif
#ifdef HAVE_SYS_PRIV_H
	output(screen, "   HAVE_SYS_PRIV_H\n");
#endif
#ifdef HAVE_SYS_PROPLIST_H
	output(screen, "   HAVE_SYS_PROPLIST_H\n");
#endif
#ifdef HAVE_SYS_PTRACE_H
	output(screen, "   HAVE_SYS_PTRACE_H\n");
#endif
#ifdef HAVE_SYS_QUOTA_H
	output(screen, "   HAVE_SYS_QUOTA_H\n");
#endif
#ifdef HAVE_SYS_RESOURCE_H
	output(screen, "   HAVE_SYS_RESOURCE_H\n");
#endif
#ifdef HAVE_SYS_SECURITY_H
	output(screen, "   HAVE_SYS_SECURITY_H\n");
#endif
#ifdef HAVE_SYS_SELECT_H
	output(screen, "   HAVE_SYS_SELECT_H\n");
#endif
#ifdef HAVE_SYS_SHM_H
	output(screen, "   HAVE_SYS_SHM_H\n");
#endif
#ifdef HAVE_SYS_SOCKET_H
	output(screen, "   HAVE_SYS_SOCKET_H\n");
#endif
#ifdef HAVE_SYS_SOCKIO_H
	output(screen, "   HAVE_SYS_SOCKIO_H\n");
#endif
#ifdef HAVE_SYS_STATFS_H
	output(screen, "   HAVE_SYS_STATFS_H\n");
#endif
#ifdef HAVE_SYS_STATVFS_H
	output(screen, "   HAVE_SYS_STATVFS_H\n");
#endif
#ifdef HAVE_SYS_STAT_H
	output(screen, "   HAVE_SYS_STAT_H\n");
#endif
#ifdef HAVE_SYS_SYSCALL_H
	output(screen, "   HAVE_SYS_SYSCALL_H\n");
#endif
#ifdef HAVE_SYS_SYSLOG_H
	output(screen, "   HAVE_SYS_SYSLOG_H\n");
#endif
#ifdef HAVE_SYS_SYSMACROS_H
	output(screen, "   HAVE_SYS_SYSMACROS_H\n");
#endif
#ifdef HAVE_SYS_TERMIO_H
	output(screen, "   HAVE_SYS_TERMIO_H\n");
#endif
#ifdef HAVE_SYS_TIME_H
	output(screen, "   HAVE_SYS_TIME_H\n");
#endif
#ifdef HAVE_SYS_TYPES_H
	output(screen, "   HAVE_SYS_TYPES_H\n");
#endif
#ifdef HAVE_SYS_UIO_H
	output(screen, "   HAVE_SYS_UIO_H\n");
#endif
#ifdef HAVE_SYS_UNISTD_H
	output(screen, "   HAVE_SYS_UNISTD_H\n");
#endif
#ifdef HAVE_SYS_UN_H
	output(screen, "   HAVE_SYS_UN_H\n");
#endif
#ifdef HAVE_SYS_VFS_H
	output(screen, "   HAVE_SYS_VFS_H\n");
#endif
#ifdef HAVE_SYS_WAIT_H
	output(screen, "   HAVE_SYS_WAIT_H\n");
#endif
#ifdef HAVE_SYS_XATTR_H
	output(screen, "   HAVE_SYS_XATTR_H\n");
#endif

	/* Show Headers */
	output(screen, "\n Headers:\n");

#ifdef HAVE_AFS_AFS_H
	output(screen, "   HAVE_AFS_AFS_H\n");
#endif
#ifdef HAVE_AFS_H
	output(screen, "   HAVE_AFS_H\n");
#endif
#ifdef HAVE_AIO_H
	output(screen, "   HAVE_AIO_H\n");
#endif
#ifdef HAVE_ALLOCA_H
	output(screen, "   HAVE_ALLOCA_H\n");
#endif
#ifdef HAVE_ARPA_INET_H
	output(screen, "   HAVE_ARPA_INET_H\n");
#endif
#ifdef HAVE_ASM_TYPES_H
	output(screen, "   HAVE_ASM_TYPES_H\n");
#endif
#ifdef HAVE_ATTR_XATTR_H
	output(screen, "   HAVE_ATTR_XATTR_H\n");
#endif
#ifdef HAVE_CFSTRINGENCODINGCONVERTER_H
	output(screen, "   HAVE_CFSTRINGENCODINGCONVERTER_H\n");
#endif
#ifdef HAVE_COM_ERR_H
	output(screen, "   HAVE_COM_ERR_H\n");
#endif
#ifdef HAVE_COREFOUNDATION_CFSTRINGENCODINGCONVERTER_H
	output(screen, "   HAVE_COREFOUNDATION_CFSTRINGENCODINGCONVERTER_H\n");
#endif
#ifdef HAVE_CTYPE_H
	output(screen, "   HAVE_CTYPE_H\n");
#endif
#ifdef HAVE_DEVNM_H
	output(screen, "   HAVE_DEVNM_H\n");
#endif
#ifdef HAVE_DIRENT_H
	output(screen, "   HAVE_DIRENT_H\n");
#endif
#ifdef HAVE_DLFCN_H
	output(screen, "   HAVE_DLFCN_H\n");
#endif
#ifdef HAVE_EXECINFO_H
	output(screen, "   HAVE_EXECINFO_H\n");
#endif
#ifdef HAVE_FAM_H
	output(screen, "   HAVE_FAM_H\n");
#endif
#ifdef HAVE_FCNTL_H
	output(screen, "   HAVE_FCNTL_H\n");
#endif
#ifdef HAVE_FLOAT_H
	output(screen, "   HAVE_FLOAT_H\n");
#endif
#ifdef HAVE_GLOB_H
	output(screen, "   HAVE_GLOB_H\n");
#endif
#ifdef HAVE_GRP_H
	output(screen, "   HAVE_GRP_H\n");
#endif
#ifdef HAVE_GSSAPI_GSSAPI_GENERIC_H
	output(screen, "   HAVE_GSSAPI_GSSAPI_GENERIC_H\n");
#endif
#ifdef HAVE_GSSAPI_GSSAPI_H
	output(screen, "   HAVE_GSSAPI_GSSAPI_H\n");
#endif
#ifdef HAVE_GSSAPI_H
	output(screen, "   HAVE_GSSAPI_H\n");
#endif
#ifdef HAVE_HISTORY_H
	output(screen, "   HAVE_HISTORY_H\n");
#endif
#ifdef HAVE_INT16_FROM_RPC_RPC_H
	output(screen, "   HAVE_INT16_FROM_RPC_RPC_H\n");
#endif
#ifdef HAVE_INT32_FROM_RPC_RPC_H
	output(screen, "   HAVE_INT32_FROM_RPC_RPC_H\n");
#endif
#ifdef HAVE_INTTYPES_H
	output(screen, "   HAVE_INTTYPES_H\n");
#endif
#ifdef HAVE_KRB5_H
	output(screen, "   HAVE_KRB5_H\n");
#endif
#ifdef HAVE_LANGINFO_H
	output(screen, "   HAVE_LANGINFO_H\n");
#endif
#ifdef HAVE_LASTLOG_H
	output(screen, "   HAVE_LASTLOG_H\n");
#endif
#ifdef HAVE_LBER_H
	output(screen, "   HAVE_LBER_H\n");
#endif
#ifdef HAVE_LDAP_H
	output(screen, "   HAVE_LDAP_H\n");
#endif
#ifdef HAVE_LIBEXC_H
	output(screen, "   HAVE_LIBEXC_H\n");
#endif
#ifdef HAVE_LIBUNWIND_H
	output(screen, "   HAVE_LIBUNWIND_H\n");
#endif
#ifdef HAVE_LIBUNWIND_PTRACE_H
	output(screen, "   HAVE_LIBUNWIND_PTRACE_H\n");
#endif
#ifdef HAVE_LIMITS_H
	output(screen, "   HAVE_LIMITS_H\n");
#endif
#ifdef HAVE_LOCALE_H
	output(screen, "   HAVE_LOCALE_H\n");
#endif
#ifdef HAVE_MEMORY_H
	output(screen, "   HAVE_MEMORY_H\n");
#endif
#ifdef HAVE_MNTENT_H
	output(screen, "   HAVE_MNTENT_H\n");
#endif
#ifdef HAVE_NDIR_H
	output(screen, "   HAVE_NDIR_H\n");
#endif
#ifdef HAVE_NETINET_IN_IP_H
	output(screen, "   HAVE_NETINET_IN_IP_H\n");
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
	output(screen, "   HAVE_NETINET_IN_SYSTM_H\n");
#endif
#ifdef HAVE_NETINET_IP_H
	output(screen, "   HAVE_NETINET_IP_H\n");
#endif
#ifdef HAVE_NETINET_TCP_H
	output(screen, "   HAVE_NETINET_TCP_H\n");
#endif
#ifdef HAVE_NET_IF_H
	output(screen, "   HAVE_NET_IF_H\n");
#endif
#ifdef HAVE_NSSWITCH_H
	output(screen, "   HAVE_NSSWITCH_H\n");
#endif
#ifdef HAVE_NSS_COMMON_H
	output(screen, "   HAVE_NSS_COMMON_H\n");
#endif
#ifdef HAVE_NSS_H
	output(screen, "   HAVE_NSS_H\n");
#endif
#ifdef HAVE_NS_API_H
	output(screen, "   HAVE_NS_API_H\n");
#endif
#ifdef HAVE_POLL_H
	output(screen, "   HAVE_POLL_H\n");
#endif
#ifdef HAVE_READLINE_H
	output(screen, "   HAVE_READLINE_H\n");
#endif
#ifdef HAVE_READLINE_HISTORY_H
	output(screen, "   HAVE_READLINE_HISTORY_H\n");
#endif
#ifdef HAVE_READLINE_READLINE_H
	output(screen, "   HAVE_READLINE_READLINE_H\n");
#endif
#ifdef HAVE_RPCSVC_NIS_H
	output(screen, "   HAVE_RPCSVC_NIS_H\n");
#endif
#ifdef HAVE_RPCSVC_YPCLNT_H
	output(screen, "   HAVE_RPCSVC_YPCLNT_H\n");
#endif
#ifdef HAVE_RPCSVC_YP_PROT_H
	output(screen, "   HAVE_RPCSVC_YP_PROT_H\n");
#endif
#ifdef HAVE_RPC_NETTYPE_H
	output(screen, "   HAVE_RPC_NETTYPE_H\n");
#endif
#ifdef HAVE_RPC_RPC_H
	output(screen, "   HAVE_RPC_RPC_H\n");
#endif
#ifdef HAVE_SECURITY_PAM_APPL_H
	output(screen, "   HAVE_SECURITY_PAM_APPL_H\n");
#endif
#ifdef HAVE_SECURITY_PAM_MODULES_H
	output(screen, "   HAVE_SECURITY_PAM_MODULES_H\n");
#endif
#ifdef HAVE_SECURITY__PAM_MACROS_H
	output(screen, "   HAVE_SECURITY__PAM_MACROS_H\n");
#endif
#ifdef HAVE_SHADOW_H
	output(screen, "   HAVE_SHADOW_H\n");
#endif
#ifdef HAVE_STDARG_H
	output(screen, "   HAVE_STDARG_H\n");
#endif
#ifdef HAVE_STDINT_H
	output(screen, "   HAVE_STDINT_H\n");
#endif
#ifdef HAVE_STDLIB_H
	output(screen, "   HAVE_STDLIB_H\n");
#endif
#ifdef HAVE_STRINGS_H
	output(screen, "   HAVE_STRINGS_H\n");
#endif
#ifdef HAVE_STRING_H
	output(screen, "   HAVE_STRING_H\n");
#endif
#ifdef HAVE_STROPTS_H
	output(screen, "   HAVE_STROPTS_H\n");
#endif
#ifdef HAVE_SYSCALL_H
	output(screen, "   HAVE_SYSCALL_H\n");
#endif
#ifdef HAVE_SYSLOG_H
	output(screen, "   HAVE_SYSLOG_H\n");
#endif
#ifdef HAVE_TERMIOS_H
	output(screen, "   HAVE_TERMIOS_H\n");
#endif
#ifdef HAVE_TERMIO_H
	output(screen, "   HAVE_TERMIO_H\n");
#endif
#ifdef HAVE_UINT16_FROM_RPC_RPC_H
	output(screen, "   HAVE_UINT16_FROM_RPC_RPC_H\n");
#endif
#ifdef HAVE_UINT32_FROM_RPC_RPC_H
	output(screen, "   HAVE_UINT32_FROM_RPC_RPC_H\n");
#endif
#ifdef HAVE_UNISTD_H
	output(screen, "   HAVE_UNISTD_H\n");
#endif
#ifdef HAVE_UTIME_H
	output(screen, "   HAVE_UTIME_H\n");
#endif
#ifdef HAVE_VALGRIND_H
	output(screen, "   HAVE_VALGRIND_H\n");
#endif
#ifdef HAVE_VALGRIND_MEMCHECK_H
	output(screen, "   HAVE_VALGRIND_MEMCHECK_H\n");
#endif
#ifdef HAVE_VALGRIND_VALGRIND_H
	output(screen, "   HAVE_VALGRIND_VALGRIND_H\n");
#endif
#ifdef HAVE_XFS_DMAPI_H
	output(screen, "   HAVE_XFS_DMAPI_H\n");
#endif

	/* Show UTMP Options */
	output(screen, "\n UTMP Options:\n");

#ifdef HAVE_GETUTMPX
	output(screen, "   HAVE_GETUTMPX\n");
#endif
#ifdef HAVE_UTMPX_H
	output(screen, "   HAVE_UTMPX_H\n");
#endif
#ifdef HAVE_UTMP_H
	output(screen, "   HAVE_UTMP_H\n");
#endif
#ifdef HAVE_UT_UT_ADDR
	output(screen, "   HAVE_UT_UT_ADDR\n");
#endif
#ifdef HAVE_UT_UT_EXIT
	output(screen, "   HAVE_UT_UT_EXIT\n");
#endif
#ifdef HAVE_UT_UT_HOST
	output(screen, "   HAVE_UT_UT_HOST\n");
#endif
#ifdef HAVE_UT_UT_ID
	output(screen, "   HAVE_UT_UT_ID\n");
#endif
#ifdef HAVE_UT_UT_NAME
	output(screen, "   HAVE_UT_UT_NAME\n");
#endif
#ifdef HAVE_UT_UT_PID
	output(screen, "   HAVE_UT_UT_PID\n");
#endif
#ifdef HAVE_UT_UT_TIME
	output(screen, "   HAVE_UT_UT_TIME\n");
#endif
#ifdef HAVE_UT_UT_TV
	output(screen, "   HAVE_UT_UT_TV\n");
#endif
#ifdef HAVE_UT_UT_TYPE
	output(screen, "   HAVE_UT_UT_TYPE\n");
#endif
#ifdef HAVE_UT_UT_USER
	output(screen, "   HAVE_UT_UT_USER\n");
#endif
#ifdef PUTUTLINE_RETURNS_UTMP
	output(screen, "   PUTUTLINE_RETURNS_UTMP\n");
#endif
#ifdef WITH_UTMP
	output(screen, "   WITH_UTMP\n");
#endif

	/* Show HAVE_* Defines */
	output(screen, "\n HAVE_* Defines:\n");

#ifdef HAVE_64BIT_LINUX
	output(screen, "   HAVE_64BIT_LINUX\n");
#endif
#ifdef HAVE_ACL_GET_PERM_NP
	output(screen, "   HAVE_ACL_GET_PERM_NP\n");
#endif
#ifdef HAVE_ADDRTYPE_IN_KRB5_ADDRESS
	output(screen, "   HAVE_ADDRTYPE_IN_KRB5_ADDRESS\n");
#endif
#ifdef HAVE_ADDR_TYPE_IN_KRB5_ADDRESS
	output(screen, "   HAVE_ADDR_TYPE_IN_KRB5_ADDRESS\n");
#endif
#ifdef HAVE_ADD_PROPLIST_ENTRY
	output(screen, "   HAVE_ADD_PROPLIST_ENTRY\n");
#endif
#ifdef HAVE_AIOCB64
	output(screen, "   HAVE_AIOCB64\n");
#endif
#ifdef HAVE_AIO_CANCEL
	output(screen, "   HAVE_AIO_CANCEL\n");
#endif
#ifdef HAVE_AIO_CANCEL64
	output(screen, "   HAVE_AIO_CANCEL64\n");
#endif
#ifdef HAVE_AIO_ERROR
	output(screen, "   HAVE_AIO_ERROR\n");
#endif
#ifdef HAVE_AIO_ERROR64
	output(screen, "   HAVE_AIO_ERROR64\n");
#endif
#ifdef HAVE_AIO_FSYNC
	output(screen, "   HAVE_AIO_FSYNC\n");
#endif
#ifdef HAVE_AIO_FSYNC64
	output(screen, "   HAVE_AIO_FSYNC64\n");
#endif
#ifdef HAVE_AIO_READ
	output(screen, "   HAVE_AIO_READ\n");
#endif
#ifdef HAVE_AIO_READ64
	output(screen, "   HAVE_AIO_READ64\n");
#endif
#ifdef HAVE_AIO_RETURN
	output(screen, "   HAVE_AIO_RETURN\n");
#endif
#ifdef HAVE_AIO_RETURN64
	output(screen, "   HAVE_AIO_RETURN64\n");
#endif
#ifdef HAVE_AIO_SUSPEND
	output(screen, "   HAVE_AIO_SUSPEND\n");
#endif
#ifdef HAVE_AIO_SUSPEND64
	output(screen, "   HAVE_AIO_SUSPEND64\n");
#endif
#ifdef HAVE_AIO_WRITE
	output(screen, "   HAVE_AIO_WRITE\n");
#endif
#ifdef HAVE_AIO_WRITE64
	output(screen, "   HAVE_AIO_WRITE64\n");
#endif
#ifdef HAVE_AIX_ACLS
	output(screen, "   HAVE_AIX_ACLS\n");
#endif
#ifdef HAVE_AP_OPTS_USE_SUBKEY
	output(screen, "   HAVE_AP_OPTS_USE_SUBKEY\n");
#endif
#ifdef HAVE_ASPRINTF
	output(screen, "   HAVE_ASPRINTF\n");
#endif
#ifdef HAVE_ASPRINTF_DECL
	output(screen, "   HAVE_ASPRINTF_DECL\n");
#endif
#ifdef HAVE_ATEXIT
	output(screen, "   HAVE_ATEXIT\n");
#endif
#ifdef HAVE_ATTR_GET
	output(screen, "   HAVE_ATTR_GET\n");
#endif
#ifdef HAVE_ATTR_GETF
	output(screen, "   HAVE_ATTR_GETF\n");
#endif
#ifdef HAVE_ATTR_LIST
	output(screen, "   HAVE_ATTR_LIST\n");
#endif
#ifdef HAVE_ATTR_LISTF
	output(screen, "   HAVE_ATTR_LISTF\n");
#endif
#ifdef HAVE_ATTR_REMOVE
	output(screen, "   HAVE_ATTR_REMOVE\n");
#endif
#ifdef HAVE_ATTR_REMOVEF
	output(screen, "   HAVE_ATTR_REMOVEF\n");
#endif
#ifdef HAVE_ATTR_SET
	output(screen, "   HAVE_ATTR_SET\n");
#endif
#ifdef HAVE_ATTR_SETF
	output(screen, "   HAVE_ATTR_SETF\n");
#endif
#ifdef HAVE_BACKTRACE_SYMBOLS
	output(screen, "   HAVE_BACKTRACE_SYMBOLS\n");
#endif
#ifdef HAVE_BER_SCANF
	output(screen, "   HAVE_BER_SCANF\n");
#endif
#ifdef HAVE_BICONV
	output(screen, "   HAVE_BICONV\n");
#endif
#ifdef HAVE_BIGCRYPT
	output(screen, "   HAVE_BIGCRYPT\n");
#endif
#ifdef HAVE_BROKEN_FCNTL64_LOCKS
	output(screen, "   HAVE_BROKEN_FCNTL64_LOCKS\n");
#endif
#ifdef HAVE_BROKEN_GETGROUPS
	output(screen, "   HAVE_BROKEN_GETGROUPS\n");
#endif
#ifdef HAVE_BROKEN_READDIR_NAME
	output(screen, "   HAVE_BROKEN_READDIR_NAME\n");
#endif
#ifdef HAVE_C99_VSNPRINTF
	output(screen, "   HAVE_C99_VSNPRINTF\n");
#endif
#ifdef HAVE_CAP_GET_PROC
	output(screen, "   HAVE_CAP_GET_PROC\n");
#endif
#ifdef HAVE_CHECKSUM_IN_KRB5_CHECKSUM
	output(screen, "   HAVE_CHECKSUM_IN_KRB5_CHECKSUM\n");
#endif
#ifdef HAVE_CHMOD
	output(screen, "   HAVE_CHMOD\n");
#endif
#ifdef HAVE_CHOWN
	output(screen, "   HAVE_CHOWN\n");
#endif
#ifdef HAVE_CHROOT
	output(screen, "   HAVE_CHROOT\n");
#endif
#ifdef HAVE_CHSIZE
	output(screen, "   HAVE_CHSIZE\n");
#endif
#ifdef HAVE_CLOCK_GETTIME
	output(screen, "   HAVE_CLOCK_GETTIME\n");
#endif
#ifdef HAVE_CLOCK_MONOTONIC
	output(screen, "   HAVE_CLOCK_MONOTONIC\n");
#endif
#ifdef HAVE_CLOCK_PROCESS_CPUTIME_ID
	output(screen, "   HAVE_CLOCK_PROCESS_CPUTIME_ID\n");
#endif
#ifdef HAVE_CLOCK_REALTIME
	output(screen, "   HAVE_CLOCK_REALTIME\n");
#endif
#ifdef HAVE_CLOSEDIR64
	output(screen, "   HAVE_CLOSEDIR64\n");
#endif
#ifdef HAVE_COMPILER_WILL_OPTIMIZE_OUT_FNS
	output(screen, "   HAVE_COMPILER_WILL_OPTIMIZE_OUT_FNS\n");
#endif
#ifdef HAVE_CONNECT
	output(screen, "   HAVE_CONNECT\n");
#endif
#ifdef HAVE_COPY_AUTHENTICATOR
	output(screen, "   HAVE_COPY_AUTHENTICATOR\n");
#endif
#ifdef HAVE_CREAT64
	output(screen, "   HAVE_CREAT64\n");
#endif
#ifdef HAVE_CRYPT
	output(screen, "   HAVE_CRYPT\n");
#endif
#ifdef HAVE_CRYPT16
	output(screen, "   HAVE_CRYPT16\n");
#endif
#ifdef HAVE_CUPS
	output(screen, "   HAVE_CUPS\n");
#endif
#ifdef HAVE_DECODE_KRB5_AP_REQ
	output(screen, "   HAVE_DECODE_KRB5_AP_REQ\n");
#endif
#ifdef HAVE_DELPROPLIST
	output(screen, "   HAVE_DELPROPLIST\n");
#endif
#ifdef HAVE_DES_SET_KEY
	output(screen, "   HAVE_DES_SET_KEY\n");
#endif
#ifdef HAVE_DEV64_T
	output(screen, "   HAVE_DEV64_T\n");
#endif
#ifdef HAVE_DEVICE_MAJOR_FN
	output(screen, "   HAVE_DEVICE_MAJOR_FN\n");
#endif
#ifdef HAVE_DEVICE_MINOR_FN
	output(screen, "   HAVE_DEVICE_MINOR_FN\n");
#endif
#ifdef HAVE_DEVNM
	output(screen, "   HAVE_DEVNM\n");
#endif
#ifdef HAVE_DIRENT_D_OFF
	output(screen, "   HAVE_DIRENT_D_OFF\n");
#endif
#ifdef HAVE_DLCLOSE
	output(screen, "   HAVE_DLCLOSE\n");
#endif
#ifdef HAVE_DLERROR
	output(screen, "   HAVE_DLERROR\n");
#endif
#ifdef HAVE_DLOPEN
	output(screen, "   HAVE_DLOPEN\n");
#endif
#ifdef HAVE_DLSYM
	output(screen, "   HAVE_DLSYM\n");
#endif
#ifdef HAVE_DLSYM_PREPEND_UNDERSCORE
	output(screen, "   HAVE_DLSYM_PREPEND_UNDERSCORE\n");
#endif
#ifdef HAVE_DQB_FSOFTLIMIT
	output(screen, "   HAVE_DQB_FSOFTLIMIT\n");
#endif
#ifdef HAVE_DUP2
	output(screen, "   HAVE_DUP2\n");
#endif
#ifdef HAVE_ENCTYPE_ARCFOUR_HMAC_MD5
	output(screen, "   HAVE_ENCTYPE_ARCFOUR_HMAC_MD5\n");
#endif
#ifdef HAVE_ENDMNTENT
	output(screen, "   HAVE_ENDMNTENT\n");
#endif
#ifdef HAVE_ENDNETGRENT
	output(screen, "   HAVE_ENDNETGRENT\n");
#endif
#ifdef HAVE_ERRNO_DECL
	output(screen, "   HAVE_ERRNO_DECL\n");
#endif
#ifdef HAVE_ETYPE_IN_ENCRYPTEDDATA
	output(screen, "   HAVE_ETYPE_IN_ENCRYPTEDDATA\n");
#endif
#ifdef HAVE_EXECL
	output(screen, "   HAVE_EXECL\n");
#endif
#ifdef HAVE_EXPLICIT_LARGEFILE_SUPPORT
	output(screen, "   HAVE_EXPLICIT_LARGEFILE_SUPPORT\n");
#endif
#ifdef HAVE_EXTATTR_DELETE_FD
	output(screen, "   HAVE_EXTATTR_DELETE_FD\n");
#endif
#ifdef HAVE_EXTATTR_DELETE_FILE
	output(screen, "   HAVE_EXTATTR_DELETE_FILE\n");
#endif
#ifdef HAVE_EXTATTR_DELETE_LINK
	output(screen, "   HAVE_EXTATTR_DELETE_LINK\n");
#endif
#ifdef HAVE_EXTATTR_GET_FD
	output(screen, "   HAVE_EXTATTR_GET_FD\n");
#endif
#ifdef HAVE_EXTATTR_GET_FILE
	output(screen, "   HAVE_EXTATTR_GET_FILE\n");
#endif
#ifdef HAVE_EXTATTR_GET_LINK
	output(screen, "   HAVE_EXTATTR_GET_LINK\n");
#endif
#ifdef HAVE_EXTATTR_LIST_FD
	output(screen, "   HAVE_EXTATTR_LIST_FD\n");
#endif
#ifdef HAVE_EXTATTR_LIST_FILE
	output(screen, "   HAVE_EXTATTR_LIST_FILE\n");
#endif
#ifdef HAVE_EXTATTR_LIST_LINK
	output(screen, "   HAVE_EXTATTR_LIST_LINK\n");
#endif
#ifdef HAVE_EXTATTR_SET_FD
	output(screen, "   HAVE_EXTATTR_SET_FD\n");
#endif
#ifdef HAVE_EXTATTR_SET_FILE
	output(screen, "   HAVE_EXTATTR_SET_FILE\n");
#endif
#ifdef HAVE_EXTATTR_SET_LINK
	output(screen, "   HAVE_EXTATTR_SET_LINK\n");
#endif
#ifdef HAVE_E_DATA_POINTER_IN_KRB5_ERROR
	output(screen, "   HAVE_E_DATA_POINTER_IN_KRB5_ERROR\n");
#endif
#ifdef HAVE_FAMOPEN2
	output(screen, "   HAVE_FAMOPEN2\n");
#endif
#ifdef HAVE_FAM_CHANGE_NOTIFY
	output(screen, "   HAVE_FAM_CHANGE_NOTIFY\n");
#endif
#ifdef HAVE_FAM_H_FAMCODES_TYPEDEF
	output(screen, "   HAVE_FAM_H_FAMCODES_TYPEDEF\n");
#endif
#ifdef HAVE_FCHMOD
	output(screen, "   HAVE_FCHMOD\n");
#endif
#ifdef HAVE_FCHOWN
	output(screen, "   HAVE_FCHOWN\n");
#endif
#ifdef HAVE_FCNTL_LOCK
	output(screen, "   HAVE_FCNTL_LOCK\n");
#endif
#ifdef HAVE_FCVT
	output(screen, "   HAVE_FCVT\n");
#endif
#ifdef HAVE_FCVTL
	output(screen, "   HAVE_FCVTL\n");
#endif
#ifdef HAVE_FDELPROPLIST
	output(screen, "   HAVE_FDELPROPLIST\n");
#endif
#ifdef HAVE_FGETEA
	output(screen, "   HAVE_FGETEA\n");
#endif
#ifdef HAVE_FGETPROPLIST
	output(screen, "   HAVE_FGETPROPLIST\n");
#endif
#ifdef HAVE_FGETXATTR
	output(screen, "   HAVE_FGETXATTR\n");
#endif
#ifdef HAVE_FLISTEA
	output(screen, "   HAVE_FLISTEA\n");
#endif
#ifdef HAVE_FLISTXATTR
	output(screen, "   HAVE_FLISTXATTR\n");
#endif
#ifdef HAVE_FOPEN64
	output(screen, "   HAVE_FOPEN64\n");
#endif
#ifdef HAVE_FREE_AP_REQ
	output(screen, "   HAVE_FREE_AP_REQ\n");
#endif
#ifdef HAVE_FREMOVEEA
	output(screen, "   HAVE_FREMOVEEA\n");
#endif
#ifdef HAVE_FREMOVEXATTR
	output(screen, "   HAVE_FREMOVEXATTR\n");
#endif
#ifdef HAVE_FSEEK64
	output(screen, "   HAVE_FSEEK64\n");
#endif
#ifdef HAVE_FSEEKO64
	output(screen, "   HAVE_FSEEKO64\n");
#endif
#ifdef HAVE_FSETEA
	output(screen, "   HAVE_FSETEA\n");
#endif
#ifdef HAVE_FSETPROPLIST
	output(screen, "   HAVE_FSETPROPLIST\n");
#endif
#ifdef HAVE_FSETXATTR
	output(screen, "   HAVE_FSETXATTR\n");
#endif
#ifdef HAVE_FSTAT
	output(screen, "   HAVE_FSTAT\n");
#endif
#ifdef HAVE_FSTAT64
	output(screen, "   HAVE_FSTAT64\n");
#endif
#ifdef HAVE_FSYNC
	output(screen, "   HAVE_FSYNC\n");
#endif
#ifdef HAVE_FTELL64
	output(screen, "   HAVE_FTELL64\n");
#endif
#ifdef HAVE_FTELLO64
	output(screen, "   HAVE_FTELLO64\n");
#endif
#ifdef HAVE_FTRUNCATE
	output(screen, "   HAVE_FTRUNCATE\n");
#endif
#ifdef HAVE_FTRUNCATE64
	output(screen, "   HAVE_FTRUNCATE64\n");
#endif
#ifdef HAVE_FTRUNCATE_EXTEND
	output(screen, "   HAVE_FTRUNCATE_EXTEND\n");
#endif
#ifdef HAVE_FUNCTION_MACRO
	output(screen, "   HAVE_FUNCTION_MACRO\n");
#endif
#ifdef HAVE_GETAUTHUID
	output(screen, "   HAVE_GETAUTHUID\n");
#endif
#ifdef HAVE_GETCWD
	output(screen, "   HAVE_GETCWD\n");
#endif
#ifdef HAVE_GETDENTS
	output(screen, "   HAVE_GETDENTS\n");
#endif
#ifdef HAVE_GETDENTS64
	output(screen, "   HAVE_GETDENTS64\n");
#endif
#ifdef HAVE_GETDIRENTRIES
	output(screen, "   HAVE_GETDIRENTRIES\n");
#endif
#ifdef HAVE_GETEA
	output(screen, "   HAVE_GETEA\n");
#endif
#ifdef HAVE_GETGRENT
	output(screen, "   HAVE_GETGRENT\n");
#endif
#ifdef HAVE_GETGRNAM
	output(screen, "   HAVE_GETGRNAM\n");
#endif
#ifdef HAVE_GETGROUPLIST
	output(screen, "   HAVE_GETGROUPLIST\n");
#endif
#ifdef HAVE_GETMNTENT
	output(screen, "   HAVE_GETMNTENT\n");
#endif
#ifdef HAVE_GETNETGRENT
	output(screen, "   HAVE_GETNETGRENT\n");
#endif
#ifdef HAVE_GETPROPLIST
	output(screen, "   HAVE_GETPROPLIST\n");
#endif
#ifdef HAVE_GETPRPWNAM
	output(screen, "   HAVE_GETPRPWNAM\n");
#endif
#ifdef HAVE_GETPWANAM
	output(screen, "   HAVE_GETPWANAM\n");
#endif
#ifdef HAVE_GETRLIMIT
	output(screen, "   HAVE_GETRLIMIT\n");
#endif
#ifdef HAVE_GETSPNAM
	output(screen, "   HAVE_GETSPNAM\n");
#endif
#ifdef HAVE_GETTIMEOFDAY_TZ
	output(screen, "   HAVE_GETTIMEOFDAY_TZ\n");
#endif
#ifdef HAVE_GETXATTR
	output(screen, "   HAVE_GETXATTR\n");
#endif
#ifdef HAVE_GET_PROPLIST_ENTRY
	output(screen, "   HAVE_GET_PROPLIST_ENTRY\n");
#endif
#ifdef HAVE_GICONV
	output(screen, "   HAVE_GICONV\n");
#endif
#ifdef HAVE_GLOB
	output(screen, "   HAVE_GLOB\n");
#endif
#ifdef HAVE_GRANTPT
	output(screen, "   HAVE_GRANTPT\n");
#endif
#ifdef HAVE_GSSAPI
	output(screen, "   HAVE_GSSAPI\n");
#endif
#ifdef HAVE_GSS_DISPLAY_STATUS
	output(screen, "   HAVE_GSS_DISPLAY_STATUS\n");
#endif
#ifdef HAVE_HPUX_ACLS
	output(screen, "   HAVE_HPUX_ACLS\n");
#endif
#ifdef HAVE_ICONV
	output(screen, "   HAVE_ICONV\n");
#endif
#ifdef HAVE_IFACE_AIX
	output(screen, "   HAVE_IFACE_AIX\n");
#endif
#ifdef HAVE_IFACE_IFCONF
	output(screen, "   HAVE_IFACE_IFCONF\n");
#endif
#ifdef HAVE_IFACE_IFREQ
	output(screen, "   HAVE_IFACE_IFREQ\n");
#endif
#ifdef HAVE_IMMEDIATE_STRUCTURES
	output(screen, "   HAVE_IMMEDIATE_STRUCTURES\n");
#endif
#ifdef HAVE_INITGROUPS
	output(screen, "   HAVE_INITGROUPS\n");
#endif
#ifdef HAVE_INNETGR
	output(screen, "   HAVE_INNETGR\n");
#endif
#ifdef HAVE_INO64_T
	output(screen, "   HAVE_INO64_T\n");
#endif
#ifdef HAVE_IPRINT
	output(screen, "   HAVE_IPRINT\n");
#endif
#ifdef HAVE_IRIX_ACLS
	output(screen, "   HAVE_IRIX_ACLS\n");
#endif
#ifdef HAVE_KERNEL_CHANGE_NOTIFY
	output(screen, "   HAVE_KERNEL_CHANGE_NOTIFY\n");
#endif
#ifdef HAVE_KERNEL_OPLOCKS_IRIX
	output(screen, "   HAVE_KERNEL_OPLOCKS_IRIX\n");
#endif
#ifdef HAVE_KERNEL_OPLOCKS_LINUX
	output(screen, "   HAVE_KERNEL_OPLOCKS_LINUX\n");
#endif
#ifdef HAVE_KERNEL_SHARE_MODES
	output(screen, "   HAVE_KERNEL_SHARE_MODES\n");
#endif
#ifdef HAVE_KRB5
	output(screen, "   HAVE_KRB5\n");
#endif
#ifdef HAVE_KRB5_ADDRESSES
	output(screen, "   HAVE_KRB5_ADDRESSES\n");
#endif
#ifdef HAVE_KRB5_AUTH_CON_SETKEY
	output(screen, "   HAVE_KRB5_AUTH_CON_SETKEY\n");
#endif
#ifdef HAVE_KRB5_AUTH_CON_SETUSERUSERKEY
	output(screen, "   HAVE_KRB5_AUTH_CON_SETUSERUSERKEY\n");
#endif
#ifdef HAVE_KRB5_CRYPTO
	output(screen, "   HAVE_KRB5_CRYPTO\n");
#endif
#ifdef HAVE_KRB5_CRYPTO_DESTROY
	output(screen, "   HAVE_KRB5_CRYPTO_DESTROY\n");
#endif
#ifdef HAVE_KRB5_CRYPTO_INIT
	output(screen, "   HAVE_KRB5_CRYPTO_INIT\n");
#endif
#ifdef HAVE_KRB5_C_ENCTYPE_COMPARE
	output(screen, "   HAVE_KRB5_C_ENCTYPE_COMPARE\n");
#endif
#ifdef HAVE_KRB5_C_VERIFY_CHECKSUM
	output(screen, "   HAVE_KRB5_C_VERIFY_CHECKSUM\n");
#endif
#ifdef HAVE_KRB5_DECODE_AP_REQ
	output(screen, "   HAVE_KRB5_DECODE_AP_REQ\n");
#endif
#ifdef HAVE_KRB5_ENCRYPT_BLOCK
	output(screen, "   HAVE_KRB5_ENCRYPT_BLOCK\n");
#endif
#ifdef HAVE_KRB5_ENCRYPT_DATA
	output(screen, "   HAVE_KRB5_ENCRYPT_DATA\n");
#endif
#ifdef HAVE_KRB5_ENCTYPES_COMPATIBLE_KEYS
	output(screen, "   HAVE_KRB5_ENCTYPES_COMPATIBLE_KEYS\n");
#endif
#ifdef HAVE_KRB5_FREE_AP_REQ
	output(screen, "   HAVE_KRB5_FREE_AP_REQ\n");
#endif
#ifdef HAVE_KRB5_FREE_DATA_CONTENTS
	output(screen, "   HAVE_KRB5_FREE_DATA_CONTENTS\n");
#endif
#ifdef HAVE_KRB5_FREE_ERROR_CONTENTS
	output(screen, "   HAVE_KRB5_FREE_ERROR_CONTENTS\n");
#endif
#ifdef HAVE_KRB5_FREE_KEYTAB_ENTRY_CONTENTS
	output(screen, "   HAVE_KRB5_FREE_KEYTAB_ENTRY_CONTENTS\n");
#endif
#ifdef HAVE_KRB5_FREE_KTYPES
	output(screen, "   HAVE_KRB5_FREE_KTYPES\n");
#endif
#ifdef HAVE_KRB5_FREE_UNPARSED_NAME
	output(screen, "   HAVE_KRB5_FREE_UNPARSED_NAME\n");
#endif
#ifdef HAVE_KRB5_GET_DEFAULT_IN_TKT_ETYPES
	output(screen, "   HAVE_KRB5_GET_DEFAULT_IN_TKT_ETYPES\n");
#endif
#ifdef HAVE_KRB5_GET_INIT_CREDS_OPT_SET_PAC_REQUEST
	output(screen, "   HAVE_KRB5_GET_INIT_CREDS_OPT_SET_PAC_REQUEST\n");
#endif
#ifdef HAVE_KRB5_GET_KDC_CRED
	output(screen, "   HAVE_KRB5_GET_KDC_CRED\n");
#endif
#ifdef HAVE_KRB5_GET_PERMITTED_ENCTYPES
	output(screen, "   HAVE_KRB5_GET_PERMITTED_ENCTYPES\n");
#endif
#ifdef HAVE_KRB5_GET_PW_SALT
	output(screen, "   HAVE_KRB5_GET_PW_SALT\n");
#endif
#ifdef HAVE_KRB5_GET_RENEWED_CREDS
	output(screen, "   HAVE_KRB5_GET_RENEWED_CREDS\n");
#endif
#ifdef HAVE_KRB5_KEYBLOCK_IN_CREDS
	output(screen, "   HAVE_KRB5_KEYBLOCK_IN_CREDS\n");
#endif
#ifdef HAVE_KRB5_KEYBLOCK_KEYVALUE
	output(screen, "   HAVE_KRB5_KEYBLOCK_KEYVALUE\n");
#endif
#ifdef HAVE_KRB5_KEYTAB_ENTRY_KEY
	output(screen, "   HAVE_KRB5_KEYTAB_ENTRY_KEY\n");
#endif
#ifdef HAVE_KRB5_KEYTAB_ENTRY_KEYBLOCK
	output(screen, "   HAVE_KRB5_KEYTAB_ENTRY_KEYBLOCK\n");
#endif
#ifdef HAVE_KRB5_KEYUSAGE_APP_DATA_CKSUM
	output(screen, "   HAVE_KRB5_KEYUSAGE_APP_DATA_CKSUM\n");
#endif
#ifdef HAVE_KRB5_KRBHST_GET_ADDRINFO
	output(screen, "   HAVE_KRB5_KRBHST_GET_ADDRINFO\n");
#endif
#ifdef HAVE_KRB5_KT_COMPARE
	output(screen, "   HAVE_KRB5_KT_COMPARE\n");
#endif
#ifdef HAVE_KRB5_KT_FREE_ENTRY
	output(screen, "   HAVE_KRB5_KT_FREE_ENTRY\n");
#endif
#ifdef HAVE_KRB5_KU_OTHER_CKSUM
	output(screen, "   HAVE_KRB5_KU_OTHER_CKSUM\n");
#endif
#ifdef HAVE_KRB5_LOCATE_KDC
	output(screen, "   HAVE_KRB5_LOCATE_KDC\n");
#endif
#ifdef HAVE_KRB5_MK_REQ_EXTENDED
	output(screen, "   HAVE_KRB5_MK_REQ_EXTENDED\n");
#endif
#ifdef HAVE_KRB5_PARSE_NAME_NOREALM
	output(screen, "   HAVE_KRB5_PARSE_NAME_NOREALM\n");
#endif
#ifdef HAVE_KRB5_PRINCIPAL2SALT
	output(screen, "   HAVE_KRB5_PRINCIPAL2SALT\n");
#endif
#ifdef HAVE_KRB5_PRINCIPAL_COMPARE_ANY_REALM
	output(screen, "   HAVE_KRB5_PRINCIPAL_COMPARE_ANY_REALM\n");
#endif
#ifdef HAVE_KRB5_PRINCIPAL_GET_COMP_STRING
	output(screen, "   HAVE_KRB5_PRINCIPAL_GET_COMP_STRING\n");
#endif
#ifdef HAVE_KRB5_PRINC_COMPONENT
	output(screen, "   HAVE_KRB5_PRINC_COMPONENT\n");
#endif
#ifdef HAVE_KRB5_PRINC_SIZE
	output(screen, "   HAVE_KRB5_PRINC_SIZE\n");
#endif
#ifdef HAVE_KRB5_SESSION_IN_CREDS
	output(screen, "   HAVE_KRB5_SESSION_IN_CREDS\n");
#endif
#ifdef HAVE_KRB5_SET_DEFAULT_IN_TKT_ETYPES
	output(screen, "   HAVE_KRB5_SET_DEFAULT_IN_TKT_ETYPES\n");
#endif
#ifdef HAVE_KRB5_SET_DEFAULT_TGS_KTYPES
	output(screen, "   HAVE_KRB5_SET_DEFAULT_TGS_KTYPES\n");
#endif
#ifdef HAVE_KRB5_SET_REAL_TIME
	output(screen, "   HAVE_KRB5_SET_REAL_TIME\n");
#endif
#ifdef HAVE_KRB5_STRING_TO_KEY
	output(screen, "   HAVE_KRB5_STRING_TO_KEY\n");
#endif
#ifdef HAVE_KRB5_STRING_TO_KEY_SALT
	output(screen, "   HAVE_KRB5_STRING_TO_KEY_SALT\n");
#endif
#ifdef HAVE_KRB5_TKT_ENC_PART2
	output(screen, "   HAVE_KRB5_TKT_ENC_PART2\n");
#endif
#ifdef HAVE_KRB5_USE_ENCTYPE
	output(screen, "   HAVE_KRB5_USE_ENCTYPE\n");
#endif
#ifdef HAVE_KV5M_KEYTAB
	output(screen, "   HAVE_KV5M_KEYTAB\n");
#endif
#ifdef HAVE_LDAP
	output(screen, "   HAVE_LDAP\n");
#endif
#ifdef HAVE_LDAP_ADD_RESULT_ENTRY
	output(screen, "   HAVE_LDAP_ADD_RESULT_ENTRY\n");
#endif
#ifdef HAVE_LDAP_DN2AD_CANONICAL
	output(screen, "   HAVE_LDAP_DN2AD_CANONICAL\n");
#endif
#ifdef HAVE_LDAP_INIT
	output(screen, "   HAVE_LDAP_INIT\n");
#endif
#ifdef HAVE_LDAP_INITIALIZE
	output(screen, "   HAVE_LDAP_INITIALIZE\n");
#endif
#ifdef HAVE_LDAP_SET_REBIND_PROC
	output(screen, "   HAVE_LDAP_SET_REBIND_PROC\n");
#endif
#ifdef HAVE_LGETEA
	output(screen, "   HAVE_LGETEA\n");
#endif
#ifdef HAVE_LGETXATTR
	output(screen, "   HAVE_LGETXATTR\n");
#endif
#ifdef HAVE_LIBASN1
	output(screen, "   HAVE_LIBASN1\n");
#endif
#ifdef HAVE_LIBCOM_ERR
	output(screen, "   HAVE_LIBCOM_ERR\n");
#endif
#ifdef HAVE_LIBCRYPTO
	output(screen, "   HAVE_LIBCRYPTO\n");
#endif
#ifdef HAVE_LIBDL
	output(screen, "   HAVE_LIBDL\n");
#endif
#ifdef HAVE_LIBEXC
	output(screen, "   HAVE_LIBEXC\n");
#endif
#ifdef HAVE_LIBFAM
	output(screen, "   HAVE_LIBFAM\n");
#endif
#ifdef HAVE_LIBGSSAPI
	output(screen, "   HAVE_LIBGSSAPI\n");
#endif
#ifdef HAVE_LIBGSSAPI_KRB5
	output(screen, "   HAVE_LIBGSSAPI_KRB5\n");
#endif
#ifdef HAVE_LIBINET
	output(screen, "   HAVE_LIBINET\n");
#endif
#ifdef HAVE_LIBK5CRYPTO
	output(screen, "   HAVE_LIBK5CRYPTO\n");
#endif
#ifdef HAVE_LIBKRB5
	output(screen, "   HAVE_LIBKRB5\n");
#endif
#ifdef HAVE_LIBLBER
	output(screen, "   HAVE_LIBLBER\n");
#endif
#ifdef HAVE_LIBLDAP
	output(screen, "   HAVE_LIBLDAP\n");
#endif
#ifdef HAVE_LIBNSCD
	output(screen, "   HAVE_LIBNSCD\n");
#endif
#ifdef HAVE_LIBNSL
	output(screen, "   HAVE_LIBNSL\n");
#endif
#ifdef HAVE_LIBNSL_S
	output(screen, "   HAVE_LIBNSL_S\n");
#endif
#ifdef HAVE_LIBPAM
	output(screen, "   HAVE_LIBPAM\n");
#endif
#ifdef HAVE_LIBREADLINE
	output(screen, "   HAVE_LIBREADLINE\n");
#endif
#ifdef HAVE_LIBRESOLV
	output(screen, "   HAVE_LIBRESOLV\n");
#endif
#ifdef HAVE_LIBROKEN
	output(screen, "   HAVE_LIBROKEN\n");
#endif
#ifdef HAVE_LIBSENDFILE
	output(screen, "   HAVE_LIBSENDFILE\n");
#endif
#ifdef HAVE_LIBSOCKET
	output(screen, "   HAVE_LIBSOCKET\n");
#endif
#ifdef HAVE_LIBUNWIND
	output(screen, "   HAVE_LIBUNWIND\n");
#endif
#ifdef HAVE_LIBUNWIND_PTRACE
	output(screen, "   HAVE_LIBUNWIND_PTRACE\n");
#endif
#ifdef HAVE_LINK
	output(screen, "   HAVE_LINK\n");
#endif
#ifdef HAVE_LINUX_PTRACE
	output(screen, "   HAVE_LINUX_PTRACE\n");
#endif
#ifdef HAVE_LINUX_XFS_QUOTAS
	output(screen, "   HAVE_LINUX_XFS_QUOTAS\n");
#endif
#ifdef HAVE_LISTEA
	output(screen, "   HAVE_LISTEA\n");
#endif
#ifdef HAVE_LISTXATTR
	output(screen, "   HAVE_LISTXATTR\n");
#endif
#ifdef HAVE_LLISTEA
	output(screen, "   HAVE_LLISTEA\n");
#endif
#ifdef HAVE_LLISTXATTR
	output(screen, "   HAVE_LLISTXATTR\n");
#endif
#ifdef HAVE_LLSEEK
	output(screen, "   HAVE_LLSEEK\n");
#endif
#ifdef HAVE_LONGLONG
	output(screen, "   HAVE_LONGLONG\n");
#endif
#ifdef HAVE_LREMOVEEA
	output(screen, "   HAVE_LREMOVEEA\n");
#endif
#ifdef HAVE_LREMOVEXATTR
	output(screen, "   HAVE_LREMOVEXATTR\n");
#endif
#ifdef HAVE_LSEEK64
	output(screen, "   HAVE_LSEEK64\n");
#endif
#ifdef HAVE_LSETEA
	output(screen, "   HAVE_LSETEA\n");
#endif
#ifdef HAVE_LSETXATTR
	output(screen, "   HAVE_LSETXATTR\n");
#endif
#ifdef HAVE_LSTAT64
	output(screen, "   HAVE_LSTAT64\n");
#endif
#ifdef HAVE_MAGIC_IN_KRB5_ADDRESS
	output(screen, "   HAVE_MAGIC_IN_KRB5_ADDRESS\n");
#endif
#ifdef HAVE_MAKEDEV
	output(screen, "   HAVE_MAKEDEV\n");
#endif
#ifdef HAVE_MEMMOVE
	output(screen, "   HAVE_MEMMOVE\n");
#endif
#ifdef HAVE_MEMSET
	output(screen, "   HAVE_MEMSET\n");
#endif
#ifdef HAVE_MKNOD
	output(screen, "   HAVE_MKNOD\n");
#endif
#ifdef HAVE_MKNOD64
	output(screen, "   HAVE_MKNOD64\n");
#endif
#ifdef HAVE_MKTIME
	output(screen, "   HAVE_MKTIME\n");
#endif
#ifdef HAVE_MLOCK
	output(screen, "   HAVE_MLOCK\n");
#endif
#ifdef HAVE_MLOCKALL
	output(screen, "   HAVE_MLOCKALL\n");
#endif
#ifdef HAVE_MMAP
	output(screen, "   HAVE_MMAP\n");
#endif
#ifdef HAVE_MUNLOCK
	output(screen, "   HAVE_MUNLOCK\n");
#endif
#ifdef HAVE_MUNLOCKALL
	output(screen, "   HAVE_MUNLOCKALL\n");
#endif
#ifdef HAVE_NANOSLEEP
	output(screen, "   HAVE_NANOSLEEP\n");
#endif
#ifdef HAVE_NATIVE_ICONV
	output(screen, "   HAVE_NATIVE_ICONV\n");
#endif
#ifdef HAVE_NEW_LIBREADLINE
	output(screen, "   HAVE_NEW_LIBREADLINE\n");
#endif
#ifdef HAVE_NL_LANGINFO
	output(screen, "   HAVE_NL_LANGINFO\n");
#endif
#ifdef HAVE_NO_ACLS
	output(screen, "   HAVE_NO_ACLS\n");
#endif
#ifdef HAVE_NO_AIO
	output(screen, "   HAVE_NO_AIO\n");
#endif
#ifdef HAVE_NSCD_FLUSH_CACHE
	output(screen, "   HAVE_NSCD_FLUSH_CACHE\n");
#endif
#ifdef HAVE_NSS_XBYY_KEY_IPNODE
	output(screen, "   HAVE_NSS_XBYY_KEY_IPNODE\n");
#endif
#ifdef HAVE_OFF64_T
	output(screen, "   HAVE_OFF64_T\n");
#endif
#ifdef HAVE_OPEN64
	output(screen, "   HAVE_OPEN64\n");
#endif
#ifdef HAVE_OPENDIR64
	output(screen, "   HAVE_OPENDIR64\n");
#endif
#ifdef HAVE_PASSWD_PW_AGE
	output(screen, "   HAVE_PASSWD_PW_AGE\n");
#endif
#ifdef HAVE_PASSWD_PW_COMMENT
	output(screen, "   HAVE_PASSWD_PW_COMMENT\n");
#endif
#ifdef HAVE_PATHCONF
	output(screen, "   HAVE_PATHCONF\n");
#endif
#ifdef HAVE_PIPE
	output(screen, "   HAVE_PIPE\n");
#endif
#ifdef HAVE_POLL
	output(screen, "   HAVE_POLL\n");
#endif
#ifdef HAVE_POSIX_ACLS
	output(screen, "   HAVE_POSIX_ACLS\n");
#endif
#ifdef HAVE_POSIX_CAPABILITIES
	output(screen, "   HAVE_POSIX_CAPABILITIES\n");
#endif
#ifdef HAVE_PRCTL
	output(screen, "   HAVE_PRCTL\n");
#endif
#ifdef HAVE_PREAD
	output(screen, "   HAVE_PREAD\n");
#endif
#ifdef HAVE_PREAD64
	output(screen, "   HAVE_PREAD64\n");
#endif
#ifdef HAVE_PUTPRPWNAM
	output(screen, "   HAVE_PUTPRPWNAM\n");
#endif
#ifdef HAVE_PUTUTLINE
	output(screen, "   HAVE_PUTUTLINE\n");
#endif
#ifdef HAVE_PUTUTXLINE
	output(screen, "   HAVE_PUTUTXLINE\n");
#endif
#ifdef HAVE_PWRITE
	output(screen, "   HAVE_PWRITE\n");
#endif
#ifdef HAVE_PWRITE64
	output(screen, "   HAVE_PWRITE64\n");
#endif
#ifdef HAVE_QUOTACTL_3
	output(screen, "   HAVE_QUOTACTL_3\n");
#endif
#ifdef HAVE_QUOTACTL_4A
	output(screen, "   HAVE_QUOTACTL_4A\n");
#endif
#ifdef HAVE_QUOTACTL_4B
	output(screen, "   HAVE_QUOTACTL_4B\n");
#endif
#ifdef HAVE_QUOTACTL_LINUX
	output(screen, "   HAVE_QUOTACTL_LINUX\n");
#endif
#ifdef HAVE_RAND
	output(screen, "   HAVE_RAND\n");
#endif
#ifdef HAVE_RANDOM
	output(screen, "   HAVE_RANDOM\n");
#endif
#ifdef HAVE_RDCHK
	output(screen, "   HAVE_RDCHK\n");
#endif
#ifdef HAVE_READDIR64
	output(screen, "   HAVE_READDIR64\n");
#endif
#ifdef HAVE_READLINK
	output(screen, "   HAVE_READLINK\n");
#endif
#ifdef HAVE_REALPATH
	output(screen, "   HAVE_REALPATH\n");
#endif
#ifdef HAVE_REMOVEEA
	output(screen, "   HAVE_REMOVEEA\n");
#endif
#ifdef HAVE_REMOVEXATTR
	output(screen, "   HAVE_REMOVEXATTR\n");
#endif
#ifdef HAVE_RENAME
	output(screen, "   HAVE_RENAME\n");
#endif
#ifdef HAVE_REWINDDIR64
	output(screen, "   HAVE_REWINDDIR64\n");
#endif
#ifdef HAVE_ROKEN_GETADDRINFO_HOSTSPEC
	output(screen, "   HAVE_ROKEN_GETADDRINFO_HOSTSPEC\n");
#endif
#ifdef HAVE_ROOT
	output(screen, "   HAVE_ROOT\n");
#endif
#ifdef HAVE_RPC_AUTH_ERROR_CONFLICT
	output(screen, "   HAVE_RPC_AUTH_ERROR_CONFLICT\n");
#endif
#ifdef HAVE_SECURE_MKSTEMP
	output(screen, "   HAVE_SECURE_MKSTEMP\n");
#endif
#ifdef HAVE_SEEKDIR64
	output(screen, "   HAVE_SEEKDIR64\n");
#endif
#ifdef HAVE_SELECT
	output(screen, "   HAVE_SELECT\n");
#endif
#ifdef HAVE_SENDFILE
	output(screen, "   HAVE_SENDFILE\n");
#endif
#ifdef HAVE_SENDFILE64
	output(screen, "   HAVE_SENDFILE64\n");
#endif
#ifdef HAVE_SENDFILEV
	output(screen, "   HAVE_SENDFILEV\n");
#endif
#ifdef HAVE_SENDFILEV64
	output(screen, "   HAVE_SENDFILEV64\n");
#endif
#ifdef HAVE_SETBUFFER
	output(screen, "   HAVE_SETBUFFER\n");
#endif
#ifdef HAVE_SETEA
	output(screen, "   HAVE_SETEA\n");
#endif
#ifdef HAVE_SETENV
	output(screen, "   HAVE_SETENV\n");
#endif
#ifdef HAVE_SETGIDX
	output(screen, "   HAVE_SETGIDX\n");
#endif
#ifdef HAVE_SETGROUPS
	output(screen, "   HAVE_SETGROUPS\n");
#endif
#ifdef HAVE_SETLINEBUF
	output(screen, "   HAVE_SETLINEBUF\n");
#endif
#ifdef HAVE_SETLOCALE
	output(screen, "   HAVE_SETLOCALE\n");
#endif
#ifdef HAVE_SETLUID
	output(screen, "   HAVE_SETLUID\n");
#endif
#ifdef HAVE_SETMNTENT
	output(screen, "   HAVE_SETMNTENT\n");
#endif
#ifdef HAVE_SETNETGRENT
	output(screen, "   HAVE_SETNETGRENT\n");
#endif
#ifdef HAVE_SETPGID
	output(screen, "   HAVE_SETPGID\n");
#endif
#ifdef HAVE_SETPRIV
	output(screen, "   HAVE_SETPRIV\n");
#endif
#ifdef HAVE_SETPROPLIST
	output(screen, "   HAVE_SETPROPLIST\n");
#endif
#ifdef HAVE_SETRESGID
	output(screen, "   HAVE_SETRESGID\n");
#endif
#ifdef HAVE_SETRESGID_DECL
	output(screen, "   HAVE_SETRESGID_DECL\n");
#endif
#ifdef HAVE_SETRESUID
	output(screen, "   HAVE_SETRESUID\n");
#endif
#ifdef HAVE_SETRESUID_DECL
	output(screen, "   HAVE_SETRESUID_DECL\n");
#endif
#ifdef HAVE_SETSID
	output(screen, "   HAVE_SETSID\n");
#endif
#ifdef HAVE_SETUIDX
	output(screen, "   HAVE_SETUIDX\n");
#endif
#ifdef HAVE_SETXATTR
	output(screen, "   HAVE_SETXATTR\n");
#endif
#ifdef HAVE_SET_AUTH_PARAMETERS
	output(screen, "   HAVE_SET_AUTH_PARAMETERS\n");
#endif
#ifdef HAVE_SHMGET
	output(screen, "   HAVE_SHMGET\n");
#endif
#ifdef HAVE_SHM_OPEN
	output(screen, "   HAVE_SHM_OPEN\n");
#endif
#ifdef HAVE_SIGACTION
	output(screen, "   HAVE_SIGACTION\n");
#endif
#ifdef HAVE_SIGBLOCK
	output(screen, "   HAVE_SIGBLOCK\n");
#endif
#ifdef HAVE_SIGPROCMASK
	output(screen, "   HAVE_SIGPROCMASK\n");
#endif
#ifdef HAVE_SIGSET
	output(screen, "   HAVE_SIGSET\n");
#endif
#ifdef HAVE_SIG_ATOMIC_T_TYPE
	output(screen, "   HAVE_SIG_ATOMIC_T_TYPE\n");
#endif
#ifdef HAVE_SIZEOF_PROPLIST_ENTRY
	output(screen, "   HAVE_SIZEOF_PROPLIST_ENTRY\n");
#endif
#ifdef HAVE_SNPRINTF
	output(screen, "   HAVE_SNPRINTF\n");
#endif
#ifdef HAVE_SNPRINTF_DECL
	output(screen, "   HAVE_SNPRINTF_DECL\n");
#endif
#ifdef HAVE_SOCKLEN_T_TYPE
	output(screen, "   HAVE_SOCKLEN_T_TYPE\n");
#endif
#ifdef HAVE_SOCK_SIN_LEN
	output(screen, "   HAVE_SOCK_SIN_LEN\n");
#endif
#ifdef HAVE_SOLARIS_ACLS
	output(screen, "   HAVE_SOLARIS_ACLS\n");
#endif
#ifdef HAVE_SRAND
	output(screen, "   HAVE_SRAND\n");
#endif
#ifdef HAVE_SRANDOM
	output(screen, "   HAVE_SRANDOM\n");
#endif
#ifdef HAVE_STAT64
	output(screen, "   HAVE_STAT64\n");
#endif
#ifdef HAVE_STAT_HIRES_TIMESTAMPS
	output(screen, "   HAVE_STAT_HIRES_TIMESTAMPS\n");
#endif
#ifdef HAVE_STAT_ST_ATIM
	output(screen, "   HAVE_STAT_ST_ATIM\n");
#endif
#ifdef HAVE_STAT_ST_ATIMENSEC
	output(screen, "   HAVE_STAT_ST_ATIMENSEC\n");
#endif
#ifdef HAVE_STAT_ST_BLKSIZE
	output(screen, "   HAVE_STAT_ST_BLKSIZE\n");
#endif
#ifdef HAVE_STAT_ST_BLOCKS
	output(screen, "   HAVE_STAT_ST_BLOCKS\n");
#endif
#ifdef HAVE_STAT_ST_CTIM
	output(screen, "   HAVE_STAT_ST_CTIM\n");
#endif
#ifdef HAVE_STAT_ST_CTIMENSEC
	output(screen, "   HAVE_STAT_ST_CTIMENSEC\n");
#endif
#ifdef HAVE_STAT_ST_MTIM
	output(screen, "   HAVE_STAT_ST_MTIM\n");
#endif
#ifdef HAVE_STAT_ST_MTIMENSEC
	output(screen, "   HAVE_STAT_ST_MTIMENSEC\n");
#endif
#ifdef HAVE_STRCASECMP
	output(screen, "   HAVE_STRCASECMP\n");
#endif
#ifdef HAVE_STRCHR
	output(screen, "   HAVE_STRCHR\n");
#endif
#ifdef HAVE_STRDUP
	output(screen, "   HAVE_STRDUP\n");
#endif
#ifdef HAVE_STRERROR
	output(screen, "   HAVE_STRERROR\n");
#endif
#ifdef HAVE_STRFTIME
	output(screen, "   HAVE_STRFTIME\n");
#endif
#ifdef HAVE_STRLCAT
	output(screen, "   HAVE_STRLCAT\n");
#endif
#ifdef HAVE_STRLCPY
	output(screen, "   HAVE_STRLCPY\n");
#endif
#ifdef HAVE_STRNDUP
	output(screen, "   HAVE_STRNDUP\n");
#endif
#ifdef HAVE_STRNLEN
	output(screen, "   HAVE_STRNLEN\n");
#endif
#ifdef HAVE_STRPBRK
	output(screen, "   HAVE_STRPBRK\n");
#endif
#ifdef HAVE_STRSIGNAL
	output(screen, "   HAVE_STRSIGNAL\n");
#endif
#ifdef HAVE_STRTOUL
	output(screen, "   HAVE_STRTOUL\n");
#endif
#ifdef HAVE_STRUCT_DIR64
	output(screen, "   HAVE_STRUCT_DIR64\n");
#endif
#ifdef HAVE_STRUCT_DIRENT64
	output(screen, "   HAVE_STRUCT_DIRENT64\n");
#endif
#ifdef HAVE_STRUCT_FLOCK64
	output(screen, "   HAVE_STRUCT_FLOCK64\n");
#endif
#ifdef HAVE_STRUCT_SECMETHOD_TABLE_METHOD_ATTRLIST
	output(screen, "   HAVE_STRUCT_SECMETHOD_TABLE_METHOD_ATTRLIST\n");
#endif
#ifdef HAVE_STRUCT_SECMETHOD_TABLE_METHOD_VERSION
	output(screen, "   HAVE_STRUCT_SECMETHOD_TABLE_METHOD_VERSION\n");
#endif
#ifdef HAVE_STRUCT_STAT_ST_RDEV
	output(screen, "   HAVE_STRUCT_STAT_ST_RDEV\n");
#endif
#ifdef HAVE_STRUCT_TIMESPEC
	output(screen, "   HAVE_STRUCT_TIMESPEC\n");
#endif
#ifdef HAVE_ST_RDEV
	output(screen, "   HAVE_ST_RDEV\n");
#endif
#ifdef HAVE_SYMLINK
	output(screen, "   HAVE_SYMLINK\n");
#endif
#ifdef HAVE_SYSCALL
	output(screen, "   HAVE_SYSCALL\n");
#endif
#ifdef HAVE_SYSCONF
	output(screen, "   HAVE_SYSCONF\n");
#endif
#ifdef HAVE_SYSLOG
	output(screen, "   HAVE_SYSLOG\n");
#endif
#ifdef HAVE_SYS_QUOTAS
	output(screen, "   HAVE_SYS_QUOTAS\n");
#endif
#ifdef HAVE_TELLDIR64
	output(screen, "   HAVE_TELLDIR64\n");
#endif
#ifdef HAVE_TICKET_POINTER_IN_KRB5_AP_REQ
	output(screen, "   HAVE_TICKET_POINTER_IN_KRB5_AP_REQ\n");
#endif
#ifdef HAVE_TIMEGM
	output(screen, "   HAVE_TIMEGM\n");
#endif
#ifdef HAVE_TRU64_ACLS
	output(screen, "   HAVE_TRU64_ACLS\n");
#endif
#ifdef HAVE_TRUNCATED_SALT
	output(screen, "   HAVE_TRUNCATED_SALT\n");
#endif
#ifdef HAVE_UNIXSOCKET
	output(screen, "   HAVE_UNIXSOCKET\n");
#endif
#ifdef HAVE_UNIXWARE_ACLS
	output(screen, "   HAVE_UNIXWARE_ACLS\n");
#endif
#ifdef HAVE_UNSIGNED_CHAR
	output(screen, "   HAVE_UNSIGNED_CHAR\n");
#endif
#ifdef HAVE_UPDWTMP
	output(screen, "   HAVE_UPDWTMP\n");
#endif
#ifdef HAVE_UPDWTMPX
	output(screen, "   HAVE_UPDWTMPX\n");
#endif
#ifdef HAVE_USLEEP
	output(screen, "   HAVE_USLEEP\n");
#endif
#ifdef HAVE_UTIMBUF
	output(screen, "   HAVE_UTIMBUF\n");
#endif
#ifdef HAVE_UTIME
	output(screen, "   HAVE_UTIME\n");
#endif
#ifdef HAVE_UTIMES
	output(screen, "   HAVE_UTIMES\n");
#endif
#ifdef HAVE_UX_UT_SYSLEN
	output(screen, "   HAVE_UX_UT_SYSLEN\n");
#endif
#ifdef HAVE_VASPRINTF
	output(screen, "   HAVE_VASPRINTF\n");
#endif
#ifdef HAVE_VASPRINTF_DECL
	output(screen, "   HAVE_VASPRINTF_DECL\n");
#endif
#ifdef HAVE_VA_COPY
	output(screen, "   HAVE_VA_COPY\n");
#endif
#ifdef HAVE_VOLATILE
	output(screen, "   HAVE_VOLATILE\n");
#endif
#ifdef HAVE_VSNPRINTF
	output(screen, "   HAVE_VSNPRINTF\n");
#endif
#ifdef HAVE_VSNPRINTF_DECL
	output(screen, "   HAVE_VSNPRINTF_DECL\n");
#endif
#ifdef HAVE_VSYSLOG
	output(screen, "   HAVE_VSYSLOG\n");
#endif
#ifdef HAVE_WAITPID
	output(screen, "   HAVE_WAITPID\n");
#endif
#ifdef HAVE_WORKING_AF_LOCAL
	output(screen, "   HAVE_WORKING_AF_LOCAL\n");
#endif
#ifdef HAVE_WRFILE_KEYTAB
	output(screen, "   HAVE_WRFILE_KEYTAB\n");
#endif
#ifdef HAVE_XFS_QUOTAS
	output(screen, "   HAVE_XFS_QUOTAS\n");
#endif
#ifdef HAVE_YP_GET_DEFAULT_DOMAIN
	output(screen, "   HAVE_YP_GET_DEFAULT_DOMAIN\n");
#endif
#ifdef HAVE__ACL
	output(screen, "   HAVE__ACL\n");
#endif
#ifdef HAVE__CHDIR
	output(screen, "   HAVE__CHDIR\n");
#endif
#ifdef HAVE__CLOSE
	output(screen, "   HAVE__CLOSE\n");
#endif
#ifdef HAVE__CLOSEDIR
	output(screen, "   HAVE__CLOSEDIR\n");
#endif
#ifdef HAVE__DUP
	output(screen, "   HAVE__DUP\n");
#endif
#ifdef HAVE__DUP2
	output(screen, "   HAVE__DUP2\n");
#endif
#ifdef HAVE__ET_LIST
	output(screen, "   HAVE__ET_LIST\n");
#endif
#ifdef HAVE__FACL
	output(screen, "   HAVE__FACL\n");
#endif
#ifdef HAVE__FCHDIR
	output(screen, "   HAVE__FCHDIR\n");
#endif
#ifdef HAVE__FCNTL
	output(screen, "   HAVE__FCNTL\n");
#endif
#ifdef HAVE__FORK
	output(screen, "   HAVE__FORK\n");
#endif
#ifdef HAVE__FSTAT
	output(screen, "   HAVE__FSTAT\n");
#endif
#ifdef HAVE__FSTAT64
	output(screen, "   HAVE__FSTAT64\n");
#endif
#ifdef HAVE__GETCWD
	output(screen, "   HAVE__GETCWD\n");
#endif
#ifdef HAVE__LLSEEK
	output(screen, "   HAVE__LLSEEK\n");
#endif
#ifdef HAVE__LSEEK
	output(screen, "   HAVE__LSEEK\n");
#endif
#ifdef HAVE__LSTAT
	output(screen, "   HAVE__LSTAT\n");
#endif
#ifdef HAVE__LSTAT64
	output(screen, "   HAVE__LSTAT64\n");
#endif
#ifdef HAVE__OPEN
	output(screen, "   HAVE__OPEN\n");
#endif
#ifdef HAVE__OPEN64
	output(screen, "   HAVE__OPEN64\n");
#endif
#ifdef HAVE__OPENDIR
	output(screen, "   HAVE__OPENDIR\n");
#endif
#ifdef HAVE__PREAD
	output(screen, "   HAVE__PREAD\n");
#endif
#ifdef HAVE__PREAD64
	output(screen, "   HAVE__PREAD64\n");
#endif
#ifdef HAVE__PWRITE
	output(screen, "   HAVE__PWRITE\n");
#endif
#ifdef HAVE__PWRITE64
	output(screen, "   HAVE__PWRITE64\n");
#endif
#ifdef HAVE__READ
	output(screen, "   HAVE__READ\n");
#endif
#ifdef HAVE__READDIR
	output(screen, "   HAVE__READDIR\n");
#endif
#ifdef HAVE__READDIR64
	output(screen, "   HAVE__READDIR64\n");
#endif
#ifdef HAVE__SEEKDIR
	output(screen, "   HAVE__SEEKDIR\n");
#endif
#ifdef HAVE__STAT
	output(screen, "   HAVE__STAT\n");
#endif
#ifdef HAVE__STAT64
	output(screen, "   HAVE__STAT64\n");
#endif
#ifdef HAVE__TELLDIR
	output(screen, "   HAVE__TELLDIR\n");
#endif
#ifdef HAVE__WRITE
	output(screen, "   HAVE__WRITE\n");
#endif
#ifdef HAVE___ACL
	output(screen, "   HAVE___ACL\n");
#endif
#ifdef HAVE___CHDIR
	output(screen, "   HAVE___CHDIR\n");
#endif
#ifdef HAVE___CLOSE
	output(screen, "   HAVE___CLOSE\n");
#endif
#ifdef HAVE___CLOSEDIR
	output(screen, "   HAVE___CLOSEDIR\n");
#endif
#ifdef HAVE___DUP
	output(screen, "   HAVE___DUP\n");
#endif
#ifdef HAVE___DUP2
	output(screen, "   HAVE___DUP2\n");
#endif
#ifdef HAVE___FACL
	output(screen, "   HAVE___FACL\n");
#endif
#ifdef HAVE___FCHDIR
	output(screen, "   HAVE___FCHDIR\n");
#endif
#ifdef HAVE___FCNTL
	output(screen, "   HAVE___FCNTL\n");
#endif
#ifdef HAVE___FORK
	output(screen, "   HAVE___FORK\n");
#endif
#ifdef HAVE___FSTAT
	output(screen, "   HAVE___FSTAT\n");
#endif
#ifdef HAVE___FSTAT64
	output(screen, "   HAVE___FSTAT64\n");
#endif
#ifdef HAVE___FXSTAT
	output(screen, "   HAVE___FXSTAT\n");
#endif
#ifdef HAVE___GETCWD
	output(screen, "   HAVE___GETCWD\n");
#endif
#ifdef HAVE___GETDENTS
	output(screen, "   HAVE___GETDENTS\n");
#endif
#ifdef HAVE___LLSEEK
	output(screen, "   HAVE___LLSEEK\n");
#endif
#ifdef HAVE___LSEEK
	output(screen, "   HAVE___LSEEK\n");
#endif
#ifdef HAVE___LSTAT
	output(screen, "   HAVE___LSTAT\n");
#endif
#ifdef HAVE___LSTAT64
	output(screen, "   HAVE___LSTAT64\n");
#endif
#ifdef HAVE___LXSTAT
	output(screen, "   HAVE___LXSTAT\n");
#endif
#ifdef HAVE___OPEN
	output(screen, "   HAVE___OPEN\n");
#endif
#ifdef HAVE___OPEN64
	output(screen, "   HAVE___OPEN64\n");
#endif
#ifdef HAVE___OPENDIR
	output(screen, "   HAVE___OPENDIR\n");
#endif
#ifdef HAVE___PREAD
	output(screen, "   HAVE___PREAD\n");
#endif
#ifdef HAVE___PREAD64
	output(screen, "   HAVE___PREAD64\n");
#endif
#ifdef HAVE___PWRITE
	output(screen, "   HAVE___PWRITE\n");
#endif
#ifdef HAVE___PWRITE64
	output(screen, "   HAVE___PWRITE64\n");
#endif
#ifdef HAVE___READ
	output(screen, "   HAVE___READ\n");
#endif
#ifdef HAVE___READDIR
	output(screen, "   HAVE___READDIR\n");
#endif
#ifdef HAVE___READDIR64
	output(screen, "   HAVE___READDIR64\n");
#endif
#ifdef HAVE___SEEKDIR
	output(screen, "   HAVE___SEEKDIR\n");
#endif
#ifdef HAVE___STAT
	output(screen, "   HAVE___STAT\n");
#endif
#ifdef HAVE___STAT64
	output(screen, "   HAVE___STAT64\n");
#endif
#ifdef HAVE___SYS_LLSEEK
	output(screen, "   HAVE___SYS_LLSEEK\n");
#endif
#ifdef HAVE___TELLDIR
	output(screen, "   HAVE___TELLDIR\n");
#endif
#ifdef HAVE___VA_COPY
	output(screen, "   HAVE___VA_COPY\n");
#endif
#ifdef HAVE___WRITE
	output(screen, "   HAVE___WRITE\n");
#endif
#ifdef HAVE___XSTAT
	output(screen, "   HAVE___XSTAT\n");
#endif

	/* Show --with Options */
	output(screen, "\n --with Options:\n");

#ifdef WITH_ADS
	output(screen, "   WITH_ADS\n");
#endif
#ifdef WITH_AFS
	output(screen, "   WITH_AFS\n");
#endif
#ifdef WITH_AIO
	output(screen, "   WITH_AIO\n");
#endif
#ifdef WITH_AUTOMOUNT
	output(screen, "   WITH_AUTOMOUNT\n");
#endif
#ifdef WITH_CIFSMOUNT
	output(screen, "   WITH_CIFSMOUNT\n");
#endif
#ifdef WITH_DFS
	output(screen, "   WITH_DFS\n");
#endif
#ifdef WITH_FAKE_KASERVER
	output(screen, "   WITH_FAKE_KASERVER\n");
#endif
#ifdef WITH_NISPLUS_HOME
	output(screen, "   WITH_NISPLUS_HOME\n");
#endif
#ifdef WITH_PAM
	output(screen, "   WITH_PAM\n");
#endif
#ifdef WITH_PROFILE
	output(screen, "   WITH_PROFILE\n");
#endif
#ifdef WITH_QUOTAS
	output(screen, "   WITH_QUOTAS\n");
#endif
#ifdef WITH_SENDFILE
	output(screen, "   WITH_SENDFILE\n");
#endif
#ifdef WITH_SMBMOUNT
	output(screen, "   WITH_SMBMOUNT\n");
#endif
#ifdef WITH_SMBWRAPPER
	output(screen, "   WITH_SMBWRAPPER\n");
#endif
#ifdef WITH_SYSLOG
	output(screen, "   WITH_SYSLOG\n");
#endif
#ifdef WITH_UTMP
	output(screen, "   WITH_UTMP\n");
#endif
#ifdef WITH_WINBIND
	output(screen, "   WITH_WINBIND\n");
#endif

	/* Show Build Options */
	output(screen, "\n Build Options:\n");

#ifdef AIX
	output(screen, "   AIX\n");
#endif
#ifdef AIX_SENDFILE_API
	output(screen, "   AIX_SENDFILE_API\n");
#endif
#ifdef BROKEN_EXTATTR
	output(screen, "   BROKEN_EXTATTR\n");
#endif
#ifdef BROKEN_GETGRNAM
	output(screen, "   BROKEN_GETGRNAM\n");
#endif
#ifdef BROKEN_NISPLUS_INCLUDE_FILES
	output(screen, "   BROKEN_NISPLUS_INCLUDE_FILES\n");
#endif
#ifdef BROKEN_REDHAT_7_SYSTEM_HEADERS
	output(screen, "   BROKEN_REDHAT_7_SYSTEM_HEADERS\n");
#endif
#ifdef BROKEN_STRNDUP
	output(screen, "   BROKEN_STRNDUP\n");
#endif
#ifdef BROKEN_STRNLEN
	output(screen, "   BROKEN_STRNLEN\n");
#endif
#ifdef BROKEN_UNICODE_COMPOSE_CHARACTERS
	output(screen, "   BROKEN_UNICODE_COMPOSE_CHARACTERS\n");
#endif
#ifdef CLUSTER_SUPPORT
	output(screen, "   CLUSTER_SUPPORT\n");
#endif
#ifdef COMPILER_SUPPORTS_LL
	output(screen, "   COMPILER_SUPPORTS_LL\n");
#endif
#ifdef DARWINOS
	output(screen, "   DARWINOS\n");
#endif
#ifdef DEFAULT_DISPLAY_CHARSET
	output(screen, "   DEFAULT_DISPLAY_CHARSET\n");
#endif
#ifdef DEFAULT_DOS_CHARSET
	output(screen, "   DEFAULT_DOS_CHARSET\n");
#endif
#ifdef DEFAULT_UNIX_CHARSET
	output(screen, "   DEFAULT_UNIX_CHARSET\n");
#endif
#ifdef DMALLOC_FUNC_CHECK
	output(screen, "   DMALLOC_FUNC_CHECK\n");
#endif
#ifdef ENABLE_DMALLOC
	output(screen, "   ENABLE_DMALLOC\n");
#endif
#ifdef FREEBSD
	output(screen, "   FREEBSD\n");
#endif
#ifdef FREEBSD_SENDFILE_API
	output(screen, "   FREEBSD_SENDFILE_API\n");
#endif
#ifdef HPUX
	output(screen, "   HPUX\n");
#endif
#ifdef HPUX_SENDFILE_API
	output(screen, "   HPUX_SENDFILE_API\n");
#endif
#ifdef INTEL_SPINLOCKS
	output(screen, "   INTEL_SPINLOCKS\n");
#endif
#ifdef IRIX
	output(screen, "   IRIX\n");
#endif
#ifdef IRIX6
	output(screen, "   IRIX6\n");
#endif
#ifdef KRB5_PRINC_REALM_RETURNS_REALM
	output(screen, "   KRB5_PRINC_REALM_RETURNS_REALM\n");
#endif
#ifdef KRB5_VERIFY_CHECKSUM_ARGS
	output(screen, "   KRB5_VERIFY_CHECKSUM_ARGS\n");
#endif
#ifdef LDAP_SET_REBIND_PROC_ARGS
	output(screen, "   LDAP_SET_REBIND_PROC_ARGS\n");
#endif
#ifdef LINUX
	output(screen, "   LINUX\n");
#endif
#ifdef LINUX_BROKEN_SENDFILE_API
	output(screen, "   LINUX_BROKEN_SENDFILE_API\n");
#endif
#ifdef LINUX_SENDFILE_API
	output(screen, "   LINUX_SENDFILE_API\n");
#endif
#ifdef MIPS_SPINLOCKS
	output(screen, "   MIPS_SPINLOCKS\n");
#endif
#ifdef MMAP_BLACKLIST
	output(screen, "   MMAP_BLACKLIST\n");
#endif
#ifdef NEXT2
	output(screen, "   NEXT2\n");
#endif
#ifdef NO_MINUS_C_MINUS_O
	output(screen, "   NO_MINUS_C_MINUS_O\n");
#endif
#ifdef OSF1
	output(screen, "   OSF1\n");
#endif
#ifdef PACKAGE_BUGREPORT
	output(screen, "   PACKAGE_BUGREPORT\n");
#endif
#ifdef PACKAGE_NAME
	output(screen, "   PACKAGE_NAME\n");
#endif
#ifdef PACKAGE_STRING
	output(screen, "   PACKAGE_STRING\n");
#endif
#ifdef PACKAGE_TARNAME
	output(screen, "   PACKAGE_TARNAME\n");
#endif
#ifdef PACKAGE_VERSION
	output(screen, "   PACKAGE_VERSION\n");
#endif
#ifdef POSIX_ACL_NEEDS_MASK
	output(screen, "   POSIX_ACL_NEEDS_MASK\n");
#endif
#ifdef POWERPC_SPINLOCKS
	output(screen, "   POWERPC_SPINLOCKS\n");
#endif
#ifdef QNX
	output(screen, "   QNX\n");
#endif
#ifdef REALPATH_TAKES_NULL
	output(screen, "   REALPATH_TAKES_NULL\n");
#endif
#ifdef RELIANTUNIX
	output(screen, "   RELIANTUNIX\n");
#endif
#ifdef REPLACE_GETPASS
	output(screen, "   REPLACE_GETPASS\n");
#endif
#ifdef REPLACE_INET_NTOA
	output(screen, "   REPLACE_INET_NTOA\n");
#endif
#ifdef REPLACE_READDIR
	output(screen, "   REPLACE_READDIR\n");
#endif
#ifdef RETSIGTYPE
	output(screen, "   RETSIGTYPE\n");
#endif
#ifdef SCO
	output(screen, "   SCO\n");
#endif
#ifdef SEEKDIR_RETURNS_VOID
	output(screen, "   SEEKDIR_RETURNS_VOID\n");
#endif
#ifdef SIZEOF_DEV_T
	output(screen, "   SIZEOF_DEV_T\n");
#endif
#ifdef SIZEOF_INO_T
	output(screen, "   SIZEOF_INO_T\n");
#endif
#ifdef SIZEOF_INT
	output(screen, "   SIZEOF_INT\n");
#endif
#ifdef SIZEOF_LONG
	output(screen, "   SIZEOF_LONG\n");
#endif
#ifdef SIZEOF_LONG_LONG
	output(screen, "   SIZEOF_LONG_LONG\n");
#endif
#ifdef SIZEOF_OFF_T
	output(screen, "   SIZEOF_OFF_T\n");
#endif
#ifdef SIZEOF_SHORT
	output(screen, "   SIZEOF_SHORT\n");
#endif
#ifdef SOCKET_WRAPPER
	output(screen, "   SOCKET_WRAPPER\n");
#endif
#ifdef SOLARIS_SENDFILE_API
	output(screen, "   SOLARIS_SENDFILE_API\n");
#endif
#ifdef SPARC_SPINLOCKS
	output(screen, "   SPARC_SPINLOCKS\n");
#endif
#ifdef STAT_STATFS2_BSIZE
	output(screen, "   STAT_STATFS2_BSIZE\n");
#endif
#ifdef STAT_STATFS2_FSIZE
	output(screen, "   STAT_STATFS2_FSIZE\n");
#endif
#ifdef STAT_STATFS2_FS_DATA
	output(screen, "   STAT_STATFS2_FS_DATA\n");
#endif
#ifdef STAT_STATFS3_OSF1
	output(screen, "   STAT_STATFS3_OSF1\n");
#endif
#ifdef STAT_STATFS4
	output(screen, "   STAT_STATFS4\n");
#endif
#ifdef STAT_STATVFS
	output(screen, "   STAT_STATVFS\n");
#endif
#ifdef STAT_STATVFS64
	output(screen, "   STAT_STATVFS64\n");
#endif
#ifdef STAT_ST_BLOCKSIZE
	output(screen, "   STAT_ST_BLOCKSIZE\n");
#endif
#ifdef STDC_HEADERS
	output(screen, "   STDC_HEADERS\n");
#endif
#ifdef STRING_STATIC_MODULES
	output(screen, "   STRING_STATIC_MODULES\n");
#endif
#ifdef SUNOS4
	output(screen, "   SUNOS4\n");
#endif
#ifdef SUNOS5
	output(screen, "   SUNOS5\n");
#endif
#ifdef SYSCONF_SC_NGROUPS_MAX
	output(screen, "   SYSCONF_SC_NGROUPS_MAX\n");
#endif
#ifdef SYSCONF_SC_NPROCESSORS_ONLN
	output(screen, "   SYSCONF_SC_NPROCESSORS_ONLN\n");
#endif
#ifdef SYSCONF_SC_NPROC_ONLN
	output(screen, "   SYSCONF_SC_NPROC_ONLN\n");
#endif
#ifdef SYSCONF_SC_PAGESIZE
	output(screen, "   SYSCONF_SC_PAGESIZE\n");
#endif
#ifdef SYSV
	output(screen, "   SYSV\n");
#endif
#ifdef TIME_WITH_SYS_TIME
	output(screen, "   TIME_WITH_SYS_TIME\n");
#endif
#ifdef UNIXWARE
	output(screen, "   UNIXWARE\n");
#endif
#ifdef USE_BOTH_CRYPT_CALLS
	output(screen, "   USE_BOTH_CRYPT_CALLS\n");
#endif
#ifdef USE_DMAPI
	output(screen, "   USE_DMAPI\n");
#endif
#ifdef USE_SETEUID
	output(screen, "   USE_SETEUID\n");
#endif
#ifdef USE_SETRESUID
	output(screen, "   USE_SETRESUID\n");
#endif
#ifdef USE_SETREUID
	output(screen, "   USE_SETREUID\n");
#endif
#ifdef USE_SETUIDX
	output(screen, "   USE_SETUIDX\n");
#endif
#ifdef USE_SPINLOCKS
	output(screen, "   USE_SPINLOCKS\n");
#endif
#ifdef WITH_ADS
	output(screen, "   WITH_ADS\n");
#endif
#ifdef WITH_AFS
	output(screen, "   WITH_AFS\n");
#endif
#ifdef WITH_AIO
	output(screen, "   WITH_AIO\n");
#endif
#ifdef WITH_AUTOMOUNT
	output(screen, "   WITH_AUTOMOUNT\n");
#endif
#ifdef WITH_CIFSMOUNT
	output(screen, "   WITH_CIFSMOUNT\n");
#endif
#ifdef WITH_DFS
	output(screen, "   WITH_DFS\n");
#endif
#ifdef WITH_FAKE_KASERVER
	output(screen, "   WITH_FAKE_KASERVER\n");
#endif
#ifdef WITH_NISPLUS_HOME
	output(screen, "   WITH_NISPLUS_HOME\n");
#endif
#ifdef WITH_PAM
	output(screen, "   WITH_PAM\n");
#endif
#ifdef WITH_PROFILE
	output(screen, "   WITH_PROFILE\n");
#endif
#ifdef WITH_QUOTAS
	output(screen, "   WITH_QUOTAS\n");
#endif
#ifdef WITH_SENDFILE
	output(screen, "   WITH_SENDFILE\n");
#endif
#ifdef WITH_SMBMOUNT
	output(screen, "   WITH_SMBMOUNT\n");
#endif
#ifdef WITH_SMBWRAPPER
	output(screen, "   WITH_SMBWRAPPER\n");
#endif
#ifdef WITH_SYSLOG
	output(screen, "   WITH_SYSLOG\n");
#endif
#ifdef WITH_WINBIND
	output(screen, "   WITH_WINBIND\n");
#endif
#ifdef WORDS_BIGENDIAN
	output(screen, "   WORDS_BIGENDIAN\n");
#endif
#ifdef _ALIGNMENT_REQUIRED
	output(screen, "   _ALIGNMENT_REQUIRED\n");
#endif
#ifdef _FILE_OFFSET_BITS
	output(screen, "   _FILE_OFFSET_BITS\n");
#endif
#ifdef _GNU_SOURCE
	output(screen, "   _GNU_SOURCE\n");
#endif
#ifdef _HPUX_SOURCE
	output(screen, "   _HPUX_SOURCE\n");
#endif
#ifdef _LARGEFILE64_SOURCE
	output(screen, "   _LARGEFILE64_SOURCE\n");
#endif
#ifdef _LARGE_FILES
	output(screen, "   _LARGE_FILES\n");
#endif
#ifdef _MAX_ALIGNMENT
	output(screen, "   _MAX_ALIGNMENT\n");
#endif
#ifdef _POSIX_C_SOURCE
	output(screen, "   _POSIX_C_SOURCE\n");
#endif
#ifdef _POSIX_SOURCE
	output(screen, "   _POSIX_SOURCE\n");
#endif
#ifdef _SYSV
	output(screen, "   _SYSV\n");
#endif
#ifdef auth_builtin_init
	output(screen, "   auth_builtin_init\n");
#endif
#ifdef auth_domain_init
	output(screen, "   auth_domain_init\n");
#endif
#ifdef auth_sam_init
	output(screen, "   auth_sam_init\n");
#endif
#ifdef auth_script_init
	output(screen, "   auth_script_init\n");
#endif
#ifdef auth_server_init
	output(screen, "   auth_server_init\n");
#endif
#ifdef auth_unix_init
	output(screen, "   auth_unix_init\n");
#endif
#ifdef auth_winbind_init
	output(screen, "   auth_winbind_init\n");
#endif
#ifdef charset_CP437_init
	output(screen, "   charset_CP437_init\n");
#endif
#ifdef charset_CP850_init
	output(screen, "   charset_CP850_init\n");
#endif
#ifdef charset_macosxfs_init
	output(screen, "   charset_macosxfs_init\n");
#endif
#ifdef charset_weird_init
	output(screen, "   charset_weird_init\n");
#endif
#ifdef const
	output(screen, "   const\n");
#endif
#ifdef gid_t
	output(screen, "   gid_t\n");
#endif
#ifdef idmap_ad_init
	output(screen, "   idmap_ad_init\n");
#endif
#ifdef idmap_ldap_init
	output(screen, "   idmap_ldap_init\n");
#endif
#ifdef idmap_rid_init
	output(screen, "   idmap_rid_init\n");
#endif
#ifdef idmap_tdb_init
	output(screen, "   idmap_tdb_init\n");
#endif
#ifdef inline
	output(screen, "   inline\n");
#endif
#ifdef ino_t
	output(screen, "   ino_t\n");
#endif
#ifdef intptr_t
	output(screen, "   intptr_t\n");
#endif
#ifdef loff_t
	output(screen, "   loff_t\n");
#endif
#ifdef mode_t
	output(screen, "   mode_t\n");
#endif
#ifdef off_t
	output(screen, "   off_t\n");
#endif
#ifdef offset_t
	output(screen, "   offset_t\n");
#endif
#ifdef pdb_ldap_init
	output(screen, "   pdb_ldap_init\n");
#endif
#ifdef pdb_smbpasswd_init
	output(screen, "   pdb_smbpasswd_init\n");
#endif
#ifdef pdb_tdbsam_init
	output(screen, "   pdb_tdbsam_init\n");
#endif
#ifdef pid_t
	output(screen, "   pid_t\n");
#endif
#ifdef rpc_echo_init
	output(screen, "   rpc_echo_init\n");
#endif
#ifdef rpc_eventlog_init
	output(screen, "   rpc_eventlog_init\n");
#endif
#ifdef rpc_lsa_ds_init
	output(screen, "   rpc_lsa_ds_init\n");
#endif
#ifdef rpc_lsa_init
	output(screen, "   rpc_lsa_init\n");
#endif
#ifdef rpc_net_init
	output(screen, "   rpc_net_init\n");
#endif
#ifdef rpc_netdfs_init
	output(screen, "   rpc_netdfs_init\n");
#endif
#ifdef rpc_ntsvcs_init
	output(screen, "   rpc_ntsvcs_init\n");
#endif
#ifdef rpc_reg_init
	output(screen, "   rpc_reg_init\n");
#endif
#ifdef rpc_samr_init
	output(screen, "   rpc_samr_init\n");
#endif
#ifdef rpc_spoolss_init
	output(screen, "   rpc_spoolss_init\n");
#endif
#ifdef rpc_srv_init
	output(screen, "   rpc_srv_init\n");
#endif
#ifdef rpc_svcctl_init
	output(screen, "   rpc_svcctl_init\n");
#endif
#ifdef rpc_wks_init
	output(screen, "   rpc_wks_init\n");
#endif
#ifdef size_t
	output(screen, "   size_t\n");
#endif
#ifdef ssize_t
	output(screen, "   ssize_t\n");
#endif
#ifdef static_decl_auth
	output(screen, "   static_decl_auth\n");
#endif
#ifdef static_decl_charset
	output(screen, "   static_decl_charset\n");
#endif
#ifdef static_decl_idmap
	output(screen, "   static_decl_idmap\n");
#endif
#ifdef static_decl_pdb
	output(screen, "   static_decl_pdb\n");
#endif
#ifdef static_decl_rpc
	output(screen, "   static_decl_rpc\n");
#endif
#ifdef static_decl_vfs
	output(screen, "   static_decl_vfs\n");
#endif
#ifdef static_init_auth
	output(screen, "   static_init_auth\n");
#endif
#ifdef static_init_charset
	output(screen, "   static_init_charset\n");
#endif
#ifdef static_init_idmap
	output(screen, "   static_init_idmap\n");
#endif
#ifdef static_init_pdb
	output(screen, "   static_init_pdb\n");
#endif
#ifdef static_init_rpc
	output(screen, "   static_init_rpc\n");
#endif
#ifdef static_init_vfs
	output(screen, "   static_init_vfs\n");
#endif
#ifdef uid_t
	output(screen, "   uid_t\n");
#endif
#ifdef vfs_afsacl_init
	output(screen, "   vfs_afsacl_init\n");
#endif
#ifdef vfs_audit_init
	output(screen, "   vfs_audit_init\n");
#endif
#ifdef vfs_cap_init
	output(screen, "   vfs_cap_init\n");
#endif
#ifdef vfs_catia_init
	output(screen, "   vfs_catia_init\n");
#endif
#ifdef vfs_default_quota_init
	output(screen, "   vfs_default_quota_init\n");
#endif
#ifdef vfs_expand_msdfs_init
	output(screen, "   vfs_expand_msdfs_init\n");
#endif
#ifdef vfs_extd_audit_init
	output(screen, "   vfs_extd_audit_init\n");
#endif
#ifdef vfs_fake_perms_init
	output(screen, "   vfs_fake_perms_init\n");
#endif
#ifdef vfs_full_audit_init
	output(screen, "   vfs_full_audit_init\n");
#endif
#ifdef vfs_netatalk_init
	output(screen, "   vfs_netatalk_init\n");
#endif
#ifdef vfs_readonly_init
	output(screen, "   vfs_readonly_init\n");
#endif
#ifdef vfs_recycle_init
	output(screen, "   vfs_recycle_init\n");
#endif
#ifdef vfs_shadow_copy_init
	output(screen, "   vfs_shadow_copy_init\n");
#endif
#ifdef wchar_t
	output(screen, "   wchar_t\n");
#endif
       /* Output the sizes of the various types */
       output(screen, "\nType sizes:\n");
       output(screen, "   sizeof(char):         %lu\n",(unsigned long)sizeof(char));
       output(screen, "   sizeof(int):          %lu\n",(unsigned long)sizeof(int));
       output(screen, "   sizeof(long):         %lu\n",(unsigned long)sizeof(long));
#if HAVE_LONGLONG
       output(screen, "   sizeof(long long):    %lu\n",(unsigned long)sizeof(long long));
#endif
       output(screen, "   sizeof(uint8):        %lu\n",(unsigned long)sizeof(uint8));
       output(screen, "   sizeof(uint16):       %lu\n",(unsigned long)sizeof(uint16));
       output(screen, "   sizeof(uint32):       %lu\n",(unsigned long)sizeof(uint32));
       output(screen, "   sizeof(short):        %lu\n",(unsigned long)sizeof(short));
       output(screen, "   sizeof(void*):        %lu\n",(unsigned long)sizeof(void*));
       output(screen, "   sizeof(size_t):       %lu\n",(unsigned long)sizeof(size_t));
       output(screen, "   sizeof(off_t):        %lu\n",(unsigned long)sizeof(off_t));
       output(screen, "   sizeof(ino_t):        %lu\n",(unsigned long)sizeof(ino_t));
       output(screen, "   sizeof(dev_t):        %lu\n",(unsigned long)sizeof(dev_t));
       output(screen, "\nBuiltin modules:\n");
       output(screen, "   %s\n", STRING_STATIC_MODULES);
}
