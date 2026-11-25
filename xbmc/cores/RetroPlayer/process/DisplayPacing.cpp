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

// Where the display could follow a game, one that running in step would speed
// up or slow down by less than this still runs in step: the change can't be
// heard, and its frames still start just in time for the screen
constexpr double MIN_FOLLOWED_RATE_CHANGE = 0.005;

int64_t ToNs(CDisplayPacing::Clock::time_point when)
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(when.time_since_epoch()).count();
}
} // namespace

bool CDisplayPacing::FollowsGame(double fps,
                                 double minRate,
                                 double maxRate,
                                 double maxRateDifference)
{
  if (minRate <= 0.0 || fps < minRate || fps > maxRate)
    return false;

  // In step, the game would run at the rate of the display's mode
  const double rateChange = std::abs(maxRate / fps - 1.0);
  return maxRateDifference <= 0.0 || rateChange > maxRateDifference ||
         rateChange >= MIN_FOLLOWED_RATE_CHANGE;
}

void CDisplayPacing::OnFrameRun()
{
  if (m_followedFrameNs.load() != 0)
    m_frameRun.Set();
}

void CDisplayPacing::WaitForNextFrame()
{
  // Counted from when the last frame was drawn rather than run, so a frame that
  // takes longer to run than the one before needs more than a frame's time
  const int64_t frameNs = m_followedFrameNs.load();
  if (frameNs != 0)
    m_frameRun.Wait(std::chrono::nanoseconds(2 * frameNs));
}

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
              "RetroPlayer[PACING]: Running at {:.3f}% speed, sound drift correction {:+.3f}%, "
              "sound delay {:.1f} ms (holding {:.1f} ms)",
              PlaybackRate() * 100.0, DriftCorrection() * 100.0, m_soundDelaySecs.load() * 1000.0,
              m_soundHeldSecs.load() * 1000.0);
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
