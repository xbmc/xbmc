#pragma once
/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  Copyright (C) 2011-2012 Pulse-Eight Limited.
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#if defined(TARGET_DARWIN)
#include <mach/mach_time.h>
#include <CoreVideo/CVHostTime.h>
#elif defined(TARGET_WINDOWS)
#include <Windows.h>
#include <time.h>
#else
#include <time.h>
#endif

namespace kodi
{
namespace time
{

//===============================================================================
/// @brief Function to get current time in milliseconds
///
/// @return Current time in milliseconds as a double value
///
///
/// -----------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
///
/// #include <kodi/tools/Time.h>
///
/// ...
/// double time = kodi::time::GetTimeMs();
/// ...
/// ~~~~~~~~~~~~~
///
inline double GetTimeMs()
{
#if defined(TARGET_DARWIN)
  return static_cast<double>(CVGetCurrentHostTime() / static_cast<double>(CVGetHostClockFrequency() * 0.001));
#elif defined(TARGET_WINDOWS)
  LARGE_INTEGER tickPerSecond;
  LARGE_INTEGER tick;
  if (QueryPerformanceFrequency(&tickPerSecond))
  {
    QueryPerformanceCounter(&tick);
    return static_cast<double>(tick.QuadPart) / (tickPerSecond.QuadPart / 1000.0);
  }
  return 0.0;
#else
  timespec time;
  clock_gettime(CLOCK_MONOTONIC, &time);
  return static_cast<double>(time.tv_sec) * 1000.0 + time.tv_nsec / 1000000.0;
#endif
}
//-------------------------------------------------------------------------------

//===============================================================================
/// @brief Function to get current time in seconds
///
/// @return Current time in seconds with the value type defined in the template
///
///
/// -----------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
///
/// #include <kodi/tools/Time.h>
///
/// ...
/// double time = kodi::time::GetTimeSec<double>();
/// ...
/// ~~~~~~~~~~~~~
///
template <class T>
inline T GetTimeSec()
{
  return static_cast<T>(GetTimeMs()) / static_cast<T>(1000.0);
}
//-------------------------------------------------------------------------------

} /* namespace time */
} /* namespace kodi */
