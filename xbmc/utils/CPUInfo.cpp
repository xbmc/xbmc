/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <cstdlib>

#include "CPUInfo.h"
#include "utils/log.h"
#include "utils/SysfsUtils.h"
#include "utils/Temperature.h"
#include <string>
#include <string.h>

#if defined(TARGET_DARWIN)
#include <sys/types.h>
#include <sys/sysctl.h>
#if defined(__ppc__) || defined (TARGET_DARWIN_EMBEDDED)
#include <mach-o/arch.h>
#endif // defined(__ppc__) || defined (TARGET_DARWIN_EMBEDDED)
#ifdef TARGET_DARWIN_OSX
#include "platform/darwin/osx/smc.h"
#endif
#include "platform/linux/LinuxResourceCounter.h"
#endif

#if defined(TARGET_FREEBSD)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/resource.h>
#endif

#if defined(TARGET_LINUX) && defined(HAS_NEON) && !defined(TARGET_ANDROID)
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <linux/auxvec.h>
#include <asm/hwcap.h>
#endif

#if defined(TARGET_ANDROID)
#include "platform/android/activity/AndroidFeatures.h"
#endif

#ifdef TARGET_WINDOWS
#include "platform/win32/CharsetConverter.h"
#include <algorithm>
#include <intrin.h>
#include <Pdh.h>
#include <PdhMsg.h>

#ifdef TARGET_WINDOWS_DESKTOP
#pragma comment(lib, "Pdh.lib")
#endif

#ifdef TARGET_WINDOWS_STORE
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.System.Diagnostics.h>
#endif

// Defines to help with calls to CPUID
#define CPUID_INFOTYPE_STANDARD 0x00000001
#define CPUID_INFOTYPE_EXTENDED 0x80000001

// Standard Features
// Bitmasks for the values returned by a call to cpuid with eax=0x00000001
#define CPUID_00000001_ECX_SSE3  (1<<0)
#define CPUID_00000001_ECX_SSSE3 (1<<9)
#define CPUID_00000001_ECX_SSE4  (1<<19)
#define CPUID_00000001_ECX_SSE42 (1<<20)

#define CPUID_00000001_EDX_MMX   (1<<23)
#define CPUID_00000001_EDX_SSE   (1<<25)
#define CPUID_00000001_EDX_SSE2  (1<<26)

// Extended Features
// Bitmasks for the values returned by a call to cpuid with eax=0x80000001
#define CPUID_80000001_EDX_MMX2     (1<<22)
#define CPUID_80000001_EDX_MMX      (1<<23)
#define CPUID_80000001_EDX_3DNOWEXT (1<<30)
#define CPUID_80000001_EDX_3DNOW    (1<<31)


// Help with the __cpuid intrinsic of MSVC
#define CPUINFO_EAX 0
#define CPUINFO_EBX 1
#define CPUINFO_ECX 2
#define CPUINFO_EDX 3

#endif

#ifdef TARGET_POSIX
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#endif

#include "utils/StringUtils.h"

// In milliseconds
#define MINIMUM_TIME_BETWEEN_READS 500

CCPUInfo::CCPUInfo(void)
{
#ifdef TARGET_POSIX
  m_fProcStat = m_fProcTemperature = m_fCPUFreq = NULL;
  m_cpuInfoForFreq = false;
#elif defined(TARGET_WINDOWS)
  m_cpuQueryFreq = nullptr;
  m_cpuQueryLoad = nullptr;
#endif
  m_lastUsedPercentage = 0;
  m_cpuFeatures = 0;

#if defined(TARGET_DARWIN)
  m_pResourceCounter = new CLinuxResourceCounter();

  size_t len = 4;
  std::string cpuVendor;

  // The number of cores.
  if (sysctlbyname("hw.activecpu", &m_cpuCount, &len, NULL, 0) == -1)
      m_cpuCount = 1;

  // The model.
#if defined(__ppc__) || defined (TARGET_DARWIN_EMBEDDED)
  const NXArchInfo *info = NXGetLocalArchInfo();
  if (info != NULL)
    m_cpuModel = info->description;
#else
  // NXGetLocalArchInfo is ugly for intel so keep using this method
  char buffer[512];
  len = 512;
  if (sysctlbyname("machdep.cpu.brand_string", &buffer, &len, NULL, 0) == 0)
    m_cpuModel = buffer;

  // The CPU vendor
  len = 512;
  if (sysctlbyname("machdep.cpu.vendor", &buffer, &len, NULL, 0) == 0)
    cpuVendor = buffer;

#endif
  // Go through each core.
  for (int i=0; i<m_cpuCount; i++)
  {
    CoreInfo core;
    core.m_id = i;
    core.m_strModel = m_cpuModel;
    core.m_strVendor = cpuVendor;
    m_cores[core.m_id] = core;
  }
#elif defined(TARGET_WINDOWS_STORE)
  SYSTEM_INFO siSysInfo;
  GetNativeSystemInfo(&siSysInfo);
  m_cpuCount = siSysInfo.dwNumberOfProcessors;
  m_cpuModel = "Unknown";

#elif defined(TARGET_WINDOWS_DESKTOP)
  using KODI::PLATFORM::WINDOWS::FromW;

  HKEY hKeyCpuRoot;

  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor", 0, KEY_READ, &hKeyCpuRoot) == ERROR_SUCCESS)
  {
    DWORD num = 0;
    std::vector<CoreInfo> cpuCores;
    wchar_t subKeyName[200]; // more than enough
    DWORD subKeyNameLen = sizeof(subKeyName) / sizeof(wchar_t);
    while (RegEnumKeyExW(hKeyCpuRoot, num++, subKeyName, &subKeyNameLen, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS)
    {
      HKEY hCpuKey;
      if (RegOpenKeyExW(hKeyCpuRoot, subKeyName, 0, KEY_QUERY_VALUE, &hCpuKey) == ERROR_SUCCESS)
      {
        CoreInfo cpuCore;
        if (swscanf_s(subKeyName, L"%i", &cpuCore.m_id) != 1)
          cpuCore.m_id = num - 1;
        wchar_t buf[300]; // more than enough
        DWORD bufSize = sizeof(buf);
        DWORD valType;
        if (RegQueryValueExW(hCpuKey, L"ProcessorNameString", nullptr, &valType, LPBYTE(buf), &bufSize) == ERROR_SUCCESS &&
            valType == REG_SZ)
        {
          cpuCore.m_strModel = FromW(buf, bufSize / sizeof(wchar_t));
          cpuCore.m_strModel = cpuCore.m_strModel.substr(0, cpuCore.m_strModel.find(char(0))); // remove extra null terminations
          StringUtils::RemoveDuplicatedSpacesAndTabs(cpuCore.m_strModel);
          StringUtils::Trim(cpuCore.m_strModel);
        }
        bufSize = sizeof(buf);
        if (RegQueryValueExW(hCpuKey, L"VendorIdentifier", nullptr, &valType, LPBYTE(buf), &bufSize) == ERROR_SUCCESS &&
            valType == REG_SZ)
        {
          cpuCore.m_strVendor = FromW(buf, bufSize / sizeof(wchar_t));
          cpuCore.m_strVendor = cpuCore.m_strVendor.substr(0, cpuCore.m_strVendor.find(char(0))); // remove extra null terminations
        }
        DWORD mhzVal;
        bufSize = sizeof(mhzVal);
        if (RegQueryValueExW(hCpuKey, L"~MHz", nullptr, &valType, LPBYTE(&mhzVal), &bufSize) == ERROR_SUCCESS &&
            valType == REG_DWORD)
          cpuCore.m_fSpeed = double(mhzVal);

        RegCloseKey(hCpuKey);

        if (cpuCore.m_strModel.empty())
          cpuCore.m_strModel = "Unknown";
        cpuCores.push_back(cpuCore);
      }
      subKeyNameLen = sizeof(subKeyName) / sizeof(wchar_t); // restore length value
    }
    RegCloseKey(hKeyCpuRoot);
    std::sort(cpuCores.begin(), cpuCores.end()); // sort cores by id
    for (size_t i = 0; i < cpuCores.size(); i++)
      m_cores[i] = cpuCores[i]; // add in sorted order
  }

  if (!m_cores.empty())
    m_cpuModel = m_cores.begin()->second.m_strModel;
  else
    m_cpuModel = "Unknown";

  SYSTEM_INFO siSysInfo;
  GetNativeSystemInfo(&siSysInfo);
  m_cpuCount = siSysInfo.dwNumberOfProcessors;

  if (PdhOpenQueryW(nullptr, 0, &m_cpuQueryFreq) == ERROR_SUCCESS)
  {
    if (PdhAddEnglishCounterW(m_cpuQueryFreq, L"\\Processor Information(0,0)\\Processor Frequency", 0, &m_cpuFreqCounter) != ERROR_SUCCESS)
      m_cpuFreqCounter = nullptr;
  }
  else
    m_cpuQueryFreq = nullptr;

  if (PdhOpenQueryW(nullptr, 0, &m_cpuQueryLoad) == ERROR_SUCCESS)
  {
    for (size_t i = 0; i < m_cores.size(); i++)
    {
      if (PdhAddEnglishCounterW(m_cpuQueryLoad, StringUtils::Format(L"\\Processor(%d)\\%% Idle Time", int(i)).c_str(), 0, &m_cores[i].m_coreCounter) != ERROR_SUCCESS)
        m_cores[i].m_coreCounter = nullptr;
    }
  }
  else
    m_cpuQueryLoad = nullptr;
#elif defined(TARGET_FREEBSD)
  size_t len;
  int i;
  char cpumodel[512];

  len = sizeof(m_cpuCount);
  if (sysctlbyname("hw.ncpu", &m_cpuCount, &len, NULL, 0) != 0)
    m_cpuCount = 1;

  len = sizeof(cpumodel);
  if (sysctlbyname("hw.model", &cpumodel, &len, NULL, 0) != 0)
    (void)strncpy(cpumodel, "Unknown", 8);
  m_cpuModel = cpumodel;

  for (i = 0; i < m_cpuCount; i++)
  {
    CoreInfo core;
    core.m_id = i;
    core.m_strModel = m_cpuModel;
    m_cores[core.m_id] = core;
  }
#else
  m_fProcStat = fopen("/proc/stat", "r");
  m_fProcTemperature = fopen("/proc/acpi/thermal_zone/THM0/temperature", "r");
  if (m_fProcTemperature == NULL)
    m_fProcTemperature = fopen("/proc/acpi/thermal_zone/THRM/temperature", "r");
  if (m_fProcTemperature == NULL)
    m_fProcTemperature = fopen("/proc/acpi/thermal_zone/THR0/temperature", "r");
  if (m_fProcTemperature == NULL)
    m_fProcTemperature = fopen("/proc/acpi/thermal_zone/TZ0/temperature", "r");
  // read from the new location of the temperature data on new kernels, 2.6.39, 3.0 etc
  if (m_fProcTemperature == NULL)
    m_fProcTemperature = fopen("/sys/class/hwmon/hwmon0/temp1_input", "r");
  if (m_fProcTemperature == NULL)
    m_fProcTemperature = fopen("/sys/class/thermal/thermal_zone0/temp", "r");  // On Raspberry PIs

  m_fCPUFreq = fopen ("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r");
  if (!m_fCPUFreq)
  {
    m_cpuInfoForFreq = true;
    m_fCPUFreq = fopen("/proc/cpuinfo", "r");
  }
  else
    m_cpuInfoForFreq = false;


  FILE* fCPUInfo = fopen("/proc/cpuinfo", "r");
  m_cpuCount = 0;
  if (fCPUInfo)
  {
    char buffer[512];

    int nCurrId = 0;
    while (fgets(buffer, sizeof(buffer), fCPUInfo))
    {
      if (strncmp(buffer, "processor", strlen("processor"))==0)
      {
        char *needle = strstr(buffer, ":");
        if (needle)
        {
          CoreInfo core;
          core.m_id = atoi(needle+2);
          nCurrId = core.m_id;
          m_cores[core.m_id] = core;
        }
        m_cpuCount++;
      }
      else if (strncmp(buffer, "vendor_id", strlen("vendor_id"))==0)
      {
        char *needle = strstr(buffer, ":");
        if (needle && strlen(needle)>3)
        {
          needle+=2;
          m_cores[nCurrId].m_strVendor = needle;
          StringUtils::Trim(m_cores[nCurrId].m_strVendor);
        }
      }
      else if (strncmp(buffer, "Processor", strlen("Processor"))==0)
      {
        char *needle = strstr(buffer, ":");
        if (needle && strlen(needle)>3)
        {
          needle+=2;
          m_cpuModel = needle;
          m_cores[nCurrId].m_strModel = m_cpuModel;
          StringUtils::Trim(m_cores[nCurrId].m_strModel);
        }
      }
      else if (strncmp(buffer, "BogoMIPS", strlen("BogoMIPS"))==0)
      {
        char *needle = strstr(buffer, ":");
        if (needle && strlen(needle)>3)
        {
          needle+=2;
          m_cpuBogoMips = needle;
          m_cores[nCurrId].m_strBogoMips = m_cpuBogoMips;
          StringUtils::Trim(m_cores[nCurrId].m_strBogoMips);
        }
      }
      else if (strncmp(buffer, "Hardware", strlen("Hardware"))==0)
      {
        char *needle = strstr(buffer, ":");
        if (needle && strlen(needle)>3)
        {
          needle+=2;
          m_cpuHardware = needle;
          m_cores[nCurrId].m_strHardware = m_cpuHardware;
          StringUtils::Trim(m_cores[nCurrId].m_strHardware);
        }
      }
      else if (strncmp(buffer, "Revision", strlen("Revision"))==0)
      {
        char *needle = strstr(buffer, ":");
        if (needle && strlen(needle)>3)
        {
          needle+=2;
          m_cpuRevision = needle;
          m_cores[nCurrId].m_strRevision = m_cpuRevision;
          StringUtils::Trim(m_cores[nCurrId].m_strRevision);
        }
      }
      else if (strncmp(buffer, "Serial", strlen("Serial"))==0)
      {
        char *needle = strstr(buffer, ":");
        if (needle && strlen(needle)>3)
        {
          needle+=2;
          m_cpuSerial = needle;
          m_cores[nCurrId].m_strSerial = m_cpuSerial;
          StringUtils::Trim(m_cores[nCurrId].m_strSerial);
        }
      }
      else if (strncmp(buffer, "model name", strlen("model name"))==0)
      {
        char *needle = strstr(buffer, ":");
        if (needle && strlen(needle)>3)
        {
          needle+=2;
          m_cpuModel = needle;
          m_cores[nCurrId].m_strModel = m_cpuModel;
          StringUtils::Trim(m_cores[nCurrId].m_strModel);
        }
      }
      else if (strncmp(buffer, "flags", 5) == 0)
      {
        char* needle = strchr(buffer, ':');
        if (needle)
        {
          char* tok = NULL,
              * save;
          needle++;
          tok = strtok_r(needle, " ", &save);
          while (tok)
          {
            if (0 == strcmp(tok, "mmx"))
              m_cpuFeatures |= CPU_FEATURE_MMX;
            else if (0 == strcmp(tok, "mmxext"))
              m_cpuFeatures |= CPU_FEATURE_MMX2;
            else if (0 == strcmp(tok, "sse"))
              m_cpuFeatures |= CPU_FEATURE_SSE;
            else if (0 == strcmp(tok, "sse2"))
              m_cpuFeatures |= CPU_FEATURE_SSE2;
            else if (0 == strcmp(tok, "sse3"))
              m_cpuFeatures |= CPU_FEATURE_SSE3;
            else if (0 == strcmp(tok, "ssse3"))
              m_cpuFeatures |= CPU_FEATURE_SSSE3;
            else if (0 == strcmp(tok, "sse4_1"))
              m_cpuFeatures |= CPU_FEATURE_SSE4;
            else if (0 == strcmp(tok, "sse4_2"))
              m_cpuFeatures |= CPU_FEATURE_SSE42;
            else if (0 == strcmp(tok, "3dnow"))
              m_cpuFeatures |= CPU_FEATURE_3DNOW;
            else if (0 == strcmp(tok, "3dnowext"))
              m_cpuFeatures |= CPU_FEATURE_3DNOWEXT;
            tok = strtok_r(NULL, " ", &save);
          }
        }
      }
    }
    fclose(fCPUInfo);
    // new socs use the sysfs soc interface to describe the hardware
    if (SysfsUtils::Has("/sys/bus/soc/devices/soc0"))
    {
      std::string machine, family, soc_id;
      if (SysfsUtils::Has("/sys/bus/soc/devices/soc0/machine"))
        SysfsUtils::GetString("/sys/bus/soc/devices/soc0/machine", machine);
      if (SysfsUtils::Has("/sys/bus/soc/devices/soc0/family"))
        SysfsUtils::GetString("/sys/bus/soc/devices/soc0/family", family);
      if (SysfsUtils::Has("/sys/bus/soc/devices/soc0/soc_id"))
        SysfsUtils::GetString("/sys/bus/soc/devices/soc0/soc_id", soc_id);
      if (m_cpuHardware.empty() && !machine.empty())
        m_cpuHardware = machine;
      if (!family.empty() && !soc_id.empty())
        m_cpuSoC = family + " " + soc_id;
    }
    //  /proc/cpuinfo is not reliable on some Android platforms
    //  At least we should get the correct cpu count for multithreaded decoding
#if defined(TARGET_ANDROID)
    if (CAndroidFeatures::GetCPUCount() > m_cpuCount)
    {
      for (int i = m_cpuCount; i < CAndroidFeatures::GetCPUCount(); i++)
      {
        // Copy info from cpu 0
        CoreInfo core(m_cores[0]);
        core.m_id = i;
        m_cores[core.m_id] = core;
      }

      m_cpuCount = CAndroidFeatures::GetCPUCount();
    }
#endif
  }
  else
  {
    m_cpuCount = 1;
    m_cpuModel = "Unknown";
  }

#endif
  StringUtils::Replace(m_cpuModel, '\r', ' ');
  StringUtils::Replace(m_cpuModel, '\n', ' ');
  StringUtils::RemoveDuplicatedSpacesAndTabs(m_cpuModel);
  StringUtils::Trim(m_cpuModel);

  /* Set some default for empty string variables */
  if (m_cpuBogoMips.empty())
    m_cpuBogoMips = "N/A";
  if (m_cpuHardware.empty())
    m_cpuHardware = "N/A";
  if (m_cpuRevision.empty())
    m_cpuRevision = "N/A";
  if (m_cpuSerial.empty())
    m_cpuSerial = "N/A";

  readProcStat(m_userTicks, m_niceTicks, m_systemTicks, m_idleTicks, m_ioTicks);
  m_nextUsedReadTime.Set(MINIMUM_TIME_BETWEEN_READS);

  ReadCPUFeatures();

  // Set MMX2 when SSE is present as SSE is a superset of MMX2 and Intel doesn't set the MMX2 cap
  if (m_cpuFeatures & CPU_FEATURE_SSE)
    m_cpuFeatures |= CPU_FEATURE_MMX2;

  if (HasNeon())
    m_cpuFeatures |= CPU_FEATURE_NEON;

}

CCPUInfo::~CCPUInfo()
{
#ifdef TARGET_POSIX
  if (m_fProcStat != NULL)
    fclose(m_fProcStat);

  if (m_fProcTemperature != NULL)
    fclose(m_fProcTemperature);

  if (m_fCPUFreq != NULL)
    fclose(m_fCPUFreq);
#elif defined(TARGET_WINDOWS_DESKTOP)
  if (m_cpuQueryFreq)
    PdhCloseQuery(m_cpuQueryFreq);

  if (m_cpuQueryLoad)
    PdhCloseQuery(m_cpuQueryLoad);
#elif defined(TARGET_DARWIN)
  delete m_pResourceCounter;
#endif
}

int CCPUInfo::getUsedPercentage()
{
  int result = 0;

  if (!m_nextUsedReadTime.IsTimePast())
    return m_lastUsedPercentage;

#if defined(TARGET_DARWIN)
  result = m_pResourceCounter->GetCPUUsage();
#else
  unsigned long long userTicks;
  unsigned long long niceTicks;
  unsigned long long systemTicks;
  unsigned long long idleTicks;
  unsigned long long ioTicks;

  if (!readProcStat(userTicks, niceTicks, systemTicks, idleTicks, ioTicks))
    return m_lastUsedPercentage;

  userTicks -= m_userTicks;
  niceTicks -= m_niceTicks;
  systemTicks -= m_systemTicks;
  idleTicks -= m_idleTicks;
  ioTicks -= m_ioTicks;

  if(userTicks + niceTicks + systemTicks + idleTicks + ioTicks == 0)
    return m_lastUsedPercentage;
  result = static_cast<int>(double(userTicks + niceTicks + systemTicks) * 100.0 / double(userTicks + niceTicks + systemTicks + idleTicks + ioTicks) + 0.5);

  m_userTicks += userTicks;
  m_niceTicks += niceTicks;
  m_systemTicks += systemTicks;
  m_idleTicks += idleTicks;
  m_ioTicks += ioTicks;
#endif
  m_lastUsedPercentage = result;
  m_nextUsedReadTime.Set(MINIMUM_TIME_BETWEEN_READS);

  return result;
}

float CCPUInfo::getCPUFrequency()
{
  // Get CPU frequency, scaled to MHz.
#if defined(TARGET_DARWIN)
  long long hz = 0;
  size_t len = sizeof(hz);
  if (sysctlbyname("hw.cpufrequency", &hz, &len, NULL, 0) == -1)
    return 0.f;
  return hz / 1000000.0;
#elif defined(TARGET_WINDOWS_STORE)
  CLog::Log(LOGDEBUG, "%s is not implemented", __FUNCTION__);
  return 0.f;
#elif defined(TARGET_WINDOWS_DESKTOP)
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

  if (!m_cores.empty())
    return float(m_cores.begin()->second.m_fSpeed);
  else
    return 0.f;
#elif defined(TARGET_FREEBSD)
  int hz = 0;
  size_t len = sizeof(hz);
  if (sysctlbyname("dev.cpu.0.freq", &hz, &len, NULL, 0) != 0)
    hz = 0;
  return (float)hz;
#else
  int value = 0;
  if (m_fCPUFreq && !m_cpuInfoForFreq)
  {
    rewind(m_fCPUFreq);
    fflush(m_fCPUFreq);
    if (fscanf(m_fCPUFreq, "%d", &value))
      value /= 1000.0;
  }
  if (m_fCPUFreq && m_cpuInfoForFreq)
  {
    rewind(m_fCPUFreq);
    fflush(m_fCPUFreq);
    float mhz, avg=0.0;
    int n, cpus=0;
    while(EOF!=(n=fscanf(m_fCPUFreq," MHz : %f ", &mhz)))
    {
      if (n>0) {
        cpus++;
        avg += mhz;
      }
      if (!fscanf(m_fCPUFreq,"%*s"))
        break;
    }

    if (cpus > 0)
      value = avg/cpus;
  }
  return value;
#endif
}

bool CCPUInfo::getTemperature(CTemperature& temperature)
{
  int         value = 0;
  char        scale = 0;

#ifdef TARGET_POSIX
#if defined(TARGET_DARWIN_OSX)
  value = SMCGetTemperature(SMC_KEY_CPU_TEMP);
  scale = 'c';
#else
  int         ret   = 0;
  FILE        *p    = NULL;
  std::string  cmd   = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_cpuTempCmd;

  temperature.SetValid(false);

  if (cmd.empty() && m_fProcTemperature == NULL)
    return false;

  if (!cmd.empty())
  {
    p = popen (cmd.c_str(), "r");
    if (p)
    {
      ret = fscanf(p, "%d %c", &value, &scale);
      pclose(p);
    }
  }
  else
  {
    // procfs is deprecated in the linux kernel, we should move away from
    // using it for temperature data.  It doesn't seem that sysfs has a
    // general enough interface to bother implementing ATM.

    rewind(m_fProcTemperature);
    fflush(m_fProcTemperature);
    ret = fscanf(m_fProcTemperature, "temperature: %d %c", &value, &scale);

    // read from the temperature file of the new kernels
    if (!ret)
    {
      ret = fscanf(m_fProcTemperature, "%d", &value);
      value = value / 1000;
      scale = 'c';
      ret++;
    }
  }

  if (ret != 2)
    return false;
#endif
#endif // TARGET_POSIX

  if (scale == 'C' || scale == 'c')
    temperature = CTemperature::CreateFromCelsius(value);
  else if (scale == 'F' || scale == 'f')
    temperature = CTemperature::CreateFromFahrenheit(value);
  else
    return false;

  return true;
}

bool CCPUInfo::HasCoreId(int nCoreId) const
{
  std::map<int, CoreInfo>::const_iterator iter = m_cores.find(nCoreId);
  if (iter != m_cores.end())
    return true;
  return false;
}

const CoreInfo &CCPUInfo::GetCoreInfo(int nCoreId)
{
  std::map<int, CoreInfo>::iterator iter = m_cores.find(nCoreId);
  if (iter != m_cores.end())
    return iter->second;

  static CoreInfo dummy;
  return dummy;
}

bool CCPUInfo::readProcStat(unsigned long long& user, unsigned long long& nice,
    unsigned long long& system, unsigned long long& idle, unsigned long long& io)
{
#if defined(TARGET_WINDOWS)
  nice = 0;
  io = 0;
#if defined (TARGET_WINDOWS_DESKTOP)
  FILETIME idleTime;
  FILETIME kernelTime;
  FILETIME userTime;
  if (GetSystemTimes(&idleTime, &kernelTime, &userTime) == 0)
    return false;

  idle = (uint64_t(idleTime.dwHighDateTime) << 32) + uint64_t(idleTime.dwLowDateTime);
  // returned "kernelTime" includes "idleTime"
  system = (uint64_t(kernelTime.dwHighDateTime) << 32) + uint64_t(kernelTime.dwLowDateTime) - idle;
  user = (uint64_t(userTime.dwHighDateTime) << 32) + uint64_t(userTime.dwLowDateTime);

  if (m_cpuFreqCounter && PdhCollectQueryData(m_cpuQueryLoad) == ERROR_SUCCESS)
  {
    for (std::map<int, CoreInfo>::iterator it = m_cores.begin(); it != m_cores.end(); ++it)
    {
      CoreInfo& curCore = it->second; // simplify usage
      PDH_RAW_COUNTER cnt;
      DWORD cntType;
      if (curCore.m_coreCounter && PdhGetRawCounterValue(curCore.m_coreCounter, &cntType, &cnt) == ERROR_SUCCESS &&
          (cnt.CStatus == PDH_CSTATUS_VALID_DATA || cnt.CStatus == PDH_CSTATUS_NEW_DATA))
      {
        const LONGLONG coreTotal = cnt.SecondValue,
                       coreIdle  = cnt.FirstValue;
        const LONGLONG deltaTotal = coreTotal - curCore.m_total,
                       deltaIdle  = coreIdle - curCore.m_idle;
        const double load = (double(deltaTotal - deltaIdle) * 100.0) / double(deltaTotal);

        // win32 has some problems with calculation of load if load close to zero
        curCore.m_fPct = (load < 0) ? 0 : load;
        if (load >= 0 || deltaTotal > 5 * 10 * 1000 * 1000) // do not update (smooth) values for 5 seconds on negative loads
        {
          curCore.m_total = coreTotal;
          curCore.m_idle = coreIdle;
        }
      }
      else
        curCore.m_fPct = double(m_lastUsedPercentage); // use CPU average as fallback
    }
  }
  else
    for (std::map<int, CoreInfo>::iterator it = m_cores.begin(); it != m_cores.end(); ++it)
      it->second.m_fPct = double(m_lastUsedPercentage); // use CPU average as fallback
#endif // TARGET_WINDOWS_DESKTOP
#if defined(TARGET_WINDOWS_STORE)
  if (winrt::Windows::Foundation::Metadata::ApiInformation::IsTypePresent(L"Windows.System.Diagnostics.SystemDiagnosticInfo"))
  {
    auto diagnostic = winrt::Windows::System::Diagnostics::SystemDiagnosticInfo::GetForCurrentSystem();
    auto usage = diagnostic.CpuUsage();
    auto report = usage.GetReport();

    user = report.UserTime().count();
    idle = report.IdleTime().count();
    system = report.KernelTime().count() - idle;
    return true;
  }
  else
    return false;
#endif // TARGET_WINDOWS_STORE
#elif defined(TARGET_FREEBSD)
  long *cptimes;
  size_t len;
  int i;

  len = sizeof(long) * 32 * CPUSTATES;
  if (sysctlbyname("kern.cp_times", NULL, &len, NULL, 0) != 0)
    return false;
  cptimes = (long*)malloc(len);
  if (cptimes == NULL)
    return false;
  if (sysctlbyname("kern.cp_times", cptimes, &len, NULL, 0) != 0)
  {
    free(cptimes);
    return false;
  }
  user = 0;
  nice = 0;
  system = 0;
  idle = 0;
  io = 0;
  for (i = 0; i < m_cpuCount; i++)
  {
    long coreUser, coreNice, coreSystem, coreIdle, coreIO;
    double total;

    coreUser   = cptimes[i * CPUSTATES + CP_USER];
    coreNice   = cptimes[i * CPUSTATES + CP_NICE];
    coreSystem = cptimes[i * CPUSTATES + CP_SYS];
    coreIO     = cptimes[i * CPUSTATES + CP_INTR];
    coreIdle   = cptimes[i * CPUSTATES + CP_IDLE];

    std::map<int, CoreInfo>::iterator iter = m_cores.find(i);
    if (iter != m_cores.end())
    {
      coreUser -= iter->second.m_user;
      coreNice -= iter->second.m_nice;
      coreSystem -= iter->second.m_system;
      coreIdle -= iter->second.m_idle;
      coreIO -= iter->second.m_io;

      total = (double)(coreUser + coreNice + coreSystem + coreIdle + coreIO);
      if(total != 0.0f)
        iter->second.m_fPct = ((double)(coreUser + coreNice + coreSystem) * 100.0) / total;

      iter->second.m_user += coreUser;
      iter->second.m_nice += coreNice;
      iter->second.m_system += coreSystem;
      iter->second.m_idle += coreIdle;
      iter->second.m_io += coreIO;

      user   += coreUser;
      nice   += coreNice;
      system += coreSystem;
      idle   += coreIdle;
      io     += coreIO;
    }
  }
  free(cptimes);
#else
  if (m_fProcStat == NULL)
    return false;

#ifdef TARGET_ANDROID
  // Just another (vanilla) NDK quirk:
  // rewind + fflush do not actually flush the buffers,
  // the same initial content is returned rather than re-read
  fclose(m_fProcStat);
  m_fProcStat = fopen("/proc/stat", "r");
#else
  rewind(m_fProcStat);
  fflush(m_fProcStat);
#endif

  char buf[256];
  if (!fgets(buf, sizeof(buf), m_fProcStat))
    return false;

  int num = sscanf(buf, "cpu %llu %llu %llu %llu %llu %*s\n", &user, &nice, &system, &idle, &io);
  if (num < 5)
    io = 0;

  while (fgets(buf, sizeof(buf), m_fProcStat) && num >= 4)
  {
    unsigned long long coreUser, coreNice, coreSystem, coreIdle, coreIO;
    int nCpu=0;
    num = sscanf(buf, "cpu%d %llu %llu %llu %llu %llu %*s\n", &nCpu, &coreUser, &coreNice, &coreSystem, &coreIdle, &coreIO);
    if (num < 6)
      coreIO = 0;

    std::map<int, CoreInfo>::iterator iter = m_cores.find(nCpu);
    if (num > 4 && iter != m_cores.end())
    {
      coreUser -= iter->second.m_user;
      coreNice -= iter->second.m_nice;
      coreSystem -= iter->second.m_system;
      coreIdle -= iter->second.m_idle;
      coreIO -= iter->second.m_io;

      double total = (double)(coreUser + coreNice + coreSystem + coreIdle + coreIO);
      if(total == 0.0f)
        iter->second.m_fPct = 0.0f;
      else
        iter->second.m_fPct = ((double)(coreUser + coreNice + coreSystem) * 100.0) / total;

      iter->second.m_user += coreUser;
      iter->second.m_nice += coreNice;
      iter->second.m_system += coreSystem;
      iter->second.m_idle += coreIdle;
      iter->second.m_io += coreIO;
    }
  }
#endif

  return true;
}

std::string CCPUInfo::GetCoresUsageString() const
{
  std::string strCores;
  if (!m_cores.empty())
  {
    for (std::map<int, CoreInfo>::const_iterator it = m_cores.begin(); it != m_cores.end(); ++it)
    {
      if (!strCores.empty())
        strCores += ' ';
      if (it->second.m_fPct < 10.0)
        strCores += StringUtils::Format("#%d: %1.1f%%", it->first, it->second.m_fPct);
      else
        strCores += StringUtils::Format("#%d: %3.0f%%", it->first, it->second.m_fPct);
    }
  }
  else
  {
    strCores += StringUtils::Format("%3.0f%%", double(m_lastUsedPercentage));
  }
  return strCores;
}

void CCPUInfo::ReadCPUFeatures()
{
#ifdef TARGET_WINDOWS
#ifndef _M_ARM
  int CPUInfo[4]; // receives EAX, EBX, ECD and EDX in that order

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

  __cpuid(CPUInfo, 0x80000000);
  int MaxExtInfoType = CPUInfo[0];

  if (MaxExtInfoType >= CPUID_INFOTYPE_EXTENDED)
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
#endif // ! _M_ARM
#elif defined(TARGET_DARWIN)
  #if defined(__ppc__)
    m_cpuFeatures |= CPU_FEATURE_ALTIVEC;
  #elif defined(TARGET_DARWIN_EMBEDDED)
  #else
    size_t len = 512 - 1; // '-1' for trailing space
    char buffer[512] ={0};

    if (sysctlbyname("machdep.cpu.features", &buffer, &len, NULL, 0) == 0)
    {
      strcat(buffer, " ");
      if (strstr(buffer,"MMX "))
        m_cpuFeatures |= CPU_FEATURE_MMX;
      if (strstr(buffer,"MMXEXT "))
        m_cpuFeatures |= CPU_FEATURE_MMX2;
      if (strstr(buffer,"SSE "))
        m_cpuFeatures |= CPU_FEATURE_SSE;
      if (strstr(buffer,"SSE2 "))
        m_cpuFeatures |= CPU_FEATURE_SSE2;
      if (strstr(buffer,"SSE3 "))
        m_cpuFeatures |= CPU_FEATURE_SSE3;
      if (strstr(buffer,"SSSE3 "))
        m_cpuFeatures |= CPU_FEATURE_SSSE3;
      if (strstr(buffer,"SSE4.1 "))
        m_cpuFeatures |= CPU_FEATURE_SSE4;
      if (strstr(buffer,"SSE4.2 "))
        m_cpuFeatures |= CPU_FEATURE_SSE42;
      if (strstr(buffer,"3DNOW "))
        m_cpuFeatures |= CPU_FEATURE_3DNOW;
      if (strstr(buffer,"3DNOWEXT "))
       m_cpuFeatures |= CPU_FEATURE_3DNOWEXT;
    }
    else
      m_cpuFeatures |= CPU_FEATURE_MMX;
  #endif
#elif defined(LINUX)
// empty on purpose, the implementation is in the constructor
#elif !defined(__powerpc__) && !defined(__ppc__) && !defined(__arm__) && !defined(__aarch64__)
  m_cpuFeatures |= CPU_FEATURE_MMX;
#elif defined(__powerpc__) || defined(__ppc__)
  m_cpuFeatures |= CPU_FEATURE_ALTIVEC;
#endif
}

bool CCPUInfo::HasNeon()
{
  static int has_neon = -1;
#if defined (TARGET_ANDROID)
  if (has_neon == -1)
    has_neon = (CAndroidFeatures::HasNeon()) ? 1 : 0;

#elif defined(TARGET_DARWIN_EMBEDDED)
  has_neon = 1;

#elif defined(TARGET_LINUX) && defined(HAS_NEON)
#if defined(__LP64__)
  has_neon = 1;
#else
  if (has_neon == -1)
  {
    has_neon = 0;
    // why are we not looking at the Features in
    // /proc/cpuinfo for neon ?
    int fd = open("/proc/self/auxv", O_RDONLY);
    if (fd >= 0)
    {
      Elf32_auxv_t auxv;
      while (read(fd, &auxv, sizeof(Elf32_auxv_t)) == sizeof(Elf32_auxv_t))
      {
        if (auxv.a_type == AT_HWCAP)
        {
          has_neon = (auxv.a_un.a_val & HWCAP_NEON) ? 1 : 0;
          break;
        }
      }
      close(fd);
    }
  }
#endif

#endif

  return has_neon == 1;
}

CCPUInfo g_cpuInfo;
