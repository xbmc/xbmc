#pragma once

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

#include "md5.h"
#include "InfoLoader.h"
#include "settings/lib/ISubSettings.h"

#define KB  (1024)          // 1 KiloByte (1KB)   1024 Byte (2^10 Byte)
#define MB  (1024*KB)       // 1 MegaByte (1MB)   1024 KB (2^10 KB)
#define GB  (1024*MB)       // 1 GigaByte (1GB)   1024 MB (2^10 MB)
#define TB  (1024*GB)       // 1 TerraByte (1TB)  1024 GB (2^10 GB)

#define MAX_KNOWN_ATTRIBUTES  46


class CSysData
{
public:
  enum INTERNET_STATE { UNKNOWN, CONNECTED, NO_DNS, DISCONNECTED };
  CSysData()
  {
    Reset();
  };

  void Reset()
  {
    internetState = UNKNOWN;
  };

  std::string systemUptime;
  std::string systemTotalUptime;
  INTERNET_STATE internetState;
  std::string videoEncoder;
  std::string cpuFrequency;
  std::string kernelVersion;
  std::string macAddress;
  std::string batteryLevel;
};

class CSysInfoJob : public CJob
{
public:
  CSysInfoJob();

  virtual bool DoWork();
  const CSysData &GetData() const;

  static CSysData::INTERNET_STATE GetInternetState();
private:
  bool SystemUpTime(int iInputMinutes, int &iMinutes, int &iHours, int &iDays);
  double GetCPUFrequency();
  std::string GetSystemUpTime(bool bTotalUptime);
  std::string GetCPUFreqInfo();
  std::string GetMACAddress();
  std::string GetVideoEncoder();
  std::string GetBatteryLevel();

  CSysData m_info;
};

class CSysInfo : public CInfoLoader, public ISubSettings
{
public:
  enum WindowsVersion
  {
    WindowsVersionUnknown = -1, // Undetected, unsupported Windows version or OS in not Windows
    WindowsVersionVista,        // Windows Vista, Windows Server 2008
    WindowsVersionWin7,         // Windows 7, Windows Server 2008 R2
    WindowsVersionWin8,         // Windows 8, Windows Server 2012
    WindowsVersionWin8_1,       // Windows 8.1
    /* Insert new Windows versions here, when they'll be known */
    WindowsVersionFuture = 100  // Future Windows version, not known to code
  };

  CSysInfo(void);
  virtual ~CSysInfo();

  virtual bool Load(const TiXmlNode *settings);
  virtual bool Save(TiXmlNode *settings) const;

  char MD5_Sign[32 + 1];

  bool GetDVDInfo(std::string& strDVDModel, std::string& strDVDFirmware);
  bool GetHDDInfo(std::string& strHDDModel, std::string& strHDDSerial,std::string& strHDDFirmware,std::string& strHDDpw,std::string& strHDDLockState);
  bool GetRefurbInfo(std::string& rfi_FirstBootTime, std::string& rfi_PowerCycleCount);

#if defined(TARGET_LINUX)
  std::string GetLinuxDistro();
#endif
#ifdef TARGET_POSIX
  std::string GetUnameVersion();
#endif
#if defined(TARGET_WINDOWS)
  std::string GetUAWindowsVersion();
#endif
  std::string GetUserAgent();
  bool HasInternet();
  bool IsAppleTV2();
  bool HasVideoToolBoxDecoder();
  bool IsAeroDisabled();
  static bool IsWindowsVersion(WindowsVersion ver);
  static bool IsWindowsVersionAtLeast(WindowsVersion ver);
  static WindowsVersion GetWindowsVersion();
  static int GetKernelBitness(void);
  static int GetXbmcBitness(void);
  static std::string GetKernelVersion();
  std::string GetCPUModel();
  std::string GetCPUBogoMips();
  std::string GetCPUHardware();
  std::string GetCPURevision();
  std::string GetCPUSerial();
  std::string GetManufacturer();
  std::string GetProduct();
  std::string GetModel();
  bool GetDiskSpace(const std::string& drive,int& iTotal, int& iTotalFree, int& iTotalUsed, int& iPercentFree, int& iPercentUsed);
  std::string GetHddSpaceInfo(int& percent, int drive, bool shortText=false);
  std::string GetHddSpaceInfo(int drive, bool shortText=false);

  int GetTotalUptime() const { return m_iSystemTimeTotalUp; }
  void SetTotalUptime(int uptime) { m_iSystemTimeTotalUp = uptime; }

  static std::string GetBuildTargetPlatformName(void);
  static std::string GetBuildTargetPlatformVersion(void);
  static std::string GetBuildTargetCpuFamily(void);

protected:
  virtual CJob *GetJob() const;
  virtual std::string TranslateInfo(int info) const;
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

private:
  CSysData m_info;
  static WindowsVersion m_WinVer;
  int m_iSystemTimeTotalUp; // Uptime in minutes!
  void Reset();
};

extern CSysInfo g_sysinfo;

