/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#include "log.h"
#ifndef _LINUX
#include <share.h>
#include "CharsetConverter.h"
#endif
#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"
#include "threads/Thread.h"
#include "utils/StdString.h"

FILE*       CLog::m_file            = NULL;
int         CLog::m_repeatCount     = 0;
int         CLog::m_repeatLogLevel  = -1;
std::string CLog::m_repeatLine;
int         CLog::m_logLevel        = LOG_LEVEL_DEBUG;

static CCriticalSection critSec;

static char levelNames[][8] =
{"DEBUG", "INFO", "NOTICE", "WARNING", "ERROR", "SEVERE", "FATAL", "NONE"};

CLog::CLog()
{}

CLog::~CLog()
{}

void CLog::Close()
{
  CSingleLock waitLock(critSec);
  if (m_file)
  {
    fclose(m_file);
    m_file = NULL;
  }
  m_repeatLine.clear();
}

void CLog::Log(int loglevel, const char *format, ... )
{
  static const char* prefixFormat = "%02.2d:%02.2d:%02.2d T:%"PRIu64" M:%9"PRIu64" %7s: ";
  CSingleLock waitLock(critSec);
#if !(defined(_DEBUG) || defined(PROFILE))
  if (m_logLevel > LOG_LEVEL_NORMAL ||
     (m_logLevel > LOG_LEVEL_NONE && loglevel >= LOGNOTICE))
#endif
  {
    if (!m_file)
      return;

    SYSTEMTIME time;
    GetLocalTime(&time);

    MEMORYSTATUS stat;
    GlobalMemoryStatus(&stat);

    CStdString strPrefix, strData;

    strData.reserve(16384);
    va_list va;
    va_start(va, format);
    strData.FormatV(format,va);
    va_end(va);

    if (m_repeatLogLevel == loglevel && m_repeatLine == strData)
    {
      m_repeatCount++;
      return;
    }
    else if (m_repeatCount)
    {
      CStdString strData2;
      strPrefix.Format(prefixFormat, time.wHour, time.wMinute, time.wSecond, (uint64_t)CThread::GetCurrentThreadId(), (uint64_t)stat.dwAvailPhys, levelNames[m_repeatLogLevel]);

      strData2.Format("Previous line repeats %d times." LINE_ENDING, m_repeatCount);
      fputs(strPrefix.c_str(), m_file);
      fputs(strData2.c_str(), m_file);
      OutputDebugString(strData2);
      m_repeatCount = 0;
    }
    
    m_repeatLine      = strData;
    m_repeatLogLevel  = loglevel;

    unsigned int length = 0;
    while ( length != strData.length() )
    {
      length = strData.length();
      strData.TrimRight(" ");
      strData.TrimRight('\n');
      strData.TrimRight("\r");
    }

    if (!length)
      return;
    
    OutputDebugString(strData);

    /* fixup newline alignment, number of spaces should equal prefix length */
    strData.Replace("\n", LINE_ENDING"                                            ");
    strData += LINE_ENDING;

    strPrefix.Format(prefixFormat, time.wHour, time.wMinute, time.wSecond, (uint64_t)CThread::GetCurrentThreadId(), (uint64_t)stat.dwAvailPhys, levelNames[loglevel]);

    fputs(strPrefix.c_str(), m_file);
    fputs(strData.c_str(), m_file);
    fflush(m_file);
  }
}

bool CLog::Init(const char* path)
{
  CSingleLock waitLock(critSec);
  if (!m_file)
  {
    // g_settings.m_logFolder is initialized in the CSettings constructor
    // and changed in CApplication::Create()
#ifdef _WIN32
    CStdStringW pathW;
    g_charsetConverter.utf8ToW(path, pathW, false);
    CStdStringW strLogFile, strLogFileOld;

    strLogFile.Format(L"%sxbmc.log", pathW);
    strLogFileOld.Format(L"%sxbmc.old.log", pathW);

    struct __stat64 info;
    if (_wstat64(strLogFileOld.c_str(),&info) == 0 &&
        !::DeleteFileW(strLogFileOld.c_str()))
      return false;
    if (_wstat64(strLogFile.c_str(),&info) == 0 &&
        !::MoveFileW(strLogFile.c_str(),strLogFileOld.c_str()))
      return false;

    m_file = _wfsopen(strLogFile.c_str(),L"wb", _SH_DENYWR);
#else
    CStdString strLogFile, strLogFileOld;

    strLogFile.Format("%sxbmc.log", path);
    strLogFileOld.Format("%sxbmc.old.log", path);

    struct stat64 info;
    if (stat64(strLogFileOld.c_str(),&info) == 0 &&
        remove(strLogFileOld.c_str()) != 0)
      return false;
    if (stat64(strLogFile.c_str(),&info) == 0 &&
        rename(strLogFile.c_str(),strLogFileOld.c_str()) != 0)
      return false;

    m_file = fopen(strLogFile.c_str(),"wb");
#endif
  }

  if (m_file)
  {
    unsigned char BOM[3] = {0xEF, 0xBB, 0xBF};
    fwrite(BOM, sizeof(BOM), 1, m_file);
  }

  return m_file != NULL;
}

void CLog::MemDump(char *pData, int length)
{
  Log(LOGDEBUG, "MEM_DUMP: Dumping from %p", pData);
  for (int i = 0; i < length; i+=16)
  {
    CStdString strLine;
    strLine.Format("MEM_DUMP: %04x ", i);
    char *alpha = pData;
    for (int k=0; k < 4 && i + 4*k < length; k++)
    {
      for (int j=0; j < 4 && i + 4*k + j < length; j++)
      {
        CStdString strFormat;
        strFormat.Format(" %02x", *pData++);
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
  CSingleLock waitLock(critSec);
  m_logLevel = level;
  CLog::Log(LOGNOTICE, "Log level changed to %d", m_logLevel);
}

int CLog::GetLogLevel()
{
  return m_logLevel;
}

void CLog::OutputDebugString(const std::string& line)
{
#if defined(_DEBUG) || defined(PROFILE)
  ::OutputDebugString(line.c_str());
  ::OutputDebugString("\n");
#endif
}
