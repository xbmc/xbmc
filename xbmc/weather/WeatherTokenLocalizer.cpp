/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WeatherTokenLocalizer.h"

#include "LangInfo.h"
#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/POUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <string>
#include <vector>

namespace
{
constexpr unsigned int LOCALIZED_TOKEN_FIRSTID = 370;
constexpr unsigned int LOCALIZED_TOKEN_LASTID = 395;
constexpr unsigned int LOCALIZED_TOKEN_FIRSTID2 = 1350;
constexpr unsigned int LOCALIZED_TOKEN_LASTID2 = 1449;
constexpr unsigned int LOCALIZED_TOKEN_FIRSTID3 = 11;
constexpr unsigned int LOCALIZED_TOKEN_LASTID3 = 17;
constexpr unsigned int LOCALIZED_TOKEN_FIRSTID4 = 71;
constexpr unsigned int LOCALIZED_TOKEN_LASTID4 = 97;

} // unnamed namespace

std::string CWeatherTokenLocalizer::LocalizeOverviewToken(const std::string& token)
{
  if (m_localizedTokens.empty())
    LoadLocalizedTokens();

  std::string locStr;
  if (!token.empty())
  {
    const auto it{m_localizedTokens.find(token)};
    if (it != m_localizedTokens.cend())
      locStr = g_localizeStrings.Get(it->second);
  }

  if (locStr.empty())
    locStr = token; // if not found, use original string as fallback

  return locStr;
}

std::string CWeatherTokenLocalizer::LocalizeOverview(const std::string& str)
{
  std::vector<std::string> words{StringUtils::Split(str, " ")};
  for (std::string& word : words)
    word = LocalizeOverviewToken(word);

  return StringUtils::Join(words, " ");
}

void CWeatherTokenLocalizer::LoadLocalizedTokens()
{
  // We load the english strings in to get our tokens
  std::string language{LANGUAGE_DEFAULT};
  const std::shared_ptr<const CSettingString> languageSetting{
      std::static_pointer_cast<const CSettingString>(
          CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(
              CSettings::SETTING_LOCALE_LANGUAGE))};
  if (languageSetting)
    language = languageSetting->GetDefault();

  // Load the strings.po file
  const std::string fileName{
      URIUtils::AddFileToFolder(CLangInfo::GetLanguagePath(language), "strings.po")};
  CPODocument poDoc;
  if (!poDoc.LoadFile(fileName))
  {
    CLog::LogF(LOGERROR, "Unable to load po file '{}'", fileName);
    return;
  }

  int counter{0};
  while (poDoc.GetNextEntry())
  {
    if (poDoc.GetEntryType() != ID_FOUND)
      continue;

    const uint32_t id{poDoc.GetEntryID()};
    poDoc.ParseEntry(ISSOURCELANG);

    if (id > LOCALIZED_TOKEN_LASTID2)
      break;

    if ((LOCALIZED_TOKEN_FIRSTID <= id && id <= LOCALIZED_TOKEN_LASTID) ||
        (LOCALIZED_TOKEN_FIRSTID2 <= id && id <= LOCALIZED_TOKEN_LASTID2) ||
        (LOCALIZED_TOKEN_FIRSTID3 <= id && id <= LOCALIZED_TOKEN_LASTID3) ||
        (LOCALIZED_TOKEN_FIRSTID4 <= id && id <= LOCALIZED_TOKEN_LASTID4))
    {
      if (!poDoc.GetMsgid().empty())
      {
        m_localizedTokens.try_emplace(poDoc.GetMsgid(), id);
        counter++;
      }
    }
  }

  CLog::LogF(LOGDEBUG, "Loaded {} weather tokens", counter);
}
