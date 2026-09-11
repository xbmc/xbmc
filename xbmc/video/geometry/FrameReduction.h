/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <vector>

namespace KODI::VIDEO::GEOMETRY
{

//! \brief How the chroma samples of a ReductionSource are laid out.
enum class ChromaLayout
{
  Planar, //!< separate U and V planes, half resolution each way
  Interleaved, //!< one plane of Cb,Cr pairs at half resolution - NV12, P010
};

//! \brief A non-owning view of one decoded 4:2:0 frame. Samples deeper than 8 bits are
//! little-endian uint16_t, and highAligned says where the significant bits sit in the word.
struct ReductionSource
{
  unsigned int width{0};
  unsigned int height{0};
  unsigned int bitDepth{8};
  bool highAligned{false};
  ChromaLayout chroma{ChromaLayout::Interleaved};

  const uint8_t* y{nullptr};
  int yStrideBytes{0};
  const uint8_t* u{nullptr}; //!< the interleaved chroma plane when layout is Interleaved
  int uStrideBytes{0};
  const uint8_t* v{nullptr}; //!< unused when layout is Interleaved
  int vStrideBytes{0};
};

//! \brief The extent a reduction produces; zero in both dimensions when none is possible.
struct ReducedSize
{
  unsigned int width{0};
  unsigned int height{0};
};

//! \brief A small CPU-readable copy of a decoded picture: planar 8-bit YUV420, tightly packed.
//! The plane vectors are reused across frames.
struct ReducedFrame
{
  unsigned int width{0};
  unsigned int height{0};
  std::vector<uint8_t> y;
  std::vector<uint8_t> u;
  std::vector<uint8_t> v;
};

//! \brief The extent ReduceFrame produces: \p targetWidth clamped to the source, the height
//! following its shape, both even. Exposed so a GPU reducer can size its blit target the same.
ReducedSize ReductionOutputSize(unsigned int sourceWidth,
                                unsigned int sourceHeight,
                                unsigned int targetWidth);

//! \brief Downscale a decoded frame to a small planar 8-bit YUV420 copy on the CPU, each
//! output sample being the box average of the source samples it covers. False when the source
//! is not describable, leaving \p out untouched.
bool ReduceFrame(const ReductionSource& source, unsigned int targetWidth, ReducedFrame& out);

} // namespace KODI::VIDEO::GEOMETRY
