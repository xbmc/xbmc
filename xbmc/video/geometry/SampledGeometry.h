/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Geometry.h"
#include "video/geometry/ContentGeometryCombiner.h"
#include "video/geometry/ContentGeometryRecord.h"
#include "video/geometry/FrameSampling.h"

#include <vector>

class CDateTime;

namespace KODI::VIDEO::GEOMETRY
{

//! \brief What sampling one file for its content geometry produced, with the parameters that
//! produced it - the defaults will move.
struct SampledGeometry
{
  bool succeeded{false}; //!< the file was opened and at least one picture decoded

  //! \brief Abandoned part way through, so the readings are incomplete. Distinct from not having
  //! succeeded: a scan that came to nothing is recorded, an interrupted one leaves no trace.
  bool cancelled{false};

  CRectInt coded; //!< the coded frame, or the analysed view of a stereoscopic one

  //! \brief The ratio the stream declares it is displayed at, which the coded frame's own ratio
  //! is not when the pixels are not square. Zero when the stream declares none.
  float displayAspect{0.0f};

  CombinedGeometry combined;

  //! Every sample, kept so a wrong cached answer can be explained afterwards.
  std::vector<GeometrySample> samples;

  //! \brief Points that produced no reading, so nothing about them reached the combiner. Counted
  //! separately from its usable and discarded totals.
  unsigned int unreadable{0};

  SamplingParams sampling;
  CombinerParams combining;
};

//! \brief Names the detector in a stored record's diagnostics. The version it ran at is
//! ContentGeometryRecord::algorithmVersion, and is not repeated here.
inline constexpr const char* CONTENT_GEOMETRY_DETECTOR{"contentbar"};

/*!
 * \brief Turn a completed scan into the row that is stored for it.
 *
 * A scan that did not succeed converts to a Failed record carrying only the identity and the
 * timestamp. A cancelled scan must not be stored at all, which this does not check for.
 *
 * \param identity the file the scan was taken from, established before it was opened
 * \param computed when the scan ran, passed in so the result is a pure function of its arguments
 */
ContentGeometryRecord MakeContentGeometryRecord(const SampledGeometry& scan,
                                                const FileIdentity& identity,
                                                const CDateTime& computed);

} // namespace KODI::VIDEO::GEOMETRY
