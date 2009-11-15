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

#include "system.h"
#include "GUIWindowSystemInfo.h"
#include "utils/GUIInfoManager.h"
#include "GUIWindowManager.h"
#include "Key.h"
#include "LocalizeStrings.h"
#ifdef HAS_SYSINFO
#include "SystemInfo.h"
#endif
#include "MediaManager.h"

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
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    g_windowManager.PreviousWindow();
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
    SetControlLabel(i++, "%s: %s", 158, SYSTEM_FREE_MEMORY);
    SetControlLabel(i++, "%s: %s", 150, NETWORK_IP_ADDRESS);
    SetControlLabel(i++, "%s %s", 13287, SYSTEM_SCREEN_RESOLUTION);
#ifdef HAS_SYSINFO
    SetControlLabel(i++, "%s %s", 13283, SYSTEM_KERNEL_VERSION);
#endif
    SetControlLabel(i++, "%s: %s", 12390, SYSTEM_UPTIME);
    SetControlLabel(i++, "%s: %s", 12394, SYSTEM_TOTALUPTIME);
  }
  else if(iControl == CONTROL_BT_STORAGE)
  {
    SetLabelDummy();
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20155));
    int i = 2;
    if (m_diskUsage.size() == 0)
      m_diskUsage = g_mediaManager.GetDiskUsage();

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
    SET_CONTROL_LABEL(i++,g_infoManager.GetLabel(SYSTEM_VIDEO_ENCODER_INFO));
    SetControlLabel(i++, "%s %s", 13287, SYSTEM_SCREEN_RESOLUTION);
#endif
    SetControlLabel(i++, "%s %s", 22007, SYSTEM_OPENGL_VENDOR);
    SetControlLabel(i++, "%s %s", 22009, SYSTEM_OPENGL_VERSION);
    SetControlLabel(i++, "%s %s", 22010, SYSTEM_GPU_TEMPERATURE);
  }
  else if(iControl == CONTROL_BT_HARDWARE)
  {
    SetLabelDummy();
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20160));
    int i = 2;
#ifdef HAS_SYSINFO
    SET_CONTROL_LABEL(i++, g_sysinfo.GetXBVerInfo());
    SetControlLabel(i++, "%s %s", 22011, SYSTEM_CPU_TEMPERATURE);
    SetControlLabel(i++, "%s %s", 13284, SYSTEM_CPUFREQUENCY);
#endif
    SetControlLabel(i++, "%s %s", 13271, SYSTEM_CPU_USAGE);
    i++; // empty line
    SetControlLabel(i++, "%s: %s", 22012, SYSTEM_TOTAL_MEMORY);
    SetControlLabel(i++, "%s: %s", 158, SYSTEM_FREE_MEMORY);
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

void CGUIWindowSystemInfo::SetControlLabel(int id, const char *format, int label, int info)
{
  CStdString tmpStr;
  tmpStr.Format(format, g_localizeStrings.Get(label).c_str(), g_infoManager.GetLabel(info).c_str());
  SET_CONTROL_LABEL(id, tmpStr);
}
