#pragma once

#ifndef CPUINFO_H
#define CPUINFO_H

#include <stdio.h>
#include <time.h>
#include "Archive.h"
#include "Temperature.h"

class CCPUInfo
{
public:
  CCPUInfo(void);
  ~CCPUInfo();
  
  int getUsedPercentage();
  CTemperature getTemperature();  
  
private:
  bool readProcStat(unsigned long long& user, unsigned long long& nice, unsigned long long& system,
    unsigned long long& idle);
  
  FILE* m_fProcStat;
  FILE* m_fProcTemperature;
  
  unsigned long long m_userTicks;
  unsigned long long m_niceTicks;
  unsigned long long m_systemTicks;
  unsigned long long m_idleTicks;
  
  int m_lastUsedPercentage;
  time_t m_lastReadTime;  
};

extern CCPUInfo g_cpuInfo;

#endif
