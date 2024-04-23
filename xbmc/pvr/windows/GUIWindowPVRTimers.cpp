/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowPVRTimers.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "pvr/timers/PVRTimersPath.h"
#include "utils/URIUtils.h"

using namespace PVR;

CGUIWindowPVRTVTimers::CGUIWindowPVRTVTimers()
: CGUIWindowPVRTimersBase(false, WINDOW_TV_TIMERS, "MyPVRTimers.xml")
{
}

std::string CGUIWindowPVRTVTimers::GetRootPath() const
{
  return CPVRTimersPath::PATH_TV_TIMERS;
}

std::string CGUIWindowPVRTVTimers::GetDirectoryPath()
{
  const std::string basePath(CPVRTimersPath(false, false).GetPath());
  return URIUtils::PathHasParent(m_vecItems->GetPath(), basePath) ? m_vecItems->GetPath() : basePath;
}

CGUIWindowPVRRadioTimers::CGUIWindowPVRRadioTimers()
: CGUIWindowPVRTimersBase(true, WINDOW_RADIO_TIMERS, "MyPVRTimers.xml")
{
}

std::string CGUIWindowPVRRadioTimers::GetRootPath() const
{
  return CPVRTimersPath::PATH_RADIO_TIMERS;
}

std::string CGUIWindowPVRRadioTimers::GetDirectoryPath()
{
  const std::string basePath(CPVRTimersPath(true, false).GetPath());
  return URIUtils::PathHasParent(m_vecItems->GetPath(), basePath) ? m_vecItems->GetPath() : basePath;
}
