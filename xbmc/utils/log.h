#pragma once
#include <stdio.h>

class CLog
{
	static FILE* fd;
public:
  CLog();
  virtual ~CLog(void);
  static void Close();
  static void Log(const char *format, ... );
	static void DebugLog(const char *format, ...);
};
