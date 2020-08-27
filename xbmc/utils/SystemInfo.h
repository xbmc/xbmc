/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "InfoLoader.h"
#include "settings/ISubSettings.h"

#include <string>

#define KB  (1024)          // 1 KiloByte (1KB)   1024 Byte (2^10 Byte)
#define MB  (1024*KB)       // 1 MegaByte (1MB)   1024 KB (2^10 KB)
#define GB  (1024*MB)       // 1 GigaByte (1GB)   1024 MB (2^10 MB)
#define TB  (1024*GB)       // 1 TerraByte (1TB)  1024 GB (2^10 GB)

#define MAX_KNOWN_ATTRIBUTES  46

#define REG_CURRENT_VERSION L"Software\\Microsoft\\Windows NT\\CurrentVersion"


class CSysData
{
public:
  enum INTERNET_STATE { UNKNOWN, CONNECTED, DISCONNECTED };
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
  std::string osVersionInfo;
  std::string macAddress;
  std::string batteryLevel;
};

class CSysInfoJob : public CJob
{
public:
  CSysInfoJob();

  bool DoWork() override;
  const CSysData &GetData() const;

  static CSysData::INTERNET_STATE GetInternetState();
private:
  static bool SystemUpTime(int iInputMinutes, int &iMinutes, int &iHours, int &iDays);
  static std::string GetSystemUpTime(bool bTotalUptime);
  static std::string GetMACAddress();
  static std::string GetVideoEncoder();
  static std::string GetBatteryLevel();

  CSysData m_info;
};

class CSysInfo : public CInfoLoader, public ISubSettings
{
public:
  enum WindowsVersion
  {
    WindowsVersionUnknown = -1, // Undetected, unsupported Windows version or OS in not Windows
    WindowsVersionWin7,         // Windows 7, Windows Server 2008 R2
    WindowsVersionWin8,         // Windows 8, Windows Server 2012
    WindowsVersionWin8_1,       // Windows 8.1
    WindowsVersionWin10,        // Windows 10
    WindowsVersionWin10_1709,   // Windows 10 1709 (FCU)
    WindowsVersionWin10_1803,   // Windows 10 1803
    WindowsVersionWin10_1809,   // Windows 10 1809
    WindowsVersionWin10_1903,   // Windows 10 1903
    WindowsVersionWin10_1909,   // Windows 10 1909
    WindowsVersionWin10_2004,   // Windows 10 2004
    WindowsVersionWin10_Future, // Windows 10 future build
    /* Insert new Windows versions here, when they'll be known */
    WindowsVersionFuture = 100  // Future Windows version, not known to code
  };
  enum WindowsDeviceFamily
  {
    Mobile = 1,
    Desktop = 2,
    IoT = 3,
    Xbox = 4,
    Surface = 5,
    Other = 100
  };

  CSysInfo(void);
  ~CSysInfo() override;

  bool Load(const TiXmlNode *settings) override;
  bool Save(TiXmlNode *settings) const override;

  char MD5_Sign[32 + 1];

  static const std::string& GetAppName(void); // the same name as CCompileInfo::GetAppName(), but const ref to std::string

  static std::string GetKernelName(bool emptyIfUnknown = false);
  static std::string GetKernelVersionFull(void); // full version string, including "-generic", "-RELEASE" etc.
  static std::string GetKernelVersion(void); // only digits with dots
  static std::string GetOsName(bool emptyIfUnknown = false);
  static std::string GetOsVersion(void);
  static std::string GetOsPrettyNameWithVersion(void);
  static std::string GetUserAgent();
  static std::string GetDeviceName();
  static std::string GetVersion();
  static std::string GetVersionShort();
  static std::string GetVersionCode();
  static std::string GetVersionGit();
  static std::string GetBuildDate();

  bool HasInternet();
  bool IsAeroDisabled();
  static bool IsWindowsVersion(WindowsVersion ver);
  static bool IsWindowsVersionAtLeast(WindowsVersion ver);
  static WindowsVersion GetWindowsVersion();
  static int GetKernelBitness(void);
  static int GetXbmcBitness(void);
  static const std::string& GetKernelCpuFamily(void);
  static std::string GetManufacturerName(void);
  static std::string GetModelName(void);
  bool GetDiskSpace(std::string drive,int& iTotal, int& iTotalFree, int& iTotalUsed, int& iPercentFree, int& iPercentUsed);
  std::string GetHddSpaceInfo(int& percent, int drive, bool shortText=false);
  std::string GetHddSpaceInfo(int drive, bool shortText=false);

  int GetTotalUptime() const { return m_iSystemTimeTotalUp; }
  void SetTotalUptime(int uptime) { m_iSystemTimeTotalUp = uptime; }

  static std::string GetBuildTargetPlatformName(void);
  static std::string GetBuildTargetPlatformVersion(void);
  static std::string GetBuildTargetPlatformVersionDecoded(void);
  static std::string GetBuildTargetCpuFamily(void);

  static std::string GetUsedCompilerNameAndVer(void);
  std::string GetPrivacyPolicy();

  static WindowsDeviceFamily GetWindowsDeviceFamily();

protected:
  CJob *GetJob() const override;
  std::string TranslateInfo(int info) const override;
  void OnJobComplete(unsigned int jobID, bool success, CJob *job) override;

private:
  CSysData m_info;
  std::string m_privacyPolicy;
  static WindowsVersion m_WinVer;
  int m_iSystemTimeTotalUp; // Uptime in minutes!
  void Reset();
};

extern CSysInfo g_sysinfo;

