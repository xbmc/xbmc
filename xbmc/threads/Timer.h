/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <chrono>
#include <functional>

class ITimerCallback
{
public:
  virtual ~ITimerCallback() = default;

  virtual void OnTimeout() = 0;
};

/*!
 * All timers share one thread, which runs their callbacks one at a time. A callback
 * therefore delays every other timer until it returns, and must not block, for example
 * wait on the main thread.
 */
class CTimer
{
public:
  explicit CTimer(ITimerCallback *callback);
  explicit CTimer(std::function<void()> const& callback);
  CTimer(const CTimer&) = delete;
  CTimer& operator=(const CTimer&) = delete;
  virtual ~CTimer();

  bool Start(std::chrono::milliseconds timeout, bool interval = false);
  bool Stop(bool wait = false);
  bool Restart();
  void RestartAsync(std::chrono::milliseconds timeout);

  bool IsRunning() const;

  float GetElapsedSeconds() const;
  float GetElapsedMilliseconds() const;

private:
  class CTimerThread;

  std::function<void()> m_callback;
  std::chrono::milliseconds m_timeout;
  bool m_interval;
  std::chrono::time_point<std::chrono::steady_clock> m_endTime;
};
