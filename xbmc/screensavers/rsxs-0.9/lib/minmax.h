/* MIN, MAX macros.
   Copyright (C) 1995, 1998, 2001, 2003, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _MINMAX_H
#define _MINMAX_H

/* Note: MIN, MAX are also defined in <sys/param.h> on some systems
   (glibc, IRIX, HP-UX, OSF/1).  Therefore you might get warnings about
   MIN, MAX macro redefinitions on some systems; the workaround is to
   #include this file as the last one among the #include list.  */

/* Before we define the following symbols we get the <limits.h> file
   since otherwise we get redefinitions on some systems if <limits.h> is
   included after this file.  Likewise for <sys/param.h>.
   If more than one of these system headers define MIN and MAX, pick just
   one of the headers (because the definitions most likely are the same).  */
#if HAVE_MINMAX_IN_LIMITS_H
# include <limits.h>
#elif HAVE_MINMAX_IN_SYS_PARAM_H
# include <sys/param.h>
#endif

/* Note: MIN and MAX should be used with two arguments of the
   same type.  They might not return the minimum and maximum of their two
   arguments, if the arguments have different types or have unusual
   floating-point values.  For example, on a typical host with 32-bit 'int',
   64-bit 'long long', and 64-bit IEEE 754 'double' types:

     MAX (-1, 2147483648) returns 4294967295.
     MAX (9007199254740992.0, 9007199254740993) returns 9007199254740992.0.
     MAX (NaN, 0.0) returns 0.0.
     MAX (+0.0, -0.0) returns -0.0.

   and in each case the answer is in some sense bogus.  */

/* MAX(a,b) returns the maximum of A and B.  */
#ifndef MAX
# define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

/* MIN(a,b) returns the minimum of A and B.  */
#ifndef MIN
# define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#endif /* _MINMAX_H */
