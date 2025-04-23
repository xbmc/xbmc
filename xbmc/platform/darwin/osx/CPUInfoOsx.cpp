/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CPUInfoOsx.h"

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/Temperature.h"

#include "platform/posix/PosixResourceCounter.h"

#include <array>
#include <string>

#include <smctemp.h>
#include <sys/sysctl.h>
#include <sys/types.h>

std::shared_ptr<CCPUInfo> CCPUInfo::GetCPUInfo()
{
  return std::make_shared<CCPUInfoOsx>();
}

CCPUInfoOsx::CCPUInfoOsx() : m_resourceCounter(new CPosixResourceCounter())
{
  int count = 0;
  size_t countLength = sizeof(count);
  if (sysctlbyname("hw.activecpu", &count, &countLength, nullptr, 0) == 0)
    m_cpuCount = count;
  else
    m_cpuCount = 1;

  std::array<char, 512> buffer;
  size_t bufferLength = buffer.size();
  if (sysctlbyname("machdep.cpu.brand_string", buffer.data(), &bufferLength, nullptr, 0) == 0)
    m_cpuModel = buffer.data();

  m_cpuModel = m_cpuModel.substr(0, m_cpuModel.find(char(0))); // remove extra null terminations

  buffer = {};
  if (sysctlbyname("machdep.cpu.vendor", buffer.data(), &bufferLength, nullptr, 0) == 0)
    m_cpuVendor = buffer.data();

  for (int core = 0; core < m_cpuCount; core++)
  {
    CoreInfo coreInfo;
    coreInfo.m_id = core;
    m_cores.emplace_back(coreInfo);
  }

  buffer = {};
  if (sysctlbyname("machdep.cpu.features", buffer.data(), &bufferLength, nullptr, 0) == 0)
  {
    std::string features = buffer.data();

    if (features.find("MMX") != std::string::npos)
      m_cpuFeatures |= CPU_FEATURE_MMX;

    if (features.find("MMXEXT") != std::string::npos)
      m_cpuFeatures |= CPU_FEATURE_MMX2;

    if (features.find("SSE") != std::string::npos)
      m_cpuFeatures |= CPU_FEATURE_SSE;

    if (features.find("SSE2") != std::string::npos)
      m_cpuFeatures |= CPU_FEATURE_SSE2;

    if (features.find("SSE3") != std::string::npos)
      m_cpuFeatures |= CPU_FEATURE_SSE3;

    if (features.find("SSSE3") != std::string::npos)
      m_cpuFeatures |= CPU_FEATURE_SSSE3;

    if (features.find("SSE4.1") != std::string::npos)
      m_cpuFeatures |= CPU_FEATURE_SSE4;

    if (features.find("SSE4.2") != std::string::npos)
      m_cpuFeatures |= CPU_FEATURE_SSE42;

    if (features.find("3DNOW") != std::string::npos)
      m_cpuFeatures |= CPU_FEATURE_3DNOW;

    if (features.find("3DNOWEXT") != std::string::npos)
      m_cpuFeatures |= CPU_FEATURE_3DNOWEXT;
  }
  else
    m_cpuFeatures |= CPU_FEATURE_MMX;

  // Set MMX2 when SSE is present as SSE is a superset of MMX2 and Intel doesn't set the MMX2 cap
  if (m_cpuFeatures & CPU_FEATURE_SSE)
    m_cpuFeatures |= CPU_FEATURE_MMX2;
}

int CCPUInfoOsx::GetUsedPercentage()
{
  if (!m_nextUsedReadTime.IsTimePast())
    return m_lastUsedPercentage;

  m_lastUsedPercentage = m_resourceCounter->GetCPUUsage();
  m_nextUsedReadTime.Set(MINIMUM_TIME_BETWEEN_READS);

  return m_lastUsedPercentage;
}

float CCPUInfoOsx::GetCPUFrequency()
{
  // Get CPU frequency, scaled to MHz.
  long long hz = 0;
  size_t len = sizeof(hz);
  if (sysctlbyname("hw.cpufrequency", &hz, &len, NULL, 0) == -1)
    return 0.f;

  return hz / 1000000.0;
}

bool CCPUInfoOsx::GetTemperature(CTemperature& temperature)
{
  if (!CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_cpuTempCmd.empty())
  {
    return CheckUserTemperatureCommand(temperature);
  }

  // isFailsoft lets smctemp average temperatures across reads for some models that
  // aren't as robust in their readings
  const bool isFailSoft = true;
  smctemp::SmcTemp smcTemp = smctemp::SmcTemp(isFailSoft);
  const double value = smcTemp.GetCpuTemp();
  if (value <= 0.0)
  {
    temperature.SetValid(false);
    return false;
  }

  temperature = CTemperature::CreateFromCelsius(value);
  return true;
}
