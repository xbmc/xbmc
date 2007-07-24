#pragma once

#ifndef CPUINFO_H
#define CPUINFO_H

#include <stdio.h>
#include <time.h>
#include "Archive.h"
#include "Temperature.h"
#include <string>

class CCPUInfo
{
public:
  CCPUInfo(void);
  ~CCPUInfo();
  
  int getUsedPercentage();
  int getCPUCount() { return m_cpuCount; }
  float getCPUFrequency() { return m_cpuFreq; }
  CTemperature getTemperature();  
  std::string& getCPUModel() { return m_cpuModel; }
  
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
  float m_cpuFreq;
  std::string m_cpuModel;
  int m_cpuCount;
};

extern CCPUInfo g_cpuInfo;

#endif
