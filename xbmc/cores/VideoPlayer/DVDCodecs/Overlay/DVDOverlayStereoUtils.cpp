/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDOverlayStereoUtils.h"

namespace KODI::VIDEO::SUBTITLES
{
bool IsStereoscopicOutputMode(RenderStereoMode renderMode)
{
  return renderMode != RenderStereoMode::OFF && renderMode != RenderStereoMode::HARDWAREBASED;
}

bool ShouldRenderStereoOverlay(DVDOverlayStereoView overlayView,
                               RenderStereoView renderView,
                               std::string_view sourceStereoMode)
{
  if (overlayView == DVDOverlayStereoView::BOTH)
    return true;

  if (sourceStereoMode == "right_left" || sourceStereoMode == "bottom_top")
  {
    if (renderView == RenderStereoView::LEFT)
      renderView = RenderStereoView::RIGHT;
    else if (renderView == RenderStereoView::RIGHT)
      renderView = RenderStereoView::LEFT;
  }

  if (renderView == RenderStereoView::OFF)
    return overlayView == DVDOverlayStereoView::LEFT;

  return (overlayView == DVDOverlayStereoView::LEFT) == (renderView == RenderStereoView::LEFT);
}
} // namespace KODI::VIDEO::SUBTITLES
