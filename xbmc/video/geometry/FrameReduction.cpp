/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FrameReduction.h"

#include <algorithm>
#include <cmath>

namespace KODI::VIDEO::GEOMETRY
{

namespace
{

/*!
 * \brief One plane's worth of box averaging, templated on sample width.
 *
 * Boundaries are integer splits of the source extent, so every source sample lands in exactly one
 * output sample. The shift to 8 bits happens once, after the average.
 */
template<typename T>
void ReducePlane(const uint8_t* data,
                 int strideBytes,
                 unsigned int step,
                 unsigned int offset,
                 unsigned int sourceWidth,
                 unsigned int sourceHeight,
                 unsigned int shift,
                 unsigned int targetWidth,
                 unsigned int targetHeight,
                 uint8_t* out)
{
  for (unsigned int ty = 0; ty < targetHeight; ++ty)
  {
    const unsigned int y0 = ty * sourceHeight / targetHeight;
    const unsigned int y1 = std::max(y0 + 1, (ty + 1) * sourceHeight / targetHeight);

    for (unsigned int tx = 0; tx < targetWidth; ++tx)
    {
      const unsigned int x0 = tx * sourceWidth / targetWidth;
      const unsigned int x1 = std::max(x0 + 1, (tx + 1) * sourceWidth / targetWidth);

      uint64_t sum = 0;
      for (unsigned int sy = y0; sy < y1; ++sy)
      {
        const T* row = reinterpret_cast<const T*>(data + static_cast<size_t>(sy) * strideBytes);
        for (unsigned int sx = x0; sx < x1; ++sx)
          sum += row[sx * step + offset];
      }

      const uint64_t count = static_cast<uint64_t>(y1 - y0) * (x1 - x0);
      out[static_cast<size_t>(ty) * targetWidth + tx] =
          static_cast<uint8_t>(((sum + count / 2) / count) >> shift);
    }
  }
}

//! \brief Reduce the three planes at one sample width, laying interleaved chroma out planar.
template<typename T>
void ReducePlanes(const ReductionSource& source,
                  unsigned int shift,
                  unsigned int outWidth,
                  unsigned int outHeight,
                  ReducedFrame& out)
{
  const unsigned int chromaWidth = source.width / 2;
  const unsigned int chromaHeight = source.height / 2;
  const unsigned int outChromaWidth = outWidth / 2;
  const unsigned int outChromaHeight = outHeight / 2;

  ReducePlane<T>(source.y, source.yStrideBytes, 1, 0, source.width, source.height, shift, outWidth,
                 outHeight, out.y.data());
  if (source.chroma == ChromaLayout::Interleaved)
  {
    ReducePlane<T>(source.u, source.uStrideBytes, 2, 0, chromaWidth, chromaHeight, shift,
                   outChromaWidth, outChromaHeight, out.u.data());
    ReducePlane<T>(source.u, source.uStrideBytes, 2, 1, chromaWidth, chromaHeight, shift,
                   outChromaWidth, outChromaHeight, out.v.data());
  }
  else
  {
    ReducePlane<T>(source.u, source.uStrideBytes, 1, 0, chromaWidth, chromaHeight, shift,
                   outChromaWidth, outChromaHeight, out.u.data());
    ReducePlane<T>(source.v, source.vStrideBytes, 1, 0, chromaWidth, chromaHeight, shift,
                   outChromaWidth, outChromaHeight, out.v.data());
  }
}

} // unnamed namespace

ReducedSize ReductionOutputSize(unsigned int sourceWidth,
                                unsigned int sourceHeight,
                                unsigned int targetWidth)
{
  if (sourceWidth == 0 || sourceHeight == 0 || targetWidth == 0)
    return {};

  const unsigned int width = std::min(targetWidth, sourceWidth) & ~1u;
  const unsigned int height = static_cast<unsigned int>(std::lround(
                                  static_cast<double>(width) * sourceHeight / sourceWidth / 2.0)) *
                              2u;
  if (width == 0 || height == 0)
    return {};

  return {width, height};
}

bool ReduceFrame(const ReductionSource& source, unsigned int targetWidth, ReducedFrame& out)
{
  if (!source.y || !source.u || source.width == 0 || source.height == 0 || targetWidth == 0 ||
      source.bitDepth < 8 || source.bitDepth > 16)
    return false;

  if (source.chroma == ChromaLayout::Planar && !source.v)
    return false;

  const auto [outWidth, outHeight] = ReductionOutputSize(source.width, source.height, targetWidth);
  if (outWidth == 0 || outHeight == 0)
    return false;

  // Top-aligned words carry their significant bits where an 8-bit read wants them, so the
  // shift is to the low byte regardless of depth; low-aligned words shift by the depth.
  const unsigned int shift = source.bitDepth == 8 ? 0u
                             : source.highAligned ? 8u
                                                  : source.bitDepth - 8u;

  out.width = outWidth;
  out.height = outHeight;
  out.y.resize(static_cast<size_t>(outWidth) * outHeight);
  out.u.resize(static_cast<size_t>(outWidth) * outHeight / 4);
  out.v.resize(static_cast<size_t>(outWidth) * outHeight / 4);

  if (source.bitDepth == 8)
    ReducePlanes<uint8_t>(source, shift, outWidth, outHeight, out);
  else
    ReducePlanes<uint16_t>(source, shift, outWidth, outHeight, out);

  return true;
}

} // namespace KODI::VIDEO::GEOMETRY
