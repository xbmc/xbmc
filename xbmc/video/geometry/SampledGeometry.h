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

#include <vector>

namespace KODI::VIDEO::GEOMETRY
{

//! \brief What sampling one file produced.
struct SampledGeometry
{
  bool succeeded{false}; //!< the file was opened and at least one picture decoded

  //! \brief Abandoned part way through. Distinct from not having succeeded: a scan that came
  //! to nothing is recorded, an interrupted one leaves no trace.
  bool cancelled{false};

  CRectInt coded; //!< the coded frame, or the analysed view of a stereoscopic one

  //! \brief The ratio the stream declares it is displayed at; zero when it declares none.
  float displayAspect{0.0f};

  CombinedGeometry combined;

  //! Every sample the combiner was given.
  std::vector<GeometrySample> samples;

  //! \brief Points that produced no reading, counted separately from the combiner's totals.
  unsigned int unreadable{0};
};

//! \brief Turn a completed scan into the row stored for it: the display ratio of each shape
//! the title is in, each once. One that read nothing stores no ratio. A cancelled scan must
//! not be stored at all, which this does not check.
ContentGeometryRecord MakeContentGeometryRecord(const SampledGeometry& scan,
                                                const FileIdentity& identity);

} // namespace KODI::VIDEO::GEOMETRY
