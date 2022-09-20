/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationSettingsHandling.h"

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/gui/GUIDialogAddonSettings.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationPowerHandling.h"
#include "application/ApplicationSkinHandling.h"
#include "application/ApplicationVolumeHandling.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#if defined(TARGET_DARWIN_OSX)
#include "utils/StringUtils.h"
#endif

namespace
{
bool IsPlaying(const std::string& condition,
               const std::string& value,
               const SettingConstPtr& setting,
               void* data)
{
  return data ? static_cast<CApplicationPlayer*>(data)->IsPlaying() : false;
}
} // namespace

void CApplicationSettingsHandling::RegisterSettings()
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  CSettingsManager* settingsMgr = settings->GetSettingsManager();

  settingsMgr->RegisterSettingsHandler(this);

  settingsMgr->RegisterCallback(this, {CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH,
                                       CSettings::SETTING_LOOKANDFEEL_SKIN,
                                       CSettings::SETTING_LOOKANDFEEL_SKINSETTINGS,
                                       CSettings::SETTING_LOOKANDFEEL_FONT,
                                       CSettings::SETTING_LOOKANDFEEL_SKINTHEME,
                                       CSettings::SETTING_LOOKANDFEEL_SKINCOLORS,
                                       CSettings::SETTING_LOOKANDFEEL_SKINZOOM,
                                       CSettings::SETTING_MUSICPLAYER_REPLAYGAINPREAMP,
                                       CSettings::SETTING_MUSICPLAYER_REPLAYGAINNOGAINPREAMP,
                                       CSettings::SETTING_MUSICPLAYER_REPLAYGAINTYPE,
                                       CSettings::SETTING_MUSICPLAYER_REPLAYGAINAVOIDCLIPPING,
                                       CSettings::SETTING_SCRAPERS_MUSICVIDEOSDEFAULT,
                                       CSettings::SETTING_SCREENSAVER_MODE,
                                       CSettings::SETTING_SCREENSAVER_PREVIEW,
                                       CSettings::SETTING_SCREENSAVER_SETTINGS,
                                       CSettings::SETTING_AUDIOCDS_SETTINGS,
                                       CSettings::SETTING_VIDEOSCREEN_GUICALIBRATION,
                                       CSettings::SETTING_VIDEOSCREEN_TESTPATTERN,
                                       CSettings::SETTING_VIDEOPLAYER_USEMEDIACODEC,
                                       CSettings::SETTING_VIDEOPLAYER_USEMEDIACODECSURFACE,
                                       CSettings::SETTING_AUDIOOUTPUT_VOLUMESTEPS,
                                       CSettings::SETTING_SOURCE_VIDEOS,
                                       CSettings::SETTING_SOURCE_MUSIC,
                                       CSettings::SETTING_SOURCE_PICTURES,
                                       CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN});

  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (!appPlayer)
    return;

  settingsMgr->RegisterCallback(
      &appPlayer->GetSeekHandler(),
      {CSettings::SETTING_VIDEOPLAYER_SEEKDELAY, CSettings::SETTING_VIDEOPLAYER_SEEKSTEPS,
       CSettings::SETTING_MUSICPLAYER_SEEKDELAY, CSettings::SETTING_MUSICPLAYER_SEEKSTEPS});

  settingsMgr->AddDynamicCondition("isplaying", IsPlaying, appPlayer.get());

  settings->RegisterSubSettings(this);
}

void CApplicationSettingsHandling::UnregisterSettings()
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  CSettingsManager* settingsMgr = settings->GetSettingsManager();
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (!appPlayer)
    return;

  settings->UnregisterSubSettings(this);
  settingsMgr->RemoveDynamicCondition("isplaying");
  settingsMgr->UnregisterCallback(&appPlayer->GetSeekHandler());
  settingsMgr->UnregisterCallback(this);
  settingsMgr->UnregisterSettingsHandler(this);
}

void CApplicationSettingsHandling::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (!setting)
    return;

  auto& components = CServiceBroker::GetAppComponents();
  const auto appSkin = components.GetComponent<CApplicationSkinHandling>();
  if (appSkin->OnSettingChanged(*setting))
    return;

  const auto appVolume = components.GetComponent<CApplicationVolumeHandling>();
  if (appVolume->OnSettingChanged(*setting))
    return;

  const auto appPower = components.GetComponent<CApplicationPowerHandling>();
  if (appPower->OnSettingChanged(*setting))
    return;

  const std::string& settingId = setting->GetId();

  if (settingId == CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN)
  {
    if (CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenRoot())
      CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution(
          CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution(), true);
  }
  else if (settingId == CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH)
  {
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_RESTART);
  }
}

void CApplicationSettingsHandling::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  if (!setting)
    return;

  auto& components = CServiceBroker::GetAppComponents();
  const auto appPower = components.GetComponent<CApplicationPowerHandling>();
  if (appPower->OnSettingAction(*setting))
    return;

  const std::string& settingId = setting->GetId();
  if (settingId == CSettings::SETTING_LOOKANDFEEL_SKINSETTINGS)
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_SKIN_SETTINGS);
  else if (settingId == CSettings::SETTING_AUDIOCDS_SETTINGS)
  {
    ADDON::AddonPtr addon;
    if (CServiceBroker::GetAddonMgr().GetAddon(
            CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
                CSettings::SETTING_AUDIOCDS_ENCODER),
            addon, ADDON::AddonType::AUDIOENCODER, ADDON::OnlyEnabled::CHOICE_YES))
      CGUIDialogAddonSettings::ShowForAddon(addon);
  }
  else if (settingId == CSettings::SETTING_VIDEOSCREEN_GUICALIBRATION)
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_SCREEN_CALIBRATION);
  else if (settingId == CSettings::SETTING_SOURCE_VIDEOS)
  {
    std::vector<std::string> params{"library://video/files.xml", "return"};
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_VIDEO_NAV, params);
  }
  else if (settingId == CSettings::SETTING_SOURCE_MUSIC)
  {
    std::vector<std::string> params{"library://music/files.xml", "return"};
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_MUSIC_NAV, params);
  }
  else if (settingId == CSettings::SETTING_SOURCE_PICTURES)
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_PICTURES);
}

bool CApplicationSettingsHandling::OnSettingUpdate(const std::shared_ptr<CSetting>& setting,
                                                   const char* oldSettingId,
                                                   const TiXmlNode* oldSettingNode)
{
  if (!setting)
    return false;

#if defined(TARGET_DARWIN_OSX)
  if (setting->GetId() == CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE)
  {
    std::shared_ptr<CSettingString> audioDevice = std::static_pointer_cast<CSettingString>(setting);
    // Gotham and older didn't enumerate audio devices per stream on osx
    // add stream0 per default which should be ok for all old settings.
    if (!StringUtils::EqualsNoCase(audioDevice->GetValue(), "DARWINOSX:default") &&
        StringUtils::FindWords(audioDevice->GetValue().c_str(), ":stream") == std::string::npos)
    {
      std::string newSetting = audioDevice->GetValue();
      newSetting += ":stream0";
      return audioDevice->SetValue(newSetting);
    }
  }
#endif

  return false;
}

bool CApplicationSettingsHandling::Load(const TiXmlNode* settings)
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto appVolume = components.GetComponent<CApplicationVolumeHandling>();
  return appVolume->Load(settings);
}

bool CApplicationSettingsHandling::Save(TiXmlNode* settings) const
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appVolume = components.GetComponent<CApplicationVolumeHandling>();
  return appVolume->Save(settings);
}
