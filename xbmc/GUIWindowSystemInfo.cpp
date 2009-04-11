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

#include "stdafx.h"
#include "GUIWindowSystemInfo.h"
#include "utils/GUIInfoManager.h"
#include "GUIWindowManager.h"

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
    int i = 2;
    SetControlLabel(i++, "%s %s", 22011, SYSTEM_CPU_TEMPERATURE);
    SetControlLabel(i++, "%s %s", 22010, SYSTEM_GPU_TEMPERATURE);
    SetControlLabel(i++, "%s: %s", 13300, SYSTEM_FAN_SPEED);
    SetControlLabel(i++, "%s: %s", 158, SYSTEM_FREE_MEMORY);
    SetControlLabel(i++, "%s: %s", 150, NETWORK_IP_ADDRESS);
    SetControlLabel(i++, "%s %s", 13287, SYSTEM_SCREEN_RESOLUTION);
#ifdef HAS_SYSINFO
    SetControlLabel(i++, "%s %s", 13283, SYSTEM_KERNEL_VERSION);
#endif
    SetControlLabel(i++, "%s: %s", 12390, SYSTEM_UPTIME);
    SetControlLabel(i++, "%s: %s", 12394, SYSTEM_TOTALUPTIME);
  }
  else if(iControl == CONTROL_BT_HDD)
  {
    SetLabelDummy();
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20156));
    int i = 2;
#ifdef HAS_SYSINFO
    SetControlLabel(i++, "%s %s", 13154, SYSTEM_HDD_MODEL);
    SetControlLabel(i++, "%s %s", 13155, SYSTEM_HDD_SERIAL);
    SetControlLabel(i++, "%s %s", 13156, SYSTEM_HDD_FIRMWARE);
    SetControlLabel(i++, "%s %s", 13157, SYSTEM_HDD_PASSWORD);
    SetControlLabel(i++, "%s %s", 13158, SYSTEM_HDD_LOCKSTATE);
    SetControlLabel(i++, "%s %s", 13150, SYSTEM_HDD_LOCKKEY);
    SetControlLabel(i++, "%s %s", 13173, SYSTEM_HDD_BOOTDATE);
    SetControlLabel(i++, "%s %s", 13174, SYSTEM_HDD_CYCLECOUNT);
    SetControlLabel(i++, "%s %s", 13151, SYSTEM_HDD_TEMPERATURE);
#endif
  }
  else if(iControl == CONTROL_BT_DVD)
  {
    SetLabelDummy();
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20157));
    int i = 2;
#ifdef HAS_SYSINFO
    SetControlLabel(i++, "%s %s", 13152, SYSTEM_DVD_MODEL);
    SetControlLabel(i++, "%s %s", 13153, SYSTEM_DVD_FIRMWARE);
    SetControlLabel(i++, "%s %s", 13294, SYSTEM_DVD_ZONE);
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
    SetControlLabel(10, "%s: %s", 20161, SYSTEM_TOTAL_SPACE);
    SetControlLabel(11, "%s: %s", 20161, SYSTEM_USED_SPACE_PERCENT);
    SET_CONTROL_LABEL(12,g_infoManager.GetLabel(SYSTEM_FREE_SPACE_PERCENT));
  }
  else if(iControl == CONTROL_BT_NETWORK)
  {
    SetLabelDummy();
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20158));
    int i = 2;
    SetControlLabel(i++, "%s %s", 146, NETWORK_IS_DHCP);
#ifdef HAS_SYSINFO
    SetControlLabel(i++, "%s %s", 151, NETWORK_LINK_STATE);
    SetControlLabel(i++, "%s: %s", 149, NETWORK_MAC_ADDRESS);
#endif
    SetControlLabel(i++, "%s: %s", 150, NETWORK_IP_ADDRESS);
    SetControlLabel(i++, "%s: %s", 13159, NETWORK_SUBNET_ADDRESS);
    SetControlLabel(i++, "%s: %s", 13160, NETWORK_GATEWAY_ADDRESS);
    SetControlLabel(i++, "%s: %s", 13161, NETWORK_DNS1_ADDRESS);
    SetControlLabel(i++, "%s: %s", 20307, NETWORK_DNS2_ADDRESS);
    SetControlLabel(i++, "%s %s", 13295, SYSTEM_INTERNET_STATE);
  }
  else if(iControl == CONTROL_BT_VIDEO)
  {
    SetLabelDummy();
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20159));
    int i = 2;
#ifdef HAS_SYSINFO
    SetControlLabel(i++, "%s %s", 13286, SYSTEM_VIDEO_ENCODER_INFO);
    SetControlLabel(i++, "%s %s", 13287, SYSTEM_SCREEN_RESOLUTION);
    SetControlLabel(i++, "%s %s", 13292, SYSTEM_AV_PACK_INFO);
    SetControlLabel(i++, "%s %s", 13293, SYSTEM_XBE_REGION);
#endif
  }
  else if(iControl == CONTROL_BT_HARDWARE)
  {
    SetLabelDummy();
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20160));
    int i = 2;
#ifdef HAS_SYSINFO
    SetControlLabel(i++, "%s %s", 13288, SYSTEM_XBOX_VERSION);
    SetControlLabel(i++, "%s %s", 13289, SYSTEM_XBOX_SERIAL);
    SetControlLabel(i++, "%s %s", 13284, SYSTEM_CPUFREQUENCY);
    SetControlLabel(i++, "%s %s", 13285, SYSTEM_XBOX_BIOS);
    SET_CONTROL_LABEL(i++, g_infoManager.GetLabel(SYSTEM_XBOX_MODCHIP));
    SetControlLabel(i++, "%s %s", 13290, SYSTEM_XBOX_PRODUCE_INFO);
    SetControlLabel(i++, "%s 1: %s", 13169, SYSTEM_CONTROLLER_PORT_1);
    SetControlLabel(i++, "%s 2: %s", 13169, SYSTEM_CONTROLLER_PORT_2);
    SetControlLabel(i++, "%s 3: %s", 13169, SYSTEM_CONTROLLER_PORT_3);
    SetControlLabel(i++, "%s 4: %s", 13169, SYSTEM_CONTROLLER_PORT_4);
#endif
  }
  SET_CONTROL_LABEL(50, g_infoManager.GetTime(TIME_FORMAT_HH_MM_SS) + " | " + g_infoManager.GetDate());
  SET_CONTROL_LABEL(51, g_localizeStrings.Get(144)+" "+g_infoManager.GetVersion());
  SET_CONTROL_LABEL(52, "XBMC "+g_infoManager.GetLabel(SYSTEM_BUILD_VERSION)+" (Compiled: "+g_infoManager.GetLabel(SYSTEM_BUILD_DATE)+")");
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

void CGUIWindowSystemInfo::SetControlLabel(int id, const char *format, int label, int info)
{
  CStdString tmpStr;
  tmpStr.Format(format, g_localizeStrings.Get(label).c_str(), g_infoManager.GetLabel(info).c_str());
  SET_CONTROL_LABEL(id, tmpStr);
}