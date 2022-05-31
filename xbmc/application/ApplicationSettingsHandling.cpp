/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationSettingsHandling.h"

#include "ApplicationPlayer.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/Skin.h"
#include "addons/gui/GUIDialogAddonSettings.h"
#include "application/ApplicationPowerHandling.h"
#include "application/ApplicationSkinHandling.h"
#include "application/ApplicationVolumeHandling.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"

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

CApplicationSettingsHandling::CApplicationSettingsHandling(
    CApplicationPlayer& appPlayer,
    CApplicationPowerHandling& powerHandling,
    CApplicationSkinHandling& skinHandling,
    CApplicationVolumeHandling& volumeHandling)
  : m_appPlayerRef(appPlayer),
    m_powerHandling(powerHandling),
    m_skinHandling(skinHandling),
    m_volumeHandling(volumeHandling)
{
}

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

  settingsMgr->RegisterCallback(
      &m_appPlayerRef.GetSeekHandler(),
      {CSettings::SETTING_VIDEOPLAYER_SEEKDELAY, CSettings::SETTING_VIDEOPLAYER_SEEKSTEPS,
       CSettings::SETTING_MUSICPLAYER_SEEKDELAY, CSettings::SETTING_MUSICPLAYER_SEEKSTEPS});

  settingsMgr->AddDynamicCondition("isplaying", IsPlaying, &m_appPlayerRef);

  settings->RegisterSubSettings(this);
}

void CApplicationSettingsHandling::UnregisterSettings()
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  CSettingsManager* settingsMgr = settings->GetSettingsManager();

  settings->UnregisterSubSettings(this);
  settingsMgr->RemoveDynamicCondition("isplaying");
  settingsMgr->UnregisterCallback(&m_appPlayerRef.GetSeekHandler());
  settingsMgr->UnregisterCallback(this);
  settingsMgr->UnregisterSettingsHandler(this);
}

void CApplicationSettingsHandling::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (!setting)
    return;

  if (m_skinHandling.OnSettingChanged(*setting))
    return;

  if (m_powerHandling.OnSettingChanged(*setting))
    return;

  if (m_powerHandling.OnSettingChanged(*setting))
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

  if (m_powerHandling.OnSettingAction(*setting))
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
            addon, ADDON::ADDON_AUDIOENCODER, ADDON::OnlyEnabled::CHOICE_YES))
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
  return m_volumeHandling.Load(settings);
}

bool CApplicationSettingsHandling::Save(TiXmlNode* settings) const
{
  return m_volumeHandling.Save(settings);
}
