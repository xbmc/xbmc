/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <assert.h>
#include <climits>
#include <cmath>
#include <stdint.h>
#include <type_traits>

#if defined(HAVE_SSE2) && defined(__SSE2__)
#include <emmintrin.h>
#endif

// use real compiler defines in here as we want to
// avoid including system.h or other magic includes.
// use 'gcc -dM -E - < /dev/null' or similar to find them.

// clang-format off
#if defined(__aarch64__) || \
    defined(__alpha__) || \
    defined(__arc__) || \
    defined(__arm__) || \
    defined(__loongarch__) || \
    defined(_M_ARM) || \
    defined(__mips__) || \
    defined(__or1k__) || \
    defined(__powerpc__) || \
    defined(__ppc__) || \
    defined(__riscv) || \
    defined(__SH4__) || \
    defined(__s390x__) || \
    defined(__sparc__) || \
    defined(__xtensa__)
#define DISABLE_MATHUTILS_ASM_ROUND_INT
#endif
// clang-format on

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
    assert(x > static_cast<double>((int) (INT_MIN / 2)) - 1.0);
    assert(x < static_cast<double>((int) (INT_MAX / 2)) + 1.0);

#if defined(DISABLE_MATHUTILS_ASM_ROUND_INT)
    /* This implementation warrants some further explanation.
     *
     * First, a couple of notes on rounding:
     * 1) C casts from float/double to integer round towards zero.
     * 2) Float/double additions are rounded according to the normal rules,
     *    in other words: on some architectures, it's fixed at compile-time,
     *    and on others it can be set using fesetround()). The following
     *    analysis assumes round-to-nearest with ties rounding to even. This
     *    is a fairly sensible choice, and is the default with ARM VFP.
     *
     * What this function wants is round-to-nearest with ties rounding to
     * +infinity. This isn't an IEEE rounding mode, even if we could guarantee
     * that all architectures supported fesetround(), which they don't. Instead,
     * this adds an offset of 2147483648.5 (= 0x80000000.8p0), then casts to
     * an unsigned int (crucially, all possible inputs are now in a range where
     * round to zero acts the same as round to -infinity) and then subtracts
     * 0x80000000 in the integer domain. The 0.5 component of the offset
     * converts what is effectively a round down into a round to nearest, with
     * ties rounding up, as desired.
     *
     * There is a catch, that because there is a double rounding, there is a
     * small region where the input falls just *below* a tie, where the addition
     * of the offset causes a round *up* to an exact integer, due to the finite
     * level of precision available in floating point. You need to be aware of
     * this when calling this function, although at present it is not believed
     * that XBMC ever attempts to round numbers in this window.
     *
     * It is worth proving the size of the affected window. Recall that double
     * precision employs a mantissa of 52 bits.
     * 1) For all inputs -0.5 <= x <= INT_MAX
     *    Once the offset is applied, the most significant binary digit in the
     *    floating-point representation is +2^31.
     *    At this magnitude, the smallest step representable in double precision
     *    is 2^31 / 2^52 = 0.000000476837158203125
     *    So the size of the range which is rounded up due to the addition is
     *    half the size of this step, or 0.0000002384185791015625
     *
     * 2) For all inputs INT_MIN/2 < x < -0.5
     *    Once the offset is applied, the most significant binary digit in the
     *    floating-point representation is +2^30.
     *    At this magnitude, the smallest step representable in double precision
     *    is 2^30 / 2^52 = 0.0000002384185791015625
     *    So the size of the range which is rounded up due to the addition is
     *    half the size of this step, or 0.00000011920928955078125
     *
     * 3) For all inputs INT_MIN <= x <= INT_MIN/2
     *    The representation once the offset is applied has equal or greater
     *    precision than the input, so the addition does not cause rounding.
     */
    return ((unsigned int) (x + 2147483648.5)) - 0x80000000;

#else
    const float round_to_nearest = 0.5f;
    int i;
#if defined(HAVE_SSE2) && defined(__SSE2__)
    const float round_dn_to_nearest = 0.4999999f;
    i = (x > 0) ? _mm_cvttsd_si32(_mm_set_sd(x + static_cast<double>(round_to_nearest)))
                : _mm_cvttsd_si32(_mm_set_sd(x - static_cast<double>(round_dn_to_nearest)));

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
    assert(x > static_cast<double>(INT_MIN / 2) - 1.0);
    assert(x < static_cast<double>(INT_MAX / 2) + 1.0);
    return static_cast<int>(x);
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

  /**
   * Compare two floating-point numbers for equality and regard them
   * as equal if their difference is below a given threshold.
   *
   * It is usually not useful to compare float numbers for equality with
   * the standard operator== since very close numbers might have different
   * representations.
   */
  template<typename FloatT>
  inline bool FloatEquals(FloatT f1, FloatT f2, FloatT maxDelta)
  {
    return (std::abs(f2 - f1) < maxDelta);
  }

  /*!
   * \brief Round a floating point number to nearest multiple
   * \param value The value to round
   * \param multiple The multiple
   * \return The rounded value
   */
  template<typename T, std::enable_if_t<std::is_floating_point<T>::value, bool> = true>
  inline T RoundF(const T value, const T multiple)
  {
    if (multiple == 0)
      return value;

    return static_cast<T>(std::round(static_cast<double>(value) / static_cast<double>(multiple)) *
                          static_cast<double>(multiple));
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

