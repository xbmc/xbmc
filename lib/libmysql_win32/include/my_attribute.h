/* Copyright (C) 2000-2003 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/*
  Helper macros used for setting different __attributes__
  on functions in a portable fashion
*/

#ifndef _my_attribute_h
#define _my_attribute_h

/*
  Disable __attribute__() on gcc < 2.7, g++ < 3.4, and non-gcc compilers.
  Some forms of __attribute__ are actually supported in earlier versions of
  g++, but we just disable them all because we only use them to generate
  compilation warnings.
*/
#ifndef __attribute__
# if !defined(__GNUC__)
#  define __attribute__(A)
# elif GCC_VERSION < 2008
#  define __attribute__(A)
# elif defined(__cplusplus) && GCC_VERSION < 3004
#  define __attribute__(A)
# endif
#endif

/*
  __attribute__((format(...))) is only supported in gcc >= 2.8 and g++ >= 3.4
  But that's already covered by the __attribute__ tests above, so this is
  just a convenience macro.
*/
#ifndef ATTRIBUTE_FORMAT
# define ATTRIBUTE_FORMAT(style, m, n) __attribute__((format(style, m, n)))
#endif

/*

   __attribute__((format(...))) on a function pointer is not supported
   until  gcc 3.1
*/
#ifndef ATTRIBUTE_FORMAT_FPTR
# if (GCC_VERSION >= 3001)
#  define ATTRIBUTE_FORMAT_FPTR(style, m, n) ATTRIBUTE_FORMAT(style, m, n)
# else
#  define ATTRIBUTE_FORMAT_FPTR(style, m, n)
# endif /* GNUC >= 3.1 */
#endif


#endif
