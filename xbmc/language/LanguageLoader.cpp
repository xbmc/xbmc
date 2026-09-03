/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/LanguageLoader.h"

#include "DatabaseManager.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/LanguageResource.h"
#include "addons/addoninfo/AddonType.h"
#include "language/LangInfo.h"
#include "language/Language.h"
#include "language/i18n/LanguageTable.h"
#include "messaging/ApplicationMessenger.h"
#include "pvr/PVRManager.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "weather/WeatherManager.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <set>
#include <string_view>

using namespace KODI::LANGUAGE;
using namespace KODI::LANGUAGE::I18N;

namespace
{
struct SpecialLanguageSetting
{
  std::string_view m_code;
  int m_message;
};

// Elements sorted in order of appearance in the settings
constexpr auto specialAudioLangSettings = std::array{
    SpecialLanguageSetting{audioLanguageSettingMediaDefault, 307},
    SpecialLanguageSetting{languageSettingOriginal, 308},
    SpecialLanguageSetting{languageSettingDefault, 309},
};

// Elements in order of appearance in the settings
constexpr auto specialSubtitlesLangSettings = std::array{
    SpecialLanguageSetting{subtitleLanguageSettingNone, 231},
    SpecialLanguageSetting{subtitleLanguageSettingForcedOnly, 13207},
    SpecialLanguageSetting{languageSettingOriginal, 308},
    SpecialLanguageSetting{languageSettingDefault, 309},
};

// Elements in order of appearance in the settings
constexpr auto specialSubtitlesDownloadLangSettings = std::array{
    SpecialLanguageSetting{languageSettingOriginal, 308},
    SpecialLanguageSetting{languageSettingDefault, 309},
};

/*!
 * \brief The pack a language names, whatever it was named by.
 * \param[in] language The pack, by addon id or by the locale it is for.
 * \return The pack, or nullptr where nothing of that name is a language pack.
 */
LanguageResourcePtr FindPack(const std::string& language)
{
  ADDON::AddonPtr addon;
  CServiceBroker::GetAddonMgr().GetAddon(ADDON::CLanguageResource::GetAddonId(language), addon,
                                         ADDON::AddonType::RESOURCE_LANGUAGE,
                                         ADDON::OnlyEnabled::CHOICE_YES);

  // Anything that is not a language pack leaves nothing to load a language from
  return std::dynamic_pointer_cast<ADDON::CLanguageResource>(addon);
}

//! \brief Load the strings every installed addon ships for the language now in use.
void LoadAddonStrings()
{
  ADDON::VECADDONS addons;
  if (!CServiceBroker::GetAddonMgr().GetInstalledAddons(addons))
    return;

  auto& resources = CServiceBroker::GetResourcesComponent();
  const std::string locale = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
      CSettings::SETTING_LOCALE_LANGUAGE);

  std::ranges::for_each(
      addons,
      [&resources, &locale](const auto& addon)
      {
        const std::string path = URIUtils::AddFileToFolder(addon->Path(), "resources", "language/");
        resources.GetLocalizeStrings().LoadAddonStrings(path, locale, addon->ID());
      });
}
} // namespace

CLanguageLoader& CLanguageLoader::GetInstance()
{
  static CLanguageLoader loader;
  return loader;
}

void CLanguageLoader::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
    return;

  const auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return;

  const auto settings = settingsComponent->GetSettings();
  if (!settings)
    return;

  const std::string& settingId = setting->GetId();
  if (settingId == CSettings::SETTING_LOCALE_AUDIOLANGUAGE)
  {
    CLanguage::GetInstance().SetAudio(
        std::static_pointer_cast<const CSettingString>(setting)->GetValue());
  }
  else if (settingId == CSettings::SETTING_LOCALE_SUBTITLELANGUAGE)
  {
    CLanguage::GetInstance().SetSubtitle(
        std::static_pointer_cast<const CSettingString>(setting)->GetValue());
  }
  else if (settingId == CSettings::SETTING_LOCALE_LANGUAGE)
  {
    if (!Load(std::static_pointer_cast<const CSettingString>(setting)->GetValue()))
    {
      // Put the setting back to a language that does load
      const auto languageSetting = settings->GetSetting(CSettings::SETTING_LOCALE_LANGUAGE);
      if (!languageSetting)
      {
        CLog::Log(LOGERROR, "Failed to load setting for: {}", CSettings::SETTING_LOCALE_LANGUAGE);
        return;
      }

      std::static_pointer_cast<CSettingString>(languageSetting)->Reset();
    }
  }
}

bool CLanguageLoader::Resolve(std::string& language)
{
  auto& addonMgr = CServiceBroker::GetAddonMgr();
  ADDON::AddonPtr addon;

  // Find the chosen language add-on if it's enabled
  if (addonMgr.GetAddon(language, addon, ADDON::AddonType::RESOURCE_LANGUAGE,
                        ADDON::OnlyEnabled::CHOICE_YES))
  {
    return true;
  }

  if (addonMgr.IsAddonInstalled(language) &&
      (!addonMgr.IsAddonDisabled(language) || addonMgr.EnableAddon(language)))
  {
    return true;
  }

  CLog::LogF(LOGWARNING, "Could not find or enable language add-on '{}', loading default...",
             language);
  language = std::static_pointer_cast<const CSettingString>(
                 CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(
                     CSettings::SETTING_LOCALE_LANGUAGE))
                 ->GetDefault();

  if (!addonMgr.GetAddon(language, addon, ADDON::AddonType::RESOURCE_LANGUAGE,
                         ADDON::OnlyEnabled::CHOICE_NO))
  {
    CLog::LogF(LOGFATAL, "Could not find default language add-on '{}'", language);
    return false;
  }

  return true;
}

bool CLanguageLoader::Load(std::string language /* = "" */, bool reloadServices /* = true */)
{
  if (language.empty())
  {
    language = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
        CSettings::SETTING_LOCALE_LANGUAGE);
  }

  if (!Resolve(language))
    return false;

  const LanguageResourcePtr pack{FindPack(language)};
  if (pack == nullptr)
  {
    CLog::LogF(LOGFATAL, "Unknown language {}", language);
    return false;
  }

  CLanguage::GetInstance().SetPack(pack);

  // An addon may name a language Kodi's own tables do not
  std::map<std::string, std::string> addonLanguages;
  GetAddonsLanguageCodes(addonLanguages);
  CLanguageTable::GetInstance().DeclareNames(addonLanguages);

  CLog::Log(LOGINFO, "CLanguageLoader: loading {} language information...", language);
  if (!g_langInfo.Load(GetLanguageInfoPath(language)))
  {
    CLog::LogF(LOGFATAL, "Failed to load {} language information", language);
    return false;
  }

  g_charsetConverter.reinitCharsetsFromSettings();

  CLog::Log(LOGINFO, "CLanguageLoader: loading {} language strings...", language);
  if (!CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Load(GetLanguagePath(),
                                                                         language))
  {
    CLog::LogF(LOGFATAL, "Failed to load {} language strings", language);
    return false;
  }

  LoadAddonStrings();

  if (reloadServices)
  {
    // also tell our weather and skin to reload as these are localized
    CServiceBroker::GetWeatherManager().Refresh();
    CServiceBroker::GetPVRManager().LocalizationChanged();
    CServiceBroker::GetDatabaseManager().LocalizationChanged();
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr,
                                               "ReloadSkin");
  }

  return true;
}

std::string CLanguageLoader::GetLanguagePath(const std::string& language)
{
  if (language.empty())
    return "";

  const std::string addonId = ADDON::CLanguageResource::GetAddonId(language);

  std::string path = URIUtils::AddFileToFolder(GetLanguagePath(), addonId);
  URIUtils::AddSlashAtEnd(path);

  return path;
}

std::string CLanguageLoader::GetLanguageInfoPath(const std::string& language)
{
  if (language.empty())
    return "";

  return URIUtils::AddFileToFolder(GetLanguagePath(language), "langinfo.xml");
}

void CLanguageLoader::GetAddonsLanguageCodes(std::map<std::string, std::string>& languages)
{
  ADDON::VECADDONS addons;
  CServiceBroker::GetAddonMgr().GetAddons(addons, ADDON::AddonType::RESOURCE_LANGUAGE);
  std::ranges::transform(addons, std::inserter(languages, languages.end()),
                         [](const auto& addon)
                         {
                           const LanguageResourcePtr langAddon =
                               std::dynamic_pointer_cast<ADDON::CLanguageResource>(addon);
                           return std::pair{langAddon->GetLanguage().ToString(), addon->Name()};
                         });
}

void CLanguageLoader::SettingOptionsISO6391LanguagesFiller(
    const std::shared_ptr<const CSetting>& /*setting*/,
    std::vector<StringSettingOption>& list,
    std::string& /*current*/)
{
  const std::vector<std::string> languages = GetLanguageNames(LanguageList::DEFAULT);

  std::ranges::transform(languages, std::back_inserter(list), [](const auto& language)
                         { return StringSettingOption{language, language}; });
}

void CLanguageLoader::SettingOptionsAudioStreamLanguagesFiller(
    const std::shared_ptr<const CSetting>& /*setting*/,
    std::vector<StringSettingOption>& list,
    std::string& /*current*/)
{
  for (const auto& special : specialAudioLangSettings)
  {
    list.emplace_back(
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(special.m_message),
        std::string{special.m_code});
  }

  AddLanguages(list);
}

void CLanguageLoader::SettingOptionsSubtitleStreamLanguagesFiller(
    const std::shared_ptr<const CSetting>& /*setting*/,
    std::vector<StringSettingOption>& list,
    std::string& /*current*/)
{
  for (const auto& special : specialSubtitlesLangSettings)
  {
    list.emplace_back(
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(special.m_message),
        std::string{special.m_code});
  }

  AddLanguages(list);
}

void CLanguageLoader::SettingOptionsSubtitleDownloadlanguagesFiller(
    const std::shared_ptr<const CSetting>& /*setting*/,
    std::vector<StringSettingOption>& list,
    std::string& /*current*/)
{
  for (const auto& special : specialSubtitlesDownloadLangSettings)
  {
    list.emplace_back(
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(special.m_message),
        std::string{special.m_code});
  }

  AddLanguages(list);
}

void CLanguageLoader::AddLanguages(std::vector<StringSettingOption>& list)
{
  const std::vector<std::string> languages = GetLanguageNames(LanguageList::INCLUDE_ADDONS);

  std::ranges::transform(languages, std::back_inserter(list), [](const auto& language)
                         { return StringSettingOption{language, language}; });
}

std::vector<std::string> CLanguageLoader::GetLanguageNames(LanguageList list)
{
  std::map<std::string, std::string> languages;
  CLanguageTable::GetInstance().List(languages);

  if (list == LanguageList::INCLUDE_ADDONS)
    GetAddonsLanguageCodes(languages);

  std::set<std::string, sortstringbyname> names;
  std::ranges::transform(languages, std::inserter(names, names.end()),
                         [](const auto& language) { return language.second; });

  return {names.begin(), names.end()};
}
