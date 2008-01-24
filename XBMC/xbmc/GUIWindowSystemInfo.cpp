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
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20154));
    SET_CONTROL_LABEL(2, g_infoManager.GetSystemHeatInfo(SYSTEM_CPU_TEMPERATURE));
    SET_CONTROL_LABEL(3, g_infoManager.GetSystemHeatInfo(SYSTEM_GPU_TEMPERATURE));
    SET_CONTROL_LABEL(4, g_infoManager.GetSystemHeatInfo(SYSTEM_FAN_SPEED));
    CStdString tmpStr = g_localizeStrings.Get(158) +": ";
    tmpStr += g_infoManager.GetLabel(SYSTEM_FREE_MEMORY); 
    SET_CONTROL_LABEL(5, tmpStr);
    SET_CONTROL_LABEL(6, g_infoManager.GetLabel(NETWORK_IP_ADDRESS));
    SET_CONTROL_LABEL(7,g_infoManager.GetLabel(SYSTEM_SCREEN_RESOLUTION));
#ifdef HAS_SYSINFO
    SET_CONTROL_LABEL(8,g_infoManager.GetLabel(SYSTEM_KERNEL_VERSION));
#endif
    SET_CONTROL_LABEL(9,g_infoManager.GetLabel(SYSTEM_UPTIME));
    SET_CONTROL_LABEL(10,g_infoManager.GetLabel(SYSTEM_TOTALUPTIME));
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
    SET_CONTROL_LABEL(4, g_infoManager.GetLabel(SYSTEM_DVD_ZONE));
#endif
  }
  else if(iControl == CONTROL_BT_STORAGE)
  {
    SetLabelDummy();
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20155));
    // for backward compatibility just show Free space info else would be to long...
    SET_CONTROL_LABEL(2, g_infoManager.GetLabel(SYSTEM_FREE_SPACE_C));
#ifdef HAS_SYSINFO
    SET_CONTROL_LABEL(3, g_infoManager.GetLabel(SYSTEM_DVD_TRAY_STATE));
#endif
    SET_CONTROL_LABEL(4, g_infoManager.GetLabel(SYSTEM_FREE_SPACE_E));
    SET_CONTROL_LABEL(5, g_infoManager.GetLabel(SYSTEM_FREE_SPACE_F));
    SET_CONTROL_LABEL(6, g_infoManager.GetLabel(SYSTEM_FREE_SPACE_G));
    SET_CONTROL_LABEL(7, g_infoManager.GetLabel(SYSTEM_FREE_SPACE_X));
    SET_CONTROL_LABEL(8, g_infoManager.GetLabel(SYSTEM_FREE_SPACE_Y));
    SET_CONTROL_LABEL(9, g_infoManager.GetLabel(SYSTEM_FREE_SPACE_Z));
    SET_CONTROL_LABEL(10,g_infoManager.GetLabel(SYSTEM_TOTAL_SPACE));
    SET_CONTROL_LABEL(11,g_infoManager.GetLabel(SYSTEM_USED_SPACE_PERCENT));
    SET_CONTROL_LABEL(12,g_infoManager.GetLabel(SYSTEM_FREE_SPACE_PERCENT));
  }
  else if(iControl == CONTROL_BT_NETWORK)
  {
    SetLabelDummy();
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20158));
    SET_CONTROL_LABEL(2, g_infoManager.GetLabel(NETWORK_IS_DHCP));
#ifdef HAS_SYSINFO
    SET_CONTROL_LABEL(3, g_infoManager.GetLabel(NETWORK_LINK_STATE));
    SET_CONTROL_LABEL(4, g_infoManager.GetLabel(NETWORK_MAC_ADDRESS));
#endif
    SET_CONTROL_LABEL(5, g_infoManager.GetLabel(NETWORK_IP_ADDRESS));
    SET_CONTROL_LABEL(6, g_infoManager.GetLabel(NETWORK_SUBNET_ADDRESS));
    SET_CONTROL_LABEL(7, g_infoManager.GetLabel(NETWORK_GATEWAY_ADDRESS));
    SET_CONTROL_LABEL(8, g_infoManager.GetLabel(NETWORK_DNS1_ADDRESS));
    SET_CONTROL_LABEL(9, g_infoManager.GetLabel(NETWORK_DNS2_ADDRESS));
    SET_CONTROL_LABEL(10, g_infoManager.GetLabel(SYSTEM_INTERNET_STATE));
  }
  else if(iControl == CONTROL_BT_VIDEO)
  {
    SetLabelDummy();
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20159));
#ifdef HAS_SYSINFO
    SET_CONTROL_LABEL(2,g_infoManager.GetLabel(SYSTEM_VIDEO_ENCODER_INFO));
    SET_CONTROL_LABEL(3,g_infoManager.GetLabel(SYSTEM_SCREEN_RESOLUTION));
    SET_CONTROL_LABEL(4,g_infoManager.GetLabel(SYSTEM_AV_PACK_INFO));
    SET_CONTROL_LABEL(5,g_infoManager.GetLabel(SYSTEM_XBE_REGION));
#endif
  }
  else if(iControl == CONTROL_BT_HARDWARE)
  {
    SetLabelDummy();
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
#endif
  }
  SET_CONTROL_LABEL(50, g_infoManager.GetTime(TIME_FORMAT_HH_MM_SS) + " | " + g_infoManager.GetDate());
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