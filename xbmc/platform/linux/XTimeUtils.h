/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PlatformDefs.h"

void GetLocalTime(LPSYSTEMTIME);

void WINAPI Sleep(uint32_t dwMilliSeconds);

int FileTimeToLocalFileTime(const FILETIME* lpFileTime, LPFILETIME lpLocalFileTime);
int SystemTimeToFileTime(const SYSTEMTIME* lpSystemTime,  LPFILETIME lpFileTime);
long CompareFileTime(const FILETIME* lpFileTime1, const FILETIME* lpFileTime2);
int FileTimeToSystemTime( const FILETIME* lpFileTime, LPSYSTEMTIME lpSystemTime);
int LocalFileTimeToFileTime( const FILETIME* lpLocalFileTime, LPFILETIME lpFileTime);

int FileTimeToTimeT(const FILETIME* lpLocalFileTime, time_t *pTimeT);
int TimeTToFileTime(time_t timeT, FILETIME* lpLocalFileTime);
