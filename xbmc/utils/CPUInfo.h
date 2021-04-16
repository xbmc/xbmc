/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/SystemClock.h"
#include "utils/Temperature.h"

#include <memory>
#include <string>
#include <vector>

enum CpuFeature
{
  CPU_FEATURE_MMX = 1 << 0,
  CPU_FEATURE_MMX2 = 1 << 1,
  CPU_FEATURE_SSE = 1 << 2,
  CPU_FEATURE_SSE2 = 1 << 3,
  CPU_FEATURE_SSE3 = 1 << 4,
  CPU_FEATURE_SSSE3 = 1 << 5,
  CPU_FEATURE_SSE4 = 1 << 6,
  CPU_FEATURE_SSE42 = 1 << 7,
  CPU_FEATURE_3DNOW = 1 << 8,
  CPU_FEATURE_3DNOWEXT = 1 << 9,
  CPU_FEATURE_ALTIVEC = 1 << 10,
  CPU_FEATURE_NEON = 1 << 11,
};

struct CoreInfo
{
  int m_id = 0;
  double m_usagePercent = 0.0;
  std::size_t m_activeTime = 0;
  std::size_t m_idleTime = 0;
  std::size_t m_totalTime = 0;
};

class CCPUInfo
{
public:
  // Defines to help with calls to CPUID
  const unsigned int CPUID_INFOTYPE_MANUFACTURER = 0x00000000;
  const unsigned int CPUID_INFOTYPE_STANDARD = 0x00000001;
  const unsigned int CPUID_INFOTYPE_EXTENDED_IMPLEMENTED = 0x80000000;
  const unsigned int CPUID_INFOTYPE_EXTENDED = 0x80000001;
  const unsigned int CPUID_INFOTYPE_PROCESSOR_1 = 0x80000002;
  const unsigned int CPUID_INFOTYPE_PROCESSOR_2 = 0x80000003;
  const unsigned int CPUID_INFOTYPE_PROCESSOR_3 = 0x80000004;

  // Standard Features
  // Bitmasks for the values returned by a call to cpuid with eax=0x00000001
  const unsigned int CPUID_00000001_ECX_SSE3 = (1 << 0);
  const unsigned int CPUID_00000001_ECX_SSSE3 = (1 << 9);
  const unsigned int CPUID_00000001_ECX_SSE4 = (1 << 19);
  const unsigned int CPUID_00000001_ECX_SSE42 = (1 << 20);

  const unsigned int CPUID_00000001_EDX_MMX = (1 << 23);
  const unsigned int CPUID_00000001_EDX_SSE = (1 << 25);
  const unsigned int CPUID_00000001_EDX_SSE2 = (1 << 26);

  // Extended Features
  // Bitmasks for the values returned by a call to cpuid with eax=0x80000001
  const unsigned int CPUID_80000001_EDX_MMX2 = (1 << 22);
  const unsigned int CPUID_80000001_EDX_MMX = (1 << 23);
  const unsigned int CPUID_80000001_EDX_3DNOWEXT = (1 << 30);
  const unsigned int CPUID_80000001_EDX_3DNOW = (1 << 31);

  // In milliseconds
  const int MINIMUM_TIME_BETWEEN_READS{500};

  static std::shared_ptr<CCPUInfo> GetCPUInfo();

  virtual bool SupportsCPUUsage() const { return true; }

  virtual int GetUsedPercentage() = 0;
  virtual float GetCPUFrequency() = 0;
  virtual bool GetTemperature(CTemperature& temperature) = 0;

  bool HasCoreId(int coreId) const;
  const CoreInfo GetCoreInfo(int coreId);
  std::string GetCoresUsageString();

  unsigned int GetCPUFeatures() const { return m_cpuFeatures; }
  int GetCPUCount() const { return m_cpuCount; }
  std::string GetCPUModel() { return m_cpuModel; }
  std::string GetCPUBogoMips() { return m_cpuBogoMips; }
  std::string GetCPUSoC() { return m_cpuSoC; }
  std::string GetCPUHardware() { return m_cpuHardware; }
  std::string GetCPURevision() { return m_cpuRevision; }
  std::string GetCPUSerial() { return m_cpuSerial; }

protected:
  CCPUInfo() = default;
  virtual ~CCPUInfo() = default;

  int m_lastUsedPercentage;
  XbmcThreads::EndTime m_nextUsedReadTime;
  std::string m_cpuVendor;
  std::string m_cpuModel;
  std::string m_cpuBogoMips;
  std::string m_cpuSoC;
  std::string m_cpuHardware;
  std::string m_cpuRevision;
  std::string m_cpuSerial;

  double m_usagePercent{0.0};
  std::size_t m_activeTime{0};
  std::size_t m_idleTime{0};
  std::size_t m_totalTime{0};

  int m_cpuCount;
  unsigned int m_cpuFeatures;

  std::vector<CoreInfo> m_cores;
};
