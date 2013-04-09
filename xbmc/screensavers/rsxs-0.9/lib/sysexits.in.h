/* exit() exit codes for some BSD system programs.
   Copyright (C) 2003, 2006-2011 Free Software Foundation, Inc.

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

/* Written by Simon Josefsson based on sysexits(3) man page */

#ifndef _@GUARD_PREFIX@_SYSEXITS_H

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif
@PRAGMA_COLUMNS@

#if @HAVE_SYSEXITS_H@

/* IRIX 6.5 has an <unistd.h> that defines a macro EX_OK with a nonzero
   value.  Override it.  See
   <http://lists.gnu.org/archive/html/bug-gnulib/2007-03/msg00361.html>  */
# ifdef __sgi
#  include <unistd.h>
#  undef EX_OK
# endif

/* The include_next requires a split double-inclusion guard.  */
# @INCLUDE_NEXT@ @NEXT_SYSEXITS_H@

/* HP-UX 11 <sysexits.h> ends at EX_NOPERM.  */
# ifndef EX_CONFIG
#  define EX_CONFIG 78
# endif

#endif

#ifndef _@GUARD_PREFIX@_SYSEXITS_H
#define _@GUARD_PREFIX@_SYSEXITS_H

#if !@HAVE_SYSEXITS_H@

# define EX_OK 0 /* same value as EXIT_SUCCESS */

# define EX_USAGE 64
# define EX_DATAERR 65
# define EX_NOINPUT 66
# define EX_NOUSER 67
# define EX_NOHOST 68
# define EX_UNAVAILABLE 69
# define EX_SOFTWARE 70
# define EX_OSERR 71
# define EX_OSFILE 72
# define EX_CANTCREAT 73
# define EX_IOERR 74
# define EX_TEMPFAIL 75
# define EX_PROTOCOL 76
# define EX_NOPERM 77
# define EX_CONFIG 78

#endif

#endif /* _@GUARD_PREFIX@_SYSEXITS_H */
#endif /* _@GUARD_PREFIX@_SYSEXITS_H */
