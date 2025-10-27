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

namespace KODI
{
namespace TIME
{
namespace
{
// Check binary compatibility of FileTime / FILETIME at compile time
static_assert(sizeof(FileTime) == sizeof(FILETIME));
static_assert(offsetof(FileTime, lowDateTime) == offsetof(FILETIME, dwLowDateTime));

// Check binary compatibility of SystemTime / SYSTEMTIME at compile time
static_assert(sizeof(SystemTime) == sizeof(SYSTEMTIME));
static_assert(offsetof(SystemTime, year) == offsetof(SYSTEMTIME, wYear));
static_assert(offsetof(SystemTime, month) == offsetof(SYSTEMTIME, wMonth));
static_assert(offsetof(SystemTime, dayOfWeek) == offsetof(SYSTEMTIME, wDayOfWeek));
static_assert(offsetof(SystemTime, day) == offsetof(SYSTEMTIME, wDay));
static_assert(offsetof(SystemTime, hour) == offsetof(SYSTEMTIME, wHour));
static_assert(offsetof(SystemTime, minute) == offsetof(SYSTEMTIME, wMinute));
static_assert(offsetof(SystemTime, second) == offsetof(SYSTEMTIME, wSecond));
} // namespace

std::tuple<bool, int64_t> GetTimezoneBias(const SystemTime& time)
{
  union FILETIME64
  {
    FILETIME ft;
    LONGLONG ft64;
  };

  // Compute offset = local_filetime - utc_filetime
  FILETIME64 utc{};
  if (FALSE == ::SystemTimeToFileTime(reinterpret_cast<const SYSTEMTIME*>(&time), &utc.ft))
    return {false, 0}; // error

  FILETIME64 local{};
  if (FALSE == ::FileTimeToLocalFileTime(&utc.ft, &local.ft))
    return {false, 0}; // error

  const int64_t gmtoff{(utc.ft64 - local.ft64) / 10000000LL / 60}; // 100-ns intervals -> minutes
  return {true, gmtoff};
}

void GetLocalTime(SystemTime* systemTime)
{
  ::GetLocalTime(reinterpret_cast<SYSTEMTIME*>(systemTime));
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
  return ::SystemTimeToFileTime(reinterpret_cast<const SYSTEMTIME*>(systemTime),
                                reinterpret_cast<FILETIME*>(fileTime));
}

long CompareFileTime(const FileTime* fileTime1, const FileTime* fileTime2)
{
  return ::CompareFileTime(reinterpret_cast<const FILETIME*>(fileTime1),
                           reinterpret_cast<const FILETIME*>(fileTime2));
}

int FileTimeToSystemTime(const FileTime* fileTime, SystemTime* systemTime)
{
  return ::FileTimeToSystemTime(reinterpret_cast<const FILETIME*>(fileTime),
                                reinterpret_cast<SYSTEMTIME*>(systemTime));
}

int LocalFileTimeToFileTime(const FileTime* localFileTime, FileTime* fileTime)
{
  return ::LocalFileTimeToFileTime(reinterpret_cast<const FILETIME*>(localFileTime),
                                   reinterpret_cast<FILETIME*>(fileTime));
}

} // namespace TIME
} // namespace KODI
