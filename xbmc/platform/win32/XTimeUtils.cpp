/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/XTimeUtils.h"

#include "platform/win32/CharsetConverter.h"

#include <FileAPI.h>

using KODI::PLATFORM::WINDOWS::FromW;

namespace KODI
{
namespace TIME
{
namespace
{
void SystemTimeToKodiTime(const SYSTEMTIME& sst, SystemTime& st)
{
  st.day = sst.wDay;
  st.dayOfWeek = sst.wDayOfWeek;
  st.hour = sst.wHour;
  st.milliseconds = sst.wMilliseconds;
  st.minute = sst.wMinute;
  st.month = sst.wMonth;
  st.second = sst.wSecond;
  st.year = sst.wYear;
}

void KodiTimeToSystemTime(const SystemTime& st, SYSTEMTIME& sst)
{
  sst.wDay = st.day;
  sst.wDayOfWeek = st.dayOfWeek;
  sst.wHour = st.hour;
  sst.wMilliseconds = st.milliseconds;
  sst.wMinute = st.minute;
  sst.wMonth = st.month;
  sst.wSecond = st.second;
  sst.wYear = st.year;
}
} // namespace

void Sleep(uint32_t milliSeconds)
{
  ::Sleep(milliSeconds);
}

uint32_t GetTimeZoneInformation(TimeZoneInformation* timeZoneInformation)
{
  if (!timeZoneInformation)
    return KODI_TIME_ZONE_ID_INVALID;

  TIME_ZONE_INFORMATION tzi;
  uint32_t result = ::GetTimeZoneInformation(&tzi);
  if (result == KODI_TIME_ZONE_ID_INVALID)
    return result;

  timeZoneInformation->bias = tzi.Bias;
  timeZoneInformation->daylightBias = tzi.DaylightBias;
  SystemTimeToKodiTime(tzi.DaylightDate, timeZoneInformation->daylightDate);
  timeZoneInformation->daylightName = FromW(tzi.DaylightName);
  timeZoneInformation->standardBias = tzi.StandardBias;
  SystemTimeToKodiTime(tzi.StandardDate, timeZoneInformation->standardDate);
  timeZoneInformation->standardName = FromW(tzi.StandardName);

  return result;
}

void GetLocalTime(SystemTime* systemTime)
{
  SYSTEMTIME time;
  ::GetLocalTime(&time);

  systemTime->year = time.wYear;
  systemTime->month = time.wMonth;
  systemTime->dayOfWeek = time.wDayOfWeek;
  systemTime->day = time.wDay;
  systemTime->hour = time.wHour;
  systemTime->minute = time.wMinute;
  systemTime->second = time.wSecond;
  systemTime->milliseconds = time.wMilliseconds;
}

int FileTimeToLocalFileTime(const FILETIME* lpFileTime, LPFILETIME lpLocalFileTime)
{
  return ::FileTimeToLocalFileTime(lpFileTime, lpLocalFileTime);
}

int SystemTimeToFileTime(const SystemTime* systemTime, LPFILETIME lpFileTime)
{
  SYSTEMTIME time;
  KodiTimeToSystemTime(*systemTime, time);
  return ::SystemTimeToFileTime(&time, lpFileTime);
}

long CompareFileTime(const FILETIME* lpFileTime1, const FILETIME* lpFileTime2)
{
  return ::CompareFileTime(lpFileTime1, lpFileTime2);
}

int FileTimeToSystemTime(const FILETIME* lpFileTime, SystemTime* systemTime)
{
  SYSTEMTIME time;
  int ret = ::FileTimeToSystemTime(lpFileTime, &time);
  SystemTimeToKodiTime(time, *systemTime);

  return ret;
}

int LocalFileTimeToFileTime(const FILETIME* lpLocalFileTime, LPFILETIME lpFileTime)
{
  return ::LocalFileTimeToFileTime(lpLocalFileTime, lpFileTime);
}

} // namespace TIME
} // namespace KODI
