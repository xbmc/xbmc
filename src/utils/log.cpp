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

#include "log.h"
#include "system.h"
#include "threads/SingleLock.h"
#include "threads/Thread.h"
#include "utils/StringUtils.h"
#include "CompileInfo.h"

static const char* const levelNames[] =
{"DEBUG", "INFO", "NOTICE", "WARNING", "ERROR", "SEVERE", "FATAL", "NONE"};

// add 1 to level number to get index of name
static const char* const logLevelNames[] =
{ "LOG_LEVEL_NONE" /*-1*/, "LOG_LEVEL_NORMAL" /*0*/, "LOG_LEVEL_DEBUG" /*1*/, "LOG_LEVEL_DEBUG_FREEMEM" /*2*/ };

// s_globals is used as static global with CLog global variables
#define s_globals XBMC_GLOBAL_USE(CLog).m_globalInstance

CLog::CLog()
{}

CLog::~CLog()
{}

void CLog::Close()
{
  CSingleLock waitLock(s_globals.critSec);
  s_globals.m_platform.CloseLogFile();
  s_globals.m_repeatLine.clear();
}

void CLog::Log(int loglevel, const char *format, ...)
{
  if (IsLogLevelLogged(loglevel))
  {
    va_list va;
    va_start(va, format);
    LogString(loglevel, StringUtils::FormatV(format, va));
    va_end(va);
  }
}

void CLog::LogFunction(int loglevel, const char* functionName, const char* format, ...)
{
  if (IsLogLevelLogged(loglevel))
  {
    std::string fNameStr;
    if (functionName && functionName[0])
      fNameStr.assign(functionName).append(": ");
    va_list va;
    va_start(va, format);
    LogString(loglevel, fNameStr + StringUtils::FormatV(format, va));
    va_end(va);
  }
}

void CLog::LogString(int logLevel, const std::string& logString)
{
  CSingleLock waitLock(s_globals.critSec);
  std::string strData(logString);
  StringUtils::TrimRight(strData);
  if (!strData.empty())
  {
    if (s_globals.m_repeatLogLevel == logLevel && s_globals.m_repeatLine == strData)
    {
      s_globals.m_repeatCount++;
      return;
    }
    else if (s_globals.m_repeatCount)
    {
      std::string strData2 = StringUtils::Format("Previous line repeats %d times.",
                                                s_globals.m_repeatCount);
      PrintDebugString(strData2);
      WriteLogString(s_globals.m_repeatLogLevel, strData2);
      s_globals.m_repeatCount = 0;
    }
    
    s_globals.m_repeatLine = strData;
    s_globals.m_repeatLogLevel = logLevel;

    PrintDebugString(strData);

    WriteLogString(logLevel, strData);
  }
}

bool CLog::Init(const std::string& path)
{
  CSingleLock waitLock(s_globals.critSec);

  // the log folder location is initialized in the CAdvancedSettings
  // constructor and changed in CApplication::Create()

  std::string appName = CCompileInfo::GetAppName();
  StringUtils::ToLower(appName);
  return s_globals.m_platform.OpenLogFile(path + appName + ".log", path + appName + ".old.log");
}

void CLog::MemDump(char *pData, int length)
{
  Log(LOGDEBUG, "MEM_DUMP: Dumping from %p", pData);
  for (int i = 0; i < length; i+=16)
  {
    std::string strLine = StringUtils::Format("MEM_DUMP: %04x ", i);
    char *alpha = pData;
    for (int k=0; k < 4 && i + 4*k < length; k++)
    {
      for (int j=0; j < 4 && i + 4*k + j < length; j++)
      {
        std::string strFormat = StringUtils::Format(" %02x", (unsigned char)*pData++);
        strLine += strFormat;
      }
      strLine += " ";
    }
    // pad with spaces
    while (strLine.size() < 13*4 + 16)
      strLine += " ";
    for (int j=0; j < 16 && i + j < length; j++)
    {
      if (*alpha > 31)
        strLine += *alpha;
      else
        strLine += '.';
      alpha++;
    }
    Log(LOGDEBUG, "%s", strLine.c_str());
  }
}

void CLog::SetLogLevel(int level)
{
  CSingleLock waitLock(s_globals.critSec);
  if (level >= LOG_LEVEL_NONE && level <= LOG_LEVEL_MAX)
  {
    s_globals.m_logLevel = level;
    CLog::Log(LOGNOTICE, "Log level changed to \"%s\"", logLevelNames[s_globals.m_logLevel + 1]);
  }
  else
    CLog::Log(LOGERROR, "%s: Invalid log level requested: %d", __FUNCTION__, level);
}

int CLog::GetLogLevel()
{
  return s_globals.m_logLevel;
}

void CLog::SetExtraLogLevels(int level)
{
  CSingleLock waitLock(s_globals.critSec);
  s_globals.m_extraLogLevels = level;
}

bool CLog::IsLogLevelLogged(int loglevel)
{
  const int extras = (loglevel & ~LOGMASK);
  if (extras != 0 && (s_globals.m_extraLogLevels & extras) == 0)
    return false;

#if defined(_DEBUG) || defined(PROFILE)
  return true;
#else
  if (s_globals.m_logLevel >= LOG_LEVEL_DEBUG)
    return true;
  if (s_globals.m_logLevel <= LOG_LEVEL_NONE)
    return false;

  // "m_logLevel" is "LOG_LEVEL_NORMAL"
  return (loglevel & LOGMASK) >= LOGNOTICE;
#endif
}


void CLog::PrintDebugString(const std::string& line)
{
#if defined(_DEBUG) || defined(PROFILE)
  s_globals.m_platform.PrintDebugString(line);
#endif // defined(_DEBUG) || defined(PROFILE)
}

bool CLog::WriteLogString(int logLevel, const std::string& logString)
{
  static const char* prefixFormat = "%02.2d:%02.2d:%02.2d T:%" PRIu64" %7s: ";

  std::string strData(logString);
  /* fixup newline alignment, number of spaces should equal prefix length */
  StringUtils::Replace(strData, "\n", "\n                                            ");

  int hour, minute, second;
  s_globals.m_platform.GetCurrentLocalTime(hour, minute, second);
  
  strData = StringUtils::Format(prefixFormat,
                                  hour,
                                  minute,
                                  second,
                                  (uint64_t)CThread::GetCurrentThreadId(),
                                  levelNames[logLevel]) + strData;

  return s_globals.m_platform.WriteStringToLog(strData);
}
