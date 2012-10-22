/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "threads/SystemClock.h"
#include "system.h"
#include "SystemInfo.h"
#ifndef _LINUX
#include <conio.h>
#else
#include <sys/utsname.h>
#endif
#include "GUIInfoManager.h"
#include "filesystem/CurlFile.h"
#include "network/Network.h"
#include "Application.h"
#include "windowing/WindowingFactory.h"
#include "settings/Settings.h"
#include "guilib/LocalizeStrings.h"
#include "CPUInfo.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#ifdef _WIN32
#include "dwmapi.h"
#endif
#if defined(TARGET_DARWIN)
#include "osx/DarwinUtils.h"
#include "osx/CocoaInterface.h"
#endif
#include "powermanagement/PowerManager.h"
#include "utils/StringUtils.h"

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
  CStdString strCPUFreq;
  double CPUFreq = GetCPUFrequency();
  strCPUFreq.Format("%4.2fMHz", CPUFreq);
  return strCPUFreq;
}

CSysData::INTERNET_STATE CSysInfoJob::GetInternetState()
{
  // Internet connection state!
  XFILE::CCurlFile http;
  if (http.IsInternet())
    return CSysData::CONNECTED;
  if (http.IsInternet(false))
    return CSysData::NO_DNS;
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
  CStdString strVal;
  strVal.Format("%d%%", g_powerManager.BatteryLevel());
  return strVal;
}

double CSysInfoJob::GetCPUFrequency()
{
#if defined (_LINUX) || defined(_WIN32)
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
    iInputMinutes = g_settings.m_iSystemTimeTotalUp + ((int)(XbmcThreads::SystemClockMillis() / 60000));
  }
  else
  {
    //Current UpTime
    iInputMinutes = (int)(XbmcThreads::SystemClockMillis() / 60000);
  }

  SystemUpTime(iInputMinutes,iMinutes, iHours, iDays);
  if (iDays > 0)
  {
    strSystemUptime.Format("%i %s, %i %s, %i %s",
      iDays,g_localizeStrings.Get(12393),
      iHours,g_localizeStrings.Get(12392),
      iMinutes, g_localizeStrings.Get(12391));
  }
  else if (iDays == 0 && iHours >= 1 )
  {
    strSystemUptime.Format("%i %s, %i %s",
      iHours,g_localizeStrings.Get(12392),
      iMinutes, g_localizeStrings.Get(12391));
  }
  else if (iDays == 0 && iHours == 0 &&  iMinutes >= 0)
  {
    strSystemUptime.Format("%i %s",
      iMinutes, g_localizeStrings.Get(12391));
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
    else if (m_info.internetState == CSysData::NO_DNS)
      return g_localizeStrings.Get(13274);
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
}

CSysInfo::~CSysInfo()
{
}

bool CSysInfo::GetDiskSpace(const CStdString drive,int& iTotal, int& iTotalFree, int& iTotalUsed, int& iPercentFree, int& iPercentUsed)
{
  bool bRet= false;
  ULARGE_INTEGER ULTotal= { { 0 } };
  ULARGE_INTEGER ULTotalFree= { { 0 } };

  if( !drive.IsEmpty() && !drive.Equals("*") )
  {
#ifdef _WIN32
    UINT uidriveType = GetDriveType(( drive + ":\\" ));
    if(uidriveType != DRIVE_UNKNOWN && uidriveType != DRIVE_NO_ROOT_DIR)
#endif
      bRet= ( 0 != GetDiskFreeSpaceEx( ( drive + ":\\" ), NULL, &ULTotal, &ULTotalFree) );
  }
  else
  {
    ULARGE_INTEGER ULTotalTmp= { { 0 } };
    ULARGE_INTEGER ULTotalFreeTmp= { { 0 } };
#ifdef _WIN32
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

bool CSysInfo::IsAeroDisabled()
{
#ifdef _WIN32
  if (IsVistaOrHigher())
  {
    BOOL aeroEnabled = FALSE;
    HRESULT res = DwmIsCompositionEnabled(&aeroEnabled);
    if (SUCCEEDED(res))
      return !aeroEnabled;
  }
  else
  {
    return true;
  }
#endif
  return false;
}

bool CSysInfo::IsVistaOrHigher()
{
#ifdef TARGET_WINDOWS
  return IsWindowsVersionAtLeast(WindowsVersionVista);
#else // TARGET_WINDOWS
  return false;
#endif // TARGET_WINDOWS
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
      if (osvi.dwMajorVersion == 5 && (osvi.dwMinorVersion == 1 || osvi.dwMinorVersion == 2 ))
        m_WinVer = WindowsVersionWinXP;
      else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
        m_WinVer = WindowsVersionVista;
      else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
        m_WinVer = WindowsVersionWin7;
      else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2)
        m_WinVer = WindowsVersionWin8;
      /* Insert checks for new Windows versions here */
      else if ( (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion > 2) || osvi.dwMajorVersion > 6)
        m_WinVer = WindowsVersionFuture;
    }
  }
#endif // TARGET_WINDOWS
  return m_WinVer;
}

bool CSysInfo::IsOS64bit()
{
#ifdef TARGET_WINDOWS
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
    return true;
  
  BOOL (WINAPI *ptrIsWow64) (HANDLE, PBOOL);
  HMODULE hKernel32 = GetModuleHandleA("kernel32");
  if (hKernel32 == NULL)
    return false; // Can't detect OS
  ptrIsWow64 = (BOOL (WINAPI *) (HANDLE, PBOOL)) GetProcAddress(hKernel32, "IsWow64Process");
  BOOL wow64proc = FALSE;
  if (ptrIsWow64 == NULL || ptrIsWow64(GetCurrentProcess(), &wow64proc) == FALSE)
    return false; // Can't detect OS
  return wow64proc != FALSE;
#else // TARGET_WINDOWS
  // TODO: Implement Linux, FreeBSD, Android, OSX
  return false;
#endif // TARGET_WINDOWS
}

CStdString CSysInfo::GetKernelVersion()
{
#if defined (_LINUX)
  struct utsname un;
  if (uname(&un)==0)
  {
    CStdString strKernel;
    strKernel.Format("%s %s %s %s", un.sysname, un.release, un.version, un.machine);
    return strKernel;
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
    case WindowsVersionWinXP:
      if (GetSystemMetrics(SM_SERVERR2))
        strKernel.append(" Server 2003 R2");
      else if (osvi.wSuiteMask & VER_SUITE_STORAGE_SERVER)
        strKernel.append(" Storage Server 2003");
      else if (osvi.wSuiteMask & VER_SUITE_WH_SERVER)
        strKernel.append(" Home Server");
      else if (osvi.wProductType == VER_NT_WORKSTATION && IsOS64bit())
        strKernel.append(" XP Professional");
      else if (osvi.wProductType != VER_NT_WORKSTATION)
        strKernel.append(" Server 2003");
      else if (osvi.wSuiteMask & VER_SUITE_PERSONAL)
        strKernel.append("XP Home Edition" );
      else
        strKernel.append("XP Professional" );
      break;
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

    if (IsOS64bit())
      strKernel.append(" 64-bit");
    else
      strKernel.append(" 32-bit");

    strKernel.append(StringUtils::Format(", build %d", osvi.dwBuildNumber));
  }
  else
  {
    strKernel.append(" unknown");
    if (IsOS64bit())
      strKernel.append(" 64-bit");
    else
      strKernel.append(" 32-bit");
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
        strRet.Format("%i MB %s", totalFree, g_localizeStrings.Get(160));
        break;
      case SYSTEM_USED_SPACE:
        strRet.Format("%i MB %s", totalUsed, g_localizeStrings.Get(20162));
        break;
      case SYSTEM_TOTAL_SPACE:
        strRet.Format("%i MB %s", total, g_localizeStrings.Get(20161));
        break;
      case SYSTEM_FREE_SPACE_PERCENT:
        strRet.Format("%i %% %s", percentFree, g_localizeStrings.Get(160));
        break;
      case SYSTEM_USED_SPACE_PERCENT:
        strRet.Format("%i %% %s", percentused, g_localizeStrings.Get(20162));
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

#if defined(_LINUX) && !defined(TARGET_DARWIN) && !defined(__FreeBSD__)
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

  FILE* pipe = popen("unset PYTHONHOME; unset PYTHONPATH; lsb_release -d  2>/dev/null | cut -f2", "r");
  if (pipe)
  {
    char buffer[256] = {'\0'};
    if (fread(buffer, sizeof(char), sizeof(buffer), pipe) > 0 && !ferror(pipe))
      result = buffer;
    pclose(pipe);
    if (!result.IsEmpty())
      return result.Trim();
  }

  FILE* file = NULL;
  for (int i = 0; result.IsEmpty() && release_file[i]; i++)
  {
    file = fopen(release_file[i], "r");
    if (file)
    {
      char buffer[256] = {'\0'};
      if (fgets(buffer, sizeof(buffer), file))
      {
        result = buffer;
        if (!result.IsEmpty())
          return result.Trim();
      }
      fclose(file);
    }
  }

  CLog::Log(LOGWARNING, "Unable to determine Linux distribution");
  return "Unknown";
}
#endif

#ifdef _LINUX
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
#else
  FILE* pipe = popen("uname -rm", "r");
  if (pipe)
  {
    char buffer[256] = {'\0'};
    if (fread(buffer, sizeof(char), sizeof(buffer), pipe) > 0 && !ferror(pipe))
    {
      result = buffer;
#if defined(TARGET_DARWIN)
      result.Trim();
      result += ", "; 
      result += GetDarwinVersionString();
#endif
    }
    else
      CLog::Log(LOGWARNING, "Unable to determine Uname version");
    pclose(pipe);
  }
#endif//else !TARGET_ANDROID

  return result.Trim();
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
    strVersion.AppendFormat(" %d.%d", osvi.dwMajorVersion, osvi.dwMinorVersion);
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
#if defined(_WIN32)
  result += GetUAWindowsVersion();
#elif defined(TARGET_DARWIN)
#if defined(TARGET_DARWIN_IOS)
  result += "iOS; ";
#else
  result += "Mac OS X; ";
#endif
  result += GetUnameVersion();
#elif defined(__FreeBSD__)
  result += "FreeBSD; ";
  result += GetUnameVersion();
#elif defined(_LINUX)
  result += "Linux; ";
  result += GetLinuxDistro();
  result += "; ";
  result += GetUnameVersion();
#endif
  result += "; http://www.xbmc.org)";

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

bool CSysInfo::HasVDADecoder()
{
  bool        result = false;

#if defined(TARGET_DARWIN_OSX)
  result = Cocoa_HasVDADecoder();
#endif
  return result;
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
