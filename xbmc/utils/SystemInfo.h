#pragma once

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

#include "md5.h"
#include "InfoLoader.h"

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

  CStdString systemUptime;
  CStdString systemTotalUptime;
  INTERNET_STATE internetState;
  CStdString videoEncoder;
  CStdString cpuFrequency;
  CStdString kernelVersion;
  CStdString macAddress;
  CStdString batteryLevel;
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
  CStdString GetSystemUpTime(bool bTotalUptime);
  CStdString GetCPUFreqInfo();
  CStdString GetMACAddress();
  CStdString GetVideoEncoder();
  CStdString GetBatteryLevel();

  CSysData m_info;
};

class CSysInfo : public CInfoLoader
{
public:
  enum WindowsVersion
  {
    WindowsVersionUnknown = -1, // Undetected, unsupported Windows version or OS in not Windows
    WindowsVersionWinXP,        // Windows XP, Windows Server 2003 (R2), Windows Home Server
    WindowsVersionVista,        // Windows Vista, Windows Server 2008
    WindowsVersionWin7,         // Windows 7, Windows Server 2008 R2
    WindowsVersionWin8,         // Windows 8, Windows Server 2012
    /* Insert new Windows versions here, when they'll be known */
    WindowsVersionFuture = 100  // Future Windows version, not known to code
  };

  CSysInfo(void);
  virtual ~CSysInfo();

  char MD5_Sign[32 + 1];

  bool GetDVDInfo(CStdString& strDVDModel, CStdString& strDVDFirmware);
  bool GetHDDInfo(CStdString& strHDDModel, CStdString& strHDDSerial,CStdString& strHDDFirmware,CStdString& strHDDpw,CStdString& strHDDLockState);
  bool GetRefurbInfo(CStdString& rfi_FirstBootTime, CStdString& rfi_PowerCycleCount);

#if defined(_LINUX) && !defined(TARGET_DARWIN) && !defined(__FreeBSD__)
  CStdString GetLinuxDistro();
#endif
#ifdef _LINUX
  CStdString GetUnameVersion();
#endif
#if defined(TARGET_WINDOWS)
  CStdString CSysInfo::GetUAWindowsVersion();
#endif
  CStdString GetUserAgent();
  bool HasInternet();
  bool IsAppleTV2();
  bool HasVDADecoder();
  bool HasVideoToolBoxDecoder();
  bool IsAeroDisabled();
  bool IsVistaOrHigher();
  static bool IsWindowsVersion(WindowsVersion ver);
  static bool IsWindowsVersionAtLeast(WindowsVersion ver);
  static WindowsVersion GetWindowsVersion();
  static bool IsOS64bit();
  static CStdString GetKernelVersion();
  CStdString GetCPUModel();
  CStdString GetCPUBogoMips();
  CStdString GetCPUHardware();
  CStdString GetCPURevision();
  CStdString GetCPUSerial();
  bool GetDiskSpace(const CStdString drive,int& iTotal, int& iTotalFree, int& iTotalUsed, int& iPercentFree, int& iPercentUsed);
  CStdString GetHddSpaceInfo(int& percent, int drive, bool shortText=false);
  CStdString GetHddSpaceInfo(int drive, bool shortText=false);

protected:
  virtual CJob *GetJob() const;
  virtual CStdString TranslateInfo(int info) const;
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

private:
  CSysData m_info;
  static WindowsVersion m_WinVer;
  void Reset();
};

extern CSysInfo g_sysinfo;

