/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DisplayPacing.h"

#include "utils/log.h"

#include <algorithm>
#include <cmath>

using namespace KODI;
using namespace RETRO;

namespace
{
// Frames closer together than this belong to the same screen refresh
constexpr int64_t MIN_INTERVAL_NS = 4'000'000;

// Frames are no longer being taken after this many intervals without one
constexpr int64_t STALE_INTERVALS = 3;

// Paced refreshes between reports of how pacing is going
constexpr int64_t REPORT_REFRESHES = 600;

int64_t ToNs(CDisplayPacing::Clock::time_point when)
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(when.time_since_epoch()).count();
}
} // namespace

void CDisplayPacing::OnFrameTaken(Clock::time_point when, bool newFrame)
{
  const int64_t takeNs = ToNs(when);
  if (m_lastTakeLocalNs != 0 && takeNs - m_lastTakeLocalNs < MIN_INTERVAL_NS)
    return;

  m_lastTakeLocalNs = takeNs;
  m_lastTakeNs.store(takeNs);

  if (!m_paced.load())
    return;

  // A paced frame that isn't ready when the screen takes one, as when the
  // margin is too short, shows the last frame again
  ++m_pacedRefreshes;
  if (!newFrame)
    ++m_missedRefreshes;

  if (m_pacedRefreshes == REPORT_REFRESHES)
  {
    if (m_missedRefreshes > 0)
      CLog::Log(LOGINFO,
                "RetroPlayer[PACING]: {} of the last {} refreshes had no new frame (margin "
                "{:.1f} ms)",
                m_missedRefreshes, m_pacedRefreshes, m_marginNs.load() / 1e6);
    CLog::Log(LOGDEBUG,
              "RetroPlayer[PACING]: Running at {:.3f}% speed, sound drift correction {:+.3f}%",
              PlaybackRate() * 100.0, DriftCorrection() * 100.0);
    m_pacedRefreshes = 0;
    m_missedRefreshes = 0;
  }
}

CDisplayPacing::Clock::duration CDisplayPacing::Interval(Clock::time_point now) const
{
  const double refreshRate = m_refreshRate ? m_refreshRate() : 0.0;
  const int64_t lastTakeNs = m_lastTakeNs.load();
  if (refreshRate <= 0.0 || lastTakeNs == 0)
    return Clock::duration::zero();

  const auto intervalNs = static_cast<int64_t>(std::llround(1e9 / refreshRate));
  if (ToNs(now) - lastTakeNs > STALE_INTERVALS * intervalNs)
    return Clock::duration::zero();

  return std::chrono::duration_cast<Clock::duration>(std::chrono::nanoseconds(intervalNs));
}

CDisplayPacing::Clock::time_point CDisplayPacing::NextTake(Clock::time_point now) const
{
  const int64_t intervalNs =
      std::chrono::duration_cast<std::chrono::nanoseconds>(Interval(now)).count();
  const int64_t nowNs = ToNs(now);

  int64_t nextNs = m_lastTakeNs.load() + intervalNs;
  if (intervalNs > 0 && nextNs <= nowNs)
    nextNs += ((nowNs - nextNs) / intervalNs + 1) * intervalNs;

  return Clock::time_point(
      std::chrono::duration_cast<Clock::duration>(std::chrono::nanoseconds(nextNs)));
}
