/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/XTimeUtils.h"

#include "PosixTimezone.h"

#include <errno.h>
#include <time.h>

#include <sched.h>
#include <sys/times.h>
#include <unistd.h>

#if defined(TARGET_DARWIN)
#include "threads/Atomics.h"
#endif

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

void Sleep(uint32_t milliSeconds)
{
#if _POSIX_PRIORITY_SCHEDULING
  if (milliSeconds == 0)
  {
    sched_yield();
    return;
  }
#endif

  usleep(milliSeconds * 1000);
}

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
  // NOTE: localtime_r() is not required to set this, but we Assume that it's set here.
  g_timezone.m_IsDST = now.tm_isdst;
}

int FileTimeToLocalFileTime(const FILETIME* lpFileTime, LPFILETIME lpLocalFileTime)
{
  ULARGE_INTEGER l;
  l.u.LowPart = lpFileTime->dwLowDateTime;
  l.u.HighPart = lpFileTime->dwHighDateTime;

  time_t ft;
  struct tm tm_ft;
  FileTimeToTimeT(lpFileTime, &ft);
  localtime_r(&ft, &tm_ft);

  l.QuadPart += static_cast<unsigned long long>(tm_ft.tm_gmtoff) * 10000000;

  lpLocalFileTime->dwLowDateTime = l.u.LowPart;
  lpLocalFileTime->dwHighDateTime = l.u.HighPart;
  return 1;
}


int SystemTimeToFileTime(const SystemTime* systemTime, LPFILETIME lpFileTime)
{
  static const int dayoffset[12] = {0, 31, 59, 90, 120, 151, 182, 212, 243, 273, 304, 334};
#if defined(TARGET_DARWIN)
  static std::atomic_flag timegm_lock = ATOMIC_FLAG_INIT;
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
  sysTime.tm_isdst = g_timezone.m_IsDST;

  // If this is a leap year, and we're past the 28th of Feb, increment tm_yday.
  if (IsLeapYear(systemTime->year) && (sysTime.tm_yday > 58))
    sysTime.tm_yday++;

#if defined(TARGET_DARWIN)
  CAtomicSpinLock lock(timegm_lock);
#endif

#if defined(TARGET_ANDROID) && !defined(__LP64__)
  time64_t t = timegm64(&sysTime);
#else
  time_t t = timegm(&sysTime);
#endif

  LARGE_INTEGER result;
  result.QuadPart = (long long)t * 10000000 + (long long)systemTime->milliseconds * 10000;
  result.QuadPart += WIN32_TIME_OFFSET;

  lpFileTime->dwLowDateTime = result.u.LowPart;
  lpFileTime->dwHighDateTime = result.u.HighPart;

  return 1;
}

long CompareFileTime(const FILETIME* lpFileTime1, const FILETIME* lpFileTime2)
{
  ULARGE_INTEGER t1;
  t1.u.LowPart = lpFileTime1->dwLowDateTime;
  t1.u.HighPart = lpFileTime1->dwHighDateTime;

  ULARGE_INTEGER t2;
  t2.u.LowPart = lpFileTime2->dwLowDateTime;
  t2.u.HighPart = lpFileTime2->dwHighDateTime;

  if (t1.QuadPart == t2.QuadPart)
     return 0;
  else if (t1.QuadPart < t2.QuadPart)
     return -1;
  else
     return 1;
}

int FileTimeToSystemTime(const FILETIME* lpFileTime, SystemTime* systemTime)
{
  LARGE_INTEGER fileTime;
  fileTime.u.LowPart = lpFileTime->dwLowDateTime;
  fileTime.u.HighPart = lpFileTime->dwHighDateTime;

  fileTime.QuadPart -= WIN32_TIME_OFFSET;
  fileTime.QuadPart /= 10000; /* to milliseconds */
  systemTime->milliseconds = fileTime.QuadPart % 1000;
  fileTime.QuadPart /= 1000; /* to seconds */

  time_t ft = fileTime.QuadPart;

  struct tm tm_ft;
  gmtime_r(&ft,&tm_ft);

  systemTime->year = tm_ft.tm_year + 1900;
  systemTime->month = tm_ft.tm_mon + 1;
  systemTime->dayOfWeek = tm_ft.tm_wday;
  systemTime->day = tm_ft.tm_mday;
  systemTime->hour = tm_ft.tm_hour;
  systemTime->minute = tm_ft.tm_min;
  systemTime->second = tm_ft.tm_sec;

  return 1;
}

int LocalFileTimeToFileTime( const FILETIME* lpLocalFileTime, LPFILETIME lpFileTime)
{
  ULARGE_INTEGER l;
  l.u.LowPart = lpLocalFileTime->dwLowDateTime;
  l.u.HighPart = lpLocalFileTime->dwHighDateTime;

  l.QuadPart += (unsigned long long) timezone * 10000000;

  lpFileTime->dwLowDateTime = l.u.LowPart;
  lpFileTime->dwHighDateTime = l.u.HighPart;

  return 1;
}

int FileTimeToTimeT(const FILETIME* lpLocalFileTime, time_t *pTimeT) {

  if (lpLocalFileTime == NULL || pTimeT == NULL)
  return false;

  ULARGE_INTEGER fileTime;
  fileTime.u.LowPart  = lpLocalFileTime->dwLowDateTime;
  fileTime.u.HighPart = lpLocalFileTime->dwHighDateTime;

  fileTime.QuadPart -= WIN32_TIME_OFFSET;
  fileTime.QuadPart /= 10000; /* to milliseconds */
  fileTime.QuadPart /= 1000; /* to seconds */

  time_t ft = fileTime.QuadPart;

  struct tm tm_ft;
  localtime_r(&ft,&tm_ft);

  *pTimeT = mktime(&tm_ft);
  return 1;
}

int TimeTToFileTime(time_t timeT, FILETIME* lpLocalFileTime) {

  if (lpLocalFileTime == NULL)
  return false;

  ULARGE_INTEGER result;
  result.QuadPart = (unsigned long long) timeT * 10000000;
  result.QuadPart += WIN32_TIME_OFFSET;

  lpLocalFileTime->dwLowDateTime  = result.u.LowPart;
  lpLocalFileTime->dwHighDateTime = result.u.HighPart;

  return 1;
}

} // namespace TIME
} // namespace KODI
