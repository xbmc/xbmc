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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#ifndef __STDC_LIMIT_MACROS
  #define __STDC_LIMIT_MACROS
#endif

#include "AEConvert.h"
#include "AEUtil.h"
#include "utils/MathUtils.h"
#include "utils/EndianSwap.h"
#include <stdint.h>

#if defined(TARGET_WINDOWS)
#include <unistd.h>
#endif
#include <math.h>
#include <string.h>

#ifdef __SSE__
#include <xmmintrin.h>
#include <emmintrin.h>
#endif

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#define CLAMP(x) std::min(-1.0f, std::max(1.0f, (float)(x)))

#ifndef INT24_MAX
#define INT24_MAX (0x7FFFFF)
#endif

#define INT32_SCALE (-1.0f / INT_MIN)

static inline int safeRound(double f)
{
  /* if the value is larger then we can handle, then clamp it */
  if (f >= INT_MAX)
    return INT_MAX;
  if (f <= INT_MIN)
    return INT_MIN;

  /* if the value is out of the MathUtils::round_int range, then round it normally */
  if (f <= static_cast<double>(INT_MIN / 2) - 1.0 || f >= static_cast <double>(INT_MAX / 2) + 1.0)
    return (int)floor(f+0.5);

  return MathUtils::round_int(f);
}

CAEConvert::AEConvertToFn CAEConvert::ToFloat(enum AEDataFormat dataFormat)
{
  switch (dataFormat)
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
#if defined(__ARM_NEON__)
    case AE_FMT_S32LE : return &S32LE_Float_Neon;
    case AE_FMT_S32BE : return &S32BE_Float_Neon;
#else
    case AE_FMT_S32LE : return &S32LE_Float;
    case AE_FMT_S32BE : return &S32BE_Float;
#endif
    case AE_FMT_DOUBLE: return &DOUBLE_Float;
    default:
      return NULL;
  }
}

CAEConvert::AEConvertFrFn CAEConvert::FrFloat(enum AEDataFormat dataFormat)
{
  switch (dataFormat)
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
#if defined(__ARM_NEON__)
    case AE_FMT_S32LE : return &Float_S32LE_Neon;
    case AE_FMT_S32BE : return &Float_S32BE_Neon;
#else
    case AE_FMT_S32LE : return &Float_S32LE;
    case AE_FMT_S32BE : return &Float_S32BE;
#endif
    case AE_FMT_DOUBLE: return &Float_DOUBLE;
    default:
      return NULL;
  }
}

unsigned int CAEConvert::U8_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  const float mul = 2.0f / UINT8_MAX;

  for (unsigned int i = 0; i < samples; ++i)
    *dest++ = *data++ * mul - 1.0f;

  return samples;
}

unsigned int CAEConvert::S8_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  const float mul = 1.0f / (INT8_MAX + 0.5f);

  for (unsigned int i = 0; i < samples; ++i)
    *dest++ = *data++ * mul;

  return samples;
}

unsigned int CAEConvert::S16LE_Float(uint8_t* data, const unsigned int samples, float *dest)
{
  static const float mul = 1.0f / (INT16_MAX + 0.5f);

#if defined(__ARM_NEON__) || defined(__VFP_FP__)
  for (unsigned int i = 0; i < samples; i++)
  {
    __asm__ __volatile__ (
                          "ldrsh r1,[%[in]]       \n\t" // Read a halfword from the source address
#ifdef __BIG_ENDIAN__
                          "revsh r1,r1           \n\t" // Swap byte order
#endif
                          "vmov s1,r1             \n\t" // Copy input into a fp working register
                          "fsitos s1,s1           \n\t" // Convert from signed int to float (single)
                          "vmul.F32 s1,s1,%[mul]  \n\t" // Scale
                          "vstr.32 s1, [%[out]]   \n\t" // Transfer the result from the coprocessor
                          :                                                      // Outputs
                          : [in] "r" (data), [out] "r" (dest), [mul] "w" (mul)   // Inputs
                          : "s1","r1"                                            // Clobbers
                          );
    data+=2;
    dest++;
  }
#else
  for (unsigned int i = 0; i < samples; ++i, data += 2)
    *dest++ = Endian_SwapLE16(*(int16_t*)data) * mul;
#endif

  return samples;
}

unsigned int CAEConvert::S16BE_Float(uint8_t* data, const unsigned int samples, float *dest)
{
  static const float mul = 1.0f / (INT16_MAX + 0.5f);

#if defined(__ARM_NEON__) || defined(__VFP_FP__)
  for (unsigned int i = 0; i < samples; i++)
  {
    __asm__ __volatile__ (
                          "ldrsh r1,[%[in]]      \n\t" // Read a halfword from the source address
#ifndef __BIG_ENDIAN__
                          "revsh r1,r1           \n\t" // Swap byte order
#endif
                          "vmov s1,r1            \n\t" // Copy input into a fp working register
                          "fsitos s1,s1          \n\t" // Convert from signed int to float (single)
                          "vmul.F32 s1,s1,%[mul] \n\t" // Scale
                          "vstr.32 s1, [%[out]]  \n\t" // Transfer the result from the coprocessor
                          :                                                     // Outputs
                          : [in] "r" (data), [out] "r" (dest), [mul] "w" (mul)  // Inputs
                          : "s1","r1"                                           // Clobbers
                          );
    data+=2;
    dest++;
  }
#else
  for (unsigned int i = 0; i < samples; ++i, data += 2)
    *dest++ = Endian_SwapBE16(*(int16_t*)data) * mul;
#endif

  return samples;
}

unsigned int CAEConvert::S24LE4_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  for (unsigned int i = 0; i < samples; ++i, data += 4)
  {
    int s = (data[2] << 24) | (data[1] << 16) | (data[0] << 8);
    *dest++ = (float)s * INT32_SCALE;
  }
  return samples;
}

unsigned int CAEConvert::S24BE4_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  for (unsigned int i = 0; i < samples; ++i, data += 4)
  {
    int s = (data[0] << 24) | (data[1] << 16) | (data[2] << 8);
    *dest++ = (float)s * INT32_SCALE;
  }
  return samples;
}

unsigned int CAEConvert::S24LE3_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  for (unsigned int i = 0; i < samples; ++i, data += 3)
  {
    int s = (data[2] << 24) | (data[1] << 16) | (data[0] << 8);
    *dest++ = (float)s * INT32_SCALE;
  }
  return samples;
}

unsigned int CAEConvert::S24BE3_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  for (unsigned int i = 0; i < samples; ++i, data += 3)
  {
    int s = (data[1] << 24) | (data[2] << 16) | (data[3] << 8);
    *dest++ = (float)s * INT32_SCALE;
  }
  return samples;
}

unsigned int CAEConvert::S32LE_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  static const float factor = 1.0f / (float)INT32_MAX;
  int32_t *src = (int32_t*)data;

  /* do this in groups of 4 to give the compiler a better chance of optimizing this */
  for (float *end = dest + (samples & ~0x3); dest < end;)
  {
    *dest++ = (float)Endian_SwapLE32(*src++) * factor;
    *dest++ = (float)Endian_SwapLE32(*src++) * factor;
    *dest++ = (float)Endian_SwapLE32(*src++) * factor;
    *dest++ = (float)Endian_SwapLE32(*src++) * factor;
  }

  /* process any remaining samples */
  for (float *end = dest + (samples & 0x3); dest < end;)
    *dest++ = (float)Endian_SwapLE32(*src++) * factor;

  return samples;
}

unsigned int CAEConvert::S32LE_Float_Neon(uint8_t *data, const unsigned int samples, float *dest)
{
#if defined(__ARM_NEON__)
  static const float factor = 1.0f / (float)INT32_MAX;
  int32_t *src = (int32_t*)data;

  /* groups of 4 samples */
  for (float *end = dest + (samples & ~0x3); dest < end; src += 4, dest += 4)
  {
    int32x4_t val = vld1q_s32(src);
    #ifdef __BIG_ENDIAN__
    val = vrev64q_s32(val);
    #endif
    float32x4_t ret = vmulq_n_f32(vcvtq_f32_s32(val), factor);
    vst1q_f32((float32_t*)dest, ret);
  }

  /* if there are >= 2 remaining samples */
  if (samples & 0x2)
  {
    int32x2_t val = vld1_s32(src);
    #ifdef __BIG_ENDIAN__
    val = vrev64_s32(val);
    #endif
    float32x2_t ret = vmul_n_f32(vcvt_f32_s32(val), factor);
    vst1_f32((float32_t *)dest, ret);
    src  += 2;
    dest += 2;
  }

  /* if there is one remaining sample */
  if (samples & 0x1)
    dest[0] = (float)src[0] * factor;

#endif /* !defined(__ARM_NEON__) */
  return samples;
}

unsigned int CAEConvert::S32BE_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  static const float factor = 1.0f / (float)INT32_MAX;
  int32_t *src = (int32_t*)data;

  /* do this in groups of 4 to give the compiler a better chance of optimizing this */
  for (float *end = dest + (samples & ~0x3); dest < end;)
  {
    *dest++ = (float)Endian_SwapBE32(*src++) * factor;
    *dest++ = (float)Endian_SwapBE32(*src++) * factor;
    *dest++ = (float)Endian_SwapBE32(*src++) * factor;
    *dest++ = (float)Endian_SwapBE32(*src++) * factor;
  }

  /* process any remaining samples */
  for (float *end = dest + (samples & 0x3); dest < end;)
    *dest++ = (float)Endian_SwapBE32(*src++) * factor;

  return samples;
}

unsigned int CAEConvert::S32BE_Float_Neon(uint8_t *data, const unsigned int samples, float *dest)
{
#if defined(__ARM_NEON__)
  static const float factor = 1.0f / (float)INT32_MAX;
  int32_t *src = (int32_t*)data;

  /* groups of 4 samples */
  for (float *end = dest + (samples & ~0x3); dest < end; src += 4, dest += 4)
  {
    int32x4_t val = vld1q_s32(src);
    #ifndef __BIG_ENDIAN__
    val = vrev64q_s32(val);
    #endif
    float32x4_t ret = vmulq_n_f32(vcvtq_f32_s32(val), factor);
    vst1q_f32((float32_t *)dest, ret);
  }

  /* if there are >= 2 remaining samples */
  if (samples & 0x2)
  {
    int32x2_t val = vld1_s32(src);
    #ifndef __BIG_ENDIAN__
    val = vrev64_s32(val);
    #endif
    float32x2_t ret = vmul_n_f32(vcvt_f32_s32(val), factor);
    vst1_f32((float32_t *)dest, ret);
    src  += 2;
    dest += 2;
  }

  /* if there is one remaining sample */
  if (samples & 0x1)
    dest[0] = (float)src[0] * factor;

#endif /* !defined(__ARM_NEON__) */
  return samples;
}

unsigned int CAEConvert::DOUBLE_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  double *src = (double*)data;
  for (unsigned int i = 0; i < samples; ++i)
    *dest++ = CLAMP(*src++ / (float)INT32_MAX);

  return samples;
}

unsigned int CAEConvert::Float_U8(float *data, const unsigned int samples, uint8_t *dest)
{
  #ifdef __SSE__
  const __m128 mul = _mm_set_ps1((float)INT8_MAX+.5f);
  const __m128 add = _mm_set_ps1(1.0f);
  unsigned int count = samples;

  /* work around invalid alignment */
  while ((((uintptr_t)data & 0xF) || ((uintptr_t)dest & 0xF)) && count > 0)
  {
    dest[0] = safeRound((data[0] + 1.0f) * ((float)INT8_MAX+.5f));
    ++data;
    ++dest;
    --count;
  }

  const uint32_t even = count & ~0x3;
  for (uint32_t i = 0; i < even; i += 4, data += 4)
  {
    __m128 in  = _mm_mul_ps(_mm_add_ps(_mm_load_ps(data), add), mul);
    __m64  con = _mm_cvtps_pi16(in);

    int16_t temp[4];
    memcpy(temp, &con, sizeof(temp));
    *dest++ = (uint8_t)temp[0];
    *dest++ = (uint8_t)temp[1];
    *dest++ = (uint8_t)temp[2];
    *dest++ = (uint8_t)temp[3];
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
  for (uint32_t i = 0; i < samples; ++i)
    *dest++ = safeRound((*data++ + 1.0f) * ((float)INT8_MAX+.5f));
  #endif

  return samples;
}

unsigned int CAEConvert::Float_S8(float *data, const unsigned int samples, uint8_t *dest)
{
  #ifdef __SSE__
  const __m128 mul = _mm_set_ps1((float)INT8_MAX+.5f);
  unsigned int count = samples;

  /* work around invalid alignment */
  while ((((uintptr_t)data & 0xF) || ((uintptr_t)dest & 0xF)) && count > 0)
  {
    dest[0] = safeRound(data[0] * ((float)INT8_MAX+.5f));
    ++data;
    ++dest;
    --count;
  }

  const uint32_t even = count & ~0x3;
  for (uint32_t i = 0; i < even; i += 4, data += 4, dest += 4)
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
  for (uint32_t i = 0; i < samples; ++i)
    *dest++ = safeRound(*data++ * ((float)INT8_MAX+.5f));
  #endif

  return samples;
}

unsigned int CAEConvert::Float_S16LE(float *data, const unsigned int samples, uint8_t *dest)
{
  int16_t *dst = (int16_t*)dest;
  #ifdef __SSE__

  unsigned int count     = samples;
  unsigned int unaligned = (0x10 - ((uintptr_t)data & 0xF)) >> 2;
  if (unaligned == 4)
    unaligned = 0;

  /*
    if we are only out by one, dont use SSE to correct it.
    this must run before we do any SSE work so that the FPU
    is in a working state without having to first call
    _mm_empty()
  */
  if (unaligned == 1)
    dst[0] = Endian_SwapLE16(safeRound(data[0] * ((float)INT16_MAX + CAEUtil::FloatRand1(-0.5f, 0.5f))));

  MEMALIGN(16, static const __m128  mul) = _mm_set_ps1((float)INT16_MAX);
  MEMALIGN(16, __m128  rand);
  MEMALIGN(16, __m128  in  );
  MEMALIGN(16, __m128i con );

  /* if unaligned is greater then one, use SSE to correct it */
  if (unaligned > 1)
  {
    switch (unaligned)
    {
      case 1: in = _mm_setr_ps(data[0], 0      , 0      , 0); break;
      case 2: in = _mm_setr_ps(data[0], data[1], 0      , 0); break;
      case 3: in = _mm_setr_ps(data[0], data[1], data[2], 0); break;
    }

    /* random round to dither */
    CAEUtil::FloatRand4(-0.5f, 0.5f, NULL, &rand);
    in  = _mm_mul_ps(in, _mm_add_ps(mul, rand));
    con = _mm_cvtps_epi32(in);

    #ifdef __BIG_ENDIAN__
    con = _mm_or_si128(_mm_slli_epi16(con, 8), _mm_srli_epi16(con, 8));
    #endif

    dst[0] = _mm_extract_epi16(con, 0);
    if (unaligned == 3)
    {
      dst[1] = _mm_extract_epi16(con, 2);
      dst[2] = _mm_extract_epi16(con, 4);
    }
    else if (unaligned == 2)
      dst[1] = _mm_extract_epi16(con, 2);
  }

  /* update our pointers and sample count */
  data  += unaligned;
  dst   += unaligned;
  count -= unaligned;

  const uint32_t even = count & ~0x3;
  for (uint32_t i = 0; i < even; i += 4, data += 4, dst += 4)
  {
    /* random round to dither */
    CAEUtil::FloatRand4(-0.5f, 0.5f, NULL, &rand);
    in   = _mm_mul_ps(_mm_load_ps(data), _mm_add_ps(mul, rand));
    con  = _mm_cvtps_epi32(in);

    #ifdef __BIG_ENDIAN__
    con = _mm_or_si128(_mm_slli_epi16(con, 8), _mm_srli_epi16(con, 8));
    #endif

    dst[0] = _mm_extract_epi16(con, 0);
    dst[1] = _mm_extract_epi16(con, 2);
    dst[2] = _mm_extract_epi16(con, 4);
    dst[3] = _mm_extract_epi16(con, 6);
  }

  /* calculate the final unaligned samples if there is any */
  if (samples != even)
  {
    unaligned = samples - even;
    switch (unaligned)
    {
      case 1: in = _mm_setr_ps(data[0], 0      , 0      , 0); break;
      case 2: in = _mm_setr_ps(data[0], data[1], 0      , 0); break;
      case 3: in = _mm_setr_ps(data[0], data[1], data[2], 0); break;
    }

    /* random round to dither */
    CAEUtil::FloatRand4(-0.5f, 0.5f, NULL, &rand);
    in  = _mm_mul_ps(in, _mm_add_ps(mul, rand));
    con = _mm_cvtps_epi32(in);

    #ifdef __BIG_ENDIAN__
    con = _mm_or_si128(_mm_slli_epi16(con, 8), _mm_srli_epi16(con, 8));
    #endif

    dst[0] = _mm_extract_epi16(con, 0);
    if (unaligned == 3)
    {
      dst[1] = _mm_extract_epi16(con, 2);
      dst[2] = _mm_extract_epi16(con, 4);
    }
    else if (unaligned == 2)
      dst[1] = _mm_extract_epi16(con, 2);
  }

  /* cleanup */
  _mm_empty();

  #else /* no SSE */

  uint32_t i    = 0;
  uint32_t even = samples & ~0x3;

  for(; i < even; i += 4)
  {
    /* random round to dither */
    float rand[4];
    CAEUtil::FloatRand4(-0.5f, 0.5f, rand);

    *dst++ = Endian_SwapLE16(safeRound(*data++ * ((float)INT16_MAX + rand[0])));
    *dst++ = Endian_SwapLE16(safeRound(*data++ * ((float)INT16_MAX + rand[1])));
    *dst++ = Endian_SwapLE16(safeRound(*data++ * ((float)INT16_MAX + rand[2])));
    *dst++ = Endian_SwapLE16(safeRound(*data++ * ((float)INT16_MAX + rand[3])));
  }

  for(; i < samples; ++i)
    *dst++ = Endian_SwapLE16(safeRound(*data++ * ((float)INT16_MAX + CAEUtil::FloatRand1(-0.5f, 0.5f))));

  #endif

  return samples << 1;
}

unsigned int CAEConvert::Float_S16BE(float *data, const unsigned int samples, uint8_t *dest)
{
  int16_t *dst = (int16_t*)dest;
  #ifdef __SSE__

  unsigned int count     = samples;
  unsigned int unaligned = (0x10 - ((uintptr_t)data & 0xF)) >> 2;
  if (unaligned == 4)
    unaligned = 0;

  /*
    if we are only out by one, dont use SSE to correct it.
    this must run before we do any SSE work so that the FPU
    is in a working state without having to first call
    _mm_empty()
  */
  if (unaligned == 1)
     dst[0] = Endian_SwapBE16(safeRound(data[0] * ((float)INT16_MAX + CAEUtil::FloatRand1(-0.5f, 0.5f))));

  MEMALIGN(16, static const __m128  mul) = _mm_set_ps1((float)INT16_MAX);
  MEMALIGN(16, __m128  rand);
  MEMALIGN(16, __m128  in  ) = _mm_setr_ps(0, 0, 0, 0);
  MEMALIGN(16, __m128i con );

  /* if unaligned is greater then one, use SSE to correct it */
  if (unaligned > 1)
  {
    switch (unaligned)
    {
      case 1: in = _mm_setr_ps(data[0], 0      , 0      , 0); break;
      case 2: in = _mm_setr_ps(data[0], data[1], 0      , 0); break;
      case 3: in = _mm_setr_ps(data[0], data[1], data[2], 0); break;
    }

    /* random round to dither */
    CAEUtil::FloatRand4(-0.5f, 0.5f, NULL, &rand);
    in  = _mm_mul_ps(in, _mm_add_ps(mul, rand));
    con = _mm_cvtps_epi32(in);

    #ifndef __BIG_ENDIAN__
    con = _mm_or_si128(_mm_slli_epi16(con, 8), _mm_srli_epi16(con, 8));
    #endif

    dst[0] = _mm_extract_epi16(con, 0);
    if (unaligned == 3)
    {
      dst[1] = _mm_extract_epi16(con, 2);
      dst[2] = _mm_extract_epi16(con, 4);
    }
    else if (unaligned == 2)
      dst[1] = _mm_extract_epi16(con, 2);
  }

  /* update our pointers and sample count */
  data  += unaligned;
  dst   += unaligned;
  count -= unaligned;

  const uint32_t even = count & ~0x3;
  for (uint32_t i = 0; i < even; i += 4, data += 4, dst += 4)
  {
    /* random round to dither */
    CAEUtil::FloatRand4(-0.5f, 0.5f, NULL, &rand);
    in   = _mm_mul_ps(_mm_load_ps(data), _mm_add_ps(mul, rand));
    con  = _mm_cvtps_epi32(in);

    #ifndef __BIG_ENDIAN__
    con = _mm_or_si128(_mm_slli_epi16(con, 8), _mm_srli_epi16(con, 8));
    #endif

    dst[0] = _mm_extract_epi16(con, 0);
    dst[1] = _mm_extract_epi16(con, 2);
    dst[2] = _mm_extract_epi16(con, 4);
    dst[3] = _mm_extract_epi16(con, 6);
  }

  /* calculate the final unaligned samples if there is any */
  if (samples != even)
  {
    unaligned = samples - even;
    switch (unaligned)
    {
      case 1: in = _mm_setr_ps(data[0], 0      , 0      , 0); break;
      case 2: in = _mm_setr_ps(data[0], data[1], 0      , 0); break;
      case 3: in = _mm_setr_ps(data[0], data[1], data[2], 0); break;
    }

    /* random round to dither */
    CAEUtil::FloatRand4(-0.5f, 0.5f, NULL, &rand);
    in  = _mm_mul_ps(in, _mm_add_ps(mul, rand));
    con = _mm_cvtps_epi32(in);

    #ifndef __BIG_ENDIAN__
    con = _mm_or_si128(_mm_slli_epi16(con, 8), _mm_srli_epi16(con, 8));
    #endif

    dst[0] = _mm_extract_epi16(con, 0);
    if (unaligned == 3)
    {
      dst[1] = _mm_extract_epi16(con, 2);
      dst[2] = _mm_extract_epi16(con, 4);
    }
    else if (unaligned == 2)
      dst[1] = _mm_extract_epi16(con, 2);
  }

  /* cleanup */
  _mm_empty();

  #else /* no SSE */

  uint32_t i    = 0;
  uint32_t even = samples & ~0x3;

  for(; i < even; i += 4)
  {
    /* random round to dither */
    float rand[4];
    CAEUtil::FloatRand4(-0.5f, 0.5f, rand);

    *dst++ = Endian_SwapBE16(safeRound(*data++ * ((float)INT16_MAX + rand[0])));
    *dst++ = Endian_SwapBE16(safeRound(*data++ * ((float)INT16_MAX + rand[1])));
    *dst++ = Endian_SwapBE16(safeRound(*data++ * ((float)INT16_MAX + rand[2])));
    *dst++ = Endian_SwapBE16(safeRound(*data++ * ((float)INT16_MAX + rand[3])));
  }

  for(; i < samples; ++i, data++, dst++)
    *dst++ = Endian_SwapBE16(safeRound(*data++ * ((float)INT16_MAX + CAEUtil::FloatRand1(-0.5f, 0.5f))));

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
  while ((((uintptr_t)data & 0xF) || ((uintptr_t)dest & 0xF)) && count > 0)
  {
    dst[0] = safeRound(data[0] * ((float)INT24_MAX+.5f));
    ++data;
    ++dst;
    --count;
  }

  const uint32_t even = count & ~0x3;
  for (uint32_t i = 0; i < even; i += 4, data += 4, dst += 4)
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
  for (uint32_t i = 0; i < samples; ++i)
    *dst++ = (safeRound(*data++ * ((float)INT24_MAX+.5f)) & 0xFFFFFF) << 8;
  #endif

  return samples << 2;
}

unsigned int CAEConvert::Float_S24NE3(float *data, const unsigned int samples, uint8_t *dest)
{
  /* We do not want to shift for S24LE3, since left-shifting would actually
   * push the MSB to the 4th byte. */
  const int leftShift =
#ifdef __BIG_ENDIAN__
    8;
#else
    0;
#endif

  /* disabled as it does not currently work */
  #if 0 && defined(__SSE__)
  int32_t *dst = (int32_t*)dest;

  const __m128 mul = _mm_set_ps1((float)INT24_MAX+.5f);
  unsigned int count = samples;

  /* work around invalid alignment */
  while ((((uintptr_t)data & 0xF) || ((uintptr_t)dest & 0xF)) && count > 0)
  {
    *((uint32_t*)(dest)) = (safeRound(*data * ((float)INT24_MAX+.5f)) & 0xFFFFFF) << leftShift;
    ++dest;
    --count;
  }

  const uint32_t even = count & ~0x3;
  for (uint32_t i = 0; i < count; i += 4, data += 4, dest += 12)
  {
    __m128  in  = _mm_mul_ps(_mm_load_ps(data), mul);
    __m128i con = _mm_cvtps_epi32(in);
    con         = _mm_slli_epi32(con, 8);
    memcpy(dst, &con, sizeof(int32_t) * 4);
    *((uint32_t*)(dest + 0)) = (dst[0] & 0xFFFFFF) << leftShift;
    *((uint32_t*)(dest + 3)) = (dst[1] & 0xFFFFFF) << leftShift;
    *((uint32_t*)(dest + 6)) = (dst[2] & 0xFFFFFF) << leftShift;
    *((uint32_t*)(dest + 9)) = (dst[3] & 0xFFFFFF) << leftShift;
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
        *((uint32_t*)(dest + 0)) = (dst[0] & 0xFFFFFF) << leftShift;
        *((uint32_t*)(dest + 3)) = (dst[1] & 0xFFFFFF) << leftShift;
      }
      else
      {
        in = _mm_setr_ps(data[0], data[1], data[2], 0);
        in = _mm_mul_ps(in, mul);
        __m128i con = _mm_cvtps_epi32(in);
        con         = _mm_slli_epi32(con, 8);
        memcpy(dst, &con, sizeof(int32_t) * 3);
        *((uint32_t*)(dest + 0)) = (dst[0] & 0xFFFFFF) << leftShift;
        *((uint32_t*)(dest + 3)) = (dst[1] & 0xFFFFFF) << leftShift;
        *((uint32_t*)(dest + 6)) = (dst[2] & 0xFFFFFF) << leftShift;
      }
    }
  }
  _mm_empty();
  #else /* no SSE */
  for (uint32_t i = 0; i < samples; ++i, ++data, dest += 3)
    *((uint32_t*)(dest)) = (safeRound(*data * ((float)INT24_MAX+.5f)) & 0xFFFFFF) << leftShift;
  #endif

  return samples * 3;
}

//float can't store INT32_MAX, it gets rounded up to INT32_MAX + 1
//INT32_MAX - 127 is the maximum value that can exactly be stored in both 32 bit float and int
#define MUL32 ((float)(INT32_MAX - 127))

unsigned int CAEConvert::Float_S32LE(float *data, const unsigned int samples, uint8_t *dest)
{
  int32_t *dst = (int32_t*)dest;
  #ifdef __SSE__
  const __m128 mul = _mm_set_ps1(MUL32);
  unsigned int count = samples;

  /* work around invalid alignment */
  while ((((uintptr_t)data & 0xF) || ((uintptr_t)dest & 0xF)) && count > 0)
  {
    dst[0] = safeRound(data[0] * MUL32);
    ++data;
    ++dst;
    --count;
  }

  const uint32_t even = count & ~0x3;
  for (uint32_t i = 0; i < even; i += 4, data += 4, dst += 4)
  {
    __m128  in  = _mm_mul_ps(_mm_load_ps(data), mul);
    __m128i con = _mm_cvtps_epi32(in);
    memcpy(dst, &con, sizeof(int32_t) * 4);
    dst[0] = Endian_SwapLE32(dst[0]);
    dst[1] = Endian_SwapLE32(dst[1]);
    dst[2] = Endian_SwapLE32(dst[2]);
    dst[3] = Endian_SwapLE32(dst[3]);
  }

  if (samples != even)
  {
    const uint32_t odd = samples - even;
    if (odd == 1)
    {
      dst[0] = safeRound(data[0] * MUL32);
      dst[0] = Endian_SwapLE32(dst[0]);
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
        dst[0] = Endian_SwapLE32(dst[0]);
        dst[1] = Endian_SwapLE32(dst[1]);
      }
      else
      {
        in = _mm_setr_ps(data[0], data[1], data[2], 0);
        in = _mm_mul_ps(in, mul);
        __m128i con = _mm_cvtps_epi32(in);
        memcpy(dst, &con, sizeof(int32_t) * 3);
        dst[0] = Endian_SwapLE32(dst[0]);
        dst[1] = Endian_SwapLE32(dst[1]);
        dst[2] = Endian_SwapLE32(dst[2]);
      }
    }
  }
  _mm_empty();
  #else

  /* no SIMD */
  for (uint32_t i = 0; i < samples; ++i, ++data, ++dst)
  {
    dst[0] = safeRound(data[0] * MUL32);
    dst[0] = Endian_SwapLE32(dst[0]);
  }
  #endif
  return samples << 2;
}


unsigned int CAEConvert::Float_S32LE_Neon(float *data, const unsigned int samples, uint8_t *dest)
{
#if defined(__ARM_NEON__)
  int32_t *dst = (int32_t*)dest;
  for (float *end = data + (samples & ~0x3); data < end; data += 4, dst += 4)
  {
    float32x4_t val = vmulq_n_f32(vld1q_f32((const float32_t *)data), MUL32);
    int32x4_t   ret = vcvtq_s32_f32(val);
    #ifdef __BIG_ENDIAN__
    ret = vrev64q_s32(ret);
    #endif
    vst1q_s32(dst, ret);
  }

  if (samples & 0x2)
  {
    float32x2_t val = vmul_n_f32(vld1_f32((const float32_t *)data), MUL32);
    int32x2_t   ret = vcvt_s32_f32(val);
    #ifdef __BIG_ENDIAN__
    ret = vrev64_s32(ret);
    #endif
    vst1_s32(dst, ret);
    data += 2;
    dst  += 2;
  }

  if (samples & 0x1)
  {
    dst[0] = safeRound(data[0] * MUL32);
    dst[0] = Endian_SwapLE32(dst[0]);
  }
#endif
  return samples << 2;
}

unsigned int CAEConvert::Float_S32BE(float *data, const unsigned int samples, uint8_t *dest)
{
  int32_t *dst = (int32_t*)dest;
  #ifdef __SSE__
  const __m128 mul = _mm_set_ps1(MUL32);
  unsigned int count = samples;

  /* work around invalid alignment */
  while ((((uintptr_t)data & 0xF) || ((uintptr_t)dest & 0xF)) && count > 0)
  {
    dst[0] = safeRound(data[0] * MUL32);
    ++data;
    ++dst;
    --count;
  }

  const uint32_t even = count & ~0x3;
  for (uint32_t i = 0; i < even; i += 4, data += 4, dst += 4)
  {
    __m128  in  = _mm_mul_ps(_mm_load_ps(data), mul);
    __m128i con = _mm_cvtps_epi32(in);
    memcpy(dst, &con, sizeof(int32_t) * 4);
    dst[0] = Endian_SwapBE32(dst[0]);
    dst[1] = Endian_SwapBE32(dst[1]);
    dst[2] = Endian_SwapBE32(dst[2]);
    dst[3] = Endian_SwapBE32(dst[3]);
  }

  if (samples != even)
  {
    const uint32_t odd = samples - even;
    if (odd == 1)
    {
      dst[0] = safeRound(data[0] * MUL32);
      dst[0] = Endian_SwapBE32(dst[0]);
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
        dst[0] = Endian_SwapBE32(dst[0]);
        dst[1] = Endian_SwapBE32(dst[1]);
      }
      else
      {
        in = _mm_setr_ps(data[0], data[1], data[2], 0);
        in = _mm_mul_ps(in, mul);
        __m128i con = _mm_cvtps_epi32(in);
        memcpy(dst, &con, sizeof(int32_t) * 3);
        dst[0] = Endian_SwapBE32(dst[0]);
        dst[1] = Endian_SwapBE32(dst[1]);
        dst[2] = Endian_SwapBE32(dst[2]);
      }
    }
  }
  _mm_empty();
  #else
  /* no SIMD */
  for (uint32_t i = 0; i < samples; ++i, ++data, ++dst)
  {
    dst[0] = safeRound(data[0] * MUL32);
    dst[0] = Endian_SwapBE32(dst[0]);
  }
  #endif

  return samples << 2;
}

unsigned int CAEConvert::Float_S32BE_Neon(float *data, const unsigned int samples, uint8_t *dest)
{
#if defined(__ARM_NEON__)
  int32_t *dst = (int32_t*)dest;
  for (float *end = data + (samples & ~0x3); data < end; data += 4, dst += 4)
  {
    float32x4_t val = vmulq_n_f32(vld1q_f32((const float32_t *)data), MUL32);
    int32x4_t   ret = vcvtq_s32_f32(val);
    #ifndef __BIG_ENDIAN__
    ret = vrev64q_s32(ret);
    #endif
    vst1q_s32(dst, ret);
  }

  if (samples & 0x2)
  {
    float32x2_t val = vmul_n_f32(vld1_f32((const float32_t *)data), MUL32);
    int32x2_t   ret = vcvt_s32_f32(val);
    #ifndef __BIG_ENDIAN__
    ret = vrev64_s32(ret);
    #endif
    vst1_s32(dst, ret);
    data += 2;
    dst  += 2;
  }

  if (samples & 0x1)
  {
    dst[0] = safeRound(data[0] * MUL32);
    dst[0] = Endian_SwapBE32(dst[0]);
  }
#endif
  return samples << 2;
}

unsigned int CAEConvert::Float_DOUBLE(float *data, const unsigned int samples, uint8_t *dest)
{
  double *dst = (double*)dest;
  for (unsigned int i = 0; i < samples; ++i)
    *dst++ = *data++;

  return samples * sizeof(double);
}

