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
#include "addons/AddonEvents.h"
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
//! An addon may name a language Kodi's own tables do not
void DeclareAddonLanguageNames()
{
  std::map<std::string, std::string> addonLanguages;
  CLanguageLoader::GetAddonsLanguageCodes(addonLanguages);
  CLanguageTable::GetInstance().DeclareNames(addonLanguages);
}

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
 * \brief Find the pack to use, enabling it or falling back to the default one.
 * \param[in,out] language The pack asked for, by addon id or locale; rewritten to the addon
 *                id of the pack found.
 * \return The pack, or nullptr when not even the default one is there.
 */
LanguageResourcePtr Resolve(std::string& language)
{
  auto& addonMgr = CServiceBroker::GetAddonMgr();
  ADDON::AddonPtr addon;

  const std::string id{ADDON::CLanguageResource::GetAddonId(language)};
  if (!addonMgr.GetAddon(id, addon, ADDON::AddonType::RESOURCE_LANGUAGE,
                         ADDON::OnlyEnabled::CHOICE_YES) &&
      addonMgr.IsAddonInstalled(id) && addonMgr.EnableAddon(id))
  {
    addonMgr.GetAddon(id, addon, ADDON::AddonType::RESOURCE_LANGUAGE,
                      ADDON::OnlyEnabled::CHOICE_YES);
  }

  if (addon)
  {
    language = id;
    return std::dynamic_pointer_cast<ADDON::CLanguageResource>(addon);
  }

  CLog::LogF(LOGWARNING, "language pack '{}' not found, using default", id);
  language = std::static_pointer_cast<const CSettingString>(
                 CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(
                     CSettings::SETTING_LOCALE_LANGUAGE))
                 ->GetDefault();

  if (!addonMgr.GetAddon(language, addon, ADDON::AddonType::RESOURCE_LANGUAGE,
                         ADDON::OnlyEnabled::CHOICE_NO))
  {
    CLog::LogF(LOGFATAL, "default language pack '{}' not found", language);
    return nullptr;
  }

  return std::dynamic_pointer_cast<ADDON::CLanguageResource>(addon);
}

//! \brief Load the strings every installed addon ships for the language pack \p locale.
void LoadAddonStrings(const std::string& locale)
{
  ADDON::VECADDONS addons;
  if (!CServiceBroker::GetAddonMgr().GetInstalledAddons(addons))
    return;

  auto& resources = CServiceBroker::GetResourcesComponent();

  std::ranges::for_each(
      addons,
      [&resources, &locale](const auto& addon)
      {
        const std::string path = URIUtils::AddFileToFolder(addon->Path(), "resources", "language/");
        resources.GetLocalizeStrings().LoadAddonStrings(path, locale, addon->ID());
      });
}

/*!
 * \brief Hand a stream language setting to CLanguage, putting a value the settings list never
 *        offered back to its default.
 * \param[in] settingId The audio or the subtitle language setting.
 */
void ApplyStreamLanguage(const std::string& settingId)
{
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const std::string value = settings->GetString(settingId);
  CLanguage& language = CLanguage::GetInstance();
  const bool known = settingId == CSettings::SETTING_LOCALE_AUDIOLANGUAGE
                         ? language.SetAudio(value)
                         : language.SetSubtitle(value);
  if (!known)
    settings->GetSetting(settingId)->Reset();
}

/*!
 * \brief The English names of every language Kodi knows of and every language the installed
 * language addons name, sorted for display.
 * \return The names, without duplicates.
 */
std::vector<std::string> GetLanguageNames()
{
  std::map<std::string, std::string> languages;
  CLanguageTable::GetInstance().List(languages);

  CLanguageLoader::GetAddonsLanguageCodes(languages);

  std::set<std::string, sortstringbyname> names;
  std::ranges::transform(languages, std::inserter(names, names.end()),
                         [](const auto& language) { return language.second; });

  return {names.begin(), names.end()};
}

//! \brief A language list: the setting's own choices, then every language by name.
template<std::size_t N>
void FillLanguageOptions(const std::array<SpecialLanguageSetting, N>& specials,
                         std::vector<StringSettingOption>& list)
{
  for (const auto& special : specials)
  {
    list.emplace_back(
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(special.m_message),
        std::string{special.m_code});
  }

  const std::vector<std::string> languages = GetLanguageNames();
  std::ranges::transform(languages, std::back_inserter(list), [](const auto& language)
                         { return StringSettingOption{language, language}; });
}
} // namespace

CLanguageLoader& CLanguageLoader::GetInstance()
{
  static CLanguageLoader loader;
  return loader;
}

void CLanguageLoader::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  const std::string& settingId = setting->GetId();
  if (settingId == CSettings::SETTING_LOCALE_AUDIOLANGUAGE ||
      settingId == CSettings::SETTING_LOCALE_SUBTITLELANGUAGE)
    ApplyStreamLanguage(settingId);
  else if (settingId == CSettings::SETTING_LOCALE_LANGUAGE)
  {
    // Put the setting back to a language that does load
    if (!Load(std::static_pointer_cast<const CSettingString>(setting)->GetValue()))
      settings->GetSetting(settingId)->Reset();
  }
}

bool CLanguageLoader::Load(std::string language /* = "" */, bool reloadServices /* = true */)
{
  if (language.empty())
  {
    language = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
        CSettings::SETTING_LOCALE_LANGUAGE);
  }

  const LanguageResourcePtr pack{Resolve(language)};
  if (pack == nullptr)
    return false;

  CLanguage::GetInstance().SetPack(pack);

  DeclareAddonLanguageNames();

  CLog::Log(LOGINFO, "CLanguageLoader: loading {} language information...", language);
  if (!CServiceBroker::GetResourcesComponent().GetLangInfo().Load(GetLanguageInfoPath(language)))
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

  LoadAddonStrings(language);

  ApplyStreamLanguage(CSettings::SETTING_LOCALE_AUDIOLANGUAGE);
  ApplyStreamLanguage(CSettings::SETTING_LOCALE_SUBTITLELANGUAGE);

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

void CLanguageLoader::OnAddonEvent(const ADDON::AddonEvent& event)
{
  // Installing or updating an addon reloads it, which is announced as one of these two
  if ((typeid(event) == typeid(ADDON::AddonEvents::Enabled) ||
       typeid(event) == typeid(ADDON::AddonEvents::ReInstalled)) &&
      CServiceBroker::GetAddonMgr().HasType(event.addonId, ADDON::AddonType::RESOURCE_LANGUAGE))
  {
    DeclareAddonLanguageNames();
  }
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

void CLanguageLoader::SettingOptionsAudioStreamLanguagesFiller(
    const std::shared_ptr<const CSetting>& /*setting*/,
    std::vector<StringSettingOption>& list,
    std::string& /*current*/)
{
  FillLanguageOptions(specialAudioLangSettings, list);
}

void CLanguageLoader::SettingOptionsSubtitleStreamLanguagesFiller(
    const std::shared_ptr<const CSetting>& /*setting*/,
    std::vector<StringSettingOption>& list,
    std::string& /*current*/)
{
  FillLanguageOptions(specialSubtitlesLangSettings, list);
}

void CLanguageLoader::SettingOptionsSubtitleDownloadlanguagesFiller(
    const std::shared_ptr<const CSetting>& /*setting*/,
    std::vector<StringSettingOption>& list,
    std::string& /*current*/)
{
  FillLanguageOptions(specialSubtitlesDownloadLangSettings, list);
}
