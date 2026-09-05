/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDOverlayCodecFFmpegUtils.h"

#include <algorithm>
#include <cstddef>

namespace KODI::VIDEO::SUBTITLES
{
namespace
{
struct Bounds
{
  int x1{0};
  int y1{0};
  int x2{0};
  int y2{0};

  bool IsEmpty() const { return x1 >= x2 || y1 >= y2; }
  int Width() const { return x2 - x1; }
  int Height() const { return y2 - y1; }
};

uint32_t GetPixel(const BitmapSubtitle& bitmap, int x, int y)
{
  const auto index = bitmap.pixels[(y - bitmap.y) * bitmap.stride + (x - bitmap.x)];
  return bitmap.palette[index];
}

Bounds GetVisibleBounds(const BitmapSubtitle& bitmap, const Bounds& region)
{
  const int x1 = std::max(bitmap.x, region.x1);
  const int y1 = std::max(bitmap.y, region.y1);
  const int x2 = std::min(bitmap.x + bitmap.width, region.x2);
  const int y2 = std::min(bitmap.y + bitmap.height, region.y2);
  if (x1 >= x2 || y1 >= y2)
    return {};

  Bounds visible{x2, y2, x1, y1};
  for (int y = y1; y < y2; ++y)
  {
    for (int x = x1; x < x2; ++x)
    {
      if (GetPixel(bitmap, x, y) == 0)
        continue;

      visible.x1 = std::min(visible.x1, x);
      visible.y1 = std::min(visible.y1, y);
      visible.x2 = std::max(visible.x2, x + 1);
      visible.y2 = std::max(visible.y2, y + 1);
    }
  }
  return visible;
}

bool HaveCompatiblePositions(const Bounds& first,
                             const Bounds& second,
                             const Bounds& firstRegion,
                             const Bounds& secondRegion)
{
  const int firstX = first.x1 - firstRegion.x1;
  const int firstY = first.y1 - firstRegion.y1;
  const int secondX = second.x1 - secondRegion.x1;
  const int secondY = second.y1 - secondRegion.y1;

  // Stereo depth shifts the two copies horizontally. A small vertical tolerance accounts for
  // rounding, while the horizontal limit prevents a repeated monoscopic caption around the
  // split boundary from being mistaken for two eye images.
  const int maxHorizontalShift = std::max(2, firstRegion.Width() / 8);
  const int maxVerticalShift = std::max(2, firstRegion.Height() / 100);
  return std::abs(firstX - secondX) <= maxHorizontalShift &&
         std::abs(firstY - secondY) <= maxVerticalShift;
}
} // unnamed namespace

BitmapStereoLayout GetBitmapStereoLayout(std::string_view stereoMode)
{
  if (stereoMode == "left_right" || stereoMode == "right_left")
    return BitmapStereoLayout::LEFT_RIGHT;
  if (stereoMode == "top_bottom" || stereoMode == "bottom_top")
    return BitmapStereoLayout::TOP_BOTTOM;
  return BitmapStereoLayout::MONO;
}

bool IsStereoBitmap(const BitmapSubtitle& bitmap, BitmapStereoLayout layout)
{
  if (!bitmap.pixels || bitmap.stride < bitmap.width || bitmap.width <= 0 || bitmap.height <= 0 ||
      bitmap.sourceWidth <= 0 || bitmap.sourceHeight <= 0)
    return false;

  Bounds first;
  Bounds second;
  if (layout == BitmapStereoLayout::LEFT_RIGHT)
  {
    if (bitmap.sourceWidth % 2 != 0)
      return false;

    const int split = bitmap.sourceWidth / 2;
    if (bitmap.x >= split || bitmap.x + bitmap.width <= split)
      return false;
    first = {0, 0, split, bitmap.sourceHeight};
    second = {split, 0, bitmap.sourceWidth, bitmap.sourceHeight};
  }
  else if (layout == BitmapStereoLayout::TOP_BOTTOM)
  {
    if (bitmap.sourceHeight % 2 != 0)
      return false;

    const int split = bitmap.sourceHeight / 2;
    if (bitmap.y >= split || bitmap.y + bitmap.height <= split)
      return false;
    first = {0, 0, bitmap.sourceWidth, split};
    second = {0, split, bitmap.sourceWidth, bitmap.sourceHeight};
  }
  else
    return false;

  const Bounds firstRegion = first;
  const Bounds secondRegion = second;
  first = GetVisibleBounds(bitmap, firstRegion);
  second = GetVisibleBounds(bitmap, secondRegion);
  if (first.IsEmpty() || second.IsEmpty() || first.Width() != second.Width() ||
      first.Height() != second.Height() ||
      !HaveCompatiblePositions(first, second, firstRegion, secondRegion))
    return false;

  std::size_t comparedPixels = 0;
  std::size_t mismatchedPixels = 0;
  for (int y = 0; y < first.Height(); ++y)
  {
    for (int x = 0; x < first.Width(); ++x)
    {
      const uint32_t firstPixel = GetPixel(bitmap, first.x1 + x, first.y1 + y);
      const uint32_t secondPixel = GetPixel(bitmap, second.x1 + x, second.y1 + y);
      if (firstPixel == 0 && secondPixel == 0)
        continue;

      ++comparedPixels;
      if (firstPixel != secondPixel)
        ++mismatchedPixels;
    }
  }

  // The packed images produced by subtitle conversion tools are normally identical. Keep a
  // small tolerance for palette conversion or rounding differences while rejecting ordinary
  // wide captions that merely cross the boundary between the two views.
  return comparedPixels > 0 && mismatchedPixels * 100 <= comparedPixels;
}

BitmapStereoCrop GetStereoCrop(const BitmapSubtitle& bitmap,
                               BitmapStereoLayout layout,
                               bool secondView)
{
  if (bitmap.width <= 0 || bitmap.height <= 0 || bitmap.sourceWidth <= 0 ||
      bitmap.sourceHeight <= 0)
    return {};

  int regionX{0};
  int regionY{0};
  int regionWidth{bitmap.sourceWidth};
  int regionHeight{bitmap.sourceHeight};

  if (layout == BitmapStereoLayout::LEFT_RIGHT && bitmap.sourceWidth % 2 == 0)
  {
    regionWidth /= 2;
    if (secondView)
      regionX = regionWidth;
  }
  else if (layout == BitmapStereoLayout::TOP_BOTTOM && bitmap.sourceHeight % 2 == 0)
  {
    regionHeight /= 2;
    if (secondView)
      regionY = regionHeight;
  }
  else
    return {};

  const int cropX = std::max(bitmap.x, regionX);
  const int cropY = std::max(bitmap.y, regionY);
  const int cropRight = std::min(bitmap.x + bitmap.width, regionX + regionWidth);
  const int cropBottom = std::min(bitmap.y + bitmap.height, regionY + regionHeight);
  if (cropX >= cropRight || cropY >= cropBottom)
    return {};

  return {cropX,
          cropY,
          cropX - regionX,
          cropY - regionY,
          cropRight - cropX,
          cropBottom - cropY,
          regionWidth,
          regionHeight};
}
} // namespace KODI::VIDEO::SUBTITLES
