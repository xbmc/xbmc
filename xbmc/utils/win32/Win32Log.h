#pragma once

/*
*      Copyright (C) 2014 Team XBMC
*      http://xbmc.org
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#include "utils/log.h"

#ifndef TARGET_WINDOWS
#error This file is for Win32 platfrom only
#endif // TARGET_WINDOWS


// CLog version for Win32 with additional widestring logging capabilities
class CWin32Log : public CLog
{
public:
  static void LogW(int loglevel, PRINTF_FORMAT_STRING const wchar_t *format, ...);
  static void LogFunctionW(int loglevel, IN_OPT_STRING const char* functionName, PRINTF_FORMAT_STRING const wchar_t* format, ...);
#define LogFW(loglevel,format,...) LogFunctionW((loglevel),__FUNCTION__,(format),##__VA_ARGS__)
};

// substitute CWin32Log instead of CLog for Win32
#define CLog CWin32Log
