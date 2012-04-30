#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <stdint.h>
#include <cassert>
#include <climits>
#include <cmath>

/*! \brief Math utility class.
 Note that the test() routine should return true for all implementations

 See http://ldesoras.free.fr/doc/articles/rounding_en.pdf for an explanation
 of the technique used on x86.
 */
namespace MathUtils
{
  // GCC does something stupid with optimization on release builds if we try
  // to assert in these functions

  /*! \brief Round to nearest integer.
   This routine does fast rounding to the nearest integer.
   In the case (k + 0.5 for any integer k) we round up to k+1, and in all other
   instances we should return the nearest integer.
   Thus, { -1.5, -0.5, 0.5, 1.5 } is rounded to { -1, 0, 1, 2 }.
   It preserves the property that round(k) - round(k-1) = 1 for all doubles k.

   Make sure MathUtils::test() returns true for each implementation.
   \sa truncate_int, test
   */
  inline int round_int (double x)
  {
    assert(x > static_cast<double>(INT_MIN / 2) - 1.0);
    assert(x < static_cast <double>(INT_MAX / 2) + 1.0);
    const float round_to_nearest = 0.5f;
    int i;
    
#ifndef _LINUX
    __asm
    {
      fld x
      fadd st, st (0)
      fadd round_to_nearest
      fistp i
      sar i, 1
    }
#else
#if defined(__powerpc__) || defined(__ppc__)
    i = floor(x + round_to_nearest);
#elif defined(__arm__)
    // From 'ARM®v7-M Architecture Reference Manual' page A7-569:
    //  "The floating-point to integer operation (vcvt) [normally] uses the Round towards Zero rounding mode"
    // Because of this...we must use some less-than-straightforward logic to perform this operation without
    //  changing the rounding mode flags

    /* The assembly below implements the following logic:
     if (x < 0)
       inc = -0.5f
     else
       inc = 0.5f
     int_val = trunc(x+inc);
     err = x - int_val;
     if (err == 0.5f)
       int_val++;
     return int_val;
     */

    __asm__ __volatile__ (
                          "fconstd d1,#%G[rnd_val]     \n\t" // Copy round_to_nearest into a working register (d1 = 0.5)
                          "fcmpezd %P[value]           \n\t" // Check value against zero (value == 0?)
                          "fmstat                      \n\t" // Copy the floating-point status flags into the general-purpose status flags
                          "it mi                       \n\t"
                          "vnegmi.F64 d1, d1           \n\t" // if N-flag is set, negate round_to_nearest (if (value < 0) d1 = -1 * d1)
                          "vadd.F64 d1,%P[value],d1    \n\t" // Add round_to_nearest to value, store result in working register (d1 += value)
                          "vcvt.S32.F64 s3,d1          \n\t" // Truncate(round towards zero) (s3 = (int)d1)
                          "vmov %[result],s3           \n\t" // Store the integer result in a general-purpose register (result = s3)
                          "vcvt.F64.S32 d1,s3          \n\t" // Convert back to floating-point (d1 = (double)s3)
                          "vsub.F64 d1,%P[value],d1    \n\t" // Calculate the error (d1 = value - d1)
                          "fconstd d2,#%G[rnd_val]     \n\t" // d2 = 0.5;
                          : [result] "=r"(i)                                  // Outputs
                          : [rnd_val] "Dv" (round_to_nearest), [value] "w"(x) // Inputs
                          : "d1", "d2", "s3"                                  // Clobbers
                          );
#else
    __asm__ __volatile__ (
                          "fadd %%st\n\t"
                          "fadd %%st(1)\n\t"
                          "fistpl %0\n\t"
                          "sarl $1, %0\n"
                          : "=m"(i) : "u"(round_to_nearest), "t"(x) : "st"
                          );
#endif
#endif
    return (i);
  }

  /*! \brief Truncate to nearest integer.
   This routine does fast truncation to an integer.
   It should simply drop the fractional portion of the floating point number.

   Make sure MathUtils::test() returns true for each implementation.
   \sa round_int, test
   */
  inline int truncate_int(double x)
  {
    assert(x > static_cast<double>(INT_MIN / 2) - 1.0);
    assert(x < static_cast <double>(INT_MAX / 2) + 1.0);

#if !defined(__powerpc__) && !defined(__ppc__) && !defined(__arm__)
    const float round_towards_m_i = -0.5f;
#endif
    int i;

#ifndef _LINUX
    __asm
    {
      fld x
      fadd st, st (0)
      fabs
      fadd round_towards_m_i
      fistp i
      sar i, 1
    }
#else
#if defined(__powerpc__) || defined(__ppc__)
    return (int)x;
#elif defined(__arm__)
    __asm__ __volatile__ (
                          "vcvt.S32.F64 %[result],%P[value]   \n\t" // Truncate(round towards zero) and store the result
                          : [result] "=w"(i)                        // Outputs
                          : [value] "w"(x)                          // Inputs
                          );
    return i;
#else
    __asm__ __volatile__ (
                          "fadd %%st\n\t"
                          "fabs\n\t"
                          "fadd %%st(1)\n\t"
                          "fistpl %0\n\t"
                          "sarl $1, %0\n"
                          : "=m"(i) : "u"(round_towards_m_i), "t"(x) : "st"
                          );
#endif
#endif
    if (x < 0)
      i = -i;
    return (i);
  }

  inline int64_t abs(int64_t a)
  {
    return (a < 0) ? -a : a;
  }

  inline void hack()
  {
    // stupid hack to keep compiler from dropping these
    // functions as unused
    MathUtils::round_int(0.0);
    MathUtils::truncate_int(0.0);
    MathUtils::abs(0);
  }

#if 0
  /*! \brief test routine for round_int and truncate_int
   Must return true on all platforms.
   */
  inline bool test()
  {
    for (int i = -8; i < 8; ++i)
    {
      double d = 0.25*i;
      int r = (i < 0) ? (i - 1) / 4 : (i + 2) / 4;
      int t = i / 4;
      if (round_int(d) != r || truncate_int(d) != t)
        return false;
    }
    return true;
  }
#endif
} // namespace MathUtils

