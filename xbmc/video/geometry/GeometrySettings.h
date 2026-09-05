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

//! \brief Should anything be measured away from the playing picture - before a title plays, as a
//! scan adds it, and by the background sweep? Implies the switch above. With it off the only
//! reading is live detection, from the frames on screen.
bool ContentGeometryNonLiveFromSettings();

//! \brief How the user wants a title whose geometry varies to resolve to one rectangle.
VariableGeometryPolicy ContentGeometryPolicyFromSettings();

//! \brief The shape the room rests at: the ratio a measurement errs toward between two
//! vocabulary entries, and the shape published while nothing is playing.
float ContentGeometryAtRestFromSettings();

//! \brief The ratio the raster-shape setting names, or zero for the full screen. The setting
//! only - CApplicationContentGeometry::RasterAspect() applies any runtime override.
float RasterAspectFromSettings();

//! \brief How thoroughly to sample, so that a stored row means the same thing whether the scan,
//! the sweep or the pre-playback pass produced it.
SamplingParams ContentGeometrySamplingFromSettings(SamplingDepth depth);

//! \brief How samples are combined into one answer. Only the varies threshold is settable.
CombinerParams ContentGeometryCombiningFromSettings();

//! \brief Whether live in-playback detection is wanted, and the rules it reads by. Refreshed
//! periodically during a playback, so the rules can be changed while a film plays.
struct LiveGeometrySettings
{
  //! \brief Off unless both detection as a whole and live tracking are on. A room whose video
  //! processor tracks the picture itself runs with detection on and this off.
  bool enabled{false};

  LiveSelectorParams selector;

  //! \brief Runtime excluded at each end of the title. Live's own values, defaulted to the
  //! scan's: excluding a region live is that many minutes served from the cached or container
  //! value.
  double leadInSeconds{120.0};
  double leadOutSeconds{60.0};

  /*!
   * \brief Width of the reduced copy a hardware-decoded picture is read through.
   * Settable in advancedsettings.xml.
   *
   * At this width a reduced reading resolves to the same vocabulary entry as the full-resolution
   * one wherever both pass the serving gates, with a worst edge error of 4 coded pixels on a 4K
   * frame - half the selector's jitter floor. Half this width lost twice as many dark HDR frames
   * to the confidence gate.
   */
  unsigned int reductionWidth{960};
};

LiveGeometrySettings LiveGeometryFromSettings();

} // namespace KODI::VIDEO::GEOMETRY
