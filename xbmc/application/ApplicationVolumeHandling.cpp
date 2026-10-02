/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationVolumeHandling.h"

#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "dialogs/GUIDialogVolumeBar.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "interfaces/AnnouncementManager.h"
#include "music/tags/ReplayGain.h"
#include "peripherals/Peripherals.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"

#if defined(TARGET_ANDROID)
#include "platform/android/activity/XBMCApp.h"
#endif

#include <cmath>

#include <tinyxml.h>

float CApplicationVolumeHandling::GetVolumePercent() const
{
  // converts the hardware volume to a percentage
  return m_volumeLevel * 100.0f;
}

float CApplicationVolumeHandling::GetVolumeRatio() const
{
  return m_volumeLevel;
}

void CApplicationVolumeHandling::SetHardwareVolume(float hardwareVolume)
{
  m_volumeLevel = std::clamp(hardwareVolume, VOLUME_MINIMUM, VOLUME_MAXIMUM);

  IAE* ae = CServiceBroker::GetActiveAE();
  if (ae)
    ae->SetVolume(m_volumeLevel);
}

void CApplicationVolumeHandling::VolumeChanged()
{
  CVariant data(CVariant::VariantTypeObject);
  data["volume"] = static_cast<int>(std::lroundf(GetVolumePercent()));
  data["muted"] = m_muted;
  const auto announcementMgr = CServiceBroker::GetAnnouncementManager();
  announcementMgr->Announce(ANNOUNCEMENT::Application, "OnVolumeChanged", data);

  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  // if player has volume control, set it.
  if (appPlayer)
  {
    appPlayer->SetVolume(m_volumeLevel);
    appPlayer->SetMute(m_muted);
  }
}

void CApplicationVolumeHandling::ShowVolumeBar(const CAction* action)
{
  const auto& wm = CServiceBroker::GetGUI()->GetWindowManager();
  auto* volumeBar = wm.GetWindow<CGUIDialogVolumeBar>(WINDOW_DIALOG_VOLUME_BAR);
  if (volumeBar != nullptr && volumeBar->IsVolumeBarEnabled())
  {
    volumeBar->Open();
    if (action)
      volumeBar->OnAction(*action);
  }
}

bool CApplicationVolumeHandling::IsMuted() const
{
  if (CServiceBroker::GetPeripherals().IsMuted())
    return true;
  IAE* ae = CServiceBroker::GetActiveAE();
  if (ae)
    return ae->IsMuted();
  return true;
}

void CApplicationVolumeHandling::ToggleMute(void)
{
  if (m_muted)
    UnMute();
  else
    Mute();
}

void CApplicationVolumeHandling::SetMute(bool mute)
{
  if (m_muted != mute)
  {
    ToggleMute();
    m_muted = mute;
  }
}

void CApplicationVolumeHandling::Mute()
{
  if (CServiceBroker::GetPeripherals().Mute())
    return;

  IAE* ae = CServiceBroker::GetActiveAE();
  if (ae)
    ae->SetMute(true);
  m_muted = true;
  VolumeChanged();
}

void CApplicationVolumeHandling::UnMute()
{
  if (CServiceBroker::GetPeripherals().UnMute())
    return;

  IAE* ae = CServiceBroker::GetActiveAE();
  if (ae)
    ae->SetMute(false);
  m_muted = false;
  VolumeChanged();
}

void CApplicationVolumeHandling::SetVolume(float iValue, bool isPercentage)
{
  float hardwareVolume = iValue;

  if (isPercentage)
    hardwareVolume /= 100.0f;

  SetHardwareVolume(hardwareVolume);
  VolumeChanged();
}

void CApplicationVolumeHandling::CacheReplayGainSettings(const CSettings& settings)
{
  // initialize m_replayGainSettings
  m_replayGainSettings.m_type =
      static_cast<ReplayGain::Type>(settings.GetInt(CSettings::SETTING_MUSICPLAYER_REPLAYGAINTYPE));
  m_replayGainSettings.m_preAmp =
      static_cast<float>(settings.GetNumber(CSettings::SETTING_MUSICPLAYER_REPLAYGAINPREAMP));
  m_replayGainSettings.m_noGainPreAmp =
      static_cast<float>(settings.GetNumber(CSettings::SETTING_MUSICPLAYER_REPLAYGAINNOGAINPREAMP));
  m_replayGainSettings.m_avoidClipping =
      settings.GetBool(CSettings::SETTING_MUSICPLAYER_REPLAYGAINAVOIDCLIPPING);
}

bool CApplicationVolumeHandling::Load(const TiXmlNode* settings)
{
  if (!settings)
    return false;

  const TiXmlElement* audioElement = settings->FirstChildElement("audio");
  if (audioElement)
  {
    XMLUtils::GetBoolean(audioElement, "mute", m_muted);
    if (!XMLUtils::GetFloat(audioElement, "fvolumelevel", m_volumeLevel, VOLUME_MINIMUM,
                            VOLUME_MAXIMUM))
      m_volumeLevel = VOLUME_MAXIMUM;
  }

  return true;
}

bool CApplicationVolumeHandling::Save(TiXmlNode* settings) const
{
  if (!settings)
    return false;

  TiXmlElement volumeNode("audio");
  TiXmlNode* audioNode = settings->InsertEndChild(volumeNode);
  if (!audioNode)
    return false;

  XMLUtils::SetBoolean(audioNode, "mute", m_muted);
  XMLUtils::SetFloat(audioNode, "fvolumelevel", m_volumeLevel);

  return true;
}

bool CApplicationVolumeHandling::OnSettingChanged(const CSetting& setting)
{
  const std::string& settingId = setting.GetId();

  if (StringUtils::EqualsNoCase(settingId, CSettings::SETTING_MUSICPLAYER_REPLAYGAINTYPE))
    m_replayGainSettings.m_type =
        static_cast<ReplayGain::Type>(static_cast<const CSettingInt&>(setting).GetValue());
  else if (StringUtils::EqualsNoCase(settingId, CSettings::SETTING_MUSICPLAYER_REPLAYGAINPREAMP) ||
           StringUtils::EqualsNoCase(settingId,
                                     CSettings::SETTING_MUSICPLAYER_REPLAYGAINNOGAINPREAMP))
  {
    const float gain{static_cast<float>(static_cast<const CSettingNumber&>(setting).GetValue())};

    // 0 dB gain value needs to be exactly 0 to avoid unwanted sign flips
    if (gain != 0.0f && (std::abs(gain) < 0.01f))
    {
      CServiceBroker::GetSettingsComponent()->GetSettings()->SetNumber(settingId, 0.0);
      return true;
    }
    if (StringUtils::EqualsNoCase(settingId, CSettings::SETTING_MUSICPLAYER_REPLAYGAINPREAMP))
      m_replayGainSettings.m_preAmp = gain;
    else if (StringUtils::EqualsNoCase(settingId,
                                       CSettings::SETTING_MUSICPLAYER_REPLAYGAINNOGAINPREAMP))
      m_replayGainSettings.m_noGainPreAmp = gain;
  }
  else if (StringUtils::EqualsNoCase(settingId,
                                     CSettings::SETTING_MUSICPLAYER_REPLAYGAINAVOIDCLIPPING))
    m_replayGainSettings.m_avoidClipping = static_cast<const CSettingBool&>(setting).GetValue();
  else
    return false;

  return true;
}

bool CApplicationVolumeHandling::OnAction(const CAction& action)
{
  switch (action.GetID())
  {
    case ACTION_MUTE:
      ToggleMute();
      ShowVolumeBar(&action);
      return true;

    case ACTION_TOGGLE_DIGITAL_ANALOG:
      TogglePassthrough();
      return true;

    case ACTION_VOLUME_UP:
    case ACTION_VOLUME_DOWN:
      if (!action.GetAmount())
        return false;
      [[fallthrough]];
    case ACTION_VOLUME_SET:
      ChangeVolume(action);
      // show visual feedback of volume or passthrough indicator
      ShowVolumeBar(&action);
      return true;

    default:
      return false;
  }
}

void CApplicationVolumeHandling::TogglePassthrough()
{
  const auto settings{CServiceBroker::GetSettingsComponent()->GetSettings()};
  settings->SetBool(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH,
                    !settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH));

  auto& windowManager{CServiceBroker::GetGUI()->GetWindowManager()};
  if (windowManager.GetActiveWindow() == WINDOW_SETTINGS_SYSTEM)
  {
    CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, windowManager.GetActiveWindow());
    windowManager.SendMessage(msg);
  }
}

void CApplicationVolumeHandling::ChangeVolume(const CAction& action)
{
  const auto settings{CServiceBroker::GetSettingsComponent()->GetSettings()};
  const auto appPlayer{CServiceBroker::GetAppComponents().GetComponent<CApplicationPlayer>()};

  // The level cannot be applied to a bitstream, but with volume control enabled
  // it is still adjusted and announced, so an external processor can follow it
  if (appPlayer->IsPassthrough() &&
      !settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGHVOLUMECONTROL))
    return;

  if (IsMuted())
    UnMute();

// Android has steps based on the max available volume level
#if defined(TARGET_ANDROID)
  const float step = (VOLUME_MAXIMUM - VOLUME_MINIMUM) / CXBMCApp::GetMaxSystemVolume();
#else
  int volumesteps = settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_VOLUMESTEPS);
  // sanity check
  if (volumesteps == 0)
    volumesteps = 90;

  float step = (VOLUME_MAXIMUM - VOLUME_MINIMUM) / volumesteps;
  if (action.GetRepeat())
    step *= action.GetRepeat() * 50; // 50 fps
#endif

  float volume = GetVolumeRatio();
  if (action.GetID() == ACTION_VOLUME_UP)
    volume += action.GetAmount() * action.GetAmount() * step;
  else if (action.GetID() == ACTION_VOLUME_DOWN)
    volume -= action.GetAmount() * action.GetAmount() * step;
  else
    volume = action.GetAmount() * step;

  if (volume != GetVolumeRatio())
    SetVolume(volume, false);
}
