/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Geometry.h"

#include <span>
#include <vector>

namespace KODI::VIDEO::GEOMETRY
{

//! \brief Confidence below which a lone reading corroborates nothing, and only recurrence
//! redeems it.
inline constexpr float MIN_CORROBORATING_CONFIDENCE = 0.05f;

//! \brief One frame's reading, as produced by the detector at one sample point.
struct GeometrySample
{
  CRectInt rect; //!< content rectangle in coded space
  float confidence{0.0f};
  bool degenerate{false}; //!< carries no reading at all - never combine as a narrow one
  double position{0.0}; //!< seconds into the title, retained for diagnostics
};

struct CombinerParams
{
  //! \brief Edges closer together than this are the same edge, absorbing boundary ringing.
  unsigned int tolerance{8};

  //! \brief Confidence at which one sample corroborates a cluster on its own. Not a filter: a
  //! sample below it still joins a cluster.
  float minConfidence{MIN_CORROBORATING_CONFIDENCE};

  //! \brief Members a cluster needs to corroborate itself with no confident sample.
  unsigned int minStationarySamples{3};

  //! \brief Share of samples a rival cluster needs before content counts as varying.
  //! Settable in advancedsettings.xml.
  float variesShare{0.10f};

  //! \brief Samples a rival cluster needs before it is a cluster at all.
  unsigned int minRivalSamples{2};

  //! \brief Ratio difference at which an uncorroborated reading is a different shape rather
  //! than a noisy one.
  float distinctAspectShare{0.05f};
};

//! \brief A group of samples that agreed on the same rectangle.
struct GeometryCluster
{
  CRectInt rect; //!< the median of the cluster's members, component-wise
  unsigned int samples{0};
  float weight{0.0f}; //!< summed confidence
};

struct CombinedGeometry
{
  CRectInt rect; //!< the answer; the coded rectangle when there is no reading

  //! \brief Outer extent of every corroborated cluster, per axis. Equals rect unless the title
  //! varies.
  CRectInt envelope;

  //! \brief The title's geometry changes partway through, decided on how often it differs
  //! rather than on confidence.
  bool varies{false};

  bool hasReading{false}; //!< false when no sample survived; rect is then the coded frame

  //! \brief Dominant cluster's share of the surviving weight, or of the samples when nothing
  //! scored.
  float share{0.0f};
  unsigned int usable{0}; //!< samples in a cluster that corroborated itself
  unsigned int discarded{0}; //!< carried no reading, or landed in a cluster nothing corroborated

  //! \brief Uncorroborated readings of a shape the answer does not account for. Not part of
  //! the answer; marks a pass a denser one would settle.
  unsigned int unexplainedShapes{0};

  //! \brief Every cluster, dominant first, retained for post-mortem.
  std::vector<GeometryCluster> clusters;
};

/*!
 * \brief Reduce per-sample readings to one rectangle, plus whether the title varies. Pure.
 *
 * The answer is always a rectangle that occurred: the dominant stationary cluster, not the
 * widest. \p coded is returned when nothing survives.
 */
CombinedGeometry CombineGeometrySamples(std::span<const GeometrySample> samples,
                                        const CRectInt& coded,
                                        const CombinerParams& params = {});

} // namespace KODI::VIDEO::GEOMETRY
