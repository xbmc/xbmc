/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/SystemClock.h"

#include <chrono>
#include <thread>

namespace ConditionPoll
{
/**
 * This is usually enough time for the condition to have occurred in a test.
 */
constexpr unsigned int defaultTimeout{20000};

/**
 * poll until the lambda returns true or the timeout occurs. If the timeout occurs then
 * the function will return false. Otherwise it will return true.
 */
template<typename L> inline bool poll(unsigned int timeoutMillis, L lambda)
{
  XbmcThreads::EndTime<> endTime{std::chrono::milliseconds(timeoutMillis)};
  bool lastValue = false;
  while (!endTime.IsTimePast() && (lastValue = lambda()) == false)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  return lastValue;
}

/**
 * poll until the lambda returns true or the defaultTimeout occurs. If the timeout occurs then
 * the function will return false. Otherwise it will return true.
 */
template<typename L> inline bool poll(L lambda)
{
  return poll(defaultTimeout, lambda);
}

}
