/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowPVRTimerRules.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "pvr/timers/PVRTimersPath.h"
#include "utils/URIUtils.h"

using namespace PVR;

CGUIWindowPVRTVTimerRules::CGUIWindowPVRTVTimerRules()
: CGUIWindowPVRTimersBase(false, WINDOW_TV_TIMER_RULES, "MyPVRTimers.xml")
{
}

std::string CGUIWindowPVRTVTimerRules::GetRootPath() const
{
  return CPVRTimersPath::PATH_TV_TIMER_RULES;
}

std::string CGUIWindowPVRTVTimerRules::GetDirectoryPath()
{
  const std::string basePath(CPVRTimersPath(false, true).GetPath());
  return URIUtils::PathHasParent(m_vecItems->GetPath(), basePath) ? m_vecItems->GetPath() : basePath;
}

CGUIWindowPVRRadioTimerRules::CGUIWindowPVRRadioTimerRules()
: CGUIWindowPVRTimersBase(true, WINDOW_RADIO_TIMER_RULES, "MyPVRTimers.xml")
{
}

std::string CGUIWindowPVRRadioTimerRules::GetRootPath() const
{
  return CPVRTimersPath::PATH_RADIO_TIMER_RULES;
}

std::string CGUIWindowPVRRadioTimerRules::GetDirectoryPath()
{
  const std::string basePath(CPVRTimersPath(true, true).GetPath());
  return URIUtils::PathHasParent(m_vecItems->GetPath(), basePath) ? m_vecItems->GetPath() : basePath;
}
