/*
 *      Copyright (C) 2018 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
    static void CalculateViewMode(VIEWMODE viewMode, unsigned int rotationDegCCW, unsigned int sourceWidth, unsigned int sourceHeight, float screenWidth, float screenHeight, float &pixelRatio, float &zoomAmount);
    static void CalcNormalRenderRect(const CRect &viewRect, float outputFrameRatio, float zoomAmount, CRect &destRect);
    static void ClipRect(const CRect &viewRect, CRect &sourceRect, CRect &destRect);
    static std::array<CPoint, 4> ReorderDrawPoints(const CRect &destRect, unsigned int orientationDegCCW, float aspectRatio);
  };
}
}
