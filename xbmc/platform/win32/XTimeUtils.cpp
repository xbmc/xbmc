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

void Sleep(uint32_t milliSeconds)
{
  ::Sleep(milliSeconds);
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

  FILETIME file;
  int ret = ::SystemTimeToFileTime(&time, &file);

  fileTime->lowDateTime = file.dwLowDateTime;
  fileTime->highDateTime = file.dwHighDateTime;

  return ret;
}

long CompareFileTime(const FileTime* fileTime1, const FileTime* fileTime2)
{
  FILETIME file1;
  file1.dwLowDateTime = fileTime1->lowDateTime;
  file1.dwHighDateTime = fileTime1->highDateTime;

  FILETIME file2;
  file2.dwLowDateTime = fileTime2->lowDateTime;
  file2.dwHighDateTime = fileTime2->highDateTime;

  return ::CompareFileTime(&file1, &file2);
}

int FileTimeToSystemTime(const FileTime* fileTime, SystemTime* systemTime)
{
  FILETIME file;
  file.dwLowDateTime = fileTime->lowDateTime;
  file.dwHighDateTime = fileTime->highDateTime;

  SYSTEMTIME time;
  int ret = ::FileTimeToSystemTime(&file, &time);
  SystemTimeToKodiTime(time, *systemTime);

  return ret;
}

int LocalFileTimeToFileTime(const FileTime* localFileTime, FileTime* fileTime)
{
  FILETIME localFile;
  localFile.dwLowDateTime = localFileTime->lowDateTime;
  localFile.dwHighDateTime = localFileTime->highDateTime;

  FILETIME file;

  int ret = ::LocalFileTimeToFileTime(&localFile, &file);

  fileTime->lowDateTime = file.dwLowDateTime;
  fileTime->highDateTime = file.dwHighDateTime;

  return ret;
}

} // namespace TIME
} // namespace KODI
