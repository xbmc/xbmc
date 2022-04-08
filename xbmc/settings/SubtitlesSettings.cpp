/*
 *  Copyright (C) 2012-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SubtitlesSettings.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "guilib/GUIFontManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/FontUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"


using namespace KODI;
using namespace SUBTITLES;

CSubtitlesSettings::CSubtitlesSettings()
{
  m_settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  m_settings->RegisterCallback(this, {CSettings::SETTING_LOCALE_SUBTITLELANGUAGE,
                                      CSettings::SETTING_SUBTITLES_PARSECAPTIONS,
                                      CSettings::SETTING_SUBTITLES_ALIGN,
                                      CSettings::SETTING_SUBTITLES_STEREOSCOPICDEPTH,
                                      CSettings::SETTING_SUBTITLES_FONTNAME,
                                      CSettings::SETTING_SUBTITLES_FONTSIZE,
                                      CSettings::SETTING_SUBTITLES_STYLE,
                                      CSettings::SETTING_SUBTITLES_COLOR,
                                      CSettings::SETTING_SUBTITLES_BORDERSIZE,
                                      CSettings::SETTING_SUBTITLES_BORDERCOLOR,
                                      CSettings::SETTING_SUBTITLES_OPACITY,
                                      CSettings::SETTING_SUBTITLES_BGCOLOR,
                                      CSettings::SETTING_SUBTITLES_BGOPACITY,
                                      CSettings::SETTING_SUBTITLES_BLUR,
                                      CSettings::SETTING_SUBTITLES_BACKGROUNDTYPE,
                                      CSettings::SETTING_SUBTITLES_SHADOWCOLOR,
                                      CSettings::SETTING_SUBTITLES_SHADOWOPACITY,
                                      CSettings::SETTING_SUBTITLES_SHADOWSIZE,
                                      CSettings::SETTING_SUBTITLES_CHARSET,
                                      CSettings::SETTING_SUBTITLES_OVERRIDEFONTS,
                                      CSettings::SETTING_SUBTITLES_OVERRIDESTYLES,
                                      CSettings::SETTING_SUBTITLES_LANGUAGES,
                                      CSettings::SETTING_SUBTITLES_STORAGEMODE,
                                      CSettings::SETTING_SUBTITLES_CUSTOMPATH,
                                      CSettings::SETTING_SUBTITLES_PAUSEONSEARCH,
                                      CSettings::SETTING_SUBTITLES_DOWNLOADFIRST,
                                      CSettings::SETTING_SUBTITLES_TV,
                                      CSettings::SETTING_SUBTITLES_MOVIE});
}

CSubtitlesSettings::~CSubtitlesSettings()
{
  m_settings->UnregisterCallback(this);
}

CSubtitlesSettings& CSubtitlesSettings::GetInstance()
{
  static CSubtitlesSettings sSubtitlesSettings;
  return sSubtitlesSettings;
}

void CSubtitlesSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
    return;

  SetChanged();
  NotifyObservers(ObservableMessageSettingsChanged);
  if (setting->GetId() == CSettings::SETTING_SUBTITLES_ALIGN)
  {
    SetChanged();
    NotifyObservers(ObservableMessagePositionChanged);
  }
}

void CSubtitlesSettings::SettingOptionsSubtitleFontsFiller(const SettingConstPtr& setting,
                                                           std::vector<StringSettingOption>& list,
                                                           std::string& current,
                                                           void* data)
{
  // From application system fonts folder we add the default font only
  std::string defaultFontPath =
      URIUtils::AddFileToFolder("special://xbmc/media/Fonts/", UTILS::FONT::FONT_DEFAULT_FILENAME);
  if (XFILE::CFile::Exists(defaultFontPath))
  {
    std::string familyName = UTILS::FONT::GetFontFamily(defaultFontPath);
    if (!familyName.empty())
    {
      list.emplace_back(g_localizeStrings.Get(571) + " " + familyName, FONT_DEFAULT_FAMILYNAME);
    }
  }
  // Add additionals fonts from the user fonts folder
  for (const std::string& familyName : g_fontManager.GetUserFontsFamilyNames())
  {
    list.emplace_back(familyName, familyName);
  }
}
