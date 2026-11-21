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

/*!
 * \brief The character set the user chose.
 * \param[in] settingId The setting holding the choice.
 * \return The set, or nothing where the setting is on its default.
 */
std::optional<std::string> ChosenCharset(const std::string& settingId)
{
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const auto setting = std::static_pointer_cast<CSettingString>(settings->GetSetting(settingId));
  if (setting->IsDefault())
    return std::nullopt;

  return setting->GetValue();
}

/*!
 * \brief Store a parsed stream language setting, or the default where it names nothing.
 * \param[out] preference Where the setting goes.
 * \param[in] parsed The setting as parsed.
 * \param[in] setting The setting as written, for the log.
 * \return Whether the setting named something.
 */
bool SetPreference(CLanguagePreference& preference,
                   const std::optional<CLanguagePreference>& parsed,
                   const std::string& setting)
{
  if (!parsed.has_value())
    CLog::LogF(LOGWARNING, "unknown language '{}', using default", setting);

  preference = parsed.value_or(CLanguagePreference{});
  return parsed.has_value();
}
} // namespace

std::optional<CLanguagePreference> CLanguagePreference::Parse(const std::string& setting)
{
  if (setting.empty() || Names(setting, languageSettingDefault))
    return CLanguagePreference{Kind::FollowUI, {}};

  if (Names(setting, languageSettingOriginal))
    return CLanguagePreference{Kind::Original, {}};

  if (const auto tag = CLanguageTag::TryParse(setting); tag.has_value())
    return CLanguagePreference{Kind::Language, *tag};

  return std::nullopt;
}

std::optional<CLanguagePreference> CLanguagePreference::ForAudio(const std::string& setting)
{
  if (Names(setting, audioLanguageSettingMediaDefault))
    return CLanguagePreference{Kind::MediaDefault, {}};

  return Parse(setting);
}

std::optional<CLanguagePreference> CLanguagePreference::ForSubtitles(const std::string& setting)
{
  if (Names(setting, subtitleLanguageSettingNone))
    return CLanguagePreference{Kind::Off, {}};

  if (Names(setting, subtitleLanguageSettingForcedOnly))
    return CLanguagePreference{Kind::ForcedOnly, {}};

  return Parse(setting);
}

void CLanguage::AddSortToken(Tokens& tokens, const std::string& word, std::string_view separators)
{
  if (separators.empty())
  {
    tokens.insert(word);
    return;
  }

  for (const char separator : separators)
    tokens.insert(word + separator);
}

CLanguage& CLanguage::GetInstance()
{
  static CLanguage language;
  return language;
}

CLanguageTag CLanguage::Audio() const
{
  return m_audio.GetLanguage().IsUndetermined() ? m_ui : m_audio.GetLanguage();
}

CLanguageTag CLanguage::Subtitle(bool fallbackToUI /* = true */) const
{
  if (!m_subtitle.GetLanguage().IsUndetermined())
    return m_subtitle.GetLanguage();

  if (!m_audio.GetLanguage().IsUndetermined())
    return m_audio.GetLanguage();

  return fallbackToUI ? m_ui : CLanguageTag{};
}

bool CLanguage::SetAudio(const std::string& setting)
{
  return SetPreference(m_audio, CLanguagePreference::ForAudio(setting), setting);
}

bool CLanguage::SetSubtitle(const std::string& setting)
{
  return SetPreference(m_subtitle, CLanguagePreference::ForSubtitles(setting), setting);
}

void CLanguage::SetPack(const LanguageResourcePtr& pack)
{
  m_pack = pack;
  m_ui = m_pack ? m_pack->GetLanguage() : CLanguageTag::English();
  MergeSortTokens();
}

void CLanguage::DeclareSortTokens(Tokens tokens)
{
  m_declaredTokens = std::move(tokens);
  MergeSortTokens();
}

void CLanguage::MergeSortTokens()
{
  m_sortTokens = m_pack ? m_pack->GetSortTokens() : Tokens{};
  m_sortTokens.insert(m_declaredTokens.begin(), m_declaredTokens.end());
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

  // The English name is the one the pack declares for itself, which is the one a user has seen
  std::string named{notation == CLanguageTag::ENGLISH_NAME ? language.PackName() : ui.In(notation)};

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
