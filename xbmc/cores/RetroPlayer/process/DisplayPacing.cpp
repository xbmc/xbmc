/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DisplayPacing.h"

#include <algorithm>
#include <cmath>

using namespace KODI;
using namespace RETRO;

namespace
{
// Frames closer together than this belong to the same screen refresh
constexpr int64_t MIN_INTERVAL_NS = 4'000'000;

// A gap longer than this means the screen stopped taking frames for a while
constexpr int64_t MAX_GAP_NS = 100'000'000;

// How far a gap may be from a whole number of refreshes and still count as
// regular, as a fraction of a refresh
constexpr double MAX_GAP_DEVIATION = 0.25;

// Regular frames needed before their interval is used
constexpr int64_t MIN_RUN_FRAMES = 30;

// A run is measured over this many frames at most, so that the interval
// follows the screen if its refresh rate changes
constexpr int64_t MAX_RUN_FRAMES = 300;

// Frames are no longer being taken regularly after this many intervals
// without one
constexpr int64_t STALE_INTERVALS = 3;

int64_t ToNs(CDisplayPacing::Clock::time_point when)
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(when.time_since_epoch()).count();
}
} // namespace

void CDisplayPacing::OnFrameTaken(Clock::time_point when)
{
  const int64_t takeNs = ToNs(when);
  const int64_t gapNs = takeNs - m_lastTakeLocalNs;

  if (m_lastTakeLocalNs != 0 && gapNs < MIN_INTERVAL_NS)
    return;

  if (m_lastTakeLocalNs == 0 || gapNs > MAX_GAP_NS)
  {
    Restart(takeNs);
    return;
  }

  // A run is measured against its own interval, which its first gap sets, so
  // that a screen whose refresh rate changes can start a run at the new rate
  int64_t refreshes = 1;
  if (m_runFrames > 1)
  {
    const int64_t runIntervalNs = (m_lastTakeLocalNs - m_runStartNs) / (m_runFrames - 1);

    // A refresh can pass without a frame being taken, so count it. A gap that
    // isn't a whole number of refreshes would skew the run, so start another.
    // So does a gap of several refreshes before the run is long enough to be
    // used, as the run may have measured a fraction of the real interval.
    const double gapRefreshes = static_cast<double>(gapNs) / runIntervalNs;
    refreshes = std::llround(gapRefreshes);
    if (refreshes < 1 || (refreshes > 1 && m_runFrames - 1 < MIN_RUN_FRAMES) ||
        std::abs(gapRefreshes - refreshes) > MAX_GAP_DEVIATION)
    {
      StartRun(takeNs);
      return;
    }
  }

  m_runFrames += refreshes;
  m_lastTakeLocalNs = takeNs;
  m_lastTakeNs.store(takeNs);

  // Measured across the whole run rather than between neighbours, so that
  // jitter in when each frame is taken doesn't reach the rate the game and its
  // sound run at. A shorter run never replaces a longer one's interval.
  const int64_t elapsedFrames = m_runFrames - 1;
  if (elapsedFrames >= MIN_RUN_FRAMES && elapsedFrames >= m_intervalFrames)
  {
    m_intervalNs.store((takeNs - m_runStartNs) / elapsedFrames);
    m_intervalFrames = std::min(elapsedFrames, MAX_RUN_FRAMES);
  }

  if (elapsedFrames >= MAX_RUN_FRAMES)
    StartRun(takeNs);
}

CDisplayPacing::Clock::duration CDisplayPacing::Interval(Clock::time_point now) const
{
  const int64_t intervalNs = m_intervalNs.load();
  if (intervalNs == 0)
    return Clock::duration::zero();

  if (ToNs(now) - m_lastTakeNs.load() > STALE_INTERVALS * intervalNs)
    return Clock::duration::zero();

  return std::chrono::duration_cast<Clock::duration>(std::chrono::nanoseconds(intervalNs));
}

CDisplayPacing::Clock::time_point CDisplayPacing::NextTake(Clock::time_point now) const
{
  const int64_t intervalNs = m_intervalNs.load();
  const int64_t lastNs = m_lastTakeNs.load();
  const int64_t nowNs = ToNs(now);

  int64_t nextNs = lastNs + intervalNs;
  if (intervalNs > 0 && nextNs <= nowNs)
    nextNs += ((nowNs - nextNs) / intervalNs + 1) * intervalNs;

  return Clock::time_point(
      std::chrono::duration_cast<Clock::duration>(std::chrono::nanoseconds(nextNs)));
}

void CDisplayPacing::Restart(int64_t takeNs)
{
  StartRun(takeNs);
  m_intervalNs.store(0);
  m_intervalFrames = 0;
}

void CDisplayPacing::StartRun(int64_t takeNs)
{
  m_runStartNs = takeNs;
  m_runFrames = 1;
  m_lastTakeLocalNs = takeNs;
  m_lastTakeNs.store(takeNs);
}
