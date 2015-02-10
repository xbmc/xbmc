#ifndef CPUINFO_H
#define CPUINFO_H

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include <string>
#include <map>
#include "threads/SystemClock.h"

#ifdef TARGET_WINDOWS
// avoid inclusion of <windows.h> and others
typedef void* HANDLE;
typedef HANDLE PDH_HQUERY;
typedef HANDLE PDH_HCOUNTER;
#endif
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
#ifdef TARGET_POSIX
  unsigned long long m_user;
  unsigned long long m_nice;
  unsigned long long m_system;
  unsigned long long m_io;
#elif defined(TARGET_WINDOWS)
  PDH_HCOUNTER m_coreCounter;
  unsigned long long m_total;
#endif
  unsigned long long m_idle;
  std::string m_strVendor;
  std::string m_strModel;
  std::string m_strBogoMips;
  std::string m_strHardware;
  std::string m_strRevision;
  std::string m_strSerial;
#ifdef TARGET_POSIX
  CoreInfo() : m_id(0), m_fSpeed(.0), m_fPct(.0), m_user(0LL), m_nice(0LL), m_system(0LL), m_io(0LL), m_idle(0LL) {}
#elif defined(TARGET_WINDOWS)
  CoreInfo() : m_id(0), m_fSpeed(.0), m_fPct(.0), m_coreCounter(NULL), m_total(0LL), m_idle(0LL) {}
#endif
  bool operator<(const CoreInfo& other) const { return m_id < other.m_id; }
};

class CCPUInfo
{
public:
  CCPUInfo(void);
  ~CCPUInfo();

  int getUsedPercentage();
  int getCPUCount() const { return m_cpuCount; }
  float getCPUFrequency();
  bool getTemperature(CTemperature& temperature);
  std::string& getCPUModel() { return m_cpuModel; }
  std::string& getCPUBogoMips() { return m_cpuBogoMips; }
  std::string& getCPUHardware() { return m_cpuHardware; }
  std::string& getCPURevision() { return m_cpuRevision; }
  std::string& getCPUSerial() { return m_cpuSerial; }

  const CoreInfo &GetCoreInfo(int nCoreId);
  bool HasCoreId(int nCoreId) const;

  std::string GetCoresUsageString() const;

  unsigned int GetCPUFeatures() const { return m_cpuFeatures; }

private:
  bool readProcStat(unsigned long long& user, unsigned long long& nice, unsigned long long& system,
                    unsigned long long& idle, unsigned long long& io);
  void ReadCPUFeatures();
  static bool HasNeon();

#ifdef TARGET_POSIX
  FILE* m_fProcStat;
  FILE* m_fProcTemperature;
  FILE* m_fCPUFreq;
  bool m_cpuInfoForFreq;
#elif defined(TARGET_WINDOWS)
  PDH_HQUERY m_cpuQueryFreq;
  PDH_HQUERY m_cpuQueryLoad;
  PDH_HCOUNTER m_cpuFreqCounter;
#endif

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
