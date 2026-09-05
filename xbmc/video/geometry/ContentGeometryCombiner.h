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

//! \brief Confidence below which a lone reading corroborates nothing - a title card reads as
//! whatever shape the words make, and only recurrence redeems it.
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
  //! \brief Edges closer together than this are the same edge. A genuine letterbox boundary is
  //! baked into the coding, so this only absorbs the odd line of boundary ringing.
  unsigned int tolerance{8};

  //! \brief Confidence at which a single sample corroborates a cluster on its own. Not a filter
  //! - a sample below this still joins a cluster.
  float minConfidence{MIN_CORROBORATING_CONFIDENCE};

  //! \brief Members a cluster needs to corroborate itself without any confident sample. A real
  //! edge lands in the same place every frame where a false one tracks content and moves.
  unsigned int minStationarySamples{3};

  //! \brief Share of samples a rival cluster needs before content counts as varying.
  //! Settable in advancedsettings.xml.
  float variesShare{0.10f};

  //! \brief Samples a rival cluster needs before it is a cluster at all, stationarity being a
  //! claim about repetition.
  unsigned int minRivalSamples{2};

  //! \brief Ratio difference at which an uncorroborated reading is a different shape rather than
  //! a noisy one. Well clear of the vocabulary's own 2% tolerance.
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

  //! \brief Outer extent of every corroborated cluster, on each axis independently, so a title
  //! varying horizontally is covered too. Equals rect unless the title varies.
  CRectInt envelope;

  //! \brief The title's geometry changes partway through. Decided on how often the geometry
  //! differs rather than on confidence, which is biased toward the unletterboxed reading.
  bool varies{false};

  bool hasReading{false}; //!< false when no sample survived; rect is then the coded frame

  //! \brief Dominant cluster's share of the surviving weight, or of the surviving samples when
  //! nothing in the title scored.
  float share{0.0f};
  unsigned int usable{0}; //!< samples in a cluster that corroborated itself
  unsigned int discarded{0}; //!< carried no reading, or landed in a cluster nothing corroborated

  //! \brief Uncorroborated readings of a shape the answer does not account for. Mark a pass a
  //! denser one would settle; not part of the answer.
  unsigned int unexplainedShapes{0};

  //! \brief Every cluster, dominant first, retained so a wrong cached answer can be post-mortemed.
  std::vector<GeometryCluster> clusters;
};

/*!
 * \brief Reduce per-sample readings to one rectangle, plus whether the title varies. Pure.
 *
 * Clusters whole rectangles rather than combining edges independently, so the answer is always a
 * rectangle that occurred, and keeps the dominant stationary cluster rather than the widest - one
 * full-frame ident would otherwise collapse a scope film to "no bars".
 *
 * \param coded the coded frame rectangle, returned when nothing survives, uncertainty resolving
 *              wider rather than narrower
 */
CombinedGeometry CombineGeometrySamples(std::span<const GeometrySample> samples,
                                        const CRectInt& coded,
                                        const CombinerParams& params = {});

} // namespace KODI::VIDEO::GEOMETRY
