/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef>
#include <utility>
#include <vector>

namespace KODI::VIDEO::GEOMETRY
{

/*!
 * \brief How hard to look, for a measurement someone has asked for by name.
 *
 * Sampling density decides whether a title is found to change shape at all, and the ordinary
 * figure has to be affordable across a whole library. Thorough raises it for one title, at
 * roughly a second a point.
 */
enum class SamplingDepth
{
  //! \brief The viewer's own figure, escalating when what it finds suggests the title varies.
  Normal,

  //! \brief Dense enough that a sequence of a minute cannot fall between two points. Does not
  //! escalate: there is nothing to escalate to.
  Thorough,
};

struct SamplingParams
{
  unsigned int points{9}; //!< sample positions across the title

  //! \brief Runtime to leave unsampled at the start, in seconds. Never zero: opening logos and
  //! idents are how a scope film gets recorded as full frame. Absolute, a logo chain being the
  //! same length whatever it fronts.
  double leadInSeconds{120.0};

  //! \brief Runtime to leave unsampled at the end, in seconds. Credits are frequently
  //! full-container, and a roll of text on black reads as the shape of the text.
  double leadOutSeconds{60.0};

  //! \brief Share of the runtime to sample when the exclusions do not fit inside it, which keeps
  //! a clip shorter than the two combined measurable.
  double shortTitleWindow{0.80};

  //! \brief Pictures to decode at each position. More than one does not vote; it measures
  //! whether the content there is stable, which is a reason to discard the position outright.
  unsigned int picturesPerPoint{1};

  //! \brief Points to use on a second pass, when the first pass looks inconclusive. Paying this
  //! on every title would triple a library sweep. Zero disables escalation.
  unsigned int escalatedPoints{27};

  //! \brief Discard rate above which the first pass is treated as inconclusive. The corpus
  //! separates cleanly: 0% and 11% on two fixed-ratio titles against 44% on the varying one.
  float escalateDiscardShare{0.34f};
};

/*!
 * \brief Is a first sampling pass inconclusive enough to be worth densifying?
 *
 * \param clusters distinct geometries the first pass found
 * \param usable samples that survived
 * \param discarded samples thrown away as degenerate or untrustworthy
 * \param unexplainedShapes readings of a shape the answer does not account for, which were not
 *        trusted on their own. One is enough to escalate.
 */
bool ShouldEscalate(std::size_t clusters,
                    unsigned int usable,
                    unsigned int discarded,
                    unsigned int unexplainedShapes = 0,
                    const SamplingParams& params = {});

//! \brief Offsets from \p candidates that are not already covered by \p sampled, so escalation
//! reuses the first pass rather than repeating it.
std::vector<double> UnsampledOffsets(const std::vector<double>& candidates,
                                     const std::vector<double>& sampled,
                                     double minSeparation = 1.0);

/*!
 * \brief Positions, in seconds, at which to sample a title of the given duration.
 *
 * Spread across the window, endpoints included within it but never at t=0 or the very end.
 * Returns empty for a duration or point count that cannot be sampled.
 */
std::vector<double> SampleOffsets(double durationSeconds, const SamplingParams& params = {});

//! \brief The first and last position that will be sampled, in seconds.
std::pair<double, double> SampleWindow(double durationSeconds, const SamplingParams& params = {});

/*!
 * \brief Is this moment of playback inside the opening or closing exclusion?
 *
 * The live analogue of SamplingParams::leadInSeconds and leadOutSeconds. Judged on playback
 * position rather than elapsed wall clock, so a resume past the opening is not blind; a source
 * with no duration excludes nothing. A title too short for the exclusions to fit falls back to
 * the central shortTitleWindow share.
 */
bool WithinLiveLeadExclusion(double positionSeconds,
                             double durationSeconds,
                             double leadInSeconds,
                             double leadOutSeconds,
                             const SamplingParams& params = {});

} // namespace KODI::VIDEO::GEOMETRY
