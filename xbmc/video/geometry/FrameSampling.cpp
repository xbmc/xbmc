/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FrameSampling.h"

#include <algorithm>
#include <cmath>

namespace KODI::VIDEO::GEOMETRY
{

std::pair<double, double> SampleWindow(double durationSeconds, const SamplingParams& params)
{
  if (durationSeconds <= 0.0)
    return {0.0, 0.0};

  const double leadIn = std::max(params.leadInSeconds, 0.0);
  const double leadOut = std::max(params.leadOutSeconds, 0.0);

  double start = leadIn;
  double end = durationSeconds - leadOut;
  if (end <= start)
  {
    const double share = std::clamp(params.shortTitleWindow, 0.0, 1.0);
    const double margin = durationSeconds * (1.0 - share) / 2.0;
    start = margin;
    end = durationSeconds - margin;
  }
  return {start, end};
}

bool WithinLiveLeadExclusion(double positionSeconds,
                             double durationSeconds,
                             double leadInSeconds,
                             double leadOutSeconds,
                             const SamplingParams& params)
{
  if (durationSeconds <= 0.0)
    return false;

  SamplingParams window = params;
  window.leadInSeconds = leadInSeconds;
  window.leadOutSeconds = leadOutSeconds;
  const auto [start, end] = SampleWindow(durationSeconds, window);
  return positionSeconds < start || positionSeconds > end;
}

std::vector<double> SampleOffsets(double durationSeconds, const SamplingParams& params)
{
  std::vector<double> offsets;
  if (durationSeconds <= 0.0 || params.points == 0)
    return offsets;

  const auto [start, end] = SampleWindow(durationSeconds, params);

  offsets.reserve(params.points);
  if (params.points == 1)
  {
    offsets.push_back((start + end) / 2.0);
    return offsets;
  }

  for (unsigned int i = 0; i < params.points; ++i)
  {
    offsets.push_back(start + (end - start) * static_cast<double>(i) /
                                  static_cast<double>(params.points - 1));
  }
  return offsets;
}

bool ShouldEscalate(std::size_t clusters,
                    unsigned int usable,
                    unsigned int discarded,
                    unsigned int unexplainedShapes,
                    const SamplingParams& params)
{
  if (params.escalatedPoints <= params.points)
    return false;

  // More than one geometry found: densify to tell the title varying from a single odd reading.
  if (clusters > 1)
    return true;

  // A shape the answer does not account for, seen once: the only sign the sampling caught the
  // edge of something.
  if (unexplainedShapes > 0)
    return true;

  const unsigned int total = usable + discarded;
  if (total == 0)
    return true; // nothing survived at all, so nothing has been measured

  return static_cast<float>(discarded) / static_cast<float>(total) > params.escalateDiscardShare;
}

std::vector<double> UnsampledOffsets(const std::vector<double>& candidates,
                                     const std::vector<double>& sampled,
                                     double minSeparation)
{
  std::vector<double> wanted;
  for (const double candidate : candidates)
  {
    const bool covered = std::any_of(sampled.begin(), sampled.end(), [&](double taken)
                                     { return std::abs(taken - candidate) < minSeparation; });
    if (!covered)
      wanted.push_back(candidate);
  }
  return wanted;
}

} // namespace KODI::VIDEO::GEOMETRY
