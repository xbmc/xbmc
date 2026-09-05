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

//! \brief How hard to look, for a measurement asked for by name.
enum class SamplingDepth
{
  Normal, //!< the viewer's own figure, escalating when what it finds suggests the title varies
  Thorough, //!< dense enough that a minute-long sequence cannot fall between two points
};

struct SamplingParams
{
  unsigned int points{9}; //!< sample positions across the title

  //! \brief Runtime to leave unsampled at the start, in seconds. Never zero.
  double leadInSeconds{120.0};

  //! \brief Runtime to leave unsampled at the end, in seconds.
  double leadOutSeconds{60.0};

  //! \brief Share of the runtime to sample when the exclusions do not fit inside it.
  double shortTitleWindow{0.80};

  //! \brief Pictures to decode at each position. More than one measures whether the content
  //! there is stable rather than voting.
  unsigned int picturesPerPoint{1};

  //! \brief Points for a second pass when the first looks inconclusive. Zero disables it.
  unsigned int escalatedPoints{27};

  //! \brief Discard rate above which the first pass is inconclusive.
  float escalateDiscardShare{0.34f};
};

//! \brief Whether a first sampling pass is inconclusive enough to densify. One
//! \p unexplainedShapes reading is enough on its own.
bool ShouldEscalate(std::size_t clusters,
                    unsigned int usable,
                    unsigned int discarded,
                    unsigned int unexplainedShapes = 0,
                    const SamplingParams& params = {});

//! \brief Offsets from \p candidates not already covered by \p sampled.
std::vector<double> UnsampledOffsets(const std::vector<double>& candidates,
                                     const std::vector<double>& sampled,
                                     double minSeparation = 1.0);

//! \brief Positions, in seconds, at which to sample a title of \p durationSeconds. Spread
//! across the window, never at t=0 or the very end. Empty when it cannot be sampled.
std::vector<double> SampleOffsets(double durationSeconds, const SamplingParams& params = {});

//! \brief The first and last position that will be sampled, in seconds.
std::pair<double, double> SampleWindow(double durationSeconds, const SamplingParams& params = {});

//! \brief Whether \p positionSeconds is inside the opening or closing exclusion. Judged on
//! playback position, not wall clock; a source with no duration excludes nothing.
bool WithinLiveLeadExclusion(double positionSeconds,
                             double durationSeconds,
                             double leadInSeconds,
                             double leadOutSeconds,
                             const SamplingParams& params = {});

} // namespace KODI::VIDEO::GEOMETRY
