#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <stdint.h>
#include <cassert>
#include <climits>
#include <cmath>

#ifdef __SSE2__
#include <emmintrin.h>
#endif

// use real compiler defines in here as we want to
// avoid including system.h or other magic includes.
// use 'gcc -dM -E - < /dev/null' or similar to find them.

#if defined(__ppc__) || \
    defined(__powerpc__) || \
    defined(__arm__)
  #define DISABLE_MATHUTILS_ASM_ROUND_INT
#endif

#if defined(__ppc__) || \
    defined(__powerpc__) || \
    defined(__arm__)
  #define DISABLE_MATHUTILS_ASM_TRUNCATE_INT
#endif

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
  inline int round_int(double x)
  {
#if 0
    assert(x > static_cast<double>(INT_MIN / 2) - 1.0);
    assert(x < static_cast<double>(INT_MAX / 2) + 1.0);
#endif

#if defined(DISABLE_MATHUTILS_ASM_ROUND_INT)
    return ((unsigned int) (x + 0x80000000.8p0)) - 0x80000000;

#else
    const float round_to_nearest = 0.5f;
    int i;
#if defined(__SSE2__)
    const float round_dn_to_nearest = 0.4999999f;
    i = (x > 0) ? _mm_cvttsd_si32(_mm_set_sd(x + round_to_nearest)) : _mm_cvttsd_si32(_mm_set_sd(x - round_dn_to_nearest));

#elif defined(TARGET_WINDOWS)
    __asm
    {
      fld x
      fadd st, st (0)
      fadd round_to_nearest
      fistp i
      sar i, 1
    }

#else
    __asm__ __volatile__ (
      "fadd %%st\n\t"
      "fadd %%st(1)\n\t"
      "fistpl %0\n\t"
      "sarl $1, %0\n"
      : "=m"(i) : "u"(round_to_nearest), "t"(x) : "st"
    );

#endif
    return i;
#endif
  }

  /*! \brief Truncate to nearest integer.
   This routine does fast truncation to an integer.
   It should simply drop the fractional portion of the floating point number.

   Make sure MathUtils::test() returns true for each implementation.
   \sa round_int, test
  */
  inline int truncate_int(double x)
  {
#if 0
    assert(x > static_cast<double>(INT_MIN / 2) - 1.0);
    assert(x < static_cast<double>(INT_MAX / 2) + 1.0);
#endif

#if defined(DISABLE_MATHUTILS_ASM_TRUNCATE_INT)
    return x;

#else
    int i;
#if defined(TARGET_WINDOWS)
    const float round_towards_m_i = -0.5f;
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
    const float round_towards_m_i = -0.5f;
    __asm__ __volatile__ (
      "fadd %%st\n\t"
      "fabs\n\t"
      "fadd %%st(1)\n\t"
      "fistpl %0\n\t"
      "sarl $1, %0\n"
      : "=m"(i) : "u"(round_towards_m_i), "t"(x) : "st"
    );
#endif
    if (x < 0)
      i = -i;
    return (i);
#endif
  }

  inline int64_t abs(int64_t a)
  {
    return (a < 0) ? -a : a;
  }

  inline unsigned bitcount(unsigned v)
  {
    unsigned c = 0;
    for (c = 0; v; c++)
      v &= v - 1; // clear the least significant bit set
    return c;
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

