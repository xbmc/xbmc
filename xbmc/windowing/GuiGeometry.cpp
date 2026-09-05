/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GuiGeometry.h"

#include "utils/AspectRatioVocabulary.h"

#include <cmath>

using KODI::UTILS::CAspectRatioVocabulary;

namespace KODI::WINDOWING
{

namespace
{

// The largest rectangle of display-space ratio `aspect` centred in `area`, whose pixels are
// `pixelRatio` wide. `area` itself when it already has that shape within tolerance.
CRect HoldToAspect(const CRect& area, float aspect, float pixelRatio)
{
  const float ratio = pixelRatio > 0.0f ? pixelRatio : 1.0f;
  const float areaAspect = area.Width() * ratio / area.Height();

  if (CAspectRatioVocabulary::Distance(aspect, areaAspect) <= CAspectRatioVocabulary::Tolerance())
    return area;

  float width = area.Width();
  float height = area.Height();

  if (aspect > areaAspect)
    height = width * ratio / aspect; // wider than the area, so width binds
  else
    width = height * aspect / ratio; // narrower, so height binds

  const float x = area.x1 + (area.Width() - width) * 0.5f;
  const float y = area.y1 + (area.Height() - height) * 0.5f;
  return {x, y, x + width, y + height};
}

} // unnamed namespace

CRect ComputeRasterRect(const RESOLUTION_INFO& info, float aspect)
{
  const CRect display{0.0f, 0.0f, static_cast<float>(info.iWidth),
                      static_cast<float>(info.iHeight)};

  if (!(aspect > 0.0f) || info.iWidth <= 0 || info.iHeight <= 0)
    return display;

  CRect area{static_cast<float>(info.Overscan.left), static_cast<float>(info.Overscan.top),
             static_cast<float>(info.Overscan.right), static_cast<float>(info.Overscan.bottom)};
  if (area.Width() <= 0.0f || area.Height() <= 0.0f)
    area = display;

  const CRect held = HoldToAspect(area, aspect, info.fPixelRatio);

  return {std::round(held.x1), std::round(held.y1), std::round(held.x2), std::round(held.y2)};
}

} // namespace KODI::WINDOWING
