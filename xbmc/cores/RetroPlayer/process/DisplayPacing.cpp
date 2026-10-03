/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DisplayPacing.h"

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

  // Measure against this run once it has an interval of its own
  const int64_t runIntervalNs =
      m_runFrames > 1 ? (m_lastTakeLocalNs - m_runStartNs) / (m_runFrames - 1) : 0;
  const int64_t referenceNs = runIntervalNs != 0 ? runIntervalNs : m_intervalNs.load();

  int64_t refreshes = 1;
  if (referenceNs != 0)
  {
    // A refresh can pass without a frame being taken, so count it. A gap that
    // isn't a whole number of refreshes would skew the run, so start another.
    const double gapRefreshes = static_cast<double>(gapNs) / referenceNs;
    refreshes = std::llround(gapRefreshes);
    if (refreshes < 1 || std::abs(gapRefreshes - refreshes) > MAX_GAP_DEVIATION)
    {
      m_runStartNs = takeNs;
      m_runFrames = 1;
      m_lastTakeLocalNs = takeNs;
      m_lastTakeNs.store(takeNs);
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
    m_intervalFrames = elapsedFrames;
  }
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
  m_runStartNs = takeNs;
  m_runFrames = 1;
  m_lastTakeLocalNs = takeNs;
  m_lastTakeNs.store(takeNs);
  m_intervalNs.store(0);
  m_intervalFrames = 0;
}
