/*
*      Copyright (C) 2005-2015 Team Kodi
*      http://kodi.tv
*
*  This library is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Lesser General Public
*  License as published by the Free Software Foundation; either
*  version 2.1 of the License, or (at your option) any later version.
*
*  This library is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public
*  License along with this library; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*
*/

#pragma once
#include <emmintrin.h>

inline void* memcpy_aligned(void* dst, const void* src, size_t size, uint8_t bpp = 0)
{
  const uint8_t shift = 16 - bpp;
  __m128i xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8;
#ifdef _M_X64
  __m128i xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15, xmm16;
#endif

  // if memory is not aligned, use memcpy
  if ((((size_t)(src) | (size_t)(dst)) & 0xF))
  {
    if (bpp == 0 || bpp == 16)
      return memcpy(dst, src, size);
    else
    {
      uint16_t * y = (uint16_t*)(src);
      uint16_t * d = (uint16_t*)(dst);
      for (size_t x = 0; x < (size >> 1); x++)
      {
        d[x] = y[x] << shift;
      }
      return dst;
    }
  }

  static const size_t regsInLoop = sizeof(size_t) * 2; // 8 or 16
  size_t reminder = size & (regsInLoop * sizeof(xmm1) - 1); // Copy 128 or 256 bytes every loop
  size_t end = 0;

  __m128i* pTrg = (__m128i*)dst;
  __m128i* pTrgEnd = pTrg + ((size - reminder) >> 4);
  __m128i* pSrc = (__m128i*)src;

  _mm_sfence();

  while(pTrg < pTrgEnd)
  //for (i = 0; i < size - 63; i += 64)
  {
    xmm1 = _mm_load_si128(pSrc);
    xmm2 = _mm_load_si128(pSrc + 1);
    xmm3 = _mm_load_si128(pSrc + 2);
    xmm4 = _mm_load_si128(pSrc + 3);
    xmm5 = _mm_load_si128(pSrc + 4);
    xmm6 = _mm_load_si128(pSrc + 5);
    xmm7 = _mm_load_si128(pSrc + 6);
    xmm8 = _mm_load_si128(pSrc + 7);
#ifdef _M_X64 // Use all 16 xmm registers
    xmm9 = _mm_load_si128(pSrc + 8);
    xmm10 = _mm_load_si128(pSrc + 9);
    xmm11 = _mm_load_si128(pSrc + 10);
    xmm12 = _mm_load_si128(pSrc + 11);
    xmm13 = _mm_load_si128(pSrc + 12);
    xmm14 = _mm_load_si128(pSrc + 13);
    xmm15 = _mm_load_si128(pSrc + 14);
    xmm16 = _mm_load_si128(pSrc + 15);
#endif
    pSrc += regsInLoop;

    if (bpp != 0 && bpp != 16)
    {
      xmm1 = _mm_slli_epi16(xmm1, shift);
      xmm2 = _mm_slli_epi16(xmm2, shift);
      xmm3 = _mm_slli_epi16(xmm3, shift);
      xmm4 = _mm_slli_epi16(xmm4, shift);
      xmm5 = _mm_slli_epi16(xmm5, shift);
      xmm6 = _mm_slli_epi16(xmm6, shift);
      xmm7 = _mm_slli_epi16(xmm7, shift);
      xmm8 = _mm_slli_epi16(xmm8, shift);
#ifdef _M_X64 // Use all 16 xmm registers
      xmm9 = _mm_slli_epi16(xmm9, shift);
      xmm10 = _mm_slli_epi16(xmm10, shift);
      xmm11 = _mm_slli_epi16(xmm11, shift);
      xmm12 = _mm_slli_epi16(xmm12, shift);
      xmm13 = _mm_slli_epi16(xmm13, shift);
      xmm14 = _mm_slli_epi16(xmm14, shift);
      xmm15 = _mm_slli_epi16(xmm15, shift);
      xmm16 = _mm_slli_epi16(xmm16, shift);
#endif
    }

    _mm_stream_si128(pTrg, xmm1);
    _mm_stream_si128(pTrg + 1, xmm2);
    _mm_stream_si128(pTrg + 2, xmm3);
    _mm_stream_si128(pTrg + 3, xmm4);
    _mm_stream_si128(pTrg + 4, xmm5);
    _mm_stream_si128(pTrg + 5, xmm6);
    _mm_stream_si128(pTrg + 6, xmm7);
    _mm_stream_si128(pTrg + 7, xmm8);
#ifdef _M_X64 // Use all 16 xmm registers
    _mm_stream_si128(pTrg + 8, xmm9);
    _mm_stream_si128(pTrg + 9, xmm10);
    _mm_stream_si128(pTrg + 10, xmm11);
    _mm_stream_si128(pTrg + 11, xmm12);
    _mm_stream_si128(pTrg + 12, xmm13);
    _mm_stream_si128(pTrg + 13, xmm14);
    _mm_stream_si128(pTrg + 14, xmm15);
    _mm_stream_si128(pTrg + 15, xmm16);
#endif
    pTrg += regsInLoop;
  }

  if (reminder >= 16)
  {
    size = reminder;
    reminder = size & 15;
    end = size >> 4;
    for (size_t i = 0; i < end; ++i)
    {
      xmm1 = _mm_load_si128(pSrc + i);
      if (bpp != 0 && bpp != 16)
        xmm1 = _mm_slli_epi16(xmm1, shift);
      _mm_store_si128(pTrg + i, xmm1);
    }
  }

  if (reminder)
  {
    __m128i temp = _mm_load_si128(pSrc + end);
    char* ps = (char*)(&temp);
    char* pt = (char*)(pTrg + end);
    for (size_t i = 0; i < reminder; ++i)
    {
      pt[i] = ps[i] << shift;
    }
  }
  return dst;
}

inline void copy_plane(uint8_t *const src, const int srcStride, int height, int width, uint8_t *const dst, const int dstStride, uint8_t bpp = 0)
{
  _mm_sfence();

  if (srcStride == dstStride)
    memcpy_aligned(dst, src, srcStride * height, bpp);
  else
  {
    for (size_t line = 0; line < height; ++line)
    {
      uint8_t * s = src + srcStride * line;
      uint8_t * d = dst + dstStride * line;
      memcpy_aligned(d, s, srcStride, bpp);
    }
  }
}

inline void convert_yuv420_nv12_chrome(uint8_t *const *src, const int *srcStride, int height, int width, uint8_t *const dst, const int dstStride)
{
  __m128i xmm0, xmm1, xmm2, xmm3, xmm4;
  _mm_sfence();

  const size_t chroma_width = (width + 1) >> 1;
  const size_t chromaHeight = height >> 1;
  size_t line, i;

  for (line = 0; line < chromaHeight; ++line)
  {
    uint8_t * u = src[0] + line * srcStride[0];
    uint8_t * v = src[1] + line * srcStride[1];
    uint8_t * d = dst + line * dstStride;

    // if memory is not aligned use memcpy
    if (((size_t)(u) | (size_t)(v) | (size_t)(d)) & 0xF)
    {
      for (i = 0; i < chroma_width; ++i)
      {
        *d++ = *u++;
        *d++ = *v++;
      }
    }
    else
    {
      for (i = 0; i < (chroma_width - 31); i += 32)
      {
        xmm0 = _mm_load_si128((__m128i*)(v + i));
        xmm1 = _mm_load_si128((__m128i*)(u + i));
        xmm2 = _mm_load_si128((__m128i*)(v + i + 16));
        xmm3 = _mm_load_si128((__m128i*)(u + i + 16));

        xmm4 = xmm0;
        xmm0 = _mm_unpacklo_epi8(xmm1, xmm0);
        xmm4 = _mm_unpackhi_epi8(xmm1, xmm4);

        xmm1 = xmm2;
        xmm2 = _mm_unpacklo_epi8(xmm3, xmm2);
        xmm1 = _mm_unpackhi_epi8(xmm3, xmm1);

        _mm_stream_si128((__m128i *)(d + (i << 1) + 0), xmm0);
        _mm_stream_si128((__m128i *)(d + (i << 1) + 16), xmm4);
        _mm_stream_si128((__m128i *)(d + (i << 1) + 32), xmm2);
        _mm_stream_si128((__m128i *)(d + (i << 1) + 48), xmm1);
      }
      if (((size_t)chroma_width) & 0xF)
      {
        d += (i << 1);
        u += i; v += i;
        for (; i < chroma_width; ++i)
        {
          *d++ = *u++;
          *d++ = *v++;
        }
      }
      else if (i < chroma_width)
      {
        xmm0 = _mm_load_si128((__m128i*)(v + i));
        xmm1 = _mm_load_si128((__m128i*)(u + i));

        xmm2 = xmm0;
        xmm0 = _mm_unpacklo_epi8(xmm1, xmm0);
        xmm2 = _mm_unpackhi_epi8(xmm1, xmm2);

        _mm_stream_si128((__m128i *)(d + (i << 1) + 0), xmm0);
        _mm_stream_si128((__m128i *)(d + (i << 1) + 16), xmm2);
      }
    }
  }
}

inline void convert_yuv420_p01x_chrome(uint8_t *const *src, const int *srcStride, int height, int width, uint8_t *const dst, const int dstStride, uint8_t bpp)
{
  const uint8_t shift = 16 - bpp;
  __m128i xmm0, xmm1, xmm2, xmm3, xmm4;
  _mm_sfence();

  // Convert to P01x - Chroma
  const size_t chromaWidth = (width + 1) >> 1;
  const size_t chromaHeight = height >> 1;
  size_t line, i;

  for (line = 0; line < chromaHeight; ++line)
  {
    uint16_t * u = (uint16_t*)(src[0] + line * srcStride[0]);
    uint16_t * v = (uint16_t*)(src[1] + line * srcStride[1]);
    uint16_t * d = (uint16_t*)(dst + line * dstStride);

    // if memory is not aligned use memcpy
    if (((size_t)(u) | (size_t)(v) | (size_t)(d)) & 0xF)
    {
      for (i = 0; i < chromaWidth; ++i)
      {
        *d++ = *u++ << shift;
        *d++ = *v++ << shift;
      }
    }
    else
    {
      for (i = 0; i < chromaWidth; i += 16)
      {
        xmm0 = _mm_load_si128((__m128i*)(v + i));
        xmm1 = _mm_load_si128((__m128i*)(u + i));
        xmm2 = _mm_load_si128((__m128i*)(v + i + 8));
        xmm3 = _mm_load_si128((__m128i*)(u + i + 8));

        xmm0 = _mm_slli_epi16(xmm0, shift);
        xmm1 = _mm_slli_epi16(xmm1, shift);
        xmm2 = _mm_slli_epi16(xmm2, shift);
        xmm3 = _mm_slli_epi16(xmm3, shift);

        xmm4 = xmm0;
        xmm0 = _mm_unpacklo_epi16(xmm1, xmm0);
        xmm4 = _mm_unpackhi_epi16(xmm1, xmm4);

        xmm1 = xmm2;
        xmm2 = _mm_unpacklo_epi16(xmm3, xmm2);
        xmm1 = _mm_unpackhi_epi16(xmm3, xmm1);

        _mm_stream_si128((__m128i *)(d + (i << 1) + 0), xmm0);
        _mm_stream_si128((__m128i *)(d + (i << 1) + 8), xmm4);
        _mm_stream_si128((__m128i *)(d + (i << 1) + 16), xmm2);
        _mm_stream_si128((__m128i *)(d + (i << 1) + 24), xmm1);
      }
    }
  }

}

inline void convert_yuv420_nv12(uint8_t *const src[], const int srcStride[], int height, int width, uint8_t *const dst[], const int dstStride[])
{
  // Convert to NV12 - Luma
  copy_plane(src[0], srcStride[0], height, width, dst[0], dstStride[0]);
  // Convert to NV12 - Chroma
  convert_yuv420_nv12_chrome(&src[1], &srcStride[1], height, width, dst[1], dstStride[1]);
}

inline void convert_yuv420_p01x(uint8_t *const src[], const int srcStride[], int height, int width, uint8_t *const dst[], const int dstStride[], uint8_t bpp)
{
  // Convert to P01x - Luma
  copy_plane(src[0], srcStride[0], height, width, dst[0], dstStride[0], bpp);
  // Convert to P01x - Chroma
  convert_yuv420_p01x_chrome(&src[1], &srcStride[1], height, width, dst[1], dstStride[1], bpp);
}
