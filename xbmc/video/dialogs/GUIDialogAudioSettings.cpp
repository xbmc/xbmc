/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogAudioSettings.h"

#include "GUIPassword.h"
#include "ServiceBroker.h"
#include "addons/Skin.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationVolumeHandling.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/IPlayer.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIMessage.h"
#include "guilib/LocalizeStrings.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "settings/lib/SettingsManager.h"
#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include <memory>
#include <string>
#include <vector>

#define SETTING_AUDIO_VOLUME                   "audio.volume"
#define SETTING_AUDIO_VOLUME_AMPLIFICATION     "audio.volumeamplification"
#define SETTING_AUDIO_CENTERMIXLEVEL           "audio.centermixlevel"
#define SETTING_AUDIO_DELAY                    "audio.delay"
#define SETTING_AUDIO_STREAM                   "audio.stream"
#define SETTING_AUDIO_PASSTHROUGH              "audio.digitalanalog"
#define SETTING_AUDIO_MAKE_DEFAULT             "audio.makedefault"

CGUIDialogAudioSettings::CGUIDialogAudioSettings()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_AUDIO_OSD_SETTINGS, "DialogSettings.xml")
{ }

CGUIDialogAudioSettings::~CGUIDialogAudioSettings() = default;

void CGUIDialogAudioSettings::FrameMove()
{
  // update the volume setting if necessary
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appVolume = components.GetComponent<CApplicationVolumeHandling>();
  float newVolume = appVolume->GetVolumeRatio();
  if (newVolume != m_volume)
    GetSettingsManager()->SetNumber(SETTING_AUDIO_VOLUME, static_cast<double>(newVolume));

  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->HasPlayer())
  {
    const CVideoSettings videoSettings = appPlayer->GetVideoSettings();

    // these settings can change on the fly
    //! @todo (needs special handling): m_settingsManager->SetInt(SETTING_AUDIO_STREAM, g_application.GetAppPlayer().GetAudioStream());
    GetSettingsManager()->SetNumber(SETTING_AUDIO_DELAY,
                                    static_cast<double>(videoSettings.m_AudioDelay));
    GetSettingsManager()->SetBool(SETTING_AUDIO_PASSTHROUGH, CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH));
  }

  CGUIDialogSettingsManualBase::FrameMove();
}

std::string CGUIDialogAudioSettings::FormatDelay(float value, float interval)
{
  if (fabs(value) < 0.5f * interval)
    return StringUtils::Format(g_localizeStrings.Get(22003), 0.0);
  if (value < 0)
    return StringUtils::Format(g_localizeStrings.Get(22004), fabs(value));

  return StringUtils::Format(g_localizeStrings.Get(22005), value);
}

std::string CGUIDialogAudioSettings::FormatDecibel(float value)
{
  return StringUtils::Format(g_localizeStrings.Get(14054), value);
}

std::string CGUIDialogAudioSettings::FormatPercentAsDecibel(float value)
{
  return StringUtils::Format(g_localizeStrings.Get(14054), CAEUtil::PercentToGain(value));
}

void CGUIDialogAudioSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_AUDIO_VOLUME)
  {
    m_volume = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    const auto appVolume = components.GetComponent<CApplicationVolumeHandling>();
    appVolume->SetVolume(m_volume, false); // false - value is not in percent
  }
  else if (settingId == SETTING_AUDIO_VOLUME_AMPLIFICATION)
  {
    float value = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    appPlayer->SetDynamicRangeCompression((long)(value * 100));
  }
  else if (settingId == SETTING_AUDIO_CENTERMIXLEVEL)
  {
    CVideoSettings vs = appPlayer->GetVideoSettings();
    vs.m_CenterMixLevel = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_AUDIO_DELAY)
  {
    float value = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    appPlayer->SetAVDelay(value);
  }
  else if (settingId == SETTING_AUDIO_STREAM)
  {
    m_audioStream = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    // only change the audio stream if a different one has been asked for
    if (appPlayer->GetAudioStream() != m_audioStream)
    {
      appPlayer->SetAudioStream(m_audioStream); // Set the audio stream to the one selected
    }
  }
  else if (settingId == SETTING_AUDIO_PASSTHROUGH)
  {
    m_passthrough = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH, m_passthrough);
  }
}

void CGUIDialogAudioSettings::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_AUDIO_MAKE_DEFAULT)
    Save();
}

bool CGUIDialogAudioSettings::Save()
{
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  if (!g_passwordManager.CheckSettingLevelLock(SettingLevel::Expert) &&
      profileManager->GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
    return true;

  // prompt user if they are sure
  if (!CGUIDialogYesNo::ShowAndGetInput(CVariant{12376}, CVariant{12377}))
    return true;

  // reset the settings
  CVideoDatabase db;
  if (!db.Open())
    return true;

  db.EraseAllVideoSettings();
  db.Close();

  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  CMediaSettings::GetInstance().GetDefaultVideoSettings() = appPlayer->GetVideoSettings();
  CMediaSettings::GetInstance().GetDefaultVideoSettings().m_AudioStream = -1;
  CServiceBroker::GetSettingsComponent()->GetSettings()->Save();

  return true;
}

void CGUIDialogAudioSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SetHeading(13396);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_OKAY_BUTTON);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 15067);
}

void CGUIDialogAudioSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  const std::shared_ptr<CSettingCategory> category = AddCategory("audiosubtitlesettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogAudioSettings: unable to setup settings");
    return;
  }

  // get all necessary setting groups
  const std::shared_ptr<CSettingGroup> groupAudio = AddGroup(category);
  if (groupAudio == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogAudioSettings: unable to setup settings");
    return;
  }
  const std::shared_ptr<CSettingGroup> groupSubtitles = AddGroup(category);
  if (groupSubtitles == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogAudioSettings: unable to setup settings");
    return;
  }
  const std::shared_ptr<CSettingGroup> groupSaveAsDefault = AddGroup(category);
  if (groupSaveAsDefault == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogAudioSettings: unable to setup settings");
    return;
  }

  bool usePopup = g_SkinInfo->HasSkinFile("DialogSlider.xml");

  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  const CVideoSettings videoSettings = appPlayer->GetVideoSettings();
  if (appPlayer->HasPlayer())
  {
    appPlayer->GetAudioCapabilities(m_audioCaps);
  }

  // register IsPlayingPassthrough condition
  GetSettingsManager()->AddDynamicCondition("IsPlayingPassthrough", IsPlayingPassthrough);

  CSettingDependency dependencyAudioOutputPassthroughDisabled(SettingDependencyType::Enable, GetSettingsManager());
  dependencyAudioOutputPassthroughDisabled.Or()
      ->Add(std::make_shared<CSettingDependencyCondition>(SETTING_AUDIO_PASSTHROUGH, "false",
                                                          SettingDependencyOperator::Equals, false,
                                                          GetSettingsManager()))
      ->Add(std::make_shared<CSettingDependencyCondition>("IsPlayingPassthrough", "", "", true,
                                                          GetSettingsManager()));
  SettingDependencies depsAudioOutputPassthroughDisabled;
  depsAudioOutputPassthroughDisabled.push_back(dependencyAudioOutputPassthroughDisabled);

  // audio settings
  // audio volume setting
  const auto appVolume = components.GetComponent<CApplicationVolumeHandling>();
  m_volume = appVolume->GetVolumeRatio();
  std::shared_ptr<CSettingNumber> settingAudioVolume =
      AddSlider(groupAudio, SETTING_AUDIO_VOLUME, 13376, SettingLevel::Basic, m_volume, 14054,
                CApplicationVolumeHandling::VOLUME_MINIMUM,
                CApplicationVolumeHandling::VOLUME_MAXIMUM / 100.0f,
                CApplicationVolumeHandling::VOLUME_MAXIMUM);
  settingAudioVolume->SetDependencies(depsAudioOutputPassthroughDisabled);
  std::static_pointer_cast<CSettingControlSlider>(settingAudioVolume->GetControl())->SetFormatter(SettingFormatterPercentAsDecibel);

  // audio volume amplification setting
  if (SupportsAudioFeature(IPC_AUD_AMP))
  {
    std::shared_ptr<CSettingNumber> settingAudioVolumeAmplification = AddSlider(groupAudio, SETTING_AUDIO_VOLUME_AMPLIFICATION, 660, SettingLevel::Basic, videoSettings.m_VolumeAmplification, 14054, VOLUME_DRC_MINIMUM * 0.01f, (VOLUME_DRC_MAXIMUM - VOLUME_DRC_MINIMUM) / 6000.0f, VOLUME_DRC_MAXIMUM * 0.01f);
    settingAudioVolumeAmplification->SetDependencies(depsAudioOutputPassthroughDisabled);
  }

  // downmix: center mix level
  {
    AddSlider(groupAudio, SETTING_AUDIO_CENTERMIXLEVEL, 39112, SettingLevel::Basic,
              videoSettings.m_CenterMixLevel, 14050, -10, 1, 30,
              -1, false, false, true, 39113);
  }

  // audio delay setting
  if (SupportsAudioFeature(IPC_AUD_OFFSET))
  {
    std::shared_ptr<CSettingNumber> settingAudioDelay = AddSlider(
        groupAudio, SETTING_AUDIO_DELAY, 297, SettingLevel::Basic, videoSettings.m_AudioDelay, 0,
        -CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoAudioDelayRange,
        AUDIO_DELAY_STEP,
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoAudioDelayRange, 297,
        usePopup);
    std::static_pointer_cast<CSettingControlSlider>(settingAudioDelay->GetControl())->SetFormatter(SettingFormatterDelay);
  }

  // audio stream setting
  if (SupportsAudioFeature(IPC_AUD_SELECT_STREAM))
    AddAudioStreams(groupAudio, SETTING_AUDIO_STREAM);

  // audio digital/analog setting
  if (SupportsAudioFeature(IPC_AUD_SELECT_OUTPUT))
  {
    m_passthrough = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH);
    AddToggle(groupAudio, SETTING_AUDIO_PASSTHROUGH, 348, SettingLevel::Basic, m_passthrough);
  }

  // subtitle stream setting
  AddButton(groupSaveAsDefault, SETTING_AUDIO_MAKE_DEFAULT, 12376, SettingLevel::Basic);
}

bool CGUIDialogAudioSettings::SupportsAudioFeature(int feature)
{
  for (Features::iterator itr = m_audioCaps.begin(); itr != m_audioCaps.end(); ++itr)
  {
    if (*itr == feature || *itr == IPC_AUD_ALL)
      return true;
  }

  return false;
}

void CGUIDialogAudioSettings::AddAudioStreams(const std::shared_ptr<CSettingGroup>& group,
                                              const std::string& settingId)
{
  if (group == NULL || settingId.empty())
    return;

  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  m_audioStream = appPlayer->GetAudioStream();
  if (m_audioStream < 0)
    m_audioStream = 0;

  AddList(group, settingId, 460, SettingLevel::Basic, m_audioStream, AudioStreamsOptionFiller, 460);
}

bool CGUIDialogAudioSettings::IsPlayingPassthrough(const std::string& condition,
                                                   const std::string& value,
                                                   const SettingConstPtr& setting,
                                                   void* data)
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  return appPlayer->IsPassthrough();
}

void CGUIDialogAudioSettings::AudioStreamsOptionFiller(const SettingConstPtr& setting,
                                                       std::vector<IntegerSettingOption>& list,
                                                       int& current,
                                                       void* data)
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  int audioStreamCount = appPlayer->GetAudioStreamCount();

  std::string strFormat = "{:s} - {:s} - {:d} " + g_localizeStrings.Get(10127);
  std::string strUnknown = "[" + g_localizeStrings.Get(13205) + "]";

  // cycle through each audio stream and add it to our list control
  for (int i = 0; i < audioStreamCount; ++i)
  {
    std::string strItem;
    std::string strLanguage;

    AudioStreamInfo info;
    appPlayer->GetAudioStreamInfo(i, info);

    if (!g_LangCodeExpander.Lookup(info.language, strLanguage))
      strLanguage = strUnknown;

    if (info.name.length() == 0)
      info.name = strUnknown;

    strItem = StringUtils::Format(strFormat, strLanguage, info.name, info.channels);

    strItem += FormatFlags(info.flags);
    strItem += StringUtils::Format(" ({}/{})", i + 1, audioStreamCount);
    list.emplace_back(strItem, i);
  }

  if (list.empty())
  {
    list.emplace_back(g_localizeStrings.Get(231), -1);
    current = -1;
  }
}

std::string CGUIDialogAudioSettings::SettingFormatterDelay(
    const std::shared_ptr<const CSettingControlSlider>& control,
    const CVariant& value,
    const CVariant& minimum,
    const CVariant& step,
    const CVariant& maximum)
{
  if (!value.isDouble())
    return "";

  float fValue = value.asFloat();
  float fStep = step.asFloat();

  if (fabs(fValue) < 0.5f * fStep)
    return StringUtils::Format(g_localizeStrings.Get(22003), 0.0);
  if (fValue < 0)
    return StringUtils::Format(g_localizeStrings.Get(22004), fabs(fValue));

  return StringUtils::Format(g_localizeStrings.Get(22005), fValue);
}

std::string CGUIDialogAudioSettings::SettingFormatterPercentAsDecibel(
    const std::shared_ptr<const CSettingControlSlider>& control,
    const CVariant& value,
    const CVariant& minimum,
    const CVariant& step,
    const CVariant& maximum)
{
  if (control == NULL || !value.isDouble())
    return "";

  std::string formatString = control->GetFormatString();
  if (control->GetFormatLabel() > -1)
    formatString = g_localizeStrings.Get(control->GetFormatLabel());

  return StringUtils::Format(formatString, CAEUtil::PercentToGain(value.asFloat()));
}

std::string CGUIDialogAudioSettings::FormatFlags(StreamFlags flags)
{
  std::vector<std::string> localizedFlags;
  if (flags & StreamFlags::FLAG_DEFAULT)
    localizedFlags.emplace_back(g_localizeStrings.Get(39105));
  if (flags & StreamFlags::FLAG_FORCED)
    localizedFlags.emplace_back(g_localizeStrings.Get(39106));
  if (flags & StreamFlags::FLAG_HEARING_IMPAIRED)
    localizedFlags.emplace_back(g_localizeStrings.Get(39107));
  if (flags &  StreamFlags::FLAG_VISUAL_IMPAIRED)
    localizedFlags.emplace_back(g_localizeStrings.Get(39108));
  if (flags & StreamFlags::FLAG_ORIGINAL)
    localizedFlags.emplace_back(g_localizeStrings.Get(39111));

  std::string formated = StringUtils::Join(localizedFlags, ", ");

  if (!formated.empty())
    formated = StringUtils::Format(" [{}]", formated);

  return formated;
}
