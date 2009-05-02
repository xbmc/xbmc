#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "md5.h"
#include "InfoLoader.h"

#define KB  (1024)          // 1 KiloByte (1KB)   1024 Byte (2^10 Byte)
#define MB  (1024*KB)       // 1 MegaByte (1MB)   1024 KB (2^10 KB)
#define GB  (1024*MB)       // 1 GigaByte (1GB)   1024 MB (2^10 MB)
#define TB  (1024*GB)       // 1 TerraByte (1TB)  1024 GB (2^10 GB)

#define MAX_KNOWN_ATTRIBUTES  46
class CBackgroundSystemInfoLoader : public CBackgroundLoader
{
public:
  CBackgroundSystemInfoLoader(CInfoLoader *pCallback) : CBackgroundLoader(pCallback) {};

protected:
  virtual void GetInformation();
};
class CSysInfo : public CInfoLoader
{
  public:
    CSysInfo(void);
    virtual ~CSysInfo();

    char MD5_Sign[32 + 1];

    bool GetDVDInfo(CStdString& strDVDModel, CStdString& strDVDFirmware);
    bool GetHDDInfo(CStdString& strHDDModel, CStdString& strHDDSerial,CStdString& strHDDFirmware,CStdString& strHDDpw,CStdString& strHDDLockState);
    bool GetRefurbInfo(CStdString& rfi_FirstBootTime, CStdString& rfi_PowerCycleCount);

    bool CreateBiosBackup();
    bool CreateEEPROMBackup();
    void WriteTXTInfoFile();

    CStdString GetVideoEncoder();
    CStdString GetKernelVersion();
#if defined(_LINUX) && !defined(__APPLE__)
    CStdString GetLinuxDistro();
#endif
#ifdef _LINUX
    CStdString GetUnameVersion();
#endif    
    CStdString GetUserAgent();
#if defined(__APPLE__)
    bool IsAppleTV();
#endif
    CStdString GetSystemUpTime(bool bTotalUptime);
    CStdString GetCPUFreqInfo();
    CStdString GetXBVerInfo();
    CStdString GetMACAddress();
    bool GetDiskSpace(const CStdString drive,int& iTotal, int& iTotalFree, int& iTotalUsed, int& iPercentFree, int& iPercentUsed);
    CStdString GetHddSpaceInfo(int& percent, int drive, bool shortText=false);
    CStdString GetHddSpaceInfo(int drive, bool shortText=false);
    CStdString GetInternetState();

    CStdString m_kernelversion;
    CStdString m_cpufrequency;
    CStdString m_videoencoder;
    CStdString m_macadress;
    CStdString m_InternetState;
    CStdString m_systemtotaluptime;
    CStdString m_systemuptime;
    CStdString m_xboxversion;

    bool m_bInternetState;
    bool m_bRequestDone;

  private:
    double GetCPUFrequency();
    bool SystemUpTime(int iInputMinutes, int &iMinutes, int &iHours, int &iDays);

    void Reset();

protected:
    virtual const char *TranslateInfo(DWORD dwInfo);
    virtual DWORD TimeToNextRefreshInMs();
};

extern CSysInfo g_sysinfo;

