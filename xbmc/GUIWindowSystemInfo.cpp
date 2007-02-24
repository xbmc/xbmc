/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "stdafx.h"
#include "utils/GUIInfoManager.h"
#include "GUIWindowSystemInfo.h"
#include "xbox/undocumented.h"
#include "xbox/network.h"
#include "xbox/xkhdd.h"
#include "xbox/XKExports.h"
#include "application.h"
#ifdef HAS_SYSINFO
#include "utils/SystemInfo.h"
#include "cores/dllloader/dllloader.h"

#define DEBUG_KEYBOARD
#define DEBUG_MOUSE

CStdString strMplayerVersion;
extern "C" XPP_DEVICE_TYPE XDEVICE_TYPE_IR_REMOTE_TABLE;
#define     XDEVICE_TYPE_IR_REMOTE  (&XDEVICE_TYPE_IR_REMOTE_TABLE)

#endif

CGUIWindowSystemInfo::CGUIWindowSystemInfo(void)
:CGUIWindow(WINDOW_SYSTEM_INFORMATION, "SettingsSystemInfo.xml")
{
#ifdef HAS_SYSINFO
  m_pXKEEPROM = new XKEEPROM;
  m_pXKEEPROM->ReadFromXBOX();
  m_XBOX_Version = m_pXKEEPROM->GetXBOXVersion();
#endif
}

CGUIWindowSystemInfo::~CGUIWindowSystemInfo(void)
{
#ifdef HAS_SYSINFO
  delete m_pXKEEPROM;
#endif
}

#ifdef HAS_SYSINFO
bool CGUIWindowSystemInfo::GetMPlayerVersion(CStdString& strVersion)
{
  DllLoader* mplayerDll;
  const char* (__cdecl* pMplayerGetVersion)();
  const char* (__cdecl* pMplayerGetCompileDate)();
  const char* (__cdecl* pMplayerGetCompileTime)();

  const char *version = NULL;
  const char *date = NULL;
  const char *btime = NULL;

  mplayerDll = new DllLoader("Q:\\system\\players\\mplayer\\mplayer.dll",true);

  if( mplayerDll->Parse() )
  {
    if (mplayerDll->ResolveExport("mplayer_getversion", (void**)&pMplayerGetVersion))
      version = pMplayerGetVersion();
    if (mplayerDll->ResolveExport("mplayer_getcompiledate", (void**)&pMplayerGetCompileDate))
      date = pMplayerGetCompileDate();
    if (mplayerDll->ResolveExport("mplayer_getcompiletime", (void**)&pMplayerGetCompileTime))
      btime = pMplayerGetCompileTime();
    if (version && date && btime)
    {
      strVersion.Format("%s (%s - %s)",version, date, btime);
    }
    else if (version)
    {
      strVersion.Format("%s",version);
    }
  }
  delete mplayerDll;
  mplayerDll=NULL;
  return true;
}
#endif

bool CGUIWindowSystemInfo::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowSystemInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
#ifdef HAS_SYSINFO
      m_wszMPlayerVersion[0] = 0;
      //Get SystemInformations on Init
      CGUIWindow::OnMessage(message);

      //Open Progress Dialog
      CGUIDialogProgress&  pDlgProgress= *((CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS));
      pDlgProgress.SetHeading(g_localizeStrings.Get(10007));
      pDlgProgress.SetLine(0, g_localizeStrings.Get(20185));
      pDlgProgress.SetLine(1, "");
      pDlgProgress.SetLine(2, g_localizeStrings.Get(20186));
      pDlgProgress.SetPercentage(0);
      pDlgProgress.Progress();
      pDlgProgress.StartModal();
      pDlgProgress.ShowProgressBar(true);

      CreateEEPROMBackup("System\\SystemInfo");
      pDlgProgress.SetLine(1, g_localizeStrings.Get(20187));
      pDlgProgress.SetPercentage(55);
      pDlgProgress.Progress();

      CSysInfo::BackupBios();
      pDlgProgress.SetLine(1, g_localizeStrings.Get(20188));
      pDlgProgress.SetPercentage(70);
      pDlgProgress.Progress();

      m_dwlastTime=0;
      GetMPlayerVersion(strMplayerVersion);
      pDlgProgress.SetLine(1, g_localizeStrings.Get(20177));
      pDlgProgress.SetPercentage(100);
      pDlgProgress.Progress();
      pDlgProgress.Close();
#else
      CGUIWindow::OnMessage(message);
#endif
      SetLabelDummy();
      b_IsHome = TRUE;
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      unsigned int iControl=message.GetSenderId();
      bool b_playing= false;
      if(iControl == CONTROL_BT_HDD)
      {
        b_IsHome = FALSE;
        SetLabelDummy();
        SET_CONTROL_LABEL(40,g_localizeStrings.Get(20156));
#ifdef HAS_SYSINFO
        //Label 2-6; HDD Values
        GetATAValues(2, 3, 4, 5, 6);

        //Label 7: HDD Lock/UnLock key
        CStdString strhddlockey;
        GetHDDKey(strhddlockey);
        SET_CONTROL_LABEL(7, strhddlockey);

        //Label 8 + 9: Refurb Info
        GetRefurbInfo(8, 9);

        //Label 10: HDD Temperature
        CStdString strItemhdd;
        GetHDDTemp(strItemhdd);
        SET_CONTROL_LABEL(10, strItemhdd);

#endif
      }
      else if(iControl == CONTROL_BT_DVD)
      {
        b_IsHome = FALSE;
        //Todo: Get DVD-ROM Supported Discs
        SetLabelDummy();
        SET_CONTROL_LABEL(40,g_localizeStrings.Get(20157));

#ifdef HAS_SYSINFO
        //Label 2-3: DVD-ROM Values
        GetATAPIValues(2, 3);
#endif
      }
      else if(iControl == CONTROL_BT_STORAGE)
      {
        b_IsHome = FALSE;
        SetLabelDummy();
        SET_CONTROL_LABEL(40,g_localizeStrings.Get(20155));

#ifdef HAS_SYSINFO
        // Label 2-10: Storage Values
        GetStorage(2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
#endif
      }
      else if(iControl == CONTROL_BT_DEFAULT)
      {
        SetLabelDummy();
        b_IsHome = TRUE;
      }
      else if(iControl == CONTROL_BT_NETWORK)
      {
        b_IsHome = FALSE;
        SetLabelDummy();
        SET_CONTROL_LABEL(40,g_localizeStrings.Get(20158));
#ifdef HAS_SYSINFO
        GetNetwork(2,5,3,6,7,8,9);  // Label 2-7

        // Label 8: Mac Address
        CStdString strMacAddress;
        GetMACAddress(strMacAddress);
        SET_CONTROL_LABEL(4, strMacAddress);

        // Label 9: Online State
        CStdString strInetCon;
        GetINetState(strInetCon);
        SET_CONTROL_LABEL(10,strInetCon);
#endif
      }
      else if(iControl == CONTROL_BT_VIDEO)
      {
        b_IsHome = FALSE;
        SetLabelDummy();
        SET_CONTROL_LABEL(40,g_localizeStrings.Get(20159));
#ifdef HAS_SYSINFO
        // Label 2: Video Encoder
        CStdString strVideoEnc;
        GetVideoEncInfo(strVideoEnc);
        SET_CONTROL_LABEL(2,strVideoEnc);

        // Label 3: Resolution
        CStdString strResol;
        GetResolution(strResol);
        SET_CONTROL_LABEL(3,strResol);

        // Label 4: AV Pack Info
        CStdString stravpack;
        GetAVPackInfo(stravpack);
        SET_CONTROL_LABEL(4,stravpack);

        // Label 5: XBE Video Region
        CStdString strVideoXBERegion;
        GetVideoXBERegion(strVideoXBERegion);
        SET_CONTROL_LABEL(5,strVideoXBERegion);

        // Label 6: DVD Zone
        CStdString strdvdzone;
        GetDVDZone(strdvdzone);
        SET_CONTROL_LABEL(6, strdvdzone);
#endif
      }
      else if(iControl == CONTROL_BT_HARDWARE)
      {
        b_IsHome = FALSE;

        SetLabelDummy();
        SET_CONTROL_LABEL(40,g_localizeStrings.Get(20160));
#ifdef HAS_SYSINFO
        CGUIDialogProgress&  pDlgProgress= *((CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS));
        pDlgProgress.SetHeading(g_localizeStrings.Get(20160));
        pDlgProgress.SetLine(0, g_localizeStrings.Get(20300));
        pDlgProgress.SetLine(1, g_localizeStrings.Get(20199));
        pDlgProgress.SetLine(2, g_localizeStrings.Get(20186));
        pDlgProgress.SetPercentage(0);
        pDlgProgress.Progress();
        pDlgProgress.StartModal();
        pDlgProgress.ShowProgressBar(true);

        // Label 2: XBOX Version
        CStdString strXBoxVer;
        GetXBVerInfo(strXBoxVer);
        SET_CONTROL_LABEL(2,strXBoxVer);

        // Label 3: XBOX Serial
        CStdString strXBSerial, strXBOXSerial;
        CStdString strlblXBSerial = g_localizeStrings.Get(13289);
        GetXBOXSerial(strXBSerial);
        strXBOXSerial.Format("%s %s",strlblXBSerial,strXBSerial);
        SET_CONTROL_LABEL(3,strXBOXSerial);

        // Label 4: CPU Speed!
        CStdString strCPUFreq;
        GetCPUFreqInfo(strCPUFreq);
        SET_CONTROL_LABEL(4, strCPUFreq);

        pDlgProgress.SetLine(1, g_localizeStrings.Get(20302));
        pDlgProgress.SetPercentage(30);
        pDlgProgress.Progress();

        // Label 5: ModChip ID!
        CStdString strModChip;
        if(GetModChipInfo(strModChip))
          SET_CONTROL_LABEL(5,strModChip);

        pDlgProgress.SetLine(1, g_localizeStrings.Get(20302));
        pDlgProgress.SetPercentage(40);
        pDlgProgress.Progress();

        // Label 6: Detected BiosName
        CStdString strBiosName;
        if (GetBIOSInfo(strBiosName))
          SET_CONTROL_LABEL(6,strBiosName);

        // Label 7: XBOX Live Key
        CStdString strXBLiveKey;
        GetXBLiveKey(strXBLiveKey);
        SET_CONTROL_LABEL(7, strXBLiveKey);

        // Label 8: XBOX ProducutionDate Info
        CStdString strXBProDate;
        GetXBProduceInfo(strXBProDate);
        SET_CONTROL_LABEL(8, strXBProDate);

        // Label 9,10,11: Attached Units!
        GetUnits(9, 10, 11);
        pDlgProgress.SetPercentage(100);
        pDlgProgress.Progress();
        pDlgProgress.Close();
#endif
      }
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowSystemInfo::Render()
{
  if (b_IsHome)
  {
    // Default Values
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20154));
    SET_CONTROL_LABEL(2, g_infoManager.GetSystemHeatInfo("cpu")); // CPU Temperature
    SET_CONTROL_LABEL(3, g_infoManager.GetSystemHeatInfo("gpu")); // GPU Temperature
    SET_CONTROL_LABEL(4, g_infoManager.GetSystemHeatInfo("fan")); // Fan Speed

    // Label 5: Set FreeMemory Info
    CStdString strFreeMem;
    GetFreeMemory(strFreeMem);
    SET_CONTROL_LABEL(5, strFreeMem);

    //Label 6: XBMC IP Adress
    if (g_infoManager.GetLabel(NETWORK_IP_ADDRESS)=="")
    {
      SET_CONTROL_LABEL(6,g_localizeStrings.Get(416));
    }
    else
    {
      SET_CONTROL_LABEL(6, g_infoManager.GetLabel(NETWORK_IP_ADDRESS));
    }

    // Label 7: Set Resolution Info
    CStdString strResol;
    GetResolution(strResol);
    SET_CONTROL_LABEL(7,strResol);

#ifdef HAS_SYSINFO
    // Label 8: Get Kernel Info
    CStdString strGetKernel;
    GetKernelVersion(strGetKernel);
    SET_CONTROL_LABEL(8,strGetKernel);

    // Label 9: Get System Uptime
    CStdString strSystemUptime;
    GetSystemUpTime(strSystemUptime);
    SET_CONTROL_LABEL(9,strSystemUptime);

    // Label 10: Get System Total Uptime
    CStdString strSystemTotalUptime;
    GetSystemTotalUpTime(strSystemTotalUptime);
    SET_CONTROL_LABEL(10,strSystemTotalUptime);
#endif

  }

  // Label 50: Get Current Time
  SET_CONTROL_LABEL(50, g_infoManager.GetTime(true) + " | " + g_infoManager.GetDate());
  // Set XBMC Build Time
  GetBuildTime(51, 52, 53); // Laber 51, 52, 53
  CGUIWindow::Render();
}

void CGUIWindowSystemInfo::SetLabelDummy()
{
  // Set Label Dummy Entry! ""
  for (int i=2; i<12; i++ )
  {
#ifdef HAS_SYSINFO
    SET_CONTROL_LABEL(i,"");
#else
    SET_CONTROL_LABEL(i,"PC version");
#endif
  }
}

#ifdef HAS_SYSINFO
bool CGUIWindowSystemInfo::GetKernelVersion(CStdString& strKernel)
{
  CStdString lblKernel=  g_localizeStrings.Get(13283);
  strKernel.Format("%s %d.%d.%d.%d",lblKernel,XboxKrnlVersion->VersionMajor,XboxKrnlVersion->VersionMinor,XboxKrnlVersion->Build,XboxKrnlVersion->Qfe);
  return true;
}

bool CGUIWindowSystemInfo::GetCPUFreqInfo(CStdString& strCPUFreq)
{
  double CPUFreq;
  CStdString lblCPUSpeed  = g_localizeStrings.Get(13284);
  CPUFreq                 = CSysInfo::GetCPUFrequency();

  strCPUFreq.Format("%s %4.2f Mhz.", lblCPUSpeed, CPUFreq);
  return true;
}

void CGUIWindowSystemInfo::GetMACAddress(CStdString& strMacAddress)
{
  char macaddress[20] = "";

  m_pXKEEPROM->GetMACAddressString((LPSTR)&macaddress, ':');

  CStdString lbl1 = g_localizeStrings.Get(149);

  strMacAddress.Format("%s: %s", lbl1, macaddress);
}

bool CGUIWindowSystemInfo::GetBIOSInfo(CStdString& strBiosName)
{
  CStdString cBIOSName;
  CStdString strlblBios = g_localizeStrings.Get(13285);
  if(CSysInfo::CheckBios(cBIOSName))
  {
    strBiosName.Format("%s %s", strlblBios,cBIOSName);
    return true;
  }
  else
  {
    strBiosName.Format("%s %s", strlblBios,"File: BiosIDs.ini Not Found!");
    return true;
  }
}

bool CGUIWindowSystemInfo::GetVideoEncInfo(CStdString& strItemVideoENC)
{
  CStdString lblVideoEnc  = g_localizeStrings.Get(13286);
  CStdString VideoEncoder = CSysInfo::GetVideoEncoder();
  strItemVideoENC.Format("%s %s", lblVideoEnc,VideoEncoder);
  return true;
}
#endif

bool CGUIWindowSystemInfo::GetResolution(CStdString& strResol)
{
  CStdString lblResInf  = g_localizeStrings.Get(13287);
  strResol.Format("%s %ix%i %s %02.2f Hz.",lblResInf,
    g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iWidth,
    g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iHeight,
    g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].strMode,
    g_infoManager.GetFPS()
    );
  return true;
}

#ifdef HAS_SYSINFO
bool CGUIWindowSystemInfo::GetXBVerInfo(CStdString& strXBoxVer)
{
  CStdString strXBOXVersion;
  CStdString lblXBver   =  g_localizeStrings.Get(13288);
  if (CSysInfo::GetXBOXVersionDetected(strXBOXVersion))
  {
    strXBoxVer.Format("%s %s", lblXBver,strXBOXVersion);
    CLog::Log(LOGDEBUG,"XBOX Version: %s",strXBOXVersion.c_str());
    return true;
  }
  else return false;
}

void CGUIWindowSystemInfo::GetXBOXSerial(CStdString& strXBOXSerial)
{
  CHAR serial[SERIALNUMBER_SIZE + 1] = "";

  m_pXKEEPROM->GetSerialNumberString(serial);

  strXBOXSerial.Format("%s", serial);
}

void CGUIWindowSystemInfo::GetXBProduceInfo(CStdString& strXBProDate)
{
  // Print XBOX Production Place and Date
  CStdString lbl = g_localizeStrings.Get(13290);
  CStdString lblYear = g_localizeStrings.Get(201);
  CStdString serialnumber;
  GetXBOXSerial(serialnumber);
  char *info = (char *) serialnumber.c_str();
  char *country;
  switch (atoi(&info[11]))
  {
  case 2:
    country = "Mexico";
    break;
  case 3:
    country = "Hungary";
    break;
  case 5:
    country = "China";
    break;
  case 6:
    country = "Taiwan";
    break;
  default:
    country = "Unknown";
    break;
  }
  CLog::Log(LOGDEBUG, "- XBOX production info: Country: %s, LineNumber: %c, Week %c%c, Year 200%c", country, info[0x00], info[0x08], info[0x09],info[0x07]);
  strXBProDate.Format("%s %s, %s 200%c, "+g_localizeStrings.Get(20169)+": %c%c "+g_localizeStrings.Get(20170)+": %c",lbl, country, lblYear,info[0x07], info[0x08],info[0x09], info[0x00]);
}

bool CGUIWindowSystemInfo::GetModChipInfo(CStdString& strModChip)
{
  // XBOX Modchip Type Detection
  CStdString ModChip    = CSysInfo::GetModCHIPDetected();
  CStdString lblModChip = g_localizeStrings.Get(13291);
   // Check if it is a SmartXX
  CStdString strIsSmartXX = CSysInfo::SmartXXModCHIP();
  if (!strIsSmartXX.Equals("None"))
  {
    strModChip.Format("%s %s", lblModChip.c_str(),strIsSmartXX);
    CLog::Log(LOGDEBUG, "- Detected ModChip: %s",strIsSmartXX);
    return true;
  }
  else
  {
    if ( ModChip.Equals("Unknown/Onboard TSOP (protected)"))
    {
      strModChip.Format("%s %s", lblModChip,g_localizeStrings.Get(20311));
    }
    else
    {
      strModChip.Format("%s %s", lblModChip,ModChip);
    }
    return true;
  }
  return false;
}

void CGUIWindowSystemInfo::GetAVPackInfo(CStdString& stravpack)
{
  //AV-[Cable]Pack Detection
  CStdString DetectedAVpack = CSysInfo::GetAVPackInfo();
  CStdString lblAVpack    = g_localizeStrings.Get(13292);
  stravpack.Format("%s %s",lblAVpack, DetectedAVpack);
  return;
}

void CGUIWindowSystemInfo::GetVideoXBERegion(CStdString& strVideoXBERegion)
{
  //Print Video Standard & XBE Region...
  CStdString lblVXBE = g_localizeStrings.Get(13293);
  CStdString XBEString, VideoStdString;

  switch (m_pXKEEPROM->GetVideoStandardVal())
  {
  case XKEEPROM::NTSC_J:
    VideoStdString = "NTSC J";
    break;
  case XKEEPROM::NTSC_M:
    VideoStdString = "NTSC M";
    break;
  case XKEEPROM::PAL_I:
    VideoStdString = "PAL I";
    break;
  case XKEEPROM::PAL_M:
    VideoStdString = "PAL M";
    break;
  default:
    VideoStdString = g_localizeStrings.Get(13205); // "Unknown"
  }

  switch(m_pXKEEPROM->GetXBERegionVal())
  {
  case XKEEPROM::NORTH_AMERICA:
    XBEString = "North America";
    break;
  case XKEEPROM::JAPAN:
    XBEString = "Japan";
    break;
  case XKEEPROM::EURO_AUSTRALIA:
    XBEString = "Europe / Australia";
    break;
  default:
    XBEString = g_localizeStrings.Get(13205); // "Unknown"
  }

  strVideoXBERegion.Format("%s %s, %s", lblVXBE, VideoStdString, XBEString);
}

void CGUIWindowSystemInfo::GetDVDZone(CStdString& strdvdzone)
{
  //Print DVD [Region] Zone ..
  CStdString lblDVDZone =  g_localizeStrings.Get(13294);
  DVD_ZONE dvdVal;

  dvdVal = m_pXKEEPROM->GetDVDRegionVal();
  strdvdzone.Format("%s %d",lblDVDZone, dvdVal);
}

bool CGUIWindowSystemInfo::GetINetState(CStdString& strInetCon)
{
  CHTTP http;
  CStdString lbl2 = g_localizeStrings.Get(13295);
  CStdString lbl3 = g_localizeStrings.Get(13296);
  CStdString lbl4 = g_localizeStrings.Get(13297);

  if (http.IsInternet())
  { // Connected to the Internet!
    strInetCon.Format("%s %s",lbl2, lbl3);
    return true;
  }
  else if (http.IsInternet(false))
  { // connected, but no DNS
    strInetCon.Format("%s %s",lbl2, g_localizeStrings.Get(13274));
    return true;
  }
  // NOT Connected to the Internet!
  strInetCon.Format("%s %s",lbl2, lbl4);
  return true;
}

void CGUIWindowSystemInfo::GetXBLiveKey(CStdString& strXBLiveKey)
{
  //Print XBLIVE Online Key..
  CStdString lbl3 = g_localizeStrings.Get(13298);
  char livekey[ONLINEKEY_SIZE * 2 + 1] = "";

  m_pXKEEPROM->GetOnlineKeyString(livekey);

  strXBLiveKey.Format("%s %s",lbl3, livekey);
}

void CGUIWindowSystemInfo::GetHDDKey(CStdString& strhddlockey)
{
  //Print HDD Key...
  CStdString lbl5 = g_localizeStrings.Get(13150);
  char hdkey[HDDKEY_SIZE * 2 + 1];

  m_pXKEEPROM->GetHDDKeyString((LPSTR)&hdkey);

  strhddlockey.Format("%s %s",lbl5, hdkey);
}

bool CGUIWindowSystemInfo::GetHDDTemp(CStdString& strItemhdd)
{
  // Get HDD Temp
  CStdString lblhdd = g_localizeStrings.Get(13151);

  BYTE bTemp= XKHDD::GetHddSmartTemp();
  CTemperature temp= CTemperature::CreateFromCelsius((double)bTemp);
  if (bTemp ==0 )
    temp.SetState(CTemperature::invalid);

  strItemhdd.Format("%s %s", lblhdd, temp.ToString());
  return true;
}
#endif

void CGUIWindowSystemInfo::GetFreeMemory(CStdString& strFreeMem)
{
#ifndef HAS_SYSINFO
#define MB (1024*1024)
#endif
  // Set FreeMemory Info
  MEMORYSTATUS stat;
  GlobalMemoryStatus(&stat);
  CStdString lblFreeMem = g_localizeStrings.Get(158);
  strFreeMem.Format("%s %i/%iMB",lblFreeMem,stat.dwAvailPhys/MB, stat.dwTotalPhys/MB);
}

#ifdef HAS_SYSINFO
bool CGUIWindowSystemInfo::GetATAPIValues(int i_lblp1, int i_lblp2)
{
  CStdString strDVDModel, strDVDFirmware;
  CStdString lblDVDModel    = g_localizeStrings.Get(13152);
  CStdString lblDVDFirmware = g_localizeStrings.Get(13153);
  if(CSysInfo::GetDVDInfo(strDVDModel, strDVDFirmware))
  {
    CStdString strDVDModelA;
    strDVDModelA.Format("%s %s",lblDVDModel, strDVDModel);
    SET_CONTROL_LABEL(i_lblp1, strDVDModelA);

    CStdString lblDVDFirmwareA;
    lblDVDFirmwareA.Format("%s %s",lblDVDFirmware, strDVDFirmware);
    SET_CONTROL_LABEL(i_lblp2, lblDVDFirmwareA);
    return true;
  }
  else return false;
}

bool CGUIWindowSystemInfo::GetATAValues(int i_lblp1, int i_lblp2, int i_lblp3, int i_lblp4, int i_lblp5)
{
  CStdString strHDDModel, strHDDSerial,strHDDFirmware,strHDDpw,strHDDLockState;
  if (CSysInfo::GetHDDInfo(strHDDModel, strHDDSerial,strHDDFirmware,strHDDpw,strHDDLockState))
  {
    CStdString strHDDModelA, strHDDSerialA, strHDDFirmwareA, strHDDpwA, strHDDLockStateA;

    CStdString lblhddm  = g_localizeStrings.Get(13154); //"HDD Model";
    CStdString lblhdds  = g_localizeStrings.Get(13155); //"HDD Serial";
    CStdString lblhddf  = g_localizeStrings.Get(13156); //"HDD Firmware";
    CStdString lblhddpw = g_localizeStrings.Get(13157); //"HDD Password";
    CStdString lblhddlk = g_localizeStrings.Get(13158); //"HDD Lock State";

    //HDD Model
    strHDDModelA.Format("%s %s",lblhddm,strHDDModel);
    SET_CONTROL_LABEL(i_lblp1, strHDDModelA);
    //CLog::Log(LOGDEBUG, "HDD Model: %s",strHDDModelA);

    //HDD Serial
    strHDDSerialA.Format("%s %s",lblhdds,strHDDSerial);
    SET_CONTROL_LABEL(i_lblp2, strHDDSerialA);
    //CLog::Log(LOGDEBUG, "HDD Serial: %s",strHDDSerialA);

    //HDD Firmware
    strHDDFirmwareA.Format("%s %s",lblhddf,strHDDFirmware);
    SET_CONTROL_LABEL(i_lblp3, strHDDFirmwareA);
    //CLog::Log(LOGDEBUG, "HDD Firmware: %s",strHDDFirmwareA);

    //HDD Lock State
    strHDDLockStateA.Format("%s %s",lblhddlk,strHDDLockState);
    SET_CONTROL_LABEL(i_lblp4, strHDDLockStateA);
    //CLog::Log(LOGDEBUG, "HDD LockState: %s",strHDDLockStateA);

    //HDD Password
    strHDDpwA.Format("%s %s",lblhddpw,strHDDpw);
    SET_CONTROL_LABEL(i_lblp5, strHDDpwA);
    //CLog::Log(LOGDEBUG, "HDD Password: %s",strHDDpwA);

    return true;
  }
  return false;
}

bool CGUIWindowSystemInfo::GetRefurbInfo(int label1, int label2)
{
  XBOX_REFURB_INFO xri;
  CStdString refurb_info;
  SYSTEMTIME sys_time;

  if (ExReadWriteRefurbInfo(&xri, sizeof(XBOX_REFURB_INFO), FALSE) < 0)
    return false;

  FileTimeToSystemTime((FILETIME*)&xri.FirstBootTime, &sys_time);

  refurb_info.Format("%s %d-%d-%d %d:%02d", g_localizeStrings.Get(13173), 
    sys_time.wMonth, 
    sys_time.wDay, 
    sys_time.wYear,
    sys_time.wHour,
    sys_time.wMinute);

  SET_CONTROL_LABEL(label1, refurb_info);

  refurb_info.Format("%s %d", g_localizeStrings.Get(13174), xri.PowerCycleCount);

  SET_CONTROL_LABEL(label2, refurb_info);

  return true;
}

bool CGUIWindowSystemInfo::GetNetwork(int i_lblp1, int i_lblp2, int i_lblp3, int i_lblp4, int i_lblp5, int i_lblp6, int i_lblp7)
{
  // Set Network Informations
  XNADDR net_stat;
  CStdString ip;

  // Set IP Type [DHCP/Fixed]
  if(XNetGetTitleXnAddr(&net_stat) & XNET_GET_XNADDR_DHCP)
    ip.Format("%s %s", g_localizeStrings.Get(146), g_localizeStrings.Get(148));
  else
    ip.Format("%s %s", g_localizeStrings.Get(146), g_localizeStrings.Get(147));

  SET_CONTROL_LABEL(i_lblp1,ip);

  // Set Ethernet Link State
  DWORD dwnetstatus = XNetGetEthernetLinkStatus();
  CStdString linkStatus = g_localizeStrings.Get(151);
  linkStatus += " ";
  if (dwnetstatus & XNET_ETHERNET_LINK_ACTIVE)
  {
    if (dwnetstatus & XNET_ETHERNET_LINK_100MBPS)
      linkStatus += "100mbps ";
    if (dwnetstatus & XNET_ETHERNET_LINK_10MBPS)
      linkStatus += "10mbps ";
    if (dwnetstatus & XNET_ETHERNET_LINK_FULL_DUPLEX)
      linkStatus += g_localizeStrings.Get(153);
    if (dwnetstatus & XNET_ETHERNET_LINK_HALF_DUPLEX)
      linkStatus += g_localizeStrings.Get(152);
  }
  else
    linkStatus += g_localizeStrings.Get(159);

  SET_CONTROL_LABEL(i_lblp3,linkStatus);

  // Get IP/Subnet/Gateway/DHCP Server/DNS1/DNS2
  const char* pszIP=g_localizeStrings.Get(150);

  CStdString strlblSubnet   = g_localizeStrings.Get(13159); //"Subnet:";
  CStdString strlblGateway  = g_localizeStrings.Get(13160); //"Gateway:";
  CStdString strlblDNS    = g_localizeStrings.Get(13161); //"DNS:";
  CStdString strlblDNS2    = g_localizeStrings.Get(20307);
  CStdString strlblDHCPServer = g_localizeStrings.Get(20308);

  CStdString strItem1, strItem2, strItem3, strItem4;

  ip.Format("%s: %s",pszIP, g_network.m_networkinfo.ip);  // IP
  strItem1.Format("%s %s", strlblSubnet, g_network.m_networkinfo.subnet); // Subnetmask
  strItem2.Format("%s %s", strlblGateway, g_network.m_networkinfo.gateway); //Gateway (Router IP)
  //strItem3.Format("%s %s", strlblDHCPServer, g_network.m_networkinfo.dhcpserver); // DHCP-Server IP

  strItem3.Format("%s: %s", strlblDNS, g_network.m_networkinfo.DNS1 ); // DNS1
  strItem4.Format("%s: %s", strlblDNS2, g_network.m_networkinfo.DNS2 ); // DNS2

  SET_CONTROL_LABEL(i_lblp2,ip);
  SET_CONTROL_LABEL(i_lblp4,strItem1);
  SET_CONTROL_LABEL(i_lblp5,strItem2);
  SET_CONTROL_LABEL(i_lblp6,strItem3);
  SET_CONTROL_LABEL(i_lblp7,strItem4);

  return true;
}

bool CGUIWindowSystemInfo::GetStorage(int i_lblp1, int i_lblp2, int i_lblp3, int i_lblp4, int i_lblp5, int i_lblp6, int i_lblp7, int i_lblp8, int i_lblp9, int i_lblp10)
{
  // Set HDD Space Informations
  ULARGE_INTEGER lTotalFreeBytesC;  ULARGE_INTEGER lTotalNumberOfBytesC;
  ULARGE_INTEGER lTotalFreeBytesE;  ULARGE_INTEGER lTotalNumberOfBytesE;
  ULARGE_INTEGER lTotalFreeBytesF;  ULARGE_INTEGER lTotalNumberOfBytesF;
  ULARGE_INTEGER lTotalFreeBytesG;  ULARGE_INTEGER lTotalNumberOfBytesG;
  ULARGE_INTEGER lTotalFreeBytesX;  ULARGE_INTEGER lTotalNumberOfBytesX;
  ULARGE_INTEGER lTotalFreeBytesY;  ULARGE_INTEGER lTotalNumberOfBytesY;
  ULARGE_INTEGER lTotalFreeBytesZ;  ULARGE_INTEGER lTotalNumberOfBytesZ;

  // Set DVD Drive State! [TrayOpen, NotReady....]
  CStdString trayState = "D: ";
  const char* pszStatus1;

  switch (CIoSupport::GetTrayState())
  {
  case TRAY_OPEN:
    pszStatus1=g_localizeStrings.Get(162);
    break;
  case DRIVE_NOT_READY:
    pszStatus1=g_localizeStrings.Get(163);
    break;
  case TRAY_CLOSED_NO_MEDIA:
    pszStatus1=g_localizeStrings.Get(164);
    break;
  case TRAY_CLOSED_MEDIA_PRESENT:
    pszStatus1=g_localizeStrings.Get(165);
    break;
  }
  trayState += pszStatus1;
  SET_CONTROL_LABEL(i_lblp2, trayState);

  //For C and E
  CStdString hdC, hdE;
  GetDiskSpace("C", lTotalNumberOfBytesC, lTotalFreeBytesC, hdC);
  GetDiskSpace("E", lTotalNumberOfBytesE, lTotalFreeBytesE, hdE);
  SET_CONTROL_LABEL(i_lblp1,hdC);
  SET_CONTROL_LABEL(i_lblp3,hdE);

  //For X, Y, Z
  CStdString hdX, hdY, hdZ;
  GetDiskSpace("X", lTotalNumberOfBytesX, lTotalFreeBytesX, hdX);
  GetDiskSpace("Y", lTotalNumberOfBytesY, lTotalFreeBytesY, hdY);
  GetDiskSpace("Z", lTotalNumberOfBytesZ, lTotalFreeBytesZ, hdZ);

  // Total Free Size: Generate from Drives#
  ULARGE_INTEGER lTotalDiscSpace;
  lTotalDiscSpace.QuadPart = (
    lTotalNumberOfBytesC.QuadPart +
    lTotalNumberOfBytesE.QuadPart +
    lTotalNumberOfBytesX.QuadPart +
    lTotalNumberOfBytesY.QuadPart +
    lTotalNumberOfBytesZ.QuadPart );

  // Total Free Size: Generate from Drives#
  ULARGE_INTEGER lTotalDiscFree;
  lTotalDiscFree.QuadPart = (
    lTotalFreeBytesC.QuadPart +
    lTotalFreeBytesE.QuadPart +
    lTotalFreeBytesX.QuadPart +
    lTotalFreeBytesY.QuadPart +
    lTotalFreeBytesZ.QuadPart );

  //For F and G
  CStdString hdF,hdG;
  bool bUseDriveF = GetDiskSpace("F", lTotalNumberOfBytesF, lTotalFreeBytesF, hdF);
  bool bUseDriveG = GetDiskSpace("G", lTotalNumberOfBytesG, lTotalFreeBytesG, hdG);

  if (bUseDriveF) {
    lTotalDiscSpace.QuadPart = lTotalDiscSpace.QuadPart + lTotalNumberOfBytesF.QuadPart;
    lTotalDiscFree.QuadPart = lTotalDiscFree.QuadPart + lTotalFreeBytesF.QuadPart;
  }
  if (bUseDriveG) {
    lTotalDiscSpace.QuadPart = lTotalDiscSpace.QuadPart + lTotalNumberOfBytesG.QuadPart;
    lTotalDiscFree.QuadPart = lTotalDiscFree.QuadPart + lTotalFreeBytesG.QuadPart;
  }
  // Total USED Size: Generate from Drives#
  ULARGE_INTEGER lTotalDiscUsed;
  ULARGE_INTEGER lTotalDiscPercent;

  lTotalDiscUsed.QuadPart   = lTotalDiscSpace.QuadPart - lTotalDiscFree.QuadPart;
  lTotalDiscPercent.QuadPart  = lTotalDiscSpace.QuadPart/100;  // => 1%

  CStdString hdTotalSize, hdTotalUsedPercent, t1,t2,t3;
  t1.Format("%u",lTotalDiscSpace.QuadPart/MB);
  t2.Format("%u",lTotalDiscUsed.QuadPart/MB);
  t3.Format("%u",lTotalDiscFree.QuadPart/MB);
  hdTotalSize.Format(g_localizeStrings.Get(20161), t1, t2, t3);  //Total Free To make it MB
  //hdTotalSize.Format("Total: %u MB, Used: %u MB, Free: %u MB ", lTotalDiscSpace.QuadPart/MB, lTotalDiscUsed.QuadPart/MB, lTotalDiscFree.QuadPart/MB );  //Total Free To make it MB

  int percentUsed = (int)(100.0f * lTotalDiscUsed.QuadPart/lTotalDiscSpace.QuadPart + 0.5f);
  hdTotalUsedPercent.Format(g_localizeStrings.Get(20162), percentUsed, 100 - percentUsed); //Total Free %

  CLog::Log(LOGDEBUG, "------------- HDD Space Info: -------------------");
  CLog::Log(LOGDEBUG, "HDD Total Size: %u MB", lTotalDiscSpace.QuadPart/MB);
  CLog::Log(LOGDEBUG, "HDD Used Size: %u MB", lTotalDiscUsed.QuadPart/MB);
  CLog::Log(LOGDEBUG, "HDD Free Size: %u MB", lTotalDiscFree.QuadPart/MB);
  CLog::Log(LOGDEBUG, "--------------HDD Percent Info: -----------------");
  CLog::Log(LOGDEBUG, "HDD Used Percent: %u%%", lTotalDiscUsed.QuadPart/lTotalDiscPercent.QuadPart );
  CLog::Log(LOGDEBUG, "HDD Free Percent: %u%%", lTotalDiscFree.QuadPart/lTotalDiscPercent.QuadPart );
  CLog::Log(LOGDEBUG, "-------------------------------------------------");

  // Detect which to show!!
  if(bUseDriveF)  // Show if Drive F is availible
  {
    SET_CONTROL_LABEL(i_lblp4,hdF);
    if(bUseDriveG)
    {
      SET_CONTROL_LABEL(i_lblp5,hdG);
      SET_CONTROL_LABEL(i_lblp6,hdX);
      SET_CONTROL_LABEL(i_lblp7,hdY);
      SET_CONTROL_LABEL(i_lblp8,hdZ);
      SET_CONTROL_LABEL(i_lblp9,hdTotalSize);
      SET_CONTROL_LABEL(i_lblp10,hdTotalUsedPercent);
    }
    else
    {
      SET_CONTROL_LABEL(i_lblp5,hdX);
      SET_CONTROL_LABEL(i_lblp6,hdY);
      SET_CONTROL_LABEL(i_lblp7,hdZ);
      SET_CONTROL_LABEL(i_lblp8,hdTotalSize);
      SET_CONTROL_LABEL(i_lblp9,hdTotalUsedPercent);
    }

  }
  else  // F and G not available
  {
    SET_CONTROL_LABEL(i_lblp4,hdX);
    SET_CONTROL_LABEL(i_lblp5,hdY);
    SET_CONTROL_LABEL(i_lblp6,hdZ);
    SET_CONTROL_LABEL(i_lblp7,hdTotalSize);
    SET_CONTROL_LABEL(i_lblp8,hdTotalUsedPercent);
  }

#ifdef _DEBUG
  //Only DebugOutput!
  MEMORYSTATUS stat;
  CHAR strOut[1024], *pstrOut;
  // Get the memory status.
  GlobalMemoryStatus( &stat );
  // Setup the output string.
  pstrOut = strOut;
  AddStr( "%4d total MB of virtual memory.\n", stat.dwTotalVirtual / MB );
  AddStr( "%4d  free MB of virtual memory.\n", stat.dwAvailVirtual / MB );
  AddStr( "%4d total MB of physical memory.\n", stat.dwTotalPhys / MB );
  AddStr( "%4d  free MB of physical memory.\n", stat.dwAvailPhys / MB );
  AddStr( "%4d total MB of paging file.\n", stat.dwTotalPageFile / MB );
  AddStr( "%4d  free MB of paging file.\n", stat.dwAvailPageFile / MB );
  AddStr( "%4d  percent of memory is in use.\n", stat.dwMemoryLoad );
  OutputDebugString( strOut );
#endif
  return true;
}

bool CGUIWindowSystemInfo::GetDiskSpace(const CStdString &drive, ULARGE_INTEGER &total, ULARGE_INTEGER& totalFree, CStdString &string)
{
  CStdString driveName = drive + ":\\";
  CStdString t1,t2;
  BOOL ret;
  if ((ret = GetDiskFreeSpaceEx(driveName.c_str(), NULL, &total, &totalFree)))
  {
    t1.Format("%u",totalFree.QuadPart/MB);
    t2.Format("%u",total.QuadPart/MB);
    string.Format(g_localizeStrings.Get(20163), drive, t1, t2);
    //string.Format("%s: %u MB of %u MB %s", drive, (totalFree.QuadPart/MB), (total.QuadPart/MB), g_localizeStrings.Get(160));
  }
  else
  {
    string.Format("%s %s: %s", g_localizeStrings.Get(155), drive, g_localizeStrings.Get(161));
    total.QuadPart = 0;
    totalFree.QuadPart = 0;
  }
  return ret == TRUE;
}
#endif
bool CGUIWindowSystemInfo::GetBuildTime(int label1, int label2, int label3)
{
  CStdString version, buildDate, mplayerVersion;
  version.Format("%s %s", g_localizeStrings.Get(144), g_infoManager.GetVersion());
  buildDate.Format("XBMC %s (Compiled :%s)", version, g_infoManager.GetBuild());
#ifdef HAS_SYSINFO
  mplayerVersion.Format("%s",strMplayerVersion);
#endif
  SET_CONTROL_LABEL(label1, version);
  SET_CONTROL_LABEL(label2, buildDate);
  SET_CONTROL_LABEL(label3, mplayerVersion);
  return true;
}
#ifdef HAS_SYSINFO
bool CGUIWindowSystemInfo::GetUnits(int i_lblp1, int i_lblp2, int i_lblp3 )
{
  // Get the Connected Units on the Front USB Ports!
  DWORD dwDeviceGamePad   = XGetDevices(XDEVICE_TYPE_GAMEPAD);      char* sclDeviceVle;
  DWORD dwDeviceKeyboard    = XGetDevices(XDEVICE_TYPE_DEBUG_KEYBOARD);   char* sclDeviceKeyb;
  DWORD dwDeviceMouse     = XGetDevices(XDEVICE_TYPE_DEBUG_MOUSE);    char* sclDeviceMouse;
  DWORD dwDeviceHeadPhone   = XGetDevices(XDEVICE_TYPE_VOICE_HEADPHONE);  char* sclDeviceHeadPhone;
  DWORD dwDeviceMicroPhone  = XGetDevices(XDEVICE_TYPE_VOICE_MICROPHONE); char* sclDeviceMicroPhone;
  DWORD dwDeviceMemory    = XGetDevices(XDEVICE_TYPE_MEMORY_UNIT);    char* sclDeviceMemory;
  DWORD dwDeviceIRRemote    = XGetDevices(XDEVICE_TYPE_IR_REMOTE);      char* sclDeviceIRRemote;

  CStdString strlblGamePads = g_localizeStrings.Get(13163); //"GamePads On Port:";
  CStdString strlblKeyboard = g_localizeStrings.Get(13164); //"Keyboard On Port:";
  CStdString strlblMouse    = g_localizeStrings.Get(13165); //"Mouse On Port:";
  CStdString strlblHeadMicro  = g_localizeStrings.Get(13166); //"Head/MicroPhone On Ports:";
  CStdString strlblMemoryStk  = g_localizeStrings.Get(13167); //"MemoryStick On Port:";
  CStdString strlblIRRemote = g_localizeStrings.Get(13168); //"IR-Remote On Port:";

  if (dwDeviceGamePad > 0)
  {
    switch (dwDeviceGamePad)
    {
    case 1:   // Values 1 ->  on Port 1
      sclDeviceVle = "1";
      break;
    case 2:   // Values 2 ->  on Port 2
      sclDeviceVle = "2";
      break;
    case 4:   // Values 4 ->  on Port 3
      sclDeviceVle = "3";
      break;
    case 8:   // Values 8 ->  on Port 4
      sclDeviceVle = "4";
      break;
    case 3:   // Values 3 ->  on Port 1&2
      sclDeviceVle = "1, 2";
      break;
    case 5:   // Values 5 ->  on Port 1&3
      sclDeviceVle = "1, 3";
      break;
    case 6:   // Values 6 ->  on Port 2&3
      sclDeviceVle = "2, 3";
      break;
    case 7:   // Values 7 ->  on Port 1&2&3
      sclDeviceVle = "1, 2, 3";
      break;
    case 9:   // Values 9  -> on Port 1&4
      sclDeviceVle = "1, 4";
      break;
    case 10:  // Values 10 -> on Port 2&4
      sclDeviceVle = "2, 4";
      break;
    case 11:  // Values 11 -> on Port 1&2&4
      sclDeviceVle = "1, 2, 4";
      break;
    case 12:  // Values 12 -> on Port 3&4
      sclDeviceVle = "3, 4";
      break;
    case 13:  // Values 13 -> on Port 1&3&4
      sclDeviceVle = "1, 3, 4";
      break;
    case 14:  // Values 14 -> on Port 2&3&4
      sclDeviceVle = "2, 3, 4";
      break;
    case 15:  // Values 15 -> on Port 1&2&3&4
      sclDeviceVle = "1, 2, 3, 4";
      break;
    default:
      sclDeviceVle = "-";
    }
  }
  else sclDeviceVle = "0";
  if (dwDeviceKeyboard > 0)
  {
    switch (dwDeviceKeyboard)
    {
    case 1:   // Values 1 ->  on Port 1
      sclDeviceKeyb = "1";
      break;
    case 2:   // Values 2 ->  on Port 2
      sclDeviceKeyb = "2";
      break;
    case 4:   // Values 4 ->  on Port 3
      sclDeviceKeyb = "3";
      break;
    case 8:   // Values 8 ->  on Port 4
      sclDeviceKeyb = "4";
      break;
    default:
      sclDeviceKeyb = "-";
    }
  }
  else sclDeviceKeyb = "0";
  if (dwDeviceMouse > 0)
  {
    switch (dwDeviceMouse)
    {
    case 1:   // Values 1 ->  on Port 1
      sclDeviceMouse = "1";
      break;
    case 2:   // Values 2 ->  on Port 2
      sclDeviceMouse = "2";
      break;
    case 4:   // Values 4 ->  on Port 3
      sclDeviceMouse = "3";
      break;
    case 8:   // Values 8 ->  on Port 4
      sclDeviceMouse = "4";
      break;
    default:
      sclDeviceMouse = "-";
    }
  }
  else sclDeviceMouse = "0";
  if (dwDeviceHeadPhone > 0)
  {
    switch (dwDeviceHeadPhone)
    {
    case 1:   // Values 1 ->  on Port 1
      sclDeviceHeadPhone = "1";
      break;
    case 2:   // Values 2 ->  on Port 2
      sclDeviceHeadPhone = "2";
      break;
    case 4:   // Values 4 ->  on Port 3
      sclDeviceHeadPhone = "3";
      break;
    case 8:   // Values 8 ->  on Port 4
      sclDeviceHeadPhone = "4";
      break;
    default:
      sclDeviceHeadPhone = "-";
    }
  }
  else sclDeviceHeadPhone = "0";
  if (dwDeviceMicroPhone > 0)
  {
    switch (dwDeviceMicroPhone)
    {
    case 1:   // Values 1 ->  on Port 1
      sclDeviceMicroPhone = "1";
      break;
    case 2:   // Values 2 ->  on Port 2
      sclDeviceMicroPhone = "2";
      break;
    case 4:   // Values 4 ->  on Port 3
      sclDeviceMicroPhone = "3";
      break;
    case 8:   // Values 8 ->  on Port 4
      sclDeviceMicroPhone = "4";
      break;
    default:
      sclDeviceMicroPhone = "-";
    }
  }
  else sclDeviceMicroPhone ="0";
  if (dwDeviceMemory > 0)
  {
    switch (dwDeviceMemory)
    {
    case 1:   // Values 1 ->  on Port 1
      sclDeviceMemory = "1";
      break;
    case 2:   // Values 2 ->  on Port 2
      sclDeviceMemory = "2";
      break;
    case 4:   // Values 4 ->  on Port 3
      sclDeviceMemory = "3";
      break;
    case 8:   // Values 8 ->  on Port 4
      sclDeviceMemory = "4";
      break;
    case 3:   // Values 3 ->  on Port 1&2
      sclDeviceMemory = "1, 2";
      break;
    case 5:   // Values 5 ->  on Port 1&3
      sclDeviceMemory = "1, 3";
      break;
    case 6:   // Values 6 ->  on Port 2&3
      sclDeviceMemory = "2, 3";
      break;
    case 7:   // Values 7 ->  on Port 1&2&3
      sclDeviceMemory = "1, 2, 3";
      break;
    case 9:   // Values 9  -> on Port 1&4
      sclDeviceMemory = "1, 4";
      break;
    case 10:  // Values 10 -> on Port 2&4
      sclDeviceMemory = "2, 4";
      break;
    case 11:  // Values 11 -> on Port 1&2&4
      sclDeviceMemory = "1, 2, 4";
      break;
    case 12:  // Values 12 -> on Port 3&4
      sclDeviceMemory = "3, 4";
      break;
    case 13:  // Values 13 -> on Port 1&3&4
      sclDeviceMemory = "1, 3, 4";
      break;
    case 14:  // Values 14 -> on Port 2&3&4
      sclDeviceMemory = "2, 3, 4";
      break;
    case 15:  // Values 15 -> on Port 1&2&3&4
      sclDeviceMemory = "1, 2, 3, 4";
      break;
    default:
      sclDeviceMemory = "-";
    }
  }
  else sclDeviceMemory = "0";
  if(dwDeviceIRRemote > 0)
  {
    switch (dwDeviceIRRemote)
    {
    case 1:   // Values 1 ->  on Port 1
      sclDeviceIRRemote = "1";
      break;
    case 2:   // Values 2 ->  on Port 2
      sclDeviceIRRemote = "2";
      break;
    case 4:   // Values 4 ->  on Port 3
      sclDeviceIRRemote = "3";
      break;
    case 8:   // Values 8 ->  on Port 4
      sclDeviceIRRemote = "4";
      break;
    default:
      sclDeviceIRRemote = "-";
    }
  }
  else sclDeviceIRRemote = "0";

  CStdString strItem1, strItem2, strItem3, strItem4;
  CStdString strItem5, strItem6, strItem7, strItem8;

  strItem1.Format("%s %s", strlblGamePads, sclDeviceVle);
  strItem2.Format("%s %s", strlblKeyboard, sclDeviceKeyb);
  strItem3.Format("%s %s", strlblMouse, sclDeviceMouse);
  strItem4.Format("%s %s %s", dwDeviceGamePad == 0 ? "" : strItem1, 
                              dwDeviceKeyboard == 0 ? "" : strItem2, 
                              dwDeviceMouse == 0 ? "" : strItem3);

  strItem5.Format("%s %s",  strlblIRRemote, sclDeviceIRRemote);
  //strItem6.Format("%s %s-%s", strlblHeadMicro, sclDeviceHeadPhone, sclDeviceMicroPhone );
  strItem6.Format("%s %s", strlblHeadMicro, sclDeviceHeadPhone, sclDeviceMicroPhone );  // Head and Micro are normly on the same port!
  strItem7.Format("%s %s", dwDeviceIRRemote == 0 ? "" : strItem5,
                           dwDeviceHeadPhone == 0 ? "" : strItem6);

  // !? Show Memory stick, because it only shows with USB->MemoryStick adapter!
  strItem8.Format("%s %s", strlblMemoryStk, sclDeviceMemory);
  SET_CONTROL_LABEL(i_lblp3, dwDeviceMemory == 0 ? "" : strItem8); // MemoryStick

  CLog::Log(LOGDEBUG,"- GamePads are Connected on Port:   %s (%d)", sclDeviceVle, dwDeviceGamePad );
  CLog::Log(LOGDEBUG,"- Keyboard is Connected on Port:    %s (%d)", sclDeviceKeyb, dwDeviceKeyboard);
  CLog::Log(LOGDEBUG,"- Mouse is Connected on Port:       %s (%d)", sclDeviceMouse, dwDeviceMouse);
  CLog::Log(LOGDEBUG,"- IR-Remote is Connected on Port:   %s (%d)", sclDeviceIRRemote, dwDeviceIRRemote);
  CLog::Log(LOGDEBUG,"- Head Phone is Connected on Port:  %s (%d)", sclDeviceHeadPhone, dwDeviceHeadPhone);
  CLog::Log(LOGDEBUG,"- Micro Phone is Connected on Port: %s (%d)", sclDeviceMicroPhone, dwDeviceMicroPhone);
  CLog::Log(LOGDEBUG,"- MemoryStick is Connected on Port: %d (%d)", dwDeviceMemory, dwDeviceMemory);

  SET_CONTROL_LABEL(i_lblp1, strItem4); // GamePad, Keyboard, Mouse
  SET_CONTROL_LABEL(i_lblp2, strItem7); // Remote, Headset & MicroPhone
  return true;
}

void CGUIWindowSystemInfo::CreateEEPROMBackup(LPCSTR BackupFilePrefix)
{
  char backup_path[MAX_PATH];

  wsprintf(backup_path, "Q:\\%s\\EEPROMBackup.bin", BackupFilePrefix);
  m_pXKEEPROM->WriteToBINFile(backup_path);

  wsprintf(backup_path, "Q:\\%s\\EEPROMBackup.cfg", BackupFilePrefix);
  m_pXKEEPROM->WriteToCFGFile(backup_path);
}

#define SYSINFO_TMP_SIZE 256

void CGUIWindowSystemInfo::WriteTXTInfoFile(LPCSTR strFilename)
{
  BOOL retVal = FALSE;
  DWORD dwBytesWrote = 0;
  CHAR tmpData[SYSINFO_TMP_SIZE];
  CStdString tmpstring;
  LPSTR tmpFileStr = new CHAR[2048];
  ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
  ZeroMemory(tmpFileStr, 2048);

  HANDLE hf = CreateFile(strFilename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hf !=  INVALID_HANDLE_VALUE)
  {
    //Write File Header..
    strcat(tmpFileStr, "*******  XBOXMEDIACENTER [XBMC] INFORMATION FILE  *******\r\n");
    if (m_XBOX_Version== m_pXKEEPROM->V1_0)
      strcat(tmpFileStr, "\r\nXBOX Version = \t\tV1.0");
    else if (m_XBOX_Version == m_pXKEEPROM->V1_1)
      strcat(tmpFileStr, "\r\nXBOX Version = \t\tV1.1");
    else if (m_XBOX_Version == m_pXKEEPROM->V1_6)
      strcat(tmpFileStr,  "\r\nXBOX Version = \t\tV1.6");
    //Get Kernel Version
    ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
    sprintf(tmpData, "\r\nKernel Version: \t%d.%d.%d.%d", XboxKrnlVersion->VersionMajor,XboxKrnlVersion->VersionMinor,XboxKrnlVersion->Build,XboxKrnlVersion->Qfe);
    strcat(tmpFileStr, tmpData);

    //Get Memory Status
    strcat(tmpFileStr, "\r\nXBOX RAM = \t\t");
    ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
    MEMORYSTATUS stat;
    GlobalMemoryStatus( &stat );
    ltoa(stat.dwTotalPhys/1024/1024, tmpData, 10);
    strcat(tmpFileStr, tmpData);
    strcat(tmpFileStr, " MBytes");

    //Write Serial Number..
    strcat(tmpFileStr, "\r\n\r\nXBOX Serial Number = \t");
    GetXBOXSerial(tmpstring);
    strcat(tmpFileStr, tmpstring.c_str());

    //Write MAC Address..
    strcat(tmpFileStr, "\r\nXBOX MAC Address = \t");
    GetMACAddress(tmpstring);
    strcat(tmpFileStr, tmpstring.c_str());

    //Write Online Key ..
    strcat(tmpFileStr, "\r\nXBOX Online Key = \t");
    GetXBLiveKey(tmpstring);
    strcat(tmpFileStr, tmpstring.c_str());

    //Write VideoMode ..
    strcat(tmpFileStr, "\r\nXBOX Video Mode = \t");
    VIDEO_STANDARD vdo = m_pXKEEPROM->GetVideoStandardVal();
    if (vdo == XKEEPROM::VIDEO_STANDARD::PAL_I)
      strcat(tmpFileStr, "PAL");
    else
      strcat(tmpFileStr, "NTSC");

    //Write XBE Region..
    strcat(tmpFileStr, "\r\nXBOX XBE Region = \t");
    ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
    m_pXKEEPROM->GetXBERegionString(tmpData);
    strcat(tmpFileStr, tmpData);

    //Write HDDKey..
    strcat(tmpFileStr, "\r\nXBOX HDD Key = \t\t");
    ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
    m_pXKEEPROM->GetHDDKeyString(tmpData);
    strcat(tmpFileStr, tmpData);

    //Write Confounder..
    strcat(tmpFileStr, "\r\nXBOX Confounder = \t");
    ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
    m_pXKEEPROM->GetConfounderString(tmpData);
    strcat(tmpFileStr, tmpData);

    //GET HDD Info...
    //Query ATA IDENTIFY
    XKHDD::ATA_COMMAND_OBJ cmdObj;
    ZeroMemory(&cmdObj, sizeof(XKHDD::ATA_COMMAND_OBJ));
    cmdObj.IPReg.bCommandReg = IDE_ATA_IDENTIFY;
    cmdObj.IPReg.bDriveHeadReg = IDE_DEVICE_MASTER;
    XKHDD::SendATACommand(IDE_PRIMARY_PORT, &cmdObj, IDE_COMMAND_READ);

    //Write HDD Model
    strcat(tmpFileStr, "\r\n\r\nXBOX HDD Model = \t");
    ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
    XKHDD::GetIDEModel(cmdObj.DATA_BUFFER, (LPSTR)tmpData);
    strcat(tmpFileStr, tmpData);

    //Write HDD Serial..
    strcat(tmpFileStr, "\r\nXBOX HDD Serial = \t");
    ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
    XKHDD::GetIDESerial(cmdObj.DATA_BUFFER, (LPSTR)tmpData);
    strcat(tmpFileStr, tmpData);

    //Write HDD Password..
    ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
    strcat(tmpFileStr, "\r\n\r\nXBOX HDD Password = \t");

    ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
    BYTE HDDpwd[20];
    ZeroMemory(HDDpwd, 20);
    XKHDD::GenerateHDDPwd((UCHAR *)XboxHDKey, cmdObj.DATA_BUFFER, (UCHAR*)&HDDpwd);
    XKGeneral::BytesToHexStr(HDDpwd, 20, tmpData);
    strcat(tmpFileStr, tmpData);

    //Query ATAPI IDENTIFY
    ZeroMemory(&cmdObj, sizeof(XKHDD::ATA_COMMAND_OBJ));
    cmdObj.IPReg.bCommandReg = IDE_ATAPI_IDENTIFY;
    cmdObj.IPReg.bDriveHeadReg = IDE_DEVICE_SLAVE;
    XKHDD::SendATACommand(IDE_PRIMARY_PORT, &cmdObj, IDE_COMMAND_READ);

    //Write DVD Model
    strcat(tmpFileStr, "\r\n\r\nXBOX DVD Model = \t");
    ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
    XKHDD::GetIDEModel(cmdObj.DATA_BUFFER, (LPSTR)tmpData);
    strcat(tmpFileStr, tmpData);
    strupr(tmpFileStr);

    WriteFile(hf, tmpFileStr, (DWORD)strlen(tmpFileStr), &dwBytesWrote, NULL);
  }
  delete[] tmpFileStr;
  CloseHandle(hf);
}


bool CGUIWindowSystemInfo::GetSystemUpTime(CStdString& strSystemUptime)
{
  CStdString lbl1 = g_localizeStrings.Get(12390);
  CStdString lblMin = g_localizeStrings.Get(12391);
  CStdString lblHou = g_localizeStrings.Get(12392);
  CStdString lblDay = g_localizeStrings.Get(12393);

  int iInputMinutes, iMinutes,iHours,iDays;
  iInputMinutes = (int)(timeGetTime() / 60000);
  CSysInfo::SystemUpTime(iInputMinutes,iMinutes, iHours, iDays);
  // Will Display Autodetected Values!
  if (iDays > 0) strSystemUptime.Format("%s: %i %s, %i %s, %i %s",lbl1, iDays,lblDay, iHours,lblHou, iMinutes, lblMin);
  else if (iDays == 0 && iHours >= 1 ) strSystemUptime.Format("%s: %i %s, %i %s",lbl1, iHours,lblHou, iMinutes, lblMin);
  else if (iDays == 0 && iHours == 0 &&  iMinutes >= 0) strSystemUptime.Format("%s: %i %s",lbl1, iMinutes, lblMin);

  return true;
}

bool CGUIWindowSystemInfo::GetSystemTotalUpTime(CStdString& strSystemUptime)
{
  CStdString lbl1 = g_localizeStrings.Get(12394);
  CStdString lblMin = g_localizeStrings.Get(12391);
  CStdString lblHou = g_localizeStrings.Get(12392);
  CStdString lblDay = g_localizeStrings.Get(12393);

  int iInputMinutes, iMinutes,iHours,iDays;
  iInputMinutes = g_stSettings.m_iSystemTimeTotalUp + ((int)(timeGetTime() / 60000));
  CSysInfo::SystemUpTime(iInputMinutes,iMinutes, iHours, iDays);
  // Will Display Autodetected Values!
  if (iDays > 0) strSystemUptime.Format("%s: %i %s, %i %s, %i %s",lbl1, iDays,lblDay, iHours,lblHou, iMinutes, lblMin);
  else if (iDays == 0 && iHours >= 1 ) strSystemUptime.Format("%s: %i %s, %i %s",lbl1, iHours,lblHou, iMinutes, lblMin);
  else if (iDays == 0 && iHours == 0 &&  iMinutes >= 0) strSystemUptime.Format("%s: %i %s",lbl1, iMinutes, lblMin);

  return true;
}
#endif