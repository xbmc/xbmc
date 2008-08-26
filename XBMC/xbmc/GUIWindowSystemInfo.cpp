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
#ifdef HAS_SYSINFO
#include "SystemInfo.h"
#endif
#ifdef _LINUX
#include "LinuxFileSystem.h"
#endif
#ifdef _WIN32PC
#include "WIN32Util.h"
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
      return true;
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
    {
      // call base class
      CGUIWindow::OnMessage(message);
      // clean up
      m_diskUsage.clear();
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
    CStdString tmpStr = g_localizeStrings.Get(158) +": ";
    tmpStr += g_infoManager.GetLabel(SYSTEM_FREE_MEMORY);
    SET_CONTROL_LABEL(i++, tmpStr);
    SET_CONTROL_LABEL(i++, g_infoManager.GetLabel(NETWORK_IP_ADDRESS));
    SET_CONTROL_LABEL(i++,g_infoManager.GetLabel(SYSTEM_SCREEN_RESOLUTION));
#ifdef HAS_SYSINFO
    SET_CONTROL_LABEL(i++,g_infoManager.GetLabel(SYSTEM_KERNEL_VERSION));
#endif
    SET_CONTROL_LABEL(i++,g_infoManager.GetLabel(SYSTEM_UPTIME));
    SET_CONTROL_LABEL(i++,g_infoManager.GetLabel(SYSTEM_TOTALUPTIME));
  }
  else if(iControl == CONTROL_BT_STORAGE)
  {
    SetLabelDummy();
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20155));
    int i = 2;
    if (m_diskUsage.size() == 0)
    {
#ifdef _WIN32PC
      m_diskUsage = CWIN32Util::GetDiskUsage();
#else
      m_diskUsage = CLinuxFileSystem::GetDiskUsage();
#endif
    }

    for (size_t d = 0; d < m_diskUsage.size(); d++)
    {
      SET_CONTROL_LABEL(i++, m_diskUsage[d]);
    }
  }
  else if(iControl == CONTROL_BT_NETWORK)
  {
    SetLabelDummy();
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20158));
    int i = 2;
#ifdef HAS_SYSINFO
    SET_CONTROL_LABEL(i++, g_infoManager.GetLabel(NETWORK_LINK_STATE));
    SET_CONTROL_LABEL(i++, g_infoManager.GetLabel(NETWORK_MAC_ADDRESS));
#endif
    SET_CONTROL_LABEL(i++, g_infoManager.GetLabel(NETWORK_IP_ADDRESS));
    SET_CONTROL_LABEL(i++, g_infoManager.GetLabel(NETWORK_SUBNET_ADDRESS));
    SET_CONTROL_LABEL(i++, g_infoManager.GetLabel(NETWORK_GATEWAY_ADDRESS));
    SET_CONTROL_LABEL(i++, g_infoManager.GetLabel(NETWORK_DNS1_ADDRESS));
    SET_CONTROL_LABEL(i++, g_infoManager.GetLabel(NETWORK_DNS2_ADDRESS));
    SET_CONTROL_LABEL(i++, g_infoManager.GetLabel(SYSTEM_INTERNET_STATE));
  }
  else if(iControl == CONTROL_BT_VIDEO)
  {
    SetLabelDummy();
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20159));
    int i = 2;
#ifdef HAS_SYSINFO
    SET_CONTROL_LABEL(i++,g_infoManager.GetLabel(SYSTEM_VIDEO_ENCODER_INFO));
    SET_CONTROL_LABEL(i++,g_infoManager.GetLabel(SYSTEM_SCREEN_RESOLUTION));
#endif
    SET_CONTROL_LABEL(i++,g_infoManager.GetLabel(SYSTEM_OPENGL_VENDOR));
    SET_CONTROL_LABEL(i++,g_infoManager.GetLabel(SYSTEM_OPENGL_VERSION));
    SET_CONTROL_LABEL(i++, g_infoManager.GetSystemHeatInfo(SYSTEM_GPU_TEMPERATURE));
  }
  else if(iControl == CONTROL_BT_HARDWARE)
  {
    SetLabelDummy();
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20160));
    int i = 2;
#ifdef HAS_SYSINFO
    SET_CONTROL_LABEL(i++, g_sysinfo.GetXBVerInfo());
    SET_CONTROL_LABEL(i++, g_infoManager.GetSystemHeatInfo(SYSTEM_CPU_TEMPERATURE));
    SET_CONTROL_LABEL(i++, g_infoManager.GetLabel(SYSTEM_CPUFREQUENCY));
#endif
    SET_CONTROL_LABEL(i++, g_infoManager.GetLabel(SYSTEM_CPU_USAGE));
    i++; // empty line
    CStdString tmpStr = g_localizeStrings.Get(22012) +": ";
    tmpStr += g_infoManager.GetLabel(SYSTEM_TOTAL_MEMORY);
    SET_CONTROL_LABEL(i++, tmpStr);
    tmpStr = g_localizeStrings.Get(158) +": ";
    tmpStr += g_infoManager.GetLabel(SYSTEM_FREE_MEMORY);
    SET_CONTROL_LABEL(i++, tmpStr);
  }
  SET_CONTROL_LABEL(52, "XBMC "+g_infoManager.GetLabel(SYSTEM_BUILD_VERSION)+" (Compiled : "+g_infoManager.GetLabel(SYSTEM_BUILD_DATE)+")");
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
