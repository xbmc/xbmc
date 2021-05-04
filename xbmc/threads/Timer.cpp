/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Timer.h"

#include <algorithm>

CTimer::CTimer(std::function<void()> const& callback)
  : CThread("Timer"),
    m_callback(callback),
    m_timeout(std::chrono::milliseconds(0)),
    m_interval(false)
{ }

CTimer::CTimer(ITimerCallback *callback)
  : CTimer(std::bind(&ITimerCallback::OnTimeout, callback))
{ }

CTimer::~CTimer()
{
  Stop(true);
}

bool CTimer::Start(uint32_t timeout, bool interval /* = false */)
{
  if (m_callback == NULL || timeout == 0 || IsRunning())
    return false;

  m_timeout = std::chrono::milliseconds(timeout);
  m_interval = interval;

  Create();
  return true;
}

bool CTimer::Stop(bool wait /* = false */)
{
  if (!IsRunning())
    return false;

  m_bStop = true;
  m_eventTimeout.Set();
  StopThread(wait);

  return true;
}

void CTimer::RestartAsync(uint32_t timeout)
{
  m_timeout = std::chrono::milliseconds(timeout);
  m_endTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout);
  m_eventTimeout.Set();
}

bool CTimer::Restart()
{
  if (!IsRunning())
    return false;

  Stop(true);

  //! @todo: fix method to use std::chrono::milliseconds
  return Start(m_timeout.count(), m_interval);
}

float CTimer::GetElapsedSeconds() const
{
  return GetElapsedMilliseconds() / 1000.0f;
}

float CTimer::GetElapsedMilliseconds() const
{
  if (!IsRunning())
    return 0.0f;

  auto now = std::chrono::steady_clock::now();
  std::chrono::duration<float, std::milli> duration = (now - (m_endTime - m_timeout));

  return duration.count();
}

void CTimer::Process()
{
  while (!m_bStop)
  {
    auto currentTime = std::chrono::steady_clock::now();
    m_endTime = currentTime + m_timeout;

    // wait the necessary time
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(m_endTime - currentTime);

    if (!m_eventTimeout.WaitMSec(duration.count()))
    {
      currentTime = std::chrono::steady_clock::now();
      if (m_endTime <= currentTime)
      {
        // execute OnTimeout() callback
        m_callback();

        // continue if this is an interval timer, or if it was restarted during callback
        if (!m_interval && m_endTime <= currentTime)
          break;
      }
    }
  }
}
