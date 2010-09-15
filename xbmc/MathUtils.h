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

#include <cassert>
#include <climits>
#include <cmath>
#include <stdint.h>

#include "PlatformDefs.h"

#ifdef __SSE2__
#include <xmmintrin.h>
#endif

namespace MathUtils
{
  // GCC does something stupid with optimization on release builds if we try
  // to assert in these functions
  inline int round_int (double x)
  {
    if (x <= INT32_MIN) return INT32_MIN;
    if (x >= INT32_MAX) return INT32_MAX;

    const float round_value = 0.5f;

    #if defined(__SSE2__)
      return _mm_cvtsd_si32(_mm_set_sd(x));

    #elif !defined(_LINUX)
      int32_t i;
      __asm
      {
        fld x
        fadd st, st (0)
        fadd round_value
        fistp i
        sar i, 1
      }
      return i;

    #elif defined(__powerpc__) || defined(__ppc__) || defined(__arm__)
      return floor(x + round_value);

    #else
      int32_t i;
      __asm__ __volatile__ (
        "fadd %%st\n"
        "fadd %%st(1)\n"
        "fistpl %0\n"
        "sarl $1, %0\n"
        : "=m"(i) : "u"(round_value), "t"(x) : "st"
      );
      return i;

    #endif
  }

  inline int ceil_int (double x)
  {
    assert(x > static_cast<double>(INT_MIN / 2) - 1.0);
    assert(x < static_cast <double>(INT_MAX / 2) + 1.0);

    #if !defined(__powerpc__) && !defined(__ppc__) && !defined(__arm__)
        const float round_towards_p_i = -0.5f;
    #endif
    int i;

#ifndef _LINUX
    __asm
    {
      fld x
      fadd st, st (0)
      fsubr round_towards_p_i
      fistp i
      sar i, 1
    }
#else
    #if defined(__powerpc__) || defined(__ppc__) || defined(__arm__)
        return (int)ceil(x);
    #else
        __asm__ __volatile__ (
            "fadd %%st\n\t"
            "fsubr %%st(1)\n\t"
            "fistpl %0\n\t"
            "sarl $1, %0\n"
            : "=m"(i) : "u"(round_towards_p_i), "t"(x) : "st"
        );
    #endif
#endif
    return (-i);
  }

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

  inline void hack()
  {
    // stupid hack to keep compiler from dropping these
    // functions as unused
    MathUtils::round_int(0.0);
    MathUtils::truncate_int(0.0);
    MathUtils::ceil_int(0.0);
  }
} // namespace MathUtils

