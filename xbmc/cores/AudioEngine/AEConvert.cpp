/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#define __STDC_LIMIT_MACROS
#include "AEConvert.h"
#include "AEUtil.h"
#include "MathUtils.h"
#include "utils/EndianSwap.h"

#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

#ifndef _WIN32
#include <unistd.h>
#endif
#include <math.h>
#include <string.h>

#ifdef __SSE__
#include <xmmintrin.h>
#include <emmintrin.h>
#endif

#define CLAMP(x) std::min(-1.0f, std::max(1.0f, (float)(x)))

#ifndef INT24_MAX
#define INT24_MAX (0x7FFFFF)
#endif

float *CAEConvert::m_LookupU8  = NULL;
float *CAEConvert::m_LookupS16 = NULL;

static inline int safeRound(double f)
{
  /* if the value is larger then we can handle, then clamp it */
  if (f >= INT_MAX) return INT_MAX;
  if (f <= INT_MIN) return INT_MIN;

  /* if the value is out of the MathUtils::round_int range, then round it normally */
  if (f <= static_cast<double>(INT_MIN / 2) - 1.0 || f >= static_cast <double>(INT_MAX / 2) + 1.0)
    return floor(f+0.5);

  return MathUtils::round_int(f);
}

CAEConvert::AEConvertToFn CAEConvert::ToFloat(enum AEDataFormat dataFormat)
{
  /* build lookup tables if we need them */
  switch(dataFormat)
  {
    case AE_FMT_U8:
    case AE_FMT_S8:
      if (!m_LookupU8)
      {
        m_LookupU8 = new float[UINT8_MAX + 1];
        for(int i = 0; i < UINT8_MAX; ++i)
          m_LookupU8[i] = ((float)i / UINT8_MAX) * 2.0f - 1.0f;
      }
      break;

    case AE_FMT_S16NE:
    case AE_FMT_S16LE:
    case AE_FMT_S16BE:
      if (!m_LookupS16)
      {
        m_LookupS16 = new float[UINT16_MAX + 1];
        for(int i = 0; i < UINT16_MAX; ++i)
          m_LookupS16[i] = ((float)i / ((float)INT16_MAX+.5f)) - 1.0f;
      }
      break;

    default:
      break;
  }

  switch(dataFormat)
  {
    case AE_FMT_U8    : return &U8_Float;
    case AE_FMT_S8    : return &S8_Float;
#ifdef __BIG_ENDIAN__
    case AE_FMT_S16NE : return &S16BE_Float;
    case AE_FMT_S32NE : return &S32BE_Float;
    case AE_FMT_S24NE4: return &S24BE4_Float;
    case AE_FMT_S24NE3: return &S24BE3_Float;
#else
    case AE_FMT_S16NE : return &S16LE_Float;
    case AE_FMT_S32NE : return &S32LE_Float;
    case AE_FMT_S24NE4: return &S24LE4_Float;
    case AE_FMT_S24NE3: return &S24LE3_Float;
#endif
    case AE_FMT_S16LE : return &S16LE_Float;
    case AE_FMT_S16BE : return &S16BE_Float;
    case AE_FMT_S24LE4: return &S24LE4_Float;
    case AE_FMT_S24BE4: return &S24BE4_Float;
    case AE_FMT_S24LE3: return &S24LE3_Float;
    case AE_FMT_S24BE3: return &S24BE3_Float;
    case AE_FMT_S32LE : return &S32LE_Float;
    case AE_FMT_S32BE : return &S32BE_Float;
    case AE_FMT_DOUBLE: return &DOUBLE_Float;
    default:
      return NULL;
  }
}

CAEConvert::AEConvertFrFn CAEConvert::FrFloat(enum AEDataFormat dataFormat)
{
  switch(dataFormat)
  {
    case AE_FMT_U8    : return &Float_U8;
    case AE_FMT_S8    : return &Float_S8;
#ifdef __BIG_ENDIAN__
    case AE_FMT_S16NE : return &Float_S16BE;
    case AE_FMT_S32NE : return &Float_S32BE;
#else
    case AE_FMT_S16NE : return &Float_S16LE;
    case AE_FMT_S32NE : return &Float_S32LE;
#endif
    case AE_FMT_S16LE : return &Float_S16LE;
    case AE_FMT_S16BE : return &Float_S16BE;
    case AE_FMT_S24NE4: return &Float_S24NE4;
    case AE_FMT_S24NE3: return &Float_S24NE3;
    case AE_FMT_S32LE : return &Float_S32LE;
    case AE_FMT_S32BE : return &Float_S32BE;
    case AE_FMT_DOUBLE: return &Float_DOUBLE;
    default:
      return NULL;
  }
}

unsigned int CAEConvert::U8_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  unsigned int i;
  for(i = 0; i < samples; ++i, ++data, ++dest)
    *dest = m_LookupU8[*data];

  return samples;
}

unsigned int CAEConvert::S8_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  unsigned int i;
  for(i = 0; i < samples; ++i, ++data, ++dest)
    *dest = m_LookupU8[(int8_t)*data - INT8_MAX];

  return samples;
}

unsigned int CAEConvert::S16LE_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  unsigned int i;
  for(i = 0; i < samples; ++i, data += 2, ++dest)
  {
#ifndef __BIG_ENDIAN__
    *dest = m_LookupS16[*(int16_t*)data + INT16_MAX];
#else
    int16_t value;
    swab((char *)data, (char *)&value, 2);
    *dest = m_LookupS16[value + INT16_MAX];
#endif
  }

  return samples;
}

unsigned int CAEConvert::S16BE_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  unsigned int i;
  for(i = 0; i < samples; ++i, data += 2, ++dest)
  {
#ifdef __BIG_ENDIAN__
    *dest = m_LookupS16[*(int16_t*)data + INT16_MAX];
#else
    int16_t value;
    swab((char *)data, (char *)&value, 2);
    *dest = m_LookupS16[value + INT16_MAX];
#endif
  }

  return samples;
}

unsigned int CAEConvert::S24LE4_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  for(unsigned int i = 0; i < samples; ++i, ++dest, data += 4)
  {
    int s = (data[2] << 24) | (data[1] << 16) | (data[0] << 8);
    *dest = (float)s / (float)(INT32_MAX - 0xFF);
  }
  return samples;
}

unsigned int CAEConvert::S24BE4_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  for(unsigned int i = 0; i < samples; ++i, ++dest, data += 4)
  {
    int s = (data[0] << 24) | (data[1] << 16) | (data[2] << 8);
    *dest = (float)s / (float)(INT32_MAX - 0xFF);
  }
  return samples;
}

unsigned int CAEConvert::S24LE3_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  for(unsigned int i = 0; i < samples; ++i, ++dest, data += 3)
  {
    int s = (data[2] << 24) | (data[1] << 16) | (data[0] << 8);
    *dest = (float)s / (float)(INT32_MAX - 0xFF);
  }
  return samples;
}

unsigned int CAEConvert::S24BE3_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  for(unsigned int i = 0; i < samples; ++i, ++dest, data += 3)
  {
    int s = (data[1] << 24) | (data[2] << 16) | (data[3] << 8);
    *dest = (float)s / (float)(INT32_MAX - 0xFF);
  }
  return samples;
}

unsigned int CAEConvert::S32LE_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  int32_t *src = (int32_t*)data;
  for(unsigned int i = 0; i < samples; ++i, ++src, ++dest)
  {
#ifdef __BIG_ENDIAN__
    *dest = CLAMP((float)Endian_Swap32(*src) / (float)INT32_MAX);
#else
    *dest = CLAMP((float)*src / (float)INT32_MAX);
#endif
  }

  return samples;
}

unsigned int CAEConvert::S32BE_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  int32_t *src = (int32_t*)data;
  for(unsigned int i = 0; i < samples; ++i, ++src, ++dest)
  {
#ifndef __BIG_ENDIAN__
    *dest = CLAMP((float)Endian_Swap32(*src) / (float)INT32_MAX);
#else
    *dest = CLAMP((float)*src / (float)INT32_MAX);
#endif
  }

  return samples;
}

unsigned int CAEConvert::DOUBLE_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  double *src = (double*)data;
  for(unsigned int i = 0; i < samples; ++i, ++src, ++dest)
    *dest = CLAMP(*src / (float)INT32_MAX);

  return samples;
}

unsigned int CAEConvert::Float_U8(float *data, const unsigned int samples, uint8_t *dest)
{
  #ifdef __SSE__
  const __m128 mul = _mm_set_ps1((float)INT8_MAX+.5f);
  const __m128 add = _mm_set_ps1(1.0f);
  unsigned int count = samples;

  /* work around invalid alignment */
  while((((uintptr_t)data & 0xF) || ((uintptr_t)dest & 0xF)) && count > 0)
  {
    dest[0] = safeRound((data[0] + 1.0f) * ((float)INT8_MAX+.5f));
    ++data;
    ++dest;
    --count;
  }

  const uint32_t even = count & ~0x3;
  for(uint32_t i = 0; i < even; i += 4, data += 4, dest += 4)
  {
    __m128 in  = _mm_mul_ps(_mm_add_ps(_mm_load_ps(data), add), mul);
    __m64  con = _mm_cvtps_pi16(in);

    int16_t temp[4];
    memcpy(temp, &con, sizeof(temp));
    dest[0] = (uint8_t)temp[0];
    dest[1] = (uint8_t)temp[1];
    dest[2] = (uint8_t)temp[2];
    dest[3] = (uint8_t)temp[3];
  }

  if (count != even)
  {
    const uint32_t odd = count - even;
    if (odd == 1)
    {
      _mm_empty();
      dest[0] = safeRound((data[0] + 1.0f) * ((float)INT8_MAX+.5f));
    }
    else
    {
      __m128 in;
      if (odd == 2)
      {
        in = _mm_setr_ps(data[0], data[1], 0, 0);
        in = _mm_mul_ps(_mm_add_ps(_mm_load_ps(data), add), mul);
        __m64 con = _mm_cvtps_pi16(in);

        int16_t temp[2];
        memcpy(temp, &con, sizeof(temp));
        dest[0] = (uint8_t)temp[0];
        dest[1] = (uint8_t)temp[1];
      }
      else
      {
        in = _mm_setr_ps(data[0], data[1], data[2], 0);
        in = _mm_mul_ps(_mm_add_ps(_mm_load_ps(data), add), mul);
        __m64 con = _mm_cvtps_pi16(in);

        int16_t temp[3];
        memcpy(temp, &con, sizeof(temp));
        dest[0] = (uint8_t)temp[0];
        dest[1] = (uint8_t)temp[1];
        dest[2] = (uint8_t)temp[2];
      }
    }
  }
  _mm_empty();
  #else /* no SSE */
  for(uint32_t i = 0; i < samples; ++i, ++data, ++dest)
    dest[0] = safeRound((data[0] + 1.0f) * ((float)INT8_MAX+.5f));
  #endif

  return samples;
}

unsigned int CAEConvert::Float_S8(float *data, const unsigned int samples, uint8_t *dest)
{
  #ifdef __SSE__
  const __m128 mul = _mm_set_ps1((float)INT8_MAX+.5f);
  unsigned int count = samples;

  /* work around invalid alignment */
  while((((uintptr_t)data & 0xF) || ((uintptr_t)dest & 0xF)) && count > 0)
  {
    dest[0] = safeRound(data[0] * ((float)INT8_MAX+.5f));
    ++data;
    ++dest;
    --count;
  }

  const uint32_t even = count & ~0x3;
  for(uint32_t i = 0; i < even; i += 4, data += 4, dest += 4)
  {
    __m128 in  = _mm_mul_ps(_mm_load_ps(data), mul);
    __m64  con = _mm_cvtps_pi8(in);
    memcpy(dest, &con, 4);
  }

  if (count != even)
  {
    const uint32_t odd = count - even;
    if (odd == 1)
    {
      _mm_empty();
      dest[0] = safeRound(data[0] * ((float)INT8_MAX+.5f));
    }
    else
    {
      __m128 in;
      if (odd == 2)
      {
        in = _mm_setr_ps(data[0], data[1], 0, 0);
        in = _mm_mul_ps(_mm_load_ps(data), mul);
        __m64 con = _mm_cvtps_pi8(in);
        memcpy(dest, &con, 2);
      }
      else
      {
        in = _mm_setr_ps(data[0], data[1], data[2], 0);
        in = _mm_mul_ps(_mm_load_ps(data), mul);
        __m64 con = _mm_cvtps_pi8(in);
        memcpy(dest, &con, 3);
      }
    }
  }
  _mm_empty();
  #else /* no SSE */
  for(uint32_t i = 0; i < samples; ++i, ++data, ++dest)
    dest[0] = safeRound(data[0] * ((float)INT8_MAX+.5f));
  #endif

  return samples;
}

unsigned int CAEConvert::Float_S16LE(float *data, const unsigned int samples, uint8_t *dest)
{
  int16_t *dst = (int16_t*)dest;
  #ifdef __SSE__
  const __m128 mul = _mm_set_ps1((float)INT16_MAX+.5f);
  unsigned int count = samples;

  /* work around invalid alignment */
  while((((uintptr_t)data & 0xF) || ((uintptr_t)dest & 0xF)) && count > 0)
  {
    dst[0] = safeRound(data[0] * ((float)INT16_MAX+.5f));
    ++data;
    ++dst;
    --count;
  }

  const uint32_t even = count & ~0x3;
  for(uint32_t i = 0; i < even; i += 4, data += 4, dst += 4)
  {
    __m128 in = _mm_mul_ps(_mm_load_ps(data), mul);
    __m64 con = _mm_cvtps_pi16(in);
    memcpy(dst, &con, sizeof(int16_t) * 4);
    #ifdef __BIG_ENDIAN__
    dst[0] = Endian_Swap16(dst[0]);
    dst[1] = Endian_Swap16(dst[1]);
    dst[2] = Endian_Swap16(dst[2]);
    dst[3] = Endian_Swap16(dst[3]);
    #endif
  }

  if (samples != even)
  {
    const uint32_t odd = samples - even;
    if (odd == 1)
    {
      _mm_empty();
      dst[0] = safeRound(data[0] * ((float)INT16_MAX+.5f));
      #ifdef __BIG_ENDIAN__
      dst[0] = Endian_Swap16(dst[0]);
      #endif
    }
    else
    {
      __m128 in;
      if (odd == 2)
      {
        in = _mm_setr_ps(data[0], data[1], 0, 0);
        in = _mm_mul_ps(in, mul);
        __m64 con = _mm_cvtps_pi16(in);
        memcpy(dst, &con, sizeof(int16_t) * 2);
        #ifdef __BIG_ENDIAN__
        dst[0] = Endian_Swap16(dst[0]);
        dst[1] = Endian_Swap16(dst[1]);
        #endif
      }
      else
      {
        in = _mm_setr_ps(data[0], data[1], data[2], 0);
        in = _mm_mul_ps(in, mul);
        __m64 con = _mm_cvtps_pi16(in);
        memcpy(dst, &con, sizeof(int16_t) * 3);
        #ifdef __BIG_ENDIAN__
        dst[0] = Endian_Swap16(dst[0]);
        dst[1] = Endian_Swap16(dst[1]);
        dst[2] = Endian_Swap16(dst[2]);
        #endif
      }
    }
  }
  _mm_empty();
  #else /* no SSE */
  for(uint32_t i = 0; i < samples; ++i, ++data, ++dst)
  {
    dst[0] = safeRound(data[0] * ((float)INT16_MAX+.5f));
    #ifdef __BIG_ENDIAN__
    dst[0] = Endian_Swap16(dst[0]);
    #endif
  }
  #endif

  return samples << 1;
}

unsigned int CAEConvert::Float_S16BE(float *data, const unsigned int samples, uint8_t *dest)
{
  int16_t *dst = (int16_t*)dest;
  #ifdef __SSE__
  const __m128 mul = _mm_set_ps1((float)INT16_MAX+.5f);
  unsigned int count = samples;

  /* work around invalid alignment */
  while((((uintptr_t)data & 0xF) || ((uintptr_t)dest & 0xF)) && count > 0)
  {
    dst[0] = safeRound(data[0] * ((float)INT16_MAX+.5f));
    ++data;
    ++dst;
    --count;
  }

  const uint32_t even = count & ~0x3;
  for(uint32_t i = 0; i < even; i += 4, data += 4, dst += 4)
  {
    __m128 in = _mm_mul_ps(_mm_load_ps(data), mul);
    __m64 con = _mm_cvtps_pi16(in);
    memcpy(dst, &con, sizeof(int16_t) * 4);
    #ifndef __BIG_ENDIAN__
    dst[0] = Endian_Swap16(dst[0]);
    dst[1] = Endian_Swap16(dst[1]);
    dst[2] = Endian_Swap16(dst[2]);
    dst[3] = Endian_Swap16(dst[3]);
    #endif
  }

  if (samples != even)
  {
    const uint32_t odd = samples - even;
    if (odd == 1)
    {
      _mm_empty();
      dst[0] = safeRound(data[0] * ((float)INT16_MAX+.5f));
      #ifndef __BIG_ENDIAN__
      dst[0] = Endian_Swap16(dst[0]);
      #endif
    }
    else
    {
      __m128 in;
      if (odd == 2)
      {
        in = _mm_setr_ps(data[0], data[1], 0, 0);
        in = _mm_mul_ps(in, mul);
        __m64 con = _mm_cvtps_pi16(in);
        memcpy(dst, &con, sizeof(int16_t) * 2);
        #ifndef __BIG_ENDIAN__
        dst[0] = Endian_Swap16(dst[0]);
        dst[1] = Endian_Swap16(dst[1]);
        #endif
      }
      else
      {
        in = _mm_setr_ps(data[0], data[1], data[2], 0);
        in = _mm_mul_ps(in, mul);
        __m64 con = _mm_cvtps_pi16(in);
        memcpy(dst, &con, sizeof(int16_t) * 3);
        #ifndef __BIG_ENDIAN__
        dst[0] = Endian_Swap16(dst[0]);
        dst[1] = Endian_Swap16(dst[1]);
        dst[2] = Endian_Swap16(dst[2]);
        #endif
      }
    }
  }
  _mm_empty();
  #else /* no SSE */
  for(uint32_t i = 0; i < samples; ++i, ++data, ++dst)
  {
    dst[0] = safeRound(data[0] * ((float)INT16_MAX+.5f));
    #ifndef __BIG_ENDIAN__
    dst[0] = Endian_Swap16(dst[0]);
    #endif
  }
  #endif

  return samples << 1;
}

unsigned int CAEConvert::Float_S24NE4(float *data, const unsigned int samples, uint8_t *dest)
{
  int32_t *dst = (int32_t*)dest;
  #ifdef __SSE__

  const __m128 mul = _mm_set_ps1((float)INT24_MAX+.5f);
  unsigned int count = samples;

  /* work around invalid alignment */
  while((((uintptr_t)data & 0xF) || ((uintptr_t)dest & 0xF)) && count > 0)
  {
    dst[0] = safeRound(data[0] * ((float)INT24_MAX+.5f));
    ++data;
    ++dst;
    --count;
  }

  const uint32_t even = count & ~0x3;
  for(uint32_t i = 0; i < even; i += 4, data += 4, dst += 4)
  {
    __m128  in  = _mm_mul_ps(_mm_load_ps(data), mul);
    __m128i con = _mm_cvtps_epi32(in);
    con         = _mm_slli_epi32(con, 8);
    memcpy(dst, &con, sizeof(int32_t) * 4);
  }

  if (samples != even)
  {
    const uint32_t odd = samples - even;
    if (odd == 1)
      dst[0] = safeRound(data[0] * ((float)INT24_MAX+.5f));
    else
    {
      __m128 in;
      if (odd == 2)
      {
        in = _mm_setr_ps(data[0], data[1], 0, 0);
        in = _mm_mul_ps(in, mul);
        __m64 con = _mm_cvtps_pi32(in);
        con       = _mm_slli_pi32(con, 8);
        memcpy(dst, &con, sizeof(int32_t) * 2);
      }
      else
      {
        in = _mm_setr_ps(data[0], data[1], data[2], 0);
        in = _mm_mul_ps(in, mul);
        __m128i con = _mm_cvtps_epi32(in);
        con         = _mm_slli_epi32(con, 8);
        memcpy(dst, &con, sizeof(int32_t) * 3);
      }
    }
  }
  _mm_empty();
  #else /* no SSE */
  for(uint32_t i = 0; i < samples; ++i, ++data, ++dst)
    *dst = safeRound(*data * ((float)INT24_MAX+.5f)) & 0xFFFFFF;
  #endif

  return samples << 2;
}

unsigned int CAEConvert::Float_S24NE3(float *data, const unsigned int samples, uint8_t *dest)
{
  #ifdef __SSE__
  int32_t *dst = (int32_t*)dest;

  const __m128 mul = _mm_set_ps1((float)INT24_MAX+.5f);
  unsigned int count = samples;

  /* work around invalid alignment */
  while((((uintptr_t)data & 0xF) || ((uintptr_t)dest & 0xF)) && count > 0)
  {
    *((uint32_t*)(dest)) = (safeRound(*data * ((float)INT24_MAX+.5f)) & 0xFFFFFF) << 8;
    ++dest;
    --count;
  }

  const uint32_t even = count & ~0x3;
  for(uint32_t i = 0; i < count; i += 4, data += 4, dest += 12)
  {
    __m128  in  = _mm_mul_ps(_mm_load_ps(data), mul);
    __m128i con = _mm_cvtps_epi32(in);
    con         = _mm_slli_epi32(con, 8);
    memcpy(dst, &con, sizeof(int32_t) * 4);
    *((uint32_t*)(dest + 0)) = (dst[0] & 0xFFFFFF) << 8;
    *((uint32_t*)(dest + 3)) = (dst[1] & 0xFFFFFF) << 8;
    *((uint32_t*)(dest + 6)) = (dst[2] & 0xFFFFFF) << 8;
    *((uint32_t*)(dest + 9)) = (dst[3] & 0xFFFFFF) << 8;
  }

  if (samples != even)
  {
    const uint32_t odd = samples - even;
    if (odd == 1)
      dst[0] = safeRound(data[0] * ((float)INT24_MAX+.5f)) & 0xFFFFFF;
    else
    {
      __m128 in;
      if (odd == 2)
      {
        in = _mm_setr_ps(data[0], data[1], 0, 0);
        in = _mm_mul_ps(in, mul);
        __m64 con = _mm_cvtps_pi32(in);
        con       = _mm_slli_pi32(con, 8);
        memcpy(dst, &con, sizeof(int32_t) * 2);
        *((uint32_t*)(dest + 0)) = (dst[0] & 0xFFFFFF) << 8;
        *((uint32_t*)(dest + 3)) = (dst[1] & 0xFFFFFF) << 8;
      }
      else
      {
        in = _mm_setr_ps(data[0], data[1], data[2], 0);
        in = _mm_mul_ps(in, mul);
        __m128i con = _mm_cvtps_epi32(in);
        con         = _mm_slli_epi32(con, 8);
        memcpy(dst, &con, sizeof(int32_t) * 3);
        *((uint32_t*)(dest + 0)) = (dst[0] & 0xFFFFFF) << 8;
        *((uint32_t*)(dest + 3)) = (dst[1] & 0xFFFFFF) << 8;
        *((uint32_t*)(dest + 6)) = (dst[2] & 0xFFFFFF) << 8;
      }
    }
  }
  _mm_empty();
  #else /* no SSE */
  for(uint32_t i = 0; i < samples; ++i, ++data, dest += 3)
    *((uint32_t*)(dest)) = (safeRound(*data * ((float)INT24_MAX+.5f)) & 0xFFFFFF) << 8;
  #endif

  return samples * 3;
}

unsigned int CAEConvert::Float_S32LE(float *data, const unsigned int samples, uint8_t *dest)
{
  int32_t *dst = (int32_t*)dest;
  #ifdef __SSE__
  const __m128 mul = _mm_set_ps1((float)INT32_MAX);
  unsigned int count = samples;

  /* work around invalid alignment */
  while((((uintptr_t)data & 0xF) || ((uintptr_t)dest & 0xF)) && count > 0)
  {
    dst[0] = safeRound(data[0] * (float)INT32_MAX);
    ++data;
    ++dst;
    --count;
  }

  const uint32_t even = count & ~0x3;
  for(uint32_t i = 0; i < even; i += 4, data += 4, dst += 4)
  {
    __m128  in  = _mm_mul_ps(_mm_load_ps(data), mul);
    __m128i con = _mm_cvtps_epi32(in);
    memcpy(dst, &con, sizeof(int32_t) * 4);
    #ifdef __BIG_ENDIAN__
    dst[0] = Endian_Swap16(dst[0]);
    dst[1] = Endian_Swap16(dst[1]);
    dst[2] = Endian_Swap16(dst[2]);
    dst[3] = Endian_Swap16(dst[3]);
    #endif
  }

  if (samples != even)
  {
    const uint32_t odd = samples - even;
    if (odd == 1)
    {
      dst[0] = safeRound(data[0] * (float)INT32_MAX);
      #ifdef __BIG_ENDIAN__
      dst[0] = Endian_Swap32(dst[0]);
      #endif
    }
    else
    {
      __m128 in;
      if (odd == 2)
      {
        in = _mm_setr_ps(data[0], data[1], 0, 0);
        in = _mm_mul_ps(in, mul);
        __m64 con = _mm_cvtps_pi32(in);
        memcpy(dst, &con, sizeof(int32_t) * 2);
        #ifdef __BIG_ENDIAN__
        dst[0] = Endian_Swap32(dst[0]);
        dst[1] = Endian_Swap32(dst[1]);
        #endif
      }
      else
      {
        in = _mm_setr_ps(data[0], data[1], data[2], 0);
        in = _mm_mul_ps(in, mul);
        __m128i con = _mm_cvtps_epi32(in);
        memcpy(dst, &con, sizeof(int32_t) * 3);
        #ifdef __BIG_ENDIAN__
        dst[0] = Endian_Swap32(dst[0]);
        dst[1] = Endian_Swap32(dst[1]);
        dst[2] = Endian_Swap32(dst[2]);
        #endif
      }
    }
  }
  _mm_empty();
  #else /* no SSE */
  for(uint32_t i = 0; i < samples; ++i, ++data, ++dst)
  {
    dst[0] = safeRound(data[0] * (float)INT32_MAX);

    #ifdef __BIG_ENDIAN__
    dst[0] = Endian_Swap32(dst[0]);
    #endif
  }
  #endif

  return samples << 2;
}

unsigned int CAEConvert::Float_S32BE(float *data, const unsigned int samples, uint8_t *dest)
{
  int32_t *dst = (int32_t*)dest;
  #ifdef __SSE__
  const __m128 mul = _mm_set_ps1((float)INT32_MAX);
  unsigned int count = samples;

  /* work around invalid alignment */
  while((((uintptr_t)data & 0xF) || ((uintptr_t)dest & 0xF)) && count > 0)
  {
    dst[0] = safeRound(data[0] * (float)INT32_MAX);
    ++data;
    ++dst;
    --count;
  }

  const uint32_t even = count & ~0x3;
  for(uint32_t i = 0; i < even; i += 4, data += 4, dst += 4)
  {
    __m128  in  = _mm_mul_ps(_mm_load_ps(data), mul);
    __m128i con = _mm_cvtps_epi32(in);
    memcpy(dst, &con, sizeof(int32_t) * 4);
    #ifndef __BIG_ENDIAN__
    dst[0] = Endian_Swap16(dst[0]);
    dst[1] = Endian_Swap16(dst[1]);
    dst[2] = Endian_Swap16(dst[2]);
    dst[3] = Endian_Swap16(dst[3]);
    #endif
  }

  if (samples != even)
  {
    const uint32_t odd = samples - even;
    if (odd == 1)
    {
      dst[0] = safeRound(data[0] * (float)INT32_MAX);
      #ifndef __BIG_ENDIAN__
      dst[0] = Endian_Swap32(dst[0]);
      #endif
    }
    else
    {
      __m128 in;
      if (odd == 2)
      {
        in = _mm_setr_ps(data[0], data[1], 0, 0);
        in = _mm_mul_ps(in, mul);
        __m64 con = _mm_cvtps_pi32(in);
        memcpy(dst, &con, sizeof(int32_t) * 2);
        #ifndef __BIG_ENDIAN__
        dst[0] = Endian_Swap32(dst[0]);
        dst[1] = Endian_Swap32(dst[1]);
        #endif
      }
      else
      {
        in = _mm_setr_ps(data[0], data[1], data[2], 0);
        in = _mm_mul_ps(in, mul);
        __m128i con = _mm_cvtps_epi32(in);
        memcpy(dst, &con, sizeof(int32_t) * 3);
        #ifndef __BIG_ENDIAN__
        dst[0] = Endian_Swap32(dst[0]);
        dst[1] = Endian_Swap32(dst[1]);
        dst[2] = Endian_Swap32(dst[2]);
        #endif
      }
    }
  }
  _mm_empty();
  #else /* no SSE */
  for(uint32_t i = 0; i < samples; ++i, ++data, ++dst)
  {
    dst[0] = safeRound(data[0] * (float)INT32_MAX);

    #ifndef __BIG_ENDIAN__
    dst[0] = Endian_Swap32(dst[0]);
    #endif
  }
  #endif

  return samples << 2;
}

unsigned int CAEConvert::Float_DOUBLE(float *data, const unsigned int samples, uint8_t *dest)
{
  double *dst = (double*)dest;
  for(unsigned int i = 0; i < samples; ++i, ++data, ++dst)
    *dst = *data;

  return samples * sizeof(double);
}

