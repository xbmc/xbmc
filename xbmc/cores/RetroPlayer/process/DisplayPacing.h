/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

namespace KODI
{
namespace RETRO
{
/*!
 * \brief When the screen takes the game's frames, and how fast the game runs
 * to keep up with it
 *
 * The renderer reports each frame it takes for the screen. The game loop uses
 * that to finish every frame just before the next one is taken, which runs the
 * game at the display's rate rather than its own, and the audio stream
 * resamples by the same rate so that the sound keeps time with the picture.
 */
class CDisplayPacing
{
public:
  using Clock = std::chrono::steady_clock;

  /*!
   * \brief A frame is being taken for the screen
   *
   * Called from the rendering thread only.
   */
  void OnFrameTaken(Clock::time_point when);

  /*!
   * \brief The time between frames taken for the screen
   *
   * \return The interval, or zero if frames aren't being taken regularly
   */
  Clock::duration Interval(Clock::time_point now) const;

  /*!
   * \brief When the next frame will be taken after now
   *
   * Only meaningful while Interval() is not zero.
   */
  Clock::time_point NextTake(Clock::time_point now) const;

  /*!
   * \brief Whether the game may run at the screen's rate instead of its own
   */
  bool Enabled() const { return m_enabled.load(); }
  void SetEnabled(bool enabled) { m_enabled.store(enabled); }

  /*!
   * \brief How far the game's rate may be from the screen's, as a fraction,
   * for the game to run at the screen's rate
   */
  double MaxRateDifference() const { return m_maxRateDifference.load(); }
  void SetMaxRateDifference(double difference) { m_maxRateDifference.store(difference); }

  /*!
   * \brief Time left between a paced frame finishing and the screen taking
   * it, to absorb jitter in when the screen takes frames
   */
  Clock::duration Margin() const
  {
    return std::chrono::duration_cast<Clock::duration>(std::chrono::nanoseconds(m_marginNs.load()));
  }
  void SetMargin(Clock::duration margin)
  {
    m_marginNs.store(std::chrono::duration_cast<std::chrono::nanoseconds>(margin).count());
  }

  /*!
   * \brief The speed the game runs at, relative to its own frame rate
   */
  double PlaybackRate() const { return m_playbackRate.load(); }
  void SetPlaybackRate(double rate) { m_playbackRate.store(rate); }

private:
  void Restart(int64_t takeNs);

  // Rendering thread only
  int64_t m_runStartNs{0};
  int64_t m_runFrames{0};
  int64_t m_lastTakeLocalNs{0};
  int64_t m_intervalFrames{0};

  std::atomic<bool> m_enabled{true};
  std::atomic<double> m_maxRateDifference{0.02};
  std::atomic<int64_t> m_marginNs{2'500'000};

  // Published for the game loop
  std::atomic<int64_t> m_lastTakeNs{0};
  std::atomic<int64_t> m_intervalNs{0};
  std::atomic<double> m_playbackRate{1.0};
};
} // namespace RETRO
} // namespace KODI
