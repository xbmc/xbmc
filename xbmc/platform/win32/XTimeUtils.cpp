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
#include <Windows.h>

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

int FileTimeToLocalFileTime(const FileTime* fileTime, FileTime* localFileTime)
{
  SYSTEMTIME systemTime{};
  if (FALSE == ::FileTimeToSystemTime(reinterpret_cast<const FILETIME*>(fileTime), &systemTime))
    return FALSE;

  SYSTEMTIME localSystemTime{};
  if (FALSE == ::SystemTimeToTzSpecificLocalTime(nullptr, &systemTime, &localSystemTime))
    return FALSE;

  return ::SystemTimeToFileTime(&localSystemTime, reinterpret_cast<FILETIME*>(localFileTime));
}

int SystemTimeToFileTime(const SystemTime* systemTime, FileTime* fileTime)
{
  SYSTEMTIME time;
  time.wYear = systemTime->year;
  time.wMonth = systemTime->month;
  time.wDayOfWeek = systemTime->dayOfWeek;
  time.wDay = systemTime->day;
  time.wHour = systemTime->hour;
  time.wMinute = systemTime->minute;
  time.wSecond = systemTime->second;
  time.wMilliseconds = systemTime->milliseconds;

  return ::SystemTimeToFileTime(&time, reinterpret_cast<FILETIME*>(fileTime));
}

long CompareFileTime(const FileTime* fileTime1, const FileTime* fileTime2)
{
  return ::CompareFileTime(reinterpret_cast<const FILETIME*>(fileTime1),
                           reinterpret_cast<const FILETIME*>(fileTime2));
}

int FileTimeToSystemTime(const FileTime* fileTime, SystemTime* systemTime)
{
  SYSTEMTIME time{};
  int ret = ::FileTimeToSystemTime(reinterpret_cast<const FILETIME*>(fileTime), &time);
  SystemTimeToKodiTime(time, *systemTime);

  return ret;
}

int LocalFileTimeToFileTime(const FileTime* localFileTime, FileTime* fileTime)
{
  return ::LocalFileTimeToFileTime(reinterpret_cast<const FILETIME*>(localFileTime),
                                   reinterpret_cast<FILETIME*>(fileTime));
}

} // namespace TIME
} // namespace KODI
