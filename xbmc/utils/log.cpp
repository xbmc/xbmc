#include "log.h"

static CLog g_logger;

CLog::CLog()
{
}

CLog::~CLog()
{
}

void CLog::Log(const char *format, ... )
{
	va_list va;

  SYSTEMTIME time;
	GetLocalTime(&time);
  CStdString strTime;
  strTime.Format("%02.2d-%02.2d-%04.4d %02.2d:%02.2d:%02.2d ",time.wDay,time.wMonth,time.wYear,time.wHour,time.wMinute,time.wSecond);
	static char tmp[16384];
	va_start(va, format);
	_vsnprintf(tmp, 16384, format, va);
	va_end(va);
  while (1)
  {
    int ilen=strlen(tmp);
    if (ilen <=0) break;
    if ( tmp[ilen-1] == '\n' || tmp[ilen-1] == '\r' || tmp[ilen-1] ==' ') tmp[ilen-1]=0;
    else break;
  }
  CStdString strLog=tmp;
  strLog+="\n";
  strcat(tmp,"\r\n");
	tmp[16384-1] = 0;
  FILE* fd=fopen("Q:\\xbmc.log","ab+");
  if (!fd) return;
  fwrite(strTime.c_str(),strTime.size(),1,fd);
  fwrite(tmp,strlen(tmp),1,fd);
  fclose(fd);
  OutputDebugString(strLog.c_str());
}

