#pragma once

/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include <string>

#if defined(TARGET_POSIX)
#include "posix/PosixInterfaceForCLog.h"
typedef class CPosixInterfaceForCLog PlatformInterfaceForCLog;
#elif defined(TARGET_WINDOWS)
#include "win32/Win32InterfaceForCLog.h"
typedef class CWin32InterfaceForCLog PlatformInterfaceForCLog;
#endif

#include "commons/ilog.h"
#include "threads/CriticalSection.h"
#include "utils/GlobalsHandling.h"

#include "utils/params_check_macros.h"

class CLog
{
public:
  CLog();
  ~CLog(void);
  static void Close();
  static void Log(int loglevel, PRINTF_FORMAT_STRING const char *format, ...) PARAM2_PRINTF_FORMAT;
  static void LogFunction(int loglevel, IN_OPT_STRING const char* functionName, PRINTF_FORMAT_STRING const char* format, ...) PARAM3_PRINTF_FORMAT;
#define LogF(loglevel,format,...) LogFunction((loglevel),__FUNCTION__,(format),##__VA_ARGS__)
  static void MemDump(char *pData, int length);
  static bool Init(const std::string& path);
  static void PrintDebugString(const std::string& line); // universal interface for printing debug strings
  static void SetLogLevel(int level);
  static int  GetLogLevel();
  static void SetExtraLogLevels(int level);
  static bool IsLogLevelLogged(int loglevel);

protected:
  class CLogGlobals
  {
  public:
    CLogGlobals(void) : m_repeatCount(0), m_repeatLogLevel(-1), m_logLevel(LOG_LEVEL_DEBUG), m_extraLogLevels(0) {}
    ~CLogGlobals() {}
    PlatformInterfaceForCLog m_platform;
    int         m_repeatCount;
    int         m_repeatLogLevel;
    std::string m_repeatLine;
    int         m_logLevel;
    int         m_extraLogLevels;
    CCriticalSection critSec;
  };
  class CLogGlobals m_globalInstance; // used as static global variable
  static void LogString(int logLevel, const std::string& logString);
  static bool WriteLogString(int logLevel, const std::string& logString);
};


namespace XbmcUtils
{
  class LogImplementation : public XbmcCommons::ILogger
  {
  public:
    virtual ~LogImplementation() {}
    inline virtual void log(int logLevel, IN_STRING const char* message) { CLog::Log(logLevel, "%s", message); }
  };
}

XBMC_GLOBAL_REF(CLog, g_log);
