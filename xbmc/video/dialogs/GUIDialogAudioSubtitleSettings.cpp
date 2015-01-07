/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://xbmc.org
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

#include "GUIDialogAudioSubtitleSettings.h"
#include "Application.h"
#include "FileItem.h"
#include "GUIPassword.h"
#include "URL.h"
#include "addons/Skin.h"
#include "cores/IPlayer.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/File.h"
#include "guilib/LocalizeStrings.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/LangCodeExpander.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"

#define SETTING_AUDIO_VOLUME                   "audio.volume"
#define SETTING_AUDIO_VOLUME_AMPLIFICATION     "audio.volumeamplification"
#define SETTING_AUDIO_DELAY                    "audio.delay"
#define SETTING_AUDIO_STREAM                   "audio.stream"
#define SETTING_AUDIO_OUTPUT_TO_ALL_SPEAKERS   "audio.outputtoallspeakers"
#define SETTING_AUDIO_PASSTHROUGH           "audio.digitalanalog"

#define SETTING_SUBTITLE_ENABLE                "subtitles.enable"
#define SETTING_SUBTITLE_DELAY                 "subtitles.delay"
#define SETTING_SUBTITLE_STREAM                "subtitles.stream"
#define SETTING_SUBTITLE_BROWSER               "subtitles.browser"

#define SETTING_AUDIO_MAKE_DEFAULT             "audio.makedefault"

using namespace std;

CGUIDialogAudioSubtitleSettings::CGUIDialogAudioSubtitleSettings()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_AUDIO_OSD_SETTINGS, "VideoOSDSettings.xml"),
    m_passthrough(false)
{ }

CGUIDialogAudioSubtitleSettings::~CGUIDialogAudioSubtitleSettings()
{ }

void CGUIDialogAudioSubtitleSettings::FrameMove()
{
  // update the volume setting if necessary
  float newVolume = g_application.GetVolume(false);
  if (newVolume != m_volume)
    m_settingsManager->SetNumber(SETTING_AUDIO_VOLUME, newVolume);

  if (g_application.m_pPlayer->HasPlayer())
  {
    const CVideoSettings &videoSettings = CMediaSettings::Get().GetCurrentVideoSettings();
    
    // these settings can change on the fly
    m_settingsManager->SetNumber(SETTING_AUDIO_DELAY, videoSettings.m_AudioDelay);
    // TODO (needs special handling): m_settingsManager->SetInt(SETTING_AUDIO_STREAM, g_application.m_pPlayer->GetAudioStream());
    m_settingsManager->SetBool(SETTING_AUDIO_OUTPUT_TO_ALL_SPEAKERS, videoSettings.m_OutputToAllSpeakers);
    m_settingsManager->SetBool(SETTING_AUDIO_PASSTHROUGH, CSettings::Get().GetBool("audiooutput.passthrough"));

    // TODO: m_settingsManager->SetBool(SETTING_SUBTITLE_ENABLE, g_application.m_pPlayer->GetSubtitleVisible());
    //   \-> Unless subtitle visibility can change on the fly, while Dialog is up, this code should be removed.
    m_settingsManager->SetNumber(SETTING_SUBTITLE_DELAY, videoSettings.m_SubtitleDelay);
    // TODO (needs special handling): m_settingsManager->SetInt(SETTING_SUBTITLE_STREAM, g_application.m_pPlayer->GetSubtitle());
  }

  CGUIDialogSettingsManualBase::FrameMove();
}

std::string CGUIDialogAudioSubtitleSettings::FormatDelay(float value, float interval)
{
  if (fabs(value) < 0.5f * interval)
    return StringUtils::Format(g_localizeStrings.Get(22003).c_str(), 0.0);
  if (value < 0)
    return StringUtils::Format(g_localizeStrings.Get(22004).c_str(), fabs(value));

  return StringUtils::Format(g_localizeStrings.Get(22005).c_str(), value);
}

std::string CGUIDialogAudioSubtitleSettings::FormatDecibel(float value)
{
  return StringUtils::Format(g_localizeStrings.Get(14054).c_str(), value);
}

std::string CGUIDialogAudioSubtitleSettings::FormatPercentAsDecibel(float value)
{
  return StringUtils::Format(g_localizeStrings.Get(14054).c_str(), CAEUtil::PercentToGain(value));
}

void CGUIDialogAudioSubtitleSettings::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);
  
  CVideoSettings &videoSettings = CMediaSettings::Get().GetCurrentVideoSettings();
  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_AUDIO_VOLUME)
  {
    m_volume = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    g_application.SetVolume(m_volume, false); // false - value is not in percent
  }
  else if (settingId == SETTING_AUDIO_VOLUME_AMPLIFICATION)
  {
    videoSettings.m_VolumeAmplification = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    g_application.m_pPlayer->SetDynamicRangeCompression((long)(videoSettings.m_VolumeAmplification * 100));
  }
  else if (settingId == SETTING_AUDIO_DELAY)
  {
    videoSettings.m_AudioDelay = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    g_application.m_pPlayer->SetAVDelay(videoSettings.m_AudioDelay);
  }
  else if (settingId == SETTING_AUDIO_STREAM)
  {
    m_audioStream = static_cast<const CSettingInt*>(setting)->GetValue();
    // only change the audio stream if a different one has been asked for
    if (g_application.m_pPlayer->GetAudioStream() != m_audioStream)
    {
      videoSettings.m_AudioStream = m_audioStream;
      g_application.m_pPlayer->SetAudioStream(m_audioStream);    // Set the audio stream to the one selected
    }
  }
  else if (settingId == SETTING_AUDIO_OUTPUT_TO_ALL_SPEAKERS)
  {
    videoSettings.m_OutputToAllSpeakers = static_cast<const CSettingBool*>(setting)->GetValue();
    g_application.Restart();
  }
  else if (settingId == SETTING_AUDIO_PASSTHROUGH)
  {
    m_passthrough = static_cast<const CSettingBool*>(setting)->GetValue();
    CSettings::Get().SetBool("audiooutput.passthrough", m_passthrough);
  }
  else if (settingId == SETTING_SUBTITLE_ENABLE)
  {
    m_subtitleVisible = videoSettings.m_SubtitleOn = static_cast<const CSettingBool*>(setting)->GetValue();
    g_application.m_pPlayer->SetSubtitleVisible(videoSettings.m_SubtitleOn);
  }
  else if (settingId == SETTING_SUBTITLE_DELAY)
  {
    videoSettings.m_SubtitleDelay = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    g_application.m_pPlayer->SetSubTitleDelay(videoSettings.m_SubtitleDelay);
  }
  else if (settingId == SETTING_SUBTITLE_STREAM)
  {
    m_subtitleStream = videoSettings.m_SubtitleStream = static_cast<const CSettingInt*>(setting)->GetValue();
    g_application.m_pPlayer->SetSubtitle(m_subtitleStream);
  }
}

void CGUIDialogAudioSubtitleSettings::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingAction(setting);
  
  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_SUBTITLE_BROWSER)
  {
    std::string strPath;
    if (URIUtils::IsInRAR(g_application.CurrentFileItem().GetPath()) || URIUtils::IsInZIP(g_application.CurrentFileItem().GetPath()))
      strPath = CURL(g_application.CurrentFileItem().GetPath()).GetHostName();
    else
      strPath = g_application.CurrentFileItem().GetPath();

    std::string strMask = ".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.aqt|.jss|.ass|.idx|.rar|.zip";
    if (g_application.GetCurrentPlayer() == EPC_DVDPLAYER)
      strMask = ".srt|.rar|.zip|.ifo|.smi|.sub|.idx|.ass|.ssa|.txt";
    VECSOURCES shares(*CMediaSourceSettings::Get().GetSources("video"));
    if (CMediaSettings::Get().GetAdditionalSubtitleDirectoryChecked() != -1 && !CSettings::Get().GetString("subtitles.custompath").empty())
    {
      CMediaSource share;
      std::vector<std::string> paths;
      paths.push_back(URIUtils::GetDirectory(strPath));
      paths.push_back(CSettings::Get().GetString("subtitles.custompath"));
      share.FromNameAndPaths("video",g_localizeStrings.Get(21367),paths);
      shares.push_back(share);
      strPath = share.strPath;
      URIUtils::AddSlashAtEnd(strPath);
    }
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, strMask, g_localizeStrings.Get(293), strPath, false, true)) // "subtitles"
    {
      if (URIUtils::HasExtension(strPath, ".sub"))
      {
        if (XFILE::CFile::Exists(URIUtils::ReplaceExtension(strPath, ".idx")))
          strPath = URIUtils::ReplaceExtension(strPath, ".idx");
      }
      
      int id = g_application.m_pPlayer->AddSubtitle(strPath);
      if (id >= 0)
      {
        m_subtitleStream = id;
        g_application.m_pPlayer->SetSubtitle(m_subtitleStream);
        g_application.m_pPlayer->SetSubtitleVisible(true);
      }
      CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleCached = true;
      Close();
    }
  }
  else if (settingId == SETTING_AUDIO_MAKE_DEFAULT)
    Save();
}

void CGUIDialogAudioSubtitleSettings::Save()
{
  if (!g_passwordManager.CheckSettingLevelLock(SettingLevelExpert) &&
      CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
    return;

  // prompt user if they are sure
  if (!CGUIDialogYesNo::ShowAndGetInput(12376, 750, 0, 12377))
    return;

  // reset the settings
  CVideoDatabase db;
  if (!db.Open())
    return;

  db.EraseVideoSettings();
  db.Close();

  CMediaSettings::Get().GetDefaultVideoSettings() = CMediaSettings::Get().GetCurrentVideoSettings();
  CMediaSettings::Get().GetDefaultVideoSettings().m_SubtitleStream = -1;
  CMediaSettings::Get().GetDefaultVideoSettings().m_AudioStream = -1;
  CSettings::Get().Save();
}

void CGUIDialogAudioSubtitleSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  CSettingCategory *category = AddCategory("audiosubtitlesettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogAudioSubtitleSettings: unable to setup settings");
    return;
  }

  // get all necessary setting groups
  CSettingGroup *groupAudio = AddGroup(category);
  if (groupAudio == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogAudioSubtitleSettings: unable to setup settings");
    return;
  }
  CSettingGroup *groupSubtitles = AddGroup(category);
  if (groupSubtitles == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogAudioSubtitleSettings: unable to setup settings");
    return;
  }
  CSettingGroup *groupSaveAsDefault = AddGroup(category);
  if (groupSaveAsDefault == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogAudioSubtitleSettings: unable to setup settings");
    return;
  }

  bool usePopup = g_SkinInfo->HasSkinFile("DialogSlider.xml");

  CVideoSettings &videoSettings = CMediaSettings::Get().GetCurrentVideoSettings();
  
  if (g_application.m_pPlayer->HasPlayer())
  {
    g_application.m_pPlayer->GetAudioCapabilities(m_audioCaps);
    g_application.m_pPlayer->GetSubtitleCapabilities(m_subCaps);
  }

  CSettingDependency dependencyAudioOutputPassthroughDisabled(SettingDependencyTypeEnable, m_settingsManager);
  dependencyAudioOutputPassthroughDisabled.And()
    ->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_AUDIO_PASSTHROUGH, "false", SettingDependencyOperatorEquals, false, m_settingsManager)));
  SettingDependencies depsAudioOutputPassthroughDisabled;
  depsAudioOutputPassthroughDisabled.push_back(dependencyAudioOutputPassthroughDisabled);
  
  // audio settings
  // audio volume setting
  m_volume = g_application.GetVolume(false);
  CSettingNumber *settingAudioVolume = AddSlider(groupAudio, SETTING_AUDIO_VOLUME, 13376, 0, m_volume, 14054, VOLUME_MINIMUM, VOLUME_MAXIMUM / 100.0f, VOLUME_MAXIMUM);
  settingAudioVolume->SetDependencies(depsAudioOutputPassthroughDisabled);
  static_cast<CSettingControlSlider*>(settingAudioVolume->GetControl())->SetFormatter(SettingFormatterPercentAsDecibel);

  // audio volume amplification setting
  if (SupportsAudioFeature(IPC_AUD_AMP))
  {
    CSettingNumber *settingAudioVolumeAmplification = AddSlider(groupAudio, SETTING_AUDIO_VOLUME_AMPLIFICATION, 660, 0, videoSettings.m_VolumeAmplification, 14054, VOLUME_DRC_MINIMUM * 0.01f, (VOLUME_DRC_MAXIMUM - VOLUME_DRC_MINIMUM) / 6000.0f, VOLUME_DRC_MAXIMUM * 0.01f);
    settingAudioVolumeAmplification->SetDependencies(depsAudioOutputPassthroughDisabled);
  }

  // audio delay setting
  if (SupportsAudioFeature(IPC_AUD_OFFSET))
  {
    CSettingNumber *settingAudioDelay = AddSlider(groupAudio, SETTING_AUDIO_DELAY, 297, 0, videoSettings.m_AudioDelay, 0, -g_advancedSettings.m_videoAudioDelayRange, 0.025f, g_advancedSettings.m_videoAudioDelayRange, 297, usePopup);
    static_cast<CSettingControlSlider*>(settingAudioDelay->GetControl())->SetFormatter(SettingFormatterDelay);
  }
  
  // audio stream setting
  if (SupportsAudioFeature(IPC_AUD_SELECT_STREAM))
    AddAudioStreams(groupAudio, SETTING_AUDIO_STREAM);

  // audio output to all speakers setting
  // TODO: remove this setting
  if (SupportsAudioFeature(IPC_AUD_OUTPUT_STEREO))
    AddToggle(groupAudio, SETTING_AUDIO_OUTPUT_TO_ALL_SPEAKERS, 252, 0, videoSettings.m_OutputToAllSpeakers);

  // audio digital/analog setting
  if (SupportsAudioFeature(IPC_AUD_SELECT_OUTPUT))
  {
    m_passthrough = CSettings::Get().GetBool("audiooutput.passthrough");
    AddToggle(groupAudio, SETTING_AUDIO_PASSTHROUGH, 348, 0, m_passthrough);
  }

  // subitlte settings
  m_subtitleVisible = g_application.m_pPlayer->GetSubtitleVisible();
  // subtitle enabled setting
  AddToggle(groupSubtitles, SETTING_SUBTITLE_ENABLE, 13397, 0, m_subtitleVisible);

  // subtitle delay setting
  if (SupportsSubtitleFeature(IPC_SUBS_OFFSET))
  {
    CSettingNumber *settingSubtitleDelay = AddSlider(groupSubtitles, SETTING_SUBTITLE_DELAY, 22006, 0, videoSettings.m_SubtitleDelay, 0, -g_advancedSettings.m_videoSubsDelayRange, 0.1f, g_advancedSettings.m_videoSubsDelayRange, 22006, usePopup);
    static_cast<CSettingControlSlider*>(settingSubtitleDelay->GetControl())->SetFormatter(SettingFormatterDelay);
  }

  // subtitle stream setting
  if (SupportsSubtitleFeature(IPC_SUBS_SELECT))
    AddSubtitleStreams(groupSubtitles, SETTING_SUBTITLE_STREAM);

  // subtitle browser setting
  if (SupportsSubtitleFeature(IPC_SUBS_EXTERNAL))
    AddButton(groupSubtitles, SETTING_SUBTITLE_BROWSER, 13250, 0);

  // subtitle stream setting
  AddButton(groupSaveAsDefault, SETTING_AUDIO_MAKE_DEFAULT, 12376, 0);
}

bool CGUIDialogAudioSubtitleSettings::SupportsAudioFeature(int feature)
{
  for (Features::iterator itr = m_audioCaps.begin(); itr != m_audioCaps.end(); ++itr)
  {
    if (*itr == feature || *itr == IPC_AUD_ALL)
      return true;
  }

  return false;
}

bool CGUIDialogAudioSubtitleSettings::SupportsSubtitleFeature(int feature)
{
  for (Features::iterator itr = m_subCaps.begin(); itr != m_subCaps.end(); ++itr)
  {
    if (*itr == feature || *itr == IPC_SUBS_ALL)
      return true;
  }

  return false;
}

void CGUIDialogAudioSubtitleSettings::AddAudioStreams(CSettingGroup *group, const std::string &settingId)
{
  if (group == NULL || settingId.empty())
    return;

  m_audioStream = g_application.m_pPlayer->GetAudioStream();
  if (m_audioStream < 0)
    m_audioStream = 0;

  AddSpinner(group, settingId, 460, 0, m_audioStream, AudioStreamsOptionFiller);
}

void CGUIDialogAudioSubtitleSettings::AddSubtitleStreams(CSettingGroup *group, const std::string &settingId)
{
  if (group == NULL || settingId.empty())
    return;

  m_subtitleStream = g_application.m_pPlayer->GetSubtitle();
  if (m_subtitleStream < 0)
    m_subtitleStream = 0;

  AddSpinner(group, settingId, 462, 0, m_subtitleStream, SubtitleStreamsOptionFiller);
}

void CGUIDialogAudioSubtitleSettings::AudioStreamsOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  int audioStreamCount = g_application.m_pPlayer->GetAudioStreamCount();

  // cycle through each audio stream and add it to our list control
  for (int i = 0; i < audioStreamCount; ++i)
  {
    std::string strItem;
    std::string strLanguage;

    SPlayerAudioStreamInfo info;
    g_application.m_pPlayer->GetAudioStreamInfo(i, info);

    if (!g_LangCodeExpander.Lookup(strLanguage, info.language))
      strLanguage = g_localizeStrings.Get(13205); // Unknown

    if (info.name.length() == 0)
      strItem = strLanguage;
    else
      strItem = StringUtils::Format("%s - %s", strLanguage.c_str(), info.name.c_str());

    strItem += StringUtils::Format(" (%i/%i)", i + 1, audioStreamCount);
    list.push_back(make_pair(strItem, i));
  }

  if (list.empty())
  {
    list.push_back(make_pair(g_localizeStrings.Get(231), -1));
    current = -1;
  }
}

void CGUIDialogAudioSubtitleSettings::SubtitleStreamsOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  int subtitleStreamCount = g_application.m_pPlayer->GetSubtitleCount();

  // cycle through each subtitle and add it to our entry list
  for (int i = 0; i < subtitleStreamCount; ++i)
  {
    SPlayerSubtitleStreamInfo info;
    g_application.m_pPlayer->GetSubtitleStreamInfo(i, info);

    std::string strItem;
    std::string strLanguage;

    if (!g_LangCodeExpander.Lookup(strLanguage, info.language))
      strLanguage = g_localizeStrings.Get(13205); // Unknown

    if (info.name.length() == 0)
      strItem = strLanguage;
    else
      strItem = StringUtils::Format("%s - %s", strLanguage.c_str(), info.name.c_str());

    strItem += StringUtils::Format(" (%i/%i)", i + 1, subtitleStreamCount);

    list.push_back(make_pair(strItem, i));
  }

  // no subtitle streams - just add a "None" entry
  if (list.empty())
  {
    list.push_back(make_pair(g_localizeStrings.Get(231), -1));
    current = -1;
  }
}

std::string CGUIDialogAudioSubtitleSettings::SettingFormatterDelay(const CSettingControlSlider *control, const CVariant &value, const CVariant &minimum, const CVariant &step, const CVariant &maximum)
{
  if (!value.isDouble())
    return "";

  float fValue = value.asFloat();
  float fStep = step.asFloat();

  if (fabs(fValue) < 0.5f * fStep)
    return StringUtils::Format(g_localizeStrings.Get(22003).c_str(), 0.0);
  if (fValue < 0)
    return StringUtils::Format(g_localizeStrings.Get(22004).c_str(), fabs(fValue));

  return StringUtils::Format(g_localizeStrings.Get(22005).c_str(), fValue);
}

std::string CGUIDialogAudioSubtitleSettings::SettingFormatterPercentAsDecibel(const CSettingControlSlider *control, const CVariant &value, const CVariant &minimum, const CVariant &step, const CVariant &maximum)
{
  if (control == NULL || !value.isDouble())
    return "";

  std::string formatString = control->GetFormatString();
  if (control->GetFormatLabel() > -1)
    formatString = g_localizeStrings.Get(control->GetFormatLabel());

  return StringUtils::Format(formatString.c_str(), CAEUtil::PercentToGain(value.asFloat()));
}
