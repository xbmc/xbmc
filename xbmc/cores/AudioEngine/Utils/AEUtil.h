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

  /* convert a linear value between 0.0 and 1.0 to a logrithmic value  */
  static inline const float LinToLog(const float dbrange, const float value)
  {
    float b = log(pow(10.0f, dbrange * 0.05f));
    float a = 1.0f / exp(b);
    return a * exp(b * value);
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
