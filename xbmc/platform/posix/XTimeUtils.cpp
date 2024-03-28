/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/XTimeUtils.h"

#include <errno.h>
#include <mutex>
#include <time.h>

#include <sys/times.h>

#if defined(TARGET_ANDROID) && !defined(__LP64__)
#include <time64.h>
#endif

#define WIN32_TIME_OFFSET ((unsigned long long)(369 * 365 + 89) * 24 * 3600 * 10000000)

namespace KODI
{
namespace TIME
{

/*
 * A Leap year is any year that is divisible by four, but not by 100 unless also
 * divisible by 400
 */
#define IsLeapYear(y) ((!(y % 4)) ? (((!(y % 400)) && (y % 100)) ? 1 : 0) : 0)

uint32_t GetTimeZoneInformation(TimeZoneInformation* timeZoneInformation)
{
  if (!timeZoneInformation)
    return KODI_TIME_ZONE_ID_INVALID;

  struct tm t;
  time_t tt = time(NULL);
  if (localtime_r(&tt, &t))
    timeZoneInformation->bias = -t.tm_gmtoff / 60;

  timeZoneInformation->standardName = tzname[0];
  timeZoneInformation->daylightName = tzname[1];

  return KODI_TIME_ZONE_ID_UNKNOWN;
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
  static const int dayoffset[12] = {0, 31, 59, 90, 120, 151, 182, 212, 243, 273, 304, 334};
#if defined(TARGET_DARWIN)
  static std::mutex timegm_lock;
#endif

  struct tm sysTime = {};
  sysTime.tm_year = systemTime->year - 1900;
  sysTime.tm_mon = systemTime->month - 1;
  sysTime.tm_wday = systemTime->dayOfWeek;
  sysTime.tm_mday = systemTime->day;
  sysTime.tm_hour = systemTime->hour;
  sysTime.tm_min = systemTime->minute;
  sysTime.tm_sec = systemTime->second;
  sysTime.tm_yday = dayoffset[sysTime.tm_mon] + (sysTime.tm_mday - 1);

  // If this is a leap year, and we're past the 28th of Feb, increment tm_yday.
  if (IsLeapYear(systemTime->year) && (sysTime.tm_yday > 58))
    sysTime.tm_yday++;

#if defined(TARGET_DARWIN)
  std::lock_guard<std::mutex> lock(timegm_lock);
#endif

#if defined(TARGET_ANDROID) && !defined(__LP64__)
  time64_t t = timegm64(&sysTime);
#else
  time_t t = timegm(&sysTime);
#endif

  LARGE_INTEGER result;
  result.QuadPart = (long long)t * 10000000 + (long long)systemTime->milliseconds * 10000;
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
  file.QuadPart /= 10000; /* to milliseconds */
  systemTime->milliseconds = file.QuadPart % 1000;
  file.QuadPart /= 1000; /* to seconds */

  time_t ft = file.QuadPart;

  struct tm tm_ft;
  gmtime_r(&ft, &tm_ft);

  systemTime->year = tm_ft.tm_year + 1900;
  systemTime->month = tm_ft.tm_mon + 1;
  systemTime->dayOfWeek = tm_ft.tm_wday;
  systemTime->day = tm_ft.tm_mday;
  systemTime->hour = tm_ft.tm_hour;
  systemTime->minute = tm_ft.tm_min;
  systemTime->second = tm_ft.tm_sec;

  return 1;
}

int LocalFileTimeToFileTime(const FileTime* localFileTime, FileTime* fileTime)
{
  ULARGE_INTEGER l;
  l.u.LowPart = localFileTime->lowDateTime;
  l.u.HighPart = localFileTime->highDateTime;

  l.QuadPart += (unsigned long long) timezone * 10000000;

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
