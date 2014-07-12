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

#include "system.h"
#include "log.h"
#include "stdio_utf8.h"
#include "stat_utf8.h"
#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"
#include "threads/Thread.h"
#include "utils/StringUtils.h"
#if defined(TARGET_ANDROID)
#include "android/activity/XBMCApp.h"
#elif defined(TARGET_WINDOWS)
#include "utils/auto_buffer.h"
#include "win32/WIN32Util.h"
#endif

static const char* const levelNames[] =
{"DEBUG", "INFO", "NOTICE", "WARNING", "ERROR", "SEVERE", "FATAL", "NONE"};

// s_globals is used as static global with CLog global variables
#define s_globals XBMC_GLOBAL_USE(CLog).m_globalInstance

CLog::CLog()
{}

CLog::~CLog()
{}

void CLog::Close()
{
  
  CSingleLock waitLock(s_globals.critSec);
  if (s_globals.m_file)
  {
    fclose(s_globals.m_file);
    s_globals.m_file = NULL;
  }
  s_globals.m_repeatLine.clear();
}

void CLog::Log(int loglevel, const char *format, ... )
{
  CSingleLock waitLock(s_globals.critSec);
  if (IsLogLevelLogged(loglevel))
  {
    std::string strData;

    va_list va;
    va_start(va, format);
    strData = StringUtils::FormatV(format,va);
    va_end(va);

    if (s_globals.m_repeatLogLevel == loglevel && s_globals.m_repeatLine == strData)
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
    s_globals.m_repeatLogLevel = loglevel;

    StringUtils::TrimRight(strData);
    if (strData.empty())
      return;
    
    PrintDebugString(strData);

    WriteLogString(loglevel, strData);
  }
}

bool CLog::Init(const char* path)
{
  CSingleLock waitLock(s_globals.critSec);
  if (!s_globals.m_file)
  {
    // the log folder location is initialized in the CAdvancedSettings
    // constructor and changed in CApplication::Create()
    std::string strLogFile = StringUtils::Format("%sxbmc.log", path);
    std::string strLogFileOld = StringUtils::Format("%sxbmc.old.log", path);

#if defined(TARGET_WINDOWS)
    // the appdata folder might be redirected to an unc share
    // convert smb to unc path that stat and fopen can handle it
    strLogFile = CWIN32Util::SmbToUnc(strLogFile);
    strLogFileOld = CWIN32Util::SmbToUnc(strLogFileOld);
#endif

    struct stat64 info;
    if (stat64_utf8(strLogFileOld.c_str(),&info) == 0 &&
        remove_utf8(strLogFileOld.c_str()) != 0)
      return false;
    if (stat64_utf8(strLogFile.c_str(),&info) == 0 &&
        rename_utf8(strLogFile.c_str(),strLogFileOld.c_str()) != 0)
      return false;

    s_globals.m_file = fopen64_utf8(strLogFile.c_str(), "wb");
  }

  if (s_globals.m_file)
  {
    unsigned char BOM[3] = {0xEF, 0xBB, 0xBF};
    fwrite(BOM, sizeof(BOM), 1, s_globals.m_file);
  }

  return s_globals.m_file != NULL;
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
  s_globals.m_logLevel = level;
  CLog::Log(LOGNOTICE, "Log level changed to %d", s_globals.m_logLevel);
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
#if defined(TARGET_WINDOWS)
  // we can't use charsetconverter here as it's initialized later than CLog and deinitialized early
  int bufSize = MultiByteToWideChar(CP_UTF8, 0, line.c_str(), line.length(), NULL, 0);
  XUTILS::auto_buffer buf(sizeof(wchar_t) * (bufSize + 1)); // '+1' for extra safety
  if (MultiByteToWideChar(CP_UTF8, 0, line.c_str(), line.length(), (wchar_t*)buf.get(), buf.size() / sizeof(wchar_t)) == bufSize)
    ::OutputDebugStringW(std::wstring((wchar_t*)buf.get(), bufSize).c_str());
  else
    ::OutputDebugStringA(line.c_str());
  ::OutputDebugStringW(L"\n");
#elif defined(TARGET_ANDROID)
  //print to adb
  CXBMCApp::android_printf("%s",line.c_str());
#endif
#endif // defined(_DEBUG) || defined(PROFILE)
}

bool CLog::WriteLogString(int logLevel, const std::string& logString)
{
  static const char* prefixFormat = "%02.2d:%02.2d:%02.2d T:%" PRIu64" %7s: ";

  if (!s_globals.m_file)
    return false;

  std::string strData(logString);
  /* fixup newline alignment, number of spaces should equal prefix length */
  StringUtils::Replace(strData, "\n", LINE_ENDING"                                            ");
  strData += LINE_ENDING;

  SYSTEMTIME time;
  GetLocalTime(&time);

  std::string strPrefix = StringUtils::Format(prefixFormat,
                                  time.wHour,
                                  time.wMinute,
                                  time.wSecond,
                                  (uint64_t)CThread::GetCurrentThreadId(),
                                  levelNames[logLevel]);

  fputs(strPrefix.c_str(), s_globals.m_file);
  fputs(strData.c_str(), s_globals.m_file);
  fflush(s_globals.m_file);
  return true;
}
