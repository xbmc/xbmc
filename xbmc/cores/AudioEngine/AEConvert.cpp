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

#include "AEConvert.h"
#include "AEUtil.h"
#include "AELookupU8.h"
#include "AELookupS16.h"
#include "MathUtils.h"
#include "utils/EndianSwap.h"

#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#ifdef __SSE__
#include <xmmintrin.h>
#endif

#define CLAMP(x) std::min(-1.0f, std::max(1.0f, (float)(x)))

CAEConvert::AEConvertToFn CAEConvert::ToFloat(enum AEDataFormat dataFormat)
{
  switch(dataFormat)
  {
    case AE_FMT_U8    : return &U8_Float;
    case AE_FMT_S8    : return &S8_Float;
#ifdef __BIG_ENDIAN__
    case AE_FMT_S16NE : return &S16BE_Float;
    case AE_FMT_S32NE : return &S32BE_Float;
#else
    case AE_FMT_S16NE : return &S16LE_Float;
    case AE_FMT_S32NE : return &S32LE_Float;
#endif
    case AE_FMT_S16LE : return &S16LE_Float;
    case AE_FMT_S16BE : return &S16BE_Float;
    case AE_FMT_S24BE : return &S24BE_Float;
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
    *dest = AELookupU8toFloat[*data];

  return samples;
}

unsigned int CAEConvert::S8_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  unsigned int i;
  for(i = 0; i < samples; ++i, ++data, ++dest)
    *dest = AELookupU8toFloat[(int8_t)*data - INT8_MAX];

  return samples;
}

unsigned int CAEConvert::S16LE_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  unsigned int i;
  for(i = 0; i < samples; ++i, data += 2, ++dest)
  {
#ifndef __BIG_ENDIAN__
    *dest = AELookupS16toFloat[*(int16_t*)data + INT16_MAX];
#else
    int16_t value;
    swab(data, &value, 2);
    *dest = AELookupS16toFloat[value + INT16_MAX];
#endif
    if (*dest > 1.0 || *dest < -1.0)
      printf("%f\n", *dest);
  }

  return samples;
}

unsigned int CAEConvert::S16BE_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  unsigned int i;
  for(i = 0; i < samples; ++i, data += 2, ++dest)
  {
#ifdef __BIG_ENDIAN__
    *(int32_t*)dest = AELookupS16toFloat[*(int16_t*)data + INT16_MAX];
#else
    int16_t value;
    swab(data, &value, 2);
    *(int32_t*)dest = AELookupS16toFloat[value + INT16_MAX];
#endif
  }

  return samples;
}

unsigned int CAEConvert::S24BE_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  unsigned int i;
  unsigned int viable = samples / 12;

  /* http://wiki.multimedia.cx/index.php?title=PCM#24-Bit_PCM */
  for(i = 0; i < viable; i += 4, data += 12, dest += 4)
  {
    dest[0] = CLAMP((float)((data[0] << 16) | (data[1] << 8) | data[ 8]) / 167772155.0f);
    dest[1] = CLAMP((float)((data[2] << 16) | (data[3] << 8) | data[ 9]) / 167772155.0f);
    dest[2] = CLAMP((float)((data[4] << 16) | (data[5] << 8) | data[10]) / 167772155.0f);
    dest[3] = CLAMP((float)((data[6] << 16) | (data[7] << 8) | data[11]) / 167772155.0f);
  }

  return viable;
}

unsigned int CAEConvert::S32LE_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  int32_t *src = (int32_t*)data;
  for(unsigned int i = 0; i < samples; ++i, ++src, ++dest)
  {
#ifdef __BIG_ENDIAN__
    *dest = CLAMP((float)Endian_Swap32(*src) / INT32_MAX);
#else
    *dest = CLAMP((float)*src / INT32_MAX);
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
    *dest = CLAMP((float)Endian_Swap32(*src) / INT32_MAX);
#else
    *dest = CLAMP((float)*src / INT32_MAX);
#endif
  }

  return samples;
}

unsigned int CAEConvert::DOUBLE_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  double *src = (double*)data;
  for(unsigned int i = 0; i < samples; ++i, ++src, ++dest)
    *dest = CLAMP(*src / INT32_MAX);

  return samples;
}

unsigned int CAEConvert::Float_U8(float *data, const unsigned int samples, uint8_t *dest)
{
  #ifdef __SSE__
  const __m128 mul = _mm_set_ps1(INT8_MAX+.5f);
  const __m128 add = _mm_set_ps1(1.0f);
  unsigned int count = samples;

  /* work around invalid alignment */
  while((((uintptr_t)data & 0xF) || ((uintptr_t)dest & 0xF)) && count > 0)
  {
    dest[0] = MathUtils::round_int((data[0] + 1.0f) * (INT8_MAX+.5f));
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
    dest[0] = temp[0];
    dest[1] = temp[1];
    dest[2] = temp[2];
    dest[3] = temp[3];
  }

  if (count != even)
  {
    const uint32_t odd = count - even;
    if (odd == 1)
      dest[0] = MathUtils::round_int((data[0] + 1.0f) * (INT8_MAX+.5f));
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
        dest[0] = temp[0];
        dest[1] = temp[1];
      }
      else
      {
        in = _mm_setr_ps(data[0], data[1], data[2], 0);
        in = _mm_mul_ps(_mm_add_ps(_mm_load_ps(data), add), mul);
        __m64 con = _mm_cvtps_pi16(in);

        int16_t temp[3];
        memcpy(temp, &con, sizeof(temp));
        dest[0] = temp[0];
        dest[1] = temp[1];
        dest[2] = temp[2];
      }
    }
  }

  #else /* no SSE */
  for(uint32_t i = 0; i < samples; ++i, ++data, ++dest)
    dest[0] = MathUtils::round_int((data[0] + 1.0f) * (INT8_MAX+.5f));
  #endif

  return samples;
}

unsigned int CAEConvert::Float_S8(float *data, const unsigned int samples, uint8_t *dest)
{
  #ifdef __SSE__
  const __m128 mul = _mm_set_ps1(INT8_MAX+.5f);
  unsigned int count = samples;

  /* work around invalid alignment */
  while((((uintptr_t)data & 0xF) || ((uintptr_t)dest & 0xF)) && count > 0)
  {
    dest[0] = MathUtils::round_int(data[0] * (INT8_MAX+.5f));
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
      dest[0] = MathUtils::round_int(data[0] * (INT8_MAX+.5f));
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

  #else /* no SSE */
  for(uint32_t i = 0; i < samples; ++i, ++data, ++dest)
    dest[0] = MathUtils::round_int(data[0] * (INT8_MAX+.5f));
  #endif

  return samples;
}

unsigned int CAEConvert::Float_S16LE(float *data, const unsigned int samples, uint8_t *dest)
{
  int16_t *dst = (int16_t*)dest;
  #ifdef __SSE__
  const __m128 mul = _mm_set_ps1(INT16_MAX+.5f);
  unsigned int count = samples;

  /* work around invalid alignment */
  while((((uintptr_t)data & 0xF) || ((uintptr_t)dest & 0xF)) && count > 0)
  {
    dst[0] = MathUtils::round_int(data[0] * (INT16_MAX+.5f));
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
      dst[0] = MathUtils::round_int(data[0] * (INT16_MAX+.5f));
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
  #else /* no SSE */
  for(uint32_t i = 0; i < samples; ++i, ++data, ++dst)
  {
    dst[0] = MathUtils::round_int(data[0] * (INT16_MAX+.5f));
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
  const __m128 mul = _mm_set_ps1(INT16_MAX+.5f);
  unsigned int count = samples;

  /* work around invalid alignment */
  while((((uintptr_t)data & 0xF) || ((uintptr_t)dest & 0xF)) && count > 0)
  {
    dst[0] = MathUtils::round_int(data[0] * (INT16_MAX+.5f));
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
      dst[0] = MathUtils::round_int(data[0] * (INT16_MAX+.5f));
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
  #else /* no SSE */
  for(uint32_t i = 0; i < samples; ++i, ++data, ++dst)
  {
    dst[0] = MathUtils::round_int(data[0] * (INT16_MAX+.5f));
    #ifndef __BIG_ENDIAN__
    dst[0] = Endian_Swap16(dst[0]);
    #endif
  }
  #endif

  return samples << 1;
}

unsigned int CAEConvert::Float_S32LE(float *data, const unsigned int samples, uint8_t *dest)
{
  int32_t *dst = (int32_t*)dest;
  #ifdef __SSE__
  const __m128 mul = _mm_set_ps1(INT32_MAX+.5f);
  unsigned int count = samples;

  /* work around invalid alignment */
  while((((uintptr_t)data & 0xF) || ((uintptr_t)dest & 0xF)) && count > 0)
  {
    dst[0] = MathUtils::round_int(data[0] * (INT32_MAX+.5f));
    ++data;
    ++dst;
    --count;
  }

  const uint32_t even = count & ~0x3;
  for(uint32_t i = 0; i < even; i += 2, data += 4, dst += 4)
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
      dst[0] = MathUtils::round_int(data[0] * (INT32_MAX+.5f));
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
  #else /* no SSE */
  for(uint32_t i = 0; i < samples; ++i, ++data, ++dst)
  {
    dst[0] = MathUtils::round_int(data[0] * (INT32_MAX+.5f));
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
  const __m128 mul = _mm_set_ps1(INT32_MAX+.5f);
  unsigned int count = samples;

  /* work around invalid alignment */
  while((((uintptr_t)data & 0xF) || ((uintptr_t)dest & 0xF)) && count > 0)
  {
    dst[0] = MathUtils::round_int(data[0] * (INT32_MAX+.5f));
    ++data;
    ++dst;
    --count;
  }

  const uint32_t even = count & ~0x3;
  for(uint32_t i = 0; i < even; i += 2, data += 4, dst += 4)
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
      dst[0] = MathUtils::round_int(data[0] * (INT32_MAX+.5f));
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
  #else /* no SSE */
  for(uint32_t i = 0; i < samples; ++i, ++data, ++dst)
  {
    dst[0] = MathUtils::round_int(data[0] * (INT32_MAX+.5f));
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

