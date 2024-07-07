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
    if (m_loopSpeedFactor != m_speedFactor)
    {
      m_lastFrameUs = std::chrono::microseconds::zero();
      m_loopSpeedFactor = m_speedFactor;
    }

    if (m_loopSpeedFactor == 0.0)
    {
      m_sleepEvent.Wait(5000ms);
    }
    else
    {
      if (m_lastFrameUs == std::chrono::microseconds::zero())
        m_lastFrameUs = NowUs();

      if (m_loopSpeedFactor > 0.0)
        m_callback->FrameEvent();
      else if (m_loopSpeedFactor < 0.0)
        m_callback->RewindEvent();

      std::chrono::microseconds nextFrameUs = (m_lastFrameUs += FrameTimeUs());

      std::chrono::microseconds sleepTimeUs = (nextFrameUs - NowUs());

      if (sleepTimeUs > std::chrono::microseconds::zero())
        m_sleepEvent.Wait(sleepTimeUs);
    }
  }
}

std::chrono::microseconds CGameLoop::FrameTimeUs() const
{
  if (m_loopSpeedFactor != 0.0)
    return std::chrono::duration_cast<std::chrono::microseconds>(1s / m_fps /
                                                                 std::abs(m_loopSpeedFactor));
  else
    return std::chrono::duration_cast<std::chrono::microseconds>(1s / m_fps);
}

std::chrono::microseconds CGameLoop::NowUs() const
{
  return std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now().time_since_epoch());
}
