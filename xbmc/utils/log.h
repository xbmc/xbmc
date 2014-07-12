#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <stdio.h>
#include <string>

#include "commons/ilog.h"
#include "threads/CriticalSection.h"
#include "utils/GlobalsHandling.h"

#ifndef PARAM2_PRINTF_FORMAT
#ifdef __GNUC__
#define PARAM2_PRINTF_FORMAT __attribute__((format(printf,2,3)))
#else
#define PARAM2_PRINTF_FORMAT
#endif
#endif // PARAM2_PRINTF_FORMAT

#ifndef PRINTF_FORMAT_STRING
#ifdef _MSC_VER
#include <sal.h>
#define PRINTF_FORMAT_STRING _In_z_ _Printf_format_string_
#define IN_STRING _In_z_
#define IN_OPT_STRING _In_opt_z_
#else  // ! _MSC_VER
#define PRINTF_FORMAT_STRING
#define IN_STRING
#define IN_OPT_STRING
#endif // ! _MSC_VER
#endif // PRINTF_FORMAT_STRING

class CLog
{
public:
  CLog();
  ~CLog(void);
  static void Close();
  static void Log(int loglevel, PRINTF_FORMAT_STRING const char *format, ...) PARAM2_PRINTF_FORMAT;
  static void MemDump(char *pData, int length);
  static bool Init(IN_STRING const char* path);
  static void SetLogLevel(int level);
  static int  GetLogLevel();
  static void SetExtraLogLevels(int level);
  static bool IsLogLevelLogged(int loglevel);

private:
  class CLogGlobals
  {
  public:
    CLogGlobals(void) : m_file(NULL), m_repeatCount(0), m_repeatLogLevel(-1), m_logLevel(LOG_LEVEL_DEBUG), m_extraLogLevels(0) {}
    ~CLogGlobals() {}
    FILE*       m_file;
    int         m_repeatCount;
    int         m_repeatLogLevel;
    std::string m_repeatLine;
    int         m_logLevel;
    int         m_extraLogLevels;
    CCriticalSection critSec;
  };
  class CLogGlobals m_globalInstance; // used as static global variable
  static void PrintDebugString(const std::string& line);
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
