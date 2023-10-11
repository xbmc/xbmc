/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CPUInfoLinux.h"

#include "utils/StringUtils.h"
#include "utils/Temperature.h"

#include "platform/linux/SysfsPath.h"

#include <exception>
#include <fstream>
#include <regex>
#include <sstream>
#include <vector>

#if (defined(__arm__) && defined(HAS_NEON)) || defined(__aarch64__)
#include <asm/hwcap.h>
#include <sys/auxv.h>
#elif defined(__i386__) || defined(__x86_64__)
#include <cpuid.h>
#endif

#include <unistd.h>

namespace
{
enum CpuStates
{
  STATE_USER,
  STATE_NICE,
  STATE_SYSTEM,
  STATE_IDLE,
  STATE_IOWAIT,
  STATE_IRQ,
  STATE_SOFTIRQ,
  STATE_STEAL,
  STATE_GUEST,
  STATE_GUEST_NICE,
  STATE_MAX
};

struct CpuData
{
public:
  std::size_t GetActiveTime() const
  {
    return state[STATE_USER] + state[STATE_NICE] + state[STATE_SYSTEM] + state[STATE_IRQ] +
           state[STATE_SOFTIRQ] + state[STATE_STEAL] + state[STATE_GUEST] + state[STATE_GUEST_NICE];
  }

  std::size_t GetIdleTime() const { return state[STATE_IDLE] + state[STATE_IOWAIT]; }

  std::size_t GetTotalTime() const { return GetActiveTime() + GetIdleTime(); }

  std::string cpu;
  std::size_t state[STATE_MAX];
};
} // namespace

std::shared_ptr<CCPUInfo> CCPUInfo::GetCPUInfo()
{
  return std::make_shared<CCPUInfoLinux>();
}

CCPUInfoLinux::CCPUInfoLinux()
{
  CSysfsPath machinePath{"/sys/bus/soc/devices/soc0/machine"};
  if (machinePath.Exists())
    m_cpuHardware = machinePath.Get<std::string>().value_or("");

  CSysfsPath familyPath{"/sys/bus/soc/devices/soc0/family"};
  if (familyPath.Exists())
    m_cpuSoC = familyPath.Get<std::string>().value_or("");

  CSysfsPath socPath{"/sys/bus/soc/devices/soc0/soc_id"};
  if (socPath.Exists())
    m_cpuSoC += " " + socPath.Get<std::string>().value_or("");

  CSysfsPath revisionPath{"/sys/bus/soc/devices/soc0/revision"};
  if (revisionPath.Exists())
    m_cpuRevision = revisionPath.Get<std::string>().value_or("");

  CSysfsPath serialPath{"/sys/bus/soc/devices/soc0/serial_number"};
  if (serialPath.Exists())
    m_cpuSerial = serialPath.Get<std::string>().value_or("");

  const std::string freqStr{"/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"};
  CSysfsPath freqPath{freqStr};
  if (freqPath.Exists())
    m_freqPath = freqStr;

  const std::array<std::string, 5> modules = {
      "coretemp",
      "k10temp",
      "scpi_sensors",
      "imx_thermal_zone",
      "cpu_thermal",
  };

  for (int i = 0; i < 20; i++)
  {
    CSysfsPath path{"/sys/class/hwmon/hwmon" + std::to_string(i) + "/name"};
    if (!path.Exists())
      continue;

    auto name = path.Get<std::string>();

    if (!name.has_value())
      continue;

    for (const auto& module : modules)
    {
      if (module == name)
      {
        std::string tempStr{"/sys/class/hwmon/hwmon" + std::to_string(i) + "/temp1_input"};
        CSysfsPath tempPath{tempStr};
        if (!tempPath.Exists())
          continue;

        m_tempPath = tempStr;
        break;
      }
    }

    if (!m_tempPath.empty())
      break;
  }

  m_cpuCount = sysconf(_SC_NPROCESSORS_ONLN);

  for (int core = 0; core < m_cpuCount; core++)
  {
    CoreInfo coreInfo;
    coreInfo.m_id = core;
    m_cores.emplace_back(coreInfo);
  }

#if defined(__i386__) || defined(__x86_64__)
  unsigned int eax;
  unsigned int ebx;
  unsigned int ecx;
  unsigned int edx;

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

  if (__get_cpuid(CPUID_INFOTYPE_STANDARD, &eax, &eax, &ecx, &edx))
  {
    if (edx & CPUID_00000001_EDX_MMX)
      m_cpuFeatures |= CPU_FEATURE_MMX;

    if (edx & CPUID_00000001_EDX_SSE)
      m_cpuFeatures |= CPU_FEATURE_SSE;

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
#else
  std::ifstream cpuinfo("/proc/cpuinfo");
  std::regex re(".*: (.*)$");

  for (std::string line; std::getline(cpuinfo, line);)
  {
    std::smatch match;

    if (std::regex_match(line, match, re))
    {
      if (match.size() == 2)
      {
        std::ssub_match value = match[1];

        if (line.find("model name") != std::string::npos)
        {
          if (m_cpuModel.empty())
            m_cpuModel = value.str();
        }

        if (line.find("BogoMIPS") != std::string::npos)
        {
          if (m_cpuBogoMips.empty())
            m_cpuBogoMips = value.str();
        }

        if (line.find("Hardware") != std::string::npos)
        {
          if (m_cpuHardware.empty())
            m_cpuHardware = value.str();
        }

        if (line.find("Serial") != std::string::npos)
        {
          if (m_cpuSerial.empty())
            m_cpuSerial = value.str();
        }

        if (line.find("Revision") != std::string::npos)
        {
          if (m_cpuRevision.empty())
            m_cpuRevision = value.str();
        }
      }
    }
  }
#endif

  m_cpuModel = m_cpuModel.substr(0, m_cpuModel.find(char(0))); // remove extra null terminations

#if defined(HAS_NEON) && defined(__arm__)
  if (getauxval(AT_HWCAP) & HWCAP_NEON)
    m_cpuFeatures |= CPU_FEATURE_NEON;
#endif

#if defined(HAS_NEON) && defined(__aarch64__)
  if (getauxval(AT_HWCAP) & HWCAP_ASIMD)
    m_cpuFeatures |= CPU_FEATURE_NEON;
#endif

  // Set MMX2 when SSE is present as SSE is a superset of MMX2 and Intel doesn't set the MMX2 cap
  if (m_cpuFeatures & CPU_FEATURE_SSE)
    m_cpuFeatures |= CPU_FEATURE_MMX2;
}

int CCPUInfoLinux::GetUsedPercentage()
{
  if (!m_nextUsedReadTime.IsTimePast())
    return m_lastUsedPercentage;

  std::vector<CpuData> cpuData;

  std::ifstream infile("/proc/stat");

  for (std::string line; std::getline(infile, line);)
  {
    if (line.find("cpu") != std::string::npos)
    {
      std::istringstream ss(line);
      CpuData info;

      ss >> info.cpu;

      for (int i = 0; i < STATE_MAX; i++)
      {
        ss >> info.state[i];
      }

      cpuData.emplace_back(info);
    }
  }

  auto activeTime = cpuData.front().GetActiveTime() - m_activeTime;
  auto idleTime = cpuData.front().GetIdleTime() - m_idleTime;
  auto totalTime = cpuData.front().GetTotalTime() - m_totalTime;

  m_activeTime += activeTime;
  m_idleTime += idleTime;
  m_totalTime += totalTime;

  m_lastUsedPercentage = activeTime * 100.0f / totalTime;
  m_nextUsedReadTime.Set(MINIMUM_TIME_BETWEEN_READS);

  cpuData.erase(cpuData.begin());

  for (std::size_t core = 0; core < cpuData.size(); core++)
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

float CCPUInfoLinux::GetCPUFrequency()
{
  if (m_freqPath.empty())
    return 0;

  auto freq = CSysfsPath(m_freqPath).Get<float>();
  return freq.has_value() ? *freq / 1000.0f : 0.0f;
}

bool CCPUInfoLinux::GetTemperature(CTemperature& temperature)
{
  if (CheckUserTemperatureCommand(temperature))
    return true;

  if (m_tempPath.empty())
    return false;

  auto temp = CSysfsPath(m_tempPath).Get<double>();
  if (!temp.has_value())
    return false;

  double value = *temp / 1000.0;

  temperature = CTemperature::CreateFromCelsius(value);
  temperature.SetValid(true);

  return true;
}
