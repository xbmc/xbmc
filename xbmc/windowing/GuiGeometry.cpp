/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GuiGeometry.h"

#include "utils/AspectRatioVocabulary.h"

#include <algorithm>
#include <cmath>

using KODI::UTILS::CAspectRatioVocabulary;

namespace KODI::WINDOWING
{

namespace
{

// The largest rectangle of display-space ratio `aspect` centred in `area`, whose pixels are
// `pixelRatio` wide. `area` itself when it already has that shape within the vocabulary
// tolerance, which holds 1.78 where 16:9 is 1.7778.
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

  // Centred, because nothing here knows better than the middle.
  const float x = area.x1 + (area.Width() - width) * 0.5f;
  const float y = area.y1 + (area.Height() - height) * 0.5f;
  return {x, y, x + width, y + height};
}

} // unnamed namespace

CRect ComputeRasterRect(const RESOLUTION_INFO& info, float rasterAspect)
{
  const CRect display{0.0f, 0.0f, static_cast<float>(info.iWidth),
                      static_cast<float>(info.iHeight)};

  if (!(rasterAspect > 0.0f) || info.iWidth <= 0 || info.iHeight <= 0)
    return display;

  // The stated shape describes the calibrated area rather than the pixel grid: a band centred in
  // the grid would sit off-centre on a screen the calibration says is not.
  CRect area{static_cast<float>(info.Overscan.left), static_cast<float>(info.Overscan.top),
             static_cast<float>(info.Overscan.right), static_cast<float>(info.Overscan.bottom)};
  if (area.Width() <= 0.0f || area.Height() <= 0.0f)
    area = display;

  const CRect held = HoldToAspect(area, rasterAspect, info.fPixelRatio);

  // Whole pixels: the interface, the clip and the picture are derived from this rectangle by
  // different code with different rounding, and a fractional edge lets them disagree by a pixel
  // - a sliver of picture showing past the OSD at the seam.
  return {std::round(held.x1), std::round(held.y1), std::round(held.x2), std::round(held.y2)};
}

CRect ComputeGuiRect(const RESOLUTION_INFO& skinRes,
                     const RESOLUTION_INFO& info,
                     float rasterAspect,
                     const CRect& guiContentRect,
                     bool keepShape,
                     float zoomFraction,
                     float& scaleX,
                     float& scaleY)
{
  const float fromWidth = static_cast<float>(skinRes.iWidth);
  const float fromHeight = static_cast<float>(skinRes.iHeight);

  CRect layout{static_cast<float>(info.Overscan.left + info.guiInsets.left),
               static_cast<float>(info.Overscan.top + info.guiInsets.top),
               static_cast<float>(info.Overscan.right - info.guiInsets.right),
               static_cast<float>(info.Overscan.bottom - info.guiInsets.bottom)};

  // Intersected rather than substituted, so the overscan calibration is respected inside it.
  // Only when a shape is stated: with none, the calibration may legally place the interface
  // outside the pixel grid.
  if (rasterAspect > 0.0f)
    layout.Intersect(ComputeRasterRect(info, rasterAspect));

  // Taken as an intersection rather than a further inset, so that whatever is playing can
  // only ever pull the GUI in from the calibration and never push it past one.
  if (!guiContentRect.IsEmpty())
    layout.Intersect(guiContentRect);

  // Skins are authored at one shape, and scaleX and scaleY below are otherwise free to disagree
  // - which stretches a 16:9 skin a third too wide in a scope raster. Held before the zoom, so
  // zooming still grows the interface rather than unstretching it.
  if (keepShape && layout.Width() > 0.0f && layout.Height() > 0.0f && fromHeight > 0.0f)
    layout = HoldToAspect(layout, skinRes.DisplayRatio(), info.fPixelRatio);

  float toPosX = layout.x1;
  float toPosY = layout.y1;
  float toWidth = layout.Width();
  float toHeight = layout.Height();

  float zoom = zoomFraction;
  toPosX -= toWidth * zoom * 0.5f;
  toWidth *= zoom + 1.0f;

  // adjust for aspect ratio as zoom is given in the vertical direction and we don't
  // do aspect ratio corrections in the gui code
  zoom = zoom / info.fPixelRatio;
  toPosY -= toHeight * zoom * 0.5f;
  toHeight *= zoom + 1.0f;

  // A stated shape rounding to no area would put an infinity into the transform every
  // coordinate passes through, at which point nothing draws - including the settings needed to
  // state a different one. The interface is left where the calibration puts it instead.
  if (!(toWidth > 0.0f) || !(toHeight > 0.0f))
  {
    scaleX = 1.0f;
    scaleY = 1.0f;
    return {static_cast<float>(info.Overscan.left), static_cast<float>(info.Overscan.top),
            static_cast<float>(info.Overscan.right), static_cast<float>(info.Overscan.bottom)};
  }

  scaleX = fromWidth / toWidth;
  scaleY = fromHeight / toHeight;

  // Recorded after the zoom, because a viewer who has zoomed the interface up meant it to be
  // that size.
  return {toPosX, toPosY, toPosX + toWidth, toPosY + toHeight};
}

CRect ComputeClipBounds(const CRect& display, const CRect& raster, const CRect& heldRect)
{
  // The raster bounds the interface whichever way it is being filled. Placing the layout is not
  // enough on its own: skins park panels outside it and slide them in - Estuary's left-hand
  // panels sit 320 units off (Includes_Animations.xml) - and those draw wherever they land.
  CRect bounds = display;
  bounds.Intersect(raster);

  // While the interface holds its own shape, the clip is the area it occupies. The raster stays
  // in force above it, so a zoomed interface is cut at the raster's edge.
  if (!heldRect.IsEmpty())
    bounds.Intersect(heldRect);

  return bounds;
}

float ComputeRasterAspectInForce(float stated, bool screenTool)
{
  return screenTool ? 0.0f : stated;
}

bool ComputeGuiKeepShape(bool hold, bool fullScreenVideo, bool screenTool)
{
  return hold && !fullScreenVideo && !screenTool;
}

} // namespace KODI::WINDOWING
