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

  bool Start(uint32_t timeout, bool interval = false);
  bool Stop(bool wait = false);
  bool Restart();
  void RestartAsync(uint32_t timeout);

  bool IsRunning() const { return CThread::IsRunning(); }

  float GetElapsedSeconds() const;
  float GetElapsedMilliseconds() const;

protected:
  void Process() override;

private:
  std::function<void()> m_callback;
  uint32_t m_timeout;
  bool m_interval;
  uint32_t m_endTime;
  CEvent m_eventTimeout;
};
