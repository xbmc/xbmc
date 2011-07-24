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
    //BIG FIXME here (still has issues with rounding -0.5 to zero and not -1)
    //the asm codes below do the following - trunc(x+0.5)
    //this isn't correct for negativ x - values - for example 
    //-1 gets rounded to zero because trunc(-1+0.5) == 0
    //this is a dirty hack until someone fixes this propably in asm
    //i've created a trac ticket for this #11767
    //this hacks decrements the x by 1 if it is negativ
    // so for -1 it would be trunc(-2+0.5) - which would be correct -1 then ...
    x = x < 0 ? x-1 : x;

    __asm__ __volatile__ (
                          "vmov.F64 d1,%[rnd_val]             \n\t" // Copy round_to_nearest into a working register
                          "vadd.F64 %P[value],%P[value],d1    \n\t" // Add round_to_nearest to value
                          "vcvt.S32.F64 %[result],%P[value]   \n\t" // Truncate(round towards zero) and store the result
                          : [result] "=w"(i), [value] "+w"(x)  // Outputs
                          : [rnd_val] "Dv" (round_to_nearest)  // Inputs
                          : "d1");                             // Clobbers
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
#if defined(__powerpc__) || defined(__ppc__) || defined(__arm__)
    return (int)x;
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

