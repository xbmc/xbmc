/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/log.h"

#include <chrono>
#include <limits>
#include <thread>

namespace XbmcThreads
{

template<typename>
struct is_chrono_duration : std::false_type
{
};

template<typename Rep, typename Period>
struct is_chrono_duration<std::chrono::duration<Rep, Period>> : std::true_type
{
};

template<typename T = std::chrono::milliseconds, bool = is_chrono_duration<T>::value>
class EndTime;

template<typename T>
class EndTime<T, true>
{
public:
  explicit EndTime(const T duration) { Set(duration); }

  EndTime() = default;
  EndTime(const EndTime& right) = delete;
  ~EndTime() = default;

  static constexpr T Max() { return m_max; }

  void Set(const T duration)
  {
    m_startTime = std::chrono::steady_clock::now();

    if (duration > m_max)
    {
      m_totalWaitTime = m_max;
      CLog::Log(LOGWARNING, "duration ({}) greater than max ({}) - duration will be truncated!",
                duration.count(), m_max.count());
    }
    else
    {
      m_totalWaitTime = duration;
    }
  }

  bool IsTimePast() const
  {
    const auto now = std::chrono::steady_clock::now();

    return ((now - m_startTime) >= m_totalWaitTime);
  }

  T GetTimeLeft() const
  {
    const auto now = std::chrono::steady_clock::now();

    const auto left = ((m_startTime + m_totalWaitTime) - now);

    if (left < T::zero())
      return T::zero();

    return std::chrono::duration_cast<T>(left);
  }

  void SetExpired() { m_totalWaitTime = T::zero(); }

  void SetInfinite() { m_totalWaitTime = m_max; }

  T GetInitialTimeoutValue() const { return m_totalWaitTime; }

  std::chrono::steady_clock::time_point GetStartTime() const { return m_startTime; }

private:
  std::chrono::steady_clock::time_point m_startTime;
  T m_totalWaitTime = T::zero();

  static constexpr T m_max =
      std::chrono::duration_cast<T>(std::chrono::steady_clock::duration::max());
};

} // namespace XbmcThreads
