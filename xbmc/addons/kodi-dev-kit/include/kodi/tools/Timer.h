/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifdef __cplusplus

#include "Thread.h"

#include <functional>

namespace kodi
{
namespace tools
{

class CTimer : protected CThread
{
public:
  class ITimerCallback;

  explicit CTimer(ITimerCallback* callback)
    : CTimer(std::bind(&ITimerCallback::OnTimeout, callback))
  {
  }

  explicit CTimer(std::function<void()> const& callback) : m_callback(callback) {}

  ~CTimer() override { Stop(true); }

  bool Start(uint64_t timeout, bool interval = false)
  {
    using namespace std::chrono;

    if (m_callback == nullptr || timeout == 0 || IsRunning())
      return false;

    m_timeout = milliseconds(timeout);
    m_interval = interval;

    CreateThread();
    return true;
  }

  bool Stop(bool wait = false)
  {
    if (!IsRunning())
      return false;

    m_threadStop = true;
    m_eventTimeout.notify_all();
    StopThread(wait);

    return true;
  }

  bool Restart()
  {
    using namespace std::chrono;

    if (!IsRunning())
      return false;

    Stop(true);
    return Start(duration_cast<milliseconds>(m_timeout).count(), m_interval);
  }

  void RestartAsync(uint64_t timeout)
  {
    using namespace std::chrono;

    m_timeout = milliseconds(timeout);
    const auto now = system_clock::now();
    m_endTime = now.time_since_epoch() + m_timeout;
    m_eventTimeout.notify_all();
  }

  bool IsRunning() const { return CThread::IsRunning(); }

  float GetElapsedSeconds() const { return GetElapsedMilliseconds() / 1000.0f; }

  float GetElapsedMilliseconds() const
  {
    using namespace std::chrono;

    if (!IsRunning())
      return 0.0f;

    const auto now = system_clock::now();
    return duration_cast<milliseconds>(now.time_since_epoch() - (m_endTime - m_timeout)).count();
  }

  class ITimerCallback
  {
  public:
    virtual ~ITimerCallback() = default;

    virtual void OnTimeout() = 0;
  };

protected:
  void Process() override
  {
    using namespace std::chrono;

    while (!m_threadStop)
    {
      auto currentTime = system_clock::now();
      m_endTime = currentTime.time_since_epoch() + m_timeout;

      // wait the necessary time
      std::mutex mutex;
      std::unique_lock<std::mutex> lock(mutex);
      const auto waitTime = duration_cast<milliseconds>(m_endTime - currentTime.time_since_epoch());
      if (m_eventTimeout.wait_for(lock, waitTime) == std::cv_status::timeout)
      {
        currentTime = system_clock::now();
        if (m_endTime.count() <= currentTime.time_since_epoch().count())
        {
          // execute OnTimeout() callback
          m_callback();

          // continue if this is an interval timer, or if it was restarted during callback
          if (!m_interval && m_endTime.count() <= currentTime.time_since_epoch().count())
            break;
        }
      }
    }
  }

private:
  bool m_interval = false;
  std::function<void()> m_callback;
  std::chrono::system_clock::duration m_timeout;
  std::chrono::system_clock::duration m_endTime;
  std::condition_variable_any m_eventTimeout;
};

} /* namespace tools */
} /* namespace kodi */

#endif /* __cplusplus */
