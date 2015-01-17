/* A substitute for ISO C99 <wchar.h>, for platforms that have issues.

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

/* Written by Eric Blake.  */

/*
 * ISO C 99 <wchar.h> for platforms that have issues.
 * <http://www.opengroup.org/susv3xbd/wchar.h.html>
 *
 * For now, this just ensures proper prerequisite inclusion order and
 * the declaration of wcwidth().
 */

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif

#ifdef __need_mbstate_t
/* Special invocation convention inside uClibc header files.  */

#@INCLUDE_NEXT@ @NEXT_WCHAR_H@

#else
/* Normal invocation convention.  */

#ifndef _GL_WCHAR_H

/* Tru64 with Desktop Toolkit C has a bug: <stdio.h> must be included before
   <wchar.h>.
   BSD/OS 4.0.1 has a bug: <stddef.h>, <stdio.h> and <time.h> must be
   included before <wchar.h>.  */
#include <stddef.h>
#include <stdio.h>
#include <time.h>

/* Include the original <wchar.h> if it exists.
   Some builds of uClibc lack it.  */
/* The include_next requires a split double-inclusion guard.  */
#if @HAVE_WCHAR_H@
# @INCLUDE_NEXT@ @NEXT_WCHAR_H@
#endif

#ifndef _GL_WCHAR_H
#define _GL_WCHAR_H

/* The definition of GL_LINK_WARNING is copied here.  */

#ifdef __cplusplus
extern "C" {
#endif


/* Define wint_t.  (Also done in wctype.in.h.)  */
#if !@HAVE_WINT_T@ && !defined wint_t
# define wint_t int
#endif


/* Return the number of screen columns needed for WC.  */
#if @GNULIB_WCWIDTH@
# if @REPLACE_WCWIDTH@
#  undef wcwidth
#  define wcwidth rpl_wcwidth
extern int wcwidth (wchar_t);
# else
#  if !defined wcwidth && !@HAVE_DECL_WCWIDTH@
/* wcwidth exists but is not declared.  */
extern int wcwidth (int /* actually wchar_t */);
#  endif
# endif
#elif defined GNULIB_POSIXCHECK
# undef wcwidth
# define wcwidth(w) \
    (GL_LINK_WARNING ("wcwidth is unportable - " \
                      "use gnulib module wcwidth for portability"), \
     wcwidth (w))
#endif


#ifdef __cplusplus
}
#endif

#endif /* _GL_WCHAR_H */
#endif /* _GL_WCHAR_H */
#endif
