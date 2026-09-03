/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/Language.h"

#include "ServiceBroker.h"
#include "addons/LanguageResource.h"
#include "language/LangInfo.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <optional>

using namespace KODI::LANGUAGE;

namespace
{
//! The character set Kodi's own strings are in
constexpr std::string_view builtInCharset = "CP1252";

bool Names(const std::string& setting, std::string_view value)
{
  return StringUtils::EqualsNoCase(setting, value);
}

/*!
 * \brief The character set the user chose.
 * \param[in] settingId The setting holding the choice.
 * \return The set, or nothing where the setting is on its default.
 */
/*!
 * \brief A display name without the qualifier it carries in parentheses - a region profile named
 *        "USA (12h)" becomes "USA".
 * \note Fit for text meant for a reader and nothing else. What a language is, CLanguageTag says.
 */
std::string WithoutQualifier(const std::string& name)
{
  const size_t openParen = name.find('(');
  if (openParen == std::string::npos)
    return name;

  std::string base = name.substr(0, openParen);
  StringUtils::TrimRight(base);
  return base;
}

std::optional<std::string> ChosenCharset(const std::string& settingId)
{
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const auto setting = std::static_pointer_cast<CSettingString>(settings->GetSetting(settingId));
  if (setting->IsDefault())
    return std::nullopt;

  return setting->GetValue();
}
} // namespace

CLanguagePreference CLanguagePreference::Parse(const std::string& setting, Kind unrecognized)
{
  if (Names(setting, languageSettingDefault))
    return {Kind::FollowUI, {}};

  if (Names(setting, languageSettingOriginal))
    return {Kind::Original, {}};

  if (const auto tag = CLanguageTag::TryParse(setting); tag.has_value())
    return {Kind::Language, *tag};

  if (!setting.empty())
    CLog::LogF(LOGERROR, "'{}' does not name a language, ignoring it", setting);

  return {unrecognized, {}};
}

CLanguagePreference CLanguagePreference::ForAudio(const std::string& setting)
{
  if (Names(setting, audioLanguageSettingMediaDefault))
    return {Kind::MediaDefault, {}};

  return Parse(setting, Kind::FollowUI);
}

CLanguagePreference CLanguagePreference::ForSubtitles(const std::string& setting)
{
  if (Names(setting, subtitleLanguageSettingNone))
    return {Kind::None, {}};

  if (Names(setting, subtitleLanguageSettingForcedOnly))
    return {Kind::ForcedOnly, {}};

  return Parse(setting, Kind::FollowUI);
}

CLanguageTag CLanguagePreference::Resolve(const CLanguageTag& ui) const
{
  switch (m_kind)
  {
    case Kind::Language:
      return m_language;
    case Kind::FollowUI:
      return ui;
    case Kind::MediaDefault:
    case Kind::Original:
    case Kind::None:
    case Kind::ForcedOnly:
      return {};
  }

  return {};
}

CLanguage& CLanguage::GetInstance()
{
  static CLanguage language;
  return language;
}

CLanguageTag CLanguage::Audio(bool fallbackToUI /* = true */) const
{
  const CLanguageTag audio{m_audio.Resolve(m_ui)};
  if (!audio.IsUndetermined() || !fallbackToUI)
    return audio;

  return m_ui;
}

CLanguageTag CLanguage::Subtitle(bool fallbackToUI /* = true */) const
{
  if (!m_subtitle.GetLanguage().IsUndetermined())
    return m_subtitle.GetLanguage();

  if (!m_audio.GetLanguage().IsUndetermined())
    return m_audio.GetLanguage();

  return fallbackToUI ? m_ui : CLanguageTag{};
}

void CLanguage::SetPack(const LanguageResourcePtr& pack)
{
  m_pack = pack;
  m_ui = m_pack ? m_pack->GetLanguage() : CLanguageTag::English();
}

std::string CLanguage::PackName() const
{
  return m_pack ? m_pack->Name() : "";
}

std::string CLanguage::GuiCharset() const
{
  if (const auto chosen = ChosenCharset(CSettings::SETTING_LOCALE_CHARSET); chosen.has_value())
    return *chosen;

  return m_pack ? m_pack->GetGuiCharset() : std::string{builtInCharset};
}

std::string CLanguage::SubtitleCharset() const
{
  if (const auto chosen = ChosenCharset(CSettings::SETTING_SUBTITLES_CHARSET); chosen.has_value())
    return *chosen;

  return m_pack ? m_pack->GetSubtitleCharset() : std::string{builtInCharset};
}

std::string KODI::LANGUAGE::DescribeLanguage(CLanguageTag::Notation notation,
                                             const CLanguage& language,
                                             const CLangInfo& region,
                                             bool withRegion)
{
  const CLanguageTag& ui{language.UI()};

  std::string named;
  switch (notation)
  {
    case CLanguageTag::ENGLISH_NAME:
      // The name the pack declares for itself, which is the one a user has seen named
      named = language.PackName();
      break;
    case CLanguageTag::ISO_NAME:
      named = ui.ToEnglishLanguageName();
      break;
    case CLanguageTag::ISO_639_1:
      named = ui.AsIso6391();
      break;
    case CLanguageTag::ISO_639_2:
      named = ui.AsIso6392B();
      break;
  }

  // Some languages have no code in the requested ISO 639 notation - Asturian, for one, has no
  // ISO 639-1 code - and there is then nothing to join a place to
  if (!withRegion || named.empty())
    return named;

  // The place, named in the notation the language notation implies
  std::string place;
  switch (notation)
  {
    case CLanguageTag::ISO_639_1:
      place = region.GetRegionTerritory().AsIso3166_1Alpha2();
      break;
    case CLanguageTag::ISO_639_2:
      place = region.GetRegionTerritory().AsIso3166_1Alpha3();
      break;
    case CLanguageTag::ISO_NAME:
      place = WithoutQualifier(region.GetCurrentRegion());
      break;
    case CLanguageTag::ENGLISH_NAME:
      place = region.GetCurrentRegion();
      break;
  }

  if (!place.empty())
    named += "-" + place;

  return named;
}

CLanguage::Tokens CLanguage::SortTokens() const
{
  Tokens tokens{m_pack ? m_pack->GetSortTokens() : Tokens{}};

  const auto& advanced = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_vecTokens;
  tokens.insert(advanced.begin(), advanced.end());

  return tokens;
}
