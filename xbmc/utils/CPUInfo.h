/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
class CLinuxResourceCounter;

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
  int    m_id = 0;
  double m_fSpeed = .0;
  double m_fPct = .0;
#ifdef TARGET_POSIX
  unsigned long long m_user = 0LL;
  unsigned long long m_nice = 0LL;
  unsigned long long m_system = 0LL;
  unsigned long long m_io = 0LL;
#elif defined(TARGET_WINDOWS)
  PDH_HCOUNTER m_coreCounter = NULL;
  unsigned long long m_total = 0;
#endif
  unsigned long long m_idle = 0LL;
  std::string m_strVendor;
  std::string m_strModel;
  std::string m_strBogoMips;
  std::string m_strSoC;
  std::string m_strHardware;
  std::string m_strRevision;
  std::string m_strSerial;
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
  std::string& getCPUSoC() { return m_cpuSoC; }
  std::string& getCPUHardware() { return m_cpuHardware; }
  std::string& getCPURevision() { return m_cpuRevision; }
  std::string& getCPUSerial() { return m_cpuSerial; }

  const CoreInfo &GetCoreInfo(int nCoreId);
  bool HasCoreId(int nCoreId) const;

  std::string GetCoresUsageString() const;

  unsigned int GetCPUFeatures() const { return m_cpuFeatures; }

private:
  CCPUInfo(const CCPUInfo&) = delete;
  CCPUInfo& operator=(const CCPUInfo&) = delete;
  bool readProcStat(unsigned long long& user, unsigned long long& nice, unsigned long long& system,
                    unsigned long long& idle, unsigned long long& io);
  void ReadCPUFeatures();
  static bool HasNeon();

#ifdef TARGET_POSIX
  FILE* m_fProcStat;
  FILE* m_fProcTemperature;
  FILE* m_fCPUFreq;
  bool m_cpuInfoForFreq;
#if defined(TARGET_DARWIN)
  CLinuxResourceCounter *m_pResourceCounter;
#endif
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
  std::string  m_cpuSoC;
  std::string  m_cpuHardware;
  std::string  m_cpuRevision;
  std::string  m_cpuSerial;
  int          m_cpuCount;
  unsigned int m_cpuFeatures;

  std::map<int, CoreInfo> m_cores;

};

extern CCPUInfo g_cpuInfo;

