/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GuiGeometry.h"

#include <cmath>

namespace KODI::WINDOWING
{

CRect ComputeAspectRect(const CRect& area, float aspect, float pixelRatio)
{
  if (!(aspect > 0.0f) || area.Width() <= 0.0f || area.Height() <= 0.0f)
    return area;

  const float ratio = pixelRatio > 0.0f ? pixelRatio : 1.0f;
  const float areaAspect = area.Width() * ratio / area.Height();

  float width = area.Width();
  float height = area.Height();

  if (aspect > areaAspect)
    height = width * ratio / aspect; // wider than the area, so width binds
  else
    width = height * aspect / ratio; // narrower, so height binds

  const float x = area.x1 + (area.Width() - width) * 0.5f;
  const float y = area.y1 + (area.Height() - height) * 0.5f;

  return {std::round(x), std::round(y), std::round(x + width), std::round(y + height)};
}

} // namespace KODI::WINDOWING
