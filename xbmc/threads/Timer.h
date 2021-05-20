/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Event.h"
#include "Thread.h"

#include <chrono>
#include <functional>

class ITimerCallback
{
public:
  virtual ~ITimerCallback() = default;

  virtual void OnTimeout() = 0;
};

class CTimer : protected CThread
{
public:
  explicit CTimer(ITimerCallback *callback);
  explicit CTimer(std::function<void()> const& callback);
  ~CTimer() override;

  bool Start(std::chrono::milliseconds timeout, bool interval = false);
  bool Stop(bool wait = false);
  bool Restart();
  void RestartAsync(std::chrono::milliseconds timeout);

  bool IsRunning() const { return CThread::IsRunning(); }

  float GetElapsedSeconds() const;
  float GetElapsedMilliseconds() const;

protected:
  void Process() override;

private:
  std::function<void()> m_callback;
  std::chrono::milliseconds m_timeout;
  bool m_interval;
  std::chrono::time_point<std::chrono::steady_clock> m_endTime;
  CEvent m_eventTimeout;
};
