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

#include "Application.h"
#include "ApplicationMessenger.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogSelect.h"
#include "FileItem.h"
#include "GUIInfoManager.h"
#include "GUIUserMessages.h"
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

RENDER_STEREO_MODE CStereoscopicsManager::ConvertVideoToGuiStereoMode(const std::string &mode)
{
  static std::map<std::string, RENDER_STEREO_MODE> convert;
  if(convert.empty())
  {
    convert["mono"]                   = RENDER_STEREO_MODE_OFF;
    convert["left_right"]             = RENDER_STEREO_MODE_SPLIT_VERTICAL;
    convert["bottom_top"]             = RENDER_STEREO_MODE_SPLIT_HORIZONTAL;
    convert["top_bottom"]             = RENDER_STEREO_MODE_SPLIT_HORIZONTAL;
    convert["checkerboard_rl"]        = RENDER_STEREO_MODE_OFF;
    convert["checkerboard_lr"]        = RENDER_STEREO_MODE_OFF;
    convert["row_interleaved_rl"]     = RENDER_STEREO_MODE_INTERLACED;
    convert["row_interleaved_lr"]     = RENDER_STEREO_MODE_INTERLACED;
    convert["col_interleaved_rl"]     = RENDER_STEREO_MODE_OFF;
    convert["col_interleaved_lr"]     = RENDER_STEREO_MODE_OFF;
    convert["anaglyph_cyan_red"]      = RENDER_STEREO_MODE_ANAGLYPH_RED_CYAN;
    convert["right_left"]             = RENDER_STEREO_MODE_SPLIT_VERTICAL;
    convert["anaglyph_green_magenta"] = RENDER_STEREO_MODE_ANAGLYPH_GREEN_MAGENTA;
    convert["block_lr"]               = RENDER_STEREO_MODE_OFF;
    convert["block_rl"]               = RENDER_STEREO_MODE_OFF;
  }

  return (RENDER_STEREO_MODE) convert[mode];
}

RENDER_STEREO_MODE CStereoscopicsManager::GetStereoModeByUserChoice(const CStdString &heading)
{
  RENDER_STEREO_MODE mode = GetStereoMode();
  // if no stereo mode is set already, suggest mode of current video by preselecting it
  if (mode == RENDER_STEREO_MODE_OFF && g_application.IsPlayingVideo() && g_infoManager.EvaluateBool("videoplayer.isstereoscopic"))
    mode = GetStereoModeOfPlayingVideo();

  CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  pDlgSelect->Reset();
  if (heading.IsEmpty())
    pDlgSelect->SetHeading(g_localizeStrings.Get(36528).c_str());
  else
    pDlgSelect->SetHeading(heading.c_str());

  // prepare selectable stereo modes
  std::vector<RENDER_STEREO_MODE> selectableModes;
  for (int i = RENDER_STEREO_MODE_OFF; i < RENDER_STEREO_MODE_COUNT; i++)
  {
    RENDER_STEREO_MODE selectableMode = (RENDER_STEREO_MODE) i;
    if (g_Windowing.SupportsStereo(selectableMode))
    {
      selectableModes.push_back(selectableMode);
      CStdString label = g_localizeStrings.Get(36502+i);
      pDlgSelect->Add( label );
      if (mode == selectableMode)
        pDlgSelect->SetSelected( label );
    }
  }

  pDlgSelect->DoModal();

  int iItem = pDlgSelect->GetSelectedLabel();
  if (iItem > -1 && pDlgSelect->IsConfirmed())
    mode = (RENDER_STEREO_MODE) selectableModes[iItem];
  else
    mode = GetStereoMode();

  return mode;
}

RENDER_STEREO_MODE CStereoscopicsManager::GetStereoModeOfPlayingVideo(void)
{
  RENDER_STEREO_MODE mode = RENDER_STEREO_MODE_OFF;

  CStdString playerMode = g_infoManager.GetLabel(VIDEOPLAYER_STEREOSCOPIC_MODE);
  if (!playerMode.IsEmpty())
    mode = (RENDER_STEREO_MODE) ConvertVideoToGuiStereoMode(playerMode);

  CLog::Log(LOGDEBUG, "StereoscopicsManager: autodetected GUI stereo mode for movie mode %s is: %s", playerMode.c_str(), GetLabelForStereoMode(mode).c_str());
  return mode;
}

CStdString CStereoscopicsManager::GetLabelForStereoMode(const RENDER_STEREO_MODE &mode)
{
  return g_localizeStrings.Get(36502 + mode);
}

RENDER_STEREO_MODE CStereoscopicsManager::GetPreferredPlaybackMode(void)
{
  RENDER_STEREO_MODE playbackMode = m_lastStereoMode;
  int preferredMode = CSettings::Get().GetInt("videoplayer.stereoscopicviewmode");
  if (preferredMode == 0) // automatic mode, detect by movie
  {
    if (g_application.IsPlayingVideo() && g_infoManager.EvaluateBool("videoplayer.isstereoscopic"))
      playbackMode = GetStereoModeOfPlayingVideo();
    else if (playbackMode == RENDER_STEREO_MODE_OFF)
      playbackMode = GetNextSupportedStereoMode(RENDER_STEREO_MODE_OFF);
  }
  else // predefined mode
  {
    playbackMode = (RENDER_STEREO_MODE) preferredMode;
  }
  return playbackMode;
}

bool CStereoscopicsManager::OnMessage(CGUIMessage &message)
{
  if (!HasStereoscopicSupport())
    return false;

  switch (message.GetMessage())
  {
  case GUI_MSG_PLAYBACK_STARTED:
    OnPlaybackStarted();
    break;
  case GUI_MSG_PLAYBACK_STOPPED:
  case GUI_MSG_PLAYBACK_ENDED:
  case GUI_MSG_PLAYLISTPLAYER_STOPPED:
    OnPlaybackStopped();
    break;
  }

  return false;
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
        targetMode = GetPreferredPlaybackMode();
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

void CStereoscopicsManager::OnPlaybackStarted(void)
{
  if (!HasStereoscopicSupport() || !g_application.IsPlayingVideo() || !g_infoManager.EvaluateBool("videoplayer.isstereoscopic"))
    return;

  int playbackMode = CSettings::Get().GetInt("videoplayer.stereoscopicplaybackmode");

  switch (playbackMode)
  {
  case 0: // Ask
    {
      CApplicationMessenger::Get().MediaPause();

      CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
      pDlgSelect->Reset();
      pDlgSelect->SetHeading(g_localizeStrings.Get(36527).c_str());

      RENDER_STEREO_MODE preferred = GetPreferredPlaybackMode();

      // add choices
      pDlgSelect->Add( g_localizeStrings.Get(36530) + " - " + CStereoscopicsManager::Get().GetLabelForStereoMode(preferred) );
      pDlgSelect->Add( g_localizeStrings.Get(36529) ); // mono / 2d
      pDlgSelect->Add( g_localizeStrings.Get(36531) ); // other / select

      pDlgSelect->DoModal();

      if(pDlgSelect->IsConfirmed())
      {
        RENDER_STEREO_MODE mode = RENDER_STEREO_MODE_OFF;
        int iItem = pDlgSelect->GetSelectedLabel();
        if (iItem == 0)
          mode = GetPreferredPlaybackMode();
        else if (iItem == 1)
          mode = RENDER_STEREO_MODE_MONO;
        else if (iItem == 2)
          mode = GetStereoModeByUserChoice();

        SetStereoMode(mode);
      }

      CApplicationMessenger::Get().MediaUnPause();
    }
    break;
  case 1: // Stereoscopic
    {
      RENDER_STEREO_MODE mode = GetStereoMode();
      if (mode == RENDER_STEREO_MODE_OFF)
        mode = GetPreferredPlaybackMode();
      SetStereoMode(mode);
    }
    break;
  case 2: // do nothing; play as is
  default:
    break;
  }
}

void CStereoscopicsManager::OnPlaybackStopped(void)
{
  if (!HasStereoscopicSupport())
    return;

  RENDER_STEREO_MODE mode = GetStereoMode();
  if (CSettings::Get().GetBool("videoplayer.quitstereomodeonstop") == true && mode != RENDER_STEREO_MODE_OFF)
  {
    SetStereoMode(RENDER_STEREO_MODE_OFF);
  }
}
