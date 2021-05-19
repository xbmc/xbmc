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
  /**
   * This function returns the system clock's number of milliseconds but with
   *  an arbitrary reference point. It handles the wrapping of any underlying
   *  system clock by setting a starting point at the first call. It should
   *  only be used for measuring time durations.
   *
   * Of course, on windows it just calls timeGetTime, so you're on your own.
   */

  unsigned int SystemClockMillis();

  /**
   * DO NOT compare the results from SystemClockMillis() to an expected end time
   *  that was calculated by adding a number of milliseconds to some start time.
   *  The reason is because the SystemClockMillis could wrap. Instead use this
   *  class which uses differences (which are safe across a wrap).
   */
  class EndTime
  {
  public:
    EndTime();
    explicit EndTime(unsigned int millisecondsIntoTheFuture);
    EndTime(const EndTime& right) = delete;
    ~EndTime() = default;

    void Set(unsigned int millisecondsIntoTheFuture);

    bool IsTimePast() const;

    unsigned int MillisLeft() const;

    void SetExpired() { m_totalWaitTime = std::chrono::milliseconds(0); }
    void SetInfinite() { m_totalWaitTime = InfiniteValue; }
    bool IsInfinite() const { return (m_totalWaitTime == InfiniteValue); }

    unsigned int GetInitialTimeoutValue() const;
    unsigned int GetStartTime() const;

  private:
    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::milliseconds m_totalWaitTime;

    const std::chrono::milliseconds InfiniteValue =
        std::chrono::milliseconds(std::numeric_limits<std::chrono::milliseconds::rep>::max());
  };
}
