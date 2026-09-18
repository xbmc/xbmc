/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/RetroPlayerTypes.h"
#include "utils/Geometry.h"

#include <algorithm>
#include <cmath>

namespace KODI::RETRO
{
class CRenderGeometryFBO
{
public:
  static CRect GetDestinationRect(const ViewportCoordinates& destCoords)
  {
    CRect bounds{destCoords[0], destCoords[0]};
    for (const CPoint& point : destCoords)
    {
      bounds.x1 = std::min(bounds.x1, point.x);
      bounds.y1 = std::min(bounds.y1, point.y);
      bounds.x2 = std::max(bounds.x2, point.x);
      bounds.y2 = std::max(bounds.y2, point.y);
    }
    return bounds;
  }

  static CRect GetTextureCoordinates(CRect sourceRect,
                                     unsigned int frameHeight,
                                     unsigned int textureWidth,
                                     unsigned int textureHeight,
                                     bool bottomLeftOrigin)
  {
    if (bottomLeftOrigin)
    {
      // Reflect around the frame, independently of the crop and backing allocation.
      sourceRect.y1 = frameHeight - sourceRect.y1;
      sourceRect.y2 = frameHeight - sourceRect.y2;
    }

    sourceRect.x1 /= textureWidth;
    sourceRect.x2 /= textureWidth;
    sourceRect.y1 /= textureHeight;
    sourceRect.y2 /= textureHeight;
    return sourceRect;
  }
};
} // namespace KODI::RETRO
