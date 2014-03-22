/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <limits.h>

#include "threads/SystemClock.h"
#include "system.h"
#include "SystemInfo.h"
#ifndef TARGET_POSIX
#include <conio.h>
#else
#include <sys/utsname.h>
#endif
#include "GUIInfoManager.h"
#include "filesystem/CurlFile.h"
#include "network/Network.h"
#include "Application.h"
#include "windowing/WindowingFactory.h"
#include "guilib/LocalizeStrings.h"
#include "CPUInfo.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#ifdef TARGET_WINDOWS
#include "dwmapi.h"
#endif
#if defined(TARGET_DARWIN)
#include "osx/DarwinUtils.h"
#include "osx/CocoaInterface.h"
#endif
#include "powermanagement/PowerManager.h"
#include "utils/StringUtils.h"
#include "utils/XMLUtils.h"
#if defined(TARGET_ANDROID)
#include "android/jni/Build.h"
#include "utils/AMLUtils.h"
#endif

/* Target identification */
#if defined(TARGET_DARWIN)
#include <Availability.h>
#elif defined(TARGET_ANDROID)
#include <android/api-level.h>
#elif defined(TARGET_FREEBSD)
#include <sys/param.h>
#elif defined(TARGET_LINUX)
#include <linux/version.h>
#endif

CSysInfo g_sysinfo;

CSysInfoJob::CSysInfoJob()
{
}

bool CSysInfoJob::DoWork()
{
  m_info.systemUptime      = GetSystemUpTime(false);
  m_info.systemTotalUptime = GetSystemUpTime(true);
  m_info.internetState     = GetInternetState();
  m_info.videoEncoder      = GetVideoEncoder();
  m_info.cpuFrequency      = GetCPUFreqInfo();
  m_info.kernelVersion     = CSysInfo::GetKernelVersion();
  m_info.macAddress        = GetMACAddress();
  m_info.batteryLevel      = GetBatteryLevel();
  return true;
}

const CSysData &CSysInfoJob::GetData() const
{
  return m_info;
}

CStdString CSysInfoJob::GetCPUFreqInfo()
{
  double CPUFreq = GetCPUFrequency();
  return StringUtils::Format("%4.2fMHz", CPUFreq);;
}

CSysData::INTERNET_STATE CSysInfoJob::GetInternetState()
{
  // Internet connection state!
  XFILE::CCurlFile http;
  if (http.IsInternet())
    return CSysData::CONNECTED;
  return CSysData::DISCONNECTED;
}

CStdString CSysInfoJob::GetMACAddress()
{
#if defined(HAS_LINUX_NETWORK) || defined(HAS_WIN32_NETWORK)
  CNetworkInterface* iface = g_application.getNetwork().GetFirstConnectedInterface();
  if (iface)
    return iface->GetMacAddress();
#endif
  return "";
}

CStdString CSysInfoJob::GetVideoEncoder()
{
  return "GPU: " + g_Windowing.GetRenderRenderer();
}

CStdString CSysInfoJob::GetBatteryLevel()
{
  return StringUtils::Format("%d%%", g_powerManager.BatteryLevel());
}

double CSysInfoJob::GetCPUFrequency()
{
#if defined (TARGET_POSIX) || defined(TARGET_WINDOWS)
  return double (g_cpuInfo.getCPUFrequency());
#else
  return 0;
#endif
}

bool CSysInfoJob::SystemUpTime(int iInputMinutes, int &iMinutes, int &iHours, int &iDays)
{
  iMinutes=0;iHours=0;iDays=0;
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

CStdString CSysInfoJob::GetSystemUpTime(bool bTotalUptime)
{
  CStdString strSystemUptime;
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

CStdString CSysInfo::TranslateInfo(int info) const
{
  switch(info)
  {
  case SYSTEM_VIDEO_ENCODER_INFO:
    return m_info.videoEncoder;
  case NETWORK_MAC_ADDRESS:
    return m_info.macAddress;
  case SYSTEM_KERNEL_VERSION:
    return m_info.kernelVersion;
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

CSysInfo::~CSysInfo()
{
}

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

bool CSysInfo::GetDiskSpace(const CStdString& drive,int& iTotal, int& iTotalFree, int& iTotalUsed, int& iPercentFree, int& iPercentUsed)
{
  bool bRet= false;
  ULARGE_INTEGER ULTotal= { { 0 } };
  ULARGE_INTEGER ULTotalFree= { { 0 } };

  if( !drive.empty() && !drive.Equals("*") )
  {
#ifdef TARGET_WINDOWS
    UINT uidriveType = GetDriveType(( drive + ":\\" ));
    if(uidriveType != DRIVE_UNKNOWN && uidriveType != DRIVE_NO_ROOT_DIR)
#endif
      bRet= ( 0 != GetDiskFreeSpaceEx( ( drive + ":\\" ), NULL, &ULTotal, &ULTotalFree) );
  }
  else
  {
    ULARGE_INTEGER ULTotalTmp= { { 0 } };
    ULARGE_INTEGER ULTotalFreeTmp= { { 0 } };
#ifdef TARGET_WINDOWS
    char* pcBuffer= NULL;
    DWORD dwStrLength= GetLogicalDriveStrings( 0, pcBuffer );
    if( dwStrLength != 0 )
    {
      dwStrLength+= 1;
      pcBuffer= new char [dwStrLength];
      GetLogicalDriveStrings( dwStrLength, pcBuffer );
      int iPos= 0;
      do {
        if( DRIVE_FIXED == GetDriveType( pcBuffer + iPos  ) &&
            GetDiskFreeSpaceEx( ( pcBuffer + iPos ), NULL, &ULTotal, &ULTotalFree ) )
        {
          ULTotalTmp.QuadPart+= ULTotal.QuadPart;
          ULTotalFreeTmp.QuadPart+= ULTotalFree.QuadPart;
        }
        iPos += (strlen( pcBuffer + iPos) + 1 );
      }while( strlen( pcBuffer + iPos ) > 0 );
    }
    delete[] pcBuffer;
#else // for linux and osx
    static const char *drv_letter[] = { "C:\\", "E:\\", "F:\\", "G:\\", "X:\\", "Y:\\", "Z:\\", NULL };
    for( int i = 0; drv_letter[i]; i++)
    {
      if( GetDiskFreeSpaceEx( drv_letter[i], NULL, &ULTotal, &ULTotalFree ) )
      {
        ULTotalTmp.QuadPart+= ULTotal.QuadPart;
        ULTotalFreeTmp.QuadPart+= ULTotalFree.QuadPart;
      }
    }
#endif
    if( ULTotalTmp.QuadPart || ULTotalFreeTmp.QuadPart )
    {
      ULTotal.QuadPart= ULTotalTmp.QuadPart;
      ULTotalFree.QuadPart= ULTotalFreeTmp.QuadPart;
      bRet= true;
    }
  }

  if( bRet )
  {
    iTotal = (int)( ULTotal.QuadPart / MB );
    iTotalFree = (int)( ULTotalFree.QuadPart / MB );
    iTotalUsed = iTotal - iTotalFree;
    if( ULTotal.QuadPart > 0 )
    {
      iPercentUsed = (int)( 100.0f * ( ULTotal.QuadPart - ULTotalFree.QuadPart ) / ULTotal.QuadPart + 0.5f );
    }
    else
    {
      iPercentUsed = 0;
    }
    iPercentFree = 100 - iPercentUsed;
  }

  return bRet;
}

CStdString CSysInfo::GetCPUModel()
{
  return "CPU: " + g_cpuInfo.getCPUModel();
}

CStdString CSysInfo::GetCPUBogoMips()
{
  return "BogoMips: " + g_cpuInfo.getCPUBogoMips();
}

CStdString CSysInfo::GetCPUHardware()
{
  return "Hardware: " + g_cpuInfo.getCPUHardware();
}

CStdString CSysInfo::GetCPURevision()
{
  return "Revision: " + g_cpuInfo.getCPURevision();
}

CStdString CSysInfo::GetCPUSerial()
{
  return "Serial: " + g_cpuInfo.getCPUSerial();
}

CStdString CSysInfo::GetManufacturer()
{
  CStdString manufacturer = "";
#if defined(TARGET_ANDROID)
  manufacturer = CJNIBuild::MANUFACTURER;
#endif
  return manufacturer;
}

CStdString CSysInfo::GetModel()
{
  CStdString model = "";
#if defined(TARGET_ANDROID)
  model = CJNIBuild::MODEL;
#endif
  return model;
}

CStdString CSysInfo::GetProduct()
{
  CStdString product = "";
#if defined(TARGET_ANDROID)
  product = CJNIBuild::PRODUCT;
#endif
  return product;
}

bool CSysInfo::IsAeroDisabled()
{
#ifdef TARGET_WINDOWS
  BOOL aeroEnabled = FALSE;
  HRESULT res = DwmIsCompositionEnabled(&aeroEnabled);
  if (SUCCEEDED(res))
    return !aeroEnabled;
#endif
  return false;
}

bool CSysInfo::HasHW3DInterlaced()
{
#if defined(TARGET_ANDROID)
  if (aml_hw3d_present())
    return true;
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
#ifdef TARGET_WINDOWS
  if (m_WinVer == WindowsVersionUnknown)
  {
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (GetVersionEx((OSVERSIONINFO *)&osvi))
    {
      if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
        m_WinVer = WindowsVersionVista;
      else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
        m_WinVer = WindowsVersionWin7;
      else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2)
        m_WinVer = WindowsVersionWin8;
      else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3) 
        m_WinVer = WindowsVersionWin8_1;
      /* Insert checks for new Windows versions here */
      else if ( (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion > 3) || osvi.dwMajorVersion > 6)
        m_WinVer = WindowsVersionFuture;
    }
  }
#endif // TARGET_WINDOWS
  return m_WinVer;
}

int CSysInfo::GetKernelBitness(void)
{
#ifdef TARGET_WINDOWS
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
    return 64;
  
  BOOL (WINAPI *ptrIsWow64) (HANDLE, PBOOL);
  HMODULE hKernel32 = GetModuleHandleA("kernel32");
  if (hKernel32 == NULL)
    return 0; // Can't detect OS
  ptrIsWow64 = (BOOL (WINAPI *) (HANDLE, PBOOL)) GetProcAddress(hKernel32, "IsWow64Process");
  BOOL wow64proc = FALSE;
  if (ptrIsWow64 == NULL || ptrIsWow64(GetCurrentProcess(), &wow64proc) == FALSE)
    return 0; // Can't detect OS
  return (wow64proc == FALSE) ? 32 : 64;
#elif defined(TARGET_POSIX)
  struct utsname un;
  if (uname(&un) == 0)
  {
    std::string machine(un.machine);
    if (machine == "x86_64" || machine == "amd64" || machine == "arm64" || machine == "aarch64" || machine == "ppc64" || machine == "ia64")
      return 64;
    return 32;
  }
  return 0; // can't detect
#else
  return 0; // unknown
#endif
}

int CSysInfo::GetXbmcBitness(void)
{
#if defined (__aarch64__) || defined(__arm64__) || defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64) || defined(__ppc64__)
  return 64;
#elif defined(__thumb__) || defined(_M_ARMT) || defined(__arm__) || defined(_M_ARM) || defined(__mips__) || defined(mips) || defined(__mips) || defined(i386) || \
  defined(__i386) || defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(_M_IX86) || defined(_X86_) || defined(__powerpc) || \
  defined(__powerpc__) || defined(__ppc__) || defined(_M_PPC)
  return 32;
#else
  return 0; // Unknown
#endif
}

CStdString CSysInfo::GetKernelVersion()
{
#if defined(TARGET_DARWIN)
  return g_sysinfo.GetUnameVersion();
#elif defined (TARGET_POSIX)
  struct utsname un;
  if (uname(&un)==0)
  {
    return StringUtils::Format("%s %s %s %s", un.sysname, un.release, un.version, un.machine);;
  }

  return "";
#else
  OSVERSIONINFOEX osvi;
  ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
  osvi.dwOSVersionInfoSize = sizeof(osvi);

  std::string strKernel = "Windows";
  if (GetVersionEx((OSVERSIONINFO *)&osvi))
  {
    switch (GetWindowsVersion())
    {
    case WindowsVersionVista:
      if (osvi.wProductType == VER_NT_WORKSTATION)
        strKernel.append(" Vista");
      else
        strKernel.append(" Server 2008");
      break;
    case WindowsVersionWin7:
      if (osvi.wProductType == VER_NT_WORKSTATION)
        strKernel.append(" 7");
      else
        strKernel.append(" Server 2008 R2");
      break;
    case WindowsVersionWin8:
      if (osvi.wProductType == VER_NT_WORKSTATION)
        strKernel.append(" 8");
      else
        strKernel.append(" Server 2012");
      break;
    case WindowsVersionWin8_1:
      if (osvi.wProductType == VER_NT_WORKSTATION)
        strKernel.append(" 8.1");
      else
        strKernel.append(" Server 2012 R2");
      break;
    case WindowsVersionFuture:
      strKernel.append(" Unknown Future Version");
      break;
    default:
      strKernel.append(" Unknown version");
      break;
    }

    // Append Service Pack version if any
    if (osvi.wServicePackMajor > 0)
    {
      strKernel.append(StringUtils::Format(" SP%d", osvi.wServicePackMajor));
      if (osvi.wServicePackMinor > 0)
      {
        strKernel.append(StringUtils::Format(".%d", osvi.wServicePackMinor));
      }
    }

    strKernel.append(StringUtils::Format(" %d-bit", GetKernelBitness()));

    strKernel.append(StringUtils::Format(", build %d", osvi.dwBuildNumber));
  }
  else
  {
    strKernel.append(" unknown");
    strKernel.append(StringUtils::Format(" %d-bit", GetKernelBitness()));
  }

  return strKernel;
#endif
}

bool CSysInfo::HasInternet()
{
  if (m_info.internetState != CSysData::UNKNOWN)
    return m_info.internetState == CSysData::CONNECTED;
  return (m_info.internetState = CSysInfoJob::GetInternetState()) == CSysData::CONNECTED;
}

CStdString CSysInfo::GetHddSpaceInfo(int drive, bool shortText)
{
 int percent;
 return GetHddSpaceInfo( percent, drive, shortText);
}

CStdString CSysInfo::GetHddSpaceInfo(int& percent, int drive, bool shortText)
{
  int total, totalFree, totalUsed, percentFree, percentused;
  CStdString strRet;
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
      strRet = "N/A";
    else
      strRet = g_localizeStrings.Get(161);
  }
  return strRet;
}

#if defined(TARGET_LINUX)
CStdString CSysInfo::GetLinuxDistro()
{
#if defined(TARGET_ANDROID)
  return "Android";
#endif
  static const char* release_file[] = { "/etc/debian_version",
                                        "/etc/SuSE-release",
                                        "/etc/mandrake-release",
                                        "/etc/fedora-release",
                                        "/etc/redhat-release",
                                        "/etc/gentoo-release",
                                        "/etc/slackware-version",
                                        "/etc/arch-release",
                                        "/etc/buildroot-release",
                                        NULL };
  CStdString result("");
  char buffer[256] = {'\0'};

  /* Try reading PRETTY_NAME from /etc/os-release first.
   * If this fails, fall back to lsb_release or distro-specific release-file. */

  FILE *os_release = fopen("/etc/os-release", "r");

  if (os_release)
  {
    char *key = NULL;
    char *val = NULL;

    while (fgets(buffer, sizeof(buffer), os_release))
    {
      key = val = buffer;
      strsep(&val, "=");

      if (strcmp(key, "PRETTY_NAME") == 0)
      {
        char *pretty_name = val;

        // remove newline and enclosing quotes
        if (pretty_name[strlen(pretty_name) - 1] == '\n')
          pretty_name[strlen(pretty_name) - 1] = '\0';

        if (pretty_name[0] == '\'' || pretty_name[0] == '\"')
        {
          pretty_name++;
          pretty_name[strlen(pretty_name) - 1] = '\0';
        }

        // unescape quotes and backslashes
        char *p = pretty_name;
        while (*p)
        {
          char *this_char = p;
          char *next_char = p + 1;

          if (*this_char == '\\' &&
              (*next_char == '\'' || *next_char == '\"' || *next_char == '\\'))
          {
            while (*this_char)
            {
              *this_char = *next_char;
              this_char++;
              next_char++;
            }
          }

          p++;
        }

        result = pretty_name;
        break;
      }
    }

    fclose(os_release);

    if (!result.empty())
      return result;
  }

  FILE* pipe = popen("unset PYTHONHOME; unset PYTHONPATH; lsb_release -d  2>/dev/null | cut -f2", "r");
  if (pipe)
  {
    if (fread(buffer, sizeof(char), sizeof(buffer), pipe) > 0 && !ferror(pipe))
      result = buffer;
    pclose(pipe);
    if (!result.empty())
      return StringUtils::Trim(result);
  }

  FILE* file = NULL;
  for (int i = 0; result.empty() && release_file[i]; i++)
  {
    file = fopen(release_file[i], "r");
    if (file)
    {
      if (fgets(buffer, sizeof(buffer), file))
      {
        result = buffer;
        if (!result.empty())
          return StringUtils::Trim(result);
      }
      fclose(file);
    }
  }

  CLog::Log(LOGWARNING, "Unable to determine Linux distribution");
  return "Unknown";
}
#endif

#ifdef TARGET_POSIX
CStdString CSysInfo::GetUnameVersion()
{
  CStdString result = "";

#if defined(TARGET_ANDROID)
  struct utsname name;
  if (uname(&name) == -1)
    result = "Android";
  result += name.release;
  result += " ";
  result += name.machine;
#elif defined(TARGET_DARWIN_IOS)
  result = GetDarwinOSReleaseString();
  result += ", ";
  result += GetDarwinVersionString();
#else
  FILE* pipe = popen("uname -rm", "r");
  if (pipe)
  {
    char buffer[256];
    if (fgets(buffer, sizeof(buffer), pipe))
    {
      result = buffer;
#if defined(TARGET_DARWIN)
      StringUtils::Trim(result);
      result += ", "; 
      result += GetDarwinVersionString();
#endif
    }
    else
      CLog::Log(LOGWARNING, "Unable to determine Uname version");
    pclose(pipe);
  }
#endif//else !TARGET_ANDROID

  return StringUtils::Trim(result);
}
#endif

#if defined(TARGET_WINDOWS)
CStdString CSysInfo::GetUAWindowsVersion()
{
  OSVERSIONINFOEX osvi = {};

  osvi.dwOSVersionInfoSize = sizeof(osvi);
  CStdString strVersion = "Windows NT";

  if (GetVersionEx((OSVERSIONINFO *)&osvi))
  {
    strVersion += StringUtils::Format(" %d.%d", osvi.dwMajorVersion, osvi.dwMinorVersion);
  }

  SYSTEM_INFO si = {};
  GetSystemInfo(&si);

  BOOL bIsWow = FALSE;
  if (IsWow64Process(GetCurrentProcess(), &bIsWow))
  {
    if (bIsWow)
    {
      strVersion.append(";WOW64");
      GetNativeSystemInfo(&si);     // different function to read the info under Wow
    }
  }

  if (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
    strVersion.append(";Win64;x64");
  else if (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64)
    strVersion.append(";Win64;IA64");

  return strVersion;
}
#endif


CStdString CSysInfo::GetUserAgent()
{
  CStdString result;
  result = "XBMC/" + g_infoManager.GetLabel(SYSTEM_BUILD_VERSION) + " (";
#if defined(TARGET_WINDOWS)
  result += GetUAWindowsVersion();
#elif defined(TARGET_DARWIN)
#if defined(TARGET_DARWIN_IOS)
  result += "iOS; ";
#else
  result += "Mac OS X; ";
#endif
  result += GetUnameVersion();
#elif defined(TARGET_FREEBSD)
  result += "FreeBSD; ";
  result += GetUnameVersion();
#elif defined(TARGET_POSIX)
  result += "Linux; ";
  result += GetLinuxDistro();
  result += "; ";
  result += GetUnameVersion();
#endif
  result += "; http://xbmc.org)";

  return result;
}

bool CSysInfo::IsAppleTV2()
{
#if defined(TARGET_DARWIN)
  return DarwinIsAppleTV2();
#else
  return false;
#endif
}

bool CSysInfo::HasVideoToolBoxDecoder()
{
  bool        result = false;

#if defined(HAVE_VIDEOTOOLBOXDECODER)
  result = DarwinHasVideoToolboxDecoder();
#endif
  return result;
}

std::string CSysInfo::GetBuildTargetPlatformName(void)
{
#if defined(TARGET_DARWIN_OSX)
  return "Darwin OSX";
#elif defined(TARGET_DARWIN_IOS_ATV2)
  return "Darwin iOS ATV2";
#elif defined(TARGET_DARWIN_IOS)
  return "Darwin iOS";
#elif defined(TARGET_FREEBSD)
  return "FreeBSD";
#elif defined(TARGET_ANDROID)
  return "Android";
#elif defined(TARGET_LINUX)
  return "Linux";
#elif defined(TARGET_WINDOWS)
  return "Win32";
#else
  return "unknown platform";
#endif
}

std::string CSysInfo::GetBuildTargetPlatformVersion(void)
{
/* Expand macro before stringify */
#define STR_MACRO(x) #x
#define XSTR_MACRO(x) STR_MACRO(x)

#if defined(TARGET_DARWIN_OSX)
  return "version " XSTR_MACRO(__MAC_OS_X_VERSION_MIN_REQUIRED);
#elif defined(TARGET_DARWIN_IOS)
  return "version " XSTR_MACRO(__IPHONE_OS_VERSION_MIN_REQUIRED);
#elif defined(TARGET_FREEBSD)
  return "version " XSTR_MACRO(__FreeBSD_version);
#elif defined(TARGET_ANDROID)
  return "API level " XSTR_MACRO(__ANDROID_API__);
#elif defined(TARGET_LINUX)
  std::string ver = StringUtils::Format("%i.%i.%i", LINUX_VERSION_CODE >> 16, (LINUX_VERSION_CODE >> 8) & 0xff, LINUX_VERSION_CODE & 0xff);
  return ver;
#elif defined(TARGET_WINDOWS)
  return "version " XSTR_MACRO(NTDDI_VERSION);
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
#elif defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || defined(__ppc__) || defined(__ppc64__) || defined(_M_PPC)
  return "PowerPC";
#else
  return "unknown CPU family";
#endif
}


CJob *CSysInfo::GetJob() const
{
  return new CSysInfoJob();
}

void CSysInfo::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  m_info = ((CSysInfoJob *)job)->GetData();
  CInfoLoader::OnJobComplete(jobID, success, job);
}
