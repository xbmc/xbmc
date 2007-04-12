
#pragma once

#include <stdio.h>
#include "FormManager.h"

#define LOG_DEBUG   0
#define LOG_INFO    1
#define LOG_ERROR   2
#define LOG_FATAL   3

namespace XboxMemoryUsage
{
  public __gc class CLogHelper
  {
  public:
    
    static CLogHelper* GetInstance(FormManager* formManager)
    {
      if (instance == NULL)
      {
        instance = new CLogHelper();
        instance->m_formManager = formManager;
      }
      return instance;
    }
    
    void Log(String* message)
    {
      if (m_formManager != NULL)
      {
        m_formManager->Log(message);
      }
    }
    
  private:
    static CLogHelper* instance;
    FormManager* m_formManager;
  };
}

class CLog
{
public:
  CLog();
  virtual ~CLog();
  //void Log(int loglevel, const char *format, ...);
  void Log(int loglevel, const char *format);
  void Log(int loglevel, const char *format, const char *s1);
  void Log(int loglevel, const char *format, const char *s1, const char *s2);
  
private:
  char temp[16 * 1024];
  CRITICAL_SECTION m_critSection;
};

extern CLog g_log;