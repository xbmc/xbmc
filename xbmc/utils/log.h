#pragma once
#include "stdstring.h"
using namespace std;

class CLog
{
public:
  CLog();
  virtual ~CLog(void);
  static void Log(const char *format, ... );
};
