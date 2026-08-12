/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Geometry.h"
#include "video/geometry/ContentBarDetector.h"

#include <optional>
#include <string>
#include <vector>

namespace KODI::VIDEO::GEOMETRY
{

//! \brief What a live reading has to clear before it is served. Nothing here is measured in
//! seconds: a live reading scales the picture inside a fixed mask opening, and moves nothing
//! physical that a confirmation window would have to protect.
struct LiveSelectorParams
{
  //! \brief Edges closer together than this are the same edge, in coded pixels. Compared between
  //! vocabulary entries rather than between readings: wider than the gap between neighbouring
  //! ratios and a 2.40 section reads as the 2.20 one already being served.
  unsigned int jitterFloor{8};

  //! \brief Frames a reading that loses picture must persist before it is served. One that gains
  //! picture is served at once. Three is 125 ms at 24 fps.
  unsigned int narrowFrames{3};
};

//! \brief A shape to serve, in coded space.
struct LiveGeometryReading
{
  CRectInt rect;

  //! \brief More than one shape has been served this playback, so no single rectangle describes
  //! the whole title.
  bool varies{false};
};

/*!
 * \brief Says which shape is on screen, this frame.
 *
 * Pure and clockless: every decision is made from the frame in hand and the frames immediately
 * before it.
 *
 * Each reading is resolved against the whole vocabulary rather than against the shapes the stored
 * measurement found, and what is served is the entry's own rectangle. Two frames of the same
 * section therefore resolve to the identical rectangle however much the boundary wandered, and a
 * reading corresponding to no real ratio is refused rather than served.
 *
 * A stream opening and a seek both serve the first reading that follows them immediately,
 * whichever direction it lies in.
 */
class CLiveGeometrySelector
{
public:
  explicit CLiveGeometrySelector(const LiveSelectorParams& params = {});

  /*!
   * \brief Describe the stream being read. Discards all state, including what was served: a
   *        different stream is a different geometry universe.
   *
   * \param coded the analysed region in coded pixels - the frame, or one view of a
   *        stereoscopic frame
   * \param par width of a coded pixel in display pixels; the plausibility gate judges display
   *        ratios, which is what the vocabulary's entries are
   * \param atRestAspect the shape the room is in, which decides which of two entries a reading
   *        between them resolves to
   */
  void Configure(const CRectInt& coded, float par, float atRestAspect = 0.0f);

  //! \return a shape to serve, or nothing when what is already served still describes the frame
  std::optional<LiveGeometryReading> Feed(const DetectionResult& sample);

  //! \brief Adopt new rules without touching the state, so they can be changed in the middle of
  //! a playback.
  void SetParams(const LiveSelectorParams& params) { m_params = params; }

  //! \brief The play position moved, so the next reading is served whatever it is.
  void ResetForSeek();

  bool HasPublished() const { return m_published.has_value(); }

  //! \brief The shape in force, for stamping onto the frame it was read from.
  const std::optional<CRectInt>& Published() const { return m_published; }

  //! \brief The distinct shapes served this playback, in coded pixels, which varies is decided
  //! from. Not what gets written back - the monitor keeps its own filtered list.
  const std::vector<CRectInt>& Shapes() const { return m_shapes; }

  //! \brief One line of state for the player's debug overlay.
  std::string Describe() const;

private:
  struct Run
  {
    CRectInt rect;
    unsigned int frames{0};
  };

  //! \brief The rectangle of \p displayRatio inside the coded frame.
  CRectInt Fit(float displayRatio) const;
  bool SameEdges(const CRectInt& a, const CRectInt& b) const;
  bool LosesPicture(const CRectInt& rect) const;
  std::optional<CRectInt> Snap(const CRectInt& rect);
  std::optional<LiveGeometryReading> Publish(const CRectInt& rect);

  LiveSelectorParams m_params;

  CRectInt m_coded;
  float m_par{1.0f};
  bool m_configured{false};
  float m_atRestAspect{0.0f};

  std::optional<CRectInt> m_published;
  std::optional<Run> m_run;
  std::vector<CRectInt> m_shapes; //!< distinct shapes served this playback

  //! \brief Playback jumped; the next reading is authoritative rather than a change to justify.
  bool m_resync{false};

  unsigned int m_degenerate{0}; //!< frames carrying no reading at all
  unsigned int m_implausible{0}; //!< frames corresponding to no real ratio
  unsigned int m_unconfident{0}; //!< frames the detector had no confidence in
};

} // namespace KODI::VIDEO::GEOMETRY
