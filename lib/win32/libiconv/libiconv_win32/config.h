/* Copyright (C) 1999-2003 Free Software Foundation, Inc.
   This file is part of the GNU LIBICONV Library.

   The GNU LIBICONV Library is free software; you can redistribute it
   and/or modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   The GNU LIBICONV Library is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU LIBICONV Library; see the file COPYING.LIB.
   If not, write to the Free Software Foundation, Inc., 59 Temple Place -
   Suite 330, Boston, MA 02111-1307, USA.  */


/* Define to 1 to enable a few rarely used encodings. */
#undef ENABLE_EXTRA

/* Define to 1 if the package shall run at any location in the filesystem. */
#define ENABLE_RELOCATABLE 1

/* Define to a type if <wchar.h> does not define. */
#undef mbstate_t

/* Define if you have <iconv.h>, the iconv_t type, and the
   iconv_open, iconv, iconv_close functions. */
#undef HAVE_ICONV
/* Define as const if the declaration of iconv() needs const. */
#define ICONV_CONST const

/* Define to 1 if you have the getc_unlocked() function. */
#undef HAVE_GETC_UNLOCKED

/* Define if you have <langinfo.h> and nl_langinfo(CODESET). */
#undef HAVE_LANGINFO_CODESET

/* Define if you have the mbrtowc() function. */
#undef HAVE_MBRTOWC

/* Define to 1 if you have the setlocale() function. */
#define HAVE_SETLOCALE 1

/* Define to 1 if you have the <stddef.h> header file. */
#define HAVE_STDDEF_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define if you have the wcrtomb() function. */
#undef HAVE_WCRTOMB

/* Define if the machine's byte ordering is little endian. */
#define WORDS_LITTLEENDIAN 1

/* Define to the value of ${prefix}, as a string. */

#define LIBDIR 1

