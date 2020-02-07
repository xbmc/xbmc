/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

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

struct TimeZoneInformation
{
  long bias;
  std::string standardName;
  SystemTime standardDate;
  long standardBias;
  std::string daylightName;
  SystemTime daylightDate;
  long daylightBias;
};

constexpr int KODI_TIME_ZONE_ID_INVALID{-1};
constexpr int KODI_TIME_ZONE_ID_UNKNOWN{0};
constexpr int KODI_TIME_ZONE_ID_STANDARD{1};
constexpr int KODI_TIME_ZONE_ID_DAYLIGHT{2};

void GetLocalTime(SystemTime* systemTime);
uint32_t GetTimeZoneInformation(TimeZoneInformation* timeZoneInformation);

void Sleep(uint32_t milliSeconds);

int FileTimeToLocalFileTime(const FILETIME* lpFileTime, LPFILETIME lpLocalFileTime);
int SystemTimeToFileTime(const SystemTime* systemTime, LPFILETIME lpFileTime);
long CompareFileTime(const FILETIME* lpFileTime1, const FILETIME* lpFileTime2);
int FileTimeToSystemTime(const FILETIME* lpFileTime, SystemTime* systemTime);
int LocalFileTimeToFileTime(const FILETIME* lpLocalFileTime, LPFILETIME lpFileTime);

int FileTimeToTimeT(const FILETIME* lpLocalFileTime, time_t* pTimeT);
int TimeTToFileTime(time_t timeT, FILETIME* lpLocalFileTime);
} // namespace TIME
} // namespace KODI
