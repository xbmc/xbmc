/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#ifndef __STDC_LIMIT_MACROS
  #define __STDC_LIMIT_MACROS
#endif

#include "AEUtil.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

#include <cassert>

#if defined(HAVE_SSE) && defined(__SSE__)
#include <xmmintrin.h>
#endif

void AEDelayStatus::SetDelay(double d)
{
  delay = d;
  maxcorrection = d;
  tick = CurrentHostCounter();
}

double AEDelayStatus::GetDelay() const
{
  double d = 0;
  if (tick)
    d = (double)(CurrentHostCounter() - tick) / CurrentHostFrequency();
  if (d > maxcorrection)
    d = maxcorrection;

  return delay - d;
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

unsigned int CAEUtil::DataFormatToBits(const enum AEDataFormat dataFormat)
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

     8,                  /* RAW    */

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

unsigned int CAEUtil::DataFormatToUsedBits(const enum AEDataFormat dataFormat)
{
  if (dataFormat == AE_FMT_S24BE4 || dataFormat == AE_FMT_S24LE4 ||
      dataFormat == AE_FMT_S24NE4 || dataFormat == AE_FMT_S24NE4MSB)
    return 24;
  else
    return DataFormatToBits(dataFormat);
}

unsigned int CAEUtil::DataFormatToDitherBits(const enum AEDataFormat dataFormat)
{
  if (dataFormat == AE_FMT_S24NE4MSB)
    return 8;
  if (dataFormat == AE_FMT_S24NE3)
    return -8;
  else
    return 0;
}

const char* CAEUtil::StreamTypeToStr(const enum CAEStreamInfo::DataType dataType)
{
  switch (dataType)
  {
    case CAEStreamInfo::STREAM_TYPE_AC3:
      return "STREAM_TYPE_AC3";
    case CAEStreamInfo::STREAM_TYPE_DTSHD:
      return "STREAM_TYPE_DTSHD";
    case CAEStreamInfo::STREAM_TYPE_DTSHD_MA:
      return "STREAM_TYPE_DTSHD_MA";
    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
      return "STREAM_TYPE_DTSHD_CORE";
    case CAEStreamInfo::STREAM_TYPE_DTS_1024:
      return "STREAM_TYPE_DTS_1024";
    case CAEStreamInfo::STREAM_TYPE_DTS_2048:
      return "STREAM_TYPE_DTS_2048";
    case CAEStreamInfo::STREAM_TYPE_DTS_512:
      return "STREAM_TYPE_DTS_512";
    case CAEStreamInfo::STREAM_TYPE_EAC3:
      return "STREAM_TYPE_EAC3";
    case CAEStreamInfo::STREAM_TYPE_MLP:
      return "STREAM_TYPE_MLP";
    case CAEStreamInfo::STREAM_TYPE_TRUEHD:
      return "STREAM_TYPE_TRUEHD";

    default:
      return "STREAM_TYPE_NULL";
  }
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

    "AE_FMT_RAW",

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

#if defined(HAVE_SSE) && defined(__SSE__)
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
#if !defined(HAVE_SSE) || !defined(__SSE__)
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

bool CAEUtil::S16NeedsByteSwap(AEDataFormat in, AEDataFormat out)
{
  const AEDataFormat nativeFormat =
#ifdef WORDS_BIGENDIAN
    AE_FMT_S16BE;
#else
    AE_FMT_S16LE;
#endif

  if (in == AE_FMT_S16NE || (in == AE_FMT_RAW))
    in = nativeFormat;
  if (out == AE_FMT_S16NE || (out == AE_FMT_RAW))
    out = nativeFormat;

  return in != out;
}

uint64_t CAEUtil::GetAVChannelLayout(const CAEChannelInfo &info)
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

CAEChannelInfo CAEUtil::GetAEChannelLayout(uint64_t layout)
{
  CAEChannelInfo channelLayout;
  channelLayout.Reset();

  if (layout & AV_CH_FRONT_LEFT)       channelLayout += AE_CH_FL;
  if (layout & AV_CH_FRONT_RIGHT)      channelLayout += AE_CH_FR;
  if (layout & AV_CH_FRONT_CENTER)     channelLayout += AE_CH_FC;
  if (layout & AV_CH_LOW_FREQUENCY)    channelLayout += AE_CH_LFE;
  if (layout & AV_CH_BACK_LEFT)        channelLayout += AE_CH_BL;
  if (layout & AV_CH_BACK_RIGHT)       channelLayout += AE_CH_BR;
  if (layout & AV_CH_FRONT_LEFT_OF_CENTER)  channelLayout += AE_CH_FLOC;
  if (layout & AV_CH_FRONT_RIGHT_OF_CENTER) channelLayout += AE_CH_FROC;
  if (layout & AV_CH_BACK_CENTER)      channelLayout += AE_CH_BC;
  if (layout & AV_CH_SIDE_LEFT)        channelLayout += AE_CH_SL;
  if (layout & AV_CH_SIDE_RIGHT)       channelLayout += AE_CH_SR;
  if (layout & AV_CH_TOP_CENTER)       channelLayout += AE_CH_TC;
  if (layout & AV_CH_TOP_FRONT_LEFT)   channelLayout += AE_CH_TFL;
  if (layout & AV_CH_TOP_FRONT_CENTER) channelLayout += AE_CH_TFC;
  if (layout & AV_CH_TOP_FRONT_RIGHT)  channelLayout += AE_CH_TFR;
  if (layout & AV_CH_TOP_BACK_LEFT)    channelLayout += AE_CH_BL;
  if (layout & AV_CH_TOP_BACK_CENTER)  channelLayout += AE_CH_BC;
  if (layout & AV_CH_TOP_BACK_RIGHT)   channelLayout += AE_CH_BR;

  return channelLayout;
}

AVSampleFormat CAEUtil::GetAVSampleFormat(AEDataFormat format)
{
  switch (format)
  {
    case AEDataFormat::AE_FMT_U8:
      return AV_SAMPLE_FMT_U8;
    case AEDataFormat::AE_FMT_S16NE:
      return AV_SAMPLE_FMT_S16;
    case AEDataFormat::AE_FMT_S32NE:
      return AV_SAMPLE_FMT_S32;
    case AEDataFormat::AE_FMT_S24NE4:
      return AV_SAMPLE_FMT_S32;
    case AEDataFormat::AE_FMT_S24NE4MSB:
      return AV_SAMPLE_FMT_S32;
    case AEDataFormat::AE_FMT_S24NE3:
      return AV_SAMPLE_FMT_S32;
    case AEDataFormat::AE_FMT_FLOAT:
      return AV_SAMPLE_FMT_FLT;
    case AEDataFormat::AE_FMT_DOUBLE:
      return AV_SAMPLE_FMT_DBL;
    case AEDataFormat::AE_FMT_U8P:
      return AV_SAMPLE_FMT_U8P;
    case AEDataFormat::AE_FMT_S16NEP:
      return AV_SAMPLE_FMT_S16P;
    case AEDataFormat::AE_FMT_S32NEP:
      return AV_SAMPLE_FMT_S32P;
    case AEDataFormat::AE_FMT_S24NE4P:
      return AV_SAMPLE_FMT_S32P;
    case AEDataFormat::AE_FMT_S24NE4MSBP:
      return AV_SAMPLE_FMT_S32P;
    case AEDataFormat::AE_FMT_S24NE3P:
      return AV_SAMPLE_FMT_S32P;
    case AEDataFormat::AE_FMT_FLOATP:
      return AV_SAMPLE_FMT_FLTP;
    case AEDataFormat::AE_FMT_DOUBLEP:
      return AV_SAMPLE_FMT_DBLP;
    case AEDataFormat::AE_FMT_RAW:
      return AV_SAMPLE_FMT_U8;
    default:
    {
      if (AE_IS_PLANAR(format))
        return AV_SAMPLE_FMT_FLTP;
      else
        return AV_SAMPLE_FMT_FLT;
    }
  }
}

uint64_t CAEUtil::GetAVChannelMask(enum AEChannel aechannel)
{
  enum AVChannel ch = GetAVChannel(aechannel);
  if (ch == AV_CHAN_NONE)
    return 0;
  return (1ULL << ch);
}

enum AVChannel CAEUtil::GetAVChannel(enum AEChannel aechannel)
{
  switch (aechannel)
  {
    case AE_CH_FL:
      return AV_CHAN_FRONT_LEFT;
    case AE_CH_FR:
      return AV_CHAN_FRONT_RIGHT;
    case AE_CH_FC:
      return AV_CHAN_FRONT_CENTER;
    case AE_CH_LFE:
      return AV_CHAN_LOW_FREQUENCY;
    case AE_CH_BL:
      return AV_CHAN_BACK_LEFT;
    case AE_CH_BR:
      return AV_CHAN_BACK_RIGHT;
    case AE_CH_FLOC:
      return AV_CHAN_FRONT_LEFT_OF_CENTER;
    case AE_CH_FROC:
      return AV_CHAN_FRONT_RIGHT_OF_CENTER;
    case AE_CH_BC:
      return AV_CHAN_BACK_CENTER;
    case AE_CH_SL:
      return AV_CHAN_SIDE_LEFT;
    case AE_CH_SR:
      return AV_CHAN_SIDE_RIGHT;
    case AE_CH_TC:
      return AV_CHAN_TOP_CENTER;
    case AE_CH_TFL:
      return AV_CHAN_TOP_FRONT_LEFT;
    case AE_CH_TFC:
      return AV_CHAN_TOP_FRONT_CENTER;
    case AE_CH_TFR:
      return AV_CHAN_TOP_FRONT_RIGHT;
    case AE_CH_TBL:
      return AV_CHAN_TOP_BACK_LEFT;
    case AE_CH_TBC:
      return AV_CHAN_TOP_BACK_CENTER;
    case AE_CH_TBR:
      return AV_CHAN_TOP_BACK_RIGHT;
    default:
      return AV_CHAN_NONE;
  }
}

int CAEUtil::GetAVChannelIndex(enum AEChannel aechannel, uint64_t layout)
{
  AVChannelLayout ch_layout = {};
  av_channel_layout_from_mask(&ch_layout, layout);
  int idx = av_channel_layout_index_from_channel(&ch_layout, GetAVChannel(aechannel));
  av_channel_layout_uninit(&ch_layout);
  return idx;
}
