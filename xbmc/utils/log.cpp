
#include "../stdafx.h"
#include "log.h"
#include <share.h>
#include "criticalsection.h"
#include "singlelock.h"
#include "StdString.h"
#include "../Settings.h"
#include "../util.h"

FILE* CLog::fd = NULL;

static CCriticalSection critSec;

CLog::CLog()
{}

CLog::~CLog()
{}

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
    if (!fd)
    {
      CStdString LogFile;
      if (g_settings.QuickXMLLoad("logpath"))
      {
        CStdString strLogPath = g_stSettings.m_szlogpath;        
        if (!strLogPath.IsEmpty() && CUtil::IsHD(strLogPath))
        {
          if( !CUtil::HasSlashAtEnd(strLogPath) )
            strLogPath += "\\";
            
          if( CreateDirectory(strLogPath.c_str(), NULL) )
            LogFile.Format("%sxbmc.log", strLogPath.c_str());
        }
      }

      if( LogFile.length() == 0 )
        LogFile = "Q:\\xbmc.log";

      fd = _fsopen(LogFile, "a+", _SH_DENYWR);
    }
      
    if (!fd)
      return ;


    SYSTEMTIME time;
    GetLocalTime(&time);

    MEMORYSTATUS stat;
    GlobalMemoryStatus(&stat);

    CStdString strPrefix, strData;
    
    strPrefix.Format("%02.2d:%02.2d:%02.2d M:%9u %7s: ", time.wHour, time.wMinute, time.wSecond, stat.dwAvailPhys, levelNames[loglevel]);

    strData.reserve(16384);
    va_list va;
    va_start(va, format);
    strData.FormatV(format,va);    
    va_end(va);
    

    int length = 0;
    while ( length != strData.length() )
    {
      length = strData.length();
      strData.TrimRight(" ");
      strData.TrimRight('\n');      
      strData.TrimRight("\r");
    }

    OutputDebugString(strData.c_str());
    OutputDebugString("\n");

    /* fixup newline alignment, number of spaces should equal prefix length */
    strData.Replace("\n", "\n                             ");
    strData += "\n";
       
    fwrite(strPrefix.c_str(), strPrefix.size(), 1, fd);
    fwrite(strData.c_str(), strData.size(), 1, fd);
    fflush(fd);
  }
#ifdef _DEBUG
  else
  {
    // In debug mode dump everything to devstudio regardless of level
    CSingleLock waitLock(critSec);
    CStdString strData;
    strData.reserve(16384);

    va_list va;
    va_start(va, format);
    strData.FormatV(format, va);    
    va_end(va);
    
    OutputDebugString(strData.c_str());
    if( strData.Right(1) != "\n" )
      OutputDebugString("\n");
    
  }
#endif
}

void CLog::DebugLog(const char *format, ... )
{
  CSingleLock waitLock(critSec);

  CStdString strData;
  strData.reserve(16384);

  va_list va;
  va_start(va, format);
  strData.FormatV(format, va);    
  va_end(va);
  
  OutputDebugString(strData.c_str());
  if( strData.Right(1) != "\n" )
    OutputDebugString("\n");
}

void CLog::DebugLogMemory()
{
  CSingleLock waitLock(critSec);
  MEMORYSTATUS stat;
  CStdString strData;

  GlobalMemoryStatus(&stat);
  strData.Format("%i bytes free\n", stat.dwAvailPhys);
  OutputDebugString(strData.c_str());
}

void CLog::MemDump(BYTE *pData, int length)
{
  DebugLog("MEM_DUMP: Dumping from %x", pData);
  for (int i = 0; i < length; i+=16)
  {
    CStdString strLine;
    strLine.Format("MEM_DUMP: %04x ", i);
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
    DebugLog(strLine.c_str());
  }
}

int CLog::GetLevel()
{
  return g_stSettings.m_iLogLevel;
}
