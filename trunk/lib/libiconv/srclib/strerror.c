/* strerror.c --- POSIX compatible system error routine

   Copyright (C) 2007-2009 Free Software Foundation, Inc.

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
  char const *msg = NULL;
  /* These error messages are taken from glibc/sysdeps/gnu/errlist.c.  */
  switch (n)
    {
# if GNULIB_defined_ETXTBSY
    case ETXTBSY:
      msg = "Text file busy";
      break;
# endif

# if GNULIB_defined_ESOCK /* native Windows platforms */
    /* EWOULDBLOCK is the same as EAGAIN.  */
    case EINPROGRESS:
      msg = "Operation now in progress";
      break;
    case EALREADY:
      msg = "Operation already in progress";
      break;
    case ENOTSOCK:
      msg = "Socket operation on non-socket";
      break;
    case EDESTADDRREQ:
      msg = "Destination address required";
      break;
    case EMSGSIZE:
      msg = "Message too long";
      break;
    case EPROTOTYPE:
      msg = "Protocol wrong type for socket";
      break;
    case ENOPROTOOPT:
      msg = "Protocol not available";
      break;
    case EPROTONOSUPPORT:
      msg = "Protocol not supported";
      break;
    case ESOCKTNOSUPPORT:
      msg = "Socket type not supported";
      break;
    case EOPNOTSUPP:
      msg = "Operation not supported";
      break;
    case EPFNOSUPPORT:
      msg = "Protocol family not supported";
      break;
    case EAFNOSUPPORT:
      msg = "Address family not supported by protocol";
      break;
    case EADDRINUSE:
      msg = "Address already in use";
      break;
    case EADDRNOTAVAIL:
      msg = "Cannot assign requested address";
      break;
    case ENETDOWN:
      msg = "Network is down";
      break;
    case ENETUNREACH:
      msg = "Network is unreachable";
      break;
    case ENETRESET:
      msg = "Network dropped connection on reset";
      break;
    case ECONNABORTED:
      msg = "Software caused connection abort";
      break;
    case ECONNRESET:
      msg = "Connection reset by peer";
      break;
    case ENOBUFS:
      msg = "No buffer space available";
      break;
    case EISCONN:
      msg = "Transport endpoint is already connected";
      break;
    case ENOTCONN:
      msg = "Transport endpoint is not connected";
      break;
    case ESHUTDOWN:
      msg = "Cannot send after transport endpoint shutdown";
      break;
    case ETOOMANYREFS:
      msg = "Too many references: cannot splice";
      break;
    case ETIMEDOUT:
      msg = "Connection timed out";
      break;
    case ECONNREFUSED:
      msg = "Connection refused";
      break;
    case ELOOP:
      msg = "Too many levels of symbolic links";
      break;
    case EHOSTDOWN:
      msg = "Host is down";
      break;
    case EHOSTUNREACH:
      msg = "No route to host";
      break;
    case EPROCLIM:
      msg = "Too many processes";
      break;
    case EUSERS:
      msg = "Too many users";
      break;
    case EDQUOT:
      msg = "Disk quota exceeded";
      break;
    case ESTALE:
      msg = "Stale NFS file handle";
      break;
    case EREMOTE:
      msg = "Object is remote";
      break;
#  if HAVE_WINSOCK2_H
    /* WSA_INVALID_HANDLE maps to EBADF */
    /* WSA_NOT_ENOUGH_MEMORY maps to ENOMEM */
    /* WSA_INVALID_PARAMETER maps to EINVAL */
    case WSA_OPERATION_ABORTED:
      msg = "Overlapped operation aborted";
      break;
    case WSA_IO_INCOMPLETE:
      msg = "Overlapped I/O event object not in signaled state";
      break;
    case WSA_IO_PENDING:
      msg = "Overlapped operations will complete later";
      break;
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
      msg = "Network subsystem is unavailable";
      break;
    case WSAVERNOTSUPPORTED:
      msg = "Winsock.dll version out of range";
      break;
    case WSANOTINITIALISED:
      msg = "Successful WSAStartup not yet performed";
      break;
    case WSAEDISCON:
      msg = "Graceful shutdown in progress";
      break;
    case WSAENOMORE: case WSA_E_NO_MORE:
      msg = "No more results";
      break;
    case WSAECANCELLED: case WSA_E_CANCELLED:
      msg = "Call was canceled";
      break;
    case WSAEINVALIDPROCTABLE:
      msg = "Procedure call table is invalid";
      break;
    case WSAEINVALIDPROVIDER:
      msg = "Service provider is invalid";
      break;
    case WSAEPROVIDERFAILEDINIT:
      msg = "Service provider failed to initialize";
      break;
    case WSASYSCALLFAILURE:
      msg = "System call failure";
      break;
    case WSASERVICE_NOT_FOUND:
      msg = "Service not found";
      break;
    case WSATYPE_NOT_FOUND:
      msg = "Class type not found";
      break;
    case WSAEREFUSED:
      msg = "Database query was refused";
      break;
    case WSAHOST_NOT_FOUND:
      msg = "Host not found";
      break;
    case WSATRY_AGAIN:
      msg = "Nonauthoritative host not found";
      break;
    case WSANO_RECOVERY:
      msg = "Nonrecoverable error";
      break;
    case WSANO_DATA:
      msg = "Valid name, no data record of requested type";
      break;
    /* WSA_QOS_* omitted */
#  endif
# endif

# if GNULIB_defined_ENOMSG
    case ENOMSG:
      msg = "No message of desired type";
      break;
# endif

# if GNULIB_defined_EIDRM
    case EIDRM:
      msg = "Identifier removed";
      break;
# endif

# if GNULIB_defined_ENOLINK
    case ENOLINK:
      msg = "Link has been severed";
      break;
# endif

# if GNULIB_defined_EPROTO
    case EPROTO:
      msg = "Protocol error";
      break;
# endif

# if GNULIB_defined_EMULTIHOP
    case EMULTIHOP:
      msg = "Multihop attempted";
      break;
# endif

# if GNULIB_defined_EBADMSG
    case EBADMSG:
      msg = "Bad message";
      break;
# endif

# if GNULIB_defined_EOVERFLOW
    case EOVERFLOW:
      msg = "Value too large for defined data type";
      break;
# endif

# if GNULIB_defined_ENOTSUP
    case ENOTSUP:
      msg = "Not supported";
      break;
# endif

# if GNULIB_defined_ESTALE
    case ESTALE:
      msg = "Stale NFS file handle";
      break;
# endif

# if GNULIB_defined_ECANCELED
    case ECANCELED:
      msg = "Operation canceled";
      break;
# endif
    }

  if (msg)
    return (char *) msg;

  {
    char *result = strerror (n);

    if (result == NULL || result[0] == '\0')
      {
	static char const fmt[] = "Unknown error (%d)";
	static char msg_buf[sizeof fmt + INT_STRLEN_BOUND (n)];
	sprintf (msg_buf, fmt, n);
	return msg_buf;
      }

    return result;
  }
}

#endif
