/* A POSIX-like <errno.h>.

   Copyright (C) 2008 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _GL_ERRNO_H

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif

/* The include_next requires a split double-inclusion guard.  */
#@INCLUDE_NEXT@ @NEXT_ERRNO_H@

#ifndef _GL_ERRNO_H
#define _GL_ERRNO_H


/* On native Windows platforms, many macros are not defined.  */
# if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__

/* POSIX says that EAGAIN and EWOULDBLOCK may have the same value.  */
#  define EWOULDBLOCK     EAGAIN

/* Values >= 100 seem safe to use.  */
#  define ETXTBSY   100
#  define GNULIB_defined_ETXTBSY 1

/* These are intentionally the same values as the WSA* error numbers, defined
   in <winsock2.h>.  */
#  define EINPROGRESS     10036
#  define EALREADY        10037
#  define ENOTSOCK        10038
#  define EDESTADDRREQ    10039
#  define EMSGSIZE        10040
#  define EPROTOTYPE      10041
#  define ENOPROTOOPT     10042
#  define EPROTONOSUPPORT 10043
#  define ESOCKTNOSUPPORT 10044  /* not required by POSIX */
#  define EOPNOTSUPP      10045
#  define EPFNOSUPPORT    10046  /* not required by POSIX */
#  define EAFNOSUPPORT    10047
#  define EADDRINUSE      10048
#  define EADDRNOTAVAIL   10049
#  define ENETDOWN        10050
#  define ENETUNREACH     10051
#  define ENETRESET       10052
#  define ECONNABORTED    10053
#  define ECONNRESET      10054
#  define ENOBUFS         10055
#  define EISCONN         10056
#  define ENOTCONN        10057
#  define ESHUTDOWN       10058  /* not required by POSIX */
#  define ETOOMANYREFS    10059  /* not required by POSIX */
#  define ETIMEDOUT       10060
#  define ECONNREFUSED    10061
#  define ELOOP           10062
#  define EHOSTDOWN       10064  /* not required by POSIX */
#  define EHOSTUNREACH    10065
#  define EPROCLIM        10067  /* not required by POSIX */
#  define EUSERS          10068  /* not required by POSIX */
#  define EDQUOT          10069
#  define ESTALE          10070
#  define EREMOTE         10071  /* not required by POSIX */
#  define GNULIB_defined_ESOCK 1

# endif


/* On OSF/1 5.1, when _XOPEN_SOURCE_EXTENDED is not defined, the macros
   EMULTIHOP, ENOLINK, EOVERFLOW are not defined.  */
# if @EMULTIHOP_HIDDEN@
#  define EMULTIHOP @EMULTIHOP_VALUE@
#  define GNULIB_defined_EMULTIHOP 1
# endif
# if @ENOLINK_HIDDEN@
#  define ENOLINK   @ENOLINK_VALUE@
#  define GNULIB_defined_ENOLINK 1
# endif
# if @EOVERFLOW_HIDDEN@
#  define EOVERFLOW @EOVERFLOW_VALUE@
#  define GNULIB_defined_EOVERFLOW 1
# endif


/* On OpenBSD 4.0 and on native Windows, the macros ENOMSG, EIDRM, ENOLINK,
   EPROTO, EMULTIHOP, EBADMSG, EOVERFLOW, ENOTSUP, ECANCELED are not defined.
   Define them here.  Values >= 2000 seem safe to use: Solaris ESTALE = 151,
   HP-UX EWOULDBLOCK = 246, IRIX EDQUOT = 1133.

   Note: When one of these systems defines some of these macros some day,
   binaries will have to be recompiled so that they recognizes the new
   errno values from the system.  */

# ifndef ENOMSG
#  define ENOMSG    2000
#  define GNULIB_defined_ENOMSG 1
# endif

# ifndef EIDRM
#  define EIDRM     2001
#  define GNULIB_defined_EIDRM 1
# endif

# ifndef ENOLINK
#  define ENOLINK   2002
#  define GNULIB_defined_ENOLINK 1
# endif

# ifndef EPROTO
#  define EPROTO    2003
#  define GNULIB_defined_EPROTO 1
# endif

# ifndef EMULTIHOP
#  define EMULTIHOP 2004
#  define GNULIB_defined_EMULTIHOP 1
# endif

# ifndef EBADMSG
#  define EBADMSG   2005
#  define GNULIB_defined_EBADMSG 1
# endif

# ifndef EOVERFLOW
#  define EOVERFLOW 2006
#  define GNULIB_defined_EOVERFLOW 1
# endif

# ifndef ENOTSUP
#  define ENOTSUP   2007
#  define GNULIB_defined_ENOTSUP 1
# endif

# ifndef ECANCELED
#  define ECANCELED 2008
#  define GNULIB_defined_ECANCELED 1
# endif


#endif /* _GL_ERRNO_H */
#endif /* _GL_ERRNO_H */
