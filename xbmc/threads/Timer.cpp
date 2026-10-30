/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Timer.h"

#include "threads/Thread.h"
#include "utils/log.h"

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <vector>

using namespace std::chrono_literals;

class CTimer::CTimerThread : public CThread
{
public:
  static CTimerThread& Get()
  {
    // Never destroyed, so that timers can still be stopped during static destruction
    static auto* thread = new CTimerThread();
    return *thread;
  }

  // A timer is running while it is scheduled and while its callback runs
  bool IsActive(const CTimer* timer) const
  {
    return timer == m_current || std::ranges::find(m_timers, timer) != m_timers.end();
  }

  void Add(CTimer* timer)
  {
    m_timers.push_back(timer);
    if (!CThread::IsRunning())
      Create();
    m_wakeUp.notify_one();
  }

  std::mutex m_mutex;
  std::condition_variable m_wakeUp;
  std::condition_variable m_callbackDone;
  std::vector<CTimer*> m_timers;
  const CTimer* m_current{nullptr};

private:
  CTimerThread() : CThread("Timer") {}

  void Process() override
  {
    std::unique_lock lock(m_mutex);
    while (true)
    {
      const auto next = std::ranges::min_element(m_timers, {}, &CTimer::m_endTime);
      if (next == m_timers.end())
      {
        m_wakeUp.wait(lock);
        continue;
      }

      CTimer* timer = *next;
      const auto currentTime = std::chrono::steady_clock::now();
      if (currentTime < timer->m_endTime)
      {
        m_wakeUp.wait_until(lock, timer->m_endTime);
        continue;
      }

      m_current = timer;
      lock.unlock();
      try
      {
        timer->m_callback();
      }
      catch (...)
      {
        CLog::LogF(LOGERROR, "Timer callback threw an exception");
      }
      lock.lock();
      m_current = nullptr;
      m_callbackDone.notify_all();

      // Stopped during the callback
      if (std::ranges::find(m_timers, timer) == m_timers.end())
        continue;

      // continue if this is an interval timer, or if it was restarted during callback
      if (timer->m_interval || timer->m_endTime > currentTime)
        timer->m_endTime = std::chrono::steady_clock::now() + timer->m_timeout;
      else
        std::erase(m_timers, timer);
    }
  }
};

CTimer::CTimer(std::function<void()> const& callback)
  : m_callback(callback),
    m_timeout(0ms),
    m_interval(false)
{ }

CTimer::CTimer(ITimerCallback *callback)
  : CTimer(std::bind(&ITimerCallback::OnTimeout, callback))
{ }

CTimer::~CTimer()
{
  Stop(true);
}

bool CTimer::Start(std::chrono::milliseconds timeout, bool interval /* = false */)
{
  if (m_callback == NULL || timeout == 0ms)
    return false;

  CTimerThread& thread = CTimerThread::Get();
  std::unique_lock lock(thread.m_mutex);
  if (thread.IsActive(this))
    return false;

  m_timeout = timeout;
  m_interval = interval;
  m_endTime = std::chrono::steady_clock::now() + timeout;
  thread.Add(this);
  return true;
}

bool CTimer::Stop(bool wait /* = false */)
{
  CTimerThread& thread = CTimerThread::Get();
  std::unique_lock lock(thread.m_mutex);
  if (!thread.IsActive(this))
    return false;

  std::erase(thread.m_timers, this);
  if (wait && !thread.IsCurrentThread())
    thread.m_callbackDone.wait(lock, [&] { return thread.m_current != this; });

  return true;
}

void CTimer::RestartAsync(std::chrono::milliseconds timeout)
{
  CTimerThread& thread = CTimerThread::Get();
  std::unique_lock lock(thread.m_mutex);
  m_timeout = timeout;
  m_endTime = std::chrono::steady_clock::now() + timeout;
  thread.m_wakeUp.notify_one();
}

bool CTimer::Restart()
{
  if (!IsRunning())
    return false;

  Stop(true);

  return Start(m_timeout, m_interval);
}

bool CTimer::IsRunning() const
{
  CTimerThread& thread = CTimerThread::Get();
  std::unique_lock lock(thread.m_mutex);
  return thread.IsActive(this);
}

float CTimer::GetElapsedSeconds() const
{
  return GetElapsedMilliseconds() / 1000.0f;
}

float CTimer::GetElapsedMilliseconds() const
{
  CTimerThread& thread = CTimerThread::Get();
  std::unique_lock lock(thread.m_mutex);
  if (!thread.IsActive(this))
    return 0.0f;

  auto now = std::chrono::steady_clock::now();
  std::chrono::duration<float, std::milli> duration = (now - (m_endTime - m_timeout));

  return duration.count();
}
