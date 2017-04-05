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
#include "games/addons/GameClient.h"
#include "threads/SingleLock.h"
#include "threads/SystemClock.h"

#include <cmath>

using namespace GAME;

#define DEFAULT_FPS  60  // In case fps is 0 (shouldn't happen)
#define FOREVER_MS   (7 * 24 * 60 * 60 * 1000) // 1 week is large enough

CGameLoop::CGameLoop(IGameLoopCallback* callback, double fps) :
  CThread("GameLoop"),
  m_callback(callback),
  m_fps(fps ? fps : DEFAULT_FPS),
  m_speedFactor(0.0),
  m_lastFrameMs(0.0)
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
  {
    CSingleLock lock(m_mutex);
    m_speedFactor = speedFactor;
  }
  m_sleepEvent.Set();
}

void CGameLoop::Process(void)
{
  double nextFrameMs = NowMs();

  CSingleLock lock(m_mutex);

  while (!m_bStop)
  {
    double speedFactor = m_speedFactor;

    {
      CSingleExit exit(m_mutex);
      if (speedFactor > 0.0)
        m_callback->FrameEvent();
      else if (speedFactor < 0.0)
        m_callback->RewindEvent();
    }

    // Record frame time
    m_lastFrameMs = nextFrameMs;

    // Calculate sleep time
    double nowMs = NowMs();
    double sleepTimeMs = SleepTimeMs(nowMs);

    // Sleep at least 1 ms to avoid sleeping forever
    while (sleepTimeMs > 1.0)
    {
      {
        CSingleExit exit(m_mutex);
        m_sleepEvent.WaitMSec(static_cast<unsigned int>(sleepTimeMs));
      }

      if (m_bStop)
        break;

      // Speed may have changed, update sleep time
      nowMs = NowMs();
      sleepTimeMs = SleepTimeMs(nowMs);
    }

    // Calculate next frame time
    nextFrameMs += FrameTimeMs();

    // If sleep time goes negative, we fell behind, so fast-forward to now
    if (sleepTimeMs < 0.0)
      nextFrameMs = nowMs;
  }
}

double CGameLoop::FrameTimeMs() const
{
  if (m_speedFactor != 0.0)
    return 1000.0 / m_fps / std::abs(m_speedFactor);
  else
    return FOREVER_MS;
}

double CGameLoop::SleepTimeMs(double nowMs) const
{
  // Calculate next frame time
  const double nextFrameMs = m_lastFrameMs + FrameTimeMs();

  // Calculate sleep time
  const double sleepTimeMs = nextFrameMs - nowMs;

  return sleepTimeMs;
}

double CGameLoop::NowMs() const
{
  return static_cast<double>(XbmcThreads::SystemClockMillis());
}
