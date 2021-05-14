/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <chrono>
#include <string>
#include <thread>

#if !defined(TARGET_WINDOWS)
#include "PlatformDefs.h"
#else
// This is needed, a forward declaration of FILETIME
// breaks everything
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

namespace KODI
{
namespace TIME
{

// For backward compatibility with CArchive

struct SystemTime
{
  unsigned short year;
  unsigned short month;
  unsigned short dayOfWeek;
  unsigned short day;
  unsigned short hour;
  unsigned short minute;
  unsigned short second;
  unsigned short milliseconds;
};

using Duration = std::chrono::duration<std::int64_t, std::ratio<1, 10'000'000>>;
using TimePoint = std::chrono::time_point<std::chrono::system_clock, Duration>;

template<typename Rep, typename Period>
void Sleep(std::chrono::duration<Rep, Period> duration)
{
  if (duration == std::chrono::duration<Rep, Period>::zero())
  {
    std::this_thread::yield();
    return;
  }

  std::this_thread::sleep_for(duration);
}

} // namespace TIME
} // namespace KODI
