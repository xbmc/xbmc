/* A substitute <strings.h>.

   Copyright (C) 2007-2011 Free Software Foundation, Inc.

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

#ifndef _@GUARD_PREFIX@_STRINGS_H

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif
@PRAGMA_COLUMNS@

/* Minix 3.1.8 has a bug: <sys/types.h> must be included before <strings.h>.
   But avoid namespace pollution on glibc systems.  */
#if defined __minix && !defined __GLIBC__
# include <sys/types.h>
#endif

/* The include_next requires a split double-inclusion guard.  */
#if @HAVE_STRINGS_H@
# @INCLUDE_NEXT@ @NEXT_STRINGS_H@
#endif

#ifndef _@GUARD_PREFIX@_STRINGS_H
#define _@GUARD_PREFIX@_STRINGS_H

#if ! @HAVE_DECL_STRNCASECMP@
/* Get size_t.  */
# include <stddef.h>
#endif


/* The definitions of _GL_FUNCDECL_RPL etc. are copied here.  */

/* The definition of _GL_ARG_NONNULL is copied here.  */

/* The definition of _GL_WARN_ON_USE is copied here.  */

#ifdef __cplusplus
extern "C" {
#endif


  /* Find the index of the least-significant set bit.  */
#if @GNULIB_FFS@
# if !@HAVE_FFS@
_GL_FUNCDECL_SYS (ffs, int, (int i));
# endif
_GL_CXXALIAS_SYS (ffs, int, (int i));
_GL_CXXALIASWARN (ffs);
#elif defined GNULIB_POSIXCHECK
# undef ffs
# if HAVE_RAW_DECL_FFS
_GL_WARN_ON_USE (ffs, "ffs is not portable - use the ffs module");
# endif
#endif

/* Compare strings S1 and S2, ignoring case, returning less than, equal to or
   greater than zero if S1 is lexicographically less than, equal to or greater
   than S2.
   Note: This function does not work in multibyte locales.  */
#if ! @HAVE_STRCASECMP@
extern int strcasecmp (char const *s1, char const *s2)
     _GL_ARG_NONNULL ((1, 2));
#endif
#if defined GNULIB_POSIXCHECK
/* strcasecmp() does not work with multibyte strings:
   POSIX says that it operates on "strings", and "string" in POSIX is defined
   as a sequence of bytes, not of characters.   */
# undef strcasecmp
# if HAVE_RAW_DECL_STRCASECMP
_GL_WARN_ON_USE (strcasecmp, "strcasecmp cannot work correctly on character "
                 "strings in multibyte locales - "
                 "use mbscasecmp if you care about "
                 "internationalization, or use c_strcasecmp , "
                 "gnulib module c-strcase) if you want a locale "
                 "independent function");
# endif
#endif

/* Compare no more than N bytes of strings S1 and S2, ignoring case,
   returning less than, equal to or greater than zero if S1 is
   lexicographically less than, equal to or greater than S2.
   Note: This function cannot work correctly in multibyte locales.  */
#if ! @HAVE_DECL_STRNCASECMP@
extern int strncasecmp (char const *s1, char const *s2, size_t n)
     _GL_ARG_NONNULL ((1, 2));
#endif
#if defined GNULIB_POSIXCHECK
/* strncasecmp() does not work with multibyte strings:
   POSIX says that it operates on "strings", and "string" in POSIX is defined
   as a sequence of bytes, not of characters.  */
# undef strncasecmp
# if HAVE_RAW_DECL_STRNCASECMP
_GL_WARN_ON_USE (strncasecmp, "strncasecmp cannot work correctly on character "
                 "strings in multibyte locales - "
                 "use mbsncasecmp or mbspcasecmp if you care about "
                 "internationalization, or use c_strncasecmp , "
                 "gnulib module c-strcase) if you want a locale "
                 "independent function");
# endif
#endif


#ifdef __cplusplus
}
#endif

#endif /* _@GUARD_PREFIX@_STRING_H */
#endif /* _@GUARD_PREFIX@_STRING_H */
