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
#include <functional>
#include <utility>

namespace KODI
{
namespace RETRO
{
/*!
 * \brief When the screen takes the game's frames, and how fast the game runs
 * to keep up with it
 *
 * The screen's refresh rate comes from the display's reference clock, which
 * follows its vblanks and its mode changes. The renderer reports each frame it
 * takes for the screen, which places those refreshes in time. The game loop
 * uses both to finish every frame just before the next one is taken, which
 * runs the game at the display's rate rather than its own, and the audio
 * stream resamples by the same rate so that the sound keeps time with the
 * picture.
 */
class CDisplayPacing
{
public:
  using Clock = std::chrono::steady_clock;

  /*!
   * \brief Where the screen's refresh rate comes from
   *
   * \param refreshRate Returns the rate in Hz, or zero or less while it isn't
   *        known, as while the display changes mode
   *
   * Set before the game loop starts and cleared after it stops.
   */
  void SetRefreshRateSource(std::function<double()> refreshRate)
  {
    m_refreshRate = std::move(refreshRate);
  }

  /*!
   * \brief A frame is being taken for the screen
   *
   * \param newFrame False if the game hasn't given a frame since the last one
   *
   * Called from the rendering thread only.
   */
  void OnFrameTaken(Clock::time_point when, bool newFrame = true);

  /*!
   * \brief The time between frames taken for the screen
   *
   * \return The interval, or zero if the refresh rate isn't known or frames
   *         aren't being taken
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

  /*!
   * \brief How much the sound is sped up or slowed down, as a fraction, to
   * absorb the drift between the game's clock and the sound card's
   */
  double DriftCorrection() const { return m_driftCorrection.load(); }
  void SetDriftCorrection(double correction) { m_driftCorrection.store(correction); }

  /*!
   * \brief For the log: the sound's delay, as the drift correction sees it, and
   * the delay it holds, in seconds
   */
  void SetSoundDelay(double delaySecs, double heldSecs)
  {
    m_soundDelaySecs.store(delaySecs);
    m_soundHeldSecs.store(heldSecs);
  }

  /*!
   * \brief Whether the game is running in step with the screen, which every
   * refresh should then show a new frame of
   */
  void SetPaced(bool paced) { m_paced.store(paced); }

private:
  std::function<double()> m_refreshRate;

  // Rendering thread only
  int64_t m_lastTakeLocalNs{0};
  int64_t m_pacedRefreshes{0};
  int64_t m_missedRefreshes{0};

  std::atomic<bool> m_enabled{true};
  std::atomic<double> m_maxRateDifference{0.02};
  std::atomic<int64_t> m_marginNs{2'500'000};
  std::atomic<bool> m_paced{false};

  // Published for the game loop
  std::atomic<int64_t> m_lastTakeNs{0};
  std::atomic<double> m_playbackRate{1.0};
  std::atomic<double> m_driftCorrection{0.0};
  std::atomic<double> m_soundDelaySecs{0.0};
  std::atomic<double> m_soundHeldSecs{0.0};
};
} // namespace RETRO
} // namespace KODI
