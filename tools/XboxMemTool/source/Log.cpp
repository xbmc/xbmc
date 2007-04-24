
#include "..\stdafx.h"
#include "Log.h"

using namespace XboxMemoryUsage;

CLog g_log;

CLog::CLog()
{
  InitializeCriticalSection(&m_critSection);
}

CLog::~CLog()
{
  DeleteCriticalSection(&m_critSection);
}

void CLog::Log(int loglevel, const char *format)
{
  EnterCriticalSection(&m_critSection);
  
  sprintf(temp, format);

  int len = strlen(temp);
  if (temp[len - 1] != '\n')
  {
    temp[len] = '\n';
    temp[len + 1] = '\0';
  }
  
  OutputDebugString(temp);
  CLogHelper::GetInstance(NULL)->Log(temp);
  
  LeaveCriticalSection(&m_critSection);
}

void CLog::Log(int loglevel, const char *format, const char *s1)
{
  EnterCriticalSection(&m_critSection);
  
  sprintf(temp, format, s1);

  int len = strlen(temp);
  if (temp[len - 1] != '\n')
  {
    temp[len] = '\n';
    temp[len + 1] = '\0';
  }
  
  OutputDebugString(temp);
  CLogHelper::GetInstance(NULL)->Log(temp);
  
  LeaveCriticalSection(&m_critSection);
}

void CLog::Log(int loglevel, const char *format, const char *s1, const char *s2)
{
  EnterCriticalSection(&m_critSection);
  
  sprintf(temp, format, s1, s2);

  int len = strlen(temp);
  if (temp[len - 1] != '\n')
  {
    temp[len] = '\n';
    temp[len + 1] = '\0';
  }
  
  OutputDebugString(temp);
  CLogHelper::GetInstance(NULL)->Log(temp);
  
  LeaveCriticalSection(&m_critSection);
}

