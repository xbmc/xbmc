/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/GameSettings.h"
#include "utils/Geometry.h"

#include <array>

namespace KODI
{
namespace RETRO
{
class CRenderUtils
{
public:
  static void CalculateStretchMode(STRETCHMODE stretchMode,
                                   unsigned int rotationDegCCW,
                                   unsigned int sourceWidth,
                                   unsigned int sourceHeight,
                                   float screenWidth,
                                   float screenHeight,
                                   float& pixelRatio,
                                   float& zoomAmount);

  static void CalcNormalRenderRect(const CRect& viewRect,
                                   float outputFrameRatio,
                                   float zoomAmount,
                                   CRect& destRect);

  static void ClipRect(const CRect& viewRect, CRect& sourceRect, CRect& destRect);

  static void CropSource(CRect& sourceRect,
                         unsigned int rotationDegCCW,
                         float viewWidth,
                         float viewHeight,
                         float sourceWidth,
                         float sourceHeight,
                         float destWidth,
                         float destHeight);

  static std::array<CPoint, 4> ReorderDrawPoints(const CRect& destRect,
                                                 unsigned int orientationDegCCW);
};
} // namespace RETRO
} // namespace KODI
