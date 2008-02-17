#pragma once

#ifndef CPUINFO_H
#define CPUINFO_H

#include <stdio.h>
#include <time.h>
#include "Archive.h"
#include "Temperature.h"
#include <string>
#include <map>

struct CoreInfo
{
  int    m_id;
  double m_fSpeed;
  double m_fPct;
  unsigned long long m_user; 
  unsigned long long m_nice;
  unsigned long long m_system;
  unsigned long long m_idle;
  struct timeval m_lastSample;
  CStdString m_strVendor;
  CStdString m_strModel;
  CoreInfo() : m_id(0), m_fSpeed(.0), m_fPct(.0), m_user(0LL), m_nice(0LL), m_system(0LL), m_idle(0LL) 
  { 
    memset(&m_lastSample,0,sizeof(struct timeval));
  }
};

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
  
  const CoreInfo &GetCoreInfo(int nCoreId);
  bool HasCoreId(int nCoreId) const;

  CStdString GetCoresUsageString() const;

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

  std::map<int, CoreInfo> m_cores;
};

extern CCPUInfo g_cpuInfo;

#endif
