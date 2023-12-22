/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSubtitleSettings.h"

#include "FileItem.h"
#include "GUIDialogSubtitles.h"
#include "GUIPassword.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "addons/Skin.h"
#include "addons/VFSEntry.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "cores/IPlayer.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "settings/lib/SettingsManager.h"
#include "utils/FileUtils.h"
#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include <string>
#include <vector>

#define SETTING_SUBTITLE_ENABLE                "subtitles.enable"
#define SETTING_SUBTITLE_DELAY                 "subtitles.delay"
#define SETTING_SUBTITLE_STREAM                "subtitles.stream"
#define SETTING_SUBTITLE_BROWSER               "subtitles.browser"
#define SETTING_SUBTITLE_SEARCH                "subtitles.search"
#define SETTING_MAKE_DEFAULT                   "audio.makedefault"

CGUIDialogSubtitleSettings::CGUIDialogSubtitleSettings()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_SUBTITLE_OSD_SETTINGS, "DialogSettings.xml")
{ }

CGUIDialogSubtitleSettings::~CGUIDialogSubtitleSettings() = default;

void CGUIDialogSubtitleSettings::FrameMove()
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->HasPlayer())
  {
    const CVideoSettings videoSettings = appPlayer->GetVideoSettings();

    // these settings can change on the fly
    //! @todo m_settingsManager->SetBool(SETTING_SUBTITLE_ENABLE, g_application.GetAppPlayer().GetSubtitleVisible());
    //   \-> Unless subtitle visibility can change on the fly, while Dialog is up, this code should be removed.
    GetSettingsManager()->SetNumber(SETTING_SUBTITLE_DELAY,
                                    static_cast<double>(videoSettings.m_SubtitleDelay));
    //! @todo (needs special handling): m_settingsManager->SetInt(SETTING_SUBTITLE_STREAM, g_application.GetAppPlayer().GetSubtitle());
  }

  CGUIDialogSettingsManualBase::FrameMove();
}

bool CGUIDialogSubtitleSettings::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_SUBTITLE_DOWNLOADED)
  {
    Close();
  }
  return CGUIDialogSettingsManualBase::OnMessage(message);
}

void CGUIDialogSubtitleSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_SUBTITLE_ENABLE)
  {
    bool value = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    if (value)
    {
      // Ensure that we use/store the subtitle stream the user currently sees in the dialog.
      appPlayer->SetSubtitle(m_subtitleStream);
    }
    appPlayer->SetSubtitleVisible(value);
  }
  else if (settingId == SETTING_SUBTITLE_DELAY)
  {
    float value = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    appPlayer->SetSubTitleDelay(value);
  }
  else if (settingId == SETTING_SUBTITLE_STREAM)
  {
    m_subtitleStream = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    appPlayer->SetSubtitle(m_subtitleStream);
  }
}

std::string CGUIDialogSubtitleSettings::BrowseForSubtitle()
{
  std::string extras;
  for (const auto& vfsAddon : CServiceBroker::GetVFSAddonCache().GetAddonInstances())
  {
    if (vfsAddon->ID() == "vfs.rar" || vfsAddon->ID() == "vfs.libarchive")
      extras += '|' + vfsAddon->GetExtensions();
  }

  std::string strPath;
  const std::string dynPath{g_application.CurrentFileItem().GetDynPath()};
  if (URIUtils::IsInRAR(dynPath) || URIUtils::IsInZIP(dynPath))
  {
    strPath = CURL(dynPath).GetHostName();
  }
  else if (!URIUtils::IsPlugin(dynPath))
  {
    strPath = dynPath;
  }

  std::string strMask =
      ".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.aqt|.jss|.ass|.vtt|.idx|.zip|.sup";

  if (g_application.GetCurrentPlayer() == "VideoPlayer")
    strMask = ".srt|.zip|.ifo|.smi|.sub|.idx|.ass|.ssa|.vtt|.txt|.sup";

  strMask += extras;

  VECSOURCES shares(*CMediaSourceSettings::GetInstance().GetSources("video"));
  if (CMediaSettings::GetInstance().GetAdditionalSubtitleDirectoryChecked() != -1 && !CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SUBTITLES_CUSTOMPATH).empty())
  {
    CMediaSource share;
    std::vector<std::string> paths;
    if (!strPath.empty())
    {
      paths.push_back(URIUtils::GetDirectory(strPath));
    }
    paths.push_back(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SUBTITLES_CUSTOMPATH));
    share.FromNameAndPaths("video",g_localizeStrings.Get(21367),paths);
    shares.push_back(share);
    strPath = share.strPath;
    URIUtils::AddSlashAtEnd(strPath);
  }

  if (CGUIDialogFileBrowser::ShowAndGetFile(shares, strMask, g_localizeStrings.Get(293), strPath, false, true)) // "subtitles"
  {
    if (URIUtils::HasExtension(strPath, ".sub"))
    {
      if (CFileUtils::Exists(URIUtils::ReplaceExtension(strPath, ".idx")))
        strPath = URIUtils::ReplaceExtension(strPath, ".idx");
    }

    return strPath;
  }

  return "";
}

void CGUIDialogSubtitleSettings::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_SUBTITLE_BROWSER)
  {
    std::string strPath = BrowseForSubtitle();
    if (!strPath.empty())
    {
      auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      appPlayer->AddSubtitle(strPath);
      Close();
    }
  }
  else if (settingId == SETTING_SUBTITLE_SEARCH)
  {
    auto dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSubtitles>(WINDOW_DIALOG_SUBTITLES);
    if (dialog)
    {
      dialog->Open();
      m_subtitleStreamSetting->UpdateDynamicOptions();
    }
  }
  else if (settingId == SETTING_MAKE_DEFAULT)
    Save();
}

bool CGUIDialogSubtitleSettings::Save()
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
  CMediaSettings::GetInstance().GetDefaultVideoSettings().m_SubtitleStream = -1;
  CServiceBroker::GetSettingsComponent()->GetSettings()->Save();

  return true;
}

void CGUIDialogSubtitleSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SetHeading(24133);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_OKAY_BUTTON);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 15067);
}

void CGUIDialogSubtitleSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  const std::shared_ptr<CSettingCategory> category = AddCategory("audiosubtitlesettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogSubtitleSettings: unable to setup settings");
    return;
  }

  // get all necessary setting groups
  const std::shared_ptr<CSettingGroup> groupAudio = AddGroup(category);
  if (groupAudio == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogSubtitleSettings: unable to setup settings");
    return;
  }
  const std::shared_ptr<CSettingGroup> groupSubtitles = AddGroup(category);
  if (groupSubtitles == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogSubtitleSettings: unable to setup settings");
    return;
  }
  const std::shared_ptr<CSettingGroup> groupSaveAsDefault = AddGroup(category);
  if (groupSaveAsDefault == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogSubtitleSettings: unable to setup settings");
    return;
  }

  bool usePopup = g_SkinInfo->HasSkinFile("DialogSlider.xml");

  const CVideoSettings videoSettings = appPlayer->GetVideoSettings();

  if (appPlayer->HasPlayer())
  {
    appPlayer->GetSubtitleCapabilities(m_subtitleCapabilities);
  }

  // subtitle settings
  m_subtitleVisible = appPlayer->GetSubtitleVisible();

  // subtitle enabled setting
  AddToggle(groupSubtitles, SETTING_SUBTITLE_ENABLE, 13397, SettingLevel::Basic, m_subtitleVisible);

  // subtitle delay setting
  if (SupportsSubtitleFeature(IPC_SUBS_OFFSET))
  {
    std::shared_ptr<CSettingNumber> settingSubtitleDelay = AddSlider(groupSubtitles, SETTING_SUBTITLE_DELAY, 22006, SettingLevel::Basic, videoSettings.m_SubtitleDelay, 0, -CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoSubsDelayRange, 0.1f, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoSubsDelayRange, 22006, usePopup);
    std::static_pointer_cast<CSettingControlSlider>(settingSubtitleDelay->GetControl())->SetFormatter(SettingFormatterDelay);
  }

  // subtitle stream setting
  if (SupportsSubtitleFeature(IPC_SUBS_SELECT))
    AddSubtitleStreams(groupSubtitles, SETTING_SUBTITLE_STREAM);

  // subtitle browser setting
  if (SupportsSubtitleFeature(IPC_SUBS_EXTERNAL))
    AddButton(groupSubtitles, SETTING_SUBTITLE_BROWSER, 13250, SettingLevel::Basic);

  AddButton(groupSubtitles, SETTING_SUBTITLE_SEARCH, 24134, SettingLevel::Basic);

  // subtitle stream setting
  AddButton(groupSaveAsDefault, SETTING_MAKE_DEFAULT, 12376, SettingLevel::Basic);
}

bool CGUIDialogSubtitleSettings::SupportsSubtitleFeature(int feature)
{
  for (auto item : m_subtitleCapabilities)
  {
    if (item == feature || item == IPC_SUBS_ALL)
      return true;
  }
  return false;
}

void CGUIDialogSubtitleSettings::AddSubtitleStreams(const std::shared_ptr<CSettingGroup>& group,
                                                    const std::string& settingId)
{
  if (group == NULL || settingId.empty())
    return;

  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  m_subtitleStream = appPlayer->GetSubtitle();
  if (m_subtitleStream < 0)
    m_subtitleStream = 0;

  m_subtitleStreamSetting = AddList(group, settingId, 462, SettingLevel::Basic, m_subtitleStream, SubtitleStreamsOptionFiller, 462);
}

void CGUIDialogSubtitleSettings::SubtitleStreamsOptionFiller(
    const SettingConstPtr& setting,
    std::vector<IntegerSettingOption>& list,
    int& current,
    void* data)
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  int subtitleStreamCount = appPlayer->GetSubtitleCount();

  // cycle through each subtitle and add it to our entry list
  for (int i = 0; i < subtitleStreamCount; ++i)
  {
    SubtitleStreamInfo info;
    appPlayer->GetSubtitleStreamInfo(i, info);

    std::string strItem;
    std::string strLanguage;

    if (!g_LangCodeExpander.Lookup(info.language, strLanguage))
      strLanguage = g_localizeStrings.Get(13205); // Unknown

    if (info.name.length() == 0)
      strItem = strLanguage;
    else
      strItem = StringUtils::Format("{} - {}", strLanguage, info.name);

    strItem += FormatFlags(info.flags);
    strItem += StringUtils::Format(" ({}/{})", i + 1, subtitleStreamCount);

    list.emplace_back(strItem, i);
  }

  // no subtitle streams - just add a "None" entry
  if (list.empty())
  {
    list.emplace_back(g_localizeStrings.Get(231), -1);
    current = -1;
  }
}

std::string CGUIDialogSubtitleSettings::SettingFormatterDelay(
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

std::string CGUIDialogSubtitleSettings::FormatFlags(StreamFlags flags)
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

  std::string formated = StringUtils::Join(localizedFlags, ", ");

  if (!formated.empty())
    formated = StringUtils::Format(" [{}]", formated);

  return formated;
}
