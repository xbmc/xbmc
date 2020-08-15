/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CPUInfoWin32.h"

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/Temperature.h"

#include "platform/win32/CharsetConverter.h"

#include <algorithm>

#include <Pdh.h>
#include <PdhMsg.h>
#include <intrin.h>

#pragma comment(lib, "Pdh.lib")

namespace
{
const unsigned int CPUINFO_EAX{0};
const unsigned int CPUINFO_EBX{1};
const unsigned int CPUINFO_ECX{2};
const unsigned int CPUINFO_EDX{3};
} // namespace

using KODI::PLATFORM::WINDOWS::FromW;

std::shared_ptr<CCPUInfo> CCPUInfo::GetCPUInfo()
{
  return std::make_shared<CCPUInfoWin32>();
}

CCPUInfoWin32::CCPUInfoWin32()
{

  HKEY hKeyCpuRoot;

  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor", 0,
                    KEY_READ, &hKeyCpuRoot) == ERROR_SUCCESS)
  {
    DWORD num = 0;
    wchar_t subKeyName[200]; // more than enough
    DWORD subKeyNameLen = sizeof(subKeyName) / sizeof(wchar_t);
    while (RegEnumKeyExW(hKeyCpuRoot, num++, subKeyName, &subKeyNameLen, nullptr, nullptr, nullptr,
                         nullptr) == ERROR_SUCCESS)
    {
      HKEY hCpuKey;
      if (RegOpenKeyExW(hKeyCpuRoot, subKeyName, 0, KEY_QUERY_VALUE, &hCpuKey) == ERROR_SUCCESS)
      {
        CoreInfo cpuCore;
        bool vendorfound = false;
        bool modelfound = false;
        if (swscanf_s(subKeyName, L"%i", &cpuCore.m_id) != 1)
          cpuCore.m_id = num - 1;
        wchar_t buf[300]; // more than enough
        DWORD bufSize = sizeof(buf);
        DWORD valType;
        if (!modelfound &&
            RegQueryValueExW(hCpuKey, L"ProcessorNameString", nullptr, &valType, LPBYTE(buf),
                             &bufSize) == ERROR_SUCCESS &&
            valType == REG_SZ)
        {
          m_cpuModel = FromW(buf, bufSize / sizeof(wchar_t));
          m_cpuModel =
              m_cpuModel.substr(0, m_cpuModel.find(char(0))); // remove extra null terminations
          StringUtils::RemoveDuplicatedSpacesAndTabs(m_cpuModel);
          StringUtils::Trim(m_cpuModel);
          modelfound = true;
        }
        bufSize = sizeof(buf);
        if (!vendorfound &&
            RegQueryValueExW(hCpuKey, L"VendorIdentifier", nullptr, &valType, LPBYTE(buf),
                             &bufSize) == ERROR_SUCCESS &&
            valType == REG_SZ)
        {
          m_cpuVendor = FromW(buf, bufSize / sizeof(wchar_t));
          m_cpuVendor =
              m_cpuVendor.substr(0, m_cpuVendor.find(char(0))); // remove extra null terminations
          vendorfound = true;
        }
        RegCloseKey(hCpuKey);

        m_cores.push_back(cpuCore);
        m_coreCounters.push_back(nullptr);
      }
      subKeyNameLen = sizeof(subKeyName) / sizeof(wchar_t); // restore length value
    }
    RegCloseKey(hKeyCpuRoot);
  }

  SYSTEM_INFO siSysInfo;
  GetNativeSystemInfo(&siSysInfo);
  m_cpuCount = siSysInfo.dwNumberOfProcessors;

  if (PdhOpenQueryW(nullptr, 0, &m_cpuQueryFreq) == ERROR_SUCCESS)
  {
    if (PdhAddEnglishCounterW(m_cpuQueryFreq, L"\\Processor Information(0,0)\\Processor Frequency",
                              0, &m_cpuFreqCounter) != ERROR_SUCCESS)
      m_cpuFreqCounter = nullptr;
  }
  else
    m_cpuQueryFreq = nullptr;

  if (PdhOpenQueryW(nullptr, 0, &m_cpuQueryLoad) == ERROR_SUCCESS)
  {
    for (size_t i = 0; i < m_cores.size(); i++)
    {
      if (i < m_coreCounters.size() &&
          PdhAddEnglishCounterW(
              m_cpuQueryLoad, StringUtils::Format(L"\\Processor(%d)\\%% Idle Time", int(i)).c_str(),
              0, &m_coreCounters[i]) != ERROR_SUCCESS)
        m_coreCounters[i] = nullptr;
    }
  }
  else
    m_cpuQueryLoad = nullptr;

  int CPUInfo[4] = {}; // receives EAX, EBX, ECD and EDX in that order

  __cpuid(CPUInfo, 0);
  int MaxStdInfoType = CPUInfo[0];

  if (MaxStdInfoType >= CPUID_INFOTYPE_STANDARD)
  {
    __cpuid(CPUInfo, CPUID_INFOTYPE_STANDARD);
    if (CPUInfo[CPUINFO_EDX] & CPUID_00000001_EDX_MMX)
      m_cpuFeatures |= CPU_FEATURE_MMX;
    if (CPUInfo[CPUINFO_EDX] & CPUID_00000001_EDX_SSE)
      m_cpuFeatures |= CPU_FEATURE_SSE;
    if (CPUInfo[CPUINFO_EDX] & CPUID_00000001_EDX_SSE2)
      m_cpuFeatures |= CPU_FEATURE_SSE2;
    if (CPUInfo[CPUINFO_ECX] & CPUID_00000001_ECX_SSE3)
      m_cpuFeatures |= CPU_FEATURE_SSE3;
    if (CPUInfo[CPUINFO_ECX] & CPUID_00000001_ECX_SSSE3)
      m_cpuFeatures |= CPU_FEATURE_SSSE3;
    if (CPUInfo[CPUINFO_ECX] & CPUID_00000001_ECX_SSE4)
      m_cpuFeatures |= CPU_FEATURE_SSE4;
    if (CPUInfo[CPUINFO_ECX] & CPUID_00000001_ECX_SSE42)
      m_cpuFeatures |= CPU_FEATURE_SSE42;
  }

  __cpuid(CPUInfo, CPUID_INFOTYPE_EXTENDED_IMPLEMENTED);
  if (CPUInfo[0] >= CPUID_INFOTYPE_EXTENDED)
  {
    __cpuid(CPUInfo, CPUID_INFOTYPE_EXTENDED);

    if (CPUInfo[CPUINFO_EDX] & CPUID_80000001_EDX_MMX)
      m_cpuFeatures |= CPU_FEATURE_MMX;
    if (CPUInfo[CPUINFO_EDX] & CPUID_80000001_EDX_MMX2)
      m_cpuFeatures |= CPU_FEATURE_MMX2;
    if (CPUInfo[CPUINFO_EDX] & CPUID_80000001_EDX_3DNOW)
      m_cpuFeatures |= CPU_FEATURE_3DNOW;
    if (CPUInfo[CPUINFO_EDX] & CPUID_80000001_EDX_3DNOWEXT)
      m_cpuFeatures |= CPU_FEATURE_3DNOWEXT;
  }

  // Set MMX2 when SSE is present as SSE is a superset of MMX2 and Intel doesn't set the MMX2 cap
  if (m_cpuFeatures & CPU_FEATURE_SSE)
    m_cpuFeatures |= CPU_FEATURE_MMX2;
}

CCPUInfoWin32::~CCPUInfoWin32()
{
  if (m_cpuQueryFreq)
    PdhCloseQuery(m_cpuQueryFreq);

  if (m_cpuQueryLoad)
    PdhCloseQuery(m_cpuQueryLoad);
}

int CCPUInfoWin32::GetUsedPercentage()
{
  int result = 0;

  if (!m_nextUsedReadTime.IsTimePast())
    return m_lastUsedPercentage;

  FILETIME idleTimestamp;
  FILETIME kernelTimestamp;
  FILETIME userTimestamp;
  if (GetSystemTimes(&idleTimestamp, &kernelTimestamp, &userTimestamp) == 0)
    return m_lastUsedPercentage;

  size_t kernelTime = 0;
  size_t userTime = 0;
  size_t activeTime = 0;
  size_t idleTime = 0;
  size_t totalTime = 0;

  idleTime = (static_cast<size_t>(idleTimestamp.dwHighDateTime) << 32) +
             static_cast<size_t>(idleTimestamp.dwLowDateTime);
  kernelTime = (static_cast<size_t>(kernelTimestamp.dwHighDateTime) << 32) +
               static_cast<size_t>(kernelTimestamp.dwLowDateTime);
  userTime = (static_cast<size_t>(userTimestamp.dwHighDateTime) << 32) +
             static_cast<size_t>(userTimestamp.dwLowDateTime);
  if (userTime + kernelTime + idleTime == 0)
    return m_lastUsedPercentage;

  // Teturned "kernelTime" includes "idleTime"
  activeTime = static_cast<size_t>(userTime + kernelTime - idleTime);
  totalTime = static_cast<size_t>(userTime + kernelTime);

  activeTime -= m_activeTime;
  idleTime -= m_idleTime;
  totalTime -= m_totalTime;

  m_activeTime += activeTime;
  m_idleTime += idleTime;
  m_totalTime += totalTime;

  m_lastUsedPercentage = activeTime * 100.0f / totalTime;
  m_nextUsedReadTime.Set(MINIMUM_TIME_BETWEEN_READS);

  if (m_cpuFreqCounter && PdhCollectQueryData(m_cpuQueryLoad) == ERROR_SUCCESS)
  {
    size_t i = 0;
    for (auto& core : m_cores)
    {
      PDH_RAW_COUNTER cnt;
      DWORD cntType;
      PDH_HCOUNTER coreCounter = nullptr;
      if (i < m_coreCounters.size())
        coreCounter = m_coreCounters[i];
      if (coreCounter && PdhGetRawCounterValue(coreCounter, &cntType, &cnt) == ERROR_SUCCESS &&
          (cnt.CStatus == PDH_CSTATUS_VALID_DATA || cnt.CStatus == PDH_CSTATUS_NEW_DATA))
      {
        const LONGLONG coreTotal = cnt.SecondValue;
        const LONGLONG coreIdle = cnt.FirstValue;
        const LONGLONG deltaTotal = coreTotal - core.m_totalTime;
        const LONGLONG deltaIdle = coreIdle - core.m_idleTime;
        const double load = (double(deltaTotal - deltaIdle) * 100.0) / double(deltaTotal);

        // win32 has some problems with calculation of load if load close to zero
        if (load < 0)
          core.m_usagePercent = 0;
        else
          core.m_usagePercent = load;
        if (load >= 0 ||
            deltaTotal > 5 * 10 * 1000 *
                             1000) // do not update (smooth) values for 5 seconds on negative loads
        {
          core.m_totalTime = coreTotal;
          core.m_idleTime = coreIdle;
        }
      }
      else
        core.m_usagePercent = double(m_lastUsedPercentage); // use CPU average as fallback

      i++;
    }
  }
  else
    for (auto& core : m_cores)
      core.m_usagePercent = double(m_lastUsedPercentage); // use CPU average as fallback


  result = static_cast<int>(m_lastUsedPercentage);

  return result;
}

float CCPUInfoWin32::GetCPUFrequency()
{
  // Get CPU frequency, scaled to MHz.
  if (m_cpuFreqCounter && PdhCollectQueryData(m_cpuQueryFreq) == ERROR_SUCCESS)
  {
    PDH_RAW_COUNTER cnt;
    DWORD cntType;
    if (PdhGetRawCounterValue(m_cpuFreqCounter, &cntType, &cnt) == ERROR_SUCCESS &&
        (cnt.CStatus == PDH_CSTATUS_VALID_DATA || cnt.CStatus == PDH_CSTATUS_NEW_DATA))
    {
      return float(cnt.FirstValue);
    }
  }

  return 0;
}

bool CCPUInfoWin32::GetTemperature(CTemperature& temperature)
{
  temperature.SetValid(false);
  return false;
}
