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
#define ASPECT_ALIGN_CENTER 0
#define ASPECT_ALIGN_LEFT 1
#define ASPECT_ALIGN_RIGHT 2
#define ASPECT_ALIGNY_CENTER 0
#define ASPECT_ALIGNY_TOP 4
#define ASPECT_ALIGNY_BOTTOM 8
#define ASPECT_ALIGN_MASK 3
#define ASPECT_ALIGNY_MASK ~3

class CAspectRatio
{
public:
  enum ASPECT_RATIO
  {
    // Do not keep aspect ratio. Image size = box size.
    AR_STRETCH = 0,
    // Keep aspect ratio. Image size >= box size.
    AR_SCALE,
    // Keep aspect ratio. Image size <= box size.
    AR_KEEP,
    // Do not scale image.
    AR_CENTER
  };

  CAspectRatio(ASPECT_RATIO aspect = AR_STRETCH)
  {
    ratio = aspect;
    align = ASPECT_ALIGN_CENTER | ASPECT_ALIGNY_CENTER;
    scaleDiffuse = true;
  }

  CAspectRatio(ASPECT_RATIO aspect, uint32_t al, bool scaleD)
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

  ASPECT_RATIO ratio;
  uint32_t align;
  bool scaleDiffuse;
};
