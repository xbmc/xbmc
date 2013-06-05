/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

/*!
 * @file StereoscopicsManager.cpp
 * @brief This class acts as container for stereoscopic related functions
 */

#include <stdlib.h>
#include "StereoscopicsManager.h"

#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/Key.h"
#include "guilib/GUIWindowManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/ISettingCallback.h"
#include "settings/Setting.h"
#include "settings/Settings.h"
#include "rendering/RenderSystem.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "windowing/WindowingFactory.h"


CStereoscopicsManager::CStereoscopicsManager(void)
{
  m_lastStereoMode = RENDER_STEREO_MODE_OFF;
}

CStereoscopicsManager::~CStereoscopicsManager(void)
{
}

CStereoscopicsManager& CStereoscopicsManager::Get(void)
{
  static CStereoscopicsManager sStereoscopicsManager;
  return sStereoscopicsManager;
}

bool CStereoscopicsManager::HasStereoscopicSupport(void)
{
  return (bool) CSettings::Get().GetBool("videoscreen.hasstereoscopicsupport");
}

RENDER_STEREO_MODE CStereoscopicsManager::GetStereoMode(void)
{
  return (RENDER_STEREO_MODE) CSettings::Get().GetInt("videoscreen.stereoscopicmode");
}

void CStereoscopicsManager::SetStereoMode(const RENDER_STEREO_MODE &mode)
{
  RENDER_STEREO_MODE currentMode = GetStereoMode();
  if (mode != currentMode)
  {
    m_lastStereoMode = currentMode;
    CSettings::Get().SetInt("videoscreen.stereoscopicmode", mode);
  }
}

RENDER_STEREO_MODE CStereoscopicsManager::GetNextSupportedStereoMode(const RENDER_STEREO_MODE &currentMode, int step)
{
  RENDER_STEREO_MODE mode = currentMode;
  do {
    mode = (RENDER_STEREO_MODE) ((mode + step) % RENDER_STEREO_MODE_COUNT);
    if(g_Windowing.SupportsStereo(mode))
      break;
   } while (mode != currentMode);
  return mode;
}

std::string CStereoscopicsManager::DetectStereoModeByString(const std::string &needle)
{
  std::string stereoMode;
  CStdString searchString(needle);
  CStdStringArray tags;
  StringUtils::ToUpper(searchString);

  CStdString tag( g_advancedSettings.m_stereoscopicflags_sbs );
  if (stereoMode.empty() && !tag.IsEmpty())
  {
    StringUtils::ToUpper(tag);
    StringUtils::SplitString(tag, "|", tags);
    if (StringUtils::ContainsKeyword(searchString, tags))
      stereoMode = "left_right";
  }

  tag = g_advancedSettings.m_stereoscopicflags_tab;
  if (stereoMode.empty() && !tag.IsEmpty())
  {
    StringUtils::ToUpper(tag);
    StringUtils::SplitString(tag, "|", tags);
    if (StringUtils::ContainsKeyword(searchString, tags))
      stereoMode = "top_bottom";
  }

  if (stereoMode.empty())
    stereoMode = "mono";

  CLog::Log(LOGDEBUG, "StereoscopicsManager: Detected stereo mode in string '%s' is '%s'", needle.c_str(), stereoMode.c_str());
  return stereoMode;
}

CStdString CStereoscopicsManager::GetLabelForStereoMode(const RENDER_STEREO_MODE &mode)
{
  return g_localizeStrings.Get(36502 + mode);
}
void CStereoscopicsManager::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();

  if (settingId == "videoscreen.hasstereoscopicsupport")
  {
    // turn off 3D mode if global toggle has been disabled
    if (((CSettingBool*)setting)->GetValue() == false)
    {
      SetStereoMode(RENDER_STEREO_MODE_OFF);
      ApplyStereoMode(RENDER_STEREO_MODE_OFF);
    }
    else
    {
      ApplyStereoMode(GetStereoMode());
    }
  }
  else if (settingId == "videoscreen.stereoscopicmode")
  {
    RENDER_STEREO_MODE mode = GetStereoMode();
    CLog::Log(LOGDEBUG, "StereoscopicsManager: stereo mode setting changed to %s", GetLabelForStereoMode(mode).c_str());
    if(HasStereoscopicSupport())
      ApplyStereoMode(mode);
  }
}

bool CStereoscopicsManager::OnAction(const CAction &action)
{
  if (!HasStereoscopicSupport())
    return false;

  RENDER_STEREO_MODE mode = GetStereoMode();

  if (action.GetID() == ACTION_STEREOMODE_NEXT)
  {
    mode = GetNextSupportedStereoMode(mode);
    // when cycling mode, skip "OFF"
    if (mode == RENDER_STEREO_MODE_OFF)
      mode = GetNextSupportedStereoMode(mode);

    SetStereoMode(mode);
    return true;
  }
  else if (action.GetID() == ACTION_STEREOMODE_PREVIOUS)
  {
    mode = GetNextSupportedStereoMode(mode, RENDER_STEREO_MODE_COUNT - 1);
    // when cycling mode, skip "OFF"
    if (mode == RENDER_STEREO_MODE_OFF)
      mode = GetNextSupportedStereoMode(mode, RENDER_STEREO_MODE_COUNT - 1);

    SetStereoMode(mode);
    return true;
  }
  else if (action.GetID() == ACTION_STEREOMODE_TOGGLE)
  {
    if (mode == RENDER_STEREO_MODE_OFF)
    {
      RENDER_STEREO_MODE targetMode = m_lastStereoMode;
      if (targetMode == RENDER_STEREO_MODE_OFF)
        targetMode = GetNextSupportedStereoMode(RENDER_STEREO_MODE_OFF);
      SetStereoMode(targetMode);
    }
    else
    {
      SetStereoMode(RENDER_STEREO_MODE_OFF);
    }
    return true;
  }

  return false;
}

void CStereoscopicsManager::ApplyStereoMode(const RENDER_STEREO_MODE &mode, bool notify)
{
  RENDER_STEREO_MODE currentMode = g_graphicsContext.GetStereoMode();
  CLog::Log(LOGDEBUG, "StereoscopicsManager::ApplyStereoMode: trying to apply stereo mode. Current: %s | Target: %s", GetLabelForStereoMode(currentMode).c_str(), GetLabelForStereoMode(mode).c_str());
  if (currentMode != mode)
  {
    g_graphicsContext.SetStereoMode(mode);
    CLog::Log(LOGDEBUG, "StereoscopicsManager: stereo mode changed to %s", GetLabelForStereoMode(mode).c_str());
    if (notify)
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(36501), GetLabelForStereoMode(mode));
  }
}
