/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <limits.h>

#include "threads/SystemClock.h"
#include "SystemInfo.h"
#ifndef TARGET_POSIX
#include <conio.h>
#else
#include <sys/utsname.h>
#endif
#include "CompileInfo.h"
#include "ServiceBroker.h"
#include "filesystem/CurlFile.h"
#include "filesystem/File.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "network/Network.h"
#include "platform/Filesystem.h"
#include "rendering/RenderSystem.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/CPUInfo.h"
#include "utils/log.h"

#ifdef TARGET_WINDOWS
#include <dwmapi.h>
#include "utils/CharsetConverter.h"
#include <VersionHelpers.h>

#ifdef TARGET_WINDOWS_STORE
#include <winrt/Windows.Security.ExchangeActiveSyncProvisioning.h>
#include <winrt/Windows.System.Profile.h>

using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::Security::ExchangeActiveSyncProvisioning;
using namespace winrt::Windows::System;
using namespace winrt::Windows::System::Profile;
#endif
#include <wincrypt.h>
#include "platform/win32/CharsetConverter.h"
#endif
#if defined(TARGET_DARWIN)
#include "platform/darwin/DarwinUtils.h"
#endif
#include "powermanagement/PowerManager.h"
#include "utils/StringUtils.h"
#include "utils/XMLUtils.h"
#if defined(TARGET_ANDROID)
#include <androidjni/Build.h>
#endif

/* Platform identification */
#if defined(TARGET_DARWIN)
#include <Availability.h>
#include <mach-o/arch.h>
#include <sys/sysctl.h>
#include "utils/auto_buffer.h"
#elif defined(TARGET_ANDROID)
#include <android/api-level.h>
#include <sys/system_properties.h>
#elif defined(TARGET_FREEBSD)
#include <sys/param.h>
#elif defined(TARGET_LINUX)
#include <linux/version.h>
#include "utils/SysfsUtils.h"
#endif

#include <system_error>

/* Expand macro before stringify */
#define STR_MACRO(x) #x
#define XSTR_MACRO(x) STR_MACRO(x)

using namespace XFILE;

#ifdef TARGET_WINDOWS_DESKTOP
static bool sysGetVersionExWByRef(OSVERSIONINFOEXW& osVerInfo)
{
  ZeroMemory(&osVerInfo, sizeof(osVerInfo));
  osVerInfo.dwOSVersionInfoSize = sizeof(osVerInfo);

  typedef NTSTATUS(__stdcall *RtlGetVersionPtr)(RTL_OSVERSIONINFOEXW* pOsInfo);
  static HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
  if (hNtDll != NULL)
  {
    static RtlGetVersionPtr RtlGetVer = (RtlGetVersionPtr) GetProcAddress(hNtDll, "RtlGetVersion");
    if (RtlGetVer && RtlGetVer(&osVerInfo) == 0)
      return true;
  }
  // failed to get OS information directly from ntdll.dll
  // use GetVersionExW() as fallback
  // note: starting from Windows 8.1 GetVersionExW() may return unfaithful information
  if (GetVersionExW((OSVERSIONINFOW*) &osVerInfo) != 0)
      return true;

  ZeroMemory(&osVerInfo, sizeof(osVerInfo));
  return false;
}
#endif // TARGET_WINDOWS_DESKTOP

#if defined(TARGET_LINUX) && !defined(TARGET_ANDROID)
static std::string getValueFromOs_release(std::string key)
{
  FILE* os_rel = fopen("/etc/os-release", "r");
  if (!os_rel)
    return "";

  char* buf = new char[10 * 1024]; // more than enough
  size_t len = fread(buf, 1, 10 * 1024, os_rel);
  fclose(os_rel);
  if (len == 0)
  {
    delete[] buf;
    return "";
  }

  std::string content(buf, len);
  delete[] buf;

  // find begin of value string
  size_t valStart = 0, seachPos;
  key += '=';
  if (content.compare(0, key.length(), key) == 0)
    valStart = key.length();
  else
  {
    key = "\n" + key;
    seachPos = 0;
    do
    {
      seachPos = content.find(key, seachPos);
      if (seachPos == std::string::npos)
        return "";
      if (seachPos == 0 || content[seachPos - 1] != '\\')
        valStart = seachPos + key.length();
      else
        seachPos++;
    } while (valStart == 0);
  }

  if (content[valStart] == '\n')
    return "";

  // find end of value string
  seachPos = valStart;
  do
  {
    seachPos = content.find('\n', seachPos + 1);
  } while (seachPos != std::string::npos && content[seachPos - 1] == '\\');
  size_t const valEnd = seachPos;

  std::string value(content, valStart, valEnd - valStart);
  if (value.empty())
    return value;

  // remove quotes
  if (value[0] == '\'' || value[0] == '"')
  {
    if (value.length() < 2)
      return value;
    size_t qEnd = value.rfind(value[0]);
    if (qEnd != std::string::npos)
    {
      value.erase(qEnd);
      value.erase(0, 1);
    }
  }

  // unescape characters
  for (size_t slashPos = value.find('\\'); slashPos < value.length() - 1; slashPos = value.find('\\', slashPos))
  {
    if (value[slashPos + 1] == '\n')
      value.erase(slashPos, 2);
    else
    {
      value.erase(slashPos, 1);
      slashPos++; // skip unescaped character
    }
  }

  return value;
}

enum lsb_rel_info_type
{
  lsb_rel_distributor,
  lsb_rel_description,
  lsb_rel_release,
  lsb_rel_codename
};

static std::string getValueFromLsb_release(enum lsb_rel_info_type infoType)
{
  std::string key, command("unset PYTHONHOME; unset PYTHONPATH; lsb_release ");
  switch (infoType)
  {
  case lsb_rel_distributor:
    command += "-i";
    key = "Distributor ID:\t";
    break;
  case lsb_rel_description:
    command += "-d";
    key = "Description:\t";
    break;
  case lsb_rel_release:
    command += "-r";
    key = "Release:\t";
    break;
  case lsb_rel_codename:
    command += "-c";
    key = "Codename:\t";
    break;
  default:
    return "";
  }
  command += " 2>/dev/null";
  FILE* lsb_rel = popen(command.c_str(), "r");
  if (lsb_rel == NULL)
    return "";

  char buf[300]; // more than enough
  if (fgets(buf, 300, lsb_rel) == NULL)
  {
    pclose(lsb_rel);
    return "";
  }
  pclose(lsb_rel);

  std::string response(buf);
  if (response.compare(0, key.length(), key) != 0)
    return "";

  return response.substr(key.length(), response.find('\n') - key.length());
}
#endif // TARGET_LINUX && !TARGET_ANDROID

CSysInfo g_sysinfo;

CSysInfoJob::CSysInfoJob() = default;

bool CSysInfoJob::DoWork()
{
  m_info.systemUptime      = GetSystemUpTime(false);
  m_info.systemTotalUptime = GetSystemUpTime(true);
  m_info.internetState     = GetInternetState();
  m_info.videoEncoder      = GetVideoEncoder();
  m_info.cpuFrequency =
      StringUtils::Format("%4.0f MHz", CServiceBroker::GetCPUInfo()->GetCPUFrequency());
  m_info.osVersionInfo     = CSysInfo::GetOsPrettyNameWithVersion() + " (kernel: " + CSysInfo::GetKernelName() + " " + CSysInfo::GetKernelVersionFull() + ")";
  m_info.macAddress        = GetMACAddress();
  m_info.batteryLevel      = GetBatteryLevel();
  return true;
}

const CSysData &CSysInfoJob::GetData() const
{
  return m_info;
}

CSysData::INTERNET_STATE CSysInfoJob::GetInternetState()
{
  // Internet connection state!
  XFILE::CCurlFile http;
  if (http.IsInternet())
    return CSysData::CONNECTED;
  return CSysData::DISCONNECTED;
}

std::string CSysInfoJob::GetMACAddress()
{
  CNetworkInterface* iface = CServiceBroker::GetNetwork().GetFirstConnectedInterface();
  if (iface)
    return iface->GetMacAddress();

  return "";
}

std::string CSysInfoJob::GetVideoEncoder()
{
  return "GPU: " + CServiceBroker::GetRenderSystem()->GetRenderRenderer();
}

std::string CSysInfoJob::GetBatteryLevel()
{
  return StringUtils::Format("%d%%", CServiceBroker::GetPowerManager().BatteryLevel());
}

bool CSysInfoJob::SystemUpTime(int iInputMinutes, int &iMinutes, int &iHours, int &iDays)
{
  iHours = 0; iDays = 0;
  iMinutes = iInputMinutes;
  if (iMinutes >= 60) // Hour's
  {
    iHours = iMinutes / 60;
    iMinutes = iMinutes - (iHours *60);
  }
  if (iHours >= 24) // Days
  {
    iDays = iHours / 24;
    iHours = iHours - (iDays * 24);
  }
  return true;
}

std::string CSysInfoJob::GetSystemUpTime(bool bTotalUptime)
{
  std::string strSystemUptime;
  int iInputMinutes, iMinutes,iHours,iDays;

  if(bTotalUptime)
  {
    //Total Uptime
    iInputMinutes = g_sysinfo.GetTotalUptime() + ((int)(XbmcThreads::SystemClockMillis() / 60000));
  }
  else
  {
    //Current UpTime
    iInputMinutes = (int)(XbmcThreads::SystemClockMillis() / 60000);
  }

  SystemUpTime(iInputMinutes,iMinutes, iHours, iDays);
  if (iDays > 0)
  {
    strSystemUptime = StringUtils::Format("%i %s, %i %s, %i %s",
                                          iDays, g_localizeStrings.Get(12393).c_str(),
                                          iHours, g_localizeStrings.Get(12392).c_str(),
                                          iMinutes, g_localizeStrings.Get(12391).c_str());
  }
  else if (iDays == 0 && iHours >= 1 )
  {
    strSystemUptime = StringUtils::Format("%i %s, %i %s",
                                          iHours, g_localizeStrings.Get(12392).c_str(),
                                          iMinutes, g_localizeStrings.Get(12391).c_str());
  }
  else if (iDays == 0 && iHours == 0 &&  iMinutes >= 0)
  {
    strSystemUptime = StringUtils::Format("%i %s",
                                          iMinutes, g_localizeStrings.Get(12391).c_str());
  }
  return strSystemUptime;
}

std::string CSysInfo::TranslateInfo(int info) const
{
  switch(info)
  {
  case SYSTEM_VIDEO_ENCODER_INFO:
    return m_info.videoEncoder;
  case NETWORK_MAC_ADDRESS:
    return m_info.macAddress;
  case SYSTEM_OS_VERSION_INFO:
    return m_info.osVersionInfo;
  case SYSTEM_CPUFREQUENCY:
    return m_info.cpuFrequency;
  case SYSTEM_UPTIME:
    return m_info.systemUptime;
  case SYSTEM_TOTALUPTIME:
    return m_info.systemTotalUptime;
  case SYSTEM_INTERNET_STATE:
    if (m_info.internetState == CSysData::CONNECTED)
      return g_localizeStrings.Get(13296);
    else
      return g_localizeStrings.Get(13297);
  case SYSTEM_BATTERY_LEVEL:
    return m_info.batteryLevel;
  default:
    return "";
  }
}

void CSysInfo::Reset()
{
  m_info.Reset();
}

CSysInfo::CSysInfo(void) : CInfoLoader(15 * 1000)
{
  memset(MD5_Sign, 0, sizeof(MD5_Sign));
  m_iSystemTimeTotalUp = 0;
}

CSysInfo::~CSysInfo() = default;

bool CSysInfo::Load(const TiXmlNode *settings)
{
  if (settings == NULL)
    return false;

  const TiXmlElement *pElement = settings->FirstChildElement("general");
  if (pElement)
    XMLUtils::GetInt(pElement, "systemtotaluptime", m_iSystemTimeTotalUp, 0, INT_MAX);

  return true;
}

bool CSysInfo::Save(TiXmlNode *settings) const
{
  if (settings == NULL)
    return false;

  TiXmlNode *generalNode = settings->FirstChild("general");
  if (generalNode == NULL)
  {
    TiXmlElement generalNodeNew("general");
    generalNode = settings->InsertEndChild(generalNodeNew);
    if (generalNode == NULL)
      return false;
  }
  XMLUtils::SetInt(generalNode, "systemtotaluptime", m_iSystemTimeTotalUp);

  return true;
}

const std::string& CSysInfo::GetAppName(void)
{
  assert(CCompileInfo::GetAppName() != NULL);
  static const std::string appName(CCompileInfo::GetAppName());

  return appName;
}

bool CSysInfo::GetDiskSpace(std::string drive,int& iTotal, int& iTotalFree, int& iTotalUsed, int& iPercentFree, int& iPercentUsed)
{
  using namespace KODI::PLATFORM::FILESYSTEM;

  space_info total = { 0 };
  std::error_code ec;

  // None of this makes sense but the idea of total space
  // makes no sense on any system really.
  // Return space for / or for C: as it's correct in a sense
  // and not much worse than trying to count a total for different
  // drives/mounts
  if (drive.empty() || drive == "*")
  {
#if defined(TARGET_WINDOWS)
    drive = "C";
#elif defined(TARGET_POSIX)
    drive = "/";
#endif
  }

#ifdef TARGET_WINDOWS_DESKTOP
  using KODI::PLATFORM::WINDOWS::ToW;
  UINT uidriveType = GetDriveType(ToW(drive + ":\\").c_str());
  if (uidriveType != DRIVE_UNKNOWN && uidriveType != DRIVE_NO_ROOT_DIR)
    total = space(drive + ":\\", ec);
#elif defined(TARGET_POSIX)
  total = space(drive, ec);
#endif
  if (ec.value() != 0)
    return false;

  iTotal = static_cast<int>(total.capacity / MB);
  iTotalFree = static_cast<int>(total.free / MB);
  iTotalUsed = iTotal - iTotalFree;
  if (total.capacity > 0)
    iPercentUsed = static_cast<int>(100.0f * (total.capacity - total.free) / total.capacity + 0.5f);
  else
    iPercentUsed = 0;

  iPercentFree = 100 - iPercentUsed;

  return true;
}

std::string CSysInfo::GetKernelName(bool emptyIfUnknown /*= false*/)
{
  static std::string kernelName;
  if (kernelName.empty())
  {
#if defined(TARGET_WINDOWS_DESKTOP)
    OSVERSIONINFOEXW osvi;
    if (sysGetVersionExWByRef(osvi) && osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
      kernelName = "Windows NT";
#elif defined(TARGET_WINDOWS_STORE)
    auto e = EasClientDeviceInformation();
    auto os = e.OperatingSystem();
    g_charsetConverter.wToUTF8(std::wstring(os.c_str()), kernelName);
#elif defined(TARGET_POSIX)
    struct utsname un;
    if (uname(&un) == 0)
      kernelName.assign(un.sysname);
#endif // defined(TARGET_POSIX)

    if (kernelName.empty())
      kernelName = "Unknown kernel"; // can't detect
  }

  if (emptyIfUnknown && kernelName == "Unknown kernel")
    return "";

  return kernelName;
}

std::string CSysInfo::GetKernelVersionFull(void)
{
  static std::string kernelVersionFull;
  if (!kernelVersionFull.empty())
    return kernelVersionFull;

#if defined(TARGET_WINDOWS_DESKTOP)
  OSVERSIONINFOEXW osvi;
  if (sysGetVersionExWByRef(osvi))
    kernelVersionFull = StringUtils::Format("%d.%d.%d", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
#elif  defined(TARGET_WINDOWS_STORE)
  // get the system version number
  auto sv = AnalyticsInfo::VersionInfo().DeviceFamilyVersion();
  wchar_t* end;
  unsigned long long  v = wcstoull(sv.c_str(), &end, 10);
  unsigned long long v1 = (v & 0xFFFF000000000000L) >> 48;
  unsigned long long v2 = (v & 0x0000FFFF00000000L) >> 32;
  unsigned long long v3 = (v & 0x00000000FFFF0000L) >> 16;
  kernelVersionFull = StringUtils::Format("%lld.%lld.%lld", v1, v2, v3);

#elif defined(TARGET_POSIX)
  struct utsname un;
  if (uname(&un) == 0)
    kernelVersionFull.assign(un.release);
#endif // defined(TARGET_POSIX)

  if (kernelVersionFull.empty())
    kernelVersionFull = "0.0.0"; // can't detect

  return kernelVersionFull;
}

std::string CSysInfo::GetKernelVersion(void)
{
  static std::string kernelVersionClear;
  if (kernelVersionClear.empty())
  {
    kernelVersionClear = GetKernelVersionFull();
    const size_t erasePos = kernelVersionClear.find_first_not_of("0123456789.");
    if (erasePos != std::string::npos)
      kernelVersionClear.erase(erasePos);
  }

  return kernelVersionClear;
}

std::string CSysInfo::GetOsName(bool emptyIfUnknown /* = false*/)
{
  static std::string osName;
  if (osName.empty())
  {
#if defined (TARGET_WINDOWS)
    osName = GetKernelName() + "-based OS";
#elif defined(TARGET_FREEBSD)
    osName = GetKernelName(true); // FIXME: for FreeBSD OS name is a kernel name
#elif defined(TARGET_DARWIN_IOS)
    osName = "iOS";
#elif defined(TARGET_DARWIN_TVOS)
    osName = "tvOS";
#elif defined(TARGET_DARWIN_OSX)
    osName = "OS X";
#elif defined (TARGET_ANDROID)
    osName = "Android";
#elif defined(TARGET_LINUX)
    osName = getValueFromOs_release("NAME");
    if (osName.empty())
      osName = getValueFromLsb_release(lsb_rel_distributor);
    if (osName.empty())
      osName = getValueFromOs_release("ID");
#endif // defined(TARGET_LINUX)

    if (osName.empty())
      osName = "Unknown OS";
  }

  if (emptyIfUnknown && osName == "Unknown OS")
    return "";

  return osName;
}

std::string CSysInfo::GetOsVersion(void)
{
  static std::string osVersion;
  if (!osVersion.empty())
    return osVersion;

#if defined(TARGET_WINDOWS) || defined(TARGET_FREEBSD)
  osVersion = GetKernelVersion(); // FIXME: for Win32 and FreeBSD OS version is a kernel version
#elif defined(TARGET_DARWIN)
  osVersion = CDarwinUtils::GetVersionString();
#elif defined(TARGET_ANDROID)
  char versionCStr[PROP_VALUE_MAX];
  int propLen = __system_property_get("ro.build.version.release", versionCStr);
  osVersion.assign(versionCStr, (propLen > 0 && propLen <= PROP_VALUE_MAX) ? propLen : 0);

  if (osVersion.empty() || std::string("0123456789").find(versionCStr[0]) == std::string::npos)
    osVersion.clear(); // can't correctly detect Android version
  else
  {
    size_t pointPos = osVersion.find('.');
    if (pointPos == std::string::npos)
      osVersion += ".0.0";
    else if (osVersion.find('.', pointPos + 1) == std::string::npos)
      osVersion += ".0";
  }
#elif defined(TARGET_LINUX)
  osVersion = getValueFromOs_release("VERSION_ID");
  if (osVersion.empty())
    osVersion = getValueFromLsb_release(lsb_rel_release);
#endif // defined(TARGET_LINUX)

  if (osVersion.empty())
    osVersion = "0.0";

  return osVersion;
}

std::string CSysInfo::GetOsPrettyNameWithVersion(void)
{
  static std::string osNameVer;
  if (!osNameVer.empty())
    return osNameVer;

#if defined (TARGET_WINDOWS_DESKTOP)
  OSVERSIONINFOEXW osvi = {};

  osNameVer = "Windows ";
  if (sysGetVersionExWByRef(osvi))
  {
    switch (GetWindowsVersion())
    {
    case WindowsVersionWin7:
      if (osvi.wProductType == VER_NT_WORKSTATION)
        osNameVer.append("7");
      else
        osNameVer.append("Server 2008 R2");
      break;
    case WindowsVersionWin8:
      if (osvi.wProductType == VER_NT_WORKSTATION)
        osNameVer.append("8");
      else
        osNameVer.append("Server 2012");
      break;
    case WindowsVersionWin8_1:
      if (osvi.wProductType == VER_NT_WORKSTATION)
        osNameVer.append("8.1");
      else
        osNameVer.append("Server 2012 R2");
      break;
    case WindowsVersionWin10:
    case WindowsVersionWin10_FCU:
      if (osvi.wProductType == VER_NT_WORKSTATION)
        osNameVer.append("10");
      else
        osNameVer.append("Unknown future server version");
      break;
    case WindowsVersionFuture:
      osNameVer.append("Unknown future version");
      break;
    default:
      osNameVer.append("Unknown version");
      break;
    }

    // Append Service Pack version if any
    if (osvi.wServicePackMajor > 0 || osvi.wServicePackMinor > 0)
    {
      osNameVer.append(StringUtils::Format(" SP%d", osvi.wServicePackMajor));
      if (osvi.wServicePackMinor > 0)
      {
        osNameVer.append(StringUtils::Format(".%d", osvi.wServicePackMinor));
      }
    }
  }
  else
    osNameVer.append(" unknown");
#elif defined(TARGET_WINDOWS_STORE)
  osNameVer = GetKernelName() + " " + GetOsVersion();
#elif defined(TARGET_FREEBSD) || defined(TARGET_DARWIN)
  osNameVer = GetOsName() + " " + GetOsVersion();
#elif defined(TARGET_ANDROID)
  osNameVer = GetOsName() + " " + GetOsVersion() + " API level " +   StringUtils::Format("%d", CJNIBuild::SDK_INT);
#elif defined(TARGET_LINUX)
  osNameVer = getValueFromOs_release("PRETTY_NAME");
  if (osNameVer.empty())
  {
    osNameVer = getValueFromLsb_release(lsb_rel_description);
    std::string osName(GetOsName(true));
    if (!osName.empty() && osNameVer.find(osName) == std::string::npos)
      osNameVer = osName + osNameVer;
    if (osNameVer.empty())
      osNameVer = "Unknown Linux Distribution";
  }

  if (osNameVer.find(GetOsVersion()) == std::string::npos)
    osNameVer += " " + GetOsVersion();
#endif // defined(TARGET_LINUX)

  if (osNameVer.empty())
    osNameVer = "Unknown OS Unknown version";

  return osNameVer;

}

std::string CSysInfo::GetManufacturerName(void)
{
  static std::string manufName;
  static bool inited = false;
  if (!inited)
  {
#if defined(TARGET_ANDROID)
    char deviceCStr[PROP_VALUE_MAX];
    int propLen = __system_property_get("ro.product.manufacturer", deviceCStr);
    manufName.assign(deviceCStr, (propLen > 0 && propLen <= PROP_VALUE_MAX) ? propLen : 0);
#elif defined(TARGET_DARWIN)
    manufName = CDarwinUtils::GetManufacturer();
#elif defined(TARGET_WINDOWS_STORE)
    auto eas = EasClientDeviceInformation();
    auto manufacturer = eas.SystemManufacturer();
    g_charsetConverter.wToUTF8(std::wstring(manufacturer.c_str()), manufName);
#elif defined(TARGET_LINUX)
    if (SysfsUtils::Has("/sys/bus/soc/devices/soc0/family"))
    {
      std::string family;
      SysfsUtils::GetString("/sys/bus/soc/devices/soc0/family", family);
      if (SysfsUtils::Has("/sys/bus/soc/devices/soc0/soc_id"))
      {
        std::string soc_id;
        SysfsUtils::GetString("/sys/bus/soc/devices/soc0/soc_id", soc_id);
        manufName = family + " " + soc_id;
      }
      else
        manufName = family;
    }
#elif defined(TARGET_WINDOWS)
    // We just don't care, might be useful on embedded
#endif
    inited = true;
  }

  return manufName;
}

std::string CSysInfo::GetModelName(void)
{
  static std::string modelName;
  static bool inited = false;
  if (!inited)
  {
#if defined(TARGET_ANDROID)
    char deviceCStr[PROP_VALUE_MAX];
    int propLen = __system_property_get("ro.product.model", deviceCStr);
    modelName.assign(deviceCStr, (propLen > 0 && propLen <= PROP_VALUE_MAX) ? propLen : 0);
#elif defined(TARGET_DARWIN_EMBEDDED)
    modelName = CDarwinUtils::getIosPlatformString();
#elif defined(TARGET_DARWIN_OSX)
    size_t nameLen = 0; // 'nameLen' should include terminating null
    if (sysctlbyname("hw.model", NULL, &nameLen, NULL, 0) == 0 && nameLen > 1)
    {
      XUTILS::auto_buffer buf(nameLen);
      if (sysctlbyname("hw.model", buf.get(), &nameLen, NULL, 0) == 0 && nameLen == buf.size())
        modelName.assign(buf.get(), nameLen - 1); // assign exactly 'nameLen-1' characters to 'modelName'
    }
#elif defined(TARGET_WINDOWS_STORE)
    auto eas = EasClientDeviceInformation();
    auto manufacturer = eas.SystemProductName();
    g_charsetConverter.wToUTF8(std::wstring(manufacturer.c_str()), modelName);
#elif defined(TARGET_LINUX)
    if (SysfsUtils::Has("/sys/bus/soc/devices/soc0/machine"))
      SysfsUtils::GetString("/sys/bus/soc/devices/soc0/machine", modelName);
#elif defined(TARGET_WINDOWS)
    // We just don't care, might be useful on embedded
#endif
    inited = true;
  }

  return modelName;
}

bool CSysInfo::IsAeroDisabled()
{
#ifdef TARGET_WINDOWS_STORE
  return true; // need to review https://msdn.microsoft.com/en-us/library/windows/desktop/aa969518(v=vs.85).aspx
#elif defined(TARGET_WINDOWS)
  BOOL aeroEnabled = FALSE;
  HRESULT res = DwmIsCompositionEnabled(&aeroEnabled);
  if (SUCCEEDED(res))
    return !aeroEnabled;
#endif
  return false;
}

CSysInfo::WindowsVersion CSysInfo::m_WinVer = WindowsVersionUnknown;

bool CSysInfo::IsWindowsVersion(WindowsVersion ver)
{
  if (ver == WindowsVersionUnknown)
    return false;
  return GetWindowsVersion() == ver;
}

bool CSysInfo::IsWindowsVersionAtLeast(WindowsVersion ver)
{
  if (ver == WindowsVersionUnknown)
    return false;
  return GetWindowsVersion() >= ver;
}

CSysInfo::WindowsVersion CSysInfo::GetWindowsVersion()
{
#ifdef TARGET_WINDOWS_DESKTOP
  if (m_WinVer == WindowsVersionUnknown)
  {
    OSVERSIONINFOEXW osvi = {};
    if (sysGetVersionExWByRef(osvi))
    {
      if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
        m_WinVer = WindowsVersionWin7;
      else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2)
        m_WinVer = WindowsVersionWin8;
      else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3)
        m_WinVer = WindowsVersionWin8_1;
      else if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0 && osvi.dwBuildNumber < 16299)
        m_WinVer = WindowsVersionWin10;
      else if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0 && osvi.dwBuildNumber >= 16299)
        m_WinVer = WindowsVersionWin10_FCU;
      /* Insert checks for new Windows versions here */
      else if ( (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion > 3) || osvi.dwMajorVersion > 10)
        m_WinVer = WindowsVersionFuture;
    }
  }
#elif  defined(TARGET_WINDOWS_STORE)
  m_WinVer = WindowsVersionWin10;
#endif // TARGET_WINDOWS
  return m_WinVer;
}

int CSysInfo::GetKernelBitness(void)
{
  static int kernelBitness = -1;
  if (kernelBitness == -1)
  {
#ifdef TARGET_WINDOWS_STORE
    Package package = Package::Current();
    auto arch = package.Id().Architecture();
    switch (arch)
    {
    case ProcessorArchitecture::X86:
      kernelBitness = 32;
      break;
    case ProcessorArchitecture::X64:
      kernelBitness = 64;
      break;
    case ProcessorArchitecture::Arm:
      kernelBitness = 32;
      break;
    case ProcessorArchitecture::Unknown: // not sure what to do here. guess 32 for now
    case ProcessorArchitecture::Neutral:
      kernelBitness = 32;
      break;
    }
#elif defined(TARGET_WINDOWS_DESKTOP)
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM)
      kernelBitness = 32;
    else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
      kernelBitness = 64;
    else
    {
      BOOL isWow64 = FALSE;
      if (IsWow64Process(GetCurrentProcess(), &isWow64) && isWow64) // fallback
        kernelBitness = 64;
    }
#elif defined(TARGET_DARWIN_EMBEDDED)
    // Note: OS X return x86 CPU type without CPU_ARCH_ABI64 flag
    const NXArchInfo* archInfo = NXGetLocalArchInfo();
    if (archInfo)
      kernelBitness = ((archInfo->cputype & CPU_ARCH_ABI64) != 0) ? 64 : 32;
#elif defined(TARGET_POSIX)
    struct utsname un;
    if (uname(&un) == 0)
    {
      std::string machine(un.machine);
      if (machine == "x86_64" || machine == "amd64" || machine == "arm64" || machine == "aarch64" || machine == "ppc64" ||
          machine == "ia64" || machine == "mips64" || machine == "s390x")
        kernelBitness = 64;
      else
        kernelBitness = 32;
    }
#endif
    if (kernelBitness == -1)
      kernelBitness = 0; // can't detect
  }

  return kernelBitness;
}

const std::string& CSysInfo::GetKernelCpuFamily(void)
{
  static std::string kernelCpuFamily;
  if (kernelCpuFamily.empty())
  {
#ifdef TARGET_WINDOWS
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ||
        si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        kernelCpuFamily = "x86";
    else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM)
      kernelCpuFamily = "ARM";
#elif defined(TARGET_DARWIN)
    const NXArchInfo* archInfo = NXGetLocalArchInfo();
    if (archInfo)
    {
      const cpu_type_t cpuType = (archInfo->cputype & ~CPU_ARCH_ABI64); // get CPU family without 64-bit ABI flag
      if (cpuType == CPU_TYPE_I386)
        kernelCpuFamily = "x86";
      else if (cpuType == CPU_TYPE_ARM)
        kernelCpuFamily = "ARM";
      else if (cpuType == CPU_TYPE_POWERPC)
        kernelCpuFamily = "PowerPC";
#ifdef CPU_TYPE_MIPS
      else if (cpuType == CPU_TYPE_MIPS)
        kernelCpuFamily = "MIPS";
#endif // CPU_TYPE_MIPS
    }
#elif defined(TARGET_POSIX)
    struct utsname un;
    if (uname(&un) == 0)
    {
      std::string machine(un.machine);
      if (machine.compare(0, 3, "arm", 3) == 0 || machine.compare(0, 7, "aarch64", 7) == 0)
        kernelCpuFamily = "ARM";
      else if (machine.compare(0, 4, "mips", 4) == 0)
        kernelCpuFamily = "MIPS";
      else if (machine.compare(0, 4, "i686", 4) == 0 || machine == "i386" || machine == "amd64" ||  machine.compare(0, 3, "x86", 3) == 0)
        kernelCpuFamily = "x86";
      else if (machine.compare(0, 4, "s390", 4) == 0)
        kernelCpuFamily = "s390";
      else if (machine.compare(0, 3, "ppc", 3) == 0 || machine.compare(0, 5, "power", 5) == 0)
        kernelCpuFamily = "PowerPC";
    }
#endif
    if (kernelCpuFamily.empty())
      kernelCpuFamily = "unknown CPU family";
  }
  return kernelCpuFamily;
}

int CSysInfo::GetXbmcBitness(void)
{
  return static_cast<int>(sizeof(void*) * 8);
}

bool CSysInfo::HasInternet()
{
  if (m_info.internetState != CSysData::UNKNOWN)
    return m_info.internetState == CSysData::CONNECTED;
  return (m_info.internetState = CSysInfoJob::GetInternetState()) == CSysData::CONNECTED;
}

std::string CSysInfo::GetHddSpaceInfo(int drive, bool shortText)
{
 int percent;
 return GetHddSpaceInfo( percent, drive, shortText);
}

std::string CSysInfo::GetHddSpaceInfo(int& percent, int drive, bool shortText)
{
  int total, totalFree, totalUsed, percentFree, percentused;
  std::string strRet;
  percent = 0;
  if (g_sysinfo.GetDiskSpace("", total, totalFree, totalUsed, percentFree, percentused))
  {
    if (shortText)
    {
      switch(drive)
      {
        case SYSTEM_FREE_SPACE:
          percent = percentFree;
          break;
        case SYSTEM_USED_SPACE:
          percent = percentused;
          break;
      }
    }
    else
    {
      switch(drive)
      {
      case SYSTEM_FREE_SPACE:
        strRet = StringUtils::Format("%i MB %s", totalFree, g_localizeStrings.Get(160).c_str());
        break;
      case SYSTEM_USED_SPACE:
        strRet = StringUtils::Format("%i MB %s", totalUsed, g_localizeStrings.Get(20162).c_str());
        break;
      case SYSTEM_TOTAL_SPACE:
        strRet = StringUtils::Format("%i MB %s", total, g_localizeStrings.Get(20161).c_str());
        break;
      case SYSTEM_FREE_SPACE_PERCENT:
        strRet = StringUtils::Format("%i %% %s", percentFree, g_localizeStrings.Get(160).c_str());
        break;
      case SYSTEM_USED_SPACE_PERCENT:
        strRet = StringUtils::Format("%i %% %s", percentused, g_localizeStrings.Get(20162).c_str());
        break;
      }
    }
  }
  else
  {
    if (shortText)
      strRet = g_localizeStrings.Get(10006); // N/A
    else
      strRet = g_localizeStrings.Get(10005); // Not available
  }
  return strRet;
}

std::string CSysInfo::GetUserAgent()
{
  static std::string result;
  if (!result.empty())
    return result;

  result = GetAppName() + "/" + CSysInfo::GetVersionShort() + " (";
#if defined(TARGET_WINDOWS)
  result += GetKernelName() + " " + GetKernelVersion();
#ifndef TARGET_WINDOWS_STORE
  BOOL bIsWow = FALSE;
  if (IsWow64Process(GetCurrentProcess(), &bIsWow) && bIsWow)
      result.append("; WOW64");
  else
#endif
  {
    SYSTEM_INFO si = {};
    GetSystemInfo(&si);
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
      result.append("; Win64; x64");
    else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
      result.append("; Win64; IA64");
    else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM)
      result.append("; ARM");
  }
#elif defined(TARGET_DARWIN)
#if defined(TARGET_DARWIN_EMBEDDED)
  std::string iDevStr(GetModelName()); // device model name with number of model version
  size_t iDevStrDigit = iDevStr.find_first_of("0123456789");
  std::string iDev(iDevStr, 0, iDevStrDigit);  // device model name without number
  if (iDevStrDigit == 0)
    iDev = "unknown";
  result += iDev + "; ";
  std::string iOSVersion(GetOsVersion());
  size_t lastDotPos = iOSVersion.rfind('.');
  if (lastDotPos != std::string::npos && iOSVersion.find('.') != lastDotPos
      && iOSVersion.find_first_not_of('0', lastDotPos + 1) == std::string::npos)
    iOSVersion.erase(lastDotPos);
  StringUtils::Replace(iOSVersion, '.', '_');
  if (iDev == "AppleTV")
  {
    // check if it's ATV4 (AppleTV5,3) or later
    auto modelMajorNumberEndPos = iDevStr.find_first_of(',', iDevStrDigit);
    std::string s{iDevStr, iDevStrDigit, modelMajorNumberEndPos - iDevStrDigit};
    if (stoi(s) >= 5)
      result += "CPU TVOS";
    else
      result += "CPU OS";
  }
  else if (iDev == "iPad")
    result += "CPU OS";
  else
    result += "CPU iPhone OS ";
  result += iOSVersion + " like Mac OS X";
#else
  result += "Macintosh; ";
  std::string cpuFam(GetBuildTargetCpuFamily());
  if (cpuFam == "x86")
    result += "Intel ";
  else if (cpuFam == "PowerPC")
    result += "PPC ";
  result += "Mac OS X ";
  std::string OSXVersion(GetOsVersion());
  StringUtils::Replace(OSXVersion, '.', '_');
  result += OSXVersion;
#endif
#elif defined(TARGET_ANDROID)
  result += "Linux; Android ";
  std::string versionStr(GetOsVersion());
  const size_t verLen = versionStr.length();
  if (verLen >= 2 && versionStr.compare(verLen - 2, 2, ".0", 2) == 0)
    versionStr.erase(verLen - 2); // remove last ".0" if any
  result += versionStr;
  std::string deviceInfo(GetModelName());

  char buildId[PROP_VALUE_MAX];
  int propLen = __system_property_get("ro.build.id", buildId);
  if (propLen > 0 && propLen <= PROP_VALUE_MAX)
  {
    if (!deviceInfo.empty())
      deviceInfo += " ";
    deviceInfo += "Build/";
    deviceInfo.append(buildId, propLen);
  }

  if (!deviceInfo.empty())
    result += "; " + deviceInfo;
#elif defined(TARGET_POSIX)
  result += "X11; ";
  struct utsname un;
  if (uname(&un) == 0)
  {
    std::string cpuStr(un.machine);
    if (cpuStr == "x86_64" && GetXbmcBitness() == 32)
      cpuStr = "i686 (x86_64)";
    result += un.sysname;
    result += " ";
    result += cpuStr;
  }
  else
    result += "Unknown";
#else
  result += "Unknown";
#endif
  result += ")";

  if (GetAppName() != "Kodi")
    result += " Kodi_Fork_" + GetAppName() + "/1.0"; // default fork number is '1.0', replace it with actual number if necessary

#ifdef TARGET_LINUX
  // Add distribution name
  std::string linuxOSName(GetOsName(true));
  if (!linuxOSName.empty())
    result += " " + linuxOSName + "/" + GetOsVersion();
#endif

#ifdef TARGET_RASPBERRY_PI
  result += " HW_RaspberryPi/1.0";
#elif defined (TARGET_DARWIN_EMBEDDED)
  std::string iDevVer;
  if (iDevStrDigit == std::string::npos)
    iDevVer = "0.0";
  else
    iDevVer.assign(iDevStr, iDevStrDigit, std::string::npos);
  StringUtils::Replace(iDevVer, ',', '.');
  result += " HW_" + iDev + "/" + iDevVer;
#endif
  // add more device IDs here if needed.
  // keep only one device ID in result! Form:
  // result += " HW_" + "deviceID" + "/" + "1.0"; // '1.0' if device has no version

#if defined(TARGET_ANDROID)
  // Android has no CPU string by default, so add it as additional parameter
  struct utsname un1;
  if (uname(&un1) == 0)
  {
    std::string cpuStr(un1.machine);
    StringUtils::Replace(cpuStr, ' ', '_');
    result += " Sys_CPU/" + cpuStr;
  }
#endif

  result += " App_Bitness/" + StringUtils::Format("%d", GetXbmcBitness());

  std::string fullVer(CSysInfo::GetVersion());
  StringUtils::Replace(fullVer, ' ', '-');
  result += " Version/" + fullVer;

  return result;
}

std::string CSysInfo::GetDeviceName()
{
  std::string friendlyName = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SERVICES_DEVICENAME);
  if (StringUtils::EqualsNoCase(friendlyName, CCompileInfo::GetAppName()))
  {
    std::string hostname("[unknown]");
    CServiceBroker::GetNetwork().GetHostName(hostname);
    return StringUtils::Format("%s (%s)", friendlyName.c_str(), hostname.c_str());
  }

  return friendlyName;
}

// Version string MUST NOT contain spaces.  It is used
// in the HTTP request user agent.
std::string CSysInfo::GetVersionShort()
{
  if (strlen(CCompileInfo::GetSuffix()) == 0)
    return StringUtils::Format("%d.%d", CCompileInfo::GetMajor(), CCompileInfo::GetMinor());
  else
    return StringUtils::Format("%d.%d-%s", CCompileInfo::GetMajor(), CCompileInfo::GetMinor(), CCompileInfo::GetSuffix());
}

std::string CSysInfo::GetVersion()
{
  return GetVersionShort() + " Git:" + CCompileInfo::GetSCMID();
}

std::string CSysInfo::GetBuildDate()
{
  return CCompileInfo::GetBuildDate();
}

std::string CSysInfo::GetBuildTargetPlatformName(void)
{
#if defined(TARGET_DARWIN_OSX)
  return "OS X";
#elif defined(TARGET_DARWIN_IOS)
  return "iOS";
#elif defined(TARGET_DARWIN_TVOS)
  return "tvOS";
#elif defined(TARGET_FREEBSD)
  return "FreeBSD";
#elif defined(TARGET_ANDROID)
  return "Android";
#elif defined(TARGET_LINUX)
  return "Linux";
#elif defined(TARGET_WINDOWS)
#ifdef NTDDI_VERSION
  return "Windows NT";
#else // !NTDDI_VERSION
  return "unknown Win32 platform";
#endif // !NTDDI_VERSION
#else
  return "unknown platform";
#endif
}

std::string CSysInfo::GetBuildTargetPlatformVersion(void)
{
#if defined(TARGET_DARWIN_OSX)
  return XSTR_MACRO(__MAC_OS_X_VERSION_MIN_REQUIRED);
#elif defined(TARGET_DARWIN_IOS)
  return XSTR_MACRO(__IPHONE_OS_VERSION_MIN_REQUIRED);
#elif defined(TARGET_DARWIN_TVOS)
  return XSTR_MACRO(__TV_OS_VERSION_MIN_REQUIRED);
#elif defined(TARGET_FREEBSD)
  return XSTR_MACRO(__FreeBSD_version);
#elif defined(TARGET_ANDROID)
  return "API level " XSTR_MACRO(__ANDROID_API__);
#elif defined(TARGET_LINUX)
  return XSTR_MACRO(LINUX_VERSION_CODE);
#elif defined(TARGET_WINDOWS)
#ifdef NTDDI_VERSION
  return XSTR_MACRO(NTDDI_VERSION);
#else // !NTDDI_VERSION
  return "(unknown Win32 platform)";
#endif // !NTDDI_VERSION
#else
  return "(unknown platform)";
#endif
}

std::string CSysInfo::GetBuildTargetPlatformVersionDecoded(void)
{
#if defined(TARGET_DARWIN_OSX)
#if defined(MAC_OS_X_VERSION_10_10) && __MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10
  if (__MAC_OS_X_VERSION_MIN_REQUIRED % 10)
    return StringUtils::Format("version %d.%d", (__MAC_OS_X_VERSION_MIN_REQUIRED / 1000) % 100, (__MAC_OS_X_VERSION_MIN_REQUIRED / 10) % 100);
  else
    return StringUtils::Format("version %d.%d.%d", (__MAC_OS_X_VERSION_MIN_REQUIRED / 1000) % 100,
    (__MAC_OS_X_VERSION_MIN_REQUIRED / 10) % 100, __MAC_OS_X_VERSION_MIN_REQUIRED % 10);
#else  // defined(MAC_OS_X_VERSION_10_10) && __MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10
  if (__MAC_OS_X_VERSION_MIN_REQUIRED % 10)
    return StringUtils::Format("version %d.%d", (__MAC_OS_X_VERSION_MIN_REQUIRED / 100) % 100, (__MAC_OS_X_VERSION_MIN_REQUIRED / 10) % 10);
  else
    return StringUtils::Format("version %d.%d.%d", (__MAC_OS_X_VERSION_MIN_REQUIRED / 100) % 100,
      (__MAC_OS_X_VERSION_MIN_REQUIRED / 10) % 10, __MAC_OS_X_VERSION_MIN_REQUIRED % 10);
#endif // defined(MAC_OS_X_VERSION_10_10) && __MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10
#elif defined(TARGET_DARWIN_EMBEDDED)
  std::string versionStr = GetBuildTargetPlatformVersion();
  static const int major = (std::stoi(versionStr) / 10000) % 100;
  static const int minor = (std::stoi(versionStr) / 100) % 100;
  static const int rev = std::stoi(versionStr) % 100;
  return StringUtils::Format("version %d.%d.%d", major, minor, rev);
#elif defined(TARGET_FREEBSD)
  // FIXME: should works well starting from FreeBSD 8.1
  static const int major = (__FreeBSD_version / 100000) % 100;
  static const int minor = (__FreeBSD_version / 1000) % 100;
  static const int Rxx = __FreeBSD_version % 1000;
  if ((major < 9 && Rxx == 0))
    return StringUtils::Format("version %d.%d-RELEASE", major, minor);
  if (Rxx >= 500)
    return StringUtils::Format("version %d.%d-STABLE", major, minor);

  return StringUtils::Format("version %d.%d-CURRENT", major, minor);
#elif defined(TARGET_ANDROID)
  return "API level " XSTR_MACRO(__ANDROID_API__);
#elif defined(TARGET_LINUX)
  return StringUtils::Format("version %d.%d.%d", (LINUX_VERSION_CODE >> 16) & 0xFF , (LINUX_VERSION_CODE >> 8) & 0xFF, LINUX_VERSION_CODE & 0xFF);
#elif defined(TARGET_WINDOWS)
#ifdef NTDDI_VERSION
  std::string version(StringUtils::Format("version %d.%d", int(NTDDI_VERSION >> 24) & 0xFF, int(NTDDI_VERSION >> 16) & 0xFF));
  if (SPVER(NTDDI_VERSION))
    version += StringUtils::Format(" SP%d", int(SPVER(NTDDI_VERSION)));
  return version;
#else // !NTDDI_VERSION
  return "(unknown Win32 platform)";
#endif // !NTDDI_VERSION
#else
  return "(unknown platform)";
#endif
}

std::string CSysInfo::GetBuildTargetCpuFamily(void)
{
#if defined(__thumb__) || defined(_M_ARMT)
  return "ARM (Thumb)";
#elif defined(__arm__) || defined(_M_ARM) || defined (__aarch64__)
  return "ARM";
#elif defined(__mips__) || defined(mips) || defined(__mips)
  return "MIPS";
#elif defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64) || \
   defined(i386) || defined(__i386) || defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(_M_IX86) || defined(_X86_)
  return "x86";
#elif defined(__s390x__)
  return "s390";
#elif defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || defined(__ppc__) || defined(__ppc64__) || defined(_M_PPC)
  return "PowerPC";
#else
  return "unknown CPU family";
#endif
}

std::string CSysInfo::GetUsedCompilerNameAndVer(void)
{
#if defined(__clang__)
#ifdef __clang_version__
  return "Clang " __clang_version__;
#else // ! __clang_version__
  return "Clang " XSTR_MACRO(__clang_major__) "." XSTR_MACRO(__clang_minor__) "." XSTR_MACRO(__clang_patchlevel__);
#endif //! __clang_version__
#elif defined (__INTEL_COMPILER)
  return "Intel Compiler " XSTR_MACRO(__INTEL_COMPILER);
#elif defined (__GNUC__)
  std::string compilerStr;
#ifdef __llvm__
  /* Note: this will not detect GCC + DragonEgg */
  compilerStr = "llvm-gcc ";
#else // __llvm__
  compilerStr = "GCC ";
#endif // !__llvm__
  compilerStr += XSTR_MACRO(__GNUC__) "." XSTR_MACRO(__GNUC_MINOR__) "." XSTR_MACRO(__GNUC_PATCHLEVEL__);
  return compilerStr;
#elif defined (_MSC_VER)
  return "MSVC " XSTR_MACRO(_MSC_FULL_VER);
#else
  return "unknown compiler";
#endif
}

std::string CSysInfo::GetPrivacyPolicy()
{
  if (m_privacyPolicy.empty())
  {
    CFile file;
    XFILE::auto_buffer buf;
    if (file.LoadFile("special://xbmc/privacy-policy.txt", buf) > 0)
    {
      std::string strBuf(buf.get(), buf.length());
      m_privacyPolicy = strBuf;
    }
    else
      m_privacyPolicy = g_localizeStrings.Get(19055);
  }
  return m_privacyPolicy;
}

CSysInfo::WindowsDeviceFamily CSysInfo::GetWindowsDeviceFamily()
{
#ifdef TARGET_WINDOWS_STORE
  auto familyName = AnalyticsInfo::VersionInfo().DeviceFamily();
  if (familyName == L"Windows.Desktop")
    return WindowsDeviceFamily::Desktop;
  else if (familyName == L"Windows.Mobile")
    return WindowsDeviceFamily::Mobile;
  else if (familyName == L"Windows.Universal")
    return WindowsDeviceFamily::IoT;
  else if (familyName == L"Windows.Team")
    return WindowsDeviceFamily::Surface;
  else if (familyName == L"Windows.Xbox")
    return WindowsDeviceFamily::Xbox;
  else
    return WindowsDeviceFamily::Other;
#endif // TARGET_WINDOWS_STORE
  return WindowsDeviceFamily::Desktop;
}

CJob *CSysInfo::GetJob() const
{
  return new CSysInfoJob();
}

void CSysInfo::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  m_info = static_cast<CSysInfoJob*>(job)->GetData();
  CInfoLoader::OnJobComplete(jobID, success, job);
}
