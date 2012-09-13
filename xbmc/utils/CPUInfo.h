#ifndef CPUINFO_H
#define CPUINFO_H

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <time.h>
#include "Archive.h"
#include <string>
#include <map>
#include "threads/SystemClock.h"

class CTemperature;

#define CPU_FEATURE_MMX      1 << 0
#define CPU_FEATURE_MMX2     1 << 1
#define CPU_FEATURE_SSE      1 << 2
#define CPU_FEATURE_SSE2     1 << 3
#define CPU_FEATURE_SSE3     1 << 4
#define CPU_FEATURE_SSSE3    1 << 5
#define CPU_FEATURE_SSE4     1 << 6
#define CPU_FEATURE_SSE42    1 << 7
#define CPU_FEATURE_3DNOW    1 << 8
#define CPU_FEATURE_3DNOWEXT 1 << 9
#define CPU_FEATURE_ALTIVEC  1 << 10
#define CPU_FEATURE_NEON     1 << 11

struct CoreInfo
{
  int    m_id;
  double m_fSpeed;
  double m_fPct;
  unsigned long long m_user;
  unsigned long long m_nice;
  unsigned long long m_system;
  unsigned long long m_idle;
  unsigned long long m_io;
  CStdString m_strVendor;
  CStdString m_strModel;
  CStdString m_strBogoMips;
  CStdString m_strHardware;
  CStdString m_strRevision;
  CStdString m_strSerial;
  CoreInfo() : m_id(0), m_fSpeed(.0), m_fPct(.0), m_user(0LL), m_nice(0LL), m_system(0LL), m_idle(0LL), m_io(0LL) {}
};

class CCPUInfo
{
public:
  CCPUInfo(void);
  ~CCPUInfo();

  int getUsedPercentage();
  int getCPUCount() { return m_cpuCount; }
  float getCPUFrequency();
  bool getTemperature(CTemperature& temperature);
  std::string& getCPUModel() { return m_cpuModel; }
  std::string& getCPUBogoMips() { return m_cpuBogoMips; }
  std::string& getCPUHardware() { return m_cpuHardware; }
  std::string& getCPURevision() { return m_cpuRevision; }
  std::string& getCPUSerial() { return m_cpuSerial; }

  const CoreInfo &GetCoreInfo(int nCoreId);
  bool HasCoreId(int nCoreId) const;

  CStdString GetCoresUsageString() const;

  unsigned int GetCPUFeatures() { return m_cpuFeatures; }

private:
  bool readProcStat(unsigned long long& user, unsigned long long& nice, unsigned long long& system,
    unsigned long long& idle, unsigned long long& io);
  void ReadCPUFeatures();
  bool HasNeon();

  FILE* m_fProcStat;
  FILE* m_fProcTemperature;
  FILE* m_fCPUInfo;

  unsigned long long m_userTicks;
  unsigned long long m_niceTicks;
  unsigned long long m_systemTicks;
  unsigned long long m_idleTicks;
  unsigned long long m_ioTicks;

  int          m_lastUsedPercentage;
  XbmcThreads::EndTime m_nextUsedReadTime; 
  std::string  m_cpuModel;
  std::string  m_cpuBogoMips;
  std::string  m_cpuHardware;
  std::string  m_cpuRevision;
  std::string  m_cpuSerial;
  int          m_cpuCount;
  unsigned int m_cpuFeatures;

  std::map<int, CoreInfo> m_cores;

};

extern CCPUInfo g_cpuInfo;

#endif
