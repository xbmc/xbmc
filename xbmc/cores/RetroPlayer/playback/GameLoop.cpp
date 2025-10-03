/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameLoop.h"

#include <chrono>
#include <cmath>

using namespace KODI;
using namespace RETRO;
using namespace std::chrono_literals;

namespace
{
// Default FPS value in case fps is 0 (which shouldn't happen)
constexpr double DEFAULT_FPS = 60.0;

// Duration to sleep while the game loop is paused
constexpr auto PAUSE_SLEEP = 5s;
} // namespace

CGameLoop::CGameLoop(IGameLoopCallback* callback, double fps)
  : CThread("GameLoop"),
    m_callback(callback),
    m_fps(fps ? fps : DEFAULT_FPS)
{
}

CGameLoop::~CGameLoop()
{
  // Ensure the game loop is stopped when the object is destroyed
  Stop();
}

void CGameLoop::Start()
{
  // Start the thread by invoking the base class's Create method
  Create();
}

void CGameLoop::Stop()
{
  // Request to stop the thread
  StopThread(false);

  // Wake up the thread if it's waiting on the sleep event
  m_sleepEvent.Set();

  // Wait until the thread is fully stopped
  StopThread(true);
}

void CGameLoop::SetSpeed(double speedFactor)
{
  // Set the new speed factor
  m_speedFactor.store(speedFactor);

  // Wake up the thread to apply the new speed immediately
  m_sleepEvent.Set();
}

void CGameLoop::PauseAsync()
{
  // Pause the game loop asynchronously by setting speed to 0
  SetSpeed(0.0);
}

void CGameLoop::Process(void)
{
  // Main game loop that runs until the thread is requested to stop
  while (!m_bStop)
  {
    // If the speed factor has changed, reset the last frame time and update
    // the loop speed factor
    const double speed = m_speedFactor.load();
    if (m_loopSpeedFactor != speed)
    {
      // Reset last frame time
      m_lastFrameUs = std::chrono::microseconds::zero();

      // Update loop speed factor
      m_loopSpeedFactor = speed;
    }

    // If the game is paused, wait before checking again
    if (m_loopSpeedFactor == 0.0)
    {
      // Wait for a long duration if paused
      m_sleepEvent.Wait(PAUSE_SLEEP);
    }
    else
    {
      // If it's the first frame or after a reset, set the last frame time to
      // the current time
      if (m_lastFrameUs == std::chrono::microseconds::zero())
        m_lastFrameUs = NowUs();

      // Trigger the appropriate callback depending on the direction of time
      // (forward or rewind)
      if (m_loopSpeedFactor > 0.0)
        m_callback->FrameEvent();
      else if (m_loopSpeedFactor < 0.0)
        m_callback->RewindEvent();

      // Calculate the time for the next frame by adding the frame duration to
      // the last frame time
      const std::chrono::microseconds nextFrameUs = m_lastFrameUs + FrameTimeUs();

      // Update the last frame time to ensure accurate timing for the next loop
      // iteration
      m_lastFrameUs = nextFrameUs;

      // Calculate how much time to sleep before the next frame should be processed
      const std::chrono::microseconds sleepTimeUs = (nextFrameUs - NowUs());

      // Only sleep if there is time left before the next frame
      if (sleepTimeUs > std::chrono::microseconds::zero())
        m_sleepEvent.Wait(sleepTimeUs);
    }
  }
}

std::chrono::microseconds CGameLoop::FrameTimeUs() const
{
  // Calculate the duration of one frame in microseconds based on the FPS and
  // speed factor
  const double factor = m_loopSpeedFactor != 0.0 ? std::abs(m_loopSpeedFactor) : 1.0;
  return std::chrono::duration_cast<std::chrono::microseconds>(1s / m_fps / factor);
}

std::chrono::microseconds CGameLoop::NowUs() const
{
  // Get the current time in microseconds since the steady clock epoch
  return std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now().time_since_epoch());
}
