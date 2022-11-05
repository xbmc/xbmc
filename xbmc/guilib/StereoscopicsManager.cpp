/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 */

/*!
 * @file StereoscopicsManager.cpp
 * @brief This class acts as container for stereoscopic related functions
 */

#include "StereoscopicsManager.h"

#include "GUIComponent.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "cores/DataCacheCore.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "rendering/RenderSystem.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <stdlib.h>

struct StereoModeMap
{
  const char*          name;
  RENDER_STEREO_MODE   mode;
};

static const struct StereoModeMap VideoModeToGuiModeMap[] =
{
  { "mono",                     RENDER_STEREO_MODE_OFF },
  { "left_right",               RENDER_STEREO_MODE_SPLIT_VERTICAL },
  { "right_left",               RENDER_STEREO_MODE_SPLIT_VERTICAL },
  { "top_bottom",               RENDER_STEREO_MODE_SPLIT_HORIZONTAL },
  { "bottom_top",               RENDER_STEREO_MODE_SPLIT_HORIZONTAL },
  { "checkerboard_rl",          RENDER_STEREO_MODE_CHECKERBOARD },
  { "checkerboard_lr",          RENDER_STEREO_MODE_CHECKERBOARD },
  { "row_interleaved_rl",       RENDER_STEREO_MODE_INTERLACED },
  { "row_interleaved_lr",       RENDER_STEREO_MODE_INTERLACED },
  { "col_interleaved_rl",       RENDER_STEREO_MODE_OFF }, // unsupported
  { "col_interleaved_lr",       RENDER_STEREO_MODE_OFF }, // unsupported
  { "anaglyph_cyan_red",        RENDER_STEREO_MODE_ANAGLYPH_RED_CYAN },
  { "anaglyph_green_magenta",   RENDER_STEREO_MODE_ANAGLYPH_GREEN_MAGENTA },
  { "anaglyph_yellow_blue",     RENDER_STEREO_MODE_ANAGLYPH_YELLOW_BLUE },
  { "block_lr",                 RENDER_STEREO_MODE_OFF }, // unsupported
  { "block_rl",                 RENDER_STEREO_MODE_OFF }, // unsupported
  {}
};

static const struct StereoModeMap StringToGuiModeMap[] =
{
  { "off",                      RENDER_STEREO_MODE_OFF },
  { "split_vertical",           RENDER_STEREO_MODE_SPLIT_VERTICAL },
  { "side_by_side",             RENDER_STEREO_MODE_SPLIT_VERTICAL }, // alias
  { "sbs",                      RENDER_STEREO_MODE_SPLIT_VERTICAL }, // alias
  { "split_horizontal",         RENDER_STEREO_MODE_SPLIT_HORIZONTAL },
  { "over_under",               RENDER_STEREO_MODE_SPLIT_HORIZONTAL }, // alias
  { "tab",                      RENDER_STEREO_MODE_SPLIT_HORIZONTAL }, // alias
  { "row_interleaved",          RENDER_STEREO_MODE_INTERLACED },
  { "interlaced",               RENDER_STEREO_MODE_INTERLACED }, // alias
  { "checkerboard",             RENDER_STEREO_MODE_CHECKERBOARD },
  { "anaglyph_cyan_red",        RENDER_STEREO_MODE_ANAGLYPH_RED_CYAN },
  { "anaglyph_green_magenta",   RENDER_STEREO_MODE_ANAGLYPH_GREEN_MAGENTA },
  { "anaglyph_yellow_blue",     RENDER_STEREO_MODE_ANAGLYPH_YELLOW_BLUE },
  { "hardware_based",           RENDER_STEREO_MODE_HARDWAREBASED },
  { "monoscopic",               RENDER_STEREO_MODE_MONO },
  {}
};

CStereoscopicsManager::CStereoscopicsManager()
  : m_settings(CServiceBroker::GetSettingsComponent()->GetSettings())
{
  m_stereoModeSetByUser = RENDER_STEREO_MODE_UNDEFINED;
  m_lastStereoModeSetByUser = RENDER_STEREO_MODE_UNDEFINED;

  //! @todo Move this to Initialize() to avoid potential problems in ctor
  std::set<std::string> settingSet{
    CSettings::SETTING_VIDEOSCREEN_STEREOSCOPICMODE
  };
  m_settings->GetSettingsManager()->RegisterCallback(this, settingSet);
}

CStereoscopicsManager::~CStereoscopicsManager(void)
{
  m_settings->GetSettingsManager()->UnregisterCallback(this);
}

void CStereoscopicsManager::Initialize()
{
  // turn off stereo mode on XBMC startup
  SetStereoMode(RENDER_STEREO_MODE_OFF);
}

RENDER_STEREO_MODE CStereoscopicsManager::GetStereoMode(void) const
{
  return static_cast<RENDER_STEREO_MODE>(m_settings->GetInt(CSettings::SETTING_VIDEOSCREEN_STEREOSCOPICMODE));
}

void CStereoscopicsManager::SetStereoModeByUser(const RENDER_STEREO_MODE &mode)
{
  // only update last user mode if desired mode is different from current
  if (mode != m_stereoModeSetByUser)
    m_lastStereoModeSetByUser = m_stereoModeSetByUser;

  m_stereoModeSetByUser = mode;
  SetStereoMode(mode);
}

void CStereoscopicsManager::SetStereoMode(const RENDER_STEREO_MODE &mode)
{
  RENDER_STEREO_MODE currentMode = GetStereoMode();
  RENDER_STEREO_MODE applyMode = mode;

  // resolve automatic mode before applying
  if (mode == RENDER_STEREO_MODE_AUTO)
    applyMode = GetStereoModeOfPlayingVideo();

  if (applyMode != currentMode && applyMode >= RENDER_STEREO_MODE_OFF)
  {
    if (CServiceBroker::GetRenderSystem()->SupportsStereo(applyMode))
      m_settings->SetInt(CSettings::SETTING_VIDEOSCREEN_STEREOSCOPICMODE, applyMode);
  }
}

RENDER_STEREO_MODE CStereoscopicsManager::GetNextSupportedStereoMode(const RENDER_STEREO_MODE &currentMode, int step) const
{
  RENDER_STEREO_MODE mode = currentMode;

  do
  {
    mode = static_cast<RENDER_STEREO_MODE>((mode + step) % RENDER_STEREO_MODE_COUNT);

    if (CServiceBroker::GetRenderSystem()->SupportsStereo(mode))
      break;
  } while (mode != currentMode);

  return mode;
}

std::string CStereoscopicsManager::DetectStereoModeByString(const std::string &needle) const
{
  std::string stereoMode;
  const std::string& searchString(needle);
  CRegExp re(true);

  if (!re.RegComp(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_stereoscopicregex_3d.c_str()))
  {
    CLog::Log(
        LOGERROR, "{}: Invalid RegExp for matching 3d content:'{}'", __FUNCTION__,
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_stereoscopicregex_3d);
    return stereoMode;
  }

  if (re.RegFind(searchString) == -1)
    return stereoMode;    // no match found for 3d content, assume mono mode

  if (!re.RegComp(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_stereoscopicregex_sbs.c_str()))
  {
    CLog::Log(
        LOGERROR, "{}: Invalid RegExp for matching 3d SBS content:'{}'", __FUNCTION__,
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_stereoscopicregex_sbs);
    return stereoMode;
  }

  if (re.RegFind(searchString) > -1)
  {
    stereoMode = "left_right";
    return stereoMode;
  }

  if (!re.RegComp(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_stereoscopicregex_tab.c_str()))
  {
    CLog::Log(
        LOGERROR, "{}: Invalid RegExp for matching 3d TAB content:'{}'", __FUNCTION__,
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_stereoscopicregex_tab);
    return stereoMode;
  }

  if (re.RegFind(searchString) > -1)
    stereoMode = "top_bottom";

  return stereoMode;
}

RENDER_STEREO_MODE CStereoscopicsManager::GetStereoModeByUserChoice() const
{
  RENDER_STEREO_MODE mode = GetStereoMode();

  // if no stereo mode is set already, suggest mode of current video by preselecting it
  if (mode == RENDER_STEREO_MODE_OFF)
    mode = GetStereoModeOfPlayingVideo();

  CGUIDialogSelect* pDlgSelect = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  pDlgSelect->Reset();

  // "Select stereoscopic 3D mode"
  pDlgSelect->SetHeading(CVariant{g_localizeStrings.Get(36528)});

  // prepare selectable stereo modes
  std::vector<RENDER_STEREO_MODE> selectableModes;
  for (int i = RENDER_STEREO_MODE_OFF; i < RENDER_STEREO_MODE_COUNT; i++)
  {
    RENDER_STEREO_MODE selectableMode = static_cast<RENDER_STEREO_MODE>(i);
    if (CServiceBroker::GetRenderSystem()->SupportsStereo(selectableMode))
    {
      selectableModes.push_back(selectableMode);
      std::string label = GetLabelForStereoMode((RENDER_STEREO_MODE) i);
      pDlgSelect->Add( label );
      if (mode == selectableMode)
        pDlgSelect->SetSelected( label );
    }

    // inject AUTO pseudo mode after OFF
    if (i == RENDER_STEREO_MODE_OFF)
    {
      selectableModes.push_back(RENDER_STEREO_MODE_AUTO);
      pDlgSelect->Add(GetLabelForStereoMode(RENDER_STEREO_MODE_AUTO));
    }
  }

  pDlgSelect->Open();

  int iItem = pDlgSelect->GetSelectedItem();
  if (iItem > -1 && pDlgSelect->IsConfirmed())
    mode = selectableModes[iItem];
  else
    mode = GetStereoMode();

  return mode;
}

RENDER_STEREO_MODE CStereoscopicsManager::GetStereoModeOfPlayingVideo(void) const
{
  RENDER_STEREO_MODE mode = RENDER_STEREO_MODE_OFF;
  std::string playerMode = GetVideoStereoMode();

  if (!playerMode.empty())
  {
    int convertedMode = ConvertVideoToGuiStereoMode(playerMode);
    if (convertedMode > -1)
      mode = static_cast<RENDER_STEREO_MODE>(convertedMode);
  }

  CLog::Log(LOGDEBUG, "StereoscopicsManager: autodetected stereo mode for movie mode {} is: {}",
            playerMode, ConvertGuiStereoModeToString(mode));
  return mode;
}

std::string CStereoscopicsManager::GetLabelForStereoMode(const RENDER_STEREO_MODE &mode) const
{
  int msgId;
  switch(mode) {
    case RENDER_STEREO_MODE_AUTO:
	  msgId = 36532;
	  break;
    case RENDER_STEREO_MODE_ANAGLYPH_YELLOW_BLUE:
	  msgId = 36510;
	  break;
    case RENDER_STEREO_MODE_INTERLACED:
	  msgId = 36507;
	  break;
    case RENDER_STEREO_MODE_CHECKERBOARD:
    msgId = 36511;
    break;
    case RENDER_STEREO_MODE_HARDWAREBASED:
	  msgId = 36508;
	  break;
    case RENDER_STEREO_MODE_MONO:
	  msgId = 36509;
	  break;
    default:
	  msgId = 36502 + mode;
  }

  return g_localizeStrings.Get(msgId);
}

RENDER_STEREO_MODE CStereoscopicsManager::GetPreferredPlaybackMode(void) const
{
  return static_cast<RENDER_STEREO_MODE>(m_settings->GetInt(CSettings::SETTING_VIDEOSCREEN_PREFEREDSTEREOSCOPICMODE));
}

int CStereoscopicsManager::ConvertVideoToGuiStereoMode(const std::string &mode)
{
  size_t i = 0;
  while (VideoModeToGuiModeMap[i].name)
  {
    if (mode == VideoModeToGuiModeMap[i].name)
      return VideoModeToGuiModeMap[i].mode;
    i++;
  }
  return -1;
}

int CStereoscopicsManager::ConvertStringToGuiStereoMode(const std::string &mode)
{
  size_t i = 0;
  while (StringToGuiModeMap[i].name)
  {
    if (mode == StringToGuiModeMap[i].name)
      return StringToGuiModeMap[i].mode;
    i++;
  }
  return ConvertVideoToGuiStereoMode(mode);
}

const char* CStereoscopicsManager::ConvertGuiStereoModeToString(const RENDER_STEREO_MODE &mode)
{
  size_t i = 0;
  while (StringToGuiModeMap[i].name)
  {
    if (StringToGuiModeMap[i].mode == mode)
      return StringToGuiModeMap[i].name;
    i++;
  }
  return "";
}

std::string CStereoscopicsManager::NormalizeStereoMode(const std::string &mode)
{
  if (!mode.empty() && mode != "mono")
  {
    int guiMode = ConvertStringToGuiStereoMode(mode);

    if (guiMode > -1)
      return ConvertGuiStereoModeToString((RENDER_STEREO_MODE) guiMode);
    else
      return mode;
  }

  return "mono";
}

CAction CStereoscopicsManager::ConvertActionCommandToAction(const std::string &command, const std::string &parameter)
{
  std::string cmd = command;
  std::string para = parameter;
  StringUtils::ToLower(cmd);
  StringUtils::ToLower(para);
  if (cmd == "setstereomode")
  {
    int actionId = -1;
    if (para == "next")
      actionId = ACTION_STEREOMODE_NEXT;
    else if (para == "previous")
      actionId = ACTION_STEREOMODE_PREVIOUS;
    else if (para == "toggle")
      actionId = ACTION_STEREOMODE_TOGGLE;
    else if (para == "select")
      actionId = ACTION_STEREOMODE_SELECT;
    else if (para == "tomono")
      actionId = ACTION_STEREOMODE_TOMONO;

    // already have a valid actionID return it
    if (actionId > -1)
      return CAction(actionId);

    // still no valid action ID, check if parameter is a supported stereomode
    if (ConvertStringToGuiStereoMode(para) > -1)
      return CAction(ACTION_STEREOMODE_SET, para);
  }
  return CAction(ACTION_NONE);
}

void CStereoscopicsManager::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();

  if (settingId == CSettings::SETTING_VIDEOSCREEN_STEREOSCOPICMODE)
  {
    RENDER_STEREO_MODE mode = GetStereoMode();
    CLog::Log(LOGDEBUG, "StereoscopicsManager: stereo mode setting changed to {}",
              ConvertGuiStereoModeToString(mode));
    ApplyStereoMode(mode);
  }
}

bool CStereoscopicsManager::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_PLAYBACK_STOPPED:
  case GUI_MSG_PLAYLISTPLAYER_STOPPED:
    OnPlaybackStopped();
    break;
  }

  return false;
}

bool CStereoscopicsManager::OnAction(const CAction &action)
{
  RENDER_STEREO_MODE mode = GetStereoMode();

  if (action.GetID() == ACTION_STEREOMODE_NEXT)
  {
    SetStereoModeByUser(GetNextSupportedStereoMode(mode));
    return true;
  }
  else if (action.GetID() == ACTION_STEREOMODE_PREVIOUS)
  {
    SetStereoModeByUser(GetNextSupportedStereoMode(mode, RENDER_STEREO_MODE_COUNT - 1));
    return true;
  }
  else if (action.GetID() == ACTION_STEREOMODE_TOGGLE)
  {
    if (mode == RENDER_STEREO_MODE_OFF)
    {
      RENDER_STEREO_MODE targetMode = GetPreferredPlaybackMode();

      // if user selected a specific mode before, make sure to
      // switch back into that mode on toggle.
      if (m_stereoModeSetByUser != RENDER_STEREO_MODE_UNDEFINED)
      {
        // if user mode is set to OFF, he manually turned it off before. In this case use the last user applied mode
        if (m_stereoModeSetByUser != RENDER_STEREO_MODE_OFF)
          targetMode = m_stereoModeSetByUser;
        else if (m_lastStereoModeSetByUser != RENDER_STEREO_MODE_UNDEFINED && m_lastStereoModeSetByUser != RENDER_STEREO_MODE_OFF)
          targetMode = m_lastStereoModeSetByUser;
      }

      SetStereoModeByUser(targetMode);
    }
    else
    {
      SetStereoModeByUser(RENDER_STEREO_MODE_OFF);
    }
    return true;
  }
  else if (action.GetID() == ACTION_STEREOMODE_SELECT)
  {
    SetStereoModeByUser(GetStereoModeByUserChoice());
    return true;
  }
  else if (action.GetID() == ACTION_STEREOMODE_TOMONO)
  {
    if (mode == RENDER_STEREO_MODE_MONO)
    {
      RENDER_STEREO_MODE targetMode = GetPreferredPlaybackMode();

      // if we have an old userdefined stereomode, use that one as toggle target
      if (m_stereoModeSetByUser != RENDER_STEREO_MODE_UNDEFINED)
      {
        // if user mode is set to OFF, he manually turned it off before. In this case use the last user applied mode
        if (m_stereoModeSetByUser != RENDER_STEREO_MODE_OFF && m_stereoModeSetByUser != mode)
          targetMode = m_stereoModeSetByUser;
        else if (m_lastStereoModeSetByUser != RENDER_STEREO_MODE_UNDEFINED && m_lastStereoModeSetByUser != RENDER_STEREO_MODE_OFF && m_lastStereoModeSetByUser != mode)
          targetMode = m_lastStereoModeSetByUser;
      }

      SetStereoModeByUser(targetMode);
    }
    else
    {
      SetStereoModeByUser(RENDER_STEREO_MODE_MONO);
    }
    return true;
  }
  else if (action.GetID() == ACTION_STEREOMODE_SET)
  {
    int stereoMode = ConvertStringToGuiStereoMode(action.GetName());
    if (stereoMode > -1)
      SetStereoModeByUser(static_cast<RENDER_STEREO_MODE>(stereoMode));
    return true;
  }

  return false;
}

void CStereoscopicsManager::ApplyStereoMode(const RENDER_STEREO_MODE &mode, bool notify)
{
  RENDER_STEREO_MODE currentMode = CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode();
  CLog::Log(LOGDEBUG,
            "StereoscopicsManager::ApplyStereoMode: trying to apply stereo mode. Current: {} | "
            "Target: {}",
            ConvertGuiStereoModeToString(currentMode), ConvertGuiStereoModeToString(mode));
  if (currentMode != mode)
  {
    CServiceBroker::GetWinSystem()->GetGfxContext().SetStereoMode(mode);
    CLog::Log(LOGDEBUG, "StereoscopicsManager: stereo mode changed to {}",
              ConvertGuiStereoModeToString(mode));
    if (notify)
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(36501), GetLabelForStereoMode(mode));
  }
}

std::string CStereoscopicsManager::GetVideoStereoMode() const
{
  std::string playerMode;

  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->IsPlaying())
    playerMode = CServiceBroker::GetDataCacheCore().GetVideoStereoMode();

  return playerMode;
}

bool CStereoscopicsManager::IsVideoStereoscopic() const
{
  std::string mode = GetVideoStereoMode();
  return !mode.empty() && mode != "mono";
}

void CStereoscopicsManager::OnStreamChange()
{
  STEREOSCOPIC_PLAYBACK_MODE playbackMode = static_cast<STEREOSCOPIC_PLAYBACK_MODE>(m_settings->GetInt(CSettings::SETTING_VIDEOPLAYER_STEREOSCOPICPLAYBACKMODE));
  RENDER_STEREO_MODE mode = GetStereoMode();

  // early return if playback mode should be ignored and we're in no stereoscopic mode right now
  if (playbackMode == STEREOSCOPIC_PLAYBACK_MODE_IGNORE && mode == RENDER_STEREO_MODE_OFF)
    return;

  if (!CStereoscopicsManager::IsVideoStereoscopic())
  {
    // exit stereo mode if started item is not stereoscopic
    // and if user prefers to stop 3D playback when movie is finished
    if (mode != RENDER_STEREO_MODE_OFF && m_settings->GetBool(CSettings::SETTING_VIDEOPLAYER_QUITSTEREOMODEONSTOP))
      SetStereoMode(RENDER_STEREO_MODE_OFF);
    return;
  }

  // if we're not in stereomode yet, restore previously selected stereo mode in case it was user selected
  if (m_stereoModeSetByUser != RENDER_STEREO_MODE_UNDEFINED)
  {
    SetStereoMode(m_stereoModeSetByUser);
    return;
  }

  RENDER_STEREO_MODE preferred = GetPreferredPlaybackMode();
  RENDER_STEREO_MODE playing = GetStereoModeOfPlayingVideo();

  if (mode != RENDER_STEREO_MODE_OFF)
  {
    // don't change mode if user selected to not exit stereomode on playback stop
    // users selecting this option usually have to manually switch their TV into 3D mode
    // and would be annoyed by having to switch TV modes when next movies comes up
    // @todo probably add a new setting for just this behavior
    if (m_settings->GetBool(CSettings::SETTING_VIDEOPLAYER_QUITSTEREOMODEONSTOP) == false)
      return;

    // only change to new stereo mode if not yet in preferred stereo mode
    if (mode == preferred || (preferred == RENDER_STEREO_MODE_AUTO && mode == playing))
      return;
  }

  switch (playbackMode)
  {
  case STEREOSCOPIC_PLAYBACK_MODE_ASK: // Ask
    {
      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_PAUSE);

      CGUIDialogSelect* pDlgSelect = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
      pDlgSelect->Reset();
      pDlgSelect->SetHeading(CVariant{g_localizeStrings.Get(36527)});

      int idx_playing   = -1;

      // add choices
      int idx_preferred = pDlgSelect->Add(g_localizeStrings.Get(36524) // preferred
                                     + " ("
                                     + GetLabelForStereoMode(preferred)
                                     + ")");

      int idx_mono = pDlgSelect->Add(GetLabelForStereoMode(RENDER_STEREO_MODE_MONO)); // mono / 2d

      if (playing != RENDER_STEREO_MODE_OFF && playing != preferred && preferred != RENDER_STEREO_MODE_AUTO && CServiceBroker::GetRenderSystem()->SupportsStereo(playing)) // same as movie
        idx_playing = pDlgSelect->Add(g_localizeStrings.Get(36532)
                                    + " ("
                                    + GetLabelForStereoMode(playing)
                                    + ")");

      int idx_select = pDlgSelect->Add( g_localizeStrings.Get(36531) ); // other / select

      pDlgSelect->Open();

      if (pDlgSelect->IsConfirmed())
      {
        int iItem = pDlgSelect->GetSelectedItem();
        if      (iItem == idx_preferred) mode = preferred;
        else if (iItem == idx_mono)      mode = RENDER_STEREO_MODE_MONO;
        else if (iItem == idx_playing)   mode = RENDER_STEREO_MODE_AUTO;
        else if (iItem == idx_select)    mode = GetStereoModeByUserChoice();

        SetStereoModeByUser(mode);
      }

      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_UNPAUSE);
    }
    break;
  case STEREOSCOPIC_PLAYBACK_MODE_PREFERRED: // Stereoscopic
    SetStereoMode(preferred);
    break;
  case 2: // Mono
    SetStereoMode(RENDER_STEREO_MODE_MONO);
    break;
  default:
    break;
  }
}

void CStereoscopicsManager::OnPlaybackStopped(void)
{
  RENDER_STEREO_MODE mode = GetStereoMode();

  if (m_settings->GetBool(CSettings::SETTING_VIDEOPLAYER_QUITSTEREOMODEONSTOP) && mode != RENDER_STEREO_MODE_OFF)
    SetStereoMode(RENDER_STEREO_MODE_OFF);

  // reset user modes on playback end to start over new on next playback and not end up in a probably unwanted mode
  if (m_stereoModeSetByUser != RENDER_STEREO_MODE_OFF)
    m_lastStereoModeSetByUser = m_stereoModeSetByUser;

  m_stereoModeSetByUser = RENDER_STEREO_MODE_UNDEFINED;
}
