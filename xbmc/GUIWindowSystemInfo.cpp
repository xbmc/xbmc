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
#include "GUIWindowSystemInfo.h"
#include "utils/GUIInfoManager.h"
#ifdef HAS_SYSINFO
  #include "utils/SystemInfo.h"
#endif

CGUIWindowSystemInfo::CGUIWindowSystemInfo(void)
:CGUIWindow(WINDOW_SYSTEM_INFORMATION, "SettingsSystemInfo.xml")
{
  iControl = CONTROL_BT_DEFAULT;
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
      iControl=message.GetSenderId();
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowSystemInfo::Render()
{
  if(iControl == CONTROL_BT_DEFAULT)
  {
    SetLabelDummy();
    // Default Values
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20154));
    SET_CONTROL_LABEL(2, g_infoManager.GetSystemHeatInfo(SYSTEM_CPU_TEMPERATURE)); // CPU Temperature
    SET_CONTROL_LABEL(3, g_infoManager.GetSystemHeatInfo(SYSTEM_GPU_TEMPERATURE)); // GPU Temperature
    SET_CONTROL_LABEL(4, g_infoManager.GetSystemHeatInfo(SYSTEM_FAN_SPEED)); // Fan Speed
    SET_CONTROL_LABEL(5, g_localizeStrings.Get(158) +": "+ g_infoManager.GetLabel(SYSTEM_FREE_MEMORY));
    SET_CONTROL_LABEL(6, g_infoManager.GetLabel(NETWORK_IP_ADDRESS));
    SET_CONTROL_LABEL(7,g_infoManager.GetLabel(SYSTEM_SCREEN_RESOLUTION));
#ifdef HAS_SYSINFO
    SET_CONTROL_LABEL(8,g_infoManager.GetLabel(SYSTEM_KERNEL_VERSION));
    SET_CONTROL_LABEL(9,g_infoManager.GetLabel(SYSTEM_UPTIME));
    SET_CONTROL_LABEL(10,g_infoManager.GetLabel(SYSTEM_TOTALUPTIME));
#endif
  }
  else if(iControl == CONTROL_BT_HDD)
  {
    SetLabelDummy();
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20156));
    #ifdef HAS_SYSINFO
    SET_CONTROL_LABEL(2, g_infoManager.GetLabel(SYSTEM_HDD_MODEL));
    SET_CONTROL_LABEL(3, g_infoManager.GetLabel(SYSTEM_HDD_SERIAL));
    SET_CONTROL_LABEL(4, g_infoManager.GetLabel(SYSTEM_HDD_FIRMWARE));
    SET_CONTROL_LABEL(5, g_infoManager.GetLabel(SYSTEM_HDD_PASSWORD));
    SET_CONTROL_LABEL(6, g_infoManager.GetLabel(SYSTEM_HDD_LOCKSTATE));
    SET_CONTROL_LABEL(7, g_infoManager.GetLabel(SYSTEM_HDD_LOCKKEY));
    SET_CONTROL_LABEL(8, g_infoManager.GetLabel(SYSTEM_HDD_BOOTDATE));
    SET_CONTROL_LABEL(9, g_infoManager.GetLabel(SYSTEM_HDD_CYCLECOUNT));
    SET_CONTROL_LABEL(10, g_infoManager.GetLabel(SYSTEM_HDD_TEMPERATURE));
    #endif
  }
  else if(iControl == CONTROL_BT_DVD)
  {
    SetLabelDummy();
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20157));
#ifdef HAS_SYSINFO
    SET_CONTROL_LABEL(2, g_infoManager.GetLabel(SYSTEM_DVD_MODEL));
    SET_CONTROL_LABEL(3, g_infoManager.GetLabel(SYSTEM_DVD_FIRMWARE));
#endif
  }
  else if(iControl == CONTROL_BT_STORAGE)
  {
    SetLabelDummy();
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20155));

#ifdef HAS_SYSINFO
    // Label 2-10: Storage Values
    GetStorage(2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
#endif
  }
  else if(iControl == CONTROL_BT_NETWORK)
  {
    SetLabelDummy();
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20158));
#ifdef HAS_SYSINFO
    // Network Informations
    SET_CONTROL_LABEL(2, g_infoManager.GetLabel(NETWORK_IS_DHCP));
    SET_CONTROL_LABEL(3, g_infoManager.GetLabel(NETWORK_LINK_STATE));
    SET_CONTROL_LABEL(4, g_infoManager.GetLabel(NETWORK_MAC_ADDRESS));
    SET_CONTROL_LABEL(5, g_infoManager.GetLabel(NETWORK_IP_ADDRESS));
    SET_CONTROL_LABEL(6, g_infoManager.GetLabel(NETWORK_SUBNET_ADDRESS));
    SET_CONTROL_LABEL(7, g_infoManager.GetLabel(NETWORK_GATEWAY_ADDRESS));
    SET_CONTROL_LABEL(8, g_infoManager.GetLabel(NETWORK_DNS1_ADDRESS));
    SET_CONTROL_LABEL(9, g_infoManager.GetLabel(NETWORK_DNS2_ADDRESS));
    SET_CONTROL_LABEL(10, g_infoManager.GetLabel(SYSTEM_INTERNET_STATE));
#endif
  }
  else if(iControl == CONTROL_BT_VIDEO)
  {
    SetLabelDummy();
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20159));
#ifdef HAS_SYSINFO
    SET_CONTROL_LABEL(2,g_infoManager.GetLabel(SYSTEM_VIDEO_ENCODER_INFO));
    SET_CONTROL_LABEL(3,g_infoManager.GetLabel(SYSTEM_SCREEN_RESOLUTION));
    SET_CONTROL_LABEL(4,g_infoManager.GetLabel(SYSTEM_AV_CABLE_PACK_INFO));
    SET_CONTROL_LABEL(5,g_infoManager.GetLabel(SYSTEM_VIDEO_XBE_REGION));
    SET_CONTROL_LABEL(6,g_infoManager.GetLabel(SYSTEM_VIDEO_DVD_ZONE));
#endif
  }
  else if(iControl == CONTROL_BT_HARDWARE)
  {
    SetLabelDummy();
    // Label 1: Hardware Informations
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20160));
#ifdef HAS_SYSINFO
    SET_CONTROL_LABEL(2, g_infoManager.GetLabel(SYSTEM_XBOX_VERSION));
    SET_CONTROL_LABEL(3, g_infoManager.GetLabel(SYSTEM_XBOX_SERIAL));
    SET_CONTROL_LABEL(4, g_infoManager.GetLabel(SYSTEM_CPUFREQUENCY));
    SET_CONTROL_LABEL(5, g_infoManager.GetLabel(SYSTEM_XBOX_BIOS));
    SET_CONTROL_LABEL(6, g_infoManager.GetLabel(SYSTEM_XBOX_MODCHIP));
    SET_CONTROL_LABEL(7, g_infoManager.GetLabel(SYSTEM_XBOX_PRODUCE_INFO));
    SET_CONTROL_LABEL(8, g_infoManager.GetLabel(SYSTEM_CONTROLLER_PORT_1));
    SET_CONTROL_LABEL(9, g_infoManager.GetLabel(SYSTEM_CONTROLLER_PORT_2));
    SET_CONTROL_LABEL(10, g_infoManager.GetLabel(SYSTEM_CONTROLLER_PORT_3));
    SET_CONTROL_LABEL(11, g_infoManager.GetLabel(SYSTEM_CONTROLLER_PORT_4));

    // Creating BackUP takes to long will moved to buildin execute
    //g_sysinfo.CreateEEPROMBackup();
    //g_sysinfo.CreateBiosBackup();
    //g_sysinfo.WriteTXTInfoFile("Q:\\System\\SystemInfo\\SYSTEM_INFO.TXT");
    //
#endif
  }
  SET_CONTROL_LABEL(50, g_infoManager.GetTime(true) + " | " + g_infoManager.GetDate());
  SET_CONTROL_LABEL(51, g_localizeStrings.Get(144)+" "+g_infoManager.GetVersion());
  SET_CONTROL_LABEL(52, "XBMC "+g_infoManager.GetLabel(SYSTEM_BUILD_VERSION)+" (Compiled : "+g_infoManager.GetLabel(SYSTEM_BUILD_DATE)+")");
  SET_CONTROL_LABEL(53, g_infoManager.GetLabel(SYSTEM_MPLAYER_VERSION));
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

  int percentUsed = (int)(100.0f * lTotalDiscUsed.QuadPart/lTotalDiscSpace.QuadPart + 0.5f);
  hdTotalUsedPercent.Format(g_localizeStrings.Get(20162), percentUsed, 100 - percentUsed); //Total Free %


  // To much log in Render() Mode
  //CLog::Log(LOGDEBUG, "------------- HDD Space Info: -------------------");
  //CLog::Log(LOGDEBUG, "HDD Total Size: %u MB", lTotalDiscSpace.QuadPart/MB);
  //CLog::Log(LOGDEBUG, "HDD Used Size: %u MB", lTotalDiscUsed.QuadPart/MB);
  //CLog::Log(LOGDEBUG, "HDD Free Size: %u MB", lTotalDiscFree.QuadPart/MB);
  //CLog::Log(LOGDEBUG, "--------------HDD Percent Info: -----------------");
  //CLog::Log(LOGDEBUG, "HDD Used Percent: %u%%", lTotalDiscUsed.QuadPart/lTotalDiscPercent.QuadPart );
  //CLog::Log(LOGDEBUG, "HDD Free Percent: %u%%", lTotalDiscFree.QuadPart/lTotalDiscPercent.QuadPart );
  //CLog::Log(LOGDEBUG, "-------------------------------------------------");


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

// To much log in Render() Mode
/*
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
*/
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