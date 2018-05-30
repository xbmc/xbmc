/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include <string>
#include <utility>

#include "commons/ilog.h"
#include "utils/StringUtils.h"


class CLog
{
public:
  CLog();
  ~CLog();
  static void Close();

  static void Log(int loglevel, const char* format)
  {
    if (IsLogLevelLogged(loglevel))
      LogString(loglevel, format);
  }

  template<typename... Args>
  static void Log(int loglevel, const char* format, Args&&... args)
  {
    if (IsLogLevelLogged(loglevel))
      LogString(loglevel, StringUtils::Format(format, std::forward<Args>(args)...));
  }

  static void Log(int loglevel, int component, const char* format)
  {
    if (IsLogLevelLogged(loglevel))
      LogString(loglevel, component, format);
  }

  template<typename... Args>
  static void Log(int loglevel, int component, const char* format, Args&&... args)
  {
    // We defer component checking to LogString to avoid having to drag in advancedsettings
    // everywhere we want to log anything
    if (IsLogLevelLogged(loglevel))
      LogString(loglevel, component, StringUtils::Format(format, std::forward<Args>(args)...));
  }

  static void LogFunction(int loglevel, std::string functionName, const char* format)
  {
    if (IsLogLevelLogged(loglevel))
      LogString(loglevel, functionName + ": " + format);
  }

  template<typename... Args>
  static void LogFunction(int loglevel,
                          std::string functionName,
                          const char* format,
                          Args&&... args)
  {
    if (IsLogLevelLogged(loglevel))
    {
      functionName.append(": ");
      LogString(loglevel, functionName + StringUtils::Format(format, std::forward<Args>(args)...));
    }
  }

  static void LogFunction(int loglevel, std::string functionName, int component, const char* format)
  {
    if (IsLogLevelLogged(loglevel))
      LogString(loglevel, component, functionName + ": " + format);
  }

  template<typename... Args>
  static void LogFunction(
      int loglevel, std::string functionName, int component, const char* format, Args&&... args)
  {
    // We defer component checking to LogString to avoid having to drag in advancedsettings
    // everywhere we want to log anything
    if (IsLogLevelLogged(loglevel))
    {
      functionName.append(": ");
      LogString(loglevel, component,
                functionName + StringUtils::Format(format, std::forward<Args>(args)...));
    }
  }
#define LogF(loglevel, ...) LogFunction((loglevel), __FUNCTION__, ##__VA_ARGS__)
#define LogFC(loglevel, component, ...) LogFunction((loglevel), __FUNCTION__, (component), ##__VA_ARGS__)
  static void MemDump(char *pData, int length);
  static bool Init(const std::string& path);
  static void PrintDebugString(const std::string& line); // universal interface for printing debug strings
  static void SetLogLevel(int level);
  static int  GetLogLevel();
  static void SetExtraLogLevels(int level);
  static bool IsLogLevelLogged(int loglevel);

protected:
  static void LogString(int logLevel, std::string&& logString);
  static void LogString(int logLevel, int component, std::string&& logString);
  static bool WriteLogString(int logLevel, const std::string& logString);
};
