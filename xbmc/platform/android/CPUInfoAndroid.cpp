/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CPUInfoAndroid.h"

#include "URL.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <cpu-features.h>
#include <dirent.h>
#include <sys/resource.h>
#include <unistd.h>

std::shared_ptr<CCPUInfo> CCPUInfo::GetCPUInfo()
{
  return std::make_shared<CCPUInfoAndroid>();
}

CCPUInfoAndroid::CCPUInfoAndroid()
  : m_posixFile(std::make_unique<CPosixFile>()),
    m_clockTicks(sysconf(_SC_CLK_TCK))
{
  if (m_posixFile && m_posixFile->Open(CURL("/proc/cpuinfo")))
  {
    std::array<char, 2048> buffer = {};

    if (0 < m_posixFile->Read(buffer.data(), buffer.size()))
    {
      for (const auto& line : StringUtils::Split(buffer.data(), '\n'))
      {
        if (line.find("vendor_id") != std::string::npos)
          m_cpuVendor = line.substr(line.find(':') + 2);

        else if (line.find("model name") != std::string::npos)
          m_cpuModel = line.substr(line.find(':') + 2);

        else if (line.find("BogoMIPS") != std::string::npos)
          m_cpuBogoMips = line.substr(line.find(':') + 2);

        else if (line.find("Hardware") != std::string::npos)
          m_cpuHardware = line.substr(line.find(':') + 2);

        else if (line.find("Serial") != std::string::npos)
          m_cpuSerial = line.substr(line.find(':') + 2);

        else if (line.find("Revision") != std::string::npos)
          m_cpuRevision = line.substr(line.find(':') + 2);
      }
    }

    m_posixFile->Close();
  }

  m_cpuCount = GetCPUCount();

  for (int i = 0; i < m_cpuCount; i++)
  {
    CoreInfo core;
    core.m_id = i;
    m_cores.emplace_back(core);
  }

  if (HasNeon())
    m_cpuFeatures |= CPU_FEATURE_NEON;
}

// Apps cannot read /proc/stat on Android, so system-wide load is unavailable.
// Report Kodi's own usage instead. The total includes threads that have exited;
// per-core values attribute each live thread's CPU time to the core it last ran
// on.
int CCPUInfoAndroid::GetUsedPercentage()
{
  if (!m_nextUsedReadTime.IsTimePast())
    return m_lastUsedPercentage;

  DIR* dir = opendir("/proc/self/task");
  if (!dir)
    return m_lastUsedPercentage;

  const auto now = std::chrono::steady_clock::now();

  rusage usage{};
  getrusage(RUSAGE_SELF, &usage);
  const double processCpuTime = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6 +
                                usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6;

  std::unordered_map<pid_t, uint64_t> threadTicks;
  std::vector<uint64_t> coreTicks(m_cores.size(), 0);

  while (const dirent* entry = readdir(dir))
  {
    if (entry->d_name[0] == '.')
      continue;

    std::ifstream file(std::string("/proc/self/task/") + entry->d_name + "/stat");
    std::string line;
    if (!std::getline(file, line))
      continue;

    // The thread name (field 2) may contain spaces, so parse after its closing ')'
    const auto pos = line.rfind(')');
    if (pos == std::string::npos)
      continue;

    std::istringstream ss(line.substr(pos + 1));
    std::string field;
    uint64_t ticks = 0;
    int core = -1;
    for (int i = 3; i <= 39 && ss >> field; i++)
    {
      if (i == 14 || i == 15) // utime, stime
        ticks += std::strtoull(field.c_str(), nullptr, 10);
      else if (i == 39) // processor
        core = std::atoi(field.c_str());
    }

    const pid_t tid = std::atoi(entry->d_name);
    threadTicks[tid] = ticks;

    const auto prev = m_threadTicks.find(tid);
    const uint64_t delta =
        (prev != m_threadTicks.end() && prev->second <= ticks) ? ticks - prev->second : ticks;
    if (core >= 0 && static_cast<std::size_t>(core) < coreTicks.size())
      coreTicks[core] += delta;
  }
  closedir(dir);

  // The first read only establishes a baseline
  if (m_lastUsageRead != std::chrono::steady_clock::time_point{})
  {
    const double elapsed = std::chrono::duration<double>(now - m_lastUsageRead).count();
    const double elapsedTicks = elapsed * m_clockTicks;

    if (elapsedTicks > 0)
    {
      m_lastUsedPercentage =
          static_cast<int>(std::min(100.0, (processCpuTime - m_lastProcessCpuTime) * 100.0 /
                                               (elapsed * std::max(m_cpuCount, 1))));

      for (std::size_t i = 0; i < m_cores.size(); i++)
        m_cores[i].m_usagePercent = std::min(100.0, coreTicks[i] * 100.0 / elapsedTicks);
    }
  }
  else
  {
    m_lastUsedPercentage = 0;
  }

  m_threadTicks = std::move(threadTicks);
  m_lastProcessCpuTime = processCpuTime;
  m_lastUsageRead = now;
  m_nextUsedReadTime.Set(MINIMUM_TIME_BETWEEN_READS);

  return m_lastUsedPercentage;
}

float CCPUInfoAndroid::GetCPUFrequency()
{
  float freq = 0.f;

  if (!m_posixFile)
    return freq;

  if (m_posixFile->Open(CURL("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq")))
  {
    std::array<char, 32> buffer = {};

    if (0 < m_posixFile->Read(buffer.data(), buffer.size()))
      freq = std::atof(buffer.data()) / 1000;

    m_posixFile->Close();
  }

  return freq;
}

int CCPUInfoAndroid::GetCPUCount()
{
  static int count = -1;

  if (count == -1)
    count = android_getCpuCount();

  return count;
}

bool CCPUInfoAndroid::HasNeon()
{
  // All ARMv8-based devices support Neon - https://developer.android.com/ndk/guides/cpu-arm-neon
  if (android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM64)
    return true;

  if (android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM)
    return ((android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0);

  return false;
}
