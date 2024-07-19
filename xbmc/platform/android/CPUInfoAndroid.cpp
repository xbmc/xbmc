/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CPUInfoAndroid.h"

#include "URL.h"
#include "cpu-features.h"
#include "utils/StringUtils.h"

#include <array>

std::shared_ptr<CCPUInfo> CCPUInfo::GetCPUInfo()
{
  return std::make_shared<CCPUInfoAndroid>();
}

CCPUInfoAndroid::CCPUInfoAndroid() : m_posixFile(std::make_unique<CPosixFile>())
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
