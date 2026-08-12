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

/*!
 * \brief A non-owning view of one decoded 4:2:0 frame for ReduceFrame.
 *
 * Samples deeper than 8 bits are little-endian uint16_t words. highAligned says where the
 * significant bits sit within the word: at the top for P010 and kin, at the bottom for FFmpeg's
 * P10/P12 planar formats.
 */
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

//! \brief A small CPU-readable copy of a decoded picture, for analysis. Planar 8-bit YUV420,
//! tightly packed. The plane vectors are reused across frames, so only the first allocates.
struct ReducedFrame
{
  unsigned int width{0};
  unsigned int height{0};
  std::vector<uint8_t> y;
  std::vector<uint8_t> u;
  std::vector<uint8_t> v;
};

//! \brief The output extent ReduceFrame produces: \p targetWidth clamped to the source, the
//! height following the source shape, both even for the 4:2:0 chroma. Exposed so a GPU reducer
//! can size its blit target the same way.
ReducedSize ReductionOutputSize(unsigned int sourceWidth,
                                unsigned int sourceHeight,
                                unsigned int targetWidth);

/*!
 * \brief Downscale a decoded frame to a small planar 8-bit YUV420 copy, on the CPU, for a
 *        picture that is CPU-addressable but in a layout or depth the detector cannot read.
 *
 * Each output sample is the box average of every source sample it covers, so a transition row
 * averages bar against picture rather than sampling one or the other.
 *
 * \return false when the source is not describable - no planes, zero dimensions, a depth
 *         outside 8..16 - and \p out is untouched
 */
bool ReduceFrame(const ReductionSource& source, unsigned int targetWidth, ReducedFrame& out);

} // namespace KODI::VIDEO::GEOMETRY
