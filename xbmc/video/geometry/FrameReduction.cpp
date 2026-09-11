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
#include <vector>

namespace KODI::VIDEO::GEOMETRY
{

namespace
{

//! \brief The integer split of the source extent each output column starts at, plus a closing
//! bound. Computed once per plane rather than per output pixel of every row.
std::vector<unsigned int> ColumnBoundaries(unsigned int sourceWidth, unsigned int targetWidth)
{
  std::vector<unsigned int> boundaries(static_cast<size_t>(targetWidth) + 1);
  for (unsigned int tx = 0; tx <= targetWidth; ++tx)
    boundaries[tx] = tx * sourceWidth / targetWidth;

  return boundaries;
}

//! \brief One plane's worth of box averaging, templated on sample width and on the stride
//! between neighbouring samples, which is 2 only for interleaved chroma.
template<typename T, unsigned int STEP>
void ReducePlane(const uint8_t* data,
                 int strideBytes,
                 unsigned int offset,
                 unsigned int sourceWidth,
                 unsigned int sourceHeight,
                 unsigned int shift,
                 unsigned int targetWidth,
                 unsigned int targetHeight,
                 uint8_t* out)
{
  if (sourceWidth == targetWidth && sourceHeight == targetHeight)
  {
    for (unsigned int ty = 0; ty < targetHeight; ++ty)
    {
      const T* row = reinterpret_cast<const T*>(data + static_cast<size_t>(ty) * strideBytes);
      uint8_t* line = out + static_cast<size_t>(ty) * targetWidth;
      for (unsigned int tx = 0; tx < targetWidth; ++tx)
        line[tx] = static_cast<uint8_t>(row[tx * STEP + offset] >> shift);
    }
    return;
  }

  const std::vector<unsigned int> columns{ColumnBoundaries(sourceWidth, targetWidth)};

  for (unsigned int ty = 0; ty < targetHeight; ++ty)
  {
    const unsigned int y0 = ty * sourceHeight / targetHeight;
    const unsigned int y1 = std::max(y0 + 1, (ty + 1) * sourceHeight / targetHeight);

    for (unsigned int tx = 0; tx < targetWidth; ++tx)
    {
      const unsigned int x0 = columns[tx];
      const unsigned int x1 = std::max(x0 + 1, columns[tx + 1]);

      uint64_t sum = 0;
      for (unsigned int sy = y0; sy < y1; ++sy)
      {
        const T* row = reinterpret_cast<const T*>(data + static_cast<size_t>(sy) * strideBytes);
        for (unsigned int sx = x0; sx < x1; ++sx)
          sum += row[sx * STEP + offset];
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

  ReducePlane<T, 1>(source.y, source.yStrideBytes, 0, source.width, source.height, shift, outWidth,
                    outHeight, out.y.data());
  if (source.chroma == ChromaLayout::Interleaved)
  {
    ReducePlane<T, 2>(source.u, source.uStrideBytes, 0, chromaWidth, chromaHeight, shift,
                      outChromaWidth, outChromaHeight, out.u.data());
    ReducePlane<T, 2>(source.u, source.uStrideBytes, 1, chromaWidth, chromaHeight, shift,
                      outChromaWidth, outChromaHeight, out.v.data());
  }
  else
  {
    ReducePlane<T, 1>(source.u, source.uStrideBytes, 0, chromaWidth, chromaHeight, shift,
                      outChromaWidth, outChromaHeight, out.u.data());
    ReducePlane<T, 1>(source.v, source.vStrideBytes, 0, chromaWidth, chromaHeight, shift,
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

  // Top-aligned words shift to the low byte regardless of depth; low-aligned ones shift by
  // the depth.
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
