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

// Types wrapping either std::chrono standardized types since C++20
// or types from https://github.com/HowardHinnant/date

#if !defined(DATE_IS_CXX20) // use std::chrono C++20

#define USE_OS_TZDB 1

#if defined(DATE_HAS_STRINGVIEW)
#define HAS_STRING_VIEW 1
#else // !defined(DATE_HAS_STRINGVIEW)
#define HAS_STRING_VIEW 0
#endif // defined(DATE_HAS_STRINGVIEW)

#define HAS_REMOTE_API 0
#include <date/date.h>
#include <date/iso_week.h>
#include <date/tz.h>

#endif // !defined(DATE_IS_CXX20)

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

// Duration and timepoint

using Duration = std::chrono::duration<std::int64_t, std::ratio<1, 10'000'000>>;
using TimePoint = std::chrono::time_point<std::chrono::system_clock, Duration>;

// Types wrapping either std::chrono standardized types since C++20
// or types from https://github.com/HowardHinnant/date

#if defined(DATE_IS_CXX20) // use std::chrono C++20

#include <format>

// clang-format off

using days           = std::chrono::days;
using local_days     = std::chrono::local_days;
using month          = std::chrono::month;
using sys_days       = std::chrono::sys_days;
using sys_seconds    = std::chrono::sys_seconds;
using year           = std::chrono::year;
using year_month_day = std::chrono::year_month_day;
using weekday        = std::chrono::weekday;

using std::chrono::current_zone;
using std::chrono::floor;
using std::format;

// clang-format on

#if defined(_MSC_VER) && (!defined(__clang__) || (_MSC_VER < 1910))
// MSVC
#if _MSC_VER < 1910
//   before VS2017
#define CONSTCD11
#else
//   VS2017 and later
#define CONSTCD11 constexpr
#endif

#elif defined(__SUNPRO_CC) && __SUNPRO_CC <= 0x5150
// Oracle Developer Studio 12.6 and earlier
#define CONSTCD11 constexpr

#elif __cplusplus >= 201402
// C++14
#define CONSTCD11 constexpr
#else
// C++11
#define CONSTCD11 constexpr
#endif

template<class Rep, class Period>
CONSTCD11 inline std::chrono::hh_mm_ss<std::chrono::duration<Rep, Period>> make_time(
    const std::chrono::duration<Rep, Period>& d)
{
  return std::chrono::hh_mm_ss<std::chrono::duration<Rep, Period>>(d);
}

template<class Duration,
         class TimeZonePtr
#if !defined(_MSC_VER) || (_MSC_VER > 1916)
#if !defined(__INTEL_COMPILER) || (__INTEL_COMPILER > 1600)
         ,
         class = typename std::enable_if<std::is_class<
             typename std::decay<decltype(*std::declval<TimeZonePtr&>())>::type>{}>::type
#endif
#endif
         >
inline std::chrono::zoned_time<typename std::common_type<Duration, std::chrono::seconds>::type,
                               TimeZonePtr>
make_zoned(TimeZonePtr zone, const std::chrono::sys_time<Duration>& st)
{
  return std::chrono::zoned_time<typename std::common_type<Duration, std::chrono::seconds>::type,
                                 TimeZonePtr>(std::move(zone), st);
}

template<class Duration>
inline std::chrono::zoned_time<typename std::common_type<Duration, std::chrono::seconds>::type>
make_zoned(const std::string& name, const std::chrono::sys_time<Duration>& st)
{
  return std::chrono::zoned_time<typename std::common_type<Duration, std::chrono::seconds>::type>(
      name, st);
}

#else // !defined(DATE_IS_CXX20)

// clang-format off

using days           = ::date::days;
using local_days     = ::date::local_days;
using month          = ::date::month;
using sys_days       = ::date::sys_days;
using sys_seconds    = ::date::sys_seconds;
using year           = ::date::year;
using year_month_day = ::date::year_month_day;
using weekday        = ::date::weekday;

using ::date::current_zone;
using ::date::floor;
using ::date::format;
using ::date::make_time;
using ::date::make_zoned;

// clang-format on

#endif // defined(DATE_IS_CXX20)

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
