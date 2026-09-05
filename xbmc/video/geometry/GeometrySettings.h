/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/geometry/ContentGeometryCombiner.h"
#include "video/geometry/EffectiveGeometry.h"
#include "video/geometry/FrameSampling.h"
#include "video/geometry/LiveGeometrySelector.h"

namespace KODI::VIDEO::GEOMETRY
{

bool ContentGeometryEnabledFromSettings();

//! \brief Whether anything is measured away from the playing picture. With it off the only
//! reading is live detection.
bool ContentGeometryNonLiveFromSettings();

//! \brief How the user wants a title whose geometry varies to resolve to one rectangle.
VariableGeometryPolicy ContentGeometryPolicyFromSettings();

//! \brief The shape the room rests at: what a measurement errs toward, and what is published
//! while nothing is playing.
float ContentGeometryAtRestFromSettings();

//! \brief The ratio the raster-shape setting names, or zero for the full screen. The setting
//! only; RasterAspect() applies any runtime override.
float RasterAspectFromSettings();

//! \brief How thoroughly to sample, shared by the scan, the sweep and the pre-playback pass.
SamplingParams ContentGeometrySamplingFromSettings(SamplingDepth depth);

//! \brief How samples are combined into one answer. Only the varies threshold is settable.
CombinerParams ContentGeometryCombiningFromSettings();

//! \brief Whether live in-playback detection is wanted, and the rules it reads by. Refreshed
//! periodically during a playback.
struct LiveGeometrySettings
{
  //! \brief Off unless both detection as a whole and live tracking are on.
  bool enabled{false};

  LiveSelectorParams selector;

  //! \brief Runtime excluded at each end of the title, live's own values defaulted to the
  //! scan's.
  double leadInSeconds{120.0};
  double leadOutSeconds{60.0};

  //! \brief Width of the reduced copy a hardware-decoded picture is read through. Settable in
  //! advancedsettings.xml.
  unsigned int reductionWidth{960};
};

LiveGeometrySettings LiveGeometryFromSettings();

} // namespace KODI::VIDEO::GEOMETRY
