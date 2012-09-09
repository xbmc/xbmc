#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#ifdef __GNUC__
#define ATTRIB_LOG_FORMAT __attribute__((format(printf,2,3)))
#else
#define ATTRIB_LOG_FORMAT
#endif

class CLog
{
public:

  class CLogGlobals
  {
  public:
    CLogGlobals() : m_file(NULL), m_repeatCount(0), m_repeatLogLevel(-1), m_logLevel(LOG_LEVEL_DEBUG) {}
    FILE*       m_file;
    int         m_repeatCount;
    int         m_repeatLogLevel;
    std::string m_repeatLine;
    int         m_logLevel;
    CCriticalSection critSec;
  };

  CLog();
  virtual ~CLog(void);
  static void Close();
  static void Log(int loglevel, const char *format, ... ) ATTRIB_LOG_FORMAT;
  static void MemDump(char *pData, int length);
  static bool Init(const char* path);
  static void SetLogLevel(int level);
  static int  GetLogLevel();
private:
  static void OutputDebugString(const std::string& line);
};

#undef ATTRIB_LOG_FORMAT

namespace XbmcUtils
{
  class LogImplementation : public XbmcCommons::ILogger
  {
  public:
    virtual ~LogImplementation() {}
    inline virtual void log(int logLevel, const char* message) { CLog::Log(logLevel,"%s",message); }
  };
}

XBMC_GLOBAL_REF(CLog::CLogGlobals,g_log_globals);
