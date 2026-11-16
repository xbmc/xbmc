/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/XTimeUtils.h"

#include <errno.h>
#include <optional>
#include <time.h>

#include <sys/times.h>

#define WIN32_TIME_OFFSET ((unsigned long long)(369 * 365 + 89) * 24 * 3600 * 10000000)

namespace
{
// Helper: days_from_civil from Howard Hinnant (public domain).
// Returns days since civil 1970-01-01 (1970-01-01 -> 0).
int64_t days_from_civil(int64_t y, unsigned m, unsigned d)
{
  // y: full year (e.g., 2025), m: 1..12, d: 1..31
  if (m <= 2)
    y -= 1;
  const int64_t era{(y >= 0 ? y : y - 399) / 400};
  const unsigned yoe{static_cast<unsigned>(y - era * 400)}; // [0, 399]
  const unsigned doy{(153u * (m + (m > 2 ? -3u : 9u)) + 2u) / 5u + d - 1u}; // [0, 365]
  const unsigned doe{yoe * 365u + yoe / 4u - yoe / 100u + doy}; // [0, 146096]
  return era * 146097LL + static_cast<int64_t>(doe) - 719468LL;
}

// Helper: civil_from_days from Howard Hinnant (public domain), the inverse of days_from_civil.
void civil_from_days(int64_t z, int64_t& y, unsigned& m, unsigned& d)
{
  z += 719468;
  const int64_t era{(z >= 0 ? z : z - 146096) / 146097};
  const unsigned doe{static_cast<unsigned>(z - era * 146097)}; // [0, 146096]
  const unsigned yoe{(doe - doe / 1460u + doe / 36524u - doe / 146096u) / 365u}; // [0, 399]
  const unsigned doy{doe - (365u * yoe + yoe / 4u - yoe / 100u)}; // [0, 365]
  const unsigned mp{(5u * doy + 2u) / 153u}; // [0, 11]
  d = doy - (153u * mp + 2u) / 5u + 1u;
  m = mp < 10u ? mp + 3u : mp - 9u;
  y = static_cast<int64_t>(yoe) + era * 400 + (m <= 2u ? 1 : 0);
}

int64_t FloorDiv(int64_t a, int64_t b)
{
  return (a >= 0 ? a : a - (b - 1)) / b;
}

std::optional<time_t> SystemTimeToTimeT(const KODI::TIME::SystemTime& systemTime)
{
  // Basic range validation
  if (systemTime.month < 1 || systemTime.month > 12 || systemTime.day < 1 || systemTime.day > 31 ||
      systemTime.hour > 23u || systemTime.minute > 59u ||
      systemTime.second > 60u /* allow 60 for leap second */)
    return {}; // error

  // Compute days since epoch
  const int64_t days{
      days_from_civil(static_cast<int64_t>(systemTime.year), systemTime.month, systemTime.day)};

  // Compute seconds since epoch
  const int64_t seconds{days * 86400LL + static_cast<int64_t>(systemTime.hour) * 3600LL +
                        static_cast<int64_t>(systemTime.minute) * 60LL +
                        static_cast<int64_t>(systemTime.second)};

  // Validate range for time_t
  if (seconds < std::numeric_limits<time_t>::min() || seconds > std::numeric_limits<time_t>::max())
    return {}; // error

  return seconds;
}
} // unnamed namespace

namespace KODI
{
namespace TIME
{
std::tuple<bool, int64_t> GetTimezoneBias(const SystemTime& time)
{
  // Times before 1970 are negative, not errors
  const std::optional<time_t> t{SystemTimeToTimeT(time)};
  if (!t)
    return {false, 0}; // error

  struct tm tms;
  if (!localtime_r(&*t, &tms))
    return {false, 0}; // error

  return {true, -tms.tm_gmtoff / 60};
}

void GetLocalTime(SystemTime* systemTime)
{
  const time_t t = time(NULL);
  struct tm now;

  localtime_r(&t, &now);
  systemTime->year = now.tm_year + 1900;
  systemTime->month = now.tm_mon + 1;
  systemTime->dayOfWeek = now.tm_wday;
  systemTime->day = now.tm_mday;
  systemTime->hour = now.tm_hour;
  systemTime->minute = now.tm_min;
  systemTime->second = now.tm_sec;
  systemTime->milliseconds = 0;
}

int FileTimeToLocalFileTime(const FileTime* fileTime, FileTime* localFileTime)
{
  ULARGE_INTEGER l;
  l.u.LowPart = fileTime->lowDateTime;
  l.u.HighPart = fileTime->highDateTime;

  time_t ft;
  struct tm tm_ft;
  FileTimeToTimeT(fileTime, &ft);
  localtime_r(&ft, &tm_ft);

  l.QuadPart += static_cast<unsigned long long>(tm_ft.tm_gmtoff) * 10000000;

  localFileTime->lowDateTime = l.u.LowPart;
  localFileTime->highDateTime = l.u.HighPart;
  return 1;
}

int SystemTimeToFileTime(const SystemTime* systemTime, FileTime* fileTime)
{
  // Computed here rather than through timegm(), which a 32-bit time_t limits to 1901 - 2038.
  // An out-of-range field carries into the next one up, as it does in timegm().
  const int64_t months{systemTime->month - 1};
  const int64_t years{FloorDiv(months, 12)};
  const int64_t days{
      days_from_civil(systemTime->year + years, static_cast<unsigned>(months - years * 12 + 1), 1) +
      systemTime->day - 1};
  const int64_t seconds{days * 86400 + systemTime->hour * 3600 + systemTime->minute * 60 +
                        systemTime->second};

  LARGE_INTEGER result;
  result.QuadPart = seconds * 10000000 + (long long)systemTime->milliseconds * 10000;
  result.QuadPart += WIN32_TIME_OFFSET;

  fileTime->lowDateTime = result.u.LowPart;
  fileTime->highDateTime = result.u.HighPart;

  return 1;
}

long CompareFileTime(const FileTime* fileTime1, const FileTime* fileTime2)
{
  ULARGE_INTEGER t1;
  t1.u.LowPart = fileTime1->lowDateTime;
  t1.u.HighPart = fileTime1->highDateTime;

  ULARGE_INTEGER t2;
  t2.u.LowPart = fileTime2->lowDateTime;
  t2.u.HighPart = fileTime2->highDateTime;

  if (t1.QuadPart == t2.QuadPart)
     return 0;
  else if (t1.QuadPart < t2.QuadPart)
     return -1;
  else
     return 1;
}

int FileTimeToSystemTime(const FileTime* fileTime, SystemTime* systemTime)
{
  LARGE_INTEGER file;
  file.u.LowPart = fileTime->lowDateTime;
  file.u.HighPart = fileTime->highDateTime;

  file.QuadPart -= WIN32_TIME_OFFSET;

  // Computed here rather than through gmtime_r(), which a 32-bit time_t limits to 1901 - 2038
  const int64_t milliseconds{FloorDiv(file.QuadPart, 10000)};
  const int64_t seconds{FloorDiv(milliseconds, 1000)};
  const int64_t days{FloorDiv(seconds, 86400)};
  const int64_t secondOfDay{seconds - days * 86400};

  int64_t year{};
  unsigned month{};
  unsigned day{};
  civil_from_days(days, year, month, day);

  systemTime->year = year;
  systemTime->month = month;
  systemTime->dayOfWeek = days - FloorDiv(days + 4, 7) * 7 + 4; // 1970-01-01 was a Thursday
  systemTime->day = day;
  systemTime->hour = secondOfDay / 3600;
  systemTime->minute = secondOfDay / 60 % 60;
  systemTime->second = secondOfDay % 60;
  systemTime->milliseconds = milliseconds - seconds * 1000;

  return 1;
}

int LocalFileTimeToFileTime(const FileTime* localFileTime, FileTime* fileTime)
{
  ULARGE_INTEGER l;
  l.u.LowPart = localFileTime->lowDateTime;
  l.u.HighPart = localFileTime->highDateTime;

  time_t ft;
  struct tm tm_ft;
  FileTimeToTimeT(localFileTime, &ft);
  localtime_r(&ft, &tm_ft);

  l.QuadPart -= static_cast<unsigned long long>(tm_ft.tm_gmtoff) * 10000000;

  fileTime->lowDateTime = l.u.LowPart;
  fileTime->highDateTime = l.u.HighPart;

  return 1;
}


int FileTimeToTimeT(const FileTime* localFileTime, time_t* pTimeT)
{
  if (!localFileTime || !pTimeT)
    return false;

  ULARGE_INTEGER fileTime;
  fileTime.u.LowPart = localFileTime->lowDateTime;
  fileTime.u.HighPart = localFileTime->highDateTime;

  fileTime.QuadPart -= WIN32_TIME_OFFSET;
  fileTime.QuadPart /= 10000; /* to milliseconds */
  fileTime.QuadPart /= 1000; /* to seconds */

  time_t ft = fileTime.QuadPart;

  struct tm tm_ft;
  localtime_r(&ft, &tm_ft);

  *pTimeT = mktime(&tm_ft);
  return 1;
}

int TimeTToFileTime(time_t timeT, FileTime* localFileTime)
{
  if (!localFileTime)
    return false;

  ULARGE_INTEGER result;
  result.QuadPart = (unsigned long long) timeT * 10000000;
  result.QuadPart += WIN32_TIME_OFFSET;

  localFileTime->lowDateTime = result.u.LowPart;
  localFileTime->highDateTime = result.u.HighPart;

  return 1;
}

} // namespace TIME
} // namespace KODI
