/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Geometry.h"

#include <cstdint>

namespace KODI::VIDEO::GEOMETRY
{

//! \brief Chroma plane subsampling of the frame handed to the detector.
enum class ChromaSubsampling
{
  YUV420,
  YUV422,
  YUV444,
};

//! \brief Signal range of the luma plane. Unspecified resolves to Limited.
enum class ColorRange
{
  Unspecified,
  Limited,
  Full,
};

//! \brief A non-owning view of one decoded plane. Samples deeper than 8 bits are little-endian
//! low-aligned uint16_t.
struct PlaneRef
{
  const uint8_t* data{nullptr};
  int strideBytes{0};
};

//! \brief A non-owning view of one decoded frame, which the caller builds from a picture.
struct FrameRef
{
  unsigned int width{0}; //!< full coded luma width
  unsigned int height{0}; //!< full coded luma height
  unsigned int bitDepth{8}; //!< bits per component, 8..16
  ChromaSubsampling subsampling{ChromaSubsampling::YUV420};
  ColorRange range{ColorRange::Unspecified};

  PlaneRef y;
  PlaneRef u; //!< optional; when either chroma plane is absent the neutrality test is skipped
  PlaneRef v;

  //! \brief Region to analyse, in full-frame coded coordinates; empty for the whole frame.
  CRectInt roi;
};

//! \brief Detection constants. Code values are 8-bit equivalents, scaled with bit depth
//! internally.
struct DetectorParams
{
  unsigned int margin{12}; //!< added to reference black to give the bar threshold
  unsigned int flatBand{12}; //!< permitted interquartile range within a bar line
  unsigned int chromaTolerance{16}; //!< permitted deviation of median Cb/Cr from neutral
  unsigned int minRunLength{4}; //!< above-threshold pixels needed to count as an overlay run
  unsigned int minContentExtent{16}; //!< below this many lines the reading is degenerate

  //! \brief Fewer lines than this on an edge is not a bar and is discarded.
  unsigned int edgeThicknessFloor{8};
  float overlayMaxExtent{0.5f}; //!< an overlay may not span more of the line than this
  float overlaySuspectExtent{0.8f}; //!< wider than overlayMaxExtent but still bounded
  float edgeStepReference{0.15f}; //!< luma step, as a fraction of full scale, scoring 1.0

  //! \brief Advisory penalties. Each scales confidence; none may move an edge.
  float symmetryPenalty{0.5f}; //!< most confidence fully asymmetric bars may cost
  float rangeAssumedPenalty{0.8f}; //!< applied when color_range had to be guessed
  float chromaAbsentPenalty{0.9f}; //!< applied when the neutrality test could not run
};

//! \brief Per-edge boundary sharpness. A letterbox boundary is a large luma step across most
//! of the boundary; a dark scene's is a small step over a fraction of it.
struct EdgeMetrics
{
  bool measured{false}; //!< false when this edge has no bar at all, so no boundary exists
  unsigned int thickness{0}; //!< bar lines removed on this edge
  float step{0.0f}; //!< luma step across the boundary as a fraction of full scale
  float coverage{0.0f}; //!< fraction of the boundary showing that step
};

//! \brief The detector's per-frame answer.
struct DetectionResult
{
  CRectInt rect; //!< content rectangle in full-frame coded coordinates

  //! \brief A black frame carries no reading, and rect is left at the whole frame.
  bool degenerate{false};

  //! \brief [0,1]: the weakest of the factors below rather than their average, then reduced by
  //! the penalties for anything that had to be assumed.
  float confidence{0.0f};

  EdgeMetrics top;
  EdgeMetrics bottom;
  EdgeMetrics left;
  EdgeMetrics right;

  float separation{0.0f}; //!< threshold headroom on the worst bar line
  float flatness{0.0f}; //!< dispersion headroom on the worst bar line
  float symmetry{0.0f}; //!< top/bottom and left/right bar agreement, advisory only

  unsigned int overlayLines{0}; //!< bar lines admitted by the overlay rule
  bool overlaySuspected{false}; //!< a bounded run too wide to admit stopped the walk
  bool rangeAssumed{false}; //!< color_range was Unspecified and Limited was assumed
  bool chromaTested{false}; //!< both chroma planes were present
  bool subFloorEdgesIgnored{false}; //!< an edge thinner than the floor was found and dropped

  unsigned int thresholdUsed{0}; //!< bar threshold in native code values
  unsigned int blackFloorObserved{0}; //!< darkest bar level seen, in native code values
};

/*!
 * \brief Find the content rectangle of a single decoded frame. Pure: no I/O, no state.
 *
 * A line is a bar line when it is dark, flat by robust statistics, and achromatic. Rows are
 * resolved first, then columns over the rows that survived. Temporal policy is not decided
 * here.
 */
DetectionResult DetectContentRect(const FrameRef& frame, const DetectorParams& params = {});

} // namespace KODI::VIDEO::GEOMETRY
