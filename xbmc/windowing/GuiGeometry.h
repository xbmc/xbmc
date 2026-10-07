/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Geometry.h"
#include "windowing/Resolution.h"

namespace KODI::WINDOWING
{

//! \brief The largest rectangle of \p rasterAspect centred in the calibrated area, or the whole
//! display when \p rasterAspect is zero or below.
CRect ComputeRasterRect(const RESOLUTION_INFO& info, float rasterAspect);

//! \brief The rectangle the interface occupies, and in \p scaleX and \p scaleY the skin-to-screen
//! scale that puts it there.
CRect ComputeGuiRect(const RESOLUTION_INFO& skinRes,
                     const RESOLUTION_INFO& info,
                     float rasterAspect,
                     const CRect& guiContentRect,
                     bool keepShape,
                     float zoomFraction,
                     float& scaleX,
                     float& scaleY);

//! \brief The display, bounded by the raster and by \p heldRect, which is empty for no hold.
CRect ComputeClipBounds(const CRect& display, const CRect& raster, const CRect& heldRect);

//! \brief The stated shape, or none while a screen tool is on screen.
float ComputeRasterAspectInForce(float stated, bool screenTool);

//! \brief Whether the hold is in force: asked for, with no fullscreen video and no screen tool.
bool ComputeGuiKeepShape(bool hold, bool fullScreenVideo, bool screenTool);

} // namespace KODI::WINDOWING
