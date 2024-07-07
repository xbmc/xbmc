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
      m_lastFrameUs = 0;
      m_sleepEvent.Wait(5000ms);
    }
    else
    {
      if (m_speedFactor > 0.0)
        m_callback->FrameEvent();
      else if (m_speedFactor < 0.0)
        m_callback->RewindEvent();

      if (m_lastFrameUs > 0)
      {
        int64_t frameTimeUs = FrameTimeUs();

        // difference between actual and optimal clamped to frame time
        m_adjustTime = std::clamp(m_lastFrameUs + frameTimeUs - NowUs(), -frameTimeUs, frameTimeUs);
      }
      else
        m_adjustTime = 0;

      m_lastFrameUs = NowUs();

      // Calculate sleep time
      int64_t sleepTimeUs = SleepTimeUs();

      if (sleepTimeUs > 0)
        m_sleepEvent.Wait(std::chrono::microseconds(sleepTimeUs));
    }
  }
}

int64_t CGameLoop::FrameTimeUs() const
{
  if (m_speedFactor != 0.0)
    return round(1000000 / m_fps / std::abs(m_speedFactor));
  else
    return round(1000000 / m_fps / 1);
}

int64_t inline CGameLoop::SleepTimeUs() const
{
  // Calculate next frame time
  const int64_t nextFrameUs = m_lastFrameUs + FrameTimeUs();

  // Calculate sleep time adjusting toward optimal
  int64_t sleepTimeUs = (nextFrameUs - NowUs()) + m_adjustTime;

  // Reset adjust time
  m_adjustTime = 0;

  return std::clamp(sleepTimeUs, static_cast<int64_t>(0), FrameTimeUs());
}

int64_t CGameLoop::NowUs() const
{
  return std::chrono::duration<double, std::micro>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}
