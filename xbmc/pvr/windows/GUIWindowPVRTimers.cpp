/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
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

#include "GUIWindowPVRTimers.h"

#include "FileItem.h"
#include "utils/URIUtils.h"

#include "pvr/timers/PVRTimers.h"

using namespace PVR;

CGUIWindowPVRTVTimers::CGUIWindowPVRTVTimers() :
  CGUIWindowPVRTimersBase(false, WINDOW_TV_TIMERS, "MyPVRTimers.xml")
{
}

std::string CGUIWindowPVRTVTimers::GetDirectoryPath()
{
  const std::string basePath(CPVRTimersPath(false, false).GetPath());
  return URIUtils::PathHasParent(m_vecItems->GetPath(), basePath) ? m_vecItems->GetPath() : basePath;
}

CGUIWindowPVRRadioTimers::CGUIWindowPVRRadioTimers() :
CGUIWindowPVRTimersBase(true, WINDOW_RADIO_TIMERS, "MyPVRTimers.xml")
{
}

std::string CGUIWindowPVRRadioTimers::GetDirectoryPath()
{
  const std::string basePath(CPVRTimersPath(true, false).GetPath());
  return URIUtils::PathHasParent(m_vecItems->GetPath(), basePath) ? m_vecItems->GetPath() : basePath;
}
