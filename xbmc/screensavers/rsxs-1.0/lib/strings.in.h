/* A substitute <strings.h>.

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

#ifndef _GL_STRINGS_H

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif

/* The include_next requires a split double-inclusion guard.  */
#@INCLUDE_NEXT@ @NEXT_STRINGS_H@

#ifndef _GL_STRINGS_H
#define _GL_STRINGS_H


/* The definition of GL_LINK_WARNING is copied here.  */


#ifdef __cplusplus
extern "C" {
#endif


/* Compare strings S1 and S2, ignoring case, returning less than, equal to or
   greater than zero if S1 is lexicographically less than, equal to or greater
   than S2.
   Note: This function does not work in multibyte locales.  */
#if ! @HAVE_STRCASECMP@
extern int strcasecmp (char const *s1, char const *s2);
#endif
#if defined GNULIB_POSIXCHECK
/* strcasecmp() does not work with multibyte strings:
   POSIX says that it operates on "strings", and "string" in POSIX is defined
   as a sequence of bytes, not of characters.   */
# undef strcasecmp
# define strcasecmp(a,b) \
    (GL_LINK_WARNING ("strcasecmp cannot work correctly on character strings " \
                      "in multibyte locales - " \
                      "use mbscasecmp if you care about " \
                      "internationalization, or use c_strcasecmp (from " \
                      "gnulib module c-strcase) if you want a locale " \
                      "independent function"), \
     strcasecmp (a, b))
#endif

/* Compare no more than N bytes of strings S1 and S2, ignoring case,
   returning less than, equal to or greater than zero if S1 is
   lexicographically less than, equal to or greater than S2.
   Note: This function cannot work correctly in multibyte locales.  */
#if ! @HAVE_DECL_STRNCASECMP@
extern int strncasecmp (char const *s1, char const *s2, size_t n);
#endif
#if defined GNULIB_POSIXCHECK
/* strncasecmp() does not work with multibyte strings:
   POSIX says that it operates on "strings", and "string" in POSIX is defined
   as a sequence of bytes, not of characters.  */
# undef strncasecmp
# define strncasecmp(a,b,n) \
    (GL_LINK_WARNING ("strncasecmp cannot work correctly on character " \
                      "strings in multibyte locales - " \
                      "use mbsncasecmp or mbspcasecmp if you care about " \
                      "internationalization, or use c_strncasecmp (from " \
                      "gnulib module c-strcase) if you want a locale " \
                      "independent function"), \
     strncasecmp (a, b, n))
#endif


#ifdef __cplusplus
}
#endif

#endif /* _GL_STRING_H */
#endif /* _GL_STRING_H */
