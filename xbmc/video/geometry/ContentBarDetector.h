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

//! \brief Signal range of the luma plane, taken from the stream's color_range. Unspecified is
//! resolved to Limited, the direction that still reports bars when it is wrong.
enum class ColorRange
{
  Unspecified,
  Limited,
  Full,
};

//! \brief A non-owning view of one decoded plane. Samples deeper than 8 bits are little-endian,
//! low-aligned uint16_t, as produced by FFmpeg's YUV*P10/P12/P16 formats.
struct PlaneRef
{
  const uint8_t* data{nullptr};
  int strideBytes{0};
};

//! \brief A non-owning view of one decoded frame. Not VideoPicture, which reaches into libavcodec
//! and the buffer pool; the caller builds one from a picture.
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

  //! \brief Region to analyse, in full-frame coded coordinates. Empty means the whole frame.
  //! A stereoscopic frame is analysed by passing one view; the answer is still full-frame.
  CRectInt roi;
};

//! \brief Derived constants, exposed so tests can pin them. Code-value quantities are 8-bit
//! equivalents and are scaled linearly with bit depth internally.
struct DetectorParams
{
  unsigned int margin{12}; //!< added to reference black to give the bar threshold
  unsigned int flatBand{12}; //!< permitted interquartile range within a bar line
  unsigned int chromaTolerance{16}; //!< permitted deviation of median Cb/Cr from neutral
  unsigned int minRunLength{4}; //!< above-threshold pixels needed to count as an overlay run
  unsigned int minContentExtent{16}; //!< below this many lines the reading is degenerate

  //! \brief Fewer lines than this on an edge is not a bar, and is discarded. Real letterboxing is
  //! tens to hundreds of lines. Matches the jitter floor.
  unsigned int edgeThicknessFloor{8};
  float overlayMaxExtent{0.5f}; //!< an overlay may not span more of the line than this
  float overlaySuspectExtent{0.8f}; //!< wider than overlayMaxExtent but still bounded
  float edgeStepReference{0.15f}; //!< luma step, as a fraction of full scale, scoring 1.0

  //! \brief Advisory penalties. Each scales confidence; none may move an edge. There is
  //! deliberately no penalty for overlaySuspected - see DetectionResult::overlaySuspected.
  float symmetryPenalty{0.5f}; //!< most confidence fully asymmetric bars may cost
  float rangeAssumedPenalty{0.8f}; //!< applied when color_range had to be guessed
  float chromaAbsentPenalty{0.9f}; //!< applied when the neutrality test could not run
};

//! \brief Per-edge boundary sharpness: a genuine letterbox boundary is a large luma step
//! sustained across most of the boundary, where a dark scene's is a small step over a fraction.
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

  //! \brief A black frame carries no reading. rect is left at the whole frame, not the empty
  //! rectangle the walk found.
  bool degenerate{false};

  //! \brief [0,1]: the weakest of the factors below, then reduced by the penalties for anything
  //! that had to be assumed. The weakest rather than an average - a reading is only as good as
  //! the edge least well evidenced.
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
 * \brief Find the content rectangle of a single decoded frame.
 *
 * Pure: no I/O, no services, no state carried between calls. A line is a bar line when it is both
 * dark and flat, judged by robust statistics rather than by a maximum deviation, and when its
 * chroma is achromatic. Rows are resolved first; columns are then resolved over the rows that
 * survived, because on a letterboxed frame every column contains the bars.
 *
 * Temporal policy - what a low-confidence or degenerate sample may do - is not decided here.
 */
DetectionResult DetectContentRect(const FrameRef& frame, const DetectorParams& params = {});

} // namespace KODI::VIDEO::GEOMETRY
