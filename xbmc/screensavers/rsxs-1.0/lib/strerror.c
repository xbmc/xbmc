/* strerror.c --- POSIX compatible system error routine

   Copyright (C) 2007-2008 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

#include <string.h>

#if REPLACE_STRERROR

# include <errno.h>
# include <stdio.h>

# if GNULIB_defined_ESOCK /* native Windows platforms */
#  if HAVE_WINSOCK2_H
#   include <winsock2.h>
#  endif
# endif

# include "intprops.h"

# undef strerror
# if ! HAVE_DECL_STRERROR
#  define strerror(n) NULL
# endif

char *
rpl_strerror (int n)
{
  /* These error messages are taken from glibc/sysdeps/gnu/errlist.c.  */
  switch (n)
    {
# if GNULIB_defined_ETXTBSY
    case ETXTBSY:
      return "Text file busy";
# endif

# if GNULIB_defined_ESOCK /* native Windows platforms */
    /* EWOULDBLOCK is the same as EAGAIN.  */
    case EINPROGRESS:
      return "Operation now in progress";
    case EALREADY:
      return "Operation already in progress";
    case ENOTSOCK:
      return "Socket operation on non-socket";
    case EDESTADDRREQ:
      return "Destination address required";
    case EMSGSIZE:
      return "Message too long";
    case EPROTOTYPE:
      return "Protocol wrong type for socket";
    case ENOPROTOOPT:
      return "Protocol not available";
    case EPROTONOSUPPORT:
      return "Protocol not supported";
    case ESOCKTNOSUPPORT:
      return "Socket type not supported";
    case EOPNOTSUPP:
      return "Operation not supported";
    case EPFNOSUPPORT:
      return "Protocol family not supported";
    case EAFNOSUPPORT:
      return "Address family not supported by protocol";
    case EADDRINUSE:
      return "Address already in use";
    case EADDRNOTAVAIL:
      return "Cannot assign requested address";
    case ENETDOWN:
      return "Network is down";
    case ENETUNREACH:
      return "Network is unreachable";
    case ENETRESET:
      return "Network dropped connection on reset";
    case ECONNABORTED:
      return "Software caused connection abort";
    case ECONNRESET:
      return "Connection reset by peer";
    case ENOBUFS:
      return "No buffer space available";
    case EISCONN:
      return "Transport endpoint is already connected";
    case ENOTCONN:
      return "Transport endpoint is not connected";
    case ESHUTDOWN:
      return "Cannot send after transport endpoint shutdown";
    case ETOOMANYREFS:
      return "Too many references: cannot splice";
    case ETIMEDOUT:
      return "Connection timed out";
    case ECONNREFUSED:
      return "Connection refused";
    case ELOOP:
      return "Too many levels of symbolic links";
    case EHOSTDOWN:
      return "Host is down";
    case EHOSTUNREACH:
      return "No route to host";
    case EPROCLIM:
      return "Too many processes";
    case EUSERS:
      return "Too many users";
    case EDQUOT:
      return "Disk quota exceeded";
    case ESTALE:
      return "Stale NFS file handle";
    case EREMOTE:
      return "Object is remote";
#  if HAVE_WINSOCK2_H
    /* WSA_INVALID_HANDLE maps to EBADF */
    /* WSA_NOT_ENOUGH_MEMORY maps to ENOMEM */
    /* WSA_INVALID_PARAMETER maps to EINVAL */
    case WSA_OPERATION_ABORTED:
      return "Overlapped operation aborted";
    case WSA_IO_INCOMPLETE:
      return "Overlapped I/O event object not in signaled state";
    case WSA_IO_PENDING:
      return "Overlapped operations will complete later";
    /* WSAEINTR maps to EINTR */
    /* WSAEBADF maps to EBADF */
    /* WSAEACCES maps to EACCES */
    /* WSAEFAULT maps to EFAULT */
    /* WSAEINVAL maps to EINVAL */
    /* WSAEMFILE maps to EMFILE */
    /* WSAEWOULDBLOCK maps to EWOULDBLOCK */
    /* WSAEINPROGRESS is EINPROGRESS */
    /* WSAEALREADY is EALREADY */
    /* WSAENOTSOCK is ENOTSOCK */
    /* WSAEDESTADDRREQ is EDESTADDRREQ */
    /* WSAEMSGSIZE is EMSGSIZE */
    /* WSAEPROTOTYPE is EPROTOTYPE */
    /* WSAENOPROTOOPT is ENOPROTOOPT */
    /* WSAEPROTONOSUPPORT is EPROTONOSUPPORT */
    /* WSAESOCKTNOSUPPORT is ESOCKTNOSUPPORT */
    /* WSAEOPNOTSUPP is EOPNOTSUPP */
    /* WSAEPFNOSUPPORT is EPFNOSUPPORT */
    /* WSAEAFNOSUPPORT is EAFNOSUPPORT */
    /* WSAEADDRINUSE is EADDRINUSE */
    /* WSAEADDRNOTAVAIL is EADDRNOTAVAIL */
    /* WSAENETDOWN is ENETDOWN */
    /* WSAENETUNREACH is ENETUNREACH */
    /* WSAENETRESET is ENETRESET */
    /* WSAECONNABORTED is ECONNABORTED */
    /* WSAECONNRESET is ECONNRESET */
    /* WSAENOBUFS is ENOBUFS */
    /* WSAEISCONN is EISCONN */
    /* WSAENOTCONN is ENOTCONN */
    /* WSAESHUTDOWN is ESHUTDOWN */
    /* WSAETOOMANYREFS is ETOOMANYREFS */
    /* WSAETIMEDOUT is ETIMEDOUT */
    /* WSAECONNREFUSED is ECONNREFUSED */
    /* WSAELOOP is ELOOP */
    /* WSAENAMETOOLONG maps to ENAMETOOLONG */
    /* WSAEHOSTDOWN is EHOSTDOWN */
    /* WSAEHOSTUNREACH is EHOSTUNREACH */
    /* WSAENOTEMPTY maps to ENOTEMPTY */
    /* WSAEPROCLIM is EPROCLIM */
    /* WSAEUSERS is EUSERS */
    /* WSAEDQUOT is EDQUOT */
    /* WSAESTALE is ESTALE */
    /* WSAEREMOTE is EREMOTE */
    case WSASYSNOTREADY:
      return "Network subsystem is unavailable";
    case WSAVERNOTSUPPORTED:
      return "Winsock.dll version out of range";
    case WSANOTINITIALISED:
      return "Successful WSAStartup not yet performed";
    case WSAEDISCON:
      return "Graceful shutdown in progress";
    case WSAENOMORE: case WSA_E_NO_MORE:
      return "No more results";
    case WSAECANCELLED: case WSA_E_CANCELLED:
      return "Call was canceled";
    case WSAEINVALIDPROCTABLE:
      return "Procedure call table is invalid";
    case WSAEINVALIDPROVIDER:
      return "Service provider is invalid";
    case WSAEPROVIDERFAILEDINIT:
      return "Service provider failed to initialize";
    case WSASYSCALLFAILURE:
      return "System call failure";
    case WSASERVICE_NOT_FOUND:
      return "Service not found";
    case WSATYPE_NOT_FOUND:
      return "Class type not found";
    case WSAEREFUSED:
      return "Database query was refused";
    case WSAHOST_NOT_FOUND:
      return "Host not found";
    case WSATRY_AGAIN:
      return "Nonauthoritative host not found";
    case WSANO_RECOVERY:
      return "Nonrecoverable error";
    case WSANO_DATA:
      return "Valid name, no data record of requested type";
    /* WSA_QOS_* omitted */
#  endif
# endif

# if GNULIB_defined_ENOMSG
    case ENOMSG:
      return "No message of desired type";
# endif

# if GNULIB_defined_EIDRM
    case EIDRM:
      return "Identifier removed";
# endif

# if GNULIB_defined_ENOLINK
    case ENOLINK:
      return "Link has been severed";
# endif

# if GNULIB_defined_EPROTO
    case EPROTO:
      return "Protocol error";
# endif

# if GNULIB_defined_EMULTIHOP
    case EMULTIHOP:
      return "Multihop attempted";
# endif

# if GNULIB_defined_EBADMSG
    case EBADMSG:
      return "Bad message";
# endif

# if GNULIB_defined_EOVERFLOW
    case EOVERFLOW:
      return "Value too large for defined data type";
# endif

# if GNULIB_defined_ENOTSUP
    case ENOTSUP:
      return "Not supported";
# endif

# if GNULIB_defined_
    case ECANCELED:
      return "Operation canceled";
# endif
    }

  {
    char *result = strerror (n);

    if (result == NULL || result[0] == '\0')
      {
	static char const fmt[] = "Unknown error (%d)";
	static char mesg[sizeof fmt + INT_STRLEN_BOUND (n)];
	sprintf (mesg, fmt, n);
	return mesg;
      }

    return result;
  }
}

#endif
