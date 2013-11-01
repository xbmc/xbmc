#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#include "AEAudioFormat.h"
#include "utils/StdString.h"
#include "PlatformDefs.h"
#include <math.h>

#ifdef TARGET_WINDOWS
#if _M_IX86_FP>0 && !defined(__SSE__)
#define __SSE__
#if _M_IX86_FP>1 && !defined(__SSE2__)
#define __SSE2__
#endif
#endif
#endif

#ifdef __SSE__
#include <xmmintrin.h>
#else
#define __m128 void
#endif

#ifdef __SSE2__
#include <emmintrin.h>
#endif

#ifdef __GNUC__
  #define MEMALIGN(b, x) x __attribute__((aligned(b)))
#else
  #define MEMALIGN(b, x) __declspec(align(b)) x
#endif

// AV sync options
enum AVSync
{
  SYNC_DISCON   = 0,
  SYNC_SKIPDUP,
  SYNC_RESAMPLE
};

class CAEUtil
{
private:
  static unsigned int m_seed;
  #ifdef __SSE2__
    static __m128i m_sseSeed;
  #endif

  static float SoftClamp(const float x);

public:
  static CAEChannelInfo          GuessChLayout     (const unsigned int channels);
  static const char*             GetStdChLayoutName(const enum AEStdChLayout layout);
  static const unsigned int      DataFormatToBits  (const enum AEDataFormat dataFormat);
  static const unsigned int      DataFormatToUsedBits (const enum AEDataFormat dataFormat);
  static const char*             DataFormatToStr   (const enum AEDataFormat dataFormat);

  /*! \brief convert a volume percentage (as a proportion) to a dB gain
   We assume a dB range of 60dB, i.e. assume that 0% volume corresponds
   to a reduction of 60dB.
   \param value the volume from 0..1
   \return the corresponding gain in dB from -60dB .. 0dB.
   \sa GainToScale
   */
  static inline const float PercentToGain(const float value)
  {
    static const float db_range = 60.0f;
    return (value - 1)*db_range;
  }

  /*! \brief convert a dB gain to volume percentage (as a proportion)
   We assume a dB range of 60dB, i.e. assume that 0% volume corresponds
   to a reduction of 60dB.
   \param the corresponding gain in dB from -60dB .. 0dB.
   \return value the volume from 0..1
   \sa ScaleToGain
   */
  static inline const float GainToPercent(const float gain)
  {
    static const float db_range = 60.0f;
    return 1+(gain/db_range);
  }

  /*! \brief convert a dB gain to a scale factor for audio manipulation
   Inverts gain = 20 log_10(scale)
   \param dB the gain in decibels.
   \return the scale factor (equivalent to a voltage multiplier).
   \sa PercentToGain
   */
  static inline const float GainToScale(const float dB)
  {
    return pow(10.0f, dB/20);
  }

  /*! \brief convert a scale factor to dB gain for audio manipulation
   Inverts GainToScale result
   \param the scale factor (equivalent to a voltage multiplier).
   \return dB the gain in decibels.
   \sa GainToScale
   */
  static inline const float ScaleToGain(const float scale)
  {
    return 20*log10(scale);
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

  static bool S16NeedsByteSwap(AEDataFormat in, AEDataFormat out);
};
