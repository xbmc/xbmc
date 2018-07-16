/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <limits>

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
    unsigned int startTime = 0;
    unsigned int totalWaitTime = 0;
  public:
    static const unsigned int InfiniteValue;
    inline EndTime() = default;
    inline explicit EndTime(unsigned int millisecondsIntoTheFuture) : startTime(SystemClockMillis()), totalWaitTime(millisecondsIntoTheFuture) {}

    inline void Set(unsigned int millisecondsIntoTheFuture) { startTime = SystemClockMillis(); totalWaitTime = millisecondsIntoTheFuture; }

    inline bool IsTimePast() const { return totalWaitTime == InfiniteValue ? false : (totalWaitTime == 0 ? true : (SystemClockMillis() - startTime) >= totalWaitTime); }

    inline unsigned int MillisLeft() const
    {
      if (totalWaitTime == InfiniteValue)
        return InfiniteValue;
      if (totalWaitTime == 0)
        return 0;
      unsigned int timeWaitedAlready = (SystemClockMillis() - startTime);
      return (timeWaitedAlready >= totalWaitTime) ? 0 : (totalWaitTime - timeWaitedAlready);
    }

    inline void SetExpired() { totalWaitTime = 0; }
    inline void SetInfinite() { totalWaitTime = InfiniteValue; }
    inline bool IsInfinite(void) const { return (totalWaitTime == InfiniteValue); }
    inline unsigned int GetInitialTimeoutValue(void) const { return totalWaitTime; }
    inline unsigned int GetStartTime(void) const { return startTime; }
  };
}
