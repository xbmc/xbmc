#pragma once
#include <stdio.h>

#define LOGDEBUG   0
#define LOGINFO    1
#define LOGNOTICE  2
#define LOGWARNING 3
#define LOGERROR   4
#define LOGSEVERE  5
#define LOGFATAL   6
#define LOGNONE    7

static char levelNames[][8] =
  {
    "DEBUG", "INFO", "NOTICE", "WARNING", "ERROR", "SEVERE", "FATAL", "NONE"
  };

class CLog
{
  static FILE* fd;
public:
  CLog();
  virtual ~CLog(void);
  static void Close();
  static void Log(int loglevel, const char *format, ... );
  static void DebugLog(const char *format, ...);
  static void MemDump(BYTE *pData, int length);
  static int GetLevel();
};
