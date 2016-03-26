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

#include <emmintrin.h>

inline void* memcpy_aligned(void* dst, const void* src, size_t size)
{
  size_t i;
  __m128i xmm1, xmm2, xmm3, xmm4;

  // if memory is not aligned, use memcpy
  if ((((size_t)(src) | (size_t)(dst)) & 0xF))
    return memcpy(dst, src, size);

  uint8_t* d = (uint8_t*)(dst);
  uint8_t* s = (uint8_t*)(src);

  for (i = 0; i < size - 63; i += 64)
  {
    xmm1 = _mm_load_si128((__m128i*)(s + i +  0));
    xmm2 = _mm_load_si128((__m128i*)(s + i + 16));
    xmm3 = _mm_load_si128((__m128i*)(s + i + 32));
    xmm4 = _mm_load_si128((__m128i*)(s + i + 48));
    _mm_stream_si128((__m128i*)(d + i +  0), xmm1);
    _mm_stream_si128((__m128i*)(d + i + 16), xmm2);
    _mm_stream_si128((__m128i*)(d + i + 32), xmm3);
    _mm_stream_si128((__m128i*)(d + i + 48), xmm4);
  }
  for (; i < size; i += 16)
  {
    xmm1 = _mm_load_si128((__m128i*)(s + i));
    _mm_stream_si128((__m128i*)(d + i), xmm1);
  }
  return dst;
}

inline void convert_yuv420_nv12(uint8_t *const src[], const int srcStride[], int height, int width, uint8_t *const dst[], const int dstStride[])
{
  __m128i xmm0, xmm1, xmm2, xmm3, xmm4;
  _mm_sfence();

  // Convert to NV12 - Luma
  if (srcStride[0] == dstStride[0])
    memcpy_aligned(dst[0], src[0], srcStride[0] * height);
  else
  {
    for (size_t line = 0; line < height; ++line)
    {
      uint8_t * s = src[0] + srcStride[0] * line;
      uint8_t * d = dst[0] + dstStride[0] * line;
      memcpy_aligned(d, s, srcStride[0]);
    }
  }
  // Convert to NV12 - Chroma
  size_t chromaWidth = (width + 1) >> 1;
  size_t chromaHeight = height >> 1;
  for (size_t line = 0; line < chromaHeight; ++line)
  {
    size_t i;
    uint8_t * u = src[1] + line * srcStride[1];
    uint8_t * v = src[2] + line * srcStride[2];
    uint8_t * d = dst[1] + line * dstStride[1];
    // if memory is not aligned use memcpy
    if (((size_t)(u) | (size_t)(v) | (size_t)(d)) & 0xF)
    {
      for (i = 0; i < chromaWidth; ++i)
      {
        *d++ = *u++;
        *d++ = *v++;
      }
    }
    else
    {
      for (i = 0; i < (chromaWidth - 31); i += 32)
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
      if (((size_t)chromaWidth) & 0xF)
      {
        d += (i << 1);
        u += i; v += i;
        for (; i < chromaWidth; ++i)
        {
          *d++ = *u++;
          *d++ = *v++;
        }
      }
      else if (i < chromaWidth)
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
