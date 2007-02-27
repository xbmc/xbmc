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
#endif

CGUIWindowSystemInfo::CGUIWindowSystemInfo(void)
:CGUIWindow(WINDOW_SYSTEM_INFORMATION, "SettingsSystemInfo.xml")
{
}

CGUIWindowSystemInfo::~CGUIWindowSystemInfo(void)
{
}

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
      CGUIWindow::OnMessage(message);
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
        SET_CONTROL_LABEL(2, g_infoManager.GetLabel(SYSTEM_HDD_MODEL));
        SET_CONTROL_LABEL(3, g_infoManager.GetLabel(SYSTEM_HDD_SERIAL));
        SET_CONTROL_LABEL(4, g_infoManager.GetLabel(SYSTEM_HDD_FIRMWARE));
        SET_CONTROL_LABEL(5, g_infoManager.GetLabel(SYSTEM_HDD_PASSWORD));
        SET_CONTROL_LABEL(6, g_infoManager.GetLabel(SYSTEM_HDD_LOCKSTATE));
        
        //Label 7: HDD Lock/UnLock key
        SET_CONTROL_LABEL(7, g_sysinfo.GetHDDKey());

        //Label 8 + 9: Refurb Info
        CStdString rfi_FirstBootTime, rfi_PowerCycleCount;
        g_sysinfo.GetRefurbInfo(rfi_FirstBootTime, rfi_PowerCycleCount);
        SET_CONTROL_LABEL(8, rfi_FirstBootTime);
        SET_CONTROL_LABEL(9, rfi_PowerCycleCount);

        //Label 10: HDD Temperature
        SET_CONTROL_LABEL(10, g_infoManager.GetLabel(SYSTEM_HDD_TEMPERATURE));
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
        SET_CONTROL_LABEL(2, g_infoManager.GetLabel(SYSTEM_DVD_MODEL));
        SET_CONTROL_LABEL(3, g_infoManager.GetLabel(SYSTEM_DVD_FIRMWARE));
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
        SET_CONTROL_LABEL(4, g_sysinfo.GetMACAddress());

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
        SET_CONTROL_LABEL(2,g_infoManager.GetLabel(SYSTEM_VIDEO_ENCODER_INFO));
        // Label 3: Resolution
        SET_CONTROL_LABEL(3,g_infoManager.GetLabel(SYSTEM_SCREEN_RESOLUTION));
        // Label 4: AV Pack Info
        SET_CONTROL_LABEL(4,g_infoManager.GetLabel(SYSTEM_AV_CABLE_PACK_INFO));

        // Label 5: XBE Video Region
        SET_CONTROL_LABEL(5,g_sysinfo.GetVideoXBERegion());

        // Label 6: DVD Zone
        SET_CONTROL_LABEL(6, g_sysinfo.GetDVDZone());
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
        SET_CONTROL_LABEL(2,g_infoManager.GetLabel(SYSTEM_XBOX_VERSION));

        // Label 3: XBOX Serial
        CStdString strXBOXSerial;
        strXBOXSerial.Format("%s %s",g_localizeStrings.Get(13289) ,g_sysinfo.GetXBOXSerial());
        SET_CONTROL_LABEL(3,strXBOXSerial);

        // Label 4: CPU Speed!
        SET_CONTROL_LABEL(4, g_infoManager.GetLabel(SYSTEM_CPUFREQUENCY));

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

        // Label 8: XBOX ProducutionDate Info
        SET_CONTROL_LABEL(7, g_sysinfo.GetXBProduceInfo());

        // Label 8,9,10,11: Attached Units!
        SET_CONTROL_LABEL(8, g_sysinfo.GetUnits(1));
        SET_CONTROL_LABEL(9, g_sysinfo.GetUnits(2));
        SET_CONTROL_LABEL(10, g_sysinfo.GetUnits(3));
        SET_CONTROL_LABEL(11, g_sysinfo.GetUnits(4));
        // Creating BackUP
        g_sysinfo.CreateEEPROMBackup();
        g_sysinfo.CreateBiosBackup();
        g_sysinfo.WriteTXTInfoFile("Q:\\System\\SystemInfo\\SYSTEM_INFO.TXT");
        //
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
    SET_CONTROL_LABEL(2, g_infoManager.GetSystemHeatInfo(SYSTEM_CPU_TEMPERATURE)); // CPU Temperature
    SET_CONTROL_LABEL(3, g_infoManager.GetSystemHeatInfo(SYSTEM_GPU_TEMPERATURE)); // GPU Temperature
    SET_CONTROL_LABEL(4, g_infoManager.GetSystemHeatInfo(SYSTEM_FAN_SPEED)); // Fan Speed
    
    // Label 5: Set FreeMemory Info
    SET_CONTROL_LABEL(5, g_localizeStrings.Get(158) +": "+ g_infoManager.GetLabel(SYSTEM_FREE_MEMORY));
    
    //Label 6: XBMC IP Adress
    SET_CONTROL_LABEL(6, g_infoManager.GetLabel(NETWORK_IP_ADDRESS));
    
    // Label 7: Set Resolution Info
    SET_CONTROL_LABEL(7,g_infoManager.GetLabel(SYSTEM_SCREEN_RESOLUTION));

#ifdef HAS_SYSINFO
    // Label 8: Get Kernel Info
    SET_CONTROL_LABEL(8,g_infoManager.GetLabel(SYSTEM_KERNEL_VERSION));

    // Label 9: Get System Uptime
    SET_CONTROL_LABEL(9,g_infoManager.GetLabel(SYSTEM_UPTIME));

    // Label 10: Get System Total Uptime
    SET_CONTROL_LABEL(10,g_infoManager.GetLabel(SYSTEM_TOTALUPTIME));
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
bool CGUIWindowSystemInfo::GetBuildTime(int label1, int label2, int label3)
{
  CStdString version, buildDate;
  version.Format("%s %s", g_localizeStrings.Get(144), g_infoManager.GetVersion());
  buildDate.Format("XBMC %s (Compiled :%s)", version, g_infoManager.GetBuild());
  SET_CONTROL_LABEL(label1, version);
  SET_CONTROL_LABEL(label2, buildDate);
  SET_CONTROL_LABEL(label3, g_infoManager.GetLabel(SYSTEM_MPLAYER_VERSION));
  return true;
}

#ifdef HAS_SYSINFO
bool CGUIWindowSystemInfo::GetBIOSInfo(CStdString& strBiosName)
{
  CStdString cBIOSName;
  CStdString strlblBios = g_localizeStrings.Get(13285);
  if(g_sysinfo.CheckBios(cBIOSName))
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
bool CGUIWindowSystemInfo::GetModChipInfo(CStdString& strModChip)
{
  // XBOX Modchip Type Detection
  CStdString ModChip    = g_sysinfo.GetModCHIPDetected();
  CStdString lblModChip = g_localizeStrings.Get(13291);
   // Check if it is a SmartXX
  CStdString strIsSmartXX = g_sysinfo.SmartXXModCHIP();
  if (!strIsSmartXX.Equals("None"))
  {
    strModChip.Format("%s %s", lblModChip.c_str(),strIsSmartXX);
    CLog::Log(LOGDEBUG, "- Detected ModChip: %s",strIsSmartXX.c_str());
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