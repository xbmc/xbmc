#include "log.h"

static CLog g_logger;

CLog::CLog()
{
}

CLog::~CLog()
{
}

// put here so debug and normal logger can share it
static char tmp[16384];

FILE* CLog::fd = NULL;

void CLog::Close()
{
  if (fd)
  {
    fclose(fd);
    fd=NULL;
  }
}
void CLog::Log(const char *format, ... )
{
	va_list va;

	if (!fd)
		fd=fopen("Q:\\xbmc.log","a+");
	if (!fd)
		return;

  SYSTEMTIME time;
	GetLocalTime(&time);
  CStdString strTime;
  strTime.Format("%02.2d-%02.2d-%04.4d %02.2d:%02.2d:%02.2d ",time.wDay,time.wMonth,time.wYear,time.wHour,time.wMinute,time.wSecond);

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

	strcat(tmp,"\n");
	tmp[16384-1] = 0;

	OutputDebugString(tmp);

  fwrite(strTime.c_str(),strTime.size(),1,fd);
  fwrite(tmp,strlen(tmp),1,fd);
  fflush(fd);
}

void CLog::DebugLog(const char *format, ... )
{
	va_list va;

	va_start(va, format);
	_vsnprintf(tmp, 16384, format, va);
	va_end(va);
	strcat(tmp, "\n");
	OutputDebugString(tmp);
}
