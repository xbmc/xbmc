/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace KODI::VIDEO::SUBTITLES
{
enum class BitmapStereoLayout
{
  MONO,
  LEFT_RIGHT,
  TOP_BOTTOM,
};

struct BitmapSubtitle
{
  const uint8_t* pixels{nullptr};
  int stride{0};
  int x{0};
  int y{0};
  int width{0};
  int height{0};
  int sourceWidth{0};
  int sourceHeight{0};

  // Fully transparent entries must be normalized to zero by the caller.
  std::array<uint32_t, 256> palette{};
};

struct BitmapStereoCrop
{
  // Coordinates in the original packed subtitle bitmap, used to copy pixels.
  int packedX{0};
  int packedY{0};

  // Coordinates and canvas size in the selected eye view.
  int x{0};
  int y{0};
  int width{0};
  int height{0};
  int sourceWidth{0};
  int sourceHeight{0};

  bool IsEmpty() const { return width <= 0 || height <= 0; }
};

/*!
 * \brief Return the packed bitmap layout described by a video stream stereo mode.
 */
BitmapStereoLayout GetBitmapStereoLayout(std::string_view stereoMode);

/*!
 * \brief Check whether a bitmap contains matching subtitle images in both packed views.
 *
 * The images may be shifted relative to each other to encode stereoscopic depth. Transparent
 * borders are therefore removed independently before the two images are compared.
 */
bool IsStereoBitmap(const BitmapSubtitle& bitmap, BitmapStereoLayout layout);

/*!
 * \brief Return the exact intersection of a packed subtitle bitmap with one eye view.
 * \param secondView False for the left/top view, true for the right/bottom view.
 */
BitmapStereoCrop GetStereoCrop(const BitmapSubtitle& bitmap,
                               BitmapStereoLayout layout,
                               bool secondView);
} // namespace KODI::VIDEO::SUBTITLES
