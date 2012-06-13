#pragma once
/*
 *      Copyright (C) 2010-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "../AEAudioFormat.h"
#include "utils/StdString.h"
#include "PlatformDefs.h"
#include <math.h>

#ifdef __SSE__
#include <xmmintrin.h>
#else
#define __m128 void
#endif

#ifdef __GNUC__
  #define MEMALIGN(b, x) x __attribute__((aligned(b)))
#else
  #define MEMALIGN(b, x) __declspec(align(b)) x
#endif

class CAEUtil
{
private:
  static unsigned int m_seed;
  #ifdef __SSE__
    static __m128i m_sseSeed;
  #endif

  static float SoftClamp(const float x);

public:
  static CAEChannelInfo          GuessChLayout     (const unsigned int channels);
  static const char*             GetStdChLayoutName(const enum AEStdChLayout layout);
  static const unsigned int      DataFormatToBits  (const enum AEDataFormat dataFormat);
  static const char*             DataFormatToStr   (const enum AEDataFormat dataFormat);

  /*! \brief convert a linear factor to a dB gain for audio manipulation
   Inverts linear = pow(10.0f, dB/20)
   \param linear factor from 0..1
   \return the gain in dB from -60dB .. 0dB.
   \sa DecibelToLinear
   */
  static inline const float LinearToDecibel(const float linear)
  {
    // clamp to -60dB
    if (linear < 0.001f) 
      return -60.0f;
    else
      return 20.0f * log10(linear);
  }

  /*! \brief convert a dB gain to a linear factor for audio manipulation
   Inverts gain = 20 log_10(linear)
   \param dB the gain in decibels.
   \return the linear factor (equivalent to a voltage multiplier).
   \sa LinearToDecibel
   */
  static inline const float DecibelToLinear(const float dB)
  {
    return pow(10.0f, dB/20.0f);
  }

  #ifdef __SSE__
  static void SSEMulArray     (float *data, const float mul, uint32_t count);
  static void SSEMulAddArray  (float *data, float *add, const float mul, uint32_t count);
  #endif
  static void ClampArray(float *data, uint32_t count);

  /*
    Rand implementations based on:
    http://software.intel.com/en-us/articles/fast-random-number-generator-on-the-intel-pentiumr-4-processor/
    This is NOT safe for crypto work, but perfectly fine for audio usage (dithering)
  */
  static float FloatRand1(const float min, const float max);
  static void  FloatRand4(const float min, const float max, float result[4], __m128 *sseresult = NULL);
};
