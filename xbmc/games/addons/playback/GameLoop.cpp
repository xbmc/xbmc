/*
 *      Copyright (C) 2016-2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GameLoop.h"
#include "threads/SystemClock.h"

#include <cmath>

using namespace KODI;
using namespace GAME;

#define DEFAULT_FPS  60  // In case fps is 0 (shouldn't happen)
#define FOREVER_MS   (7 * 24 * 60 * 60 * 1000) // 1 week is large enough

CGameLoop::CGameLoop(IGameLoopCallback* callback, double fps) :
  CThread("GameLoop"),
  m_callback(callback),
  m_fps(fps ? fps : DEFAULT_FPS),
  m_speedFactor(0.0),
  m_lastFrameMs(0.0),
  m_adjustTime(0.0)
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
      m_sleepEvent.WaitMSec(5000);
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
        m_sleepEvent.WaitMSec(static_cast<unsigned int>(sleepTimeMs));

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
  return static_cast<double>(XbmcThreads::SystemClockMillis());
}
