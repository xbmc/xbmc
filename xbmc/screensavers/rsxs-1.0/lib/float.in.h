/* A correct <float.h>.

   Copyright (C) 2007-2008 Free Software Foundation, Inc.

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

#ifndef _GL_FLOAT_H

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif

/* The include_next requires a split double-inclusion guard.  */
#@INCLUDE_NEXT@ @NEXT_FLOAT_H@

#ifndef _GL_FLOAT_H
#define _GL_FLOAT_H

/* 'long double' properties.  */
#if defined __i386__ && (defined __BEOS__ || defined __OpenBSD__)
/* Number of mantissa units, in base FLT_RADIX.  */
# undef LDBL_MANT_DIG
# define LDBL_MANT_DIG   64
/* Number of decimal digits that is sufficient for representing a number.  */
# undef LDBL_DIG
# define LDBL_DIG        18
/* x-1 where x is the smallest representable number > 1.  */
# undef LDBL_EPSILON
# define LDBL_EPSILON    1.0842021724855044340E-19L
/* Minimum e such that FLT_RADIX^(e-1) is a normalized number.  */
# undef LDBL_MIN_EXP
# define LDBL_MIN_EXP    (-16381)
/* Maximum e such that FLT_RADIX^(e-1) is a representable finite number.  */
# undef LDBL_MAX_EXP
# define LDBL_MAX_EXP    16384
/* Minimum positive normalized number.  */
# undef LDBL_MIN
# define LDBL_MIN        3.3621031431120935063E-4932L
/* Maximum representable finite number.  */
# undef LDBL_MAX
# define LDBL_MAX        1.1897314953572317650E+4932L
/* Minimum e such that 10^e is in the range of normalized numbers.  */
# undef LDBL_MIN_10_EXP
# define LDBL_MIN_10_EXP (-4931)
/* Maximum e such that 10^e is in the range of representable finite numbers.  */
# undef LDBL_MAX_10_EXP
# define LDBL_MAX_10_EXP 4932
#endif

#endif /* _GL_FLOAT_H */
#endif /* _GL_FLOAT_H */
