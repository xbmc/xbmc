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

#include <sys/resource.h>
#include <sys/sysctl.h>
#include <sys/types.h>

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

    for (int state = 0; state < CPUSTATES; state++)
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

    m_cores[core].m_usagePercent = activeTime * 100.0f / totalTime;

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
