/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CPUInfoDarwinEmbed.h"

#include "utils/Temperature.h"

#include "platform/posix/PosixResourceCounter.h"

#include <array>
#include <string>

#include <mach-o/arch.h>
#include <sys/sysctl.h>
#include <sys/types.h>

std::shared_ptr<CCPUInfo> CCPUInfo::GetCPUInfo()
{
  return std::make_shared<CCPUInfoDarwinEmbed>();
}

CCPUInfoDarwinEmbed::CCPUInfoDarwinEmbed() : m_resourceCounter(new CPosixResourceCounter())
{
  int count = 0;
  size_t countLength = sizeof(count);
  if (sysctlbyname("hw.activecpu", &count, &countLength, nullptr, 0) == 0)
    m_cpuCount = count;
  else
    m_cpuCount = 1;

  std::array<char, 512> buffer;
  size_t length = buffer.size();
  if (sysctlbyname("machdep.cpu.brand_string", buffer.data(), &length, nullptr, 0) == 0)
    m_cpuModel = buffer.data();

  buffer = {};
  if (sysctlbyname("machdep.cpu.vendor", buffer.data(), &length, nullptr, 0) == 0)
    m_cpuVendor = buffer.data();

  for (int core = 0; core < m_cpuCount; core++)
  {
    CoreInfo coreInfo;
    coreInfo.m_id = core;
    m_cores.emplace_back(coreInfo);
  }

  m_cpuFeatures |= CPU_FEATURE_NEON;
}

int CCPUInfoDarwinEmbed::GetUsedPercentage()
{
  if (!m_nextUsedReadTime.IsTimePast())
    return m_lastUsedPercentage;

  m_lastUsedPercentage = m_resourceCounter->GetCPUUsage();
  m_nextUsedReadTime.Set(MINIMUM_TIME_BETWEEN_READS);

  return m_lastUsedPercentage;
}

float CCPUInfoDarwinEmbed::GetCPUFrequency()
{
  // Get CPU frequency, scaled to MHz.
  long long hz = 0;
  size_t len = sizeof(hz);
  if (sysctlbyname("hw.cpufrequency", &hz, &len, nullptr, 0) < 0)
    return 0.f;

  return hz / 1000000.0;
}
