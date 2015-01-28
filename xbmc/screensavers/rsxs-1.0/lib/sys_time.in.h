/* Provide a more complete sys/time.h.

   Copyright (C) 2007-2008 Free Software Foundation, Inc.

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

/* Written by Paul Eggert.  */

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif

#if defined _GL_SYS_TIME_H

/* Simply delegate to the system's header, without adding anything.  */
# if @HAVE_SYS_TIME_H@
#  @INCLUDE_NEXT@ @NEXT_SYS_TIME_H@
# endif

#else

# define _GL_SYS_TIME_H

# if @HAVE_SYS_TIME_H@
#  @INCLUDE_NEXT@ @NEXT_SYS_TIME_H@
# else
#  include <time.h>
# endif

#ifdef __cplusplus
extern "C" {
#endif

# if ! @HAVE_STRUCT_TIMEVAL@
struct timeval
{
  time_t tv_sec;
  long int tv_usec;
};
# endif

# if @REPLACE_GETTIMEOFDAY@
#  undef gettimeofday
#  define gettimeofday rpl_gettimeofday
int gettimeofday (struct timeval *restrict, void *restrict);
# endif

#ifdef __cplusplus
}
#endif

#endif /* _GL_SYS_TIME_H */
