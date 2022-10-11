/*
 *  Copyright (C) 2012-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SubtitlesSettings.h"

#include "guilib/GUIFontManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "settings/lib/Setting.h"
#include "utils/FileUtils.h"
#include "utils/FontUtils.h"
#include "utils/URIUtils.h"

using namespace KODI;
using namespace SUBTITLES;

CSubtitlesSettings::CSubtitlesSettings(const std::shared_ptr<CSettings>& settings)
  : m_settings(settings)
{
  m_settings->RegisterCallback(
      this,
      {CSettings::SETTING_LOCALE_SUBTITLELANGUAGE,  CSettings::SETTING_SUBTITLES_PARSECAPTIONS,
       CSettings::SETTING_SUBTITLES_ALIGN,          CSettings::SETTING_SUBTITLES_STEREOSCOPICDEPTH,
       CSettings::SETTING_SUBTITLES_FONTNAME,       CSettings::SETTING_SUBTITLES_FONTSIZE,
       CSettings::SETTING_SUBTITLES_STYLE,          CSettings::SETTING_SUBTITLES_COLOR,
       CSettings::SETTING_SUBTITLES_BORDERSIZE,     CSettings::SETTING_SUBTITLES_BORDERCOLOR,
       CSettings::SETTING_SUBTITLES_OPACITY,        CSettings::SETTING_SUBTITLES_BGCOLOR,
       CSettings::SETTING_SUBTITLES_BGOPACITY,      CSettings::SETTING_SUBTITLES_BLUR,
       CSettings::SETTING_SUBTITLES_BACKGROUNDTYPE, CSettings::SETTING_SUBTITLES_SHADOWCOLOR,
       CSettings::SETTING_SUBTITLES_SHADOWOPACITY,  CSettings::SETTING_SUBTITLES_SHADOWSIZE,
       CSettings::SETTING_SUBTITLES_MARGINVERTICAL, CSettings::SETTING_SUBTITLES_CHARSET,
       CSettings::SETTING_SUBTITLES_OVERRIDEFONTS,  CSettings::SETTING_SUBTITLES_OVERRIDESTYLES,
       CSettings::SETTING_SUBTITLES_LANGUAGES,      CSettings::SETTING_SUBTITLES_STORAGEMODE,
       CSettings::SETTING_SUBTITLES_CUSTOMPATH,     CSettings::SETTING_SUBTITLES_PAUSEONSEARCH,
       CSettings::SETTING_SUBTITLES_DOWNLOADFIRST,  CSettings::SETTING_SUBTITLES_TV,
       CSettings::SETTING_SUBTITLES_MOVIE});
}

CSubtitlesSettings::~CSubtitlesSettings()
{
  m_settings->UnregisterCallback(this);
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

Align CSubtitlesSettings::GetAlignment()
{
  return static_cast<Align>(m_settings->GetInt(CSettings::SETTING_SUBTITLES_ALIGN));
}

void CSubtitlesSettings::SetAlignment(Align align)
{
  m_settings->SetInt(CSettings::SETTING_SUBTITLES_ALIGN, static_cast<int>(align));
}

HorizontalAlign CSubtitlesSettings::GetHorizontalAlignment()
{
  return static_cast<HorizontalAlign>(
      m_settings->GetInt(CSettings::SETTING_SUBTITLES_CAPTIONSALIGN));
}

std::string CSubtitlesSettings::GetFontName()
{
  return m_settings->GetString(CSettings::SETTING_SUBTITLES_FONTNAME);
}

FontStyle CSubtitlesSettings::GetFontStyle()
{
  return static_cast<FontStyle>(m_settings->GetInt(CSettings::SETTING_SUBTITLES_STYLE));
}

int CSubtitlesSettings::GetFontSize()
{
  return m_settings->GetInt(CSettings::SETTING_SUBTITLES_FONTSIZE);
}

UTILS::COLOR::Color CSubtitlesSettings::GetFontColor()
{
  return UTILS::COLOR::ConvertHexToColor(m_settings->GetString(CSettings::SETTING_SUBTITLES_COLOR));
}

int CSubtitlesSettings::GetFontOpacity()
{
  return m_settings->GetInt(CSettings::SETTING_SUBTITLES_OPACITY);
}

int CSubtitlesSettings::GetBorderSize()
{
  return m_settings->GetInt(CSettings::SETTING_SUBTITLES_BORDERSIZE);
}

UTILS::COLOR::Color CSubtitlesSettings::GetBorderColor()
{
  return UTILS::COLOR::ConvertHexToColor(
      m_settings->GetString(CSettings::SETTING_SUBTITLES_BORDERCOLOR));
}

int CSubtitlesSettings::GetShadowSize()
{
  return m_settings->GetInt(CSettings::SETTING_SUBTITLES_SHADOWSIZE);
}

UTILS::COLOR::Color CSubtitlesSettings::GetShadowColor()
{
  return UTILS::COLOR::ConvertHexToColor(
      m_settings->GetString(CSettings::SETTING_SUBTITLES_SHADOWCOLOR));
}

int CSubtitlesSettings::GetShadowOpacity()
{
  return m_settings->GetInt(CSettings::SETTING_SUBTITLES_SHADOWOPACITY);
}

int CSubtitlesSettings::GetBlurSize()
{
  return m_settings->GetInt(CSettings::SETTING_SUBTITLES_BLUR);
}

BackgroundType CSubtitlesSettings::GetBackgroundType()
{
  return static_cast<BackgroundType>(
      m_settings->GetInt(CSettings::SETTING_SUBTITLES_BACKGROUNDTYPE));
}

UTILS::COLOR::Color CSubtitlesSettings::GetBackgroundColor()
{
  return UTILS::COLOR::ConvertHexToColor(
      m_settings->GetString(CSettings::SETTING_SUBTITLES_BGCOLOR));
}

int CSubtitlesSettings::GetBackgroundOpacity()
{
  return m_settings->GetInt(CSettings::SETTING_SUBTITLES_BGOPACITY);
}

bool CSubtitlesSettings::IsOverrideFonts()
{
  return m_settings->GetBool(CSettings::SETTING_SUBTITLES_OVERRIDEFONTS);
}

OverrideStyles CSubtitlesSettings::GetOverrideStyles()
{
  return static_cast<OverrideStyles>(
      m_settings->GetInt(CSettings::SETTING_SUBTITLES_OVERRIDESTYLES));
}

float CSubtitlesSettings::GetVerticalMarginPerc()
{
  // We return the vertical margin as percentage
  // to fit the current screen resolution
  return static_cast<float>(m_settings->GetNumber(CSettings::SETTING_SUBTITLES_MARGINVERTICAL));
}

void CSubtitlesSettings::SettingOptionsSubtitleFontsFiller(const SettingConstPtr& setting,
                                                           std::vector<StringSettingOption>& list,
                                                           std::string& current,
                                                           void* data)
{
  // From application system fonts folder we add the default font only
  std::string defaultFontPath =
      URIUtils::AddFileToFolder("special://xbmc/media/Fonts/", UTILS::FONT::FONT_DEFAULT_FILENAME);
  if (CFileUtils::Exists(defaultFontPath))
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
