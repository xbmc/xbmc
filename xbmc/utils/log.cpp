
#include "../stdafx.h"
#include "log.h"
#include <share.h>
#include "criticalsection.h"
#include "singlelock.h"
#include "StdString.h"
#include "../Settings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static CLog g_logger;

CLog::CLog()
{}

CLog::~CLog()
{}

// put here so debug and normal logger can share it
static char tmp[16384];

FILE* CLog::fd = NULL;
CCriticalSection critSec;




void CLog::Close()
{
  CSingleLock waitLock(critSec);
  if (fd)
  {
    fclose(fd);
    fd = NULL;
  }
}


void CLog::Log(int loglevel, const char *format, ... )
{
  if (loglevel >= g_stSettings.m_iLogLevel)
  {
    CSingleLock waitLock(critSec);
    CStdString str = levelNames[loglevel];
    str += " ";
    va_list va;

    if (!fd)
      fd = _fsopen("Q:\\xbmc.log", "a+", _SH_DENYWR);
    if (!fd)
      return ;

    SYSTEMTIME time;
    GetLocalTime(&time);
    CStdString strTime;
    strTime.Format("%02.2d-%02.2d-%04.4d %02.2d:%02.2d:%02.2d ", time.wDay, time.wMonth, time.wYear, time.wHour, time.wMinute, time.wSecond);

    va_start(va, format);
    _vsnprintf(tmp, 16384, format, va);
    va_end(va);

    while (1)
    {
      int ilen = strlen(tmp);
      if (ilen <= 0) break;
      if ( tmp[ilen - 1] == '\n' || tmp[ilen - 1] == '\r' || tmp[ilen - 1] == ' ') tmp[ilen - 1] = 0;
      else break;
    }

    strcat(tmp, "\n");
    tmp[16384 - 1] = 0;

    OutputDebugString(tmp);

    fwrite(strTime.c_str(), strTime.size(), 1, fd);
    fwrite(str.c_str(), str.size(), 1, fd);
    fwrite(tmp, strlen(tmp), 1, fd);
    fflush(fd);
  }
#ifdef _DEBUG
  else
  {
    // In debug mode dump everything to devstudio regardless of level
    CSingleLock waitLock(critSec);
    va_list va;

    va_start(va, format);
    _vsnprintf(tmp, 16384, format, va);
    va_end(va);
    strcat(tmp, "\n");
    OutputDebugString(tmp);
  }
#endif
}

void CLog::DebugLog(const char *format, ... )
{
  CSingleLock waitLock(critSec);
  va_list va;

  va_start(va, format);
  _vsnprintf(tmp, 16384, format, va);
  va_end(va);
  strcat(tmp, "\n");
  OutputDebugString(tmp);
}

void CLog::MemDump(BYTE *pData, int length)
{
  DebugLog("MEM_DUMP: Dumping from %x", pData);
  for (int i = 0; i < length; i+=16)
  {
    CStdString strLine;
    strLine.Format("MEM_DUMP: %04x ", i);
    for (int k=0; k < 4; k++)
    {
      for (int j=0; j < 4; j++)
      {
        CStdString strFormat;
        strFormat.Format(" %02x", *pData++);
        strLine += strFormat;
      }
      strLine += " ";
    }
    DebugLog(strLine.c_str());
  }
}

int CLog::GetLevel()
{
  return g_stSettings.m_iLogLevel;
}
