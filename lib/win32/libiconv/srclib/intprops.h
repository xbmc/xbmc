/* intprops.h -- properties of integer types

   Copyright (C) 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

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

/* Written by Paul Eggert.  */

#include <limits.h>

/* The extra casts in the following macros work around compiler bugs,
   e.g., in Cray C 5.0.3.0.  */

/* True if the arithmetic type T is an integer type.  bool counts as
   an integer.  */
#define TYPE_IS_INTEGER(t) ((t) 1.5 == 1)

/* True if negative values of the signed integer type T use two's
   complement, ones' complement, or signed magnitude representation,
   respectively.  Much GNU code assumes two's complement, but some
   people like to be portable to all possible C hosts.  */
#define TYPE_TWOS_COMPLEMENT(t) ((t) ~ (t) 0 == (t) -1)
#define TYPE_ONES_COMPLEMENT(t) ((t) ~ (t) 0 == 0)
#define TYPE_SIGNED_MAGNITUDE(t) ((t) ~ (t) 0 < (t) -1)

/* True if the arithmetic type T is signed.  */
#define TYPE_SIGNED(t) (! ((t) 0 < (t) -1))

/* The maximum and minimum values for the integer type T.  These
   macros have undefined behavior if T is signed and has padding bits.
   If this is a problem for you, please let us know how to fix it for
   your host.  */
#define TYPE_MINIMUM(t) \
  ((t) (! TYPE_SIGNED (t) \
	? (t) 0 \
	: TYPE_SIGNED_MAGNITUDE (t) \
	? ~ (t) 0 \
	: ~ (t) 0 << (sizeof (t) * CHAR_BIT - 1)))
#define TYPE_MAXIMUM(t) \
  ((t) (! TYPE_SIGNED (t) \
	? (t) -1 \
	: ~ (~ (t) 0 << (sizeof (t) * CHAR_BIT - 1))))

/* Return zero if T can be determined to be an unsigned type.
   Otherwise, return 1.
   When compiling with GCC, INT_STRLEN_BOUND uses this macro to obtain a
   tighter bound.  Otherwise, it overestimates the true bound by one byte
   when applied to unsigned types of size 2, 4, 16, ... bytes.
   The symbol signed_type_or_expr__ is private to this header file.  */
#if __GNUC__ >= 2
# define signed_type_or_expr__(t) TYPE_SIGNED (__typeof__ (t))
#else
# define signed_type_or_expr__(t) 1
#endif

/* Bound on length of the string representing an integer type or expression T.
   Subtract 1 for the sign bit if T is signed; log10 (2.0) < 146/485;
   add 1 for integer division truncation; add 1 more for a minus sign
   if needed.  */
#define INT_STRLEN_BOUND(t) \
  ((sizeof (t) * CHAR_BIT - signed_type_or_expr__ (t)) * 146 / 485 \
   + signed_type_or_expr__ (t) + 1)

/* Bound on buffer size needed to represent an integer type or expression T,
   including the terminating null.  */
#define INT_BUFSIZE_BOUND(t) (INT_STRLEN_BOUND (t) + 1)
