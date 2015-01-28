/* Substitute for <sys/select.h>.
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

#ifndef _GL_SYS_SELECT_H

#if @HAVE_SYS_SELECT_H@

# if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
# endif

/* On many platforms, <sys/select.h> assumes prior inclusion of
   <sys/types.h>.  */
# include <sys/types.h>

/* On OSF/1 4.0, <sys/select.h> provides only a forward declaration
   of 'struct timeval', and no definition of this type..  */
# include <sys/time.h>

/* The include_next requires a split double-inclusion guard.  */
# @INCLUDE_NEXT@ @NEXT_SYS_SELECT_H@

#endif

#ifndef _GL_SYS_SELECT_H
#define _GL_SYS_SELECT_H

#if !@HAVE_SYS_SELECT_H@

/* A platform that lacks <sys/select.h>.  */

# include <sys/socket.h>

/* The definition of GL_LINK_WARNING is copied here.  */

# ifdef __cplusplus
extern "C" {
# endif

# if @GNULIB_SELECT@
#  if @HAVE_WINSOCK2_H@
#   undef select
#   define select rpl_select
extern int rpl_select (int, fd_set *, fd_set *, fd_set *, struct timeval *);
#  endif
# elif @HAVE_WINSOCK2_H@
#  undef select
#  define select select_used_without_requesting_gnulib_module_select
# elif defined GNULIB_POSIXCHECK
#  undef select
#  define select(n,r,w,e,t) \
     (GL_LINK_WARNING ("select is not always POSIX compliant - " \
                       "use gnulib module select for portability"), \
      select (n, r, w, e, t))
# endif

# ifdef __cplusplus
}
# endif

#endif

#endif /* _GL_SYS_SELECT_H */
#endif /* _GL_SYS_SELECT_H */
