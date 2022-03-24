/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
  explicit EndTime(const T duration)
    : m_startTime(std::chrono::steady_clock::now()), m_totalWaitTime(duration)
  {
  }

  EndTime() = default;
  EndTime(const EndTime& right) = delete;
  ~EndTime() = default;

  void Set(const T duration)
  {
    m_startTime = std::chrono::steady_clock::now();
    m_totalWaitTime = duration;
  }

  bool IsTimePast() const
  {
    if (m_totalWaitTime == m_infinity)
      return false;

    const auto now = std::chrono::steady_clock::now();

    return ((now - m_startTime) >= m_totalWaitTime);
  }

  T GetTimeLeft() const
  {
    if (m_totalWaitTime == m_infinity)
      return m_infinity;

    const auto now = std::chrono::steady_clock::now();

    const auto left = ((m_startTime + m_totalWaitTime) - now);

    if (left < T::zero())
      return T::zero();

    return std::chrono::duration_cast<T>(left);
  }

  void SetExpired() { m_totalWaitTime = T::zero(); }

  void SetInfinite() { m_totalWaitTime = m_infinity; }

  bool IsInfinite() const { return (m_totalWaitTime == m_infinity); }

  T GetInitialTimeoutValue() const { return m_totalWaitTime; }

  std::chrono::steady_clock::time_point GetStartTime() const { return m_startTime; }

private:
  std::chrono::steady_clock::time_point m_startTime;
  T m_totalWaitTime = T::zero();

  const T m_infinity = T::max();
};

} // namespace XbmcThreads
