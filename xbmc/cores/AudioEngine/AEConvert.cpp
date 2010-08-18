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

CAEConvert::AEConvertToFn CAEConvert::ToFloat(enum AEDataFormat dataFormat)
{
  switch(dataFormat)
  {
    case AE_FMT_U8   : return &U8_Float;
    case AE_FMT_S16LE: return &S16LE_Float;
    case AE_FMT_S16BE: return &S16BE_Float;
    case AE_FMT_S24BE: return &S24BE_Float;
    default:
      return NULL;
  }
}

CAEConvert::AEConvertFrFn CAEConvert::FrFloat(enum AEDataFormat dataFormat)
{
  switch(dataFormat)
  {
    case AE_FMT_U8   : return &Float_U8;
    case AE_FMT_S8   : return &Float_S8;
    case AE_FMT_S16LE: return &Float_S16LE;
    case AE_FMT_S16BE: return &Float_S16BE;
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

unsigned int CAEConvert::S16LE_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  unsigned int i;
  for(i = 0; i < samples; ++i, data += 2, ++dest)
  {
#ifndef __BIG_ENDIAN__
    *dest = AELookupS16toFloat[*(int16_t*)data + 32768];
#else
    int16_t value;
    swab(data, &value, 2);
    *dest = AELookupS16toFloat[value + 32768];
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
    *(int32_t*)dest = AELookupS16toFloat[*(int16_t*)data + 32768];
#else
    int16_t value;
    swab(data, &value, 2);
    *(int32_t*)dest = AELookupS16toFloat[value + 32768];
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
    dest[0] = ((data[0] << 16) | (data[1] << 8) | data[ 8]) / 167772155.0f;
    dest[1] = ((data[2] << 16) | (data[3] << 8) | data[ 9]) / 167772155.0f;
    dest[2] = ((data[4] << 16) | (data[5] << 8) | data[10]) / 167772155.0f;
    dest[3] = ((data[6] << 16) | (data[7] << 8) | data[11]) / 167772155.0f;
  }

  return viable;
}

unsigned int CAEConvert::Float_U8(float *data, const unsigned int samples, uint8_t *dest)
{
  #ifdef __SSE__
  const uint32_t even = (samples / 4) * 4;
  const __m128 mul = _mm_set_ps1(INT8_MAX+.5f);
  const __m128 add = _mm_set_ps1(1.0f);

  for(uint32_t i = 0; i < even; i += 4, data += 4, dest += 4)
  {
    __m128 val = _mm_mul_ps(_mm_add_ps(_mm_load_ps(data), add), mul);
    __m64  con = _mm_cvtps_pi16(val);

    /* this is needed as there is no SSE instruction to return unsigned 8bit ints */
    int16_t *tmp = (int16_t*)&con;
    dest[0] = tmp[0];
    dest[1] = tmp[1];
    dest[2] = tmp[2];
    dest[3] = tmp[3];
  }

  if (samples != even)
  {
    const uint32_t odd = samples - even;
    if (odd == 1)
      *dest = MathUtils::round_int((*data + 1.0f) * (INT8_MAX+.5f));
    else
    {
      __m128 in;
      memcpy(&in, data, sizeof(float) * odd);
      __m128 val = _mm_mul_ps(_mm_add_ps(in, add), mul);
      __m64  con = _mm_cvtps_pi16(val);

      /* this is needed as there is no SSE instruction to return unsigned 8bit ints */
      int16_t *tmp = (int16_t*)&con;
      uint8_t buf[4];
      buf[0] = tmp[0];
      buf[1] = tmp[1];
      buf[2] = tmp[2];
      buf[3] = tmp[3];
      memcpy(dest, buf, odd);
    }
  }
  #else /* no SSE */
  for(uint32_t i = 0; i < samples; ++i, ++data, ++dest)
    *dest = MathUtils::round_int((*data + 1.0f) * (INT8_MAX+.5f));
  #endif

  return samples;
}

unsigned int CAEConvert::Float_S8(float *data, const unsigned int samples, uint8_t *dest)
{
  #ifdef __SSE__
  const uint32_t even = (samples / 4) * 4;
  const __m128 mul = _mm_set_ps1(INT8_MAX+.5f);

  for(uint32_t i = 0; i < even; i += 4, data += 4, dest += 4)
  {
    __m128 val = _mm_mul_ps(_mm_load_ps(data), mul);
    __m64  con = _mm_cvtps_pi8(val);
    memcpy(dest, &con, 4);
  }

  if (samples != even)
  {
    const uint32_t odd = samples - even;
    if (odd == 1)
      *dest = MathUtils::round_int(*data * (INT8_MAX+.5f));
    else
    {
      __m128 in;
      memcpy(&in, data, sizeof(float) * odd);
      __m128 val = _mm_mul_ps(in, mul);
      __m64  con = _mm_cvtps_pi8(val);
      memcpy(dest, &con, odd);
    }
  }
  #else /* no SSE */
  for(uint32_t i = 0; i < samples; ++i, ++data, ++dest)
    *dest = MathUtils::round_int(*data * (INT8_MAX+.5f));
  #endif

  return samples;
}



unsigned int CAEConvert::Float_S16LE(float *data, const unsigned int samples, uint8_t *dest)
{
  int16_t *dst = (int16_t*)dest;
  #ifdef __SSE__
  const uint32_t even = (samples / 4) * 4;
  const __m128 mul = _mm_set_ps1(INT16_MAX+.5f);

  for(uint32_t i = 0; i < even; i += 4, data += 4, dst += 4)
  {
    __m128 val = _mm_mul_ps(_mm_load_ps(data), mul);
    *((__m64*)dst) = _mm_cvtps_pi16(val);
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
      *dst = MathUtils::round_int(*data * (INT16_MAX+.5f));
      #ifdef __BIG_ENDIAN__
      *dst = Endian_Swap16(*dst);
      #endif
    }
    else
    {
      __m128 in;
      memcpy(&in, data, sizeof(float) * odd);
      __m128 val = _mm_mul_ps(in, mul);
      __m64  con = _mm_cvtps_pi16(val);
      #ifdef __BIG_ENDIAN__
      int16_t *swap = (int16_t*)&con;
      swap[0] = Endian_Swap16(swap[0]);
      swap[1] = Endian_Swap16(swap[1]);
      swap[2] = Endian_Swap16(swap[2]);
      swap[3] = Endian_Swap16(swap[3]);
      #endif
      memcpy(dst, &con, sizeof(int16_t) * odd);
    }
  }
  #else /* no SSE */
  for(uint32_t i = 0; i < samples; ++i, ++data, ++dst)
  {
    *dst = MathUtils::round_int(*data * (INT16_MAX+.5f));
    #ifdef __BIG_ENDIAN__
    *dst = Endian_Swap16(*dst);
    #endif
  }
  #endif

  return samples << 1;
}

unsigned int CAEConvert::Float_S16BE(float *data, const unsigned int samples, uint8_t *dest)
{
  int16_t *dst = (int16_t*)dest;
  #ifdef __SSE__
  const uint32_t even = (samples / 4) * 4;
  const __m128 mul = _mm_set_ps1(INT16_MAX+.5f);

  for(uint32_t i = 0; i < even; i += 4, data += 4, dst += 4)
  {
    __m128 val = _mm_mul_ps(_mm_load_ps(data), mul);
    *((__m64*)dst) = _mm_cvtps_pi16(val);
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
      *dst = MathUtils::round_int(*data * (INT16_MAX+.5f));
      #ifndef __BIG_ENDIAN__
      *dst = Endian_Swap16(*dst);
      #endif
    }
    else
    {
      __m128 in;
      memcpy(&in, data, sizeof(float) * odd);
      __m128 val = _mm_mul_ps(in, mul);
      __m64  con = _mm_cvtps_pi16(val);
      #ifndef __BIG_ENDIAN__
      int16_t *swap = (int16_t*)&con;
      swap[0] = Endian_Swap16(swap[0]);
      swap[1] = Endian_Swap16(swap[1]);
      swap[2] = Endian_Swap16(swap[2]);
      swap[3] = Endian_Swap16(swap[3]);
      #endif
      memcpy(dst, &con, sizeof(int16_t) * odd);
    }
  }
  #else /* no SSE */
  for(uint32_t i = 0; i < samples; ++i, ++data, ++dst)
  {
    *dst = MathUtils::round_int(*data * (INT16_MAX+.5f));
    #ifndef __BIG_ENDIAN__
    *dst = Endian_Swap16(*dst);
    #endif
  }
  #endif

  return samples << 1;
}

