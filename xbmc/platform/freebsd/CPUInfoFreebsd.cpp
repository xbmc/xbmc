/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CPUInfoFreebsd.h"

#include "utils/Temperature.h"
#include "utils/log.h"

#include <array>
#include <vector>

// clang-format off
/* sys/types.h must be included early, esp. before sysy/systl.h, otherwise:
   /usr/include/sys/sysctl.h:1117:25: error: unknown type name 'u_int' */

#include <sys/types.h>
// clang-format on

#if defined(__i386__) || defined(__x86_64__)
#include <cpuid.h>
#elif __has_include(<sys/auxv.h>)
#include <sys/auxv.h>
#endif

#include <sys/resource.h>
#include <sys/sysctl.h>

namespace
{

struct CpuData
{
public:
  std::size_t GetActiveTime() const { return state[CP_USER] + state[CP_NICE] + state[CP_SYS]; }

  std::size_t GetIdleTime() const { return state[CP_INTR] + state[CP_IDLE]; }

  std::size_t GetTotalTime() const { return GetActiveTime() + GetIdleTime(); }

  std::size_t state[CPUSTATES];
};

} // namespace

std::shared_ptr<CCPUInfo> CCPUInfo::GetCPUInfo()
{
  return std::make_shared<CCPUInfoFreebsd>();
}

CCPUInfoFreebsd::CCPUInfoFreebsd()
{
  int count = 0;
  size_t countLength = sizeof(count);
  if (sysctlbyname("hw.ncpu", &count, &countLength, nullptr, 0) == 0)
    m_cpuCount = count;
  else
    m_cpuCount = 1;

  std::array<char, 512> cpuModel;
  size_t length = cpuModel.size();
  if (sysctlbyname("hw.model", cpuModel.data(), &length, nullptr, 0) == 0)
    m_cpuModel = cpuModel.data();

  for (int i = 0; i < m_cpuCount; i++)
  {
    CoreInfo core;
    core.m_id = i;
    m_cores.emplace_back(core);
  }
#if defined(__i386__) || defined(__x86_64__)
  uint32_t eax, ebx, ecx, edx;

  m_cpuVendor.clear();

  if (__get_cpuid(CPUID_INFOTYPE_MANUFACTURER, &eax, &ebx, &ecx, &edx))
  {
    m_cpuVendor.append(reinterpret_cast<const char*>(&ebx), 4);
    m_cpuVendor.append(reinterpret_cast<const char*>(&edx), 4);
    m_cpuVendor.append(reinterpret_cast<const char*>(&ecx), 4);
  }

  if (__get_cpuid(CPUID_INFOTYPE_EXTENDED_IMPLEMENTED, &eax, &ebx, &ecx, &edx))
  {
    if (eax >= CPUID_INFOTYPE_PROCESSOR_3)
    {
      m_cpuModel.clear();

      if (__get_cpuid(CPUID_INFOTYPE_PROCESSOR_1, &eax, &ebx, &ecx, &edx))
      {
        m_cpuModel.append(reinterpret_cast<const char*>(&eax), 4);
        m_cpuModel.append(reinterpret_cast<const char*>(&ebx), 4);
        m_cpuModel.append(reinterpret_cast<const char*>(&ecx), 4);
        m_cpuModel.append(reinterpret_cast<const char*>(&edx), 4);
      }

      if (__get_cpuid(CPUID_INFOTYPE_PROCESSOR_2, &eax, &ebx, &ecx, &edx))
      {
        m_cpuModel.append(reinterpret_cast<const char*>(&eax), 4);
        m_cpuModel.append(reinterpret_cast<const char*>(&ebx), 4);
        m_cpuModel.append(reinterpret_cast<const char*>(&ecx), 4);
        m_cpuModel.append(reinterpret_cast<const char*>(&edx), 4);
      }

      if (__get_cpuid(CPUID_INFOTYPE_PROCESSOR_3, &eax, &ebx, &ecx, &edx))
      {
        m_cpuModel.append(reinterpret_cast<const char*>(&eax), 4);
        m_cpuModel.append(reinterpret_cast<const char*>(&ebx), 4);
        m_cpuModel.append(reinterpret_cast<const char*>(&ecx), 4);
        m_cpuModel.append(reinterpret_cast<const char*>(&edx), 4);
      }
    }
  }

  m_cpuModel = m_cpuModel.substr(0, m_cpuModel.find(char(0))); // remove extra null terminations

  if (__get_cpuid(CPUID_INFOTYPE_STANDARD, &eax, &eax, &ecx, &edx))
  {
    if (edx & CPUID_00000001_EDX_MMX)
      m_cpuFeatures |= CPU_FEATURE_MMX;

    // Set MMX2 when SSE is present as SSE is a superset of MMX2 and Intel doesn't set the MMX2 cap
    if (edx & CPUID_00000001_EDX_SSE)
      m_cpuFeatures |= (CPU_FEATURE_SSE | CPU_FEATURE_MMX2);

    if (edx & CPUID_00000001_EDX_SSE2)
      m_cpuFeatures |= CPU_FEATURE_SSE2;

    if (ecx & CPUID_00000001_ECX_SSE3)
      m_cpuFeatures |= CPU_FEATURE_SSE3;

    if (ecx & CPUID_00000001_ECX_SSSE3)
      m_cpuFeatures |= CPU_FEATURE_SSSE3;

    if (ecx & CPUID_00000001_ECX_SSE4)
      m_cpuFeatures |= CPU_FEATURE_SSE4;

    if (ecx & CPUID_00000001_ECX_SSE42)
      m_cpuFeatures |= CPU_FEATURE_SSE42;
  }

  if (__get_cpuid(CPUID_INFOTYPE_EXTENDED_IMPLEMENTED, &eax, &eax, &ecx, &edx))
  {
    if (eax >= CPUID_INFOTYPE_EXTENDED)
    {
      if (edx & CPUID_80000001_EDX_MMX)
        m_cpuFeatures |= CPU_FEATURE_MMX;

      if (edx & CPUID_80000001_EDX_MMX2)
        m_cpuFeatures |= CPU_FEATURE_MMX2;

      if (edx & CPUID_80000001_EDX_3DNOW)
        m_cpuFeatures |= CPU_FEATURE_3DNOW;

      if (edx & CPUID_80000001_EDX_3DNOWEXT)
        m_cpuFeatures |= CPU_FEATURE_3DNOWEXT;
    }
  }
#endif

#if defined(HAS_NEON)
#if defined(__ARM_NEON)
  m_cpuFeatures |= CPU_FEATURE_NEON;
#elif __has_include(<sys/auxv.h>)
  unsigned long hwcap = 0;
  elf_aux_info(AT_HWCAP, &hwcap, sizeof(hwcap));
  if (hwcap & HWCAP_NEON)
    m_cpuFeatures |= CPU_FEATURE_NEON;
#endif
#endif
}

int CCPUInfoFreebsd::GetUsedPercentage()
{
  if (!m_nextUsedReadTime.IsTimePast())
    return m_lastUsedPercentage;

  size_t len = sizeof(long);

  if (sysctlbyname("kern.cp_times", nullptr, &len, nullptr, 0) != 0)
    return false;

  std::vector<long> cptimes(len);
  size_t cptimesLength = cptimes.size();
  if (sysctlbyname("kern.cp_times", cptimes.data(), &cptimesLength, nullptr, 0) != 0)
    return false;

  size_t activeTime{0};
  size_t idleTime{0};
  size_t totalTime{0};

  std::vector<CpuData> cpuData;

  for (int i = 0; i < m_cpuCount; i++)
  {
    CpuData info;

    for (size_t state = 0; state < CPUSTATES; state++)
    {
      info.state[state] = cptimes[i * CPUSTATES + state];
    }

    activeTime += info.GetActiveTime();
    idleTime += info.GetIdleTime();
    totalTime += info.GetTotalTime();

    cpuData.emplace_back(info);
  }

  activeTime -= m_activeTime;
  idleTime -= m_idleTime;
  totalTime -= m_totalTime;

  m_activeTime += activeTime;
  m_idleTime += idleTime;
  m_totalTime += totalTime;

  m_lastUsedPercentage = activeTime * 100.0f / totalTime;
  m_nextUsedReadTime.Set(MINIMUM_TIME_BETWEEN_READS);

  for (size_t core = 0; core < cpuData.size(); core++)
  {
    auto activeTime = cpuData[core].GetActiveTime() - m_cores[core].m_activeTime;
    auto idleTime = cpuData[core].GetIdleTime() - m_cores[core].m_idleTime;
    auto totalTime = cpuData[core].GetTotalTime() - m_cores[core].m_totalTime;

    m_cores[core].m_usagePercent = activeTime * 100.0 / totalTime;

    m_cores[core].m_activeTime += activeTime;
    m_cores[core].m_idleTime += idleTime;
    m_cores[core].m_totalTime += totalTime;
  }

  return static_cast<int>(m_lastUsedPercentage);
}

float CCPUInfoFreebsd::GetCPUFrequency()
{
  int hz = 0;
  size_t len = sizeof(hz);
  if (sysctlbyname("dev.cpu.0.freq", &hz, &len, nullptr, 0) != 0)
    hz = 0;

  return static_cast<float>(hz);
}


bool CCPUInfoFreebsd::GetTemperature(CTemperature& temperature)
{
  if (CheckUserTemperatureCommand(temperature))
    return true;

  int value;
  size_t len = sizeof(value);

  /* Temperature is in Kelvin * 10 */
  if (sysctlbyname("dev.cpu.0.temperature", &value, &len, nullptr, 0) != 0)
    return false;

  temperature = CTemperature::CreateFromKelvin(static_cast<double>(value) / 10.0);
  temperature.SetValid(true);

  return true;
}
