#ifndef __LINUX_RESOURCE_COUNTER__H__
#define __LINUX_RESOURCE_COUNTER__H__

#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>

#include <sys/time.h>
#include <time.h>

class CLinuxResourceCounter
{
public:
  CLinuxResourceCounter();
  virtual ~CLinuxResourceCounter();

  double GetCPUUsage();
  void   Reset();

protected:
  struct rusage  m_usage;
  struct timeval m_tmLastCheck;
  double m_dLastUsage;
};

#endif

