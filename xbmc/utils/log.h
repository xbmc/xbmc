#pragma once
#include "stdstring.h"
using namespace std;

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
