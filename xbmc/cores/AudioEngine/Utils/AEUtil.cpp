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
#ifndef __STDC_LIMIT_MACROS
  #define __STDC_LIMIT_MACROS
#endif

#include "AEUtil.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

#include <cassert>

extern "C" {
#include "libavutil/channel_layout.h"
}

using namespace std;

/* declare the rng seed and initialize it */
unsigned int CAEUtil::m_seed = (unsigned int)(CurrentHostCounter() / 1000.0f);
#ifdef __SSE2__
  /* declare the SSE seed and initialize it */
  MEMALIGN(16, __m128i CAEUtil::m_sseSeed) = _mm_set_epi32(CAEUtil::m_seed, CAEUtil::m_seed+1, CAEUtil::m_seed, CAEUtil::m_seed+1);
#endif

void   AEDelayStatus::SetDelay(double d)
{
  delay = d;
  tick  = CurrentHostCounter();
}

double AEDelayStatus::GetDelay()
{
  double d = delay;
  if(tick)
    d -= (double)(CurrentHostCounter() - tick) / CurrentHostFrequency();
  if(d < 0)
    return 0.0;
  else
    return d;
}

CAEChannelInfo CAEUtil::GuessChLayout(const unsigned int channels)
{
  CLog::Log(LOGWARNING, "CAEUtil::GuessChLayout - "
    "This method should really never be used, please fix the code that called this");

  CAEChannelInfo result;
  if (channels < 1 || channels > 8)
    return result;

  switch (channels)
  {
    case 1: result = AE_CH_LAYOUT_1_0; break;
    case 2: result = AE_CH_LAYOUT_2_0; break;
    case 3: result = AE_CH_LAYOUT_3_0; break;
    case 4: result = AE_CH_LAYOUT_4_0; break;
    case 5: result = AE_CH_LAYOUT_5_0; break;
    case 6: result = AE_CH_LAYOUT_5_1; break;
    case 7: result = AE_CH_LAYOUT_7_0; break;
    case 8: result = AE_CH_LAYOUT_7_1; break;
  }

  return result;
}

const char* CAEUtil::GetStdChLayoutName(const enum AEStdChLayout layout)
{
  if (layout < 0 || layout >= AE_CH_LAYOUT_MAX)
    return "UNKNOWN";

  static const char* layouts[AE_CH_LAYOUT_MAX] =
  {
    "1.0",
    "2.0", "2.1", "3.0", "3.1", "4.0",
    "4.1", "5.0", "5.1", "7.0", "7.1"
  };

  return layouts[layout];
}

const unsigned int CAEUtil::DataFormatToBits(const enum AEDataFormat dataFormat)
{
  if (dataFormat < 0 || dataFormat >= AE_FMT_MAX)
    return 0;

  static const unsigned int formats[AE_FMT_MAX] =
  {
    8,                   /* U8     */

    16,                  /* S16BE  */
    16,                  /* S16LE  */
    16,                  /* S16NE  */
    
    32,                  /* S32BE  */
    32,                  /* S32LE  */
    32,                  /* S32NE  */
    
    32,                  /* S24BE  */
    32,                  /* S24LE  */
    32,                  /* S24NE  */
    32,                  /* S24NER */
    
    24,                  /* S24BE3 */
    24,                  /* S24LE3 */
    24,                  /* S24NE3 */
    
    sizeof(double) << 3, /* DOUBLE */
    sizeof(float ) << 3, /* FLOAT  */
    
    16,                  /* AAC    */
    16,                  /* AC3    */
    16,                  /* DTS    */
    16,                  /* EAC3   */
    16,                  /* TRUEHD */
    16,                  /* DTS-HD */
    32,                  /* LPCM   */

     8,                  /* U8P    */
    16,                  /* S16NEP */
    32,                  /* S32NEP */
    32,                  /* S24NEP */
    32,                  /* S24NERP*/
    24,                  /* S24NE3P*/
    sizeof(double) << 3, /* DOUBLEP */
    sizeof(float ) << 3  /* FLOATP  */
 };

  return formats[dataFormat];
}

const unsigned int CAEUtil::DataFormatToUsedBits(const enum AEDataFormat dataFormat)
{
  if (dataFormat == AE_FMT_S24BE4 || dataFormat == AE_FMT_S24LE4 ||
      dataFormat == AE_FMT_S24NE4 || dataFormat == AE_FMT_S24NE4MSB)
    return 24;
  else
    return DataFormatToBits(dataFormat);
}

const unsigned int CAEUtil::DataFormatToDitherBits(const enum AEDataFormat dataFormat)
{
  if (dataFormat == AE_FMT_S24NE4MSB)
    return 8;
  if (dataFormat == AE_FMT_S24NE3)
    return -8;
  else
    return 0;
}

const char* CAEUtil::DataFormatToStr(const enum AEDataFormat dataFormat)
{
  if (dataFormat < 0 || dataFormat >= AE_FMT_MAX)
    return "UNKNOWN";

  static const char *formats[AE_FMT_MAX] =
  {
    "AE_FMT_U8",

    "AE_FMT_S16BE",
    "AE_FMT_S16LE",
    "AE_FMT_S16NE",
    
    "AE_FMT_S32BE",
    "AE_FMT_S32LE",
    "AE_FMT_S32NE",
    
    "AE_FMT_S24BE4",
    "AE_FMT_S24LE4",
    "AE_FMT_S24NE4",  /* S24 in 4 bytes */
    "AE_FMT_S24NE4MSB",
    
    "AE_FMT_S24BE3",
    "AE_FMT_S24LE3",
    "AE_FMT_S24NE3", /* S24 in 3 bytes */
    
    "AE_FMT_DOUBLE",
    "AE_FMT_FLOAT",
    
    /* for passthrough streams and the like */
    "AE_FMT_AAC",
    "AE_FMT_AC3",
    "AE_FMT_DTS",
    "AE_FMT_EAC3",
    "AE_FMT_TRUEHD",
    "AE_FMT_DTSHD",
    "AE_FMT_LPCM",

    /* planar formats */
    "AE_FMT_U8P",
    "AE_FMT_S16NEP",
    "AE_FMT_S32NEP",
    "AE_FMT_S24NE4P",
    "AE_FMT_S24NE4MSBP",
    "AE_FMT_S24NE3P",
    "AE_FMT_DOUBLEP",
    "AE_FMT_FLOATP"
  };

  return formats[dataFormat];
}

#ifdef __SSE__
void CAEUtil::SSEMulArray(float *data, const float mul, uint32_t count)
{
  const __m128 m = _mm_set_ps1(mul);

  /* work around invalid alignment */
  while (((uintptr_t)data & 0xF) && count > 0)
  {
    data[0] *= mul;
    ++data;
    --count;
  }

  uint32_t even = count & ~0x3;
  for (uint32_t i = 0; i < even; i+=4, data+=4)
  {
    __m128 to      = _mm_load_ps(data);
    *(__m128*)data = _mm_mul_ps (to, m);
  }

  if (even != count)
  {
    uint32_t odd = count - even;
    if (odd == 1)
      data[0] *= mul;
    else
    {
      __m128 to;
      if (odd == 2)
      {
        to = _mm_setr_ps(data[0], data[1], 0, 0);
        __m128 ou = _mm_mul_ps(to, m);
        data[0] = ((float*)&ou)[0];
        data[1] = ((float*)&ou)[1];
      }
      else
      {
        to = _mm_setr_ps(data[0], data[1], data[2], 0);
        __m128 ou = _mm_mul_ps(to, m);
        data[0] = ((float*)&ou)[0];
        data[1] = ((float*)&ou)[1];
        data[2] = ((float*)&ou)[2];
      }
    }
  }
}

void CAEUtil::SSEMulAddArray(float *data, float *add, const float mul, uint32_t count)
{
  const __m128 m = _mm_set_ps1(mul);

  /* work around invalid alignment */
  while ((((uintptr_t)data & 0xF) || ((uintptr_t)add & 0xF)) && count > 0)
  {
    data[0] += add[0] * mul;
    ++add;
    ++data;
    --count;
  }

  uint32_t even = count & ~0x3;
  for (uint32_t i = 0; i < even; i+=4, data+=4, add+=4)
  {
    __m128 ad      = _mm_load_ps(add );
    __m128 to      = _mm_load_ps(data);
    *(__m128*)data = _mm_add_ps (to, _mm_mul_ps(ad, m));
  }

  if (even != count)
  {
    uint32_t odd = count - even;
    if (odd == 1)
      data[0] += add[0] * mul;
    else
    {
      __m128 ad;
      __m128 to;
      if (odd == 2)
      {
        ad = _mm_setr_ps(add [0], add [1], 0, 0);
        to = _mm_setr_ps(data[0], data[1], 0, 0);
        __m128 ou = _mm_add_ps(to, _mm_mul_ps(ad, m));
        data[0] = ((float*)&ou)[0];
        data[1] = ((float*)&ou)[1];
      }
      else
      {
        ad = _mm_setr_ps(add [0], add [1], add [2], 0);
        to = _mm_setr_ps(data[0], data[1], data[2], 0);
        __m128 ou = _mm_add_ps(to, _mm_mul_ps(ad, m));
        data[0] = ((float*)&ou)[0];
        data[1] = ((float*)&ou)[1];
        data[2] = ((float*)&ou)[2];
      }
    }
  }
}
#endif

inline float CAEUtil::SoftClamp(const float x)
{
#if 1
    /*
       This is a rational function to approximate a tanh-like soft clipper.
       It is based on the pade-approximation of the tanh function with tweaked coefficients.
       See: http://www.musicdsp.org/showone.php?id=238
    */
    if (x < -3.0f)
      return -1.0f;
    else if (x >  3.0f)
      return 1.0f;
    float y = x * x;
    return x * (27.0f + y) / (27.0f + 9.0f * y);
#else
    /* slower method using tanh, but more accurate */

    static const double k = 0.9f;
    /* perform a soft clamp */
    if (x >  k)
      x = (float) (tanh((x - k) / (1 - k)) * (1 - k) + k);
    else if (x < -k)
      x = (float) (tanh((x + k) / (1 - k)) * (1 - k) - k);

    /* hard clamp anything still outside the bounds */
    if (x >  1.0f)
      return  1.0f;
    if (x < -1.0f)
      return -1.0f;

    /* return the final sample */
    return x;
#endif
}

void CAEUtil::ClampArray(float *data, uint32_t count)
{
#ifndef __SSE__
  for (uint32_t i = 0; i < count; ++i)
    data[i] = SoftClamp(data[i]);

#else
  const __m128 c1 = _mm_set_ps1(27.0f);
  const __m128 c2 = _mm_set_ps1(27.0f + 9.0f);

  /* work around invalid alignment */
  while (((uintptr_t)data & 0xF) && count > 0)
  {
    data[0] = SoftClamp(data[0]);
    ++data;
    --count;
  }

  uint32_t even = count & ~0x3;
  for (uint32_t i = 0; i < even; i+=4, data+=4)
  {
    /* tanh approx clamp */
    __m128 dt = _mm_load_ps(data);
    __m128 tmp     = _mm_mul_ps(dt, dt);
    *(__m128*)data = _mm_div_ps(
      _mm_mul_ps(
        dt,
        _mm_add_ps(c1, tmp)
      ),
      _mm_add_ps(c2, tmp)
    );
  }

  if (even != count)
  {
    uint32_t odd = count - even;
    if (odd == 1)
      data[0] = SoftClamp(data[0]);
    else
    {
      __m128 dt;
      __m128 tmp;
      __m128 out;
      if (odd == 2)
      {
        /* tanh approx clamp */
        dt  = _mm_setr_ps(data[0], data[1], 0, 0);
        tmp = _mm_mul_ps(dt, dt);
        out = _mm_div_ps(
          _mm_mul_ps(
            dt,
            _mm_add_ps(c1, tmp)
          ),
          _mm_add_ps(c2, tmp)
        );

        data[0] = ((float*)&out)[0];
        data[1] = ((float*)&out)[1];
      }
      else
      {
        /* tanh approx clamp */
        dt  = _mm_setr_ps(data[0], data[1], data[2], 0);
        tmp = _mm_mul_ps(dt, dt);
        out = _mm_div_ps(
          _mm_mul_ps(
            dt,
            _mm_add_ps(c1, tmp)
          ),
          _mm_add_ps(c2, tmp)
        );

        data[0] = ((float*)&out)[0];
        data[1] = ((float*)&out)[1];
        data[2] = ((float*)&out)[2];
      }
    }
  }
#endif
}

/*
  Rand implementations based on:
  http://software.intel.com/en-us/articles/fast-random-number-generator-on-the-intel-pentiumr-4-processor/
  This is NOT safe for crypto work, but perfectly fine for audio usage (dithering)
*/
float CAEUtil::FloatRand1(const float min, const float max)
{
  const float delta  = (max - min) / 2;
  const float factor = delta / (float)INT32_MAX;
  return ((float)(m_seed = (214013 * m_seed + 2531011)) * factor) - delta;
}

void CAEUtil::FloatRand4(const float min, const float max, float result[4], __m128 *sseresult/* = NULL */)
{
  #ifdef __SSE2__
    /*
      this method may be called from other SSE code, we need
      to calculate the delta & factor using SSE as the FPU
      state is unknown and _mm_clear() is expensive.
    */
    MEMALIGN(16, static const __m128 point5  ) = _mm_set_ps1(0.5f);
    MEMALIGN(16, static const __m128 int32max) = _mm_set_ps1((const float)INT32_MAX);
    MEMALIGN(16, __m128 f) = _mm_div_ps(
      _mm_mul_ps(
        _mm_sub_ps(
          _mm_set_ps1(max),
          _mm_set_ps1(min)
        ),
        point5
      ),
      int32max
    );

    MEMALIGN(16, __m128i cur_seed_split);
    MEMALIGN(16, __m128i multiplier);
    MEMALIGN(16, __m128i adder);
    MEMALIGN(16, __m128i mod_mask);
    MEMALIGN(16, __m128 res);
    MEMALIGN(16, static const unsigned int mult  [4]) = {214013, 17405, 214013, 69069};
    MEMALIGN(16, static const unsigned int gadd  [4]) = {2531011, 10395331, 13737667, 1};
    MEMALIGN(16, static const unsigned int mask  [4]) = {0xFFFFFFFF, 0, 0xFFFFFFFF, 0};

    adder          = _mm_load_si128((__m128i*)gadd);
    multiplier     = _mm_load_si128((__m128i*)mult);
    mod_mask       = _mm_load_si128((__m128i*)mask);
    cur_seed_split = _mm_shuffle_epi32(m_sseSeed, _MM_SHUFFLE(2, 3, 0, 1));

    m_sseSeed      = _mm_mul_epu32(m_sseSeed, multiplier);
    multiplier     = _mm_shuffle_epi32(multiplier, _MM_SHUFFLE(2, 3, 0, 1));
    cur_seed_split = _mm_mul_epu32(cur_seed_split, multiplier);

    m_sseSeed      = _mm_and_si128(m_sseSeed, mod_mask);
    cur_seed_split = _mm_and_si128(cur_seed_split, mod_mask);
    cur_seed_split = _mm_shuffle_epi32(cur_seed_split, _MM_SHUFFLE(2, 3, 0, 1));
    m_sseSeed      = _mm_or_si128(m_sseSeed, cur_seed_split);
    m_sseSeed      = _mm_add_epi32(m_sseSeed, adder);

    /* adjust the value to the range requested */
    res = _mm_cvtepi32_ps(m_sseSeed);
    if (sseresult)
      *sseresult = _mm_mul_ps(res, f);
    else
    {
      res = _mm_mul_ps(res, f);
      _mm_storeu_ps(result, res);

      /* returning a float array, so cleanup */
      _mm_empty();
    }

  #else
    const float delta  = (max - min) / 2.0f;
    const float factor = delta / (float)INT32_MAX;

    /* cant return sseresult if we are not using SSE intrinsics */
    assert(result && !sseresult);

    result[0] = ((float)(m_seed = (214013 * m_seed + 2531011)) * factor) - delta;
    result[1] = ((float)(m_seed = (214013 * m_seed + 2531011)) * factor) - delta;
    result[2] = ((float)(m_seed = (214013 * m_seed + 2531011)) * factor) - delta;
    result[3] = ((float)(m_seed = (214013 * m_seed + 2531011)) * factor) - delta;
  #endif
}

bool CAEUtil::S16NeedsByteSwap(AEDataFormat in, AEDataFormat out)
{
  const AEDataFormat nativeFormat =
#ifdef WORDS_BIGENDIAN
    AE_FMT_S16BE;
#else
    AE_FMT_S16LE;
#endif

  if (in == AE_FMT_S16NE || AE_IS_RAW(in))
    in = nativeFormat;
  if (out == AE_FMT_S16NE || AE_IS_RAW(out))
    out = nativeFormat;

  return in != out;
}

uint64_t CAEUtil::GetAVChannelLayout(CAEChannelInfo &info)
{
  uint64_t channelLayout = 0;
  if (info.HasChannel(AE_CH_FL))   channelLayout |= AV_CH_FRONT_LEFT;
  if (info.HasChannel(AE_CH_FR))   channelLayout |= AV_CH_FRONT_RIGHT;
  if (info.HasChannel(AE_CH_FC))   channelLayout |= AV_CH_FRONT_CENTER;
  if (info.HasChannel(AE_CH_LFE))  channelLayout |= AV_CH_LOW_FREQUENCY;
  if (info.HasChannel(AE_CH_BL))   channelLayout |= AV_CH_BACK_LEFT;
  if (info.HasChannel(AE_CH_BR))   channelLayout |= AV_CH_BACK_RIGHT;
  if (info.HasChannel(AE_CH_FLOC)) channelLayout |= AV_CH_FRONT_LEFT_OF_CENTER;
  if (info.HasChannel(AE_CH_FROC)) channelLayout |= AV_CH_FRONT_RIGHT_OF_CENTER;
  if (info.HasChannel(AE_CH_BC))   channelLayout |= AV_CH_BACK_CENTER;
  if (info.HasChannel(AE_CH_SL))   channelLayout |= AV_CH_SIDE_LEFT;
  if (info.HasChannel(AE_CH_SR))   channelLayout |= AV_CH_SIDE_RIGHT;
  if (info.HasChannel(AE_CH_TC))   channelLayout |= AV_CH_TOP_CENTER;
  if (info.HasChannel(AE_CH_TFL))  channelLayout |= AV_CH_TOP_FRONT_LEFT;
  if (info.HasChannel(AE_CH_TFC))  channelLayout |= AV_CH_TOP_FRONT_CENTER;
  if (info.HasChannel(AE_CH_TFR))  channelLayout |= AV_CH_TOP_FRONT_RIGHT;
  if (info.HasChannel(AE_CH_TBL))   channelLayout |= AV_CH_TOP_BACK_LEFT;
  if (info.HasChannel(AE_CH_TBC))   channelLayout |= AV_CH_TOP_BACK_CENTER;
  if (info.HasChannel(AE_CH_TBR))   channelLayout |= AV_CH_TOP_BACK_RIGHT;

  return channelLayout;
}

AVSampleFormat CAEUtil::GetAVSampleFormat(AEDataFormat format)
{
  if      (format == AE_FMT_U8)     return AV_SAMPLE_FMT_U8;
  else if (format == AE_FMT_S16NE)  return AV_SAMPLE_FMT_S16;
  else if (format == AE_FMT_S32NE)  return AV_SAMPLE_FMT_S32;
  else if (format == AE_FMT_S24NE4) return AV_SAMPLE_FMT_S32;
  else if (format == AE_FMT_S24NE4MSB)return AV_SAMPLE_FMT_S32;
  else if (format == AE_FMT_S24NE3) return AV_SAMPLE_FMT_S32;
  else if (format == AE_FMT_FLOAT)  return AV_SAMPLE_FMT_FLT;
  else if (format == AE_FMT_DOUBLE) return AV_SAMPLE_FMT_DBL;

  else if (format == AE_FMT_U8P)     return AV_SAMPLE_FMT_U8P;
  else if (format == AE_FMT_S16NEP)  return AV_SAMPLE_FMT_S16P;
  else if (format == AE_FMT_S32NEP)  return AV_SAMPLE_FMT_S32P;
  else if (format == AE_FMT_S24NE4P) return AV_SAMPLE_FMT_S32P;
  else if (format == AE_FMT_S24NE4MSBP)return AV_SAMPLE_FMT_S32P;
  else if (format == AE_FMT_S24NE3P) return AV_SAMPLE_FMT_S32P;
  else if (format == AE_FMT_FLOATP)  return AV_SAMPLE_FMT_FLTP;
  else if (format == AE_FMT_DOUBLEP) return AV_SAMPLE_FMT_DBLP;

  if (AE_IS_PLANAR(format))
    return AV_SAMPLE_FMT_FLTP;
  else
    return AV_SAMPLE_FMT_FLT;
}

uint64_t CAEUtil::GetAVChannel(enum AEChannel aechannel)
{
  switch (aechannel)
  {
  case AE_CH_FL:   return AV_CH_FRONT_LEFT;
  case AE_CH_FR:   return AV_CH_FRONT_RIGHT;
  case AE_CH_FC:   return AV_CH_FRONT_CENTER;
  case AE_CH_LFE:  return AV_CH_LOW_FREQUENCY;
  case AE_CH_BL:   return AV_CH_BACK_LEFT;
  case AE_CH_BR:   return AV_CH_BACK_RIGHT;
  case AE_CH_FLOC: return AV_CH_FRONT_LEFT_OF_CENTER;
  case AE_CH_FROC: return AV_CH_FRONT_RIGHT_OF_CENTER;
  case AE_CH_BC:   return AV_CH_BACK_CENTER;
  case AE_CH_SL:   return AV_CH_SIDE_LEFT;
  case AE_CH_SR:   return AV_CH_SIDE_RIGHT;
  case AE_CH_TC:   return AV_CH_TOP_CENTER;
  case AE_CH_TFL:  return AV_CH_TOP_FRONT_LEFT;
  case AE_CH_TFC:  return AV_CH_TOP_FRONT_CENTER;
  case AE_CH_TFR:  return AV_CH_TOP_FRONT_RIGHT;
  case AE_CH_TBL:  return AV_CH_TOP_BACK_LEFT;
  case AE_CH_TBC:  return AV_CH_TOP_BACK_CENTER;
  case AE_CH_TBR:  return AV_CH_TOP_BACK_RIGHT;
  default:
    return 0;
  }
}

int CAEUtil::GetAVChannelIndex(enum AEChannel aechannel, uint64_t layout)
{
  return av_get_channel_layout_channel_index(layout, GetAVChannel(aechannel));
}
