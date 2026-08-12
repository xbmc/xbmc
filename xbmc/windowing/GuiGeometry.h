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

/*!
 * \file GuiGeometry.h
 * \brief Where the raster is and where the interface sits inside it. Pure arithmetic.
 */

namespace KODI::WINDOWING
{

/*! \brief The raster for a resolution and a stated shape.
 The largest rectangle of the stated shape centred in the calibrated area. The whole display
 when no shape is stated, and the whole calibrated area when the stated shape is the area's own
 within the vocabulary tolerance, so that it loses no hairline to rounding.
 \param info the resolution, whose Overscan carries the calibration
 \param rasterAspect the stated shape, or zero and below for "the display's own"
 */
CRect ComputeRasterRect(const RESOLUTION_INFO& info, float rasterAspect);

/*! \brief Where the interface is laid out.
 The calibration minus the GUI insets, bounded by the raster when a shape is stated,
 intersected with any playback confinement, held to the skin's own proportions when asked,
 and finally grown by the skin zoom.
 \param skinRes the resolution the skin is authored at
 \param info the resolution being drawn to
 \param rasterAspect the stated raster shape, or zero and below for none
 \param guiContentRect the playback confinement, empty for none
 \param keepShape hold the skin's authored proportions rather than stretching
 \param zoomFraction the skin zoom as a fraction, zero for none
 \param scaleX horizontal skin-to-screen scale for the resulting rectangle
 \param scaleY vertical skin-to-screen scale for the resulting rectangle
 \return the rectangle the interface occupies, in screen coordinates
 */
CRect ComputeGuiRect(const RESOLUTION_INFO& skinRes,
                     const RESOLUTION_INFO& info,
                     float rasterAspect,
                     const CRect& guiContentRect,
                     bool keepShape,
                     float zoomFraction,
                     float& scaleX,
                     float& scaleY);

/*! \brief The furthest the interface may reach.
 The display bounded by the raster whenever one is in force, and additionally by the rectangle
 the interface occupies while its shape is being held - which also bounds a skin zoom, so zooming
 grows the interface within the raster rather than past it.
 \param heldRect the rectangle the interface is held to, empty for no hold in force
 */
CRect ComputeClipBounds(const CRect& display, const CRect& raster, const CRect& heldRect);

/*! \brief The shape in force, which is no shape at all while a screen tool is on - a tool drawn
 inside a correction would be aligned against the last answer rather than against the screen.
 \param stated the shape the viewer or an automation has stated, zero for none
 \param screenTool whether a screen tool is on screen
 */
float ComputeRasterAspectInForce(float stated, bool screenTool);

/*! \brief Whether the interface is holding its own shape.
 \param hold whether the hold is asked for
 \param fullScreenVideo whether the picture is showing, where the OSD spans the picture
 \param screenTool whether a screen tool is on screen, which holds nothing
 */
bool ComputeGuiKeepShape(bool hold, bool fullScreenVideo, bool screenTool);

} // namespace KODI::WINDOWING
