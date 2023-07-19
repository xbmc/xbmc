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

#define DEFAULT_FPS 60 // In case fps is 0 (shouldn't happen)
#define FOREVER_MS (7 * 24 * 60 * 60 * 1000) // 1 week is large enough

CGameLoop::CGameLoop(IGameLoopCallback* callback, double fps)
  : CThread("GameLoop"), m_callback(callback), m_fps(fps ? fps : DEFAULT_FPS), m_speedFactor(0.0)
{
}

CGameLoop::~CGameLoop()
{
  Stop();
}

void CGameLoop::Start()
{
  Create();
}

void CGameLoop::Stop()
{
  StopThread(false);
  m_sleepEvent.Set();
  StopThread(true);
}

void CGameLoop::SetSpeed(double speedFactor)
{
  m_speedFactor = speedFactor;

  m_sleepEvent.Set();
}

void CGameLoop::PauseAsync()
{
  SetSpeed(0.0);
}

void CGameLoop::Process(void)
{
  while (!m_bStop)
  {
    if (m_speedFactor == 0.0)
    {
      m_lastFrameMs = 0.0;
      m_sleepEvent.Wait(5000ms);
    }
    else
    {
      if (m_speedFactor > 0.0)
        m_callback->FrameEvent();
      else if (m_speedFactor < 0.0)
        m_callback->RewindEvent();

      if (m_lastFrameMs > 0.0)
      {
        m_lastFrameMs += FrameTimeMs();
        m_adjustTime = m_lastFrameMs - NowMs();
      }
      else
      {
        m_lastFrameMs = NowMs();
        m_adjustTime = 0.0;
      }

      // Calculate sleep time
      double sleepTimeMs = SleepTimeMs();

      // Sleep at least 1 ms to avoid sleeping forever
      while (sleepTimeMs > 1.0)
      {
        m_sleepEvent.Wait(std::chrono::milliseconds(static_cast<unsigned int>(sleepTimeMs)));

        if (m_bStop)
          break;

        // Speed may have changed, update sleep time
        sleepTimeMs = SleepTimeMs();
      }
    }
  }
}

double CGameLoop::FrameTimeMs() const
{
  if (m_speedFactor != 0.0)
    return 1000.0 / m_fps / std::abs(m_speedFactor);
  else
    return 1000.0 / m_fps / 1.0;
}

double CGameLoop::SleepTimeMs() const
{
  // Calculate next frame time
  const double nextFrameMs = m_lastFrameMs + FrameTimeMs();

  // Calculate sleep time
  double sleepTimeMs = (nextFrameMs - NowMs()) + m_adjustTime;

  // Reset adjust time
  m_adjustTime = 0.0;

  // Positive or zero
  sleepTimeMs = (sleepTimeMs >= 0.0 ? sleepTimeMs : 0.0);

  return sleepTimeMs;
}

double CGameLoop::NowMs() const
{
  return std::chrono::duration<double, std::milli>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}
