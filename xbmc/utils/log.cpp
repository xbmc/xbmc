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
  strcat(tmp,"\r\n");
	tmp[16384-1] = 0;
  FILE* fd=fopen("Q:\\xbmc.log","a+");
  if (!fd) return;
  fwrite(strTime.c_str(),strTime.size(),1,fd);
  fwrite(tmp,strlen(tmp),1,fd);
  fclose(fd);
	OutputDebugString(tmp);
}

