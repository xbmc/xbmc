/* Provide a more complete sys/types.h.

   Copyright (C) 2011 Free Software Foundation, Inc.

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

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif
@PRAGMA_COLUMNS@

#ifndef _@GUARD_PREFIX@_SYS_TYPES_H

/* The include_next requires a split double-inclusion guard.  */
#@INCLUDE_NEXT@ @NEXT_SYS_TYPES_H@

#ifndef _@GUARD_PREFIX@_SYS_TYPES_H
#define _@GUARD_PREFIX@_SYS_TYPES_H

/* MSVC 9 defines size_t in <stddef.h>, not in <sys/types.h>.  */
/* But avoid namespace pollution on glibc systems.  */
#if ((defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__) \
    && ! defined __GLIBC__
# include <stddef.h>
#endif

#endif /* _@GUARD_PREFIX@_SYS_TYPES_H */
#endif /* _@GUARD_PREFIX@_SYS_TYPES_H */
