/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SystemClock.h"

#include "utils/TimeUtils.h"

#include <stdint.h>

namespace XbmcThreads
{

EndTime::EndTime() : m_startTime(std::chrono::system_clock::now())
{
}

EndTime::EndTime(unsigned int millisecondsIntoTheFuture)
  : m_startTime(std::chrono::system_clock::now()),
    m_totalWaitTime(static_cast<std::chrono::milliseconds>(millisecondsIntoTheFuture))
{
}

void EndTime::Set(unsigned int millisecondsIntoTheFuture)
{
  m_startTime = std::chrono::system_clock::now();
  m_totalWaitTime = static_cast<std::chrono::milliseconds>(millisecondsIntoTheFuture);
}

bool EndTime::IsTimePast() const
{
  if (m_totalWaitTime == InfiniteValue)
    return false;

  if (m_totalWaitTime == std::chrono::milliseconds(0))
    return true;

  return ((std::chrono::system_clock::now() - m_startTime) >= m_totalWaitTime);
}

unsigned int EndTime::MillisLeft() const
{
  if (m_totalWaitTime == InfiniteValue)
    return static_cast<unsigned int>(InfiniteValue.count());

  if (m_totalWaitTime == std::chrono::milliseconds(0))
    return 0;

  std::chrono::milliseconds timeWaitedAlready =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() -
                                                            m_startTime);

  if (timeWaitedAlready >= m_totalWaitTime)
    return 0;

  return static_cast<unsigned int>(
      std::chrono::duration_cast<std::chrono::milliseconds>(m_totalWaitTime - timeWaitedAlready)
          .count());
}

unsigned int EndTime::GetInitialTimeoutValue() const
{
  return static_cast<unsigned int>(m_totalWaitTime.count());
}

unsigned int EndTime::GetStartTime() const
{
  return static_cast<unsigned int>(
      std::chrono::duration_cast<std::chrono::milliseconds>(m_startTime.time_since_epoch())
          .count());
}

unsigned int SystemClockMillis()
{
  return static_cast<unsigned int>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count());
}

const std::chrono::milliseconds EndTime::InfiniteValue =
    std::numeric_limits<std::chrono::milliseconds>::max();
}
