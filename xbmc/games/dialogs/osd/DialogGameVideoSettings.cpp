/*
 *      Copyright (C) 2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DialogGameVideoSettings.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIDialog.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "profiles/ProfilesManager.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "video/ViewModeSettings.h"
#include "Application.h"
#include "ApplicationPlayer.h"
#include "GUIPassword.h"
#include "ServiceBroker.h"

using namespace KODI;
using namespace GAME;

// Video dimensions
#define SETTING_GAME_SCALINGMETHOD       "game.scalingmethod"
#define SETTING_GAME_VIEW_MODE           "game.viewmode"

// Video actions
#define SETTING_GAME_MAKE_DEFAULT        "game.save"
#define SETTING_GAME_CALIBRATION         "game.calibration"

CDialogGameVideoSettings::CDialogGameVideoSettings() :
  CGUIDialogSettingsManualBase(WINDOW_DIALOG_GAME_VIDEO_SETTINGS, "DialogSettings.xml")
{
  // Initialize CGUIWindow
  m_loadType = KEEP_IN_MEMORY;
}

void CDialogGameVideoSettings::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (!setting)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  CGameSettings &gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_GAME_SCALINGMETHOD)
  {
    gameSettings.SetScalingMethod(static_cast<ESCALINGMETHOD>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue()));

    //! @todo
    CVideoSettings &videoSettings = CMediaSettings::GetInstance().GetCurrentVideoSettings();
    videoSettings.m_ScalingMethod = static_cast<ESCALINGMETHOD>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
  }
  else if (settingId == SETTING_GAME_VIEW_MODE)
  {
    gameSettings.SetViewMode(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());

    g_application.m_pPlayer->SetRenderViewMode(gameSettings.ViewMode());
  }
}

void CDialogGameVideoSettings::OnSettingAction(std::shared_ptr<const CSetting> setting)
{
  if (!setting)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_GAME_CALIBRATION)
  {
    // Launch calibration window
    if (CProfilesManager::GetInstance().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE  &&
        g_passwordManager.CheckSettingLevelLock(CServiceBroker::GetSettings().GetSetting(CSettings::SETTING_VIDEOSCREEN_GUICALIBRATION)->GetLevel()))
      return;
    g_windowManager.ForceActivateWindow(WINDOW_SCREEN_CALIBRATION);
  }
  //! @todo implement
  else if (settingId == SETTING_GAME_MAKE_DEFAULT)
    Save();
}

void CDialogGameVideoSettings::Save()
{
  if (CProfilesManager::GetInstance().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE &&
      !g_passwordManager.CheckSettingLevelLock(::SettingLevel::Expert))
    return;

  // Prompt user if they are sure
  if (CGUIDialogYesNo::ShowAndGetInput(CVariant(12376), CVariant(12377)))
  {
    CMediaSettings::GetInstance().GetDefaultGameSettings() = CMediaSettings::GetInstance().GetCurrentGameSettings();
    CServiceBroker::GetSettings().Save();
  }
}

void CDialogGameVideoSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SetHeading(13395);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_OKAY_BUTTON);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 15067);
}

void CDialogGameVideoSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  const std::shared_ptr<CSettingCategory> category = AddCategory("gamevideosettings", -1);
  if (!category)
  {
    CLog::Log(LOGERROR, "CDialogGameVideoSettings: unable to setup settings");
    return;
  }

  // Get all necessary setting groups
  const std::shared_ptr<CSettingGroup> groupVideo = AddGroup(category);
  if (!groupVideo)
  {
    CLog::Log(LOGERROR, "CDialogGameVideoSettings: unable to setup settings");
    return;
  }
  const std::shared_ptr<CSettingGroup> groupActions = AddGroup(category);
  if (!groupActions)
  {
    CLog::Log(LOGERROR, "CDialogGameVideoSettings: unable to setup settings");
    return;
  }

  CGameSettings &gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();
  
  TranslatableIntegerSettingOptions entries;
  entries.push_back(std::make_pair(16301, VS_SCALINGMETHOD_NEAREST));
  entries.push_back(std::make_pair(16302, VS_SCALINGMETHOD_LINEAR));
  entries.push_back(std::make_pair(16303, VS_SCALINGMETHOD_CUBIC ));
  entries.push_back(std::make_pair(16304, VS_SCALINGMETHOD_LANCZOS2));
  entries.push_back(std::make_pair(16323, VS_SCALINGMETHOD_SPLINE36_FAST));
  entries.push_back(std::make_pair(16315, VS_SCALINGMETHOD_LANCZOS3_FAST));
  entries.push_back(std::make_pair(16322, VS_SCALINGMETHOD_SPLINE36));
  entries.push_back(std::make_pair(16305, VS_SCALINGMETHOD_LANCZOS3));
  entries.push_back(std::make_pair(16306, VS_SCALINGMETHOD_SINC8));
//  entries.push_back(make_pair(?????, VS_SCALINGMETHOD_NEDI));
  entries.push_back(std::make_pair(16307, VS_SCALINGMETHOD_BICUBIC_SOFTWARE));
  entries.push_back(std::make_pair(16308, VS_SCALINGMETHOD_LANCZOS_SOFTWARE));
  entries.push_back(std::make_pair(16309, VS_SCALINGMETHOD_SINC_SOFTWARE));
  entries.push_back(std::make_pair(13120, VS_SCALINGMETHOD_VDPAU_HARDWARE));
  entries.push_back(std::make_pair(16319, VS_SCALINGMETHOD_DXVA_HARDWARE));

  // Remove unsupported methods
  for(TranslatableIntegerSettingOptions::iterator it = entries.begin(); it != entries.end(); )
  {
    if (g_application.m_pPlayer->Supports(static_cast<ESCALINGMETHOD>(it->second)))
      ++it;
    else
      it = entries.erase(it);
  }

  // Video settings
  AddSpinner(groupVideo, SETTING_GAME_SCALINGMETHOD, 16300, SettingLevel::Basic, static_cast<int>(gameSettings.ScalingMethod()), entries);

  if (g_application.m_pPlayer->Supports(RENDERFEATURE_STRETCH) || g_application.m_pPlayer->Supports(RENDERFEATURE_PIXEL_RATIO))
    AddList(groupVideo, SETTING_GAME_VIEW_MODE, 629, SettingLevel::Basic, gameSettings.ViewMode(), CViewModeSettings::ViewModesFiller, 629);

  // General settings
  AddButton(groupActions, SETTING_GAME_MAKE_DEFAULT, 12376, SettingLevel::Basic);
  AddButton(groupActions, SETTING_GAME_CALIBRATION, 214, SettingLevel::Basic);
}
