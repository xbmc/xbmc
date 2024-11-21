/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>

// image alignment for <aspect>keep</aspect>, <aspect>scale</aspect> or <aspect>center</aspect>
inline constexpr unsigned int ASPECT_ALIGN_CENTER = 0;
inline constexpr unsigned int ASPECT_ALIGN_LEFT = 1;
inline constexpr unsigned int ASPECT_ALIGN_RIGHT = 2;
inline constexpr unsigned int ASPECT_ALIGNY_CENTER = 0;
inline constexpr unsigned int ASPECT_ALIGNY_TOP = 4;
inline constexpr unsigned int ASPECT_ALIGNY_BOTTOM = 8;
inline constexpr unsigned int ASPECT_ALIGN_MASK = 3;
inline constexpr unsigned int ASPECT_ALIGNY_MASK = ~3;

class CAspectRatio
{
public:
  enum class AspectRatio
  {
    // Do not keep aspect ratio. Image size = box size.
    STRETCH = 0,
    // Keep aspect ratio. Image size >= box size.
    SCALE,
    // Keep aspect ratio. Image size <= box size.
    KEEP,
    // Do not scale image.
    CENTER
  };
  using enum AspectRatio;

  CAspectRatio(AspectRatio aspect = STRETCH)
  {
    ratio = aspect;
    align = ASPECT_ALIGN_CENTER | ASPECT_ALIGNY_CENTER;
    scaleDiffuse = true;
  }

  CAspectRatio(AspectRatio aspect, uint32_t al, bool scaleD)
    : ratio(aspect), align(al), scaleDiffuse(scaleD)
  {
  }

  bool operator!=(const CAspectRatio& right) const
  {
    if (ratio != right.ratio)
      return true;
    if (align != right.align)
      return true;
    if (scaleDiffuse != right.scaleDiffuse)
      return true;
    return false;
  };

  AspectRatio ratio;
  uint32_t align;
  bool scaleDiffuse;
};
